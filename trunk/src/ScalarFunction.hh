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

#ifndef __SCALAR_FUNCTION_HH_DEFINED__
#define __SCALAR_FUNCTION_HH_DEFINED__

#include <semaphore.h>

#include "Parallel.hh"
#include "PrimitiveFunction.hh"
#include "Value.icc"
#include "Id.hh"

//-----------------------------------------------------------------------------
/** Base class for scalar functions (functions whose monadic and/or dyadic
    version are scalar.
 */
class ScalarFunction : public PrimitiveFunction
{
public:
   /// Construct a ScalarFunction with \b Id \b id
   ScalarFunction(TokenTag tag, CellFunctionStatistics * stat_AB,
                                CellFunctionStatistics * stat_B,
                                ShapeItem thresh_AB,
                                ShapeItem thresh_B)
   : PrimitiveFunction(tag, stat_AB, stat_B)
   {
     set_dyadic_threshold(thresh_AB);
     set_monadic_threshold(thresh_B);
   }

   /// Evaluate a scalar function monadically.
   Token eval_scalar_B(Value_P B, prim_f1 fun);

   /// Evaluate a scalar function dyadically.
   Token eval_scalar_AB(Value_P A, Value_P B, prim_f2 fun);

   /// overloaded Function::get_scalar_f2
   virtual prim_f2 get_scalar_f2() const = 0;

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
   void expand_pointers(Cell * cell_Z, Value & Z_owner, const Cell * cell_A,
                        const Cell * cell_B, prim_f2 fun);

   /// A helper function for eval_scalar_AXB().
   Value_P eval_scalar_AXB(Value_P A, bool * axis_present,
                           Value_P B, prim_f2 fun, bool reversed);

   /// Evaluate \b the identity function.
   Token eval_scalar_identity_fun(Value_P B, Axis axis, Value_P FI0);

   /// parallel eval_scalar_AB
   static Thread_context::PoolFunction PF_eval_scalar_AB;

   /// parallel eval_scalar_B
   static Thread_context::PoolFunction PF_eval_scalar_B;
};

#define PERF_A(x)   TOK_F2_ ## x,                           \
                  & Performance::cfs_F2_    ## x ## _AB, 0, \
                    Performance::thresh_F2_ ## x ## _AB, -1

#define PERF_AB(x)  TOK_F12_ ## x,                    \
                  & Performance::cfs_F12_ ## x ## _AB,    \
                  & Performance::cfs_F12_ ## x ## _B,     \
                    Performance::thresh_F12_ ## x ## _AB, \
                    Performance::thresh_F12_ ## x ## _B

#define PERF_B(x)   TOK_F12_ ## x,                          \
                  0, & Performance::cfs_F12_    ## x ## _B, \
                  -1,  Performance::thresh_F12_ ## x ## _B

//-----------------------------------------------------------------------------
/** Scalar functions binomial and factorial.
 */
