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

#ifndef __BIF_F12_FORMAT_HH_DEFINED__
#define __BIF_F12_FORMAT_HH_DEFINED__

#include "PrimitiveFunction.hh"

/** A helper struct for Bif_F12_FORMAT
 It represents a sub-field (int part, fract part, or exponent)
 of the format field
 */
struct Format_sub
{
   Format_sub()
   : out_len(0),
     flt_mask(0),
     min_len(0)
   {}

   UCS_string      format;         ///< the format ('0'...'9' and comma)
   int             out_len;        ///< the length in the output
   uint32_t        flt_mask;       ///< decorator floating mode
   int             min_len;        ///< the minimum length in the output

   /// return the number of characters in \b format
   int size() const
      { return format.size(); }

   /// set flt_mask according to the digits in member \b format
   /// return the number of digits from '1' to '4' (including)
   int map_field(int type);

   /// a debug function for printing \b this Format_sub
   ostream & print(ostream & out) const;

   /// return true iff format contains a '4' (i.e. counteract the effect of 
   /// 1, 2, or 3) or none of 1 (float if negative), 2 (float if positive),
   /// or 3 (always float) was given
   bool no_float() const
        { if (flt_mask & BIT_4)   return true;        // counteract 1, 2, or 3
          return ! (flt_mask & (BIT_1 | BIT_2 | BIT_3));   // no 1, 2, or 3
        }

   /// return true if the decorator is floating floating 
   bool do_float(bool value_is_negative) const
      { if (flt_mask & BIT_1)   return  value_is_negative;
        if (flt_mask & BIT_2)   return !value_is_negative;
        return true;
      }

   /// return the pad character (default: space, otherwise controlled by ⎕FC)
   Unicode pad_char(Unicode qfc) const
      { return flt_mask & BIT_8 ? qfc : UNI_ASCII_SPACE; }

   /// Fill buf at position x,y with data according to fmt
   UCS_string insert_int_commas(const UCS_string & data,
                                bool & overflow) const;

   /// Fill buf at position x,y with data according to fmt
   UCS_string insert_fract_commas(const UCS_string & data) const;
};
//-----------------------------------------------------------------------------
/** System function format
 */
class Bif_F12_FORMAT : public NonscalarFunction
{
public:
   /// constructor
   Bif_F12_FORMAT()
   : NonscalarFunction(TOK_F12_FORMAT)
   {}

   static Bif_F12_FORMAT * fun;   ///< Built-in function
   static Bif_F12_FORMAT  _fun;   ///< Built-in function

   /// Return true iff uni is '0' .. '9', comma, or full-stop
   static bool is_control_char(Unicode uni);

   /// An entire format field (LIFER = Left-Int-Fract-Expo-Right
   struct Format_LIFER
      {
        /// constructor
        Format_LIFER(UCS_string fmt);

        /// return the size of the format
        int format_size() const
           { return left_deco.size() + int_part.size() + fract_part.size()
                  + expo_deco.size() + exponent.size() + right_deco.size(); }

        /// return the size of the output
        int out_size() const
           { return left_deco.out_len + int_part.out_len + fract_part.out_len
                  + expo_deco.out_len + exponent.out_len + right_deco.out_len; }

        /// format \b value by example
        UCS_string format_example(APL_Float value);

        /// format the left decorator and integer part (everything left of the
        /// decimal dot)
        UCS_string format_left_side(const UCS_string data_int, bool negative,
                                    bool & overflow);

        /// format the fract part, exponent, and right decorator (everything
        /// left of the decimal dot)
        UCS_string format_right_side(const UCS_string data_fract, bool negative,
                                     const UCS_string data_expo);

        /// Print \b value into int, fract, and expo fields
        void fill_data_fields(double value, UCS_string & data_int,
                              UCS_string & data_fract, UCS_string & data_expo,
                              bool & overflow);

        Format_sub left_deco;    ///< the left decorator
        Format_sub int_part;     ///< the integer part
        Format_sub fract_part;   ///< the fractional part
        Format_sub expo_deco;    ///< the exponent decorator
        Format_sub exponent;     ///< the exponent
        Format_sub right_deco;   ///< the left decorator

        /// the exponent char to be used
        Unicode exponent_char;

        /// true if the exponent is negative
        bool expo_negative;
      };

   /// A character array with the display of B
   static Value_P monadic_format(Value_P B);

protected:
   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// A character array with B formatted by example
   Value_P format_by_example(Value_P A, Value_P B);

   /// split entire format string string into \b column format strings
   void split_example_into_columns(const UCS_string & format,
                                   vector<UCS_string> & col_formats);

   /// A character array with  a columns of B formatted by specification
   PrintBuffer format_col_spec(int width, int precision, const Cell * cB,
                               int cols, int rows);

   /// add a row (consisting of \b data) to \b PrintBuffer \b ret
   void add_row(PrintBuffer & ret, int row, bool has_char, bool has_num,
                Unicode align_char, UCS_string & data);

   /// A character array with B formatted by specification
   Value_P format_by_specification(Value_P A, Value_P B);

   /// format value with \b precision mantissa digits (floating format)
   static UCS_string format_spec_float(APL_Float value, int precision);

   /// format value with \b precision mantissa digits (exponential format)
   static UCS_string format_spec_expo(APL_Float value, int precision);
};
//-----------------------------------------------------------------------------

#endif // __BIF_F12_FORMAT_HH_DEFINED__
