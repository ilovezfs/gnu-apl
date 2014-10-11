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

#ifndef __VALUEHISTORY_HH_DEFINED__
#define __VALUEHISTORY_HH_DEFINED__

#include "Common.hh"
#include <ostream>

using namespace std;

class Value;

/// a ringbuffer containing events that are related to the manipulation
/// of values
class VH_entry
{
   friend void print_history(ostream & out, const Value * val,
                             const char * loc);

public:
   /// Constructor: empty (invalid) event
   VH_entry() {}

   /// Constructor: event for \b val
   VH_entry(const Value * val, VH_event ev, int ia, const char * loc);

   /// init the event history
   static void init();

   /// ring buffer of events
   static VH_entry history[VALUEHISTORY_SIZE + 1];   // +1 for Solaris

   /// next event in history (which is a ring buffer)
   static int idx;

protected:
   /// print the event
   void print(int & flags, ostream & out, const Value * val,
               const VH_entry * previous) const;

   /// the Value to which this event belongs
   const Value * val;

   /// the event number
   VH_event event;

   /// integer arg (the meaning depends on event)
   int iarg;

   /// the caller (source location) of the VH_entry constructor
   const char  * loc;

   /// the testcase file (if any) where the value was created
   const char  * testcase_file;

   /// the line number in the testcase file
   int           testcase_line;
};

extern void print_history(ostream & out, const Value * val,
                                const char * loc);
#endif // __VALUEHISTORY_HH_DEFINED__
