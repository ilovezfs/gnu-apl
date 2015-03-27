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
EOC_arg arg(Value_P(), B, A);
POWER_ALRB & _arg = arg.u.u_POWER_ALRB;
   _arg.qct = Workspace::get_CT();
   _arg.LO = LO.get_function();
   _arg.user_RO = false;

   // RO can be a function or a value (integer count)
   //
   if (RO.get_ValueType() == TV_VAL)   // integer count
      {
        _arg.how = 0;
        Value_P value = RO.get_apl_val();
        Assert(!!value);
        if (value->element_count() != 1)
           {
             if (value->get_rank() > 1)   RANK_ERROR;
             else                         LENGTH_ERROR;
           }

        if (!value->get_ravel(0).is_near_int())   DOMAIN_ERROR;
        _arg.repeat_count = value->get_ravel(0).get_checked_near_int();

        // special cases: 0, negative, and 1
        //
        if (_arg.repeat_count == 0)
           {
             Value_P Z(B);
             return Token(TOK_APL_VALUE1, Z);
           }

        if (_arg.repeat_count < 0)   // inverse
           {
             Function * inverse = _arg.LO->get_dyadic_inverse();
             if (inverse == 0)   DOMAIN_ERROR;   // no inverse

             _arg.LO = inverse;
             _arg.repeat_count = -_arg.repeat_count;
           }

        if (_arg.repeat_count == 1)
           {
             return  _arg.LO->eval_AB(A, B);
           }
      }
   else if (RO.get_ValueType() == TV_FUN)   // function
      {
        _arg.how = 2;
        _arg.RO = RO.get_function();
        _arg.user_RO = _arg.RO->is_user_defined();
      }
   else Assert(0);

   return finish_ALRB(arg, true);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_POWER::finish_ALRB(EOC_arg & arg, bool first)
{
POWER_ALRB & _arg = arg.u.u_POWER_ALRB;

   if (_arg.how == 0)   // count-down repeat_count
      {
        for (;;)
            {
              Token result_LO = !arg.A ? _arg.LO->eval_B(arg.B)
                                       : _arg.LO->eval_AB(arg.A, arg.B);

              if (result_LO.get_tag() == TOK_ERROR)   return result_LO;
              --_arg.repeat_count;

              if (result_LO.get_Class() == TC_VALUE)   // LO was primitive
                 {
                   if (_arg.repeat_count == 0)   return result_LO;
                   arg.B = result_LO.get_apl_val();
                   continue;
                 }

              Assert(result_LO.get_tag() == TOK_SI_PUSHED);

             if (first)
                Workspace::SI_top()->add_eoc_handler(eoc_ALRB, arg, LOC);
             else
                Workspace::SI_top()->move_eoc_handler(eoc_ALRB, &arg, LOC);

              return result_LO;   // continue in user defined function...
            }

        Assert(0 && "Not reached");
      }

again:
   if (_arg.how == 1)   // evaluate condition function RO
      {
        Token result_RO = _arg.RO->eval_AB(arg.Z, arg.B);
        if (result_RO.get_tag() == TOK_ERROR)   return result_RO;

        if (result_RO.get_tag() == TOK_SI_PUSHED)   // RO was user-defined
           {
             if (first)
                Workspace::SI_top()->add_eoc_handler(eoc_ALRB, arg, LOC);
             else
                Workspace::SI_top()->move_eoc_handler(eoc_ALRB, &arg, LOC);
              return result_RO;   // continue in user defined function...
           }

        Assert(result_RO.get_Class() == TC_VALUE);
        Value_P condition = result_RO.get_apl_val();
        const bool stop = get_condition_value(*condition, _arg.qct);
        if (stop)
           {
             Value_P Z(arg.Z);
             return Token(TOK_APL_VALUE1, Z);
           }

        arg.B = arg.Z;
        arg.Z.clear(LOC);
        _arg.how = 2;
      }

   // condition was true (and the condition function was primitive).
   // Evaluate LO
   //
   {
     Token result_LO = !arg.A ? _arg.LO->eval_B(arg.B)
                              : _arg.LO->eval_AB(arg.A, arg.B);
     if (result_LO.get_tag() == TOK_ERROR)   return result_LO;

     if (result_LO.get_tag() == TOK_SI_PUSHED)   // RO was user-defined
        {
          if (first)
             Workspace::SI_top()->add_eoc_handler(eoc_ALRB, arg, LOC);
          else
             Workspace::SI_top()->move_eoc_handler(eoc_ALRB, &arg, LOC);
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
Bif_OPER2_POWER::eoc_ALRB(Token & token)
{
EOC_arg * arg = Workspace::SI_top()->remove_eoc_handlers();
EOC_arg * next = arg->next;
POWER_ALRB & _arg = arg->u.u_POWER_ALRB;

   if (token.get_Class() != TC_VALUE)  return false;   // stop it

   // at this point some user-defined function was successful and has
   // returned a value. For how == 0 or 2 the user defined function
   // was the work function (LO). For how = 1 the user defined function
   // was the condition function (RO).
   //
   if (_arg.how == 0)   // count-down mode
      {
        if (_arg.repeat_count == 0)
           {
             delete arg;
             Workspace::SI_top()->set_eoc_handlers(next);
             if (next)   return next->handler(token);

             return false;  // stop it
           }

        Workspace::pop_SI(LOC);

        arg->B = token.get_apl_val();
        copy_1(token, finish_ALRB(*arg, false), LOC);
        Assert(token.get_tag() == TOK_SI_PUSHED);
        Workspace::SI_top()->move_eoc_handler(eoc_ALRB, arg, LOC);
        return true;   // continue
      }

   if (_arg.how == 1)   // user-defined RO (condition) has returned
      {
how_1:
        Value_P cond =  token.get_apl_val();
        const bool stop = get_condition_value(*cond, _arg.qct);
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
        Workspace::SI_top()->move_eoc_handler(eoc_ALRB, arg, LOC);
        return true;   // continue
      }

   // user-defined LO has returned
   //
   arg->Z = token.get_apl_val();
   if (_arg.user_RO)   // if RO is user-defined
      {
        Workspace::pop_SI(LOC);

        Token result_RO = _arg.RO->eval_AB(arg->Z, arg->B);
        if (result_RO.get_tag() == TOK_ERROR)   return false;
        Assert(result_RO.get_tag() == TOK_SI_PUSHED);

        _arg.how = 1;
        move_1(token, result_RO, LOC);

         Workspace::SI_top()->move_eoc_handler(eoc_ALRB, arg, LOC);
        return true;   // continue
      }
   else                 // primitive condition
      {
        Token result_RO = _arg.RO->eval_AB(arg->Z, arg->B);
        move_1(token, result_RO, LOC);
        if (token.get_tag() == TOK_ERROR)   return false;

        _arg.how = 1;
        goto how_1;
      }
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_POWER::eval_LRB(Token & LO, Token & RO, Value_P B)
{
EOC_arg arg(Value_P(), B, Value_P());
POWER_ALRB & _arg = arg.u.u_POWER_ALRB;
   _arg.qct = Workspace::get_CT();
   _arg.LO = LO.get_function();
   _arg.user_RO = false;

   // RO can be a function or a value (integer count)
   //
   if (RO.get_ValueType() == TV_VAL)   // integer count
      {
        _arg.how = 0;
        Value_P value = RO.get_apl_val();
        Assert(!!value);
        if (value->element_count() != 1)
           {
             if (value->get_rank() > 1)   RANK_ERROR;
             else                         LENGTH_ERROR;
           }

        if (!value->get_ravel(0).is_near_int())   DOMAIN_ERROR;
        _arg.repeat_count = value->get_ravel(0).get_checked_near_int();

        // special cases: 0, negative, and 1
        //
        if (_arg.repeat_count == 0)
           {
             Value_P Z(B);
             return Token(TOK_APL_VALUE1, Z);
           }

        if (_arg.repeat_count < 0)   // inverse
           {
             Function * inverse = _arg.LO->get_monadic_inverse();
             if (inverse == 0)   DOMAIN_ERROR;   // no inverse

             _arg.LO = inverse;
             _arg.repeat_count = -_arg.repeat_count;
           }

        if (_arg.repeat_count == 1)
           {
             return  _arg.LO->eval_B(B);
           }
      }
   else if (RO.get_ValueType() == TV_FUN)   // function
      {
        _arg.how = 2;
        _arg.RO = RO.get_function();
        _arg.user_RO = _arg.RO->is_user_defined();
      }
   else Assert(0);

   return finish_ALRB(arg, true);
}
//-----------------------------------------------------------------------------
bool
Bif_OPER2_POWER::get_condition_value(const Value & RO, double qct)
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
