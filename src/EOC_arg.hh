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

#ifndef __EOC_HANDLER_ARGS_HH_DEFINED_
#define __EOC_HANDLER_ARGS_HH_DEFINED_

#include "Common.hh"
#include "Shape.hh"
#include "Value.icc"

/// arguments of the EOC handler for (A) f⍤[X] B
struct RANK
{
  ShapeItem rk_chunk_A;      ///< rank of lower dimensions of A
  ShapeItem rk_chunk_B;      ///< rank of lower dimensions of B
  ShapeItem a;               ///< current A cell
  ShapeItem b;               ///< current B cell
  int       Z_nested;        ///< Z is nested

  char axes[MAX_RANK + 1];   ///< axes for ⍤[X]
};

/// the type of a function to be called at the end of a context.
/// the function returns true to retry and false to continue with token.
typedef bool (*EOC_HANDLER)(Token & token);

/// the second argument for an EOC_HANDLER. The actual type depends on the
/// handler. An EOC_arg contains all information that is necessary for
/// the EOC handler to compute the result token.
class EOC_arg
{
public:
   /// enumeration of all EOC handlers for serializing them in Archive.cc
   enum EOC_type
      {
         EOC_None,         ///< no type
         EOC_Quad_EA_AB,   ///<  ⎕EA
         EOC_Quad_EA_B,    ///<  ⎕EA
         EOC_Quad_EC,      ///<  ⎕EC
         EOC_Quad_INP,     ///<  ⎕INP
      };

   /// constructor
   EOC_arg(Value_P vpA, Value_P vpB, const char * _loc)
   : handler(0),
     loc(0),
     next(0),
     A(vpA, _loc),
     B(vpB, _loc)
   { ++EOC_arg_count; }

   /// activation constructor
   EOC_arg(EOC_HANDLER h, const EOC_arg & other, const char * l)
      {
        new (this) EOC_arg(other, l);
        handler = h;
        loc = l;
      }

   /// copy constructor
   EOC_arg(const EOC_arg & other, const char * _loc)
   : handler(other.handler),
     loc(_loc),
     next(other.next),
     A(other.A, _loc),
     B(other.B, _loc)
   { ++EOC_arg_count; }

   /// copy constructor
   EOC_arg(const EOC_arg & other)
   : handler(other.handler),
     loc(other.loc),
     next(other.next),
     A(other.A, LOC),
     B(other.B, LOC)
   { Backtrace::show(__FILE__, __LINE__);   ++EOC_arg_count; }

   ~EOC_arg()   { --EOC_arg_count; }

   /// clear the Value_P in this EOC_arg
   void clear(const char * _loc)
      {
        A.clear(_loc);
        B.clear(_loc);
      }

   /// the handler
   EOC_HANDLER handler;

   /// from where the handler was installed (for debugging purposes)
   const char * loc;

   /// the next handler (after this one has finished)
   EOC_arg * next;

   /// left value argument
   Value_P A;

   /// right value argument
   Value_P B;

   /// return the EOC_type for handler
   static EOC_type get_EOC_type(EOC_HANDLER handler);

   /// return the EOC_handler for type
   static EOC_HANDLER get_EOC_handler(EOC_type type);

   /// the total number of constructed EOC_arg (to detect stale ones)
   static ShapeItem EOC_arg_count;
};
//-----------------------------------------------------------------------------

#endif // __EOC_HANDLER_ARGS_HH_DEFINED__
