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

#include "Bif_OPER1_EACH.hh"
#include "PointerCell.hh"
#include "Workspace.hh"

Bif_OPER1_EACH Bif_OPER1_EACH::_fun;

Bif_OPER1_EACH * Bif_OPER1_EACH::fun = &Bif_OPER1_EACH::_fun;

//-----------------------------------------------------------------------------
Token
Bif_OPER1_EACH::eval_ALB(Value_P A, Token & _LO, Value_P B)
{
   // dyadic EACH

Function * LO = _LO.get_function();
   Assert1(LO);

   if (A->is_empty() || B->is_empty())
      {
        if (!LO->has_result())   return Token(TOK_VOID);

        Value_P Fill_A = Bif_F12_TAKE::first(A);
        Value_P Fill_B = Bif_F12_TAKE::first(B);
        Shape shape_Z;

        if (A->is_empty())          shape_Z = A->get_shape();
        else if (!A->is_scalar())   DOMAIN_ERROR;

        if (B->is_empty())          shape_Z = B->get_shape();
        else if (!B->is_scalar())   DOMAIN_ERROR;

        Value_P Z1 = LO->eval_fill_AB(Fill_A, Fill_B).get_apl_val();

        Value_P Z(shape_Z, LOC);
        new (&Z->get_ravel(0)) PointerCell(Z1, Z.getref());
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

EOC_arg arg(B);   arg.A = A;
EACH_ALB & _arg = arg.u.u_EACH_ALB;

   _arg.dA = 1;
   _arg.LO = LO;
   _arg.dB = 1;

// if (A->is_scalar() && B->is_scalar())   return LO->eval_AB(A, B);

   if (A->is_scalar())
      {
        _arg.count = B->element_count();

        _arg.dA = 0;
        if (LO->has_result())
           arg.Z = Value_P(B->get_shape(), LOC);
      }
   else if (B->is_scalar())
      {
        _arg.dB = 0;
        _arg.count = A->element_count();
        if (LO->has_result())
           arg.Z = Value_P(A->get_shape(), LOC);
      }
   else if (A->same_shape(*B))
      {
        _arg.count = B->element_count();
        if (LO->has_result())
           arg.Z = Value_P(A->get_shape(), LOC);
      }
   else
      {
        if (A->same_rank(*B))   LENGTH_ERROR;
        RANK_ERROR;
      }

   _arg.cA = &A->get_ravel(0);
   _arg.cB = &B->get_ravel(0);

   // arg.Z can be 0 (if not x(LO->has_result())

   _arg.z = -1;
   return finish_eval_ALB(arg);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_EACH::finish_eval_ALB(EOC_arg & arg)
{
EACH_ALB & _arg = arg.u.u_EACH_ALB;

   while (++_arg.z < _arg.count)
      {
        Value_P LO_A = _arg.cA->to_value(LOC);     // left argument of LO
        Value_P LO_B = _arg.cB->to_value(LOC);     // right argument of LO;
        _arg.sub = !(_arg.cA->is_pointer_cell() || _arg.cB->is_pointer_cell());

        _arg.cA += _arg.dA;
        _arg.cB += _arg.dB;

        Token result = _arg.LO->eval_AB(LO_A, LO_B);

        // if LO was a primitive function, then result may be a value.
        // if LO was a user defined function then result may be TOK_SI_PUSHED.
        // in both cases result could be TOK_ERROR.
        //
        if (result.get_Class() == TC_VALUE)
           {
             Value_P vZ = result.get_apl_val();

             if (!_arg.sub)
                arg.Z->next_ravel()->init_from_value(vZ, arg.Z.getref(), LOC);
             else if (vZ->is_simple_scalar())
                arg.Z->next_ravel()->init(vZ->get_ravel(0), arg.Z.getref());
             else
                new (arg.Z->next_ravel())   PointerCell(vZ, arg.Z.getref());

            continue;   // next z
           }

        if (result.get_tag() == TOK_ERROR)   return result;

        if (result.get_tag() == TOK_SI_PUSHED)
           {
             // LO was a user defined function
             //
             if (_arg.z)   // subsequent call
                Workspace::SI_top()->move_eoc_handler(eoc_ALB, &arg, LOC);
             else           // first call
                Workspace::SI_top()->add_eoc_handler(eoc_ALB, arg, LOC);

             return result;   // continue in user defined function...
           }

        Q1(result);   FIXME;
      }

   if (!arg.Z)   return Token(TOK_VOID);   // LO without result

   arg.Z->set_default(*arg.B.get());
   arg.Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, arg.Z);
}
//-----------------------------------------------------------------------------
bool
Bif_OPER1_EACH::eoc_ALB(Token & token)
{
EOC_arg * arg = Workspace::SI_top()->remove_eoc_handlers();
EOC_arg * next = arg->next;
EACH_ALB & _arg = arg->u.u_EACH_ALB;

   if (!!arg->Z)   // LO with result, maybe successful
      {
       if (token.get_Class() != TC_VALUE)  return false;   // LO error: stop it

       Value_P vZ = token.get_apl_val();

       if (!_arg.sub)
          arg->Z->next_ravel()->init_from_value(vZ, arg->Z.getref(), LOC);
       else if (vZ->is_simple_scalar())
          arg->Z->next_ravel()->init(vZ->get_ravel(0), arg->Z.getref());
       else
          new (arg->Z->next_ravel())   PointerCell(vZ, arg->Z.getref());
      }
   else        // LO without result, maybe successful
      {
        if (token.get_tag() != TOK_VOID)    return false;   // LO error: stop it
      }

   if (_arg.z < (_arg.count - 1))   Workspace::pop_SI(LOC);

   copy_1(token, finish_eval_ALB(*arg), LOC);
   if (token.get_tag() == TOK_SI_PUSHED)   return true;   // continue

   delete arg;
   Workspace::SI_top()->set_eoc_handlers(next);
   if (next)   return next->handler(token);

   return false;   // stop it
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_EACH::eval_LB(Token & _LO, Value_P B)
{
   // monadic EACH

Function * LO = _LO.get_function();
   Assert1(LO);

   if (B->is_scalar())   return LO->eval_B(B);

   if (B->is_empty())
      {
        if (!LO->has_result())   return Token(TOK_VOID);

        Value_P Fill_B = Bif_F12_TAKE::first(B);
        Token tZ = LO->eval_fill_B(Fill_B);
        Value_P Z = tZ.get_apl_val();
        Z->set_shape(B->get_shape());
        return Token(TOK_APL_VALUE1, Z);
      }

EOC_arg arg(B);
EACH_LB & _arg = arg.u.u_EACH_LB;

   _arg.LO = LO;
   _arg.cB = &B->get_ravel(0);
   if (LO->has_result())   arg.Z = Value_P(B->get_shape(), LOC);

   _arg.count = B->element_count();

   _arg.z = -1;
   return finish_eval_LB(arg);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_EACH::finish_eval_LB(EOC_arg & arg)
{
EACH_LB & _arg = arg.u.u_EACH_LB;

   while (++_arg.z < _arg.count)
      {
        if (_arg.LO->get_fun_valence() == 0)
           {
             // we alloe niladic functions N so that one can loop over them with
             // N ¨ 1 2 3 4
             //
             Token result = _arg.LO->eval_();

             if (result.get_tag() == TOK_SI_PUSHED)
                {
                  // LO was a user defined function or ⍎
                  //
                  if (_arg.z)   // subsequent call
                     Workspace::SI_top()->move_eoc_handler(eoc_LB, &arg, LOC);
                  else           // first call
                     Workspace::SI_top()->add_eoc_handler(eoc_LB, arg, LOC);
                  return result;   // continue in user defined function...
                }

             // if LO was a primitive function, then result may be a value.
             // if LO was a user defined function then result may be
             // TOK_SI_PUSHED in both cases result could be TOK_ERROR.
             //
             if (result.get_Class() == TC_VALUE)
                {
                  Value_P vZ = result.get_apl_val();

                  if (!_arg.sub)
                     arg.Z->next_ravel()->init_from_value(vZ, arg.Z.getref(), LOC);
                  else if (vZ->is_simple_scalar())
                     arg.Z->next_ravel()->init(vZ->get_ravel(0), arg.Z.getref());
                  else
                     new (arg.Z->next_ravel())   PointerCell(vZ, arg.Z.getref());

                  continue;   // next z
           }

             if (result.get_tag() == TOK_VOID)   continue;   // next z

             if (result.get_tag() == TOK_ERROR)   return result;

             Q1(result);   FIXME;
           }
        else
           {
             Value_P LO_B;         // right argument of LO;

             if (_arg.cB->is_pointer_cell())
                {
                  LO_B = _arg.cB++->get_pointer_value();
                  _arg.sub = true;
                }
             else
                {
                  LO_B = Value_P(LOC);
   
                  LO_B->get_ravel(0).init(*_arg.cB++, LO_B.getref());
                  LO_B->set_complete();
                  _arg.sub = false;
                }

             Token result = _arg.LO->eval_B(LO_B);

             if (result.get_tag() == TOK_SI_PUSHED)
                {
                  // LO was a user defined function or ⍎
                  //
                  if (_arg.z)   // subsequent call
                     Workspace::SI_top()->move_eoc_handler(eoc_LB, &arg, LOC);
                  else           // first call
                     Workspace::SI_top()->add_eoc_handler(eoc_LB, arg, LOC);
                  return result;   // continue in user defined function...
                }

             // if LO was a primitive function, then result may be a value.
             // if LO was a user defined function then result may be
             // TOK_SI_PUSHED in both cases result could be TOK_ERROR.
             //
             if (result.get_Class() == TC_VALUE)
                {
                  Value_P vZ = result.get_apl_val();

                  if (!_arg.sub)
                     arg.Z->next_ravel()->init_from_value(vZ, arg.Z.getref(),
                                                          LOC);
                  else if (vZ->is_simple_scalar())
                     arg.Z->next_ravel()->init(vZ->get_ravel(0),arg.Z.getref());
                  else
                     new (arg.Z->next_ravel())   PointerCell(vZ,arg.Z.getref());

                  continue;   // next z
                }

             if (result.get_tag() == TOK_VOID)   continue;   // next z

             if (result.get_tag() == TOK_ERROR)   return result;

             Q1(result);   FIXME;
           }
      }

   if (!arg.Z)   return Token(TOK_VOID);   // LO without result

   arg.Z->set_default(*arg.B.get());
   arg.Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, arg.Z);
}
//-----------------------------------------------------------------------------
bool
Bif_OPER1_EACH::eoc_LB(Token & token)
{
EOC_arg * arg = Workspace::SI_top()->remove_eoc_handlers();
EOC_arg * next = arg->next;
EACH_LB & _arg = arg->u.u_EACH_LB;

   if (!!arg->Z)   // LO with result, maybe successful
      {
       if (token.get_Class() != TC_VALUE)  return false;   // LO error: stop it

        Value_P vZ = token.get_apl_val();

        if (!_arg.sub)
           arg->Z->next_ravel()->init_from_value(vZ, arg->Z.getref(), LOC);
        else if (vZ->is_simple_scalar())
           arg->Z->next_ravel()->init(vZ->get_ravel(0), arg->Z.getref());
        else
           new (arg->Z->next_ravel())   PointerCell(vZ, arg->Z.getref());
      }
   else        // LO without result, maybe successful
      {
       if (token.get_tag() != TOK_VOID)    return false;   // LO error: stop it
      }

   if (_arg.z < (_arg.count - 1))   Workspace::pop_SI(LOC);

   copy_1(token, finish_eval_LB(*arg), LOC);
   if (token.get_tag() == TOK_SI_PUSHED)   return true;   // continue

   delete arg;
   Workspace::SI_top()->set_eoc_handlers(next);
   if (next)   return next->handler(token);

   return false;   // stop it
}
//-----------------------------------------------------------------------------

