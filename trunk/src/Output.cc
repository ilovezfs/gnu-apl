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

#include "Command.hh"
#include "Common.hh"
#include "DiffOut.hh"
#include "main.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include "Svar_DB.hh"
#include "TestFiles.hh"

bool Output::colors_enabled = false;
bool Output::print_sema_held = false;

/// a filebuf for stdin echo
class CinOut : public filebuf
{
   /// overloaded filebuf::overflow
   virtual int overflow(int c);
} cin_filebuf;

/// a filebuf for stderr output
class ErrOut : public filebuf
{
protected:
   /// overloaded filebuf::overflow()
   virtual int overflow(int c);

public:
   /** a helper function telling whether the constructor for CERR was called
       if CERR is used before its constructor was called (which can happen in
       when constructors of static objects are called and use CERR) then a
       segmentation fault would occur.

       We avoid that by using get_CERR() instead of CERR in such constructors.
       get_CERR() checks \b used and returns cerr instead of CERR if it is
       false.
    **/
   filebuf * use()   { used = true;   return this; }

   /// true iff the constructor for CERR was called
   static bool used;   // set when CERR is constructed

} cerr_filebuf;   ///< a filebuf for CERR

bool ErrOut::used = false;

DiffOut dout_filebuf;

ostream CIN(&cin_filebuf);
ostream CERR(cerr_filebuf.use());
ostream COUT(&dout_filebuf);

extern ostream & get_CERR();
ostream & get_CERR()
{
   return ErrOut::used ? CERR : cerr;
};

Output::ColorMode Output::color_mode = COLM_UNDEF;

/// VT100 escape sequence to turn cin color on
const char color_cin[]  = { CIN_COLOR_WANTED, 0 };   // from config.h

/// VT100 escape sequence to turn cout color on
const char color_cout[]  = { COUT_COLOR_WANTED, 0 };   // from config.h

/// VT100 escape sequence to turn cerr color on
const char color_cerr[] = { CERR_COLOR_WANTED, 0 };   // from config.h

/// VT100 escape sequence to reset colors to their default
const char reset_colors[] = { RESET_COLORS_WANTED, 0 };   // from config.h

/// VT100 escape sequence to clear to end of line
const char clear_eol[] = { CLEAR_EOL_WANTED, 0 };   // from config.h

//-----------------------------------------------------------------------------
int
CinOut::overflow(int c)
{
   if (do_not_echo)   return 0;
   if (!Output::print_sema_held)
      {
        Svar_DB::start_print(LOC);
        Output::print_sema_held = true;
      }

   Output::set_color_mode(Output::COLM_INPUT);
   cerr << (char)c;

   if (c == 0x0A)
      {
        if (Output::print_sema_held)
           {
             Svar_DB::end_print(LOC);
             Output::print_sema_held = false;
           }
      }

   return 0;
}
//-----------------------------------------------------------------------------
int
ErrOut::overflow(int c)
{
   if (!Output::print_sema_held)
      {
        Svar_DB::start_print(LOC);
        Output::print_sema_held = true;
      }

   Output::set_color_mode(Output::COLM_ERROR);
   cerr << (char)c;

   if (c == '\n')
      {
        Svar_DB::end_print(LOC);
        Output::print_sema_held = false;
      }

   return 0;
}
//-----------------------------------------------------------------------------
void
Output::reset_dout()
{
   dout_filebuf.reset();
}
//-----------------------------------------------------------------------------
void
Output::reset_colors()
{
   if (!colors_enabled)   return;

   cout << reset_colors << clear_eol;
   cerr << reset_colors << clear_eol;
}
//-----------------------------------------------------------------------------
void
Output::set_color_mode(Output::ColorMode mode)
{
   if (!colors_enabled)   return;

   if (color_mode != mode)   // mode changed
      {
        switch(color_mode = mode)
           {
             case COLM_INPUT:  cout << color_cin  << clear_eol;   break;
             case COLM_OUTPUT: cout << color_cout << clear_eol;   break;
             case COLM_ERROR:  cerr << color_cerr << clear_eol;   break;
           }
      }
}
//-----------------------------------------------------------------------------
void 
Output::toggle_color(const UCS_string & arg)
{
int a = 0;
   while (a < arg.size() && arg[a] < UNI_ASCII_SPACE)   ++a;

   if (arg.starts_iwith("ON"))         colors_enabled = true;
   else if (arg.starts_iwith("OFF"))   colors_enabled = false;
   else                                colors_enabled = !colors_enabled;
}
//-----------------------------------------------------------------------------
