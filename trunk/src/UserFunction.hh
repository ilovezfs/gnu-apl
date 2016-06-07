/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2016  Dr. Jürgen Sauermann

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

#ifndef __USERFUNCTION_HH_DEFINED__
#define __USERFUNCTION_HH_DEFINED__

#include <sys/types.h>

#include "Error.hh"
#include "Executable.hh"
#include "Function.hh"
#include "Symbol.hh"
#include "UserFunction_header.hh"
#include "UTF8_string.hh"

//-----------------------------------------------------------------------------
/**
    A user-defined function.
 */
class UserFunction : public Function, public Executable
{
public:
   /// constructor for a lambda
   UserFunction(Fun_signature sig, int lambda_num,
                const UCS_string & text, const Token_string & body);

   /// Destructor.
   ~UserFunction();

   virtual bool is_lambda() const
      { return header.get_name()[0] == UNI_LAMBDA; }

   /// overloaded Executable::get_ufun()
   virtual const UserFunction * get_ufun() const
   { return this; }

   /// overloaded Function::get_ufun1()
   virtual UserFunction * get_ufun1()
   { return this; }

   /// overloaded Function::get_ufun1()
   virtual const UserFunction * get_ufun1() const
   { return this; }

   /// overloaded Executable::get_ufun()
   virtual UserFunction * get_ufun()
   { return this; }

   /// overloaded Function::get_fun_valence()
   virtual int get_fun_valence() const
      { return header.get_fun_valence(); }

   /// overloaded Function::get_oper_valence()
   virtual int get_oper_valence() const
      { return header.get_oper_valence(); }

   /// overloaded Function::has_result()
   virtual bool has_result() const
      { return header.has_result(); }

   /// overloaded Function::has_axis()
   virtual bool has_axis() const
      { return header.has_axis(); }

   /// pop all
   void pop_local_vars() const
      { header.pop_local_vars(); }

   /// print the local variables for command )SINL
   void print_local_vars(ostream & out) const
      { return header.print_local_vars(out); }

   /// Overloaded \b Function::is_operator.
   virtual bool is_operator() const
      { return header.is_operator(); }

   /// add a label
   void add_label(Symbol * sym, Function_Line line)
      { header.add_label(sym, line); }

   /// return the execution properties (dyadic ⎕FX) of this defined function
   virtual const int * get_exec_properties() const
      { return exec_properties; } 

   /// ovewrloaded Executable::cannot_suspend
   virtual bool cannot_suspend() const
      { return exec_properties[1] != 0; }

   /// set execution properties (as per A ⎕FX)
   void set_exec_properties(const int * props)
      { loop(i, 4)   exec_properties[i] = props[i]; }

   /// overloaded Executable::print()
   virtual ostream & print(ostream & out) const;

   /// return the name of this function
   virtual UCS_string get_name() const
      { return header.get_name(); }

   /// return e.g. 'FOO[10]' 
   UCS_string get_name_and_line(Function_PC pc) const;

   /// Overloaded Function::print_properties()
   virtual void print_properties(ostream & out, int indent) const;

   /// )SAVE this function in the workspace named \b workspace
   /// (in the file system).
   void save(const char * workspace, const char * function);

   /// )LOAD this function into the workspace named \b workspace.
   /// Return a pounter to this newly created function (or 0 on error).
   static UserFunction * load(const char * workspace, const char * function);

   /// Overloaded Function::destroy()
   virtual void destroy();

   /// Load this function into the workspace named \b workspace.
   static void load(const char * workspace, const char * function,
                    UserFunction * & fun);

   /// create a user defined function according to \b data of length \b len
   /// in workspace \b w.
   static UserFunction * fix(const UCS_string & text, int & err_line,
                             bool keep_existing, const char * loc,
                             const UTF8_string &  creator, bool tolerant);

   /// (re-)create a lambda
   static UserFunction * fix_lambda(Symbol & var, const UCS_string & text);

   /// return the pc of the first token in line l (valid line), or
   /// the pc of the last token in the function (invalid line)
   Function_PC pc_for_line(Function_Line line) const;
   
   /// Overloaded Function::has_alpha()
   virtual bool has_alpha() const   { return true; }

   /// overloaded Executable::get_sym_Z()
   virtual Symbol * get_sym_Z() const   { return header.Z(); }

   /// overloaded Executable::get_line()
   Function_Line get_line(Function_PC pc) const;

   /// Overloaded Function::eval_()
   virtual Token eval_();

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// Overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B);

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// Overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   /// Overloaded Function::eval_LB.
   virtual Token eval_LB(Token & LO, Value_P B);

