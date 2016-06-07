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
#include "UserFunction.hh"
#include "Value.icc"
#include "Workspace.hh"

Simple_string<Macro *> Macro::all_macros;

//-----------------------------------------------------------------------------
UserFunction::UserFunction(const UCS_string txt,
                           const char * loc, const UTF8_string & _creator,
                           bool tolerant, bool macro)
  : Function(ID::USER_SYMBOL, TOK_FUN2),
    Executable(txt, true, PM_FUNCTION, loc),
    header(txt),
    creator(_creator),
    error_line(0),   // assume header is wrong
    error_info("Unspecified")
{
   if (header.get_error())
      {
        error_info = header.get_error_info();
        return;
      }

   set_creation_time(now());

   exec_properties[0] = 0;
   exec_properties[1] = 0;
   exec_properties[2] = 0;
   exec_properties[3] = 0;

   line_starts.push_back(Function_PC_0);   // will be set later.

   if (header.get_error() != E_NO_ERROR)   // bad header
      {
        error_info = header.get_error_info();
        return;
      }

   // set Function::tag
   //
   if      (header.RO())   tag = TOK_OPER2;
   else if (header.LO())   tag = TOK_OPER1;
   else if (header.A())    tag = TOK_FUN2;
   else if (header.B())    tag = TOK_FUN1;
   else                    tag = TOK_FUN0;

   parse_body(loc, tolerant);
   if (error_line > 0)
      {
        error_info = "Error in function body";
        return;
      }

   error_line = -1;   // no error
   error_info = 0;
}
//-----------------------------------------------------------------------------
UserFunction::UserFunction(Fun_signature sig, int lambda_num,
                           const UCS_string & text, const Token_string & bdy)
  : Function(ID::USER_SYMBOL, TOK_FUN0),
    Executable(sig, lambda_num, text, LOC),
    header(sig, lambda_num),
    creator(UNI_LAMBDA),
    error_line(0),
    error_info("Unspecified")
{
   set_creation_time(now());

   exec_properties[0] = 0;
   exec_properties[1] = 0;
   exec_properties[2] = 0;
   exec_properties[3] = 0;

   if (header.get_error() != E_NO_ERROR)   // bad header
      {
        error_info = header.get_error_info();
        return;
      }

   if      (header.RO())   tag = TOK_OPER2;
   else if (header.LO())   tag = TOK_OPER1;
   else if (header.A())    tag = TOK_FUN2;
   else if (header.B())    tag = TOK_FUN1;
   else                    tag = TOK_FUN0;

   parse_body_line(Function_Line_0, bdy, false, false, LOC);
   setup_lambdas();
   line_starts.push_back(Function_PC(bdy.size() - 1));
   error_line = -1;   // no error
   error_info = 0;
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
   if (header.Z())   header.Z()->push();

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

   if (header.LO())    SYNTAX_ERROR;   // defined as operator

   Workspace::push_SI(this, LOC);

   if (header.Z())   header.Z()->push();
   if (header.A())   header.A()->push();
   if (header.X())   header.X()->push();
   if (header.B())   header.B()->push_value(B);

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

   if (header.LO())    SYNTAX_ERROR;   // defined as operator

   Workspace::push_SI(this, LOC);

   if (header.Z())   header.Z()->push();
   if (header.A())   header.A()->push();
   if (header.X())   header.X()->push_value(X);
   if (header.B())   header.B()->push_value(B);

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

   if (header.LO())    SYNTAX_ERROR;    // defined as operator
   if (!header.A())    VALENCE_ERROR;   // monadic

   Workspace::push_SI(this, LOC);

   if (header.Z())   header.Z()->push();
   if (header.A())   header.A()->push_value(A);
   if (header.X())   header.X()->push();
   if (header.B())   header.B()->push_value(B);

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

   if (header.LO())    SYNTAX_ERROR;    // defined as operator
   if (!header.A())    VALENCE_ERROR;   // monadic

   Workspace::push_SI(this, LOC);

   if (header.Z())   header.Z()->push();
   if (header.A())   header.A()->push_value(A);
   if (header.X())   header.X()->push_value(X);
   if (header.B())   header.B()->push_value(B);

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

   if (header.RO())    SYNTAX_ERROR;   // dyadic operator called monadically

   Workspace::push_SI(this, LOC);

   if (header.Z())         header.Z()->push();
   if (header.A())         header.A()->push();
   if (LO.is_function())   header.LO()->push_function(LO.get_function());
   else                    header.LO()->push_value(LO.get_apl_val());
   if (header.X())         header.X()->push();
                           header.B()->push_value(B);

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

   if (header.RO())    SYNTAX_ERROR;   // dyadic operator called monadically

   Workspace::push_SI(this, LOC);

   if (header.Z())         header.Z()->push();
   if (header.A())         header.A()->push();
   if (LO.is_function())   header.LO()->push_function(LO.get_function());
   else                    header.LO()->push_value(LO.get_apl_val());
   if (header.X())         header.X()->push_value(X);
                           header.B()->push_value(B);

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

   if (header.RO())    SYNTAX_ERROR;    // defined as dyadic operator
   if (!header.A())    VALENCE_ERROR;   // monadic

   Workspace::push_SI(this, LOC);

   if (header.X())         header.X()->push();

   if (header.Z())         header.Z()->push();
                           header.A()->push_value(A);
   if (LO.is_function())   header.LO()->push_function(LO.get_function());
   else                    header.LO()->push_value(LO.get_apl_val());
                           header.B()->push_value(B);

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

   if (header.RO())    SYNTAX_ERROR;    // defined as dyadic operator
   if (!header.A())    VALENCE_ERROR;   // monadic

   Workspace::push_SI(this, LOC);

   if (header.Z())         header.Z()->push();
   if (header.X())         header.X()->push();
                           header.A()->push_value(A);
   if (LO.is_function())   header.LO()->push_function(LO.get_function());
   else                    header.LO()->push_value(LO.get_apl_val());
   if (header.X())         header.X()->push_value(X);
                           header.B()->push_value(B);

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

   if (!header.RO())    SYNTAX_ERROR;   // not defined as dyadic operator

   Workspace::push_SI(this, LOC);

   if (header.Z())         header.Z()->push();
   if (header.A())         header.A()->push();

   if (LO.is_function())   header.LO()->push_function(LO.get_function());
   else                    header.LO()->push_value(LO.get_apl_val());
   if (RO.is_function())   header.RO()->push_function(RO.get_function());
   else                    header.RO()->push_value(RO.get_apl_val());
   if (header.X())         header.X()->push();
                           header.B()->push_value(B);

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

   if (!header.RO())    SYNTAX_ERROR;   // not defined as dyadic operator

   Workspace::push_SI(this, LOC);

   if (header.Z())         header.Z()->push();
   if (header.A())         header.A()->push();
   if (LO.is_function())   header.LO()->push_function(LO.get_function());
   else                    header.LO()->push_value(LO.get_apl_val());
   if (RO.is_function())   header.RO()->push_function(RO.get_function());
   if (header.X())         header.X()->push_value(X);
   else                    header.RO()->push_value(RO.get_apl_val());
                           header.B()->push_value(B);

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

   if (!header.RO())    SYNTAX_ERROR;   // defined monadic op called dyadically
   if (!header.A())    VALENCE_ERROR;   // monadic

   Workspace::push_SI(this, LOC);

   if (header.Z())         header.Z()->push();

                           header.A()->push_value(A);
   if (LO.is_function())   header.LO()->push_function(LO.get_function());
   else                    header.LO()->push_value(LO.get_apl_val());
   if (RO.is_function())   header.RO()->push_function(RO.get_function());
   else                    header.RO()->push_value(RO.get_apl_val());
   if (header.X())         header.X()->push();
                           header.B()->push_value(B);

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

   if (!header.RO())   SYNTAX_ERROR;   // defined monadic op called dyadically
   if (!header.A())    VALENCE_ERROR;   // monadic

   Workspace::push_SI(this, LOC);

   if (header.Z())         header.Z()->push();
                           header.A()->push_value(A);
   if (LO.is_function())   header.LO()->push_function(LO.get_function());
   else                    header.LO()->push_value(LO.get_apl_val());
   if (RO.is_function())   header.RO()->push_function(RO.get_function());
   else                    header.RO()->push_value(RO.get_apl_val());
   if (header.X())         header.X()->push_value(X);
                           header.B()->push_value(B);

   header.eval_common();

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_fill_B(Value_P B)
{
Value_P Z = B->clone(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
UserFunction::eval_fill_AB(Value_P A, Value_P B)
{
Value_P Z = B->clone(LOC);
   return Token(TOK_APL_VALUE1, Z);
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
             PrintBuffer pb(*val_A, pctx, 0);
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
             PrintBuffer pb(*val_B, pctx, 0);
             message_2.append(UCS_string(pb, 1, DEFAULT_Quad_PW));
           }
      }

   error.right_caret = error.left_caret + message_2.size() - 7;
}
//-----------------------------------------------------------------------------
void
UserFunction::set_trace_stop(Function_Line * lines, int line_count, bool stop)
{
   // Sort lines
   //
DynArray(bool, ts_lines, line_starts.size());
   loop(ts, line_starts.size())   ts_lines[ts] = false;
   loop(ll, line_count)
      {
        Function_Line line = lines[ll];
        if (line >= 1 && line < (int)line_starts.size())  ts_lines[line] = true;
      }

   if (stop)
      {
        stop_lines.clear();
        loop(ts, line_starts.size())
           {
             if (ts_lines[ts])   stop_lines.push_back((Function_Line)ts);
           }
      }
   else
      {
        trace_lines.clear();
        loop(ts, line_starts.size())
           {
             if (ts_lines[ts])   trace_lines.push_back((Function_Line)ts);
           }
      }

   parse_body(LOC, false);
}
//-----------------------------------------------------------------------------
void
UserFunction::parse_body(const char * loc, bool tolerant)
{
   line_starts.clear();
   line_starts.push_back(Function_PC_0);   // will be set later.

   clear_body();

   for (int l = 1; l < get_text_size(); ++l)
      {
        bool stop_line = false;
        loop(s, stop_lines.size())
           {
             if (stop_lines[s] == l)
                {
                  stop_line = true;
                  break;
                }
           }

        bool trace_line = false;
        loop(t, trace_lines.size())
           {
             if (trace_lines[t] == l)
                {
                  trace_line = true;
                  break;
                }
           }

        error_line = l;   // assume error
        line_starts.push_back(Function_PC(body.size()));

        if (stop_line)
           {
             body.append(Token(TOK_STOP_LINE));
             const int64_t tr = 0;
             body.append(Token(TOK_END, tr));
           }

        const UCS_string & line = get_text(l);
        ErrorCode ec = E_SYNTAX_ERROR;
        try {
              ec = parse_body_line(Function_Line(l), line, trace_line,
                                   tolerant, loc);

              if (tolerant && ec != E_NO_ERROR)
                 {
                   UCS_string new_line = "## ";
                   new_line.append(line);
                   text[l] = new_line;
                   CERR << "WARNING: SYNTAX ERROR in function "
                        << header.get_name() << endl;
                 }
            }
        catch(Error err)
            {
              return;
            }
      }

   error_line = -1;   // OK
   setup_lambdas();

   Log(LOG_UserFunction__fix)
      {
        CERR << "body.size() is " << body.size() << endl
             << "line_starts.size() is " << line_starts.size() <<endl; 
      }

   // let [0] be the end of the function.
   line_starts[0] = Function_PC(body.size());

   if (header.Z())   body.append(Token(TOK_RETURN_SYMBOL, header.Z()), LOC);
   else              body.append(Token(TOK_RETURN_VOID), LOC);
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
   fun = fix(ucs, error_line, false, LOC, filename, false);
}
//-----------------------------------------------------------------------------
Function_PC
UserFunction::pc_for_line(Function_Line line) const
{
   if (line < 1)                          return Function_PC(body.size() - 1);
   if (line >= (int)line_starts.size())   return Function_PC(body.size() - 1);
   return line_starts[line];
}
//-----------------------------------------------------------------------------
UserFunction *
UserFunction::fix(const UCS_string & text, int & err_line,
                  bool keep_existing, const char * loc,
                  const UTF8_string & creator, bool tolerant)
{
   Log(LOG_UserFunction__fix)
      {
        CERR << "fix pmode=user function:" << endl << text << endl
             <<  "------------------- UserFunction::fix() --" << endl;
      }

   if (Workspace::SI_top())   Workspace::SI_top()->set_safe_execution(true);

UserFunction * ufun = new UserFunction(text, loc, creator, tolerant, false);
   if (Workspace::SI_top())   Workspace::SI_top()->set_safe_execution(false);

const char * info = ufun->get_error_info();
   err_line = ufun->get_error_line();

   if (info || err_line != -1)   // something went wrong
      {
        if (info)
           {
             Log(LOG_UserFunction__fix)   CERR << "Error: " << info << endl;
             Workspace::more_error() = UCS_string(info);
           }

         if (err_line == 0)
           {
             Workspace::more_error().append_utf8(" (function header)");
             Log(LOG_UserFunction__fix) CERR << "Bad header line" <<  endl;
           }
         else if (err_line > 0)
           {
             Workspace::more_error().append_utf8(" (function line [");
             Workspace::more_error().append_number(err_line);
             Workspace::more_error().append_utf8("] of:\n");
             Workspace::more_error().append(text);

             Log(LOG_UserFunction__fix)
                CERR << "Bad function line: " << err_line << endl;
           }

        delete ufun;
        return 0;
      }

Symbol * symbol = Workspace::lookup_symbol(ufun->header.get_name());
Function * old_function = symbol->get_function();
   if (old_function && keep_existing)
      {
        err_line = 0;
        delete ufun;
        return 0;
      }

   // check that the function can be defined (e.g. is not on the )SI stack)
   //
   if (old_function && symbol->cant_be_defined())
      {
        err_line = 0;
        delete ufun;
        return 0;
      }
     
   if (old_function)
      {
        const UserFunction * old_ufun = old_function->get_ufun1();
        Assert(old_ufun);
        delete old_ufun;
      }

   // bind function to symbol
   //
   if (ufun->header.LO())   ufun->header.FUN()->set_nc(NC_OPERATOR, ufun);
   else                     ufun->header.FUN()->set_nc(NC_FUNCTION, ufun);

   Log(LOG_UserFunction__fix)
      {
        CERR << " addr " << (const void *)ufun << endl;
        ufun->print(CERR);
      }

   return ufun;
}
//-----------------------------------------------------------------------------
UserFunction *
UserFunction::fix_lambda(Symbol & var, const UCS_string & text)
{
int signature = SIG_FUN | SIG_Z;
int t = 0;

   while (t < text.size())
       {
         switch(text[t++])
            {
              case UNI_CHI:            signature |= SIG_X;    continue;
              case UNI_OMEGA:          signature |= SIG_B;    continue;
              case UNI_ALPHA_UNDERBAR: signature |= SIG_LO;   continue;
              case UNI_OMEGA_UNDERBAR: signature |= SIG_RO;   continue;
              case UNI_ALPHA:          signature |= SIG_A;    continue;
              case UNI_ASCII_LF:       break;   // header done
              default:                 continue;
            }

         break;   // header done
       }

UCS_string body_text;
   for (; t < text.size(); ++t)   body_text.append(text[t]);

   while (body_text.last() == UNI_ASCII_LF)  body_text.pop();

Token_string body;
   {
     Token ret_lambda(TOK_RETURN_SYMBOL, &Workspace::get_v_LAMBDA());
     body.append(ret_lambda);
     const int64_t trace = 0;
     Token tok_endl(TOK_ENDL, trace);
     body.append(tok_endl);
   }

const Parser parser(PM_FUNCTION, LOC);
const ErrorCode ec = parser.parse(body_text, body);
   if (ec)
      {
        CERR << "Parsing '" << body_text << "' failed" << endl;
        return 0;
      }

UserFunction * ufun = new UserFunction((Fun_signature)signature, 0,
                                       body_text, body);
   return ufun;
}
//-----------------------------------------------------------------------------
void
UserFunction::destroy()
{
   // delete will call ~Executable(), which releases the values owned by body.
   //
   if (is_lambda())   decrement_refcount(LOC);
   else               delete this;
}
//-----------------------------------------------------------------------------
ostream &
UserFunction::print(ostream & out) const
{
   out << header.get_name();
   return out;

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
   out << ind << "Body Lines:      " << line_starts.size() << endl
       << ind << "Creator:         " << get_creator()      << endl
       << ind << "Body: " << body << endl;
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
   Assert(pc >= -1);
   if (pc < 0)   pc = Function_PC_0;

   // search line_starts backwards until a line with non-greater pc is found.
   //
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
void
UserFunction::adjust_line_starts()
{
   // this function is called from Executable::setup_lambdas() just before
   // Parser::remove_void_token(body) in order to adjust line_starts
   //
DynArray(ShapeItem, gaps, line_starts.size());   // count TOK_VOID in every line
   loop(ls, line_starts.size())
      {
         gaps[ls] = 0;
         if (ls == 0)   continue;   // function header (has no TOK_VOID)

         const ShapeItem from = line_starts[ls];
         ShapeItem to = body.size();    // end of function (for last line)
         if (ls < (line_starts.size() - 1))   to = line_starts[ls + 1];

        for (ShapeItem b = from; b < to; ++b)
            {
             if (body[b].get_tag() == TOK_VOID)   ++gaps[ls];
            }

      }

int total_gaps = 0;
   loop(ls, line_starts.size())
       {
          line_starts[ls] = Function_PC(line_starts[ls] - total_gaps);
          total_gaps += gaps[ls];
       }
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
Macro::Macro(Macro * & _owner, const char * text)
   : UserFunction(UCS_string(UTF8_string(text)), LOC, "Macro::Macro()",
                  false, true),
     owner(_owner)
{
   CERR << "MACRO: " << endl << text;

   if (error_info || (error_line != -1))   // something went wrong
      {
        if (error_info)         CERR << "error_info: " << error_info << endl;
        if (error_line != -1)   CERR << "error_line: " << error_line << endl;
        return;
      }

   all_macros.append(this);
}
//-----------------------------------------------------------------------------
Macro::~Macro()
{
   CERR << "DELETED MACRO" << endl;
   owner = 0;
}
//-----------------------------------------------------------------------------
void
Macro::clear_all()
{
   while (all_macros.size())
      {
        delete all_macros.last();
        all_macros.pop();
      }
}
//-----------------------------------------------------------------------------
