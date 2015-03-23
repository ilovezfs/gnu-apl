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

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ComplexCell.hh"
#include "Error.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "UTF8_string.hh"
#include "Value.icc"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
bool
FloatCell::equal(const Cell & A, APL_Float qct) const
{
   if (!A.is_numeric())       return false;
   if (A.is_complex_cell())   return A.equal(*this, qct);
   return tolerantly_equal(A.get_real_value(), get_real_value(), qct);
}
//-----------------------------------------------------------------------------
bool
FloatCell::greater(const Cell & other) const
{
const APL_Float this_val  = get_real_value();

   switch(other.get_cell_type())
      {
        case CT_INT:
             {
               const APL_Integer other_val = other.get_int_value();
               if (this_val == other_val)   return this > &other;
               return this_val > other_val;
             }

        case CT_FLOAT:
             {
               const APL_Float other_val = other.get_real_value();
               if (this_val == other_val)   return this > &other;
               return this_val > other_val;
             }

        case CT_CHAR:    return true;
        case CT_COMPLEX: break;
        case CT_POINTER: return false;
        case CT_CELLREF: DOMAIN_ERROR;
        default:         Assert(0 && "Bad celltype");
      }

const Comp_result comp = compare(other);
   if (comp == COMP_EQ)   return this > &other;
   return (comp == COMP_GT);
}
//-----------------------------------------------------------------------------
bool
FloatCell::get_near_bool(APL_Float qct)  const
{
   if (value.fval > qct)   // maybe 1
      {
        if (value.fval > (1.0 + qct))   DOMAIN_ERROR;
        if (value.fval < (1.0 - qct))   DOMAIN_ERROR;
        return true;
      }

   // maybe 0. We already know that value.fval ≤ qct
   //
   if (value.fval < -qct)   DOMAIN_ERROR;
   return false;
}
//-----------------------------------------------------------------------------
void
FloatCell::demote_float_to_int(APL_Float qct)
{
   if (Cell::is_near_int(value.fval, qct))
      {
        if (value.fval < 0)
           {
             APL_Integer ival(0.3 - value.fval);
             new (this) IntCell(-ival);
           }
        else
           {
             APL_Integer ival(0.3 + value.fval);
             new (this) IntCell(ival);
           }
      }
}
//-----------------------------------------------------------------------------
Comp_result
FloatCell::compare(const Cell & other) const
{
   if (other.is_integer_cell())   // integer
      {
        const APL_Float qct = Workspace::get_CT();
        if (equal(other, qct))   return COMP_EQ;
        return (value.fval <= other.get_int_value())  ? COMP_LT : COMP_GT;
      }

   if (other.is_float_cell())
      {
        const APL_Float qct = Workspace::get_CT();
        if (equal(other, qct))   return COMP_EQ;
        return (value.fval <= other.get_real_value()) ? COMP_LT : COMP_GT;
      }

   if (other.is_complex_cell())   return (Comp_result)(-other.compare(*this));

   DOMAIN_ERROR;
}
//-----------------------------------------------------------------------------
// monadic built-in functions...
//-----------------------------------------------------------------------------
ErrorCode
FloatCell::bif_factorial(Cell * Z) const
{
const APL_Float qct = Workspace::get_CT();

   if (is_near_int(qct))
      {
        const APL_Integer iv = get_near_int(qct);
        IntCell icell(iv);
        icell.bif_factorial(Z);
        return E_NO_ERROR;
      }

   // max N! that fits into double is about 170
   //
   if (value.fval > 170)   return E_DOMAIN_ERROR;

const double arg = value.fval + 1.0;
   new (Z) FloatCell(tgamma(arg));
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
FloatCell::bif_conjugate(Cell * Z) const
{
   if (is_near_int(Workspace::get_CT()))
      new (Z) IntCell(get_checked_near_int());
   else
      new (Z) FloatCell(value.fval);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
FloatCell::bif_negative(Cell * Z) const
{
   new (Z) FloatCell(- value.fval);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
FloatCell::bif_direction(Cell * Z) const
{
   // bif_direction does NOT use ⎕CT
   //
   if      (value.fval == 0)   new (Z) IntCell( 0);
   else if (value.fval < 0)    new (Z) IntCell(-1);
   else                        new (Z) IntCell( 1);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
FloatCell::bif_magnitude(Cell * Z) const
{
   if (value.fval >= 0.0)   new (Z) FloatCell( value.fval);
   else                     new (Z) FloatCell(-value.fval);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
FloatCell::bif_reciprocal(Cell * Z) const
{
const double z = 1.0/value.fval;
   if (!isfinite(z))   return E_DOMAIN_ERROR;;

   new (Z) FloatCell(1.0/value.fval);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
FloatCell::bif_roll(Cell * Z) const
{
const APL_Integer qio = Workspace::get_IO();
   if (!is_near_int(Workspace::get_CT()))   return E_DOMAIN_ERROR;

const APL_Integer set_size = get_checked_near_int();
   if (set_size <= 0)   return E_DOMAIN_ERROR;

const uint64_t rnd = Workspace::get_RL(set_size);
   new (Z) IntCell(qio + (rnd % set_size));
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
FloatCell::bif_pi_times(Cell * Z) const
{
   new (Z) FloatCell(M_PI * value.fval);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
FloatCell::bif_pi_times_inverse(Cell * Z) const
{
   new (Z) FloatCell(value.fval / M_PI);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
FloatCell::bif_ceiling(Cell * Z) const
{
   // we want values slightly above an int to be rounded down rather than up
   //
const APL_Float qct = Workspace::get_CT();
const double d_ret = ceil(value.fval - Workspace::get_CT());

   if (Cell::is_near_int(d_ret, qct))   new (Z) IntCell(llrint(d_ret));
   else                                 new (Z) FloatCell(d_ret);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
FloatCell::bif_floor(Cell * Z) const
{
   // we want values slightly below an int to be rounded up rather than down
   //
const APL_Float qct = Workspace::get_CT();
const double d_ret = floor(value.fval + qct);

   if (Cell::is_near_int(d_ret, qct))   new (Z) IntCell(llrint(d_ret)); 
   else                                 new (Z) FloatCell(d_ret);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
FloatCell::bif_exponential(Cell * Z) const
{
   // e to the B-th power
   //
   new (Z) FloatCell(exp(value.fval));
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
FloatCell::bif_nat_log(Cell * Z) const
{
   if (value.fval > 0)
      {
        new (Z) FloatCell(log(value.fval));
      }
   else if (value.fval < 0)
      {
        const double real = log(-value.fval);
        const double imag = M_PI;   // argz(-1)
        new (Z) ComplexCell(real, imag);
      }
   else
      {
        return E_DOMAIN_ERROR;
      }
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
// dyadic built-in functions...
//
// where possible a function with non-real A is delegated to the corresponding
// member function of A. For numeric cells that is the ComplexCell function
// and otherwise the default function (that returns E_DOMAIN_ERROR.
//
//-----------------------------------------------------------------------------
ErrorCode
FloatCell::bif_add(Cell * Z, const Cell * A) const
{
   if (A->is_real_cell())
      {
        new (Z) FloatCell(A->get_real_value() + get_real_value());
        Z->demote_float_to_int(Workspace::get_CT());
        return E_NO_ERROR;
      }

   // delegate to A
   //
   return A->bif_add(Z, this);
}
//-----------------------------------------------------------------------------
ErrorCode
FloatCell::bif_subtract(Cell * Z, const Cell * A) const
{
   if (A->is_real_cell())
      {
        new (Z) FloatCell(A->get_real_value() - get_real_value());
        Z->demote_float_to_int(Workspace::get_CT());
        return E_NO_ERROR;
      }

   // delegate to A
   //
   return A->bif_subtract(Z, this);
}
//-----------------------------------------------------------------------------
ErrorCode
FloatCell::bif_multiply(Cell * Z, const Cell * A) const
{
   if (!A->is_numeric())   return E_DOMAIN_ERROR;

const APL_Float ar = A->get_real_value();
const APL_Float ai = A->get_imag_value();

   if (ai == 0.0)   // real result
      {
        const APL_Float z = ar * value.fval;
        if (!isfinite(z))   return E_DOMAIN_ERROR;;
        new (Z) FloatCell(z);
        return E_NO_ERROR;
      } 

   // complex result
   //
const APL_Complex a(ar, ai);
const APL_Complex z = a * value.fval;
   if (!isfinite(z.real()))   return E_DOMAIN_ERROR;;
   if (!isfinite(z.imag()))   return E_DOMAIN_ERROR;;
   new (Z) ComplexCell(z);
   return E_NO_ERROR;
}     
//-----------------------------------------------------------------------------
ErrorCode
FloatCell::bif_divide(Cell * Z, const Cell * A) const
{
   if (!A->is_numeric())   return E_DOMAIN_ERROR;

const APL_Float ar = A->get_real_value();
const APL_Float ai = A->get_imag_value();

   if (value.fval == 0.0)   // A ÷ 0
      {
         if (ar != 0.0)   return E_DOMAIN_ERROR;
         if (ai != 0.0)   return E_DOMAIN_ERROR;

         return IntCell::z1(Z);   // 0÷0 is 1 in APL
      }


   if (ai == 0.0)   // real result
      {
        const APL_Float real = ar / value.fval ;
        new (Z) FloatCell(real);
         return E_NO_ERROR;
      }

   // complex result
   //
const APL_Complex a(ar, ai);
const APL_Complex z = a / value.fval;
   if (!isfinite(z.real()))   return E_DOMAIN_ERROR;;
   if (!isfinite(z.imag()))   return E_DOMAIN_ERROR;;
   new (Z) ComplexCell(z);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
FloatCell::bif_power(Cell * Z, const Cell * A) const
{
   // some A to the real B-th power
   //
   if (!A->is_numeric())   return E_DOMAIN_ERROR;

const APL_Float ar = A->get_real_value();
const APL_Float ai = A->get_imag_value();

   // 1. A == 0
   //
   if (ar == 0.0 && ai == 0.0)
       {
         if (value.fval == 0.0)   return IntCell::z1(Z);   // 0⋆0 is 1
         if (value.fval > 0)      return IntCell::z0(Z);   // 0⋆N is 0
         return E_DOMAIN_ERROR;;                           // 0⋆¯N = 1÷0
       }

   // 2. real A > 0   (real result)
   //
   if (ai == 0.0)
      {
        if (ar  == 1.0)   return IntCell::z1(Z);   // 1⋆b = 1
        if (ar > 0)
           {
             const APL_Float z = pow(ar, value.fval);
             if (!isfinite(z))   return E_DOMAIN_ERROR;;
             new (Z) FloatCell(z);
             return E_NO_ERROR;
           }

        /* fall through */
      }

   // 3. complex result
   //
   {
     const APL_Complex a(ar, ai);
     const APL_Complex z = pow(a, value.fval);
     if (!isfinite(z.real()))   return E_DOMAIN_ERROR;;
     if (!isfinite(z.imag()))   return E_DOMAIN_ERROR;;

     new (Z) ComplexCell(z);
     return E_NO_ERROR;
   }
}
//-----------------------------------------------------------------------------
ErrorCode
FloatCell::bif_residue(Cell * Z, const Cell * A) const
{
   if (A->is_complex_cell())
      {
        ComplexCell B(get_real_value(), 0);
        return B.bif_residue(Z, A);
      }

   if (A->is_numeric())
      {
        const APL_Float qct = Workspace::get_CT();

        // if A is zero , return B
        //
        if (A->is_near_zero(0))
           {
             new (Z) FloatCell(get_real_value());
             return E_NO_ERROR;
           }

        // if ⎕CT != 0 and B÷A is close to an integer within ⎕CT then return 0.
        // Otherwise return B mod A
        //
        const APL_Float remainder = fmod(get_real_value(), A->get_real_value()); 
        if (qct == 0.0)   // ⎕CT is 0
           {
             new (Z) FloatCell(remainder);
             return E_NO_ERROR;
           }

        if (Cell::is_near_zero(remainder, qct))
           {
             new (Z) IntCell(0);
             return E_NO_ERROR;
           }

        if (Cell::is_near_zero(A->get_real_value() - remainder, qct))
           {
             new (Z) IntCell(0);
             return E_NO_ERROR;
           }

        new (Z) FloatCell(remainder);
        return E_NO_ERROR;
      }

   return E_DOMAIN_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
FloatCell::bif_maximum(Cell * Z, const Cell * A) const
{
   if (A->is_integer_cell())
      {
         const APL_Integer a = A->get_int_value();
         if (APL_Float(a) >= value.fval)   new (Z) IntCell(a);
         else                              new (Z) FloatCell(value.fval);
         return E_NO_ERROR;
      }

   if (A->is_float_cell())
      {
         const APL_Float a = A->get_real_value();
         if (a >= value.fval)   new (Z) FloatCell(a);
         else                   new (Z) FloatCell(value.fval);
         return E_NO_ERROR;
      }

   // delegate to A
   //
   return A->bif_maximum(Z, this);
}
//-----------------------------------------------------------------------------
ErrorCode
FloatCell::bif_minimum(Cell * Z, const Cell * A) const
{
   if (A->is_integer_cell())
      {
         const APL_Integer a = A->get_int_value();
         if (APL_Float(a) <= value.fval)   new (Z) IntCell(a);
         else                              new (Z) FloatCell(value.fval);
         return E_NO_ERROR;
      }

   if (A->is_float_cell())
      {
         const APL_Float a = A->get_real_value();
         if (a <= value.fval)   new (Z) FloatCell(a);
         else                   new (Z) FloatCell(value.fval);
         return E_NO_ERROR;
      }

   // delegate to A
   //
   return A->bif_minimum(Z, this);
}
//=============================================================================
// throw/nothrow boundary. Functions above MUST NOT (directly or indirectly)
// throw while funcions below MAY throw.
//=============================================================================
PrintBuffer
FloatCell::character_representation(const PrintContext & pctx) const
{
bool scaled = pctx.get_scaled();   // may be changed by print function
UCS_string ucs = UCS_string(value.fval, scaled, pctx);

ColInfo info;
   info.flags |= CT_FLOAT;
   if (scaled)   info.flags |= real_has_E;

   // assume integer.
   //
int int_fract = ucs.size();
   info.real_len = ucs.size();
   info.int_len = ucs.size();
   loop(u, ucs.size())
      {
        if (ucs[u] == UNI_ASCII_FULLSTOP)
           {
             info.int_len = u;
             if (!pctx.get_scaled())   break;
             continue;
           }

        if (ucs[u] == UNI_ASCII_E)
           {
             if (info.int_len > u)   info.int_len = u;
             int_fract = u;
             break;
           }
      }

   info.fract_len = int_fract - info.int_len;

   return PrintBuffer(ucs, info);
}
//-----------------------------------------------------------------------------
bool
FloatCell::is_big(APL_Float val, int quad_pp)
{
static const double big[MAX_Quad_PP + 1] =
{
                  1ULL, // not used since MIN_Quad_PP == 1
                 10ULL,
                100ULL,
               1000ULL,
              10000ULL,
             100000ULL,
            1000000ULL,
           10000000ULL,
          100000000ULL,
         1000000000ULL,
        10000000000ULL,
       100000000000ULL,
      1000000000000ULL,
     10000000000000ULL,
    100000000000000ULL,
   1000000000000000ULL,
  10000000000000000ULL,
};

   return val >= big[quad_pp] || val <= -big[quad_pp];
}
//-----------------------------------------------------------------------------
bool
FloatCell::need_scaling(APL_Float val, int quad_pp)
{
   // A number is printed in scaled format if (see lrm pp. 11-13) either:
   //
   // (1) its integer part is longer that quad-PP, or

   // (2a) is non-zero, and
   // (2b) its integer part is 0, and
   // (2c) its fractional part starts with at least 5 zeroes.
   //
   if (val < 0)   val = - val;   // simplify comparisons

   if (is_big(val, quad_pp))        return true;    // case 1.

   if (val == 0.0)                  return false;   // not 2a.

   if (val < 0.000001)              return true;    // case 2

   return false;
}
//-----------------------------------------------------------------------------
void
FloatCell::map_FC(UCS_string & ucs)
{
   loop(u, ucs.size())
      {
        switch(ucs[u])
           {
             case UNI_ASCII_FULLSTOP: ucs[u] = Workspace::get_FC(0);   break;
             case UNI_ASCII_COMMA:    ucs[u] = Workspace::get_FC(1);   break;
             case UNI_OVERBAR:        ucs[u] = Workspace::get_FC(5);   break;
             default:                 break;
           }
      }
}
//-----------------------------------------------------------------------------
