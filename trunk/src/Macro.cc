/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2016  Dr. Jürgen Sauermann

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

#include <stdlib.h>

#include "Assert.hh"
#include "Macro.hh"

#define mac_def(n, _txt) Macro * Macro::n = 0;
#include "Macro.def"

Macro * Macro::all_macros[MAC_COUNT];

//-----------------------------------------------------------------------------
Macro::Macro(Macro_num num, const char * text)
   : UserFunction(UCS_string(UTF8_string(text)), LOC, "Macro::Macro()",
                  false, true),
     macro_number(num)
{
// CERR << "MACRO: " << endl << text;

   all_macros[macro_number] = this;

   if (error_info || (error_line != -1))   // something went wrong
      {
        CERR << endl << "*** Fatal error in macro #" << macro_number << endl;
        if (error_info)         CERR << "error_info: " << error_info << endl;
        if (error_line != -1)   CERR << "error_line: " << error_line << endl;
        exit(1);
      }
}
//-----------------------------------------------------------------------------
Macro::~Macro()
{
   Assert(0 && "~Macro() called");
}
//-----------------------------------------------------------------------------
void
Macro::init_macros()
{
#define mac_def(n, txt) n = new Macro(Macro::MAC_ ## n, txt);
#include "Macro.def"
}
//-----------------------------------------------------------------------------
Macro *
Macro::get_macro(Macro_num num)
{
   Assert(num >= 0);
   Assert(num < MAC_COUNT);
   return all_macros[num];
}
//-----------------------------------------------------------------------------
void
Macro::unmark_all_macros()
{
#define mac_def(n, _txt) n->unmark_all_values();
#include "Macro.def"
}
//-----------------------------------------------------------------------------

