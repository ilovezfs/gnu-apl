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

#ifndef __BIF_OPER2_POWER_HH_DEFINED__
#define __BIF_OPER2_POWER_HH_DEFINED__

#include "PrimitiveOperator.hh"

//-----------------------------------------------------------------------------
/** Primitive operator ⍤ (rank)
 */
class Bif_OPER2_POWER : public PrimitiveOperator
{
public:
   /// constructor
   Bif_OPER2_POWER() : PrimitiveOperator(TOK_OPER2_POWER) {}

   /// overloaded Function::eval_ALRB()
   virtual Token eval_ALRB(Value_P A, Token & LO, Token & RO_y, Value_P B);

   /// overloaded Function::eval_LRB()
   virtual Token eval_LRB(Token & LO, Token & RO_y, Value_P B);

   static Bif_OPER2_POWER fun;      ///< Built-in function

protected:
   /// helper for eval_ALRB
   static Token finish_ALRB(EOC_arg & arg);

   /// helper for eval_ALRB
   static bool eoc_ALRB(Token & token, EOC_arg & arg);

   /// helper for eval_ALRB
   static Token finish_LRB(EOC_arg & arg);

   /// helper for eval_ALRB
   static bool eoc_LRB(Token & token, EOC_arg & arg);

   /// return boolean value of 1-element value \b COND
   static bool get_condition_value(const Value & COND, double qct);
};
//-----------------------------------------------------------------------------

#endif // __BIF_OPER2_POWER_HH_DEFINED__
