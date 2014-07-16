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
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <iomanip>

#include "Backtrace.hh"
#include "Logging.hh"
#include "main.hh"
#include "ProcessorID.hh"
#include "Svar_DB_server.hh"
#include "Svar_signals.hh"
#include "UdpSocket.hh"

extern ostream & get_CERR();

extern uint16_t get_udp_port_for_id(const AP_num3 & id);
extern int get_tcp_fd_for_id(const AP_num3 & id);

//-----------------------------------------------------------------------------
Svar_event
Svar_DB_server::clear_all_events(AP_num3 id)
{
int ret = SVE_NO_EVENTS;

   // clear event bit in all shared variables
   //
   for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
       {
         Svar_record & svar = offered_vars[o];
         if (id == svar.offering.id)
            {
              ret |= svar.offering.events;
              svar.offering.events = SVE_NO_EVENTS;
            }
         else if (id == svar.accepting.id)
            {
              ret |= svar.accepting.events;
              svar.accepting.events = SVE_NO_EVENTS;
            }
       }

   return (Svar_event)ret;
}
//-----------------------------------------------------------------------------
SV_key
Svar_DB_server::get_events(Svar_event & events, AP_num3 proc) const
{
   // return the first key with an event for proc (if any)
   //
   for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
       {
         const Svar_record & svar = offered_vars[o];
         if (proc == svar.offering.id)
            {
              if (svar.offering.events != SVE_NO_EVENTS)    return svar.key;
            }
         else if (proc == svar.accepting.id)
            {
              if (svar.accepting.events != SVE_NO_EVENTS)   return svar.key;
            }
       }

   return 0;   // no key.
}
//-----------------------------------------------------------------------------
void
Svar_DB_server::add_event(SV_key key, AP_num3 proc, Svar_event event)
{
   if (key == 0)   return;   // no key

Svar_record * svar = find_var(key);
   if (svar)
      {
        if (proc == svar->offering.id)
           {
              svar->offering.events = (Svar_event)
                                      (svar->offering.events | event);
           }
        else if (proc == svar->accepting.id)
           {
              svar->accepting.events = (Svar_event)
                                       (svar->accepting.events | event);
           }
      }
}
//-----------------------------------------------------------------------------
SV_key
Svar_DB_server::find_pairing_key(SV_key key) const
{
const Svar_record * svar1 = find_var(key);
   if (svar1 == 0)   return 0;

   for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
       {
         const Svar_record & svar2 = offered_vars[o];
         if (!svar2.valid())     continue;
         if (svar2.key == key)   continue;   // don't compare svar1 with svar1
          if (compare_ctl_dat_etc(svar1->varname, svar2.varname))
             return svar2.key;
       }

   return 0;   // pairing not found
}
//-----------------------------------------------------------------------------
Svar_record *
Svar_DB_server::find_var(SV_key key) const
{
   if (key == 0)   return 0;

   for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
       {
         const Svar_record * svar = offered_vars + o;
         if (key == svar->key)   return (Svar_record *)svar;
       }

   return 0;
}
//-----------------------------------------------------------------------------
AP_num3
Svar_DB_server::find_offering_id(SV_key key) const
{
Svar_record * svar = find_var(key);

   if (svar)   return svar->offering.id;

AP_num3 not_found(NO_AP, AP_NULL, NO_AP);
   return not_found;
}
//-----------------------------------------------------------------------------
void
Svar_DB_server::get_offering_processors(AP_num to_proc,
                                        vector<AP_num> & processors)
{
vector<AP_num> procs;

   // return pending AND matched offers, general or to to_proc
   //
   for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
       {
         Svar_record & svar = offered_vars[o];
         if (!svar.valid())         continue;

         if (svar.accepting.id.proc == to_proc)   // specific offer
             procs.push_back(svar.offering.id.proc);

         else if (svar.accepting.id.proc == 0)   // general offer
             procs.push_back(svar.offering.id.proc);
       }

   // remove duplicates...
   //
   for (;;)
       {
          // find largest proc
          //
          AP_num largest = AP_NULL;
          loop(p, procs.size())
              {
                if (largest < procs[p])   largest = procs[p];
              }
         if (largest == AP_NULL)   break;
         processors.push_back(largest);
       }
}
//-----------------------------------------------------------------------------
void
Svar_DB_server::get_offered_variables(AP_num to_proc, AP_num from_proc,
                                      vector<uint32_t> & vars) const
{
   // return pending variables but not matched offers, general or to to_proc
   //
   for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
       {
         const Svar_record & svar = offered_vars[o];
         if (!svar.valid())         continue;

         if (svar.accepting.id.proc == to_proc)   // specific offer
            {
              for (const uint32_t * n = svar.varname;; ++n)
                  {
                    vars.push_back(*n);
                    if (*n == 0)   break;
                  }
            }
         else if (svar.accepting.id.proc == 0)    // general offer
            {
              for (const uint32_t * n = svar.varname;; ++n)
                  {
                    vars.push_back(*n);
                    if (*n == 0)   break;
                  }
            }
       }
}
//-----------------------------------------------------------------------------
/// return true iff ctl/dat is CTL/DAT, or Cxxx/Dxxx (or vice versa)
bool
Svar_DB_server::compare_ctl_dat_etc(const uint32_t * ctl, const uint32_t * dat)
{
   if (*ctl == 'D' && *dat == 'C')   // maybe vice versa: change roles
      return compare_ctl_dat_etc(dat, ctl);

   if (*ctl++ != 'C')   return false;   // not CTL or Cxxx
   if (*dat++ != 'D')   return false;   // not DAT or Dxxx

   // check CTL vs. DAT
   //
   if (ctl[0] == 'T' && ctl[1] == 'L' && ctl[2] == 0)
      {
        if (dat[0] != 'A')   return false;
        if (dat[1] != 'T')   return false;
        if (dat[2] == 0)     return true;

        // continue since CTL might match DTL as well
      }
   for (; ; ++ctl, ++dat)
       {
         if (*ctl != *dat)   return false;
         if (*ctl == 0)      return true;
       }

   /* not reached */
   return false;
}
//-----------------------------------------------------------------------------
SV_key
Svar_DB_server::match_or_make(const uint32_t * UCS_varname, const AP_num3 & to,
                              const Svar_partner & from)
{
// CERR << "got offer from ("; from.print(CERR); CERR << ") to " << to << " ";
// Svar_record::print_name(CERR, UCS_varname) << endl;

Svar_record * svar = match_pending_offer(UCS_varname, to, from);
   if (svar)
      {
        Log(LOG_shared_variables)
           {
             get_CERR() << "found pending offer from "
                        << svar->offering.id.proc << " ";
             svar->print_name(get_CERR());
             get_CERR() << " key 0x" << hex << svar->key << dec << endl;
           }

         usleep(50000);
         return svar->key;
      }

usleep(50000);
   svar = create_offer(UCS_varname, to, from);

   Log(LOG_shared_variables)
      {
        get_CERR() << "created new offer:" << endl;
//      Svar_DB::print(get_CERR());
      }

   return svar->key;
}
//-----------------------------------------------------------------------------
Svar_record *
Svar_DB_server::match_pending_offer(const uint32_t * UCS_varname,
                                    const AP_num3 & to,
                                    const Svar_partner & from)
{
Svar_record * pending_offer = 0;

   for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
       {
         Svar_record & svar = offered_vars[o];
         if (svar.get_coupling() != SV_OFFERED)                       continue;
         if (!svar.match_name(UCS_varname))                           continue;

         // if both offers are general then they do not match
         //
         if (to.proc == AP_GENERAL &&
             svar.accepting.id.proc == AP_GENERAL)                    continue;

         if (svar.accepting.id.proc == AP_GENERAL)   // existing general offer
            {
              if (!(svar.offering.id == to))                          continue;
            }
         else if (to.proc == AP_GENERAL)              // new offer is general
            {
              if (svar.accepting.id.proc != from.id.proc)             continue;
            }
         else
            {
              if (!(svar.offering.id == to))                          continue;
              if (!(svar.accepting.id == from.id))                    continue;
            }

         // found pending offer
         // 
         pending_offer = &svar;
         break;
       }

   if (pending_offer)
      {
         pending_offer->accepting = from;
         pending_offer->offering.events = SVE_OFFER_MATCHED;
         return pending_offer;            // match found
      }

   return 0;   // no match
}
//-----------------------------------------------------------------------------
Svar_record *
Svar_DB_server::create_offer(const uint32_t * UCS_varname,
                             const AP_num3 & to, const Svar_partner & from)
{
   // at this point, no matching offer from 'to' was found. If the offer
   // is non-general (i.e. 'to' is a specific processor) then to should
   // get an offer mismatch event.
   //

   // find free entry and insert the offer
   //
   for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
       {
         Svar_record * svar = offered_vars + o;
         if (svar->valid())   continue;

         SV_key key  = from.pid;       key <<= 16;
                key |= from.id.proc;   key <<= 16;
                key |= ++seq;
         svar->key = key;
         for (int v = 0; v < (MAX_SVAR_NAMELEN + 1); ++v)
             {
              svar->varname[v] = UCS_varname[v];
              if (UCS_varname[v] == 0)   break;
             }

         svar->varname[MAX_SVAR_NAMELEN] = 0;
         svar->offering = from;

         svar->accepting.id = to;

         // if to is registered then send a signal
         //
         const uint16_t port = get_udp_port_for_id(to);
         if (port)
            {
              UdpClientSocket sock(LOC, port);
              MAKE_OFFER_c signal(sock, svar->key);
            }

         return svar;   // success
       }

   return 0;   // table full
}
//-----------------------------------------------------------------------------
