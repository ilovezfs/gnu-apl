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

// #define USE_POLL   /* use poll() instead of select() */

#include <errno.h>
#include <limits.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef USE_POLL
# include <poll.h>
#else
# include <sys/select.h>
#endif

#include "config.h"   // for HAVE_ macros
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>

#define __COMMON_HH_DEFINED__ // to avoid #error in APL_types.hh
#define AP_NUM /* simple AP_NUM */

#include "APL_types.hh"
#include "ProcessorID.hh"
#include "SystemLimits.hh"
#include "Svar_signals.hh"
#include "Svar_DB_server.hh"

using namespace std;

Svar_DB_server db;
int verbosity = 0;
ostream * debug = 0;

#define Log(x) if (verbosity > 0)
bool LOG_shared_variables = false;

/// Stringify x.
#define STR(x) #x
/// The current location in the source file.
#define LOC Loc(__FILE__, __LINE__)
/// The location line l in file f.
#define Loc(f, l) f ":" STR(l)

AP_num3 ProcessorID::id;

//-----------------------------------------------------------------------------
const char * prog_name()
{
   return "APserver";
}
//-----------------------------------------------------------------------------
/// a database containing all connected processors
extern vector<AP3_fd> connected_procs;
vector<AP3_fd> connected_procs;
//-----------------------------------------------------------------------------
struct key_value
{
   key_value(SV_key k)
   : key(k)
   {}

   /// the key for the variable
  SV_key key;

   /// the value of the variable
  string var_value;
};

/// the current value of all shared variables
vector<key_value> var_values;
//-----------------------------------------------------------------------------

const char * prog = "????";

extern ostream & get_CERR();
ostream & get_CERR() { return cerr; }

bool hang_on = false;   // dont exit after last connection was closed

static void print_db(ostream & out);

//-----------------------------------------------------------------------------
extern TCP_socket get_tcp_fd_for_id(const AP_num3 & id);
TCP_socket
get_tcp_fd_for_id(const AP_num3 & id)
{
   for (int j = 0; j < connected_procs.size(); ++j)
       {
         if (id == connected_procs[j].ap3)   return connected_procs[j].fd;
       }

   return NO_TCP_SOCKET;
}
//-----------------------------------------------------------------------------
extern TCP_socket get_tcp_fd2_for_id(const AP_num3 & id);
TCP_socket
get_tcp_fd2_for_id(const AP_num3 & id)
{
   for (int j = 0; j < connected_procs.size(); ++j)
       {
         if (id == connected_procs[j].ap3 &&
             connected_procs[j].fd2 != NO_TCP_SOCKET)
            return connected_procs[j].fd2;
       }

   return NO_TCP_SOCKET;
}
//-----------------------------------------------------------------------------
static
AP_num3 fd_to_id(TCP_socket fd)
{
   for (int j = 0; j < connected_procs.size(); ++j)
       {
         if (fd == connected_procs[j].fd)    return connected_procs[j].ap3;
         if (fd == connected_procs[j].fd2)   return connected_procs[j].ap3;
       }

   return AP_num3();
}
//-----------------------------------------------------------------------------
TCP_socket
get_TCP_for_key(SV_key key)
{
Svar_record * svar = db.find_var(key, LOC);
   if (!svar)   goto not_found;

   for (int j = 0; j < connected_procs.size(); ++j)
       {
         if (connected_procs[j].fd2 == NO_TCP_SOCKET)   continue;

         if (connected_procs[j].ap3 == svar->offering.id &&
             svar->offering.tcp_fd != NO_TCP_SOCKET)
            {
              return connected_procs[j].fd2;
            }

         if (connected_procs[j].ap3 == svar->accepting.id &&
             svar->accepting.tcp_fd != NO_TCP_SOCKET)
            {
              return connected_procs[j].fd2;
            }
       }

not_found:
   cerr << prog_name() << ": could not find TCP for key " << key << endl;
   return NO_TCP_SOCKET;
}
//-----------------------------------------------------------------------------
TCP_socket
get_peer_fd(SV_key key, TCP_socket fd)
{
AP_num3 from_id = fd_to_id(fd);

Svar_record * svar = db.find_var(key, LOC);
   if (!svar)   return NO_TCP_SOCKET;

AP_num3 peer_id;
   if      (from_id == svar->offering.id)    peer_id = svar->accepting.id;
   else if (from_id == svar->accepting.id)   peer_id = svar->offering.id;
   else
      {
         cerr << "*** key " << (key & 0xFFFF)
              << " has no fd " << fd << " at " << LOC << endl;
         print_db(cerr);
        return NO_TCP_SOCKET;
      }

   return get_tcp_fd_for_id(peer_id);
}
//-----------------------------------------------------------------------------
TCP_socket
get_peer_fd2(SV_key key, TCP_socket fd)
{
AP_num3 from_id = fd_to_id(fd);

Svar_record * svar = db.find_var(key, LOC);
   if (!svar)   return NO_TCP_SOCKET;

AP_num3 peer_id;
   if      (from_id == svar->offering.id)    peer_id = svar->accepting.id;
   else if (from_id == svar->accepting.id)   peer_id = svar->offering.id;
   else
      {
         cerr << "*** key " << (key & 0xFFFF)
              << " has no fd " << fd << " at " << LOC << endl;
         print_db(cerr);
        return NO_TCP_SOCKET;
      }

   return get_tcp_fd2_for_id(peer_id);
}
//-----------------------------------------------------------------------------
static struct sigaction old_control_C_action;
static struct sigaction new_control_C_action;

