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

#ifndef __CHARCELL_HH_DEFINED__
#define __CHARCELL_HH_DEFINED__

#include "Cell.hh"

//-----------------------------------------------------------------------------
/*!
    A cell containing a single APL character
 */
class CharCell : public Cell
{
public:
   /// Construct a character cell containing \b av
   CharCell(Unicode av)
      { value.aval = av; }

   /// overloaded Cell::is_character_cell()
   virtual bool is_character_cell() const
      { return true; }

   /// overloaded Cell::greater()
   virtual bool greater(const Cell * other, bool ascending) const;

   /// overloaded Cell::equal()
   virtual bool equal(const Cell & other, APL_Float qct) const;

   /// overloaded Cell::is_example_field()
   virtual bool is_example_field() const;

   /// overloaded Cell::compare()
   virtual Comp_result compare(const Cell & other) const;

   /// the Quad_CR representation of this cell
   virtual PrintBuffer character_representation(const PrintContext &pctx) const;

protected:
   /// overloaded Cell::get_cell_type()
   virtual CellType get_cell_type() const
      { return CT_CHAR; }

   /// overloaded Cell::get_cell_subtype()
   virtual CellType get_cell_subtype() const;

   /// overloaded Cell::get_char_value()
   virtual Unicode get_char_value() const   { return value.aval; }

   /// overloaded Cell::get_classname()
   virtual const char * get_classname()  const   { return "CharCell"; }

   /// overloaded Cell::CDR_size()
   virtual int CDR_size() const;
};
//-----------------------------------------------------------------------------

#endif // __CHARCELL_HH_DEFINED__
