/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2014  Dr. Jürgen Sauermann

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
#include <limits.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#include "config.h"   // for HAVE_ macros

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#include <iomanip>

#include "Backtrace.hh"
#include "Logging.hh"
#include "Svar_DB.hh"
#include "Svar_signals.hh"
#include "UserPreferences.hh"

extern ostream & get_CERR();

uint16_t Svar_DB::APserver_port = APSERVER_PORT;

TCP_socket Svar_DB::DB_tcp = NO_TCP_SOCKET;

Svar_record Svar_record_P::cache;

//-----------------------------------------------------------------------------
void
Svar_DB::start_APserver(const char * server_sockname,
                        const char * bin_dir, bool logit)
{
   // bin_dir is the directory where the apl interpreter binary lives.
   // The APserver then lives in:
   //
   // bin_dir/      if apl was built and installed, or
   // bin_dir/APs/  if apl was started from the src directory in the source tree
   //
   // set APserver_path to the case that applies.
   //
char APserver_path[PATH_MAX + 1];
   snprintf(APserver_path, PATH_MAX, "%s/APserver", bin_dir);
   APserver_path[PATH_MAX] = 0;
   if (access(APserver_path, X_OK) != 0)   // no APserver
      {
        logit && get_CERR() << "    Executable " << APserver_path
                 << " not found (this is OK when apl was started\n"
                    "    from the src directory): " << strerror(errno) << endl;

        snprintf(APserver_path, PATH_MAX, "%s/APs/APserver", bin_dir);
        if (access(APserver_path, X_OK) != 0)   // no APs/APserver either
           {
             get_CERR() << "Executable " << APserver_path << " not found.\n"
"This could means that 'apl' was not installed ('make install') or that it\n"
"was started in a non-standard way. The expected location of APserver is \n"
"either the same directory as the binary 'apl' or the subdirectory 'APs' of\n"
"that directory (the directory should also be in $PATH)." << endl;

             return;
           }
      }

   logit && get_CERR() << "Found " << APserver_path << endl;

char popen_args[PATH_MAX + 1];
   {
     if (server_sockname)
        snprintf(popen_args, PATH_MAX,
                 "%s --path %s --auto", APserver_path, server_sockname);
     else
        snprintf(popen_args, PATH_MAX,
                 "%s --port %u --auto", APserver_path, APserver_port);
   }

   logit && get_CERR() << "Starting " << popen_args << "..." << endl;

FILE * fp = popen(popen_args, "r");
   if (fp == 0)
      {
        get_CERR() << "popen(" << popen_args << " failed: " << strerror(errno)
             << endl;
        return;
      }

   for (int cc; (cc = getc(fp)) != EOF;)
       {
         logit && get_CERR() << (char)cc;
       }

   logit && get_CERR() << endl;

const int APserver_result = pclose(fp);
   if (APserver_result)
      {
         get_CERR() << "pclose(APserver) returned error:"
                    << strerror(errno) << endl;
      }

   return;
}
//-----------------------------------------------------------------------------
TCP_socket
Svar_DB::connect_to_APserver(const char * bin_dir, const char * prog,
                                      bool logit)
{
int sock = NO_TCP_SOCKET;
const char * server_sockname = APSERVER_PATH;
char peer[100];

   // we use AF_UNIX sockets if the platform supports it and unix_socket_name
   // is provided. Otherwise fall back to TCP.
   //
#if HAVE_SYS_UN_H && APSERVER_TRANSPORT != 0
      {
        logit && get_CERR() << prog
                            << ": Using AF_UNIX socket towards APserver..."
                            << endl;
        sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock == NO_TCP_SOCKET)
           {
             get_CERR() << prog
                        << ": socket(AF_UNIX, SOCK_STREAM, 0) failed at "
                        << LOC << endl;
             return NO_TCP_SOCKET;
           }

        snprintf(peer, sizeof(peer), "%s", server_sockname);
      }
#else // use TCP
      {
        server_sockname = 0;
        logit && get_CERR() << "Using TCP socket towards APserver..."
                            << endl;
        sock = (TCP_socket)(socket(AF_INET, SOCK_STREAM, 0));
        if (sock == NO_TCP_SOCKET)
           {
             get_CERR() << "*** socket(AF_INET, SOCK_STREAM, 0) failed at "
                        << LOC << endl;
             return NO_TCP_SOCKET;
           }

        // disable nagle
        {
          const int ndelay = 1;
          setsockopt(sock, 6, TCP_NODELAY, &ndelay, sizeof(int));
        }

        // bind local port to 127.0.0.1
        //
        sockaddr_in local;
        memset(&local, 0, sizeof(sockaddr_in));
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(0x7F000001);

        if (::bind(sock, (const sockaddr *)&local, sizeof(sockaddr_in)))
           {
             get_CERR() << "bind(127.0.0.1) failed:" << strerror(errno) << endl;
             ::close(sock);
             return NO_TCP_SOCKET;
           }

        snprintf(peer, sizeof(peer), "127.0.0.1 TCP port %d", APserver_port);
      }
