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

#ifndef __REALCELL_HH_DEFINED__
#define __REALCELL_HH_DEFINED__

#include "NumericCell.hh"

/*! A cell that is either an integer cell or a floating point cell.
    This class contains all cell functions for which the detailed type
    makes no difference.
*/

class RealCell : public NumericCell
{
protected:
   /// overloaded Cell::is_real_cell().
   virtual bool is_real_cell() const { return true; }

   /// overloaded Cell::bif_circle_fun().
   virtual ErrorCode bif_circle_fun(Cell * Z, const Cell * A) const;

   /// overloaded Cell::bif_circle_fun_inverse().
   virtual ErrorCode bif_circle_fun_inverse(Cell * Z, const Cell * A) const;

   /// compute circle function \b fun
   ErrorCode do_bif_circle_fun(Cell * Z, int fun) const;

   /// overloaded Cell::bif_logarithm().
   virtual ErrorCode bif_logarithm(Cell * Z, const Cell * A) const;

   /// overloaded Cell::get_classname().
   virtual const char * get_classname() const   { return "RealCell"; }
};
//-----------------------------------------------------------------------------

#endif // __REALCELL_HH_DEFINED__
