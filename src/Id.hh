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

#ifndef __ID_HH_DEFINED__
#define __ID_HH_DEFINED__

#include <iostream>

#include "UTF8_string.hh"

class Function;
class Symbol;
class UCS_string;

/*!
 An Identifier for each internal object (primitives, Quad-symbols, and more).
 The ID can be derived in different ways:

 1. from the name of an ⎕AV element, e.g.  ID_F2_AND or ID_ASSIGN
 2. from a name,                     e.g.  ID_APL_VALUE or ID_CHARACTER
 3. from a distinguished var name,   e.g.  ID_Quad_AI or ID_Quad_AV
 4. from a distinguished fun name,   e.g.  ID_Quad_AT or ID_Quad_EM
 5. from a special token name              ID_L_PARENT1 or ID_R_PARENT1

  This is controlled by 5 corresponding macros: av() pp() qv() qf() resp. st()
 */

//-----------------------------------------------------------------------------
class ID
{
public:
   /// an ID. Every object known to APL (primitive, ⎕xx, ...) has one
   enum Id
   {
#define av(i, _u, v)          i v,
#define pp(i, _u, v)          i v,
#define qf(i, _u, v) Quad_ ## i v,
#define qv(i, _u, v) Quad_ ## i v,
#define sf(i, _u, v)          i v,
#define st(i, _u, v)          i v,

   #include "Id.def"
   };

   /// return the printable name for id
  static const char * name(Id id);

   /// If \b id is the ID of primitive function, primitive operator, or
   /// quad function, then return a pointer to it. Otherwise return 0.
   static Function * get_system_function(ID::Id id);

   /// If \b id is the ID of a quad variable, then return a pointer to its
   /// symbol. Otherwise return 0.
   static Symbol * get_system_variable(ID::Id id);

};
//-----------------------------------------------------------------------------

#endif // __ID_HH_DEFINED__

