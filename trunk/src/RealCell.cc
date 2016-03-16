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

#include <math.h>

#include "Value.icc"
#include "ComplexCell.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "RealCell.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
ErrorCode
RealCell::bif_logarithm(Cell * Z, const Cell * A) const
{
   if (!A->is_numeric())          return E_DOMAIN_ERROR;
   if (get_real_value() == 0.0)   return E_DOMAIN_ERROR;

   // A⍟B is defined as: (⍟B)÷(⍟A)
   // if A=1 then ⍟A is 0 which causes division by 0 unless ⍟B is 0 as well.
   //
   if (A->is_near_one())   // ⍟A is 0
      {
         if (!this->is_near_one())   return E_DOMAIN_ERROR;

         // both ⍟A and ⍟B are 0, so we get 0÷0 (= 1 in APL)
         //
         new (Z) IntCell(1);
         return E_NO_ERROR;
      }

   if (A->is_real_cell())
      {
        if (get_real_value() < 0)
           return ComplexCell::zv(Z,
               log(get_complex_value()) / log(A->get_complex_value()));

        return FloatCell::zv(Z,
               log(get_real_value()) / log(A->get_real_value()));
      }

   if (A->is_complex_cell())
      {
        return ComplexCell::zv(Z,
               log(get_complex_value()) / log(A->get_complex_value()));
      }

   return E_DOMAIN_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
RealCell::bif_circle_fun(Cell * Z, const Cell * A) const
{
const APL_Integer fun = A->get_near_int();
   new (Z) FloatCell(0);   // prepare for DOMAIN ERROR

const ErrorCode ret = do_bif_circle_fun(Z, fun);
   if (!Z->is_finite())   return E_DOMAIN_ERROR;
   return ret;
}
//-----------------------------------------------------------------------------
ErrorCode
RealCell::bif_circle_fun_inverse(Cell * Z, const Cell * A) const
{
const APL_Integer fun = A->get_near_int();
   new (Z) FloatCell(0);   // prepare for DOMAIN ERROR

ErrorCode ret = E_DOMAIN_ERROR;
   switch(fun)
      {
        case 1: case -1:
        case 2: case -2:
        case 3: case -3:
        case 4: case -4:
        case 5: case -5:
        case 6: case -6:
        case 7: case -7:
                ret = do_bif_circle_fun(Z, -fun);
                if (!Z->is_finite())   return E_DOMAIN_ERROR;
                return ret;

        case -10:  // +A (conjugate) is self-inverse
                 ret = do_bif_circle_fun(Z, fun);
                if (!Z->is_finite())   return E_DOMAIN_ERROR;
                return ret;

        default: return E_DOMAIN_ERROR;
      }

   // not reached
   return E_DOMAIN_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
RealCell::do_bif_circle_fun(Cell * Z, int fun) const
{
const APL_Float b = get_real_value();

   switch(fun)
      {
        case -12: ComplexCell(0, b).bif_exponential(Z);   return E_NO_ERROR;
        case -11: return ComplexCell::zv(Z, 0.0, b );
        case -10: return FloatCell::zv(Z,       b );
        case  -9: return FloatCell::zv(Z,       b );
        case  -8: return ComplexCell::do_bif_circle_fun(Z, -8, b);

        case  -7: if (b > -1.0 && b < 1.0)   return FloatCell::zv(Z, atanh(b));
                  if (b == -1.0 || b == 1.0)   return E_DOMAIN_ERROR;
                  return ComplexCell::do_bif_circle_fun(Z, -7, b);

        case  -6: if (b > 1.0)   return FloatCell::zv(Z, acosh(b));
                  return ComplexCell::do_bif_circle_fun(Z, -6, b);

        case  -5: return FloatCell::zv(Z, asinh(b));
        case  -4: if (b >= -1)   return FloatCell::zv(Z, sqrt(b*b - 1.0));
                  return ComplexCell::do_bif_circle_fun(Z, -4, b);
        case  -3: return FloatCell::zv(Z, atan (b));
        case  -2: if (b >= -1.0 && b <= 1.0)  return FloatCell::zv(Z, acos (b));
                  return ComplexCell::do_bif_circle_fun(Z, -2, b);
        case  -1: if (b >= -1.0 && b <= 1.0)  return FloatCell::zv(Z, asin (b));
                  return ComplexCell::do_bif_circle_fun(Z, -1, b);
        case   0: if (b*b < 1)   return FloatCell::zv(Z, sqrt (1 - b*b));
                  return ComplexCell::do_bif_circle_fun(Z, 0, b);
        case   1: return FloatCell::zv(Z,  sin (b));
        case   2: return FloatCell::zv(Z,  cos (b));
        case   3: return FloatCell::zv(Z,  tan (b));
        case   4: return FloatCell::zv(Z, sqrt (1 + b*b));
        case   5: return FloatCell::zv(Z,  sinh(b));
        case   6: return FloatCell::zv(Z,  cosh(b));
        case   7: return FloatCell::zv(Z,  tanh(b));
        case   8: return ComplexCell::do_bif_circle_fun(Z, 8, b);
        case   9: return FloatCell::zv(Z, b );
        case  10: return FloatCell::zv(Z, (b < 0) ? -b : b);
        case  11: return FloatCell::zv(Z, 0.0 );
        case  12: return FloatCell::zv(Z, (b < 0) ? M_PI : 0.0);
      }

   // invalid fun
   //
   return E_DOMAIN_ERROR;
}
//-----------------------------------------------------------------------------
