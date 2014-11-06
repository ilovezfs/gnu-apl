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
#include "Bif_F12_FORMAT.hh"
#include "Bif_F12_SORT.hh"
#include "Bif_OPER1_COMMUTE.hh"
#include "Bif_OPER1_EACH.hh"
#include "Bif_OPER1_REDUCE.hh"
#include "Bif_OPER1_SCAN.hh"
#include "Bif_OPER2_INNER.hh"
#include "Bif_OPER2_OUTER.hh"
#include "Bif_OPER2_POWER.hh"
#include "Bif_OPER2_RANK.hh"
#include "Common.hh"
#include "Id.hh"
#include "Output.hh"
#include "PrimitiveFunction.hh"
#include "PrintOperator.hh"
#include "QuadFunction.hh"
#include "Quad_FX.hh"
#include "Quad_SVx.hh"
#include "Quad_TF.hh"
#include "ScalarFunction.hh"
#include "UCS_string.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
const char *
ID::name(ID::Id id)
{
   switch(id)
      {
#define pp(i, _u, _v) case i:          return #i;
#define qf(i,  u, _v)  case Quad_ ## i: return u;
#define qv(i,  u, _v)  case Quad_ ## i: return u;
#define sf(i,  u, _v)  case i:          return u;
#define st(i,  u, _v)  case i:          return u;

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
#define pp(i, _u, _v)
#define qf(i, _u, _v) case ID::Quad_ ## i: return &Quad_ ## i::fun;
#define qv(i, _u, _v)
#define sf(i, _u, _v) case ID:: i:        return &Bif_ ## i::fun;
#define st(i, _u, _v)

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
#define pp(_i, _u, _v)
#define qf(_i, _u, _v)
#define qv( i, _u, _v)   case ID::Quad_ ## i: \
                              return &Workspace::get_v_Quad_ ## i();
#define sf(_i, _u, _v) 
#define st(_i, _u, _v) 

#include "Id.def"

        default: break;
      }

   return 0;
}
//-----------------------------------------------------------------------------
