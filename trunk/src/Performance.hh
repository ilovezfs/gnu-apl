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

#ifndef __PERFORMANCE_HH_DEFINED__
#define __PERFORMANCE_HH_DEFINED__

#include <stdint.h>

#include <iostream>

using namespace std;

//-----------------------------------------------------------------------------
/// performance statistics IDs
enum Pfstat_ID
{
#define perfo(id, name) PFS_ ## id,
#include "Performance.def"
        PFS_MAX,
        PFS_ALL
};

class Statistics
{
public:
   Statistics();

   ~Statistics();

   virtual void print(ostream & out, int max_namelen) = 0;
};

class Statistics_record
{
public:
   Statistics_record()   { reset(); }

   void reset()   { count = 0;   data = 0;   data2 = 0; }

   void add_sample(uint64_t val)
      { ++count;   data += val;   data2 += val*val; }

   void print(ostream & out);

protected:
   uint64_t count;
   uint64_t data;
   double   data2;   // can grow quickly!
};

/// performance counters for a cell level function
class CellFunctionStatistics : public Statistics
{
public:
   CellFunctionStatistics(Pfstat_ID _id, const char * _name)
   : id(_id),
     name(_name)
   { reset(); }

   void reset()
        {
          first.reset();
          subsequent.reset();
        }

   void add_sample(uint64_t val, bool subseq)
      {
         if (subseq)   subsequent.add_sample(val);
         else          first.add_sample(val);
       }

   const char * get_name() const
      { return name; }

   virtual void print(ostream & out, int max_namelen);

protected:
   const Pfstat_ID id;
   const char * name;

   Statistics_record first;
   Statistics_record subsequent;
};
/**
     Performance (cycle-) counters at different levels
 **/
class Performance
{
public:

   /// return statistics object for ID \b id
   static Statistics * get_statistics(Pfstat_ID id);

   // print all counters
   static void print(Pfstat_ID which, ostream & out);

   // reset all counters
   static void reset_all();

#define perfo(id, name)   \
   static CellFunctionStatistics cfs_ ## id;   ///< cell function statistics
#include "Performance.def"
};

#endif // __PERFORMANCE_HH_DEFINED__
