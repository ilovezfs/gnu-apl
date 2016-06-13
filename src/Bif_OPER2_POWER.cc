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
#include "Bif_OPER2_POWER.hh"
#include "IntCell.hh"
#include "Macro.hh"
#include "PointerCell.hh"
#include "Workspace.hh"

Bif_OPER2_POWER   Bif_OPER2_POWER::_fun;
Bif_OPER2_POWER * Bif_OPER2_POWER::fun = &Bif_OPER2_POWER::_fun;

//-----------------------------------------------------------------------------
Token
Bif_OPER2_POWER::eval_ALRB(Value_P A, Token & LO, Token & RO, Value_P B)
{
   if (RO.get_ValueType() == TV_VAL)   // integer count
      return eval_form_1(A, LO, RO.get_apl_val(), B);
   else
      return eval_form_2(A, LO, RO, B);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_POWER::eval_LRB(Token & LO, Token & RO, Value_P B)
{
   if (RO.get_ValueType() == TV_VAL)   // integer count
      return eval_form_1(Value_P(), LO, RO.get_apl_val(), B);
   else
      return eval_form_2(Value_P(), LO, RO, B);
}
//-----------------------------------------------------------------------------
// the eval_form_2() function is for LO ⍣ N B and A LO ⍣ N B variants
// (with condition function RO and worker function LO)
Token
Bif_OPER2_POWER::eval_form_2(Value_P A, Token & _LO, Token & _RO, Value_P B)
{
Function * LO = _LO.get_function();   Assert(LO);
Function * RO = _RO.get_function();   Assert(RO);

   if (!LO->has_result())   DOMAIN_ERROR;
   if (!RO->has_result())   DOMAIN_ERROR;

   if (LO->may_push_SI() || RO->may_push_SI())   // user-defined or macro
      {
        if (!A)   return Macro::Z__LO_POWER_RO_B  ->eval_LRB (   _LO, _RO, B);
        else      return Macro::Z__A_LO_POWER_RO_B->eval_ALRB(A, _LO, _RO, B);
      }

   // primitive LO and RO
   //
   for (;;)
       {
         const Token result_LO = !A ? LO->eval_B(B) : LO->eval_AB(A, B);
         if (result_LO.get_tag() == TOK_ERROR)   return result_LO;

         Assert(result_LO.get_Class() == TC_VALUE);
         Value_P LO_Z = result_LO.get_apl_val();

         const Token result_RO = RO->eval_AB(LO_Z, B);
         if (result_RO.get_tag() == TOK_ERROR)   return result_RO;

         Assert(result_RO.get_Class() == TC_VALUE);
         Value_P condition = result_RO.get_apl_val();
         if (condition->is_scalar_extensible() &&
             condition->get_ravel(0).is_near_bool() &&
             condition->get_ravel(0).get_near_int() == 1)
            return Token(TOK_APL_VALUE1, LO_Z);

        B = LO_Z;
        LO_Z.clear(LOC);
      }
}
//-----------------------------------------------------------------------------
// the eval_form_1() function is for LO ⍣ N B and A LO ⍣ N B variants
// (with numeric RO and worker function LO)
Token
Bif_OPER2_POWER::eval_form_1(Value_P A, Token & _LO, Value_P N, Value_P B)
{
Function * LO = _LO.get_function();
   Assert(LO);

   Assert(!!N);
   if (N->element_count() != 1)
      {
        if (N->get_rank() > 1)   RANK_ERROR;
        else                     LENGTH_ERROR;
      }

   if (!N->get_ravel(0).is_near_int())   DOMAIN_ERROR;
ShapeItem repeat_cnt = N->get_ravel(0).get_checked_near_int();

   // special cases: 0, negative, and 1
   //
   if (repeat_cnt == 0)
      {
        Value_P Z(B);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (repeat_cnt < 0)   // inverse
      {
        Function * inverse = LO->get_dyadic_inverse();
        if (inverse == 0)   DOMAIN_ERROR;   // no inverse for LO

        LO = inverse;
        repeat_cnt = - repeat_cnt;
      }

   if (repeat_cnt == 1)
      {
        if (!A)   return  LO->eval_B(B);
        else      return  LO->eval_AB(A, B);
      }

   // at this point, repeat_cnt > 1
   //
   if (LO->may_push_SI())   // user-defined or macro
      {
        Token N(TOK_APL_VALUE1, IntScalar(repeat_cnt, LOC));
        if (LO->has_result())
           if (!A)   return Macro::Z__LO_POWER_N_B  ->eval_LRB (   _LO, N, B);
           else      return Macro::Z__A_LO_POWER_N_B->eval_ALRB(A, _LO, N, B);
        else
           if (!A)   return Macro::LO_POWER_N_B  ->eval_LRB (   _LO, N, B);
           else      return Macro::A_LO_POWER_N_B->eval_ALRB(A, _LO, N, B);
      }

   for (;;)
       {
         Token result = !A ? LO->eval_B(B)
                           : LO->eval_AB(A, B);

         if (result.get_tag() == TOK_ERROR)   return result;
         Assert(result.get_Class() == TC_VALUE);
         if (--repeat_cnt == 0)   return result;
         B = result.get_apl_val();
       }
}
//-----------------------------------------------------------------------------
