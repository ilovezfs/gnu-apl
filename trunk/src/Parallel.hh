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

#include <ostream>
#include <semaphore.h>
#include <vector>

#include "Cell.hh"

#define PRINT_LOCKED(x) \
   { sem_wait(&Parallel::print_sema); x; sem_post(&Parallel::print_sema); }

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

  occur before and after the master thread waits for terminal input. That is,
  the worker threads are blocked while the master therad waits for terminal
  input.

  The transitions

        busy-waiting ←→ working

  occur when the execution of APL primitives, primarily scalar functions,
  suggests to execute in parallel (i.e. if the vectors involved are
  sufficiently long).

  In order to speed up the above transitions, they are not performed by
  the master thread (which would need O(P) cycles for fork()ing or join()ing
  P thrads) but by the worker threads (inluding the master) themselves.
  This brings down the fork/join() times from O(P) to O(log P). While this
  is not an issue for small core counts (like for a typical 4-core PC) we have
  seen this as a problem on an 80-core machine that we benchmarked.

  The TaskTree class manages the relation (who is fork()/join()ing whom) of
  the different threads. It is (re-)initialized only after core_count()
  has changed.
 **/
class TaskTree
{
public:
   static void init(CoreCount count, bool logit);

   /// print all TaskTree nodes
   static void print_all(ostream & out);

   /// print this TaskTree node
   void print(ostream & out) const;

   const CoreNumber * get_forked() const
      { return forked; }

   CoreCount get_forked_count() const
      { return forked_count; }

   CoreNumber get_forker() const
      { return forker; }

   static CoreCount get_task_count()
      { return task_count; }

   static const TaskTree & get_entry(CoreNumber n)
      { return entries[n]; }

protected:
   /// constructor
   TaskTree();

   /// destructor
   ~TaskTree();

   /// the task number of this entry (0 ... core_count-1)
   CoreNumber N;

   /// the task that fork()ed this task and that is join()ed after work.
   CoreNumber forker;

   /// the number of tasks that are fork()ed by this task
   CoreCount forked_count;

   /// the tasks that are fork()ed by this task
   CoreNumber * forked;

   void init_entry(CoreNumber n);

   static TaskTree * entries;

   static CoreCount task_count;

   static int log_task_count;
};
//=============================================================================
/// the argument of a function that is executed by the pool (i.e. in parallel)
class Thread_context;

/// a function that is executed by the pool (i.e. in parallel)
typedef void PoolFunction(Thread_context & ctx);

class Thread_context
{
public:
   /// constructor
   Thread_context();

   CoreNumber get_N() const
       { return N; }

   /// busy-wait until the thread that has forked us has reached new_mileage
   static void wait_for_master_mileage(int new_mileage)
      {
        while (get_master().mileage != new_mileage)   /* busy wait */ ;
      }

   /// busy-wait until the threads that we forked have reached new_mileage
   void wait_for_forked_mileage(int new_mileage) const
      {
         for (int f = 0; f < forked_threads_count; ++f)
             {
              // wait until sub thread f has moved forward
              //
               const CoreNumber sub = forked_threads[f];
               const Thread_context & sub_ctx = thread_contexts[sub];
               while (sub_ctx.mileage != new_mileage)   /* busy wait */ ;
             }
      }

   /// start parallel execution of work in a sub-tree. This is NOT done
   /// in a recursive fashion but simply by waiting for the master to
   /// increase its mileage
   void PF_fork()
      {
        wait_for_master_mileage(mileage + 1);
      }

   /// start parallel execution of work at the master
   static void M_fork()
      {
        get_master().increment_mileage();
      }
   /// end parallel execution of work
   void PF_join()
      {
        wait_for_forked_mileage(mileage + 1);
        increment_mileage();
      }

   void M_join()
      {
        wait_for_forked_mileage(mileage);
      }

   void increment_mileage()
      { ++mileage; }

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

   /// lock all pool members on pool_sema
   void M_lock_pool();

   /// unlock all pool members from pool_sema
   void unlock_forked();

   /// terminate all pool tasks
   void M_kill_pool();

   /// the next work to be done
   static PoolFunction * do_work;

