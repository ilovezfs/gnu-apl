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

#ifndef __EOC_HANDLER_ARGS_HH_DEFINED_
#define __EOC_HANDLER_ARGS_HH_DEFINED_

#include "Common.hh"
#include "Shape.hh"
#include "Value.hh"

/// arguments of the EOC handler for ⎕EA
struct quad_EA
{
   /// the left arg to ⎕EA
  Value_P A;

   /// the right arg to ⎕EA
  Value_P B;
};

/// arguments of the EOC handler for ⎕EC
struct quad_EC
{
  // the EOC handler for ⎕EC has no arguments
};

/// arguments of the EOC handler for ⎕INP
struct quad_INP
{
  UCS_string_list * lines;       /// the lines of the final result.

  UCS_string * end_marker;       ///< the end of the data read by ⎕INP
  UCS_string * esc1;             ///< start marker (escape from ⎕INP)
  UCS_string * esc2;             ///< end marker (return to ⎕INP)

  bool done;                     ///< true if end_marker read
};

/// arguments of the EOC handler for A ∘.f B
struct OUTER_PROD
{
  Value_P Z;         ///< final operator result
  Cell * cZ;         ///< current result
  Value_P value_A;   ///< current left arg
  Value_P RO_A;      ///< current left arg for RO
  ShapeItem a;       ///< current A index
  ShapeItem len_A;   ///< number of cells in left arg
  Function * RO;     ///< user defined function
  Value_P B;   ///< operator right arg
  Value_P value_B;   ///< current right arg
  Value_P RO_B;      ///< current right arg for RO
  ShapeItem b;       ///< current B index
  ShapeItem len_B;   ///< number of cells in right arg
  const Cell * cA;   ///< current left arg
  const Cell * cB;   ///< current right arg
  int how;           ///< how to continue in finish_outer_product()
};

/// arguments of the EOC handler for A g.f B
struct INNER_PROD
{
   Value_P Z;           ///< final operator result
   Cell * cZ;           ///< current result
   Value_P * args_A;    ///< left args of RO
   ShapeItem a;         ///< current A1 index
   ShapeItem items_A;   ///< number of cells in A1
   Function * LO;       ///< left user defined function
   Function * RO;       ///< right user defined function
   Value_P B;           ///< operator right arg
   Value_P * args_B;    ///< right args of RO
   ShapeItem b;         ///< current B1 index
   ShapeItem items_B;   ///< number of cells in B1
   ShapeItem v1;        ///< current LO index
   Value_P V1;          ///< vector being LO-reduced
   Value_P LO_B;        ///< current right arg of LO
   int how;             ///< how to continue in finish_inner_product()
   bool last_ufun;      ///< true for the last user defined function call
};

/// arguments of the EOC handler for one f/B result cell
struct reduce_beam
{
   /// initialize \b this reduce_beam
   void init(Value_P Z,  const Shape3 & Z3, Function * LO,
             Value_P B, ShapeItem bm, ShapeItem A0, int A0_inc)
      {
        need_pop = false;
        frame.init(Z, Z3, B, bm, A0_inc);
        beam.init(LO, &B->get_ravel(0), Z3.l(), A0);
      }

   /// true if SI was pushed
   bool need_pop;

   /// information about one beam
   struct _beam
      {
         /// initialize \b this _beam (first beam in frame)
         void init(Function * _LO, const Cell * B_h0l,
                   ShapeItem _dist, ShapeItem _A0)
            { LO = _LO;
              if (_A0 < 0)   { dist = _dist;     length = - _A0; }
              else           { dist = - _dist;   length = _A0;   }

              reset(B_h0l);
            }

         /// reset \b this _beam (next beam in frame)
         void reset(const Cell * B_h0l)
            { idx = 0;
              if (dist < 0)    src = B_h0l - (length * dist);
              else             src = B_h0l - dist;
            }

         /// advance B in \b this beam
         const Cell * next_B()
            { ++idx;  src += dist;  return src; }

         /// return true iff this beam is finished (one reduction result item
         /// has been computed
         bool done() const
            { return idx >= length; }

         ShapeItem dist;     ///< the distance between beam elements
         ShapeItem length;   ///< number of beam elements
         Function * LO;      ///< left operand

         const Cell * src;   ///< the next cell of the beam
         ShapeItem    idx;   ///< the current index of base
      } beam;   ///< the current beam

   /// information about all beams
   struct _frame
      {
        /// initialize \b this _frame
        void init(Value_P _Z, const Shape3 & Z3,
                  Value_P _B, ShapeItem _max_bm, int _A0_inc)
           {
             Z = _Z;
             B = _B;
             max_h = Z3.h();
             max_bm = _max_bm;
             max_zm = Z3.m();
             max_l = Z3.l();
             A0_inc = _A0_inc;

             cZ = &Z->get_ravel(0);
             h = 0;
             m = 0;
             l = 0;
           }

        /// advance Z in \b this frame
        Cell * next_Z()
           { ++l;
             if (l >= max_l)    { l = 0;   ++m; }
             if (m >= max_zm)   { m = 0;   ++h; }
             return cZ++;
           }

        /// return true iff this frame is finished (the last reduction result
        /// item has been computed
        bool done() const
           { return h >= max_h; }

        /// the beam start for Z[h;m]
        const Cell * beam_start()
           { return &B->get_ravel(l + max_l*(m * (1 - A0_inc) + max_bm * h)); } 

