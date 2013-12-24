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

#include "Shape.hh"
#include "Value.hh"

//-----------------------------------------------------------------------------
Shape::Shape(Value_P A, APL_Float qct, int qio_A)
{
   // check that A is a shape value, like the left argument of A⍴B
   //
   if (A->get_rank() > 1)   RANK_ERROR;

   rho_rho = A->element_count();

   if (rho_rho > MAX_RANK)   LIMIT_ERROR_RANK;   // of A

   loop(r, rho_rho)   rho[r] = A->get_ravel(r).get_near_int(qct) - qio_A;
}
//-----------------------------------------------------------------------------
Shape Shape::abs() const
{
Shape ret;
   ret.rho_rho = rho_rho;
   loop(r, rho_rho)
      {
        if (rho[r] < 0)   ret.rho[r] = - rho[r];
        else              ret.rho[r] =   rho[r];
      }

   return ret;
}
//-----------------------------------------------------------------------------
bool
Shape::operator ==(const Shape & other) const
{
   if (rho_rho != other.rho_rho)   return false;
 
   loop(r, rho_rho)
   if (rho[r] != other.rho[r])   return false;

   return true;
}
//-----------------------------------------------------------------------------
void
Shape::expand_rank(Rank rk)
{
   if (rho_rho < rk)   // expand rank by prepending axes of length 1
      {
        const Rank diff = rk - rho_rho;
        loop(r, rho_rho)   rho[r + diff] = rho[r];
        loop(r, diff)   rho[r] = 1;
        rho_rho = rk;
      }
}
//-----------------------------------------------------------------------------
void
Shape::expand(const Shape & B)
{
   // increase rank as necessary
   //
   expand_rank(B.get_rank());

   // increase axes as necessary
   //
   loop(r, rho_rho)
      {
        if (rho[r] < B.rho[r])   rho[r] = B.rho[r];
      }
}
//-----------------------------------------------------------------------------
Shape
Shape::insert_axis(Axis axis, ShapeItem len) const
{
   if (get_rank() >= MAX_RANK)   LIMIT_ERROR_RANK;

   if (axis <= 0)   // insert before first axis
      {
        Shape ret(len);
        loop(r, get_rank())   ret.add_shape_item(get_shape_item(r));
        return ret;
      }

   if (axis >= get_rank())   // insert after last axis
      {
        Shape ret(*this);
        ret.add_shape_item(len);
        return ret;
      }

   // insert after (including) axis
   //
Shape ret;
   loop(r, axis)   ret.add_shape_item(get_shape_item(r));
   ret.add_shape_item(len);
   for (Rank r = axis; r < get_rank(); ++r)
       ret.add_shape_item(get_shape_item(r));

   return ret;
}
//-----------------------------------------------------------------------------
Shape
Shape::remove_axis(Axis axis) const
{
Shape ret(*this);
   ret.remove_shape_item(axis);
   return ret;
}
//-----------------------------------------------------------------------------
Shape
Shape::inverse_permutation() const
{
Shape ret;

   // first, set all items to -1.
   loop(r, get_rank())   ret.add_shape_item(-1);

   // then, set all items to the shape items of this shape
   loop(r, get_rank())
       {
         const ShapeItem rx = get_shape_item(r);
         if (rx < 0)                         DOMAIN_ERROR;
         if (rx >= get_rank())               DOMAIN_ERROR;
         if (ret.get_shape_item(rx) != -1)   DOMAIN_ERROR;
          ret.set_shape_item(rx, r);
       }

   return ret;
}
//-----------------------------------------------------------------------------
Shape
Shape::permute(const Shape & perm) const
{
Shape ret;
   loop(r, perm.get_rank())
      {
        ret.add_shape_item(get_shape_item(perm.get_shape_item(r)));
      }

   return ret;
}
//-----------------------------------------------------------------------------
ShapeItem
Shape::ravel_pos(const Shape & idx) const
{
ShapeItem p = 0;
ShapeItem w = 1;

   for(Rank r = get_rank(); r-- > 0;)
      {
        p += w*idx.get_shape_item(r);
        w *= get_shape_item(r);
      }

   return p;
}
//-----------------------------------------------------------------------------
void
Shape::check_same(const Shape & B, ErrorCode rank_err, ErrorCode len_err,
                  const char * loc) const
{
   if (get_rank() != B.get_rank())
      throw_apl_error(rank_err, loc);

   loop(r, get_rank())
      {
        if (get_shape_item(r) != B.get_shape_item(r))
           throw_apl_error(len_err, loc);
      }
}
//-----------------------------------------------------------------------------
bool
Shape::is_permutation() const
{
ShapeItem sh[get_rank()];

   // first, set all items to -1.
   //
   loop(r, get_rank())   sh[r] = -1;

   // then, set all items to the shape items of this shape
   //
   loop(r, get_rank())
       {
         const ShapeItem rx = get_shape_item(r);
         if (rx < 0)             return false;
         if (rx >= get_rank())   return false;
         if (sh[rx] != -1)       return false;
          sh[rx] = r;
       }

   return true;
}
//-----------------------------------------------------------------------------
ostream &
operator <<(ostream & out, const Shape & shape)
{
   out << "⊏";
   loop(r, shape.get_rank())
       {
         if (r)   out << ":";
         out << shape.get_shape_item(r);
       }
   out << "⊐";
   return out;
}
//-----------------------------------------------------------------------------
