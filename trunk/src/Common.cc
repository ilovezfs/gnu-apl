/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2015  Dr. Jürgen Sauermann

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

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include "CharCell.hh"
#include "Common.hh"
#include "ComplexCell.hh"
#include "FloatCell.hh"
#include "LineInput.hh"
#include "LibPaths.hh"
#include "NativeFunction.hh"
#include "Output.hh"
#include "ProcessorID.hh"
#include "Svar_DB.hh"
#include "Value.icc"
#include "UserPreferences.hh"
#include "ValueHistory.hh"

uint64_t total_memory = 0;

APL_time_us interrupt_when = 0;
uint64_t interrupt_count = 0;
bool interrupt_raised = false;

bool attention_raised = false;
uint64_t attention_count = 0;

//-----------------------------------------------------------------------------
void
init_1(const char * argv0, bool log_startup)
{
rlimit rl;
   getrlimit(RLIMIT_AS, &rl);
   total_memory = rl.rlim_cur;

   if (log_startup)
      CERR << "sizeof(Svar_record) is    " << sizeof(Svar_record) << endl
           << "sizeof(Svar_partner) is   " << sizeof(Svar_partner)
           << endl;

   getrlimit(RLIMIT_NPROC, &rl);
   if (log_startup)
      CERR << "increasing rlimit RLIMIT_NPROC from " <<  rl.rlim_cur
           << " to infinity" << endl;
   rl.rlim_cur = RLIM_INFINITY;
   setrlimit(RLIMIT_NPROC, &rl);

   Avec::init();
   LibPaths::init(argv0, log_startup);
   Value::init();
   VH_entry::init();
}
//-----------------------------------------------------------------------------
/// initialize subsystems that depend on argv[]
void
init_2(bool log_startup)
{
   Output::init(log_startup);
   Svar_DB::init(LibPaths::get_APL_bin_path(),
                 LibPaths::get_APL_bin_name(),
                 log_startup, uprefs.system_do_svars);

   LineInput::init(true);

   Parallel::init(log_startup || LOG_Parallel);
}
//-----------------------------------------------------------------------------
/// the opposite of init()
void
cleanup(bool soft)
{
   if (soft)   // proper clean-up
      {
        ProcessorID::disconnect();

        NativeFunction::cleanup();

        // write line history
        //
        LineInput::close(false);

        Output::reset_colors();
      }
   else        // minimal clean-up
      {
        LineInput::close(true);
        Output::reset_colors();
      }
}
//-----------------------------------------------------------------------------
void
control_C(int)
{
APL_time_us when = now();

   CIN << "^C";

   attention_raised = true;
   ++attention_count;
   if ((when - interrupt_when) < 1000000)   // second ^C within 1 second
      {
        interrupt_raised = true;
        ++interrupt_count;
      }

   interrupt_when = when;
}
//-----------------------------------------------------------------------------

// Probes...

int64_t Probe::dummy = 0;
Probe Probe::probes[Probe::PROBE_COUNT];

Probe & Probe::P0 = probes[0];
Probe & Probe::P_1 = probes[Probe::PROBE_COUNT - 1];
Probe & Probe::P_2 = probes[Probe::PROBE_COUNT - 2];
Probe & Probe::P_3 = probes[Probe::PROBE_COUNT - 3];
Probe & Probe::P_4 = probes[Probe::PROBE_COUNT - 4];
Probe & Probe::P_5 = probes[Probe::PROBE_COUNT - 5];

