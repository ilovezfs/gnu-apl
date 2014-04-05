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

#ifndef __CRES_HH_DEFINED__
#define __CRES_HH_DEFINED__

#ifndef __COMMON_HH_DEFINED__
#error "This file must not be #included directly - #include Common.hh instead"
#endif

enum CoreCount
{
  CCNT_UNKNOWN = -1,
  CCNT_MIN     = 1
};

#include "../config.h"   // for _WANTED macros from configure


#ifndef HAVE_OMP_H
   //
   // override CORE_COUNT_WANTED because omp.h is not present
   //
#undef  CORE_COUNT_WANTED
#define CORE_COUNT_WANTED 0
#endif


#ifndef HAVE_LIBGOMP
   //
   // override CORE_COUNT_WANTED because libgomp is not present
   //
#undef  CORE_COUNT_WANTED
#define CORE_COUNT_WANTED 0
#endif

   // #define MULTICORE unless CORE_COUNT_WANTED == 0
   //
#if CORE_COUNT_WANTED != 0
# define MULTICORE 1
#endif

   // #define STATIC_CORE_COUNT for CORE_COUNT_WANTED >= 0
   //
#if CORE_COUNT_WANTED > 0
# define STATIC_CORE_COUNT CORE_COUNT_WANTED   // as ./configured
#elif CORE_COUNT_WANTED == 0
# define STATIC_CORE_COUNT 1                   // static 1
#elif CORE_COUNT_WANTED == -1                  // dynamic
#elif CORE_COUNT_WANTED == -2                  // dynamic
#elif CORE_COUNT_WANTED == -3                  // dynamic
#else
#warning "invalid ./configure value for option CORE_COUNT_WANTED, using 0"
# define STATIC_CORE_COUNT 1                   // static 1, no MULTICORE
#endif

#ifdef STATIC_CORE_COUNT

# define core_count() STATIC_CORE_COUNT

#else

extern CoreCount __core_count;

inline CoreCount core_count() { return __core_count; }

#endif

#ifdef MULTICORE

#include <omp.h>

/// set up count cores, return count or less (if fewer cores available)
extern CoreCount setup_cores(CoreCount count);

/// max. number of cores on this system
extern CoreCount max_cores();

#else

# define setup_cores(x) (CCNT_MIN)
# define max_cores()    (CCNT_MIN)

#endif

#endif // __CRES_HH_DEFINED__

