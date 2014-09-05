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

#ifndef __LINEINPUT_HH_DEFINED__
#define __LINEINPUT_HH_DEFINED__

#include <termios.h>

#include "Output.hh"
#include "PrintOperator.hh"
#include "UCS_string.hh"

//-----------------------------------------------------------------------------
/// kind of input
enum LineInputMode
{
   LIM_ImmediateExecution,
   LIM_Quad_Quad,
   LIM_Quad_INP,
   LIM_Quote_Quad,
   LIM_Nabla,
};
//-----------------------------------------------------------------------------
/// the lines that the user has previously entered
class LineHistory
{
public:
   /// constuctor: empty line history
   LineHistory(int maxl)
   : current_line(0),
     put(0),
     max_lines(maxl)
   { UCS_string u("xxx");   add_line(u); }

   /// read history from file
   void read_history(const char * filename);

   /// save history to file
   void save_history(const char * filename);

   /// clear history
   void clear_history(ostream & out);

   /// print history to \b out
   void print_history(ostream & out);

   /// start a new up/down sequence
   void next()
      { current_line = put;
        if (current_line < 0)   current_line += lines.size();    // wrap
        if (current_line >= lines.size())   current_line = 0;    // wrap
      }

   /// move to next older entry
   const UCS_string * up();

   /// move to next newer entry
   const UCS_string * down();

   /// add one line to \b this history
   void add_line(const UCS_string & line);

protected:
   /// the current line (controlled by up()/down())
   int current_line;

   /// the oldest line
   int put;

   /// the max. history size
   const int max_lines;

   vector<UCS_string> lines;
};
//-----------------------------------------------------------------------------
/// a context for one user-input line
class LineEditContext
{
public:
   LineEditContext(LineInputMode mode, int rows, int cols, LineHistory & hist,
                   const UCS_string & prmt);

   /// clear (after ^C)
   void clear()
      { user_line.clear(); }

   /// return the number of screen rows
   int get_screen_rows() const
      { return screen_rows; } 

   /// return the number of screen columns
   int get_screen_cols() const
      { return screen_cols; } 

   /// total length (prompt + user_line)
   int get_total_length() const
      { return prompt.size() + user_line.size(); } 

   /// return true if prompt + offset is on column 0
   bool on_bad_col(int offset) const
      { const int  col = (prompt.size() + offset) % screen_cols;
        return col == 0 || col == (screen_cols - 1); }

   /// refresh cursor if its position may be wrong (due to forward or
   /// backward wraparound being enabled or not)
   void refresh_wrapped_cursor()
      { if (on_bad_col(uidx))   set_cursor(); }

   void move_idx(int new_idx)
      { uidx = new_idx;   set_cursor(); }

   void set_cursor()
      { const int offs = uidx + prompt.size();
        CIN.set_cursor(offs/screen_cols - allocated_height, offs%screen_cols);
      }

   /// adjust (increment) the allocated height (long lines)
   void adjust_allocated_height();

   /// refresh screen from cursor onwards
   void refresh_from_cursor();

   /// refresh screen from prompt (including) onwards
   void refresh_all();

   /// insert uni at cursor position
   void insert_char(Unicode uni);

   /// delete char at cursor position
   void delete_char();

   void toggle_insert_mode()   { insert_mode = !insert_mode; }

   /// move cursor home
   void cursor_home()   { move_idx(uidx = 0); }

   /// move cursor to end
   void cursor_end()   { move_idx(uidx = user_line.size()); }

   /// move cursor left
   void cursor_left()   { if (uidx > 0)   move_idx(--uidx); }

   /// move cursor right
   void cursor_right()   { if (uidx < user_line.size())   move_idx(++uidx); }

   /// move cursor left and delete char
   void backspace()
      { if (uidx > 0)   { move_idx(--uidx);   delete_char(); } }

   /// tab expansion
   void tab(LineInputMode mode);

