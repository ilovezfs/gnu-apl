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

#include "Bif_OPER2_POWER.hh"
#include "IntCell.hh"
#include "PointerCell.hh"
#include "Workspace.hh"

Bif_OPER2_POWER Bif_OPER2_POWER::fun;

//-----------------------------------------------------------------------------
Token
Bif_OPER2_POWER::eval_ALRB(Value_P A, Token & LO, Token & RO, Value_P B)
{
EOC_arg arg(0, B, A);
POWER_ALRB & _arg = arg.u.u_POWER_ALRB;
   _arg.qct = Workspace::get_CT();
   _arg.LO = LO.get_function();
   _arg.user_COND = false;

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

        if (!value->get_ravel(0).is_near_int(_arg.qct))   DOMAIN_ERROR;
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
        _arg.how = 1;
        _arg.RO = RO.get_function();
        _arg.user_COND = _arg.RO->is_user_defined();
      }
   else Assert(0);

   return finish_ALRB(arg);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_POWER::finish_ALRB(EOC_arg & arg)
{
POWER_ALRB & _arg = arg.u.u_POWER_ALRB;

   if (_arg.how == 0)   // count-down repeat_count
      {
        for (;;)
            {
              Token result = _arg.LO->eval_AB(arg.A, arg.B);
              if (result.get_tag() == TOK_ERROR)   return result;
              --_arg.repeat_count;

              if (result.get_Class() == TC_VALUE)   // LO was primitive
                 {
                   if (_arg.repeat_count == 0)   return result;
                   arg.B = result.get_apl_val();
                   continue;
                 }

              Assert(result.get_tag() == TOK_SI_PUSHED);

              Workspace::SI_top()->set_eoc_handler(eoc_ALRB);
              Workspace::SI_top()->get_eoc_arg() = arg;
              return result;   // continue in user defined function...
            }

        Assert(0 && "Not reached");
      }

again:
   if (_arg.how == 1)   // evaluate condition function RO
      {
        Token result = _arg.RO->eval_AB(arg.A, arg.B);
        if (result.get_tag() == TOK_ERROR)   return result;

        if (result.get_tag() == TOK_SI_PUSHED)   // RO was user-defined
           {
              Workspace::SI_top()->set_eoc_handler(eoc_ALRB);
              Workspace::SI_top()->get_eoc_arg() = arg;
              return result;   // continue in user defined function...
           }

        Assert(result.get_Class() == TC_VALUE);
        Value_P COND = result.get_apl_val();
        const bool goon = get_condition_value(*COND, _arg.qct);
        if (!goon)
           {
             Value_P Z(arg.B);
             return Token(TOK_APL_VALUE1, Z);
           }

        _arg.how = 2;
      }

   // condition was true (and the condition function was primitive).
   // Evaluate LO
   //
   {
     Token result = _arg.LO->eval_AB(arg.A, arg.B);
     if (result.get_tag() == TOK_ERROR)   return result;

     if (result.get_tag() == TOK_SI_PUSHED)   // RO was user-defined
        {
          Workspace::SI_top()->set_eoc_handler(eoc_ALRB);
          Workspace::SI_top()->get_eoc_arg() = arg;
          return result;   // continue in user defined function...
        }

     Assert(result.get_Class() == TC_VALUE);
     arg.B = result.get_apl_val();
   }

   // at this point, both the condition function RO and function LO were
   // were primitive. Repeat.
   //
   _arg.how = 1;
   goto again;
}
//-----------------------------------------------------------------------------
bool
Bif_OPER2_POWER::eoc_ALRB(Token & token, EOC_arg & si_arg)
{
EOC_arg arg = si_arg;
POWER_ALRB & _arg = arg.u.u_POWER_ALRB;

   if (token.get_Class() != TC_VALUE)  return false;   // stop it

   // at this point some user-defined function was successful and has
   // returned a value. For how == 0 or 2 the user defined function
   // was the WORK function (LO). For how = 1 the user defined function
   // was the COND function (RO).
   //
   if (_arg.how == 0)   // count-down mode
      {
        if (_arg.repeat_count == 0)   return false;  // stop it

        Workspace::pop_SI(LOC);

        arg.B = token.get_apl_val();
        copy_1(token, finish_ALRB(arg), LOC);
        Assert(token.get_tag() == TOK_SI_PUSHED);
        return true;   // continue
      }

   if (_arg.how == 1)   // user-defined RO (condition) has returned
      {
how_1:
        Value_P cond =  token.get_apl_val();
        const bool goon = get_condition_value(*cond, _arg.qct);

        if (!goon)
           {
             Value_P Z(arg.B);
             Token TZ(TOK_APL_VALUE1, Z);
             move_1(token, TZ, LOC);
             return false;   // stop it
           }

        Workspace::pop_SI(LOC);

        _arg.how = 2;
        copy_1(token, finish_ALRB(arg), LOC);
        Assert(token.get_tag() == TOK_SI_PUSHED);
        return true;   // continue
      }

   // user-defined WORK has returned
   //
   arg.B = token.get_apl_val();
   if (_arg.user_COND)   // user-defined COND
      {
        Workspace::pop_SI(LOC);

        Token cond_token = _arg.RO->eval_AB(arg.A, arg.B);
        if (cond_token.get_tag() == TOK_ERROR)   return false;
        Assert(cond_token.get_tag() == TOK_SI_PUSHED);

        _arg.how = 1;
        move_1(token, cond_token, LOC);

        Workspace::SI_top()->set_eoc_handler(eoc_ALRB);
        Workspace::SI_top()->get_eoc_arg() = arg;

        return true;   // continue
      }
   else                 // primitive condition
      {
        Token cond_token = _arg.RO->eval_AB(arg.A, arg.B);
        move_1(token, cond_token, LOC);
        if (token.get_tag() == TOK_ERROR)   return false;

        _arg.how = 1;
        goto how_1;
      }
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_POWER::eval_LRB(Token & LO, Token & RO, Value_P B)
{
EOC_arg arg(0, B, 0);
POWER_ALRB & _arg = arg.u.u_POWER_ALRB;
   _arg.qct = Workspace::get_CT();
   _arg.LO = LO.get_function();
   _arg.user_COND = false;

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

        if (!value->get_ravel(0).is_near_int(_arg.qct))   DOMAIN_ERROR;
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
        _arg.how = 1;
        _arg.RO = RO.get_function();
        _arg.user_COND = _arg.RO->is_user_defined();
      }
   else Assert(0);

   return finish_LRB(arg);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER2_POWER::finish_LRB(EOC_arg & arg)
{
POWER_ALRB & _arg = arg.u.u_POWER_ALRB;

   if (_arg.how == 0)   // count-down repeat_count
      {
        for (;;)
            {
              Token result = _arg.LO->eval_B(arg.B);
              if (result.get_tag() == TOK_ERROR)   return result;
              --_arg.repeat_count;

              if (result.get_Class() == TC_VALUE)   // LO was primitive
                 {
                   if (_arg.repeat_count == 0)   return result;
                   arg.B = result.get_apl_val();
                   continue;
                 }

              Assert(result.get_tag() == TOK_SI_PUSHED);

              Workspace::SI_top()->set_eoc_handler(eoc_LRB);
              Workspace::SI_top()->get_eoc_arg() = arg;
              return result;   // continue in user defined function...
            }

        Assert(0 && "Not reached");
      }

again:
   if (_arg.how == 1)   // evaluate condition function RO
      {
        Token result = _arg.RO->eval_B(arg.B);
        if (result.get_tag() == TOK_ERROR)   return result;

        if (result.get_tag() == TOK_SI_PUSHED)   // RO was user-defined
           {
              Workspace::SI_top()->set_eoc_handler(eoc_LRB);
              Workspace::SI_top()->get_eoc_arg() = arg;
              return result;   // continue in user defined function...
           }

        Assert(result.get_Class() == TC_VALUE);
        Value_P COND = result.get_apl_val();
        const bool goon = get_condition_value(*COND, _arg.qct);
        if (!goon)
           {
             Value_P Z(arg.B);
             return Token(TOK_APL_VALUE1, Z);
           }

        _arg.how = 2;
      }

   // condition was true (and the condition function was primitive).
   // Evaluate LO
   //
   {
     Token result = _arg.LO->eval_B(arg.B);
     if (result.get_tag() == TOK_ERROR)   return result;

     if (result.get_tag() == TOK_SI_PUSHED)   // RO was user-defined
        {
          Workspace::SI_top()->set_eoc_handler(eoc_LRB);
          Workspace::SI_top()->get_eoc_arg() = arg;
          return result;   // continue in user defined function...
        }

     Assert(result.get_Class() == TC_VALUE);
     arg.B = result.get_apl_val();
   }

   // at this point, both the condition function RO and function LO were
   // were primitive. Repeat.
   //
   _arg.how = 1;
   goto again;
}
//-----------------------------------------------------------------------------
bool
Bif_OPER2_POWER::eoc_LRB(Token & token, EOC_arg & si_arg)
{
EOC_arg arg = si_arg;
POWER_ALRB & _arg = arg.u.u_POWER_ALRB;

   if (token.get_Class() != TC_VALUE)  return false;   // stop it

   // at this point some user-defined function was successful and has
   // returned a value. For how == 0 or 2 the user defined function
   // was the WORK function (LO). For how = 1 the user defined function
   // was the COND function (RO).
   //
   if (_arg.how == 0)   // count-down mode
      {
        if (_arg.repeat_count == 0)   return false;  // stop it

        Workspace::pop_SI(LOC);

        arg.B = token.get_apl_val();
        copy_1(token, finish_LRB(arg), LOC);
        Assert(token.get_tag() == TOK_SI_PUSHED);
        return true;   // continue
      }

   if (_arg.how == 1)   // user-defined RO (condition) has returned
      {
how_1:
        Value_P cond =  token.get_apl_val();
        const bool goon = get_condition_value(*cond, _arg.qct);

        if (!goon)
           {
             Value_P Z(arg.B);
             Token TZ(TOK_APL_VALUE1, Z);
             move_1(token, TZ, LOC);
             return false;   // stop it
           }

        Workspace::pop_SI(LOC);

        _arg.how = 2;
        copy_1(token, finish_LRB(arg), LOC);
        Assert(token.get_tag() == TOK_SI_PUSHED);
        return true;   // continue
      }

   // user-defined WORK has returned
   //
   arg.B = token.get_apl_val();
   if (_arg.user_COND)   // user-defined COND
      {
        Workspace::pop_SI(LOC);

        Token cond_token = _arg.RO->eval_B(arg.B);
        if (cond_token.get_tag() == TOK_ERROR)   return false;
        Assert(cond_token.get_tag() == TOK_SI_PUSHED);

        _arg.how = 1;
        move_1(token, cond_token, LOC);

        Workspace::SI_top()->set_eoc_handler(eoc_LRB);
        Workspace::SI_top()->get_eoc_arg() = arg;

        return true;   // continue
      }
   else                 // primitive condition
      {
        Token cond_token = _arg.RO->eval_B(arg.B);
        move_1(token, cond_token, LOC);
        if (token.get_tag() == TOK_ERROR)   return false;

        _arg.how = 1;
        goto how_1;
      }
}
//-----------------------------------------------------------------------------
bool
Bif_OPER2_POWER::get_condition_value(const Value & COND, double qct)
{
   if (COND.element_count() != 1)
      {
        if (COND.get_rank() > 1)   RANK_ERROR;
        else                       LENGTH_ERROR;
      }

   if (!COND.get_ravel(0).is_near_bool(qct))   DOMAIN_ERROR;

   return COND.get_ravel(0).get_checked_near_int();
}
//-----------------------------------------------------------------------------