static void
control_C(int)
{
   cerr << "\nAPserver terminated by SIGINT" << endl;
   exit(0);
}
//-----------------------------------------------------------------------------
static struct sigaction old_control_BSL_action;
static struct sigaction new_control_BSL_action;

static void
control_BSL(int)
{
   print_db(cerr << "\r");
}
//-----------------------------------------------------------------------------
void
maybe_start_AP(const AP_num3 & id)
{
   // currently APs are started by the APL interpeter and not by APserver.
   //
   return;

   if (id.proc >= AP_FIRST_USER)   return;   // not  an AP

   for (int j = 0; j < connected_procs.size(); ++j)
       {
         if (connected_procs[j].ap3 == id)   return;   // already running
       }

char AP_progname[PATH_MAX + 1];
   snprintf(AP_progname, sizeof(AP_progname), "%s", prog);

char * bslash = strrchr(AP_progname, '/');
   if (bslash)
      {
        const int offset = bslash - AP_progname;
        snprintf(bslash + 1, PATH_MAX - offset - 1, "AP%u", id.proc);
      }
   else
      {
        snprintf(AP_progname, PATH_MAX, "AP%u", id.proc);
      }
   
   AP_progname[PATH_MAX] = 0;

   debug && *debug << "*** starting " << AP_progname << endl;

char popen_arg[PATH_MAX + 1];
   if (debug)
      {
        snprintf(popen_arg, PATH_MAX,
                 "%s --id %u --par %u --gra %u --auto -v",
                 AP_progname, id.proc, id.parent, id.grand);
      }
   else
      {
        snprintf(popen_arg, PATH_MAX,
                 "%s --id %u --par %u --gra %u --auto",
                 AP_progname, id.proc, id.parent, id.grand);
      }
 

FILE * fp = popen(popen_arg, "r");
   if (fp == 0)
      {
        cerr << "popen(" << popen_arg << ") failed: " << strerror(errno)
             << endl;
        return;
      }

   for (int cc; (cc = getc(fp)) != EOF;)   cerr << (char)cc;
   cerr << endl;

   pclose(fp);
}
//-----------------------------------------------------------------------------
static void
close_fd(TCP_socket fd)
{
   // the AP on fd has closed its TCP connection which means it has died
   // clean up the databases.
   //
AP_num3 removed_AP;
bool found_fd = false;
TCP_socket removed_fd2 = NO_TCP_SOCKET;

   for (int j = 0; j < connected_procs.size(); ++j)
       {
         if (fd == connected_procs[j].fd)
            {
              found_fd = true;
              removed_AP = connected_procs[j].ap3;
              connected_procs.erase(connected_procs.begin() + j);
              (verbosity > 0) && cerr << "erased fd " << fd << " (Id "
                                      << removed_AP.proc << ":"
                                      << removed_AP.parent << ":"
                                      << removed_AP.grand
                                      << ") from connected_procs" << endl;
              removed_fd2 = connected_procs[j].fd2;
              break;
            }
         else if (fd == connected_procs[j].fd2)
            {
              found_fd = true;
              removed_AP = connected_procs[j].ap3;
              connected_procs.erase(connected_procs.begin() + j);
              (verbosity > 0) && cerr << "erased fd " << fd << " (Id "
                                      << removed_AP.proc << ":"
                                      << removed_AP.parent << ":"
                                      << removed_AP.grand
                                      << ") from connected_procs" << endl;
              removed_fd2 = connected_procs[j].fd;
              break;
            }
       }

   ::close(fd);
   if (removed_fd2 != NO_TCP_SOCKET)   ::close(removed_fd2);

   if (!found_fd)   // should not happen
      {
         cerr << "*** internal error in APserver: (dis-)connected fd not found"
              << endl;
         return;
      }

   // retract offers made by the removed processor
   //
   for (int o = 0; o < db.offered_vars.size(); ++o)
       {
         Svar_record & svar = db.offered_vars[o];

         if (svar.accepting.id == removed_AP)      svar.remove_accepting();
         if (svar.offering.id == removed_AP)       svar.remove_offering();
         if (svar.get_coupling() == NO_COUPLING)
            {
              db.offered_vars.erase(db.offered_vars.begin() + o);
              --o;
            }
       }

   // disconnect dependent processors...
   //
   if (removed_AP.grand == 0)   // otherwise removed_AP is an AP
      {
        if (removed_AP.parent)   // removed_AP is proc.parent.0
           {
             for (int j = 0; j < connected_procs.size(); ++j)
                 {
                   if (connected_procs[j].ap3.parent == removed_AP.proc &&
                       connected_procs[j].ap3.grand  == removed_AP.parent)
                      {
                        debug && *debug << "removing dependent processor "
                             << connected_procs[j].ap3.proc << endl;

                       if (connected_procs[j].fd != NO_TCP_SOCKET)
                          close_fd(connected_procs[j].fd);
                      }
                 }
           }
        else                     // removed_AP is proc.0.0
           {
             for (int j = 0; j < connected_procs.size(); ++j)
                 {
                   if (connected_procs[j].ap3.parent == removed_AP.proc &&
                       connected_procs[j].ap3.grand == 0)
                      {
                        debug && *debug << "removing dependent processor "
                             << connected_procs[j].ap3.proc << endl;

                       if (connected_procs[j].fd != NO_TCP_SOCKET)
                          close_fd(connected_procs[j].fd);
                      }
                 }
           }
      }

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
   ap3_fd.ap3.proc   = AP_NULL;
   ap3_fd.fd         = fd;

   connected_procs.push_back(ap3_fd);
}
//-----------------------------------------------------------------------------
static void
do_signal(TCP_socket fd, Signal_base * request)
{
   // set ProcessorID::id to the ID of the other end of fd, so that we
   // impersonate that processor
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

   if (ap_fd)   ProcessorID::set_id(ap_fd->ap3);
   else         ProcessorID::clear_id();

   switch(request->get_sigID())
      {
        case sid_READ_SVAR_RECORD:
             {
               const SV_key key = request->get__READ_SVAR_RECORD__key();
               const Svar_record * svar = db.find_var(key, LOC);

               if (svar)
                  {
                    string data((const char *)svar, sizeof(Svar_record));
                    { SVAR_RECORD_IS_c(fd, data); }
                  }
               else
                  {
                    char dummy[sizeof(Svar_record)];
                    memset(&dummy, 0, sizeof(Svar_record));
                    string data((const char *)&dummy, sizeof(Svar_record));
                    { SVAR_RECORD_IS_c(fd, data); }
                  }
             }
             return;

        case sid_REGISTER_PROCESSOR:
             {
               // find connected_procs entry for fd...
               //
               if (ap_fd == 0)   // not found
                  {
                    cerr << prog << ": could not find fd " << fd
                         << " in connected_procs" << endl;
                    close_fd(fd);
                    return;
                  }

               const int proc   = request->get__REGISTER_PROCESSOR__proc();
               const int parent = request->get__REGISTER_PROCESSOR__parent();
               const int grand  = request->get__REGISTER_PROCESSOR__grand();
               const bool evconn =  request->get__REGISTER_PROCESSOR__evconn();
               const AP_num3 ap3((AP_num)proc, (AP_num)parent, (AP_num)grand);

               if (evconn)
                  {
                    for (int j = 0; j < connected_procs.size(); ++j)
                        {
                          AP3_fd & ap_fd1 = connected_procs[j];
                          if (ap_fd1.ap3 == ap3)
                             {
                               ap_fd1.fd2 = fd;
                               connected_procs.erase(connected_procs.begin()
                                           + (ap_fd - &connected_procs[0]));
                               return;
                             }
                        }

                    cerr << "*** NOT FOUND at " << LOC << "***" << endl;
                  }
               ap_fd->ap3 = ap3;
               ap_fd->progname = request->get__REGISTER_PROCESSOR__progname();
             }
             return;

        case sid_MATCH_OR_MAKE:
             {
               uint32_t varname[MAX_SVAR_NAMELEN];
               memcpy(varname,
                      request->get__MATCH_OR_MAKE__varname().data(),
                      sizeof(varname));
               const int to_proc   = request->get__MATCH_OR_MAKE__to_proc();
               const int to_parent = request->get__MATCH_OR_MAKE__to_parent();
               const int to_grand  = request->get__MATCH_OR_MAKE__to_grand();
               const int fr_proc   = request->get__MATCH_OR_MAKE__from_proc();
               const int fr_parent = request->get__MATCH_OR_MAKE__from_parent();
               const int fr_grand  = request->get__MATCH_OR_MAKE__from_grand();

               const AP_num3 to((AP_num)to_proc,
                                (AP_num)to_parent,
                                (AP_num)to_grand);

               const AP_num3 fr((AP_num)fr_proc,
                                (AP_num)fr_parent,
                                (AP_num)fr_grand);

               maybe_start_AP(to);

               Svar_partner from(fr, fd);
               TCP_socket tcp2 = get_tcp_fd2_for_id(from.id);
               SV_key key = db.match_or_make(varname, to, from, tcp2);
               MATCH_OR_MAKE_RESULT_c(fd, key);
             }
             return;

        case sid_GET_EVENTS:
             {
               Svar_event events = SVE_NO_EVENTS;
               const int proc   = request->get__GET_EVENTS__proc();
               const int parent = request->get__GET_EVENTS__parent();
               const int grand  = request->get__GET_EVENTS__grand();
               const AP_num3 ap3((AP_num)proc, (AP_num)parent, (AP_num)grand);
               const SV_key key = db.get_events(events, ap3);
               EVENTS_ARE_c(fd, key, events);
             }
             return;

        case sid_CLEAR_ALL_EVENTS:
             {
               const int proc   = request->get__CLEAR_ALL_EVENTS__proc();
               const int parent = request->get__CLEAR_ALL_EVENTS__parent();
               const int grand  = request->get__CLEAR_ALL_EVENTS__grand();
               const AP_num3 ap3((AP_num)proc, (AP_num)parent, (AP_num)grand);
               const Svar_event events = db.clear_all_events(ap3);
               EVENTS_ARE_c(fd, 0, events);
             }
             return;

        case sid_ADD_EVENT:
             {
               const SV_key key = request->get__ADD_EVENT__key();
               const int proc   = request->get__ADD_EVENT__proc();
               const int parent = request->get__ADD_EVENT__parent();
               const int grand  = request->get__ADD_EVENT__grand();
               const int event  = request->get__ADD_EVENT__event();
               const AP_num3 ap3((AP_num)proc, (AP_num)parent, (AP_num)grand);
              db.add_event(key, ap3, (Svar_event)event);
             }
             return;

        case sid_IS_REGISTERED_ID:
             {
               const int proc   = request->get__IS_REGISTERED_ID__proc();
               const int parent = request->get__IS_REGISTERED_ID__parent();
               const int grand  = request->get__IS_REGISTERED_ID__grand();
               const AP_num3 ap3((AP_num)proc, (AP_num)parent, (AP_num)grand);
               int is_registered = 0;
               for (int p = 0; p < connected_procs.size(); ++p)
                   {
                     const AP3_fd & pro = connected_procs[p];
                     if (pro.ap3 == ap3)
                        {
                          is_registered = 1;
                          break;
                        }
                   }
               YES_NO_c(fd, is_registered);
             }
             return;

        case sid_FIND_OFFERING_ID:
             {
               const SV_key key = request->get__FIND_OFFERING_ID__key();
               const AP_num3 offering_id = db.find_offering_id(key);
               OFFERING_ID_IS_c(fd, offering_id.proc,
                                    offering_id.parent,
                                    offering_id.grand);
             }
             return;

        case sid_GET_OFFERING_PROCS:
             {
               const AP_num to_proc = (AP_num)
                            request->get__GET_OFFERING_PROCS__offered_to_proc();
               vector<AP_num> processors;
               db.get_offering_processors(to_proc, processors);
               string sprocs((const char *)processors.data(),
                             processors.size()*sizeof(AP_num));
               OFFERING_PROCS_ARE_c(fd, sprocs);
             }
             return;

        case sid_GET_OFFERED_VARS:
             {
               const AP_num to_proc = (AP_num)
                            request->get__GET_OFFERED_VARS__offered_to_proc();
               const AP_num from_proc = (AP_num)
                            request->get__GET_OFFERED_VARS__accepted_by_proc();
               vector<uint32_t> varnames;
               db.get_offered_variables(to_proc, from_proc, varnames);
               string svars((const char *)varnames.data(),
                            varnames.size()*sizeof(uint32_t));
               OFFERED_VARS_ARE_c(fd, svars);
             }
             return;

        case sid_FIND_PAIRING_KEY:
             {
               const SV_key key = request->get__FIND_PAIRING_KEY__key();
               const SV_key pairing_key = db.find_pairing_key(key);
               PAIRING_KEY_IS_c(fd, pairing_key);
             }
             return;

        case sid_PRINT_SVAR_DB:
             {
               ostringstream out;
               print_db(out);
               SVAR_DB_PRINTED_c(fd, out.str());
             }
             return;

        case sid_ASSIGN_WSWS_VAR:
             {
               const SV_key key = request->get__ASSIGN_WSWS_VAR__key();
               debug && *debug << "writing var data for key 0x"
                               << hex << key << dec << endl;

               bool existing = false;
               for (int vv = 0; vv < var_values.size(); ++vv)
                   {
                     if (var_values[vv].key == key)
                        {
                          existing = true;
                          var_values[vv].var_value =
                                    request->get__ASSIGN_WSWS_VAR__cdr_value();
                          break;
                        }
                   }

               if (!existing)   // new variable
                  {
                    // in order to avoid unnecessary copying of the new_value
                    // we first append key with an empty string and then
                    // update its var_value
                    //
                    const key_value kv(key);
                    var_values.push_back(kv);
                    var_values.back().var_value =
                                    request->get__ASSIGN_WSWS_VAR__cdr_value();
                  }

               Svar_record * svar = db.find_var(key, LOC);
               if (svar) svar->set_state(false, "APserver 524");
               else cerr << "*** key not in db" << endl;
             }
             return;

        case sid_READ_WSWS_VAR:
             {
               const SV_key key = request->get__READ_WSWS_VAR__key();
               debug && *debug << "reading var data for key 0x"
                               << hex << key << dec << endl;

               bool existing = false;
               for (int vv = 0; vv < var_values.size(); ++vv)
                   {
                     if (var_values[vv].key == key)
                        {
                          existing = true;
                          WSWS_VALUE_IS_c(fd, var_values[vv].var_value);

                          Svar_record * svar = db.find_var(key, LOC);
                          if (svar) svar->set_state(true, LOC);
                          else cerr << "*** key not in db at " << LOC << endl;
                          return;
                        }
                   }
               cerr << "*** data not in db at " << LOC << endl;
               string empty;
               WSWS_VALUE_IS_c(fd, empty);
             }
             return;

        case sid_RETRACT_VAR:     // ⎕SVR varname
             {
               const SV_key key = request->get__RETRACT_VAR__key();
               Svar_record * svar = db.find_var(key, LOC);
               if (svar)
                  {
                    svar->retract();
                    if (svar->get_coupling() != NO_COUPLING &&
                        svar->offering.id.proc < AP_FIRST_USER)
                       {
                    const TCP_socket sock = get_TCP_for_key(key);

                         RETRACT_OFFER_c(sock, key);
                         return;
                       }
                  }
               else cerr << "*** key not in db at " << LOC << endl;
             }
             return;

        case sid_ASSIGN_VALUE:     // Svar←X for APs
	     {
               const SV_key key = request->get__ASSIGN_VALUE__key();
               const TCP_socket ap_sock = get_peer_fd2(key, fd);
               if (ap_sock == NO_TCP_SOCKET)
                  {
                    cerr << "NO tcp2 at " << LOC << endl;
                    print_db(cerr);
                      return;
                  }

               ASSIGN_VALUE_c(ap_sock, key,
                              request->get__ASSIGN_VALUE__cdr_value());
             }
             return;

        case sid_SVAR_ASSIGNED:     // Svar←X response from AP
             {
               const SV_key key = request->get__SVAR_ASSIGNED__key();
               const TCP_socket apl = get_peer_fd(key, fd);
               SVAR_ASSIGNED_c sva(apl, key,
                                   request->get__SVAR_ASSIGNED__error(),
                                   request->get__SVAR_ASSIGNED__error_loc());
             }
             return;

        case sid_MAY_USE: 
             {
               const SV_key key  = request->get__MAY_USE__key();
               const int attempt = request->get__MAY_USE__attempt();
               bool allowed = true;
               Svar_record * svar = db.find_var(key, LOC);
               if (svar)   allowed = svar->may_use(attempt);

               // it can be that the control vector allows reading but no value
               // has been set yet. In that case we do not allow use of the
               // variable
               //
               if (allowed && svar && svar->is_ws_to_ws())
                  {
                    bool have_value = false;
                    for (int vv = 0; vv < var_values.size(); ++vv)
                        {
                          if (var_values[vv].key == key)
                             {
                               have_value = true;
                               break;
                             }
                        }

                    if (!have_value)   allowed = false;
                  }

               YES_NO_c(fd, allowed);
             }
             return;

        case sid_MAY_SET: 
             {
               const SV_key key  = request->get__MAY_SET__key();
               const int attempt = request->get__MAY_SET__attempt();
               bool allowed = true;
               Svar_record * svar = db.find_var(key, LOC);
               if (svar)   allowed = svar->may_set(attempt);

               YES_NO_c(fd, allowed);
             }
             return;

        case sid_GET_VALUE:     // +Svar for APs
             {
               const SV_key key = request->get__GET_VALUE__key();
               const TCP_socket ap_sock = get_peer_fd2(key, fd);
               if (ap_sock == NO_TCP_SOCKET)
                  {
                    cerr << "NO tcp2 at " << LOC << endl;
                    print_db(cerr);
                      return;
                  }

               GET_VALUE_c(ap_sock, key);

               char * del = 0;
               char buffer[2*MAX_SIGNAL_CLASS_SIZE + 40000];
               Signal_base * response = Signal_base::recv_TCP(ap_sock, buffer,
                                                  sizeof(buffer), del, debug);
               VALUE_IS_c vis(fd, response->get__VALUE_IS__key(),
                                  response->get__VALUE_IS__error(),
                                  response->get__VALUE_IS__error_loc(),
                                  response->get__VALUE_IS__cdr_value());

               if (del)   delete del;
             }
             return;

        case sid_SET_STATE:
             {
               const SV_key key = request->get__SET_STATE__key();
               Svar_record * svar = db.find_var(key, LOC);

               if (svar) svar->set_state(request->get__SET_STATE__new_state(),
                                   request->get__SET_STATE__loc().c_str());
               else cerr << "*** key not in db at " << LOC << endl;
             }
             return;

        case sid_SET_CONTROL:
             {
               const SV_key key = request->get__SET_CONTROL__key();
               Svar_record * svar = db.find_var(key, LOC);

               if (svar) svar->set_control((Svar_Control)request->
                                            get__SET_CONTROL__new_control());
               else cerr << "*** key not in db at " << LOC << endl;
             }
             return;

        default: break;
      }

   cerr << prog_name() << ": got unexpected signal ID "
        << request->get_sigID() << " ("
        << request->get_sigName() << ")" << endl;
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
         (verbosity > 0) && cerr << prog << ": connection[" << fd
                                 << "] closed by peer" << endl;
         close_fd(fd);
         return;
      }

   do_signal(fd, request);

   if (del)   delete del;
}
//-----------------------------------------------------------------------------
#ifdef HAVE_SYS_UN_H
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
   return -1;
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
void
usage()
{
   cerr <<
"usage: " << prog << " [options]\n"
"    options: \n"
"    -h, --help             print this help\n"
"    -H                     hang on (after last connection was closed)\n"
"    -d                     same as -v --path default-unix-sockname\n"
"    --path <unix-sock>     listen on unix-sock\n"
"    --port <tcp-port>      listen on TCP port tcp-port\n"
"    -v                     be verbose\n";

}
//-----------------------------------------------------------------------------
int
main(int argc, char * argv[])
{
   prog = argv[0];

int listen_port = APSERVER_PORT;
const char * listen_name = APSERVER_PATH;
bool got_path = false;
bool got_port = false;
bool auto_start = false;
int janitor = 0;

   for (int a = 1; a < argc; )
       {
         const char * opt = argv[a++];
         const char * val = (a < argc) ? argv[a] : 0;

         if (!strcmp(opt, "-h"))
            {
              usage();
              return 0;
            }

         if (!strcmp(opt, "--help"))
            {
              usage();
              return 0;
            }

         if (!strcmp(opt, "--auto"))
            {
              auto_start = true;
            }
         else if (!strcmp(opt, "-H"))
            {
              hang_on = true;
            }
         else if (!strcmp(opt, "-d"))
            {
              cerr << "APSERVER_TRANSPORT is: " << APSERVER_TRANSPORT << endl;

#if APSERVER_TRANSPORT == 0   // TCP
              cerr << "APSERVER_PORT is: " << APSERVER_PORT << endl;
               got_port = true;
#else                         // AF_UNIX
              cerr << "APSERVER_PATH is: " << APSERVER_PATH << endl;
               got_path = true;
#endif
              ++verbosity;
              debug = &cerr;
              LOG_shared_variables = true;
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
              debug = &cerr;
              LOG_shared_variables = true;
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

   // enable ^C and ^\ when in debig mode
   //
   memset(&new_control_C_action, 0, sizeof(struct sigaction));
   memset(&new_control_BSL_action, 0, sizeof(struct sigaction));

   if (verbosity > 0)   // debug mode
      {
        new_control_C_action.sa_handler = &control_C;
        new_control_BSL_action.sa_handler = &control_BSL;
      }
   else                 // default mode
      {
        new_control_C_action.sa_handler   = SIG_IGN;
        new_control_BSL_action.sa_handler = SIG_IGN;
      }

   sigaction(SIGINT,  &new_control_C_action,   &old_control_C_action);
   sigaction(SIGQUIT, &new_control_BSL_action, &old_control_BSL_action);

   if (verbosity > 0)
      cerr << "sizeof(Svar_DB_server) is " << sizeof(Svar_DB_server) << endl
           << "sizeof(Svar_record) is    " << sizeof(Svar_record) << endl
           << "sizeof(Svar_partner) is   " << sizeof(Svar_partner)
           << endl;

const int listen_sock = got_path ? open_UNIX_socket(listen_name)
                                 : open_TCP_socket(listen_port);
   if (listen_sock < 0)   return 20;

   fclose(stdout);                 // cause getc() of caller to return EOF !

   if (auto_start && fork())   return 0;         // parent returns (daemonize)

   memset(&db, 0, sizeof(db));

int max_fd = listen_sock;

   (verbosity > 0) && cerr << prog << ": entering main loop..." << endl;
   for (;;)
      {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        errno = 0;

#ifdef USE_POLL // use poll()

        DynArray(struct pollfd, fds, 2*connected_procs.size() + 1);
        fds[0].fd = listen_sock;
        fds[0].events = POLLIN | POLLPRI;
        int fd_idx = 1;
        for (int j = 0; j < connected_procs.size(); ++j)
            {
              TCP_socket fd = connected_procs[j].fd;
              if (fd != NO_TCP_SOCKET)
                 {
                   fds[fd_idx].fd = fd;
                   fds[fd_idx].events = POLLIN | POLLPRI;
                   ++fd_idx;
                 }

              fd = connected_procs[j].fd2;
              if (fd != NO_TCP_SOCKET)
                 {
                   fds[fd_idx].fd = fd;
                   fds[fd_idx].events = POLLIN | POLLPRI;
                   ++fd_idx;
                 }
            }

         int ret = poll(fds, fd_idx, -1);
         if (ret <= 0)
            {
             if (errno == EINTR)   continue;

              cerr << prog << ": *** poll() returned unexpected "
                   << ret << ": " << strerror(errno) << endl;
              continue;
            }

         // emulate select()
         //
         for (int i = 0; i < fd_idx; ++i)
             {
               if (fds[i].revents & (POLLIN | POLLPRI | POLLERR | POLLHUP))
                  FD_SET(fds[i].fd, &read_fds);
             }

#else // use select()

        FD_SET(listen_sock, &read_fds);
        for (int j = 0; j < connected_procs.size(); ++j)
            {
              TCP_socket fd = connected_procs[j].fd;
              if (fd != NO_TCP_SOCKET)
                 {
                   if (max_fd < fd)   max_fd = fd;
                   FD_SET(fd, &read_fds);
                 }

              fd = connected_procs[j].fd2;
              if (fd != NO_TCP_SOCKET)
                 {
                   if (max_fd < fd)   max_fd = fd;
                   FD_SET(fd, &read_fds);
                 }
            }

        timeval timeout = { 1, 0 };   // one second

        const int count = select(max_fd + 1, &read_fds, 0, 0, &timeout);
        if (count < 0)
           {
             if (errno == EINTR)   continue;

             cerr << prog << ": count <= 0 in select(): "
                  << strerror(errno) << endl;
             return 3;
           }

        if (count == 0)   // timeout
           {
             continue;   // until we find a method to detect a closed socket

             if (connected_procs.size() == 0)   continue;
             ++janitor;
             if (janitor >= connected_procs.size())   janitor = 0;

             int jfd = connected_procs[janitor].fd;
             if (jfd != NO_TCP_SOCKET)
                {
                  const ssize_t result = ::recv(jfd, 0, 0, MSG_DONTWAIT);
                  cerr << "janitor got " << errno << " on fd " << jfd << endl;
                }

             jfd = connected_procs[janitor].fd2;
             if (jfd != NO_TCP_SOCKET)
                {
                  const ssize_t result = ::recv(jfd, 0, 0, MSG_DONTWAIT);
                  cerr << "janitor got " << errno << " on fd " << jfd << endl;
                }
             
             continue;
           }
#endif
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
void
print_db(ostream & out)
{
  // print active processors
   //
   out << "╔══════════════════════┬════════┬══════════════════─═══╗" << endl
       << "║ Processor            │ fd fd2 │ Program              ║" << endl
       << "╠══════════════════════╪════════╪══════════════════════╣" << endl;
   for (int p = 0; p < connected_procs.size(); ++p)
       {
         const AP3_fd & pro = connected_procs[p];
         char cc[100];
         if (pro.ap3.grand)
              {
                snprintf(cc, sizeof(cc), "%u.%u.%u",
                         pro.ap3.proc, pro.ap3.parent, pro.ap3.grand);
              }
         else if (pro.ap3.parent)
              {
                snprintf(cc, sizeof(cc), "%u.%u", pro.ap3.proc,
                         pro.ap3.parent);
              }
         else
              {
                snprintf(cc, sizeof(cc), "%u", pro.ap3.proc);
              }
         cc[sizeof(cc) - 1] = 0;
         out << "║ "  << left  << setw(20) << cc;
               
         out << " │" << right << setw(3) << pro.fd << " " << setw(3) << pro.fd2
             << " │ " << left  << setw(20) << pro.progname
             << " ║"  << right << endl;
       }
   out << "╚══════════════════════╧════════╧══════════════════════╝" << endl;

   // print shared variables
   out <<
"╔═════╤═╦═══════════╤═══╤══╦═══════════╤═══╤══╦════╤══════════╗\n"
"║     │ ║ Offering  │   │  ║ Accepting │   │  ║OAOA│          ║\n"
"║ Seq │C║ Proc,par  │Fd2│Fl║ Proc,par  │Fd2│Fl║SSUU│ Varname  ║\n"
"╠═════╪═╬═══════════╪═══╪══╬═══════════╪═══╪══╬════╪══════════╣\n";
   for (int o = 0; o < db.offered_vars.size(); ++o)
       {
         const Svar_record & svar = db.offered_vars[o];
         if (svar.valid())   svar.print(out);
       }

   out <<
"╚═════╧═╩═══════════╧═══╧══╩═══════════╧═══╧══╩════╧══════════╝\n"
       << endl;

}
//-----------------------------------------------------------------------------
ostream & operator << (ostream & out, const AP_num3 & ap3)
{
   return out << ap3.proc << "." << ap3.parent << "." << ap3.grand;
}
//-----------------------------------------------------------------------------
ostream &
operator << (ostream & os, Unicode uni)
{
   if (uni < 0x80)      return os << (char)uni;

   if (uni < 0x800)     return os << (char)(0xC0 | (uni >> 6))
                                  << (char)(0x80 | (uni & 0x3F));

   if (uni < 0x10000)    return os << (char)(0xE0 | (uni >> 12))
                                   << (char)(0x80 | (uni >>  6 & 0x3F))
                                   << (char)(0x80 | (uni       & 0x3F));

   if (uni < 0x200000)   return os << (char)(0xF0 | (uni >> 18))
                                   << (char)(0x80 | (uni >> 12 & 0x3F))
                                   << (char)(0x80 | (uni >>  6 & 0x3F))
                                   << (char)(0x80 | (uni       & 0x3F));

   if (uni < 0x4000000)  return os << (char)(0xF8 | (uni >> 24))
                                   << (char)(0x80 | (uni >> 18 & 0x3F))
                                   << (char)(0x80 | (uni >> 12 & 0x3F))
                                   << (char)(0x80 | (uni >>  6 & 0x3F))
                                   << (char)(0x80 | (uni       & 0x3F));

   return os << (char)(0xFC | (uni >> 30))
             << (char)(0x80 | (uni >> 24 & 0x3F))
             << (char)(0x80 | (uni >> 18 & 0x3F))
             << (char)(0x80 | (uni >> 12 & 0x3F))
             << (char)(0x80 | (uni >>  6 & 0x3F))
             << (char)(0x80 | (uni       & 0x3F));
}
//-----------------------------------------------------------------------------

