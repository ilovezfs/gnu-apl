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

#ifndef __BIF_OPER2_INNER_HH_DEFINED__
#define __BIF_OPER2_INNER_HH_DEFINED__

#include "PrimitiveOperator.hh"

//-----------------------------------------------------------------------------
/** Primitive operator inner product.
 */
class Bif_OPER2_INNER : public PrimitiveOperator
{
public:
   /// Constructor.
   Bif_OPER2_INNER() : PrimitiveOperator(TOK_OPER2_INNER) {}

   /// Overloaded Function::eval_ALRB().
   virtual Token eval_ALRB(Value_P A, Token & LO, Token & RO, Value_P B);

   static Bif_OPER2_INNER * fun;   ///< Built-in function.
   static Bif_OPER2_INNER  _fun;   ///< Built-in function.

protected:
   /// Compute the inner product of A and B with functions LO and RO.
   Token inner_product(Value_P A, Token & LO, Token & RO, Value_P B);

   /// EOC handler for inner_product with user defined RO
   static bool eoc_inner_product(Token & token, EOC_arg & arg);

   /// the context for an inner product
   struct PJob_product
      {
        Cell * cZ;          ///< result cell pointer
        const Cell * cA;    ///< left value argument cell pointer
        int incA;           ///< left argument increment (for scalar extension)
        ShapeItem ZAh;      ///< high dimensions of result length
        prim_f2 LO;         ///< left function argument
        ShapeItem LO_len;   ///< left operator length
        prim_f2 RO;         ///< right function argument
        const Cell * cB;    ///< right value argument cell pointer
        int incB;           ///< right argument increment (for scalar extension)
        ShapeItem ZBl;      ///< low dimensions of result length
        ErrorCode ec;       ///< error code
        CoreCount cores;    ///< number of cores to be used
      };

   /// the context for an inner product
   static PJob_product job;

   /// inner product for scalar LO and RO
   inline void scalar_inner_product() const;

   /// the main loop for an inner product with scalar functions
   static void PF_scalar_inner_product(Thread_context & tctx);

   /// helper for RO. returns true if the final token was computed, and false
   /// if finish_inner_product shall be called again
   static Token finish_inner_product(EOC_arg & arg);
	};
//-----------------------------------------------------------------------------

#endif // __BIF_OPER2_INNER_HH_DEFINED__
