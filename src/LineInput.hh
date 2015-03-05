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

class Nabla;

//-----------------------------------------------------------------------------
/// kind of input
enum LineInputMode
{
   LIM_ImmediateExecution,
   LIM_Quote_Quad,
   LIM_Quad_Quad,
   LIM_Quad_INP,
   LIM_Nabla,
};
//-----------------------------------------------------------------------------
/// the lines that the user has previously entered
class LineHistory
{
public:
   /// constructor: empty line history
   LineHistory(int maxl);

   /// constructor: line history from function being ∇-edited
   LineHistory(const Nabla & nabla);

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
        if (current_line < 0)   current_line += hist_lines.size();    // wrap
        if (current_line >= hist_lines.size())   current_line = 0;    // wrap
      }

   /// move to next older entry
   const UCS_string * up();

   /// move to next newer entry
   const UCS_string * down();

   /// add one line to \b this history
   void add_line(const UCS_string & line);

   /// replace the last line of \b this history
   void replace_line(const UCS_string & line);

   /// the history for ⍞
   static LineHistory quote_quad_history;

   /// the history for ⎕
   static LineHistory quad_quad_history;

   /// the history for ⎕INP
   static LineHistory quad_INP_history;

protected:
   /// the current line (controlled by up()/down())
   int current_line;

   /// the oldest line
   int put;

   /// the max. history size
   const int max_lines;

   /// the history
   vector<UCS_string> hist_lines;
};
//-----------------------------------------------------------------------------
/// a context for one user-input line
class LineEditContext
{
public:
   /// constructor
   LineEditContext(LineInputMode mode, int rows, int cols,
                   LineHistory & hist, const UCS_string & prmt);

   /// destructor
   ~LineEditContext();

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

   /// move the cursor
   void move_idx(int new_idx)
      { uidx = new_idx;   set_cursor(); }

   /// set the cursor (writing the appropriate ESC sequence to CIN)
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

   /// delete char at cursor position
   void delete_char();

   /// insert uni at cursor position
   void insert_char(Unicode uni);

   /// toggle the insert mode
   void toggle_ins_mode();

   /// cut from cursor to end of line
   void cut_to_EOL();

   /// paste to cursor position
   void paste();

   /// move cursor home
   void cursor_HOME()   { move_idx(uidx = 0); }

   /// move cursor to end
   void cursor_END()   { move_idx(uidx = user_line.size()); }

   /// move cursor left
   void cursor_LEFT()   { if (uidx > 0)   move_idx(--uidx); }

   /// move cursor right
   void cursor_RIGHT()   { if (uidx < user_line.size())   move_idx(++uidx); }

   /// move cursor left and delete char
   void backspc()
      { if (uidx > 0)   { move_idx(--uidx);   delete_char(); } }

   /// tab expansion
   void tab_expansion(LineInputMode mode);

   /// move backwards in history
   void cursor_UP();

   /// move forward in history
   void cursor_DOWN();

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

   /// true if input is in insert mode (as opposed to replace mode)
   bool ins_mode;

   /// the line being edited
   UCS_string user_line;

   /// the line history
   LineHistory history;

   /// true if history was entered
   bool history_entered;

   /// dito
   UCS_string user_line_before_history;

   /// a biffer for ^K/^Y
   static UCS_string cut_buffer;
};
//-----------------------------------------------------------------------------
/** InputMux fetches one line from either an input file, or interactively
   from the user if no input file is present
 **/
class InputMux
{
public:
   /// get one line
   static void get_line(LineInputMode mode, const UCS_string & prompt,
                        UCS_string & line, bool & eof, LineHistory & hist);
};
//-----------------------------------------------------------------------------
/// a class for obtaining one line of input from the user (editable)
class LineInput
{
public:
   /// get a line from the user
   static void get_terminal_line(LineInputMode mode, const UCS_string & prompt,
                                 UCS_string & line, bool & eof,
                                 LineHistory & hist);

   /// get a line from from user
   static void edit_line(LineInputMode mode, const UCS_string & prompt,
                         UCS_string & user_line, bool & eof,
                         LineHistory & hist);

   /// initialize the input subsystem
   static void init(bool do_read_history);

   /// close this line input, maybe updating the history
   static void close(bool do_not_write_hist)
      { if (the_line_input && do_not_write_hist)
            the_line_input->write_history = false;
        delete the_line_input;   the_line_input = 0; }

   /// clear history
   static void clear_history(ostream & out)
      {  the_line_input->history.clear_history(out); }

   /// print history to \b out
   static void print_history(ostream & out)
      {  the_line_input->history.print_history(out); }

   /// add a line to the history
   static void add_history_line(const UCS_string & line)
      {  the_line_input->history.add_line(line); }

   /// replace the last line of the history
   static void replace_history_line(const UCS_string & line)
      {  the_line_input->history.replace_line(line); }

   /// return the history
   static LineHistory & get_history()
      { return the_line_input->history; }

protected:
   /// constructor
   LineInput(bool do_read_history);

   /// destructor
   ~LineInput();

   /// lines previously entered
   LineHistory history;

   /// the stdin termios at startup of the interpreter. Will be restored
   /// when the interpreter exits.
   struct termios initial_termios;

   /// the current stdin termios.
   struct termios current_termios;

   /// the first tcgetattr() errno (or 0 if none)
   int initial_termios_errno;

   /// write history when done
   bool write_history;

   /// get one character from user
   static Unicode get_uni();

   /// single LineInput instance that restores stdin termios on destruction
   static LineInput * the_line_input;
};
//-----------------------------------------------------------------------------
/// A mapping from ESC sequences to (internal) pseudo-Unicodes such as
/// UNI_CursorUp and friends
struct ESCmap
{
   /// return true if seq is a true prefix of \b this ESCmap
   bool has_prefix(const char * seq, int seq_len) const;

   /// return true if seq is the sequence of \b this ESCmap
   bool is_equal(const char * seq, int seq_len) const;

   /// refresh the lengths (after keyboard strings have been updated)
   static void refresh_lengths();

   /// return true if an entry has prefix \b seq of length len
   static bool need_more(const char * seq, int len);

   /// the length of \b seqence
   int len;

   /// the escape sequence (including ESC, not 0-terminated!)
   const char * seqence;

   /// the (pseudo-) Unicode for the sequence
   Unicode uni;

   /// a mapping from keyboard escape sequences to Unicodes
   static ESCmap the_ESCmap[];
};
//-----------------------------------------------------------------------------

#endif // __LINEINPUT_HH_DEFINED__
