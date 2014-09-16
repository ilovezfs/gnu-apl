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

#ifndef __SCALAR_FUNCTION_HH_DEFINED__
#define __SCALAR_FUNCTION_HH_DEFINED__

#include <semaphore.h>

#include "PrimitiveFunction.hh"
#include "Value.icc"
#include "Id.hh"

typedef ErrorCode (Cell::*prim_f1)(Cell *) const;
typedef ErrorCode (Cell::*prim_f2)(Cell *, const Cell *) const;

//-----------------------------------------------------------------------------
/** Base class for scalar functions (functions whose monadic and/or dyadic
    version are scalar.
 */
class ScalarFunction : public PrimitiveFunction
{
public:
   /// Construct a ScalarFunction with \b Id \b id in
   ScalarFunction(TokenTag tag,
                  CellFunctionStatistics * stat_AB,
                  CellFunctionStatistics * stat_B)
   : PrimitiveFunction(tag, stat_AB, stat_B)
   {}

   /// Evaluate a scalar function monadically.
   Token eval_scalar_B(Value_P B, prim_f1 fun);

   /// Evaluate a scalar function dyadically.
   Token eval_scalar_AB(Value_P A, Value_P B, prim_f2 fun);

   /// Evaluate a scalar function dyadically with axis.
   Token eval_scalar_AXB(Value_P A, Value_P X, Value_P B, prim_f2 fun);

   /// overloaded Function::eval_fill_AB()
   virtual Token eval_fill_AB(Value_P A, Value_P B);

   /// overloaded Function::eval_fill_B()
   virtual Token eval_fill_B(Value_P B);

   /// overloaded Function::has_result()
   virtual bool has_result() const   { return true; }

protected:
   /// Apply a function to a nested sub array.
   void expand_pointers(Cell * cell_Z, const Cell * cell_A,
                        const Cell * cell_B, prim_f2 fun);

   /// A helper function for eval_scalar_AXB().
   Value_P eval_scalar_AXB(Value_P A, bool * axis_present,
                           Value_P B, prim_f2 fun, bool reversed);

   /// Evaluate \b the identity function.
   Token eval_scalar_identity_fun(Value_P B, Axis axis, Value_P FI0);

   /// a helper struct for a non-recursive implementation of
   /// monaadic scalar functions with nested values.
   struct Worklist_item1
      {
        ShapeItem len_Z;   ///< the number of result cells
        Cell * cZ;         ///< result
        const Cell * cB;   ///< right argument

        /// return B[z]
        const Cell & B_at(ShapeItem z) const   { return cB[z]; }

        /// return Z[z]
        Cell & Z_at(ShapeItem z) const   { return cZ[z]; }
      };

   /// a helper struct for a non-recursive implementation of
   /// dyadic scalar functions with nested values.
   struct Worklist_item2
      {
        ShapeItem len_Z;   ///< the number of result cells
        Cell * cZ;         ///< result
        const Cell * cA;   ///< left argument
        int inc_A;         ///< 0 (for scalar A) or 1
        const Cell * cB;   ///< right argument
        int inc_B;         ///< 0 (for scalar B) or 1

        /// return A[z]
        const Cell & A_at(ShapeItem z) const   { return cA[z * inc_A]; }

        /// return B[z]
        const Cell & B_at(ShapeItem z) const   { return cB[z * inc_B]; }

        /// return Z[z]
        Cell & Z_at(ShapeItem z) const         { return cZ[z]; }
      };
};
//-----------------------------------------------------------------------------
/** Scalar functions binomial and factorial.
 */
class Bif_F12_BINOM : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F12_BINOM()
   : ScalarFunction(TOK_F12_BINOM,
                    &Performance::cfs_BINOM_AB,
                    &Performance::cfs_BINOM_B)
   {}

   static Bif_F12_BINOM     fun;       ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_scalar_B(B, &Cell::bif_factorial); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_binomial); }

   /// overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_scalar_identity_fun(B, axis, IntScalar(1, LOC)); }

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_scalar_AXB(A, X, B, &Cell::bif_binomial); }
};
//-----------------------------------------------------------------------------
/** Scalar function less than.
 */
class Bif_F2_LESS : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F2_LESS()
   : ScalarFunction(TOK_F2_LESS, &Performance::cfs_LESS_AB, 0)
   {}

   static Bif_F2_LESS       fun;         ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_less_than); }

   /// overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_scalar_identity_fun(B, axis, IntScalar(0, LOC)); }

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_scalar_AXB(A, X, B, &Cell::bif_less_than); }
};
//-----------------------------------------------------------------------------
/** Scalar function equal.
 */
