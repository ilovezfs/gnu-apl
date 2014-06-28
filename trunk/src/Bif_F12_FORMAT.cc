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

#include <string.h>
#include <stdio.h>

#include "Avec.hh"
#include "Bif_F12_FORMAT.hh"
#include "CharCell.hh"
#include "PrimitiveFunction.hh"
#include "PrintOperator.hh"
#include "Value.icc"
#include "Workspace.hh"
                                            
Bif_F12_FORMAT Bif_F12_FORMAT::fun;       // ⍕

//-----------------------------------------------------------------------------
Token
Bif_F12_FORMAT::eval_B(Value_P B)
{
   // ISO and lrm: If B is a character array, then Z is B
   //
   if (!B->NOTCHAR())   return Token(TOK_APL_VALUE1, B);

   if (B->is_empty())
      {
        Value_P Z(new Value(B->get_shape(), LOC));
        new (&Z->get_ravel(0)) CharCell(UNI_ASCII_SPACE);
        return Token(TOK_APL_VALUE1, Z);
      }

Value_P Z;
   if (B->get_rank() > 2)
      {
        // reduce N>2 dimensions to 2 dimensions
        //
        const Shape shape_B1(B->get_rows(), B->get_cols());
        Value_P B1 = B->clone(LOC);
        B1->set_shape(shape_B1);

        Z = monadic_format(B1);

        Shape shape_Z(B->get_shape());
        shape_Z.set_last_shape_item(Z->get_last_shape_item());
        Assert(shape_Z.get_volume() == Z->element_count());
        Z->set_shape(shape_Z);
      }
   else
      {
        Z = monadic_format(B);
      }

   // adjust rank as needed:
   // ρρZ ←→ ,1⌈⍴ρB     if B is simple
   // ρρZ ←→ ,1 or ,2   if B is nested
   //
const Depth depth = B->compute_depth();
Shape sZ;
   if (depth > 1)   // nested
      {
        // the  examples in lrm contradict the text in lrm.
        //
        // Page 136 shows: R←2 3ρ'ONE' 1 1 'TWO' 2 22 which contains only
        // scalar and vector items (R itself being a matrix) and therefore
        // ⍕R shoule be a vector according to page 137:
        //
        // "Nested Arrays: When R is a nested array, Z is a vector if all"
        // items of R at any depth are scalars or vectors."
        // 
        // lrm also contradicts the ISO standard regarding the rank of Z.
        //
        // We try our best...
        //
        if (B->is_one_dimensional())
           {
             sZ = Shape(Z->element_count());
           }
        else
           {
             sZ = Z->get_shape();   // leave Z as is (2-dimensional)
           }
      }
   else             // simple:                    result rank: 
      {
        if (B->get_rank() < 2)  sZ = Shape(Z->get_cols());
        else
           {
             sZ = B->get_shape();
             sZ.set_last_shape_item(Z->get_last_shape_item());
           }
      }

        if (sZ != Z->get_shape())
           {
             Assert(sZ.get_volume() == Z->element_count());
             Z->set_shape(sZ);
           }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_FORMAT::eval_AB(Value_P A, Value_P B)
{
Value_P Z;

   if (A->is_char_vector() || A->is_char_scalar())
      {
        Z = format_by_example(A, B);
      }
   else if (A->is_int_vector(Workspace::get_CT())
              || A->is_int_scalar(Workspace::get_CT()))
      {
        Z = format_by_specification(A, B);
      }
   else
      {
        A->print_boxed(CERR, "left argument of ⍕: ");
        DOMAIN_ERROR;
      }

   Z->set_default_Spc();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Value_P
Bif_F12_FORMAT::monadic_format(Value_P B)
{
   Assert(B->get_rank() <= 2);

PrintContext pctx = Workspace::get_PrintContext();
   pctx.set_style(PrintStyle(pctx.get_style() | PST_NO_FRACT_0));

const PrintBuffer pb(*B, pctx);

const ShapeItem width  = pb.get_width(0);
const ShapeItem height = pb.get_height();

   // monadic_format() returns the Value with rank 1 or 2 which may be changed
   // in Bif_F12_FORMAT::eval_B() later on to match the rather arbitrary rules
   // in lrm.
   //
Value_P Z;
   if (height == 1)   Z = Value_P(new Value(width, LOC));
   else               Z = Value_P(new Value(Shape(height, width), LOC));

   loop(h, height)
   loop(w, width)
      {
        const Unicode uni = pb.get_char(w, h);
        if (is_iPAD_char(uni))  new (Z->next_ravel()) CharCell(UNI_ASCII_SPACE);
        else                    new (Z->next_ravel()) CharCell(uni);
      }

   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Bif_F12_FORMAT::format_by_example(Value_P A, Value_P B)
{
   if (A->get_rank() != 1)   RANK_ERROR;

   // convert the ravel of char vector A into an UCS_string.
   //
UCS_string format = A->get_UCS_ravel();
   if (format.size() == 0)   LENGTH_ERROR;

const ShapeItem cols = B->get_cols();
const ShapeItem rows = B->get_rows();

   // split format into format fields, one per column.
   // If there is only one format field, then repeat it cols times.
   //
vector<UCS_string> col_formats;
   split_example_into_columns(format, col_formats);
   if (col_formats. size() == 1)
      {
        UCS_string f = format;
        loop(c, cols - 1)
           {
             format.append(f);
             col_formats.push_back(col_formats.front());
           }
      }

   if (col_formats.size() != cols)   LENGTH_ERROR;

   // convert each column format string into a Format_LIFER
   //
vector<Format_LIFER> col_items;
   loop(c, col_formats.size())
       col_items.push_back(Format_LIFER(col_formats[c]));

   Log(LOG_Bif_F12_FORMAT)
      {
        Q1(format)
        Q1(format.size())
        Q1(col_formats.size())

        loop(c, col_items.size())
            {
              Q1(col_formats[c])

              Q1(col_items[c].left_deco)
              Q1(col_items[c].int_part)
              Q1(col_items[c].fract_part)
              Q1(col_items[c].expo_deco)
              Q1(col_items[c].exponent)
              Q1(col_items[c].expo_negative)
              Q1(col_items[c].right_deco)
            }
      }

Shape shape_Z(B->get_shape());
   if (B->is_scalar())   shape_Z.add_shape_item(1);
   shape_Z.set_last_shape_item(format.size());

Value_P Z(new Value(shape_Z, LOC));

   try
      {
        loop(r, rows)
           {
             UCS_string row;
             loop(c, cols)
                {
                  const Cell & cB = B->get_ravel(c + r*cols);
                  if (!cB.is_real_cell())   DOMAIN_ERROR;

                  const APL_Float value = cB.get_real_value();

                  const UCS_string item = col_items[c].format_example(value);
                  Log(LOG_Bif_F12_FORMAT)   Q1(item)
                  row.append(item);
                }

             Log(LOG_Bif_F12_FORMAT)   { Q1(row) Q1(format) }

             Assert(row.size() == format.size());
             loop(c, row.size())
                new (&Z->get_ravel(r*format.size() + c))  CharCell(row[c]);
           }
      }
   catch (Error err)
      {
        throw err;   // rethrow
      }
   catch (...)
      {
      }

   return Z;
}
//-----------------------------------------------------------------------------
void
Bif_F12_FORMAT::split_example_into_columns(const UCS_string & format,
                                   vector<UCS_string> & col_formats)
{
bool fmt_seen = false;
UCS_string fmt;
   loop(f, format.size())
       {
         const Unicode cc = format[f];
         fmt.append(cc);

         const bool is_fmt_char = is_control_char(cc);
         if (is_fmt_char)   fmt_seen = true;

         if ((cc == UNI_ASCII_SPACE) && fmt_seen)   // end of field
            {
              col_formats.push_back(fmt);
              fmt.clear();    // start a new field;
              fmt_seen = false;
              continue;   // next char
            }

         if (cc == UNI_ASCII_6)   // end of field after next char
            {
              ++f;   // next char is right decorator (and end of field)
              if (f < format.size())   fmt.append(format[f]);

              col_formats.push_back(fmt);
              fmt.clear();    // start a new field;
              fmt_seen = false;
              continue;   // next char
            }
       }

   if ((!fmt_seen) && (col_formats.size() > 0))
      {
        col_formats.back().append(fmt);
      }
   else
      {
        col_formats.push_back(fmt);
      }
}
//-----------------------------------------------------------------------------
UCS_string
Bif_F12_FORMAT::Format_LIFER::format_example(APL_Float value)
{
UCS_string data_int;
UCS_string data_fract;
UCS_string data_expo;
const double val_1 = (value < 0) ? -value : value;

bool overflow = false;
   fill_data_fields(val_1, data_int, data_fract, data_expo, overflow);
   Log(LOG_Bif_F12_FORMAT)
      {
        Q1(overflow)
        Q1(data_int)
        Q1(data_fract)
        Q1(data_expo)
      }
   if (overflow)   return UCS_string(out_size(), Workspace::get_FC(3));

const UCS_string left = format_left_side(data_int, value < 0, overflow);
   if (overflow)   return UCS_string(out_size(), Workspace::get_FC(3));
   Assert(left.size() == (left_deco.out_len + int_part.out_len));

const UCS_string right = format_right_side(data_fract, value<0, data_expo);
   Assert(right.size() == (fract_part.out_len + expo_deco.out_len +
                           exponent.out_len + right_deco.out_len));

   return left + right;
}
//-----------------------------------------------------------------------------
UCS_string
Bif_F12_FORMAT::Format_LIFER::format_left_side(const UCS_string data_int,
                                               bool negative, bool & overflow)
{
const UCS_string data = int_part.insert_int_commas(data_int, overflow);

   if (overflow)   return UCS_string();

   Assert(int_part.size() >= data.size());
const Unicode pad_char = int_part.pad_char(Workspace::get_FC(2));
const UCS_string pad(int_part.size() - data.size(), pad_char);

UCS_string ucs;
   if (int_part.no_float())                 // floating disabled.
      {
        ucs.append(left_deco.format);
        ucs.append(pad);
      }
   else if (int_part.do_float(negative))   // floating enabled, deco shown
      {
        ucs.append(pad);
        ucs.append(left_deco.format);
      }
   else                                     // floating enabled, deco hidden
      {
        ucs.append(pad);
        ucs.append(UCS_string(left_deco.size(), UNI_ASCII_SPACE));
      }

   ucs.append(data);
   return ucs;
}
//-----------------------------------------------------------------------------
UCS_string
Bif_F12_FORMAT::Format_LIFER::format_right_side(const UCS_string data_fract,
                                                bool negative,
                                                const UCS_string data_expo)
{
int pad_count = 0;
UCS_string ucs;

   if (fract_part.out_len)
      {
        const UCS_string data = fract_part.insert_fract_commas(data_fract);
        Assert(fract_part.size() >= data.size());
        pad_count += fract_part.size() - data.size();

        // print nothing instead of .
        if (data.size() == 0)   ++pad_count;
        else                    ucs.append(Workspace::get_FC(0)); 
        ucs.append(data);
      }

   if (exponent.size())
      {
        UCS_string data(1, exponent_char);

        const Unicode pad_char = exponent.pad_char(Workspace::get_FC(2));

        if (exponent.no_float())              // floating disabled.
           {
             data.append(expo_deco.format);
             data.append(data_expo);
           }
        else if (exponent.do_float(expo_negative))   // floating, deco shown
           {
             data.append(expo_deco.format);
             data.append(data_expo);
           }
        else                                 // floating enabled and deco hidden
           {
             data.append(UCS_string(expo_deco.format.size(), pad_char));
             data.append(data_expo);
           }

        pad_count += expo_deco.out_len + exponent.out_len - data.size();
        ucs.append(data);
      }

   // now do the right side padding.
   {
     const Unicode pad_char = fract_part.pad_char(Workspace::get_FC(2));
     const UCS_string pad(pad_count, pad_char);

     if (fract_part.no_float())                // floating disabled.
        {
          ucs.append(right_deco.format);
          ucs.append(pad);
        }
     else if (fract_part.do_float(negative))  // floating, deco shown
        {
          ucs.append(pad);
          ucs.append(right_deco.format);
        }
     else                                      // floating, deco hidden
        {
          ucs.append(pad);
          ucs.append(UCS_string(right_deco.size(), pad_char));
        }
   }

   return ucs;
}
//-----------------------------------------------------------------------------
Bif_F12_FORMAT::Format_LIFER::Format_LIFER(const UCS_string format)
   : exponent_char(UNI_ASCII_E),
     expo_negative(false)
{
   // we split format into our format chunks...
   //
int f = 0;
bool exponent_pending = false;

// left_decorator:
   while (f < format.size())
      {
        if (is_control_char(format[f]))   goto integral_part;
        else                              left_deco.format.append(format[f++]);
      }
   goto fields_done;

integral_part:
   while (f < format.size())
      {
        const Unicode cc = format[f++];

        if (cc == UNI_ASCII_FULLSTOP)   goto fractional_part;
        if (cc == UNI_ASCII_E)          goto exponent_part;
        if (is_control_char(cc))
           {
             int_part.format.append(cc);
             if (cc == UNI_ASCII_6)          goto right_decorator;
           }
        else
           {
             --f;
            goto right_decorator;
           }
      }
   goto fields_done;

fractional_part:
   while (f < format.size())
      {
        const Unicode cc = format[f++];
        if (cc == UNI_ASCII_7)          exponent_pending = true;

        if (is_control_char(cc))
           {
             fract_part.format.append(cc);
             if (cc == UNI_ASCII_6)          goto right_decorator;
           }
        else
           {
             exponent_char = cc;
             goto exponent_decorator;
           }
      }
   goto fields_done;

exponent_decorator:
   if (!exponent_pending)   { --f;   goto right_decorator; }

   while (f < format.size())
      {
        const Unicode cc = format[f++];

        if (!is_control_char(cc))
           {
             expo_deco.format.append(cc);
           }
        else
           {
             --f;
             goto exponent_part;
           }
      }

exponent_part:
   while (f < format.size())
      {
        const Unicode cc = format[f++];

        if (is_control_char(cc))
           {
             exponent.format.append(cc);
             if (cc == UNI_ASCII_6)          goto right_decorator;
           }
        else
           {
             --f;
             goto right_decorator;
           }
      }

right_decorator:   /// the right decorator
   while (f < format.size())
      right_deco.format.append(format[f++]);

fields_done:

int sum = 0;
   sum += left_deco.out_len = left_deco .format.size();

   sum += int_part  .out_len = int_part  .format.size();

   if (fract_part.format.size())
      sum += fract_part.out_len = fract_part.format.size() + 1;

   if (exponent.size())
      {
        sum += expo_deco.out_len = expo_deco.size();
        sum += exponent.out_len = exponent.format.size() + 1;
      }

   right_deco.out_len = right_deco.format.size();

   // compute floating flags.
   //
   {
     const int count = int_part  .map_field(0)
                     + fract_part.map_field(1);

     if (count == 1)   // flags apply to both sides.
        {
          int_part.  flt_mask |= fract_part.flt_mask & (BIT_1 | BIT_2 | BIT_3);
          fract_part.flt_mask |= int_part  .flt_mask & (BIT_1 | BIT_2 | BIT_3);
        }

     exponent.map_field(2);
   }
}
//-----------------------------------------------------------------------------
int
Format_sub::map_field(int type)
{
int flt_cnt = 0;

   loop(f, format.size())
      {
        switch(format[f])
           {
             case UNI_ASCII_0:              flt_mask |= BIT_0;   break; 
             case UNI_ASCII_1: ++flt_cnt;   flt_mask |= BIT_1;   break; 
             case UNI_ASCII_2: ++flt_cnt;   flt_mask |= BIT_2;   break; 
             case UNI_ASCII_3: ++flt_cnt;   flt_mask |= BIT_3;   break; 
             case UNI_ASCII_4: ++flt_cnt;   flt_mask |= BIT_4;   break; 
             case UNI_ASCII_5:              flt_mask |= BIT_5;   break; 
             case UNI_ASCII_6:              flt_mask |= BIT_6;   break; 
             case UNI_ASCII_7:              flt_mask |= BIT_7;   break; 
             case UNI_ASCII_8:              flt_mask |= BIT_8;   break; 
             case UNI_ASCII_9:              flt_mask |= BIT_9;   break;
             default:                                            break;
           }
      }

   if (flt_mask & (BIT_0 | BIT_9))
      {
        min_len = format.size();
        loop(f, format.size())
            {
              const size_t pos = (type == 1) ? format.size() - f - 1 : f;
              const Unicode uni = format[pos];
              if (uni == UNI_ASCII_0)   break;
              if (uni == UNI_ASCII_9)   break;
              --min_len;
            }
      }

   return flt_cnt;
}
//-----------------------------------------------------------------------------
ostream &
Format_sub::print(ostream & out) const
{
   out << "format '" << format << "'"
          " min " << min_len << ", out_len " << out_len << ", flags: ";
   loop(d, 32)   if (flt_mask & (1 << d))   out << char('0' + d);
   return out;
}
//-----------------------------------------------------------------------------
void
Bif_F12_FORMAT::Format_LIFER::fill_data_fields(double value,
                UCS_string & data_int, UCS_string & data_fract,
                UCS_string & data_expo, bool & overflow)
{
char format[40];
const int data_buf_len = int_part.out_len + 1        // 123.
                       + fract_part.out_len          // 456
                       + 1 + exponent.out_len + 1;   // E-22
DynArray(char, data_buf, data_buf_len);
char * fract_end = 0;

   if (exponent.size())
      {
        // create a format like %.5E (if fract_part has 5 digits)
        //
        const int flen = snprintf(format, sizeof(format), "%%.%luE",
                                  (unsigned long)fract_part.size());
        Assert(flen < sizeof(format));   // format was big enough.

        const int dlen = snprintf(&data_buf[0], data_buf_len, format, value);
        Assert(dlen < data_buf_len);

        char * ep = strchr(&data_buf[0], 'E');
        Assert(ep);
        fract_end = ep++;
        if      (*ep == '+')   ++ep;
        else if (*ep == '-')   { expo_negative = true;   ++ep; }

        int elen = strlen(ep);

        // insert leading zeros until we have at least min_len digits.
        //
        for (; elen < exponent.min_len; ++elen)   data_expo.append(UNI_ASCII_0);

        data_expo.append(UCS_string(UTF8_string(ep)));
      }
   else   // no exponent in format string.
      {
        const int flen = snprintf(format, sizeof(format), "%%.%luf",
                                  (unsigned long)fract_part.size());
        Assert(flen < sizeof(format));   // assume no snprintf() overflow

        const int dlen = snprintf(&data_buf[0], data_buf_len, format, value);
        data_buf[data_buf_len - 1] = 0;

        // the int part could be longer than allowed by the exaple string.
        //
        const char * dot = strchr(data_buf, '.');
        const int ilen = dot ? (dot - data_buf) : strlen(data_buf);
        if (ilen > int_part.out_len)
           {
             if (Workspace::get_FC(3) == UNI_ASCII_0)   DOMAIN_ERROR;
             overflow = true;
             return ;
           }

        Assert(dlen < data_buf_len);
        fract_end = &data_buf[dlen];
      }

char * int_end = strchr(&data_buf[0], '.');
   if (fract_part.size() == 0)
      {
        Assert(int_end == 0);
        int_end = fract_end;
      }
   else
      {
        char * fract_digits = int_end + 1;
        Assert(int_end);

        int flen = fract_end - fract_digits;

        // remove trailing zeros, but leave at least min_len chars.
        while (fract_digits[flen - 1] == '0' && flen > fract_part.min_len)
               fract_digits[--flen] = 0;

        loop(f, flen)   data_fract.append(Unicode(fract_digits[f]));
      }

const int ilen = int_end - &data_buf[0];

   // insert leading zeros so that we will have at least min_len digits
   // after appending the integer data.
   //
   loop(d, int_part.min_len - ilen)   data_int.append(UNI_ASCII_0);

   loop(i, ilen)   data_int.append(Unicode(data_buf[i]));

   // convert 0.xxx to .xxx
   //
   if (  data_int.size() == 1
      && data_int[0] == UNI_ASCII_0
      && int_part.min_len == 0
      && fract_part.size())   data_int.clear();

   Log(LOG_Bif_F12_FORMAT)
      {
        Q1(format)
        Q1(value)
        Q1(data_int)
        Q1(data_fract)
        Q1(data_expo)
      }
}
//-----------------------------------------------------------------------------
UCS_string
Format_sub::insert_int_commas(const UCS_string & data, bool & overflow) const
{
size_t fill_pos = -1;
Unicode fill_char = UNI_ASCII_SPACE;

   if (flt_mask & (BIT_0 | BIT_8 | BIT_9))   // format has a '0', '8', or '9'
      {
        loop(f, format.size())
           {
             const Unicode fc = format[f];
              if (fc == UNI_ASCII_0 || fc == UNI_ASCII_9)
                 {
                   fill_pos = f;
                   fill_char = UNI_ASCII_0;
                   break;
                 }

              if (fc == UNI_ASCII_8)
                 {
                   fill_pos = f;
                   fill_char = Workspace::get_FC(2);
                   break;
                 }
           }
      }

   // we move backwards from the end of the format field, filling it
   // with chars from data of from format.
   //
UCS_string ucs;
size_t d = data.size();

   loop(f1, format.size())
      {
        const size_t f = format.size() - f1 - 1;
        const Unicode fc = format[f];
         if (fc == UNI_ASCII_COMMA)
            {
              ucs.append(Workspace::get_FC(1));   //  == ⎕FC[2] when ⎕IO is 1
            }
         else if (Avec::is_digit(fc))
            {
              if (d)                    ucs.append(data[--d]);
              else if (f >= fill_pos)   ucs.append(fill_char);
              else                      break;
            }
         else
            {
              CERR << "Offending format char [" << f << "] : '"
                   << fc << "' at " << LOC << endl;
              break;
            }
      }

   if (d)   // format too short
      {
         if (Workspace::get_FC(3) == UNI_ASCII_0)   DOMAIN_ERROR;
         else                             overflow = true;
      }

   if ((flt_mask & BIT_9) && ucs.all_zeroes())   ucs.clear();

   return ucs.reverse();
}
//-----------------------------------------------------------------------------
UCS_string
Format_sub::insert_fract_commas(const UCS_string & data) const
{
UCS_string ucs;
int d = 0;

   loop(f, format.size())
      {
        const Unicode fc = format[f];
         if (fc == UNI_ASCII_COMMA)
            {
              ucs.append(Workspace::get_FC(1));
            }
         else if (Avec::is_digit(fc))
            {
              if (d < data.size())   ucs.append(data[d++]);
              else                   break;
            }
         else
            {
              CERR << "Offending format char [" << f << "] : '"
                   << fc << "' at " << LOC << endl;
            }
      }

   if ((flt_mask & BIT_9) && ucs.all_zeroes())   ucs.clear();

   return ucs;
}
//-----------------------------------------------------------------------------
bool
Bif_F12_FORMAT::is_control_char(Unicode uni)
{
   return Avec::is_digit(uni)      ||
          (uni == UNI_ASCII_COMMA) ||
          (uni == UNI_ASCII_FULLSTOP);
}
//-----------------------------------------------------------------------------
Value_P
Bif_F12_FORMAT::format_by_specification(Value_P A, Value_P B)
{
const Shape shape_1(1);
const Shape & shape_B = B->get_rank() ? B->get_shape() : shape_1;

const ShapeItem rows_B = shape_B.get_rows();
const ShapeItem cols_B = shape_B.get_cols();

   if (A->get_rank() > 1)   RANK_ERROR;
const ShapeItem len_A = A->is_scalar() ? 1 : A->get_shape_item(0);

   if (len_A != 1 && len_A != 2 && len_A != 2*cols_B)
      LENGTH_ERROR;

const APL_Float qct = Workspace::get_CT();
   if (shape_B.get_volume() == 0)   // empty B
      {
        ShapeItem W = 0;
        loop(c, cols_B)
           {
             if (len_A <= 2)   W += A->get_ravel(0).get_near_int(qct);
             else              W += A->get_ravel(2*c).get_near_int(qct);
           }

        Shape shape_Z = shape_B.without_axis(shape_B.get_rank() - 1);
        shape_Z.add_shape_item(W);
        const ShapeItem ec_Z = shape_Z.get_volume();

        Value_P Z(new Value(shape_Z, LOC));
        loop(z, ec_Z)   new (Z->next_ravel()) CharCell(UNI_ASCII_SPACE);

        Z->set_default_Spc();
        return Z;
      }

PrintBuffer pb;

   loop(col, cols_B)
       {
         uint32_t col_width;
         uint32_t precision;

         if (len_A == 1)
            {
              col_width = 0;
              precision = A->get_ravel(0).get_near_int(qct);
            }
         else if (len_A == 2)
            {
              col_width = A->get_ravel(0).get_near_int(qct);
              precision = A->get_ravel(1).get_near_int(qct);
            }
         else
            {
              col_width = A->get_ravel(2*col)    .get_near_int(qct);
              precision = A->get_ravel(2*col + 1).get_near_int(qct);
            }

         PrintBuffer pb_col(format_col_spec(col_width, precision,
                                         &B->get_ravel(col), cols_B, rows_B));

         if (col_width == 0)   pb_col.pad_l(UNI_ASCII_SPACE, 1);

         if (col)   pb.append_col(pb_col);
         else       pb = pb_col;
       }

const ShapeItem pb_w = pb.get_width(0);
const ShapeItem pb_h = pb.get_height();
Shape shape_Z(shape_B);
   shape_Z.set_last_shape_item(pb_w);

Value_P Z(new Value(shape_Z, LOC));

   loop(h, pb_h)
   loop(w, pb_w)   new (Z->next_ravel()) CharCell(pb.get_char(w, h));

   return Z;
}
//-----------------------------------------------------------------------------
PrintBuffer
Bif_F12_FORMAT::format_col_spec(int width, int precision, const Cell * cB,
                                int cols, int rows)
{
PrintBuffer ret;

bool has_char = false;
bool has_num = false;

   loop(r, rows)
      {
        const Cell & cell = cB[r*cols];
        if (cell.is_numeric())   has_num = true;
        else                     has_char = true;
      }

   loop(r, rows)
      {
        const Cell & cell = cB[r*cols];
        if (cell.is_character_cell())
           {
             UCS_string data = UCS_string(cell.get_char_value());

             add_row(ret, r, has_char, has_num, UNI_ASCII_E, data);
             continue;
           }

        if (cell.is_pointer_cell())
           {
             Value_P value = cell.get_pointer_value();
             UCS_string data = value->get_UCS_ravel();

             if (width && data.size() > width)   // overflow
                {
                  if (Workspace::get_FC(3) == UNI_ASCII_0)   DOMAIN_ERROR;

                  data = UCS_string(width, Workspace::get_FC(3));
                }

             add_row(ret, r, has_char, has_num, UNI_ASCII_E, data);
             continue;
           }

        if (!cell.is_real_cell())   DOMAIN_ERROR;

        const APL_Float value = cB[r*cols].get_real_value();
        if (precision >= 0)   // floating format
           {
             UCS_string data = format_spec_float(value, precision);
             if (width && data.size() > width)   // overflow
                {
                  if (Workspace::get_FC(3) == UNI_ASCII_0)   DOMAIN_ERROR;
                  data = UCS_string(width, Workspace::get_FC(3));
                }

             add_row(ret, r, has_char, has_num, UNI_ASCII_E, data);
           }
        else                  // exponential format
           {
             UCS_string data = format_spec_expo(value, - precision);
             add_row(ret, r, has_char, has_num, UNI_ASCII_E, data);
          }
      }

   if (width && ret.get_width(0) < width)
      ret.pad_l(UNI_ASCII_SPACE, width - ret.get_width(0));

   return ret;
}
//-----------------------------------------------------------------------------
void
Bif_F12_FORMAT::add_row(PrintBuffer & ret, int row, bool has_char,
                        bool has_num, Unicode align_char, UCS_string & data)
{
   if (row == 0)         // first row: don't  align
      {
        ret.append_ucs(data);
      }
   else if (!has_char)   // only numbers: align at '.' or 'E'
      {
        ret.append_aligned(data, align_char);
      }
   else if (!has_num)    // only chars: align left
      {
        const int d = ret.get_width(0) - data.size();
        if      (d < 0)   ret.pad_r(UNI_ASCII_SPACE, -d);
        else if (d > 0)   data.append(UCS_string(d, UNI_ASCII_SPACE));
        ret.append_ucs(data);
      }
   else                 // chars and numbers: align right
      {
        const int d = ret.get_width(0) - data.size();
        if      (d < 0)   ret.pad_l(UNI_ASCII_SPACE, -d);
        else if (d > 0)   data = UCS_string(d, UNI_ASCII_SPACE) + data;
        ret.append_ucs(data);
      }
}
//-----------------------------------------------------------------------------
UCS_string
Bif_F12_FORMAT::format_spec_float(APL_Float value, int precision)
{
UCS_string ret = UCS_string::from_double_fixed_prec(value, precision);

   // Note: the examples in the apl standard use a leading 0 (like  0.00)
   // while lrm shows .00 instead. We follow lrm and remove a leading 0.
   // Interestingly, the exponential format leaves a leading 0 in both
   // the standard and lrm
   //
   if (precision)   // we have a '.'
      {
        if (ret[0] == UNI_ASCII_0    &&
            ret[1] == UNI_ASCII_FULLSTOP)        // 0.xxx → .xxx
           {
             ret.remove_front();
           }
        else if (ret[0] == UNI_OVERBAR     &&
                 ret[1] == UNI_ASCII_0     &&
                 ret[2] == UNI_ASCII_FULLSTOP)   //  ¯0.x → ¯.x
           {
             ret[1] = UNI_OVERBAR;
             ret.remove_front();
           }
      }
   else if (ret.size() == 2       && 
            ret[0] == UNI_OVERBAR &&
            ret[1] == UNI_ASCII_0)               // ¯0 → 0
           {
             ret.remove_front();
           }

   return ret;
}
//-----------------------------------------------------------------------------
UCS_string
Bif_F12_FORMAT::format_spec_expo(APL_Float value, int precision)
{
  return UCS_string::from_double_expo_prec(value, precision - 1);
}
//-----------------------------------------------------------------------------
ostream &
operator<<(ostream & out, const Format_sub & fmt)
{
   return fmt.print(out);
}
//-----------------------------------------------------------------------------
