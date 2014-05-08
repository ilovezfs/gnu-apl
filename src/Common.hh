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

#ifndef __COMMON_HH_DEFINED__
#define __COMMON_HH_DEFINED__

#include "../config.h"   // for xxx_WANTED and other macros from ./configure

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

#ifndef MAX_RANK_WANTED
#define MAX_RANK_WANTED 6
#endif

enum { MAX_RANK = MAX_RANK_WANTED };

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// if someone (like curses on Solaris) has #defined erase() then
// #undef it because vector would not be happy with it
#ifdef erase
#undef erase
#endif
#include <vector>

#include <complex>
#include <iostream>
#include <iomanip>
#include <fstream>

#include "APL_types.hh"
#include "Assert.hh"
#include "Cores.hh"
#include "Logging.hh"
#include "SystemLimits.hh"

using namespace std;

extern const char * scriptname;

/// echo of APL input (to stdout)
extern ostream CIN;

/// normal APL output (to stdout)
extern ostream COUT;

/// debug output (to stderr)
extern ostream CERR;

/// debug output (to stderr)
extern ostream UERR;

class UCS_string;

#define loop(v, e) for (ShapeItem v = 0; v < ShapeItem(e); ++v)

// #define TROUBLESHOOT_NEW_DELETE

void * common_new(size_t size);
void common_delete(void * p);

/// current time as microseconds since epoch
APL_time_us now();

/// CPU cycle counter (if present)
#if HAVE_RDTSC
inline uint64_t cycle_counter()
{
unsigned int lo, hi;
   __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
   return ((uint64_t)hi << 32) | lo;
}

#else

# define cycle_counter() (-1)

#endif

//-----------------------------------------------------------------------------
/*
  Software probes. A probe is a measurement of CPU cycles executed between two
  points P1 and P2 in the source code.
 */
class Probe
{
public:
   enum { PROBE_COUNT = 100,   ///< the number of probes
          PROBE_LEN   = 20     ///< the number of measurements in each probe
        };

   Probe()
      { init(); }

   void init()
      {
        idx = 0;
        start_p = &dummy;
        stop_p  = &dummy;
      }

   /// init the p'th probe
   static int init(int p)
      { if ((unsigned int)p >= (unsigned int)PROBE_COUNT)   return -3;
        probes[p].init();
        return 0;
      }

   static void init_all()
      { loop(p, PROBE_COUNT)   init(p); }

   void start()
      {
         if (idx < PROBE_LEN)
            {
               measurement & m = measurements[idx];
               start_p = &measurements[idx].cycles_from;
               stop_p =  &measurements[idx].cycles_to;
            }
         else
            {
               start_p = &dummy;
               stop_p  = &dummy;
            }

         // write to *start_p and *stop_p so that they are loaded into the cache
         //
         *stop_p = cycle_counter();
         *start_p = cycle_counter();

         // now the real start of the measurment
         //
         *start_p = cycle_counter();
      }

   void stop()
      {
        // the real end of the measurment
        //
        *stop_p = cycle_counter();
        ++idx;
      }

   /// get the m'th time (from P1 to P2) of this probe
   int64_t get_time(int m) const
      { if ((unsigned int)m >= (unsigned int)idx)   return -1;
        const int64_t diff = measurements[m].cycles_to
                           - measurements[m].cycles_from;
        if (diff < 0)   return -2;
        return diff;
      }

   /// get the m'th time of the p'th probe
   static int get_time(int p, int m)
      { if ((unsigned int)p >= (unsigned int)PROBE_COUNT)   return -3;
        return probes[p].get_time(m);
      }

   /// get the m'th start time of this probe
   int64_t get_start(int m) const
      { if ((unsigned int)m >= (unsigned int)idx)   return -1;
        return measurements[m].cycles_from;
      }

   /// get the m'th start time of the p'th probe
   static int get_start(int p, int m)
      { if ((unsigned int)p >= (unsigned int)PROBE_COUNT)   return -3;
        return probes[p].get_start(m);
      }

   /// get the m'th stop time of this probe
   int64_t get_stop(int m) const
      { if ((unsigned int)m >= (unsigned int)idx)   return -1;
        return measurements[m].cycles_to;
      }

   /// get the m'th stop time of the p'th probe
   static int get_stop(int p, int m)
      { if ((unsigned int)p >= (unsigned int)PROBE_COUNT)   return -3;
        return probes[p].get_stop(m);
      }

   /// get the number of times int the p'th probe
   static int get_length(int p)
      { if ((unsigned int)p >= (unsigned int)PROBE_COUNT)   return -3;
        return probes[p].idx;
      }

   static Probe & P0;    ///< start of vector probe
   static Probe & P_1;   ///< individual probe 1
   static Probe & P_2;   ///< individual probe 2
   static Probe & P_3;   ///< individual probe 3
   static Probe & P_4;   ///< individual probe 4
   static Probe & P_5;   ///< individual probe 5

protected:
   struct measurement
      {
        int64_t cycles_from;   ///< the cycle counter at point 1
        int64_t cycles_to;     ///< the cycle counter at point 2
      };

