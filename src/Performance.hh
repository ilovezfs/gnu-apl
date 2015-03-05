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

#include "../config.h"

#if defined(PERFORMANCE_COUNTERS_WANTED) && defined(HAVE_RDTSC)

# define PERFORMANCE_START(counter) const uint64_t counter = cycle_counter();
# define PERFORMANCE_END(statistics, counter, len) \
   Performance::statistics.add_sample(cycle_counter() - counter, len);
# define CELL_PERFORMANCE_END(get_stat, counter, subseq)   \
   { const uint64_t end = cycle_counter();                 \
     CellFunctionStatistics * stat = get_stat;             \
     if (stat)   stat->add_sample(end - counter, subseq); }



#else

# define PERFORMANCE_START(counter)
# define PERFORMANCE_END(statistics, counter, len)
# define CELL_PERFORMANCE_END(get_stat, counter, subseq)

#endif

using namespace std;

//-----------------------------------------------------------------------------
/// performance statistics IDs
enum Pfstat_ID
{
        PFS_MIN1 = 0,
        PFS_MIN1_1 = PFS_MIN1 - 1,
#define perfo_1( id,  ab, name, _thr) PFS_ ## id ## ab,
#define perfo_2(_id, _ab, name, _thr)
#define perfo_3(_id, _ab, name, _thr)
#define perfo_4(_id, _ab, name, _thr)
#include "Performance.def"
        PFS_MAX1,
        PFS_MAX1_1 = PFS_MAX1 - 1,

        PFS_MIN2,
        PFS_MIN2_1 = PFS_MIN2 - 1,
#define perfo_1(_id, _ab, name, _thr)
#define perfo_2( id,  ab, name, _thr) PFS_ ## id ## ab,
#define perfo_3(_id, _ab, name, _thr)
#define perfo_4(_id, _ab, name, _thr)
#include "Performance.def"
        PFS_MAX2,
        PFS_MAX2_1 = PFS_MAX2 - 1,

        PFS_MIN3,
        PFS_MIN3_1 = PFS_MIN3 - 1,
#define perfo_1(_id, _ab, name, _thr)
#define perfo_2(_id, _ab, name, _thr)
#define perfo_3( id,  ab, name, _thr) PFS_ ## id ## ab,
#define perfo_4( id,  ab, name, _thr) PFS_ ## id ## ab,
#include "Performance.def"
        PFS_MAX3,
        PFS_MAX3_1 = PFS_MAX3 - 1,

        PFS_SCALAR_B_overhead,
        PFS_SCALAR_AB_overhead,
        PFS_ALL
};
//=============================================================================
/// one statistics for computing the mean and the variance of samples
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
   void print(ostream & out);

   /// write count, data, and data2 to file
   void save_record(ostream & outf);

   /// return the number of samples
   uint64_t get_count() const
      { return count; }

   /// return the sum of samples
   uint64_t get_sum() const
      { return data; }

   /// return the average
   uint64_t get_average() const
      { return count ? data/count : 0; }

   /// return the sum of squares
   double get_sum2() const
      { return data2; }

   /// print num as 5 characters (digits, dot, and multiplier (k, m, g, ...
   static void print5(ostream & out, uint64_t num);

protected:
   /// number of samples
   uint64_t count;

   /// sum of sample values
   uint64_t data;

   /// sum of squares of sample values
   double data2;   // can grow quickly!
};
//=============================================================================
/// base class for different kinds of statistics
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
   virtual void save_data(ostream & outf, const char * perf_name) = 0;

   /// reset \b this statistics
   virtual void reset() = 0;

   /// return the statistics for the first passes
   virtual const Statistics_record * get_first_record() const
      { return 0; }

   /// return the statistics for the subsequent passes
   virtual const Statistics_record * get_record() const
      { return 0; }

   /// return the name of \b this statistics
   const char * get_name() const
      { return get_name(id); }

   /// return the name of the statistics with ID \b id
   static const char * get_name(Pfstat_ID id);

