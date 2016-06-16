/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2016  Dr. Jürgen Sauermann

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

#include "Bif_OPER2_RANK.hh"
#include "IntCell.hh"
#include "Macro.hh"
#include "PointerCell.hh"
#include "Workspace.hh"

Bif_OPER2_RANK   Bif_OPER2_RANK::_fun;
Bif_OPER2_RANK * Bif_OPER2_RANK::fun = &Bif_OPER2_RANK::_fun;

/* general comment: we use the term 'chunk' instead of 'p-rank' to avoid
 * confusion with the rank of a value
 */

//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::eval_LRB(Token & LO, Token & y, Value_P B)
{
   if (B->element_count() == 1 && B->get_ravel(0).is_pointer_cell())
      B = B->get_ravel(0).get_pointer_value();

Rank rank_chunk_B = B->get_rank();
   y123_to_B(y.get_apl_val(), rank_chunk_B);

   return do_LyXB(LO, Value_P(), B, rank_chunk_B);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::eval_LRXB(Token & LO, Token & y, Value_P X, Value_P B)
{
   if (B->element_count() == 1 && B->get_ravel(0).is_pointer_cell())
      B = B->get_ravel(0).get_pointer_value();

Rank rank_chunk_B = B->get_rank();

   y123_to_B(y.get_apl_val(), rank_chunk_B);

   return do_LyXB(LO, X, B, rank_chunk_B);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::do_LyXB(Token & _LO, Value_P X, Value_P B, Rank rank_chunk_B)
{
Function * LO = _LO.get_function();
   Assert(LO);
   if (!LO->has_result())   DOMAIN_ERROR;

   // split shape of B into high (=frame) and low (= chunk) shapes.
   //
const Shape shape_Z = B->get_shape().high_shape(B->get_rank() - rank_chunk_B);
   if (shape_Z.is_empty())
      {
        Value_P Fill_B = Bif_F12_TAKE::first(B);
        Token tZ = LO->eval_fill_B(Fill_B);
        Value_P Z = tZ.get_apl_val();
        Z->set_shape(B->get_shape());
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

const Shape shape_B = B->get_shape().low_shape(rank_chunk_B);

Value_P vsh_B(shape_B.get_rank(), LOC);
   new (&vsh_B->get_ravel(0)) IntCell(0);   // prototype
   loop(sh, shape_B.get_rank())
            new (vsh_B->next_ravel()) IntCell(shape_B.get_shape_item(sh));
   vsh_B->check_value(LOC);

Value_P vsh_Z(shape_Z.get_rank(), LOC);
   new (&vsh_Z->get_ravel(0)) IntCell(0);   // prototype
   loop(sh, shape_Z.get_rank())
            new (vsh_Z->next_ravel()) IntCell(shape_Z.get_shape_item(sh));
   vsh_Z->check_value(LOC);

Value_P X5(5, LOC);
   if (!X)   new (X5->next_ravel())   IntCell(-1);                // no X
   else      new (X5->next_ravel())   PointerCell(X, X5.getref());   // X

   new (X5->next_ravel())   IntCell(shape_B.get_volume());        // LB
   new (X5->next_ravel())   PointerCell(vsh_B, X5.getref());      // rho_B
   new (X5->next_ravel())   IntCell(shape_Z.get_volume());        // N_max
   new (X5->next_ravel())   PointerCell(vsh_Z, X5.getref());      // rho_Z
   X5->check_value(LOC);
   return Macro::Z__LO_RANK_X5_B->eval_LXB(_LO, X5, B);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::eval_ALRB(Value_P A, Token & LO, Token & y, Value_P B)
{
   if (B->element_count() == 1 && B->get_ravel(0).is_pointer_cell())
      B = B->get_ravel(0).get_pointer_value();

Rank rank_chunk_A = A->get_rank();
Rank rank_chunk_B = B->get_rank();
   y123_to_AB(y.get_apl_val(), rank_chunk_A, rank_chunk_B);

   return do_ALyXB(A, rank_chunk_A, LO, Value_P(), B, rank_chunk_B);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::eval_ALRXB(Value_P A, Token & LO, Token & y,
                           Value_P X, Value_P B)
{
   if (B->element_count() == 1 && B->get_ravel(0).is_pointer_cell())
      B = B->get_ravel(0).get_pointer_value();

Rank rank_chunk_A = A->get_rank();
Rank rank_chunk_B = B->get_rank();

   y123_to_AB(y.get_apl_val(), rank_chunk_A, rank_chunk_B);

   return do_ALyXB(A, rank_chunk_A, LO, X, B, rank_chunk_B);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::do_ALyXB(Value_P A, Rank rank_chunk_A, Token & _LO,
                         Value_P X, Value_P B, Rank rank_chunk_B)
{
Function * LO = _LO.get_function();
   Assert(LO);
   if (!LO->has_result())   DOMAIN_ERROR;

Rank rk_A_frame = A->get_rank() - rank_chunk_A;   // rk_A_frame is y8
Rank rk_B_frame = B->get_rank() - rank_chunk_B;   // rk_B_frame is y9

   // if both high-ranks are 0, then return A LO B.
   //
   if (rk_A_frame == 0 && rk_B_frame == 0)   return LO->eval_AB(A, B);

   // split shapes of A1 and B1 into high (frame) and low (chunk) shapes.
   // Even though A and B have the same shape, rk_A_frame and rk_B_frame
   // could be different, leading to different split shapes for A and B
   //
const Shape shape_Z = rk_B_frame ? B->get_shape().high_shape(rk_B_frame)
                                 : A->get_shape().high_shape(rk_A_frame);

   if (rk_A_frame && rk_B_frame)   // A and B frames non-scalar
      {
        if (rk_A_frame != rk_B_frame)                           RANK_ERROR;
        if (shape_Z != A->get_shape().high_shape(rk_A_frame))   LENGTH_ERROR;
      }

   if (shape_Z.is_empty())
      {
        Value_P Fill_A = Bif_F12_TAKE::first(A);
        Value_P Fill_B = Bif_F12_TAKE::first(B);
        Shape shape_Z;

        if (A->is_empty())          shape_Z = A->get_shape();
        else if (!A->is_scalar())   DOMAIN_ERROR;

        if (B->is_empty())          shape_Z = B->get_shape();
        else if (!B->is_scalar())   DOMAIN_ERROR;

        Value_P Z1 = LO->eval_fill_AB(Fill_A, Fill_B).get_apl_val();

        Value_P Z(shape_Z, LOC);
        new (&Z->get_ravel(0)) PointerCell(Z1, Z.getref());
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

const Shape shape_A = A->get_shape().low_shape(rank_chunk_A);
Value_P vsh_A(shape_A.get_rank(), LOC);
   new (&vsh_A->get_ravel(0)) IntCell(0);   // prototype
   loop(sh, shape_A.get_rank())
            new (vsh_A->next_ravel()) IntCell(shape_A.get_shape_item(sh));
   vsh_A->check_value(LOC);

const Shape shape_B = B->get_shape().low_shape(rank_chunk_B);
Value_P vsh_B(shape_B.get_rank(), LOC);
   new (&vsh_B->get_ravel(0)) IntCell(0);   // prototype
   loop(sh, shape_B.get_rank())
            new (vsh_B->next_ravel()) IntCell(shape_B.get_shape_item(sh));
   vsh_B->check_value(LOC);

Value_P vsh_Z(shape_Z.get_rank(), LOC);
   new (&vsh_Z->get_ravel(0)) IntCell(0);   // prototype
   loop(sh, shape_Z.get_rank())
            new (vsh_Z->next_ravel()) IntCell(shape_Z.get_shape_item(sh));
   vsh_Z->check_value(LOC);

Value_P X7(7, LOC);
   if (!X)   new (X7->next_ravel())   IntCell(-1);                // no X
   else      new (X7->next_ravel())   PointerCell(X, X7.getref());   // X

   new (X7->next_ravel())   IntCell(shape_A.get_volume());        // LA
   new (X7->next_ravel())   PointerCell(vsh_A, X7.getref());      // rho_A
   new (X7->next_ravel())   IntCell(shape_B.get_volume());        // LB
   new (X7->next_ravel())   PointerCell(vsh_B, X7.getref());      // rho_B
   new (X7->next_ravel())   IntCell(shape_Z.get_volume());        // N_max
   new (X7->next_ravel())   PointerCell(vsh_Z, X7.getref());      // rho_Z
   X7->check_value(LOC);
   return Macro::Z__A_LO_RANK_X7_B->eval_ALXB(A, _LO, X7, B);
}
//-----------------------------------------------------------------------------
void
Bif_OPER2_RANK::y123_to_B(Value_P y123, Rank & rank_B)
{
   // y123_to_AB() splits the ranks of A and B into a (higher-dimensions)
   // "frame" and a (lower-dimensions) "chunk" as specified by y123.

   // 1. on entry rank_B is the rank of B.
   //
   //    Remember the rank of B to limit rank_B
   //    if values in y123 should exceed them.
   //
const Rank rk_B = rank_B;

   if (!y123)                   VALUE_ERROR;
   if ( y123->get_rank() > 1)   DOMAIN_ERROR;

   // 2. the number of elements in y determine how rank_B shall be computed:
   //
   //                    -- monadic f⍤ --       -- dyadic f⍤ --
   //          	        rank_A     rank_B       rank_A   rank_B
   // ---------------------------------------------------------
   // y        :        N/A        y            y        y
   // yA yB    :        N/A        yB           yA       yB
   // yM yA yB :        N/A        yM           yA       yB
   // ---------------------------------------------------------

   switch(y123->element_count())
      {
        case 1: rank_B = y123->get_ravel(0).get_near_int();   break;

        case 2:          y123->get_ravel(0).get_near_int();
                rank_B = y123->get_ravel(1).get_near_int();   break;

        case 3: rank_B = y123->get_ravel(0).get_near_int();
                         y123->get_ravel(1).get_near_int();
                         y123->get_ravel(2).get_near_int();   break;

        default: LENGTH_ERROR;
      }

   // 3. adjust rank_B if they exceed its initial value or
   // if it is negative
   //
   if (rank_B > rk_B)   rank_B = rk_B;
   if (rank_B < 0)      rank_B += rk_B;
   if (rank_B < 0)      rank_B = 0;
}
//-----------------------------------------------------------------------------
void
Bif_OPER2_RANK::y123_to_AB(Value_P y123, Rank & rank_A, Rank & rank_B)
{
   // y123_to_AB() splits the ranks of A and B into a (higher-dimensions)
   // "frame" and a (lower-dimensions) "chunk" as specified by y123.

   // 1. on entry rank_A and rank_B are the ranks of A and B.
   //
   //    Remember the ranks of A and B to limit rank_A and rank_B
   //    if values in y123 should exceed them.
   //
const Rank rk_A = rank_A;
const Rank rk_B = rank_B;

   if (!y123)                   VALUE_ERROR;
   if ( y123->get_rank() > 1)   DOMAIN_ERROR;

   // 2. the number of elements in y determine how rank_A and rank_B
   // shall be computed:
   //
   //                    -- monadic f⍤ --       -- dyadic f⍤ --
   //          	        rank_A     rank_B       rank_A   rank_B
   // ---------------------------------------------------------
   // y        :        N/A        y            y        y
   // yA yB    :        N/A        yB           yA       yB
   // yM yA yB :        N/A        yM           yA       yB
   // ---------------------------------------------------------

   switch(y123->element_count())
      {
        case 1:  rank_A = y123->get_ravel(0).get_near_int();
                 rank_B = rank_A;                            break;

        case 2:  rank_A = y123->get_ravel(0).get_near_int();
                 rank_B = y123->get_ravel(1).get_near_int();  break;

        case 3:           y123->get_ravel(0).get_near_int();
                 rank_A = y123->get_ravel(1).get_near_int();
                 rank_B = y123->get_ravel(2).get_near_int();  break;

        default: LENGTH_ERROR;
      }

   // 3. adjust rank_A and rank_B if they exceed their initial value or
   // if they are negative
   //
   if (rank_A > rk_A)   rank_A = rk_A;
   if (rank_A < 0)      rank_A += rk_A;
   if (rank_A < 0)      rank_A = 0;

   if (rank_B > rk_B)   rank_B = rk_B;
   if (rank_B < 0)      rank_B += rk_B;
   if (rank_B < 0)      rank_B = 0;
}
//-----------------------------------------------------------------------------
void
Bif_OPER2_RANK::split_y123_B(Value_P y123_B, Value_P & y123, Value_P & B)
{
   // The ISO standard and NARS define the reduction pattern for the RANK
   // operator ⍤ as:
   //
   // Z ← A f ⍤ y B		(ISO)
   // Z ←   f ⍤ y B		(ISO)
   // Z ← A f ⍤ [X] y B		(NARS)
   // Z ←   f ⍤ [X] y B		(NARS)
   //
   // GNU APL may bind y to B at tokenization time if y and B are constants
   // This function tries to "unbind" its argument y123_B into the original
   // components y123 (= y in the standard) and B. The tokenization time
   // binding is shown as y123:B
   //
   //    Usage               y123   : B        Result:   j123       B
   //-------------------------------------------------------------------------
   // 1.   f ⍤ (y123):B...   nested   any                y123       B
   // 2.  (f ⍤ y123:⍬)       simple   empty              y123       -
   // 3.   f ⍤ y123:(B)      simple   nested skalar      y123       B
   // 4a.  f ⍤ y123:B...     simple   any                y123       B...
   //

   // y123_B shall be a skalar or vector
   //
   if (y123_B->get_rank() > 1)   RANK_ERROR;

const ShapeItem length = y123_B->element_count();
   if (length == 0)   LENGTH_ERROR;

   // check for case 1 (the only one with nested first element)
   //
   if (y123_B->get_ravel(0).is_pointer_cell())   // (y123)
      {
         y123 = y123_B->get_ravel(0).get_pointer_value();
         if (length == 1)        // empty B
            {
            }
         else if (length == 2)   // skalar B
            {
              const Cell & B0 = y123_B->get_ravel(1);
              if (B0.is_pointer_cell())   // (B)
                 {
                   B = B0.get_pointer_value();
                 }
              else
                 {
                   B = Value_P(LOC);
                   B->next_ravel()->init(B0, B.getref(), LOC);
                 }
            }
         else                    // vector B
            {
              B = Value_P(length - 1, LOC);
              loop(l, length - 1)
                  B->next_ravel()->init(y123_B->get_ravel(l + 1),
                                                          B.getref(), LOC);
            }
         return;
      }

   // case 1. ruled out, so the first 1, 2, or 3 cells are j123.
   // see how many (at most)
   //
int y123_len = 0;
   loop(yy, 3)
      {
        if (yy >= length)   break;
        if (y123_B->get_ravel(yy).is_near_int())   ++y123_len;
        else                                          break;
      }
   if (y123_len == 0)   LENGTH_ERROR;   // at least y1 is needed

   // cases 2.-4. start with integers of length 1, 2, or 3
   //
   if (length == y123_len)   // case 2: y123:⍬
      {
        y123 = y123_B;
        return;
      }

   if (length == (y123_len + 1) &&
       y123_B->get_ravel(y123_len).is_pointer_cell())   // case 3. y123:⊂B
      {
        y123 = Value_P(y123_len, LOC);
        loop(yy, y123_len)
            y123->next_ravel()->init(y123_B->get_ravel(yy), y123.getref(), LOC);
        B = y123_B->get_ravel(y123_len).get_pointer_value();
        return;
      }

   // case 4: y123:B...
   //
   y123 = Value_P(y123_len, LOC);
   loop(yy, y123_len)
       y123->next_ravel()->init(y123_B->get_ravel(yy), y123.getref(), LOC);

   B = Value_P(length - y123_len, LOC);
   loop(bb, (length - y123_len))
       B->next_ravel()->init(y123_B->get_ravel(bb + y123_len), B.getref(), LOC);
}
//-----------------------------------------------------------------------------