   /// move backwards in history
   void cursor_up();

   /// move forward in history
   void cursor_down();

   /// return current user input
   const UCS_string & get_user_line() const
      { return user_line; }

protected:
   /// the number of screen rows
   int screen_rows;

   /// the number of screen columns
   int screen_cols;

   /// the number of screen rows used for editing
   int allocated_height;

   /// the prompt
   UCS_string prompt;

   /// current offset into user_line
   int uidx;

   bool insert_mode;

   /// the line being edited
   UCS_string user_line;

   /// the line history
   LineHistory history;

   /// true if history was entered
   bool history_entered;

   UCS_string user_line_before_history;
};
//-----------------------------------------------------------------------------
/** InputMux fetches one line from either an input file, or interactively
   from the user if no input file is present
 **/
class InputMux
{
public:
   static void get_line(LineInputMode mode, const UCS_string * prompt,
                        UCS_string & line, bool & eof);
};
//-----------------------------------------------------------------------------
class LineInput
{
public:
   /// get a line from the user
   static void get_terminal_line(LineInputMode mode, const UCS_string * prompt,
                                 UCS_string & line, bool & eof);

   /// get a line from from user
   static void no_readline(LineInputMode mode, const UCS_string & prompt,
                           UCS_string & user_line, bool & eof);

   static void init(bool do_read_history)
      { the_line_input = new LineInput(do_read_history); }

   static void close()
      { delete the_line_input;   the_line_input = 0; }

   /// clear history
   static void clear_history(ostream & out)
      {  the_line_input->history.clear_history(out); }

   /// print history to \b out
   static void print_history(ostream & out)
      {  the_line_input->history.print_history(out); }

protected:
   LineInput(bool do_read_history);

   ~LineInput();

   LineHistory history;

   /// the stdin termios at startup of the interpreter. Will be restored
   /// when the interpreter exits.
   struct termios initial_termios;

   /// the current stdin termios.
   struct termios current_termios;

   /// the first tcgetattr() errno (or 0 if none)
   int initial_termios_errno;

   /// get one character from user
   static Unicode get_uni();

   /// single LineInput instance that restores stdin termios on destruction
   static LineInput * the_line_input;
};
//-----------------------------------------------------------------------------
struct ESCmap
{
   /// return true if seq is a true prefix of \b this ESCmap
   bool has_prefix(const char * seq, int seq_len) const;

   /// return true if seq is the sequence of \b this ESCmap
   bool is_equal(const char * seq, int seq_len) const;

   /// the length of \b seqence
   int len;

   /// the escape sequence (including ESC, not 0-terminated!)
   const char * seqence;

   /// the (pseudo-) Unicode for the sequence
   Unicode uni;

   /// max. length of an ESC sequence sent by a keyboard
   enum { MAX_ESC_LEN = 20 };

   /// default ESC sequence for Cursor Up key
   static const char ESC_CursorUp   [MAX_ESC_LEN];

   /// default ESC sequence for Cursor Down key
   static const char ESC_CursorDown [MAX_ESC_LEN];

   /// default ESC sequence for Cursor Right key
   static const char ESC_CursorRight[MAX_ESC_LEN];

   /// default ESC sequence for Cursor Left key
   static const char ESC_CursorLeft [MAX_ESC_LEN];

   /// default ESC sequence for End key
   static const char ESC_CursorEnd  [MAX_ESC_LEN];

   /// default ESC sequence for Home key
   static const char ESC_CursorHome [MAX_ESC_LEN];

   /// default ESC sequence for Insert key
   static const char ESC_InsertMode [MAX_ESC_LEN];

   /// default ESC sequence for Delete key
   static const char ESC_Delete     [MAX_ESC_LEN];

   /// a mapping from keyboard escape sequences to Unicodes
   static ESCmap the_ESCmap[];
};
//-----------------------------------------------------------------------------

#endif // __LINEINPUT_HH_DEFINED__
