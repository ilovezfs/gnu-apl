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

#include "../config.h"          // for HAVE_ macros from configure
#include "UserPreferences.hh"   // for preferences and command line options

#if defined(__sun) && defined(__SVR4)
# define NOMACROS
#endif

// check if ncurses (or curses) is usable and #include the proper header file.
// Usable means that one of the header files ncurses.h or curses.h AND a
// curses library are available
//
#if HAVE_LIBCURSES
# if HAVE_NCURSES_H

# include <ncurses.h>
#define CURSES_USABLE 1

# elif HAVE_CURSES_H

# include <curses.h>
#define CURSES_USABLE 1

# endif
#endif

// Then check for term.h and #include it
//
#if HAVE_TERM_H
# include <term.h>
#else
# undef CURSES_USABLE
#endif

// curses on Solaris #defines erase() and tab() which conflicts with e.g.
// vector::erase() and others
//
#ifdef erase
# undef erase
#endif

#ifdef tab
# undef tab
#endif

#include "Command.hh"
#include "Common.hh"
#include "DiffOut.hh"
#include "InputFile.hh"
#include "main.hh"
#include "LineInput.hh"
#include "Output.hh"
#include "Performance.hh"
#include "PrintOperator.hh"
#include "Svar_DB.hh"

bool Output::use_curses = false;   // possibly overridden by uprefs
bool Output::keys_curses = false;   // possibly overridden by uprefs
bool Output::colors_enabled = false;
bool Output::colors_changed = false;

int Output::color_CIN_foreground = 0;
int Output::color_CIN_background = 7;
int Output::color_COUT_foreground = 0;
int Output::color_COUT_background = 8;
int Output::color_CERR_foreground = 5;
int Output::color_CERR_background = 8;
int Output::color_UERR_foreground = 5;
int Output::color_UERR_background = 8;

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

} CERR_filebuf;   ///< a filebuf for CERR

bool ErrOut::used = false;

DiffOut DOUT_filebuf(false);
DiffOut UERR_filebuf(true);

// Android defines its own CIN, COUT, CERR, and UERR ostreams
#ifndef WANT_ANDROID

CinOut CIN_filebuf;
CIN_ostream CIN;

ostream COUT(&DOUT_filebuf);
ostream CERR(CERR_filebuf.use());
ostream UERR(&UERR_filebuf);

#endif

extern ostream & get_CERR();
ostream & get_CERR()
{
   return ErrOut::used ? CERR : cerr;
};

Output::ColorMode Output::color_mode = COLM_UNDEF;

/// CSI sequence for ANSI/VT100 escape sequences (ESC [)
#define CSI "\x1B["

/// VT100 escape sequence to change to cin color
char Output::color_CIN[MAX_ESC_LEN] = CSI "0;30;47m";

/// VT100 escape sequence to change to cout color
char Output::color_COUT[MAX_ESC_LEN] = CSI "0;30;48;2;255;255;255m";

/// VT100 escape sequence to change to cerr color
char Output::color_CERR[MAX_ESC_LEN] = CSI "0;35;48;2;255;255;255m";

/// VT100 escape sequence to change to uerr color
char Output::color_UERR[MAX_ESC_LEN] = CSI "0;35;48;2;255;255;255m";

/// VT100 escape sequence to reset colors to their default
char Output::color_RESET[MAX_ESC_LEN] = CSI "0;30;48;2;255;255;255m";

/// VT100 escape sequence to clear to end of line
char Output::clear_EOL[MAX_ESC_LEN] = CSI "K";

/// VT100 escape sequence to clear to end of screen
char Output::clear_EOS[MAX_ESC_LEN] = CSI "J";

/// VT100 escape sequence to exit attribute mode
char Output::exit_attr_mode[MAX_ESC_LEN] = CSI "m" "\x1B"  "(B";

/// the ESC sequences sent by the cursor keys...
char Output::ESC_CursorUp   [MAX_ESC_LEN]   = CSI "A";    ///< Key ↑
char Output::ESC_CursorDown [MAX_ESC_LEN]   = CSI "B";    ///< Key ↓
char Output::ESC_CursorRight[MAX_ESC_LEN]   = CSI "C";    ///< Key →
char Output::ESC_CursorLeft [MAX_ESC_LEN]   = CSI "D";    ///< Key ←
char Output::ESC_CursorEnd  [MAX_ESC_LEN]   = CSI "F";    ///< Key End
char Output::ESC_CursorHome [MAX_ESC_LEN]   = CSI "D";    ///< Key Home
char Output::ESC_InsertMode [MAX_ESC_LEN]   = CSI "2~";   ///< Key Ins
char Output::ESC_Delete     [MAX_ESC_LEN]   = CSI "3~";   ///< Key Del

