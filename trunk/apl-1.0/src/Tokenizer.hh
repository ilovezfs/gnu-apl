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

#ifndef __TOKENIZER_HH_DEFINED__
#define __TOKENIZER_HH_DEFINED__

#include "Token.hh"

/*!
     Functions related to tokenizing of input lines and user defined functions.
 */
class Tokenizer
{
public:
   /// Constructor
   Tokenizer(ParseMode pm)
   : pmode(pm)
   {}

   /// Tokenize UTF-8 string \b input into token string \b tos.
   void tokenize(const UCS_string & input, Token_string & tos);

protected:
   /// Tokenize a function
   void tokenize_function(Source<Unicode> & src, Token_string & tos);

   /// Tokenize a QUAD function or variable
   void tokenize_quad(Source<Unicode> & src, Token_string & tos);

   /// Tokenize a single quoted string
   void tokenize_string1(Source<Unicode> & src, Token_string & tos);

   /// Tokenize a double quoted string
   void tokenize_string2(Source<Unicode> & src, Token_string & tos);

   /// Tokenize a number (integer, floating point, or complex).
   void tokenize_number(Source<Unicode> & src, Token_string & tos);

   /// Tokenize a real number (integer or floating point).
   bool tokenize_real(Source<Unicode> &src, APL_Float & flt_val,
                      APL_Integer & int_val);

   /// Tokenize a symbol
   void tokenize_symbol(Source<Unicode> & src, Token_string & tos);

   /// the parsing mode of this parser
   const ParseMode pmode;
};

#endif // __TOKENIZER_HH_DEFINED__
