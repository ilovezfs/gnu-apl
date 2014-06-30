/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2014  Dr. Jürgen Sauermann

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
#include "PointerCell.hh"
#include "Workspace.hh"

Bif_OPER2_RANK     Bif_OPER2_RANK::fun;

/* general comment: we use the term 'chunk' instead of 'p-rank' to avoid
 * confusion with the rank of a value
 */

//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::eval_LB(Token & LO, Value_P y123_B)
{
   // B is expected to be "glued" by the prefix parser, which should result
   // in a 2, 3, or 4 element vector:
   //
   //  y1        ⊂B,    or
   //  y1 y2     ⊂B,    or
   //  y1 y2 y3  ⊂B
   //
   if (y123_B->get_rank() != 1)       RANK_ERROR;

const int y_len = y123_B->element_count() - 1;

   if (y_len < 1)   LENGTH_ERROR;
   if (y_len > 3)   LENGTH_ERROR;

Value_P B  = y123_B->get_ravel(y_len).get_pointer_value();
Rank rk_chunk_B = B->get_rank();

   y123_to_B(y_len, y123_B, rk_chunk_B);

   return do_LyXB(LO.get_function(), 0, B, rk_chunk_B);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::eval_LXB(Token & LO, Value_P X, Value_P B)
{
   // B is expected to be "glued" by the prefix parser, which should result
   // in a 2, 3, or 4 element vector:
   //
   //  y1        ⊂B,    or
   //  y1 y2     ⊂B,    or
   //  y1 y2 y3  ⊂B
   //
   if (B->get_rank() != 1)       RANK_ERROR;

const int y_len = B->element_count() - 1;

   if (y_len < 1)   LENGTH_ERROR;
   if (y_len > 3)   LENGTH_ERROR;

Value_P arg  = B->get_ravel(y_len).get_pointer_value();
Rank rk_chunk_B = arg->get_rank();

   y123_to_B(y_len, B, rk_chunk_B);

Shape sh_X(X, Workspace::get_CT(), Workspace::get_IO());
   return do_LyXB(LO.get_function(), &sh_X, arg, rk_chunk_B);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::eval_LRB(Token & LO, Token & y, Value_P B)
{
Rank rk_chunk_B = B->get_rank();

Value_P vy = y.get_apl_val();
   y123_to_B(vy->element_count(), vy, rk_chunk_B);
   return do_LyXB(LO.get_function(), 0, B, rk_chunk_B);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::do_LyXB(Function * LO, const Shape * axes,
                        Value_P B, Rank rk_chunk_B)
{
   if (!LO->has_result())   DOMAIN_ERROR;

   // split shape of B into high and low shapes.
   //
const Shape shape_Z = B->get_shape().high_shape(B->get_rank() - rk_chunk_B);
Value_P Z(new Value(shape_Z, LOC));
EOC_arg arg(Z, B);
RANK_LyXB & _arg = arg.u.u_RANK_LyXB;

   _arg.get_sh_chunk_B() = B->get_shape().low_shape(rk_chunk_B);
   _arg.get_sh_frame() = shape_Z;
   _arg.ec_frame = shape_Z.get_volume();
   _arg.ec_chunk_B = _arg.get_sh_chunk_B().get_volume();
   _arg.cB = &B->get_ravel(0);
   _arg.LO = LO;
   _arg.axes_valid = (axes != 0);
   if (axes)   _arg.get_axes() = *axes;
   _arg.how = 0;

   return finish_LyXB(arg);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::finish_LyXB(EOC_arg & arg)
{
RANK_LyXB & _arg = arg.u.u_RANK_LyXB;

   if (_arg.how == 1)   goto how_1;

   Assert1(_arg.how == 0);
   _arg.h = 0;

loop_h:
   {
     Value_P BB(new Value(_arg.get_sh_chunk_B(), LOC));
     Assert1(_arg.ec_chunk_B == _arg.get_sh_chunk_B().get_volume());
     loop(l, _arg.ec_chunk_B)   BB->get_ravel(l).init(*_arg.cB++);
     BB->check_value(LOC);

     Token result = _arg.LO->eval_B(BB);
     if (result.get_tag() == TOK_SI_PUSHED)
        {
          // LO was a user defined function or ⍎
          //
          _arg.how = 1;
          Workspace::SI_top()->set_eoc_handler(eoc_LyXB);
          Workspace::SI_top()->get_eoc_arg() = arg;
          return result;   // continue in user defined function...
        }

     if (result.get_tag() == TOK_ERROR)   return result;

     if (result.get_Class() == TC_VALUE)
        {
          Value_P ZZ = result.get_apl_val();
          if (ZZ->is_scalar())   arg.Z->next_ravel()->init(ZZ->get_ravel(0));
          else                   new (arg.Z->next_ravel())   PointerCell(ZZ);
          goto next_h;
       }

     Q1(result);   FIXME;
   }

how_1:
next_h:
   if (++_arg.h < _arg.ec_frame)   goto loop_h;

   arg.Z->check_value(LOC);
   if (!_arg.axes_valid)   return Bif_F12_PICK::disclose(arg.Z, true);

   return Bif_F12_PICK::disclose_with_axis(_arg.get_axes(), arg.Z, true);
}
//-----------------------------------------------------------------------------
bool
Bif_OPER2_RANK::eoc_LyXB(Token & token, EOC_arg & si_arg)
{
EOC_arg arg = si_arg;
RANK_LyXB & _arg = arg.u.u_RANK_LyXB;

   if (token.get_Class() != TC_VALUE)  return false;   // stop it

   // the user defined function has returned a value. Store it.
   //
Value_P ZZ = token.get_apl_val();
   if (ZZ->is_scalar())   arg.Z->next_ravel()->init(ZZ->get_ravel(0));
   else                   new (arg.Z->next_ravel())   PointerCell(ZZ);

   if (_arg.h < (_arg.ec_frame - 1))   Workspace::pop_SI(LOC);

   copy_1(token, finish_LyXB(arg), LOC);
   if (token.get_tag() == TOK_SI_PUSHED)   return true;   // continue

   return false;   // stop it
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::eval_ALB(Value_P A, Token & LO, Value_P B)
{
   // B is expected to be "glued" by the prefix parser, which should result
   // in a 2, 3, or 4 element vector:
   //
   //  y1        ⊂B,    or
   //  y1 y2     ⊂B,    or
   //  y1 y2 y3  ⊂B
   //
   if (B->get_rank() != 1)       RANK_ERROR;

const int y_len = B->element_count() - 1;

   if (y_len < 1)   LENGTH_ERROR;
   if (y_len > 3)   LENGTH_ERROR;

Value_P arg  = B->get_ravel(y_len).get_pointer_value();
Rank rk_chunk_A = A->get_rank();
Rank rk_chunk_B = arg->get_rank();

   y123_to_AB(y_len, B, rk_chunk_A, rk_chunk_B);

   return do_ALyXB(A, rk_chunk_A, LO.get_function(), 0, arg, rk_chunk_B);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::eval_ALXB(Value_P A, Token & LO, Value_P X, Value_P B)
{
   // B is expected to be "glued" by the prefix parser, which should result
   // in a 2, 3, or 4 element vector:
   //
   //  y1        ⊂B,    or
   //  y1 y2     ⊂B,    or
   //  y1 y2 y3  ⊂B
   //
   if (B->get_rank() != 1)       RANK_ERROR;

const int y_len = B->element_count() - 1;

   if (y_len < 1)   LENGTH_ERROR;
   if (y_len > 3)   LENGTH_ERROR;

Value_P arg  = B->get_ravel(y_len).get_pointer_value();
Rank rk_chunk_A = A->get_rank();
Rank rk_chunk_B = arg->get_rank();

   y123_to_AB(y_len, B, rk_chunk_A, rk_chunk_B);

Shape sh_X(X, Workspace::get_CT(), Workspace::get_IO());
   return do_ALyXB(A, rk_chunk_A, LO.get_function(), &sh_X, arg, rk_chunk_B);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::eval_ALRB(Value_P A, Token & LO, Token & RO_y, Value_P B)
{
Rank rk_chunk_A = A->get_rank();
Rank rk_chunk_B = B->get_rank();

Value_P vy = RO_y.get_apl_val();
   y123_to_AB(vy->element_count(), vy, rk_chunk_A, rk_chunk_B);
   return do_ALyXB(A, rk_chunk_A, LO.get_function(), 0, B, rk_chunk_B);

return TOK_VOID;
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::eval_ALRXB(Value_P A, Token & LO, Token & RO_y,
                           Value_P X, Value_P B)
{
Rank rk_chunk_A = A->get_rank();
Rank rk_chunk_B = B->get_rank();

Value_P vRO_y = RO_y.get_apl_val();
   y123_to_AB(vRO_y->element_count(), vRO_y, rk_chunk_A, rk_chunk_B);

Shape sh_X(X, Workspace::get_CT(), Workspace::get_IO());
   return do_ALyXB(A, rk_chunk_A, LO.get_function(), &sh_X, B, rk_chunk_B);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::do_ALyXB(Value_P A, Rank rk_chunk_A, Function *  LO,
                         const Shape * axes, Value_P B, Rank rk_chunk_B)
{
   if (!LO->has_result())   DOMAIN_ERROR;

Rank rk_A_frame = A->get_rank() - rk_chunk_A;   // rk_A_frame is y8
Rank rk_B_frame = B->get_rank() - rk_chunk_B;   // rk_B_frame is y9

   // if both high-ranks are 0, then return A LO B.
   //
   if (rk_A_frame == 0 && rk_B_frame == 0)   return LO->eval_AB(A, B);

   // split shapes of A1 and B1 into high (frame) and low (chunk) shapes.
   // Even though A and B have the same shape, rk_A_frame and rk_B_frame
   // could be different, leading to different split shapes for A and B
   //
bool repeat_A = false;
bool repeat_B = false;
Shape sh_A_frame = A->get_shape().high_shape(A->get_rank() - rk_chunk_A);
const Shape sh_B_frame = B->get_shape().high_shape(B->get_rank() - rk_chunk_B);
Shape shape_Z;
   if (rk_A_frame == 0)   // "conform" A to B
      {
        shape_Z = sh_B_frame;
        repeat_A = true;
      }
   else if (rk_B_frame == 0)   // "conform" B to A
      {
        shape_Z = sh_A_frame;
        repeat_B = true;
      }
   else
      {
        if (rk_A_frame != rk_B_frame)   RANK_ERROR;
        if (sh_A_frame != sh_B_frame)   LENGTH_ERROR;
        shape_Z = sh_B_frame;
      }

Value_P Z(new Value(shape_Z, LOC));
EOC_arg arg(Z, B, A);
RANK_ALyXB & _arg = arg.u.u_RANK_ALyXB;

   _arg.get_sh_chunk_A() = A->get_shape().low_shape(rk_chunk_A);
   _arg.get_sh_chunk_B() = B->get_shape().low_shape(rk_chunk_B);
   _arg.get_sh_frame() = B->get_shape().high_shape(B->get_rank() - rk_chunk_B);

   // a trick to avoid conforming A to B or B to A. If A or B
   // needs to be conformed, then we set the corresponding repeat_A or 
   // repeat_B to true and copy the same A or B again and again
   //
   _arg.repeat_A = repeat_A;
   _arg.repeat_B = repeat_B;

   if (_arg.repeat_A)   // "conform" A to B
      {
         rk_A_frame = rk_B_frame;
         sh_A_frame = _arg.get_sh_frame();
      }
   else if (_arg.repeat_B)   // "conform" B to A
      {
         rk_B_frame = rk_A_frame;
         _arg.get_sh_frame() = sh_A_frame;
      }
 
   // at this point sh_A_frame == sh_B_frame, so we can use either of them
   // to compute ec_chunk_A
   //
   _arg.ec_frame   = _arg.get_sh_frame().get_volume();
   _arg.ec_chunk_A = _arg.get_sh_chunk_A().get_volume();
   _arg.ec_chunk_B = _arg.get_sh_chunk_B().get_volume();

   _arg.cA = &A->get_ravel(0);
   _arg.LO = LO;
   _arg.axes_valid = (axes != 0);
   if (axes)   _arg.get_axes() = *axes;
   _arg.cB = &B->get_ravel(0);
   _arg.how = 0;

   return finish_ALyXB(arg);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_RANK::finish_ALyXB(EOC_arg & arg)
{
RANK_ALyXB & _arg = arg.u.u_RANK_ALyXB;

   if (_arg.how == 1)   goto how_1;

   Assert1(_arg.how == 0);
   _arg.h = 0;

loop_h:
   {
     Value_P AA(new Value(_arg.get_sh_chunk_A(), LOC));
     Value_P BB(new Value(_arg.get_sh_chunk_B(), LOC));

     loop(l, _arg.ec_chunk_A)   AA->get_ravel(l).init(*_arg.cA++);
     loop(l, _arg.ec_chunk_B)   BB->get_ravel(l).init(*_arg.cB++);

     if (_arg.repeat_A)   _arg.cA = &arg.A->get_ravel(0);
     if (_arg.repeat_B)   _arg.cB = &arg.B->get_ravel(0);

     AA->check_value(LOC);
     BB->check_value(LOC);

     Token result = _arg.LO->eval_AB(AA, BB);
     if (result.get_tag() == TOK_SI_PUSHED)
        {
          // LO was a user defined function or ⍎
          //
          _arg.how = 1;
          Workspace::SI_top()->set_eoc_handler(eoc_ALyXB);
          Workspace::SI_top()->get_eoc_arg() = arg;
          return result;   // continue in user defined function...
        }

     if (result.get_tag() == TOK_ERROR)   return result;

     if (result.get_Class() == TC_VALUE)
        {
          Value_P ZZ = result.get_apl_val();
          if (ZZ->is_scalar())   arg.Z->next_ravel()->init(ZZ->get_ravel(0));
          else                   new (arg.Z->next_ravel())   PointerCell(ZZ);

          goto next_h;
       }

     Q1(result);   FIXME;
   }

how_1:
next_h:
   if (++_arg.h < _arg.ec_frame)   goto loop_h;

   arg.Z->check_value(LOC);
   if (!_arg.axes_valid)   return Bif_F12_PICK::disclose(arg.Z, true);

   return Bif_F12_PICK::disclose_with_axis(_arg.get_axes(), arg.Z, true);
}
//-----------------------------------------------------------------------------
bool
Bif_OPER2_RANK::eoc_ALyXB(Token & token, EOC_arg & si_arg)
{
EOC_arg arg = si_arg;
RANK_ALyXB & _arg = arg.u.u_RANK_ALyXB;

   if (token.get_Class() != TC_VALUE)  return false;   // stop it

   // the user defined function has returned a value. Store it.
   //
Value_P ZZ = token.get_apl_val();
   if (ZZ->is_scalar())   arg.Z->next_ravel()->init(ZZ->get_ravel(0));
   else                   new (arg.Z->next_ravel())   PointerCell(ZZ);

   if (_arg.h < (_arg.ec_frame - 1))   Workspace::pop_SI(LOC);

   copy_1(token, finish_ALyXB(arg), LOC);
   if (token.get_tag() == TOK_SI_PUSHED)   return true;   // continue

   return false;   // stop it
}
//-----------------------------------------------------------------------------
void
Bif_OPER2_RANK::y123_to_B(int count, Value_P y123, Rank & rank_B)
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

const APL_Float qct = Workspace::get_CT();

   switch(count)
      {
        case 1: rank_B = y123->get_ravel(0).get_near_int(qct);   break;

        case 2:          y123->get_ravel(0).get_near_int(qct);
                rank_B = y123->get_ravel(1).get_near_int(qct);   break;

        case 3: rank_B = y123->get_ravel(0).get_near_int(qct);
                         y123->get_ravel(1).get_near_int(qct);
                         y123->get_ravel(2).get_near_int(qct);   break;

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
Bif_OPER2_RANK::y123_to_AB(int count, Value_P y123,
                           Rank & rank_A, Rank & rank_B)
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

const APL_Float qct = Workspace::get_CT();

   switch(count)
      {
        case 1:  rank_A = y123->get_ravel(0).get_near_int(qct);
                 rank_B = rank_A;                            break;

        case 2:  rank_A = y123->get_ravel(0).get_near_int(qct);
                 rank_B = y123->get_ravel(1).get_near_int(qct);  break;

        case 3:           y123->get_ravel(0).get_near_int(qct);
                 rank_A = y123->get_ravel(1).get_near_int(qct);
                 rank_B = y123->get_ravel(2).get_near_int(qct);  break;

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
