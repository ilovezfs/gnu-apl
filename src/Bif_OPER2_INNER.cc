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

#include "Bif_OPER2_INNER.hh"
#include "Bif_OPER1_REDUCE.hh"
#include "Macro.hh"
#include "PointerCell.hh"
#include "Workspace.hh"

Bif_OPER2_INNER   Bif_OPER2_INNER::_fun;
Bif_OPER2_INNER * Bif_OPER2_INNER::fun = &Bif_OPER2_INNER::_fun;

Bif_OPER2_INNER::PJob_product Bif_OPER2_INNER::job;

//-----------------------------------------------------------------------------
Token
Bif_OPER2_INNER::eval_ALRB(Value_P A, Token & _LO, Token & _RO, Value_P B)
{
   if (!_LO.is_function())    SYNTAX_ERROR;
   if (!_RO.is_function())    SYNTAX_ERROR;

Function * LO = _LO.get_function();
Function * RO = _RO.get_function();
   Assert1(LO);
   Assert1(RO);

   if (!RO->has_result())   DOMAIN_ERROR;
   if (!LO->has_result())   DOMAIN_ERROR;

Shape shape_A1;
ShapeItem len_A = 1;
   if (!A->is_scalar())
      {
        len_A = A->get_last_shape_item();
        shape_A1 = A->get_shape().without_axis(A->get_rank() - 1);
      }

Shape shape_B1;
ShapeItem len_B = 1;
   if (!B->is_scalar())
      {
        len_B = B->get_shape_item(0);
        shape_B1 = B->get_shape().without_axis(0);
      }

   // we do not check len_A == len_B here, since a non-scalar LO may
   // accept different lengths of its left and right arguments

const ShapeItem items_A = shape_A1.get_volume();
const ShapeItem items_B = shape_B1.get_volume();

   if (items_A == 0 || items_B == 0)   // empty result
      {
        // the outer product portion of LO.RO is empty.
        // Apply the fill function of RO
        //
        const Shape shape_Z = shape_A1 + shape_B1;
        return fill(shape_Z, A, RO, B, LOC);
      }

   if (LO->may_push_SI() || RO->may_push_SI())   // user defined LO or RO
      {
        return Macro::Z__A_LO_INNER_RO_B->eval_ALRB(A, _LO, _RO, B);
      }

Value_P Z(shape_A1 + shape_B1, LOC);

   // an important (and the most likely) special case is LO and RO being scalar
   // functions. This case can be implemented in a far simpler fashion than
   // the general case.
   //
   job.LO = LO->get_scalar_f2();
   job.RO = RO->get_scalar_f2();
   if (job.LO && job.RO && A->is_simple() && B->is_simple() && len_A)
      {
        job.incA   = (A->element_count() == 1) ? 0 : 1;
        job.incB   = (B->element_count() == 1) ? 0 : 1;

   // len_A must be len_B, unless at least one length is 1
   //
        if (len_A != len_B && job.incA && job.incB)   LENGTH_ERROR;

        job.cZ     = &Z->get_ravel(0);
        job.cA     = &A->get_ravel(0);
        job.ZAh    = items_A;
        job.LO_len = A->is_scalar() ? len_B : len_A;
        job.cB     = &B->get_ravel(0);
        job.ZBl    = items_B;
        job.ec     = E_NO_ERROR;

        scalar_inner_product();
        if (job.ec != E_NO_ERROR)   throw_apl_error(job.ec, LOC);

        Z->set_default(*B.get());
 
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

const bool A_enclosed = A->get_rank() > 1;
const bool B_enclosed = B->get_rank() > 1;

   // enclose last axis of A if necessary
   //
   if (A_enclosed)
      {
        const Shape last_axis(A->get_rank() - 1);
        A = Bif_F12_PARTITION::enclose_with_axes(last_axis, A);
      }
   
   // enclose first axis of B if necessary
   //
   if (B_enclosed)
      {
        const Shape first_axis(0);
        B = Bif_F12_PARTITION::enclose_with_axes(first_axis, B);
      }
   
   loop (z, items_A * items_B)
      {
        const ShapeItem a = z / items_B;
        const ShapeItem b = z % items_B;

        Value_P RO_A(A, LOC);
        if (A_enclosed)   RO_A = A->get_ravel(a).get_pointer_value();

        Value_P RO_B(B, LOC);
        if (B_enclosed)   RO_B = B->get_ravel(b).get_pointer_value();

        const Token T1 = RO->eval_AB(RO_A, RO_B);

        if (T1.get_tag() == TOK_ERROR)   return T1;

        Value_P A_RO_B = T1.get_apl_val();
        new (Z->next_ravel()) PointerCell(A_RO_B, Z.getref());

        // at this point Z[z] is the result of a built-in function RO.
        // Compute LO/Z[z].
        Value_P ZZ = Z->get_ravel(z).get_pointer_value();
        const Token T2 = Bif_OPER1_REDUCE::fun->eval_LB(_LO, ZZ);
        Z->get_ravel(z).release(LOC);

        if (T2.get_tag() == TOK_ERROR)   return T2;

        Z->get_ravel(z).init_from_value(T2.get_apl_val(), Z.getref(), LOC);
      }

   Z->set_default(*B.get());
 
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
Bif_OPER2_INNER::scalar_inner_product() const
{
#ifdef PERFORMANCE_COUNTERS_WANTED
#ifdef HAVE_RDTSC
const uint64_t start_1 = cycle_counter();
#endif
#endif

  // the empty cases have been ruled out already in inner_product()

const ShapeItem Z_len = job.ZAh * job.ZBl;
   job.ec = E_NO_ERROR;

#if PARALLEL_ENABLED
   if (  Parallel::run_parallel
      && Thread_context::get_active_core_count() > 1
      && Z_len > get_dyadic_threshold())
      {
        job.cores = Thread_context::get_active_core_count();
        Thread_context::do_work = PF_scalar_inner_product;
        Thread_context::M_fork("scalar_inner_product");   // start pool
        PF_scalar_inner_product(Thread_context::get_master());
        Thread_context::M_join();
      }
   else
#endif // PARALLEL_ENABLED
      {
        job.cores = CCNT_1;
        PF_scalar_inner_product(Thread_context::get_master());
      }

#ifdef PERFORMANCE_COUNTERS_WANTED
#ifdef HAVE_RDTSC
const uint64_t end_1 = cycle_counter();
   Performance::fs_OPER2_INNER_AB.add_sample(end_1 - start_1, Z_len);
#endif
#endif
}
//-----------------------------------------------------------------------------
void
Bif_OPER2_INNER::PF_scalar_inner_product(Thread_context & tctx)
{
const ShapeItem Z_len = job.ZAh * job.ZBl;

const ShapeItem slice_len = (Z_len + job.cores - 1)/job.cores;
ShapeItem z = tctx.get_N() * slice_len;
ShapeItem end_z = z + slice_len;
   if (end_z > Z_len)   end_z = Z_len;

Cell product;   // the result of LO, e.g. of × in +/×

   for (; z < end_z; ++z)
       {
        const ShapeItem zah = z/job.ZBl;         // z row = A row
        const ShapeItem zbl = z - zah*job.ZBl;   // z column = B column
        const Cell * row_A = job.cA + job.incA*(zah * job.LO_len);
        const Cell * col_B = job.cB + job.incB*zbl;
        loop(l, job.LO_len)
           {
             job.ec = (col_B->*job.RO)(&product, row_A);
             if (job.ec != E_NO_ERROR)   return;

             if (l == 0)   // first element
                {
                  job.ec = (col_B->*job.RO)(job.cZ + z, row_A);
                  if (job.ec != E_NO_ERROR)   return;
                }
             else
                {
                  job.ec = (col_B->*job.RO)(&product, row_A);
                  if (job.ec != E_NO_ERROR)   return;
                  job.ec = (product.*job.LO)(job.cZ + z, job.cZ + z);
                  if (job.ec != E_NO_ERROR)   return;
                }
            row_A += job.incA;
            col_B += job.incB*job.ZBl;
           }
       }
}
//-----------------------------------------------------------------------------
