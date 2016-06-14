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

#include "Bif_OPER1_REDUCE.hh"
#include "Macro.hh"
#include "Workspace.hh"

Bif_OPER1_REDUCE    Bif_OPER1_REDUCE ::_fun;
Bif_OPER1_REDUCE1   Bif_OPER1_REDUCE1::_fun;

Bif_OPER1_REDUCE  * Bif_OPER1_REDUCE ::fun = &Bif_OPER1_REDUCE ::_fun;
Bif_OPER1_REDUCE1 * Bif_OPER1_REDUCE1::fun = &Bif_OPER1_REDUCE1::_fun;

//-----------------------------------------------------------------------------
Token
Bif_REDUCE::replicate(Value_P A, Value_P B, Axis axis)
{
   // turn scalar B into ,B
   //
Shape shape_B = B->get_shape();
   if (shape_B.get_rank() == 0)
      {
         shape_B.add_shape_item(1);
         axis = 0;
      }

   if (A->get_rank() > 1)             RANK_ERROR;
   if (axis >= shape_B.get_rank())    INDEX_ERROR;

const ShapeItem len_B = shape_B.get_shape_item(axis);
ShapeItem len_A = A->element_count();
ShapeItem len_Z = 0;
vector<ShapeItem> rep_counts;
   rep_counts.reserve(len_B);
   if (len_A == 1)   // single a -> a a ... a (len_B times)
      {
        len_A = len_B;
        APL_Integer rep_A = A->get_ravel(0).get_near_int();
        loop(a, len_A)   rep_counts.push_back(rep_A);
        if (rep_A > 0)        len_Z =  rep_A*len_B;
        else if (rep_A < 0)   len_Z = -rep_A*len_B;
      }
   else
      {
        ShapeItem geq_A = 0;   // number of items >= 0 in A
        loop(a, len_A)
           {
             APL_Integer rep_A = A->get_ravel(a).get_near_int();
             rep_counts.push_back(rep_A);
             if (rep_A > 0)        { len_Z += rep_A;   ++geq_A; }
             else if (rep_A < 0)   len_Z -= rep_A;
             else                  ++geq_A;
           }

        if (len_B != 1 && geq_A != len_B)   LENGTH_ERROR;
      }

Shape shape_Z(shape_B);
   shape_Z.set_shape_item(axis, len_Z);

Value_P Z(shape_Z, LOC);

const Shape3 shape_B3(shape_B, axis);

   loop(h, shape_B3.h())
      {
        ShapeItem bm = 0;
        loop(m, rep_counts.size())
           {
             const ShapeItem rep = rep_counts[m];

             if (rep >= 0)   // copy l*rep items
                {
                  loop(r, rep)
                  loop(l, shape_B3.l())
                     {
                       const ShapeItem src = shape_B3.hml(h, bm, l);
                       Z->next_ravel()->init(B->get_ravel(src), Z.getref(),LOC);
                     }
                  if (shape_B3.m() > 1)   ++bm;
                }
             else  // init l*-rep items with the fill item
                {
                  loop(r, -rep)
                  loop(l, shape_B3.l())
                     {
                       const ShapeItem src = shape_B3.hml(h, 0, l);
                       Z->next_ravel()
                        ->init_type(B->get_ravel(src), Z.getref(), LOC);
                     }

                  // cB is not incremented when fill item is used.
                }
           }
      }

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_REDUCE::reduce(Token & _LO, Value_P B, Axis axis)
{
Function * LO = _LO.get_function();
   Assert1(LO);
   if (!LO->has_result())            DOMAIN_ERROR;
   if (LO->get_fun_valence() != 2)   SYNTAX_ERROR;

   // if B is a scalar, then Z is B.
   //
   if (B->get_rank() == 0)      return Token(TOK_APL_VALUE1, B->clone(LOC));

   if (axis >= B->get_rank())   AXIS_ERROR;

const ShapeItem m_len = B->get_shape_item(axis);

const Shape shape_Z = B->get_shape().without_axis(axis);

   if (m_len == 0)   // apply the identity function
      {
        return LO->eval_identity_fun(B, axis);
      }

   if (m_len == 1)   return Bif_F12_RHO::do_reshape(shape_Z, *B);

   // non-trivial reduce (len > 1)
   //
   if (LO->may_push_SI())   // user defined LO
      {
        Value_P X = IntScalar(axis + Workspace::get_IO(), LOC);
        return Macro::Z__LO_REDUCE_X_B->eval_LXB(_LO, X, B);
      }

const Shape3 B3(B->get_shape(), axis);
const Shape3 Z3(B3.h(), 1, B3.l());
   return do_reduce(shape_Z, Z3, B3.m(), LO, axis, B, B->get_shape_item(axis));
}
//-----------------------------------------------------------------------------
Token
Bif_REDUCE::reduce_n_wise(Value_P A, Token & _LO, Value_P B, Axis axis)
{
Function * LO = _LO.get_function();
   Assert(LO);
   if (!LO->has_result())   DOMAIN_ERROR;

   if (A->element_count() != 1)   LENGTH_ERROR;
const APL_Integer A0 = A->get_ravel(0).get_int_value();
const int n_wise = A0 < 0 ? -A0 : A0;   // the number of items (= M1 in iso)

   if (B->is_scalar())
      {
        if (n_wise > 2)    DOMAIN_ERROR;
        if (n_wise == 0)
           {
              Token ident = LO->eval_identity_fun(B, axis);
              Value_P val(2, LOC);
              val->next_ravel()->init(ident.get_apl_val()->get_ravel(0),
                                      val.getref(), LOC);
              val->next_ravel()->init(ident.get_apl_val()->get_ravel(0),
                                      val.getref(), LOC);
              return Token(TOK_APL_VALUE1, val);
           }

        // n_wise is 1 or 2: return (2 - n_wise) ⍴ B
        //
        Shape sh;
        if (n_wise == 2)   sh.add_shape_item(0);
        return Bif_F12_RHO::do_reshape(sh, B.getref());
      }
   else
      {
        if (n_wise > (1 + B->get_shape_item(axis)))   DOMAIN_ERROR;
      }

   Assert1(LO);

   if (B->get_rank() == 0)      return Token(TOK_APL_VALUE1, B->clone(LOC));

   if (axis >= B->get_rank())   AXIS_ERROR;

   if (n_wise == 0)   // apply the identity function
      {
        Shape shape_B1 = B->get_shape().insert_axis(axis, 0);
        shape_B1.increment_shape_item(axis + 1);
        Value_P val(shape_B1, LOC);
        val->get_ravel(0).init(B->get_ravel(0), val.getref(), LOC); // prototype

        Token result = LO->eval_identity_fun(val, axis);
        return result;
      }

   if (n_wise == (1 + B->get_shape_item(axis)))   // empty result
      {
        // the examples in ISO/IEC 13751 (2000) p. 116, as well as lrm,
        // suggest an empty result, while the algorithm on pp. 115/116 seems
        // not to work for n_wise == (1 + B->get_shape_item(axis)).
        // We follow the examples and lrm.
        //
        Shape shape_Z = B->get_shape();
        shape_Z.set_shape_item(axis, 0);
        Value_P Z(shape_Z, LOC);
        Z->get_ravel(0).init(B->get_ravel(0), Z.getref(), LOC);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

Shape shape_Z(B->get_shape());
   shape_Z.set_shape_item(axis, shape_Z.get_shape_item(axis) - n_wise + 1);

   if (n_wise == 1)   return Bif_F12_RHO::do_reshape(shape_Z, *B);

   if (LO->may_push_SI())   // user defined LO
      {
        Value_P X = IntScalar(axis + Workspace::get_IO(), LOC);
        if (A0 < 0)  return Macro::Z__nA_LO_REDUCE_X_B->eval_ALXB(A, _LO, X, B);
        else         return Macro::Z__pA_LO_REDUCE_X_B->eval_ALXB(A, _LO, X, B);
      }

const Shape3 Z3(shape_Z, axis);
   return do_reduce(shape_Z, Z3, A0, LO, axis, B, B->get_shape_item(axis));
}
//-----------------------------------------------------------------------------
Token
Bif_REDUCE::do_reduce(const Shape & shape_Z, const Shape3 & Z3, ShapeItem nwise,
                      Function * LO, Axis axis, Value_P B, ShapeItem bm)
{
   if (shape_Z.is_empty())   return LO->eval_identity_fun(B, axis);

Value_P Z(shape_Z, LOC);

const ShapeItem len_L   = Z3.get_last_shape_item();
const ShapeItem len_BML = bm * len_L;
const ShapeItem len_ZM  = Z3.m();

ShapeItem todo_B = 0;   // new beam

prim_f2 scalar_LO = LO->get_scalar_f2();

ShapeItem b = 0;
   for (ShapeItem z = -1; todo_B || z < (Z->element_count() - 1);)
      {
        if (todo_B == 0)   // new beam
           {
             ++z;
             
             const ShapeItem z_L =  z % len_L;
             const ShapeItem z_M = (z / len_L) % len_ZM;
             const ShapeItem z_H =  z / (len_L * len_ZM);

             if      (nwise == -1)   todo_B = 1 + z_M;        // scan
             else if (nwise < -1)    todo_B = - nwise;
             else                    todo_B =   nwise;

             b = z_L + z_H * len_BML          // start of row in B
                    + (todo_B - 1) * len_L;   // end of beam in B

             // for reverse (i,e. nwise < -1) reduction direction go to the
             // start of the beam instead of the end of the beam.
             //
             if (nwise < -1)   b += (nwise + 1) * len_L;

            if (nwise != -1)   b += z_M * len_L; // start of beam

             // Z[z] ? B[b]
             //
             Z->next_ravel()->init(B->get_ravel(b), Z.getref(), LOC);
           }

        // update position and check for end of beam
        //
        Assert(todo_B > 0);
        if (nwise < -1)   b += len_L;   // move  forward
        else              b -= len_L;   // move backward
        --todo_B;
        if (todo_B > 0)
           {
           // one reduction step (one call of LO)
           //
           if (scalar_LO                        &&
               Z->get_ravel(z).is_simple_cell() &&
               B->get_ravel(b).is_simple_cell())
              {
                Cell * cZ = &Z->get_ravel(z);
                ErrorCode ec = (cZ->*scalar_LO)(cZ, &B->get_ravel(b));
                if (ec == E_NO_ERROR)   continue;
                throw_apl_error(ec, LOC);
              }
           else
              {
                Value_P LO_A = B->get_ravel(b).to_value(LOC);
                Value_P LO_B = Z->get_ravel(z).to_value(LOC);
                Token result = LO->eval_AB(LO_A, LO_B);
                Z->get_ravel(z).release(LOC);

                if (result.get_Class() == TC_VALUE)
                   {
                     Value_P ZZ = result.get_apl_val();
                     Z->get_ravel(z).init_from_value(ZZ, Z.getref(), LOC);
                     continue;
                   }

                if (result.get_tag() == TOK_ERROR)   return result;

                Q1(result);   FIXME;
              }
           }
      }

   Z->set_default(*B.get());

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_REDUCE::eval_AXB(Value_P A, Value_P X, Value_P B)
{
const Rank axis = Value::get_single_axis(X.get(), B->get_rank());
   return replicate(A, B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_REDUCE::eval_LXB(Token & _LO, Value_P X, Value_P B)
{
const Rank axis = Value::get_single_axis(X.get(), B->get_rank());
   return reduce(_LO, B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_REDUCE::eval_ALXB(Value_P A, Token & _LO, Value_P X, Value_P B)
{
const Rank axis = Value::get_single_axis(X.get(), B->get_rank());
   return reduce_n_wise(A, _LO, B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_REDUCE1::eval_AXB(Value_P A,
                            Value_P X, Value_P B)
{
const Rank axis = Value::get_single_axis(X.get(), B->get_rank());

   return replicate(A, B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_REDUCE1::eval_LXB(Token & LO, Value_P X, Value_P B)
{
const Rank axis = Value::get_single_axis(X.get(), B->get_rank());
   return reduce(LO, B, axis);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_REDUCE1::eval_ALXB(Value_P A, Token & LO, Value_P X, Value_P B)
{
const Rank axis = Value::get_single_axis(X.get(), B->get_rank());
   return reduce_n_wise(A, LO, B, axis);
}
//-----------------------------------------------------------------------------
