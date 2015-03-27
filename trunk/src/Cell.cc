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

#include "CharCell.hh"
#include "ComplexCell.hh"
#include "Error.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "LvalCell.hh"
#include "Output.hh"
#include "PointerCell.hh"
#include "PrintOperator.hh"
#include "Value.icc"
#include "SystemLimits.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
void *
Cell::operator new(std::size_t s, void * pos)
{
   return pos;
}
//-----------------------------------------------------------------------------
void
Cell::init(const Cell & other, Value & cell_owner)
{
   Assert(&other);
   switch(other.get_cell_type())
      {
        default:
             Assert(0);

        case CT_CHAR:
             new (this) CharCell(other.get_char_value());
             return;

        case CT_INT:
             new (this) IntCell(other.get_int_value());
             return;

        case CT_FLOAT:
             new (this) FloatCell(other.get_real_value());
             return;

        case CT_COMPLEX:
             new (this) ComplexCell(other.get_complex_value());
             return;

        case CT_POINTER:
             {
               Value_P Z = other.get_pointer_value()->clone(LOC);

               new (this) PointerCell(Z, cell_owner);
             }
             return;

        case CT_CELLREF:
             new (this) LvalCell(other.get_lval_value(),
                                 other.get_cell_owner());
             return;
      }

   Assert(0 && "Bad cell type");
}
//-----------------------------------------------------------------------------
void
Cell::init_from_value(Value_P value, Value & cell_owner, const char * loc)
{
   if (value->is_scalar())
      {
        init(value->get_ravel(0), cell_owner);
      }
   else
      {
        new (this) PointerCell(value, cell_owner);
      }
}
//-----------------------------------------------------------------------------
Value_P
Cell::to_value(const char * loc) const
{
Value_P ret;
   if (is_pointer_cell())
      {
        ret = get_pointer_value(); // ->clone(LOC);
      }
   else
      {
        ret = Value_P(loc);
        ret->get_ravel(0).init(*this, ret.getref());
        ret->check_value(LOC);
      }

   return ret;
}
//-----------------------------------------------------------------------------
Cell *
Cell::init_type(const Cell & other, Value & cell_owner)
{
   // Note: this function changes the type of this cell, but the
   // compiler may not notice that and use the declaration for this
   // Cell. Using the return value avoids that.
   //
   if (other.is_pointer_cell())
      {
        new (this) PointerCell(other.get_pointer_value()->clone(LOC),
                               cell_owner);
        get_pointer_value()->to_proto();
      }
   else if (other.is_lval_cell())
      {
        FIXME;
      }
   else if (other.is_character_cell())
      {
        new (this) CharCell(UNI_ASCII_SPACE);
      }
   else // numeric
      {
        new (this) IntCell(0);
      }

   return this;
}
//-----------------------------------------------------------------------------
void
Cell::copy(Value & val, const Cell * & src, ShapeItem count)
{
   loop(c, count)
      {
        Assert1(val.more());
        val.next_ravel()->init(*src++, val);
      }
}
//-----------------------------------------------------------------------------
bool
Cell::greater(const Cell & other) const
{
   CERR << "greater() called on object of class" << get_classname() << endl;
   Assert(0);
}
//-----------------------------------------------------------------------------
bool
Cell::equal(const Cell & other, APL_Float qct) const
{
   CERR << "equal() called on object of class" << get_classname() << endl;
   Assert(0);
}
//-----------------------------------------------------------------------------
bool
Cell::tolerantly_equal(APL_Complex A, APL_Complex B, APL_Float C)
{
   // if A equals B, return true
   //
   if (A == B) return true;

   // if A and B are not in the same half-plane, return false.
   //
   // Implementation: If A and B are in the same real half-plane then
   //                 the product of their real parts is ≥ 0,
   //
   //                 If A and B are in the same imag half-plane then
   //                 the product of their imag parts is ≥ 0,
   //
   //                 Otherwise: they are not in the same half-plane
   //                 and we return false;
   //
   if (A.real() * B.real() < 0.0 &&
       A.imag() * B.imag() < 0.0)   return false;

   // If the distance-between A and B is ≤ C times the larger-magnitude
   // of A and B, return true
   //
   // Implementation: Instead of mag(A-B)  ≤ C × max(mag(A),   mag(B))
   // we compute                 mag²(A-B) ≤ C² × max(mag²(A), mag²(B))
   //
   // 1. compute max(mag²A, mag²B)
   //
const APL_Float mag2_A   = A.real() * A.real() + A.imag() * A.imag();
const APL_Float mag2_B   = B.real() * B.real() + B.imag() * B.imag();
const APL_Float mag2_max = mag2_A > mag2_B ? mag2_A : mag2_B;

   // 2. compute mag²(A-B)
   //
const APL_Complex A_B = A - B;
const APL_Float dist2_A_B = A_B.real() * A_B.real()
                          + A_B.imag() * A_B.imag();
   // compare
   //

   return dist2_A_B <= C*C*mag2_max;
}
//-----------------------------------------------------------------------------
bool
Cell::tolerantly_equal(APL_Float A, APL_Float B, APL_Float C)
{
   // if the signs of A and B differ then they are unequal (ISO standard
   // page 19). We treat exact 0.0 as having both signs
   //
   if (A == B)               return true;
   if (A < 0.0 && B > 0.0)   return false;
   if (A > 0.0 && B < 0.0)   return false;

APL_Float mag_A = A < 0 ? -A : A;
APL_Float mag_B = B < 0 ? -B : B;
APL_Float mag_max = (mag_A > mag_B) ? mag_A : mag_B;

const APL_Float dist_A_B = (A > B) ? (A - B) : (B - A);

   return (dist_A_B < C*mag_max);
}
//-----------------------------------------------------------------------------
bool
Cell::is_near_int(APL_Float value)
{
   if (value > LARGE_INT)   return true;
   if (value < SMALL_INT)   return true;

const double result = nearbyint(value);
const double diff = value - result;
   if (diff >= INTEGER_TOLERANCE)    return false;
   if (diff <= -INTEGER_TOLERANCE)   return false;

   return true;
}
//-----------------------------------------------------------------------------
APL_Integer
Cell::near_int(APL_Float value)
{
const double result = nearbyint(value);
const double diff = value - result;
   if (diff > INTEGER_TOLERANCE)    DOMAIN_ERROR;
   if (diff < -INTEGER_TOLERANCE)   DOMAIN_ERROR;

   if (result > 0)   return   APL_Integer(0.3 + result);
   else              return - APL_Integer(0.3 - result);
}
//-----------------------------------------------------------------------------
bool
Cell::greater_vec(const Cell * ca, const Cell * cb, const void * comp_arg)
{
const ShapeItem comp_len = *(const ShapeItem *)comp_arg;
const APL_Float qct = Workspace::get_CT();

   // most frequently comp_len is 1, so we optimize for this case.
   //
   if (comp_len == 1)
      {
        const bool equal = ca[0].equal(cb[0], qct);
        if (equal)   return ca > cb;
        const bool result = ca[0].greater(cb[0]);
        return result;
      }

   loop(c, comp_len)
      {
        const bool equal = ca[c].equal(cb[c], qct);
        if (equal)   continue;
        const bool result = ca[c].greater(cb[c]);
        return result;
      }

   return ca > cb;   // a and b are equal: sort by position
}
//-----------------------------------------------------------------------------
bool
Cell::smaller_vec(const Cell * ca, const Cell * cb, const void * comp_arg)
{
const ShapeItem comp_len = *(const ShapeItem *)comp_arg;
const APL_Float qct = Workspace::get_CT();

   // most frequently comp_len is 1, so we optimize for this case.
   //
   if (comp_len == 1)
      {
        const bool equal = ca[0].equal(cb[0], qct);
        if (equal)   return ca > cb;
        const bool result = ca[0].greater(cb[0]);
        return !result;
      }

   loop(c, comp_len)
      {
        const bool equal = ca[c].equal(cb[c], qct);
        if (equal)   continue;
        const bool result = ca[c].greater(cb[c]);
        return !result;
      }

   return ca > cb;   // a and b are equal: sort by position
}
//-----------------------------------------------------------------------------
ostream & 
operator <<(ostream & out, const Cell & cell)
{
PrintBuffer pb = cell.character_representation(PR_BOXED_GRAPHIC);
UCS_string ucs(pb, 0, Workspace::get_PrintContext().get_PW());
   return out << ucs << " ";
}
//-----------------------------------------------------------------------------
ErrorCode
Cell::bif_equal(Cell * Z, const Cell * A) const
{
   // incompatible types ?
   //
   if (is_character_cell() != A->is_character_cell())   return IntCell::z0(Z);

   return IntCell::zv(Z, equal(*A, Workspace::get_CT()));
}
//-----------------------------------------------------------------------------
ErrorCode
Cell::bif_not_equal(Cell * Z, const Cell * A) const
{
   // incompatible types ?
   //
   if (is_character_cell() != A->is_character_cell())   return IntCell::z1(Z);

   return IntCell::zv(Z, !equal(*A, Workspace::get_CT()));
}
//-----------------------------------------------------------------------------
ErrorCode
Cell::bif_greater_than(Cell * Z, const Cell * A) const
{
   return IntCell::zv(Z, (A->compare(*this) == COMP_GT) ? 1 : 0);
}
//-----------------------------------------------------------------------------
ErrorCode
Cell::bif_less_eq(Cell * Z, const Cell * A) const
{
   return IntCell::zv(Z, (A->compare(*this) != COMP_GT) ? 1 : 0);
}
//-----------------------------------------------------------------------------
ErrorCode
Cell::bif_less_than(Cell * Z, const Cell * A) const
{
   return IntCell::zv(Z, (A->compare(*this) == COMP_LT) ? 1 : 0);
}
//-----------------------------------------------------------------------------
ErrorCode
Cell::bif_greater_eq(Cell * Z, const Cell * A) const
{
   return IntCell::zv(Z, (A->compare(*this) != COMP_LT) ? 1 : 0);
}
//-----------------------------------------------------------------------------
