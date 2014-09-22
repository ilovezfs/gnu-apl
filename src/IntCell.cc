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

#include "Value.icc"
#include "ErrorCode.hh"
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
IntCell::equal(const Cell & other, APL_Float qct) const
{
   if (other.is_integer_cell())    return value.ival == other.get_int_value();
   if (!other.is_numeric())        return false;
   return other.equal(*this, qct);
}
//-----------------------------------------------------------------------------
// monadic built-in functions...
//-----------------------------------------------------------------------------

const APL_Integer int_factorials[] =
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
   /* 20! */  0x21c3677c82b40000LL
};

const APL_Float float_factorials[] =
{
   /*   0! */   1.000000000000000E0   ,
   /*   1! */   1.000000000000000E0   ,
   /*   2! */   2.000000000000000E0   ,
   /*   3! */   6.000000000000000E0   ,
   /*   4! */   2.400000000000000E1   ,
   /*   5! */   1.200000000000000E2   ,
   /*   6! */   7.200000000000000E2   ,
   /*   7! */   5.040000000000001E3   ,
   /*   8! */   4.032000000000000E4   ,
   /*   9! */   3.628800000000000E5   ,
   /*  10! */   3.628800000000000E6   ,
   /*  11! */   3.991680000000001E7   ,
   /*  12! */   4.790016000000000E8   ,
   /*  13! */   6.227020800000002E9   ,
   /*  14! */   8.717829120000003E10  ,
   /*  15! */   1.307674368000000E12  ,
   /*  16! */   2.092278988800000E13  ,
   /*  17! */   3.556874280960001E14  ,
   /*  18! */   6.402373705728002E15  ,
   /*  19! */   1.216451004088320E17  ,
   /*  20! */   2.432902008176640E18  ,
   /*  21! */   5.109094217170945E19  ,
   /*  22! */   1.124000727777608E21  ,
   /*  23! */   2.585201673888498E22  ,
   /*  24! */   6.204484017332394E23  ,
   /*  25! */   1.551121004333099E25  ,
   /*  26! */   4.032914611266057E26  ,
   /*  27! */   1.088886945041835E28  ,
   /*  28! */   3.048883446117139E29  ,
   /*  29! */   8.841761993739700E30  ,
   /*  30! */   2.652528598121910E32  ,
   /*  31! */   8.222838654177924E33  ,
   /*  32! */   2.631308369336936E35  ,
   /*  33! */   8.683317618811886E36  ,
   /*  34! */   2.952327990396041E38  ,
   /*  35! */   1.033314796638614E40  ,
   /*  36! */   3.719933267899012E41  ,
   /*  37! */   1.376375309122635E43  ,
   /*  38! */   5.230226174666011E44  ,
   /*  39! */   2.039788208119745E46  ,
   /*  40! */   8.159152832478979E47  ,
   /*  41! */   3.345252661316380E49  ,
   /*  42! */   1.405006117752880E51  ,
   /*  43! */   6.041526306337384E52  ,
   /*  44! */   2.658271574788449E54  ,
   /*  45! */   1.196222208654802E56  ,
   /*  46! */   5.502622159812089E57  ,
   /*  47! */   2.586232415111683E59  ,
   /*  48! */   1.241391559253607E61  ,
   /*  49! */   6.082818640342676E62  ,
   /*  50! */   3.041409320171338E64  ,
   /*  51! */   1.551118753287382E66  ,
   /*  52! */   8.065817517094388E67  ,
   /*  53! */   4.274883284060025E69  ,
   /*  54! */   2.308436973392414E71  ,
   /*  55! */   1.269640335365828E73  ,
   /*  56! */   7.109985878048636E74  ,
   /*  57! */   4.052691950487722E76  ,
   /*  58! */   2.350561331282879E78  ,
   /*  59! */   1.386831185456899E80  ,
   /*  60! */   8.320987112741390E81  ,
   /*  61! */   5.075802138772250E83  ,
   /*  62! */   3.146997326038794E85  ,
   /*  63! */   1.982608315404440E87  ,
   /*  64! */   1.268869321858842E89  ,
   /*  65! */   8.247650592082470E90  ,
   /*  66! */   5.443449390774431E92  ,
   /*  67! */   3.647111091818869E94  ,
   /*  68! */   2.480035542436830E96  ,
   /*  69! */   1.711224524281413E98  ,
   /*  70! */   1.197857166996989E100 ,
   /*  71! */   8.504785885678620E101 ,
   /*  72! */   6.123445837688608E103 ,
   /*  73! */   4.470115461512683E105 ,
   /*  74! */   3.307885441519385E107 ,
   /*  75! */   2.480914081139540E109 ,
   /*  76! */   1.885494701666050E111 ,
   /*  77! */   1.451830920282858E113 ,
   /*  78! */   1.132428117820629E115 ,
   /*  79! */   8.946182130782972E116 ,
   /*  80! */   7.156945704626378E118 ,
   /*  81! */   5.797126020747363E120 ,
   /*  82! */   4.753643337012839E122 ,
   /*  83! */   3.945523969720656E124 ,
   /*  84! */   3.314240134565352E126 ,
   /*  85! */   2.817104114380549E128 ,
   /*  86! */   2.422709538367272E130 ,
   /*  87! */   2.107757298379527E132 ,
   /*  88! */   1.854826422573984E134 ,
   /*  89! */   1.650795516090845E136 ,
   /*  90! */   1.485715964481761E138 ,
   /*  91! */   1.352001527678402E140 ,
   /*  92! */   1.243841405464130E142 ,
   /*  93! */   1.156772507081641E144 ,
   /*  94! */   1.087366156656742E146 ,
   /*  95! */   1.032997848823905E148 ,
   /*  96! */   9.916779348709490E149 ,
   /*  97! */   9.619275968248207E151 ,
   /*  98! */   9.426890448883240E153 ,
   /*  99! */   9.332621544394408E155 ,
   /* 100! */   9.332621544394408E157 ,
   /* 101! */   9.425947759838357E159 ,
   /* 102! */   9.614466715035121E161 ,
   /* 103! */   9.902900716486176E163 ,
   /* 104! */   1.029901674514562E166 ,
   /* 105! */   1.081396758240290E168 ,
   /* 106! */   1.146280563734708E170 ,
   /* 107! */   1.226520203196138E172 ,
   /* 108! */   1.324641819451828E174 ,
   /* 109! */   1.443859583202492E176 ,
   /* 110! */   1.588245541522742E178 ,
   /* 111! */   1.762952551090243E180 ,
   /* 112! */   1.974506857221073E182 ,
   /* 113! */   2.231192748659812E184 ,
   /* 114! */   2.543559733472186E186 ,
   /* 115! */   2.925093693493014E188 ,
   /* 116! */   3.393108684451897E190 ,
   /* 117! */   3.969937160808718E192 ,
   /* 118! */   4.684525849754287E194 ,
   /* 119! */   5.574585761207601E196 ,
   /* 120! */   6.689502913449124E198 ,
   /* 121! */   8.094298525273436E200 ,
   /* 122! */   9.875044200833596E202 ,
   /* 123! */   1.214630436702533E205 ,
   /* 124! */   1.506141741511140E207 ,
   /* 125! */   1.882677176888925E209 ,
   /* 126! */   2.372173242880046E211 ,
   /* 127! */   3.012660018457658E213 ,
   /* 128! */   3.856204823625802E215 ,
   /* 129! */   4.974504222477286E217 ,
   /* 130! */   6.466855489220473E219 ,
   /* 131! */   8.471580690878811E221 ,
   /* 132! */   1.118248651196003E224 ,
   /* 133! */   1.487270706090685E226 ,
   /* 134! */   1.992942746161518E228 ,
   /* 135! */   2.690472707318050E230 ,
   /* 136! */   3.659042881952546E232 ,
   /* 137! */   5.012888748274990E234 ,
   /* 138! */   6.917786472619488E236 ,
   /* 139! */   9.615723196941085E238 ,
   /* 140! */   1.346201247571752E241 ,
   /* 141! */   1.898143759076170E243 ,
   /* 142! */   2.695364137888161E245 ,
   /* 143! */   3.854370717180069E247 ,
   /* 144! */   5.550293832739300E249 ,
   /* 145! */   8.047926057471988E251 ,
   /* 146! */   1.174997204390910E254 ,
   /* 147! */   1.727245890454637E256 ,
   /* 148! */   2.556323917872863E258 ,
   /* 149! */   3.808922637630566E260 ,
   /* 150! */   5.713383956445850E262 ,
   /* 151! */   8.627209774233233E264 ,
   /* 152! */   1.311335885683452E267 ,
   /* 153! */   2.006343905095681E269 ,
   /* 154! */   3.089769613847348E271 ,
   /* 155! */   4.789142901463389E273 ,
   /* 156! */   7.471062926282889E275 ,
   /* 157! */   1.172956879426414E278 ,
   /* 158! */   1.853271869493734E280 ,
   /* 159! */   2.946702272495036E282 ,
   /* 160! */   4.714723635992057E284 ,
   /* 161! */   7.590705053947215E286 ,
   /* 162! */   1.229694218739448E289 ,
   /* 163! */   2.004401576545301E291 ,
   /* 164! */   3.287218585534293E293 ,
   /* 165! */   5.423910666131586E295 ,
   /* 166! */   9.003691705778428E297 ,
   /* 167! */   1.503616514864998E300 ,
   /* 168! */   2.526075744973197E302 ,
   /* 169! */   4.269068009004701E304 ,
   /* 170! */   7.257415615307993E306
};
enum
{
   int_fact_max   = sizeof(int_factorials)   / sizeof(APL_Integer),
   float_fact_max = sizeof(float_factorials) / sizeof(APL_Float)
};

