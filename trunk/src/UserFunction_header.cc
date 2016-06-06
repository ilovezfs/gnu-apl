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

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>

#include "Error.hh"
#include "Output.hh"
#include "Parser.hh"
#include "StateIndicator.hh"
#include "Symbol.hh"
#include "UserFunction_header.hh"
#include "Value.icc"
#include "Workspace.hh"

//=============================================================================
/// a user-define function signature and its properties
const struct _header_pattern
{
   Fun_signature signature;      ///< a bitmap for header items
   int           symbol_count;   ///< number of header items (excl local vars)
   int           token_count;    ///< number of header token (excl local vars)
   TokenTag      tags[11];       ///< tags of the header token (excl local vars)
} header_patterns[] =            ///< all valid function headers
{
/// function result
#define __Z  TOK_LSYMB, TOK_ASSIGN

/// lambda result (λ)
#define __z  TOK_LAMBDA, TOK_ASSIGN

/// left function argument
#define __A  TOK_SYMBOL

/// left lambda argument
#define __a  TOK_ALPHA

/// left lambda operator argument
#define __au  TOK_ALPHA_U

/// a niladic function
#define __F0 TOK_SYMBOL

/// a monadic function
#define __F1 TOK_SYMBOL

/// a dyadic function
#define __F2 TOK_SYMBOL

/// a monadic operator
#define __OP1 TOK_L_PARENT, TOK_SYMBOL, TOK_SYMBOL, TOK_R_PARENT

/// a monadic operator in lambda
#define __op1 TOK_L_PARENT, TOK_ALPHA_U, TOK_SYMBOL, TOK_R_PARENT

/// a dyadic operator
#define __OP2 TOK_L_PARENT, TOK_SYMBOL, TOK_SYMBOL, TOK_SYMBOL, TOK_R_PARENT

/// a dyadic operator in lambda
#define __op2 TOK_L_PARENT, TOK_ALPHA_U, TOK_SYMBOL, TOK_OMEGA_U, TOK_R_PARENT

/// an axis
#define __x   TOK_L_BRACK,  TOK_CHI, TOK_R_BRACK

/// an axis
#define __X   TOK_L_BRACK,  TOK_SYMBOL, TOK_R_BRACK

/// right function argument
#define __B  TOK_SYMBOL

/// right lambda argument
#define __b  TOK_OMEGA

/// left lambda operator argument
#define __ou  TOK_OMEGA_U

   // niladic
   //
 { SIG_F0             , 1,  1, {           __F0,            } },

 { SIG_Z_F0           , 2,  3, { __Z,      __F0,            } },

   // monadic
   //
 { SIG_F1_B           , 2,  2, {           __F1,       __B, } },
 { SIG_F1_X_B         , 3,  5, {           __F1,  __X, __B, } },
 { SIG_LO_OP1_B       , 3,  5, {           __OP1,      __B, } },
 { SIG_LO_OP1_X_B     , 4,  8, {           __OP1, __X, __B, } },
 { SIG_LO_OP2_RO_B    , 4,  6, {           __OP2,      __B, } },

 { SIG_Z_F1_B         , 3,  4, { __Z,      __F1,       __B, } },
 { SIG_Z_F1_X_B       , 4,  7, { __Z,      __F1,  __X, __B, } },
 { SIG_Z_LO_OP1_B     , 4,  7, { __Z,      __OP1,      __B, } },
 { SIG_Z_LO_OP1_X_B   , 5, 10, { __Z,      __OP1, __X, __B, } },
 { SIG_Z_LO_OP2_RO_B  , 5,  8, { __Z,      __OP2,      __B, } },

 { SIG_Z_F1_B         , 3,  4, { __z,      __F1,       __b, } },
 { SIG_Z_F1_X_B       , 4,  7, { __z,      __F1,  __x, __b, } },
 { SIG_Z_LO_OP1_B     , 4,  7, { __z,      __op1,      __b, } },
 { SIG_Z_LO_OP1_X_B   , 5, 10, { __z,      __op1, __x, __b, } },
 { SIG_Z_LO_OP2_RO_B  , 5,  8, { __z,      __op2,      __b, } },

   // dyadic
   //
 { SIG_A_F2_B         , 3,  3, {      __A, __F2,       __B } },
 { SIG_A_F2_X_B       , 4,  6, {      __A, __F2,  __X, __B } },
 { SIG_A_LO_OP1_B     , 4,  6, {      __A, __OP1,      __B } },
 { SIG_A_LO_OP1_X_B   , 5,  9, {      __A, __OP1, __X, __B } },
 { SIG_A_LO_OP2_RO_B  , 5,  7, {      __A, __OP2,      __B } },

 { SIG_Z_A_F2_B       , 4,  5, { __Z, __A, __F2,       __B } },
 { SIG_Z_A_F2_X_B     , 5,  8, { __Z, __A, __F2,  __X, __B } },
 { SIG_Z_A_LO_OP1_B   , 5,  8, { __Z, __A, __OP1,      __B } },
 { SIG_Z_A_LO_OP1_X_B , 6, 11, { __Z, __A, __OP1, __X, __B } },
 { SIG_Z_A_LO_OP2_RO_B, 6,  9, { __Z, __A, __OP2,      __B } },

 { SIG_Z_A_F2_B       , 4,  5, { __z, __a, __F2,       __b } },
 { SIG_Z_A_F2_X_B     , 5,  8, { __z, __a, __F2,  __x, __b } },
 { SIG_Z_A_LO_OP1_B   , 5,  8, { __z, __a, __op1,      __b } },
 { SIG_Z_A_LO_OP1_X_B , 6, 11, { __z, __a, __op1, __x, __b } },
 { SIG_Z_A_LO_OP2_RO_B, 6,  9, { __z, __a, __op2,      __b } },
};

