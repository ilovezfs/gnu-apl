#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <iomanip>

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
int64_t total = 0;
   for (int c = 0; c < cores; ++c)
       if (total < (ctx[c].t2 - start))   total = ctx[c].t2 - start;

   fprintf(stderr, "%s int64_t: %10u total, %5.2f cycles per %s"
                   " (on %d cores)\n",
           opname, total, (cores*(double)total)/LEN, opname, cores);
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


   fprintf(stderr, "%s int64_t: %10u total, %5.2f cycles per %s (sequential)\n",
           opname, t2 - t1, ((double)(t2 - t1))/LEN, opname);
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
