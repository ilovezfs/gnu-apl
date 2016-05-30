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

/* Terminology:

   The power operator ⍣ can be called with a numerical or function left
   argument, called Form1 and Form2 in Dyalog APL:

   (A) (LO ⍣ N ) B   Form 1: (A) LO B (or its inverse) is called ∣N times
   (A) (LO ⍣ RO) B   Form 2  (A) LO B is called if RO says so

   We call LO the work function and RO the condition function.

   If LO and/or RO is user defined, then 3 different EOC handlers may occur:

   eoc_form_1:   for Form 1 and user defined work function LO
   eoc_RO:       for Form 2 and user defined condition function RO
   eoc_LO:       for Form 2 and user defined work function LO

   The _arg.how parameter indicates which eoc handler is to be executed next.
   This results in one of the following patterns:

   - no EOC handlers if neither LO nor RO are user defined
   - how = 0 0 0 0 ...   Form 1 with user defined LO
   - how = 1 1 1 1 ...   Form 2 with user defined LO and primitive RO
   - how = 2 2 2 2 ...   Form 2 with primitive LO and user defined RO
   - how = 1 2 1 2 ...   Form 2 with user defined LO and user defined RO
 */

#include "Bif_OPER2_POWER.hh"
#include "IntCell.hh"
#include "PointerCell.hh"
#include "Workspace.hh"

Bif_OPER2_POWER   Bif_OPER2_POWER::_fun;
Bif_OPER2_POWER * Bif_OPER2_POWER::fun = &Bif_OPER2_POWER::_fun;

