/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2015  Dr. Dirk Laurie

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

#include <cstring>
#include <sstream>
#include <ostream>

#include <Workspace.hh>
#include <Command.hh>
#include <FloatCell.hh>
#include <ComplexCell.hh>
#include <PointerCell.hh>

#include "libapl.h"

using namespace std;

/******************************************************************************
   1. APL value constructor functions. The APL_value returned must be released
      with release_value() below at some point in time (unless it is 0)
 */

/// A new integer scalar.
APL_value
int_scalar(int64_t val, const char * loc)
{
Value_P Z(loc);
   new (Z->next_ravel()) IntCell(val);
   Value_P::increment_owner_count(Z.get(), loc);   // keep value
   return Z.get();
}
//-----------------------------------------------------------------------------
/// A new floating point scalar.
APL_value
double_scalar(double val, const char * loc)
{
Value_P Z(loc);
   new (Z->next_ravel()) FloatCell(val);
   Value_P::increment_owner_count(Z.get(), loc);   // keep value
   return Z.get();
}
//-----------------------------------------------------------------------------
/// A new complex scalar.
APL_value
complex_scalar(double real, double imag, const char * loc)
{
Value_P Z(loc);
   new (Z->next_ravel()) ComplexCell(real, imag);
   Value_P::increment_owner_count(Z.get(), loc);   // keep value
   return Z.get();
}
//-----------------------------------------------------------------------------
/// A new character scalar.
APL_value
char_scalar(int uni, const char * loc)
{
Value_P Z(loc);
   new (Z->next_ravel()) CharCell((Unicode)uni);
   Value_P::increment_owner_count(Z.get(), loc);   // keep value
   return Z.get();
}
//-----------------------------------------------------------------------------

/// A new APL value with given rank and shape. All ravel elements are
/// initialized to integer 0.
APL_value
apl_value(int rank, const int64_t * shape, const char * loc)
{
const Shape sh(rank, shape);
Value_P Z(sh, loc);

   if (Z->element_count())
      {
         while (Cell * cell = Z->next_ravel())   new (cell)   IntCell(0);
      }
   else
      {
        new (&Z->get_ravel(0))   IntCell(0);   // prototype
      }

   Z->check_value(LOC);
   Value_P::increment_owner_count(Z.get(), loc);   // keep value
   return Z.get();
}
//-----------------------------------------------------------------------------
/// A new character vector.
APL_value
char_vector(const char * str, const char * loc)
{
UTF8_string utf8(str);
UCS_string ucs(utf8);

Value_P Z(ucs, loc);
   Value_P::increment_owner_count(Z.get(), loc);   // keep value
   return Z.get();
}

/******************************************************************************
   2. APL value destructor function. All non-0 APL_values must be released
      at some point in time (even const ones). release_value(0) is not needed
      but accepted.
 */
void
release_value(const APL_value val, const char * loc)
{
Value * v = (Value *) val;
   if (val)   Value_P::decrement_owner_count(v, loc);
}


/******************************************************************************
   3. read access to APL values. All ravel indices count from ⎕IO←0.
 */

//-----------------------------------------------------------------------------
/// return ⍴⍴val
int
get_rank(const APL_value val)
{
   return val->get_rank();
}
//-----------------------------------------------------------------------------

/// return (⍴val)[axis]
int64_t
get_axis(const APL_value val, unsigned int axis)
{
   return axis < val->get_rank() ? val->get_shape_item(axis) : -1;
}
//-----------------------------------------------------------------------------

/// return ×/⍴val
uint64_t
get_element_count(const APL_value val)
{
   return val->element_count();
}
//-----------------------------------------------------------------------------
/// return the type of (,val)[idx]
int
get_type(const APL_value val, uint64_t idx)
{
   if (idx >= val->nz_element_count())   return 0;
   return val->get_ravel(idx).get_cell_type();
}
//-----------------------------------------------------------------------------
/// return non-0 if val is a simple character vector.
int is_string(const APL_value val)
{
   return val->is_char_vector();
}
//-----------------------------------------------------------------------------

/// return the character val[idx] (after having checked is_char())
int
get_char(const APL_value val, uint64_t idx)
{
   return val->get_ravel(idx).get_char_value();
}
//-----------------------------------------------------------------------------

/// return the integer val[idx] (after having checked is_int())
int64_t
get_int(const APL_value val, uint64_t idx)
{
   return val->get_ravel(idx).get_int_value();
}
//-----------------------------------------------------------------------------

/// return the real part of val[idx] (after having checked is_numeric())
double
get_real(const APL_value val, uint64_t idx)
{
   return val->get_ravel(idx).get_real_value();
}
//-----------------------------------------------------------------------------

/// return the imag part of val[idx] (after having checked is_numeric())
double
get_imag(const APL_value val, uint64_t idx)
{
   return val->get_ravel(idx).get_imag_value();
}
//-----------------------------------------------------------------------------

/// return the (nested) value val[idx] (after having checked is_value()).
/// The APL_value returned must be released with release_value() later on.
///
APL_value
get_value(const APL_value val, uint64_t idx)
{
Value_P sub = val->get_ravel(idx).get_pointer_value();
   Value_P::increment_owner_count(sub.get(), LOC);
   return sub.get();
}


/******************************************************************************
   4. write access to APL values. All ravel indices count from ⎕IO←0.
 */

