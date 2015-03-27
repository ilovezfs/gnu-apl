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

/// arguments of the EOC handler for ⎕EA
struct quad_EA
{
   // ⎕EA has no additional arguments
};

/// arguments of the EOC handler for ⎕EC
struct quad_EC
{
   // ⎕EC has no additional arguments
};

/// arguments of the EOC handler for ⎕INP
struct quad_INP
{
  UCS_string_list * lines;       ///< the lines of the final result.

  UCS_string * end_marker;       ///< the end of the data read by ⎕INP
  UCS_string * esc1;             ///< start marker (escape from ⎕INP)
  UCS_string * esc2;             ///< end marker (return to ⎕INP)

  bool done;                     ///< true if end_marker read
};

/// arguments of the EOC handler for A ∘.f B
struct OUTER_PROD
{
  Function * RO;     ///< user defined function
  ShapeItem len_B;   ///< number of cells in right arg
  ShapeItem len_Z;   ///< number of cells in result
  ShapeItem z;       ///< current Z index
};

/// arguments of the EOC handler for A g.f B
struct INNER_PROD
{
  Value_P * args_A;    ///< left args of RO
  ShapeItem a;         ///< current A1 index
  ShapeItem items_A;   ///< number of cells in A1
  Function * LO;       ///< left user defined function
  Function * RO;       ///< right user defined function
  Value_P * args_B;    ///< right args of RO
  ShapeItem b;         ///< current B1 index
  ShapeItem items_B;   ///< number of cells in B1
  ShapeItem v1;        ///< current LO index
  int how;             ///< how to continue in finish_inner_product()
  bool last_ufun;      ///< true for the last user defined function call
};

/// arguments of the EOC handler for one f/B result cell
struct REDUCTION
{
  // parameters that are constant for the entire reduction
  Function * LO;            ///< left user defined function
  bool scan;                ///< true for LO \ B
  ShapeItem len_L;          ///< length of dimensions below (excluding) axis
  ShapeItem len_L_s;        ///< len_L or 0 for scan
  ShapeItem len_BML;        ///< length of dimensions below (including) axis
  ShapeItem len_ZM;         ///< length of reduce axis in Z
  ShapeItem len_Z;          ///< number of reductions (beams)
  ShapeItem beam_len;       ///< dito, -1: variable (scan)
  ShapeItem dB;             ///< - len_L (or len_L    if inverse)
  ShapeItem start_offset;   ///< 0       (or len_L*dB if inverse);

  // parameters that are constant for one beam (= one z)
  ShapeItem z;              ///< current beam
  ShapeItem z_L;            ///< current beam (low dimension of z)
  ShapeItem z_M;            ///< current beam (middle dimension of z)
  ShapeItem z_H;            ///< current beam (middle dimension of z)

  // parameters that are change during the reduction of a beam
  ShapeItem todo_B;         ///< # of reductions left in this beam, -1: new beam
  ShapeItem b;              ///< current b
};

/// arguments of the EOC handler for A f¨ B
struct EACH_ALB
{
  const Cell * cA;   ///< current left arg
  uint32_t dA;       ///< cA increment (0 or 1)
  Function * LO;     ///< user defined function
  const Cell * cB;   ///< current left arg
  uint32_t dB;       ///< cB increment (0 or 1)
  ShapeItem z;       ///< current result index
  ShapeItem count;   ///< number of iterations
  bool sub;          ///< create a PointerCell
};

/// arguments of the EOC handler for f¨ B
struct EACH_LB
{
  Function * LO;     ///< user defined function
  const Cell * cB;   ///< current left arg
  ShapeItem z;       ///< current result index
  ShapeItem count;   ///< number of iterations
  bool sub;          ///< create a PointerCell
};

/// arguments of the EOC handler for (A) f⍤[X] B
struct RANK
{
  Function * LO;                     ///< user defined function
  ShapeItem h;                       ///< current high index
  char _sh_chunk_B[sizeof(Shape)];   ///< low dimensions of B
  char _sh_frame  [sizeof(Shape)];   ///< high dimensions of B
  ShapeItem ec_chunk_B;              ///< items in _sh_chunk_B
  const Cell * cB;                   ///< current B cell
  ShapeItem ec_frame;                ///< max. high index
  Axis axes_valid;                   ///< LO[X] rather than LO
  char _axes  [sizeof(Shape)];       ///< axes for ⍤[X]

