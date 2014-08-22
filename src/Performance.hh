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
//=============================================================================
class Statistics
{
public:
   /// constructor
   Statistics();

   /// destructor
   ~Statistics();

   /// print statistics
   virtual void print(ostream & out, int max_namelen) = 0;

   /// write statistics to file
   virtual void save_data(ostream & outf, const char * id_name) = 0;
};
//=============================================================================
class Statistics_record
{
public:
   /// constructor
   Statistics_record()   { reset(); }

   /// reset record to count 0, mean 0, variance 0
   void reset()   { count = 0;   data = 0;   data2 = 0; }

   /// add one sample
   void add_sample(uint64_t val)
      { ++count;   data += val;   data2 += val*val; }

   /// print count, data, and data2
   void print(ostream & out);

   /// write count, data, and data2 to file
   void save_record(ostream & outf);

protected:
   /// number of samples
   uint64_t count;

   /// sum of sample values
   uint64_t data;

   /// sum of squares of sample values
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

   /// overloaded Statistics::print()
   virtual void print(ostream & out, int max_namelen);

   /// overloaded Statistics::save_data()
   virtual void save_data(ostream & outf, const char * id_name);

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

   // write all counters to .def file
   static void save_data(ostream & out, ostream & out_file);

   // reset all counters
   static void reset_all();

#define perfo(id, name)   \
   static CellFunctionStatistics cfs_ ## id;   ///< cell function statistics
#include "Performance.def"
};

#endif // __PERFORMANCE_HH_DEFINED__
