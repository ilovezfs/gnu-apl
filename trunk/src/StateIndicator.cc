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

#include <iomanip>

#include "Command.hh"
#include "Executable.hh"
#include "IndexExpr.hh"
#include "Output.hh"
#include "Prefix.hh"
#include "StateIndicator.hh"
#include "SystemLimits.hh"
#include "UserFunction.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
StateIndicator::StateIndicator(Executable * exec, StateIndicator * _par)
   : executable(exec),
     safe_execution(false),
     eoc_handler(0),
     eoc_arg(Value_P()),
     level(_par ? 1 + _par->get_level() : 0),
     error(E_NO_ERROR, LOC),
     current_stack(*this, exec->get_body()),
     parent(_par)
{
}
//-----------------------------------------------------------------------------
StateIndicator::~StateIndicator()
{
   // flush the FIFO. Do that before delete executable so that values
   // copied directly from the body of the executable are not killed.
   //
   current_stack.clean_up();

   // if executable is a user defined function then pop its local vars.
   // otherwise delete the body token
   //
   if (executable->get_parse_mode() == PM_FUNCTION)
      {
        const UserFunction * ufun = get_executable()->get_ufun();
        if (ufun)   ufun->pop_local_vars();
      }
   else
      {
         Assert1(executable);
         delete executable;
         executable = 0;
      }
}
//-----------------------------------------------------------------------------
void
StateIndicator::goon(Function_Line new_line, const char * loc)
{
const Function_PC pc = get_executable()->get_ufun()->pc_for_line(new_line);

   Log(LOG_StateIndicator__push_pop)
      CERR << "Continue SI[" << level << "] at line " << new_line
           << " pc=" << pc << " at " << loc << endl;

   if (get_executable()->get_body()[pc].get_tag() == TOK_STOP_LINE)   // S∆
      {
        // pc points to a S∆ token. We are jumping back from immediate
        // execution, so we don't want to stop again.
        //
        set_PC(pc + 2);
      }
   else
      {
        set_PC(pc);
      }

   Log(LOG_prefix_parser)   CERR << "GOTO [" << get_line() << "]" << endl;

   current_stack.reset(LOC);
}
//-----------------------------------------------------------------------------
void
StateIndicator::retry(const char * loc)
{
   Log(LOG_StateIndicator__push_pop || LOG_prefix_parser)
      CERR << endl << "RETRY " << loc << ")" << endl;
}
//-----------------------------------------------------------------------------
UCS_string
StateIndicator::function_name() const
{
   Assert(executable);

   switch(executable->get_parse_mode())
      {
        case PM_FUNCTION:
             return executable->get_name();

        case PM_STATEMENT_LIST:
             {
               UCS_string ret;
               ret.append(UNI_DIAMOND);
               return ret;
             }

        case PM_EXECUTE:
             {
               UCS_string ret;
               ret.append(UNI_EXECUTE);
               return ret;
             }
      }

   Assert(0);
} 
//-----------------------------------------------------------------------------
void
StateIndicator::print(ostream & out) const
{
   out << "Depth:    " << level << endl;
   out << "Exec:     " << executable << endl;
   out << "Safe ex:  " << (safe_execution ? "yes" : "no") << endl;

   Assert(executable);

   switch(get_executable()->get_parse_mode())
      {
        case PM_FUNCTION:
             out << "Pmode:    ∇ "
                 << executable->get_ufun()->get_name_and_line(get_PC());
             break;

        case PM_STATEMENT_LIST:
             out << "Pmode:    ◊ " << " " << executable->get_text(0);
             break;

        case PM_EXECUTE:
             out << "Pmode:    ⍎ " << " " << executable->get_text(0);
             break;

        default:
             out << "??? Bad pmode " << executable->get_parse_mode();
      }
   out << endl;

   out << "PC:       " << get_PC();
   out << " " << executable->get_body()[get_PC()] << endl;
   out << "Stat:     " << executable->statement_text(get_PC());
   out << endl;

   out << "err_code: " << HEX(error.error_code) << endl;
   out << "thrown:   at " << error.throw_loc << endl;
   out << "e_msg_1:  '" << error.get_error_line_1() << "'" << endl;
   out << "e_msg_2:  '" << error.get_error_line_2() << "'" << endl;
   out << "e_msg_3:  '" << error.get_error_line_3() << "'" << endl;

   out << endl;
}
//-----------------------------------------------------------------------------
void
StateIndicator::list(ostream & out, SI_mode mode) const
{
   Assert(this);

   if (mode & SIM_debug)   // command ]SI or ]SIS
      {
        print(out);
        return;
      }

   // pmode column
   //
   switch(get_executable()->get_parse_mode())
      {
        case PM_FUNCTION:
             Assert(executable);
             if (mode == SIM_SI)   // )SI
                {
                  out << executable->get_ufun()->get_name_and_line(get_PC());
                  break;
                }

             if (mode & SIM_statements)   // )SIS
                {
                  if (error.error_code)
                     {
                       out << error.get_error_line_2() << endl
                           << error.get_error_line_3();
                     }
                  else
                     {
                       const UCS_string name_and_line =
                            executable->get_ufun()->get_name_and_line(get_PC());
                       out << name_and_line
                           << "  " << executable->statement_text(get_PC())
                           << endl
                           << UCS_string(name_and_line.size(), UNI_ASCII_SPACE)
                           << "  ^";   // ^^^
                     }
                }

             if (mode & SIM_name_list)   // )SINL
                {
                  const UCS_string name_and_line =
                        executable->get_ufun()->get_name_and_line(get_PC());
                       out << name_and_line << " ";
                       executable->get_ufun()->print_local_vars(out);
                }
             break;

        case PM_STATEMENT_LIST:
             out << "⋆";
             if (mode & SIM_statements)   // )SIS
                {
                  if (!executable)      break;

                  // )SIS and we have a statement
                  //
                  out << "  "
                      << executable->statement_text(get_PC())
                      << endl << "   ^";   // ^^^
                }
             break;

        case PM_EXECUTE:
             out << "⋆⋆  ";
             if (mode & SIM_statements)   // )SIS
                {
                  if (!executable)   break;

                  // )SIS and we have a statement
                  //
                  if (error.error_code)
                     out << error.get_error_line_2() << endl
                         << error.get_error_line_3();
                  else
                     out << "  "
                         << executable->statement_text(get_PC());
                }
             break;
      }

   out << endl;
}
//-----------------------------------------------------------------------------
void
StateIndicator::update_error_info(Error & err)
{
bool locked = false;
const UserFunction * ufun = executable->get_ufun();

   // prepare second error line (failed statement)
   //
   if (ufun)
      {
        if (err.show_locked || ufun->get_exec_properties()[1])
           {
             locked = true;
             err.error_message_2 = UCS_string(6, UNI_ASCII_SPACE);
           }
        else
           {
             err.error_message_2 = ufun->get_name_and_line(get_PC());
             err.error_message_2.append(UNI_ASCII_SPACE);
             err.error_message_2.append(UNI_ASCII_SPACE);
           }
      }
   else
      {
        err.error_message_2 = UCS_string(6, UNI_ASCII_SPACE);
      }

   // prepare third line (carets)
   //
   err.left_caret = err.error_message_2.size();
   err.right_caret = -1;

   if (locked)
      {
        executable->get_ufun()->set_locked_error_info(err);
      }
   else
      {
        const Function_PC from = current_stack.get_range_low();
        Function_PC to   = current_stack.get_range_high();
        const Function_PC2 error_range(from, to);
        executable->set_error_info(err, error_range);
      }

   // print error, unless we are in safe execution mode.
   //
   {
     bool print_error = true;
     for (const StateIndicator * si = this; si; si = si->get_parent())
         {
           if (si->get_safe_execution())
              {
                print_error = false;
                break;
              }
         }
     if (print_error)   err.print_em(UERR, LOC);
   }

   error = err;
}
//-----------------------------------------------------------------------------
ostream &
StateIndicator::indent(ostream & out) const
{
   if (level < 0)
      {
         CERR << "[negative level " << HEX(level) << "]" << endl;
      }
   else if (level > 100)
      {
         CERR << "[huge level " << HEX(level) << "]" << endl;
      }
   else
      {
         loop(d, level)   out << "   ";
      }

   return out;
}
//-----------------------------------------------------------------------------
Token
StateIndicator::jump(Value_P value)
{
   // perform a jump. We either remain in the current function (and then
   // return TOK_VOID), or we (want to) jump into back into the calling
   // function (and then return TOK_BRANCH.). The jump itself (if any)
   // is executed in Prefix.cc.
   //
   if (value->get_rank() > 1)   DOMAIN_ERROR;

   if (value->element_count() == 0)     // →''
      {
        // →⍬ in immediate execution means resume (retry) suspended function
        // →⍬ on ⍎ or defined functions means do nothing
        //
        if (get_executable()->get_parse_mode() == PM_STATEMENT_LIST)
           return Token(TOK_BRANCH, int64_t(Function_Retry));

        return Token(TOK_NOBRANCH);           // stay in context
      }

const Function_Line line = value->get_line_number(Workspace::get_CT());

const UserFunction * ufun = get_executable()->get_ufun();

   if (ufun)   // →N in user defined function
      {
        set_PC(ufun->pc_for_line(line));   // →N to valid line in user function
        return Token(TOK_VOID);         // stay in context
      }

   // →N in ⍎ or ◊
   //
   return Token(TOK_BRANCH, int64_t(line < 0 ? Function_Line_0 : line));
}
//-----------------------------------------------------------------------------
void
StateIndicator::escape()  
{
   if (eoc_handler)
      {
        Token tok(TOK_ESCAPE);
        eoc_handler(tok, eoc_arg);
      }
}
//-----------------------------------------------------------------------------
Token
StateIndicator::run()
{
Token result = current_stack.reduce_statements();

   Log(LOG_prefix_parser)
      CERR << "Prefix::reduce_statements(si=" << level << ") returned "
           << result << " in StateIndicator::run()" << endl;
   return result;
}
//-----------------------------------------------------------------------------
void
StateIndicator::unmark_all_values() const
{
   Assert(executable);
   executable->unmark_all_values();

   current_stack.unmark_all_values();
}
//-----------------------------------------------------------------------------
int
StateIndicator::show_owners(ostream & out, const Value & value) const
{
int count = 0;

   Assert(executable);
char cc[100];
   snprintf(cc, sizeof(cc), "    SI[%d] ", level);
   count += executable->show_owners(cc, out, value);

   snprintf(cc, sizeof(cc), "    SI[%d] ", level);
   count += current_stack.show_owners(cc, out, value);

   return count;
}
//-----------------------------------------------------------------------------
void
StateIndicator::info(ostream & out, const char * loc) const
{
   out << "SI[" << level << ":" << get_PC() << "] "
       << get_parse_mode_name() << " "
       << executable->get_text(0) << " creator: " << executable->get_loc()
       << "   seen at: " << loc << endl;
}
//-----------------------------------------------------------------------------
Value_P
StateIndicator::get_L()
{
Token * tok_L = current_stack.locate_L();
   if (tok_L)   return tok_L->get_apl_val();
   return Value_P();
}
//-----------------------------------------------------------------------------
Value_P
StateIndicator::get_R()
{
Token * tok_R = current_stack.locate_R();
   if (tok_R)   return tok_R->get_apl_val();
   return Value_P();
}
//-----------------------------------------------------------------------------
Value_P
StateIndicator::get_X()
{
Value_P * X = current_stack.locate_X();
   if (X)   return *X;
   return Value_P();
}
//-----------------------------------------------------------------------------
void
StateIndicator::set_L(Value_P new_value)
{
Token * tok_L = current_stack.locate_L();
   if (tok_L == 0)   return;

Value_P old_value = tok_L->get_apl_val();   // so that 
   move_2(*tok_L, Token(tok_L->get_tag(), new_value), LOC);
}
//-----------------------------------------------------------------------------
void
StateIndicator::set_R(Value_P new_value)
{
Token * tok_R = current_stack.locate_R();
   if (tok_R == 0)   return;

Value_P old_value = tok_R->get_apl_val();   // so that 
   move_2(*tok_R, Token(tok_R->get_tag(), new_value), LOC);
}
//-----------------------------------------------------------------------------
void
StateIndicator::set_X(Value_P new_value)
{
Value_P * X = current_stack.locate_X();
   if (X)   *X = new_value;
}
//-----------------------------------------------------------------------------
Function_Line
StateIndicator::get_line() const
{
int pc = get_PC();
   if (pc)   --pc;
   return executable->get_line(Function_PC(pc));
}
//-----------------------------------------------------------------------------
#ifdef WANT_LIBAPL
typedef int (*result_callback)(const Value * result, int committed);
extern "C" result_callback res_callback;
result_callback res_callback = 0;
#endif
void
StateIndicator::statement_result(Token & result, bool trace)
{
   Log(LOG_StateIndicator__enter_leave)
      CERR << "StateIndicator::statement_result(pmode="
           << get_parse_mode_name() << ", result=" << result << endl;

   if (trace)
      {
        const UserFunction * ufun = executable->get_ufun();
        if (ufun && (ufun->get_exec_properties()[0] == 0))
           {
             const Function_Line line =
                            executable->get_line((Function_PC)(get_PC() - 1));
             result.show_trace(COUT, ufun->get_name(), line);
           }
      }

   fun_oper_cache.reset();

// if (get_executable()->get_parse_mode() == PM_EXECUTE)   return;

   // if result is a value then print it, unless it is a committed value
   // (i.e. TOK_APL_VALUE2)
   //
   if (result.get_ValueType() != TV_VAL)   return;

const TokenTag tag = result.get_tag();
Value_P B(result.get_apl_val());
   Assert(!!B);

   // print TOK_APL_VALUE and TOK_APL_VALUE1, but not TOK_APL_VALUE2
   //
bool print_value = tag == TOK_APL_VALUE1 || tag == TOK_APL_VALUE3;

#ifdef WANT_LIBAPL
   if (res_callback)   // callback installed
      {
        // the callback decides whether the value shall be printed (even
        // if it was committed)
        //
        print_value = res_callback(B.get(), !print_value);
      }
#endif

   if (!print_value)   return;

   Quad_QUOTE::done(false, LOC);

const int boxing_format = Command::get_boxing_format();
   if (boxing_format == -1)
      {
        if (Quad_SYL::print_length_limit &&
            B->element_count() >= Quad_SYL::print_length_limit)
           {
             // the value exceeds print_length_limit.
             // We cut the longest dimension in half until we are below the
             // limit
             //
             Shape sh(B->get_shape());
             while (sh.get_volume() >= Quad_SYL::print_length_limit)
                {
                  Rank longest = 0;
                  loop(r, sh.get_rank())
                     {
                       if (sh.get_shape_item(r) > sh.get_shape_item(longest))
                          longest = r;
                     }

                  sh.set_shape_item(longest, sh.get_shape_item(longest) / 2);
                }

             Value_P B1 = Bif_F12_TAKE::do_take(sh, B).get_apl_val();
             B1->print(COUT);

             CERR << "      *** display of value was truncated (limit "
                     "⎕SYL[⎕IO + " << Quad_SYL::SYL_PRINT_LIMIT
                  << "] reached)  ***" << endl;
           }
        else
           {
             B->print(COUT);
           }
      }
   else
      {
        const PrintContext pctx = Workspace::get_PrintContext(PST_NONE);
        Value_P B1 = Quad_CR::do_CR(boxing_format, *B.get(), pctx);
        B1->print(COUT);
      }
}
//-----------------------------------------------------------------------------
Unicode
StateIndicator::get_parse_mode_name() const
{
   switch(get_executable()->get_parse_mode())
      {
        case PM_FUNCTION:       return UNI_NABLA;
        case PM_STATEMENT_LIST: return UNI_DIAMOND;
        case PM_EXECUTE:        return UNI_EXECUTE;
     }

   CERR << "pmode = " << get_executable()->get_parse_mode() << endl;
   Assert(0 && "bad pmode");
}
//-----------------------------------------------------------------------------

