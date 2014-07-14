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

#ifndef __SVAR_DB_MEMORY_HH_DEFINED__
#define __SVAR_DB_MEMORY_HH_DEFINED__

#include <stdint.h>

#include "config.h"   // for HAVE_ macros
#include "Svar_record.hh"

//-----------------------------------------------------------------------------
/// the Svar database in the server
struct Svar_DB_server
{
   /// see Svar_DB::match_or_make()
   SV_key match_or_make(const uint32_t * UCS_varname,
                        const AP_num3 & to,
                        const Svar_partner & from,
                        SV_Coupling & coupling);

   /// register partner \b p in this database
   void register_processor(const Svar_partner & p);

   /// return the registered processor \b proc
   uint16_t get_udp_port_for_id(const AP_num3 & id) const;

   /// return true if \b id is not used by any registered proc, parent, or grand
   bool is_unused_id(AP_num id) const;

   /// return the UDP port number for processor \b proc with parent \b parent
   uint16_t get_udp_port(AP_num proc, AP_num parent) const;

   /// remove \b p from this database
   void unregister_processor(const AP_num3 & id);

   /// remove stale variables using \b proc
   void remove_stale();

   /// see Svar_DB::retract_all()
   void retract_all(const AP_num3 & id);

   /// see Svar_DB::get_offering_processors()
   void get_offering_processors(AP_num to_proc, vector<AP_num> & processors);

   /// see Svar_DB::get_offered_variables()
   void get_offered_variables(AP_num to_proc, AP_num from_proc,
                      vector<uint32_t> & varnames) const;

   /// see Svar_DB::add_event()
   void add_event(SV_key key, AP_num3 proc, Svar_event event);

   /// see Svar_DB::clear_all_events()
   Svar_event clear_all_events(AP_num3 id);

   /// see Svar_DB::get_events()
   SV_key get_events(Svar_event & events, AP_num3 proc) const;

   /// see Svar_DB::pairing_key()
   SV_key find_pairing_key(SV_key key) const;

   /// compare function to find AP210 CTL/DAT or Cnnn/Dnnn varname pairs
   static bool compare_ctl_dat_etc(const uint32_t * ctl, const uint32_t * dat);

   /// see Svar_DB::get_peer()
   Svar_partner get_peer(SV_key key);

   /// try to match an existing offer, return a pointer to it if found and
   /// initialize the accepting side of the offer
   Svar_record * match_pending_offer(const uint32_t * UCS_varname,
                                      const AP_num3 & to,
                                      const Svar_partner & from);

   /// create a new offer in the DB, return a pointer to it, or 0 if the
   /// Svar_DB is full
   Svar_record * create_offer(const uint32_t * UCS_varname,
                               const AP_num3 & to, const Svar_partner & from);

   /// find the variable with key \b key
   Svar_record * find_var(SV_key key) const;

   /// find offering processor id of the variable with key \b key
   AP_num3 find_offering_id(SV_key key) const;

   /// a sequence number helping to create keys
   uint32_t seq;

   /// the offered variables
   Svar_record offered_vars[MAX_SVARS_OFFERED];

   /// processors registered in this database
   Svar_partner active_processors[MAX_ACTIVE_PROCS];
};
//-----------------------------------------------------------------------------

#endif // __SVAR_DB_MEMORY_HH_DEFINED__
