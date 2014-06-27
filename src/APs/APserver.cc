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
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <iomanip>
#include <vector>

#include "SystemLimits.hh"

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
check_op(const char * op, TCP_socket fd, int got, int expected)
{
   if (got != expected)
      {
        usleep(50000);
        cerr << endl << "*** " << op << " failed on fd " << fd
             << " (got " << got << " expected " << expected
             << "). closing fd." << endl;

        close_fd(fd);
        return;
      }
}
//-----------------------------------------------------------------------------
static void
connection_readable(TCP_socket fd)
{
char command;
ssize_t len = ::recv(fd, &command, 1, 0);

   if (len <= 0)
      {
         (verbosity > 0) && cerr << prog << "connection[" << fd
              << "] closed (recv_TCP() returned 0" << endl;
         close_fd(fd);
         return;
      }

   switch(command)
      {
        case 'i':
             {
               (verbosity > 0) && cerr << "[" << fd << "] identify - ";

               // read proc/parent/grand from socket
               AP_num buffer[3];
               len = ::recv(fd, buffer, sizeof(buffer), MSG_WAITALL);
               check_op("recv() identity", fd, len, sizeof(buffer));

               // find connected_procs entry for fd...
               //
               int idx = -1;
               for (int j = 0; j < connected_procs.size(); ++j)
                   {
                     if (connected_procs[j].fd == fd)
                        {
                          idx = j;
                          break;
                        }
                   }

               if (idx == -1)   // not found
                  {
                    cerr << prog << ": could not find fd " << fd
                         << " in connected_procs" << endl;
                    close_fd(fd);
                    return;
                  }
               connected_procs[idx].ap3.proc   = buffer[0];
               connected_procs[idx].ap3.parent = buffer[1];
               connected_procs[idx].ap3.grand  = buffer[2];

               (verbosity > 0) && cerr << "done. Id is "
                                  << buffer[0] << ":" << buffer[1]
                    << ":" << buffer[2] << endl;
             }
             return;

        case 'a':
             {
               (verbosity > 0) && cerr << "[" << fd << "] read all - ";
               len = ::send(fd, &db, sizeof(db), 0);
               check_op("send() all", fd, len, sizeof(db));
               (verbosity > 0) && cerr << "done." << endl;
             }
             return;

        case 'A':
             {
               (verbosity > 0) && cerr << "[" << fd << "] update all - ";
               len = ::send(fd, &db, sizeof(db), 0);
               check_op("send() all", fd, len, sizeof(db));
               (verbosity > 0) && cerr << "sent - ";

               // send updated db back
               //
               len = ::recv(fd, &db, sizeof(db), 0);
               check_op("recv()", fd, len, sizeof(db));
               (verbosity > 0) && cerr << "done." << endl;
             }
             return;

        case 'r':
             {
               (verbosity > 0) && cerr << "[" << fd << "] read record - ";
               SV_key key;
               len = ::recv(fd, &key, sizeof(key), MSG_WAITALL);
               check_op("recv() key", fd, len, sizeof(key));

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
                    len = ::send(fd, svar, sizeof(offered_SVAR), 0);
                    check_op("send() record", fd, len, sizeof(offered_SVAR));
                  }
               else
                  {
                    offered_SVAR dummy;
                    memset(&dummy, 0, sizeof(offered_SVAR));
                    len = ::send(fd, &dummy, sizeof(offered_SVAR), 0);
                    check_op("send() dummy", fd, len, sizeof(offered_SVAR));
                  }

               (verbosity > 0) && cerr << "record sent - done." << endl;
             }
             return;

        case 'R':
             {
               (verbosity > 0) && cerr << "[" << fd << "] update record - ";
               SV_key key;
               len = ::recv(fd, &key, sizeof(key), MSG_WAITALL);
               check_op("recv() key", fd, len, sizeof(key));

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
                    len = ::send(fd, svar, sizeof(offered_SVAR), 0);
                    check_op("send() record", fd, len, sizeof(offered_SVAR));

                    (verbosity > 0) && cerr << "record sent - ";

                    len = ::recv(fd, svar, sizeof(offered_SVAR), 0);
                    check_op("recv()", fd, len, sizeof(offered_SVAR));

                    (verbosity > 0) && cerr << "done." << endl;
                  }
               else
                  {
                    offered_SVAR dummy;
                    memset(&dummy, 0, sizeof(offered_SVAR));
                    len = ::send(fd, &dummy, sizeof(offered_SVAR), 0);
                    check_op("send() dummy", fd, len, sizeof(offered_SVAR));
                  }

               (verbosity > 0) && cerr << "done." << endl;
             }
             return;

        default: cerr << "got unknown command'" << command
                      << "' on fd " << fd << endl;
                 close_fd(fd);
                 return;

      }
}
//-----------------------------------------------------------------------------
int
main(int argc, char * argv[])
{
int listen_port = Default_APserver_tcp_port;
   prog = argv[0];
   for (int a = 1; a < argc; )
       {
         const char * opt = argv[a++];
         const char * val = (a < argc) ? argv[a] : 0;

         if (!strcmp(opt, "-H"))
            {
              hang_on = true;
            }
         else if (!strcmp(opt, "--port"))
            {
              ++a;
              if (!val)
                 {
                   cerr << "--port without argument" << endl;
                   return a;
                 }

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

   if (argc == 3)
      {
        listen_port = atoi(argv[2]);
      }

   (verbosity > 0) && cerr << prog << ": listening on TCP port "
                      << listen_port << endl;

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
        cerr << prog << ": ::bind("
             << listen_port << ") failed:" << strerror(errno) << endl;
        return 3;
      }

   if (::listen(listen_sock, 10))
      {
        cerr << prog << ": ::listen(fd "
             << listen_sock << ") failed:" << strerror(errno) << endl;
        return 3;
      }

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

