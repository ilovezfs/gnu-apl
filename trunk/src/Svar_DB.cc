/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2013  Dr. Jürgen Sauermann

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <errno.h>
#include <fcntl.h>           /* For O_* constants */
#include <netinet/tcp.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#include "config.h"   // for HAVE_ macros
#ifdef HAVE_LINUX_UN_H
#include <linux/un.h>
#endif

#include <iomanip>

#include "Backtrace.hh"
#include "Logging.hh"
#include "main.hh"
#include "Svar_DB.hh"
#include "Svar_signals.hh"
#include "UdpSocket.hh"
#include "UserPreferences.hh"

extern ostream & get_CERR();

uint16_t Svar_DB::APserver_port = Default_APserver_tcp_port;

TCP_socket Svar_DB::DB_tcp = NO_TCP_SOCKET;

Svar_record Svar_record_P::cache;

//=============================================================================
void
Svar_DB::connect_to_APserver(const char * bin_dir, const char * prog,
                                      bool logit)
{
const char * server_sockname = Svar_record::get_APserver_unix_socket_name();
char peer[100];

   // we use AF_UNIX sockets if the platform supports it and unix_socket_name
   // is provided. Otherwise fall back to TCP.
   //
#if HAVE_LINUX_UN_H
   if (server_sockname)
      {
        logit && get_CERR() << prog
                            << ": Using AF_UNIX socket towards APserver..."
                            << endl;
        DB_tcp = (TCP_socket)(socket(AF_UNIX, SOCK_STREAM, 0));
        if (DB_tcp == NO_TCP_SOCKET)
           {
             get_CERR() << prog
                        << ": socket(AF_UNIX, SOCK_STREAM, 0) failed at "
                        << LOC << endl;
             return;
           }

        snprintf(peer, sizeof(peer), "%s", server_sockname);
      }
   else // use TCP
#endif
      {
        server_sockname = 0;
        logit && get_CERR() << "Using TCP socket towards APserver..."
                            << endl;
        DB_tcp = (TCP_socket)(socket(AF_INET, SOCK_STREAM, 0));
        if (DB_tcp == NO_TCP_SOCKET)
           {
             get_CERR() << "*** socket(AF_INET, SOCK_STREAM, 0) failed at "
                        << LOC << endl;
             return;
           }

        // disable nagle
        {
          const int ndelay = 1;
          setsockopt(DB_tcp, 6, TCP_NODELAY, &ndelay, sizeof(int));
        }

        // bind local port to 127.0.0.1
        //
        sockaddr_in local;
        memset(&local, 0, sizeof(sockaddr_in));
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(0x7F000001);

        if (::bind(DB_tcp, (const sockaddr *)&local, sizeof(sockaddr_in)))
           {
             get_CERR() << "bind(127.0.0.1) failed:" << strerror(errno) << endl;
             return;
           }

        snprintf(peer, sizeof(peer), "127.0.0.1 TCP port %d", APserver_port);
      }

   // We try to connect to the TCP port number APnnn_port (of the APserver)
   // on localhost. If that fails then no APserver is running; we fork one
   // and try again.
   //
   for (bool retry = false; ; retry = true)
       {
        if (server_sockname)
           {
#if HAVE_LINUX_UN_H
             sockaddr_un remote;
             memset(&remote, 0, sizeof(sockaddr_un));
             remote.sun_family = AF_UNIX;
             strcpy(remote.sun_path + ABSTRACT_OFFSET, server_sockname);

             if (::connect(DB_tcp, (sockaddr *)&remote,
                           sizeof(remote)) == 0)   break;   // success
#endif
           }
        else   // TCP
           {
             sockaddr_in remote;
             memset(&remote, 0, sizeof(sockaddr_in));
             remote.sin_family = AF_INET;
             remote.sin_port = htons(APserver_port);
             remote.sin_addr.s_addr = htonl(0x7F000001);

             if (::connect(DB_tcp, (sockaddr *)&remote,
                           sizeof(remote)) == 0)   break;   // success
           }

         if (logit)
            {
              get_CERR() << "connecting to " << peer << endl;

              if (retry)   get_CERR() <<
                 "    (this is supposed to succeed.)" << endl;
              else         get_CERR() <<
                 "    (this is expected to fail, unless APserver"
                 " was started manually)" << endl;
            }

         if (retry)
            {
              get_CERR() << "::connect() to existing APserver failed: "
                   << strerror(errno) << endl;

              ::close(DB_tcp);
              DB_tcp = NO_TCP_SOCKET;

              return;
           }

         // fork an APserver
         //
         logit && get_CERR() << "forking new APserver listening on "
                             << peer << endl;

         const pid_t pid = fork();
         if (pid)
            {
              // give child a little time to start up...
              //
              usleep(20000);
            }
         else   // child: run as APserver
            {
              ::close(DB_tcp);
              DB_tcp = NO_TCP_SOCKET;

              char arg0[FILENAME_MAX + 20];
              snprintf(arg0, sizeof(arg0), "%s/APserver", bin_dir);
              char arg1[20];
              char arg2[100];
              if (server_sockname)
                 {
                    strcpy(arg1, "--path");
                    strcpy(arg2, server_sockname);
                 }
              else
                 {
                    strcpy(arg1, "--port");
                    snprintf(arg2, sizeof(arg2), "%d", APserver_port);
                 }
              char * argv[] = { arg0, arg1, arg2, 0 };
              char * envp[] = { 0 };
              execve(arg0, argv, envp);

              // execve() failed, try APs subdir...
              //
              if (logit)   get_CERR() << "execve(" << arg0;
              snprintf(arg0, sizeof(arg0), "%s/APs/APserver", bin_dir);
              if (logit)   get_CERR() << ") failed ("<< strerror(errno)
                                      << "), trying execve(" << arg0 << ")"
                                      << endl;

              execve(arg0, argv, envp);

              get_CERR() << "execve(" << arg0 
                         << ") also failed ("<< strerror(errno)
                         << ")" << endl;
              exit(99);
            }
       }

   // at this point DB_tcp is != NO_TCP_SOCKET and connected.
   //
   usleep(20000);
   logit && get_CERR() << "connected to APserver, DB_tcp is " << DB_tcp << endl;
}
//-----------------------------------------------------------------------------
void
Svar_DB::DB_tcp_error(const char * op, int got, int expected)
{
   get_CERR() << "⋆⋆⋆ " << op << " failed: got " << got << " when expecting "
        << expected << " (" << strerror(errno) << ")" << endl;

   ::close(DB_tcp);
   DB_tcp = NO_TCP_SOCKET;
}
//=============================================================================

