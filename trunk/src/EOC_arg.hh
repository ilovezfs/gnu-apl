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

/// ⎕INP has no arguments anymore
struct QUAD_INP
{
};

/// arguments of the EOC handler for A ∘.f B
struct OUTER_PROD
{
  ShapeItem len_B;   ///< number of cells in right arg
  ShapeItem len_Z;   ///< number of cells in result
};

/// arguments of the EOC handler for A g.f B
struct INNER_PROD
{
  ShapeItem items_A;     ///< number of cells in A1
  ShapeItem items_B;     ///< number of cells in B1
  ShapeItem how;         ///< how to continue in finish_inner_product()
  bool      last_ufun;   ///< true for the last user defined function call
  bool      A_enclosed;   ///< true if A was enclosed
  bool      B_enclosed;   ///< true if B was enclosed
};

/// arguments of the EOC handler for one f/B result cell
struct REDUCTION
{
  // parameters that are constant for the entire reduction
  //
  ShapeItem nwise;          ///< left argument (possibly negative)
  ShapeItem len_L;          ///< length of dimensions below (excluding) axis
  ShapeItem len_BML;        ///< length of dimensions below (including) axis
  ShapeItem len_ZM;         ///< length of reduce axis in Z

  // parameters that are changing during the reduction of a beam
  ShapeItem todo_B;         ///< # of reductions left in this beam, -1: new beam
  ShapeItem b;              ///< current b
};

/// arguments of the EOC handler for (A) f¨ B
struct EACH_ALB
{
  ShapeItem dA;          ///< cA increment (0 or 1)
  ShapeItem dB;          ///< cB increment (0 or 1)
  ShapeItem len_Z;       ///< number of iterations
  ShapeItem sub;         ///< create a PointerCell
  ShapeItem SI_pushed;   ///< user-defined LO
};

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

/// arguments of the EOC handler for A f⋆g B
struct POWER_ALRB
{
  ShapeItem how;                   ///< how to finish_eval_ALRB()
  ShapeItem repeat_count;          ///< repeat count N for  form  A f ⍣ N B
  ShapeItem user_RO;               ///< true if RO is user-defined
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
         EOC_None,         ///<
         EOC_Quad_EA_AB,   ///<  ⎕EA
         EOC_Quad_EA_B,    ///<  ⎕EA
         EOC_Quad_EC,      ///<  ⎕EC
         EOC_Quad_INP,     ///<  ⎕INP
         EOC_Outer_Prod,   ///<  ∘.f
         EOC_Inner_LO,     ///<  f of f.g
         EOC_Inner_RO,     ///<  g of f.g
         EOC_Reduce,       ///<  f/
         EOC_Each_AB,      ///<  A f¨ B
         EOC_Each_B,       ///<  f¨ B
         EOC_Rank,         ///<  f⍤
         EOC_Power_0,      ///<  f⍣N  user-defined f
         EOC_Power_LO,     ///<  f⍣g  user-defined f
         EOC_Power_RO,     ///<  f⍣g  user-defined g
      };

   /// constructor
   EOC_arg(Value_P vpZ, Value_P vpA, Function * lo, Function * ro, Value_P vpB)
   : handler(0),
     loc(0),
     next(0),
     Z(vpZ),
     A(vpA),
     LO(lo),
     RO(ro),
     B(vpB),
     z(-1)
   { memset(&u, 0, sizeof(u));   ++EOC_arg_count; }

   /// constructor
   EOC_arg(Value_P vpZ, Value_P vpA, Function * lo, Function * ro, Value_P vpB,
           const char * loc)
   : handler(0),
     loc(0),
     next(0),
     Z(vpZ, loc),
     A(vpA, loc),
     LO(lo),
     RO(ro),
     B(vpB, loc),
     z(-1)
   { memset(&u, 0, sizeof(u));   ++EOC_arg_count; }

   /// activation constructor
   EOC_arg(EOC_HANDLER h, const EOC_arg & other, const char * l)
      {
        new (this) EOC_arg(other, l);
        handler = h;
        loc = l;
      }

   /// copy constructor
   EOC_arg(const EOC_arg & other, const char * loc)
   : handler(other.handler),
     loc(loc),
     next(other.next),
     Z(other.Z, loc),
     A(other.A, loc),
     LO(other.LO),
     RO(other.RO),
     B(other.B, loc),
     z(other.z)
   { u = other.u;   ++EOC_arg_count; }

   /// copy constructor
   EOC_arg(const EOC_arg & other)
   : handler(other.handler),
     loc(other.loc),
     next(other.next),
     Z(other.Z, LOC),
     A(other.A, LOC),
     LO(other.LO),
     RO(other.RO),
     B(other.B, LOC),
     z(other.z)
   { u = other.u; Backtrace::show(__FILE__, __LINE__);   ++EOC_arg_count; }

   ~EOC_arg()   { --EOC_arg_count; }

   /// clear the Value_P in this EOC_arg
   void clear(const char * loc)
      {
        Z.clear(loc);
        A.clear(loc);
        B.clear(loc);
      }

   /// the handler
   EOC_HANDLER handler;

   /// from where the handler was installed (for debugging purposes)
   const char * loc;

   /// the next handler (after this one has finished)
   EOC_arg * next;

   /// result
   Value_P Z;

   /// left value argument
   Value_P A;

   /// left function argument
   Function * LO;

   /// right function argument
   Function * RO;

   /// right value argument
   Value_P B;

   /// current Z index
   ShapeItem z;

   /// return the EOC_type for handler
   static EOC_type get_EOC_type(EOC_HANDLER handler);

   /// return the EOC_handler for type
   static EOC_HANDLER get_EOC_handler(EOC_type type);

   /// the structs defined above...
#define EOC_structs                                                   \
   QUAD_INP    u_Quad_INP;        /**< space for ⎕INP context      */ \
   OUTER_PROD  u_OUTER_PROD;      /**< space for ∘.g context       */ \
   INNER_PROD  u_INNER_PROD;      /**< space for f.g context       */ \
   REDUCTION   u_REDUCTION;       /**< space for f/ context        */ \
   EACH_ALB    u_EACH_ALB;        /**< space for A¨B context       */ \
   RANK        u_RANK;            /**< space for A f⍤[X] B context */ \
   POWER_ALRB  u_POWER_ALRB;      /**< space for A f⍣g B context   */

   /// a helper union for computing the size of the largest struct
   union EOC_arg_u1 { EOC_structs };

   enum { u_DATA_LEN = (sizeof(EOC_arg_u1)
                     +  sizeof(ShapeItem) - 1) / sizeof(ShapeItem) };

   /// additional EOC handler specific arguments
   union EOC_arg_u
      { EOC_structs 
        uint8_t u_data[u_DATA_LEN];   ///< for serialization in Archive.cc
      } u; ///< a union big enough for all EOC args

   /// the total number of constructed EOC_arg (to detect stale ones)
   static ShapeItem EOC_arg_count;
};
//-----------------------------------------------------------------------------

#endif // __EOC_HANDLER_ARGS_HH_DEFINED__