ErrorCode
IntCell::bif_factorial(Cell * Z) const
{
const APL_Integer iv = value.ival;

   // N! fits into:
   // 32-bit signed integer for N <= 12
   // 64-bit signed integer for N <= 20
   // 64-bit float for N <= 170
   //
   
   if (iv < 0)     return E_DOMAIN_ERROR;

   if (iv < int_fact_max)   // integer result that fits into a 64-bit int
      {
        new (Z) IntCell(int_factorials[iv]);
        return E_NO_ERROR;
      }

   if (iv < float_fact_max)   // float result that fits into a 64 bit double
      {
        new (Z) FloatCell(float_factorials[iv]);
        return E_NO_ERROR;
      }

   return E_DOMAIN_ERROR;
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
        case  0: return E_DOMAIN_ERROR;
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
   if (set_size <= 0)   return E_DOMAIN_ERROR;

const uint64_t rnd = Workspace::get_RL(set_size);
   new (Z) IntCell(qio + (rnd % set_size));
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
IntCell::bif_pi_times_inverse(Cell * Z) const
{
   new (Z) FloatCell(value.ival / M_PI);
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
   // e to the B-th power
   //
   new (Z) FloatCell(exp((APL_Float)value.ival));
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_nat_log(Cell * Z) const
{
   if (value.ival > 0)
      {
        new (Z) FloatCell(log((APL_Float)value.ival));
      }
   else if (value.ival < 0)
      {
        const APL_Float real = log(-((APL_Float)value.ival));
        const APL_Float imag = M_PI;   // argz(-1)
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
// where possible a function with non-int A is delegated to the corresponding
// member function of A. For numeric cells that is the FloatCell or ComplexCell
// function and otherwise the default function (that returns E_DOMAIN_ERROR.
//
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_add(Cell * Z, const Cell * A) const
{
   if (A->is_integer_cell())
      {
        // both cells are integers.
        //
        const APL_Integer a = A->get_int_value();
        const APL_Integer b =    get_int_value();
        const APL_Float sum = a + (APL_Float)b;

        if (sum > LARGE_INT || sum < SMALL_INT)   new (Z) FloatCell(sum);
        else                                      new (Z) IntCell(a + b);
        return E_NO_ERROR;
      }

   // delegate to A
   //
   return A->bif_add(Z, this);
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_subtract(Cell * Z, const Cell * A) const
{
   if (A->is_integer_cell())
      {
        // both cells are integers.
        //
        const APL_Integer a = A->get_int_value();
        const APL_Integer b =    get_int_value();
        const APL_Float diff = a - (APL_Float)b;

        if (diff > LARGE_INT || diff < SMALL_INT)   new (Z) FloatCell(diff);
        else                                        new (Z) IntCell(a - b);
        return E_NO_ERROR;
      }

   // delegate to A
   //
   this->bif_negative(Z);   // Z = -B
   return A->bif_add(Z, Z);        // Z = A + Z = A + -B
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_divide(Cell * Z, const Cell * A) const
{
   if (A->is_integer_cell())
      {
        // both cells are integers.
        //
        const APL_Integer a = A->get_int_value();
        const APL_Integer b =    get_int_value();

        if (b == 0)   // allowed if a == 0 as well
           {
             if (a != 0)   return E_DOMAIN_ERROR;
             new (Z) IntCell(1);   // 0 ÷ 0 is defined as 1
             return E_NO_ERROR;
           }

        const APL_Float i_quot = a / b;
        const APL_Float r_quot = a / (APL_Float)b;

        if (r_quot > LARGE_INT ||
            r_quot < SMALL_INT)     new (Z) FloatCell(r_quot);
        else if (a != i_quot * b)   new (Z) FloatCell(r_quot);
   else                             new (Z) IntCell(i_quot);
        return E_NO_ERROR;
      }

   // delegate to A
   //
   this->bif_reciprocal(Z);   // Z = ÷B
   return A->bif_multiply(Z, Z);     // Z = A × Z = A × ÷B
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_multiply(Cell * Z, const Cell * A) const
{
   if (A->is_integer_cell())
      {
        // both cells are integers.
        //
        const APL_Integer a = A->get_int_value();
        const APL_Integer b =    get_int_value();
        const APL_Float prod = a * (APL_Float)b;

        if (prod > LARGE_INT || prod < SMALL_INT)   new (Z) FloatCell(prod);
        else                                        new (Z) IntCell(a * b);
        return E_NO_ERROR;
      }

   // delegate to A
   //
   return A->bif_multiply(Z, this);
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_power(Cell * Z, const Cell * A) const
{
   // some A to the integer B-th power
   //
   if (!A->is_numeric())   return E_DOMAIN_ERROR;

APL_Integer b = get_int_value();
const bool invert_Z = b < 0;
   if (invert_Z)   b = - b;

   // at this point, b ≥ 0
   //
   if (b <= 1)   // special cases A⋆1, A⋆0, and A⋆¯1
      {
        if (b == 0)   return z1(Z);  // A⋆0 is 1

        if (invert_Z)               return A->bif_reciprocal(Z);
        if (A->is_real_cell())      return A->bif_conjugate(Z);
        new (Z) ComplexCell(A->get_complex_value());
        return E_NO_ERROR;
      }

   if (A->is_integer_cell())
      {
        APL_Integer a = A->get_int_value();
        if (a == 0)
           {
             if (invert_Z)   return E_DOMAIN_ERROR;

             return z0(Z);   // 0^b = 0
           }
        if (a == 1)    return z1(Z);   // 1^b = 1
        if (a == -1)   // -1^b = 1 or -1
           {
              if (b & 1)   return z_1(Z);   // -1^(2N+2) = -1
              else         return z1(Z);    // -1^(2N+1) =  1
           }

        // try to compute A⋆B as integers
        //
        const bool negate_Z = (a < 0) && (b & 1);
        if (a < 0)   a = -a;

        // z is an int small enough for int_64
        // a⋆b = a⋆(bn +   bn-1   + ... + b0)  with   bj = 0 or 2⋆j
        //     = a⋆bn  × a⋆bn-1 × ... × a⋆b0   with a⋆bj = 1 or a⋆2⋆j
        //
        bool overflow = false;
        APL_Integer a_2_n = a;  // a^(2^n) = a^(2^(n-1)) × a^(2^(n-1))
        APL_Integer zi = 1;
        for (int b1 = b; b1; b1 >>= 1)
           {
             if (b1 & 1)
                {
                  if (zi >= 0x7FFFFFFFFFFFFFFFULL / a_2_n)
                     {
                       overflow = true;
                       break;
                     }
                  zi *= a_2_n;
                  if (b1 == 1)   break;
                }

             if (a_2_n >= 100000000ULL)
                {
                  overflow = true;
                  break;
                }

             a_2_n *= a_2_n;
           }

        if (!overflow)
           {
             if (negate_Z)   zi = - zi;
             if (invert_Z)
                {
                  if (zi == 0)   return E_DOMAIN_ERROR;
                  new (Z) FloatCell(1.0 / zi);
                }
             else
                {
                  new (Z) IntCell(zi);
                }
             return E_NO_ERROR;
           }

        // A and B are integers, but Z is too big for integers
        //
        APL_Float z = pow((APL_Float)a, (APL_Float)b);

        if (negate_Z)   z = - z;
        if (invert_Z)   z = 1.0 / z;
        new (Z) FloatCell(z);
        return E_NO_ERROR;
      }

   if (A->is_float_cell())
      {
        APL_Float a = A->get_real_value();
        const bool negate_Z = (a < 0) && (b & 1);
        if (a < 0)   a = -a;

        APL_Float z = pow(a, (APL_Float)b);
        if (negate_Z)   z = - z;
        if (invert_Z)
           {
             if (z == 0.0)   return E_DOMAIN_ERROR;
             z = 1.0 / z;
           }
        new (Z) FloatCell(z);
        return E_NO_ERROR;
      }

   // complex A to the B-th power
   //
   {
     APL_Complex a = A->get_complex_value();
     if (a.imag() == 0 && a.real() < 0)   return E_DOMAIN_ERROR;

     APL_Complex z = pow(a, (APL_Complex)b);
     if (invert_Z)
        {
             if (z == 0.0)   return E_DOMAIN_ERROR;
             z = 1.0 / z;
        }
        new (Z) ComplexCell(z);
        return E_NO_ERROR;
   }
}
//-----------------------------------------------------------------------------
ErrorCode
IntCell::bif_residue(Cell * Z, const Cell * A) const
{
const APL_Float qct = Workspace::get_CT();

   // special cases
   //
   if (A->is_near_zero(qct))   // 0∣B is B
      {
        new (Z) IntCell(get_int_value());
        return E_NO_ERROR;
      }

   if (value.ival == 0)   // B∣0 is 0
      {
        new (Z) IntCell(0);
        return E_NO_ERROR;
      }

   if (A->is_integer_cell())
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
        return E_NO_ERROR;
      }

   if (A->is_real_cell())
      {
        const APL_Float valA = A->get_real_value();
        const APL_Float f_quot = value.ival / valA;
        APL_Integer i_quot = floor(f_quot);

        // compute i_quot with respect to ⎕CT
        //
        if (tolerantly_equal(i_quot + 1, f_quot, qct))  ++i_quot;

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
        return E_NO_ERROR;
      }

   if (A->is_complex_cell())   // i.e. complex
      {
        const APL_Complex a = A->get_complex_value();
        const APL_Complex b(value.ival);

        // We divide A by B, round down the result, and subtract.
        //
        A->bif_divide(Z, this);      // Z = B/A
        Z->bif_floor(Z);             // Z = A/B rounded down.
        new (Z) ComplexCell(b - a*Z->get_complex_value());
        return E_NO_ERROR;
      }

   return E_DOMAIN_ERROR;
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
         return E_NO_ERROR;
      }

   // delegate to A
   //
   return A->bif_maximum(Z, this);
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

#include "Error.hh"

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
PrintBuffer
IntCell::character_representation(const PrintContext & pctx) const
{
   if (pctx.get_scaled())
      {
        FloatCell fc(get_int_value());
        return fc.character_representation(pctx);
      }

char cc[40];
int len = snprintf(cc, sizeof(cc), "%lld", (long long)(value.ival));

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

   if (val == 0)              return 4;
   if (val == 0xFFFFFFFF)     return 4;
   return 8;
}
//-----------------------------------------------------------------------------
