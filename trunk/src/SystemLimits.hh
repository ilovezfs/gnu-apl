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

#ifndef __SYSTEM_LIMITS_HH_DEFINED__
#define __SYSTEM_LIMITS_HH_DEFINED__

///  System limits and default values defined for this interpreter.

enum
{
#define syl1(_n, e, v) e = v,
#define syl2(_n, e, v)
#define syl3(_n, e, v)
#include "SystemLimits.def"

   DEFAULT_Quad_PP = 10,
   DEFAULT_Quad_PW = 80,
};

// int64_enums not supported by Solaris
#define syl1(_n, e, v)
#define syl2(_n, e, v)
#define syl3(_n, e, v) const int64_t e = v;
#include "SystemLimits.def"

/// these numbers shall have an integer log₁₀ !
#define MAX_Quad_CT       (1.0e-9)
#define DEFAULT_Quad_CT   (1.0e-13)
#define REAL_TOLERANCE    (1.0e-10)   // ≤ MAX_Quad_CT
#define INTEGER_TOLERANCE (1.0e-10)   // ≤ MAX_Quad_CT

#define BIG_INT64_F 9223372036854775807.0
#define BIG_FLOAT   1.79769313486231470e308

#endif // __SYSTEM_LIMITS_HH_DEFINED__
