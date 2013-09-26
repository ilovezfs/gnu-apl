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

#include "Avec.hh"
#include "Common.hh"
#include "Id.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include "QuadFunction.hh"
#include "Quad_SVx.hh"
#include "Quad_TF.hh"
#include "SkalarFunction.hh"
#include "UCS_string.hh"
#include "Workspace.hh"

// a number of fixed UCS strings, one for each Id.
//
#define av(x, u) const UCS_string id_ ## x (UNI_ ## u);
#define pp(x, u) const UCS_string id_ ## x (UTF8_string(#x));
#define qf(x, u) const UCS_string id_QUAD_ ## x (UTF8_string("\xe2" "\x8e" "\x95" #x));
#define qv(x, u) const UCS_string id_QUAD_ ## x (UTF8_string("\xe2" "\x8e" "\x95" #x));
#define st(x, u) const UCS_string id_ ## x (UTF8_string(u));

#define id_def(_id, _uni, _val, _mac)   _mac(_id, _uni)
#include "Id.def"

//-----------------------------------------------------------------------------
ostream &
operator << (ostream & out, Id id)
{
   return out << id_name(id);
}
//-----------------------------------------------------------------------------

const UCS_string &
id_name(Id id)
{
   switch(id)
      {
#define av(x) case ID_ ## x: return id_ ## x;
#define pp(x) case ID_ ## x: return id_ ## x;
#define qf(x) case ID_QUAD_ ## x: return id_QUAD_ ## x;
#define qv(x) case ID_QUAD_ ## x: return id_QUAD_ ## x;
#define st(x) case ID_ ## x: return id_ ## x;

#define id_def(_id, _uni, _val, _mac) _mac(_id)
#include "Id.def"
      }

   CERR << "Unknown Id " << HEX(id);
   Assert(0 && "Bad Id");
}
//-----------------------------------------------------------------------------
Function *
get_system_function(Id id)
{
   switch(id)
      {
#define av(x) case ID_ ## x: return &Bif_F12_ROLL::fun;
#define pp(x)
#define qf(x) case ID_QUAD_ ## x: return &Quad_ ## x::fun;
#define qv(x)
#define st(x) 

#define id_def(_id, _uni, _val, _mac) _mac(_id)
#include "Id.def"
      }

   return 0;
}
//-----------------------------------------------------------------------------
Symbol *
get_system_variable(Id id)
{
   switch(id)
      {
#define av(x)
#define pp(x)
#define qf(x)
#define qv(x) case ID_QUAD_ ## x:return &Workspace::the_workspace->v_quad_ ## x;
#define st(x) 

#define id_def(_id, _uni, _val, _mac) _mac(_id)
#include "Id.def"
      }

   return 0;
}
//-----------------------------------------------------------------------------