#endif

   // We try to connect to the APserver. If that fails then no
   // APserver is running; we fork one and try again.
   //
   for (bool retry = false; ; retry = true)
       {
        if (server_sockname)
           {
#if HAVE_SYS_UN_H
             sockaddr_un remote;
             memset(&remote, 0, sizeof(sockaddr_un));
             remote.sun_family = AF_UNIX;
             strcpy(remote.sun_path + ABSTRACT_OFFSET, server_sockname);

             if (::connect(sock, (sockaddr *)&remote,
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

             if (::connect(sock, (sockaddr *)&remote,
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

         if (retry && bin_dir)
            {
              get_CERR() << "::connect() to existing APserver failed: "
                   << strerror(errno) << endl;

              ::close(sock);
              return NO_TCP_SOCKET;
           }

         // start an APserver
         //
         logit && get_CERR() << "starting a new APserver listening on "
                             << peer << endl;

         start_APserver(server_sockname, bin_dir, logit);
       }

   // at this point sock is != NO_TCP_SOCKET and connected.
   //
   usleep(20000);
   logit && get_CERR() << "connected to APserver, socket is " << sock << endl;

   return (TCP_socket)sock;
}
//-----------------------------------------------------------------------------
void
Svar_DB::DB_tcp_error(const char * op, int got, int expected)
{
   // op == 0 indicates close
   //
   if (op)
      {
        get_CERR() << "⋆⋆⋆ " << op << " failed: got " << got << " when expecting "
                   << expected << " (" << strerror(errno) << ")" << endl;
      }

   shutdown(DB_tcp, SHUT_RDWR);
}
//=============================================================================

Svar_record_P::Svar_record_P(SV_key key)
{
   if (!Svar_DB::APserver_available())   return;

const int sock = Svar_DB::get_DB_tcp();

   { READ_SVAR_RECORD_c request(sock, key); }

char * del = 0;
char buffer[2*MAX_SIGNAL_CLASS_SIZE + sizeof(Svar_record)];
ostream * log = (LOG_startup || LOG_Svar_DB_signals) ? & cerr : 0;
Signal_base * response = Signal_base::recv_TCP(sock, buffer, sizeof(buffer),
                                               del, log);
   if (response)
      {
        memcpy(&cache, response->get__SVAR_RECORD_IS__record().data(),
               sizeof(Svar_record));
        delete response;
      }
   else   get_CERR() << "Svar_record_P() failed at " << LOC << endl;
   if (del)   delete del;
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

   DB_tcp = connect_to_APserver(bin_dir, prog, logit);
   if (APserver_available())
      {
        if (logit)   get_CERR() << "using Svar_DB on APserver!" << endl;
      }
}
//-----------------------------------------------------------------------------
SV_key
Svar_DB::match_or_make(const uint32_t * UCS_varname, const AP_num3 & to,
                       const Svar_partner & from)
{
const TCP_socket tcp = get_Svar_DB_tcp(__FUNCTION__);
   if (tcp == NO_TCP_SOCKET)   return 0;


uint32_t vname1[MAX_SVAR_NAMELEN];
   memset(vname1, 0, sizeof(vname1));
   loop(v, MAX_SVAR_NAMELEN)
      {
        if (*UCS_varname)   vname1[v] = *UCS_varname++;
        else                break;
      }

string vname((const char *)vname1, MAX_SVAR_NAMELEN*sizeof(uint32_t));

MATCH_OR_MAKE_c request(tcp, vname,
                             to.proc,      to.parent,      to.grand,
                             from.id.proc, from.id.parent, from.id.grand);

char * del = 0;
char buffer[2*MAX_SIGNAL_CLASS_SIZE + 16];
Signal_base * response = Signal_base::recv_TCP(tcp, buffer, sizeof(buffer),
                                               del, 0);

   if (response)
      {
        const SV_key ret = response->get__MATCH_OR_MAKE_RESULT__key();
        delete response;
        return ret;
     }

   else            return 0;
}
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
char buffer[2*MAX_SIGNAL_CLASS_SIZE + 16];
Signal_base * response = Signal_base::recv_TCP(tcp, buffer, sizeof(buffer),
                                               del, 0);

   if (response)
      {
        events = (Svar_event)response->get__EVENTS_ARE__events();
        const SV_key ret = response->get__EVENTS_ARE__key();
        delete response;
        return ret;
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
char buffer[2*MAX_SIGNAL_CLASS_SIZE + 16];
Signal_base * response = Signal_base::recv_TCP(tcp, buffer, sizeof(buffer),
                                               del, 0);

   if (response)
      {
        const Svar_event ret = (Svar_event)response->get__EVENTS_ARE__events();
        delete response;
        return ret;
      }
   else
      {
        return SVE_NO_EVENTS;
      }
}
//-----------------------------------------------------------------------------
void
Svar_DB::set_control(SV_key key, Svar_Control ctl)
{
const TCP_socket tcp = get_Svar_DB_tcp(__FUNCTION__);
   if (tcp == NO_TCP_SOCKET)   return;

SET_CONTROL_c request(tcp, key, ctl);
}
//-----------------------------------------------------------------------------
void
Svar_DB::set_state(SV_key key, bool used, const char * loc)
{
const TCP_socket tcp = get_Svar_DB_tcp(__FUNCTION__);
   if (tcp == NO_TCP_SOCKET)   return;

string sloc(loc);
SET_STATE_c request(tcp, key, used, sloc);
}
//-----------------------------------------------------------------------------
bool
Svar_DB::may_set(SV_key key, int attempt)
{
const TCP_socket tcp = get_Svar_DB_tcp(__FUNCTION__);
   if (tcp == NO_TCP_SOCKET)   return 0;

MAY_SET_c request(tcp, key, attempt);

char * del = 0;
char buffer[2*MAX_SIGNAL_CLASS_SIZE + 16];
Signal_base * response = Signal_base::recv_TCP(tcp, buffer, sizeof(buffer),
                                               del, 0);

   if (response)
      {
        const bool ret = response->get__YES_NO__yes();
        delete response;
        return ret;
     }

   return true;
}
//-----------------------------------------------------------------------------
bool
Svar_DB::may_use(SV_key key, int attempt)
{
const TCP_socket tcp = get_Svar_DB_tcp(__FUNCTION__);
   if (tcp == NO_TCP_SOCKET)   return 0;

MAY_USE_c request(tcp, key, attempt);

char * del = 0;
char buffer[2*MAX_SIGNAL_CLASS_SIZE];
Signal_base * response = Signal_base::recv_TCP(tcp, buffer, sizeof(buffer),
                                               del, 0);

   if (response)
      {
        const bool ret = response->get__YES_NO__yes();
        delete response;
        return ret;
     }
   return true;
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
void
Svar_DB::retract_var(SV_key key)
{
const TCP_socket tcp = get_Svar_DB_tcp(__FUNCTION__);
   if (tcp == NO_TCP_SOCKET)   return;

RETRACT_VAR_c request(tcp, key);
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
char buffer[2*MAX_SIGNAL_CLASS_SIZE + 16];
Signal_base * response = Signal_base::recv_TCP(tcp, buffer, sizeof(buffer),
                                               del, 0);

   if (response)
      {
         offering_id.proc   = (AP_num)(response->get__OFFERING_ID_IS__proc());
         offering_id.parent = (AP_num)(response->get__OFFERING_ID_IS__parent());
         offering_id.grand  = (AP_num)(response->get__OFFERING_ID_IS__grand());
        delete response;
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
char buffer[2*MAX_SIGNAL_CLASS_SIZE + 16];
Signal_base * response = Signal_base::recv_TCP(tcp, buffer, sizeof(buffer),
                                               del, 0);

   if (response)
      {
        const string & op = response->get__OFFERING_PROCS_ARE__offering_procs();
        const AP_num * procs = (const AP_num *)(op.data());
        const size_t count = op.size() / sizeof(AP_num);

        loop(c, count)   processors.push_back(*procs++);
        delete response; 
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
char buffer[2*MAX_SIGNAL_CLASS_SIZE + 16];
Signal_base * response = Signal_base::recv_TCP(tcp, buffer, sizeof(buffer),
                                               del, 0);

   if (response)
      {
        const string & ov = response->get__OFFERED_VARS_ARE__offered_vars();
        const uint32_t * names = (const uint32_t *)(ov.data());
        const size_t count = ov.size() / sizeof(uint32_t);

        loop(c, count)   varnames.push_back(*names++);
        delete response; 
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
char buffer[2*MAX_SIGNAL_CLASS_SIZE + 16];
Signal_base * response = Signal_base::recv_TCP(tcp, buffer, sizeof(buffer),
                                               del, 0);

   if (response)
      {
        const bool ret = response->get__YES_NO__yes();
        delete response;
        return ret;
     }

   return false;
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
char buffer[2*MAX_SIGNAL_CLASS_SIZE + 16];
Signal_base * response = Signal_base::recv_TCP(tcp, buffer, sizeof(buffer),
                                               del, 0);

   if (response)
      {
        const SV_key ret = response->get__PAIRING_KEY_IS__pairing_key();
        delete response;
        return ret;
     }

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

   if (response)
      {
        out << response->get__SVAR_DB_PRINTED__printout();
        delete response;
      }
}
//-----------------------------------------------------------------------------

