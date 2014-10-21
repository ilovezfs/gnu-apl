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

#ifndef __COLLATING_CACHE_HH_DEFINED__
#define __COLLATING_CACHE_HH_DEFINED__

#include "Common.hh"
#include "PrimitiveFunction.hh"
#include "Token.hh"
#include "Shape.hh"

class Cell;

//-----------------------------------------------------------------------------
/// one item of a CollatingCache
struct CollatingCacheEntry
{
   /// constructor: an entry with character \b c and shape \b shape
   CollatingCacheEntry(Unicode uni, const Shape & shape)
   : ce_char(uni),
     ce_shape(shape)
   {}

   /// copy from another CollatingCacheEntry
   CollatingCacheEntry & operator=(const CollatingCacheEntry & other)
      { new (this) CollatingCacheEntry(other.ce_char, other.ce_shape);
        return *this; }

   /// return the axis'th item of shape
   ShapeItem get_pos(Rank axis) const  { return ce_shape.get_shape_item(axis); }

   /// the character
   const Unicode ce_char;

   /// the shape
   Shape ce_shape;

   /// compare this entry with \b other
   int compare(const CollatingCacheEntry & other, bool ascending,
               Rank axis) const;
};
//-----------------------------------------------------------------------------
/// A collating cache which is the internal representation of the left
/// argument A of dydic A⍋B or A⍒B
class CollatingCache : public vector<CollatingCacheEntry>
{
public:
   /// constructor: cache of rank r and comparison length clen
   CollatingCache(Rank r, ShapeItem clen)
   : rank(r),
     comp_len(clen)
   {}

   /// return the number of dimensions of the collating sequence
   Rank get_rank() const { return rank; }

   /// return the number of items to compare
   ShapeItem get_comp_len() const { return comp_len; }

   /// compare cache items ia and ib ascendingly.
   /// \b comp_arg is a CollatingCache pointer
   static bool greater_vec(const Cell * ia, const Cell * ib, const void * arg);

   /// compare cache items ia and ib descendingly.
   /// \b comp_arg is a CollatingCache pointer
   static bool smaller_vec(const Cell * ia, const Cell * ib,
                           const void * arg);
protected:
   /// the rank of the collating sequence
   const Rank rank;

   /// the number of items to compare
   const ShapeItem comp_len;
};
//=============================================================================
/** primitive functions grade up and grade down
 */
class Bif_F12_SORT : public NonscalarFunction
{
public:
   /// Constructor
   Bif_F12_SORT(TokenTag tag)
   : NonscalarFunction(tag)
   {}

   /// sort integer vector B 
   static Token sort(Value_P B, bool ascending);

protected:
   /// a helper structure for sorting: a cahr and a shape
   struct char_shape
      {
         APL_Char achar;    ///< a char
         Shape    ashape;   ///< a shape
      };

   /// sort char vector B according to collationg sequence A
   Token sort_collating(Value_P A, Value_P B, bool ascending);

   /// the collating cache that determines the order of elements
   static ShapeItem collating_cache(Unicode uni, Value_P A,
                                    CollatingCache & cache);
};
//-----------------------------------------------------------------------------
/** System function grade up ⍋
 */
class Bif_F12_SORT_ASC : public Bif_F12_SORT
{
public:
   /// Constructor
   Bif_F12_SORT_ASC()
   : Bif_F12_SORT(TOK_F12_SORT_ASC)
   {}

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B)
      { return sort(B, true); }

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B)
      { return sort_collating(A, B, true); }

   static Bif_F12_SORT_ASC fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------
/** System function grade down ⍒
 */
class Bif_F12_SORT_DES : public Bif_F12_SORT
{
public:
   /// Constructor
   Bif_F12_SORT_DES()
   : Bif_F12_SORT(TOK_F12_SORT_DES)
   {}

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B)
      { return sort(B, false); }

   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B)
      { return sort_collating(A, B, false); }

   static Bif_F12_SORT_DES fun;   ///< Built-in function
protected:
};
//-----------------------------------------------------------------------------


#endif // __COLLATING_CACHE_HH_DEFINED__
