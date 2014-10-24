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

#ifndef __PARALLEL_HH_DEFINED__
#define __PARALLEL_HH_DEFINED__

#include "../config.h"

// set PARALLEL_ENABLED if wanted and its prerequisites are satisfied
//
#if CORE_COUNT_WANTED == 0
   //
   // parallel not wanted
   //
# undef PARALLEL_ENABLED

#elif HAVE_AFFINITY_NP
   //
   // parallel wanted and pthread_setaffinity_np() supported
   //
# define PARALLEL_ENABLED 1

#else
   //
   // parallel wanted, but pthread_setaffinity_np() and friends are missing
   //
#warning "CORE_COUNT_WANTED configured, but pthread_setaffinity_np() missing"
# undef PARALLEL_ENABLED

#endif

// define some atomic functions (even if the platform does not support them)
//
#if HAVE_EXT_ATOMICITY_H

#include <ext/atomicity.h>

inline int atomic_fetch_add(volatile _Atomic_word & counter, int increment)
   { _GLIBCXX_READ_MEM_BARRIER;
     const int ret = __gnu_cxx::__exchange_and_add_dispatch(
                                        (_Atomic_word *)&counter, increment);
     _GLIBCXX_WRITE_MEM_BARRIER;
     return ret; }

inline int atomic_read(volatile _Atomic_word & counter)
   { _GLIBCXX_READ_MEM_BARRIER;
     return __gnu_cxx::__exchange_and_add_dispatch(
                                        (_Atomic_word *)&counter, 0); }

inline void atomic_add(volatile _Atomic_word & counter, int increment)
   { __gnu_cxx::__atomic_add_dispatch((_Atomic_word *)&counter, increment);
     _GLIBCXX_WRITE_MEM_BARRIER;
   }

#elif HAVE_OSX_ATOMIC

#include <libkern/OSAtomic.h>

typedef int32_t _Atomic_word;

inline int atomic_fetch_add(volatile _Atomic_word & counter, int increment)
   { return OSAtomicAdd32Barrier(increment, &counter) - increment; }

inline int atomic_read(volatile _Atomic_word & counter)
   { return OSAtomicAdd32Barrier(0, &counter); }

inline void atomic_add(volatile _Atomic_word & counter, int increment)
   { OSAtomicAdd32Barrier(increment, &counter); }

#elif HAVE_SOLARIS_ATOMIC

#include <atomic.h>

typedef uint32_t _Atomic_word;

inline int atomic_fetch_add(volatile _Atomic_word & counter, int increment)
   { return atomic_add_32_nv(&counter, increment) - increment; }

inline int atomic_read(volatile _Atomic_word & counter)
   { return atomic_add_32_nv(&counter, 0); }

inline void atomic_add(volatile _Atomic_word & counter, int increment)
   { atomic_add_32(&counter, increment); }

#else

typedef int _Atomic_word;

inline int atomic_fetch_add(volatile _Atomic_word & counter, int increment)
   { return counter; }

inline int atomic_read(volatile _Atomic_word & counter)
   { return counter; }

inline void atomic_add(volatile _Atomic_word & counter, int increment)
   { counter += increment; }

#endif

#include <ostream>
#include <semaphore.h>
#include <vector>

#include "Cell.hh"

class Value;

#define PRINT_LOCKED(x) \
   { sem_wait(&Parallel::print_sema); x; sem_post(&Parallel::print_sema); }

#define POOL_LOCK(l, x) \
   { Parallel::acquire_lock(l); { x; } Parallel::release_lock(l); }

using namespace std;

/// the number of cores/tasks to be used
enum CoreCount
{
  CCNT_UNKNOWN = -1,   ///< unknown core count
  CCNT_0       = 0,    ///<< no core
  CCNT_1       = 1,    ///< one core ...
};

/// the cores/tasks to be used
enum CoreNumber
{
  CNUM_INVALID = -1,  ///< invalid core
  CNUM_MASTER  = 0,   ///< the interpreter core
  CNUM_WORKER1 = 1,   ///< the first worker core ...
};

