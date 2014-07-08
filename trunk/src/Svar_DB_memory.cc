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
#include "Svar_DB_memory.hh"
#include "Svar_signals.hh"
#include "UdpSocket.hh"

extern ostream CERR;
extern ostream & get_CERR();

//=============================================================================
const char *
event_name(Svar_event ev)
{
   switch(ev)
      {
        case SVE_NO_EVENTS:          return "(no event)";
        case SVE_OFFER_MISMATCH:     return "offer mismatch";
        case SVE_OFFER_MATCHED:      return "offer matched";
        case SVE_OFFER_RETRACT:      return "offer retracted";
        case SVE_ACCESS_CONTROL:     return "access control changed";
        case SVE_USE_BY_OFF_SUCCESS: return "use by offering successful";
        case SVE_USE_BY_OFF_FAILED:  return "use by offering failed";
        case SVE_SET_BY_OFF_SUCCESS: return "set by offering successful";
        case SVE_SET_BY_OFF_FAILED:  return "set by offering failed";
        case SVE_USE_BY_ACC_SUCCESS: return "use by accepting successful";
        case SVE_USE_BY_ACC_FAILED:  return "use by accepting failed";
        case SVE_SET_BY_ACC_SUCCESS: return "set by accepting successful";
        case SVE_SET_BY_ACC_FAILED:  return "set by accepting failed";
      }

   return "(unknown event)";
}
//-----------------------------------------------------------------------------
bool
Svar_partner::pid_alive(pid_t pid)
{
   if (pid == 0)   return false;   // no pid

const int kill_result = kill(pid, 0);
   if (kill_result == 0)   return true;   // kill worked, hence pif exists

   // if kill failed due to lack of permission then we don't clean up
   //
   if (errno == EPERM)   return true;

   // if kill failed due to an invalid pid then clean it up
   //
   if (errno == ESRCH)   return false;

   return true;   // should not come here; remain on the safe side
}
//-----------------------------------------------------------------------------
ostream &
Svar_partner::print(ostream & out) const
{
   out << setw(5) << id.proc;
   if (id.parent)   out << "," << left << setw(5) << id.parent << right;
   else             out << "      ";

   out << "│" << setw(5) << pid
       << "│" << setw(5) << port << "│"
       << hex << uppercase << setfill('0') << setw(2) << flags
       << dec << nouppercase << setfill(' ');

   return out;
}
//-----------------------------------------------------------------------------
void
offered_SVAR::remove_accepting()
{
   accepting.clear();
}
//-----------------------------------------------------------------------------
void
offered_SVAR::remove_offering()
{
const AP_num3 offered_to = offering.id;

   offering = accepting;
   accepting.clear();
   accepting.id = offered_to;
}
//-----------------------------------------------------------------------------
void
offered_SVAR::remove_stale(int & count)
{
   if (get_coupling() == SV_COUPLED)   // fully coupled
      {
        if (!accepting.pid_alive())
           {
             Log(LOG_shared_variables)
                {
                  if (count == 0)
                     get_CERR() << "removing stale variables..." << endl;

                  get_CERR() << "remove stale accepting proc "
                             << accepting.id.proc << endl;
                }
             remove_accepting();
             ++count;
           }
      }

   if (get_coupling() >= SV_OFFERED)   // fully coupled or offered
      {
        if (!offering.pid_alive())
           {
             Log(LOG_shared_variables)
                {
                  if (count == 0)
                     get_CERR() << "removing stale variables..." << endl;

                  get_CERR() << "remove stale offering proc "
                             << offering.id.proc << endl;
                }

             if (get_coupling() == SV_COUPLED)   // fully coupled
                {
                  // the variable is fully coupled, therefore the accepting side
                  // is still alive and should be informed
                  //
                  UdpClientSocket sock(LOC, accepting.port);
                  RETRACT_OFFER_c signal(sock, key);

                  offering = accepting;
                  accepting.clear();

                }
             else
                {
                  // nobody left: just  clear the entry.
                  //
                  clear();
                }

             ++count;
           }
      }
}
//-----------------------------------------------------------------------------
SV_Coupling
offered_SVAR::retract()
{
   if (this == 0)   return NO_COUPLING;

const SV_Coupling old_coupling = get_coupling();
const uint16_t port1 = offering.port;
const uint16_t port2 = accepting.port;

   if (ProcessorID::get_id() == offering.id)         remove_offering();
   else if (ProcessorID::get_id() == accepting.id)   remove_accepting();
   else
      {
        bad_proc(__FUNCTION__, ProcessorID::get_id());
        return NO_COUPLING;
      }

   // clear variable if last partner has retracted, or else send signal
   // to the remaining partner.
   //
   if (get_coupling() == NO_COUPLING)   // retract of a non-coupled variable
      {
        clear();
      }

   // inform partners
   //
   if (port1)
      {
        UdpClientSocket sock(LOC, port1);
        RETRACT_OFFER_c signal(sock, key);
      }

   if (port2)
      {
        UdpClientSocket sock(LOC, port2);
        RETRACT_OFFER_c signal(sock, key);
      }

   return old_coupling;
}
//-----------------------------------------------------------------------------
uint16_t
offered_SVAR::data_owner_port()   const
{
   if (this == 0)   return -1;

   if (get_coupling() != SV_COUPLED)   return offering.port;

   if (accepting.id.proc <= offering.id.proc)   return accepting.port;
   else                                         return offering.port;
}
//-----------------------------------------------------------------------------
bool
offered_SVAR::match_name(const uint32_t * UCS_other) const
{
  // compare INCLUDING terminating 0;
  for (int v = 0; v < (MAX_SVAR_NAMELEN); ++v)
      {
        if (varname[v] != UCS_other[v])
           {
             if (varname[v]   == 0x22C6)   return true;   // ⋆
             if (varname[v]   == '*')      return true;
             if (UCS_other[v] == 0x22C6)   return true;   // ⋆
             if (UCS_other[v] == '*')      return true;
             return false;
           }
        if (varname[v] == 0)              return true;   // short name
      }

   return true;                                           // long name
}
//-----------------------------------------------------------------------------
Svar_Control
mirror(int flags)
{
int ret = 0;
   if (flags & SET_BY_OFF)   ret |= SET_BY_ACC;
   if (flags & SET_BY_ACC)   ret |= SET_BY_OFF;
   if (flags & USE_BY_OFF)   ret |= USE_BY_ACC;
   if (flags & USE_BY_ACC)   ret |= USE_BY_OFF;
   return Svar_Control(ret);
}
//-----------------------------------------------------------------------------
Svar_Control
offered_SVAR::get_control() const
{
   if (this == 0)   return NO_SVAR_CONTROL;

int ctl = offering.get_control() | accepting.get_control();

   if (ProcessorID::get_id() == accepting.id)   ctl = mirror(ctl);
   return Svar_Control(ctl);
}
//-----------------------------------------------------------------------------
void
offered_SVAR::set_control(Svar_Control ctl)
{
   if (this == 0)   return;

   Log(LOG_shared_variables)
      {
        CERR << "set_control(" << ctl << ") on ";
        print_name(CERR);
        CERR << " by " << ProcessorID::get_id().proc << endl;
      }

   if (ProcessorID::get_id() == offering.id)
      {
        offering.set_control(ctl);
        if (get_coupling() == SV_COUPLED)   // fully coupled: inform peer
           {
             UdpClientSocket sock(LOC, accepting.port);
             NEW_EVENT_c signal(sock, key, SVE_ACCESS_CONTROL);
           }
      }
   else if (ProcessorID::get_id() == accepting.id)   // hence fully coupled
      {
        accepting.set_control(mirror(ctl));
        {
          UdpClientSocket sock(LOC, offering.port);
          NEW_EVENT_c signal(sock, key, SVE_ACCESS_CONTROL);
        }
      }
   else
      {
        bad_proc(__FUNCTION__, ProcessorID::get_id());
      }
}
//-----------------------------------------------------------------------------
Svar_state
offered_SVAR::get_state() const
{
   if (this == 0)   return SVS_NOT_SHARED;

   return state;
}
//-----------------------------------------------------------------------------
void
offered_SVAR::set_state(bool used, const char * loc)
{
   if (this == 0)   return;

usleep(50000);

   Log(LOG_shared_variables)
      {
        const char * op = used ? "used" : "set";
        CERR << "set_state(" << op << ") on ";
        print_name(CERR);
        CERR << " by " << ProcessorID::get_id().proc << " at " << loc << endl;
      }

   // the control vector as seen by the offering side
   //
const int control = offering.get_control() | accepting.get_control();
Svar_event event = SVE_NO_EVENTS;
uint16_t peer_port = 0;

   if (ProcessorID::get_id() == offering.id)
      {
         offering.flags &= ~OSV_EVENT;   // clear events
         peer_port = accepting.port;

        if (used)   // offering has used the variable (unless read-back)
           {
             if (state == SVS_OFF_HAS_SET)   ;   // read-back
             else
                {
                  if (control & USE_BY_OFF)   event = SVE_USE_BY_OFF_SUCCESS;
                  state = SVS_IDLE;
                }
           }
        else        // offering has set the variable
           {
             if (control & SET_BY_OFF)   event = SVE_SET_BY_OFF_SUCCESS;
             state = SVS_OFF_HAS_SET;
           }
      }
   else if (ProcessorID::get_id() == accepting.id)
      {
         accepting.flags &= ~OSV_EVENT;   // clear events
         peer_port = offering.port;

        if (used)   // accepting has used the variable (unless read-back)
           {
             if (state == SVS_ACC_HAS_SET)   ; // read-back
             else
                {
                  if (control & USE_BY_ACC)   event = SVE_USE_BY_ACC_SUCCESS;
                  state = SVS_IDLE;
                }
           }
        else        // accepting has set the variable
           {
             if (control & SET_BY_ACC)   event = SVE_SET_BY_ACC_SUCCESS;
             state = SVS_ACC_HAS_SET;
           }
      }
   else   // only the partners should call set_state();
      {
        bad_proc(__FUNCTION__, ProcessorID::get_id());
        return;
      }

   // if access was restricted then inform peer
   //
   if (peer_port && event)
      {
        UdpClientSocket sock(LOC, peer_port);
        NEW_EVENT_c signal(sock, key, event);
      }
}
//-----------------------------------------------------------------------------
Svar_partner
offered_SVAR::get_peer() const
{
   if (this == 0)
      {
          Svar_partner peer;
          peer.clear();
          return peer;
      }

   if (ProcessorID::get_id() == offering.id)    return accepting;
   if (ProcessorID::get_id() == accepting.id)   return offering;

   bad_proc(__FUNCTION__, ProcessorID::get_id());

Svar_partner peer;
   peer.clear();
   return peer;
}
//-----------------------------------------------------------------------------
bool
offered_SVAR::may_use(int attempt) const
{
   if (this == 0)   return false;

   // control restriction as seen by the offering partner
   //
const int control = offering.get_control() | accepting.get_control();
const int restriction = control & state;

   if (ProcessorID::get_id() == offering.id)
      {
        if ((restriction & USE_BY_OFF) == 0)   return true;   // no restriction

        if (accepting.port && (attempt == 0))   // maybe send event to peer
           {
             UdpClientSocket sock(LOC, accepting.port);
             NEW_EVENT_c signal(sock, key, SVE_USE_BY_OFF_FAILED);
           }
        return false;
      }

   if (ProcessorID::get_id() == accepting.id)
      {
        if ((restriction & USE_BY_ACC) == 0)   return true;   // no restriction

        if (offering.port && (attempt == 0))   // maybe send event to peer
           {
             UdpClientSocket sock(LOC, offering.port);
             NEW_EVENT_c signal(sock, key, SVE_USE_BY_ACC_FAILED);
           }
        return false;
      }

   bad_proc(__FUNCTION__, ProcessorID::get_id());
   return false;
}
//-----------------------------------------------------------------------------
bool
offered_SVAR::may_set(int attempt) const
{
   if (this == 0)   return false;

   // control restriction as seen by the offering partner
   //
const int control = offering.get_control() | accepting.get_control();
const int restriction = control & state;

   if (ProcessorID::get_id() == offering.id)
      {
        if ((restriction & SET_BY_OFF) == 0)   return true;   // no restriction

        if (accepting.port && (attempt == 0))   // maybe send event to peer
           {
             UdpClientSocket sock(LOC, accepting.port);
             NEW_EVENT_c signal(sock, key, SVE_SET_BY_OFF_FAILED);
           }
        return false;
      }

   if (ProcessorID::get_id() == accepting.id)
      {
        if ((restriction & SET_BY_ACC) == 0)   return true;   // no restriction

        if (offering.port && (attempt == 0))   // maybe send event to peer
           {
             UdpClientSocket sock(LOC, offering.port);
             NEW_EVENT_c signal(sock, key, SVE_SET_BY_ACC_FAILED);
           }
        return false;
      }

   bad_proc(__FUNCTION__, ProcessorID::get_id());
   return false;
}
//-----------------------------------------------------------------------------
void
Svar_DB_memory::register_processor(const Svar_partner & p)
{
   // if processor is already (i.e. still) registered, update it.
   //
   for (int a = 0; a < MAX_ACTIVE_PROCS; ++a)
       {
         Svar_partner_events & slot = active_processors[a];

         if (slot.partner.id == p.id)   // existing partner
            {
              Log(LOG_shared_variables)
                 CERR << "refreshing existing proc " << p.id.proc << endl;

              slot.partner = p;
              slot.events = SVE_NO_EVENTS;
              return;
            }
       }

   // new processor: find empty slot and insert
   //
   for (int a = 0; a < MAX_ACTIVE_PROCS; ++a)
       {
         Svar_partner_events & slot = active_processors[a];
         if (slot.partner.id.proc == AP_NULL)   // free slot
            {
              Log(LOG_shared_variables)
                 CERR << "registered processor " << p.id.proc << endl;
              slot.partner = p;
              slot.events = SVE_NO_EVENTS;
              return;
            }
       }

   // table full.
   //
   CERR << "*** table full in register_processor()" << endl;
}
//-----------------------------------------------------------------------------
Svar_partner_events
Svar_DB_memory::get_registered(const AP_num3 & id) const
{
   for (int a = 0; a < MAX_ACTIVE_PROCS; ++a)
       {
         const Svar_partner_events & slot = active_processors[a];
         if (slot.partner.id == id)   return slot;
       }

Svar_partner_events svp;
   svp.clear();
   return svp;
}
//-----------------------------------------------------------------------------
bool
Svar_DB_memory::is_unused_id(AP_num id) const
{
   for (int a = 0; a < MAX_ACTIVE_PROCS; ++a)
       {
         const Svar_partner_events & slot = active_processors[a];
         if (slot.partner.id.proc   == id)   return false;
         if (slot.partner.id.parent == id)   return false;
         if (slot.partner.id.grand  == id)   return false;
       }

   // not used
   //
   return true;
}
//-----------------------------------------------------------------------------
AP_num
Svar_DB_memory::get_unused_id() const
{
   for (AP_num proc = AP_FIRST_USER; ; proc = AP_num(proc + 1))
       {
         // for the almost impossible case that ap has wrapped
         //
         if (proc < AP_FIRST_USER)   proc = AP_FIRST_USER;
         if (is_unused_id(proc))     return proc;
       }

   Assert(0 && "Not reached");
}
//-----------------------------------------------------------------------------
uint16_t
Svar_DB_memory::get_udp_port(AP_num proc, AP_num parent) const
{
   for (int a = 0; a < MAX_ACTIVE_PROCS; ++a)
       {
         const Svar_partner_events & slot = active_processors[a];
         if (slot.partner.id.proc == proc &&
             slot.partner.id.parent == parent)   return slot.partner.port;
       }

   return 0;
}
//-----------------------------------------------------------------------------
void
Svar_DB_memory::unregister_processor(const Svar_partner & p)
{
   retract_all(p.id);

   for (int a = 0; a < MAX_ACTIVE_PROCS; ++a)
       {
         Svar_partner_events & slot = active_processors[a];
         if (slot.partner.id == p.id)   // found
            {
              Log(LOG_shared_variables)
                 CERR << "unregistered processor " << p.id.proc << endl;
              slot.clear();
              break;
            }
       }

   // send DISCONNECT to dependent processors
   //
   for (int a = 0; a < MAX_ACTIVE_PROCS; ++a)
       {
         const Svar_partner_events & slot = active_processors[a];
         if (slot.partner.id.parent == p.id.proc &&
             slot.partner.id.grand == p.id.parent)
            {
              Log(LOG_shared_variables)
                 CERR << "sending DISCONNECT to proc "
                      << int(slot.partner.id.proc)
                   << endl;
              UdpClientSocket sock(LOC, slot.partner.port);
              DISCONNECT_c signal(sock);
            }
       }
}
//-----------------------------------------------------------------------------
void
Svar_DB_memory::remove_stale()
{
   // don't use CERR here since CERR may not yet have been initialized yet.
   //
int stale_procs = 0;
   for (int a = 0; a < MAX_ACTIVE_PROCS; ++a)
       {
         Svar_partner_events & sp = active_processors[a];
         if (sp.partner.pid_alive())   continue;
         if (sp.partner.pid == 0)      continue;

         if (stale_procs == 0)
            get_CERR() << "removing stale processors..." << endl;

         get_CERR() << "   clearing stale pid " << sp.partner.pid << endl;
         sp.clear();
         ++stale_procs;
       }

int stale_vars = 0;
   for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
       {
         offered_vars[o].remove_stale(stale_vars);
       }
}
//-----------------------------------------------------------------------------
void
offered_SVAR::bad_proc(const char * function, const AP_num3 & id) const
{
   CERR << function << "(): proc " << id.proc
        << " does not match offering proc " << offering.id.proc
        << " nor accepting proc " << accepting.id.proc << endl;
}
//-----------------------------------------------------------------------------
void
offered_SVAR::print(ostream & out) const
{
const Svar_state st = get_state();
   out << "║" << setw(5) << (key & 0xFFFF) << "│" << get_coupling() << "║";
   offering.print(out)  << "║";
   accepting.print(out) << "║";
   if (st & SET_BY_OFF)   out << "1";    else   out << "0";
   if (st & SET_BY_ACC)   out << "1";    else   out << "0";
   if (st & USE_BY_OFF)   out << "1";    else   out << "0";
   if (st & USE_BY_ACC)   out << "1│";   else   out << "0│";
   print_name(out, varname, 10) << "║" << endl;
}
//-----------------------------------------------------------------------------
ostream &
offered_SVAR::print_name(ostream & out, const uint32_t * name, int len)
{
   while (*name)
       {
         uint32_t uni = *name++;
         if (uni < 0x80)
           {
             out << char(uni);
           }
        else if (uni < 0x800)
           {
             const uint8_t b1 = uni & 0x3F;   uni >>= 6;
             out << char(uni | 0xC0)
                 << char(b1  | 0x80);
           }
        else if (uni < 0x10000)
           {
             const uint8_t b2 = uni & 0x3F;   uni >>= 6;
             const uint8_t b1 = uni & 0x3F;   uni >>= 6;
             out << char(uni | 0xE0)
                 << char(b1  | 0x80)
                 << char(b2  | 0x80);
           }
        else if (uni < 0x110000)
           {
             const uint8_t b3 = uni & 0x3F;   uni >>= 6;
             const uint8_t b2 = uni & 0x3F;   uni >>= 6;
             const uint8_t b1 = uni & 0x3F;   uni >>= 6;
             out << char(uni | 0xE0)
                 << char(b1  | 0x80)
                 << char(b2  | 0x80)
                 << char(b3  | 0x80);
           }

        --len;
       }

   while (len-- > 0)   out << " ";

   return out;
}
//-----------------------------------------------------------------------------
void
Svar_DB_memory::retract_all(const AP_num3 & id)
{
   Log(LOG_shared_variables)
      CERR << "retract_all(proc " << id.proc << ")" << endl;

   for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
       {
         offered_SVAR & svar = offered_vars[o];
         if (!svar.valid())         continue;

         if (svar.offering.id == id)         svar.retract();
         else if (svar.accepting.id == id)   svar.retract();
       }
}
//-----------------------------------------------------------------------------
Svar_event
Svar_DB_memory::clear_all_events()
{
Svar_event ret = SVE_NO_EVENTS;
const AP_num3 & caller_id = ProcessorID::get_id();

   // clear event bits of calling partner
   //
   for (int a = 0; a < MAX_ACTIVE_PROCS; ++a)
       {
         Svar_partner_events & slot = active_processors[a];

         if (slot.partner.id == caller_id)
            {
              ret = slot.events;
              slot.events = SVE_NO_EVENTS;
            }
       }

   // clear event bit in all shared variables
   //
   for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
       {
         offered_SVAR & svar = offered_vars[o];
         if (caller_id == svar.offering.id)
            {
              svar.offering.flags &= ~OSV_EVENT;
            }
         else if (caller_id == svar.accepting.id)
            {
              svar.accepting.flags &= ~OSV_EVENT;
            }
       }

   return ret;
}
//-----------------------------------------------------------------------------
SV_key
Svar_DB_memory::get_events(Svar_event & events, AP_num3 proc) const
{
   for (int a = 0; a < MAX_ACTIVE_PROCS; ++a)
       {
         const Svar_partner_events & slot = active_processors[a];

         if (slot.partner.id == proc)   events = slot.events;
       }

   // return the first key with an event for proc (if any)
   //
   for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
       {
         const offered_SVAR & svar = offered_vars[o];
         if (proc == svar.offering.id)
            {
              if (svar.offering.flags & OSV_EVENT)    return svar.key;
            }
         else if (proc == svar.accepting.id)
            {
              if (svar.accepting.flags & OSV_EVENT)   return svar.key;
            }
       }

   return 0;   // no key.
}
//-----------------------------------------------------------------------------
void
Svar_DB_memory::add_event(Svar_event event, AP_num3 proc, SV_key key)
{
   // set event bit for proc
   //
   for (int a = 0; a < MAX_ACTIVE_PROCS; ++a)
       {
         Svar_partner_events & slot = active_processors[a];

         if (slot.partner.id == proc)
            {
              slot.events = Svar_event(slot.events | event);
              UdpClientSocket sock(LOC, slot.partner.port);
              NEW_EVENT_c signal(sock, key, event);
            }
       }

   if (key == 0)   return;   // no key

offered_SVAR * svar = find_var(key);
   if (svar)
      {
        if (proc == svar->offering.id)       svar->offering.flags  |= OSV_EVENT;
        else if (proc == svar->accepting.id) svar->accepting.flags |= OSV_EVENT;
      }
}
//-----------------------------------------------------------------------------
SV_key
Svar_DB_memory::pairing_key(SV_key key, bool (*compare)
                             (const uint32_t * v1, const uint32_t * v2)) const
{
const offered_SVAR * svar1 = find_var(key);
   if (svar1 == 0)   return 0;

   for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
       {
         const offered_SVAR & svar2 = offered_vars[o];
         if (!svar2.valid())     continue;
         if (svar2.key == key)   continue;   // don't compare svar1 with svar1
          if (compare(svar1->varname, svar2.varname))   return svar2.key;
       }

   return 0;   // not found
}
//-----------------------------------------------------------------------------
offered_SVAR *
Svar_DB_memory::find_var(SV_key key) const
{
   if (key == 0)   return 0;

   for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
       {
         const offered_SVAR * svar = offered_vars + o;
         if (key == svar->key)   return (offered_SVAR *)svar;
       }

   return 0;
}
//-----------------------------------------------------------------------------
void
Svar_DB_memory::get_processors(int to_proc, vector<int32_t> & processors)
{
   // return pending AND matched offers, general or to to_proc
   //
   for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
       {
         offered_SVAR & svar = offered_vars[o];
         if (!svar.valid())         continue;

         if (svar.accepting.id.proc == to_proc)
             processors.push_back(svar.offering.id.proc);
         else if (svar.accepting.id.proc == 0)
             processors.push_back(svar.offering.id.proc);
       }
}
//-----------------------------------------------------------------------------
void
Svar_DB_memory::get_variables(int to_proc, int from_proc,
                                  vector<const uint32_t *> & vars) const
{
   // return pending variables but not matched offers, general or to to_proc
   //
   for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
       {
         const offered_SVAR & svar = offered_vars[o];
         if (!svar.valid())         continue;

         if (svar.accepting.id.proc == to_proc)
             vars.push_back(svar.varname);
         else if (svar.accepting.id.proc == 0)
             vars.push_back(svar.varname);
       }
}
//-----------------------------------------------------------------------------
offered_SVAR * 
Svar_DB_memory::match_or_make(const uint32_t * UCS_varname, const AP_num3 & to,
                              const Svar_partner & from, SV_Coupling & coupling)
{
// CERR << "got offer from ("; from.print(CERR); CERR << ") to " << to << " ";
// offered_SVAR::print_name(CERR, UCS_varname) << endl;

offered_SVAR * svar = match_pending_offer(UCS_varname, to, from);

   Log(LOG_shared_variables)   if (svar)
      {
        CERR << "found pending offer from " << svar->offering.id.proc << " ";
        svar->print_name(CERR) << " key 0x" << hex << svar->key << dec << endl;
      }

   if (svar)
      {
        coupling = svar->get_coupling();
        return svar;
      }

   svar = create_offer(UCS_varname, to, from);

   Log(LOG_shared_variables)
      {
        CERR << "created new offer:" << endl;
//      Svar_DB::print(CERR);
      }

   if (svar)   coupling = svar->get_coupling();
   else        coupling = NO_COUPLING;
   return svar;
}
//-----------------------------------------------------------------------------
offered_SVAR *
Svar_DB_memory::match_pending_offer(const uint32_t * UCS_varname,
                                    const AP_num3 & to,
                                    const Svar_partner & from)
{
   for (int o = 0; o < MAX_SVARS_OFFERED; ++o)
       {
         offered_SVAR * svar = offered_vars + o;
         if (svar->get_coupling() != SV_OFFERED)                       continue;
         if (!svar->match_name(UCS_varname))                           continue;

         // if both offers are general then they do not match
         //
         if (to.proc == AP_GENERAL &&
             svar->accepting.id.proc == AP_GENERAL)                    continue;

         if (svar->accepting.id.proc == AP_GENERAL)   // existing general offer
            {
              if (!(svar->offering.id == to))                          continue;
            }
         else if (to.proc == AP_GENERAL)              // new offer is general
            {
              if (svar->accepting.id.proc != from.id.proc)             continue;
            }
         else
            {
              if (!(svar->offering.id == to))                          continue;
              if (!(svar->accepting.id == from.id))                    continue;
            }

         // the two offer match. Fill in accepting side.
         //
         svar->accepting = from;

         // inform both partners that the coupling is complete
         //
         {
           UdpClientSocket sock(LOC, svar->offering.port);
           OFFER_MATCHED_c signal(sock, svar->key);
         }
         {
           UdpClientSocket sock(LOC, svar->accepting.port);
           OFFER_MATCHED_c signal(sock, svar->key);
         }
         return svar;            // match found
       }

   return 0;   // no match
}
//-----------------------------------------------------------------------------
offered_SVAR *
Svar_DB_memory::create_offer(const uint32_t * UCS_varname,
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
         offered_SVAR * svar = offered_vars + o;
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

         // inform from that there is a new variable
         //
         {
           UdpClientSocket sock(LOC, from.port);
           NEW_VARIABLE_c signal(sock, svar->key);
         }

         // if to is registered then send a signal
         //
         const Svar_partner_events peer = get_registered(to);
         if (peer.partner.id.proc > AP_NULL)
            {
              svar->offering.flags |= OSV_OFFER_SENT;
              UdpClientSocket sock(LOC, peer.partner.port);
              MAKE_OFFER_c signal(sock, svar->key);
            }

         return svar;   // success
       }

   return 0;   // table full
}
//-----------------------------------------------------------------------------
const char *
Svar_DB_memory::get_APclient_unix_socket_name()
{
#ifdef HAVE_LINUX_UN_H
static char cc[100];
   snprintf(cc, sizeof(cc), "/tmp/GNU-APL/APclient-%u", getpid());
   return cc;
#else
   return 0;
#endif
}
//-----------------------------------------------------------------------------
