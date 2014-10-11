
#include <iostream>
#include <stdint.h>

using namespace std;

//-----------------------------------------------------------------------------
inline uint64_t cycle_counter()
{
unsigned int lo, hi;
   __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
   return ((uint64_t)hi << 32) | lo;
}
//-----------------------------------------------------------------------------
int
main(int argc, char *argv[])
{
   enum { LEN = 100000000 };

int64_t * data = new int64_t[LEN];

const uint64_t t1 = cycle_counter();

   for (int l = 0; l < LEN; ++l)   data[l] = 42;

const uint64_t t2 = cycle_counter();

int64_t sum = 1;
   for (int l = 0; l < LEN; ++l)   sum += data[l];

const uint64_t t3 = cycle_counter();

   cerr << "write int64_t: " << (t2 - t1) << " total, "
        << ((t2 - t1)/LEN) << " cycles per write" << endl;
   cerr << "read  int64_t: " << (t3 - t2) << " total, "
        << ((t3 - t2)/LEN) << " cycles per read" << endl;
}
//-----------------------------------------------------------------------------