   /// Overloaded Function::eval_LXB()
   virtual Token eval_LXB(Token & LO, Value_P X, Value_P B);

   /// Overloaded Function::eval_ALB.
   virtual Token eval_ALB(Value_P A, Token & LO, Value_P B);

   /// Overloaded Function::eval_ALXB()
   virtual Token eval_ALXB(Value_P A, Token & LO, Value_P X, Value_P B);

   /// Overloaded Function::eval_LRB()
   virtual Token eval_LRB(Token & LO, Token & RO, Value_P B);

   /// Overloaded Function::eval_LRXB()
   virtual Token eval_LRXB(Token & LO, Token & RO, Value_P X, Value_P B);

   /// Overloaded Function::eval_ALRB()
   virtual Token eval_ALRB(Value_P A, Token & LO, Token & RO, Value_P B);

   /// Overloaded Function::eval_ALRXB()
   virtual Token eval_ALRXB(Value_P A, Token & LO, Token & RO,
                            Value_P X, Value_P B);

   /// Quad_CR of this function
   virtual UCS_string canonical(bool with_lines) const;

   /// overloaded Executable::line_start()
   virtual Function_PC line_start(Function_Line line) const
      { return line_starts[line]; }

   /// overloaded Executable::adjust_line_starts
   virtual void adjust_line_starts();

   /// compute lines 2 and 3 in \b error
   void set_locked_error_info(Error & error) const;

   /// return the entity that has created the function
   const UTF8_string get_creator() const
      { return creator; }

   /// set trace or stop vector
   void set_trace_stop(Function_Line * lines, int line_count, bool stop);

   /// recompile the body
   void parse_body(const char * loc, bool tolerant);

   /// return stop lines (from S∆fun ← lines)
   const vector<Function_Line> & get_stop_lines() const
      { return stop_lines; }

   /// return trace lines (from S∆fun ← lines)
   const vector<Function_Line> & get_trace_lines() const
      { return trace_lines; }

protected:
   /// constructor for a normal (i.e. non-lambda) user defined function
   UserFunction(const UCS_string txt, const char * loc,
                const UTF8_string &  _creator, bool tolerant, bool macro);

   /// overladed Function::may_push_SI()
   virtual bool may_push_SI() const   { return true; }

   /// Overloaded Function::eval_fill_B()
   virtual Token eval_fill_B(Value_P B);

   /// Overloaded Function::eval_fill_AB()
   virtual Token eval_fill_AB(Value_P A, Value_P B);

   /// return the line number where an error has occurred (-1 if none)
   int get_error_line() const
      { return error_line; }

   /// return information about an error (if any)
   const char * get_error_info() const
      { return error_info; }

   /// helper function to print token with Function or Value content
   static ostream & print_val_or_fun(ostream & out, Token & tok);

   /// "[nn] " prefix
   static UCS_string line_prefix(Function_Line l);

   /// the header (line [0]) of the user-defined function
   UserFunction_header header;

   /** Offsets to the first token in every line (for jumps).
       lines[0] points to the last line, which is automatically
       added and is TOK_RETURN_VOID for void functions
       and TOK_RETURN_VALUET for functions returning a value.

       An N line function:

       [0] R←A FOO B
       [1] L1: xxx
       ...
       [N-1]   yyy

      will, for example, have a N+1 element jump vector:

      [0] goto end of function        --------------+
      [1] L1: xxx                                   |
      ...                                           |
      [N-1]   yyy                                   |
      [N] TOK_RETURN_SYMBOL or TOK_RETURN_VOID   <--+

   **/
   vector<Function_PC> line_starts;

   /// stop lines (from S∆fun ← lines)
   vector<Function_Line> stop_lines;

   /// trace lines (from S∆fun ← lines)
   vector<Function_Line> trace_lines;

   /// execution properties as per 3⎕AT
   int exec_properties[4];

   /// the entity (∇ editor, ⎕FX, or filename) that has created this function
   const UTF8_string creator;

   /// the line number where an error has occurred (-1 if none)
   int error_line;

   /// information about an error (if any)
   const char * error_info;
};
//-----------------------------------------------------------------------------
class Macro : public UserFunction
{
public:
   Macro(Macro * & _owner, const char * text);

   ~Macro();

   static void clear_all();

   static Simple_string<Macro *> all_macros;

protected:
   Macro * & owner;
};
//-----------------------------------------------------------------------------

#endif // __USERFUNCTION_HH_DEFINED__
