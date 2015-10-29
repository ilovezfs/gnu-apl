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

#include <unistd.h>

#include "ArrayIterator.hh"
#include "Avec.hh"
#include "Bif_F12_FORMAT.hh"
#include "CDR.hh"
#include "CharCell.hh"
#include "ComplexCell.hh"
#include "FloatCell.hh"
#include "IndexIterator.hh"
#include "IntCell.hh"
#include "LineInput.hh"
#include "Output.hh"
#include "PointerCell.hh"
#include "PrintOperator.hh"
#include "QuadFunction.hh"
#include "Quad_FX.hh"
#include "Quad_TF.hh"
#include "Tokenizer.hh"
#include "UserFunction.hh"
#include "Value.icc"
#include "Workspace.hh"

extern char **environ;

// ⎕-function instances
//
Quad_AF    Quad_AF   ::_fun;
Quad_AT    Quad_AT   ::_fun;
Quad_DL    Quad_DL   ::_fun;
Quad_EA    Quad_EA   ::_fun;
Quad_EC    Quad_EC   ::_fun;
Quad_ENV   Quad_ENV  ::_fun;
Quad_ES    Quad_ES   ::_fun;
Quad_EX    Quad_EX   ::_fun;
Quad_INP   Quad_INP  ::_fun;
Quad_NA    Quad_NA   ::_fun;
Quad_NC    Quad_NC   ::_fun;
Quad_NL    Quad_NL   ::_fun;
Quad_SI    Quad_SI   ::_fun;
Quad_UCS   Quad_UCS  ::_fun;
Quad_STOP  Quad_STOP ::_fun;            // S∆
Quad_TRACE Quad_TRACE::_fun;           // T∆

// ⎕-function pointers
//
Quad_AF    * Quad_AF   ::fun = &Quad_AF   ::_fun;
Quad_AT    * Quad_AT   ::fun = &Quad_AT   ::_fun;
Quad_DL    * Quad_DL   ::fun = &Quad_DL   ::_fun;
Quad_EA    * Quad_EA   ::fun = &Quad_EA   ::_fun;
Quad_EC    * Quad_EC   ::fun = &Quad_EC   ::_fun;
Quad_ENV   * Quad_ENV  ::fun = &Quad_ENV  ::_fun;
Quad_ES    * Quad_ES   ::fun = &Quad_ES   ::_fun;
Quad_EX    * Quad_EX   ::fun = &Quad_EX   ::_fun;
Quad_INP   * Quad_INP  ::fun = &Quad_INP  ::_fun;
Quad_NA    * Quad_NA   ::fun = &Quad_NA   ::_fun;
Quad_NC    * Quad_NC   ::fun = &Quad_NC   ::_fun;
Quad_NL    * Quad_NL   ::fun = &Quad_NL   ::_fun;
Quad_SI    * Quad_SI   ::fun = &Quad_SI   ::_fun;
Quad_UCS   * Quad_UCS  ::fun = &Quad_UCS  ::_fun;
Quad_STOP  * Quad_STOP ::fun = &Quad_STOP ::_fun;
Quad_TRACE * Quad_TRACE::fun = &Quad_TRACE::_fun;

