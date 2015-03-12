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

#ifndef __STATE_INDICATOR_HH_DEFINED__
#define __STATE_INDICATOR_HH_DEFINED__

#include "Common.hh"
#include "DerivedFunction.hh"
#include "EOC_handler_args.hh"
#include "Error.hh"
#include "Parser.hh"
#include "Prefix.hh"
#include "PrintOperator.hh"

class Executable;

//-----------------------------------------------------------------------------
/**
    One entry of the state indicator (SI) of the APL interpreter.
    Compared to e.g.  C++, the state indicator is (one element of) the
    function call stack of the interpreter.
 */
class StateIndicator
{
   friend class XML_Loading_Archive;
   friend class XML_Saving_Archive;

public:
   /// constructor
   StateIndicator(Executable * exec, StateIndicator * _par);

   /// destructor
   ~StateIndicator();

   /// continue this StateIndicator (to line N after →N back into it)
   void goon(Function_Line N, const char * loc);

   /// retry this StateIndicator (after →'')
   void retry(const char * loc);

   /// Return the function name, or "*" for an immediate execution context
   UCS_string function_name() const;

   /// list the stack entry (for commands ]SI, )SI, and )SIS)
   void list(ostream & out, SI_mode mode) const;

   /// list the stack entry (for command ]SI)
   void print(ostream & out) const;

   /// print spaces according to level
   ostream & indent(ostream & out) const;

   /// return pointer to the current user function, statements, or execute
   Executable * get_executable()
      { return executable; }

   /// return pointer to the current user function, statements, or execute
   const Executable * get_executable() const
      { return executable; }

   /// remove EOC handlers and clear it
   EOC_arg * remove_eoc_handlers(EOC_arg * & nxt);

   EOC_arg * get_eoc_handlers() const
      { return eoc_handlers; }

   void set_eoc_handlers(EOC_arg * eoc)
      { eoc_handlers = eoc; }

   /// return the name of the parse mode
   Unicode get_parse_mode_name() const;

   /// return the current PC
   Function_PC get_PC() const
      { return current_stack.get_PC(); }

   /// set the current PC
   void set_PC(Function_PC new_pc)
      { current_stack.set_PC(new_pc); }

   /// evaluate a →N statement. Update pc return true iff context has changed
   Token jump(Value_P val);

   /// return the nesting level (oldest SI has level 0, next has level 1, ...)
   int get_level() const   { return level; };

   /// return the current line number
   Function_Line get_line() const;

   /// Maybe print B (according to tag) and erase B
   void statement_result(Token & result, bool trace);

   /// update error information in \b error and copy it to SI
   void update_error_info(Error & err);

   /// Escape from \b user function (exit from each invocation until
   /// immediate execution is reached)
   void escape();

   /// execute token in body...
   Token run();

   /// clear the marked bit in all parsers
   void unmark_all_values() const;

   /// print all owners of \b value
   int show_owners(ostream & out, const Value & value) const;

   /// print a short debug info
   void info(ostream & out, const char * loc) const;

   /// return the error related info in this context
   Error & get_error() { return error; }

   /// return the error related info in this context
   const Error & get_error() const { return error; }

   /// return left arg
   Value_P get_L();

   /// change left arg
   void set_L(Value_P value);

   /// return right arg
   Value_P get_R();

   /// change right arg
   void set_R(Value_P value);

   /// return axis arg
   Value_P get_X();

   /// change axis arg
   void set_X(Value_P value);

   /// add an EOC handler
   void add_eoc_handler(EOC_HANDLER handler, EOC_arg & arg, const char * loc);

   /// move an EOC handler from \b old_si to \b this si
   void move_eoc_handler(EOC_HANDLER handler, EOC_arg * old_eoc,
                         const char * loc);

   /// call eoc_handler. Return false if there is none or else the result
   /// of the eoc_handler. false indicates that the eoc_handler has finished.
   bool call_eoc_handler(Token & token);

   /// return safe_execution mode
   bool get_safe_execution() const
      { return safe_execution; }

   /// set safe_execution mode
   void set_safe_execution(bool on_off)
      { safe_execution = on_off; }

   /// get the current prefix parser
   Prefix & get_prefix()
      { return current_stack; }

   /// return the SI that has called this one
   StateIndicator * get_parent() const
      { return parent; }

   /// a small storage for DerivedFunction objects.
  DerivedFunctionCache fun_oper_cache;

protected:
   /// the user function that is being executed
   Executable * executable;

   /// true iff this context is in safe execution mode (⎕EA, ⎕EC)
   bool safe_execution;

   /// a (normally empty) list of EOC handler function and their arguyments.
   /// The handlers are called at the end of execution of this context.
   EOC_arg * eoc_handlers;

   /// The nesting level (of sub-executions)
   const int level;

   /// details of the last error in this context.
   Error error;

   /// the current-stack of this context.
   Prefix current_stack;

   /// the StateIndicator that has called this one
   StateIndicator * parent;
};
//-----------------------------------------------------------------------------

#endif // __STATE_INDICATOR_HH_DEFINED__
