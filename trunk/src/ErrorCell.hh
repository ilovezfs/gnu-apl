/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2014  Dr. Jürgen Sauermann

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

#ifndef __ERRORCELL_HH_DEFINED__
#define __ERRORCELL_HH_DEFINED__

#include "Cell.hh"

//-----------------------------------------------------------------------------
/*!
    A cell containing an error code.  This class essentially
    overloads certain functions in class Cell with error related
    implementations.
 */


class ErrorCell : public Cell
{
public:
   /// Construct an error cell with ErrorCode \b e
   ErrorCell(ErrorCode  e)
      { value.eval = e; }

   /// overloaded Cell::is_error_cell()
   virtual bool is_error_cell() const
      { return true; }

protected:
   /// overloaded Cell::get_cell_type()
   virtual CellType get_cell_type() const
      { return CT_ERROR; }

   /// overloaded Cell::get_error_value()
   virtual ErrorCode get_error_value() const
      { return value.eval; }

   /// overloaded Cell::get_classname()
   virtual const char * get_classname()  const   { return "ErrorCell"; }
};

#endif // __ERRORCELL_HH_DEFINED__
