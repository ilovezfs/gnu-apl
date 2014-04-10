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

#include <stdio.h>
#include <math.h>

#include "Value.hh"
#include "Error.hh"
#include "PointerCell.hh"
#include "ComplexCell.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "Workspace.hh"

/*
   general comment: for operations between an IntCell and a "higher" numeric
   Cell type (i.e. FloatCell or ComplexCell), we try to call the corresponding
   function (or its opposite) of the higher type.

   For example:
   INT ≠ FLOAT  --> FLOAT ≠ INT
   INT ≤ FLOAT  --> FLOAT ≥ INT   (≤ is the opposite of ≥)

 */
//-----------------------------------------------------------------------------
CellType
IntCell::get_cell_subtype() const
{
   if (value.ival < 0)   // negative integer (only fits in signed containers)
      {
        if (-value.ival <= 0x80)
           return (CellType)(CT_INT | CTS_S8 | CTS_S16 | CTS_S32 | CTS_S64);

        if (-value.ival <= 0x8000)
           return (CellType)(CT_INT | CTS_S16 | CTS_S32 | CTS_S64);

        if (-value.ival <= 0x80000000)
           return (CellType)(CT_INT | CTS_S32 | CTS_S64);

        return (CellType)(CT_INT | CTS_S64);
      }

   // positive integer
   //
   if (value.ival == 0)   // 0: bit (fits in all containers)
      return (CellType)(CT_INT | CTS_BIT |
                                 CTS_X8  | CTS_S8  | CTS_U8  |
                                 CTS_X16 | CTS_S16 | CTS_U16 |
                                 CTS_X32 | CTS_S32 | CTS_U32 |
                                 CTS_X64 | CTS_S64 | CTS_U64);

   if (value.ival == 1)   // 1: bit (fits in all containers)
      return (CellType)(CT_INT | CTS_BIT |
                                 CTS_X8  | CTS_S8  | CTS_U8  |
                                 CTS_X16 | CTS_S16 | CTS_U16 |
                                 CTS_X32 | CTS_S32 | CTS_U32 |
                                 CTS_X64 | CTS_S64 | CTS_U64);

   if (value.ival <= 0x7F)
      return (CellType)(CT_INT | CTS_X8  | CTS_S8  | CTS_U8  |
                                 CTS_X16 | CTS_S16 | CTS_U16 |
                                 CTS_X32 | CTS_S32 | CTS_U32 |
                                 CTS_X64 | CTS_S64 | CTS_U64);

   if (value.ival <= 0xFF)
      return (CellType)(CT_INT |                     CTS_U8  |
                                 CTS_X16 | CTS_S16 | CTS_U16 |
                                 CTS_X32 | CTS_S32 | CTS_U32 |
                                 CTS_X64 | CTS_S64 | CTS_U64);

   if (value.ival <= 0x7FFF)
      return (CellType)(CT_INT | CTS_X16 | CTS_S16 | CTS_U16 |
                                 CTS_X32 | CTS_S32 | CTS_U32 |
                                 CTS_X64 | CTS_S64 | CTS_U64);

   if (value.ival <= 0xFFFF)
      return (CellType)(CT_INT |                     CTS_U16 |
                                 CTS_X32 | CTS_S32 | CTS_U32 |
                                 CTS_X64 | CTS_S64 | CTS_U64);

   if (value.ival <= 0x7FFFFFFF)
      return (CellType)(CT_INT | CTS_X32 | CTS_S32 | CTS_U32 |
                                 CTS_X64 | CTS_S64 | CTS_U64);

   if (value.ival <= 0xFFFFFFFF)
      return (CellType)(CT_INT |                     CTS_U32 |
                                 CTS_X64 | CTS_S64 | CTS_U64);

   if (value.ival <= 0x7FFFFFFFFFFFFFFFLL)   // note: this is always the case
      return (CellType)(CT_INT | CTS_X64 | CTS_S64 | CTS_U64);

   return (CellType)(CT_INT | CTS_U64);
}
//-----------------------------------------------------------------------------
bool
IntCell::greater(const Cell * other, bool ascending) const
{
const APL_Integer this_val  = get_int_value();

   switch(other->get_cell_type())
      {
        case CT_INT:
             {
               const APL_Integer other_val = other->get_int_value();
               if (this_val == other_val)   return this > other;
               return this_val > other_val ? ascending : !ascending;
             }

        case CT_FLOAT:
             {
               const APL_Float other_val = other->get_real_value();
               if (this_val == other_val)   return this > other;
               return this_val > other_val ? ascending : !ascending;
             }

        case CT_COMPLEX: break;
        case CT_CHAR:    return ascending;   // never greater
        case CT_POINTER: return !ascending;
        case CT_CELLREF: DOMAIN_ERROR;
        default:         Assert(0 && "Bad celltype");
      }

const Comp_result comp = compare(*other);
   if (comp == COMP_EQ)   return this > other;
   if (comp == COMP_GT)   return ascending;
   else                   return !ascending;
}
//-----------------------------------------------------------------------------
bool
IntCell::equal(const Cell & other, APL_Float qct) const
{
   if (other.is_integer_cell())    return value.ival == other.get_int_value();
   if (!other.is_numeric())        return false;
   return other.equal(*this, qct);
}
//-----------------------------------------------------------------------------
Comp_result
IntCell::compare(const Cell & other) const
{
   if (other.is_integer_cell())
      {
       if (value.ival == other.get_int_value())   return COMP_EQ;
       return (value.ival < other.get_int_value()) ? COMP_LT : COMP_GT;
      }

   if (other.is_numeric())   // float or complex
      {
        return (Comp_result)(-other.compare(*this));
      }

   DOMAIN_ERROR;
}
//-----------------------------------------------------------------------------
// monadic built-in functions...
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_factorial(Cell * Z) const
{
const APL_Integer iv = value.ival;

   // N! fits in:
   // 32-bit signed integer for N <= 12
   // 64-bit signed integer for N <= 20
   // 64-bit float for N <= 170
   //
   
   if (iv < 0)     DOMAIN_ERROR;
   if (iv > 170)   DOMAIN_ERROR;

   if (iv < 21)   // integer result that fits into a signed 64 bit integer
      {
        const APL_Integer table[21] =
           {
             /*  0! */  0x0000000000000001LL,
             /*  1! */  0x0000000000000001LL,
             /*  2! */  0x0000000000000002LL,
             /*  3! */  0x0000000000000006LL,
             /*  4! */  0x0000000000000018LL,
             /*  5! */  0x0000000000000078LL,
             /*  6! */  0x00000000000002d0LL,
             /*  7! */  0x00000000000013b0LL,
             /*  8! */  0x0000000000009d80LL,
             /*  9! */  0x0000000000058980LL,
             /* 10! */  0x0000000000375f00LL,
             /* 11! */  0x0000000002611500LL,
             /* 12! */  0x000000001c8cfc00LL,
             /* 13! */  0x000000017328cc00LL,
             /* 14! */  0x000000144c3b2800LL,
             /* 15! */  0x0000013077775800LL,
             /* 16! */  0x0000130777758000LL,
             /* 17! */  0x0001437eeecd8000LL,
             /* 18! */  0x0016beecca730000LL,
             /* 19! */  0x01b02b9306890000LL,
             /* 20! */  0x21c3677c82b40000LL,
           };

        new (Z) IntCell(table[iv]);
        return E_NO_ERROR;
      }

const double arg = double(iv + 1);
const double res = tgamma(arg);
   new (Z) FloatCell(res);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_conjugate(Cell * Z) const
{
   new (Z) IntCell(value.ival);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_negative(Cell * Z) const
{
   if (value.ival == 0x8000000000000000LL)   // integer overflow
      new (Z) FloatCell(- value.ival);
   else
      new (Z) IntCell(- value.ival);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_direction(Cell * Z) const
{
   if      (value.ival > 0)   new (Z) IntCell( 1);
   else if (value.ival < 0)   new (Z) IntCell(-1);
   else                       new (Z) IntCell( 0);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_magnitude(Cell * Z) const
{
   if (value.ival >= 0)   new (Z) IntCell(value.ival);
   else                   bif_negative(Z);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_reciprocal(Cell * Z) const
{
   switch(value.ival)
      {
        case  0: DOMAIN_ERROR;
        case  1: new (Z) IntCell(1);    return E_NO_ERROR;
        case -1: new (Z) IntCell(-1);   return E_NO_ERROR;
        default: new (Z) FloatCell(1.0/value.ival);
      }
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_roll(Cell * Z) const
{
const APL_Integer qio = Workspace::get_IO();
const APL_Integer set_size = value.ival;
   if (set_size < 0)   DOMAIN_ERROR;

const APL_Integer rnd = Workspace::get_RL();
   if (set_size == 0)   new (Z) FloatCell((1.0*rnd)/0x8000000000000000ULL);
   else                 new (Z) IntCell(qio + (rnd % set_size));
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_pi_times(Cell * Z) const
{
   new (Z) FloatCell(M_PI * value.ival);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_ceiling(Cell * Z) const
{
   new (Z) IntCell(value.ival);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_floor(Cell * Z) const
{
   new (Z) IntCell(value.ival);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_exponential(Cell * Z) const
{
   new (Z) FloatCell(exp((double)value.ival));
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_nat_log(Cell * Z) const
{
   if (value.ival > 0)
      {
        new (Z) FloatCell(log((double)value.ival));
      }
   else if (value.ival < 0)
      {
        const double real = log(-((double)value.ival));
        const double imag = M_PI;   // argz(-1)
        new (Z) ComplexCell(real, imag);
      }
   else
      {
        DOMAIN_ERROR;
      }
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
// dyadic built-in functions...
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_add(Cell * Z, const Cell * A) const
{
   if (!A->is_integer_cell())
      {
        A->bif_add(Z, this);
        return E_NO_ERROR;
      }

   // both cells are integers.
   //
const int64_t a = A->get_int_value();
const int64_t b =    get_int_value();
const double sum = a + (double)b;

   if (sum > LARGE_INT || sum < SMALL_INT)   new (Z) FloatCell(sum);
   else                                      new (Z) IntCell(a + b);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_subtract(Cell * Z, const Cell * A) const
{
   if (!A->is_integer_cell())
      {
        this->bif_negative(Z);   // Z = -B
        A->bif_add(Z, Z);        // Z = A + Z = A + -B
        return E_NO_ERROR;
      }

   // both cells are integers.
   //
const int64_t a = A->get_int_value();
const int64_t b =    get_int_value();
const double diff = a - (double)b;

   if (diff > LARGE_INT || diff < SMALL_INT)   new (Z) FloatCell(diff);
   else                                        new (Z) IntCell(a - b);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_divide(Cell * Z, const Cell * A) const
{
   if (!A->is_integer_cell())
      {
        this->bif_reciprocal(Z);   // Z = ÷B
        A->bif_multiply(Z, Z);     // Z = A × Z = A × ÷B
        return E_NO_ERROR;
      }

   // both cells are integers.
   //
const int64_t a = A->get_int_value();
const int64_t b =    get_int_value();

   if (b == 0)
      {
        if (a != 0)   DOMAIN_ERROR;
        new (Z) IntCell(1);   // 0 ÷ 0 is defined as 1
        return E_NO_ERROR;
      }

const double i_quot = a / b;
const double r_quot = a / (double)b;

   if (r_quot > LARGE_INT || r_quot < SMALL_INT)   new (Z) FloatCell(r_quot);
   else if (a != i_quot * b)                       new (Z) FloatCell(r_quot);
   else                                            new (Z) IntCell(i_quot);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_multiply(Cell * Z, const Cell * A) const
{
   if (!A->is_integer_cell())
      {
        A->bif_multiply(Z, this);
        return E_NO_ERROR;
      }

   // both cells are integers.
   //
const int64_t a = A->get_int_value();
const int64_t b =    get_int_value();
const double prod = a * (double)b;

   if (prod > LARGE_INT || prod < SMALL_INT)   new (Z) FloatCell(prod);
   else                                        new (Z) IntCell(a * b);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_power(Cell * Z, const Cell * A) const
{
   if (!A->is_numeric())   return E_DOMAIN_ERROR;

const int64_t b = get_int_value();
   
   if (!A->is_integer_cell())
      {
        FloatCell B(b);
        B.bif_power(Z, A);
        return E_NO_ERROR;
      }

const int64_t a = A->get_int_value();

   if (a == 0)   { new (Z) IntCell(0);   return E_NO_ERROR; }   // 0^N = 0
   if (a == 1)   { new (Z) IntCell(1);   return E_NO_ERROR; }   // 1^N = 1
   if (b == 0)   { new (Z) IntCell(1);   return E_NO_ERROR; }   // N^0 = 1
   if (b == 1)   { new (Z) IntCell(a);   return E_NO_ERROR; }   // N^1 = N

const double power = pow((double)a, (double)b);
   if (power > LARGE_INT || b < 0)
      {
        new (Z) FloatCell(power);
        return E_NO_ERROR;
      }

   // power is an int small enough for int_64
   // a^b = a^(bn + bn-1 + ... + b0)         with   bj ∈ {0, 2^n}
   //       a^(bn) × a^(bn-1) × ... × a^b0)  with a^bj ∈ {1, a(^2^n) = a^}

int64_t a_2_n = a;  // a^(2^n) = a^(2^(n-1)) × a^(2^(n-1))
int64_t ipow = 1;
   for (int b1 = b; b1; b1 >>= 1)
      {
        if (b1 & 1)   ipow *= a_2_n;
        a_2_n *= a_2_n;
      }

   new (Z) IntCell(ipow);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_residue(Cell * Z, const Cell * A) const
{
const APL_Float qct = Workspace::get_CT();

   if (A->is_near_zero(qct))   // 0∣B is B
      {
        Z->init(*this);
      }
   else if (value.ival == 0)   // B∣0 is 0
      {
        new (Z) IntCell(0);
      }
   else if (A->is_integer_cell())
      {
         APL_Integer rest = value.ival % A->get_int_value();
         if (A->get_int_value() < 0)   // A negative -> Z negative
            {
              if (rest > 0)   rest += A->get_int_value();   // += since A < 0
            }
         else                          // A positive -> Z positive
            {
              if (rest < 0)   rest += A->get_int_value();
            }
         new (Z) IntCell(rest);
      }
   else if (A->is_real_cell())
      {
         const APL_Float valA = A->get_real_value();
         const APL_Float f_quot = value.ival / valA;
         APL_Integer i_quot = floor(f_quot);

         // compute i_quot with respect to ⎕CT
         //
         if (FloatCell::equal_within_quad_CT(i_quot + 1, f_quot, qct))
            ++i_quot;

         APL_Float rest = value.ival - valA * i_quot;
         if (Cell::is_near_zero(rest, qct))
            {
              new (Z) IntCell(0);
              return E_NO_ERROR;
            }

         if (valA < 0)   // A negative -> Z negative
            {
              if (rest > 0)   rest += valA;   // += since A < 0
            }
          else
            {
              if (rest < 0)   rest += valA;
            }
          new (Z) FloatCell(rest);
      }
   else if (A->is_numeric())   // i.e. complex
      {
        const APL_Complex a = A->get_complex_value();
        const APL_Complex b(value.ival);

        // We divide A by B, round down the result, and subtract.
        //
        A->bif_divide(Z, this);      // Z = B/A
        Z->bif_floor(Z);             // Z = A/B rounded down.
        new (Z) ComplexCell(b - a*Z->get_complex_value());
      }
   else   // residue not defined for complex.
      {
        DOMAIN_ERROR;
      }
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_maximum(Cell * Z, const Cell * A) const
{
   if (A->is_integer_cell())
      {
         const APL_Integer a = A->get_int_value();
         if (a >= value.ival)   new (Z) IntCell(a);
         else                   new (Z) IntCell(value.ival);
      }
   else if (A->is_float_cell())
      {
         const APL_Float a = A->get_real_value();
         if (a >= APL_Float(value.ival))   new (Z) FloatCell(a);
         else                              new (Z) IntCell(value.ival);
      }
   else
      {
        A->bif_maximum(Z, this);
      }
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_minimum(Cell * Z, const Cell * A) const
{
   if (A->is_integer_cell())
      {
         const APL_Integer a = A->get_int_value();
         if (a <= value.ival)   new (Z) IntCell(a);
         else                   new (Z) IntCell(value.ival);
      }
   else if (A->is_float_cell())
      {
         const APL_Float a = A->get_real_value();
         if (a <= APL_Float(value.ival))   new (Z) FloatCell(a);
         else                              new (Z) IntCell(value.ival);
      }
   else
      {
        A->bif_minimum(Z, this);
      }
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
PrintBuffer
IntCell::character_representation(const PrintContext & pctx) const
{
   if (pctx.get_scaled())
      {
        FloatCell fc(get_int_value());
        return fc.character_representation(pctx);
      }

char cc[40];
int len = snprintf(cc, sizeof(cc), "%lld", value.ival);

UCS_string ucs;

   loop(l, len)
       {
         if (cc[l] == '-')   ucs.append(UNI_OVERBAR);
         else                ucs.append(Unicode(cc[l]));
       }

ColInfo info;
   info.flags |= CT_INT;
   info.real_len = ucs.size();
   info.int_len = ucs.size();
   
   return PrintBuffer(ucs, info);
}
//-----------------------------------------------------------------------------
int
IntCell::CDR_size() const
{
   // use 4 byte for small integers and 8 bytes for others (converted to float).
   //
const APL_Integer val = get_int_value();
const int32_t high = val >> 32;

   if (val == 0)              return 4;
   if (val == 0xFFFFFFFF)     return 4;
   return 8;
}
//-----------------------------------------------------------------------------