protected:
   /// the ID of \b this statistics
   const Pfstat_ID id;
};
//=============================================================================
/// performance counters for a APL function
class FunctionStatistics : public Statistics
{
public:
   /// constructor: FunctionStatistics with ID \b id
   FunctionStatistics(Pfstat_ID id)
   : Statistics(id)
   { reset(); }

   /// overloaded Statistics::print()
   virtual void reset()
        {
          vec_cycles.reset();
          vec_lengths.reset();
        }

   /// overloaded Statistics::print()
   virtual void print(ostream & out);

   /// overloaded Statistics::save_data()
   virtual void save_data(ostream & outf, const char * perf_name);

   /// return the (CPU-)cycles statistics
   virtual const Statistics_record * get_record() const
      { return &vec_cycles; }

   /// return the (CPU-)cycles statistics
   const Statistics_record & get_data() const
      { return vec_cycles; }

   /// add a sample
   void add_sample(uint64_t val, uint64_t veclen)
      {
         vec_cycles.add_sample(val);
         vec_lengths.add_sample(veclen);
       }

protected:
   /// the vector lengths
   Statistics_record vec_lengths;

   /// the cycles executed
   Statistics_record vec_cycles;
};
//-----------------------------------------------------------------------------
/// performance counters for a cell level function
class CellFunctionStatistics : public Statistics
{
public:
   /// constructor: reset this statistics
   CellFunctionStatistics(Pfstat_ID _id)
   : Statistics(_id)
   { reset(); }

   /// reset this statistics: clear \b first and \b subsequent records
   virtual void reset()
        {
          first.reset();
          subsequent.reset();
        }

   /// add a sample to \b this statistics
   void add_sample(uint64_t val, bool subseq)
      {
         if (subseq)   subsequent.add_sample(val);
         else          first.add_sample(val);
       }

   /// overloaded Statistics::print()
   virtual void print(ostream & out);

   /// overloaded Statistics::save_data()
   virtual void save_data(ostream & outf, const char * perf_name);

   /// return the record for the first executions
   virtual const Statistics_record * get_first_record() const
      { return  &first; }

   /// return the record for subsequent executions
   virtual const Statistics_record * get_record() const
      { return  &subsequent; }

   /// return the cycles sum
   uint64_t get_sum() const
      { return first.get_sum() + subsequent.get_sum(); }

   /// return the square of cycles sums
   double get_sum2() const
      { return first.get_sum2() + subsequent.get_sum2(); }

   /// return the number of first executions
   uint64_t get_count1() const
      { return first.get_count(); }

   /// return the number of subsequent executions
   uint64_t get_countN() const
      { return subsequent.get_count(); }

   /// return the number of all executions
   uint64_t get_count() const
      { return first.get_count() + subsequent.get_count(); }

protected:
   /// statistics for first executions
   Statistics_record first;

   /// statistics for subsequent executions
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

   /// print all counters
   static void print(Pfstat_ID which, ostream & out);

   /// write all counters to .def file
   static void save_data(ostream & out, ostream & out_file);

   /// reset all counters
   static void reset_all();

#define perfo_1(id, ab, name, thr)                   \
   /** monadic cell function statistics **/          \
   static CellFunctionStatistics cfs_ ## id ## ab;   \
   /** monadic parallel executionthreshold **/       \
   static const ShapeItem thresh_ ## id ## ab = thr;

#define perfo_2(id, ab, name, thr)                   \
   /** dyadic cell function statistics **/           \
   static CellFunctionStatistics cfs_ ## id ## ab;   \
   /** dyadic parallel executionthreshold **/        \
   static const ShapeItem thresh_ ## id ## ab = thr;

#define perfo_3(id, ab, name, thr)                   \
   /** function statistics **/                       \
   static FunctionStatistics fs_ ## id ## ab;

#define perfo_4(id, ab, name, thr)                   \
   /** function statistics **/                       \
   static FunctionStatistics fs_ ## id ## ab;

#include "Performance.def"
};

#endif // __PERFORMANCE_HH_DEFINED__