   measurement measurements[PROBE_LEN];

   int idx;
   int64_t * start_p;
   int64_t * stop_p;

   static int64_t dummy;
   static Probe probes[];
};
//-----------------------------------------------------------------------------
/// Year, Month, Day, hour, minute, second, millisecond
struct YMDhmsu
{
   /// construct YMDhmsu from usec since Jan. 1. 1970 00:00:00
   YMDhmsu(APL_time_us t);

   /// return usec since Jan. 1. 1970 00:00:00
   APL_time_us get() const;

   int year;     ///< year, e.g. 2013
   int month;    ///< month 1-12
   int day;      ///< day 1-31
   int hour;     ///< hour 0-23
   int minute;   ///< minute 0-59
   int second;   ///< second 0-59
   int micro;    ///< microseconds 0-999999
};

#ifdef TROUBLESHOOT_NEW_DELETE
inline void * operator new(size_t size)   { return common_new(size); }
inline void   operator delete(void * p)   { common_delete(p); }
#endif

using namespace std;

//-----------------------------------------------------------------------------

/// return true iff \b uni is a padding character (used internally).
inline bool is_iPAD_char(Unicode uni)
   { return ((uni >= UNI_iPAD_U2) && (uni <= UNI_iPAD_U1)) ||
            ((uni >= UNI_iPAD_U0) && (uni <= UNI_iPAD_L9)); }

//-----------------------------------------------------------------------------
// dynamic arrays. Some platforms don't support them and we fix that here.

#if HAVE_DYNAMIC_ARRAYS
   //
   // the platform supports dynamic arrays
   //
# define DynArray(Type, Name, Size) Type Name[Size];

#else // not HAVE_DYNAMIC_ARRAYS
   //
   // the platform does not support dynamic arrays
   //
   template<typename Type>
   class __DynArray
      {
        public:
           __DynArray(ShapeItem len)
              { data = new Type[len]; }

           const Type & operator[](ShapeItem idx) const
               { return data[idx]; }

           Type & operator[](ShapeItem idx)
               { return data[idx]; }

           ~__DynArray()
              { delete[] data; }

        protected:
           Type * data;
      };

# define DynArray(Type, Name, Size) __DynArray<Type> Name(Size);

#endif // HAVE_DYNAMIC_ARRAYS

//-----------------------------------------------------------------------------

/// Stringify x.
#define STR(x) #x
/// The current location in the source file.
#define LOC Loc(__FILE__, __LINE__)
/// The location line l in file f.
#define Loc(f, l) f ":" STR(l)

extern ostream & get_CERR();

/// print something and the source code location
#define Q(x) get_CERR() << std::left << setw(20) << #x ":" << " '" << x << "' at " LOC << endl;

/// same as Q1 (for printouts guarded by Log macros). Unlike Q() which should
/// NOT remain in the code Q1 should remain in the code.
#define Q1(x) get_CERR() << std::left << setw(20) << #x ":" << " '" << x << "' at " LOC << endl;

//-----------------------------------------------------------------------------

/// The total memory that we have.
extern uint64_t total_memory;

#ifdef VALUE_HISTORY_WANTED

   enum { VALUEHISTORY_SIZE = 100000 };
   extern void add_event(const Value * val, VH_event ev, int ia,
                        const char * loc);
#  define ADD_EVENT(val, ev, ia, loc)   add_event(val, ev, ia, loc);

#else

   enum { VALUEHISTORY_SIZE = 0 };
#  define ADD_EVENT(_val, _ev, _ia, _loc)

#endif

/// Function_Line ++ (post increment)
inline int operator ++(Function_Line & fl, int)   { return ((int &)fl)++; }

/// Function_PC ++ (post increment)
inline Function_PC
operator ++(Function_PC & pc, int)
{
const Function_PC ret = pc; 
   pc = Function_PC(pc + 1);
   return ret;
}

/// Function_PC ++ (pre increment)
inline Function_PC &
operator ++(Function_PC & pc)
{
   pc = Function_PC(pc + 1);
   return pc;
}

/// Function_PC -- (pre decrement)
inline Function_PC &
operator --(Function_PC & pc)
{
   pc = Function_PC(pc - 1);
   return pc;
}
//-----------------------------------------------------------------------------

#define uhex  std::hex << uppercase << setfill('0')
#define lhex  std::hex << nouppercase << setfill('0')
#define nohex std::dec << nouppercase << setfill(' ')

/// formatting for hex (and similar) values
#define HEX(x)     "0x" << uhex <<                 int64_t(x) << nohex
#define HEX2(x)    "0x" << uhex << std::right << \
                           setw(2) << int(x) << std::left << nohex
#define HEX4(x)    "0x" << uhex << std::right << \
                           setw(4) << int(x) << std::left << nohex
#define UNI(x)     "U+" << uhex <<          setw(4) << int(x) << nohex

//-----------------------------------------------------------------------------

#include "SharedValuePointer.hh"

#endif // __COMMON_HH_DEFINED__
