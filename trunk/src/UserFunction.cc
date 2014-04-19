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
#include "TestFiles.hh"
#include "UserFunction.hh"
#include "Value.hh"
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

/// a niladic function
#define __F0 TOK_SYMBOL

/// a monadic function
#define __F1 TOK_SYMBOL

/// a dyadic function
#define __F2 TOK_SYMBOL

/// a monadic operator
#define __OP1 TOK_L_PARENT, TOK_SYMBOL, TOK_SYMBOL, TOK_R_PARENT

/// a dyadic operator
#define __OP2 TOK_L_PARENT, TOK_SYMBOL, TOK_SYMBOL, TOK_SYMBOL, TOK_R_PARENT

/// an axis
#define __x   TOK_L_BRACK,  TOK_CHI, TOK_R_BRACK

/// an axis
#define __X   TOK_L_BRACK,  TOK_SYMBOL, TOK_R_BRACK

/// right function argument
#define __B  TOK_SYMBOL

/// right lambda argument
#define __b  TOK_OMEGA

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
 { SIG_Z_LO_OP1_B     , 4,  7, { __z,      __OP1,      __b, } },
 { SIG_Z_LO_OP1_X_B   , 5, 10, { __z,      __OP1, __x, __b, } },
 { SIG_Z_LO_OP2_RO_B  , 5,  8, { __z,      __OP2,      __b, } },

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
 { SIG_Z_A_LO_OP1_B   , 5,  8, { __z, __a, __OP1,      __b } },
 { SIG_Z_A_LO_OP1_X_B , 6, 11, { __z, __a, __OP1, __x, __b } },
 { SIG_Z_A_LO_OP2_RO_B, 6,  9, { __z, __a, __OP2,      __b } },
};

/// the number of signatures
enum { PATTERN_COUNT = sizeof(header_patterns) / sizeof(*header_patterns) };

//-----------------------------------------------------------------------------
UserFunction_header::UserFunction_header(const UCS_string text)
  : from_lambda(false),
    error(E_SYNTAX_ERROR),
    error_cause(0),
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

   if (header_line.size() == 0)   { error_cause = LOC;   return; }

   Log(LOG_UserFunction__set_line)
      {
        CERR << "[0] " << header_line << endl;
        // show_backtrace(__FILE__, __LINE__);
      }

   // add a semicolon as a guaranteed end marker.
   // This to avoids checks of the header token count
   // 
   header_line.append(Unicode(';'));

Token_string tos;
   {
     const Parser parser(PM_FUNCTION, LOC);
     const ErrorCode err = parser.parse(header_line, tos);

     if (err != E_NO_ERROR)   { error = err;   error_cause = LOC;  return; }
   }

   // count symbols before first semicolon, allow one symbol too much.
   //
