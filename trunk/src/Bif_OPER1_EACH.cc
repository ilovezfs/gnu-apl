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

#include "Bif_OPER1_EACH.hh"
#include "Macro.hh"
#include "PointerCell.hh"
#include "UserFunction.hh"
#include "Workspace.hh"

Bif_OPER1_EACH Bif_OPER1_EACH::_fun;

Bif_OPER1_EACH * Bif_OPER1_EACH::fun = &Bif_OPER1_EACH::_fun;

//-----------------------------------------------------------------------------
Token
Bif_OPER1_EACH::eval_ALB(Value_P A, Token & _LO, Value_P B)
{
   // dyadic EACH

   if (!(A->is_scalar() || B->is_scalar() || A->same_shape(*B)))
      {
        if (A->same_rank(*B))   LENGTH_ERROR;
        else                    RANK_ERROR;
      }

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

   if (LO->may_push_SI())   // user defined LO
      {
         // remember scalarity of A and B BEFORE disclosing them
         //
         const bool scalar_A = A->is_scalar();
         const bool scalar_B = B->is_scalar();

        // we will call a macro, so disclose nested scalars (if any) here...
        //
        if (scalar_A && A->get_ravel(0).is_pointer_cell())
           A = A->get_ravel(0).get_pointer_value();

        if (scalar_B && B->get_ravel(0).is_pointer_cell())
           B = B->get_ravel(0).get_pointer_value();

        Macro * macro = 0; 
        if (LO->has_result())
           if (scalar_A)
              if (scalar_B)   macro = Macro::Z__sA_LO_EACH_sB;
              else            macro = Macro::Z__sA_LO_EACH_vB;
           else
              if (scalar_B)   macro = Macro::Z__vA_LO_EACH_sB;
              else            macro = Macro::Z__vA_LO_EACH_vB;
        else
           if (scalar_A)
              if (scalar_B)   LO->eval_ALB(A, _LO, B);
              else                  macro = Macro::sA_LO_EACH_vB;
           else
              if (scalar_B)   macro = Macro::vA_LO_EACH_sB;
              else            macro = Macro::vA_LO_EACH_vB;

        return macro->eval_ALB(A, _LO, B);
      }

Value_P Z;
int dA = 1;
int dB = 1;
ShapeItem len_Z = 0;

   if (A->is_scalar())
      {
        len_Z = B->element_count();
        dA = 0;
        if (LO->has_result())   Z = Value_P(B->get_shape(), LOC);
      }
   else if (B->is_scalar())
      {
        dB = 0;
        len_Z = A->element_count();
        if (LO->has_result())   Z = Value_P(A->get_shape(), LOC);
      }
   else
      {
        len_Z = B->element_count();
        if (LO->has_result())   Z = Value_P(A->get_shape(), LOC);
      }

   loop(z, len_Z)
      {
        const Cell * cA = &A->get_ravel(dA * z);
        const Cell * cB = &B->get_ravel(dB * z);
        Value_P LO_A = cA->to_value(LOC);     // left argument of LO
        Value_P LO_B = cB->to_value(LOC);     // right argument of LO;

        Token result = LO->eval_AB(LO_A, LO_B);

        // if LO was a primitive function, then result may be a value.
        // if LO was a user defined function then result may be TOK_SI_PUSHED.
        // in both cases result could be TOK_ERROR.
        //
        if (result.get_Class() == TC_VALUE)
           {
             Value_P vZ = result.get_apl_val();

             Cell * cZ = Z->next_ravel();
             if (vZ->is_simple_scalar())
                cZ->init(vZ->get_ravel(0), Z.getref(), LOC);
             else
                new (cZ)   PointerCell(vZ, Z.getref());

            continue;   // next z
           }

        if (result.get_tag() == TOK_ERROR)   return result;

        Q1(result);   FIXME;
      }

   if (!Z)   return Token(TOK_VOID);   // LO without result

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_EACH::eval_LB(Token & _LO, Value_P B)
{
   // monadic EACH

Function * LO = _LO.get_function();
   Assert1(LO);

   if (B->is_empty())
      {
        if (!LO->has_result())   return Token(TOK_VOID);

        Value_P Fill_B = Bif_F12_TAKE::first(B);
        Token tZ = LO->eval_fill_B(Fill_B);
        Value_P Z = tZ.get_apl_val();
        Z->set_shape(B->get_shape());
        return Token(TOK_APL_VALUE1, Z);
      }

   if (LO->may_push_SI())   // user defined LO
      {
        if (LO->has_result())   return Macro::Z__LO_EACH_B->eval_LB(_LO, B);
        else                    return Macro::LO_EACH_B->eval_LB(_LO, B);
      }

const ShapeItem len_Z = B->element_count();
Value_P Z;
   if (LO->has_result())   Z = Value_P(B->get_shape(), LOC);

   loop (z, len_Z)
      {
        if (LO->get_fun_valence() == 0)
           {
             // we allow niladic functions N so that one can loop over them with
             // N ¨ 1 2 3 4
             //
             Token result = LO->eval_();

             if (result.get_Class() == TC_VALUE)
                {
                  Value_P vZ = result.get_apl_val();

                  Cell * cZ = Z->next_ravel();
                  if (vZ->is_simple_scalar())
                     cZ->init(vZ->get_ravel(0), Z.getref(), LOC);
                  else
                     new (cZ)  PointerCell(vZ, Z.getref());

                  continue;   // next z
           }

             if (result.get_tag() == TOK_VOID)   continue;   // next z

             if (result.get_tag() == TOK_ERROR)   return result;

             Q1(result);   FIXME;
           }
        else
           {
             Value_P LO_B;         // right argument of LO;
             const Cell * cB = &B->get_ravel(z);

             if (cB->is_pointer_cell())
                {
                  LO_B = cB->get_pointer_value();
                }
             else
                {
                  LO_B = Value_P(LOC);
   
                  LO_B->get_ravel(0).init(*cB, LO_B.getref(), LOC);
                  LO_B->set_complete();
                }

             Token result = LO->eval_B(LO_B);
             if (result.get_Class() == TC_VALUE)
                {
                  Value_P vZ = result.get_apl_val();

                  Cell * cZ = Z->next_ravel();
                  if (0)
                     cZ->init_from_value(vZ, Z.getref(), LOC);
                  else if (vZ->is_simple_scalar())
                     cZ->init(vZ->get_ravel(0), Z.getref(), LOC);
                  else
                     new (cZ)   PointerCell(vZ, Z.getref());

                  continue;   // next z
                }

             if (result.get_tag() == TOK_VOID)   continue;   // next z

             if (result.get_tag() == TOK_ERROR)   return result;

             Q1(result);   FIXME;
           }
      }

   if (!Z)   return Token(TOK_VOID);   // LO without result

   Z->set_default(*B.get());
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------