        // fixed variables
        Value_P Z;                 ///< final result
        Value_P B;                 ///< right operator argument
        ShapeItem    max_bm;       ///< max. m for B
        ShapeItem    max_h;        ///< max. h
        ShapeItem    max_zm;       ///< max. m for Z
        ShapeItem    max_l;        ///< max. l
        int          A0_inc;       ///< window increment (= 1 for scan)

        // running variables
        Cell * cZ;                 ///< beam result
        ShapeItem    h;            ///< current h
        ShapeItem    m;            ///< current m
        ShapeItem    l;            ///< current l
      } frame;   ///< the current frame
};

/// arguments of the EOC handler for A f¨ B
struct EACH_ALB
{
  Value_P A;         ///< operator left argument
  const Cell * cA;   ///< current left arg
  uint32_t dA;       ///< cA increment (0 or 1)
  Function * LO;     ///< user defined function
  Value_P B;         ///< operator right argument
  const Cell * cB;   ///< current left arg
  uint32_t dB;       ///< cB increment (0 or 1)
  Cell * cZ;         ///< current result cell
  ShapeItem z;       ///< current result index
  ShapeItem count;   ///< number of iterations
  bool sub;          ///< create a PointerCell
  Value_P Z;         ///< final operator result
  int how;           ///< how to continue in finish_eval_LB()
};

/// arguments of the EOC handler for f¨ B
struct EACH_LB
{
  Function * LO;     ///< user defined function
  Value_P B;   ///< operator right argument
  const Cell * cB;   ///< current left arg
  Value_P Z;         ///< final operator result
  Cell * cZ;         ///< current result cell
  ShapeItem z;       ///< current result index
  ShapeItem count;   ///< number of iterations
  bool sub;          ///< create a PointerCell
  int how;           ///< how to continue in finish_eval_LB()
};

/// arguments of the EOC handler for f⍤[X] B
struct RANK_LXB
{
  Function * LO;                         ///< user defined function
  ShapeItem h;                           ///< current high index
  char _sh_B_low [ sizeof(Shape) ];      ///< low dimensions of B
  char _sh_B_high[ sizeof(Shape) ];      ///< high dimensions of B
  ShapeItem ec_B_low;                    ///< items in _sh_B_low
  const Cell * cB;                       ///< current B cell
  Value_P* ZZ ;                          ///< current ZZ value
  char _sh_Z_max_low[ sizeof(Shape) ];   ///< max. low dimensions of Z
  ShapeItem ec_high;                     ///< max. high index
  int how;                               ///< how to finish_eval_LXB()

  /// return sh_B_low
  Shape & get_sh_B_low()       { return *(Shape *)&_sh_B_low;     }

  /// return sh_B_high
  Shape & get_sh_B_high()      { return *(Shape *)&_sh_B_high;    }

  /// return sh_Z_max_low
  Shape & get_sh_Z_max_low()   { return *(Shape *)&_sh_Z_max_low; }
};

/// arguments of the EOC handler for A f⍤[X] B
struct RANK_ALXB
{
  Function * LO;                         ///< user defined function
  ShapeItem h;                           ///< current high index
  Value_P A;                             ///< left operator argument
  bool repeat_A;                         ///< skalar-extend A
  char _sh_A_low [ sizeof(Shape) ];      ///< low dimensions of A
  ShapeItem ec_A_low;                    ///< items in _sh_B_low
  const Cell * cA;                       ///< current A cell
  Value_P B;                             ///< left operator argument
  bool repeat_B;                         ///< skalar-extend B
  char _sh_B_low [ sizeof(Shape) ];      ///< low dimensions of B
  char _sh_B_high[ sizeof(Shape) ];      ///< high dimensions of B
  ShapeItem ec_B_low;                    ///< items in _sh_B_low
  const Cell * cB;                       ///< current B cell
  Value_P* ZZ ;                          ///< current ZZ value
  char _sh_Z_max_low[ sizeof(Shape) ];   ///< max. low dimensions of Z
  ShapeItem ec_high;                     ///< max. high index
  int how;                               ///< how to finish_eval_LXB()

  /// return sh_A_low
  Shape & get_sh_A_low()       { return *(Shape *)&_sh_A_low;     }

  /// return sh_B_low
  Shape & get_sh_B_low()       { return *(Shape *)&_sh_B_low;     }

  /// return sh_B_high
  Shape & get_sh_B_high()      { return *(Shape *)&_sh_B_high;    }

  /// return sh_Z_max_low
  Shape & get_sh_Z_max_low()   { return *(Shape *)&_sh_Z_max_low; }
};

/// the second argument for an EOC_HANDLER. The actual type depends on the
/// handler. An _EOC_arg contains all information that is neccessary for
/// the EOC handler to compute the result token.
union _EOC_arg
{
#define U_TIEM(x)                   \
   /** space for one x **/  char ea_ ## x [ sizeof(x) ];    \
   /** return an x **/      x & _ ## x()  { return *(x*) & ea_ ## x; }

   U_TIEM(quad_EA)
   U_TIEM(quad_EC)
   U_TIEM(quad_INP)
   U_TIEM(OUTER_PROD)
   U_TIEM(INNER_PROD)
   U_TIEM(reduce_beam)
   U_TIEM(EACH_ALB)
   U_TIEM(EACH_LB)
   U_TIEM(RANK_LXB)
   U_TIEM(RANK_ALXB)
};

/// the type of a function to be called at the end of a context.
/// the function returns true to retry and false to continue with token.
typedef bool (*EOC_HANDLER)(Token & token, _EOC_arg & arg);

//-----------------------------------------------------------------------------

#endif // __EOC_HANDLER_ARGS_HH_DEFINED__