/// the CPUs reported by the OS
enum CPU_Number
{
   CPU_0 = 0
};

/// the number of CPUs available
enum CPU_count
{
   CPU_CNT_1 = 1
};
//=============================================================================
/**
  Multi-core GNU APL uses a pool of threads numbered 0, 1, ... core_count()-1

  (master-) thread 0 is the interpreter (which also performs parallel work),
  while (worker-) threads 1 ... are only activated when some paralle work
  is available.

  The worker threads 1... are either working, or busy-waiting for work, or
  blocked on a semaphore:

         init
          ↓
       blocked ←→ busy-waiting ←→ working

  The transitions

      blocked ←→ busy-waiting

  occur before and after the master thread waits for terminal input or when
  the number of cores is changed (with ⎕SYL[26;2]). That is,
  the worker threads are blocked while the master thread waits for terminal
  input.

  The transitions

        busy-waiting ←→ working

  occur when the execution of APL primitives, primarily scalar functions,
  suggests to execute in parallel (i.e. if the vectors involved are
  sufficiently long).

 **/
//=============================================================================
class Thread_context;

class Thread_context
{
public:
   /// a function that is executed by the pool (i.e. in parallel)
   typedef void PoolFunction(Thread_context & ctx);

   /// constructor
   Thread_context();

   CoreNumber get_N() const
       { return N; }

   /// start parallel execution of work at the master
   static void M_fork(const char * jname)
      {
        get_master().job_name = jname;
        atomic_add(busy_worker_count, active_core_count - 1);
        atomic_add(get_master().job_number, 1);
      }

   /// start parallel execution of work in a worker
   void PF_fork()
      {
        while (atomic_read(get_master().job_number) == job_number)
              /* busy wait */ ;
      }

   /// end parallel execution of work at the master
   static void M_join()
      {
        while (atomic_read(busy_worker_count) != 0)   /* busy wait */ ;
      }

   /// end parallel execution of work in a worker
   void PF_join()
      {
        atomic_add(busy_worker_count, -1);   // we are ready
        atomic_add(job_number, 1);            // we reached master job_number

        // wait until all workers finished or new job from master
        while (atomic_read(busy_worker_count) != 0 &&
               atomic_read(get_master().job_number) == job_number)
              /* busy wait */ ;
      }

   /// bind thread to core
   void bind_to_cpu(CPU_Number cpu, bool logit);

   /// print all TaskTree nodes
   static void print_all(ostream & out);

   /// print all mileages nodes
   static void print_mileages(ostream & out, const char * loc);

   /// print this TaskTree node
   void print(ostream & out) const;

   /// initialize all thread contexts (set all but N and thread)
   static void init(CoreCount thread_count, bool logit);

   static Thread_context * get_context(CoreNumber n)
      { return thread_contexts + n; }

   static Thread_context & get_master()
      { return thread_contexts[CNUM_MASTER]; }

   /// make all workers lock on pool_sema
   void M_lock_pool();

   /// terminate all worker threads
   static void kill_pool();

   /// the next work to be done
   static PoolFunction * do_work;

   /// a thread-function that should not be called
   static PoolFunction PF_no_work;

   /// block/unblock on the pool semaphore
   static PoolFunction PF_lock_unlock_pool;

   /// number of currently used cores
   static CoreCount get_active_core_count()
      { return active_core_count; }

   /// set the number of currently used cores
   static void set_active_core_count(CoreCount new_count)
      { active_core_count = new_count; }

protected:
   /// thread number (0 = interpreter, 1... worker threads
   CoreNumber N;

   void init_entry(CoreNumber n);

   /// the cpu core to which this thread is bound
   CPU_Number CPU;

public:
   /// the thread executing this context
   pthread_t thread;

   /// a counter controlling the start of parallel jobs
   volatile int job_number;

   const char * job_name;

   bool do_join;

   /// a semaphore to block this context
   sem_t pool_sema;

   /// true if blocked on pool_sema
   bool blocked;

protected:
   static Thread_context * thread_contexts;

