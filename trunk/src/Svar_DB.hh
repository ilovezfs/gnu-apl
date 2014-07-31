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

#ifndef __SVAR_DB_HH_DEFINED__
#define __SVAR_DB_HH_DEFINED__

#include "Common.hh"
#include "ProcessorID.hh"
#include "Svar_record.hh"

#include <stdint.h>
#include <unistd.h>

#include <iostream>

using namespace std;

#if APSERVER_TRANSPORT == 1
# define ABSTRACT_OFFSET 1
#else
# define ABSTRACT_OFFSET 0
#endif

//-----------------------------------------------------------------------------
/// a pointer to one record (one shared variable) of the shared Svar_DB_memory
class Svar_record_P
{
public:
   Svar_record_P(bool read_only, SV_key key);

   ~Svar_record_P();

   const Svar_record * operator->() const
      { return offered_svar_p; }

   Svar_record * operator->()
      { return offered_svar_p; }

protected:
   bool read_only;

   static Svar_record * offered_svar_p;

   static Svar_record cache;

private:
   /// don't copy...
  Svar_record_P(const Svar_record_P & other);

   /// don't copy...
   Svar_record_P & operator =(const Svar_record_P & other);
};
//-----------------------------------------------------------------------------
# define READ_RECORD(key, open_act, closed_act)             	\
    if (Svar_DB::APserver_available())                 \
         { const Svar_record_P svar(true, key); open_act }   	\
    else { closed_act }

/// access to a database in APserver that contains all shared variables on
/// this machine
class Svar_DB
{
public:
   /// open (and possibly initialize) the shared variable database
   static void init(const char * bin_path, const char * prog,
                    bool logit, bool do_svars);

   /// open (and possibly initialize) the shared variable database
   void open_shared_memory(const char * progname, bool logit, bool do_svars);

   /// match a new offer against the DB. Return: 0 on error, 1 if the
   /// new offer was inserted into the DB (sicne no match was found), or
   /// 2 if a pending offer was found. In the latter case, the pending offer
   /// was removed and its relevant data returned through the d_... args.
   static SV_key match_or_make(const uint32_t * UCS_varname,
                               const AP_num3 & to, const Svar_partner & from);

   /// return true if \b id is registered in APserver
   static bool is_registered_id(const AP_num3 & id);

   /// get TCP socket to APserver, complain if not connected
   static TCP_socket get_Svar_DB_tcp(const char * calling_function);

   /// retract an offer, return previous coupling
   static void retract_var(SV_key key);

   /// return pointer to varname or 0 if key does not exist
   static const uint32_t * get_varname(SV_key key)
      {
        READ_RECORD(key, return svar->get_varname(); , return 0; )
      }

   /// find ID of the procesdsor that has offered the variable with key \b key
   static AP_num3 find_offering_id(SV_key key);

   /// add processors with pending offers to \b to_proc. Duplicates
   /// are OK and will be removed later
   static void get_offering_processors(AP_num to_proc,
                                       vector<AP_num> & processors);

   /// return all variables shared between \b to_proc and \b from_proc
   static void get_offered_variables(AP_num to_proc, AP_num from_proc,
                                     vector<uint32_t> & varnames);

   /// return coupling of \b entry with \b key.
   static SV_Coupling get_coupling(SV_key key)
      {
        READ_RECORD(key, return svar->get_coupling(); , return NO_COUPLING;)
      }

   /// return the current control vector of this variable
   static Svar_Control get_control(SV_key key)
      {
        READ_RECORD(key, return svar->get_control(); , return NO_SVAR_CONTROL; )
      }

   /// set the current control vector of this variable
   static void set_control(SV_key key, Svar_Control ctl);

   /// return the current state of this variable
   static Svar_state get_state(SV_key key)
      {
        READ_RECORD(key, return svar->get_state(); , return SVS_NOT_SHARED; )
      }

   static bool is_ws_to_ws(SV_key key)
      {
        READ_RECORD(key, return svar->is_ws_to_ws(); , return false; )
      }

   /// return true iff setting the shared variable is allowed
   static bool may_set(SV_key key, int attempt);

   /// return true iff reading the shared variable is allowed
   static bool may_use(SV_key key, int attempt);

   /// set the current state of this variable
   static void set_state(SV_key key, bool used, const char * loc);

   /// some shared variables names belong to a pair, like CTL and DAT in AP210.
   /// return the key of the other variable 
   static SV_key find_pairing_key(SV_key key);

   /// clear all events, return the current event bitmap.
   static Svar_event clear_all_events(AP_num3 id);

   /// return events for processor \b proc
   static SV_key get_events(Svar_event & events, AP_num3 proc);

   /// set an event for proc (and maybe also for key)
   static void add_event(SV_key key, AP_num3 id, Svar_event event);

   /// print the database
   static void print(ostream & out);

   /// return a socket that is connect to APserver
   static TCP_socket connect_to_APserver(const char * bin_path,
                                         const char * prog, bool logit);

   static void disconnect()
      {
        if (DB_tcp != NO_TCP_SOCKET)
           { ::close(DB_tcp);   DB_tcp = NO_TCP_SOCKET; }
      }

   static bool APserver_available()
      { return DB_tcp != NO_TCP_SOCKET; }

   static TCP_socket get_DB_tcp()
      { return DB_tcp; }

   static void DB_tcp_error(const char * op, int got, int expected);

protected:
   /// start an APserver process
   static void start_APserver(const char * server_sockname,
                              const char * bin_dir, bool logit);

   static TCP_socket DB_tcp;

   static uint16_t APserver_port;

private:
   /// don't create...
  Svar_DB();

   /// don't copy...
  Svar_DB(const Svar_DB & other);

   /// don't copy...
   Svar_DB& operator =(const Svar_DB & other);
};

#endif // __SVAR_DB_HH_DEFINED__
