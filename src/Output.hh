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

#ifndef __OUTPUT_HH_DEFINED__
#define __OUTPUT_HH_DEFINED__

#include <stdio.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>

#include "Assert.hh"
#include "UCS_string.hh"

using namespace std;

/*
 The classes below are used to combine normal user I/O and automatic
 testcase execution. It works like this:

       (cin)
         |
         |
         V
     +---+---+             +-------------------+
     | Input | <--------   | testcase files(s) |
     +---+---+             +---------+---------+
         |                           |
         |                           |
         V                           |
      +--+--+                        |
      | APL |                        |
      +--+--+                        |
         |                           |
         |                           |
         +-----------------------+   |
         |                       |   |
         |                       V   V
        |                    +--+---+--+
         |                    | compare |
         |                    +----+----+
         |                         |
         |                         |
         V                         V
       (cout)                (test results)
 
 */

/** the output of the APL interpreter to cout and maybe to a test result file
 **/
class Output
{
public:
   /// the color to be usedd for output
   enum ColorMode
      {
         COLM_UNDEF,    ///< color undefined
         COLM_INPUT,    ///< color for (echo of) input characters (CIN)
         COLM_OUTPUT,   ///< color for normal APL output (COUT)
         COLM_ERROR,    ///< color for debug output (CERR)
         COLM_UERROR,   ///< color for APL error output (UERR)
      };

   enum { MAX_ESC_LEN = 100 };   ///< max. length of an ESC sequence

   /// set the color mode (if colors_enabled). Outputs the escape sequence
   /// for \b mode when the color mode changes
   static void set_color_mode(ColorMode mode);

   /// reset() dout_filebuf
   static void reset_dout();

   /// reset colors to black and white
   static void reset_colors();

   /// set or toggle color mode (implementation of command ]XTERM)
   static void toggle_color(const UCS_string & arg);

   /// initialize curses library
   static void  init(bool logit);

   /// escape sequence for CIN colors
   static char color_CIN[MAX_ESC_LEN];

   /// escape sequence for COUT colors
   static char color_COUT[MAX_ESC_LEN];

   /// escape sequence for CERR colors
   static char color_CERR[MAX_ESC_LEN];

   /// escape sequence for UERR colors
   static char color_UERR[MAX_ESC_LEN];

   /// escape sequence for resetting colors
   static char color_RESET[MAX_ESC_LEN];

   /// foreground color for CIN
   static int color_CIN_foreground;

   /// background color for CIN
   static int color_CIN_background;
 
   /// foreground color for COUT
   static int color_COUT_foreground;

   /// background color for COUT
   static int color_COUT_background;
 
   /// foreground color for CERR
   static int color_CERR_foreground;

   /// background color for CERR
   static int color_CERR_background;
 
   /// foreground color for UERR
   static int color_UERR_foreground;

   /// background color for UERR
   static int color_UERR_background;
 
   /// escape sequence for clear to end of line
   static char clear_EOL[MAX_ESC_LEN];

   /// escape sequence for clear to end of screen
   static char clear_EOS[MAX_ESC_LEN];

   /// default ESC sequence for Cursor Up key
   static char ESC_CursorUp[MAX_ESC_LEN];

   /// default ESC sequence for Cursor Down key
   static char ESC_CursorDown[MAX_ESC_LEN];

   /// default ESC sequence for Cursor Right key
   static char ESC_CursorRight[MAX_ESC_LEN];

   /// default ESC sequence for Cursor Left key
   static char ESC_CursorLeft[MAX_ESC_LEN];

   /// default ESC sequence for End key
   static char ESC_CursorEnd[MAX_ESC_LEN];

   /// default ESC sequence for Home key
   static char ESC_CursorHome[MAX_ESC_LEN];

   /// default ESC sequence for Insert key
   static char ESC_InsertMode[MAX_ESC_LEN];

   /// default ESC sequence for Delete key
   static char ESC_Delete[MAX_ESC_LEN];

   /// escape sequence for exiting attribute mode
   static char exit_attr_mode[MAX_ESC_LEN];

   /// true if curses shall be used for output ESC sequences
   static bool use_curses;

   /// true if curses shall be used for input ESC sequences
   static bool keys_curses;

   /// read/append an ESC sequence in str and store it in \b dest
   static int read_ESC_sequence(char * dest, int destlen, int append,
                                const char * capname, char * str, int p1);
protected:
   /// true if colors were changed (and then reset_colors() shall reset
   /// them when leaving the interpreter
   static bool colors_changed;

   /// the current color mode
   static ColorMode color_mode;

   /// true if colors are currently enabled (by XTERM command)
   static bool colors_enabled;
};

/// a filebuf for stdin echo
class CinOut : public filebuf
{
   /// overloaded filebuf::overflow
   virtual int overflow(int c);
};
extern CinOut CIN_filebuf;

/// an ostream for stdin echo and a few editing capabilities
class CIN_ostream : public ostream
{
public:
   CIN_ostream()
   : ostream(&CIN_filebuf)
   {}

   /// set cursor to y:x (upper left corner is 0:0, negative y: from bottom)
   void set_cursor(int y, int x);

   /// clear to end of line
   ostream & clear_EOL()
      { return *this << Output::clear_EOL; }

   /// clear to end of screen supported ?
   bool can_clear_EOS() const
      { return *Output::clear_EOS != 0; }

   /// clear to end of line
   ostream & clear_EOS()
      { return *this << Output::clear_EOS; }
};
extern CIN_ostream CIN;

#endif // __OUTPUT_HH_DEFINED__