//-----------------------------------------------------------------------------
Token
Bif_OPER2_POWER::eval_ALRB(Value_P A, Token & LO, Token & RO, Value_P B)
{
   if (RO.get_ValueType() == TV_VAL)   // integer count
      return eval_form_1(A, LO.get_function(), RO.get_apl_val(), B);

EOC_arg * arg = new EOC_arg(Value_P(), A, LO.get_function(), 0, B);
POWER_ALRB & _arg = arg->u.u_POWER_ALRB;
   _arg.user_RO = 0;

   _arg.how = 2;
   arg->RO = RO.get_function();
   _arg.user_RO = arg->RO->is_user_defined() ? 1 : 0;

   return finish_ALRB(*arg, true);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_POWER::eval_LRB(Token & LO, Token & RO, Value_P B)
{
   if (RO.get_ValueType() == TV_VAL)   // integer count
      return eval_form_1(LO.get_function(), RO.get_apl_val(), B);

EOC_arg * arg = new EOC_arg(Value_P(), Value_P(), LO.get_function(), 0, B);
POWER_ALRB & _arg = arg->u.u_POWER_ALRB;
   _arg.user_RO = false;

   _arg.how = 2;
   arg->RO = RO.get_function();
   _arg.user_RO = arg->RO->is_user_defined();

   return finish_ALRB(*arg, true);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_POWER::finish_ALRB(EOC_arg & arg, bool first)
{
POWER_ALRB & _arg = arg.u.u_POWER_ALRB;

again:
   if (_arg.how == 1)   // evaluate condition function RO
      {
        Token result_RO = arg.RO->eval_AB(arg.Z, arg.B);
        if (result_RO.get_tag() == TOK_ERROR)   return result_RO;

        if (result_RO.get_tag() == TOK_SI_PUSHED)   // RO was user-defined
           {
             Workspace::SI_top()->add_eoc_handler(eoc_RO, arg, LOC);
             delete &arg;
             return result_RO;   // continue in user defined function...
           }

        Assert(result_RO.get_Class() == TC_VALUE);
        Value_P condition = result_RO.get_apl_val();
        const bool stop = get_condition_value(*condition);
        if (stop)
           {
             Value_P Z(arg.Z);
             return Token(TOK_APL_VALUE1, Z);
           }

        arg.B = arg.Z;
        arg.Z.clear(LOC);
        _arg.how = 2;
      }

   // condition RO was true (and the condition function was primitive).
   // Evaluate LO
   //
   {
     Token result_LO = !arg.A ? arg.LO->eval_B(arg.B)
                              : arg.LO->eval_AB(arg.A, arg.B);
     if (result_LO.get_tag() == TOK_ERROR)   return result_LO;

     if (result_LO.get_tag() == TOK_SI_PUSHED)   // RO was user-defined
        {
          Workspace::SI_top()->add_eoc_handler(eoc_LO, arg, LOC);
          delete &arg;
          return result_LO;   // continue in user defined function...
        }

     Assert(result_LO.get_Class() == TC_VALUE);
     arg.Z = result_LO.get_apl_val();
   }

   // at this point, both the condition function RO and function LO were
   // were primitive. Repeat.
   //
   _arg.how = 1;
   goto again;
}
//-----------------------------------------------------------------------------
bool
Bif_OPER2_POWER::eoc_RO(Token & token)
{
   if (token.get_Class() != TC_VALUE)  return false;   // stop it

EOC_arg * arg = Workspace::SI_top()->remove_eoc_handlers();
EOC_arg * next = arg->next;
POWER_ALRB & _arg = arg->u.u_POWER_ALRB;

   // the user-defined condition function RO was successful and has
   // returned a value.
   //
   Assert(_arg.how == 1);

Value_P cond =  token.get_apl_val();
const bool stop = get_condition_value(*cond);
   if (stop)
      {
        Value_P Z(arg->Z);
        Token TZ(TOK_APL_VALUE1, Z);
        move_1(token, TZ, LOC);

        delete arg;
        Workspace::SI_top()->set_eoc_handlers(next);
        if (next)   return next->handler(token);

        return false;   // stop it
      }

   arg->B = arg->Z;
   arg->Z.clear(LOC);

   Workspace::pop_SI(LOC);

   _arg.how = 2;
   copy_1(token, finish_ALRB(*arg, false), LOC);
   Assert(token.get_tag() == TOK_SI_PUSHED);
   return true;   // continue
}
//-----------------------------------------------------------------------------
bool
Bif_OPER2_POWER::eoc_LO(Token & token)
{
   if (token.get_Class() != TC_VALUE)  return false;   // stop it

EOC_arg * arg = Workspace::SI_top()->remove_eoc_handlers();
EOC_arg * next = arg->next;
POWER_ALRB & _arg = arg->u.u_POWER_ALRB;

   // the user-defined work function LO was successful and has returned a value
   //
   Assert(_arg.how == 2);

   arg->Z = token.get_apl_val();
   if (_arg.user_RO)   // condition function RO is user-defined
      {
        Workspace::pop_SI(LOC);

        Token result_RO = arg->RO->eval_AB(arg->Z, arg->B);
        if (result_RO.get_tag() == TOK_ERROR)   return false;

        Assert(result_RO.get_tag() == TOK_SI_PUSHED);

        _arg.how = 1;
        move_1(token, result_RO, LOC);

        Workspace::SI_top()->add_eoc_handler(eoc_RO, *arg, LOC);
         delete arg;
        return true;   // continue
      }

   // at this point, condition function RO is primitive
   //
Token result_RO = arg->RO->eval_AB(arg->Z, arg->B);
   move_1(token, result_RO, LOC);
   if (token.get_tag() == TOK_ERROR)   return false;

   _arg.how = 1;

Value_P cond =  token.get_apl_val();
const bool stop = get_condition_value(*cond);
   if (stop)
      {
        Value_P Z(arg->Z);
        Token TZ(TOK_APL_VALUE1, Z);
        move_1(token, TZ, LOC);

        delete arg;
        Workspace::SI_top()->set_eoc_handlers(next);
        if (next)   return next->handler(token);

        return false;   // stop it
      }

   arg->B = arg->Z;
   arg->Z.clear(LOC);

   Workspace::pop_SI(LOC);

   _arg.how = 2;
   copy_1(token, finish_ALRB(*arg, false), LOC);
   Assert(token.get_tag() == TOK_SI_PUSHED);
   return true;   // continue
}
//-----------------------------------------------------------------------------
bool
Bif_OPER2_POWER::get_condition_value(const Value & RO)
{
   if (RO.element_count() != 1)
      {
        if (RO.get_rank() > 1)   RANK_ERROR;
        else                     LENGTH_ERROR;
      }

   if (!RO.get_ravel(0).is_near_bool())   DOMAIN_ERROR;

   return RO.get_ravel(0).get_checked_near_int();
}
//-----------------------------------------------------------------------------
// the xxx_form_1() functions are for LO ⍣ N B and A LO ⍣ N B variants
// (with numeric RO and worker function LO)
Token
Bif_OPER2_POWER::eval_form_1(Value_P A, Function * LO, Value_P N, Value_P B)
{
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
        return  LO->eval_AB(A, B);
      }

EOC_arg * arg = new EOC_arg(Value_P(), A, LO, 0, B);
POWER_ALRB & _arg = arg->u.u_POWER_ALRB;
   _arg.user_RO = 0;
   _arg.repeat_count = repeat_cnt;

   return finish_form_1(*arg, true);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_POWER::eval_form_1(Function * LO, Value_P N, Value_P B)
{
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
        Function * inverse = LO->get_monadic_inverse();
        if (inverse == 0)   DOMAIN_ERROR;   // no inverse

        LO = inverse;
        repeat_cnt = -repeat_cnt;
      }

   if (repeat_cnt == 1)
      {
        return  LO->eval_B(B);
      }

EOC_arg * arg = new EOC_arg(Value_P(), Value_P(), LO, 0, B);
POWER_ALRB & _arg = arg->u.u_POWER_ALRB;
   _arg.user_RO = false;
   _arg.repeat_count = repeat_cnt;

   return finish_form_1(*arg, true);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_POWER::finish_form_1(EOC_arg & arg, bool first)
{
POWER_ALRB & _arg = arg.u.u_POWER_ALRB;

   for (;;)
       {
         Token result_LO = !arg.A ? arg.LO->eval_B(arg.B)
                                  : arg.LO->eval_AB(arg.A, arg.B);

         if (result_LO.get_tag() == TOK_ERROR)   return result_LO;
         --_arg.repeat_count;

         if (result_LO.get_Class() == TC_VALUE)   // LO was primitive
            {
              if (_arg.repeat_count == 0)
                 {
                   delete &arg;
                   return result_LO;
                 }
              arg.B = result_LO.get_apl_val();
              continue;
            }

         Assert(result_LO.get_tag() == TOK_SI_PUSHED);

         Workspace::SI_top()->add_eoc_handler(eoc_form_1, arg, LOC);
         delete &arg;
         return result_LO;   // continue in user defined function...
       }
}
//-----------------------------------------------------------------------------
bool
Bif_OPER2_POWER::eoc_form_1(Token & token)
{
   if (token.get_Class() != TC_VALUE)  return false;   // stop it

EOC_arg * arg = Workspace::SI_top()->remove_eoc_handlers();
EOC_arg * next = arg->next;
POWER_ALRB & _arg = arg->u.u_POWER_ALRB;

   // at this point the user defined work funtion LO was successful and has
   // returned a result

   if (_arg.repeat_count == 0)
      {
        // all iterations done.
        //
        delete arg;
        Workspace::SI_top()->set_eoc_handlers(next);
        if (next)   return next->handler(token);

        return false;  // stop it
      }

   Workspace::pop_SI(LOC);

   arg->B = token.get_apl_val();
   copy_1(token, finish_form_1(*arg, false), LOC);
   Assert(token.get_tag() == TOK_SI_PUSHED);
   return true;   // continue
}
//-----------------------------------------------------------------------------