//-----------------------------------------------------------------------------
/// val[idx]←unicode
void
set_char(int new_char, APL_value val, uint64_t idx)
{
Cell * cell = &val->get_ravel(idx);
   if (cell->is_pointer_cell())   cell->get_pointer_value().clear(LOC);

   new (cell)   CharCell((Unicode)new_char);
}
//-----------------------------------------------------------------------------

/// val[idx]←new_int
void
set_int(int64_t new_int, APL_value val, uint64_t idx)
{
Cell * cell = &val->get_ravel(idx);
   if (cell->is_pointer_cell())   cell->get_pointer_value().clear(LOC);

   new (cell)   IntCell(new_int);
}
//-----------------------------------------------------------------------------

/// val[idx]←new_double
void
set_double(double new_double, APL_value val, uint64_t idx)
{
Cell * cell = &val->get_ravel(idx);
   if (cell->is_pointer_cell())   cell->get_pointer_value().clear(LOC);

   new (cell)   FloatCell(new_double);
}
//-----------------------------------------------------------------------------

/// val[idx]←new_complex
void
set_complex(double new_real, double new_imag, APL_value val, uint64_t idx)
{
Cell * cell = &val->get_ravel(idx);
   if (cell->is_pointer_cell())   cell->get_pointer_value().clear(LOC);

   new (cell)   ComplexCell(new_real, new_imag);
}
//-----------------------------------------------------------------------------
void
set_value(APL_value new_value, APL_value val, uint64_t idx)
{
Cell * cell = &val->get_ravel(idx);
   if (cell->is_pointer_cell())   cell->get_pointer_value().clear(LOC);

   if (new_value->is_simple_scalar())   // e.g. ⊂5 is 5
      {
        cell->init(new_value->get_ravel(0), *val);
      }
   else if (new_value->is_scalar())     // e.g. ⊂⊂5 is ⊂5
      {
        const Cell & src = new_value->get_ravel(0);
        if (!src.is_pointer_cell())   DOMAIN_ERROR;
        new (cell)   PointerCell(src.get_pointer_value()->clone(LOC), *val);
      }
   else
      {
        new (cell)   PointerCell(new_value->clone(LOC), *val);
      }
}

/******************************************************************************
   5. other
 */

void
apl_exec(const char* line)
{ 
UTF8_string line_utf8(line);
UCS_string line_ucs(line_utf8);
  Command::process_line(line_ucs);
} 
//-----------------------------------------------------------------------------
const char *
apl_command(const char * command)
{
UTF8_string command_utf8(command);
UCS_string command_ucs(command_utf8);
stringstream out;

  Command::do_APL_command(out, command_ucs);

const string st = out.str();
  return strndup(st.data(), st.size());
}
//-----------------------------------------------------------------------------
APL_value
get_var_value(const char * var_name, const char * loc)
{
UTF8_string var_name_utf8(var_name);
UCS_string var_name_ucs(var_name_utf8);
Symbol * symbol = Workspace::lookup_existing_symbol(var_name_ucs);
   if (symbol == 0)                       return 0;   // unknown name
   if (symbol->get_nc() != NC_VARIABLE)   return 0;   // name is not a variable

Value_P Z = symbol->get_value();
   if (!Z)                              return 0;

   Value_P::increment_owner_count(Z.get(), loc);
   return Z.get();
}
//-----------------------------------------------------------------------------
int
set_var_value(const char * var_name, const APL_value new_value,
              const char * loc)
{
UTF8_string var_name_utf8(var_name);
UCS_string var_name_ucs(var_name_utf8);

   // check name...
   //
   if (!Avec::is_quad(var_name_ucs[0]) &&
       !Avec::is_first_symbol_char(var_name_ucs[0]))   return 1;

   loop(s, var_name_ucs.size())
      {
        if (!Avec::is_symbol_char(var_name_ucs[s]))   return 2;
      }

Symbol * symbol = Workspace::lookup_symbol(var_name_ucs);
   if (symbol == 0)   return 3;   // unknown name

   if (new_value == 0)   return 0;   // only test var_name

  if (symbol->get_nc() != NC_VARIABLE &&
      symbol->get_nc() != NC_UNUSED_USER_NAME)   return 4;

Value_P B(new_value, loc);
   symbol->assign(B, loc); 
   return 0;   // ok
}
//-----------------------------------------------------------------------------
void
print_value(const APL_value value, FILE * file)
{
stringstream out;
   value->print(out);

const string st = out.str();
   fwrite(st.data(), 1, st.size(), file);
}
//-----------------------------------------------------------------------------
char *
print_value_to_string(const APL_value value)
{
stringstream out;
   value->print(out);

const string st = out.str();
   return strndup(st.data(), st.size());
}
//-----------------------------------------------------------------------------
ostream &
print_value(const APL_value value, ostream & out)
{
   value->print(out);
   return out;
}
//-----------------------------------------------------------------------------
int
UTF8_to_Unicode(const char * utf, int * length)
{
int len = 0;
const Unicode uni = UTF8_string::toUni((const UTF8 *)utf, len);
   if (length)   *length = len;
   return uni;
}
//-----------------------------------------------------------------------------
void
Unicode_to_UTF8(int uni, char * dest)
{
UCS_string ucs((Unicode)uni);
UTF8_string utf8(ucs);
   memcpy(dest, utf8.get_items(), utf8.size());
   dest[utf8.size()] = 0;
}
//-----------------------------------------------------------------------------

