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

/*  Copyright (C) 2012 Dr. Juergen Sauermann
    This file is part of udp_signal.

    udp_signal is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    udp_signal is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with udp_signal.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
    this file implements classes UdpClientSocket and UdpServerSocket which
    are used to send signals from one process to another process.
 */

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <string>
#include <iostream>
#include <iomanip>

#include "../config.h"   // for HAVE_SYS_PRCTL_H
#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#else
#define prctl(x, y)
#endif

#include "UdpSocket.hh"

using namespace std;

//----------------------------------------------------------------------------
UdpSocket::UdpSocket(bool _is_server)
  : local_port(-1),
    remote_port(-1),
    local_ip(-1),
    remote_ip(-1),
    is_server(_is_server),
    udp_socket(-1),
    debug(0)
{
}
//----------------------------------------------------------------------------
UdpSocket::UdpSocket(uint16_t _local_port, uint16_t _remote_port,
             uint32_t _local_ip,   uint32_t _remote_ip, bool _is_server)
  : local_port(_local_port),
    remote_port(_remote_port),
    local_ip(_local_ip),
    remote_ip(_remote_ip),
    is_server(_is_server),
    udp_socket(-1),
    debug(0)
{
   if (remote_port == -1)   return;

   udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
   assert(udp_socket != -1);

sockaddr_in local;
   memset(&local, 0, sizeof(sockaddr_in));
   local.sin_family = AF_INET;
   local.sin_port = htons(local_port);
   local.sin_addr.s_addr = htonl(local_ip);

   if (bind(udp_socket, (const sockaddr *)&local, sizeof(sockaddr_in)))
      {
        cerr << "*** UdpSocket::UdpSocket() : bind("
             << local_port << ") failed:" << strerror(errno) << endl;
        assert(0);
      }

   // refresh the local port and IP, since bind() may have set them.
   //
   socklen_t size = sizeof(sockaddr_in);
   getsockname(udp_socket, (sockaddr *)&local, &size);
   local_port = ntohs(local.sin_port);
   local_ip = ntohl(local.sin_addr.s_addr);
}
//----------------------------------------------------------------------------
int
UdpSocket::send(const std::string & buffer) const
{
sockaddr_in remote;
   memset(&remote, 0, sizeof(sockaddr_in));
   remote.sin_family = AF_INET;
   remote.sin_port = htons(remote_port);
   remote.sin_addr.s_addr = htonl(remote_ip);

   for (;;)
       {
         errno = 0;
         const int sent = sendto(udp_socket, buffer.c_str(), buffer.size(), 0,
                                  (const sockaddr *)&remote, sizeof(remote));

         if (sent != buffer.size())   // something went wrong
            {
              if (errno == EINTR)   continue;   // signal received

              cerr << "UdpSocket::send() failed: " << strerror(errno) << endl;
              return -errno;
            }

            break;
       }

   return 0;
}
//----------------------------------------------------------------------------
int
UdpSocket::recv(void * buffer, int bufsize, uint16_t & from_port,
                uint32_t & from_ip, uint32_t timeout_ms)
{
sockaddr_in remote;
socklen_t remote_len = sizeof(remote);

   if (timeout_ms)   // abort recv after timeout_ms
      {
        for (;;)
            {
              fd_set read_fds;
              FD_ZERO(&read_fds);
              FD_SET(udp_socket, &read_fds);
              timeval tv = { timeout_ms/1000, 1000*(timeout_ms%1000) };

              errno = 0;
              const int cnt = select(udp_socket + 1, &read_fds, 0, 0, &tv);
              if (cnt == 0)
                 {
                   errno = ETIMEDOUT;
                   return 0;
                 }
              else if (cnt < 0)
                 {
                   if (errno == EINTR)   continue;   // signal received

                   cerr << "UdpSocket::recv() failed: select() returned "
                        << strerror(errno) << endl;
                   return 0;
                 }

              break;   // success: proceed with recv
            }
      }

int len = -1;

   for (;;)
       {
         errno = 0;
         len = recvfrom(udp_socket, buffer, bufsize, 0,
                         (sockaddr *)&remote, &remote_len);

         if (len <= 0)
            {
              if (errno == EINTR)   continue;   // signal received

              cerr << "UdpSocket::recv() failed: " << strerror(errno) << endl;
              return 0;
            }

         break;
       }

   from_port = ntohs(remote.sin_port);
   from_ip = ntohl(remote.sin_addr.s_addr);

   if (is_server)
      {
        remote_port = from_port;
        remote_ip = from_ip;
      }

   return len;
}
//----------------------------------------------------------------------------

/**
 ** we use a convention that a server for "service" does prctl(PR_SET_NAME)
    with argument "service:port" and provides the service on UDP port \b port
 **/
int
UdpSocket::find_server_port(const char * service)
{
DIR * dir = opendir("/proc");
   if (dir == 0)
      {
        cerr << "Could not open /proc : " << strerror(errno) << endl;
        return 0;
      }

   for (;;)
       {
         dirent * entry = readdir(dir);
         if (entry == 0)   break;

         int pid;
         if (sscanf(entry->d_name, "%u", &pid) != 1)   continue;

         char stat_filename[40];
         snprintf(stat_filename, sizeof(stat_filename), "/proc/%d/stat", pid);
         int stat_fd = open(stat_filename, O_RDONLY);
         if (stat_fd == -1)   continue;

         char stat_data[40];
         ssize_t len = read(stat_fd, stat_data, sizeof(stat_data));
         close(stat_fd);

         if (len <= 0)    continue;

         if (len >= sizeof(stat_data))   len = sizeof(stat_data) - 1;
         stat_data[len] = 0;

         char stat_wanted[40];
         snprintf(stat_wanted, sizeof(stat_wanted),
                  "%u (%s:%%u)", pid, service);

         int port;
         if (sscanf(stat_data, stat_wanted, &port) == 1)   // found
            {
              closedir(dir);
              return port;
            }
       }

   closedir(dir);
   return 0;
}
//----------------------------------------------------------------------------
int
UdpServerSocket::register_server_port(const char * service)
{
   if (strlen(service) > 10)   return 1;

char name[17];   // prctl allows max. 16 chars for the process name
   snprintf(name, sizeof(name), "%s:%d", service, get_local_port());
   name[16] = 0;
   prctl(PR_SET_NAME, name);

   return 0;   // OK
}
//----------------------------------------------------------------------------

