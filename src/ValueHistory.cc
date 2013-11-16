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

#include <string.h>

#include <iomanip>
#include <vector>

#include "Common.hh"
#include "Error.hh"
#include "PrintOperator.hh"
#include "TestFiles.hh"
#include "UCS_string.hh"
#include "ValueHistory.hh"

VH_entry VH_entry::history[VALUEHISTORY_SIZE];
int VH_entry::idx = 0;

//----------------------------------------------------------------------------
VH_entry::VH_entry(const Value * _val, VH_event _ev, const char * _loc)
  : event(_ev),
    val(_val),
    loc(_loc)
{
   testcase_file = TestFiles::get_testfile_name();
   testcase_line = TestFiles::get_current_lineno();
}
//----------------------------------------------------------------------------
void
VH_entry::init()
{
   memset(history, 0, sizeof(history));
}
//----------------------------------------------------------------------------
void
add_event(const Value * val, VH_event ev, const char * loc)
{
VH_entry * entry = VH_entry::history + VH_entry::idx++;
   if (VH_entry::idx >= VALUEHISTORY_SIZE)   VH_entry::idx = 0;

   new(entry) VH_entry(val, ev, loc);
}
//----------------------------------------------------------------------------
void
VH_entry::print_history(ostream & out, const Value * val)
{
   // search backwards for events of val.
   //
vector<const VH_entry *> var_events;
int cidx = idx;

   loop(e, VALUEHISTORY_SIZE)
       {
         --cidx;
         if (cidx < 0)   cidx = VALUEHISTORY_SIZE - 1;

         const VH_entry * entry = history + cidx;
         
         if (entry->event == VHE_NONE)     break;      // end of history

         if ((entry->event & VHE_MASK) == VHE_ERROR)
            { 
              // add error event to every value history
              //
              var_events.push_back(entry);
              continue;
            }

          if (entry->val != val)            continue;   // some other var

          var_events.push_back(entry);

          if (entry->event == VHE_CREATE)   break;   // create event found
       }

   out << endl << "value " << (const void *)val
       << " has " << var_events.size()
       << " events in its history:" << endl;

int flags = 0;
   loop(e, var_events.size())
       var_events[var_events.size() - e - 1]->print(flags, out, val);
   out << endl;
}
//----------------------------------------------------------------------------
static UCS_string
flags_name(ValueFlags flags)
{
UCS_string ret;

  if (flags & VF_marked)   ret += UNI_ASCII_M;
  if (flags & VF_complete) ret += UNI_ASCII_C;
  if (flags & VF_left)     ret += UNI_ASCII_AMPERSAND;
  if (flags & VF_arg)      ret += UNI_ASCII_A;
  if (flags & VF_eoc)      ret += UNI_ASCII_E;
  if (flags & VF_deleted)  ret += UNI_DELTA;
  if (flags & VF_index)    ret += UNI_SQUISH_QUAD;
  if (flags & VF_nested)   ret += UNI_ASCII_s;
  if (flags & VF_forever)  ret += UNI_INFINITY;
  if (flags & VF_assigned) ret += UNI_LEFT_ARROW;
  if (flags & VF_shared)   ret += UNI_NABLA;

   while (ret.size() < 4)   ret += UNI_ASCII_SPACE;
   return ret;
}
//----------------------------------------------------------------------------
void
VH_entry::print(int & flags, ostream & out, const Value * val) const
{
const ValueFlags flags_before = (ValueFlags)flags;

   switch(event & VHE_MASK)
      {
        case VHE_CREATE:
             out << "  Create     " << flags_before << " ";
             out << "            ";
             break;

        case VHE_UNROLL:
             out << "  Rollback   " << flags_before << " ";
             break;

        case VHE_CHECK:
             flags |= VF_complete;
             out << "  Check      " << flags_before << " ";
             if (flags_before != flags)   out << (ValueFlags)flags << " ";
             else                         out << "            ";
             break;

        case VHE_SETFLAG:
             flags |= event & ~VHE_MASK;
             out << "  Set " << flags_name((ValueFlags)event)
                 << "   " << flags_before << " ";
             if (flags_before != flags)   out << (ValueFlags)flags << " ";
             else                         out << "            ";
             break;

        case VHE_CLEARFLAG:
             flags &= ~(event & ~VHE_MASK);
             out << "  Clear " << flags_name((ValueFlags)event)
                 << " " << flags_before << " ";
             if (flags_before != flags)   out << (ValueFlags)flags << " ";
             else                         out << "            ";
             break;

        case VHE_ERASE:
             out << "  Erase      " << flags_before << " ";
             if (flags_before & VF_deleted)
                {
                  out << "** TWICE ** ";
                }
             else if (flags_before & VF_DONT_DELETE)
                {
                  out << "still owned ";
                }
             else
                {
                  flags |= VF_deleted;
                  if (flags_before != flags)   out << (ValueFlags)flags << " ";
                  else                         out << "            ";
                }
             break;

        case VHE_ERROR:
             out << "  " << setw(34)
                 << Error::error_name((ErrorCode)(event & ~VHE_MASK)) << " ";
             break;
        default:
             out << "Unknown event " << HEX(event) << endl;
             return;
      }

   out << left << setw(30) << loc;

   if (testcase_file)
      {
        const char * filename = testcase_file;
        if (strncmp(filename, "testcases/", 10) == 0)   filename += 10;
        out << filename << ":" << testcase_line;
      }
   out << endl;
}
//----------------------------------------------------------------------------