//-----------------------------------------------------------------------------
void *
common_new(size_t size)
{
void * ret = malloc(size);
   CERR << "NEW " << HEX(ret) << "-" << HEX((const char *)ret + size)
        << "  (" << HEX(size) << ")" << endl;
   return ret;
}
//-----------------------------------------------------------------------------
void
common_delete(void * p)
{
   CERR << "DEL " << HEX(p) << endl;
   free(p);
}
//-----------------------------------------------------------------------------
APL_time_us
now()
{
timeval tv_now;
   gettimeofday(&tv_now, 0);

APL_time_us ret = tv_now.tv_sec;
   ret *= 1000000;
   ret += tv_now.tv_usec;
   return ret;
}
//-----------------------------------------------------------------------------
YMDhmsu::YMDhmsu(APL_time_us at)
   : micro(at % 1000000)
{
const time_t secs = at/1000000;
tm t;
   gmtime_r(&secs, &t);

   year   = t.tm_year + 1900;
   month  = t.tm_mon  + 1;
   day    = t.tm_mday;
   hour   = t.tm_hour;
   minute = t.tm_min;
   second = t.tm_sec;
}
//-----------------------------------------------------------------------------
APL_time_us
YMDhmsu::get() const
{
tm t;
   t.tm_year  = year - 1900;
   t.tm_mon   = month - 1;
   t.tm_mday  = day;
   t.tm_hour  = hour;
   t.tm_min   = minute;
   t.tm_sec   = second;
   t.tm_isdst = 0;

APL_time_us ret =  mktime(&t);
   ret *= 1000000;
   ret += micro;
   return ret;
}
//-----------------------------------------------------------------------------
ostream &
operator << (ostream & out, const Function_PC2 & ft)
{
   return out << ft.low << ":" << ft.high;
}
//-----------------------------------------------------------------------------
ostream &
print_flags(ostream & out, ValueFlags flags)
{
   return out << ((flags & VF_marked)   ?  "M" : "-")
              << ((flags & VF_complete) ?  "C" : "-");
}
//-----------------------------------------------------------------------------
int
nibble(Unicode uni)
{
   switch(uni)
      {
        case UNI_ASCII_0 ... UNI_ASCII_9:   return      (uni - UNI_ASCII_0);
        case UNI_ASCII_A ... UNI_ASCII_F:   return 10 + (uni - UNI_ASCII_A);
        case UNI_ASCII_a ... UNI_ASCII_f:   return 10 + (uni - UNI_ASCII_a);
        default: break;
      }

   return -1;   // uni is not a hex digit
}
//-----------------------------------------------------------------------------
int
sixbit(Unicode uni)
{
   switch(uni)
      {
        case UNI_ASCII_A ... UNI_ASCII_Z:   return      (uni - UNI_ASCII_A);
        case UNI_ASCII_a ... UNI_ASCII_z:   return 26 + (uni - UNI_ASCII_a);
        case UNI_ASCII_0 ... UNI_ASCII_9:   return 52 + (uni - UNI_ASCII_0);

        case UNI_ASCII_PLUS:                             // standard    62
        case UNI_ASCII_MINUS:               return 62;   // alternative 62

        case UNI_ASCII_SLASH:                            // standard    63
        case UNI_ASCII_UNDERSCORE:          return 63;   // alternative 63

        case UNI_ASCII_EQUAL:               return 64;   // fill character

        default: break;
      }

   return -1;   // uni is not a hex digit
}
//-----------------------------------------------------------------------------
Value_P
IntScalar(APL_Integer val, const char * loc)
{
Value_P Z(loc);
   new (Z->next_ravel())   IntCell(val);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
FloatScalar(APL_Float val, const char * loc)
{
Value_P Z(loc);
   new (Z->next_ravel())   FloatCell(val);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
ComplexScalar(APL_Complex val, const char * loc)
{
Value_P Z(loc);
   new (Z->next_ravel()) ComplexCell(val);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
CharScalar(Unicode uni, const char * loc)
{
Value_P Z(loc);
   new (Z->next_ravel()) CharCell(uni);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Idx0(const char * loc)
{
Value_P Z((ShapeItem)0, loc);
   new (&Z->get_ravel(0))   IntCell(0);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Str0(const char * loc)
{
Value_P Z((ShapeItem)0, loc);
   new (&Z->get_ravel(0))   CharCell(UNI_ASCII_SPACE);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Str0_0(const char * loc)
{
Shape sh((ShapeItem)0, (ShapeItem)0);
Value_P Z(sh, loc);
   new (&Z->get_ravel(0))   CharCell(UNI_ASCII_SPACE);
   return Z;
}
//-----------------------------------------------------------------------------
ostream & operator << (ostream & out, const AP_num3 & ap3)
{
   return out << ap3.proc << "." << ap3.parent << "." << ap3.grand;
}
//-----------------------------------------------------------------------------

