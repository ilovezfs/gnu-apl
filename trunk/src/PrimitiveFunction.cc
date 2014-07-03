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

#include <string.h>
#include <stdio.h>

#include "ArrayIterator.hh"
#include "Avec.hh"
#include "Bif_OPER1_EACH.hh"
#include "CharCell.hh"
#include "ComplexCell.hh"
#include "Command.hh"
#include "CollatingCache.hh"
#include "FloatCell.hh"
#include "Id.hh"
#include "IndexExpr.hh"
#include "IndexIterator.hh"
#include "IntCell.hh"
#include "LvalCell.hh"
#include "Output.hh"
#include "PointerCell.hh"
#include "PrimitiveFunction.hh"
#include "PrintOperator.hh"
#include "StateIndicator.hh"
#include "UserFunction.hh"
#include "Value.icc"
#include "Workspace.hh"
                                            
Bif_F0_ZILDE      Bif_F0_ZILDE::fun;         // ⍬
Bif_F1_EXECUTE    Bif_F1_EXECUTE::fun;       // ⍎
Bif_F2_INDEX      Bif_F2_INDEX::fun;         // ⌷
Bif_F12_PARTITION Bif_F12_PARTITION::fun;    // ⊂
Bif_F12_PICK      Bif_F12_PICK::fun;         // ⊃
Bif_F12_COMMA     Bif_F12_COMMA::fun;        // ,
Bif_F12_COMMA1    Bif_F12_COMMA1::fun;       // ⍪
Bif_F12_TAKE      Bif_F12_TAKE::fun;         // ↑
Bif_F12_DROP      Bif_F12_DROP::fun;         // ↓
Bif_F12_ELEMENT   Bif_F12_ELEMENT::fun;      // ∈
Bif_F12_EQUIV     Bif_F12_EQUIV::fun;        // ≡
Bif_F12_NEQUIV    Bif_F12_NEQUIV::fun;       // ≢
Bif_F12_ENCODE    Bif_F12_ENCODE::fun;       // ⊤
Bif_F12_DECODE    Bif_F12_DECODE::fun;       // ⊥
Bif_F12_DOMINO    Bif_F12_DOMINO::fun;       // ⌹
Bif_F12_ROTATE    Bif_F12_ROTATE::fun;       // ⌽
Bif_F12_ROTATE1   Bif_F12_ROTATE1::fun;      // ⊖
Bif_F12_TRANSPOSE Bif_F12_TRANSPOSE::fun;    // ⍉
Bif_F12_SORT_ASC  Bif_F12_SORT_ASC::fun;     // ⍋
Bif_F12_SORT_DES  Bif_F12_SORT_DES::fun;     // ⍒
Bif_F12_INDEX_OF  Bif_F12_INDEX_OF::fun;     // ⍳
Bif_F12_RHO       Bif_F12_RHO::fun;          // ⍴
Bif_F2_INTER      Bif_F2_INTER::fun;         // ∩
Bif_F12_UNION     Bif_F12_UNION::fun;        // ∪
Bif_F2_LEFT       Bif_F2_LEFT::fun;          // ⊣
Bif_F2_RIGHT      Bif_F2_RIGHT::fun;         // ⊢

const CharCell PrimitiveFunction::c_filler(UNI_ASCII_SPACE);
const IntCell  PrimitiveFunction::n_filler(0);

