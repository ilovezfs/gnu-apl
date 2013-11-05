/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2013  Dr. Jürgen Sauermann

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
   LOG_MAX_SYMBOL_COUNT   =   16,   ///< 64k symbols
   MAX_FRACT_DIGITS       =   20,   ///< in format etc.
   MIN_QUAD_PW            =   30,   ///< as per IBM manual
   DEFAULT_QUAD_PW        =   80,   ///< dito.
   MAX_QUAD_PW            = 1000,   ///< system specific

   /// the default ⎕PP value
   DEFAULT_QUAD_PP        =  10,

   /// the max. ⎕PP value. Values > 16 cause rounding errors in the display
   /// of floating point numbers.
   MAX_QUAD_PP            =  16,

   /// the min. ⎕PP value
   MIN_QUAD_PP            =   1,

   MAX_SVAR_NAMELEN       =  64,   ///< dito 
   MAX_SVARS_OFFERED      =  64,   ///< dito 
   MAX_ACTIVE_PROCESSORS  =  16,   ///< dito 

   MAX_SYMBOL_COUNT       = 1 << LOG_MAX_SYMBOL_COUNT,   ///< dito

   MAX_FUN_OPER           = 16,   ///< max. adjacent operators (as in +////X)
};

#define MAX_QUAD_CT     (1.0e-9)
#define DEFAULT_QUAD_CT (1.0e-13)
#define DEFAULT_QUAD_PT (1.0e-10)

#define BIG_INT64_F 9223372036854775807.0
#endif // __SYSTEM_LIMITS_HH_DEFINED__
