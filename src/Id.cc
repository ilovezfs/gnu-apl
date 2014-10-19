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

#include "Avec.hh"
#include "Common.hh"
#include "Id.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include "QuadFunction.hh"
#include "Quad_FX.hh"
#include "Quad_SVx.hh"
#include "Quad_TF.hh"
#include "ScalarFunction.hh"
#include "UCS_string.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
#define pp(x, _u) const UCS_string id_ ## x (UTF8_string(#x));
#define qf(x, _u) const UCS_string id_Quad_ ## x (UTF8_string("\xe2" "\x8e" "\x95" #x));
#define qv(x, _u) const UCS_string id_Quad_ ## x (UTF8_string("\xe2" "\x8e" "\x95" #x));
#define st(x, u) const UCS_string id_ ## x (UTF8_string(u));

#define id_def(id, uni, _val, mac)   mac(id, uni)
#include "Id.def"

//-----------------------------------------------------------------------------
const char *
ID::name(ID::Id id)
{
   switch(id)
      {
#define pp(x, _u) case x:          return #x;
#define qf(x, _u) case Quad_ ## x: return "\xe2" "\x8e" "\x95" #x;
#define qv(x, _u) case Quad_ ## x: return "\xe2" "\x8e" "\x95" #x;
#define st(x, u)  case x:          return u;

#define id_def(id, uni, _val, mac)   mac(id, uni)
#include "Id.def"

        default: break;
      }

   return "Unknown-ID";
}
//-----------------------------------------------------------------------------
ostream &
operator << (ostream & out, ID::Id id)
{
   return out << ID::name(id);
}
//-----------------------------------------------------------------------------
Function *
ID::get_system_function(ID::Id id)
{
   switch(id)
      {
#define pp(x)
#define qf(x) case ID::Quad_ ## x: return &Quad_ ## x::fun;
#define qv(x)
#define st(x) 

#define id_def(_id, _uni, _val, _mac) _mac(_id)
#include "Id.def"

        default: break;
      }

   return 0;
}
//-----------------------------------------------------------------------------
Symbol *
ID::get_system_variable(ID::Id id)
{
   switch(id)
      {
#define pp(x)
#define qf(x)
#define qv(x) case ID::Quad_ ## x:return &Workspace::get_v_Quad_ ## x();
#define st(x) 

#define id_def(_id, _uni, _val, _mac) _mac(_id)
#include "Id.def"

        default: break;
      }

   return 0;
}
//-----------------------------------------------------------------------------
