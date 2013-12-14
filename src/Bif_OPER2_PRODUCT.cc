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

EOC_arg arg;
OUTER_PROD & _arg = arg.u.u_OUTER_PROD;

   arg.Z = Value_P(new Value(A->get_shape() + B->get_shape(), LOC), LOC);
   _arg.cZ = &arg.Z->get_ravel(0);
   _arg.len_A = A->element_count();
   _arg.len_B = B->element_count();

   arg.V1 = Value_P(new Value(LOC), LOC);   // helper value for non-pointer cA
   if (A->is_eoc())   A = A->clone(LOC);
   arg.A = A;

   _arg.RO = RO;

   if (B->is_eoc())   B = B->clone(LOC);
   arg.B = B;

   arg.V2 = Value_P(new Value(LOC), LOC);   // helper value for non-pointer cB
   arg.V2->set_eoc();
   _arg.cA = &A->get_ravel(0);
   _arg.cB = &B->get_ravel(0);

   arg.set_EOC();
   _arg.how = 0;

   return finish_outer_product(arg);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_PRODUCT::finish_outer_product(EOC_arg & arg)
{
OUTER_PROD & _arg = arg.u.u_OUTER_PROD;
   if (_arg.how == 1)   goto how_1;

how_0:
   Assert1(_arg.how == 0);

   _arg.a = 0;
loop_a:

   _arg.cB = &arg.B->get_ravel(0);

   if (_arg.cA->is_pointer_cell())
      {
        arg.RO_A = _arg.cA++->get_pointer_value();
      }
   else
      {
        arg.V1->get_ravel(0).init(*_arg.cA++);
        arg.V1->set_complete();
        arg.RO_A = arg.V1;
      }

   _arg.b = 0;
   _arg.cB = &arg.B->get_ravel(0);
loop_b:

   if (_arg.cB->is_pointer_cell())
      {
        arg.RO_B = _arg.cB++->get_pointer_value();
      }
   else
      {
        arg.V2->get_ravel(0).init(*_arg.cB++);
        arg.V2->set_complete();
        arg.RO_B = arg.V2;
      }

   {
     Token result = _arg.RO->eval_AB(arg.RO_A, arg.RO_B);

   // if RO was a primitive function, then result may be a value.
   // if RO was a user defined function then result may be
   // TOK_SI_PUSHED. In both cases result could be TOK_ERROR.
   //
   if (result.get_Class() == TC_VALUE)
      {
        Value_P ZZ = result.get_apl_val();
        _arg.cZ++->init_from_value(ZZ, LOC);
        goto next_a_b;
      }

   if (result.get_tag() == TOK_ERROR)   return result;

   if (result.get_tag() == TOK_SI_PUSHED)
      {
        // RO was a user defined function
        //
        _arg.how = 1;
        Workspace::SI_top()->set_eoc_handler(eoc_outer_product);
        Workspace::SI_top()->get_eoc_arg() = arg;

        return result;   // continue in user defined function...
      }
   }

how_1:
next_a_b:

   if (++_arg.b < _arg.len_B)   goto loop_b;
   if (++_arg.a < _arg.len_A)   goto loop_a;

   arg.Z->set_default(arg.B);

Value_P Z(arg.clear_EOC(LOC), LOC);
   return CHECK(Z, LOC);
}
//-----------------------------------------------------------------------------
bool
Bif_OPER2_PRODUCT::eoc_outer_product(Token & token, EOC_arg & si_arg)
{
   // we copy _arg since pop_SI() will destroy it
   //
EOC_arg arg = si_arg;
OUTER_PROD & _arg = arg.u.u_OUTER_PROD;

   if (token.get_Class() != TC_VALUE)  return false;   // stop it

   // the user defined function has returned a value. Store it.
   //
   {
     Value_P ZZ = token.get_apl_val();
     _arg.cZ++->init_from_value(ZZ, LOC);   // erase()s token.get_apl_val()
   }

   // pop the SI unless this is the last cZ to be computed
   //
   {
      const bool more = _arg.a < (_arg.len_A - 1) || _arg.b < (_arg.len_B - 1);
      if (more)   Workspace::pop_SI(LOC);
   }
   copy_1(token, finish_outer_product(arg), LOC);
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

EOC_arg arg;
INNER_PROD & _arg = arg.u.u_INNER_PROD;
   arg.A = A;
   _arg.LO = LO;
   _arg.RO = RO;
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

   _arg.items_A = shape_A1.element_count();
   _arg.items_B = shape_B1.element_count();

   if (_arg.items_A == 0 || _arg.items_B == 0)   // empty result
      {
        // the outer product portion of LO.RO is empty.
        // Apply the fill function of RO
        //
        const Shape shape_Z = shape_A1 + shape_B1;
        return fill(shape_Z, A, RO, B, LOC);
      }

   if (A->is_eoc())   A = A->clone(LOC);
   if (B->is_eoc())   B = B->clone(LOC);

   arg.Z = Value_P(new Value(shape_A1 + shape_B1, LOC), LOC);
   _arg.cZ = &arg.Z->get_ravel(0);

   // create a vector with the rows of A
   //
   _arg.args_A = new Value*[_arg.items_A];
   loop(i, _arg.items_A)
       {
         Value * v = new Value(len_A, LOC);
         v->get_ravel(0).init(A->get_ravel(0 + i*len_A));
         loop(a, len_A)
            {
              const ShapeItem src = (A->is_skalar()) ? 0 : a + i*len_A;
              v->get_ravel(a).init(A->get_ravel(src));
            }
         v->set_default(A);
         CHECK_VAL(v, LOC);
         v->set_shared();
         _arg.args_A[i] = v;
       }

   // create a vector with the columns of B
   //
   _arg.args_B = new Value *[_arg.items_B];
   loop(i, _arg.items_B)
       {
         Value * v = new Value(len_B, LOC);
         loop(b, len_B)
            {
              const ShapeItem src = (B->is_skalar()) ? 0 : b*_arg.items_B + i;
              v->get_ravel(b).init(B->get_ravel(src));
            }

         v->set_default(B);
         CHECK_VAL(v, LOC);

         v->set_shared();
         _arg.args_B[i] = v;
       }

   arg.set_EOC();
   _arg.how = 0;
   return finish_inner_product(arg);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_PRODUCT::finish_inner_product(EOC_arg & arg)
{
INNER_PROD & _arg = arg.u.u_INNER_PROD;

   if (_arg.how > 0)
      {
        if (_arg.how == 1)   goto how_1;
        if (_arg.how == 2)   goto how_2;
        FIXME;
      }

how_0:
   Assert1(_arg.how == 0);
   _arg.last_ufun = false;

   _arg.a = 0;
loop_a:

   _arg.b = 0;
loop_b:

   {
     const Token T1 = _arg.RO->eval_AB(Value_P(_arg.args_A[_arg.a], LOC),
                                      Value_P(_arg.args_B[_arg.b], LOC));

   if (T1.get_tag() == TOK_ERROR)   return T1;

   if (T1.get_tag() == TOK_SI_PUSHED)
      {
        // RO is a user defined function. If LO is also user defined then the
        // context pushed is not the last user defined function call for this
        // operator. Otherwise it may or may not be the last call.
        //
        if (_arg.a >= (_arg.items_A - 1) && _arg.b >= (_arg.items_B - 1))
           {
             // at this point, the last cZ is being computed. If LO is not
             // user defined, then this call is the last user defined
             // function call. Otherwise the LO-call below will be the last.
             //
             if (!_arg.LO->may_push_SI())   _arg.last_ufun = true;
           }

        _arg.how = 1;
        StateIndicator & si = *Workspace::SI_top();
        si.set_eoc_handler(eoc_inner_product);
        si.get_eoc_arg() = arg;

        return T1;   // continue in user defined function...
      }

     arg.V1 = T1.get_apl_val();   // V1 is A[a] RO B1[b]
     arg.V1->set_eoc();
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
          Token LO(TOK_FUN1, _arg.LO);
          const Token T2 = Bif_OPER1_REDUCE::fun.eval_LB(LO, arg.V1);
          if (T2.get_tag() == TOK_ERROR)   return T2;
          _arg.cZ++->init_from_value(T2.get_apl_val(), LOC);

          arg.V1->clear_arg();
          arg.V1->clear_eoc();
          arg.V1->erase(LOC);
          arg.V1.clear(LOC);

          goto next_a_b;
        }

     arg.V1->set_arg();   // prevent erase() of V1

     // use V2 as accumulator for LO-reduction
     //
     arg.V2 = arg.V1->get_ravel(ec_V1 - 1).to_value(LOC);
     _arg.v1 = ec_V1 - 2;
   }

loop_v:
       {
         Value_P LO_A = arg.V1->get_ravel(_arg.v1).to_value(LOC);
         LO_A->set_eoc();
         const Token T2 = _arg.LO->eval_AB(LO_A, arg.V2);
         LO_A->clear_arg();
         LO_A->clear_eoc();
         LO_A->erase(LOC);
         arg.V2->clear_arg();
         arg.V2->clear_eoc();
         arg.V2->erase(LOC);
         arg.V2.clear(LOC);

         if (T2.get_tag() == TOK_ERROR)   return T2;

         if (T2.get_tag() == TOK_SI_PUSHED)
            {
              // LO was a user defined function. check if this was the last call
              //
              if (_arg.a >= (_arg.items_A - 1) && _arg.b >= (_arg.items_B - 1))
                 {
                   Assert1(_arg.LO->may_push_SI());
                   if (_arg.v1 == 0)   _arg.last_ufun = true;
                 }

              _arg.how = 2;
              StateIndicator & si = *Workspace::SI_top();
              si.set_eoc_handler(eoc_inner_product);
              si.get_eoc_arg() = arg;

              return T2;   // continue in user defined function...
            }


         if (T2.get_Class() != TC_VALUE)
            {
              Q(T2)   FIXME;
            }

         arg.V2 = T2.get_apl_val();
         arg.V2->set_eoc();
       }

how_2:
   if (--_arg.v1 >= 0)   goto loop_v;

   _arg.cZ++->init_from_value(arg.V2, LOC);

   arg.V1->clear_arg();
   arg.V1->clear_eoc();
   arg.V1->erase(LOC);
   arg.V1.clear(LOC);

   arg.V2->clear_eoc();
   arg.V2->erase(LOC);
   arg.V2.clear(LOC);

next_a_b:
   if (++_arg.b < _arg.items_B)   goto loop_b;
   if (++_arg.a < _arg.items_A)   goto loop_a;

   loop(i, _arg.items_A)
      {
         _arg.args_A[i]->clear_shared();
         _arg.args_A[i]->erase(LOC);
      }
   delete _arg.args_A;

   loop(i, _arg.items_B)
      {
         _arg.args_B[i]->clear_shared();
         _arg.args_B[i]->erase(LOC);
      }
   delete _arg.args_B;

   arg.Z->set_default(arg.B);
 
Value_P Z(arg.clear_EOC(LOC), LOC);
   return CHECK(Z, LOC);
}
//-----------------------------------------------------------------------------
bool
Bif_OPER2_PRODUCT::eoc_inner_product(Token & token, EOC_arg & si_arg)
{
   // copy si_arg since pop_SI() will destroy it
   //
EOC_arg arg = si_arg;
INNER_PROD & _arg = arg.u.u_INNER_PROD;

   if (!_arg.last_ufun)   Workspace::pop_SI(LOC);

   if (token.get_Class() == TC_VALUE)
      {
       // a user defined function has returned a value. Store it.
       //
       if      (_arg.how == 1)   (arg.V1 = token.get_apl_val())->set_eoc();
       else if (_arg.how == 2)   (arg.V2 = token.get_apl_val())->set_eoc();
       else                      FIXME;
      }

   copy_1(token, finish_inner_product(arg), LOC);
   if (token.get_tag() == TOK_SI_PUSHED)   return true;   // continue

   return false;
}
//-----------------------------------------------------------------------------
