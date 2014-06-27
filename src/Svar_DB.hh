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

#ifndef __SVAR_DB_HH_DEFINED__
#define __SVAR_DB_HH_DEFINED__

#include "Common.hh"
#include "ProcessorID.hh"
#include "Svar_DB_memory.hh"

#include <stdint.h>
#include <unistd.h>

#include <iostream>

using namespace std;

// #define USE_APserver

//-----------------------------------------------------------------------------

/// a pointer to the entire shared Svar_DB_memory.
class Svar_DB_memory_P
{
public:
   Svar_DB_memory_P(bool read_only);

   ~Svar_DB_memory_P();

   /// connect to APserver
   static void connect_to_APserver(const char * bin_path, bool logit);

   static void disconnect()
      {
        if (DB_tcp != NO_TCP_SOCKET)
           { ::close(DB_tcp);   DB_tcp = NO_TCP_SOCKET; }
      }

   static bool APserver_available()
      { return has_memory() && DB_tcp != NO_TCP_SOCKET; }

   static bool has_memory()
      { return memory_p != 0; }

   static TCP_socket get_DB_tcp()
      { return DB_tcp; }

   static void DB_tcp_error(const char * op, int got, int expected);

   const Svar_DB_memory * operator->() const
      { Assert(memory_p); return memory_p; }

   Svar_DB_memory * operator->()
      { Assert(!read_only && memory_p); return memory_p; }

   static Svar_DB_memory * memory_p;

   static Svar_DB_memory cache;

protected:
   bool read_only;

   static TCP_socket DB_tcp;

   static uint16_t APserver_port;

private:
   /// don't copy...
  Svar_DB_memory_P(const Svar_DB_memory_P & other);

   /// don't copy...
   Svar_DB_memory_P & operator =(const Svar_DB_memory_P & other);
};
//-----------------------------------------------------------------------------
/// a pointer to one record (one shared variable) of the shared Svar_DB_memory
class offered_SVAR_P
{
public:
   offered_SVAR_P(bool read_only, SV_key key);

   ~offered_SVAR_P();

   const offered_SVAR * operator->() const
      { return offered_svar_p; }

   offered_SVAR * operator->()
      { return offered_svar_p; }

protected:
   bool read_only;

   static offered_SVAR * offered_svar_p;

   static offered_SVAR cache;

private:
   /// don't copy...
  offered_SVAR_P(const offered_SVAR_P & other);

   /// don't copy...
   offered_SVAR_P & operator =(const offered_SVAR_P & other);
};
//-----------------------------------------------------------------------------
#define READ_SVDB(open_act, closed_act)                 \
   if (Svar_DB_memory_P::has_memory())                  \
        { const Svar_DB_memory_P db(true); open_act }   \
   else { closed_act }

#define UPD_SVDB(open_act, closed_act)                  \
   if (Svar_DB_memory_P::has_memory())                  \
        { Svar_DB_memory_P db(false); open_act }        \
   else { closed_act }

#ifdef USE_APserver

# define READ_RECORD(key, open_act, closed_act)               \
    if (Svar_DB_memory_P::has_memory())                       \
         { const offered_SVAR_P svar(true, key); open_act }   \
    else { closed_act }

# define UPD_RECORD(key, open_act, closed_act)           \
    if (Svar_DB_memory_P::has_memory())                  \
         { offered_SVAR_P svar(false, key); open_act }   \
    else { closed_act }

#else // database in shared memory (no APserver)

# define READ_RECORD(key, open_act, closed_act)               \
    if (Svar_DB_memory_P::has_memory())                       \
{ const Svar_DB_memory_P db(true);                            \
  const offered_SVAR * svar = db->find_var(key); open_act }   \
    else { closed_act }

# define UPD_RECORD(key, open_act, closed_act)          \
    if (Svar_DB_memory_P::has_memory())                 \
{ Svar_DB_memory_P db(false);                           \
  offered_SVAR * svar = db->find_var(key); open_act }   \
    else { closed_act }

#endif

/// a database in shared memory that contains all shared variables on
/// this machine
class Svar_DB
{
public:
   Svar_DB()
   {}

   /// remove all offers from this process
   ~Svar_DB();

   /// open (and possibly initialize) the shared variable database
   static void init(const char * progname, bool logit, bool do_svars);

   /// open (and possibly initialize) the shared variable database
   void open_shared_memory(const char * progname, bool logit, bool do_svars);

   /// match a new offer against the DB. Return: 0 on error, 1 if the
   /// new offer was inserted into the DB (sicne no match was found), or
   /// 2 if a pending offer was found. In the latter case, the pending offer
   /// was removed and its relevant data returned through the d_... args.
   static offered_SVAR * match_or_make(const uint32_t * UCS_varname,
                                       const AP_num3 & to,
                                       const Svar_partner & from,
                                       SV_Coupling & coupling)
      {
        offered_SVAR * svar;
        UPD_SVDB(svar = db->match_or_make(UCS_varname, to,
                  from, coupling); , svar = 0; )
        return svar;
      }

