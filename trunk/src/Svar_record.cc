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
#include "PrintOperator.hh"
#include "ProcessorID.hh"
#include "Svar_record.hh"
#include "Svar_signals.hh"

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
Svar_record::remove_accepting()
{
   accepting.clear();
}
//-----------------------------------------------------------------------------
void
Svar_record::remove_offering()
{
const AP_num3 offered_to = offering.id;

   offering = accepting;
   accepting.clear();
   accepting.id = offered_to;
}
//-----------------------------------------------------------------------------
void
Svar_record::remove_stale(int & count)
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
Svar_record::retract()
{
   if (this == 0)   return NO_COUPLING;

const SV_Coupling old_coupling = get_coupling();
const uint16_t port1 = offering.port;
const uint16_t port2 = accepting.port;

   if (ProcessorID::get_id() == offering.id)         remove_offering();
   else if (ProcessorID::get_id() == accepting.id)   remove_accepting();
   else
      {
        bad_proc("Svar_record::retract", ProcessorID::get_id());
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
Svar_record::data_owner_port(bool & ws_to_ws)   const
{
   if (this == 0)   return -1;

   if (get_coupling() != SV_COUPLED)
      {
        ws_to_ws = offering.id.proc >= 1000;
        return offering.port;
      }

   ws_to_ws =  offering.id.proc >= 1000 && accepting.id.proc >= 1000;
   if (accepting.id.proc <= offering.id.proc)   return accepting.port;
   else                                         return offering.port;
}
//-----------------------------------------------------------------------------
bool
Svar_record::match_name(const uint32_t * UCS_other) const
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
Svar_record::get_control() const
{
   if (this == 0)   return NO_SVAR_CONTROL;

int ctl = offering.get_control() | accepting.get_control();

   if (ProcessorID::get_id() == accepting.id)   ctl = mirror(ctl);
   return Svar_Control(ctl);
}
//-----------------------------------------------------------------------------
void
Svar_record::set_control(Svar_Control ctl)
{
   if (this == 0)   return;

   Log(LOG_shared_variables)
      {
        get_CERR() << "set_control(" << ctl << ") on ";
        print_name(get_CERR());
        get_CERR() << " by " << ProcessorID::get_id().proc << endl;
      }

   if (ProcessorID::get_id() == offering.id)
      {
        offering.set_control(ctl);
        if (get_coupling() == SV_COUPLED)   // fully coupled: inform peer
           {
              accepting.events = (Svar_event)
                                 (accepting.events | SVE_ACCESS_CONTROL);
           }
      }
   else if (ProcessorID::get_id() == accepting.id)   // hence fully coupled
      {
        accepting.set_control(mirror(ctl));
        offering.events = (Svar_event)(offering.events | SVE_ACCESS_CONTROL);
      }
   else
      {
        bad_proc(__FUNCTION__, ProcessorID::get_id());
      }
}
//-----------------------------------------------------------------------------
Svar_state
Svar_record::get_state() const
{
   if (this == 0)   return SVS_NOT_SHARED;

   return state;
}
//-----------------------------------------------------------------------------
void
Svar_record::set_state(bool used, const char * loc)
{
   if (this == 0)   return;

usleep(50000);

   Log(LOG_shared_variables)
      {
        const char * op = used ? "used" : "set";
        get_CERR() << "set_state(" << op << ") on ";
        print_name(get_CERR());
        get_CERR() << " by " << ProcessorID::get_id().proc << " at " << loc << endl;
      }

   // the control vector as seen by the offering side
   //
const int control = offering.get_control() | accepting.get_control();
Svar_event event = SVE_NO_EVENTS;
Svar_partner * peer = 0;

   if (ProcessorID::get_id() == offering.id)
      {
        peer = &offering;
        offering.events = SVE_NO_EVENTS;   // clear events

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
        peer = &offering;
        accepting.events = SVE_NO_EVENTS;   // clear events

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
   if (peer && event != SVE_NO_EVENTS)
      {
        peer->events = (Svar_event)(peer->events | event);
      }
}
//-----------------------------------------------------------------------------
Svar_partner
Svar_record::get_peer() const
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
Svar_record::may_use(int attempt)
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
             accepting.events = (Svar_event)
                                (accepting.events | SVE_USE_BY_OFF_FAILED);
           }
        return false;
      }

   if (ProcessorID::get_id() == accepting.id)
      {
        if ((restriction & USE_BY_ACC) == 0)   return true;   // no restriction

        if (offering.port && (attempt == 0))   // maybe send event to peer
           {
             offering.events = (Svar_event)
                               (offering.events | SVE_USE_BY_ACC_FAILED);
           }
        return false;
      }

   bad_proc(__FUNCTION__, ProcessorID::get_id());
   return false;
}
//-----------------------------------------------------------------------------
bool
Svar_record::may_set(int attempt)
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
             accepting.events = (Svar_event)
                                (accepting.events | SVE_SET_BY_OFF_FAILED);
           }
        return false;
      }

   if (ProcessorID::get_id() == accepting.id)
      {
        if ((restriction & SET_BY_ACC) == 0)   return true;   // no restriction

        if (offering.port && (attempt == 0))   // maybe send event to peer
           {
             offering.events = (Svar_event)
                               (offering.events | SVE_SET_BY_ACC_FAILED);
           }
        return false;
      }

   bad_proc(__FUNCTION__, ProcessorID::get_id());
   return false;
}
//-----------------------------------------------------------------------------
void
Svar_record::bad_proc(const char * function, const AP_num3 & id) const
{
   get_CERR() << function << "(): proc " << id.proc
        << " does not match offering proc " << offering.id.proc
        << " nor accepting proc " << accepting.id.proc << endl;
}
//-----------------------------------------------------------------------------
void
Svar_record::print(ostream & out) const
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
Svar_record::print_name(ostream & out, const uint32_t * name, int len)
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
