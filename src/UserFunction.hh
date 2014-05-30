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

#ifndef __USERFUNCTION_HH_DEFINED__
#define __USERFUNCTION_HH_DEFINED__

#include <sys/types.h>

#include "Error.hh"
#include "Executable.hh"
#include "Function.hh"
#include "Symbol.hh"
#include "UTF8_string.hh"

//-----------------------------------------------------------------------------
/**
    The (symbols in the) header of a user-defined function.
 */
class UserFunction_header
{
public:
   /// constructor from first line in \b txt
   UserFunction_header(const UCS_string txt);

   /// constructor from signature (for lambdas)
   UserFunction_header(Fun_signature sig, const UCS_string fname);

   /// return the number of value arguments
   int get_fun_valence() const
      {
        if (sym_A)    return 2;   // dyadic function
        if (sym_B)    return 1;   // monadic function
        return 0;                 // niladic function
      }

   /// return the number of operator arguments
   int get_oper_valence() const
      {
        if (sym_RO)   return 2;   // dyadic operator
        if (sym_LO)   return 1;   // monadic operator
        return 0;                 // niladic function
      }

   /// return source location where error was detected
   const char * get_error_cause() const   { return error_cause; }

   /// return true if the header indicates a result (Z←)
   bool has_result() const   { return sym_Z != 0; }

   /// return true if the header contains variables (result, arguments,
   /// or local variables
   bool has_vars() const
      { return sym_Z || sym_A || sym_LO || sym_X || sym_RO || sym_B ||
               local_vars.size(); }

   /// return true iff the function returns a value
   int has_axis() const   { return sym_X != 0; }

   bool is_operator() const   { return sym_LO != 0; }

   /// return the name of the function
   const UCS_string & get_name() const   { return function_name; }

   /// return the Symbol for the function result
   Symbol * Z()   const   { return sym_Z; }

   /// return the Symbol for the left argument
   Symbol * A()   const   { return sym_A; }

   /// return the Symbol for the left function argument
   Symbol * LO()   const   { return sym_LO; }

   /// return the Symbol for the left function argument
   Symbol * FUN()   const   { return sym_FUN; }

   /// return the Symbol for the right argument
   Symbol * X()   const   { return sym_X; }

   /// return the Symbol for the right function argument
   Symbol * RO()   const   { return sym_RO; }

   /// return the Symbol for the right argument
   Symbol * B()   const   { return sym_B; }

   /// return true if header was parsed successfully
   ErrorCode get_error() const   { return error; }

   /// print local vars etc.
   void print_properties(ostream & out, int indent) const;

   /// pop all local vars, labels, and parameters
   void pop_local_vars() const;

   /// print the local variables for command )SINL
   void print_local_vars(ostream & out) const;

   /// add a label
   void add_label(Symbol * sym, Function_Line line)
      {
        labVal label = { sym, line };
        label_values.push_back(label);
      }

   /// Check that all function params, local vars. and labels are unique.
   void check_duplicate_symbols();

   /// the header (as per SIG_xxx and local_vars)
   static UCS_string lambda_header(Fun_signature sig, const UCS_string & fname);

   /// push Z (if defined), local variables, and labels.
   void eval_common();

   /// true if this user defined function was created from a lambda body
   const bool from_lambda;

protected:
   /// Check that sym occurs at most once in \b Symbol array \b sym.
   void check_duplicate_symbol(const Symbol * sym);

   /// error if header was not parsed successfully
   ErrorCode error;

   /// source location where error was detected
   const char * error_cause;

   /// the name of this function
   UCS_string function_name;

   /// optional result
   Symbol * sym_Z;

   /// optional left function arg
   Symbol * sym_A;

   /// optional left operator function
   Symbol * sym_LO;

   /// optional symbol for function name (0 for lambdas)
   Symbol * sym_FUN;

   /// optional right operator function
   Symbol * sym_RO;