   /// match_or_make() called from APmain.cc
   static void match_or_make(SV_key key, const Svar_partner & from,
                             SV_Coupling & coupling)
      {
        offered_SVAR * svar = find_var(key);
        match_or_make(svar->varname, svar->offering.id, from, coupling);
        svar->offering.flags &= ~OSV_OFFER_SENT;
      }

   /// register processor in the database
   static void register_processor(const Svar_partner & svp)
      {
        UPD_SVDB(db->register_processor(svp); , )
      }

   /// return the partner with id \b id
   static Svar_partner_events get_registered(const AP_num3 & id)
      {
        Svar_partner_events svp;
        READ_SVDB(svp = db->get_registered(id); , svp.clear();)
        return svp;
      }

   /// return true if \b id is registered
   static bool is_registered(const AP_num3 & id)
      {
        const Svar_partner_events svp = get_registered(id);
        return svp.partner.id.proc > AP_NULL;   // NO_AP or AP_NULL
      }

   /// return true if \b id is not used by any registered proc, parent, or grand
   static bool is_unused_id(AP_num id)
      {
        READ_SVDB(return db->is_unused_id(id); , return false; )
      }

   /// return an id >= AP_FIRST_USER that is not mentioned in any registered
   /// processor (i,e, not proc, parent. or grand)
   static AP_num get_unused_id()
      {
        READ_SVDB(return db->get_unused_id(); , return NO_AP; )
      }

   /// return the UDP port for communication with AP \b proc
   static uint16_t get_udp_port(AP_num proc, AP_num parent)
      {
        READ_SVDB(return db->get_udp_port(proc, parent); , return 0; )
      }

   /// unregister processor in the database
   static void unregister_processor(const Svar_partner & p)
      {
        UPD_SVDB(db->unregister_processor(p); , )
      }

   /// retract an offer, return previous coupling
   static SV_Coupling retract_var(SV_key key)
      {
        UPD_RECORD(key, return svar->retract(); , return NO_COUPLING; )
      }

   /// return pointer to varname or 0 if key does not exist
   static const uint32_t * get_varname(SV_key key)
      {
        READ_RECORD(key, return svar->get_varname(); , return 0; )
      }

   /// find variable with key \b key
   static offered_SVAR * find_var(SV_key key)
      {
        UPD_SVDB(return db->find_var(key); , return 0; )
      }

   /// add processors with pending offers to \b processors. Duplicates
   /// are OK and will be removed later
   static void get_processors(int to_proc, vector<int32_t> & processors)
      {
        UPD_SVDB(db->get_processors(to_proc, processors); ,)
      }

   /// return all variables shared between \b to_proc and \b from_proc
   static void get_variables(int to_proc, int from_proc,
                             vector<const uint32_t *> & vars)
      {
        READ_SVDB(db->get_variables(to_proc, from_proc, vars); , )
      }

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
   static Svar_Control set_control(SV_key key, Svar_Control ctl)
      {
        UPD_RECORD(key, svar->set_control(ctl); return svar->get_control(); ,
                                                return NO_SVAR_CONTROL; )
      }

   /// return the current state of this variable
   static Svar_state get_state(SV_key key)
      {
        READ_RECORD(key, return svar->get_state(); , return SVS_NOT_SHARED; )
      }

   static int data_owner_port(SV_key key)
      {
        READ_RECORD(key, return svar->data_owner_port(); , return -1; )
      }

   static bool may_set(SV_key key, int attempt)
      {
        READ_RECORD(key, return svar->may_set(attempt); , return true; )
      }

   static bool may_use(SV_key key, int attempt)
      {
        READ_RECORD(key, return svar->may_use(attempt); , return true; )
      }

   /// set the current state of this variable
   static void set_state(SV_key key, bool used, const char * loc)
      {
        UPD_RECORD(key, svar->set_state(used, loc); , )
      }

   /// return the share partner
   static Svar_partner get_peer(SV_key key)
      {
        Svar_partner peer;
        UPD_RECORD(key, peer = svar->get_peer(); , peer.clear(); )
        return peer;
      }

   /// some shared variables belong to a pair, like CTL and DAT in AP210.
   /// return the key of the other variable (as determined by the compare()
   /// function procided), of 0 if the key or the other variable don't exist
   static SV_key pairing_key(SV_key key,
          bool (*compare)(const uint32_t * v1, const uint32_t * v2))
      {
        READ_SVDB(return db->pairing_key(key, compare); , return 0; )
      }

   /// clear all events, return the current event bitmap.
   static Svar_event clear_all_events()
      {
        UPD_SVDB(return db->clear_all_events(); , return SVE_NO_EVENTS; )
      }

   /// return events for processor \b proc
   static SV_key get_events(Svar_event & events, AP_num3 proc)
      {
        READ_SVDB(return db->get_events(events, proc); , return 0;)
      }

   /// set an event for proc (and maybe also for key)
   static void add_event(Svar_event event, AP_num3 proc, SV_key key)
      {
        UPD_SVDB(db->add_event(event, proc, key); , )
      }

   /// print the database
   static void print(ostream & out);
};

#endif // __SVAR_DB_HH_DEFINED__
