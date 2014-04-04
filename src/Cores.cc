#include <pthread.h>

#include "Common.hh"

#ifdef MULTICORE

CoreCount __core_count = CCNT_MIN;
CoreCount __max_count  = CCNT_UNKNOWN;

//-----------------------------------------------------------------------------
CoreCount max_cores()
{
pthread_t thread = pthread_self();

   // determine max count only once
   //
   if (__max_count != CCNT_UNKNOWN)   return __max_count;

cpu_set_t CPUs;
const int err = pthread_getaffinity_np(thread, sizeof(CPUs), &CPUs);
   if (err)
      {
        get_CERR() << "pthread_getaffinity_np() failed with error "
                   << err << endl
                   << "setting __core_count to 1." << endl;
        __max_count = CCNT_MIN;
      }
   else
      {
        __max_count = (CoreCount)CPU_COUNT(&CPUs);
        if (__max_count < 1)   __max_count = CCNT_MIN;
      }

   return __max_count;
}
//-----------------------------------------------------------------------------
CoreCount setup_cores(CoreCount count)
{
   if (count > max_cores())
      {
         get_CERR() << "There were " << count
                    << " cores requested, but only " << max_cores()
                    << " seem to be available.\n"
                    << "Using " << max_cores() << " cores" << endl;
         count = max_cores();
      }

   __core_count = count;
   get_CERR() << "CPU core count is: " << __core_count << endl;
   return count;
}
//-----------------------------------------------------------------------------

#endif // MULTICORE
