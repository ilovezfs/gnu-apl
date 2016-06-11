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

#ifndef __USERFUNCTION__HEADER_HH_DEFINED__
#define __USERFUNCTION__HEADER_HH_DEFINED__

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
   /// constructor from first line in \b txt (for normal defined functions)
   UserFunction_header(const UCS_string & txt, bool macro);

   /// constructor from signature (for lambdas)
   UserFunction_header(Fun_signature sig, int lambda_num);

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
   const char * get_error_info() const   { return error_info; }

   /// return true if the header indicates a result (Z←)
   bool has_result() const   { return sym_Z != 0; }

   /// return true if the header contains variables (result, arguments,
   /// or local variables
   bool has_vars() const
      { return sym_Z || sym_A || sym_LO || sym_X || sym_RO || sym_B ||
               local_vars.size(); }

   /// return true iff the function returns a value
   int has_axis() const   { return sym_X != 0; }

   /// return true if this defined function header is an operator
   bool is_operator() const   { return sym_LO != 0; }

   /// return the name of the function
   UCS_string get_name() const   { return function_name; }

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

   /// return error if header was not parsed successfully
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
   void remove_duplicate_local_variables();

   /// the header (as per SIG_xxx and local_vars)
   static UCS_string lambda_header(Fun_signature sig, int lambda_num);

   /// push Z (if defined), local variables, and labels.
   void eval_common();

protected:
   /// remove \b sym from local_vars if it occurs at pos or above
   void remove_duplicate_local_var(const Symbol * sym, unsigned int pos);

   /// error if header was not parsed successfully
   ErrorCode error;

   /// source location where error was detected
   const char * error_info;

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

   /// optional right operator function axis
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

#endif // __USERFUNCTION__HEADER_HH_DEFINED__
