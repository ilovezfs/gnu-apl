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

#ifndef __BIF_OPER2_OUTER_HH_DEFINED__
#define __BIF_OPER2_OUTER_HH_DEFINED__

#include "PrimitiveOperator.hh"

//-----------------------------------------------------------------------------
/**
   A dummy function for the product operator.
 **/
class Bif_JOT : public PrimitiveFunction
{
public:
   /// Constructor.
   Bif_JOT() : PrimitiveFunction(TOK_JOT) {}
 
   virtual int get_oper_valence() const   { return 2; }
 
   static Bif_JOT * fun;             ///< Built-in function.
   static Bif_JOT  _fun;             ///< Built-in function.
 
protected:
};
//-----------------------------------------------------------------------------
/** Primitive operator outer product.
 */
class Bif_OPER2_OUTER : public PrimitiveOperator
{
public:
   /// Constructor.
   Bif_OPER2_OUTER() : PrimitiveOperator(TOK_OPER2_OUTER) {}

   /// Overloaded Function::eval_ALRB().
   virtual Token eval_ALRB(Value_P A, Token & LO, Token & RO, Value_P B);

   static Bif_OPER2_OUTER * fun;   ///< Built-in function.
   static Bif_OPER2_OUTER  _fun;   ///< Built-in function.

protected:
   /// Compute the outer product of A and B with function RO.
   Token outer_product(Value_P A, Token & RO, Value_P B);

   /// EOC handler for outer_product with user defined RO
   static bool eoc_OUTER(Token & token);

   /// the context for an outer product
   struct PJob_product
      {
        Cell * cZ;         ///< result cell pointer
        const Cell * cA;   ///< left value argument cell pointer
        int incA;          ///< left argument increment (for scalar extension)
        ShapeItem ZAh;     ///< high dimensions of result length
        prim_f2 RO;        ///<< right function argument
        const Cell * cB;   ///< right value argument cell pointer
        int incB;          ///< right argument increment (for scalar extension)
        ShapeItem ZBl;     ///< low dimensions of result length
        ErrorCode ec;      ///< error code
        CoreCount cores;   ///< number of cores to be used
      };

   /// the context for an outer product
   static PJob_product job;

   /// outer product for scalar RO
   inline void scalar_outer_product() const;

   /// the main loop for an outer product with scalar functions
   static void PF_scalar_outer_product(Thread_context & tctx);

   /// helper for RO. returns true if the final token was computed, and false
   /// if finish_outer_product shall be called again
   static Token finish_outer_product(EOC_arg & arg);
};
//-----------------------------------------------------------------------------

#endif // __BIF_OPER2_OUTER_HH_DEFINED__
