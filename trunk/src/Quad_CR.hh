
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

#ifndef __Quad_CR_HH_DEFINED__
#define __Quad_CR_HH_DEFINED__

#include "QuadFunction.hh"

/**
   The system function ⎕CR (Character Representation).
 */
class Quad_CR : public QuadFunction
{
public:
   /// Constructor.
   Quad_CR() : QuadFunction(TOK_Quad_CR) {}

   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   /// compute \b a ⎕CR \b B
   static Value_P do_CR(APL_Integer a, const Value & B, PrintContext pctx);

   static Quad_CR * fun;          ///< Built-in function.
   static Quad_CR  _fun;          ///< Built-in function.

   /// portable variable encoding of value named name (varname or varname ⊂)
   static void do_CR10_var(vector<UCS_string> & result,
                           const UCS_string & name, const UCS_string & pick,
                           const Value & value);

protected:
   enum V_mode
      {
        Vm_NONE,
        Vm_NUM,
        Vm_QUOT,
        Vm_UCS
      };

   /// 10 ⎕CR symbol_name (variable or function name). Also used for )OUT
   static void do_CR10(vector<UCS_string> & result, const Value & symbol_name);

   /// one ravel item in 10 ⎕CR symbol_name
   static V_mode do_CR10_item(UCS_string & item, const Cell & cell, V_mode mode,
                              bool may_quote, bool & item_nested);

   /// decide if 'xxx' or ⎕UCS(xxx) shall be used
   static bool use_quote(V_mode mode, const Value & value, ShapeItem pos);

};
//-----------------------------------------------------------------------------

#endif //__Quad_CR_HH_DEFINED__