/// the number of signatures
enum { PATTERN_COUNT = sizeof(header_patterns) / sizeof(*header_patterns) };

//-----------------------------------------------------------------------------
UserFunction_header::UserFunction_header(const UCS_string & text)
  : error(E_SYNTAX_ERROR),
    error_info("Unspecified"),
    sym_Z(0),
    sym_A(0),
    sym_LO(0),
    sym_FUN(0),
    sym_RO(0),
    sym_X(0),
    sym_B(0)
{
UCS_string header_line;

   loop(t, text.size())
       {
         const Unicode uni = text[t];
         if (uni == UNI_ASCII_CR)        ;        // ignore CR
         else if (uni == UNI_ASCII_LF)   break;   // stop at LF
         else                            header_line.append(uni);
       }

   if (header_line.size() == 0)
      {
        error_info = "empty header line";
        return;
      }

   Log(LOG_UserFunction__set_line)
      {
        CERR << "[0] " << header_line << endl;
        // show_backtrace(__FILE__, __LINE__);
      }

   // add a semicolon as a guaranteed end marker.
   // This avoids checks of the header token count
   // 
   header_line.append(Unicode(';'));

Token_string tos;
   {
     const Parser parser(PM_FUNCTION, LOC);
     error = parser.parse(header_line, tos);

     if (error)
        {
          error_info = "parse error in header";
          return;
        }
   }

   // count symbols before first semicolon, allow one symbol too much.
   //
int sym_count = 0;
int tos_idx = 0;
Symbol * symbols[12];
   for (; tos_idx < 12; ++tos_idx)
      {
         if (tos_idx >= tos.size())                   break;
         if (tos[tos_idx].get_tag() == TOK_SEMICOL)   break;
         if (tos[tos_idx].get_Class() == TC_SYMBOL)
            symbols[sym_count++] = tos[tos_idx].get_sym_ptr();
      }

   // find matching signature. If sym_count or tos_idx were too high above,
   // then we will not find them in header_patterns and signal syntax error.
   //
Fun_signature signature = SIG_NONE;
   loop(s, PATTERN_COUNT)
      {
        if (header_patterns[s].symbol_count != sym_count)   continue;
        if (header_patterns[s].token_count != tos_idx)      continue;
        bool match = true;
        loop(t, tos_idx)
           {
             if (tos[t].get_tag() != header_patterns[s].tags[t])    // mismatch
                {
                   match = false;
                   break;
                }
           }

        if (match)
           {
             signature = header_patterns[s].signature;
             break;   // found signature
           }
      }

   if (signature == SIG_NONE)
      {
        error_info = "bad header signature";
        return;
      }

   // note: constructor has set all symbol pointers to 0!
   // store symbol pointers according to signature.
   {
     int sidx = 0;
     if (signature & SIG_Z)    sym_Z   = symbols[sidx++];
     if (signature & SIG_A)    sym_A   = symbols[sidx++];
     if (signature & SIG_LO)   sym_LO  = symbols[sidx++];
     if (signature & SIG_FUN)  sym_FUN = symbols[sidx++];
     if (signature & SIG_RO)   sym_RO  = symbols[sidx++];
     if (signature & SIG_X)    sym_X   = symbols[sidx++];
     if (signature & SIG_B)    sym_B   = symbols[sidx++];

     Assert1(sidx == sym_count);   // otherwise header_patterns is faulty
     Assert1(sym_FUN);

     function_name = sym_FUN->get_name();
   }

   while (tos_idx < (tos.size() - 1))
      {
        if (tos[tos_idx++].get_tag() != TOK_SEMICOL)
           {
             error_info = "semicolon expected in function header";
             return;
           }


        if (tos_idx == tos.size())
           {
             error_info = "trailing semicolon in function header";
             return;
           }

        const TokenTag tag = tos[tos_idx].get_tag();
        if (tag != TOK_SYMBOL && tag != TOK_Quad_CT
                              && tag != TOK_Quad_FC
                              && tag != TOK_Quad_IO
                              && tag != TOK_Quad_PP
                              && tag != TOK_Quad_PR
                              && tag != TOK_Quad_PW
                              && tag != TOK_Quad_RL)
           {
             CERR << "Offending token at " LOC " is: " << tos[tos_idx] << endl;
             error_info = "Bad token in function header";
             return;
           }

        local_vars.push_back(tos[tos_idx++].get_sym_ptr());
      }

   remove_duplicate_local_variables();

   error_info = 0;
   error = E_NO_ERROR;
}
//-----------------------------------------------------------------------------
UserFunction_header::UserFunction_header(Fun_signature sig, int lambda_num)
  : error(E_SYNTAX_ERROR),
    error_info("Unspecified"),
    sym_Z(0),
    sym_A(0),
    sym_LO(0),
    sym_FUN(0),
    sym_RO(0),
    sym_X(0),
    sym_B(0)
{
   function_name.append(UNI_LAMBDA);
   function_name.append_number(lambda_num);

   // make sure that sig is valid
   //
Fun_signature sig1 = (Fun_signature)(sig | SIG_FUN);
bool valid_signature = false;
   loop(p, PATTERN_COUNT)
      {
        if (header_patterns[p].signature == sig1)
          {
            valid_signature = true;
            break;
          }
      }

   if (!valid_signature)   
      {
         error = E_SYNTAX_ERROR;
         error_info = "Invalid signature";
         return;
      }

                       sym_Z  = &Workspace::get_v_LAMBDA();
   if (sig & SIG_A)    sym_A  = &Workspace::get_v_ALPHA();
   if (sig & SIG_LO)   sym_LO = &Workspace::get_v_ALPHA_U();
   if (sig & SIG_RO)   sym_RO = &Workspace::get_v_OMEGA_U();
   if (sig & SIG_B)    sym_B  = &Workspace::get_v_OMEGA();
   if (sig & SIG_X)    sym_X  = &Workspace::get_v_CHI();

   error = E_NO_ERROR;
}
//-----------------------------------------------------------------------------
void
UserFunction_header::pop_local_vars() const
{
   loop(l, label_values.size())   label_values[l].sym->pop();

   loop(l, local_vars.size())   local_vars[l]->pop();

   if (sym_B)    sym_B ->pop();
   if (sym_X)    sym_X ->pop();
   if (sym_RO)   sym_RO->pop();
   if (sym_LO)   sym_LO->pop();
   if (sym_A)    sym_A ->pop();
   if (sym_Z)    sym_Z ->pop();
}
//-----------------------------------------------------------------------------
void
UserFunction_header::print_local_vars(ostream & out) const
{
   if (sym_Z)     out << " " << *sym_Z;
   if (sym_A)     out << " " << *sym_A;
   if (sym_LO)    out << " " << *sym_LO;
   if (sym_RO)    out << " " << *sym_RO;
   if (sym_B)     out << " " << *sym_B;

   loop(l, local_vars.size())   out << " " << *local_vars[l];
}
//-----------------------------------------------------------------------------
void
UserFunction_header::remove_duplicate_local_variables()
{
   // remove local vars that are also labels, arguments or return values.
   // This is to avoid pushing them twice
   //
   remove_duplicate_local_var(sym_Z,   0);
   remove_duplicate_local_var(sym_A,   0);
   remove_duplicate_local_var(sym_LO,  0);
   remove_duplicate_local_var(sym_FUN, 0);
   remove_duplicate_local_var(sym_RO,  0);
   remove_duplicate_local_var(sym_X,   0);
   remove_duplicate_local_var(sym_B,   0);

   loop(l, label_values.size())
      remove_duplicate_local_var(label_values[l].sym, 0);

   loop(l, local_vars.size())
      remove_duplicate_local_var(local_vars[l], l + 1);
}
//-----------------------------------------------------------------------------
void
UserFunction_header::remove_duplicate_local_var(const Symbol * sym,
                                                unsigned int pos)
{
   // remove sym from the vector of local variables. Only the local vars
   // at pos or higher are being removed
   //
   if (sym == 0)   return;   // unused symbol

   while (pos < local_vars.size())
       {
         if (sym == local_vars[pos])
            {
              local_vars[pos] = local_vars.back();
              local_vars.pop_back();
              continue;
            }
         ++pos;
       }
}
//-----------------------------------------------------------------------------
UCS_string
UserFunction_header::lambda_header(Fun_signature sig, int lambda_num)
{
UCS_string u;

   if (sig & SIG_Z)      u.append_utf8("λ←");
   if (sig & SIG_A)      u.append_utf8("⍺ ");
   if (sig & SIG_LORO)   u.append_utf8("(");
   if (sig & SIG_LO)     u.append_utf8("⍶ ");
                         u.append_utf8("λ"); 
                         u.append_number(lambda_num); 
   if (sig & SIG_RO)     u.append_utf8(" ⍹ ");
   if (sig & SIG_LORO)   u.append_utf8(")");
   if (sig & SIG_X)      u.append_utf8("[χ]");
   if (sig & SIG_B)      u.append_utf8(" ⍵");

   return u;
}
//-----------------------------------------------------------------------------
void
UserFunction_header::print_properties(ostream & out, int indent) const
{
UCS_string ind(indent, UNI_ASCII_SPACE);
   if (is_operator())   out << "Operator " << function_name << endl;
   else                 out << "Function " << function_name << endl;

   if (sym_Z)    out << ind << "Result:         " << *sym_Z  << endl;
   if (sym_A)    out << ind << "Left Argument:  " << *sym_A  << endl;
   if (sym_LO)   out << ind << "Left Op Arg:    " << *sym_LO << endl;
   if (sym_RO)   out << ind << "Right Op Arg:   " << *sym_RO << endl;
   if (sym_B)    out << ind << "Right Argument: " << *sym_B  << endl;

   if (local_vars.size())
      {
        out << ind << "Local Variables:";
        loop(l, local_vars.size())   out << " " << *local_vars[l];
        out << endl;
      }

   if (label_values.size())
      {
        out << ind << "Labels:        ";
        loop(l, label_values.size())
           {
             if (l)   out << ",";
             out << " " << *label_values[l].sym
                 << "=" << label_values[l].line;
           }
        out << endl;
      }
}
//-----------------------------------------------------------------------------
void
UserFunction_header::eval_common()
{
   Log(LOG_UserFunction__enter_leave)   CERR << "eval_common()" << endl;

   // push local variables...
   //
   loop(l, local_vars.size())   local_vars[l]->push();

   // push labels...
   //
   loop(l, label_values.size())
       label_values[l].sym->push_label(label_values[l].line);
}
//-----------------------------------------------------------------------------