  /// return sh_chunk_B
  Shape & get_sh_chunk_B()       { return *(Shape *)&_sh_chunk_B;  }

  /// return sh_frame
  Shape & get_sh_frame()         { return *(Shape *)&_sh_frame;    }

  /// return sh_axes
  Shape & get_axes()             { return *(Shape *)&_axes;    }

   // additional args for dyadic RANK operator
  bool repeat_A;                     ///< scalar-extend A
  bool repeat_B;                     ///< scalar-extend B
  char _sh_chunk_A[sizeof(Shape)];   ///< low dimensions of A
  ShapeItem ec_chunk_A;              ///< items in _sh_chunk_B
  const Cell * cA;                   ///< current A cell

  /// return sh_chunk_A
  Shape & get_sh_chunk_A()       { return *(Shape *)&_sh_chunk_A;     }
};

/// arguments of the EOC handler for A f⋆g B
struct POWER_ALRB
{
  int how;                         ///< how to finish_eval_ALRB()
  double qct;                      ///< comparison tolerance
  ShapeItem repeat_count;          ///< repeat count N for  form  A f ⍣ N B
  Function * LO;                   ///< work function f for forms A f ⍣ N/g B
  Function * RO;                   ///< condition fun g for form  A f ⍣ g B
  bool user_RO;                    ///< true if RO is user-defined
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
   /// constructor for dyadic derived function
   EOC_arg(Value_P vpZ, Value_P vpB, Value_P vpA)
   : handler(0),
     loc(0),
     next(0),
     Z(vpZ),
     B(vpB),
     A(vpA)
   {}

   /// constructor for monadic derived function without result (or
   /// result created at return)
   EOC_arg(Value_P vpB)
   : handler(0),
     loc(0),
     next(0),
     B(vpB)
   {}

   /// constructor for monadic derived function with result
   EOC_arg(Value_P vpZ, Value_P vpB)
   : handler(0),
     loc(0),
     next(0),
     Z(vpZ),
     B(vpB)
   {}

   /// activation constructor
   EOC_arg(EOC_HANDLER h, const EOC_arg & other, const char * l)
      {
        new (this) EOC_arg(other);
        handler = h;
        loc = l;
      }

   /// copy constructor
   EOC_arg(const EOC_arg & other)
   : handler(other.handler),
     loc(other.loc),
     next(other.next),
     Z(other.Z),
     B(other.B),
     A(other.A),
     V1(other.V1),
     V2(other.V2),
     RO_A(other.RO_A),
     RO_B(other.RO_B)
   { u = other.u; }

   /// the handler
   EOC_HANDLER handler;

   /// from where the handler was installed (for debugging purposes)
   const char * loc;

   EOC_arg * next;

   /// result
   Value_P Z;

   /// left argument
   Value_P B;

   /// right argument
   Value_P A;

  /// INNER_PROD: argument for LO-reduction
  /// OUTER_PROD: helper value for non-pointer left RO argument
  Value_P V1;

  /// INNER_PROD: accumulator for LO-reduction
  /// OUTER_PROD: helper value for non-pointer right RO argument
  Value_P V2;

  /// OUTER_PROD: left RO argument
  Value_P RO_A;

   /// OUTER_PROD: right RO argument
  Value_P RO_B;

   /// additional EOC handler specific arguments
   union EOC_arg_u
      {
        quad_EA     u_quad_EA;         ///< space for ⎕EA context
        quad_EC     u_quad_EC;         ///< space for ⎕EC context
        quad_INP    u_quad_INP;        ///< space for ⎕INP context
        OUTER_PROD  u_OUTER_PROD;      ///< space for ∘.g context
        INNER_PROD  u_INNER_PROD;      ///< space for f.g context
        REDUCTION   u_REDUCTION;       ///< space for f/ context
        EACH_ALB    u_EACH_ALB;        ///< space for A¨B context
        EACH_LB     u_EACH_LB;         ///< space for ¨B context
        RANK        u_RANK;            ///< space for A f⍤[X] B context
        POWER_ALRB  u_POWER_ALRB;      ///< space for A f⍣g B context
      } u; ///< a union big enough for all EOC args
};

//-----------------------------------------------------------------------------

#endif // __EOC_HANDLER_ARGS_HH_DEFINED__