//-----------------------------------------------------------------------------
Token
PrimitiveFunction::eval_fill_AB(Value_P A, Value_P B)
{
   return eval_AB(A, B);
}
//-----------------------------------------------------------------------------
Token
PrimitiveFunction::eval_fill_B(Value_P B)
{
   return eval_B(B);
}
//-----------------------------------------------------------------------------
ostream &
PrimitiveFunction::print(ostream & out) const
{
   return out << get_Id();
}
//-----------------------------------------------------------------------------
void
PrimitiveFunction::print_properties(ostream & out, int indent) const
{
UCS_string ind(indent, UNI_ASCII_SPACE);
   out << ind << "System Function ";
   print(out);
   out << endl;
}
//=============================================================================
Token
Bif_F0_ZILDE::eval_()
{
   return Token(TOK_APL_VALUE1, Idx0(LOC));
}
//=============================================================================
Token
Bif_F12_RHO::eval_B(Value_P B)
{
Value_P Z(new Value(B->get_rank(), LOC));

   loop(r, B->get_rank())   new (&Z->get_ravel(r)) IntCell(B->get_shape_item(r));

   Z->set_default_Zero();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_RHO::eval_AB(Value_P A, Value_P B)
{
const Shape shape_Z(A, Workspace::get_CT(), 0);

   return do_reshape(shape_Z, B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_RHO::do_reshape(const Shape & shape_Z, Value_P B)
{
const ShapeItem len_B = B->element_count();
const ShapeItem len_Z = shape_Z.get_volume();

   // check that shape_Z is positive
   //
   loop(z, shape_Z.get_rank())
      {
        if (shape_Z.get_shape_item(z) < 0)   DOMAIN_ERROR;
      }

#if 0
   // optimize if Z is not larger than B.
   //
   if (len_Z <= len_B && B->is_temp())
      {
        Log(LOG_optimization) CERR << "optimizing A⍴B" << endl;

        // release unused cells
        //
        ShapeItem rr = len_Z;
        if (rr == 0)   rr = 1;
        while (rr < len_B)
            B->get_ravel(rr++).release(LOC);

        B->set_shape(shape_Z);
        return Token(TOK_APL_VALUE1, B);
      }
#endif

Value_P Z(new Value(shape_Z, LOC));


   if (len_B == 0)   // empty B: use prototype
      {
        loop(z, len_Z)   Z->get_ravel(z).init_type(B->get_ravel(0));
      }
   else
      {
        loop(z, len_Z)   Z->get_ravel(z).init(B->get_ravel(z % len_B));
      }

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_INDEX_OF::eval_B(Value_P B)
{
   // interval

const APL_Float qct = Workspace::get_CT();
const APL_Integer qio = Workspace::get_IO();

   if (!B->is_scalar_or_len1_vector())
      {
        if (B->get_rank() > 1)   RANK_ERROR;
        else                    LENGTH_ERROR;
      }

const Cell * cell = &B->get_ravel(0);
const APL_Integer len = cell->get_near_int(qct);
   if (len < 0)   DOMAIN_ERROR;

Value_P Z(new Value(len, LOC));

   loop(z, len)   new (&Z->get_ravel(z)) IntCell(qio + z);

   Z->set_default_Zero();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_INDEX_OF::eval_AB(Value_P A, Value_P B)
{
const APL_Float qct = Workspace::get_CT();
const APL_Integer qio = Workspace::get_IO();

   // Index of
   //
   if (!A->is_scalar_or_vector())   RANK_ERROR;

const uint64_t len_A  = A->element_count();
const uint64_t len_BZ = B->element_count();

Value_P Z(new Value(B->get_shape(), LOC));

   loop(bz, len_BZ)
       {
         const Cell & cell_B = B->get_ravel(bz);
         bool found = false;
         loop(a, len_A)
             {
               if (cell_B.equal(A->get_ravel(a), qct))
                  {
                    new (&Z->get_ravel(bz)) IntCell(qio + a);
                    found = true;
                    break;
                  }
             }
         if (!found)
            new (&Z->get_ravel(bz)) IntCell(qio + len_A);
       }

   Z->set_default_Zero();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_COMMA::ravel(const Shape & new_shape, Value_P B)
{
Value_P Z(new Value(new_shape, LOC));

const ShapeItem count = B->element_count();
   Assert(count == Z->element_count());

   loop(c, count)   Z->get_ravel(c).init(B->get_ravel(c));

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Value_P
Bif_COMMA::prepend_scalar(const Cell & cell_A, Axis axis, Value_P B)
{
   if (B->is_empty())
      {
        Shape shape_Z = B->get_shape();
        shape_Z.set_shape_item(axis, shape_Z.get_shape_item(axis) + 1);
        Value_P Z(new Value(shape_Z, LOC));
        if (Z->is_empty())
           {
              Z->get_ravel(0).init(cell_A);
           }
        else
           {
             loop(z, Z->element_count())   Z->next_ravel()->init(cell_A);
           }
        return Z;
      }

   if (B->is_scalar())
      {
        Value_P Z(new Value(2, LOC));
        Z->get_ravel(0).init(cell_A);
        Z->get_ravel(1).init(B->get_ravel(0));
        Z->check_value(LOC);
        return Z;
      }

   if (axis >= B->get_rank())   INDEX_ERROR;

Shape shape_Z(B->get_shape());
   shape_Z.set_shape_item(axis, shape_Z.get_shape_item(axis) + 1);

Value_P Z(new Value(shape_Z, LOC));

const Shape3 shape_B3(B->get_shape(), axis);
   const uint32_t slice_a = shape_B3.l();
   const uint32_t slice_b = shape_B3.l() * B->get_shape_item(axis);

const Cell * cB = &B->get_ravel(0);

   loop(hz, shape_B3.h())
       {
         loop(lz, slice_a)   Z->next_ravel()->init(cell_A);

         Cell::copy(*Z.get(), cB, slice_b);
       }

   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Bif_COMMA::append_scalar(Value_P A, Axis axis, const Cell & cell_B)
{
   if (A->is_empty())
      {
        Shape shape_Z = A->get_shape();
        shape_Z.set_shape_item(axis, shape_Z.get_shape_item(axis) + 1);
        Value_P Z(new Value(shape_Z, LOC));
        if (Z->is_empty())
           {
              Z->get_ravel(0).init(cell_B);
           }
        else
           {
             loop(z, Z->element_count())   Z->next_ravel()->init(cell_B);
           }
        return Z;
      }

   // A->is_scalar() is handled by prepend_scalar()

   if (axis >= A->get_rank())   INDEX_ERROR;

Shape shape_Z(A->get_shape());
   shape_Z.set_shape_item(axis, shape_Z.get_shape_item(axis) + 1);

Value_P Z(new Value(shape_Z, LOC));

const Shape3 shape_A3(A->get_shape(), axis);
const uint32_t slice_a = shape_A3.l() * A->get_shape_item(axis);
const uint32_t slice_b = shape_A3.l();

const Cell * cA = &A->get_ravel(0);

   loop(hz, shape_A3.h())
       {
         Cell::copy(*Z.get(), cA, slice_a);
         loop(lz, slice_b)   Z->next_ravel()->init(cell_B);
       }

   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Token
Bif_COMMA::catenate(Value_P A, Axis axis, Value_P B)
{
   if (A->is_scalar())
      {
        const Cell & cell_A = A->get_ravel(0);
        Value_P Z = prepend_scalar(cell_A, axis, B);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (B->is_scalar())
      {
        const Cell & cell_B = B->get_ravel(0);
        Value_P Z = append_scalar(A, axis, cell_B);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if ((A->get_rank() + 1) == B->get_rank())
      {
        Shape shape_Z;

        // check shape conformance.
        {
          ShapeItem ra = 0;
          loop(rb, B->get_rank())
             {
               if (rb != axis)
                  {
                    shape_Z.add_shape_item(B->get_shape_item(rb));
                    if (A->get_shape_item(ra) != B->get_shape_item(rb))
                       {
                         LENGTH_ERROR;
                       }
                    ++ra;
                  }
               else
                  {
                    shape_Z.add_shape_item(B->get_shape_item(rb) + 1);
                  }
             }
        }

        Value_P Z(new Value(shape_Z, LOC));

        Z->set_default(*B.get());

        const Shape3 shape_B3(B->get_shape(), axis);
        const uint32_t slice_a = shape_B3.l();
        const uint32_t slice_b = shape_B3.l() * B->get_shape_item(axis);

        const Cell * cA = &A->get_ravel(0);
        const Cell * cB = &B->get_ravel(0);

        loop(hz, shape_B3.h())
            {
              Cell::copy(*Z.get(), cA, slice_a);
              Cell::copy(*Z.get(), cB, slice_b);
            }

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (A->get_rank() == (B->get_rank() + 1))
      {
        // e.g.	        ∆∆∆ , 3
        //              ∆∆∆   4
        //
        Shape shape_Z;

        // check shape conformance. We step ranks ra and rb, except for the
        // axis where only ra is incremented.
        {
          uint32_t rb = 0;
          loop(ra, A->get_rank())
             {
               if (ra != axis)
                  {
                    if (A->get_shape_item(ra) != B->get_shape_item(rb))
                       LENGTH_ERROR;

                    shape_Z.add_shape_item(A->get_shape_item(ra));
                    ++rb;
                  }
               else
                  {
                    shape_Z.add_shape_item(A->get_shape_item(ra) + 1);
                  }
             }
        }

        Value_P Z(new Value(shape_Z, LOC));

        Z->set_default(*B.get());

        const Shape3 shape_A3(A->get_shape(), axis);
        const uint32_t slice_a = shape_A3.l() * A->get_shape_item(axis);
        const uint32_t slice_b = shape_A3.l();

        const Cell * cA = &A->get_ravel(0);
        const Cell * cB = &B->get_ravel(0);

        loop (hz, shape_A3.h())
            {
              Cell::copy(*Z.get(), cA, slice_a);
              Cell::copy(*Z.get(), cB, slice_b);
            }

        Z->set_default(*B.get());

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (A->get_rank() != B->get_rank())   RANK_ERROR;

   // A and B have the same rank. The shapes need to agree, except for the
   // dimension corresponding to the axis.
   //
Shape shape_Z;

   loop(r, A->get_rank())
       {
         if (r != axis)
            {
              if (A->get_shape_item(r) != B->get_shape_item(r))
                 LENGTH_ERROR;

              shape_Z.add_shape_item(A->get_shape_item(r));
            }
         else
            {
              shape_Z.add_shape_item(A->get_shape_item(r) +
                                   + B->get_shape_item(r));
            }
       }

Value_P Z(new Value(shape_Z, LOC));

   Z->set_default(*B.get());

const Shape3 shape_A3(A->get_shape(), axis);

const Cell * cA = &A->get_ravel(0);
const Cell * cB = &B->get_ravel(0);
const uint32_t slice_a = shape_A3.l() * A->get_shape_item(axis);
const uint32_t slice_b = shape_A3.l() * B->get_shape_item(axis);

   loop(hz, shape_A3.h())
       {
         Cell::copy(*Z.get(), cA, slice_a);
         Cell::copy(*Z.get(), cB, slice_b);
       }

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_COMMA::laminate(Value_P A, Axis axis, Value_P B)
{
   // shapes of A and B must be the same, unless one of them is a scalar.
   //
   if (!A->is_scalar() && !B->is_scalar())
      A->get_shape().check_same(B->get_shape(),
                                E_INDEX_ERROR, E_LENGTH_ERROR, LOC);

const Shape shape_Z = A->is_scalar() ? B->get_shape().insert_axis(axis, 2)
                                    : A->get_shape().insert_axis(axis, 2);

Value_P Z(new Value(shape_Z, LOC));

const Shape3 shape_Z3(shape_Z, axis);
   if (shape_Z3.m() != 2)   AXIS_ERROR;

const Cell * cA = &A->get_ravel(0);
const Cell * cB = &B->get_ravel(0);

   if (A->is_scalar())
      {
        if (B->is_scalar())
           {
              Z->next_ravel()->init(*cA);
              Z->next_ravel()->init(*cB);
           }
        else
           {
             loop(h, shape_Z3.h())
                 {
                   loop(l, shape_Z3.l())   Z->next_ravel()->init(*cA);
                   Cell::copy(*Z.get(), cB, shape_Z3.l());
                }
           }
      }
   else
      {
        if (B->is_scalar())
           {
             loop(h, shape_Z3.h())
                 {
                   Cell::copy(*Z.get(), cA, shape_Z3.l());
                   loop(l, shape_Z3.l())   Z->next_ravel()->init(*cB);
                }
           }
        else
           {
             loop(h, shape_Z3.h())
                 {
                   Cell::copy(*Z.get(), cA, shape_Z3.l());
                   Cell::copy(*Z.get(), cB, shape_Z3.l());
                }
           }
      }

   Z->set_default(*B.get());

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_COMMA::ravel_axis(Value_P X, Value_P B, Axis axis)
{
const APL_Float qct = Workspace::get_CT();
const APL_Integer qio = Workspace::get_IO();

   if (!X)   SYNTAX_ERROR;

   // I must be a (integer or real) scalar or simple integer vector
   //
   if (X->get_rank() > 1)   INDEX_ERROR;

   // There are 3 variants, determined by I:
   //
   // 1) I is empty:             append a new last axis of length 1
   // 2) I is a fraction:        append a new axis before I
   // 3a) I is integer scalar:   return B
   // 3b) I is integer vector:   combine axes

   // case 1:   ,['']B or ,[⍳0]B : append new first or last axis of length 1.
   //
   if (X->element_count() == 0)
      {
        if (B->get_rank() == MAX_RANK)   AXIS_ERROR;

        const Shape shape_Z = B->get_shape().insert_axis(axis, 1);
        return ravel(shape_Z, B);
      }

   // case 2:   ,[x.y]B : insert axis before axis x+1
   //
   if (!X->get_ravel(0).is_near_int(qct))  // fraction: insert an axis
      {
        if (B->get_rank() == MAX_RANK)   INDEX_ERROR;

        const APL_Float new_axis = X->get_ravel(0).get_real_value() - qio;
        const Axis axis((new_axis < 0) ? -1 : new_axis);
        const Shape shape_Z = B->get_shape().insert_axis(axis + 1, 1);
        return ravel(shape_Z, B);
      }

   // case 3a: ,[n]B : return B (combine single axis doesn't change anything)
   //
   if (X->is_scalar_or_len1_vector())   // single int: return B->
      {
        Token result(TOK_APL_VALUE1, B->clone(LOC));
        return result;
      }

   // case 3b: ,[n1 ... nk]B : combine axes.
   //
const Shape axes(X, qct, qio);

const ShapeItem from = axes.get_shape_item(0);
   if (from < 0)   AXIS_ERROR;

const ShapeItem to   = axes.get_last_shape_item();
   if (to > B->get_rank())   AXIS_ERROR;

   // check that the axes are contiguous and compute the number of elements
   // in the combined axes
   //
ShapeItem count = 1;
   loop(a, axes.get_rank())
      {
        if (axes.get_shape_item(a) != (from + a))   AXIS_ERROR;
        count *= B->get_shape_item(from + a);
      }

Shape shape_Z;
   loop (r, from)   shape_Z.add_shape_item(B->get_shape_item(r));
   shape_Z.add_shape_item(count);
   for (Rank r = to + 1; r < B->get_rank(); ++r)
       shape_Z.add_shape_item(B->get_shape_item(r));

   return ravel(shape_Z, B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_COMMA::eval_B(Value_P B)
{
const Shape shape_Z(B->element_count());
   if (B->is_temp())
      {
        Log(LOG_optimization) CERR << "optimizing ,B" << endl;

        B->set_shape(shape_Z);
        return Token(TOK_APL_VALUE1, B);
      }

   return ravel(shape_Z, B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_COMMA::eval_AB(Value_P A, Value_P B)
{
Rank max_rank = A->get_rank();
   if (max_rank < B->get_rank())  max_rank = B->get_rank(); 
   return catenate(A, max_rank - 1, B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_COMMA::eval_AXB(Value_P A, Value_P X, Value_P B)
{
   // catenate or laminate
   //
   if (!X->is_scalar_or_len1_vector())   AXIS_ERROR;

const Cell & cX = X->get_ravel(0);

const APL_Float qct = Workspace::get_CT();
   if (cX.is_near_int(qct))   // catenate along existing axis
      {
        const Axis axis = cX.get_checked_near_int() - Workspace::get_IO();
        if (axis < 0)                                       AXIS_ERROR;
        if (axis >= A->get_rank() && axis >= B->get_rank())   AXIS_ERROR;
        return catenate(A, axis, B);
      }

   return laminate(A, Axis(cX.get_real_value()) - Workspace::get_IO() + 1, B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_COMMA1::eval_B(Value_P B)
{
   // turn B into a matrix
   //
ShapeItem c1 = 1;   // assume B is scalar
ShapeItem c2 = 1;   // assume B is scalar;

   if (B->get_rank() >= 1)
      {
        c1 = B->get_shape_item(0);
        for (Rank r = 1; r < B->get_rank(); ++r)   c2 *= B->get_shape_item(r);
      }

Shape shape_Z(c1, c2);
   if (B->is_temp())
      {
        Log(LOG_optimization) CERR << "optimizing ,B" << endl;

        B->set_shape(shape_Z);
        return Token(TOK_APL_VALUE1, B);
      }
   return ravel(shape_Z, B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_COMMA1::eval_AB(Value_P A, Value_P B)
{
   return catenate(A, 0, B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_COMMA1::eval_AXB(Value_P A, Value_P X, Value_P B)
{
   return Bif_F12_COMMA::fun.eval_AXB(A, X, B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_DOMINO::eval_B(Value_P B)
{
   if (B->is_scalar())
      {
        Value_P Z(new Value(LOC));

        B->get_ravel(0).bif_reciprocal(&Z->get_ravel(0));
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (B->get_rank() == 1)   // inversion at the unit sphere
      {
        const APL_Float qct = Workspace::get_CT();
        const uint32_t len = B->get_shape_item(0);
        APL_Complex r2(0.0);
        loop(l, len)
            {
              const APL_Complex b = B->get_ravel(l).get_complex_value();
              r2 += b*b;
            }

        if (r2.real() < qct && r2.real() > -qct && 
            r2.imag() < qct && r2.imag() > -qct)
            DOMAIN_ERROR;
            
        Value_P Z(new Value(len, LOC));

        if (r2.imag() < qct && r2.imag() > -qct)   // real result
           {
             loop(l, len)
                 {
                   const APL_Float b = B->get_ravel(l).get_real_value();
                   new (&Z->get_ravel(l))   FloatCell(b / r2.real());
                 }
           }
        else                                       // complex result
           {
             loop(l, len)
                 {
                   const APL_Complex b = B->get_ravel(l).get_complex_value();
                   new (&Z->get_ravel(l))   ComplexCell(b / r2);
                 }
           }

        Z->set_default(*B.get());
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem rows = B->get_shape_item(0);
const ShapeItem cols = B->get_shape_item(1);
   if (cols > rows)   LENGTH_ERROR;

   // create an identity matrix I and call eval_AB(I, B).
   //
const Shape shape_I(rows, rows);
Value_P I(new Value(shape_I, LOC));

   loop(r, rows)
   loop(c, rows)
       new (I->next_ravel()) FloatCell((c == r) ? 1.0 : 0.0);

Token result = eval_AB(I, B);
   return result;
}
//-----------------------------------------------------------------------------
struct doublecomplex { double r; double i; };
extern Value_P divide_matrix(ShapeItem rows, Value_P A, Value_P B,
                             const Shape & shape_Z, APL_Float qct);

Token
Bif_F12_DOMINO::eval_AB(Value_P A, Value_P B)
{
const APL_Float qct = Workspace::get_CT();
ShapeItem rows_A = 1;
ShapeItem cols_A = 1;

Shape shape_Z;

   // if rank of A or B is < 2 then treat it as a 
   // 1 by n (or 1 by 1) matrix..
   //
ShapeItem rows_B = 1;
ShapeItem cols_B = 1;
   switch(B->get_rank())
      {
         case 0:  break;

         case 1:  rows_B = B->get_shape_item(0);
                  break;

         case 2:  cols_B = B->get_shape_item(1);
                  rows_B = B->get_shape_item(0);
                  shape_Z.add_shape_item(cols_B);
                  break;

         default: RANK_ERROR;
      }

   switch(A->get_rank())
      {
         case 0:  break;

         case 1:  rows_A = A->get_shape_item(0);
                  break;

         case 2:  cols_A = A->get_shape_item(1);
                  rows_A = A->get_shape_item(0);
                  shape_Z.add_shape_item(cols_A);
                  break;

         default: RANK_ERROR;
      }

   if (rows_B <  cols_B)   LENGTH_ERROR;
   if (rows_A != rows_B)   LENGTH_ERROR;

Value_P Z = divide_matrix(rows_A, A, B, shape_Z, qct);

   Z->set_default(*B.get());

   // round items close to integers
   //
   {
     ShapeItem ec_Z = shape_Z.get_volume();
     Cell * cZ = &Z->get_ravel(0);
     loop(z, ec_Z)
        {
          if (cZ->is_complex_cell())   cZ++->demote_complex_to_real(1.0e-15);
          else                         cZ++->demote_float_to_int(1.0e-15);
        }
   }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_DOMINO::eval_fill_B(Value_P B)
{
   return Bif_F12_TRANSPOSE::fun.eval_B(B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_DOMINO::eval_fill_AB(Value_P A, Value_P B)
{
Shape shape_Z;
   loop(r, A->get_rank() - 1)  shape_Z.add_shape_item(A->get_shape_item(r + 1));
   loop(r, B->get_rank() - 1)  shape_Z.add_shape_item(B->get_shape_item(r + 1));

Value_P Z(new Value(shape_Z, LOC));
   while (Z->more())   new (Z->next_ravel())   IntCell(0);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_ROTATE::reverse(Value_P B, Axis axis)
{
   if (B->is_scalar())
      {
        Token result(TOK_APL_VALUE1, B->clone(LOC));
        return result;
      }

const Shape3 shape_B3(B->get_shape(), axis);

Value_P Z(new Value(B->get_shape(), LOC));

   loop(h, shape_B3.h())
   loop(l, shape_B3.l())
       {
         const uint32_t hl = h * shape_B3.m() * shape_B3.l() + l;
         const Cell * cB = &B->get_ravel(hl);
         Cell * cZ = &Z->get_ravel(hl);
         loop(m, shape_B3.m())
            cZ[m*shape_B3.l()].init(cB[(shape_B3.m() - m - 1)*shape_B3.l()]);
       }

   Z->set_default(*B.get());

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_ROTATE::rotate(Value_P A, Value_P B, Axis axis)
{
const APL_Float qct = Workspace::get_CT();
int32_t gsh = 0;   // global shift (scalar A); 0 means local shift (A) used.

const Shape3 shape_B3(B->get_shape(), axis);
const Shape shape_A2(shape_B3.h(), shape_B3.l());

   if (A->is_scalar_or_len1_vector())
      {
        gsh = A->get_ravel(0).get_near_int(qct);
        if (gsh == 0)   // nothing to do.
           {
             Token result(TOK_APL_VALUE1, B->clone(LOC));
             return result;
           }
      }
   else   // otherwise shape A must be shape B with axis removed.
      {
        A->get_shape().check_same(B->get_shape().without_axis(axis),
                                 E_RANK_ERROR, E_LENGTH_ERROR, LOC);
      }


Value_P Z(new Value(B->get_shape(), LOC));

   loop(h, shape_B3.h())
   loop(m, shape_B3.m())
   loop(l, shape_B3.l())
       {
         ShapeItem src = gsh;
         if (!src)   src = A->get_ravel(l + h*shape_B3.l()).get_near_int(qct);
         src += shape_B3.m() + m;
         while (src < 0)               src += shape_B3.m();
         while (src >= shape_B3.m())   src -= shape_B3.m();
         Z->next_ravel()->init(B->get_ravel(shape_B3.hml(h, src, l)));
       }

   Z->set_default(*B.get());

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ROTATE::eval_XB(Value_P X, Value_P B)
{
const Axis axis = X->get_single_axis(B->get_rank());
   return reverse(B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ROTATE::eval_AXB(Value_P A, Value_P X, Value_P B)
{
const Axis axis = X->get_single_axis(B->get_rank());

   return rotate(A, B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ROTATE1::eval_XB(Value_P X, Value_P B)
{
const Axis axis = X->get_single_axis(B->get_rank());

   return reverse(B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ROTATE1::eval_AXB(Value_P A, Value_P X, Value_P B)
{
const Axis axis = X->get_single_axis(B->get_rank());

   return rotate(A, B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_TRANSPOSE::eval_B(Value_P B)
{
Shape shape_A;

   // monadic transpose is A⍉B with A = ... 4 3 2 1 0
   //
   loop(r, B->get_rank())   shape_A.add_shape_item(B->get_rank() - r - 1);

Value_P Z = transpose(shape_A, B);
   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_TRANSPOSE::eval_AB(Value_P A, Value_P B)
{
   // A should be a scalar or vector.
   //
   if (A->get_rank() > 1)   RANK_ERROR;

   if (B->is_scalar())   // B is a scalar (so A should be empty)
      {
        if (A->element_count() != 0)   LENGTH_ERROR;
        Value_P Z = B->clone(LOC);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

const Shape shape_A(A, Workspace::get_CT(), Workspace::get_IO());
   if (shape_A.get_rank() != B->get_rank())   LENGTH_ERROR;

   // the elements in A shall be valid axes of B->
   loop(r, A->get_rank())
      {
        if (shape_A.get_shape_item(r) < 0)               DOMAIN_ERROR;
        if (shape_A.get_shape_item(r) >= B->get_rank())   DOMAIN_ERROR;
      }

Value_P Z = (shape_A.get_rank() == B->get_rank() && is_permutation(shape_A))
          ? transpose(shape_A, B) : transpose_diag(shape_A, B);

   Z->set_default(*B.get());

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Value_P
Bif_F12_TRANSPOSE::transpose(const Shape & A, Value_P B)
{
const Shape shape_Z = permute(B->get_shape(), inverse_permutation(A));
Value_P Z(new Value(shape_Z, LOC));

   if (Z->is_empty())
      {
         Z->set_default(*B.get());
         return Z;
      }

const Cell * cB = &B->get_ravel(0);

   for (ArrayIterator it_Z(shape_Z); !it_Z.done(); ++it_Z)
       {
         const Shape idx_B = permute(it_Z.get_values(), A);
         const ShapeItem b = B->get_shape().ravel_pos(idx_B);
         Z->next_ravel()->init(cB[b]);
       }

   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Bif_F12_TRANSPOSE::transpose_diag(const Shape & A, Value_P B)
{
   // check that: ∧/(⍳⌈/0,A)∈A
   //
   // I.e. 0, 1, ... max_A are in A
   // we search sequentially, since A is small.
   // we construct shape_Z as we go.
   //
Shape shape_Z;
   {
     // check that 0 ≤ A[a] ≤ ⍴⍴B, and compute max_A = ⌈/A
     //
     ShapeItem max_A = 0;   // the largest index in A (- qio).
     loop(ra, A.get_rank())
         {
           if (A.get_shape_item(ra) < 0)               DOMAIN_ERROR;
           if (A.get_shape_item(ra) >= B->get_rank())   DOMAIN_ERROR;
           if (max_A < A.get_shape_item(ra))   max_A = A.get_shape_item(ra);
         }

     // check that every m < max_A is in A->
     // the first item found is added shape_Z.
     //
     loop(m, max_A + 1)
         {
           loop(ra, A.get_rank())
               {
                if (m == A.get_shape_item(ra))
                   {
                     shape_Z.add_shape_item(B->get_shape_item(ra));
                     goto found;
                   }
               }

           // not found
           DOMAIN_ERROR;

           found: ;
         }
   }

Value_P Z(new Value(shape_Z, LOC));
   if (Z->is_empty())
      {
         Z->set_default(*B.get());
        return Z;
      }

const Cell * cB = &B->get_ravel(0);

   for (ArrayIterator it_Z(shape_Z); !it_Z.done(); ++it_Z)
       {
         const Shape idx_B = permute(it_Z.get_values(), A);
         const ShapeItem b = B->get_shape().ravel_pos(idx_B);
         Z->next_ravel()->init(cB[b]);
       }

   return Z;
}
//-----------------------------------------------------------------------------
Shape
Bif_F12_TRANSPOSE::inverse_permutation(const Shape & sh)
{
ShapeItem rho[MAX_RANK];

   // first, set all items to -1.
   //
   loop(r, sh.get_rank())   rho[r] = -1;

   // then, set all items to the shape items of sh
   //
   loop(r, sh.get_rank())
       {
         const ShapeItem rx = sh.get_shape_item(r);
         if (rx < 0)                 DOMAIN_ERROR;
         if (rx >= sh.get_rank())    DOMAIN_ERROR;
         if (rho[rx] != -1)          DOMAIN_ERROR;

         rho[rx] = r;
       }

   return Shape(sh.get_rank(), rho);
}
//-----------------------------------------------------------------------------
Shape
Bif_F12_TRANSPOSE::permute(const Shape & sh, const Shape & perm)
{
Shape ret;

   loop(r, perm.get_rank())
      {
        ret.add_shape_item(sh.get_shape_item(perm.get_shape_item(r)));
      }

   return ret;
}
//-----------------------------------------------------------------------------
bool
Bif_F12_TRANSPOSE::is_permutation(const Shape & sh)
{
ShapeItem rho[MAX_RANK];

   // first, set all items to -1.
   //
   loop(r, sh.get_rank())   rho[r] = -1;

   // then, set all items to the shape items of sh
   //
   loop(r, sh.get_rank())
       {
         const ShapeItem rx = sh.get_shape_item(r);
         if (rx < 0)                return false;
         if (rx >= sh.get_rank())   return false;
         if (rho[rx] != -1)         return false;
          rho[rx] = r;
       }

   return true;
}
//=============================================================================
Token
Bif_F12_DECODE::eval_AB(Value_P A, Value_P B)
{
   // ρZ  is: (¯1↓ρA),1↓ρB
   // ρρZ is: (0⌈¯1+ρρA) + (0⌈¯1+ρρB)
   //
Shape shape_A1;
   if (!A->is_scalar())
      shape_A1 = A->get_shape().without_axis(A->get_rank() - 1);

Shape shape_B1(B->get_shape());
   if (shape_B1.get_rank() > 0)   shape_B1 = B->get_shape().without_axis(0);

const ShapeItem l_len_A = A->get_rank() ? A->get_last_shape_item() : 1;
const ShapeItem h_len_B = B->get_rank() ? B->get_shape_item(0)     : 1;

const ShapeItem h_len_A = shape_A1.get_volume();
const ShapeItem l_len_B = shape_B1.get_volume();

   if (l_len_A == 0 || h_len_B == 0)   // empty result
      {
        const Shape shape_Z(shape_A1 + shape_B1);
        Value_P Z(new Value(shape_Z, LOC));
        new (&Z->get_ravel(0))  IntCell(0);   // prototype
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if ((l_len_A != 1) && (h_len_B != 1) && (l_len_A != h_len_B))   LENGTH_ERROR;

const APL_Float qct = Workspace::get_CT();

const Shape shape_Z = shape_A1 + shape_B1;

Value_P Z(new Value(shape_Z, LOC));

const Cell * cA = &A->get_ravel(0);

   loop(h, h_len_A)
       {
         loop(l, l_len_B)
             {
               const Cell * cB = &B->get_ravel(l);
               decode(Z->next_ravel(), l_len_A, cA, h_len_B, cB, l_len_B, qct);
             }
         cA += l_len_A;
       }

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
Bif_F12_DECODE::decode(Cell * cZ, ShapeItem len_A, const Cell * cA,
                       ShapeItem len_B, const Cell * cB, ShapeItem dB,
                       double qct)
{
APL_Complex value(0.0, 0.0);
APL_Complex weight(1.0, 0.0);
uint32_t len = len_A == 1 ? len_B : len_A;

   cA += len_A - 1;         // point to the lowest item in A
   cB += dB*(len_B - 1);    // point to the lowest item in B
   loop(l, len)
      {
        value += weight*cB[0].get_complex_value();
        weight *= cA[0].get_complex_value();
        if (len_A != 1)   --cA;
        if (len_B != 1)   cB -= dB;
      }

   if (value.imag() > qct)
      new (cZ)   ComplexCell(value);
   else if (value.imag() < -qct)
      new (cZ)   ComplexCell(value);
   else if (Cell::is_near_int(value.real(), qct))
      new (cZ)   IntCell(value.real(), qct);
   else
      new (cZ)   FloatCell(value.real());
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ENCODE::eval_AB(Value_P A, Value_P B)
{
   if (A->is_scalar())   return Bif_F12_STILE::fun.eval_AB(A, B);

const ShapeItem ec_A = A->element_count();
const ShapeItem ec_B = B->element_count();
   if (ec_A == 0 || ec_B == 0)   // empty A or B
      {
        const Shape shape_Z(A->get_shape() + B->get_shape());
        Value_P Z(new Value(shape_Z, LOC));
        new (&Z->get_ravel(0))  IntCell(0);   // prototype
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

const ShapeItem ah = A->get_shape_item(0);    // first dimension
const ShapeItem al = A->element_count()/ah;   // remaining dimensions

const APL_Float qct = Workspace::get_CT();

const Shape shape_Z = A->get_shape() + B->get_shape();
const ShapeItem dZ = shape_Z.get_volume()/shape_Z.get_shape_item(0);

Value_P Z(new Value(shape_Z, LOC));

   loop(a1, al)
      {
        const Cell * cB = &B->get_ravel(0);

        // find largest Celltype in a1...
        //
        CellType ct_a1 = CT_INT;
        loop(h, ah)
            {
              const CellType ct = A->get_ravel(a1 + h*al).get_cell_type();
              if (ct == CT_INT)            ;
              else if (ct == CT_FLOAT)     { if (ct_a1 == CT_INT)  ct_a1 = ct; }
              else if (ct == CT_COMPLEX)   ct_a1 = CT_COMPLEX;
              else                         DOMAIN_ERROR;
            }

        loop(b, ec_B)
            {
              CellType ct = ct_a1;
              const CellType ct_b = cB->get_cell_type();
              if (ct_b == CT_INT)            ;
              else if (ct_b == CT_FLOAT)     { if (ct == CT_INT)  ct = ct_b; }
              else if (ct_b == CT_COMPLEX)   ct = CT_COMPLEX;
              else                           DOMAIN_ERROR;

              if (ct == CT_INT)
                 encode(dZ, Z->next_ravel(), ah, al,
                        &A->get_ravel(a1), cB++->get_int_value());
              else if (ct == CT_FLOAT)
                 encode(dZ, Z->next_ravel(), ah, al,
                        &A->get_ravel(a1), cB++->get_real_value(), qct);
              else
                 encode(dZ, Z->next_ravel(), ah, al,
                        &A->get_ravel(a1), cB++->get_complex_value(), qct);
            }
       }

   Z->set_default(*B.get());

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
Bif_F12_ENCODE::encode(ShapeItem dZ, Cell * cZ, ShapeItem ah, ShapeItem al,
                       const Cell * cA, APL_Integer b)
{
   // we work downwards from the higher indices...
   //
   cA += ah*al;   // the end of A
   cZ += ah*dZ;   // the end of Z

   loop(l, ah)
       {
         cA -= al;
         cZ -= dZ;

        const IntCell cC(b);
        cC.bif_residue(cZ, cA);

        if (cA->get_int_value() == 0)
           {
             b = 0;
           }
         else
           {
             b -= cZ->get_int_value();
             b /= cA->get_int_value();
           }
       }
}
//-----------------------------------------------------------------------------
void
Bif_F12_ENCODE::encode(ShapeItem dZ, Cell * cZ, ShapeItem ah, ShapeItem al,
                       const Cell * cA, APL_Float b, double qct)
{
   // we work downwards from the higher indices...
   //
   cA += ah*al;   // the end of A
   cZ += ah*dZ;   // the end of Z

   loop(l, ah)
       {
         cA -= al;
         cZ -= dZ;

        const FloatCell cC(b);
        cC.bif_residue(cZ, cA);

        if (cA->is_near_zero(qct))
           {
             b = 0.0;
           }
         else
           {
             b -= cZ->get_real_value();
             b /= cA->get_real_value();
           }

         if (cZ->is_near_zero(qct))
            {
              new (cZ) IntCell(0);
            }
         else
            {
              cZ->demote_float_to_int(qct);
            }
       }
}
//-----------------------------------------------------------------------------
void
Bif_F12_ENCODE::encode(ShapeItem dZ, Cell * cZ, ShapeItem ah, ShapeItem al,
                       const Cell * cA, APL_Complex b, double qct)
{
   // we work downwards from the higher indices...
   //
   cA += ah*al;   // the end of A
   cZ += ah*dZ;   // the end of Z

   loop(l, ah)
       {
         cA -= al;
         cZ -= dZ;

        const ComplexCell cC(b);
        cC.bif_residue(cZ, cA);

        if (cA->is_near_zero(qct))
           {
             b = APL_Complex(0, 0);
           }
         else
           {
             b -= cZ->get_complex_value();
             b /= cA->get_complex_value();
           }

         if (cZ->is_near_zero(qct))
            {
              // demote_complex_to_real doesn't for near-zero numbers
              new (cZ) IntCell(0);
            }
         else
            {
              cZ->demote_complex_to_real(qct);
              cZ->demote_float_to_int(qct);
            }
       }
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ELEMENT::eval_B(Value_P B)
{
   // enlist
const ShapeItem len_Z = B->get_enlist_count();
Value_P Z(new Value(len_Z, LOC));

Cell * z = &Z->get_ravel(0);
   B->enlist(z, B->is_lval());

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_ELEMENT::eval_AB(Value_P A, Value_P B)
{
   // member
   //
const APL_Float qct = Workspace::get_CT();
const ShapeItem len_Z = A->element_count();
const ShapeItem len_B = B->element_count();
Value_P Z(new Value(A->get_shape(), LOC));

   loop(z, len_Z)
       {
         const Cell & cell_A = A->get_ravel(z);
         APL_Integer same = 0;
         loop(b, len_B)
             if (cell_A.equal(B->get_ravel(b), qct))
                {
                  same = 1;
                  break;
                }
         new (&Z->get_ravel(z))   IntCell(same);
       }
   
   Z->set_default_Zero();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_PARTITION::eval_AXB(Value_P A, Value_P X, Value_P B)
{
   return partition(A, B, X->get_single_axis(B->get_rank()));
}
//-----------------------------------------------------------------------------
Token
Bif_F12_PARTITION::eval_B(Value_P B)
{
   if (B->is_simple_scalar())
      {
        Token result(TOK_APL_VALUE1, B->clone(LOC));
        return result;
      }

Value_P Z(new Value(LOC));

   new (&Z->get_ravel(0)) PointerCell(B->clone(LOC));   // calls B->set_nested

   Assert(Z->compute_depth() == (B->compute_depth() + 1));
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_PARTITION::eval_XB(Value_P X, Value_P B)
{
Shape it_shape;
Shape it_weight;
Shape shape_Z;
Shape weight_Z;

const Shape weight_B = B->get_shape().reverse_scan();

   // put the dimensions mentioned in X into it_shape and the
   // others into shape_Z
   //
   Shape shape_X = X->to_shape();

   loop(r, B->get_rank())        // the axes not in shape_X
       {
         if (!shape_X.contains_axis(r))
            {
              shape_Z.add_shape_item(B->get_shape_item(r));
              weight_Z.add_shape_item(weight_B.get_shape_item(r));
            }
       }

   loop(r, shape_X.get_rank())   // the axes in shape_X
       {
         const ShapeItem x_r = shape_X.get_shape_item(r);

         // check that X∈⍳⍴⍴B
         //
         if (x_r < 0)                AXIS_ERROR;
         if (x_r >= B->get_rank())   AXIS_ERROR;

         it_shape.add_shape_item(B->get_shape_item(x_r));
         it_weight.add_shape_item(weight_B.get_shape_item(x_r));
       }

   if (it_shape.get_rank() == 0)   // empty axes
      {
        //  ⊂[⍳0]B   ←→   ⊂¨B
        Token part(TOK_FUN1, &Bif_F12_PARTITION::fun);
        return Bif_OPER1_EACH::fun.eval_LB(part, B);
      }

Value_P Z(new Value(shape_Z, LOC));
   if (Z->is_empty())
      {
         Z->set_default(*B.get());
         return Token(TOK_APL_VALUE1, Z);
      }

   for (ArrayIterator it_Z(shape_Z); !it_Z.done(); ++it_Z)
      {
        const ShapeItem off_Z = it_Z.multiply(weight_Z);   // offset in B

        Value_P vZ(new Value(it_shape, LOC));
        new (Z->next_ravel()) PointerCell(vZ);

        Cell * dst_it = &vZ->get_ravel(0);
        for (ArrayIterator it_it(it_shape); !it_it.done(); ++it_it)
           {
             const ShapeItem off_it = it_it.multiply(it_weight);  // offset in B
             dst_it++->init(B->get_ravel(off_Z + off_it));
           }

        vZ->check_value(LOC);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_PARTITION::partition(Value_P A, Value_P B, Axis axis)
{
   if (A->get_rank() > 1)    RANK_ERROR;
   if (B->get_rank() == 0)   RANK_ERROR;

   if (A->is_scalar())
      {
        APL_Integer val = A->get_ravel(0).get_int_value();
        if (val == 0)
           {
             return Token(TOK_APL_VALUE1, Idx0(LOC));
           }

        return eval_B(B);
      }

   if (A->get_shape_item(0) != B->get_shape_item(axis))   LENGTH_ERROR;

   // determine the length of the partitioned dimension...
   //
ShapeItem len_Z = 0;
   {
     ShapeItem prev = 0;
     loop(l, A->get_shape_item(0))
        {
           const APL_Integer am = A->get_ravel(l).get_int_value();
           if (am < 0)   DOMAIN_ERROR;
           if (am  > prev)   ++len_Z;
           prev = am;
        }
   }

Shape shape_Z(B->get_shape());
   shape_Z.set_shape_item(axis, len_Z);

Value_P Z(new Value(shape_Z, LOC));


   if (Z->is_empty())
      {
        Z->set_default(*B.get());

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

const Shape3 shape_B3(B->get_shape(), axis);
Cell * cZ;

   loop(h, shape_B3.h())
   loop(l, shape_B3.l())
       {
         ShapeItem from = -1;
         ShapeItem prev_am = 0;
         ShapeItem zm = 0;
         cZ = &Z->get_ravel(h*len_Z*shape_B3.l() + l);

         loop(m, shape_B3.m())
             {
               const APL_Integer am = A->get_ravel(m).get_int_value();
               Assert(am >= 0);   // already verified above.

               if (am == 0)   // skip element m (and following)
                  {
                    if (from != -1)   // an old segment is pending
                       {
                         copy_segment(cZ + zm, h, from, m, shape_B3.m(),
                                      l, shape_B3.l(), B);
                         zm += shape_B3.l();
                       }
                    from = -1;
                  }
               else if (am > prev_am)   // new segment
                  {
                    if (from != -1)   // an old segment is pending
                       {
                         copy_segment(cZ + zm, h, from, m,
                                      shape_B3.m(), l, shape_B3.l(), B);
                         zm += shape_B3.l();
                       }
                    from = m;
                  }
               prev_am = am;
             }

         if (from != -1)   // an old segment is pending
            {
              copy_segment(cZ + zm, h, from, shape_B3.m(),
                           shape_B3.m(), l, shape_B3.l(), B);

              // zm += shape_B3.l();   not needed since we are done.
            }
       }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
Bif_F12_PARTITION::copy_segment(Cell * dest, ShapeItem h, ShapeItem m_from,
                                ShapeItem m_to, ShapeItem m_len,
                                ShapeItem l, ShapeItem l_len, Value_P B)
{
   Assert(m_from < m_to);
   Assert(m_to <= m_len);
   Assert(l < l_len);

Value_P V(new Value(m_to - m_from, LOC));

Cell * vv = &V->get_ravel(0);
   for (ShapeItem m = m_from; m < m_to; ++m)
       {
         const Cell & cb = B->get_ravel(l + (m + h*m_len)*l_len);
         vv++->init(cb);
       }

   new (dest) PointerCell(V);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_PICK::disclose(Value_P B, bool rank_tolerant)
{
const ShapeItem len_B = B->element_count();
const Shape it_shape = item_shape(B, rank_tolerant);
const Shape shape_Z = B->get_shape() + it_shape;

Value_P Z(new Value(shape_Z, LOC));

const ShapeItem llen = it_shape.get_volume();
   if (llen == 0)   // empty enclosed value
      {
        const Cell & B0 = B->get_ravel(0);
         if (B0.is_pointer_cell())
            {
              Value_P vB = B0.get_pointer_value();
              Value_P B_proto = vB->prototype(LOC);
              Z->get_ravel(0).init(B_proto->get_ravel(0));
            }
         else
            {
              Z->get_ravel(0).init(B0);
            }

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   loop(h, len_B)
       {
         const Cell & B_item = B->get_ravel(h);
         if (B_item.is_pointer_cell())
            {
              Value_P vB = B_item.get_pointer_value();
              Bif_F12_TAKE::fill(it_shape, &Z->get_ravel(h*llen), vB);
            }
         else if (B_item.is_character_cell())   // simple char scalar
            {
              Z->get_ravel(h*llen).init(B_item);
              for (ShapeItem c = 1; c < llen; ++c)
                  Z->get_ravel(h*llen + c).init(c_filler);
            }
         else   // simple scalar
            {
              Z->get_ravel(h*llen).init(B_item);
              for (ShapeItem c = 1; c < llen; ++c)
                  Z->get_ravel(h*llen + c).init(n_filler);
            }
       }

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_PICK::disclose_with_axis(const Shape & axes_X, Value_P B,
                                 bool rank_tolerant)
{
   // disclose with axis

   // all items of B must have the same rank. it_shape is the smallest
   // shape that can contain each item of B.
   //
const Shape it_shape = item_shape(B, rank_tolerant);

   // the number of items in X must be the number of axes in it_shape
   if (it_shape.get_rank() != axes_X.get_rank())   AXIS_ERROR;

   // distribute shape_B and it_shape into 2 shapes perm and shape_Z.
   // if a dimension is mentioned in axes_X then it goes into shape_Z and
   // otherwise into perm.
   //
Shape perm;
Shape shape_Z;
   {
     ShapeItem B_idx = 0;
     //
     loop(z, B->get_rank() + it_shape.get_rank())
        {
          // check if z is in X, remembering its position if so.
          //
          bool z_in_X = false;
          ShapeItem x_pos = -1;
          loop(x, axes_X.get_rank())
             if (axes_X.get_shape_item(x) == z)
                {
                  z_in_X = true;
                  x_pos = x;
                  break;
                }

          if (z_in_X)   // z is an item dimension: put it in shape_Z
             {
               shape_Z.add_shape_item(it_shape.get_shape_item(x_pos));
             }
          else          // z is a B dimension: put it in perm
             {
               if (B_idx >= B->get_rank())   INDEX_ERROR;
               perm.add_shape_item(z);
               shape_Z.add_shape_item(B->get_shape_item(B_idx++));
             }
        }

     // append X to perm with each X item reduced by the B items before it.
     loop(x, axes_X.get_rank())
        {
          Rank before_x = 0;   // items before X that are not in X
          loop(x1, x - 1)
             if (!axes_X.contains_axis(x1))   ++before_x;

          perm.add_shape_item(axes_X.get_shape_item(x) + before_x);
        }
   }

Value_P Z(new Value(shape_Z, LOC));
   if (Z->is_empty())
      {
         Z->set_default(*B.get());
         return Token(TOK_APL_VALUE1, Z);
      }

   // loop over sources and place them in the result.
   //
PermutedArrayIterator it_Z(shape_Z, perm);

   for (ArrayIterator it_B(B->get_shape()); !it_B.done(); ++it_B)
      {
        const Cell & B_item = B->get_ravel(it_B.get_total());
        const Cell * src = 0;
        if (B_item.is_pointer_cell())
           {
             Value_P vB = B_item.get_pointer_value();
             ArrayIterator vB_it(vB->get_shape());
             for (ArrayIterator it_it(it_shape); !it_it.done(); ++it_it)
                 {
                   if (vB->get_shape().contains(it_it.get_values()))
                      {
                        src = &vB->get_ravel(vB_it.get_total());
                        ++vB_it;
                      }
                   else if (vB->get_ravel(0).is_character_cell())  // char
                        src = &c_filler;
                   else                                   // simple numeric
                        src = &n_filler;

                   Z->get_ravel(it_Z.get_total()).init(*src);
                   ++it_Z;
                 }
           }
        else
           {
             for (ArrayIterator it_it(it_shape); !it_it.done(); ++it_it)
                 {
                   if (it_it.get_total() == 0)   // first element: use B_item
                      {
                        src = &B_item;
                      }
                   else if (B_item.is_character_cell())   // simple char scalar
                      {
                        src = &c_filler;
                      }
                   else                                // simple numeric scalar
                      {
                        src = &n_filler;
                      }

                   Z->get_ravel(it_Z.get_total()).init(*src);
                   ++it_Z;
                 }
            }
      }

   Z->set_default(*B.get());

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Shape
Bif_F12_PICK::item_shape(Value_P B, bool rank_tolerant)
{
   // all items of B are scalars or (nested) arrays of the same rank R.
   // return the shape with rank R and the (per-dimension) max. of
   // each shape item
   //
const ShapeItem len_B = B->nz_element_count();

Shape ret;   // of the first non-scalar in B

   loop(b, len_B)
       {
         const Cell & cell = B->get_ravel(b);
         if (!cell.is_pointer_cell())   continue;   // simple scalar

         Value_P v = cell.get_pointer_value();

         if (ret.get_rank() == 0)   // first non-scalar
            {
              ret = v->get_shape();
              continue;
            }

         // the items of B must have the same rank, unless we are rank_tolerant
         //
         if (ret.get_rank() != v->get_rank())
            {
              if (!rank_tolerant)   RANK_ERROR;

              if (ret.get_rank() < v->get_rank())
                 ret.expand_rank(v->get_rank());

              // if ret.get_rank() > v->get_rank() then we are OK because
              // only the dimensions present in v are expanded below.
            }

         loop(r, v->get_rank())
             {
               if (ret.get_shape_item(r) < v->get_shape_item(r))
                  {
                    ret.set_shape_item(r, v->get_shape_item(r));
                  }
             }
       }

   return ret;
}
//-----------------------------------------------------------------------------
Token
Bif_F12_PICK::eval_AB(Value_P A, Value_P B)
{
   if (A->get_rank() > 1)    RANK_ERROR;

const ShapeItem ec_A = A->element_count();

   // if A is empty, return B
   //
   if (ec_A == 0)   return Token(TOK_APL_VALUE1, B);

const APL_Float qct = Workspace::get_CT();
const APL_Integer qio = Workspace::get_IO();

Value_P Z = pick(&A->get_ravel(0), ec_A, B, qct, qio, B->is_lval());

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Value_P
Bif_F12_PICK::pick(const Cell * cA, ShapeItem len_A, Value_P B,
                   APL_Float qct, APL_Integer qio, bool lval)
{
ShapeItem c = 0;

   if (cA->is_pointer_cell())   // B is an array
      {
        Value_P A = cA->get_pointer_value();
        if (A->get_rank() > 1)   RANK_ERROR;

        const ShapeItem len_A = A->element_count();
        if (B->get_rank() != len_A)   RANK_ERROR;

        const Shape weight = B->get_shape().reverse_scan();
        const Shape A_as_shape(A, qct, qio);

        loop(r, A->element_count())
            {
              const ShapeItem ar = A_as_shape.get_shape_item(r);
              if (ar < 0)                       INDEX_ERROR;
              if (ar >= B->get_shape_item(r))   INDEX_ERROR;
              c += weight.get_shape_item(r) * ar;
            }
      }
   else   // A is a scalar, so B must be a vector.
      {
        if (B->get_rank() != 1)         RANK_ERROR;
        const APL_Integer a = cA->get_near_int(qct) - qio;
        if (a < 0)                       INDEX_ERROR;
        if (a >= B->get_shape_item(0))   INDEX_ERROR;
        c = a;
      }

const Cell * cB = &B->get_ravel(c);

   if (len_A > 1)   // more levels coming.
      {
        if (cB->is_pointer_cell())
           {
             return pick(cA + 1, len_A - 1, cB->get_pointer_value(),
                         qct, qio, lval);
           }

        if (cB->is_lval_cell())
           {
             Assert(lval);
             Cell & cell = *cB->get_lval_value();
             if (!cell.is_pointer_cell())   DOMAIN_ERROR;

             Value_P subval = cell.get_pointer_value();
             Value_P subrefs = subval->get_cellrefs(LOC);
             return pick(cA + 1, len_A - 1, subrefs, qct, qio, lval);
           }

        // at this point the depth implied by A is greater than the
        // depth of B.
        //
        DOMAIN_ERROR;
      }

   // at this point, cB is the cell in B pick'ed by A->
   //
   if (cB->is_pointer_cell())
      {
        Value_P Z = cB->get_pointer_value()->clone(LOC);
        return Z;
      }

   if (cB->is_lval_cell())   // e.g. (A⊃B) ← C
      {
        Assert(lval);

        Cell * cell = cB->get_lval_value();
        if (cell->is_pointer_cell())
           {
             Value_P sub = cell->get_pointer_value();
             return sub->get_cellrefs(LOC);
           }

        Value_P Z(new Value(LOC));
        new (Z->next_ravel())   LvalCell(cell);
        return Z;
      }
   else if (lval)   // e.g. (A⊃C) ← B
      {
        Value_P Z(new Value(LOC));
        new (Z->next_ravel()) LvalCell((Cell *)cB);
        return Z;
      }
   else
      {
        Value_P Z(new Value(LOC));
        Z->next_ravel()->init(*cB);
        return Z;
      }
}
//-----------------------------------------------------------------------------
Token
Bif_F2_INDEX::eval_AB(Value_P A, Value_P B)
{
   if (A->get_rank() > 1)   RANK_ERROR;

const ShapeItem ec_A = A->element_count();
   if (ec_A != B->get_rank())   RANK_ERROR;

   // index_expr is in reverse order!
   //
IndexExpr * index_expr = new IndexExpr(ASS_none, LOC);
   loop(a, ec_A)
      {
         const Cell & cell = A->get_ravel(ec_A - a - 1);
         Value_P val;
         if (cell.is_pointer_cell())
            {
              val = cell.get_pointer_value()->clone(LOC);
              if (val->compute_depth() > 1)   DOMAIN_ERROR;
            }
        else
            {
              const APL_Integer i = cell.get_int_value();
              val = Value_P(new Value(LOC));
              if (i < 0)   DOMAIN_ERROR;

              new (&val->get_ravel(0))   IntCell(i);
            }
        index_expr->add(val);
      }

   index_expr->quad_ct = Workspace::get_CT();
   index_expr->quad_io = Workspace::get_IO();

Value_P Z;
   if (index_expr->value_count() == 1)   // one-dimensional index
      {
        Value_P single_index = index_expr->extract_value(0);
        Z = B->index(single_index);
      }
   else                                  // 0- or multi-dimensional index
      {
        Z = B->index(*index_expr);
      }

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F2_INDEX::eval_AXB(Value_P A, Value_P X, Value_P B)
{
   if (A->get_rank() > 1)   RANK_ERROR;

const Shape axes_present = X->to_shape();

const ShapeItem ec_A = A->element_count();
   if (ec_A != axes_present.get_rank())   RANK_ERROR;
   if (ec_A > B->get_rank())              RANK_ERROR;

   // index_expr is in reverse order!
   //
const APL_Integer qio = Workspace::get_IO();
IndexExpr * index_expr = new IndexExpr(ASS_none, LOC);
   loop(rb, B->get_rank())   index_expr->add(Value_P());
   index_expr->quad_ct = Workspace::get_CT();
   index_expr->quad_io = qio;

   loop(a, ec_A)
      {
         const Rank axis = axes_present.get_shape_item(a);

         const Cell & cell = A->get_ravel(a);
         Value_P val;
         if (cell.is_pointer_cell())
            {
              val = cell.get_pointer_value()->clone(LOC);
              if (val->compute_depth() > 1)   DOMAIN_ERROR;
            }
        else
            {
              const APL_Integer i = cell.get_int_value();
              val = Value_P(new Value(LOC));
              if (i < 0)   DOMAIN_ERROR;

              new (&val->get_ravel(0))   IntCell(i);
            }

        index_expr->set_value(axis, val);
      }

   if (index_expr->value_count() == 1)
      {
        Value_P idx = index_expr->extract_value(0);
        Value_P Z = B->index(idx);

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }
   else
      {
        Value_P Z = B->index(*index_expr);

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }
}
//-----------------------------------------------------------------------------
Value_P
Bif_F12_TAKE::first(Value_P B)
{
   if (B->element_count() == 0)   // empty value: return prototype
      {
        Value_P Z = B->prototype(LOC);
        Z->check_value(LOC);
        return Z;
      }

const Cell & first_B = B->get_ravel(0);
   if (!first_B.is_pointer_cell())   // simple cell
      {
        Value_P Z(new Value(LOC));
        Z->get_ravel(0).init(first_B);
        Z->check_value(LOC);
        return Z;
      }

Value_P v1 = first_B.get_pointer_value();
   if (v1->is_lval())
      {
        Value_P B1(new Value(LOC));
        new (&B1->get_ravel(0))   PointerCell(v1);

        Value_P Z(new Value(LOC));
        new (&Z->get_ravel(0))   LvalCell(&B1->get_ravel(0));

        Z->check_value(LOC);
        return Z;
      }
   else
      {
        const ShapeItem ec = v1->nz_element_count();
        Value_P Z(new Value(v1->get_shape(), LOC));
        loop(e, ec)   Z->get_ravel(e).init(v1->get_ravel(e));

        Z->check_value(LOC);
        return Z;
      }
}
//-----------------------------------------------------------------------------
Token
Bif_F12_TAKE::eval_AXB(Value_P A, Value_P X, Value_P B)
{
   if (X->element_count() == 0)   // no axes
      {
        Token result(TOK_APL_VALUE1, B->clone(LOC));
        return result;
      }

   // A↑[X]B ←→ ⊃[X](⊂A)↑¨⊂[X]B
   //
Value_P cA = Bif_F12_PARTITION::fun.eval_B(A).get_apl_val();           // ⊂A
Value_P cB = Bif_F12_PARTITION::fun.eval_XB(X, B).get_apl_val();       // ⊂[X]B
Token take(TOK_FUN2, &Bif_F12_TAKE::fun);
Value_P cT = Bif_OPER1_EACH::fun.eval_ALB(cA, take, cB).get_apl_val(); // cA↑¨cB

Token result = Bif_F12_PICK::fun.eval_XB(X, cT);
   return result;
}
//-----------------------------------------------------------------------------
Token
Bif_F12_TAKE::eval_AB(Value_P A, Value_P B)
{
   // A must be an integer scalar or vector
   //
   if (A->get_rank() > 1)   RANK_ERROR;

Shape ravel_A(A, Workspace::get_CT(), 0);
Shape ravel_A1(ravel_A);
   if (ravel_A1.get_rank() == 0)   ravel_A1.add_shape_item(1);   // A ← ,A
   if (ravel_A1.get_rank() > MAX_RANK)     LENGTH_ERROR;

   if (B->is_scalar())
      {
        if (ravel_A.get_rank() == 0)
           {
             Token result(TOK_APL_VALUE1, B->clone(LOC));
             return result;
           }

        Cell proto;
        proto.init_type(B->get_ravel(0));
        const Shape ravel_A1_abs = ravel_A1.abs();
        Value_P Z(new Value(ravel_A1_abs, LOC));
        const ShapeItem ec_Z = Z->element_count();   // incl. proto

        loop(z, ec_Z)   Z->next_ravel()->init(proto);
        if (ec_Z == 0)   Z->get_ravel(0).init(proto);

        // compute the position of the non-default element. It is the first
        // item of the corresponfig dimension if the axis is positive, or the
        // last element if the axis is negative.
        //
        ShapeItem pos = 0;
        ShapeItem weight = 1;
        loop(r, ravel_A1.get_rank())
            {
              const ShapeItem len = ravel_A1.get_shape_item(r);
              if (len < 0)   { pos -= (1 + len) * weight;   weight *= -len; }
              else           {                              weight *=  len; }
            }
        Z->get_ravel(pos).init(B->get_ravel(0));

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (ravel_A1.get_rank() != B->get_rank())   RANK_ERROR;

   return do_take(ravel_A, B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_TAKE::do_take(const Shape shape_Zi, Value_P B)
{
Shape shape_Z(shape_Zi);

   loop(r, shape_Zi.get_rank())
       {
         ShapeItem a = shape_Zi.get_shape_item(r);

         if (a < 0)    shape_Z.set_shape_item(r, -a);
       }

Value_P Z(new Value(shape_Z, LOC));

   fill(shape_Zi, &Z->get_ravel(0), B);

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
Bif_F12_TAKE::fill(const Shape & shape_Zi, Cell * Z, Value_P B)
{
   if (shape_Zi.is_empty())   return;

   Assert(shape_Zi.get_rank() == B->get_rank());

   // compute shape_Z = abs(shape_Zi), and Q with Z[i] = B[Q + i].
   //
Shape shape_Z;
Shape Q;

   loop(r, shape_Zi.get_rank())
       {
         const ShapeItem Zi = shape_Zi.get_shape_item(r);

         if (Zi >= 0)
            {
              shape_Z.add_shape_item(Zi);
              Q.add_shape_item(0);
            }
         else
            {
              shape_Z.add_shape_item(-Zi);
              Q.add_shape_item(B->get_shape_item(r) + Zi);
            }
       }

   if (shape_Z.is_empty())   return;

const Shape weight_B = B->get_shape().reverse_scan();

   for (ArrayIterator it(shape_Z); !it.done(); ++it)
      {
        ShapeItem bpos = 0;
        bool fill = false;
        loop(r, shape_Z.get_rank())
            {
              const ShapeItem idx_B = it.get_value(r) + Q.get_shape_item(r);
              if ((idx_B <  0) || (idx_B >= B->get_shape_item(r)))
                 {
                   fill = true;
                   break;
                 }

              bpos += weight_B.get_shape_item(r) * idx_B;
            }

        if (fill)   Z->init_type(B->get_ravel(0));
        else        Z->init(B->get_ravel(bpos));
        ++Z;
      }
}
//-----------------------------------------------------------------------------
Token
Bif_F12_DROP::eval_AB(Value_P A, Value_P B)
{
Shape ravel_A(A, Workspace::get_CT(), 0);
   if (A->get_rank() > 1)   RANK_ERROR;

   if (B->is_scalar())
      {
        // if B is a scalar then the result rank shall be the length of A->
        // the result may be empty (shape 0 0 ... 0) if we drop something
        // or non-empty (shape 1 1 ... 1) if we drop nothing.
        //
        const ShapeItem len_Z = ravel_A.get_volume() ? 0 : 1;

        Shape shape_Z;
        loop(r, ravel_A.get_rank())   shape_Z.add_shape_item(len_Z);

        Value_P Z(new Value(shape_Z, LOC));

        Z->get_ravel(0).init(B->get_ravel(0));
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (ravel_A.get_rank() == 0)   ravel_A.add_shape_item(1);   // A = ,A

   if (ravel_A.get_rank() != B->get_rank())   RANK_ERROR;

   loop(r, ravel_A.get_rank())
       {
         const APL_Integer a = ravel_A.get_shape_item(r);
         const ShapeItem amax = B->get_shape_item(r);

         if      (a >= amax)  ravel_A.set_shape_item(r, 0);
         else if (a >= 0)     ravel_A.set_shape_item(r, a - amax);
         else if (a > -amax)  ravel_A.set_shape_item(r, amax + a);
         else                 ravel_A.set_shape_item(r, 0);
       }

   return Bif_F12_TAKE::do_take(ravel_A, B);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_DROP::eval_AXB(Value_P A, Value_P X, Value_P B)
{
   if (X->element_count() == 0)   // no axes
      {
        Token result(TOK_APL_VALUE1, B->clone(LOC));
        return result;
      }

   if (X->get_rank() > 1)    INDEX_ERROR;

const uint64_t len_X = X->element_count();
   if (len_X > MAX_RANK)     INDEX_ERROR;
   if (len_X == 0)           INDEX_ERROR;

   if (A->get_rank() > 1)    RANK_ERROR;

uint64_t len_A = A->element_count();
   if (len_A != len_X)   LENGTH_ERROR;

const APL_Float   qct = Workspace::get_CT();
const APL_Integer qio = Workspace::get_IO();

   // init ravel_A = shape_B and seen.
   //
Shape ravel_A(B->get_shape());
bool seen[MAX_RANK];
   loop(r, B->get_rank())   seen[r] = false;

   loop(r, len_X)
       {
         const APL_Integer a = A->get_ravel(r).get_near_int(qct);
         const uint32_t x = X->get_ravel(r).get_near_int(qct) - qio;

         if (x >= B->get_rank())   INDEX_ERROR;
         if (seen[x])              INDEX_ERROR;
         seen[x] = true;

         const int32_t amax = B->get_shape_item(x);
         if      (a >= amax)   ravel_A.set_shape_item(x, 0);
         else if (a >= 0)      ravel_A.set_shape_item(x, a - amax);
         else if (a > -amax)   ravel_A.set_shape_item(x, amax + a);
         else                  ravel_A.set_shape_item(x, 0);
       }

   return Bif_F12_TAKE::do_take(ravel_A, B);
}
//-----------------------------------------------------------------------------
Token
Bif_SORT::sort(Value_P B, bool ascending)
{
   if (B->is_scalar())   RANK_ERROR;

const ShapeItem len_BZ = B->get_shape_item(0);
   if (len_BZ == 0)   return Token(TOK_APL_VALUE1, Idx0(LOC));

const ShapeItem comp_len = B->element_count()/len_BZ;
DynArray(const Cell *, array, len_BZ);
   loop(bz, len_BZ)   array[bz] = &B->get_ravel(bz*comp_len);

   Cell::heapsort(&array[0], len_BZ, ascending, &comp_len, &Cell::greater_vec);

Value_P Z(new Value(len_BZ, LOC));

const APL_Integer qio = Workspace::get_IO();
const Cell * base = &B->get_ravel(0);
   loop(bz, len_BZ)
       new (Z->next_ravel())   IntCell(qio + (array[bz] - base)/comp_len);

   Z->set_default_Zero();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_SORT::sort_collating(Value_P A, Value_P B, bool ascending)
{
   if (A->is_scalar())   RANK_ERROR;
   if (A->NOTCHAR())     DOMAIN_ERROR;

const APL_Integer qio = Workspace::get_IO();
   if (B->NOTCHAR())     DOMAIN_ERROR;
   if (B->is_scalar())   return Token(TOK_APL_VALUE1, IntScalar(qio, LOC));

const ShapeItem len_BZ = B->get_shape_item(0);
   if (len_BZ == 0)   return Token(TOK_APL_VALUE1, Idx0(LOC));
   if (len_BZ == 1)   return Token(TOK_APL_VALUE1, IntScalar(qio, LOC));

Value_P B1(new Value(B->get_shape(), LOC));
const ShapeItem ec_B = B->element_count();
const ShapeItem comp_len = ec_B/len_BZ;
CollatingCache cc_cache(A->get_rank(), comp_len);
   loop(b, ec_B)
      {
        const Unicode uni = B->get_ravel(b).get_char_value();
        const ShapeItem b1 = collating_cache(uni, A, cc_cache);
        new (&B1->get_ravel(b)) IntCell(b1);
      }

DynArray(const Cell *, array, len_BZ);
   loop(bz, len_BZ)   array[bz] = &B1->get_ravel(bz*comp_len);

   Cell::heapsort(&array[0], len_BZ, ascending, &cc_cache,
                  &CollatingCache::greater_vec);

Value_P Z(new Value(len_BZ, LOC));

const Cell * base = &B1->get_ravel(0);
   loop(bz, len_BZ)
       new (&Z->get_ravel(bz)) IntCell(qio + (array[bz] - base)/comp_len);

   Z->set_default_Zero();

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
ShapeItem
Bif_SORT::collating_cache(Unicode uni, Value_P A, CollatingCache & cache)
{
   // first check if uni is already in the cache...
   //
   loop(s, cache.size())
      {
        if (uni == cache[s].ce_char)   return s;
      }

   // uni is not in the cache. See if it is in the collating sequence.
   //
const ShapeItem ec_A = A->element_count();
CollatingCacheEntry entry(uni, A->get_shape());
   loop(a, ec_A)
      {
        if (uni != A->get_ravel(a).get_char_value())   continue;

        ShapeItem aq = a;
        loop(r, entry.ce_shape.get_rank())
           {
             const Rank axis = entry.ce_shape.get_rank() - r - 1;
             const ShapeItem ar = aq % A->get_shape_item(axis);
             if (entry.ce_shape.get_shape_item(axis) > ar)
                entry.ce_shape.set_shape_item(axis, ar);
             aq /= A->get_shape_item(axis);
           }
      }

   cache.push_back(entry);
   return cache.size() - 1;
}
//=============================================================================
Token
Bif_F12_EQUIV::eval_B(Value_P B)
{
const Depth depth = B->compute_depth();

Value_P Z(new Value(LOC));

   new (&Z->get_ravel(0)) IntCell(depth);

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_EQUIV::eval_AB(Value_P A, Value_P B)
{
   // match
   //

const APL_Float qct = Workspace::get_CT();
const ShapeItem count = A->nz_element_count();  // compare at least prototype

   if (!A->same_shape(*B))   return Token(TOK_APL_VALUE1, IntScalar(0, LOC));   // no match

   loop(c, count)
       if (!A->get_ravel(c).equal(B->get_ravel(c), qct))
          {
            return Token(TOK_APL_VALUE1, IntScalar(0, LOC));   // no match
          }

   return Token(TOK_APL_VALUE1, IntScalar(1, LOC));   // match
}
//=============================================================================
Token
Bif_F12_NEQUIV::eval_B(Value_P B)
{
   // Tally
   //
const ShapeItem len = B->is_scalar() ? 1 : B->get_shape().get_shape_item(0);

   return Token(TOK_APL_VALUE1, IntScalar(len, LOC));   // match
}
//-----------------------------------------------------------------------------
Token
Bif_F12_NEQUIV::eval_AB(Value_P A, Value_P B)
{
   // match
   //

const APL_Float qct = Workspace::get_CT();
const ShapeItem count = A->nz_element_count();  // compare at least prototype

   if (!A->same_shape(*B))   return Token(TOK_APL_VALUE1, IntScalar(1, LOC));   // no match

   loop(c, count)
       if (!A->get_ravel(c).equal(B->get_ravel(c), qct))
          {
            return Token(TOK_APL_VALUE1, IntScalar(1, LOC));   // no match
          }

   return Token(TOK_APL_VALUE1, IntScalar(0, LOC));   // match
}
//=============================================================================
Token
Bif_F1_EXECUTE::eval_B(Value_P B)
{
   if (B->get_rank() > 1)   RANK_ERROR;

UCS_string statement(*B.get());

   if (statement.size() == 0)   return Token(TOK_NO_VALUE);

   return execute_statement(statement);
}
//-----------------------------------------------------------------------------
Token
Bif_F1_EXECUTE::execute_statement(UCS_string & statement)
{
   statement.remove_leading_and_trailing_whitespaces();
   if (statement[0] == UNI_ASCII_R_PARENT ||
       statement[0] == UNI_ASCII_R_BRACK)
      {
        UTF8_ostream out;
        const bool valid = Command::do_APL_command(out, statement);
        if (!valid)   throw_apl_error(E_INCORRECT_COMMAND, LOC);

        UTF8_string result_utf8 = out.get_data();
        if (result_utf8.last() != UNI_ASCII_LF)
           result_utf8.append(UNI_ASCII_LF);

        vector<ShapeItem> line_starts;
        line_starts.push_back(0);
        loop(r, result_utf8.size())
           {
             if (result_utf8[r] == UNI_ASCII_LF)   line_starts.push_back(r + 1);
           }

        Value_P Z(new Value((ShapeItem)line_starts.size() - 1, LOC));
        loop(l, line_starts.size() - 1)
           {
             ShapeItem len;
             if (l < line_starts.size() - 1)
                len = line_starts[l + 1] - line_starts[l] - 1;
             else
                len = result_utf8.size() - line_starts[l];

             UTF8_string line_utf8(&result_utf8[line_starts[l]], len);
             UCS_string line_ucs(line_utf8);
             Value_P ZZ(new Value(line_ucs, LOC));
             new (Z->next_ravel())   PointerCell(ZZ);
           }

        return Token(TOK_APL_VALUE1, Z);
      }

ExecuteList * fun = ExecuteList::fix(statement.no_pad(), LOC);
   if (fun == 0)   SYNTAX_ERROR;

   Log(LOG_UserFunction__execute)   fun->print(CERR);

   Workspace::push_SI(fun, LOC);

   Log(LOG_StateIndicator__push_pop)
      {
        Workspace::SI_top()->info(CERR, LOC);
      }

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_UNION::eval_AB(Value_P A, Value_P B)
{
Value_P unique_A = do_unique(A, true);
Value_P unique_B = do_unique(B, true);

   // count equal elements
   //
ShapeItem eq_count = 0;
const Cell * cA = &unique_A->get_ravel(0);
const Cell * cB = &unique_B->get_ravel(0);
const Cell * endA = cA + unique_A->element_count();
const Cell * endB = cB + unique_B->element_count();

   while (cA < endA && cB < endB)   // both have elements
       {
         const Comp_result comp = cA->compare(*cB);
         if      (comp == COMP_LT)   ++cA;    // cA < cB
         else if (comp == COMP_GT)   ++cB;    // cA > cB
         else                                 // cA == cB
            {
              ++cA;
              ++cB;
              ++eq_count;
            }
       }

Value_P Z(new Value(unique_A->nz_element_count()
                  + unique_B->nz_element_count() - eq_count, LOC));
   cA = &unique_A->get_ravel(0);
   cB = &unique_B->get_ravel(0);
   while (cA < endA && cB < endB)   // both have elements
       {
         const Comp_result comp = cA->compare(*cB);
         if      (comp == COMP_LT)   Z->next_ravel()->init(*cA++);
         else if (comp == COMP_GT)   Z->next_ravel()->init(*cB++);
         else                             
            {
              Z->next_ravel()->init(*cA++);
              ++cB;
            }
       }

   // the remaining elements in the larger vsalue...
   //
   while (cA < endA)   Z->next_ravel()->init(*cA++);
   while (cB < endB)   Z->next_ravel()->init(*cB++);

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Value_P
Bif_F12_UNION::do_unique(Value_P B, bool sorted)
{
   if (B->get_rank() > 1)   RANK_ERROR;

const ShapeItem ec_B = B->element_count();
   if (ec_B <= 1)  // 0 or 1 element: return B
      {
        if (B->is_temp())
           {
              Log(LOG_optimization) CERR << "optimizing ∪B" << endl;
              return B;
           }

        Value_P Z = B->clone(LOC);
        Z->check_value(LOC);
        return Z;
      }

Token B1_tok = Bif_F12_SORT_ASC::fun.eval_B(B);
   if (B1_tok.get_Class() != TC_VALUE)   return B1_tok.get_apl_val();

const APL_Integer qio = Workspace::get_IO();
const APL_Float qct = Workspace::get_CT();
Value_P B1 = B1_tok.get_apl_val();

   // compute result size
   //
ShapeItem ec_Z = 0;   // first element is always in the result.
const Cell * last = &B->get_ravel(B1->get_ravel(0).get_int_value() - qio);
   loop(j, ec_B)
      {
        const APL_Integer idx = B1->get_ravel(j).get_int_value();
        const Cell * cj = &B->get_ravel(idx - qio);
        const bool new_element = (j == 0) || !last->equal(*cj, qct);
        if (new_element)
           {
             ++ec_Z;
             last = cj;
           }
      }

Value_P Z1(new Value(ec_Z, LOC));

   loop(j, ec_B)
      {
        const APL_Integer idx = B1->get_ravel(j).get_int_value();
        const Cell * cj = &B->get_ravel(idx - qio);
        const bool new_element = (j == 0) || !last->equal(*cj, qct);
        if (new_element)
           {
             new (Z1->next_ravel()) IntCell(idx);
             last = cj;
           }
      }

   // at this point , B[Z1] is the set of unique elements, but sorted. 
   // We may need to sort Z1 in order to get the original order.
   //
   if (sorted)
      {
         Value_P Z4 = B->index(Z1);
         return Z4;
      }

Token Z2_tok = Bif_F12_SORT_ASC::fun.eval_B(Z1);
   Assert(Z2_tok.get_Class() == TC_VALUE);
Value_P Z2 = Z2_tok.get_apl_val();

Value_P Z3 = Z1->index(Z2);
Value_P Z4 = B->index(Z3);

   Z4->check_value(LOC);
   return Z4;
}
//-----------------------------------------------------------------------------
Token
Bif_F2_INTER::eval_AB(Value_P A, Value_P B)
{
Value_P unique_A = Bif_F12_UNION::do_unique(A, true);
Value_P unique_B = Bif_F12_UNION::do_unique(B, true);

   // count equal elements
   //
ShapeItem eq_count = 0;
const Cell * cA = &unique_A->get_ravel(0);
const Cell * cB = &unique_B->get_ravel(0);
const Cell * endA = cA + unique_A->element_count();
const Cell * endB = cB + unique_B->element_count();

   while (cA < endA && cB < endB)   // both have elements
       {
         const Comp_result comp = cA->compare(*cB);
         if      (comp == COMP_LT)   ++cA;    // cA < cB
         else if (comp == COMP_GT)   ++cB;    // cA > cB
         else                                 // cA == cB
            {
              ++cA;
              ++cB;
              ++eq_count;
            }
       }

Value_P Z(new Value(eq_count, LOC));
   cA = &unique_A->get_ravel(0);
   cB = &unique_B->get_ravel(0);
   while (cA < endA && cB < endB)   // both have elements
       {
         const Comp_result comp = cA->compare(*cB);
         if      (comp == COMP_LT)   ++cA;
         else if (comp == COMP_GT)   ++cB;
         else                             
            {
              Z->next_ravel()->init(*cA++);
              ++cB;
            }
       }

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================