   /// a thread-function that should not be called
   static PoolFunction PF_no_work;

   /// block/unblock on the pool semaphore
   static PoolFunction PF_lock_unlock_pool;

   /// terminate this thread
   static PoolFunction PF_kill_pool;

protected:
   /// thread number (0 = interpreter, 1... worker threads
   CoreNumber N;

   /// the counter that controls forking and joining of threads
   volatile int mileage;

   void init_entry(CoreNumber n);

   const CoreNumber * forked_threads;

   CoreCount forked_threads_count;

   CoreNumber join_thread;

   /// the cpu core to which this thread is bound
   CPU_Number CPU;

public:
   /// the thread executing this context
   pthread_t thread;

   /// a semaphore to block this context
   sem_t pool_sema;

protected:
   static Thread_context * thread_contexts;

   static CoreCount thread_contexts_count;
};
//=============================================================================
class PrimitiveFunction;

/**
 The description of one flat parallel job
 **/
struct Parallel_job
{
   /// the length of the result
   ShapeItem len_Z;

   /// ravel of the result
   Cell * cZ;

   /// ravel of the left argument
   const Cell * cA;

   /// 0 (for scalar A) or 1
   int inc_A;

   /// ravel of the right argument
   const Cell * cB;

   /// 0 (for scalar B) or 1
   int inc_B;

   /// the APL function being computed
   PrimitiveFunction * fun;

   /// the monadic cell function to be computed
   prim_f1 fun1;

   /// the dyadic cell function to be computed
   prim_f2 fun2;

   /// an error detected during computation of, eg. fun1 or fun2
   ErrorCode error;

   /// return A[z]
   const Cell & A_at(ShapeItem z) const
      { return cA[z * inc_A]; }

   /// return B[z]
   const Cell & B_at(ShapeItem z) const
      { return cB[z * inc_B]; }

   /// return Z[z]
   Cell & Z_at(ShapeItem z) const
      { return cZ[z]; }

};
//=============================================================================
class Parallel
{
public:
   /// true if parallel execution is enabled
   static const bool run_parallel;

   /// number of currently used cores
   static CoreCount get_active_core_count()
      { return active_core_count; }

   /// number of available cores
   static CPU_count get_total_CPU_count()
      { return (CPU_count)(all_CPUs.size()); }

   /// lock all pool members on pool_sema
   static void lock_pool()
      { if (active_core_count > 1)
           Thread_context::get_master().M_lock_pool(); }

   /// unlock all pool members from pool_sema
   static void unlock_pool()
      { if (active_core_count > 1)
           Thread_context::get_master().unlock_forked(); }

   static void kill_pool()
      { if (active_core_count > 1)
           Thread_context::get_master().M_kill_pool(); }

   /// initialize (first)
   static void init(bool logit);

   /// re-initialize (subsequent)
   static void reinit(bool logit);

   /// set new active core count, return true on error
   static bool set_core_count(CoreCount count);

   static sem_t print_sema;

   static sem_t pthread_create_sema;

   static CPU_Number get_CPU(int idx)
      { return all_CPUs[idx]; }

   /// the worklist
   static vector<Parallel_job> parallel_jobs;

   /// set current job. CAUTION: parallel_jobs may be modified by the
   /// execution of current_job. Therefore we copy current_job from
   /// parallel_jobs (and can then safely use current_job &).
   static Parallel_job & set_current_job(ShapeItem todo_idx)
      { current_job = parallel_jobs[todo_idx];   return current_job; }

   static Parallel_job & get_current_job()
      { return current_job; }

protected:
   /// the number of cores currently used
   static CoreCount active_core_count;

   /// the main() function of the worker threads
   static void * worker_main(void *);

   /// initialize all_CPUs and active_core_count
   static void init_CPUs(bool logit);

   /// the core numbers (as per pthread_getaffinity_np())
   static vector<CPU_Number>all_CPUs;

   /// the currently executed worklist item
   static Parallel_job current_job;

   /// a semaphore protecting worklist insertion
   static sem_t todo_sema;

   /// a semaphore protecting Value constructors
   static sem_t new_value_sema;
};
//=============================================================================

#endif // __PARALLEL_HH_DEFINED__