/// the ESC sequences sent by the cursor keys with SHIFT and/or CTRL...
/// the "\0" in the middle is a wildcard match for 0x32/0x35/0x36
char Output::ESC_CursorUp_1   [MAX_ESC_LEN] = CSI "1;" "\0" "A";   ///< Key ↑
char Output::ESC_CursorDown_1 [MAX_ESC_LEN] = CSI "1;" "\0" "B";   ///< Key ↓
char Output::ESC_CursorRight_1[MAX_ESC_LEN] = CSI "1;" "\0" "C";   ///< Key →
char Output::ESC_CursorLeft_1 [MAX_ESC_LEN] = CSI "1;" "\0" "D";   ///< Key ←
char Output::ESC_CursorEnd_1  [MAX_ESC_LEN] = CSI "1;" "\0" "F";   ///< Key End
char Output::ESC_CursorHome_1 [MAX_ESC_LEN] = CSI "1;" "\0" "D";   ///< Key Home
char Output::ESC_InsertMode_1 [MAX_ESC_LEN] = CSI "2;" "\0" "~";   ///< Key Ins
char Output::ESC_Delete_1     [MAX_ESC_LEN] = CSI "3;" "\0" "~";   ///< Key Del

//-----------------------------------------------------------------------------
int
CinOut::overflow(int c)
{
PERFORMANCE_START(cerr_perf)
   if (!InputFile::echo_current_file())   return 0;

   Output::set_color_mode(Output::COLM_INPUT);
   cerr << (char)c;
PERFORMANCE_END(fs_CERR_B, cerr_perf, 1)

   return 0;
}
//-----------------------------------------------------------------------------
int
ErrOut::overflow(int c)
{
PERFORMANCE_START(cerr_perf)

   Output::set_color_mode(Output::COLM_ERROR);
   cerr << (char)c;
PERFORMANCE_END(fs_CERR_B, cerr_perf, 1)

   return 0;
}
//-----------------------------------------------------------------------------
void
Output::init(bool logit)
{
   if (!isatty(fileno(stdout)))
      {
        cout.flush();
        cout.setf(ios::unitbuf);
      }

#if CURSES_USABLE

   if (logit)
      {
        if (use_curses)
           CERR << "initializing output ESC sequences from libcurses" << endl;
        else
           CERR << "using ANSI terminal output ESC sequences (or those "
                   "configured in your preferences file(s))" << endl;


        if (keys_curses)
           CERR << "initializing keyboard ESC sequences from libcurses" <<endl;
        else
           CERR << "using ANSI terminal input ESC sequences(or those "
                   "configured in your preferences file(s))" << endl;
      }

int errors = 0;

   if (use_curses)
      {
        const int ret = setupterm(0, STDOUT_FILENO, 0);
        if (ret != 0)   ++errors;

           // read some ESC sequences
           //
#define READ_SEQ(dest, app, cap, p1) \
   errors += read_ESC_sequence(dest, MAX_ESC_LEN, app, #cap, cap, p1);

        READ_SEQ(clear_EOL,      0, clr_eol, 0);
        READ_SEQ(clear_EOS,      0, clr_eos, 0);
        READ_SEQ(exit_attr_mode, 0, exit_attribute_mode, 0);

        READ_SEQ(color_CIN, 0, set_foreground, color_CIN_foreground);
        READ_SEQ(color_CIN, 1, set_background, color_CIN_background);

        READ_SEQ(color_COUT, 0, set_foreground, color_COUT_foreground);
        READ_SEQ(color_COUT, 1, set_background, color_COUT_background);

        READ_SEQ(color_CERR, 0, set_foreground, color_CERR_foreground);
        READ_SEQ(color_CERR, 1, set_background, color_CERR_background);

        READ_SEQ(color_UERR, 0, set_foreground, color_UERR_foreground);
        READ_SEQ(color_UERR, 1, set_background, color_UERR_background);
      }

   // cursor keys. This does not work currently because the keys reported
   // by: key_up, key_down, etc are different from the hardwired VT100 keys
   //
   // The other group: cursor_up, cursor_down, etc is closer but cursor_down
   // is linefeed
   //
   if (keys_curses)
      {
        READ_SEQ(ESC_CursorUp,    0, key_up,    0);
        READ_SEQ(ESC_CursorDown,  0, key_down,  0);
        READ_SEQ(ESC_CursorLeft,  0, key_left,  0);
        READ_SEQ(ESC_CursorRight, 0, key_right, 0);
        READ_SEQ(ESC_CursorEnd,   0, key_end,   0);
        READ_SEQ(ESC_CursorHome,  0, key_home,  0);
        READ_SEQ(ESC_InsertMode,  0, key_ic,    0);
        READ_SEQ(ESC_Delete,      0, key_dc,    0);

        ESCmap::refresh_lengths();
      }

   if (errors)
      {
        CERR <<
"\n*** use of libcurses was requested, but something went wrong during its\n"
"initialization. Expect garbled output and non-functional keys." << endl;
         use_curses = false;
      }
#endif
}
//-----------------------------------------------------------------------------
int
Output::read_ESC_sequence(char * dest, int destlen, int append, 
                          const char * capname, char * str, int p1)
{
#if CURSES_USABLE
   if (str == 0)
      {
        const char * term = getenv("TERM");
        CERR << "capability '" << capname
             << "' is not contained in the description";
        if (term)   CERR << " of terminal " << term;
        CERR << endl;
        return 1;
      }

   if (str == (char *)-1)
      {
        const char * term = getenv("TERM");
        CERR << "capability '" << capname 
             << "' is not a string capability";
        CERR << endl;
        if (term)   CERR << " of terminal " << term;
        return 1;
      }

// CERR << "BEFORE: ";
// for (int i = 0; i < strlen(dest); ++i)   CERR << " " << HEX2(dest[i]);
// CERR << endl;

   if (!append)   *dest = 0;

const int offset = strlen(dest);
const char * seq = tparm(str, p1, 0, 0, 0, 0, 0, 0, 0, 0);
const int seq_len = strlen(seq);

   if (seq_len + offset >= (destlen - 1))
      {
        CERR << "ESC sequence too long" << endl;
        return 1;
      }

   strncpy(dest + offset, seq, destlen - offset - 1);

//   CERR << "AFTER:  ";
//   for (int i = 0; i < strlen(dest); ++i)   CERR << " " << HEX2(dest[i]);
//   CERR << endl;

   return 0;

#else
   return 1;   // should not happen
#endif
}
//-----------------------------------------------------------------------------
void
Output::reset_dout()
{
   DOUT_filebuf.reset();
}
//-----------------------------------------------------------------------------
void
Output::reset_colors()
{
   if (!colors_changed)   return;

   if (use_curses)
      {
        cout << exit_attr_mode << clear_EOL;
        cerr << exit_attr_mode << clear_EOL;
      }
   else
      {
        cout << color_RESET << clear_EOL;
        cerr << color_RESET << clear_EOL;
      }
}
//-----------------------------------------------------------------------------
void
Output::set_color_mode(Output::ColorMode mode)
{
   if (!colors_enabled)      return;   // colors disabled
   if (color_mode == mode)   return;   // no change in color mode

   // mode changed
   //
   color_mode = mode;
   colors_changed = true;   // to reset them in reset_colors()

   switch(color_mode)
      {
        case COLM_INPUT:  cerr << color_CIN  << clear_EOL;   break;

        case COLM_OUTPUT:
             if (use_curses)   cout << exit_attr_mode << clear_EOL;
             else              cout << color_COUT << clear_EOL;
             break;

        case COLM_ERROR:  cerr << color_CERR << clear_EOL;   break;

        case COLM_UERROR: cout << color_UERR << clear_EOL;   break;

        default: break;
      }
}
//-----------------------------------------------------------------------------
void 
Output::toggle_color(const UCS_string & arg)
{
   if (arg.starts_iwith("ON"))         colors_enabled = true;
   else if (arg.starts_iwith("OFF"))   colors_enabled = false;
   else                                colors_enabled = !colors_enabled;
}
//-----------------------------------------------------------------------------
bool
Output::color_enabled()
{
   return Output::colors_enabled;
}
//=============================================================================
void
CIN_ostream::set_cursor(int y, int x)
{
   if (uprefs.raw_cin)   return;

#if CURSES_USABLE
   if (Output::use_curses)
      {
        if (y < 0)
           {
             // y < 0 means from bottom upwards
             //
             *this << CSI << "30;" << (1 + x) << "H" << CSI << "99B";
             if (y < -1)   *this <<tparm(parm_down_cursor,
                                         -(y + 1), 0, 0, 0, 0, 0, 0, 0, 0);
             *this << std::flush;
           }
        else
           {
             // y ≥ 0 is from top downwards. This is currently not used.
             //
             *this << tparm(cursor_address, y, x, 0, 0, 0, 0, 0, 0, 0)
                   << std::flush;
           }
      }
   else
#endif
      {
        if (y < 0)
           {
             // y < 0 means from bottom upwards
             //
             *this << CSI << "30;" << (1 + x) << 'H'
                   << CSI << "99B" << std::flush;
             if (y < -1)   *this << CSI << (-(y + 1)) << "A";
           }
        else
           {
             // y ≥ 0 is from top downwards. This is currently not used.
             *this << CSI << (1 + y) << ";" << (1 + x) << 'H' << std::flush;
           }
      }
}
//-----------------------------------------------------------------------------
