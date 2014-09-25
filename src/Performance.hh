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
        PFS_MIN1 = 0,
        PFS_MIN1_1 = PFS_MIN1 - 1,
#define perfo_1(id, name, _thr) PFS_ ## id,
#define perfo_2(id, name, _thr)
#define perfo_3(id, name)
#include "Performance.def"
        PFS_MAX1,
        PFS_MAX1_1 = PFS_MAX1 - 1,

        PFS_MIN2,
        PFS_MIN2_1 = PFS_MIN2 - 1,
#define perfo_1(id, name, _thr)
#define perfo_2(id, name, _thr) PFS_ ## id,
#define perfo_3(id, name)
#include "Performance.def"
        PFS_MAX2,
        PFS_MAX2_1 = PFS_MAX2 - 1,

        PFS_MIN3,
        PFS_MIN3_1 = PFS_MIN3 - 1,
#define perfo_1(id, name, _thr)
#define perfo_2(id, name, _thr)
#define perfo_3(id, name) PFS_ ## id,
#include "Performance.def"
        PFS_MAX3,
        PFS_MAX3_1 = PFS_MAX3 - 1,

        PFS_SCALAR_B_overhead,
        PFS_SCALAR_AB_overhead,
        PFS_ALL
};
//=============================================================================
class Statistics_record
{
public:
   /// constructor
   Statistics_record()   { reset(); }

   /// constructor
   Statistics_record(uint64_t cnt, uint64_t da, double da2)
   : count(cnt),
     data(da),
     data2(da2)
   {}

   /// reset record to count 0, mean 0, variance 0
   void reset()   { count = 0;   data = 0;   data2 = 0; }

   /// add one sample
   void add_sample(uint64_t val)
      { ++count;   data += val;   data2 += val*val; }

   /// print count, data, and data2
   void print(ostream & out, int l_N = 8, int l_u = 7);

   /// write count, data, and data2 to file
   void save_record(ostream & outf);

   uint64_t get_count() const
      { return count; }

   uint64_t get_sum() const
      { return data; }

   double get_sum2() const
      { return data2; }

protected:
   /// number of samples
   uint64_t count;

   /// sum of sample values
   uint64_t data;

   /// sum of squares of sample values
   double data2;   // can grow quickly!
};
//=============================================================================
class Statistics
{
public:
   /// constructor
   Statistics(Pfstat_ID _id)
  : id(_id)
   {}

   /// destructor
   ~Statistics();

   /// print statistics
   virtual void print(ostream & out) = 0;

   /// write statistics to file
   virtual void save_data(ostream & outf, const char * id_name) = 0;

   virtual void reset() = 0;

   virtual const Statistics_record * get_first_record() const
      { return 0; }

   virtual const Statistics_record * get_record() const
      { return 0; }

   const char * get_name() const
      { return get_name(id); }

   static const char * get_name(Pfstat_ID _id);

protected:
   const Pfstat_ID id;
};
//=============================================================================
/// performance counters for a APL function
class FunctionStatistics : public Statistics
{
public:
   FunctionStatistics(Pfstat_ID _id)
   : Statistics(_id)
   { reset(); }

   FunctionStatistics(Pfstat_ID _id, const Statistics_record & rec,
                      uint64_t lsum)
   : Statistics(_id),
     data(rec),
     len_sum(lsum)
   {}

   /// overloaded Statistics::print()
   virtual void reset()
        {
          data.reset();
          len_sum = 0;
        }

   /// overloaded Statistics::print()
   virtual void print(ostream & out);

   /// overloaded Statistics::save_data()
   virtual void save_data(ostream & outf, const char * id_name);

   virtual const Statistics_record * get_record() const
      { return  &data; }

   const Statistics_record & get_data() const
      { return data; }

   void add_sample(uint64_t val, uint64_t veclen)
      {
         data.add_sample(val);
         len_sum += veclen;
       }

protected:
   Statistics_record data;

   /// sum of vector lengths
   uint64_t len_sum;
};
//-----------------------------------------------------------------------------
/// performance counters for a cell level function
class CellFunctionStatistics : public Statistics
{
public:
   CellFunctionStatistics(Pfstat_ID _id)
   : Statistics(_id)
   { reset(); }

   virtual void reset()
        {
          first.reset();
          subsequent.reset();
        }

   void add_sample(uint64_t val, bool subseq)
      {
         if (subseq)   subsequent.add_sample(val);
         else          first.add_sample(val);
       }

   /// overloaded Statistics::print()
   virtual void print(ostream & out);

   /// overloaded Statistics::save_data()
   virtual void save_data(ostream & outf, const char * id_name);

   virtual const Statistics_record * get_first_record() const
      { return  &first; }

   virtual const Statistics_record * get_record() const
      { return  &subsequent; }

   uint64_t get_sum() const
      { return first.get_sum() + subsequent.get_sum(); }

   double get_sum2() const
      { return first.get_sum2() + subsequent.get_sum2(); }

   uint64_t get_count() const
      { return first.get_count() + subsequent.get_count(); }

protected:
   Statistics_record first;
   Statistics_record subsequent;
};
//=============================================================================
/**
     Performance (cycle-) counters at different levels
 **/
class Performance
{
public:

   /// return statistics object for ID \b id
   static Statistics * get_statistics(Pfstat_ID id);

   /// return statistics type of ID \b id
   static int get_statistics_type(Pfstat_ID id);

   // print all counters
   static void print(Pfstat_ID which, ostream & out);

   // write all counters to .def file
   static void save_data(ostream & out, ostream & out_file);

   // reset all counters
   static void reset_all();

#define perfo_1(id, name, thr)                 \
   /** monadic cell function statistics **/    \
   static CellFunctionStatistics cfs_ ## id;   \
   /** monadic parallel executionthreshold **/ \
   static const ShapeItem thresh_ ## id = thr;

#define perfo_2(id, name, thr)                \
   /** dyadic cell function statistics **/    \
   static CellFunctionStatistics cfs_ ## id;  \
   /** dyadic parallel executionthreshold **/ \
   static const ShapeItem thresh_ ## id = thr;

#define perfo_3(id, name)   \
   static FunctionStatistics fs_ ## id;   ///< function statistics
#include "Performance.def"
};

#endif // __PERFORMANCE_HH_DEFINED__
