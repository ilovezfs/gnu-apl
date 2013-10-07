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

#include <string.h>

#include "Bif_OPER1_COMMUTE.hh"
#include "Bif_OPER1_EACH.hh"
#include "Bif_OPER2_PRODUCT.hh"
#include "Bif_OPER1_RANK.hh"
#include "Bif_OPER1_REDUCE.hh"
#include "Bif_OPER1_SCAN.hh"
#include "CharCell.hh"
#include "ComplexCell.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "Output.hh"
#include "PointerCell.hh"
#include "PrintOperator.hh"
#include "Symbol.hh"
#include "SystemLimits.hh"
#include "SystemVariable.hh"
#include "Tokenizer.hh"
#include "Value.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
/** convert \b UCS_string input into a Token_string tos.
*/
ErrorCode
Tokenizer::tokenize(const UCS_string & input, Token_string & tos)
{
   try
      {
        do_tokenize(input, tos);
      }
   catch (Error err)
      {
        const int caret_1 = input.size() - rest_1;
        const int caret_2 = input.size() - rest_2;
        err.set_error_line_2(input, caret_1, caret_2);
        return err.error_code;
      }

   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
/** convert \b UCS_string input into a Token_string tos.
*/
void
Tokenizer::do_tokenize(const UCS_string & input, Token_string & tos)
{
   Log(LOG_tokenize)
      CERR << "tokenize: input[" << input.size() << "] is: «"
           << input << "»" << endl;

Source<Unicode> src(input);
   while (rest_1 = rest_2 = src.rest())
      {
        const Unicode uni = *src;
        if (uni == UNI_COMMENT)   break;

        const Token tok = Avec::uni_to_token(uni);

        Log(LOG_tokenize)
           {
             Source<Unicode> s1(src, 0, 24);
             CERR << "  tokenize(" <<  src.rest() << " chars) sees [tag "
                  << HEX(tok.get_tag()) << " «"
                  << uni << "»] " << s1;
             if (src.rest() != s1.rest())   CERR << " ...";
             CERR << endl;
           }

        switch(tok.get_Class())
            {
              case TC_END:   // chars without APL meaning
                   rest_2 = src.rest();
                   throw_parse_error(E_NO_TOKEN, LOC, loc);
                   break;

              case TC_RETURN:
              case TC_LINE:
              case TC_VALUE:
              case TC_INDEX:
              case TC_VOID:
                   CERR << "Offending token: " << tok.get_tag()
                        << " (" << tok << ")" << endl;
                   if (tok.get_tag() == TOK_CHARACTER)
                      CERR << "Unicode: " << UNI(tok.get_char_val()) << endl;
                   rest_2 = src.rest();
                   throw_parse_error(E_NON_APL_CHAR, LOC, loc);
                   break;

              case TC_SYMBOL:
                   if (uni == UNI_QUAD_QUAD)
                      {
                         tokenize_quad(src, tos);
                      }
                   else if (uni == UNI_QUOTE_QUAD)
                      {
                        ++src;
                        tos += Token(TOK_QUAD_QUOTE,
                                     &Workspace::the_workspace->v_quad_QUOTE);
                      }
                   else
                      {
                        tokenize_symbol(src, tos);
                      }
                   break;

              case TC_FUN0:
              case TC_FUN12:
              case TC_OPER1:
                   tokenize_function(src, tos);
                   break;

              case TC_OPER2:
                   if (tok.get_tag() == TOK_OPER2_PRODUCT && src.rest())
                      {
                        // tok is a dot. This could mean that . is either
                        //
                        // the start of a number:     e.g. +.3
                        // or an operator:            e.g. +.*
                        //
                        const Unicode uni_1 = src[1];
                        const Token tok_1 = Avec::uni_to_token(uni_1);
                        if ((tok_1.get_tag() & TC_MASK) == TC_NUMERIC)
                           tokenize_number(src, tos);
                        else
                           tokenize_function(src, tos);
                      }
                   else
                      {
                        tokenize_function(src, tos);
                      }
                   break;

              case TC_R_ARROW:
                   ++src;
                   if (src.rest())   tos += tok;
                   else              tos += Token(TOK_ESCAPE);
                   break;

              case TC_ASSIGN:
              case TC_L_PARENT:
              case TC_R_PARENT:
              case TC_L_BRACK:
              case TC_R_BRACK:
                   ++src;
                   tos += tok;
                   break;

              case TC_DIAMOND:
                   if (pmode == PM_EXECUTE)
                      {
                        rest_2 = src.rest();
                        throw_parse_error(E_BAD_EXECUTE_CHAR, LOC, loc);
                      }

                   ++src;
                   tos += tok;
                   break;

              case TC_COLON:
                   if (pmode != PM_FUNCTION)
                      {
                        rest_2 = src.rest();
                        throw_parse_error(E_BAD_EXECUTE_CHAR, LOC, loc);
                      }

                   ++src;
                   tos += tok;
                   break;

              case TC_NUMERIC:
                   tokenize_number(src, tos);
                   break;

              case TC_SPACE:
              case TC_NEWLINE:
                   ++src;
                   break;

              case TC_QUOTE:
                   if (tok.get_tag() == TOK_QUOTE1)
                      tokenize_string1(src, tos);
                   else
                      tokenize_string2(src, tos);
                   break;

              default:
                   CERR << "Input: " << input << endl
                        << "uni:   " << uni << endl
                        << "Token = " << tok.get_tag() << endl;

                   if (Id(tok.get_tag() >> 16) != ID_No_ID)
                      {
                        CERR << ", Id = " << Id(tok.get_tag() >> 16);
                      }
                   CERR << endl;
                   Assert(0 && "Should not happen");
            }
      }

   Log(LOG_tokenize)
      CERR << "tokenize() done (no error)" << endl;
}
//-----------------------------------------------------------------------------
void
Tokenizer::tokenize_function(Source<Unicode> & src, Token_string & tos)
{
   Log(LOG_tokenize)   CERR << "tokenize_function(" << src << ")" << endl;

const Unicode uni = src.get();
Token tok = Avec::uni_to_token(uni);

#define sys(t, f) \
   case TOK_ ## t: tok = Token(tok.get_tag(), &Bif_ ## f::fun);   break;

   switch(tok.get_tag())
      {
        sys(F1_EXECUTE,    F1_EXECUTE)

        sys(F2_AND,        F2_AND)
        sys(F2_EQUAL,      F2_EQUAL)
        sys(F2_FIND,       F2_FIND)
        sys(F2_GREATER,    F2_GREATER)
        sys(F2_INDEX,      F2_INDEX)
        sys(F2_LESS,       F2_LESS)
        sys(F2_LEQ,        F2_LEQ)
        sys(F2_MEQ,        F2_MEQ)
        sys(F2_NAND,       F2_NAND)
        sys(F2_NOR,        F2_NOR)
        sys(F2_OR,         F2_OR)
        sys(F2_UNEQ,       F2_UNEQ)

        sys(F12_BINOM,     F12_BINOM)
        sys(F12_CIRCLE,    F12_CIRCLE)
        sys(F12_COMMA,     F12_COMMA)
        sys(F12_COMMA1,    F12_COMMA1)
        sys(F12_DECODE,    F12_DECODE)
        sys(F12_DIVIDE,    F12_DIVIDE)
        sys(F12_DOMINO,    F12_DOMINO)
        sys(F12_DROP,      F12_DROP)
        sys(F12_ELEMENT,   F12_ELEMENT)
        sys(F12_ENCODE,    F12_ENCODE)
        sys(F12_EQUIV,     F12_EQUIV)
        sys(F12_FORMAT,    F12_FORMAT)
        sys(F12_INDEX_OF,  F12_INDEX_OF)
        sys(F2_LEFT,       F2_LEFT)
        sys(F12_LOGA,      F12_LOGA)
        sys(F12_MINUS,     F12_MINUS)
        sys(F12_PARTITION, F12_PARTITION)
        sys(F12_PICK,      F12_PICK)
        sys(F12_PLUS,      F12_PLUS)
        sys(F12_POWER,     F12_POWER)
        sys(F12_RHO,       F12_RHO)
        sys(F2_RIGHT,      F2_RIGHT)
        sys(F12_RND_DN,    F12_RND_DN)
        sys(F12_RND_UP,    F12_RND_UP)
        sys(F12_ROLL,      F12_ROLL)
        sys(F12_ROTATE,    F12_ROTATE)
        sys(F12_ROTATE1,   F12_ROTATE1)
        sys(F12_SORT_ASC,  F12_SORT_ASC)
        sys(F12_SORT_DES,  F12_SORT_DES)
        sys(F12_STILE,     F12_STILE)
        sys(F12_TAKE,      F12_TAKE)
        sys(F12_TRANSPOSE, F12_TRANSPOSE)
        sys(F12_TIMES,     F12_TIMES)
        sys(F1_UNIQUE,     F1_UNIQUE)
        sys(F12_WITHOUT,   F12_WITHOUT)

        sys(JOT,           JOT)

        sys(OPER1_EACH,    OPER1_EACH)
        sys(OPER1_COMMUTE, OPER1_COMMUTE)
        sys(OPER1_RANK,    OPER1_RANK)
        sys(OPER1_REDUCE,  OPER1_REDUCE)
        sys(OPER1_REDUCE1, OPER1_REDUCE1)
        sys(OPER1_SCAN,    OPER1_SCAN)
        sys(OPER1_SCAN1,   OPER1_SCAN1)

        sys(OPER2_PRODUCT, OPER2_PRODUCT)

        default: CERR << endl << "Token = " << tok << endl;
                 Assert(0 && "Missing Function");
      }

#undef sys
   tos += Token(tok);
}
//-----------------------------------------------------------------------------
void
Tokenizer::tokenize_quad(Source<Unicode> & src, Token_string & tos)
{
   Log(LOG_tokenize)
      CERR << "tokenize_quad(" << src.rest() << " chars)"<< endl;

UCS_string ucs;
   ucs += src.get();
   Assert(ucs[0] == UNI_QUAD_QUAD);

   ucs += (src.rest() > 0) ? src[0] : Invalid_Unicode;
   ucs += (src.rest() > 1) ? src[1] : Invalid_Unicode;
   ucs += (src.rest() > 2) ? src[2] : Invalid_Unicode;

int len = 0;
const Token t = Workspace::the_workspace->get_quad(ucs, len);
   src.skip(len - 1);
   tos += t;
}
//-----------------------------------------------------------------------------
/** tokenize a single quoted string.
 ** If the string is a single character, then we
 **  return a TOK_CHARACTER. Otherwise we return TOK_APL_VALUE1.
 **/
void
Tokenizer::tokenize_string1(Source<Unicode> & src, Token_string & tos)
{
   Log(LOG_tokenize)   CERR << "tokenize_string1(" << src << ")" << endl;

const Unicode uni = src.get();
   Assert(uni == UNI_SINGLE_QUOTE);

UCS_string string_value;

   while (src.rest())
       {
         const Unicode uni = src.get();

         // a single ' is the end of the string, while a double ' (i.e. '')
         // a single '. 
         //
         if (uni == UNI_SINGLE_QUOTE)
            {
              if ((src.rest() == 0) || (*src != UNI_SINGLE_QUOTE))   break;

              string_value += UNI_SINGLE_QUOTE;
              ++src;      // skip the second '
            }
         else if (uni == UNI_ASCII_CR)
            {
              continue;
            }
         else if (uni == UNI_ASCII_LF)
            {
              rest_2 = src.rest();
              throw_parse_error(E_NO_STRING_END, LOC, loc);
            }
         else
            {
              string_value += uni;
            }
       }

   if (string_value.size() == 1)   // skalar
      {
        tos += Token(TOK_CHARACTER, string_value[0]);
      }
   else if (string_value.size() == 0)
      {
        tos += Token(TOK_APL_VALUE1, &Value::Str0);
      }
   else
      {
        Value_P val = new Value(string_value.size(), LOC);

        loop(l, string_value.size())
      new (&val->get_ravel(l)) CharCell(string_value[l]);

   tos += Token(TOK_APL_VALUE1, val);
      }
}
//-----------------------------------------------------------------------------
/** tokenize a double quoted string.
 ** If the string is a single character, then we
 **  return a TOK_CHARACTER. Otherwise we return TOK_APL_VALUE1.
 **/
void
Tokenizer::tokenize_string2(Source<Unicode> & src, Token_string & tos)
{
   Log(LOG_tokenize)   CERR << "tokenize_string2(" << src << ")" << endl;

const Unicode uni = src.get();
   Assert(uni == UNI_ASCII_DOUBLE_QUOTE);

UCS_string string_value;

   while (src.rest())
       {
         const Unicode uni = src.get();

         if (uni == UNI_ASCII_DOUBLE_QUOTE)
            {
              break;
            }
         else if (uni == UNI_ASCII_CR)
            {
              continue;
            }
         else if (uni == UNI_ASCII_LF)
            {
              rest_2 = src.rest();
              throw_parse_error(E_NO_STRING_END, LOC, loc);
            }
         else
            {
              string_value += uni;
            }
       }

   if (string_value.size() == 1)   // skalar
      {
        tos += Token(TOK_CHARACTER, string_value[0]);
      }
   else if (string_value.size() == 0)
      {
        tos += Token(TOK_APL_VALUE1, &Value::Str0);
      }
   else
      {
        Value_P val = new Value(string_value.size(), LOC);

        loop(l, string_value.size())
           new (&val->get_ravel(l)) CharCell(string_value[l]);

        tos += Token(TOK_APL_VALUE1, val);
      }
}
//-----------------------------------------------------------------------------
void
Tokenizer::tokenize_number(Source<Unicode> & src, Token_string & tos)
{
   Log(LOG_tokenize)   CERR << "tokenize_number(" << src << ")" << endl;

   // numbers:
   // real
   // real 'J' real
   // real 'D' real   // magnitude + angle in degrees
   // real 'R' real   // magnitude + angle in radian

APL_Float real_flt;   // always valid
APL_Integer real_int;   // valid if real_floating is true
const bool real_valid = tokenize_real(src, real_flt, real_int);
bool real_floating = real_flt != real_int;

   if (!real_valid)
      {
        rest_2 = src.rest();
        throw_parse_error(E_BAD_NUMBER, LOC, loc);
      }
   else if (src.rest() && *src == UNI_ASCII_J)
      {
        ++src;   // skip 'J'

        APL_Float imag_flt;   // always valid
        APL_Integer imag_int;   // valid if real_floating is true
        const bool imag_valid =  tokenize_real(src, imag_flt, imag_int);

        if (!imag_valid)
           {
             --src;
             if (real_floating)   tos += Token(TOK_REAL,    real_flt);
             else                 tos += Token(TOK_INTEGER, real_int);
             return;
           }

        tos += Token(TOK_COMPLEX, real_flt, imag_flt);
      }
   else if (src.rest() && *src == UNI_ASCII_D)
      {
        ++src;   // skip 'D'

        APL_Float angle;     // always valid
        APL_Integer imag_int;   // valid if imag_floating is true
        const bool imag_valid =  tokenize_real(src, angle, imag_int);

        if (!imag_valid)
           {
             --src;
             if (real_floating)   tos += Token(TOK_REAL,    real_flt);
             else                 tos += Token(TOK_INTEGER, real_int);
             return;
           }

        // real_flt is the magnitude and the angle is in degrees.
        //
        APL_Float real = cos(M_PI*angle / 180);
        APL_Float imag = sin(M_PI*angle / 180);
        tos += Token(TOK_COMPLEX, real_flt*real, real_flt*real);
      }
   else if (src.rest() && *src == UNI_ASCII_R)
      {
        ++src;   // skip 'R'

        APL_Float angle;     // always valid
        APL_Integer imag_int;   // valid if imag_floating is true
        const bool imag_valid =  tokenize_real(src, angle, imag_int);

        if (!imag_valid)
           {
             --src;
             if (real_floating)   tos += Token(TOK_REAL,    real_flt);
             else                 tos += Token(TOK_INTEGER, real_int);
             return;
           }

        // real_flt is the magnitude and the angle is in radian.
        //
        APL_Float real = cos(angle);
        APL_Float imag = sin(angle);
        tos += Token(TOK_COMPLEX, real_flt*real, real_flt*real);
      }
   else 
     {
         if (real_floating)   tos += Token(TOK_REAL,    real_flt);
         else                 tos += Token(TOK_INTEGER, real_int);
      }
}
//-----------------------------------------------------------------------------
bool
Tokenizer::tokenize_real(Source<Unicode> &src, APL_Float & flt_val,
                         APL_Integer & int_val)
{
const int32_t src_size = src.rest();
bool dot_seen = false;
int E_pos = 0;   // position of 'e' in utf
int minus_pos = 0;
bool digit_seen = false;

   // copy chars from src to utf
   //
UTF8_string utf;

   // a '-' can occur in the mantissa and in the exponent. To make a '-'
   // unique, we handle a leading (i.e. mantissa) '-' beforehand.
   //
   if (*src == UNI_OVERBAR)
      {
        utf += '-';
        ++src;
      }

   for (bool goon = true; goon && src.rest();)
      {
        const Unicode cc = src.get();
        switch(cc)
           {
             case UNI_ASCII_0 ... UNI_ASCII_9:
                  utf += cc;
                  digit_seen = true;
                  break;

             case UNI_ASCII_FULLSTOP:
                  if (dot_seen || E_pos)   return false;   // error
                  utf += '.';
                  dot_seen = true;
                  break;

             case UNI_ASCII_E:
             case UNI_ASCII_e:
                  if (E_pos)   return false;   // a second e or E
                  utf += 'e';
                  E_pos = utf.size();
                  break;

             case UNI_OVERBAR:
                  if (!E_pos)      return false;   // ¯ without E
                  if (minus_pos)   return false;   // a second ¯ in exponent

                  utf += '-';
                  minus_pos = utf.size();
                  break;

             default: --src;   goon = false;
           }
      }

   // E or E¯ without exponent digits
   //
   if (utf.size() == E_pos)       return false;
   if (utf.size() == minus_pos)   return false;
   {
     const int real_cnt = sscanf((const char *) utf.c_str(), "%lg", &flt_val);
     const int int_cnt  = sscanf((const char *) utf.c_str(), "%lld", &int_val);
     if (digit_seen && (real_cnt > 0))   return true;   // valid real number
   }


   // invalid number: restore src
   //
   return false;
}
//-----------------------------------------------------------------------------
void
Tokenizer::tokenize_symbol(Source<Unicode> & src, Token_string & tos)
{
   Log(LOG_tokenize)   CERR << "tokenize_symbol() : " << src.rest() << endl;

UCS_string symbol;
   symbol += src.get();

   while (src.rest())
       {
         const Unicode uni = *src;
         if (!Avec::is_symbol_char(uni))   break;
         symbol += uni;
         ++src;
       }

Symbol * sym = Workspace::the_workspace->symbol_table.lookup_symbol(symbol);
   tos += Token(TOK_SYMBOL, sym);
}
//-----------------------------------------------------------------------------