//=============================================================================
Token
Quad_AF::eval_B(Value_P B)
{
const ShapeItem ec = B->element_count();
Value_P Z(B->get_shape(), LOC);

   loop(v, ec)
       {
         const Cell & cell_B = B->get_ravel(v);

         if (cell_B.is_character_cell())   // Unicode to AV index
            {
              const Unicode uni = cell_B.get_char_value();
              int32_t pos = Avec::find_av_pos(uni);
              if (pos < 0)   new (&Z->get_ravel(v)) IntCell(MAX_AV);
              else           new (&Z->get_ravel(v)) IntCell(pos);
              continue;
            }

         if (cell_B.is_integer_cell())
            {
              const APL_Integer idx = cell_B.get_near_int();
              new (&Z->get_ravel(v))   CharCell(Quad_AV::indexed_at(idx));
              continue;
            }

         DOMAIN_ERROR;
       }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_AT::eval_AB(Value_P A, Value_P B)
{
   // A should be an integer scalar 1, 2, 3, or 4
   //
   if (A->get_rank() > 0)   RANK_ERROR;
const APL_Integer mode = A->get_ravel(0).get_near_int();
   if (mode < 1)   DOMAIN_ERROR;
   if (mode > 4)   DOMAIN_ERROR;

const ShapeItem cols = B->get_cols();
const ShapeItem rows = B->get_rows();
   if (rows == 0)   LENGTH_ERROR;

const int mode_vec[] = { 3, 7, 4, 2 };
const int mode_len = mode_vec[mode - 1];
Shape shape_Z(rows);
   shape_Z.add_shape_item(mode_len);

Value_P Z(shape_Z, LOC);

   loop(r, rows)
      {
        // get the symbol name by stripping trailing spaces
        //
        const ShapeItem b = r*cols;   // start of the symbol name
        UCS_string symbol_name;
        loop(c, cols)
           {
            const Unicode uni = B->get_ravel(b + c).get_char_value();
            if (uni == UNI_ASCII_SPACE)   break;
            symbol_name.append(uni);
           }

        NamedObject * obj = Workspace::lookup_existing_name(symbol_name);
        if (obj == 0)   throw_symbol_error(symbol_name, LOC);

        const Function * function = obj->get_function();
        if (function)             // user defined or system function.
           {
             function->get_attributes(mode, &Z->get_ravel(r*mode_len));
             continue;
           }

        Symbol * symbol = obj->get_symbol();
        if (symbol)               // user defined or system var.
           {
             symbol->get_attributes(mode, &Z->get_ravel(r*mode_len));
             continue;
           }

        // neither function nor variable (e.g. unused name)
        VALUE_ERROR;

        if (Avec::is_quad(symbol_name[0]))   // system function or variable
           {
             int l;
             const Token t(Workspace::get_quad(symbol_name, l));
             if (t.get_Class() == TC_SYMBOL)   // system variable
                {
                  // Assert() if symbol_name is not a function
                  t.get_sym_ptr();
                }
             else
                {
                  // throws SYNTAX_ERROR if t is not a function
                  t.get_function();
                }
           }
        else                                   // user defined
           {
             Symbol * symbol = Workspace::lookup_existing_symbol(symbol_name);
             if (symbol == 0)   VALUE_ERROR;

             symbol->get_attributes(mode, &Z->get_ravel(r*mode_len));
           }
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_DL::eval_B(Value_P B)
{
const APL_time_us start = now();

   // B should be an integer or real scalar
   //
   if (B->get_rank() > 0)                 RANK_ERROR;
   if (!B->get_ravel(0).is_real_cell())   DOMAIN_ERROR;

const APL_time_us end = start + 1000000 * B->get_ravel(0).get_real_value();
   if (end < start)                            DOMAIN_ERROR;
   if (end > start + 31*24*60*60*1000000LL)   DOMAIN_ERROR;   // > 1 month

   for (;;)
       {
         // compute time remaining.
         //
         const APL_time_us remaining_us =  end - now();
         if (remaining_us <= 0)   break;

         const int wait_sec  = remaining_us/1000000;
         const int wait_usec = remaining_us%1000000;
         timeval tv = { wait_sec, wait_usec };
         if (select(0, 0, 0, 0, &tv) == 0)   break;
       }

   // return time elapsed.
   //
Value_P Z(LOC);
   new (&Z->get_ravel(0)) FloatCell(0.000001*(now() - start));

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_EA::eval_AB(Value_P A, Value_P B)
{
ExecuteList * fun = 0;
const UCS_string statement_B(*B.get());
Token constant;

   try
      {
        fun = ExecuteList::fix(statement_B, constant, LOC);
      }
   catch (...)
      {
      }

   if (fun == 0)   // ExecuteList::fix() failed, or constant value parsed
      {
        if (constant.get_tag() == TOK_APL_VALUE1)   return constant;

        // syntax error in B: try to fix A. This will reset ⎕EM and ⎕ET even
        // though we whould keep them (lrm p. 178). Therefore we save the error
        // info from fixing B
        //
        const Error error_B = *Workspace::get_error();
        try
           {
             const UCS_string statement_A(*A.get());
             fun = ExecuteList::fix(statement_A, constant, LOC);
           }
       catch (...)
           {
           }

        if (fun == 0)
           {
             if (constant.get_tag() == TOK_APL_VALUE1)   return constant;
             SYNTAX_ERROR;   // syntax error in A and B: give up.
          }

        // A could be fixed: execute it.
        //
        Log(LOG_UserFunction__execute)   fun->print(CERR);

        Workspace::push_SI(fun, LOC);
        *Workspace::get_error() = error_B;
        Workspace::SI_top()->set_safe_execution(true);

        // install end of context handler. The handler will do nothing when
        // B succeeds, but will execute A if not.
        //
        EOC_arg arg(Value_P(), A, 0, 0, B);
        Workspace::SI_top()->add_eoc_handler(eoc_A_and_B_done, arg, LOC);

        return Token(TOK_SI_PUSHED);
      }

   Assert(fun);

   Log(LOG_UserFunction__execute)   fun->print(CERR);

   Workspace::push_SI(fun, LOC);
   Workspace::SI_top()->set_safe_execution(true);

   // install end of context handler. The handler will do nothing when
   // B succeeds, but will execute A if not.
   //
   {
     EOC_arg arg(Value_P(), A, 0, 0, B);

     Workspace::SI_top()->add_eoc_handler(eoc_B_done, arg, LOC);
   }

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
bool
Quad_EA::eoc_B_done(Token & token)
{
   // in A ⎕EA B, ⍎B was executed and may or may not have failed.
   //
   if (token.get_tag() != TOK_ERROR)   // ⍎B succeeded
      {
        // do not clear ⎕EM and ⎕ET; they should remain visible.

        StateIndicator * si = Workspace::SI_top();
        si->set_safe_execution(false);

        EOC_arg * arg = Workspace::SI_top()->remove_eoc_handlers();
        EOC_arg * next = arg->next;

        delete arg;
        si->set_eoc_handlers(next);
        if (next)   return next->handler(token);

        return false;   // ⍎B successful.
      }

   // in A ⎕EA B, ⍎B has failed...
   //
   // lrm p. 178: "⎕EM and ⎕ET are set, execution of B is abandoned without
   // an error message, and the expression represented by A is executed."
   //
EOC_arg * arg = Workspace::SI_top()->remove_eoc_handlers();
const UCS_string statement_A(*arg->A.get());

ExecuteList * fun = 0;
   try
      {
        fun = ExecuteList::fix(statement_A, token, LOC);
      }
   catch (...)
      {
        // syntax error in A (and B has failed before): give up
        //
        SYNTAX_ERROR;
      }

   // A could be ⎕FXed: execute it.
   //
   if (fun == 0 && token.get_tag() == TOK_APL_VALUE1)
      {
        StateIndicator * si = Workspace::SI_top();
        si->set_safe_execution(false);

        EOC_arg * next = arg->next;

        delete arg;
        si->set_eoc_handlers(next);
        if (next)   return next->handler(token);
        return false;
      }
   Assert(fun);

   Log(LOG_UserFunction__execute)   fun->print(CERR);

   Workspace::pop_SI(LOC);
   Workspace::push_SI(fun, LOC);
   Workspace::SI_top()->get_error().init((ErrorCode)(token.get_int_val()), LOC);

   Workspace::SI_top()->set_safe_execution(true);

   // install end of context handler for the result of ⍎A. 
   //
   {
     EOC_arg arg1(Value_P(), arg->A, 0, 0, arg->B);
     Workspace::SI_top()->add_eoc_handler(eoc_A_and_B_done, arg1, LOC);
   }

   delete arg;
   return true;
}
//-----------------------------------------------------------------------------
bool
Quad_EA::eoc_A_and_B_done(Token & token)
{
   // in A ⎕EA B, ⍎B has failed, and ⍎A was executed and may or
   // may not have failed.
   //
StateIndicator * si = Workspace::SI_top();
EOC_arg * arg = si->remove_eoc_handlers();
EOC_arg * next = arg->next;

   Assert(si);
   si->set_safe_execution(false);

   if (token.get_tag() != TOK_ERROR)   // ⍎A succeeded
      {
        delete arg;
        StateIndicator * si1 = si->get_parent();
        si1->get_error() = si->get_error();

        Workspace::SI_top()->set_eoc_handlers(next);
        if (next)   return next->handler(token);

        return false;   // ⍎A successful.
      }

Value_P A = arg->A;
Value_P B = arg->B;
const UCS_string statement_A(*A.get());

   // here both ⍎B and ⍎A failed. ⎕EM shows only B, but should show
   // A ⎕EA B instead.
   //

   // display of errors was disabled since ⎕EM was incorrect.
   // now we have fixed ⎕EM and can display it.
   //
   si->get_error().print_em(UERR, LOC);
   si->get_error().clear_error_line_1();

UCS_string ucs_2(6, UNI_ASCII_SPACE);
int left_caret = 6;
int right_caret;

const UCS_string statement_B(*B.get());

   right_caret = left_caret + statement_A.size() + 3;

   ucs_2.append(UNI_SINGLE_QUOTE);
   ucs_2.append(statement_A);
   ucs_2.append(UNI_SINGLE_QUOTE);

   ucs_2.append(UNI_ASCII_SPACE);

   ucs_2.append(UNI_Quad_Quad);
   ucs_2.append(UNI_ASCII_E);
   ucs_2.append(UNI_ASCII_A);

   ucs_2.append(UNI_ASCII_SPACE);

   ucs_2.append(UNI_SINGLE_QUOTE);
   ucs_2.append(statement_B);
   ucs_2.append(UNI_SINGLE_QUOTE);

   si->get_error().set_error_line_2(ucs_2, left_caret, right_caret);

   UERR << si->get_error().get_error_line_2() << endl
        << si->get_error().get_error_line_3() << endl;

   delete arg;
   Workspace::SI_top()->set_eoc_handlers(next);
   if (next)   return next->handler(token);
   return false;
}
//=============================================================================
Token
Quad_EC::eval_B(Value_P B)
{
const UCS_string statement_B(*B.get());

ExecuteList * fun = 0;
Token constant;

   try
      {
        fun = ExecuteList::fix(statement_B, constant, LOC);
      }
   catch (...)
      {
      }

   if (fun == 0)
      {
        if (constant.get_tag() == TOK_APL_VALUE1)   return constant;

        // syntax error in B
        //
        Value_P Z(3, LOC);
        Value_P Z1(2, LOC);
        Value_P Z2(Error::error_name(E_SYNTAX_ERROR), LOC);
        new (&Z1->get_ravel(0))   IntCell(Error::error_major(E_SYNTAX_ERROR));
        new (&Z1->get_ravel(1))   IntCell(Error::error_minor(E_SYNTAX_ERROR));

        new (&Z->get_ravel(0)) IntCell(0);        // return code = error
        new (&Z->get_ravel(1)) PointerCell(Z1, Z.getref());   // ⎕ET value
        new (&Z->get_ravel(2)) PointerCell(Z2, Z.getref());   // ⎕EM

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   Assert(fun);

   Log(LOG_UserFunction__execute)   fun->print(CERR);

   Workspace::push_SI(fun, LOC);
   Workspace::SI_top()->set_safe_execution(true);

   // install end of context handler.
   // The handler will create the result after executing B.
   //
   {
     EOC_arg arg(Value_P(), Value_P(), 0, 0, Value_P());

     Workspace::SI_top()->add_eoc_handler(eoc, arg, LOC);
   }

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
bool
Quad_EC::eoc(Token & result_B)
{
StateIndicator * si = Workspace::SI_top();
EOC_arg * arg = si->remove_eoc_handlers();
EOC_arg * next = arg->next;

   si->set_safe_execution(false);

Value_P Z(3, LOC);
Value_P Z2;

int result_type = 0;
ErrorCode ec = E_NO_ERROR;

   switch(result_B.get_tag())
      {
       case TOK_ERROR:
            {
              result_type = 0;
              const Error & err = si->get_error();

              ec = ErrorCode(result_B.get_int_val());

              UCS_string row_1 = Error::error_name(ec);
              UCS_string row_2 = err.get_error_line_2();
              UCS_string row_3 = err.get_error_line_3();
              int cols = row_1.size();
              if (cols < row_2.size())   cols = row_2.size();
              if (cols < row_3.size())   cols = row_3.size();

              const Shape sh_Z2(3, cols);
              Z2 = Value_P(sh_Z2, LOC);   // 3 line message like ⎕EM
              Cell * C2 = &Z2->get_ravel(0);
              loop(c, cols)
                 if (c < row_1.size())   new (C2++) CharCell(row_1[c]);
                 else                    new (C2++) CharCell(UNI_ASCII_SPACE);
              loop(c, cols)
                 if (c < row_2.size())   new (C2++) CharCell(row_2[c]);
                 else                    new (C2++) CharCell(UNI_ASCII_SPACE);
              loop(c, cols)
                 if (c < row_3.size())   new (C2++) CharCell(row_3[c]);
                 else                    new (C2++) CharCell(UNI_ASCII_SPACE);
            }
            break;

        case TOK_APL_VALUE1:
        case TOK_APL_VALUE3:
             result_type = 1;
             Z2 = result_B.get_apl_val();
             break;

        case TOK_APL_VALUE2:
             result_type = 2;
             Z2 = result_B.get_apl_val();
             break;

        case TOK_NO_VALUE:
        case TOK_VOID:
             result_type = 3;
             Z2 = Idx0(LOC);  // 0⍴0
             break;

        case TOK_BRANCH:
             result_type = 4;
             Z2 = Value_P(LOC);
             new (&Z2->get_ravel(0))   IntCell(result_B.get_int_val());
             break;

        case TOK_ESCAPE:
             result_type = 5;
             Z2 = Idx0(LOC);  // 0⍴0
             break;

        default: CERR << "unexpected result tag " << result_B.get_tag()
                      << " in Quad_EC::eoc()" << endl;
                 Assert(0);
      }

Value_P Z1(2, LOC);
   new (&Z1->get_ravel(0)) IntCell(Error::error_major(ec));
   new (&Z1->get_ravel(1)) IntCell(Error::error_minor(ec));

   new (&Z->get_ravel(0)) IntCell(result_type);
   new (&Z->get_ravel(1)) PointerCell(Z1, Z.getref());
   new (&Z->get_ravel(2)) PointerCell(Z2, Z.getref());

   Z->check_value(LOC);
   move_2(result_B, Token(TOK_APL_VALUE1, Z), LOC);

   delete arg;
   Workspace::SI_top()->set_eoc_handlers(next);
   if (next)   return next->handler(result_B);

   return false;
}
//=============================================================================
Token
Quad_ENV::eval_B(Value_P B)
{
   if (!B->is_char_string())   DOMAIN_ERROR;

const ShapeItem ec_B = B->element_count();

vector<const char *> evars;

   for (char **e = environ; *e; ++e)
       {
         const char * env = *e;

         // check if env starts with B.
         //
         bool match = true;
         loop(b, ec_B)
            {
              if (B->get_ravel(b).get_char_value() != Unicode(env[b]))
                 {
                   match = false;
                   break;
                 }
            }

         if (match)   evars.push_back(env);
       }

const Shape sh_Z(evars.size(), 2);
Value_P Z(sh_Z, LOC);

   loop(e, evars.size())
      {
        const char * env = evars[e];
        UCS_string ucs;
        while (*env)
           {
             if (*env != '=')   ucs.append(Unicode(*env++));
             else               break;
           }
        ++env;   // skip '='

        Value_P varname(ucs, LOC);

        ucs.clear();
        while (*env)   ucs.append(Unicode(*env++));

        Value_P varval(ucs, LOC);

        new (Z->next_ravel()) PointerCell(varname, Z.getref());
        new (Z->next_ravel()) PointerCell(varval, Z.getref());
      }

   Z->set_default_Spc();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_ES::eval_AB(Value_P A, Value_P B)
{
const UCS_string ucs(*A.get());
Error error(E_NO_ERROR, LOC);
const Token ret = event_simulate(&ucs, B, error);
   if (error.error_code == E_NO_ERROR)   return ret;

   throw error;
}
//-----------------------------------------------------------------------------
Token
Quad_ES::eval_B(Value_P B)
{
Error error(E_NO_ERROR, LOC);
const Token ret = event_simulate(0, B, error);
   if (error.error_code == E_NO_ERROR)   return ret;

   throw error;
}
//-----------------------------------------------------------------------------
Token
Quad_ES::event_simulate(const UCS_string * A, Value_P B, Error & error)
{
   // B is empty: no action
   //
   if (B->element_count() == 0)   return Token();

   error.init(get_error_code(B), error.throw_loc);

   if (error.error_code == E_NO_ERROR)   // B = 0 0: reset ⎕ET and ⎕EM.
      {
        Workspace::clear_error(LOC);
        return Token();
      }

   if (error.error_code == E_ASSERTION_FAILED)   // B = 0 ASSERTION_FAILED
      {
        Assert(0 && "simulated ASSERTION_FAILED in ⎕ES");
      }

   // at this point we shall throw the error. Add some error details.
   //
   // set up error message 1
   //
   if (A)                                 // A ⎕ES B
      {
        error.error_message_1 = *A;
      }
   else if (error.error_code == E_USER_DEFINED_ERROR)   // ⎕ES with character B
      {
        error.error_message_1 = UCS_string(*B.get());
      }
   else if (error.is_known())             //  ⎕ES B with known major/minor B
      {
        /* error_message_1 already OK */ ;
      }
   else                                   //  ⎕ES B with unknown major/minor B
      {
        error.error_message_1.clear();
      }

   error.show_locked = true;

   Assert(Workspace::SI_top());
   if (StateIndicator * si = Workspace::SI_top()->get_parent())
      {
        const UserFunction * ufun = si->get_executable()->get_ufun();
        if (ufun)
           {
             // lrm p 282: When ⎕ES is executed from within a defined function
             // and B is not empty, the event action is generated as though
             // the function were primitive.
             //
             error.error_message_2 = UCS_string(6, UNI_ASCII_SPACE);
             error.error_message_2.append(ufun->get_name());
             error.left_caret = 6;
             error.right_caret = -1;
             Workspace::pop_SI(LOC);
             Workspace::SI_top()->get_error() = error;
             error.print_em(UERR, LOC);
             return Token();
           }
      }

   Workspace::SI_top()->update_error_info(error);
   return Token();
}
//-----------------------------------------------------------------------------
ErrorCode
Quad_ES::get_error_code(Value_P B)
{
   // B shall be one of:                  → Error code
   //
   // 1. empty, or                        → No error
   // 2. a 2 element integer vector, or   → B[0]/N[1]
   // 3. a simple character vector.       → user defined
   //
   // Otherwise, throw an error

   if (B->get_rank() > 1)   RANK_ERROR;

   if (B->element_count() == 0)   return E_NO_ERROR;
   if (B->is_char_string())       return E_USER_DEFINED_ERROR;
   if (B->element_count() != 2)   LENGTH_ERROR;

const APL_Integer major = B->get_ravel(0).get_near_int();
const APL_Integer minor = B->get_ravel(1).get_near_int();

   return ErrorCode(major << 16 | (minor & 0xFFFF));
}
//=============================================================================
Token
Quad_EX::eval_B(Value_P B)
{
   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem var_count = B->get_rows();
vector<UCS_string> vars(var_count);
   B->to_varnames(vars, false);

Shape sh_Z;
   if (var_count > 1)   sh_Z.add_shape_item(var_count);
Value_P Z(sh_Z, LOC);

   loop(z, var_count)   new (Z->next_ravel()) IntCell(expunge(vars[z]));

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
int
Quad_EX::expunge(UCS_string name)
{
   if (name.size() == 0)   return 0;

Symbol * symbol = Workspace::lookup_existing_symbol(name);
   if (symbol == 0)   return 0;
   return symbol->expunge();
}
//=============================================================================
Token
Quad_INP::eval_AB(Value_P A, Value_P B)
{
Value_P Z(1000, LOC);
   loop(z, Z->element_count())   new (Z->next_ravel())   IntCell(0);
   Z->check_value(LOC);

EOC_arg * arg = new EOC_arg(Z, A, 0, 0, B);

   // B is the end of document marker for a 'HERE document', similar
   // to ENDCAT in cat << ENDCAT.
   //
   // make sure that B is a non-empty string and extract it.
   //
   if (B->get_rank() > 1)         RANK_ERROR;
   if (B->element_count() == 0)   LENGTH_ERROR;

Token tok(TOK_FIRST_TIME);
   Workspace::SI_top()->move_eoc_handler(eoc_INP, arg, LOC);
   eoc_INP(tok);
   return tok;
}
//-----------------------------------------------------------------------------
Token
Quad_INP::eval_B(Value_P B)
{
Value_P Z(1000, LOC);
   loop(z, Z->element_count())   new (Z->next_ravel())   IntCell(0);
   Z->check_value(LOC);

EOC_arg * arg = new EOC_arg(Z, Value_P(), 0, 0, B);

   // B is the end of document marker for a 'HERE document', similar
   // to ENDCAT in cat << ENDCAT.
   //
   // make sure that B is a non-empty string and extract it.
   //
   if (B->get_rank() > 1)         RANK_ERROR;
   if (B->element_count() == 0)   LENGTH_ERROR;

Token tok(TOK_FIRST_TIME);
   Workspace::SI_top()->move_eoc_handler(eoc_INP, arg, LOC);
   eoc_INP(tok);
   return tok;
}
//-----------------------------------------------------------------------------
void
Quad_INP::get_esc(EOC_arg * arg, UCS_string & esc1, UCS_string & esc2,
                  UCS_string & end_marker)
{
   end_marker = arg->B->get_UCS_ravel();
   if (!arg->A)   return;

   // A is either one string (esc1 == esc2) or two (nested) strings
   // for esc1 and esc2 respectively.
   //
   if (arg->A->compute_depth() == 2)   // two (nested) strings
      {
        if (arg->A->get_rank() != 1)        RANK_ERROR;
        if (arg->A->element_count() != 2)   LENGTH_ERROR;

        loop(e, 2)
           {
             const Cell & cell = arg->A->get_ravel(e);
             if (cell.is_pointer_cell())   // char vector
                {
                  if (e)   esc2 = cell.get_pointer_value()->get_UCS_ravel();
                  else     esc1 = cell.get_pointer_value()->get_UCS_ravel();
                }
             else                          // char scalar
                {
                  if (e)   esc2 = cell.get_pointer_value()->get_UCS_ravel();
                  else     esc1 = cell.get_pointer_value()->get_UCS_ravel();
                }
           }

        if (esc1.size() == 0)   LENGTH_ERROR;
      }
   else                       // one string 
      {
        esc1 = arg->A->get_UCS_ravel();
        esc2 = esc1;
      }
}
//-----------------------------------------------------------------------------
bool
Quad_INP::eoc_INP(Token & token)
{
EOC_arg * arg = Workspace::SI_top()->remove_eoc_handlers();
QUAD_INP & _arg = arg->u.u_Quad_INP;
EOC_arg * next = arg->next;

const bool first = token.get_tag() == TOK_FIRST_TIME;

UCS_string line;

   if (token.get_tag() == TOK_ERROR)
      {
         CERR << "Error in ⎕INP" << endl;
         UCS_string err("*** ");
         err.append(Error::error_name((ErrorCode)(token.get_int_val())));
         err.append_utf8(" in ⎕INP ***");
         Value_P val(err, LOC);
         move_2(token, Token(TOK_APL_VALUE1, val), LOC);
      }

   if (!first)
      {
        // token is the result of ⍎ exec and Z has at least 1 line. Split
        // he last line of Z into prefix and suffix and insert result:
        //
        //                       char-string            char-matrix
        //
        //   ...                 ...                    ...
        //   prefix,suffix   →   prefix,result,suffix   prefix,result
        //                                                     ...
        //                                                     result,suffix
        //
        {
          Cell * last_cell = &arg->Z->get_ravel(arg->z);
          line = last_cell->get_pointer_value()->get_UCS_ravel();
          last_cell->release(LOC);
          new (last_cell) IntCell(1);
          --arg->z;
        }

        const ShapeItem diff = line.size() - _arg.prefix_len;   // split point
        UCS_string suffix;
        if (diff > 0)
           {
             suffix = UCS_string(line, _arg.prefix_len, diff);
             line.shrink(_arg.prefix_len);
           }

        Value_P result = token.get_apl_val();
        if (result->is_char_string())
           {
             line.append(result->get_UCS_ravel());
             if (diff > 0)   line.append(suffix);
           }
        else
           {
             // char matrix: insert indented
             //
             Value_P matrix = Bif_F12_FORMAT::monadic_format(result);
             if (matrix->is_char_string())
                {
                  line.append(matrix->get_UCS_ravel());
                  if (diff > 0)   line.append(suffix);
                }
             else
                {
                  const Cell * cm = &matrix->get_ravel(0);

                  expand_Z(arg, matrix->get_rows());
                  loop(r, matrix->get_rows() - 1)
                     {
                       if (r)   // store previous line
                          {
                            Value_P ZZ(line, LOC);
                            Cell * last_cell = &arg->Z->get_ravel(arg->z);
                            new (last_cell)   PointerCell(ZZ, arg->Z.getref());
                            ++arg->z;

                            line = UCS_string(_arg.prefix_len, UNI_ASCII_SPACE);
                          }

                       loop(r, matrix->get_cols())
                           line.append(cm++->get_char_value());
                     }
                }
           }
      }

UCS_string esc1;
UCS_string esc2;
UCS_string end_marker;
   get_esc(arg, esc1, esc2, end_marker);

   for (;;)   // read lines until end reached.
       {
         if (line.size() == 0)
            {
              bool eof = false;
              UCS_string prompt;
              InputMux::get_line(LIM_Quad_INP, prompt, line, eof,
                                 LineHistory::quad_INP_history);
            }
         const bool done = (line.substr_pos(end_marker) != -1);

         if (esc1.size())   // start marker defined (dyadic ⎕INP)
            {
              UCS_string exec;     // line esc1 ... esc2 (excluding)

              const int e1_pos = line.substr_pos(esc1);
              if (e1_pos != -1)   // and the start marker is present.
                 {
                   _arg.prefix_len = e1_pos;

                   UCS_string new_last_line(line, 0, e1_pos);

                   exec = line.drop(e1_pos + esc1.size());
                   exec.remove_lt_spaces();   // no leading and trailing spaces
                   line.clear();

                   if (esc2.size() == 0)
                      {
                        // empty esc2 means exec is the rest of the line.
                        // nothing to do.
                      }
                   else
                      {
                        // non-empty esc2: search for it
                        //
                        const int e2_pos = exec.substr_pos(esc2);
                        if (e2_pos == -1)   // esc2 not found: exec rest of line
                           {
                             // esc2 not in exec: take rest of line
                             // i.e. nothing to do.
                           }
                        else
                           {
                             const int rest = e2_pos + esc2.size();
                             line = UCS_string(exec, rest, exec.size() - rest);
                             new_last_line.append(line);   // append the rest
                             exec.shrink(e2_pos);
                           }
                      }

                   Value_P ZZ(new_last_line, LOC);
                   expand_Z(arg, 1);
                   new (&arg->Z->get_ravel(++arg->z))
                       PointerCell(ZZ, arg->Z.getref());
                 }

              if (exec.size())
                 {
                   // if SI is already pushed (i.e. this is a subsequent
                   // call to eoc_INP() then pop it.
                   //
                   if (token.get_tag() != TOK_FIRST_TIME)
                      Workspace::pop_SI(LOC);

                   move_2(token, Bif_F1_EXECUTE::execute_statement(exec), LOC);
                   Assert(token.get_tag() == TOK_SI_PUSHED);

                   Workspace::SI_top()->move_eoc_handler(eoc_INP, arg, LOC);
                   return true;   // continue
                 }
            }

         if (done || interrupt_raised)   break;

         Value_P ZZ(line, LOC);
         expand_Z(arg, 1);
         new (&arg->Z->get_ravel(++arg->z))   PointerCell(ZZ, arg->Z.getref());
         line.clear();
       }

   // discard unused 0-cells
   //
const Shape shape_Z(arg->z + 1);
   arg->Z->set_shape(shape_Z);

   if (arg->z == 0)   // then Z←⊂''
      {
        Shape sh(0);   // length 0 vector
        Value_P ZZ(sh, LOC);
        ZZ->set_default_Spc();
        new (&arg->Z->get_ravel(0)) PointerCell(ZZ, arg->Z.getref());
      }

   arg->Z->check_value(LOC);
   move_2(token, Token(TOK_APL_VALUE1, arg->Z), LOC);

   delete arg;
   Workspace::SI_top()->set_eoc_handlers(next);
   if (next)   return next->handler(token);

   return false;   // continue
}
//-----------------------------------------------------------------------------
void
Quad_INP::expand_Z(EOC_arg * arg, ShapeItem new_lines)
{
const ShapeItem len_Z = arg->Z->element_count();
   if ((arg->z + new_lines + 5) < len_Z)   return;

Value_P Z1(2*len_Z, LOC);
   loop(z1, len_Z)
      {
        if (arg->Z->get_ravel(z1).is_pointer_cell())
           {
             Value_P sub = arg->Z->get_ravel(z1).get_pointer_value();
             new (Z1->next_ravel())   PointerCell(sub, Z1.getref());
           }
         else
           {
             new (Z1->next_ravel())   IntCell(0);
           }
      }

   loop(z1, len_Z)   new (Z1->next_ravel())   IntCell(0);

   arg->Z = Z1;
}
//-----------------------------------------------------------------------------
Token
Quad_INP::eval_XB(Value_P X, Value_P B)
{
   if (X->element_count() != 1)
      {
        if (X->get_rank() > 1)   RANK_ERROR;
        else                     LENGTH_ERROR;
      }

APL_Integer x = X->get_ravel(0).get_near_int();
   if (x == 0)   return eval_B(B);
   if (x > 1)    DOMAIN_ERROR;

   // B is the end of document marker for a 'HERE document', similar
   // to ENDCAT in cat << ENDCAT.
   //
   // make sure that B is a non-empty string and extract it.
   //
   if (B->get_rank() > 1)         RANK_ERROR;
   if (B->element_count() == 0)   LENGTH_ERROR;

UCS_string end_marker(B->get_UCS_ravel());

vector<UCS_string> lines;
Parser parser(PM_EXECUTE, LOC);

   for (;;)
      {
         bool eof = false;
         UCS_string line;
         UCS_string prompt;
         InputMux::get_line(LIM_Quad_INP, prompt, line, eof,
                            LineHistory::quad_INP_history);
         if (eof)   break;
         const int end_pos = line.substr_pos(end_marker);
         if (end_pos != -1)  break;

         Token_string tos;
         ErrorCode ec = parser.parse(line, tos);
         if (ec != E_NO_ERROR)
            {
              throw_apl_error(ec, LOC);
            }

         if ((tos.size() & 1) == 0)   LENGTH_ERROR;

         // expect APL values at even positions and , at odd positions
         //
         loop(t, tos.size())
            {
              Token & tok = tos[t];
              if (t & 1)   // , or ⍪
                 {
                   if (tok.get_tag() != TOK_F12_COMMA &&
                       tok.get_tag() != TOK_F12_COMMA1)   DOMAIN_ERROR;
                 }
              else         // value
                 {
                   if (tok.get_Class() != TC_VALUE)   DOMAIN_ERROR;
                 }

            }
         lines.push_back(line);
      }

Value_P Z(lines.size(), LOC);
   loop(z, lines.size())
      {
         Token_string tos;
         parser.parse(lines[z], tos);
         const ShapeItem val_count = (tos.size() + 1)/2;
         Value_P ZZ(val_count, LOC);
         loop(v, val_count)
            {
              new (ZZ->next_ravel()) PointerCell(tos[2*v].get_apl_val(),
                                                 ZZ.getref());
            }

         ZZ->check_value(LOC);
         new (Z->next_ravel()) PointerCell(ZZ, Z.getref());
      }

   if (lines.size() == 0)   // empty result
      {
        Value_P ZZ(UCS_string(), LOC);
        new(&Z->get_ravel(0)) PointerCell(ZZ, Z.getref());
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_NC::eval_B(Value_P B)
{
   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem var_count = B->get_rows();
vector<UCS_string> vars(var_count);
   B->to_varnames(vars, false);

Shape sh_Z;
   if (var_count > 1)   sh_Z.add_shape_item(var_count);
Value_P Z(sh_Z, LOC);

   loop(v, var_count)   new (Z->next_ravel())   IntCell(get_NC(vars[v]));

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
APL_Integer
Quad_NC::get_NC(const UCS_string ucs)
{
   if (ucs.size() == 0)   return -1;   // invalid name

const Unicode uni = ucs[0];
Symbol * symbol = 0;
   if      (uni == UNI_ALPHA)            symbol = &Workspace::get_v_ALPHA();
   else if (uni == UNI_LAMBDA)           symbol = &Workspace::get_v_LAMBDA();
   else if (uni == UNI_CHI)              symbol = &Workspace::get_v_CHI();
   else if (uni == UNI_OMEGA)            symbol = &Workspace::get_v_OMEGA();
   else if (uni == UNI_ALPHA_UNDERBAR)   symbol = &Workspace::get_v_OMEGA_U();
   else if (uni == UNI_OMEGA_UNDERBAR)   symbol = &Workspace::get_v_OMEGA_U();
   if (ucs.size() != 1)   symbol = 0;    // unless more than one char

   if (Avec::is_quad(uni))   // distinguished name
      {
        int len = 0;
        const Token t = Workspace::get_quad(ucs, len);
        if (len < 2)                      return 5;    // ⎕ : quad variable
        if (t.get_Class() == TC_VOID)     return -1;   // invalid
        if (t.get_Class() == TC_SYMBOL)   symbol = t.get_sym_ptr();
        else                              return  6;   // quad function
      }

   if (symbol)   // system variable (⎕xx, ⍺, ⍶, ⍵, ⍹, λ, or χ
      {
        const NameClass nc = symbol->get_nc();
        if (nc == 0)   return nc;
        return nc + 3;
      }

   // user-defined name
   //
   symbol = Workspace::lookup_existing_symbol(ucs);

   if (!symbol)
      {
        // symbol not found. Distinguish between invalid and unused names
        //
        if (!Avec::is_first_symbol_char(ucs[0]))   return -1;   // invalid
        loop (u, ucs.size())
           {
             if (!Avec::is_symbol_char(ucs[u]))   return -1;   // invalid
           }
        return 0;   // unused
      }

const NameClass nc = symbol->get_nc();

   // return nc, but map shared variables to regular variables
   //
   return nc == NC_SHARED_VAR ? NC_VARIABLE : nc;
}
//=============================================================================
Token
Quad_NL::do_quad_NL(Value_P A, Value_P B)
{
   if (!!A && !A->is_char_string())   DOMAIN_ERROR;
   if (B->get_rank() > 1)             RANK_ERROR;
   if (B->element_count() == 0)   // nothing requested
      {
        return Token(TOK_APL_VALUE1, Str0_0(LOC));
      }

UCS_string first_chars;
   if (!!A)   first_chars = UCS_string(*A);

   // 1. create a bitmap of ⎕NC values requested in B
   //
int requested_NCs = 0;
   {
     loop(b, B->element_count())
        {
          const APL_Integer bb = B->get_ravel(b).get_near_int();
          if (bb < 1)   DOMAIN_ERROR;
          if (bb > 6)   DOMAIN_ERROR;
          requested_NCs |= 1 << bb;
        }
   }

   // 2, build a name table, starting with user defined names
   //
vector<UCS_string> names;
   {
     const ShapeItem symbol_count = Workspace::symbols_allocated();
     DynArray(Symbol *, table, symbol_count);
     Workspace::get_all_symbols(&table[0], symbol_count);

     loop(s, symbol_count)
        {
          Symbol * symbol = table[s];
          if (symbol->is_erased())   continue;   // erased symbol

          NameClass nc = symbol->get_nc();
          if (nc == NC_SHARED_VAR)   nc = NC_VARIABLE;

          if (!(requested_NCs & 1 << nc))   continue;   // name class not in B

          if (first_chars.size())
             {
               const Unicode first_char = symbol->get_name()[0];
               if (!first_chars.contains(first_char))   continue;
             }

          names.push_back(symbol->get_name());
        }
   }

   // 3, append ⎕-vars and ⎕-functions to name table (unless prevented by A)
   //
   if (first_chars.size() == 0 || first_chars.contains(UNI_Quad_Quad))
      {
#define ro_sv_def(x, _str, _txt)                                   \
   { Symbol * symbol = &Workspace::get_v_ ## x();           \
     if ((requested_NCs & 1 << 5) && symbol->get_nc() != 0) \
        names.push_back(symbol->get_name()); }

#define rw_sv_def(x, _str, _txt)                                   \
   { Symbol * symbol = &Workspace::get_v_ ## x();           \
     if ((requested_NCs & 1 << 5) && symbol->get_nc() != 0) \
        names.push_back(symbol->get_name()); }

#define sf_def(x, _str, _txt)                                      \
   { if (requested_NCs & 1 << 6)   names.push_back((x::fun->get_name())); }
#include "SystemVariable.def"
      }


   // 4. compute length of longest name
   //
ShapeItem longest = 0;
   loop(n, names.size())
      {
        if (longest < names[n].size())
           longest = names[n].size();
      }

const Shape shZ(names.size(), longest);
Value_P Z(shZ, LOC);

   // 5. construct result. The number of symbols is small and ⎕NL is
   // (or should) not be a perfomance // critical function, so we can
   // use a simple (find smallest and print) // approach.
   //
   for (int count = names.size(); count; --count)
      {
        // find smallest.
        //
        uint32_t smallest = count - 1;
        loop(t, count - 1)
           {
             if (names[smallest].compare(names[t]) > 0)   smallest = t;
           }
        // copy name to result, padded with spaces.
        //
        loop(l, longest)
           {
             const UCS_string & ucs = names[smallest];
             new (Z->next_ravel())
                 CharCell(l < ucs.size() ? ucs[l] : UNI_ASCII_SPACE);
           }

        // remove smalles from table
        //
        names[smallest] = names[count - 1];
      }

   Z->set_default_Spc();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_SI::eval_AB(Value_P A, Value_P B)
{
   if (A->element_count() != 1)   // not scalar-like
      {
        if (A->get_rank() > 1)   RANK_ERROR;
        else                     LENGTH_ERROR;
      }
APL_Integer a = A->get_ravel(0).get_near_int();
const ShapeItem len = Workspace::SI_entry_count();
   if (a >= len)   DOMAIN_ERROR;
   if (a < -len)   DOMAIN_ERROR;
   if (a < 0)   a += len;   // negative a counts backwards from end

const StateIndicator * si = 0;
   for (si = Workspace::SI_top(); si; si = si->get_parent())
       {
         if (si->get_level() == a)   break;   // found
       }

   Assert(si);

   if (B->element_count() != 1)   // not scalar-like
      {
        if (B->get_rank() > 1)   RANK_ERROR;
        else                     LENGTH_ERROR;
      }

const Function_PC PC = Function_PC(si->get_PC() - 1);
const Executable * exec = si->get_executable();
const ParseMode pm = exec->get_parse_mode();
const UCS_string & fun_name = exec->get_name();
const Function_Line fun_line = exec->get_line(PC);

Value_P Z;

const APL_Integer b = B->get_ravel(0).get_near_int();
   switch(b)
      {
        case 1:  Z = Value_P(fun_name, LOC);
                 break;

        case 2:  Z = Value_P(LOC);
                new (&Z->get_ravel(0)) IntCell(fun_line);
                break;

        case 3:  {
                   UCS_string fun_and_line(fun_name);
                   fun_and_line.append(UNI_ASCII_L_BRACK);
                   fun_and_line.append_number(fun_line);
                   fun_and_line.append(UNI_ASCII_R_BRACK);
                   Z = Value_P(fun_and_line, LOC); 
                 }
                 break;

        case 4:  if (si->get_error().error_code)
                    {
                      const UCS_string & text =
                                        si->get_error().get_error_line_2();
                      Z = Value_P(text, LOC);
                    }
                 else
                    {
                      const UCS_string text = exec->statement_text(PC);
                      Z = Value_P(text, LOC);
                    }
                 break;

        case 5: Z = Value_P(LOC);
                new (&Z->get_ravel(0)) IntCell(PC);                      break;

        case 6: Z = Value_P(LOC);
                new (&Z->get_ravel(0)) IntCell(pm);                      break;

        default: DOMAIN_ERROR;
      }

   Z->set_default_Zero();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Quad_SI::eval_B(Value_P B)
{
   if (B->element_count() != 1)   // not scalar-like
      {
        if (B->get_rank() > 1)   RANK_ERROR;
        else                     LENGTH_ERROR;
      }

const APL_Integer b = B->get_ravel(0).get_near_int();
const ShapeItem len = Workspace::SI_entry_count();

   if (b < 1)   DOMAIN_ERROR;
   if (b > 4)   DOMAIN_ERROR;

   // at this point we should not fail...
   //
Value_P Z(len, LOC);

ShapeItem z = 0;
   for (const StateIndicator * si = Workspace::SI_top();
        si; si = si->get_parent())
       {
         Cell * cZ = &Z->get_ravel(len - ++z);

         const Function_PC PC = Function_PC(si->get_PC() - 1);
         const Executable * exec = si->get_executable();
         const ParseMode pm = exec->get_parse_mode();
         const UCS_string & fun_name = exec->get_name();
         const Function_Line fun_line = exec->get_line(PC);

         switch (b)
           {
             case 1:  new (cZ) PointerCell(
                                Value_P(fun_name, LOC), Z.getref());
                      break;

             case 2:  new (cZ) IntCell(fun_line);
                      break;

             case 3:  {
                        UCS_string fun_and_line(fun_name);
                        fun_and_line.append(UNI_ASCII_L_BRACK);
                        fun_and_line.append_number(fun_line);
                        fun_and_line.append(UNI_ASCII_R_BRACK);
                        new (cZ) PointerCell(Value_P(
                                    fun_and_line, LOC), Z.getref()); 
                      }
                      break;

             case 4:  if (si->get_error().error_code)
                         {
                           const UCS_string & text =
                                        si->get_error().get_error_line_2();
                           new (cZ) PointerCell(Value_P(text, LOC), Z.getref()); 
                         }
                      else
                         {
                           const UCS_string text = exec->statement_text(PC);
                           new (cZ) PointerCell(Value_P( text, LOC),
                                                Z.getref()); 
                         }
                      break;

             case 5:  new (cZ) IntCell(PC);                             break;
             case 6:  new (cZ) IntCell(pm);                             break;
           }

       }

   Z->set_default_Zero();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_UCS::eval_B(Value_P B)
{
Value_P Z(B->get_shape(), LOC);

const ShapeItem ec = B->nz_element_count();
   loop(v, ec)
       {
         const Cell & cell_B = B->get_ravel(v);

         if (cell_B.is_character_cell())   // char to Unicode
            {
              const Unicode uni = cell_B.get_char_value();
              new (&Z->get_ravel(v)) IntCell(uni);
              continue;
            }

         if (cell_B.is_integer_cell())
            {
              const APL_Integer bint = cell_B.get_near_int();
              new (&Z->get_ravel(v))   CharCell(Unicode(bint));
              continue;
            }

         Q1(cell_B.get_cell_type())
         DOMAIN_ERROR;
       }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
UserFunction *
Stop_Trace::locate_fun(const Value & fun_name)
{
   if (!fun_name.is_char_string())   return 0;

UCS_string fun_name_ucs(fun_name);
   if (fun_name_ucs.size() == 0)   return 0;

Symbol * fun_symbol = Workspace::lookup_existing_symbol(fun_name_ucs);
   if (fun_symbol == 0)
      {
        CERR << "symbol " << fun_name_ucs << " not found" << endl;
        return 0;
      }

Function * fun = fun_symbol->get_function();
   if (fun == 0)
      {
        CERR << "symbol " << fun_name_ucs << " is not a function" << endl;
        return 0;
      }

UserFunction * ufun = fun->get_ufun1();
   if (ufun == 0)
      {
        CERR << "symbol " << fun_name_ucs
             << " is not a defined function" << endl;
        return 0;
      }

   return ufun;
}
//-----------------------------------------------------------------------------
Token
Stop_Trace::reference(const vector<Function_Line> & lines, bool assigned)
{
Value_P Z(lines.size(), LOC);

   loop(z, lines.size())   new (Z->next_ravel()) IntCell(lines[z]);

   Z->set_default_Zero();
   if (assigned)   return Token(TOK_APL_VALUE2, Z);
   else            return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
Stop_Trace::assign(UserFunction * ufun, const Value & new_value, bool stop)
{
DynArray(Function_Line, lines, new_value.element_count());
int line_count = 0;

   loop(l, new_value.element_count())
      {
         APL_Integer line = new_value.get_ravel(l).get_near_int();
         if (line < 1)   continue;
         lines[line_count++] = (Function_Line)line;
      }

   ufun->set_trace_stop(lines.get_data(), line_count, stop);
}
//=============================================================================
Token
Quad_STOP::eval_AB(Value_P A, Value_P B)
{
   // Note: Quad_STOP::eval_AB can be called directly or via S∆. If
   //
   // 1. called via S∆   then A is the function and B are the lines.
   // 2. called directly then B is the function and A are the lines.
   //
UserFunction * ufun = locate_fun(*A);
   if (ufun)   // case 1.
      {
        assign(ufun, *B, true);
        return reference(ufun->get_stop_lines(), true);
      }

   // case 2.
   //
   ufun = locate_fun(*B);
   if (ufun == 0)   DOMAIN_ERROR;

   assign(ufun, *A, true);
   return reference(ufun->get_stop_lines(), true);
}
//-----------------------------------------------------------------------------
Token
Quad_STOP::eval_B(Value_P B)
{
UserFunction * ufun = locate_fun(*B);
   if (ufun == 0)   DOMAIN_ERROR;

   return reference(ufun->get_stop_lines(), false);
}
//=============================================================================
Token
Quad_TRACE::eval_AB(Value_P A, Value_P B)
{
   // Note: Quad_TRACE::eval_AB can be called directly or via S∆. If
   //
   // 1. called via S∆   then A is the function and B are the lines.
   // 2. called directly then B is the function and A are the lines.
   //
UserFunction * ufun = locate_fun(*A);
   if (ufun)   // case 1.
      {
        assign(ufun, *B, false);
        return reference(ufun->get_trace_lines(), true);
      }

   // case 2.
   //
   ufun = locate_fun(*B);
   if (ufun == 0)   DOMAIN_ERROR;

   assign(ufun, *A, false);
   return reference(ufun->get_trace_lines(), true);
}
//-----------------------------------------------------------------------------
Token
Quad_TRACE::eval_B(Value_P B)
{
UserFunction * ufun = locate_fun(*B);
   if (ufun == 0)   DOMAIN_ERROR;

   return reference(ufun->get_trace_lines(), false);
}
//=============================================================================

