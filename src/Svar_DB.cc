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

#include <iomanip>

#include "Backtrace.hh"
#include "Logging.hh"
#include "main.hh"
#include "Svar_DB.hh"
#include "Svar_signals.hh"
#include "UdpSocket.hh"

extern ostream CERR;
extern ostream & get_CERR();

uint16_t Svar_DB_memory_P::APserver_port = Default_APserver_tcp_port;

TCP_socket Svar_DB_memory_P::DB_tcp = NO_TCP_SOCKET;

Svar_DB_memory Svar_DB_memory_P::cache;
offered_SVAR   offered_SVAR_P::cache;

//=============================================================================

Svar_DB_memory * Svar_DB_memory_P::memory_p = 0;

Svar_DB_memory_P::Svar_DB_memory_P(bool ronly)
   : read_only(ronly)
{
   if (!APserver_available())   return;

   if (ronly)   { READ_SVAR_DB_c request(DB_tcp);   }
   else         { UPDATE_SVAR_DB_c request(DB_tcp); }

char * del = 0;
char buffer[2*MAX_SIGNAL_CLASS_SIZE + sizeof(Svar_DB_memory)];
ostream * log = (LOG_startup || LOG_Svar_DB_signals) ? & cerr : 0;
Signal_base * response = Signal_base::recv_TCP(DB_tcp, buffer, sizeof(buffer),
                                               del, log);
   if (response)   memcpy(&cache, response->get__SVAR_DB_IS__db().data(),
                          sizeof(Svar_DB_memory));
   else   CERR << "Svar_DB_memory_P() failed at " << LOC << endl;
   if (del)   delete del;
}
//-----------------------------------------------------------------------------
Svar_DB_memory_P::~Svar_DB_memory_P()
{
   if (read_only)               return;
   if (!APserver_available())   return;

std::string data((const char *)&cache, sizeof(cache));
SVAR_DB_IS_c updated(DB_tcp, data);
}
//-----------------------------------------------------------------------------
void
Svar_DB_memory_P::connect_to_APserver(const char * bin_path, bool logit)
{
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
     setsockopt(DB_tcp, SOL_TCP, TCP_NODELAY, &ndelay, sizeof(int));
   }

   // bind local port to 127.0.0.1
   //
   {
     sockaddr_in local;
     memset(&local, 0, sizeof(sockaddr_in));
     local.sin_family = AF_INET;
     local.sin_addr.s_addr = htonl(0x7F000001);

     if (::bind(DB_tcp, (const sockaddr *)&local, sizeof(sockaddr_in)))
        {
          get_CERR() << "bind(127.0.0.1) failed:" << strerror(errno) << endl;
          return;
        }

   }

   // We try to connect to the TCP port number APnnn_port (of the APserver)
   // on localhost. If that fails then no APserver is running; we fork one
   // and try again.
   //
   for (bool retry = false; ; retry = true)
       {
         sockaddr_in addr;
         memset(&addr, 0, sizeof(sockaddr_in));
         addr.sin_family = AF_INET;
         addr.sin_port = htons(APserver_port);
         addr.sin_addr.s_addr = htonl(0x7F000001);

         if (::connect(DB_tcp, (sockaddr *)&addr,
                       sizeof(addr)) == 0)   break;   // success

         if (logit)
            {
              get_CERR() << "connecting to 127.0.0.1 port " << APserver_port
                         << "." << endl;

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
         logit && get_CERR() << "forking new APserver listening on TCP port "
                             << APserver_port << endl;

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
              snprintf(arg0, sizeof(arg0), "%s/APserver", bin_path);
              char arg1[] = { "--port" };
              char arg2[20];
              snprintf(arg2, sizeof(arg2), "%d", APserver_port);
              char * argv[] = { arg0, arg1, arg2, 0 };
              char * envp[] = { 0 };
              execve(arg0, argv, envp);

              // execve() failed, try APs subdir...
              //
              if (logit)   get_CERR() << "execve(" << arg0;
              snprintf(arg0, sizeof(arg0), "%s/APs/APserver", bin_path);
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
Svar_DB_memory_P::DB_tcp_error(const char * op, int got, int expected)
{
   CERR << "⋆⋆⋆ " << op << " failed: got " << got << " when expecting "
        << expected << " (" << strerror(errno) << ")" << endl;

   ::close(DB_tcp);
   DB_tcp = NO_TCP_SOCKET;
}
//=============================================================================

offered_SVAR * offered_SVAR_P::offered_svar_p = 0;

offered_SVAR_P::offered_SVAR_P(bool ronly, SV_key key)
   : read_only(ronly)
{
   if (!Svar_DB_memory_P::APserver_available())   return;

const int sock = Svar_DB_memory_P::get_DB_tcp();
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
char buffer[2*MAX_SIGNAL_CLASS_SIZE + sizeof(offered_SVAR)];
ostream * log = (LOG_startup || LOG_Svar_DB_signals) ? & cerr : 0;
Signal_base * response = Signal_base::recv_TCP(sock, buffer, sizeof(buffer),
                                               del, log);
   if (response)
      {
        memcpy(&cache, response->get__SVAR_RECORD_IS__record().data(),
               sizeof(offered_SVAR));
        offered_svar_p = &cache;
      }
   else   CERR << "offered_SVAR_P() failed at " << LOC << endl;
   if (del)   delete del;
}
//-----------------------------------------------------------------------------
offered_SVAR_P::~offered_SVAR_P()
{
   if (read_only)                                 return;
   if (!Svar_DB_memory_P::APserver_available())   return;

std::string data((const char *)&cache, sizeof(cache));
SVAR_RECORD_IS_c updated(Svar_DB_memory_P::get_DB_tcp(), data);
}
//=============================================================================
void
Svar_DB::init(const char * bin_path, bool logit, bool do_svars)
{
   if (!do_svars)   // shared variables disable
      {
        // shared variables disable by the user. We init memory_p to be
        // on the safe side, but complain only if logit is true
        //
        memset(&Svar_DB_memory_P::cache, 0, sizeof(Svar_DB_memory_P::cache));
        Svar_DB_memory_P::memory_p = &Svar_DB_memory_P::cache;

        if (logit)
           get_CERR() << "Not opening shared memory because command "
                         "line option --noSV (or similar) was given." << endl;
        return;
      }

   Svar_DB_memory_P::memory_p = &Svar_DB_memory_P::cache;
   Svar_DB_memory_P::connect_to_APserver(bin_path, logit);
   if (Svar_DB_memory_P::APserver_available())
      {
        if (logit)   CERR << "using Svar_DB on APserver!" << endl;
      }
   else
      {
        CERR << "*** using local Svar_DB cache" << endl;
        memset(&Svar_DB_memory_P::cache, 0, sizeof(Svar_DB_memory_P::cache));
        Svar_DB_memory_P::memory_p = &Svar_DB_memory_P::cache;
      }
}
//-----------------------------------------------------------------------------
Svar_DB::~Svar_DB()
{
   if (!Svar_DB_memory_P::has_memory())   return;

const pid_t our_pid = getpid();

   // remove all variables offered by this process.
   //
Svar_DB_memory_P db(false);
   for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
       {
         offered_SVAR & svar = db->offered_vars[o];

         if (svar.accepting.pid == our_pid)   svar.remove_accepting();
         if (svar.offering.pid == our_pid)    svar.remove_offering();
         if (svar.get_coupling() == NO_COUPLING)   svar.clear();
       }

   Svar_DB_memory_P::disconnect();

}
//-----------------------------------------------------------------------------
void
Svar_DB::print(ostream & out)
{
   if (!Svar_DB_memory_P::has_memory())
      {
        out << "*** Svar_DB is not open!" << endl;
        return;
      }

const Svar_DB_memory_P db(true);

   // print active processors
   //
   out << "┌───────────┬─────┬─────┬──┐" << endl
       << "│ Proc, par │ PID │ Port│Fl│" << endl
       << "╞═══════════╪═════╪═════╪══╡" << endl;
   for (int p = 0; p < MAX_ACTIVE_PROCS; ++p)
       {
         const Svar_partner_events & sp = db->active_processors[p];
           if (sp.partner.id.proc)
              {
                out << "│";   sp.partner.print(CERR) << "│" << endl;
              }
       }
   out << "╘═══════════╧═════╧═════╧══╛" << endl;

   // print shared variables
   out <<
"╔═════╤═╦═══════════╤═════╤═════╤══╦═══════════╤═════╤═════╤══╦════╤══════════╗\n"
"║     │ ║ Offering  │     │     │  ║ Accepting │     │     │  ║OAOA│          ║\n"
"║ Seq │C║ Proc,par  │ PID │ Port│Fl║ Proc,par  │ PID │ Port│Fl║SSUU│ Varname  ║\n"
"╠═════╪═╬═══════════╪═════╪═════╪══╬═══════════╪═════╪═════╪══╬════╪══════════╣\n";
   for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
       {
         const offered_SVAR & svar = db->offered_vars[o];
         if (svar.valid())   svar.print(out);
       }

   out <<
"╚═════╧═╩═══════════╧═════╧═════╧══╩═══════════╧═════╧═════╧══╩════╧══════════╝\n"
       << endl;
}
//-----------------------------------------------------------------------------

