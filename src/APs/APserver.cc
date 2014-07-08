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
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "config.h"   // for HAVE_ macros
#ifdef HAVE_LINUX_UN_H
#include <linux/un.h>
#endif

#include <iostream>
#include <iomanip>
#include <vector>

#include "SystemLimits.hh"
#include "Svar_signals.hh"

#define __COMMON_HH_DEFINED__ // to avoid #error in APL_types.hh
#include "Svar_DB_memory.hh"

#include <iostream>

using namespace std;

Svar_DB_memory db;
int verbosity = 0;

struct AP3_fd
{
   AP_num3 ap3;
   TCP_socket fd;
};

vector<AP3_fd> connected_procs;

const char * prog = "????";

extern ostream & get_CERR();
ostream & get_CERR() { return cerr; }

bool hang_on = false;   // dont exit after last connection was closed

//-----------------------------------------------------------------------------
static void
close_fd(TCP_socket fd)
{
   for (int j = 0; j < connected_procs.size(); ++j)
       {
         const TCP_socket fd_j = connected_procs[j].fd;
         if (fd == fd_j)
            {
              connected_procs.erase(connected_procs.begin() + j);
              break;
            }
       }

   ::close(fd);

   // maybe exit if the last connection was removed
   //
   (verbosity > 0) && cerr << prog << ": closed fd " << fd << " ("
        << connected_procs.size() << " remaining)" << endl;

   if (hang_on)   return;

   if (connected_procs.size() == 0)
      {
        (verbosity > 0) &&  cerr << prog
                      << ": exiting after last connection was closed" << endl;
        exit(0);
      }
}
//-----------------------------------------------------------------------------
static void
new_connection(TCP_socket fd)
{
   (verbosity > 0) && cerr << prog << ": new connection: fd " << fd << endl;

AP3_fd ap3_fd;
   ap3_fd.ap3.proc   = NO_AP;
   ap3_fd.ap3.parent = AP_NULL;
   ap3_fd.ap3.grand  = AP_NULL;
   ap3_fd.fd         = fd;

   connected_procs.push_back(ap3_fd);
}
//-----------------------------------------------------------------------------
static void
do_signal(TCP_socket fd, Signal_base * request)
{
usleep(50000);

   switch(request->get_sigID())
      {
        case sid_READ_SVAR_DB:
             {
               (verbosity > 0) && cerr << "[" << fd << "] read all - ";

               string data((const char *)&db, sizeof(db));
               { SVAR_DB_IS_c response(fd, data); }
             }
             return;

        case sid_UPDATE_SVAR_DB:
             {
               (verbosity > 0) && cerr << "[" << fd << "] update all - ";
               string data((const char *)&db, sizeof(db));
               { SVAR_DB_IS_c response(fd, data); }

               char buffer[2*MAX_SIGNAL_CLASS_SIZE + sizeof(Svar_DB_memory)];
               char * del = 0;
               ostream * debug = verbosity ? &cerr : 0;
               Signal_base * update = Signal_base::recv_TCP(fd, buffer,
                                                    sizeof(buffer), del, debug);
               if (update)   memcpy(&db, update->get__SVAR_DB_IS__db().data(),
                                    sizeof(Svar_DB_memory));
               else   cerr << "recv_TCP() failed at line " << __LINE__ << endl;

               if (del)   delete del;
             }
             return;

        case sid_READ_SVAR_RECORD:
             {
               (verbosity > 0) && cerr << "[" << fd << "] read record - ";
               SV_key key = request->get__READ_SVAR_RECORD__key();

               (verbosity > 0) && cerr << "key " << hex << uppercase
                                  << setfill('0') <<  key << setfill(' ')
                                  << dec << nouppercase << " - ";

               offered_SVAR * svar = 0;
               if (key)
                  {
                    for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
                        {
                          if (key == db.offered_vars[o].key)
                             {
                               svar = db.offered_vars + o;
                               break;
                             }

                        }
                  }

               if (svar)
                  {
                    string data((const char *)svar, sizeof(offered_SVAR));
                    { SVAR_RECORD_IS_c(fd, data); }
                  }
               else
                  {
                    char dummy[sizeof(offered_SVAR)];
                    memset(&dummy, 0, sizeof(offered_SVAR));
                    string data((const char *)&dummy, sizeof(offered_SVAR));
                    { SVAR_RECORD_IS_c(fd, data); }
                  }

               (verbosity > 0) && cerr << "record sent - done." << endl;
             }
             return;

        case sid_UPDATE_SVAR_RECORD:
             {
               (verbosity > 0) && cerr << "[" << fd << "] update record - ";
               SV_key key = request->get__UPDATE_SVAR_RECORD__key();

               (verbosity > 0) && cerr << "key " << hex << uppercase
                                  << setfill('0') <<  key << setfill(' ')
                                  << dec << nouppercase << " - ";

               offered_SVAR * svar = 0;
               if (key)
                  {
                    for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
                        {
                          if (key == db.offered_vars[o].key)
                             {
                               svar = db.offered_vars + o;
                               break;
                             }
                        }
                  }

               if (svar)
                  {
                    string data((const char *)svar, sizeof(offered_SVAR));
                    { SVAR_RECORD_IS_c(fd, data); }
                    (verbosity > 0) && cerr << " record sent - ";

                    char buffer[2*MAX_SIGNAL_CLASS_SIZE + sizeof(offered_SVAR)];
                    char * del = 0;
                    ostream * debug = verbosity ? &cerr : 0;
                    Signal_base * update = Signal_base::recv_TCP(fd, buffer,
                                                    sizeof(buffer), del, debug);
                    if (update)
                       {
                         memcpy(svar,
                                update->get__SVAR_RECORD_IS__record().data(),
                                         sizeof(offered_SVAR));
                         (verbosity > 0) && cerr <<
                                   "  update record received - done " << endl;
                       }
                    else   cerr << "recv_TCP() failed at line " << __LINE__
                                << endl;

               if (del)   delete del;
                  }
               else
                  {
                    char dummy[sizeof(offered_SVAR)];
                    memset(&dummy, 0, sizeof(offered_SVAR));
                    string data((const char *)&dummy, sizeof(offered_SVAR));
                    { SVAR_RECORD_IS_c(fd, data); }
                    (verbosity > 0) && cerr << "dummy record sent - done "
                                            << endl;
                  }
             }
             return;

        case sid_MY_PID_IS:
             {
               (verbosity > 0) && cerr << "[" << fd << "] identify - ";

               // find connected_procs entry for fd...
               //
               AP3_fd * ap_fd = 0;
               for (int j = 0; j < connected_procs.size(); ++j)
                   {
                     if (connected_procs[j].fd == fd)
                        {
                          ap_fd = &connected_procs[j];
                          break;
                        }
                   }

               if (ap_fd == 0)   // not found
                  {
                    cerr << prog << ": could not find fd " << fd
                         << " in connected_procs" << endl;
                    close_fd(fd);
                    return;
                  }
               ap_fd->ap3.proc   = (AP_num)(request->get__MY_PID_IS__pid());
               ap_fd->ap3.parent = (AP_num)(request->get__MY_PID_IS__parent());
               ap_fd->ap3.grand  = (AP_num)(request->get__MY_PID_IS__grand());

               (verbosity > 0) && cerr << "done. Id is "
                                  << ap_fd->ap3.proc << ":" << ap_fd->ap3.parent
                    << ":" << ap_fd->ap3.grand << endl;
             }
             return;

        default: break;
      }

   cerr << "got unknown signal ID " << request->get_sigID() << endl;
}
//-----------------------------------------------------------------------------
static void
connection_readable(TCP_socket fd)
{
char buffer[50000];
char * del = 0;
ostream * debug = verbosity ? &cerr : 0;
Signal_base * request = Signal_base::recv_TCP(fd, buffer, sizeof(buffer),
                                              del, debug);
   if (request == 0)
      {
         (verbosity > 0) && cerr << prog << "connection[" << fd
              << "] closed (recv_TCP() returned 0" << endl;
         close_fd(fd);
         return;
      }

   do_signal(fd, request);

   if (del)   delete del;
}
//-----------------------------------------------------------------------------
#ifdef HAVE_LINUX_UN_H
int
open_UNIX_socket(const char * listen_name)
{
const int listen_sock = socket(AF_UNIX, SOCK_STREAM, 0);
   {
     int yes = 1;
     setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
   }

struct sockaddr_un local;

   memset(&local, 0, sizeof(sockaddr_un));
   local.sun_family = AF_UNIX;

   strcpy(local.sun_path + ABSTRACT_OFFSET, listen_name);

   if (::bind(listen_sock, (const sockaddr *)&local, sizeof(sockaddr_un)))
      {
        cerr << prog << ": ::bind("
             << listen_name << ") failed:" << strerror(errno) << endl;
        return -13;
      }

   if (::listen(listen_sock, 10))
      {
        cerr << prog << ": ::listen(\""
             << listen_name << "\") failed:" << strerror(errno) << endl;
        return -14;
      }

   (verbosity > 0) && cerr << prog << ": listening on abstract AF_UNIX socket '"
                           << listen_name << "'" << endl;

   return listen_sock;
}
#else
int
open_UNIX_socket(const char * listen_name)
{
   cerr << "*** this platform does not support address family AF_UNIX!";
}
#endif
//-----------------------------------------------------------------------------
int
open_TCP_socket(int listen_port)
{
const int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
   {
     int yes = 1;
     setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
   }

sockaddr_in local;
   memset(&local, 0, sizeof(sockaddr_in));
   local.sin_family = AF_INET;
   local.sin_port = htons(listen_port);
   local.sin_addr.s_addr = htonl(0x7F000001);

   if (::bind(listen_sock, (const sockaddr *)&local, sizeof(sockaddr_in)))
      {
        cerr << prog << ": ::bind(127.0.0.1 port"
             << listen_port << ") failed:" << strerror(errno) << endl;
        return -13;
      }

   if (::listen(listen_sock, 10))
      {
        cerr << prog << ": ::listen(fd "
             << listen_sock << ") failed:" << strerror(errno) << endl;
        return -14;
      }

   (verbosity > 0) && cerr << prog << ": listening on TCP port "
                           << listen_port << endl;

   return listen_sock;
}
//-----------------------------------------------------------------------------
int
main(int argc, char * argv[])
{
int listen_port = Default_APserver_tcp_port;
const char * listen_name = Svar_DB_memory::get_APserver_unix_socket_name();
bool got_path = false;
bool got_port = false;

   prog = argv[0];
   for (int a = 1; a < argc; )
       {
         const char * opt = argv[a++];
         const char * val = (a < argc) ? argv[a] : 0;

         if (!strcmp(opt, "-H"))
            {
              hang_on = true;
            }
         else if (!strcmp(opt, "--path"))
            {
              ++a;
              if (!val)
                 {
                   cerr << "--path without argument" << endl;
                   return a;
                 }
               listen_name = val;
               got_path = true;
            }
         else if (!strcmp(opt, "--port"))
            {
              ++a;
              if (!val)
                 {
                   cerr << "--port without argument" << endl;
                   return a;
                 }
               listen_port = atoi(val);
               got_port = true;
            }
         else if (!strcmp(opt, "-v"))
            {
              ++verbosity;
            }
         else
            {
              cerr << "unknown command line option " << opt << endl;
              return a;
            }
       }

   if (got_path && got_port)
      {
        cerr << "cannot specify both --path and --port " << endl;
        return argc;
      }

   if (verbosity > 0)
      cerr << "sizeof(Svar_DB_memory) is " << sizeof(Svar_DB_memory) << endl
           << "sizeof(offered_SVAR) is " << sizeof(offered_SVAR) << endl
           << "sizeof(Svar_partner_events) is " << sizeof(Svar_partner_events)
           << endl;

const int listen_sock = got_path ? open_UNIX_socket(listen_name)
                                 : open_TCP_socket(listen_port);
   if (listen_sock < 0)   return 20;

   memset(&db, 0, sizeof(db));

int max_fd = listen_sock;

   (verbosity > 0) && cerr << prog << ": entering main loop..." << endl;
   for (;;)
      {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(listen_sock, &read_fds);
        int max_fd = listen_sock;
        for (int j = 0; j < connected_procs.size(); ++j)
            {
              const TCP_socket fd = connected_procs[j].fd;
              if (max_fd < fd)   max_fd = fd;
              FD_SET(fd, &read_fds);
            }

        errno = 0;
        const int count = select(max_fd + 1, &read_fds, 0, 0, 0);
        if (count <= 0)
           {
             cerr << prog << ": count <= 0 in select(): "
                  << strerror(errno) << endl;
             return 3;
           }

        // something happened, check listen_sock first for new connections
        //
        if (FD_ISSET(listen_sock, &read_fds))
           {
             struct sockaddr_in from;
             socklen_t from_len = sizeof(sockaddr_in);
             const int new_fd = ::accept(listen_sock, (sockaddr *)&from,
                                         &from_len);

             // disable nagle
             {
               const int ndelay = 1;
               setsockopt(new_fd, 6, TCP_NODELAY, &ndelay, sizeof(int));
             }

             if (new_fd == -1)
                {
                  cerr << prog << ": ::accept() failed: "
                       << strerror(errno) << endl;
                  exit(1);
                  continue;
                }

             new_connection((TCP_socket)new_fd);
             continue;
           }

        // connections readable
        //
        for (int j = 0; j < connected_procs.size(); ++j)
            {
              const TCP_socket fd = connected_procs[j].fd;
              if (FD_ISSET(fd, &read_fds))   connection_readable(fd);
            }
      }
}
//-----------------------------------------------------------------------------

