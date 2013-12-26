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

#ifndef __NATIVE_FUNCTION_HH_DEFINED__
#define __NATIVE_FUNCTION_HH_DEFINED__

#include <vector>
#include <sys/types.h>

#include "Executable.hh"
#include "Function.hh"
#include "Error.hh"

//-----------------------------------------------------------------------------
/**
    A user-defined function.
 */
class NativeFunction : public Function
{
public:
   static NativeFunction *  fix(void * so_handle, const UCS_string fun_name);

protected:
   /// constructor (only fix() may call it)
   NativeFunction(void * so_handle, UCS_string apl_name);

   /// destructor: close so_handle
   ~NativeFunction();

   /// overloaded Function::has_result()
   virtual int has_result() const;

   /// overloaded Function::is_operator()
   virtual bool is_operator() const;

   /// Overloaded Function::print_properties()
   virtual void print_properties(ostream & out, int indent) const;

   /// Overloaded Function::print()
   virtual ostream & print(std::ostream&) const;

   /// Overloaded Function::eval_()
   virtual Token eval_();

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// Overloaded Function::eval_LB()
   virtual Token eval_LB(Token & LO, Value_P B);

   /// Overloaded Function::eval_ALB()
   virtual Token eval_ALB(Value_P A, Token & LO, Value_P B);

   /// Overloaded Function::eval_LRB()
   virtual Token eval_LRB(Token & LO, Token & RO, Value_P B);

   /// Overloaded Function::eval_ALRB()
   virtual Token eval_ALRB(Value_P A, Token & LO, Token & RO, Value_P B);

   /// Overloaded Function::eval_XB()
   virtual Token eval_XB(Value_P X, Value_P B);

   /// Overloaded Function::eval_AXB()
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   /// Overloaded Function::eval_LXB()
   virtual Token eval_LXB(Token & LO, Value_P X, Value_P B);

   /// Overloaded Function::eval_ALXB()
   virtual Token eval_ALXB(Value_P A, Token & LO, Value_P X, Value_P B);

   /// Overloaded Function::eval_LRXB()
   virtual Token eval_LRXB(Token & LO, Token & RO, Value_P X, Value_P B);

   /// Overloaded Function::eval_ALRXB()
   virtual Token eval_ALRXB(Value_P A, Token & LO, Token & RO, Value_P X,
                            Value_P B);

   /// Overloaded Function::eval_fill_B()
   virtual Token eval_fill_B(Value_P B);

   /// Overloaded Function::eval_fill_AB()
   virtual Token eval_fill_AB(Value_P A, Value_P B);

   /// Overloaded Function::eval_identity_fun()
   virtual Token eval_identity_fun(Value_P B, Axis axis);

   /// dl_open() handle of shared library
   void * const handle;

   /// APL name of the function
   UCS_string name;

   bool valid;
   bool b_has_result;

   typedef const Value & Vr;
   typedef Function & Fr;

   Token (*f_eval_)();
   Token (*f_eval_B)(Vr B);
   Token (*f_eval_AB)(Vr A, Vr B);
   Token (*f_eval_LB)(Fr LO, Vr B);
   Token (*f_eval_ALB)(Vr A, Fr LO, Vr B);
   Token (*f_eval_LRB)(Fr LO, Fr RO, Vr B);
   Token (*f_eval_ALRB)(Vr A, Fr LO, Fr RO, Vr B);
   Token (*f_eval_XB)(Vr X, Vr B);
   Token (*f_eval_AXB)(Vr A, Vr X, Vr B);
   Token (*f_eval_LXB)(Fr LO, Vr X, Vr B);
   Token (*f_eval_ALXB)(Vr A, Fr LO, Vr X, Vr B);
   Token (*f_eval_LRXB)(Fr LO, Fr RO, Vr X, Vr B);
   Token (*f_eval_ALRXB)(Vr A, Fr LO, Fr RO, Vr X, Vr B);
   Token (*f_eval_fill_B)(Vr B);
   Token (*f_eval_fill_AB)(Vr A, Vr B);
   Token (*f_eval_identity_fun)(Vr B, Axis axis);
};
//-----------------------------------------------------------------------------

#endif // __NATIVE_FUNCTION_HH_DEFINED__
