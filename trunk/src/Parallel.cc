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

#include "Common.hh"
#include "Parallel.hh"
#include "SystemVariable.hh"
#include "UserPreferences.hh"

Thread_context * Thread_context::thread_contexts = 0;
CoreCount Thread_context::thread_contexts_count = CCNT_0;
Thread_context::PoolFunction * Thread_context::do_work = &Thread_context::PF_no_work;
volatile _Atomic_word Thread_context::busy_worker_count = 0;
CoreCount Thread_context::active_core_count = CCNT_0;

sem_t Parallel::print_sema;
sem_t Parallel::pthread_create_sema;
vector<CPU_Number>Parallel::all_CPUs;

volatile _Atomic_word Parallel_job_list_base::parallel_jobs_lock = 0;
const char * Parallel_job_list_base::started_loc = 0;

#if CORE_COUNT_WANTED == 0
const bool Parallel::run_parallel = false;
#else
const bool Parallel::run_parallel = true;
#endif

//=============================================================================
Thread_context::Thread_context()
   : N(CNUM_INVALID),
     thread(0),
     job_count(0)
{
}
//-----------------------------------------------------------------------------
void
Thread_context::init_entry(CoreNumber n)
{
   N = n;
   sem_init(&pool_sema, 0, 0);
}
//-----------------------------------------------------------------------------
void
Thread_context::init(CoreCount count, bool logit)
{
   delete thread_contexts;

   thread_contexts_count = count;
   thread_contexts = new Thread_context[thread_contexts_count];
   loop(c, thread_contexts_count)
       thread_contexts[c].init_entry((CoreNumber)c);
}
//-----------------------------------------------------------------------------
void
Thread_context::bind_to_cpu(CPU_Number core, bool logit)
{
#ifndef PARALLEL_ENABLED

   CPU = CPU_0;

#else

   CPU = core;

   if (logit)
      {
        PRINT_LOCKED(CERR << "Binding thread #" << N
                          << " to core " << core << endl;);
      }

cpu_set_t cpus;
   CPU_ZERO(&cpus);

   if (active_core_count == CCNT_1)
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
#endif // PARALLEL_ENABLED
}
//-----------------------------------------------------------------------------
void
Thread_context::print_all(ostream & out)
{
   PRINT_LOCKED(
      out << "Thread_context size: " << thread_contexts_count << endl;
      loop(e, thread_contexts_count)   thread_contexts[e].print(out);
      out << endl)
}
//-----------------------------------------------------------------------------
void
Thread_context::print_mileages(ostream & out, const char * loc)
{
   out << "    " << busy_worker_count << " of " << active_core_count
       << " workers busy:";

   loop(e, thread_contexts_count)   out << " " << thread_contexts[e].job_count;

   out <<" at " << loc << endl;
}
//-----------------------------------------------------------------------------
void
Thread_context::print(ostream & out) const
{
   out << "Thread_context #" << N << ":" << endl;

int semval = 42;
   sem_getvalue((sem_t *)&pool_sema, &semval);
   out << endl
       << "    thread:    " << (void *)thread  << endl
       << "    pool sema: " << semval  << endl;
}
//=============================================================================
void
Parallel::init(bool logit)
{
   sem_init(&print_sema,          /* shared */ 0, /* value */ 1);
   sem_init(&pthread_create_sema, /* shared */ 0, /* value */ 0);

   init_CPUs(logit);
   reinit(logit);
}
//-----------------------------------------------------------------------------
bool
Parallel::set_core_count(CoreCount count)
{
   // this function is called from ⎕SYL[26]←
   //
   if (count < CCNT_1)                             return true;   // error
   if ((CPU_count)count > get_total_CPU_count())   return true;   // error

   if (Thread_context::get_active_core_count() > 1)
      {
        Log(LOG_Parallel)
           {
             CERR << "killing old pool..." << endl;
             Thread_context::print_mileages(CERR, LOC);
           }

        kill_pool();

        Log(LOG_Parallel)
           {
             Thread_context::print_mileages(CERR, LOC);
             CERR << "killing old pool done." << endl;
           }
      }

   Thread_context::set_active_core_count(count);
   reinit(LOG_Parallel);
   return false;   // no error
}
//-----------------------------------------------------------------------------
void
Parallel::reinit(bool logit)
{
   Thread_context::init(Thread_context::get_active_core_count(), logit);

   Thread_context::get_context(CNUM_MASTER)->thread = pthread_self();
   for (int w = CNUM_WORKER1; w < Thread_context::get_active_core_count(); ++w)
       {
         Thread_context * tctx = Thread_context::get_context((CoreNumber)w);
         pthread_create(&(tctx->thread), /* attr */ 0, worker_main, tctx);

         // wait until new thread has reached its work loop
         sem_wait(&pthread_create_sema);
       }

   // bind threads to cores
   //
   loop(c, Thread_context::get_active_core_count())
       Thread_context::get_context((CoreNumber)c)
                       ->bind_to_cpu(all_CPUs[c], logit);

   if (logit)   Thread_context::print_all(CERR);
}
//-----------------------------------------------------------------------------
void
Parallel::init_CPUs(bool logit)
{
#ifndef PARALLEL_ENABLED

   all_CPUs.push_back(CPU_0);
   Thread_context::set_active_core_count(CCNT_1);
   return;

# if CORE_COUNT_WANTED != 0
# error "CORE_COUNT_WANTED configured, but no pthread_getaffinity_np()"
# endif

#else

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
        Thread_context::set_active_core_count(CCNT_1);
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
        Thread_context::set_active_core_count(CCNT_1);
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
   Thread_context::set_active_core_count(

#if CORE_COUNT_WANTED > 0
      (CoreCount)CORE_COUNT_WANTED);          // parallel, as per ./configure
#elif CORE_COUNT_WANTED == 0
      CCNT_1);                                // sequential
