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

#include "Bif_F12_FORMAT.hh"
#include "Bif_OPER1_COMMUTE.hh"
#include "Bif_OPER1_EACH.hh"
#include "Bif_OPER2_INNER.hh"
#include "Bif_OPER2_OUTER.hh"
#include "Bif_OPER2_POWER.hh"
#include "Bif_OPER2_RANK.hh"
#include "Bif_OPER1_REDUCE.hh"
#include "Bif_OPER1_SCAN.hh"
#include "CharCell.hh"
#include "Common.hh"
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
#include "Value.icc"
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
   while ((rest_1 = rest_2 = src.rest()) != 0)
      {
        const Unicode uni = *src;
        if (uni == UNI_COMMENT)   break;

        const Token tok = Avec::uni_to_token(uni, LOC);

        Log(LOG_tokenize)
           {
             Source<Unicode> s1(src, 0, 24);
             CERR << "  tokenize(" <<  src.rest() << " chars) sees [tag "
                  << tok.tag_name() << " «" << uni << "»] " << s1;
             if (src.rest() != s1.rest())   CERR << " ...";
             CERR << endl;
           }

        switch(tok.get_Class())
            {
              case TC_END:   // chars without APL meaning
                   rest_2 = src.rest();
                   {
                     UCS_string more("Tokenizer: No token for Unicode U+");
                     more.append_hex(uni, true);
                     throw_tokenize_error(E_NO_TOKEN, more);
                   }
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
                   if (Avec::is_quad(uni))
                      {
                         tokenize_quad(src, tos);
                      }
                   else if (uni == UNI_QUOTE_Quad)
                      {
                        ++src;
                        tos.append(Token(TOK_Quad_QUOTE,
                                   &Workspace::get_v_Quad_QUOTE()), LOC);
                      }
                   else if (uni == UNI_ALPHA)
                      {
                        ++src;
                        tos.append(Token(TOK_ALPHA,
                                   &Workspace::get_v_ALPHA()), LOC);
                      }
                   else if (uni == UNI_ALPHA_UNDERBAR)
                      {
                        ++src;
                        tos.append(Token(TOK_ALPHA_U,
                                   &Workspace::get_v_ALPHA_U()), LOC);
                      }
                   else if (uni == UNI_CHI)
                      {
                        ++src;
                        tos.append(Token(TOK_CHI,
                                   &Workspace::get_v_CHI()), LOC);
                      }
                   else if (uni == UNI_LAMBDA)
                      {
                        ++src;
                        tos.append(Token(TOK_LAMBDA,
                                   &Workspace::get_v_LAMBDA()), LOC);
                      }
                   else if (uni == UNI_OMEGA)
                      {
                        ++src;
                        tos.append(Token(TOK_OMEGA,
                                   &Workspace::get_v_OMEGA()), LOC);
                      }
                   else if (uni == UNI_OMEGA_UNDERBAR)
                      {
                        ++src;
                        tos.append(Token(TOK_OMEGA_U,
                                   &Workspace::get_v_OMEGA_U()), LOC);
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
                   if (tok.get_tag() == TOK_OPER2_INNER && src.rest())
                      {
                        // tok is a dot. This could mean that . is either
                        //
                        // the start of a number:     e.g. +.3
                        // or an operator:            e.g. +.*
                        // or a syntax error:         e.g. Done.
                        //
                        if (src.rest() == 1)   // syntax error
                           throw_parse_error(E_SYNTAX_ERROR, LOC, loc);

                        const Unicode uni_1 = src[1];
                        const Token tok_1 = Avec::uni_to_token(uni_1, LOC);
                        if ((tok_1.get_tag() & TC_MASK) == TC_NUMERIC)
                           tokenize_number(src, tos);
                        else
                           tokenize_function(src, tos);
                      }
                   else
                      {
                        tokenize_function(src, tos);
                      }
                   if (tos.size() >= 2 &&
                       tos.last().get_tag() == TOK_OPER2_INNER &&
                       tos[tos.size() - 2].get_tag() == TOK_JOT)
                      {
                        new (&tos.last()) Token(TOK_OPER2_OUTER,
                                                &Bif_OPER2_OUTER::fun);
                      }
                       
                   break;

              case TC_R_ARROW:
                   ++src;
                   if (src.rest())   tos.append(tok);
                   else              tos.append(Token(TOK_ESCAPE), LOC);
                   break;

              case TC_ASSIGN:
              case TC_L_PARENT:
              case TC_R_PARENT:
              case TC_L_BRACK:
              case TC_R_BRACK:
              case TC_L_CURLY:
              case TC_R_CURLY:
                   ++src;
                   tos.append(tok, LOC);
                   break;

              case TC_DIAMOND:
                   ++src;
                   tos.append(tok, LOC);
                   break;

              case TC_COLON:
                   if (pmode != PM_FUNCTION)
                      {
                        rest_2 = src.rest();
                        throw_parse_error(E_BAD_EXECUTE_CHAR, LOC, loc);
                      }

                   ++src;
                   tos.append(tok, LOC);
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

                   if (tok.get_Id() != ID::No_ID)
                      {
                        CERR << ", Id = " << ID::Id(tok.get_tag() >> 16);
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
const Token tok = tokenize_function(uni);
   tos.append(tok, LOC);
}
//-----------------------------------------------------------------------------
Token
Tokenizer::tokenize_function(Unicode uni)
{
const Token tok = Avec::uni_to_token(uni, LOC);

#define sys(t, f) \
   case TOK_ ## t: return Token(tok.get_tag(), &Bif_ ## f::fun);   break;

   switch(tok.get_tag())
      {
        sys(F0_ZILDE,      F0_ZILDE)
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
        sys(F2_INTER,      F2_INTER)
        sys(F2_LEFT,       F2_LEFT)
        sys(F12_LOGA,      F12_LOGA)
        sys(F12_MINUS,     F12_MINUS)
        sys(F12_NEQUIV,    F12_NEQUIV)
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
        sys(F12_UNION,     F12_UNION)
        sys(F12_WITHOUT,   F12_WITHOUT)

        sys(JOT,           JOT)

        sys(OPER1_COMMUTE, OPER1_COMMUTE)
        sys(OPER1_EACH,    OPER1_EACH)
        sys(OPER2_POWER,   OPER2_POWER)
        sys(OPER2_RANK,    OPER2_RANK)
        sys(OPER1_REDUCE,  OPER1_REDUCE)
        sys(OPER1_REDUCE1, OPER1_REDUCE1)
        sys(OPER1_SCAN,    OPER1_SCAN)
        sys(OPER1_SCAN1,   OPER1_SCAN1)

        sys(OPER2_INNER,   OPER2_INNER)

        default: break;
      }

   // CAUTION: cannot print entire token here because Avec::uni_to_token()
   // inits the token tag but not any token pointers!
   //
   CERR << endl << "Token = " << tok.get_tag() << endl;
   Assert(0 && "Missing Function");

#undef sys
   return tok;
}
//-----------------------------------------------------------------------------
void
Tokenizer::tokenize_quad(Source<Unicode> & src, Token_string & tos)
{
   Log(LOG_tokenize)
      CERR << "tokenize_quad(" << src.rest() << " chars)"<< endl;

   src.get();               // discard (possibly alternative) ⎕
UCS_string ucs(UNI_Quad_Quad);
   Assert(ucs[0]);

   if (src.rest() > 0)   ucs.append(src[0]);
   if (src.rest() > 1)   ucs.append(src[1]);
   if (src.rest() > 2)   ucs.append(src[2]);
   if (src.rest() > 3)   ucs.append(src[3]);
   if (src.rest() > 4)   ucs.append(src[4]);

int len = 0;
const Token t = Workspace::get_quad(ucs, len);
   src.skip(len - 1);
   tos.append(t, LOC);
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
bool got_end = false;

   while (src.rest())
       {
         const Unicode uni = src.get();

         if (uni == UNI_SINGLE_QUOTE)
            {
              // a single ' is the end of the string, while a double '
              // (i.e. '') is a single '. 
              //
              if ((src.rest() == 0) || (*src != UNI_SINGLE_QUOTE))
                 {
                   got_end = true;
                   break;
                 }

              string_value.append(UNI_SINGLE_QUOTE);
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
              string_value.append(uni);
            }
       }

   if (!got_end)   throw_parse_error(E_NO_STRING_END, LOC, loc);

   if (string_value.size() == 1)   // scalar
      {
        tos.append(Token(TOK_CHARACTER, string_value[0]));
      }
   else
      {
        tos.append(Token(TOK_APL_VALUE1,
                         Value_P(new Value(string_value, LOC))), LOC);
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

   // skip the leading "
   {
     const Unicode uni = src.get();
     Assert1(uni == UNI_ASCII_DOUBLE_QUOTE);
   }

UCS_string string_value;
bool got_end = false;

   while (src.rest())
       {
         const Unicode uni = src.get();

         if (uni == UNI_ASCII_DOUBLE_QUOTE)     // terminating "
            {
              got_end = true;
              break;
            }
         else if (uni == UNI_ASCII_CR)          // ignore CR
            {
              continue;
            }
         else if (uni == UNI_ASCII_LF)          // end of line before "
            {
              rest_2 = src.rest();
              throw_parse_error(E_NO_STRING_END, LOC, loc);
            }
         else if (uni == UNI_ASCII_BACKSLASH)   // backslash
            {
              const Unicode uni1 = src.get();
              switch(uni1)
                 {
                   case '0':  string_value.append(UNI_ASCII_NUL);         break;
                   case 'a':  string_value.append(UNI_ASCII_BEL);         break;
                   case 'b':  string_value.append(UNI_ASCII_BS);          break;
                   case 't':  string_value.append(UNI_ASCII_HT);          break;
                   case 'n':  string_value.append(UNI_ASCII_LF);          break;
                   case 'v':  string_value.append(UNI_ASCII_VT);          break;
                   case 'f':  string_value.append(UNI_ASCII_FF);          break;
                   case 'r':  string_value.append(UNI_ASCII_CR);          break;
                   case '[':  string_value.append(UNI_ASCII_ESC);         break;
                   case '"':  string_value.append(UNI_ASCII_DOUBLE_QUOTE);break;
                   case '\\': string_value.append(UNI_ASCII_BACKSLASH);   break;
                   default:   string_value.append(uni);
                              string_value.append(uni1);
                 }
            }
         else
            {
              string_value.append(uni);
            }
       }

   if (!got_end)   throw_parse_error(E_NO_STRING_END, LOC, loc);

   else
      {
        tos.append(Token(TOK_APL_VALUE1,
                         Value_P(new Value(string_value, LOC))), LOC);
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

APL_Float real_flt   = 0.0;   // always valid
APL_Integer real_int = 0;     // valid if need_float is false
bool need_float = false;
const bool real_valid = tokenize_real(src, need_float, real_flt, real_int);
   if (!real_valid)
      {
        rest_2 = src.rest();
        throw_parse_error(E_BAD_NUMBER, LOC, loc);
      }

   if (src.rest() && *src == UNI_ASCII_J)
      {
        ++src;   // skip 'J'

        APL_Float   imag_flt = 0.0;   // always valid
        APL_Integer imag_int = 0;   // valid if need_float is false
        bool need_float = false;
        const bool imag_valid = tokenize_real(src, need_float,
                                              imag_flt, imag_int);

        if (!imag_valid)
           {
             --src;
             if (need_float)   tos.append(Token(TOK_REAL,    real_flt));
             else              tos.append(Token(TOK_INTEGER, real_int));
             return;
           }

        tos.append(Token(TOK_COMPLEX, real_flt, imag_flt));
      }
   else if (src.rest() && *src == UNI_ASCII_D)
      {
        ++src;   // skip 'D'

        APL_Float   degrees_flt = 0;  // always valid
        APL_Integer degrees_int = 0;  // valid if imag_floating is true
        bool need_float = false;
        const bool imag_valid = tokenize_real(src, need_float,
                                              degrees_flt, degrees_int);

        if (!imag_valid)
           {
             --src;
             if (need_float)   tos.append(Token(TOK_REAL,    real_flt));
             else              tos.append(Token(TOK_INTEGER, real_int));
             return;
           }

        // real_flt is the magnitude and the angle is in degrees.
        //
        APL_Float real = cos(M_PI*degrees_flt / 180);
        APL_Float imag = sin(M_PI*degrees_flt / 180);
        tos.append(Token(TOK_COMPLEX, real_flt*real, real_flt*imag));
      }
   else if (src.rest() && *src == UNI_ASCII_R)
      {
        ++src;   // skip 'R'

        APL_Float radian_flt;     // always valid
        APL_Integer radian_int;   // valid if imag_floating is true
        bool need_float = false;
        const bool imag_valid = tokenize_real(src, need_float,
                                              radian_flt, radian_int);

        if (!imag_valid)
           {
             --src;
             if (need_float)   tos.append(Token(TOK_REAL,    real_flt));
             else              tos.append(Token(TOK_INTEGER, real_int));
             return;
           }

        // real_flt is the magnitude and the angle is in radian.
        //
        APL_Float real = cos(radian_flt);
        APL_Float imag = sin(radian_flt);
        tos.append(Token(TOK_COMPLEX, real_flt*real, real_flt*imag));
      }
   else 
     {
         if (need_float)   tos.append(Token(TOK_REAL,    real_flt));
         else              tos.append(Token(TOK_INTEGER, real_int));
      }
}
//-----------------------------------------------------------------------------
bool
Tokenizer::tokenize_real(Source<Unicode> & src, bool & need_float,
                         APL_Float & flt_val, APL_Integer & int_val)
{
   flt_val = 0.0;
   int_val = 0;
   need_float = false;
   
UTF8_string digits;
UTF8_string fract_part;
UTF8_string expo_part;
bool negative = false;
bool expo_negative = false;

   // leading sign
   //
   if (src.rest() && *src == UNI_OVERBAR)
      {
        negative = true;
        ++src;
      }

   // leading zeros in integer part
   //
   while (src.rest() && *src == '0')   ++src;

   // integer part
   //
   while (src.rest() && *src >= '0' && *src <= '9')
      {
        digits.append(src.get());
      }

   // fractional part
   //
   if (src.rest() &&  *src == UNI_ASCII_FULLSTOP)   // fract part present
      {
        ++src;
        while (src.rest() && *src >= '0' && *src <= '9')
           {
             fract_part.append(src.get());
           }
      }

   // exponent part
   //
   if (src.rest() &&
       (*src == UNI_ASCII_E || *src == UNI_ASCII_e))   // exponent present
      {
        need_float = true;
        ++src;   // skip e/E
        if (src.rest() && *src == UNI_OVERBAR)
           {
             ++src;   // skip ¯
             expo_negative = true;
           }

        while (src.rest() && *src >= '0' && *src <= '9')
           {
             expo_part.append(src.get());
           }
      }

int expo = 0;
   loop(d, expo_part.size())   expo = 10*expo + (expo_part[d] - '0');
   if (expo_negative)   expo = - expo;
   expo += digits.size();

   digits.append(fract_part);

   // at this point, digits is the fractional part ff... of 0.ff... ×10⋆expo
   // discard leading fractional 0s and adjust expo accordingly
   //
   while (digits.size() && digits[0] == '0')
      {
        digits.drop_leading(1);   // discard '0'
        --expo;
      }

   // at this point, digits is the fractional part ff... of 0.ff... ×10⋆expo
   // from 0.1000... to 0.9999... Discard digits beyond integer precision.
   //
   // The largest 64-bit integer is 0x7FFFFFFFFFFFFFFF = 9223372036854775807
   // which has 19 decimal digits. Discard all after first 20 digits and
   // round according to the last digit.
   //
   if (digits.size() > 20)   digits.shrink(20);

   if (digits.size() == 20)
      {
        bool carry = false;
        if (digits.last() >= '5')   // round up
           {
             carry = true;
             for (int j = digits.size() - 2; j >= 0; --j)
                 {
                   if (digits[j] == '9')   // propagate carry
                      {
                         digits[j] = '0';
                      }
                   else                    // eat carry
                      {
                         ++digits[j];
                         carry = false;
                         break;
                      }
                 }
           }


        // if carry survived then digits were 0.999... which was then rounded
        // up to 1.000...
        //
        if (carry)
           {
             ++expo;   // 1.0 → 0.1
             digits.shrink(1);
             digits[0] = '1';
           }
        else
           {
             digits.pop();   // discard 20th digit.
           }
      }

   // special cases: all digits 0 or very small number
   //
   if (digits.size() == 0)   return true;   // all digits were 0
   if (expo <= -307)         return true;   // very small number

   Assert(digits.size() <= 19);

   if (expo > 19)   need_float = true;   /// large integer

   // special case: integer between 9223372036854775807 and 9999999999999999999
   //
   if (expo == 19 && digits[0] == '9')
      {
        const char * maxint = "9223372036854775807";
        loop(j, digits.size())
            {
               if (digits[j] < maxint[j])   break;
               if (digits[j] == maxint[j])   continue;
               need_float = true;
               break;
            }
      }

   if (digits.size() > expo)   need_float = true;

   if (need_float)
      {
        if (digits.size() > 17)   digits.shrink(17);

       int64_t v = 0;
        loop(j, digits.size())
          {
            v = 10*v + (digits[j] - '0');
            --expo;
          }

        flt_val = v;
        if (expo > 0)
           {
             if (expo >= 256)   { expo -= 256;   flt_val *= 1E256; }
             if (expo >= 128)   { expo -= 128;   flt_val *= 1E128; }
             if (expo >=  64)   { expo -=  64;   flt_val *= 1E64;  }
             if (expo >=  32)   { expo -=  32;   flt_val *= 1E32;  }
             if (expo >=  16)   { expo -=  16;   flt_val *= 1E16;  }
             if (expo >=   8)   { expo -=   8;   flt_val *= 1E8;   }
             if (expo >=   4)   { expo -=   4;   flt_val *= 1E4;   }
             if (expo >=   2)   { expo -=   2;   flt_val *= 1E2;   }
             if (expo >=   1)   { expo -=   1;   flt_val *= 1E1;   }
           }
        else if (expo < 0)
           {
             if (expo <= -256)   { expo += 256;   flt_val /= 1E256; }
             if (expo <= -128)   { expo += 128;   flt_val /= 1E128; }
             if (expo <=  -64)   { expo +=  64;   flt_val /= 1E64;  }
             if (expo <=  -32)   { expo +=  32;   flt_val /= 1E32;  }
             if (expo <=  -16)   { expo +=  16;   flt_val /= 1E16;  }
             if (expo <=   -8)   { expo +=   8;   flt_val /= 1E8;   }
             if (expo <=   -4)   { expo +=   4;   flt_val /= 1E4;   }
             if (expo <=   -2)   { expo +=   2;   flt_val /= 1E2;   }
             if (expo <=   -1)   { expo +=   1;   flt_val /= 1E1;   }
           }
        if (negative)   flt_val = - flt_val;
      }
   else
      {
        int_val = 0;
        loop(j, digits.size())   int_val = 10*int_val + (digits[j] - '0');
        if (negative)   int_val = - int_val;
        flt_val = int_val;
      }

   return true;
}
//-----------------------------------------------------------------------------
void
Tokenizer::tokenize_symbol(Source<Unicode> & src, Token_string & tos)
{
   Log(LOG_tokenize)   CERR << "tokenize_symbol() : " << src.rest() << endl;

UCS_string symbol;
   symbol.append(src.get());

   while (src.rest())
       {
         const Unicode uni = *src;
         if (!Avec::is_symbol_char(uni))   break;
         symbol.append(uni);
         ++src;
       }

   if (symbol.size() > 2 && symbol[1] == UNI_DELTA  &&
       (symbol[0] == UNI_ASCII_S || symbol[0] == UNI_ASCII_T))
      {
        // S∆ or T∆

        while (src.rest() && *src <= UNI_ASCII_SPACE)   src.get();   // spaces
        UCS_string symbol1(symbol, 2, symbol.size() - 2);   // without S∆/T∆
        Value_P AB(new Value(symbol1, LOC));
        Function * ST = 0;
        if (symbol[0] == UNI_ASCII_S) ST = &Quad_STOP::fun;
        else                          ST = &Quad_TRACE::fun;

        const bool assigned = (src.rest() && *src == UNI_LEFT_ARROW);
        if (assigned)   // dyadic: AB ∆fun
           {
             src.get();                                // skip ←
             Log(LOG_tokenize)
                CERR << "Stop/Trace assigned: " << symbol1 << endl;
             tos.append(Token(TOK_APL_VALUE1, AB));   // left argument of ST
             tos.append(Token(TOK_FUN2, ST));
           }
        else
           {
             Log(LOG_tokenize)
                CERR << "Stop/Trace referenved: " << symbol1 << endl;
             tos.append(Token(TOK_FUN2, ST));
             tos.append(Token(TOK_APL_VALUE1, AB));   // right argument of ST
           }

        return;
      }

Symbol * sym = Workspace::lookup_symbol(symbol);
   tos.append(Token(TOK_SYMBOL, sym));
}
//-----------------------------------------------------------------------------