class Bif_F2_EQUAL : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F2_EQUAL()
   : ScalarFunction(TOK_F2_EQUAL, &Performance::cfs_EQUAL_AB, 0)
   {}

   static Bif_F2_EQUAL      fun;        ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_equal); }

   /// overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_scalar_identity_fun(B, axis, IntScalar(1, LOC)); }

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_scalar_AXB(A, X, B, &Cell::bif_equal); }
};
//-----------------------------------------------------------------------------
/** Scalar function greater than.
 */
class Bif_F2_GREATER : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F2_GREATER()
   : ScalarFunction(TOK_F2_GREATER, &Performance::cfs_GREATER_AB, 0)
   {}

   static Bif_F2_GREATER    fun;      ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_greater_than); }

   /// overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_scalar_identity_fun(B, axis, IntScalar(0, LOC)); }

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_scalar_AXB(A, X, B, &Cell::bif_greater_than); }
};
//-----------------------------------------------------------------------------
/** Scalar function and.
 */
class Bif_F2_AND : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F2_AND()
   : ScalarFunction(TOK_F2_AND, &Performance::cfs_AND_AB, 0)
   {}

   static Bif_F2_AND        fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_and); }

   /// return the associative cell function of this function
   virtual assoc_f2 get_assoc() const { return &Cell::bif_and; }

   /// overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_scalar_identity_fun(B, axis, IntScalar(1, LOC)); }

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_scalar_AXB(A, X, B, &Cell::bif_and); }
};
//-----------------------------------------------------------------------------
/** Scalar function or.
 */
class Bif_F2_OR : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F2_OR()
   : ScalarFunction(TOK_F2_OR, &Performance::cfs_OR_AB, 0)
   {}

   static Bif_F2_OR         fun;           ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_or); }

   /// return the associative cell function of this function
   virtual assoc_f2 get_assoc() const { return &Cell::bif_or; }

   /// overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_scalar_identity_fun(B, axis, IntScalar(0, LOC)); }

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_scalar_AXB(A, X, B, &Cell::bif_or); }
};
//-----------------------------------------------------------------------------
/** Scalar function less or equal.
 */
class Bif_F2_LEQ : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F2_LEQ()
   : ScalarFunction(TOK_F2_LEQ, &Performance::cfs_LEQ_AB, 0)
   {}

   static Bif_F2_LEQ        fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_less_eq); }

   /// overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_scalar_identity_fun(B, axis, IntScalar(1, LOC)); }

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_scalar_AXB(A, X, B, &Cell::bif_less_eq); }
};
//-----------------------------------------------------------------------------
/** Scalar function equal.
 */
class Bif_F2_MEQ : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F2_MEQ()
   : ScalarFunction(TOK_F2_MEQ, &Performance::cfs_MEQ_AB, 0)
   {}

   static Bif_F2_MEQ        fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_greater_eq); }

   /// overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_scalar_identity_fun(B, axis, IntScalar(1, LOC)); }

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_scalar_AXB(A, X, B, &Cell::bif_greater_eq); }
};
//-----------------------------------------------------------------------------
/** Scalar function not equal
 */
class Bif_F2_UNEQ : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F2_UNEQ()
   : ScalarFunction(TOK_F2_UNEQ, &Performance::cfs_UNEQ_AB, 0)
   {}

   static Bif_F2_UNEQ       fun;         ///< Built-in function.

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_not_equal); }

protected:
   /// overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_scalar_identity_fun(B, axis, IntScalar(0, LOC)); }

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_scalar_AXB(A, X, B, &Cell::bif_not_equal); }
};
//-----------------------------------------------------------------------------
/** Scalar function find.
 */
class Bif_F2_FIND : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F2_FIND()
   : ScalarFunction(TOK_F2_FIND, 0, 0)
   {}

   static Bif_F2_FIND       fun;         ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   /// Return true iff A is contained in B.
   static bool contained(const Shape & shape_A, const Cell * cA,
                         Value_P B, const Shape & idx_B, double qct);
};
//-----------------------------------------------------------------------------
/** Scalar function nor.
 */
class Bif_F2_NOR : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F2_NOR()
   : ScalarFunction(TOK_F2_NOR, &Performance::cfs_NOR_AB, 0)
   {}

   static Bif_F2_NOR        fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_nor); }

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_scalar_AXB(A, X, B, &Cell::bif_nor); }
};
//-----------------------------------------------------------------------------
/** Scalar function nand.
 */
class Bif_F2_NAND : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F2_NAND()
   : ScalarFunction(TOK_F2_NAND, &Performance::cfs_NAND_AB, 0)
   {}

   static Bif_F2_NAND       fun;         ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_nand); }

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_scalar_AXB(A, X, B, &Cell::bif_nand); }
};
//-----------------------------------------------------------------------------
/** Scalar functions power and exponential.
 */