class Bif_F12_BINOM : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F12_BINOM()
   : ScalarFunction(PERF_AB(BINOM))
   {}

   static Bif_F12_BINOM * fun;       ///< Built-in function.
   static Bif_F12_BINOM   _fun;      ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_scalar_B(B, &Cell::bif_factorial); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_binomial); }

   /// overloaded Function::get_scalar_f2
   virtual prim_f2 get_scalar_f2() const
      { return &Cell::bif_binomial; }

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
   : ScalarFunction(PERF_A(LESS))
   {}

   static Bif_F2_LESS * fun;         ///< Built-in function.
   static Bif_F2_LESS  _fun;         ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_less_than); }

   /// overloaded Function::get_scalar_f2
   virtual prim_f2 get_scalar_f2() const
      { return &Cell::bif_less_than; }

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
   : ScalarFunction(PERF_A(EQUAL))
   {}

   static Bif_F2_EQUAL * fun;        ///< Built-in function.
   static Bif_F2_EQUAL  _fun;        ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_equal); }

   /// overloaded Function::get_scalar_f2
   virtual prim_f2 get_scalar_f2() const
      { return &Cell::bif_equal; }

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
   : ScalarFunction(PERF_A(GREATER))
   {}

   static Bif_F2_GREATER * fun;      ///< Built-in function.
   static Bif_F2_GREATER  _fun;      ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_greater_than); }

   /// overloaded Function::get_scalar_f2
   virtual prim_f2 get_scalar_f2() const
      { return &Cell::bif_greater_than; }

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
   : ScalarFunction(PERF_A(AND))
   {}

   static Bif_F2_AND * fun;          ///< Built-in function.
   static Bif_F2_AND  _fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_and); }

   /// overloaded Function::get_scalar_f2
   virtual prim_f2 get_scalar_f2() const
      { return &Cell::bif_and; }

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
   : ScalarFunction(PERF_A(OR))
   {}

   static Bif_F2_OR * fun;           ///< Built-in function.
   static Bif_F2_OR  _fun;           ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_or); }

   /// overloaded Function::get_scalar_f2
   virtual prim_f2 get_scalar_f2() const
      { return &Cell::bif_or; }

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
   : ScalarFunction(PERF_A(LEQ))
   {}

   static Bif_F2_LEQ * fun;          ///< Built-in function.
   static Bif_F2_LEQ  _fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_less_eq); }

   /// overloaded Function::get_scalar_f2()
   virtual prim_f2 get_scalar_f2() const
      { return &Cell::bif_less_eq; }

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
   : ScalarFunction(PERF_A(MEQ))
   {}

   static Bif_F2_MEQ * fun;          ///< Built-in function.
   static Bif_F2_MEQ  _fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_greater_eq); }

   /// overloaded Function::get_scalar_f2()
   virtual prim_f2 get_scalar_f2() const
      { return &Cell::bif_greater_eq; }

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
   : ScalarFunction(PERF_A(UNEQ))
   {}

   static Bif_F2_UNEQ * fun;         ///< Built-in function.
   static Bif_F2_UNEQ  _fun;         ///< Built-in function.

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_not_equal); }

   /// overloaded Function::get_scalar_f2()
   virtual prim_f2 get_scalar_f2() const
      { return &Cell::bif_not_equal; }

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
   : ScalarFunction(PERF_A(FIND))
   {}

   static Bif_F2_FIND * fun;         ///< Built-in function.
   static Bif_F2_FIND  _fun;         ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   /// overloaded Function::get_scalar_f2()
   virtual prim_f2 get_scalar_f2() const
      { return 0; }

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
   : ScalarFunction(PERF_A(NOR))
   {}

   static Bif_F2_NOR * fun;          ///< Built-in function.
   static Bif_F2_NOR  _fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_nor); }

   /// overloaded Function::get_scalar_f2()
   virtual prim_f2 get_scalar_f2() const
      { return &Cell::bif_nor; }

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
   : ScalarFunction(PERF_A(NAND))
   {}

   static Bif_F2_NAND * fun;         ///< Built-in function.
   static Bif_F2_NAND  _fun;         ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_nand); }

   /// overloaded Function::get_scalar_f2()
   virtual prim_f2 get_scalar_f2() const
      { return &Cell::bif_nand; }

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
   : ScalarFunction(PERF_AB(POWER))
   {}

   static Bif_F12_POWER * fun;       ///< Built-in function.
   static Bif_F12_POWER  _fun;       ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_scalar_B(B, &Cell::bif_exponential); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_power); }

   /// overloaded Function::get_scalar_f2()
   virtual prim_f2 get_scalar_f2() const
      { return &Cell::bif_power; }

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
   : ScalarFunction(PERF_AB(PLUS))
   {}

   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_scalar_B(B, &Cell::bif_conjugate); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_add); }

   /// overloaded Function::get_scalar_f2()
   virtual prim_f2 get_scalar_f2() const
      { return &Cell::bif_add; }

   /// return the associative cell function of this function
   virtual assoc_f2 get_assoc() const { return &Cell::bif_add; }

   static Bif_F12_PLUS * fun;        ///< Built-in function.
   static Bif_F12_PLUS  _fun;        ///< Built-in function.

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
   : ScalarFunction(PERF_AB(MINUS))
   {}

   static Bif_F12_MINUS * fun;       ///< Built-in function.
   static Bif_F12_MINUS  _fun;       ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_scalar_B(B, &Cell::bif_negative); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_subtract); }

   /// overloaded Function::get_scalar_f2()
   virtual prim_f2 get_scalar_f2() const
      { return &Cell::bif_subtract; }

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
   : ScalarFunction(PERF_B(ROLL))
   {}

   static Bif_F12_ROLL * fun;        ///< Built-in function.
   static Bif_F12_ROLL  _fun;        ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   /// dial A from B.
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   /// overloaded Function::get_scalar_f2
   virtual prim_f2 get_scalar_f2() const
      { return 0; }

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
   : ScalarFunction(PERF_B(WITHOUT))
   {}

   static Bif_F12_WITHOUT * fun;     ///< Built-in function.
   static Bif_F12_WITHOUT  _fun;     ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_scalar_B(B, &Cell::bif_not); }

   /// Compute A without B.
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   /// overloaded Function::get_scalar_f2
   virtual prim_f2 get_scalar_f2() const
      { return 0; }
};
//-----------------------------------------------------------------------------
/** Scalar functions times and direction.
 */
