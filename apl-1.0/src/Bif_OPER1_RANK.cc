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

#include "Bif_OPER1_RANK.hh"
#include "IntCell.hh"
#include "Workspace.hh"

Bif_OPER1_RANK     Bif_OPER1_RANK::fun;

//-----------------------------------------------------------------------------
Token
Bif_OPER1_RANK::eval_LB(Token & _LO, Value_P B)
{
Function * LO = _LO.get_function();

   if (!LO->has_result())   DOMAIN_ERROR;

   // the rank operator always needs an axis. We convert the
   // weird Sharp syntax of A ⍤ j B into A ⍤[j] B
   //
   // B is expected to be "glued" by the prefix parser, which should result
   // in a 2, 3, or 4 element vector  j1 ⊂B  or  j1 j2 ⊂B  or  j1 j2 j3 ⊂B
   //
   if (B->get_rank() != 1)       RANK_ERROR;

const int axes_count = B->element_count() - 1;

   if (axes_count < 1)   AXIS_ERROR;
   if (axes_count > 3)   AXIS_ERROR;

Value_P arg  = B->get_ravel(axes_count).get_pointer_value();
Value_P X = new Value(axes_count, LOC);
   loop(a, axes_count)   
   new (&X->get_ravel(a))   IntCell(B->get_ravel(a).get_int_value());

Token ret = eval_LXB(_LO, X, arg);
   arg->erase(LOC);
   X->erase(LOC);
   return ret;
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_RANK::eval_LXB(Token & _LO, Value_P X, Value_P B)
{
Function * LO = _LO.get_function();

   if (!LO->has_result())   DOMAIN_ERROR;

Rank rk_A_low = -1;   // indicate monadic
Rank rk_B_low = B->get_rank();
   compute_ranks(X, rk_A_low, rk_B_low);

   // split shape of B into high and low shapes.
   //
RANK_LXB arg;
   arg.get_sh_B_low() = B->get_shape().low_shape(rk_B_low);
   arg.get_sh_B_high() = B->get_shape().high_shape(B->get_rank() - rk_B_low);
   new (&arg.get_sh_Z_max_low()) Shape;
   arg.ec_high = arg.get_sh_B_high().element_count();
   arg.ec_B_low = arg.get_sh_B_low().element_count();
   arg.cB = &B->get_ravel(0);
   arg.LO = LO;
   arg.ZZ = new Value_P[arg.ec_high];
   arg.how = 0;

   return finish_eval_LXB(arg);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_RANK::finish_eval_LXB(RANK_LXB & arg)
{
   if (arg.how == 1)   goto how_1;

how_0:
   Assert1(arg.how == 0);
   arg.h = 0;

loop_h:
   {
     Value_P BB = new Value(arg.get_sh_B_low(), LOC);
     Assert1(arg.ec_B_low == arg.get_sh_B_low().element_count());
     loop(l, arg.ec_B_low)   BB->get_ravel(l).init(*arg.cB++);

     Token result = arg.LO->eval_B(BB);   // erases BB
     BB->erase(LOC);
     if (result.get_tag() == TOK_SI_PUSHED)
        {
          // LO was a user defined function or ⍎
          //
          arg.how = 1;
          Workspace::the_workspace->SI_top()->set_eoc_handler(eoc_LXB);
          Workspace::the_workspace->SI_top()->get_eoc_arg()._RANK_LXB() = arg;
          return result;   // continue in user defined function...
        }

     if (result.get_Class() == TC_VALUE)
        {
          arg.ZZ[arg.h] = result.get_apl_val();

          // adjust result rank to ZZ if needed
          //
          arg.get_sh_Z_max_low().expand(arg.ZZ[arg.h]->get_shape());

          goto next_h;
       }

     if (result.get_tag() == TOK_ERROR)   return result;

     Q(result);   FIXME;
   }

how_1:
next_h:
   if (++arg.h < arg.ec_high)   goto loop_h;

const ShapeItem ec_Z_max_low = arg.get_sh_Z_max_low().element_count();
Shape sh_Z = arg.get_sh_B_high() + arg.get_sh_Z_max_low();
Value_P Z = new Value(sh_Z, LOC);
Cell * cZ = &Z->get_ravel(0);

   loop(hh, arg.ec_high)
      {
        // sh_Z_max_low ↑ ZZ  if neccessary
        //
        if (arg.get_sh_Z_max_low() != arg.ZZ[hh]->get_shape())
           {
             Assert(ec_Z_max_low >= arg.ZZ[hh]->element_count());
             Token zz = Bif_F12_TAKE::fun.do_take(arg.get_sh_Z_max_low(),
                                                  arg.ZZ[hh]);
             arg.ZZ[hh]->erase(LOC);
             arg.ZZ[hh] = zz.get_apl_val();
           }

        // copy partial result into final result.
        //
        Cell * cZZ = &arg.ZZ[hh]->get_ravel(0);
        loop(z, ec_Z_max_low)
           {
             cZ++->init(*cZZ);
             new (cZZ++) IntCell(0);
           }
        arg.ZZ[hh]->erase(LOC);
      }

   delete arg.ZZ;
   return CHECK(Z, LOC);
}
//-----------------------------------------------------------------------------
bool
Bif_OPER1_RANK::eoc_LXB(Token & token, _EOC_arg & _arg)
{
RANK_LXB arg = _arg._RANK_LXB();

   if (token.get_Class() != TC_VALUE)  return false;   // stop it

   // the user defined function has returned a value. Store it.
   //
   {
     arg.ZZ[arg.h] = token.get_apl_val();
     // adjust result rank to ZZ if needed
     //
     arg.get_sh_Z_max_low().expand(arg.ZZ[arg.h]->get_shape());
   }

   if (arg.h < (arg.ec_high - 1))   Workspace::the_workspace->pop_SI(LOC);

   token = finish_eval_LXB(arg);
   if (token.get_tag() == TOK_SI_PUSHED)   return true;   // continue

   return false;   // stop it
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_RANK::eval_ALB(Value_P A, Token & _LO, Value_P B)
{
Function * LO = _LO.get_function();

   if (!LO->has_result())   DOMAIN_ERROR;

   // the rank operator always needs an axis. We convert the
   // weird Sharp syntax of A ⍤ j B into A ⍤[j] B
   //
   // B is expected to be "glued" by the prefix parser, which should
   // result in a 2, 3, or 4 element vector j1 ⊂B  or  j1 j2 ⊂B  or  j1 j2 j3 ⊂B
   //
   if (B->get_rank() != 1)       RANK_ERROR;

const int axes_count = B->element_count() - 1;

   if (axes_count < 1)   AXIS_ERROR;
   if (axes_count > 3)   AXIS_ERROR;

Value_P arg  = B->get_ravel(axes_count).get_pointer_value();
Value_P X = new Value(axes_count, LOC);
   loop(a, axes_count)   
   new (&X->get_ravel(a))   IntCell(B->get_ravel(a).get_int_value());

Token ret = eval_ALXB(A, _LO, X, arg);
   X->erase(LOC);
   return ret;
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_RANK::eval_ALXB(Value_P A, Token & _LO, Value_P X, Value_P B)
{
Function * LO = _LO.get_function();

   if (!LO->has_result())   DOMAIN_ERROR;

Rank rk_A_low = A->get_rank();   // not used
Rank rk_B_low = B->get_rank();
   compute_ranks(X, rk_A_low, rk_B_low);

Rank rk_A_high = A->get_rank() - rk_A_low;   // rk_A_high is y8
Rank rk_B_high = B->get_rank() - rk_B_low;   // rk_B_high is y9

   // if both high-ranks are 0, then return A LO B.
   //
   if (rk_A_high == 0 && rk_B_high == 0)   return LO->eval_AB(A, B);

   // split shapes of A1 and B1 into high and low shapes. Even though A1 and B1
   // have the same shape, rk_A_high and rk_B_high could be different, leading
   // to different split shapes for A1 and B1
   //
RANK_ALXB arg;
   arg.get_sh_A_low() = A->get_shape().low_shape(rk_A_low);
Shape sh_A_high = A->get_shape().high_shape(A->get_rank() - rk_A_low);
   arg.get_sh_B_low() = B->get_shape().low_shape(rk_B_low);
   arg.get_sh_B_high() = B->get_shape().high_shape(B->get_rank() - rk_B_low);
   new (&arg.get_sh_Z_max_low())   Shape;

   // a trick to actually avoid conforming A to B or B to A. If A or B
   // needs to be conformed, then we set the corresponding repeat_A or 
   // repeat_B to true and copy the same A or B again and again
   //
   arg.repeat_A = (rk_A_high == 0);
   arg.repeat_B = (rk_B_high == 0);

   if (arg.repeat_A)   // "conform" A to B
      {
         rk_A_high = rk_B_high;
         sh_A_high = arg.get_sh_B_high();
      }
   else if (arg.repeat_B)   // "conform" B to A
      {
         rk_B_high = rk_A_high;
         arg.get_sh_B_high() = sh_A_high;
      }
   else                       // check lengths
      {
        if (rk_A_high != rk_B_high)             RANK_ERROR;
        if (sh_A_high != arg.get_sh_B_high())   LENGTH_ERROR;
      }
 
   // at this point sh_A_high == sh_B_high, so we can use either of them
   // to compute ec_A_low
   //
   arg.ec_high = arg.get_sh_B_high().element_count();
   arg.ec_A_low = arg.get_sh_A_low().element_count();
   arg.ec_B_low = arg.get_sh_B_low().element_count();

   arg.ZZ = new Value_P[arg.ec_high];
   arg.A = A;
   arg.cA = &A->get_ravel(0);
   arg.LO = LO;
   arg.B = B;
   arg.cB = &B->get_ravel(0);

   arg.how = 0;
   return finish_eval_ALXB(arg);
}
//-----------------------------------------------------------------------------
Token
Bif_OPER1_RANK::finish_eval_ALXB(RANK_ALXB & arg)
{
   if (arg.how == 1)   goto how_1;

how_0:
   Assert1(arg.how == 0);
   arg.h = 0;

loop_h:
   {
     Value_P AA = new Value(arg.get_sh_A_low(), LOC);
     Value_P BB = new Value(arg.get_sh_B_low(), LOC);

     loop(l, arg.ec_A_low)   AA->get_ravel(l).init(*arg.cA++);
     loop(l, arg.ec_B_low)   BB->get_ravel(l).init(*arg.cB++);

     if (arg.repeat_A)   arg.cA = &arg.A->get_ravel(0);
     if (arg.repeat_B)   arg.cB = &arg.B->get_ravel(0);

     Token result = arg.LO->eval_AB(AA, BB);   // erases AA and BB
     AA->erase(LOC);
     BB->erase(LOC);
     if (result.get_tag() == TOK_SI_PUSHED)
        {
          // LO was a user defined function or ⍎
          //
          arg.how = 1;
          Workspace::the_workspace->SI_top()->set_eoc_handler(eoc_ALXB);
          Workspace::the_workspace->SI_top()->get_eoc_arg()._RANK_ALXB() = arg;
          return result;   // continue in user defined function...
        }

     if (result.get_Class() == TC_VALUE)
        {
          arg.ZZ[arg.h] = result.get_apl_val();

          // adjust result rank to ZZ if needed
          //
          arg.get_sh_Z_max_low().expand(arg.ZZ[arg.h]->get_shape());

          goto next_h;
       }

     if (result.get_tag() == TOK_ERROR)   return result;

     Q(result);   FIXME;
   }

how_1:
next_h:
   if (++arg.h < arg.ec_high)   goto loop_h;

const ShapeItem ec_Z_max_low = arg.get_sh_Z_max_low().element_count();
Shape sh_Z = arg.get_sh_B_high() + arg.get_sh_Z_max_low();
Value_P Z = new Value(sh_Z, LOC);
Cell * cZ = &Z->get_ravel(0);

   loop(hh, arg.ec_high)
      {
        // sh_Z_max_low ↑ ZZ  if neccessary
        //
        if (arg.get_sh_Z_max_low() != arg.ZZ[hh]->get_shape())
           {
             Assert(ec_Z_max_low >= arg.ZZ[hh]->element_count());
             Token zz = Bif_F12_TAKE::fun.do_take(arg.get_sh_Z_max_low(),
                                                  arg.ZZ[hh]);
             arg.ZZ[hh]->erase(LOC);
             arg.ZZ[hh] = zz.get_apl_val();
           }

        // copy partial result into final result.
        //
        Cell * cZZ = &arg.ZZ[hh]->get_ravel(0);
        loop(z, ec_Z_max_low)
           {
             cZ++->init(*cZZ);
             new (cZZ++) IntCell(0);
           }
        arg.ZZ[hh]->erase(LOC);
      }

   delete arg.ZZ;
   return CHECK(Z, LOC);
}
//-----------------------------------------------------------------------------
bool
Bif_OPER1_RANK::eoc_ALXB(Token & token, _EOC_arg & _arg)
{
RANK_ALXB arg = _arg._RANK_ALXB();

   if (token.get_Class() != TC_VALUE)  return false;   // stop it

   // the user defined function has returned a value. Store it.
   //
   {
     arg.ZZ[arg.h] = token.get_apl_val();

     // adjust result rank to ZZ if needed
     //
     arg.get_sh_Z_max_low().expand(arg.ZZ[arg.h]->get_shape());
   }

   if (arg.h < (arg.ec_high - 1))   Workspace::the_workspace->pop_SI(LOC);

   token = finish_eval_ALXB(arg);
   if (token.get_tag() == TOK_SI_PUSHED)   return true;   // continue

   return false;   // stop it
}
//-----------------------------------------------------------------------------
void
Bif_OPER1_RANK::compute_ranks(Value_P X,
                              /* in/out */  Rank & rk_A_low,
                              /* in/out */  Rank & rk_B_low)
{
const Rank rk_A = rk_A_low;
const Rank rk_B = rk_B_low;

   if (X == 0)            AXIS_ERROR;
   if (X->get_rank() > 1)   DOMAIN_ERROR;

const APL_Float qct = Workspace::get_CT();

   if (rk_A == -1)   // monadic
      {
        switch(X->element_count())
           {
             case 1:  rk_B_low = X->get_ravel(0).get_near_int(qct);   break;

             case 2:             X->get_ravel(0).get_near_int(qct);
                      rk_B_low = X->get_ravel(1).get_near_int(qct);   break;

             case 3:  rk_B_low = X->get_ravel(0).get_near_int(qct);
                                 X->get_ravel(1).get_near_int(qct);
                                 X->get_ravel(2).get_near_int(qct);   break;

             default: LENGTH_ERROR;
           }
      }
   else              // dyadic
      {
        switch(X->element_count())
           {
             case 1:  rk_A_low = X->get_ravel(0).get_near_int(qct);
                      rk_B_low = rk_A_low;                               break;

             case 2:  rk_A_low = X->get_ravel(0).get_near_int(qct);
                      rk_B_low = X->get_ravel(1).get_near_int(qct);   break;

             case 3:             X->get_ravel(0).get_near_int(qct);
                      rk_A_low = X->get_ravel(1).get_near_int(qct);
                      rk_B_low = X->get_ravel(2).get_near_int(qct);   break;

             default: LENGTH_ERROR;
           }
      }

   if (rk_A_low > rk_A)   rk_A_low = rk_A;
   if (rk_A_low < 0)      rk_A_low += rk_A;
   if (rk_A_low < 0)      rk_A_low = 0;

   if (rk_B_low > rk_B)   rk_B_low = rk_B;
   if (rk_B_low < 0)      rk_B_low += rk_B;
   if (rk_B_low < 0)      rk_B_low = 0;
}
//-----------------------------------------------------------------------------