   ///< optional right operator function axis
   Symbol * sym_X;

   /// optional right function arg
   Symbol * sym_B;

   /// The local variables of \b this function.
   vector<Symbol *> local_vars;

   /// A single label. A label is a local variable (sym) with an integer
   /// function line (in which sym: was specified as the first tokens
   /// in the line)
   struct labVal
   {
      Symbol      * sym;    ///< The symbol for the label variable.
      Function_Line line;   ///< The line number of the label.
   };

   /// The labels of \b this function.
   vector<labVal> label_values;
};
//-----------------------------------------------------------------------------
/**
    A user-defined function.
 */
class UserFunction : public Function, public Executable
{
public:
   /// Construct a user defined function
   UserFunction(const UCS_string txt,
                int & error_line, const char * & error_cause,
                bool keep_existing, const char * loc,
                const UTF8_string &  _creator);

   /// Construct a lambda
   UserFunction(Fun_signature sig, const UCS_string & fname,
                const UCS_string & text, const Token_string & body);

   /// Destructor.
   ~UserFunction();

   bool is_lambda() const
      { return header.from_lambda; }

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

   // pop all
   void pop_local_vars() const
      { header.pop_local_vars(); }

   /// print the local variables for command )SINL
   void print_local_vars(ostream & out) const
      { return header.print_local_vars(out); }

   /// Overloaded \b Function::is_operator.
   virtual bool is_operator() const
      { return header.is_operator(); }

   void add_label(Symbol * sym, Function_Line line)
      { header.add_label(sym, line); }

   virtual const int * get_exec_properties() const
      { return exec_properties; } 

   /// set execution properties (as per A ⎕FX)
   void set_exec_properties(const int * props)
      { loop(i, 4)   exec_properties[i] = props[i]; }

   /// overloaded Executable::print()
   virtual ostream & print(ostream & out) const;

   /// return the name of this function, or ◊ or ⍎
   virtual const UCS_string & get_name() const;

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
   static UserFunction * fix(const UCS_string & data, int & error_line,
                             bool keep_existing, const char * loc,
                             const UTF8_string &  creator);

   /// return the pc of the first token in line l (valid line), or
   /// the pc of the last token in the function (invalid line)
   Function_PC pc_for_line(int l) const;
   
   /// Overloaded Function::has_alpha()
   virtual bool has_alpha() const   { return true; }

   /// overloaded Executable::get_sym_Z()
   virtual Symbol * get_sym_Z() const   { return header.Z(); }

   /// return the parse context for this function
   virtual ParseMode get_parse_mode() const   { return PM_FUNCTION; }

   /// overloaded Executable::get_line()
   Function_Line get_line(Function_PC pc) const;

   /// Quad_CR of this function
   virtual UCS_string canonical(bool with_lines) const;

   /// overloaded Executable::line_start()
   virtual Function_PC line_start(Function_Line line) const
      { return line_starts[line]; }

   /// compute lines 2 and 3 in \b error
   void set_locked_error_info(Error & error) const;

   const UTF8_string get_creator() const
      { return creator; }

protected:
   /// overladed Function::may_push_SI()
   virtual bool may_push_SI() const   { return true; }

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

   /// Overloaded Function::eval_fill_B()
   virtual Token eval_fill_B(Value_P B);

   /// Overloaded Function::eval_fill_AB()
   virtual Token eval_fill_AB(Value_P A, Value_P B);

   UserFunction_header header;

   /// helper function to print token with Function or Value content
   static ostream & print_val_or_fun(ostream & out, Token & tok);

   /// "[nn] " prefix
   static UCS_string line_prefix(Function_Line l);

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

   /// execution properties as per 3⎕AT
   int exec_properties[4];

   /// the entity (∇ editor, ⎕FX, or filename) that has created this function
   const UTF8_string creator;
};
//-----------------------------------------------------------------------------

#endif // __USERFUNCTION_HH_DEFINED__