class Bif_F12_POWER : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F12_POWER()
   : ScalarFunction(TOK_F12_POWER,
                    &Performance::cfs_POWER_AB,
                    &Performance::cfs_POWER_B)
   {}

   static Bif_F12_POWER     fun;       ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_scalar_B(B, &Cell::bif_exponential); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_power); }

   /// overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_scalar_identity_fun(B, axis, IntScalar(1, LOC)); }

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_scalar_AXB(A, X, B, &Cell::bif_power); }

   /// overloaded Function::get_monadic_inverse()
   virtual Function * get_monadic_inverse() const;

   /// overloaded Function::get_dyadic_inverse()
   virtual Function * get_dyadic_inverse() const;
};
//-----------------------------------------------------------------------------
/** Scalar functions add and conjugate.
 */
class Bif_F12_PLUS : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F12_PLUS()
   : ScalarFunction(TOK_F12_PLUS,
                    &Performance::cfs_PLUS_AB,
                    &Performance::cfs_PLUS_B)
   {}

   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_scalar_B(B, &Cell::bif_conjugate); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_add); }

   /// return the associative cell function of this function
   virtual assoc_f2 get_assoc() const { return &Cell::bif_add; }

   static Bif_F12_PLUS      fun;        ///< Built-in function.

protected:
   /// overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_scalar_identity_fun(B, axis, IntScalar(0, LOC)); }

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_scalar_AXB(A, X, B, &Cell::bif_add); }

   /// overloaded Function::get_monadic_inverse()
   virtual Function * get_monadic_inverse() const;

   /// overloaded Function::get_dyadic_inverse()
   virtual Function * get_dyadic_inverse() const;
};
//-----------------------------------------------------------------------------
/** Scalar functions subtrct and negative.
 */
class Bif_F12_MINUS : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F12_MINUS()
   : ScalarFunction(TOK_F12_MINUS,
                    &Performance::cfs_MINUS_AB,
                    &Performance::cfs_MINUS_B)
   {}

   static Bif_F12_MINUS     fun;       ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_scalar_B(B, &Cell::bif_negative); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_subtract); }

   /// overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_scalar_identity_fun(B, axis, IntScalar(0, LOC)); }

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_scalar_AXB(A, X, B, &Cell::bif_subtract); }

   /// overloaded Function::get_monadic_inverse()
   virtual Function * get_monadic_inverse() const;

   /// overloaded Function::get_dyadic_inverse()
   virtual Function * get_dyadic_inverse() const;
};
//-----------------------------------------------------------------------------
/** Scalar function roll and non-scalar function dial.
 */
class Bif_F12_ROLL : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F12_ROLL()
   : ScalarFunction(TOK_F12_ROLL, 0, &Performance::cfs_ROLL_B)
   {}

   static Bif_F12_ROLL      fun;        ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   /// dial A from B.
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   /// recursively check that all ravel elements of B are integers ≥ 0 and
   /// return \b true iff not.
   static bool check_B(const Value & B, const APL_Float qct);
};
//-----------------------------------------------------------------------------
/** Scalar function not and non-scalar function without.
 */
class Bif_F12_WITHOUT : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F12_WITHOUT()
   : ScalarFunction(TOK_F12_WITHOUT, 0, &Performance::cfs_WITHOUT_B)
   {}

   static Bif_F12_WITHOUT   fun;     ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_scalar_B(B, &Cell::bif_not); }

   /// Compute A without B.
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);
};
//-----------------------------------------------------------------------------
/** Scalar functions times and direction.
 */
class Bif_F12_TIMES : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F12_TIMES()
   : ScalarFunction(TOK_F12_TIMES,
                    &Performance::cfs_TIMES_AB,
                    &Performance::cfs_TIMES_B)
   {}

   static Bif_F12_TIMES     fun;       ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_scalar_B(B, &Cell::bif_direction); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_multiply); }

   /// return the associative cell function of this function
   virtual assoc_f2 get_assoc() const { return &Cell::bif_multiply; }

   /// overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_scalar_identity_fun(B, axis, IntScalar(1, LOC)); }

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_scalar_AXB(A, X, B, &Cell::bif_multiply); }

   /// overloaded Function::get_monadic_inverse()
   virtual Function * get_monadic_inverse() const;

   /// overloaded Function::get_dyadic_inverse()
   virtual Function * get_dyadic_inverse() const;
};
//-----------------------------------------------------------------------------
/** Scalar functions divide and reciprocal.
 */
