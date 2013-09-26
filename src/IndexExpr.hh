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

#ifndef __INDEXEXPR_HH_DEFINED__
#define __INDEXEXPR_HH_DEFINED__

#include <vector>

#include "Cell.hh"
#include "DynamicObject.hh"
#include "Value.hh"

//-----------------------------------------------------------------------------
/**
     An array of index values.
 */
class IndexExpr : public DynamicObject
{
public:
   /// constructor: empty (0-dimensional) IndexExpr
   IndexExpr(const char * loc);

   /// destructor
   ~IndexExpr();

   /// The quad-ct for this index.
   APL_Float quad_ct;

   /// The quad-io for this index.
   uint32_t quad_io;

   /// The values (0 meaning all indices as in [] or [;].
   vector<Value_P>values;

   /// append a value.
   void add(Value_P val)
      { if (val)   val->set_index();   values.push_back(val); }

   /// return the number of values (= number of semicolons + 1)
   size_t value_count() const   { return values.size(); }

   /// return true iff the number of dimensions is 1 (i.e. no ; and non-empty)
   bool is_axis() const   { return values.size() == 1; }

   /// Return an axis (from an IndexExpr of rank 1.
   Rank get_axis(Rank max_axis) const;

   /// set axis \b axis to \b val
   void set_value(Axis axis, Value_P val);

   /// Return a set of axes as shape. This is used to convert a number of
   /// axes (like in [2 3]) into a shape (like 2 3).
   Shape to_shape() const;

   /// print stale IndexExprs, and return the number of stale IndexExprs.
   static int print_stale(ostream & out);

   /// erase stale IndexExprs
   static void erase_stale(const char * loc);

protected:
};
//-----------------------------------------------------------------------------

#endif // __INDEXEXPR_HH_DEFINED__
