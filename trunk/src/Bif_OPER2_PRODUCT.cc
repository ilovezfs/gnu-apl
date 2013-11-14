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

#include "Bif_OPER2_PRODUCT.hh"
#include "Bif_OPER1_REDUCE.hh"
#include "Workspace.hh"

Bif_JOT            Bif_JOT::fun;
Bif_OPER2_PRODUCT  Bif_OPER2_PRODUCT::fun;

//-----------------------------------------------------------------------------
Token
Bif_OPER2_PRODUCT::eval_ALRB(Value_P A, Token & _LO, Token & _RO, Value_P B)
{
   if (!_LO.is_function())    SYNTAX_ERROR;
   if (!_RO.is_function())    SYNTAX_ERROR;

Function * LO = _LO.get_function();
Function * RO = _RO.get_function();

   if (!RO->has_result())   DOMAIN_ERROR;
   if (!LO->has_result())   DOMAIN_ERROR;

   if (LO->get_token().get_tag() == TOK_JOT)   // ∘.RO : outer product
      {
        return outer_product(A, _RO, B);
      }
   else                                        // LO.RO : inner product
      {
        return inner_product(A, _LO, _RO, B);
      }
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_PRODUCT::outer_product(Value_P A, Token & _RO, Value_P B)
{
Function * RO = _RO.get_function();
   Assert(RO);

OUTER_PROD arg;

   arg.Z = new Value(A->get_shape() + B->get_shape(), LOC);
   arg.cZ = &arg.Z->get_ravel(0);
   arg.len_A = A->element_count();
   arg.len_B = B->element_count();

   arg.value_A = new Value(LOC);   // helper value for non-pointer cA
   arg.value_A->set_eoc();
   arg.A = A;
   arg.unlock_A = !A->is_eoc();
   arg.A->set_eoc();

   arg.RO = RO;

   arg.B = B;
   arg.unlock_B = !B->is_eoc();
   arg.B->set_eoc();

   arg.value_B = new Value(LOC);   // helper value for non-pointer cB
   arg.value_B->set_eoc();
   arg.cA = &A->get_ravel(0);
   arg.cB = &B->get_ravel(0);

   arg.how = 0;
   return finish_outer_product(arg);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_PRODUCT::finish_outer_product(OUTER_PROD & arg)
{
   if (arg.how == 1)   goto how_1;

how_0:
   Assert1(arg.how == 0);

   arg.a = 0;
loop_a:

   arg.cB = &arg.B->get_ravel(0);

   if (arg.cA->is_pointer_cell())
      {
        arg.RO_A = arg.cA++->get_pointer_value();
      }
   else
      {
        arg.value_A->get_ravel(0).init(*arg.cA++);
        arg.value_A->set_complete();
        arg.RO_A = arg.value_A;
      }

   arg.b = 0;
   arg.cB = &arg.B->get_ravel(0);
loop_b:

   if (arg.cB->is_pointer_cell())
      {
        arg.RO_B = arg.cB++->get_pointer_value();
      }
   else
      {
        arg.value_B->get_ravel(0).init(*arg.cB++);
        arg.value_B->set_complete();
        arg.RO_B = arg.value_B;
      }

   {
     Token result = arg.RO->eval_AB(arg.RO_A, arg.RO_B);

   // if RO was a primitive function, then result may be a value.
   // if RO was a user defined function then result may be
   // TOK_SI_PUSHED. In both cases result could be TOK_ERROR.
   //
   if (result.get_Class() == TC_VALUE)
      {
        Value_P ZZ = result.get_apl_val();
        arg.cZ++->init_from_value(ZZ, LOC);
        goto next_a_b;
      }

   if (result.get_tag() == TOK_ERROR)   return result;

   if (result.get_tag() == TOK_SI_PUSHED)
      {
        // RO was a user defined function
        //
        arg.how = 1;
        Workspace::the_workspace->SI_top()->set_eoc_handler(eoc_outer_product);
        Workspace::the_workspace->SI_top()->get_eoc_arg()._OUTER_PROD() = arg;

        return result;   // continue in user defined function...
      }
   }

how_1:
next_a_b:

   if (++arg.b < arg.len_B)   goto loop_b;
   if (++arg.a < arg.len_A)   goto loop_a;

   arg.value_A->clear_eoc();
   arg.value_A->erase(LOC);

   arg.value_B->clear_eoc();
   arg.value_B->erase(LOC);

   arg.Z->set_default(arg.B);

   if (arg.unlock_A)   arg.A->clear_eoc();
   if (arg.unlock_B)   arg.B->clear_eoc();
   arg.A->erase(LOC);
   if (arg.B != arg.A)   arg.B->erase(LOC);

   return CHECK(arg.Z, LOC);
}
//-----------------------------------------------------------------------------
bool
Bif_OPER2_PRODUCT::eoc_outer_product(Token & token, _EOC_arg & _arg)
{
   // we copy _arg since pop_SI() will destroy it
   //
OUTER_PROD arg = _arg._OUTER_PROD();

   if (token.get_Class() != TC_VALUE)  return false;   // stop it

   // the user defined function has returned a value. Store it.
   //
   {
     Value_P ZZ = token.get_apl_val();
     arg.cZ++->init_from_value(ZZ, LOC);   // erase()s token.get_apl_val()
   }

   // pop the SI unless this is the last cZ to be computed
   //
   {
      const bool more = arg.a < (arg.len_A - 1) || arg.b < (arg.len_B - 1);
      if (more)   Workspace::the_workspace->pop_SI(LOC);
   }
   token = finish_outer_product(arg);
   if (token.get_tag() == TOK_SI_PUSHED)   return true;   // continue

   return false;   // stop it
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_PRODUCT::inner_product(Value_P A, Token & _LO, Token & _RO, Value_P B)
{
Function * LO = _LO.get_function();
Function * RO = _RO.get_function();

   Assert1(LO);
   Assert1(RO);

INNER_PROD arg;
   arg.A = A;
   arg.LO = LO;
   arg.RO = RO;
   arg.B = B;

Shape shape_A1(A->get_shape());
ShapeItem len_A = 1;
   if (!A->is_skalar())
      {
        len_A = A->get_last_shape_item();
        shape_A1.remove_last_shape_item();
      }

Shape shape_B1(B->get_shape());   
ShapeItem len_B = 1;
   if (!B->is_skalar())
      {
        len_B = B->get_shape_item(0);
        shape_B1.remove_shape_item(0);
      }


   // we do not check len_A == len_B since LO may accept different lengths

   arg.items_A = shape_A1.element_count();
   arg.items_B = shape_B1.element_count();

   if (arg.items_A == 0 || arg.items_B == 0)   // empty result
      {
        // the outer product portion of LO.RO is empty.
        // Apply the fill function of RO
        //
        const Shape shape_Z = shape_A1 + shape_B1;
        return fill(shape_Z, A, RO, B, LOC);
      }

   arg.unlock_A = !A->is_eoc();
   arg.A->set_eoc();
   arg.unlock_B = !B->is_eoc();
   arg.B->set_eoc();

   arg.Z = new Value(shape_A1 + shape_B1, LOC);
   arg.cZ = &arg.Z->get_ravel(0);

   // create a vector with the rows of A
   //
   arg.args_A = new Value_P[arg.items_A];
   loop(i, arg.items_A)
       {
         Value_P v = new Value(len_A, LOC);
         v->get_ravel(0).init(A->get_ravel(0 + i*len_A));
         loop(a, len_A)
            {
              const ShapeItem src = (A->is_skalar()) ? 0 : a + i*len_A;
              v->get_ravel(a).init(A->get_ravel(src));
            }
         v->set_default(A);
         CHECK_VAL(v, LOC);
         v->set_shared();
         arg.args_A[i] = v;
       }

   // create a vector with the columns of B
   //
   arg.args_B = new Value_P[arg.items_B];
   loop(i, arg.items_B)
       {
         Value_P v = new Value(len_B, LOC);
         loop(b, len_B)
            {
              const ShapeItem src = (B->is_skalar()) ? 0 : b*arg.items_B + i;
              v->get_ravel(b).init(B->get_ravel(src));
            }

         v->set_default(B);
         CHECK_VAL(v, LOC);

         v->set_shared();
         arg.args_B[i] = v;
       }

   arg.how = 0;
   return finish_inner_product(arg);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_PRODUCT::finish_inner_product(INNER_PROD & arg)
{
   if (arg.how > 0)
      {
        if (arg.how == 1)   goto how_1;
        if (arg.how == 2)   goto how_2;
        FIXME;
      }

how_0:
   Assert1(arg.how == 0);
   arg.last_ufun = false;

   arg.a = 0;
loop_a:

   arg.b = 0;
loop_b:

   {
     const Token T1 = arg.RO->eval_AB(arg.args_A[arg.a], arg.args_B[arg.b]);

   if (T1.get_tag() == TOK_ERROR)   return T1;

   if (T1.get_tag() == TOK_SI_PUSHED)
      {
        // RO is a user defined function. If LO is also user defined then the
        // context pushed is not the last user defined function call for this
        // operator. Otherwise it may or may not be the last call.
        //
        if (arg.a >= (arg.items_A - 1) && arg.b >= (arg.items_B - 1))
           {
             // at this point, the last cZ is being computed. If LO is not
             // user defined, then this call is the last user defined
             // function call. Otherwise the LO-call below will be the last.
             //
             if (!arg.LO->may_push_SI())   arg.last_ufun = true;
           }

        arg.how = 1;
        StateIndicator & si = *Workspace::the_workspace->SI_top();
        si.set_eoc_handler(eoc_inner_product);
        si.get_eoc_arg()._INNER_PROD() = arg;

        return T1;   // continue in user defined function...
      }

     arg.V1 = T1.get_apl_val();   // V1 is A[a] RO B1[b]
   }

how_1:
   {
     const ShapeItem ec_V1 = arg.V1->element_count();

     if (ec_V1 <= 1)   // reduction of 0- or 1-element vector
        {
          // reduction of 0- or 1-element vector does not call LO->eval_AB()
          // but rather LO->eval_identity_fun() or reshape(B).
          // No need to handle TOK_SI_PUSHED for user-defined LO.
          //
          Token LO(TOK_FUN1, arg.LO);
          const Token T2 = Bif_OPER1_REDUCE::fun.eval_LB(LO, arg.V1);
          if (T2.get_tag() == TOK_ERROR)   return T2;
          arg.cZ++->init_from_value(T2.get_apl_val(), LOC);

          arg.V1->clear_arg();
          arg.V1->erase(LOC);

          goto next_a_b;
        }

     arg.V1->set_arg();   // prevent erase() of V1

     // use LO_B as accumulator for LO-reduction
     //
     arg.LO_B = arg.V1->get_ravel(ec_V1 - 1).to_value(LOC);
     arg.v1 = ec_V1 - 2;
   }

loop_v:
       {
         Value_P LO_A = arg.V1->get_ravel(arg.v1).to_value(LOC);
         LO_A->set_eoc();
         arg.LO_B->set_eoc();
         const Token T2 = arg.LO->eval_AB(LO_A, arg.LO_B);
         LO_A->clear_arg();
         LO_A->clear_eoc();
         LO_A->erase(LOC);
         arg.LO_B->clear_arg();
         arg.LO_B->clear_eoc();
         arg.LO_B->erase(LOC);

         if (T2.get_tag() == TOK_ERROR)   return T2;

         if (T2.get_tag() == TOK_SI_PUSHED)
            {
              // LO was a user defined function. check if this was the last call
              //
              if (arg.a >= (arg.items_A - 1) && arg.b >= (arg.items_B - 1))
                 {
                   Assert1(arg.LO->may_push_SI());
                   if (arg.v1 == 0)   arg.last_ufun = true;
                 }

              arg.how = 2;
              StateIndicator & si = *Workspace::the_workspace->SI_top();
              si.set_eoc_handler(eoc_inner_product);
              si.get_eoc_arg()._INNER_PROD() = arg;

              return T2;   // continue in user defined function...
            }


         if (T2.get_Class() != TC_VALUE)
            {
              Q(T2)   FIXME;
            }

         arg.LO_B = T2.get_apl_val();
       }

how_2:
   if (--arg.v1 >= 0)   goto loop_v;

   arg.cZ++->init_from_value(arg.LO_B, LOC);

   arg.V1->clear_arg();
   arg.V1->erase(LOC);

next_a_b:
   if (++arg.b < arg.items_B)   goto loop_b;
   if (++arg.a < arg.items_A)   goto loop_a;

   loop(i, arg.items_A)
      {
         arg.args_A[i]->clear_shared();
         arg.args_A[i]->erase(LOC);
      }
   delete arg.args_A;

   loop(i, arg.items_B)
      {
         arg.args_B[i]->clear_shared();
         arg.args_B[i]->erase(LOC);
      }
   delete arg.args_B;

   arg.Z->set_default(arg.B);

   if (arg.unlock_A)   arg.A->clear_eoc();
   if (arg.unlock_B)   arg.B->clear_eoc();
   arg.A->erase(LOC);
   if (arg.A != arg.B)   arg.B->erase(LOC);

   return CHECK(arg.Z, LOC);
}
//-----------------------------------------------------------------------------
bool
Bif_OPER2_PRODUCT::eoc_inner_product(Token & token, _EOC_arg & _arg)
{
   // we copy _arg since pop_SI() will destroy it
   //
INNER_PROD arg = _arg._INNER_PROD();
   if (!arg.last_ufun)   Workspace::the_workspace->pop_SI(LOC);

   if (token.get_Class() == TC_VALUE)
      {
       // a user defined function has returned a value. Store it.
       //
       if      (arg.how == 1)   arg.V1 = token.get_apl_val();
       else if (arg.how == 2)   arg.LO_B = token.get_apl_val();
       else                     FIXME;
      }

   token = finish_inner_product(arg);
   if (token.get_tag() == TOK_SI_PUSHED)   return true;   // continue

   return false;
}
//-----------------------------------------------------------------------------
