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

#ifndef __COMPLEXCELL_HH_DEFINED__
#define __COMPLEXCELL_HH_DEFINED__

#include "NumericCell.hh"

//-----------------------------------------------------------------------------
/*!
 A cell containing a single APL complex value. This class essentially
 overloads certain functions in class Cell with complex number specific
 implementations.
 */
class ComplexCell : public NumericCell
{
public:
   /// Construct an complex number cell from a complex number
   ComplexCell(APL_Complex c);

   /// Construct an complex number cell from real part \b r and imag part \b i.
   ComplexCell(APL_Float r, APL_Float i);

   /// overloaded Cell::is_complex_cell().
   virtual bool is_complex_cell() const   { return true; }

   /// overloaded Cell::greater().
   virtual bool greater(const Cell * other, bool ascending) const;

   /// overloaded Cell::equal().
   virtual bool equal(const Cell & other, APL_Float qct) const;

   /// overloaded Cell::compare()
   virtual Comp_result compare(const Cell & other) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_ceiling(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_conjugate(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_direction(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_exponential(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_factorial(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_floor(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_magnitude(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_nat_log(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_negative(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_pi_times(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_pi_times_inverse(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_reciprocal(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_roll(Cell * Z) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_add(Cell * Z, const Cell * A) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_subtract(Cell * Z, const Cell * A) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_divide(Cell * Z, const Cell * A) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_equal(Cell * Z, const Cell * A) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_logarithm(Cell * Z, const Cell * A) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_multiply(Cell * Z, const Cell * A) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_power(Cell * Z, const Cell * A) const;

  /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_maximum(Cell * Z, const Cell * A) const;

  /// Overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_minimum(Cell * Z, const Cell * A) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_residue(Cell * Z, const Cell * A) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_circle_fun(Cell * Z, const Cell * A) const;

   /// overloaded from the corresponding Cell:: function (see class Cell).
   virtual ErrorCode bif_circle_fun_inverse(Cell * Z, const Cell * A) const;

   /// the Quad_CR representation of this cell.
   virtual PrintBuffer character_representation(const PrintContext &pctx) const;

   /// return true iff this cell needs scaling (exponential format) in pctx.
   virtual bool need_scaling(const PrintContext &pctx) const;

   /// the lanczos approximation for gamma(1.0 + x + iy) for x >= 0.
   static APL_Complex gamma(APL_Float x, const APL_Float y);

protected:
   /// compute circle function \b fun
   ErrorCode do_bif_circle_fun(Cell * Z, int fun) const;

   /// 1.0
   static APL_Complex ONE()       { return APL_Complex(1, 0); }

   /// i
   static APL_Complex PLUS_i()   { return APL_Complex(0, 1); }

   /// -i
   static APL_Complex MINUS_i()   { return APL_Complex(0, -1); }

   /// the phase of \b cpx number.
   APL_Float phase() const;

   /// overloaded Cell::get_cell_type().
   virtual CellType get_cell_type() const
      { return CT_COMPLEX; }

   /// overloaded Cell::get_real_value().
   virtual APL_Float get_real_value() const;

   /// overloaded Cell::get_imag_value().
   virtual APL_Float get_imag_value() const;

   /// overloaded Cell::get_complex_value()
   virtual APL_Complex get_complex_value() const   { return cval(); }

   /// overloaded Cell::get_near_bool()
   virtual bool get_near_bool(APL_Float qct)  const;

   /// overloaded Cell::get_near_int()
   virtual APL_Integer get_near_int(APL_Float qct)  const;

   /// overloaded Cell::get_checked_near_int()
   virtual APL_Integer get_checked_near_int()  const
      { return APL_Integer(value.cval_r + 0.3); }

   /// overloaded Cell::is_near_int()
   virtual bool is_near_int(APL_Float qct) const;

   /// overloaded Cell::is_near_zero()
   virtual bool is_near_zero(APL_Float qct) const;

   /// overloaded Cell::is_near_one()
   virtual bool is_near_one(APL_Float qct) const;

   /// overloaded Cell::is_near_real()
   virtual bool is_near_real(APL_Float qct) const
      { return (value2.cval_i < qct) && (value2.cval_i > -qct); }

   /// overloaded Cell::get_classname()
   virtual const char * get_classname() const   { return "ComplexCell"; }

   /// overloaded Cell::demote_complex_to_real()
   virtual void demote_complex_to_real(APL_Float qct);

   /// overloaded Cell::CDR_size()
   virtual int CDR_size() const { return 16; }
};
//=============================================================================

#endif // __COMPLEXCELL_HH_DEFINED__