class Bif_F12_TIMES : public ScalarFunction
{
public:
   /// Constructor.
   Bif_F12_TIMES()
   : ScalarFunction(PERF_AB(TIMES))
   {}

   static Bif_F12_TIMES * fun;       ///< Built-in function.
   static Bif_F12_TIMES  _fun;       ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_scalar_B(B, &Cell::bif_direction); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_multiply); }

   /// overloaded Function::get_scalar_f2()
   virtual prim_f2 get_scalar_f2() const
      { return &Cell::bif_multiply; }

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
   : ScalarFunction(PERF_AB(DIVIDE))
   {}

   static Bif_F12_DIVIDE * fun;      ///< Built-in function.
   static Bif_F12_DIVIDE  _fun;      ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_scalar_B(B, &Cell::bif_reciprocal); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_divide); }

   /// overloaded Function::get_scalar_f2
   virtual prim_f2 get_scalar_f2() const
      { return &Cell::bif_divide; }

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
   : ScalarFunction(PERF_AB(CIRCLE)),
     inverse(inv)
   {}

   static Bif_F12_CIRCLE * fun;              ///< Built-in function.
   static Bif_F12_CIRCLE  _fun;              ///< Built-in function.
   static Bif_F12_CIRCLE * fun_inverse;      ///< Built-in function.
   static Bif_F12_CIRCLE  _fun_inverse;      ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return (inverse) ? eval_scalar_B(B, &Cell::bif_pi_times_inverse)
                         : eval_scalar_B(B, &Cell::bif_pi_times); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return (inverse) ? eval_scalar_AB(A, B, &Cell::bif_circle_fun_inverse)
                         : eval_scalar_AB(A, B, &Cell::bif_circle_fun); }

   /// overloaded Function::get_scalar_f2
   virtual prim_f2 get_scalar_f2() const
      { return (inverse) ? &Cell::bif_circle_fun_inverse
                         : &Cell::bif_circle_fun; }

   /// overloaded Function::get_monadic_inverse()
   virtual Function * get_monadic_inverse() const;

   /// overloaded Function::get_dyadic_inverse()
   virtual Function * get_dyadic_inverse() const;

   /// true if the inverse shall be computed. This allows Bif_F12_CIRCLE to
   /// be instantiated twice: once for non-inverted operation and once for
   /// inverted operation, and both instances can share some code in this class
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
   : ScalarFunction(PERF_AB(RND_UP))
   {}

   static Bif_F12_RND_UP * fun;      ///< Built-in function.
   static Bif_F12_RND_UP  _fun;      ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_scalar_B(B, &Cell::bif_ceiling); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_maximum); }

   /// overloaded Function::get_scalar_f2()
   virtual prim_f2 get_scalar_f2() const
      { return &Cell::bif_maximum; }

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
   : ScalarFunction(PERF_AB(RND_DN))
   {}

   static Bif_F12_RND_DN * fun;      ///< Built-in function.
   static Bif_F12_RND_DN  _fun;      ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_scalar_B(B, &Cell::bif_floor); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_minimum); }

   /// overloaded Function::get_scalar_f2()
   virtual prim_f2 get_scalar_f2() const
      { return &Cell::bif_minimum; }

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
   : ScalarFunction(PERF_AB(STILE))
   {}

   static Bif_F12_STILE * fun;       ///< Built-in function.
   static Bif_F12_STILE  _fun;       ///< Built-in function.

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_residue); }

   /// overloaded Function::get_scalar_f2()
   virtual prim_f2 get_scalar_f2() const
      { return &Cell::bif_residue; }

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
   : ScalarFunction(PERF_AB(LOGA))
   {}

   static Bif_F12_LOGA * fun;        ///< Built-in function.
   static Bif_F12_LOGA  _fun;        ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return eval_scalar_B(B, &Cell::bif_nat_log); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return eval_scalar_AB(A, B, &Cell::bif_logarithm); }

   /// overloaded Function::get_scalar_f2()
   virtual prim_f2 get_scalar_f2() const
      { return &Cell::bif_logarithm; }

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
