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

#include "EOC_arg.hh"
#include "QuadFunction.hh"

ShapeItem EOC_arg::EOC_arg_count = 0;

//-----------------------------------------------------------------------------
EOC_arg::EOC_type
EOC_arg::get_EOC_type(EOC_HANDLER handler)
{
   if (handler == Quad_EA::eoc_A_and_B_done)    return EOC_Quad_EA_AB;
   if (handler == Quad_EA::eoc_B_done)          return EOC_Quad_EA_B;
   if (handler == Quad_EC::eoc)                 return EOC_Quad_EC;
   if (handler == Quad_INP::eoc_INP)            return EOC_Quad_INP;

   Assert(0 && "Bad EOC_handler");
   return EOC_None;
}
//-----------------------------------------------------------------------------
EOC_HANDLER
EOC_arg::get_EOC_handler(EOC_type type)
{
   switch(type)
      {
        case EOC_Quad_EA_AB: return Quad_EA::eoc_A_and_B_done;
        case EOC_Quad_EA_B:  return Quad_EA::eoc_B_done;
        case EOC_Quad_EC:    return Quad_EC::eoc;
        case EOC_Quad_INP:   return Quad_INP::eoc_INP;
        default:  Assert(0 && "Bad EOC_type");
      }

   return 0;
}
//-----------------------------------------------------------------------------
