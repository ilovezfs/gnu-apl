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

#include "Bif_OPER1_EACH.hh"
#include "PointerCell.hh"
#include "Workspace.hh"

Bif_OPER1_EACH     Bif_OPER1_EACH::fun;

//-----------------------------------------------------------------------------
Token
Bif_OPER1_EACH::eval_ALB(Value_P A, Token & _LO, Value_P B)
{
   // dyadic EACH

Function * LO = _LO.get_function();

   Assert1(LO);

EACH_ALB arg;
   arg.Z.clear(LOC);
   arg.cZ = 0;
   arg.dA = 1;
   arg.LO = LO;
   arg.dB = 1;

   if (A->is_empty() || B->is_empty())
      {
        if (!LO->has_result())   return Token(TOK_VOID);

        Value_P Fill_A = Bif_F12_TAKE::fun.eval_B(A).get_apl_val();
        Value_P Fill_B = Bif_F12_TAKE::fun.eval_B(B).get_apl_val();
        Shape shape_Z;

        if (A->is_empty())          shape_Z = A->get_shape();
        else if (!A->is_skalar())   DOMAIN_ERROR;

        if (B->is_empty())          shape_Z = B->get_shape();
        else if (!B->is_skalar())   DOMAIN_ERROR;

        arg.Z = LO->eval_fill_AB(Fill_A, Fill_B).get_apl_val();
        Fill_A->erase(LOC);
        Fill_B->erase(LOC);

        Value_P Z1(new Value(shape_Z, LOC), LOC);
        new (&Z1->get_ravel(0)) PointerCell(arg.Z);

        return CHECK(Z1, LOC);   // Z ??
      }

   if (A->nz_element_count() == 1)
      {
        if (B->is_skalar_or_len1_vector())   return LO->eval_AB(A, B);

        arg.count = B->element_count();

        arg.dA = 0;
        if (LO->has_result())   arg.Z = Value_P(new Value(B->get_shape(), LOC), LOC);
      }
   else if (B->nz_element_count() == 1)
      {
        arg.dB = 0;
        arg.count = A->element_count();
        if (LO->has_result())   arg.Z = Value_P(new Value(A->get_shape(), LOC), LOC);
      }
   else if (A->same_shape(B))
      {
        arg.count = B->element_count();
        if (LO->has_result())   arg.Z = Value_P(new Value(A->get_shape(), LOC), LOC);
      }
   else
      {
        if (A->same_rank(B))   LENGTH_ERROR;
        RANK_ERROR;
      }

   // if LO is user-defined, then we return from eval_ALB() while still holding
   // pointers (arg.cA and arg.cB) to its ravel. We therefore set_eoc() to
   // prevent premature erasure of A and B.
   //
   A->set_eoc();
   B->set_eoc();

   arg.A = A;
   arg.cA = &A->get_ravel(0);
   arg.B = B;
   arg.cB = &B->get_ravel(0);

   if (!!arg.Z)   arg.cZ = &arg.Z->get_ravel(0);

   arg.how = 0;
   return finish_eval_ALB(arg);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_EACH::finish_eval_ALB(EACH_ALB & arg)
{
   if (arg.how == 1)   goto how_1;

how_0:
   Assert1(arg.how == 0);
   arg.z = 0;

loop_z:
   {
     Value_P LO_A = arg.cA->to_value(LOC);     // left argument of LO
     Value_P LO_B = arg.cB->to_value(LOC);     // right argument of LO;

     arg.cA += arg.dA;
     arg.cB += arg.dB;
     arg.sub = !(LO_A->is_nested() || LO_B->is_nested());

     Token result = arg.LO->eval_AB(LO_A, LO_B);

     LO_A->clear_arg();
     LO_A->erase(LOC);
     LO_B->clear_arg();
     LO_B->erase(LOC);

     // if LO was a primitive function, then result may be a value.
     // if LO was a user defined function then result may be TOK_SI_PUSHED.
     // in both cases result could be TOK_ERROR.
     //
     if (result.get_Class() == TC_VALUE)
        {
          Value_P vZ = result.get_apl_val();
          if (arg.sub)   new (arg.cZ)  PointerCell(vZ);
          else           arg.cZ->init_from_value(vZ, LOC);

          ++arg.cZ;
          goto next_z;   // next z
        }

     if (result.get_tag() == TOK_ERROR)   return result;

     if (result.get_tag() == TOK_SI_PUSHED)
        {
          // LO was a user defined function
          //
          arg.how = 1;
          Workspace::the_workspace->SI_top()->set_eoc_handler(eoc_ALB);
          Workspace::the_workspace->SI_top()->get_eoc_arg()._EACH_ALB() = arg;

          return result;   // continue in user defined function...
        }

     Q(result);   FIXME;
   }

how_1:
next_z:
   if (++arg.z < arg.count)   goto loop_z;

   arg.Z->set_default(arg.B);

   arg.A->clear_eoc();
   arg.B->clear_eoc();

   arg.A->erase(LOC);
   if (arg.A != arg.B)   arg.B->erase(LOC);

   if (!arg.Z)   return Token(TOK_VOID);   // LO without result

   return CHECK(arg.Z, LOC);
}
//-----------------------------------------------------------------------------
bool
Bif_OPER1_EACH::eoc_ALB(Token & token, _EOC_arg & _arg)
{
EACH_ALB arg = _arg._EACH_ALB();

   if (!!arg.Z)   // LO with result, maybe successful
      {
       if (token.get_Class() != TC_VALUE)  return false;   // LO error: stop it

       Value_P vZ = token.get_apl_val();
       if (arg.sub)   new (arg.cZ++)  PointerCell(vZ);
       else           arg.cZ++->init_from_value(vZ, LOC);
      }
   else        // LO without result, maybe successful
      {
        if (token.get_tag() != TOK_VOID)    return false;   // LO error: stop it
      }

   if (arg.z < (arg.count - 1))   Workspace::the_workspace->pop_SI(LOC);

   copy_1(token, finish_eval_ALB(arg), LOC);
   if (token.get_tag() == TOK_SI_PUSHED)   return true;   // continue

   return false;   // stop it
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_EACH::eval_LB(Token & _LO, Value_P B)
{
   // monadic EACH

Function * LO = _LO.get_function();
   Assert1(LO);

   // if LO is user-defined, then we return from eval_LB() while still holding
   // a pointer (arg.cB) to its ravel. We therefore set_eoc() to prevent
   // premature erasure of B.
   //
   B->set_eoc();

EACH_LB arg;
   arg.LO = LO;
   arg.B = B;
   arg.cB = &B->get_ravel(0);
   if (LO->has_result())
      {
        arg.Z = Value_P(new Value(B->get_shape(), LOC), LOC);
        arg.cZ = &arg.Z->get_ravel(0);
      }
   else
      {
        arg.Z.clear(LOC);
        arg.cZ = 0;
      }

   arg.count = B->element_count();

   arg.how = 0;
   return finish_eval_LB(arg);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_EACH::finish_eval_LB(EACH_LB & arg)
{
   if (arg.how == 1)   goto how_1;

how_0:
   Assert1(arg.how == 0);
   arg.z = 0;

loop_z:
   {
     Value_P LO_B;         // right argument of LO;
     bool cup_B = false;   // clean-up B ?

     if (arg.cB->is_pointer_cell())
        {
          LO_B = arg.cB++->get_pointer_value();
          arg.sub = true;
        }
     else
        {
          LO_B = Value_P(new Value(LOC), LOC);
          LO_B->set_arg();

          LO_B->get_ravel(0).init(*arg.cB++);
          LO_B->set_complete();
          cup_B = true;   // clean-up B
          arg.sub = false;
        }

     Token result = arg.LO->eval_B(LO_B);

     if (cup_B)
        {
          LO_B->clear_arg();
          LO_B->erase(LOC);
        }

     if (result.get_tag() == TOK_SI_PUSHED)
        {
          // LO was a user defined function or ⍎
          //
          arg.how = 1;
          Workspace::the_workspace->SI_top()->set_eoc_handler(eoc_LB);
          Workspace::the_workspace->SI_top()->get_eoc_arg()._EACH_LB() = arg;
          return result;   // continue in user defined function...
        }

     // if LO was a primitive function, then result may be a value.
     // if LO was a user defined function then result may be TOK_SI_PUSHED.
     // in both cases result could be TOK_ERROR.
     //
     if (result.get_Class() == TC_VALUE)
        {
          Value_P vZ = result.get_apl_val();

          if (arg.sub)   new (arg.cZ++)   PointerCell(vZ);
          else           arg.cZ++->init_from_value(vZ, LOC);

          goto next_z;
        }

     if (result.get_tag() == TOK_VOID)   goto next_z;

     if (result.get_tag() == TOK_ERROR)   return result;

     Q(result);   FIXME;
   }

how_1:
next_z:
   if (++arg.z < arg.count)   goto loop_z;

   arg.B->clear_eoc();

   if (!arg.Z)   return Token(TOK_VOID);   // LO wothout result

   arg.Z->set_default(arg.B);

   return CHECK(arg.Z, LOC);
}
//-----------------------------------------------------------------------------
bool
Bif_OPER1_EACH::eoc_LB(Token & token, _EOC_arg & _arg)
{
EACH_LB arg = _arg._EACH_LB();

   if (!!arg.Z)   // LO with result, maybe successful
      {
       if (token.get_Class() != TC_VALUE)  return false;   // LO error: stop it

        Value_P vZ = token.get_apl_val();
        if (arg.sub)   new (arg.cZ++)  PointerCell(vZ);
        else           arg.cZ++->init_from_value(vZ, LOC);
      }
   else        // LO without result, maybe successful
      {
       if (token.get_tag() != TOK_VOID)    return false;   // LO error: stop it
      }

   if (arg.z < (arg.count - 1))   Workspace::the_workspace->pop_SI(LOC);
   copy_1(token, finish_eval_LB(arg), LOC);
   if (token.get_tag() == TOK_SI_PUSHED)   return true;   // continue

   return false;   // stop it
}
//-----------------------------------------------------------------------------