#elif CORE_COUNT_WANTED == -1
       (CoreCount)(get_total_CPU_count()));   // parallel. all available cores
#elif CORE_COUNT_WANTED == -2
      uprefs.requested_cc);                   // parallel, --cc option
      if (active_core_count < CCNT_1)   active_core_count = CCNT_1;
      if (active_core_count > all_CPUs.size())
         active_core_count = (CoreCount)(all_CPUs.size());
#elif CORE_COUNT_WANTED == -3
      CCNT_1);                                // parallel ⎕SYL (initially 1)
#else
#warning "invalid ./configure value for option CORE_COUNT_WANTED, using 0"
      CCNT_1);                                // sequential
#endif

#endif // PARALLEL_ENABLED
}
//-----------------------------------------------------------------------------
void *
Parallel::worker_main(void * arg)
{
Thread_context & tctx = *(Thread_context *)arg;

   Log(LOG_Parallel)
      {
        PRINT_LOCKED(CERR << "worker #" << tctx.get_N() << " started" << endl)
      }

   sem_post(&pthread_create_sema);

   for (;;)
       {
         tctx.PF_fork();
         Thread_context::do_work(tctx);
         tctx.PF_join();
       }

   /* not reached */
   return 0;
}
//-----------------------------------------------------------------------------
void
Thread_context::PF_no_work(Thread_context & tctx)
{
   PRINT_LOCKED(CERR << "*** function no_work() called by thread #"
                     << tctx.get_N() << endl)
}
//-----------------------------------------------------------------------------
void
Thread_context::PF_lock_unlock_pool(Thread_context & tctx)
{
   Log(LOG_Parallel)
      {
        PRINT_LOCKED(CERR << "task #" << tctx.get_N()
                          << " will now block on its pool_sema" << endl)
      }

   sem_wait(&tctx.pool_sema);

   Log(LOG_Parallel)
      {
        PRINT_LOCKED(CERR << "task #" << tctx.get_N()
                          << " was unblocked from pool_sema" << endl)
      }
}
//-----------------------------------------------------------------------------
void
Thread_context::PF_kill_pool(Thread_context & tctx)
{
   Log(LOG_Parallel)
      {
        PRINT_LOCKED(CERR << "    worker #" << tctx.get_N()
                          << " will be killed" << endl)
      }

   // we do not return, so we join() here
   //
   tctx.PF_join();
   pthread_exit(0);
}
//-----------------------------------------------------------------------------
void
Thread_context::M_lock_pool()
{
   do_work = PF_lock_unlock_pool;
   M_fork();
}
//-----------------------------------------------------------------------------
void
Thread_context::M_kill_pool()
{
   do_work = PF_kill_pool;
   M_fork();
   M_join();
}
//=============================================================================
