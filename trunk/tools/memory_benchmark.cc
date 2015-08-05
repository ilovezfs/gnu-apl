/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2015  Dr. Jürgen Sauermann

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

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <iomanip>

#include "../config.h"

using namespace std;

int64_t LEN = 420000000;

//-----------------------------------------------------------------------------
inline uint64_t cycle_counter()
{
unsigned int lo, hi;
   __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
   return ((uint64_t)hi << 32) | lo;
}
//-----------------------------------------------------------------------------
struct thread_context
{
  pthread_t  thread;
  uint64_t   core_num;
  uint64_t   slice_len;
  int64_t  * data;
  uint64_t   t1;
  uint64_t   t2;
};

bool goon = false;

//-----------------------------------------------------------------------------
void *
thread_main_write(void * vp)
{
thread_context & ctx = *(thread_context *)vp;

   if (ctx.core_num)   // subordinate
      {
        while (!goon)   ;   // busy wait for goon
        ctx.t1 = cycle_counter();
      }
   else                // master 
      {
        usleep(100000);   // allow subordinate to start up
        ctx.t1 = cycle_counter();
        goon = true;
      }

   // do some work...
   //
const int slice_len = ctx.slice_len;
int64_t * data = ctx.data;
const int64_t * end = data + slice_len;
   while (data < end)   *data++ = 42;

   ctx.t2 = cycle_counter();
   return 0;
}
//-----------------------------------------------------------------------------
void *
thread_main_read(void * vp)
{
thread_context & ctx = *(thread_context *)vp;

   if (ctx.core_num)   // subordinate
      {
        while (!goon)   ;   // busy wait for goon
        ctx.t1 = cycle_counter();
      }
   else                // master 
      {
        usleep(100000);   // allow subordinate to start up
        ctx.t1 = cycle_counter();
        goon = true;
      }

   // do some work...
   //
const int slice_len = ctx.slice_len;
int64_t * data = ctx.data;
const int64_t * end = data + slice_len;
int64_t sum = 0;
   while (data < end)   sum += *data++;

   ctx.t2 = cycle_counter();
   return 0;
}
//-----------------------------------------------------------------------------
void
multi(int cores, int op, int64_t * data)
{
thread_context ctx[cores];
int64_t slice_len = LEN / cores;
const char * opname = "write";
void * (*op_fun)(void *) = &thread_main_write;

   if (op == 1)
      {
        opname = "read ";
        op_fun = &thread_main_read;
      }

   for (int c = 0; c < cores; ++c)
      {
        ctx[c].core_num = c;
        ctx[c].data = data + c*slice_len;
        ctx[c].slice_len = slice_len;

        ctx[c].thread = pthread_self();
        if (c)
           {
             pthread_attr_t attr;
             pthread_attr_init(&attr);
             pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
             pthread_create(&ctx[c].thread, &attr, op_fun,  ctx + c);
             pthread_attr_destroy(&attr);
           }

        cpu_set_t cpus;
        CPU_ZERO(&cpus);
        CPU_SET(c, &cpus);
        pthread_setaffinity_np(ctx[c].thread, sizeof(cpu_set_t), &cpus);
      }

   op_fun(ctx);
   for (int c = 1; c < cores; ++c)
      {
         void * ret;
        pthread_join(ctx[c].thread, &ret);
      }

   goon = false;

const uint64_t start = ctx[0].t1;

   if (0)
   for (int c = 0; c < cores; ++c)
       {
         cerr << "thread " << c << ": t1="
              << setw(10) << (ctx[c].t1 - start) << " t2= "
              << setw(10) << (ctx[c].t2 - start) << " duration= "
              << setw(10) << (ctx[c].t2 - ctx[c].t1) << endl;
       }

   // summary
   //
int max = 0;
   for (int c = 0; c < cores; ++c)
       {
         const int diff = ctx[c].t2 - start;
         if (max < diff)   max = diff;
       }

   fprintf(stderr, "%s int64_t: %10d total, %5.2f cycles per %s"
                   " (on %d cores)\n",
           opname, max, (cores*(double)max)/LEN, opname, cores);
}
//-----------------------------------------------------------------------------
void
sequential(int op, int64_t * data, bool verbose)
{
uint64_t t1;
uint64_t t2;
const char * opname;

   if (op == 0)
      {
        opname = "write";
        int64_t * d = data;
        const int64_t * e = data + LEN;

        t1 = cycle_counter();
        while (d < e)   *d++ = 42;
        t2 = cycle_counter();
      }
   else if (op == 1)
      {
        opname = "read ";
        int64_t sum = 1;
        int64_t * d = data;

        t1 = cycle_counter();
        const int64_t * e = data + LEN;
        while (d < e)   sum += *d++;
        t2 = cycle_counter();
      }
   else assert(0 && "Bad opnum");

   if (!verbose)   return;


   fprintf(stderr,
           "%s int64_t: %10d; total, %5.2f cycles per %s (sequential)\n",
           opname, (int)(t2 - t1), ((double)(t2 - t1))/LEN, opname);
}
//-----------------------------------------------------------------------------
int
main(int argc, char *argv[])
{
cpu_set_t CPUs;   CPU_ZERO(&CPUs);

const int err = pthread_getaffinity_np(pthread_self(), sizeof(CPUs), &CPUs);
   assert(err == 0);
const int CPUs_present = CPU_COUNT(&CPUs);

   cerr << "running memory benchmark on 1.." << CPUs_present
        << " cores..." << endl;

int64_t * data = new int64_t[LEN];

   // cache warm-up
   //
   sequential(0, data, false);
   sequential(1, data, false);

   for (int c = 0; c <= CPUs_present; ++c)
      {
        if (c == 0)
           {
             sequential(0, data, true);
             sequential(1, data, true);
           }
        else
           {
             multi(c, 0, data);
             multi(c, 1, data);
           }
      }
}
//-----------------------------------------------------------------------------
