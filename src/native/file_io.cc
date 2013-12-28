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

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "../Native_interface.hh"

   // CONVENTION: all functions must have an axis argument (like X
   // in A fun[X} B); the axis argument is a function number that selects
   // one of several functions provided by this library...
   //

//-----------------------------------------------------------------------------
/// a mandatory function that tells if the set of native functions defined
/// in this shared library returns a value (most likely it does)
bool
has_result()
{
   return true;
}
//-----------------------------------------------------------------------------
Token
eval_XB(const Value & X, const Value & B)
{
const APL_Float qct = Workspace::get_CT();
const int function_number = X.get_ravel(0).get_near_int(qct);

   switch(function_number)
      {
         case 0:   // return (last) errno
              {
                Value_P Z(Value_P(new Value(LOC)));
                new (&Z->get_ravel(0))   IntCell(errno);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 1:   // return strerror(B)
              {
                const int b = B.get_ravel(0).get_near_int(qct);
                const char * text = strerror(b);
                const int len = strlen(text);
                Value_P Z(new Value(len, LOC));
                loop(t, len)
                    new (&Z->get_ravel(t))   CharCell((Unicode)(text[t]));

                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }
      }

   CERR << "Bad eval_XB() function number: " << function_number << endl;
   DOMAIN_ERROR;
}
//-----------------------------------------------------------------------------
Token
eval_AXB(const Value & A, const Value & X, const Value & B)
{
const APL_Float qct = Workspace::get_CT();
const int function_number = X.get_ravel(0).get_near_int(qct);

   switch(function_number)
      {
      }

   CERR << "eval_AXB() function number: " << function_number << endl;
   DOMAIN_ERROR;
}
//-----------------------------------------------------------------------------