Svar_record * Svar_record_P::offered_svar_p = 0;

Svar_record_P::Svar_record_P(bool ronly, SV_key key)
   : read_only(ronly)
{
   if (!Svar_DB::APserver_available())   return;

const int sock = Svar_DB::get_DB_tcp();
   offered_svar_p = 0;

   if (ronly)
      {
        READ_SVAR_RECORD_c request(sock, key);
      }
   else
      {
        UPDATE_SVAR_RECORD_c request(sock, key);
      }

char * del = 0;
char buffer[2*MAX_SIGNAL_CLASS_SIZE + sizeof(Svar_record)];
ostream * log = (LOG_startup || LOG_Svar_DB_signals) ? & cerr : 0;
Signal_base * response = Signal_base::recv_TCP(sock, buffer, sizeof(buffer),
                                               del, log);
   if (response)
      {
        memcpy(&cache, response->get__SVAR_RECORD_IS__record().data(),
               sizeof(Svar_record));
        offered_svar_p = &cache;
      }
   else   get_CERR() << "Svar_record_P() failed at " << LOC << endl;
   if (del)   delete del;
}
//-----------------------------------------------------------------------------
Svar_record_P::~Svar_record_P()
{
   if (read_only)                        return;
   if (!Svar_DB::APserver_available())   return;

std::string data((const char *)&cache, sizeof(cache));
SVAR_RECORD_IS_c updated(Svar_DB::get_DB_tcp(), data);
}
//=============================================================================
void
Svar_DB::init(const char * bin_dir, const char * prog,
              bool logit, bool do_svars)
{
   if (!do_svars)   // shared variables disabled
      {
        if (logit)
           get_CERR() << "Not opening shared memory because command "
                         "line option --noSV (or similar) was given." << endl;
        return;
      }

   connect_to_APserver(bin_dir, prog, logit);
   if (APserver_available())
      {
        if (logit)   get_CERR() << "using Svar_DB on APserver!" << endl;
      }
}
//-----------------------------------------------------------------------------
#if 1
SV_key
Svar_DB::match_or_make(const uint32_t * UCS_varname, const AP_num3 & to,
                       const Svar_partner & from)
{
const TCP_socket tcp = get_Svar_DB_tcp(__FUNCTION__);
   if (tcp == NO_TCP_SOCKET)   return 0;


string vname((const char *)UCS_varname, MAX_SVAR_NAMELEN*sizeof(uint32_t));

MATCH_OR_MAKE_c request(tcp, vname,
                             to.proc,      to.parent,      to.grand,
                             from.id.proc, from.id.parent, from.id.grand,
                             from.pid,     from.port);

char * del = 0;
char buffer[2*MAX_SIGNAL_CLASS_SIZE];
Signal_base * response = Signal_base::recv_TCP(tcp, buffer, sizeof(buffer),
                                               del, 0);

   if (response)   return response->get__MATCH_OR_MAKE_RESULT__key();
   else            return 0;
}
#endif
//-----------------------------------------------------------------------------
SV_key
Svar_DB::get_events(Svar_event & events, AP_num3 id)
{
const TCP_socket tcp = get_Svar_DB_tcp(__FUNCTION__);
   if (tcp == NO_TCP_SOCKET)
      {
        events = SVE_NO_EVENTS;
        return 0;
      }

GET_EVENTS_c request(tcp, id.proc, id.parent, id.grand);

char * del = 0;
char buffer[2*MAX_SIGNAL_CLASS_SIZE];
Signal_base * response = Signal_base::recv_TCP(tcp, buffer, sizeof(buffer),
                                               del, 0);

   if (response)
      {
        events = (Svar_event)response->get__EVENTS_ARE__events();
        return response->get__EVENTS_ARE__key();
      }
   else
      {
        events = SVE_NO_EVENTS;
        return 0;
      }
}
//-----------------------------------------------------------------------------
Svar_event
Svar_DB::clear_all_events(AP_num3 id)
{
const TCP_socket tcp = get_Svar_DB_tcp(__FUNCTION__);
   if (tcp == NO_TCP_SOCKET)
      {
        return SVE_NO_EVENTS;
      }

CLEAR_ALL_EVENTS_c request(tcp, id.proc, id.parent, id.grand);

char * del = 0;
char buffer[2*MAX_SIGNAL_CLASS_SIZE];
Signal_base * response = Signal_base::recv_TCP(tcp, buffer, sizeof(buffer),
                                               del, 0);

   if (response)
      {
        return (Svar_event)(response->get__EVENTS_ARE__events());
      }
   else
      {
        return SVE_NO_EVENTS;
      }
}
//-----------------------------------------------------------------------------
void
Svar_DB::add_event(SV_key key, AP_num3 id, Svar_event event)
{
const TCP_socket tcp = get_Svar_DB_tcp(__FUNCTION__);
   if (tcp == NO_TCP_SOCKET)   return;

ADD_EVENT_c request(tcp, key, id.proc, id.parent, id.grand, event);
}
//-----------------------------------------------------------------------------
AP_num3
Svar_DB::find_offering_id(SV_key key)
{
AP_num3 offering_id;
const TCP_socket tcp = get_Svar_DB_tcp(__FUNCTION__);
   if (tcp == NO_TCP_SOCKET)   return offering_id;

FIND_OFFERING_ID_c request(tcp, key);

char * del = 0;
char buffer[2*MAX_SIGNAL_CLASS_SIZE];
Signal_base * response = Signal_base::recv_TCP(tcp, buffer, sizeof(buffer),
                                               del, 0);

   if (response)
      {
         offering_id.proc   = (AP_num)(response->get__OFFERING_ID_IS__proc());
         offering_id.parent = (AP_num)(response->get__OFFERING_ID_IS__parent());
         offering_id.grand  = (AP_num)(response->get__OFFERING_ID_IS__grand());
      }
       
   return offering_id;
}
//-----------------------------------------------------------------------------
void
Svar_DB::get_offering_processors(AP_num to_proc, vector<AP_num> & processors)
{
const TCP_socket tcp = get_Svar_DB_tcp(__FUNCTION__);
   if (tcp == NO_TCP_SOCKET)   return;

GET_OFFERING_PROCS_c request(tcp, to_proc);

char * del = 0;
char buffer[2*MAX_SIGNAL_CLASS_SIZE];
Signal_base * response = Signal_base::recv_TCP(tcp, buffer, sizeof(buffer),
                                               del, 0);

   if (response)
      {
        const string & op = response->get__OFFERING_PROCS_ARE__offering_procs();
        const AP_num * procs = (const AP_num *)(op.data());
        const size_t count = op.size() / sizeof(AP_num);

        loop(c, count)   processors.push_back(*procs++);
      }
}
//-----------------------------------------------------------------------------
void
Svar_DB::get_offered_variables(AP_num to_proc, AP_num from_proc,
                               vector<uint32_t> & varnames)
{
const TCP_socket tcp = get_Svar_DB_tcp(__FUNCTION__);
   if (tcp == NO_TCP_SOCKET)   return;

GET_OFFERED_VARS_c request(tcp, to_proc, from_proc);

char * del = 0;
char buffer[2*MAX_SIGNAL_CLASS_SIZE];
Signal_base * response = Signal_base::recv_TCP(tcp, buffer, sizeof(buffer),
                                               del, 0);

   if (response)
      {
        const string & ov = response->get__OFFERED_VARS_ARE__offered_vars();
        const uint32_t * names = (const uint32_t *)(ov.data());
        const size_t count = ov.size() / sizeof(uint32_t);

        loop(c, count)   varnames.push_back(*names++);
      }
}
//-----------------------------------------------------------------------------
bool
Svar_DB::is_registered_id(const AP_num3 & id)
{
const TCP_socket tcp = get_Svar_DB_tcp(__FUNCTION__);
   if (tcp == NO_TCP_SOCKET)   return 0;

IS_REGISTERED_ID_c request(tcp, id.proc, id.parent, id.grand);

char * del = 0;
char buffer[2*MAX_SIGNAL_CLASS_SIZE];
Signal_base * response = Signal_base::recv_TCP(tcp, buffer, sizeof(buffer),
                                               del, 0);

   if (response)   return response->get__YES_NO__yes();
   return false;
}
//-----------------------------------------------------------------------------
uint16_t
Svar_DB::get_udp_port(AP_num proc, AP_num parent)
{
const TCP_socket tcp = get_Svar_DB_tcp(__FUNCTION__);
   if (tcp == NO_TCP_SOCKET)   return 0;

GET_UDP_PORT_c request(tcp, proc, parent);

char * del = 0;
char buffer[2*MAX_SIGNAL_CLASS_SIZE];
Signal_base * response = Signal_base::recv_TCP(tcp, buffer, sizeof(buffer),
                                               del, 0);

   if (response)   return response->get__UDP_PORT_IS__port();
   return 0;
}
//-----------------------------------------------------------------------------
TCP_socket
Svar_DB::get_Svar_DB_tcp(const char * calling_function)
{
const TCP_socket tcp = Svar_DB::get_DB_tcp();
   if (tcp == NO_TCP_SOCKET)
      {
        get_CERR() << "Svar_DB not connected in Svar_DB::"
             << calling_function << "()" << endl;
      }

   return tcp;
}
//-----------------------------------------------------------------------------
SV_key
Svar_DB::find_pairing_key(SV_key key)
{
const TCP_socket tcp = Svar_DB::get_DB_tcp();
   if (tcp == NO_TCP_SOCKET)   return 0;

FIND_PAIRING_KEY_c request(tcp, key);

char * del = 0;
char buffer[2*MAX_SIGNAL_CLASS_SIZE];
Signal_base * response = Signal_base::recv_TCP(tcp, buffer, sizeof(buffer),
                                               del, 0);

   if (response)   return response->get__PAIRING_KEY_IS__pairing_key();
   return 0;
}
//-----------------------------------------------------------------------------
void
Svar_DB::print(ostream & out)
{
const TCP_socket tcp = Svar_DB::get_DB_tcp();
   if (tcp == NO_TCP_SOCKET)   return;

PRINT_SVAR_DB_c request(tcp);

char * del = 0;
char buffer[2*MAX_SIGNAL_CLASS_SIZE + 4000];
Signal_base * response = Signal_base::recv_TCP(tcp, buffer, sizeof(buffer),
                                               del, 0);

   if (response)   out << response->get__SVAR_DB_PRINTED__printout();
}
//-----------------------------------------------------------------------------

