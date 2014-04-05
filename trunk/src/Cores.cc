/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2014  Dr. Jürgen Sauermann

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
   omp_set_num_threads(__core_count);
   get_CERR() << "OMP thread count set to " << __core_count << "("
              << max_cores() << " cores)" << endl;
   
   return count;
}
//-----------------------------------------------------------------------------

#endif // MULTICORE