class Bif_F12_DIVIDE : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F12_DIVIDE()
   : ScalarFunction(TOK_F12_DIVIDE,
                    &Performance::cfs_DIVIDE_AB,
                    &Performance::cfs_DIVIDE_B)
   {}

   static Bif_F12_DIVIDE    fun;      ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_scalar_B(B, &Cell::bif_reciprocal); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_divide); }

   /// overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_scalar_identity_fun(B, axis, IntScalar(1, LOC)); }

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_scalar_AXB(A, X, B, &Cell::bif_divide); }

   /// overloaded Function::get_monadic_inverse()
   virtual Function * get_monadic_inverse() const;

   /// overloaded Function::get_dyadic_inverse()
   virtual Function * get_dyadic_inverse() const;
};
//-----------------------------------------------------------------------------
/** Scalar functions circle functions and pi times.
 */
class Bif_F12_CIRCLE : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F12_CIRCLE(bool inv)
   : ScalarFunction(TOK_F12_CIRCLE,
                    &Performance::cfs_CIRCLE_AB,
                    &Performance::cfs_CIRCLE_B),
     inverse(inv)
   {}

   static Bif_F12_CIRCLE    fun;              ///< Built-in function.
   static Bif_F12_CIRCLE    fun_inverse;      ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return (inverse) ? eval_scalar_B(B, &Cell::bif_pi_times_inverse)
                         : eval_scalar_B(B, &Cell::bif_pi_times); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return (inverse) ? eval_scalar_AB(A, B, &Cell::bif_circle_fun_inverse)
                         : eval_scalar_AB(A, B, &Cell::bif_circle_fun); }

   /// overloaded Function::get_monadic_inverse()
   virtual Function * get_monadic_inverse() const;

   /// overloaded Function::get_dyadic_inverse()
   virtual Function * get_dyadic_inverse() const;

   const bool inverse;
};
//-----------------------------------------------------------------------------
/** Scalar functions minimum and round up.
 */
class Bif_F12_RND_UP : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F12_RND_UP()
   : ScalarFunction(TOK_F12_RND_UP,
                    &Performance::cfs_RND_UP_AB,
                    &Performance::cfs_RND_UP_B)
   {}

   static Bif_F12_RND_UP    fun;      ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_scalar_B(B, &Cell::bif_ceiling); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_maximum); }

   /// return the associative cell function of this function
   virtual assoc_f2 get_assoc() const { return &Cell::bif_maximum; }

   /// overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_scalar_identity_fun(B, axis, FloatScalar(-BIG_FLOAT, LOC)); }

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_scalar_AXB(A, X, B, &Cell::bif_maximum); }
};
//-----------------------------------------------------------------------------
/** Scalar functions maximum and round down.
 */
class Bif_F12_RND_DN : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F12_RND_DN()
   : ScalarFunction(TOK_F12_RND_DN,
                    &Performance::cfs_RND_DN_AB,
                    &Performance::cfs_RND_DN_B)
   {}

   static Bif_F12_RND_DN    fun;      ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_scalar_B(B, &Cell::bif_floor); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_minimum); }

   /// return the associative cell function of this function
   virtual assoc_f2 get_assoc() const { return &Cell::bif_minimum; }

   /// overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
      { return eval_scalar_identity_fun(B, axis, FloatScalar(BIG_FLOAT, LOC)); }

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_scalar_AXB(A, X, B, &Cell::bif_minimum); }
};
//-----------------------------------------------------------------------------
/** Scalar functions residue and magnitude.
 */
class Bif_F12_STILE : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F12_STILE()
   : ScalarFunction(TOK_F12_STILE,
                    &Performance::cfs_STILE_AB,
                    &Performance::cfs_STILE_B)
   {}

   static Bif_F12_STILE     fun;       ///< Built-in function.

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_residue); }

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_scalar_B(B, &Cell::bif_magnitude); }

   /// overloaded Function::eval_identity_fun();
   virtual Token eval_identity_fun(Value_P B, Axis axis)
   { return eval_scalar_identity_fun(B, axis, IntScalar(0, LOC)); }

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_scalar_AXB(A, X, B, &Cell::bif_residue); }
};
//-----------------------------------------------------------------------------
/** Scalar functions logarithms.
 */
class Bif_F12_LOGA : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F12_LOGA()
   : ScalarFunction(TOK_F12_LOGA,
                    &Performance::cfs_LOGA_AB,
                    &Performance::cfs_LOGA_B)
   {}

   static Bif_F12_LOGA      fun;        ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_scalar_B(B, &Cell::bif_nat_log); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_logarithm); }

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B)
      { return eval_scalar_AXB(A, X, B, &Cell::bif_logarithm); }

   /// overloaded Function::get_monadic_inverse()
   virtual Function * get_monadic_inverse() const;

   /// overloaded Function::get_dyadic_inverse()
   virtual Function * get_dyadic_inverse() const;
};
//-----------------------------------------------------------------------------

#endif // __SCALAR_FUNCTION_HH_DEFINED__
