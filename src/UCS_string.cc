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

#include <math.h>
#include <string.h>

#include "Common.hh"
#include "FloatCell.hh"
#include "Output.hh"
#include "PrintBuffer.hh"
#include "PrintOperator.hh"
#include "UCS_string.hh"
#include "UTF8_string.hh"
#include "Value.icc"

//-----------------------------------------------------------------------------
UCS_string::UCS_string(const char * cstring)
   : Simple_string<Unicode>(0, 0)
{
   items_valid = strlen(cstring);
   items_allocated = items_valid + ADD_ALLOC;
   items = new Unicode[items_allocated];

   for (int l = 0; l < items_valid; ++l)   items[l] = Unicode(cstring[l]);
   items[items_valid] = UNI_ASCII_NUL;
}
//-----------------------------------------------------------------------------
UCS_string::UCS_string(const UTF8_string & utf)
   : Simple_string<Unicode>(0, 0)
{
   Log(LOG_char_conversion)
      CERR << "UCS_string::UCS_string(): utf = " << utf << endl;

   for (uint32_t i = 0; i < utf.size();)
      {
        const uint32_t b0 = utf[i++];
        uint32_t bx = b0;
        uint32_t more;
        if      ((b0 & 0x80) == 0x00)   { more = 0;             }
        else if ((b0 & 0xE0) == 0xC0)   { more = 1; bx &= 0x1F; }
        else if ((b0 & 0xF0) == 0xE0)   { more = 2; bx &= 0x0F; }
        else if ((b0 & 0xF8) == 0xF0)   { more = 3; bx &= 0x0E; }
        else if ((b0 & 0xFC) == 0xF8)   { more = 4; bx &= 0x07; }
        else if ((b0 & 0xFE) == 0xFC)   { more = 5; bx &= 0x03; }
        else
           {
             utf.dump_hex(CERR << "Bad UTF8 string: ", 40)
                               << " at " << LOC <<  endl;
             Backtrace::show(__FILE__, __LINE__);
             return;
           }

        uint32_t uni = 0;
        for (; more; --more)
            {
              if (i >= utf.size())
                 {
                   utf.dump_hex(CERR << "Truncated UTF8 string: ", 40)
                      << " at " << LOC <<  endl;
                   return;
                 }

              const UTF8 subc = utf[i++];
              if ((subc & 0xC0) != 0x80)
                 {
                   utf.dump_hex(CERR << "Bad UTF8 string: ", 40)
                      << " at " << LOC <<  endl;
                   Backtrace::show(__FILE__, __LINE__);
                   return;
                 }

              bx  <<= 6;
              uni <<= 6;
              uni |= subc & 0x3F;
            }

         append(Unicode(bx | uni));
      }

   Log(LOG_char_conversion)
      CERR << "UCS_string::UCS_string(): ucs = " << *this << endl;
}
//-----------------------------------------------------------------------------
UCS_string::UCS_string(APL_Float value, bool & scaled,
                       const PrintContext & pctx)
   : Simple_string<Unicode>(0, 0)
{
int quad_pp = pctx.get_PP();
   if (quad_pp > MAX_Quad_PP)   quad_pp = MAX_Quad_PP;
   if (quad_pp < MIN_Quad_PP)   quad_pp = MIN_Quad_PP;

const bool negative = (value < 0);
   if (negative)   value = -value;

int expo = 0;

   if (value >= 10.0)   // large number, positive exponent
      {
       while (value >= 1e16)
             {
               if (value > 1e307)
                  {
                    if (negative)   append_utf8("¯∞");
                    else            append_utf8("∞");
                    FloatCell::map_FC(*this);
                    return;
                  }
               value *= 1e-16;   expo += 16;
             }
       while (value >= 1e4)    { value *= 1e-4;    expo +=  4; }
       while (value >= 1e1)    { value *= 1e-1;    ++expo;     }
      }
   else if (value < 1.0)   // small number, negative exponent
      {
       while (value < 1e-16)
             {
               if (value < 1e-305)   // very small number: make it 0
                  {
                    append(UNI_ASCII_0);
                    return;
                  }
               value *= 1e16;   expo -= 16;
             }
       while (value < 1e-4)    { value *= 1e4;    expo -=  4; }
       while (value < 1.0)     { value *= 10.0;   --expo;     }
      }

   // In theory, at this point, 1.0 ≤ value < 10.0. In reality value can
   // be outside, though, due to rounding errors.

   // create a string with quad_pp + 1 significant digits.
   // The last digit is used for rounding and then discarded.
   //
UCS_string digits;
   loop(d, (quad_pp + 2))
      {
        if (value >= 10.0)
           {
             digits.append((Unicode)(10 + '0'));
             value = 0.0;
           }
        else if (value < 0.0)
           {
             digits.append((Unicode)('0'));
             value = 0.0;
           }
        else
           {
             const int dig = (int)value;
             value -= dig;
             value *= 10;
             digits.append((Unicode)(dig + '0'));
           }
      }

   if (digits[0] != '0')   digits.pop();

   // round last digit
   //
const Unicode last = digits.last();
   digits.pop();

   if (last >= '5')   digits.last() = (Unicode)(digits.last() + 1);
 
   // adjust carries of 2nd to last digit
   //
   for (int d = digits.size() - 1; d > 0; --d)   // all but first
       {
        if (digits[d] > '9')
           {
             digits[d] =     (Unicode)(digits[d]     - 10);
             digits[d - 1] = (Unicode)(digits[d - 1] +  1);
           }
       }

   // adjust carry of 1st digit
   //
   if (digits[0] > '9')
      {
        digits[0] = (Unicode)(digits[0] - 10);
        digits.insert_before(0, UNI_ASCII_1);
        ++expo;
        digits.pop();
      }

   // remove trailing zeros
   //
   while (digits.size() && digits.last() == UNI_ASCII_0)   digits.pop();

   // force scaled format if:
   //
   // value < .00001     ( value has ≥ 5 leading zeroes)
   // value > 10⋆quad_pp ( integer larger than ⎕PP)
   //
   if ((expo < -6) || (expo > quad_pp))   scaled = true;

   if (negative)   append(UNI_OVERBAR);

   if (scaled)
      {
        append(digits[0]);       // integer part
        if (digits.size() > 1)   // fractional part
           {
             append(UNI_ASCII_FULLSTOP);
             loop(d, (digits.size() - 1))   append(digits[d + 1]);
           }
        if (expo < 0)
           {
             append(UNI_ASCII_E);
             append(UNI_OVERBAR);
             append_number(-expo);
           }
        else if (expo > 0)
           {
             append(UNI_ASCII_E);
             append_number(expo);
           }
        else if (!(pctx.get_style() & PST_NO_EXPO_0)) // expo == 0
           {
             append(UNI_ASCII_E);
             append(UNI_ASCII_0);
           }
      }
   else
      {
        if (expo < 0)   // 0.000...
           {
             append(UNI_ASCII_0);
             append(UNI_ASCII_FULLSTOP);
             loop(e, (-(expo + 1)))   append(UNI_ASCII_0);
             append(digits);
           }
        else   // expo >= 0
           {
             loop(e, expo + 1)
                {
                  if (e < digits.size())   append(digits[e]);
                  else                     append(UNI_ASCII_0);
                }

             if ((expo + 1) < digits.size())   // there are fractional digits
                {
                  append(UNI_ASCII_FULLSTOP);
                  for (int e = expo + 1; e < digits.size(); ++e)
                     {
                       if (e < digits.size())   append(digits[e]);
                       else                     break;
                     }
                }
           }
      }

   FloatCell::map_FC(*this);
}
//-----------------------------------------------------------------------------
UCS_string::UCS_string(const PrintBuffer & pb, Rank rank, int quad_PW)
   : Simple_string<Unicode>(0, 0)
{
   if (pb.get_height() == 0)   return;      // empty PrintBuffer

const int pb_width = pb.get_width(0);
vector<int> breakpoints;

   // compute breakpoints
   //
   for (int col = 0; col < pb_width;)
       {
         const int max_chunk_width = col ? quad_PW - 6 : quad_PW;

         // find breakpoint
         //
         int chunk_len = max_chunk_width;
         const int rest = pb_width - col;
         if (chunk_len < rest)   // rest too large
            {
              // search backwards for break char
              //
              int pos = col + chunk_len;
              if (pos > pb_width)   pos = pb_width;
              while (--pos > col)
                  {
                    const Unicode uni = pb.get_line(0)[pos];
                    if (uni == UNI_iPAD_U2 || uni == UNI_iPAD_U3)
                       {
                          chunk_len = pos - col + 1;
                          break;
                       }
                  }
            }

         breakpoints.push_back(chunk_len);
         col += chunk_len;
       }

   // print rows, breaking at breakpoints
   //
   loop(row, pb.get_height())
       {
         if (row)   append(UNI_ASCII_LF);   // end previous row
         int col = 0;
         loop(b, breakpoints.size())
            {
              const int chunk_len = breakpoints[b];
              if (col)   append_utf8("\n      ");
              UCS_string trow(pb.get_line(row), col, chunk_len);
              trow.remove_trailing_padchars();
              append(trow);
              col += chunk_len;
            }
       }

   // replacing pad chars with blanks.
   //
   loop(u, size())   if (is_iPAD_char((*this)[u]))   (*this)[u] = UNI_ASCII_SPACE;
}
//-----------------------------------------------------------------------------
void
UCS_string::remove_trailing_padchars()
{
   // remove trailing pad chars from align() and append_string(),
   // but leave other pad chars intact.
   // But only if the line has no frame (vert).
   //

   // If the line contains UNI_iPAD_L0 (higher dimension separator)
   // then discard all chars.
   //
   loop(u, size())
       {
         if ((*this)[u] == UNI_LINE_VERT)    break;
         if ((*this)[u] == UNI_LINE_VERT2)   break;
         if ((*this)[u] == UNI_iPAD_L0)
            {
              clear();
              return;
            }

       }

   while (size())
      {
        const Unicode last = back();
        if (last == UNI_iPAD_L0 ||
            last == UNI_iPAD_L1 ||
            last == UNI_iPAD_L2 ||
            last == UNI_iPAD_L3 ||
            last == UNI_iPAD_L4 ||
            last == UNI_iPAD_U7)
            pop_back();
        else
            break;
      }
}
//-----------------------------------------------------------------------------
/// constructor
UCS_string::UCS_string(const Value & value)
   : Simple_string<Unicode>(0, 0)
{
   if (value.get_rank() > 1) RANK_ERROR;

const ShapeItem ec = value.element_count();
   reserve(ec);

   loop(e, ec)   append(value.get_ravel(e).get_char_value());
}
//-----------------------------------------------------------------------------
UCS_string::UCS_string(istream & in)
   : Simple_string<Unicode>(0, 0)
{
   for (;;)
      {
        const Unicode uni = UTF8_string::getc(in);
        if (uni == Invalid_Unicode)   return;
        if (uni == UNI_ASCII_LF)      return;
        append(uni);
      }
}
//-----------------------------------------------------------------------------
void
UCS_string::copy_black(UCS_string & dest, int & idx) const
{
   while (idx < size() && operator[](idx) <= ' ')   ++idx;
   while (idx < size() && operator[](idx) >  ' ')   dest.append(operator[](idx++));
   while (idx < size() && operator[](idx) <= ' ')   ++idx;
}
//-----------------------------------------------------------------------------
void
UCS_string::copy_black_list(vector<UCS_string> & dest) const
{
   for (int idx = 0; ; )
      {
        UCS_string next;
        copy_black(next, idx);
        if (next.size() == 0)   break;

        dest.push_back(next);
      }
}
//-----------------------------------------------------------------------------
bool
UCS_string::contained_in(const vector<UCS_string> & list) const
{
   loop(l, list.size())
      {
        if (*this == list[l])   return true;
      }

   return false;
}
//-----------------------------------------------------------------------------
int
UCS_string::substr_pos(const UCS_string & sub) const
{
const int start_positions = 1 + size() - sub.size();
   loop(start, start_positions)
      {
        bool mismatch = false;
        loop(u, sub.size())
           {
             if ((*this)[start + u] != sub[u])
                {
                  mismatch = true;
                  break;
                }
           }

        if (!mismatch)   return start;   // found sub at start
      }

   return -1;   // not found
}
//-----------------------------------------------------------------------------
bool 
UCS_string::starts_with(const char * prefix) const
{
   loop(s, size())
      {
        const char pc = *prefix++;
        if (pc == 0)   return true;   // prefix matches this string.

        const Unicode uni = (*this)[s];
        if (uni != Unicode(pc))   return false;
      }

   // strings agree, but prefix is longer
   //
   return false;   
}
//-----------------------------------------------------------------------------
bool 
UCS_string::starts_iwith(const char * prefix) const
{
   loop(s, size())
      {
        char pc = *prefix++;
        if (pc == 0)   return true;   // prefix matches this string.
        if (pc >= 'a' && pc <= 'z')   pc -= 'a' - 'A';

        int uni = (*this)[s];
        if (uni >= 'a' && uni <= 'z')   uni -= 'a' - 'A';

        if (uni != Unicode(pc))   return false;
      }

   return *prefix == 0;   
}
//-----------------------------------------------------------------------------
UCS_string
UCS_string::no_pad() const
{
UCS_string ret;
   loop(s, size())
      {
        Unicode uni = (*this)[s];
        if (is_iPAD_char(uni))   uni = UNI_ASCII_SPACE;
        ret.append(uni);
      }

   return ret;
}
//-----------------------------------------------------------------------------
UCS_string
UCS_string::remove_pad() const
{
UCS_string ret;
   loop(s, size())
      {
        Unicode uni = (*this)[s];
        if (!is_iPAD_char(uni))   ret.append(uni);
      }

   return ret;
}
//-----------------------------------------------------------------------------
void
UCS_string::remove_lt_spaces()
{
   // remove trailing spaces
   //
   while (size() && (*this)[size() - 1] <= ' ')   shrink(size() - 1);

   // count leading spaces
   //
int count = 0;
   loop(s, size())
      {
        if ((*this)[s] <= ' ')   ++count;
        else                     break;
      }

   // remove leading spaces
   //
   erase(0, count);
}
//-----------------------------------------------------------------------------
UCS_string
UCS_string::reverse() const
{
UCS_string ret;
   for (int s = size(); s > 0;)   ret.append((*this)[--s]);
   return ret;
}
//-----------------------------------------------------------------------------
void
UCS_string::append_utf8(const UTF8 * str)
{
const size_t len = strlen((const char *)str);
const UTF8_string utf(str, len);
const UCS_string ucs(utf);

   append(ucs);
}
//-----------------------------------------------------------------------------
void
UCS_string::append_ascii(const char * str)
{
   while (*str)   append(Unicode(*str++));
}
//-----------------------------------------------------------------------------
void
UCS_string::append_number(ShapeItem num)
{
char cc[40];
   snprintf(cc, sizeof(cc) - 1, "%lld", (long long)num);
   loop(c, sizeof(cc))
      {
        if (cc[c])   append(Unicode(cc[c]));
        else         break;
      }
}
//-----------------------------------------------------------------------------
void
UCS_string::append_float(double num)
{
char cc[60];
   snprintf(cc, sizeof(cc) - 1, "%lf", num);
   loop(c, sizeof(cc))
      {
        if (cc[c])   append(Unicode(cc[c]));
        else         break;
      }
}
//-----------------------------------------------------------------------------
size_t
UCS_string::to_vector(vector<UCS_string> & result) const
{
size_t max_len = 0;

   result.clear();
   if (size() == 0)   return max_len;

   result.push_back(UCS_string());
   loop(s, size())
      {
        const Unicode uni = (*this)[s];
        if (uni == UNI_ASCII_LF)    // line done
           {
             const size_t len = result.back().size();
             if (max_len < len)   max_len = len;

             if (s < size() - 1)   // more coming
                result.push_back(UCS_string());
           }
        else
           {
             if (uni != UNI_ASCII_CR)         // ignore \r.
                result[result.size() - 1].append(uni);
           }
      }

   // if the last line lacked a \n we check max_len here again.
const size_t len = result.back().size();
   if (max_len < len)   max_len = len;

   return max_len;
}
//-----------------------------------------------------------------------------
Unicode
UCS_string::upper(Unicode uni)
{
   return (uni < 'a' || uni > 'z') ? uni : Unicode(uni - ' ');
}
//-----------------------------------------------------------------------------
int
UCS_string::atoi() const
{
int ret = 0;

   loop(s, size())
      {
        const Unicode uni = (*this)[s];

        if (!ret && uni <= UNI_ASCII_SPACE)   continue;   // leading whitespace

        if (uni < UNI_ASCII_0)                break;      // non-digit
        if (uni > UNI_ASCII_9)                break;      // non-digit

        ret *= 10;
        ret += uni - UNI_ASCII_0;
      }

   return ret;
}
//-----------------------------------------------------------------------------
ostream &
operator << (ostream & os, Unicode uni)
{       
   if (uni < 0x80)      return os << (char)uni;
        
   if (uni < 0x800)     return os << (char)(0xC0 | (uni >> 6))
                                  << (char)(0x80 | (uni & 0x3F));
        
   if (uni < 0x10000)    return os << (char)(0xE0 | (uni >> 12))
                                   << (char)(0x80 | (uni >>  6 & 0x3F))
                                   << (char)(0x80 | (uni       & 0x3F));

   if (uni < 0x200000)   return os << (char)(0xF0 | (uni >> 18))
                                   << (char)(0x80 | (uni >> 12 & 0x3F))
                                   << (char)(0x80 | (uni >>  6 & 0x3F))
                                   << (char)(0x80 | (uni       & 0x3F));

   if (uni < 0x4000000)  return os << (char)(0xF8 | (uni >> 24))
                                   << (char)(0x80 | (uni >> 18 & 0x3F))
                                   << (char)(0x80 | (uni >> 12 & 0x3F))
                                   << (char)(0x80 | (uni >>  6 & 0x3F))
                                   << (char)(0x80 | (uni       & 0x3F));

   return os << (char)(0xFC | (uni >> 30))
             << (char)(0x80 | (uni >> 24 & 0x3F))
             << (char)(0x80 | (uni >> 18 & 0x3F))
             << (char)(0x80 | (uni >> 12 & 0x3F))
             << (char)(0x80 | (uni >>  6 & 0x3F))
             << (char)(0x80 | (uni       & 0x3F));
}
//-----------------------------------------------------------------------------
ostream &
operator << (ostream & os, const UCS_string & ucs)
{
const int fill_len = os.width() - ucs.size();;

   if (fill_len > 0)
      {
        os.width(0);
        loop(u, ucs.size())   os << ucs[u];
        loop(f, fill_len)     os << os.fill();
      }
   else
      {
        loop(u, ucs.size())   os << ucs[u];
      }

   return os;
}
//-----------------------------------------------------------------------------
UCS_string
UCS_string::from_int(int64_t value)
{
   if (value >= 0)   return from_uint(value);

UCS_string ret(UNI_OVERBAR);
   return ret + from_uint(- value);
}
//-----------------------------------------------------------------------------
UCS_string
UCS_string::from_uint(uint64_t value)
{
   if (value == 0)   return UCS_string("0");

int digits[40];
int * d = digits;

   while (value)
      {
        const uint64_t v_10 = value / 10;
        *d++ = value - 10*v_10;
        value = v_10;
      }

UCS_string ret;
   while (d > digits)   ret.append(Unicode(UNI_ASCII_0 + *--d));
   return ret;
}
//-----------------------------------------------------------------------------
UCS_string
UCS_string::from_big(double & val)
{
   Assert(val >= 0);

long double value = val;
int digits[320];   // DBL_MAX is 1.79769313486231470E+308
int * d = digits;

const long double initial_fract = modf(value, &value);
long double fract;
   for (; value >= 1.0; ++d)
      {
         fract = modf(value / 10.0, &value);   // U.x -> .U
         *d = (int)((fract + .02) * 10.0);
         fract -= 0.1 * *d;
      }

   val = initial_fract;

UCS_string ret;
   if (d == digits)   ret.append(UNI_ASCII_0);   // 0.xxx

   while (d > digits)   ret.append(Unicode(UNI_ASCII_0 + *--d));
   return ret;
}
//-----------------------------------------------------------------------------
UCS_string
UCS_string::from_double_expo_prec(double v, unsigned int fract_digits)
{
UCS_string ret;

   if (v == 0.0)
      {
        ret.append(UNI_ASCII_0);
        if (fract_digits)   // unless integer only
           {
             ret.append(UNI_ASCII_FULLSTOP);
             loop(f, fract_digits)   ret.append(UNI_ASCII_0);
           }
        ret.append(UNI_ASCII_E);
        ret.append(UNI_ASCII_0);
        return ret;
      }

   if (v < 0)   { ret.append(UNI_OVERBAR);   v = - v; }

int expo = 0;
   while (v >= 1.0E1)
      {
        if (v >= 1.0E9)
           if (v >= 1.0E13)
              if (v >= 1.0E15)
                 if     (v >= 1.0E16)      { v *= 1.0E-16;   expo += 16; }
                 else /* v >= 1.0E15 */    { v *= 1.0E-15;   expo += 15; }
              else
                 if     (v >= 1.0E14)      { v *= 1.0E-14;   expo += 14; }
                 else /* v >= 1.0E13 */    { v *= 1.0E-13;   expo += 13; }
           else
              if (v >= 1.0E11)
                 if     (v >= 1.0E12)      { v *= 1.0E-12;   expo += 12; }
                 else /* v >= 1.0E11 */    { v *= 1.0E-11;   expo += 11; }
              else
                 if     (v >= 1.0E10)      { v *= 1.0E-10;   expo += 10; }
                 else /* v >= 1.0E9 */     { v *= 1.0E-9;    expo += 9;  }
        else
           if (v >= 1.0E5)
              if (v >= 1.0E7)
                 if     (v >= 1.0E8)       { v *= 1.0E-8;    expo += 8;  }
                 else /* v >= 1.0E7 */     { v *= 1.0E-7;    expo += 7;  }
              else
                 if     (v >= 1.0E6)       { v *= 1.0E-6;    expo += 6;  }
                 else /* v >= 1.0E5 */     { v *= 1.0E-5;    expo += 5;  }
           else
              if (v >= 1.0E3)
                 if     (v >= 1.0E4)       { v *= 1.0E-4;    expo += 4;  }
                 else /* v >= 1.0E3 */     { v *= 1.0E-3;    expo += 3;  }
              else
                 if     (v >= 1.0E2)       { v *= 1.0E-2;    expo += 2;  }
                 else /* v >= 1.0E1 */     { v *= 1.0E-1;    expo += 1;  }
      }

   while (v < 1.0E0)
      {
        if (v < 1.0E-8)
           if (v < 1.0E-12)
              if (v < 1.0E-14)
                 if     (v < 1.0E-15)      { v *= 1.0E-16;   expo += 16; }
                 else /* v < 1.0E-14 */    { v *= 1.0E-15;   expo += 15; }
              else
                 if     (v < 1.0E-13)      { v *= 1.0E-14;   expo += 14; }
                 else /* v < 1.0E-12 */    { v *= 1.0E-13;   expo += 13; }
           else
              if (v < 1.0E-10)
                 if     (v < 1.0E-11)      { v *= 1.0E12;   expo += -12; }
                 else /* v < 1.0E-10 */    { v *= 1.0E11;   expo += -11; }
              else
                 if     (v < 1.0E-9 )      { v *= 1.0E10;   expo += -10; }
                 else /* v < 1.0E-8 */     { v *= 1.0E9;    expo += -9;  }
        else
           if (v < 1.0E-4)
              if (v < 1.0E-6)
                 if     (v < 1.0E-7)       { v *= 1.0E8;    expo += -8;  }
                 else /* v < 1.0E-6 */     { v *= 1.0E7;    expo += -7;  }
              else
                 if     (v < 1.0E-5)       { v *= 1.0E6;    expo += -6;  }
                 else /* v < 1.0E-4 */     { v *= 1.0E5;    expo += -5;  }
           else
              if (v < 1.0E-2)
                 if     (v < 1.0E-3)       { v *= 1.0E4;    expo += -4;  }
                 else /* v < 1.0E-2 */     { v *= 1.0E3;    expo += -3;  }
              else
                 if     (v < 1.0E-1)       { v *= 1.0E2;    expo += -2;  }
                 else /* v < 1.0E0  */     { v *= 1.0E1;    expo += -1;  }
      }

   Assert(v >= 1.0);
   Assert(v < 10.0);

   // print mantissa in fixed format
   //
   ret.append(from_double_fixed_prec(v, fract_digits));
   ret.append(UNI_ASCII_E);
   ret.append(from_int(expo));

   return ret;
}
//-----------------------------------------------------------------------------
UCS_string
UCS_string::from_double_fixed_prec(double v, unsigned int fract_digits)
{
UCS_string ret;

   if (v < 0)   { ret.append(UNI_OVERBAR);   v = - v; }

   ret.append(from_big(v));   // leaves fractional part of v in v

   ret.append(UNI_ASCII_FULLSTOP);

   loop(f, fract_digits + 1)
      {
        v *= 10;
        const int vv = v;
        ret.append(Unicode(UNI_ASCII_0 + vv));
        v -= vv;
      }

   ret.round_last_digit();
   return ret;
}
//-----------------------------------------------------------------------------
void
UCS_string::round_last_digit()
{
   Assert1(size() > 1);
   if (back() >= UNI_ASCII_5)   // round up
      {
        for (int q = size() - 2; q >= 0; --q)
            {
              const Unicode cc = items[q];
              if (cc >= UNI_ASCII_0 && cc <= UNI_ASCII_9)
                 {
                   items[q] = Unicode(cc + 1);
                   if (cc != UNI_ASCII_9)   break;    // 0-8 rounded up: stop
                   items[q] = UNI_ASCII_0;    // 9 rounded up: say 0 and repeat
                 }
            }
      }

   shrink(size() - 1);
   if (items[size() - 1] == UNI_ASCII_FULLSTOP)   shrink(size() - 1);
}
//----------------------------------------------------------------------------
bool
UCS_string::contains(Unicode uni)
{
   loop(u, size())   if ((*this)[u] == uni)   return true;
   return false;
}
//----------------------------------------------------------------------------