int sym_count = 0;
int tos_idx = 0;
Symbol * symbols[12];
   for (; tos_idx < 12; ++tos_idx)
      {
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

   if (signature == SIG_NONE)   { error_cause = LOC;   return; }

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

   for (;;)
      {
        if (tos[tos_idx++].get_tag() != TOK_SEMICOL)
           {
             error_cause = LOC;
             return;
           }

        if  (tos_idx >= tos.size())   break;   // local vars done

        const TokenTag tag = tos[tos_idx].get_tag();
        if (tag != TOK_SYMBOL && tag != TOK_Quad_IO
                              && tag != TOK_Quad_CT)
           {
             CERR << "Offending token at " LOC " is: " << tos[tos_idx] << endl;
             return;
           }

        local_vars.push_back(tos[tos_idx++].get_sym_ptr());
      }

   error = E_NO_ERROR;
}
//-----------------------------------------------------------------------------
UserFunction_header::UserFunction_header(Fun_signature sig,
                                         const UCS_string fname)
  : from_lambda(true),
    error(E_SYNTAX_ERROR),
    function_name(fname),
    sym_Z(0),
    sym_A(0),
    sym_LO(0),
    sym_FUN(0),
    sym_RO(0),
    sym_X(0),
    sym_B(0)
{
                       sym_Z = &Workspace::get_v_LAMBDA();
   if (sig & SIG_A)    sym_A = &Workspace::get_v_ALPHA();
   if (sig & SIG_LO)   sym_X = &Workspace::get_v_ALPHA_U();
   if (sig & SIG_RO)   sym_X = &Workspace::get_v_OMEGA_U();
   if (sig & SIG_B)    sym_B = &Workspace::get_v_OMEGA();
   if (sig & SIG_X)    sym_X = &Workspace::get_v_CHI();

   if (sig & SIG_FUN)   // named lambda
      {
         sym_FUN = Workspace::lookup_symbol(fname);
      }

   error = E_NO_ERROR;
}
//-----------------------------------------------------------------------------
Value_P
UserFunction_header::pop_local_vars() const
{
   loop(l, label_values.size())   label_values[l].sym->pop(true);

   loop(l, local_vars.size())   local_vars[l]->pop(true);

   if (sym_B)    sym_B->pop(true);
   if (sym_X)    sym_X->pop(true);
   if (sym_A)    sym_A->pop(true);
   if (sym_LO)   sym_LO->pop(true);
   if (sym_RO)   sym_RO->pop(true);

   if (!sym_Z)   return Value_P();

   return sym_Z->pop(false);
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
UserFunction_header::check_duplicate_symbols()
{
   check_duplicate_symbol(sym_Z);
   check_duplicate_symbol(sym_A);
   check_duplicate_symbol(sym_LO);
   check_duplicate_symbol(sym_FUN);
   check_duplicate_symbol(sym_RO);
   check_duplicate_symbol(sym_B);

   loop(l, local_vars.size())
      check_duplicate_symbol(local_vars[l]);

   loop(l, label_values.size())
      check_duplicate_symbol(label_values[l].sym);
}
//-----------------------------------------------------------------------------
void
UserFunction_header::check_duplicate_symbol(const Symbol * sym)
{
   if (sym == 0)   return;   // unused symbol

int count = 0;
   if (sym == sym_Z)     ++count;
   if (sym == sym_A)     ++count;
   if (sym == sym_LO)    ++count;
   if (sym == sym_FUN)   ++count;
   if (sym == sym_RO)    ++count;
   if (sym == sym_B)     ++count;

   loop(l, local_vars.size())   if (sym == local_vars[l])   ++count;

   loop(l, label_values.size())   if (sym == label_values[l].sym)   ++count;

   if (count <= 1)   return;   // OK

   CERR << "Duplicate symbol ";
   sym->print(CERR);
   CERR << "!!!" << endl;

   DEFN_ERROR;
}
//-----------------------------------------------------------------------------
UCS_string
UserFunction_header::lambda_header(Fun_signature sig, const UCS_string & fname)
{
UCS_string u;

   if (sig & SIG_Z)      u.append_utf8("λ←");
   if (sig & SIG_A)      u.append_utf8("⍺ ");
   if (sig & SIG_LO)   { u.append_utf8("(⍶ ");
                         u.append(fname); 
                         if (sig & SIG_RO)  u.append_utf8(" ⍹ ");
                         u.append_utf8(")");
                       }
                    else
                       {
                         u.append(fname); 
                       }
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

   if (Z())   Z()->push();

   // push local variables...
   //
   loop(l, local_vars.size())   local_vars[l]->push();

   // push labels...
   //
   loop(l, label_values.size())
       label_values[l].sym->push_label(label_values[l].line);
}
//=============================================================================
UserFunction::UserFunction(const UCS_string txt,
                           int & error_line, const char * & error_cause,
                           bool keep_existing, const char * loc,
                           const UTF8_string & _creator)
  : Function(ID_USER_SYMBOL, TOK_FUN2),
    Executable(txt, loc),
    header(txt),
    creator(_creator)
{
   set_creation_time(now());

   exec_properties[0] = 0;
   exec_properties[1] = 0;
   exec_properties[2] = 0;
   exec_properties[3] = 0;

   line_starts.push_back(Function_PC_0);   // will be set later.

   if (header.get_error() != E_NO_ERROR)   // bad header
      {
        error_line = 0;
        error_cause = header.get_error_cause();
        DEFN_ERROR;
      }

   // set Function::tag
   //
   if      (header.RO())   tag = TOK_OPER2;
   else if (header.LO())   tag = TOK_OPER1;
   else if (header.A())    tag = TOK_FUN2;
   else if (header.B())    tag = TOK_FUN1;
   else                    tag = TOK_FUN0;


   // check that the function can be defined.
   //
const char * why_not = header.FUN()->cant_be_defined();
   if (why_not)
      {
         error_cause = why_not;
         DEFN_ERROR;
      }
     
Function * old_function = header.FUN()->get_function();
   if (old_function && keep_existing)
      {
        error_cause = "function exists";
        DEFN_ERROR;
      }

   for (int l = 1; l < get_text_size(); ++l)
      {
        error_line = l;
        line_starts.push_back(Function_PC(body.size()));

        const UCS_string & text = get_text(l);
        parse_body_line(Function_Line(l), text, loc);
        setup_lambdas();
      }

#if 0
   // resolve named lambdas
   //
   loop(l, named_lambdas.size())
      {
        UserFunction * lambda = named_lambdas[l];
        const UCS_string & lambda_name = lambda->get_name();
        loop(b, body.size())
           {
             if (body[b].get_ValueType() != TV_SYM)   continue;

             Symbol * sym = body[b].get_sym_ptr();
             if (!sym)                                continue;

             const UCS_string & sym_name = sym->get_name();
             if (lambda_name != sym_name)             continue;

             move_2(body[b], lambda->get_token(), LOC);
           }
      }
#endif

   Log(LOG_UserFunction__fix)
      {
        CERR << "body.size() is " << body.size() << endl
             << "line_starts.size() is " << line_starts.size() <<endl; 
      }

   // let [0] be the end of the function.
   line_starts[0] = Function_PC(body.size());

   if (header.Z())   body.append(Token(TOK_RETURN_SYMBOL, header.Z()), LOC);
   else              body.append(Token(TOK_RETURN_VOID), LOC);

   header.check_duplicate_symbols();

   if (header.LO())   header.FUN()->set_nc(NC_OPERATOR, this);
   else               header.FUN()->set_nc(NC_FUNCTION, this);

   if (old_function)
      {
        const UserFunction * old_ufun = old_function->get_ufun1();
        Assert(old_ufun);
        delete old_ufun;
      }

   error_line = -1;   // assume no error
}
//-----------------------------------------------------------------------------
UserFunction::UserFunction(Fun_signature sig, const UCS_string & fname,
                           const UCS_string & text, const Token_string & bdy)
  : Function(ID_USER_SYMBOL, TOK_FUN0),
    Executable(sig, fname, text, LOC),
    header(sig, fname),
    creator(UNI_LAMBDA)
{
   exec_properties[0] = 0;
   exec_properties[1] = 0;
   exec_properties[2] = 0;
   exec_properties[3] = 0;

   if      (header.RO())   tag = TOK_OPER2;
   else if (header.LO())   tag = TOK_OPER1;
   else if (header.A())    tag = TOK_FUN2;
   else if (header.B())    tag = TOK_FUN1;
   else                    tag = TOK_FUN0;

   parse_body_line(Function_Line_0, bdy, LOC);
   setup_lambdas();
   line_starts.push_back(Function_PC(bdy.size() - 1));

   if (header.FUN())
      {
        if (header.LO())   header.FUN()->set_nc(NC_OPERATOR, this);
        else               header.FUN()->set_nc(NC_FUNCTION, this);
      }
}
//-----------------------------------------------------------------------------
UserFunction::~UserFunction()
{
   Log(LOG_UserFunction__enter_leave)
      CERR << "Function " << get_name() << " deleted." << endl;
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_()
{
   Log(LOG_UserFunction__enter_leave)
      CERR << "Function " << get_name() << " calls eval_()" << endl;

   if (header.B())   SYNTAX_ERROR;   // not defined niladic

   Workspace::push_SI(this, LOC);

   header.eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_B(Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls eval_B("
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (header.LO())    SYNTAX_ERROR;   // defined as operator

   if (header.B())   header.B()->push_value(B);
   if (header.X())   header.X()->push();
   if (header.A())   header.A()->push();

   header.eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_XB(Value_P X, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls eval_B("
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (header.LO())    SYNTAX_ERROR;   // defined as operator

   if (header.B())   header.B()->push_value(B);
   if (header.X())   header.X()->push_value(X);
   if (header.A())   header.A()->push();

   header.eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_AB(Value_P A, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls eval_AB("
             << Token(TOK_APL_VALUE1, A) << ", "
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (header.LO())    SYNTAX_ERROR;   // defined as operator

   if (header.B())   header.B()->push_value(B);
   if (header.X())   header.X()->push();
   if (header.A())   header.A()->push_value(A);

   header.eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_AXB(Value_P A, Value_P X, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls eval_AB("
             << Token(TOK_APL_VALUE1, A) << ", "
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (header.LO())    SYNTAX_ERROR;   // defined as operator

   if (header.B())   header.B()->push_value(B);
   if (header.X())   header.X()->push_value(X);
   if (header.A())   header.A()->push_value(A);

   header.eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_LB(Token & LO, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls " << __FUNCTION__ << "(";
        print_val_or_fun(CERR, LO) << ", "
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (header.RO())    SYNTAX_ERROR;   // dyadic operator called monadically

                header.B()->push_value(B);
   if (header.X())   header.X()->push();
   if (header.A())   header.A()->push();
   if (LO.is_function())   header.LO()->push_function(LO.get_function());
   else                    header.LO()->push_value(LO.get_apl_val());

   header.eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_LXB(Token & LO, Value_P X, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls " << __FUNCTION__ << "(";
        print_val_or_fun(CERR, LO) << ", "
             << Token(TOK_APL_VALUE1, X) << ", "
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (header.RO())    SYNTAX_ERROR;   // dyadic operator called monadically

                header.B()->push_value(B);
   if (header.X())   header.X()->push_value(X);
   if (header.A())   header.A()->push();
   if (LO.is_function())   header.LO()->push_function(LO.get_function());
   else                    header.LO()->push_value(LO.get_apl_val());

   header.eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_ALB(Value_P A, Token & LO, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls " << __FUNCTION__ << "("
             << Token(TOK_APL_VALUE1, A) << ", " << endl;
        print_val_or_fun(CERR, LO) << ", "
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (!header.A())    SYNTAX_ERROR;   // monadic function called dyadically
   if (header.X())   header.X()->push();
   if (header.RO())    SYNTAX_ERROR;   // defined as dyadic operator

   header.B()->push_value(B);
   header.A()->push_value(A);
   if (LO.is_function())   header.LO()->push_function(LO.get_function());
   else                    header.LO()->push_value(LO.get_apl_val());

   header.eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_ALXB(Value_P A, Token & LO, Value_P X, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls " << __FUNCTION__ << "("
             << Token(TOK_APL_VALUE1, A) << ", " << endl;
        print_val_or_fun(CERR, LO) << ", "
             << Token(TOK_APL_VALUE1, X) << ", "
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (!header.A())    SYNTAX_ERROR;   // monadic function called dyadically
   if (header.RO())    SYNTAX_ERROR;   // defined as dyadic operator

   header.B()->push_value(B);
   if (header.X())   header.X()->push();
   header.A()->push_value(A);
   if (header.X())   header.X()->push_value(X);
   if (LO.is_function())   header.LO()->push_function(LO.get_function());
   else                    header.LO()->push_value(LO.get_apl_val());

   header.eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_LRB(Token & LO, Token & RO, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls " << __FUNCTION__ << "(";
        print_val_or_fun(CERR, LO) << ", ";
        print_val_or_fun(CERR, RO) << ", "
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (!header.RO())    SYNTAX_ERROR;   // not defined as dyadic operator

                 header.B()->push_value(B);
   if (header.X())    header.X()->push();
   if (header.A())    header.A()->push();

   if (LO.is_function())   header.LO()->push_function(LO.get_function());
   else                    header.LO()->push_value(LO.get_apl_val());
   if (RO.is_function())   header.RO()->push_function(RO.get_function());
   else                    header.RO()->push_value(RO.get_apl_val());

   header.eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_LRXB(Token & LO, Token & RO, Value_P X, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls " << __FUNCTION__ << "(";
        print_val_or_fun(CERR, LO) << ", ";
        print_val_or_fun(CERR, RO) << ", "
             << Token(TOK_APL_VALUE1, X) << ", "
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (!header.RO())    SYNTAX_ERROR;   // not defined as dyadic operator

                header.B()->push_value(B);
   if (header.X())   header.X()->push_value(X);
   if (header.A())   header.A()->push();
   if (LO.is_function())   header.LO()->push_function(LO.get_function());
   else                    header.LO()->push_value(LO.get_apl_val());
   if (RO.is_function())   header.RO()->push_function(RO.get_function());
   else                    header.RO()->push_value(RO.get_apl_val());

   header.eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_ALRB(Value_P A, Token & LO, Token & RO, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls " << __FUNCTION__ << "("
             << Token(TOK_APL_VALUE1, A) << ", " << endl;
        print_val_or_fun(CERR, LO) << ", ";
        print_val_or_fun(CERR, RO) << ", "
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (!header.A())    SYNTAX_ERROR;    // defined monadic called dyadic
   if (header.X())      header.X()->push();
   if (!header.RO())    SYNTAX_ERROR;   // defined monadic op called dyadic

                 header.B()->push_value(B);
                 header.A()->push_value(A);
   if (LO.is_function())   header.LO()->push_function(LO.get_function());
   else                    header.LO()->push_value(LO.get_apl_val());
   if (RO.is_function())   header.RO()->push_function(RO.get_function());
   else                    header.RO()->push_value(RO.get_apl_val());

   header.eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_ALRXB(Value_P A, Token & LO, Token & RO,
                         Value_P X, Value_P B)
{
   Log(LOG_UserFunction__enter_leave)
      {
        CERR << "Function " << get_name() << " calls " << __FUNCTION__ << "("
             << Token(TOK_APL_VALUE1, A) << ", " << endl;
        print_val_or_fun(CERR, LO) << ", ";
        print_val_or_fun(CERR, RO) << ", "
             << Token(TOK_APL_VALUE1, X) << ", "
             << Token(TOK_APL_VALUE1, B) << ")" << endl;
      }

   Workspace::push_SI(this, LOC);

   if (!header.A())    SYNTAX_ERROR;    // defined monadic called dyadic
   if (!header.RO())    SYNTAX_ERROR;   // defined monadic op called dyadic

                 header.B()->push_value(B);
   if (header.X())    header.X()->push_value(X);
                 header.A()->push_value(A);
   if (LO.is_function())   header.LO()->push_function(LO.get_function());
   else                    header.LO()->push_value(LO.get_apl_val());
   if (RO.is_function())   header.RO()->push_function(RO.get_function());
   else                    header.RO()->push_value(RO.get_apl_val());

   header.eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
void
UserFunction::set_locked_error_info(Error & error) const
{
UCS_string & message_2 = error.error_message_2;

   if (header.A())
      {
        Value_P val_A = header.A()->get_value();
        if (!!val_A)
           {
             PrintContext pctx(PR_APL_FUN);
             PrintBuffer pb(*val_A, pctx);
             message_2.append(UCS_string(pb, 1, DEFAULT_Quad_PW));
             message_2.append(UNI_ASCII_SPACE);
           }
      }

   message_2.append(header.get_name());

   if (header.B())
      {
        Value_P val_B = header.B()->get_value();
        if (!!val_B)
           {
             message_2.append(UNI_ASCII_SPACE);
             PrintContext pctx(PR_APL_FUN);
             PrintBuffer pb(*val_B, pctx);
             message_2.append(UCS_string(pb, 1, DEFAULT_Quad_PW));
           }
      }

   error.right_caret = error.left_caret + message_2.size() - 7;
}
//-----------------------------------------------------------------------------
UserFunction *
UserFunction::load(const char * workspace, const char * function)
{
UserFunction * fun = 0;

   try
      {
        load(workspace, function, fun);
      }
   catch (Error err)
      {
        delete fun;

        err.print(CERR);
      }
   catch (...)
      {
        delete fun;
        CERR << "Caught some exception\n";
        return 0;
      }

   return fun;
}
//-----------------------------------------------------------------------------
void
UserFunction::load(const char * workspace, const char * function,
                   UserFunction * & fun)
{
char filename[FILENAME_MAX + 10];
   snprintf(filename, FILENAME_MAX + 5,
            "workspaces/%s/%s.fun", workspace, function);

   if (strlen(filename) > FILENAME_MAX)
      {
        CERR << "file name '" << filename << "' is too long" << endl;
        throw_apl_error(E_SYSTEM_LIMIT_FILENAME, LOC);
      }

int in = open(filename, O_RDONLY);
   if (in == -1)
      {
        CERR << "Can't open() workspace file '" 
             << filename << "': " << strerror(errno) << endl;
        throw_apl_error(E_WS_OPEN, LOC);
      }

struct stat st;
   if (fstat(in, &st) == -1)
      {
        CERR << "Can't fstat() workspace file '" 
             << filename << "': " << strerror(errno) << endl;
        close(in);
        throw_apl_error(E_WS_FSTAT, LOC);
      }

off_t len = st.st_size;
void * start = mmap(0, len, PROT_READ, MAP_SHARED, in, 0);

   if (start == (const void *)-1)
      {
        CERR << "Can't mmap() workspace file '" 
             << filename << "': " << strerror(errno) << endl;
        close(in);
        throw_apl_error(E_WS_MMAP, LOC);
      }

UTF8_string utf((const UTF8 *)start, len);

   // skip trailing \r and \n.
   //
   while (len > 0 && (utf[len - 1] == '\r' || utf[len - 1] == '\n'))   --len;
   utf.erase(len);

UCS_string ucs(utf);
   munmap((char *)start, st.st_size);
   close(in);

int error_line = -1;
   fun = fix(ucs, error_line, false, LOC, filename);
}
//-----------------------------------------------------------------------------
Function_PC
UserFunction::pc_for_line(int l) const
{
   if (l < 1)                     return Function_PC(body.size() - 1);
   if (l >= line_starts.size())   return Function_PC(body.size() - 1);
   return line_starts[l];
}
//-----------------------------------------------------------------------------
UserFunction *
UserFunction::fix(const UCS_string & text, int & error_line,
                  bool keep_existing, const char * loc,
                  const UTF8_string & creator)
{
   Log(LOG_UserFunction__fix)
      {
        CERR << "fix pmode=user function:" << endl << text << endl
             <<  "------------------- fix --" << endl;
      }

   if (Workspace::SI_top())   Workspace::SI_top()->set_safe_execution(true);

UserFunction * fun = 0;
const char * error_cause = 0;
   try
      {
        fun = new UserFunction(text, error_line, error_cause,
                               keep_existing, loc, creator);
      }
   catch (Error err)
      {
        err.print(CERR);
        if (Workspace::SI_top())
           Workspace::SI_top()->set_safe_execution(false);
        if (error_cause)   Workspace::more_error() = UCS_string(error_cause);
        return 0;
      }
   catch (...)
      {
        CERR << __FUNCTION__ << "caught something unexpected at " LOC << endl;
        if (Workspace::SI_top())
           Workspace::SI_top()->set_safe_execution(false);
        return 0;
      }

   Log(LOG_UserFunction__fix)
      {
        CERR << " addr " << (const void *)fun << endl;
        fun->print(CERR);
      }

   if (Workspace::SI_top())
      Workspace::SI_top()->set_safe_execution(false);

   // UserFunction::UserFunction() sets error line to -1 if successful.
   // We may have created a UserFunction but with an error. In this case
   // we delete it here.
   //
   if (error_line != -1)
      {
        delete fun;
        return 0;
      }

   return fun;
}
//-----------------------------------------------------------------------------
void
UserFunction::destroy()
{
   // delete will call ~Executable(), which releases the values owned by body.
   delete this;
}
//-----------------------------------------------------------------------------
ostream &
UserFunction::print(ostream & out) const
{
   return out << header.get_name();
/*
   out << "Function header:" << endl;
   if (header.Z())     out << "Result:         " << *header.Z()   << endl;
   if (header.A())     out << "Left Argument:  " << *header.A()   << endl;
   if (header.LO())    out << "Left Op Arg:    " << *header.LO()  << endl;
                       out << "Function:       " << header.get_name() << endl;
   if (header.RO())    out << "Right Op Arg:   " << *header.RO()  << endl;
   if (header.B())     out << "Right Argument: " << *header.B()   << endl;
   return Executable::print(out);
*/
}
//-----------------------------------------------------------------------------
void
UserFunction::print_properties(ostream & out, int indent) const
{
   header.print_properties(out, indent);
UCS_string ind(indent, UNI_ASCII_SPACE);
   out << ind << "Body Lines:     " << line_starts.size() << endl
       << ind << "Creator:        " << get_creator()      << endl;
}
//-----------------------------------------------------------------------------
const UCS_string &
UserFunction::get_name() const
{
   return header.get_name();
}
//-----------------------------------------------------------------------------
UCS_string
UserFunction::get_name_and_line(Function_PC pc) const
{
UCS_string ret = header.get_name();
   ret.append(UNI_ASCII_L_BRACK);

   // pc may point to the next token already. If that is the case then
   // we go back one token.
   //
   if (pc > 0 && body[pc - 1].get_Class() == TC_END)   pc = Function_PC(pc - 1);

const Function_Line line = get_line(pc);
   ret.append_number(line);
   ret.append(UNI_ASCII_R_BRACK);
   return ret;
}
//-----------------------------------------------------------------------------
Function_Line
UserFunction::get_line(Function_PC pc) const
{
   Assert(pc >= 0);
   for (int l = line_starts.size() - 1; l > 0; --l)
       {
         if (line_starts[l] <= pc)   return Function_Line(l);
       }

   return Function_Line_1;
}
//-----------------------------------------------------------------------------
UCS_string
UserFunction::canonical(bool with_lines) const
{
UCS_string ucs;
   loop(t, text.size())
      {
        if (with_lines)   ucs.append(line_prefix(Function_Line(t)));
        ucs.append(text[t]);
        ucs.append(UNI_ASCII_LF);
      }

   return ucs;
}
//-----------------------------------------------------------------------------
ostream &
UserFunction::print_val_or_fun(ostream & out, Token & tok)
{
   if (tok.is_function())         out << *tok.get_function();
   else if (tok.is_apl_val())     out << tok;
   else if (tok.is_void())        out << "((VOID))";
   else                           FIXME;

   return out;
}
//-----------------------------------------------------------------------------
UCS_string
UserFunction::line_prefix(Function_Line l)
{
UCS_string ucs;
   ucs.append(UNI_ASCII_L_BRACK);
   if (l < Function_Line_10)   ucs.append(UNI_ASCII_SPACE);

char cc[40];
   snprintf(cc, sizeof(cc), "%d", l);
   for (const char * s = cc; *s; ++s)   ucs.append(Unicode(*s));

   ucs.append(UNI_ASCII_R_BRACK);
   ucs.append(UNI_ASCII_SPACE);
   return ucs;
}
//-----------------------------------------------------------------------------
