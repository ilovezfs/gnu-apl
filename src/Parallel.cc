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

#include <sched.h>

#include "../config.h"

#include "Common.hh"
#include "Parallel.hh"
#include "SystemVariable.hh"
#include "UserPreferences.hh"

TaskTree * TaskTree::entries = 0;
CoreCount TaskTree::task_count = CCNT_0;
int TaskTree::log_task_count = 0;

Thread_context * Thread_context::thread_contexts = 0;
CoreCount Thread_context::thread_contexts_count = CCNT_0;
Thread_context::PoolFunction * Thread_context::do_work
                                             = &Thread_context::PF_no_work;

sem_t Parallel::print_sema;
sem_t Parallel::pthread_create_sema;
vector<CPU_Number>Parallel::all_CPUs;
CoreCount Parallel::active_core_count;

#if CORE_COUNT_WANTED == 0
const bool run_parallel = false;
#else
const bool run_parallel = true;
#endif

//=============================================================================
TaskTree::TaskTree()
   : N(CNUM_INVALID),
     forker(CNUM_INVALID),
     forked_count(CCNT_0),
     forked(0)
{
}
//-----------------------------------------------------------------------------
TaskTree::~TaskTree()
{
   delete forked;
}
//-----------------------------------------------------------------------------
void
TaskTree::init(CoreCount count, bool logit)
{
   delete [] entries;

   Assert(count >= 1);
   task_count = count;
   log_task_count = 0;

   while ((1 << log_task_count) < task_count)   ++ log_task_count;

   entries = new TaskTree[task_count];
   loop(c, task_count)   entries[c].init_entry((CoreNumber)c);

   if (logit)   print_all(CERR);
}
//-----------------------------------------------------------------------------
void
TaskTree::init_entry(CoreNumber n)
{
   N = n;
   if (task_count <= 1)   return;

   forked = new CoreNumber[log_task_count];

   for (int dist = (1 << (log_task_count - 1)); dist; dist >>= 1)
       {
         const int mask = dist - 1;
         if (N & mask)   continue;

         const int peer = N ^ dist;

         if (peer >= task_count)   continue;
         if (N & dist)             continue;

         forked[forked_count] = (CoreNumber)peer;
         forked_count = (CoreCount)(forked_count + 1);
         entries[peer].forker = N;
       }
}
//-----------------------------------------------------------------------------
void
TaskTree::print_all(ostream & out)
{
   out << "TaskTree size: " << task_count << endl;

   loop(e, task_count)   entries[e].print(out);
}
//-----------------------------------------------------------------------------
void
TaskTree::print(ostream & out) const
{
   out << "Task #" << N << ": forked by: " << setw(2) << forker
       << ", forking[" << forked_count << "]:";

   loop(c, forked_count)   out << " " << forked[c];
   out << endl;
}
//=============================================================================
Thread_context::Thread_context()
   : N(CNUM_INVALID),
     mileage(0),
     thread(0)
{
}
//-----------------------------------------------------------------------------
void
Thread_context::init_entry(CoreNumber n)
{
   N = n;

const TaskTree & tt = TaskTree::get_entry(N);

   forked_threads_count = tt.get_forked_count();
   forked_threads = tt.get_forked();
   join_thread = tt.get_forker();

   sem_init(&pool_sema, 0, 0);
}
//-----------------------------------------------------------------------------
void
Thread_context::init(CoreCount count, bool logit)
{
   delete thread_contexts;

   thread_contexts_count = count;
   thread_contexts = new Thread_context[thread_contexts_count];
   loop(c, thread_contexts_count)   thread_contexts[c].init_entry((CoreNumber)c);

   if (logit)   print_all(CERR);
}
//-----------------------------------------------------------------------------
void
Thread_context::bind_to_cpu(CPU_Number core, bool logit)
{
#ifndef HAVE_AFFINITY_NP
   CPU = CPU_0;
   return;

   // the code below is not reached; we #define pthread_setaffinity_np()
   // so that is compiles...
   //
#define pthread_setaffinity_np(_a, _b, _c) 0
#endif

   CPU = core;

   if (logit)
      {
        sem_wait(&Parallel::print_sema);
           CERR << "Binding thread #" << N << " to core " << core << endl;
        sem_post(&Parallel::print_sema);
      }

cpu_set_t cpus;
   CPU_ZERO(&cpus);

   if (Parallel::get_active_core_count() == CCNT_1)
      {
        // there is only one core in total (which is the master).
        // restore its affinity to all cores.
        //
        loop(a, Parallel::get_total_CPU_count())
            CPU_SET(Parallel::get_CPU(a), &cpus);
      }
   else
      {
        CPU_SET(CPU, &cpus);
      }

const int err = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpus);
   if (err)
      {
        cerr << "pthread_setaffinity_np() failed with error "
             << err << endl;
      }
}
//-----------------------------------------------------------------------------
void
Thread_context::print_all(ostream & out)
{
   sem_wait(&Parallel::print_sema);
      out << "Thread_context size: " << thread_contexts_count << endl;
      loop(e, thread_contexts_count)   thread_contexts[e].print(out);
   sem_post(&Parallel::print_sema);
}
//-----------------------------------------------------------------------------
void
Thread_context::print_mileages(ostream & out, const char * loc)
{
   sem_wait(&Parallel::print_sema);
      out << "    mileages:";
      loop(e, thread_contexts_count)   out << " " << thread_contexts[e].mileage;
      out << " at " << loc << endl;
   sem_post(&Parallel::print_sema);
}
//-----------------------------------------------------------------------------
void
Thread_context::print(ostream & out) const
{
   out << "Thread_context #" << N << ":" << endl
       << "    join thread: " << join_thread << endl
       << "    fork ";
   if (forked_threads_count == 0)        out << "no threads";
   else if (forked_threads_count == 1)   out << "thread:";
   else    out << forked_threads_count << " threads:";

   loop(c, forked_threads_count)   out << " #" << forked_threads[c];

int semval = 42;
   sem_getvalue((sem_t *)&pool_sema, &semval);
   out << endl
       << "    mileage:   " << mileage << endl
       << "    thread:    " << thread  << endl
       << "    pool sema: " << semval  << endl;
}
//=============================================================================
void
Parallel::init(bool logit)
{
   sem_init(&print_sema, 0, 1);
   sem_init(&pthread_create_sema, 0, 0);

   init_CPUs(logit);
   reinit(logit);
}
//-----------------------------------------------------------------------------
bool
Parallel::set_core_count(CoreCount count)
{
   // this function is called from ⎕SYL[26]←
   //
   if (active_core_count > 1)
      {
        CERR << "killing old pool..." << endl;
        Thread_context::print_mileages(CERR, LOC);
        kill_pool();
        Thread_context::print_mileages(CERR, LOC);
      }

   if (count < CCNT_1)                             return true;
   if ((CPU_count)count > get_total_CPU_count())   return true;

   active_core_count = count;
   reinit(LOG_Parallel);
   return false;   // no error
}
//-----------------------------------------------------------------------------
void
Parallel::reinit(bool logit)
{
   TaskTree::init(active_core_count, logit);

   Thread_context::init(active_core_count, logit);

   Thread_context::get_context(CNUM_MASTER)->thread = pthread_self();
   for (int w = CNUM_WORKER1; w < active_core_count; ++w)
       {
         Thread_context * tctx = Thread_context::get_context((CoreNumber)w);
         pthread_create(&(tctx->thread), /* attr */ 0, worker_main, tctx);

         // wait until new thread has reached its work loop
         sem_wait(&pthread_create_sema);
       }

   // bind threads to cores
   //
   loop(c, active_core_count)
       Thread_context::get_context((CoreNumber)c)
                       ->bind_to_cpu(all_CPUs[c], logit);

   return;

CERR << "locking pool... " << endl;
   lock_pool();
   usleep(100000);
   Thread_context::print_mileages(CERR, LOC);

CERR << "unlocking pool... " << endl;
   unlock_pool();
   usleep(100000);
   Thread_context::print_mileages(CERR, LOC);
}
//-----------------------------------------------------------------------------
void
Parallel::init_CPUs(bool logit)
{
#ifndef HAVE_AFFINITY_NP
# define pthread_getaffinity_np(_a, _b, c) 0; (CPU_SET(0, c));
#endif

   // first set up all_CPUs which contains the available cores
   //
cpu_set_t CPUs;
   CPU_ZERO(&CPUs);

const int err = pthread_getaffinity_np(pthread_self(), sizeof(CPUs), &CPUs);
   if (err)
      {
        CERR << "pthread_getaffinity_np() failed with error "
             << err << endl;
        all_CPUs.push_back(CPU_0);
        active_core_count = CCNT_1;
        return;
      }

   loop(c, CPU_COUNT(&CPUs))
       {
         if (CPU_ISSET(c, &CPUs))   all_CPUs.push_back((CPU_Number)c);
       }

   if (all_CPUs.size() == 0)
      {
        CERR << "*** no cores detected, assuming at least one! "
             << err << endl;
        all_CPUs.push_back(CPU_0);
        active_core_count = CCNT_1;
        return;
      }

   if (logit)
      {
        CERR << "detected " << all_CPUs.size() << " cores:";
        loop(cc, all_CPUs.size())   CERR << " #" << all_CPUs[cc];
        CERR << endl;
      }

   // then we determine the number of cores that shall be used
   //
   active_core_count =

#if CORE_COUNT_WANTED > 0
      (CoreCount)CORE_COUNT_WANTED;          // parallel, as per ./configure
#elif CORE_COUNT_WANTED == 0
      CCNT_1;                                // sequential
#elif CORE_COUNT_WANTED == -1
       (CoreCount)(get_total_CPU_count());   // parallel. all available cores
#elif CORE_COUNT_WANTED == -2
      uprefs.requested_cc;                   // parallel, --cc option
#elif CORE_COUNT_WANTED == -3
      CCNT_1;                                // parallel ⎕SYL (initially 1)
#else
#warning "invalid ./configure value for option CORE_COUNT_WANTED, using 0"
      CCNT_1;                                // sequential
#endif
}
//-----------------------------------------------------------------------------
void *
Parallel::worker_main(void * arg)
{
Thread_context & tctx = *(Thread_context *)arg;

   sem_wait(&print_sema);
      CERR << "worker #" << tctx.get_N() << " started" << endl;
   sem_post(&print_sema);

   sem_post(&pthread_create_sema);

   for (;;)
       {
         tctx.fork();
         Thread_context::do_work(tctx);
         tctx.join();
       }

   /* not reached */
   return 0;
}
//-----------------------------------------------------------------------------
void
Thread_context::PF_no_work(Thread_context & tctx)
{
   sem_wait(&Parallel::print_sema);
      CERR << "*** function no_work() called by thread #"
           << tctx.get_N() << endl;
   sem_post(&Parallel::print_sema);
}
//-----------------------------------------------------------------------------
void
Thread_context::PF_lock_unlock_pool(Thread_context & tctx)
{
   sem_wait(&Parallel::print_sema);
      CERR << "task #" << tctx.get_N()
           << " will now block on pool_sema" << endl;
   sem_post(&Parallel::print_sema);

   sem_wait(&tctx.pool_sema);

   sem_wait(&Parallel::print_sema);
      CERR << "task #" << tctx.get_N()
           << " was unblocked from pool_sema" << endl;
   sem_post(&Parallel::print_sema);

   tctx.unlock_pool();
}
//-----------------------------------------------------------------------------
void
Thread_context::PF_kill_pool(Thread_context & tctx)
{
   // we do not return, so we join here
   //
   tctx.join();

   sem_wait(&Parallel::print_sema);
      CERR << "worker #" << tctx.get_N() << " finished" << endl;
   sem_post(&Parallel::print_sema);

   pthread_exit(0);
}
//-----------------------------------------------------------------------------
void
Thread_context::lock_pool()
{
   do_work = PF_lock_unlock_pool;
   increment_mileage();
}
//-----------------------------------------------------------------------------
void
Thread_context::unlock_pool()
{
   loop(f, forked_threads_count)
       {
         Thread_context * forked = get_context(forked_threads[f]);
         CERR << "task #" << get_N()
           << " unblocks pool_sema of #" << forked->get_N() << endl;
         sem_post(&forked->pool_sema);
       }
}
//-----------------------------------------------------------------------------
void
Thread_context::kill_pool()
{
   do_work = PF_kill_pool;
   increment_mileage();
}
//-----------------------------------------------------------------------------