   static CoreCount thread_contexts_count;

   /// the number of unfinished workers (excluding master)
   static volatile _Atomic_word busy_worker_count;

   /// the number of cores currently used
   static CoreCount active_core_count;
};
//=============================================================================
class Parallel
{
public:
   static void acquire_lock(volatile _Atomic_word & lock)
      {
         // chances are low that the lock is held. Therefore we try a simple
         // atomic_fetch_add() first and return on success.
         // This should not harm the lock because we do this only once per
         // thread and acquire_lock()
         //
         if (atomic_fetch_add(lock, 1) == 0)   return;
         atomic_add(lock, -1);   // undo the atomic_fetch_add()

         // the lock was busy
         //
         for (;;)
             {
               // Wait to see a 0 on the lock. This is to avoid that the
               // atomic_fetch_add() lock attempts occupy the lock without
               // actually obtaining the lock. Waiting for 0 guarantees that
               // at least one thread succeeds below.
               //
               if (atomic_read(lock))   continue;   // not 0: try again

               if (atomic_fetch_add(lock, 1) == 0)   return;
             }
      }

   static void release_lock(volatile _Atomic_word & lock)
      {
        atomic_add(lock, -1);
      }

   /// true if parallel execution is enabled
   static bool run_parallel;

   /// number of available cores
   static CoreCount get_max_core_count()
      { return (CoreCount)(all_CPUs.size()); }

   /// make all pool members lock on their pool_sema
   static void lock_pool(bool logit);

   /// unlock all pool members from their pool_sema
   static void unlock_pool(bool logit);

   /// initialize
   static void init(bool logit);

   /// set new active core count, return true on error
   static bool set_core_count(CoreCount new_count, bool logit);

   static sem_t print_sema;

   static sem_t pthread_create_sema;

   static CPU_Number get_CPU(int idx)
      { return all_CPUs[idx]; }

protected:
   /// the main() function of the worker threads
   static void * worker_main(void *);

   /// initialize \b all_CPUs (which then determines the max. core count)
   static void init_all_CPUs(bool logit);

   /// the CPU numbers that can be used
   static vector<CPU_Number>all_CPUs;
};
//=============================================================================
/// a number of jobs to be executed in parallel
class Parallel_job_list_base
{
public:
   /// a lock protecting \b jobs AND Value::Value() constructors
   static volatile _Atomic_word parallel_jobs_lock;

   /// from where the joblist was started
   static const char * started_loc;
};

template <class T>
class Parallel_job_list : public Parallel_job_list_base
{
public:
   Parallel_job_list()
   : idx(0)
   {} 

   void start(const T & first_job, const char * loc)
      {
         if (started_loc)
            {
              PRINT_LOCKED(
              CERR << endl << "*** Attempt to start a new joblist at " << loc
                   << " while joblist from " << started_loc
                   << " is not finished" << endl;
              Backtrace::show(__FILE__, __LINE__))
            }

         started_loc = loc;
         idx = 0;
         jobs.clear();
         jobs.push_back(first_job);
      }

   /// cancel a job list
   void cancel_jobs()
      { idx = jobs.size();   started_loc = 0; }

   /// set current job. CAUTION: jobs may be modified by the
   /// execution of current_job. Therefore we copy current_job from
   /// jobs (and can then safely use current_job *).
   T * next_job()
      { if (idx == jobs.size()) { started_loc = 0;   return 0; }
        current_job = jobs[idx++];   return &current_job; }

   /// return the current job
   T & get_current_job()
      { return current_job; }

   /// return the current size
   int get_size()
      { return jobs.size(); }

   /// return the current index
   int get_index()
      { return idx; }

   void add_job(T & job)
      { jobs.push_back(job); }

   bool busy()
      { return started_loc != 0; }

   const char * get_started_loc()
      { return started_loc; }

protected:
   vector<T> jobs;
   int idx;

   /// the currently executed worklist item
   T current_job;
};
//=============================================================================

#endif // __PARALLEL_HH_DEFINED__
