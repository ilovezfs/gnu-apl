/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2015  Dr. Jürgen Sauermann

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

#ifndef __NABLA_HH_DEFINED__
#define __NABLA_HH_DEFINED__

#include "UCS_string.hh"

class Symbol;

/// a line number like 1.2 while editing
struct LineLabel
{
   /// constuctor: line mj
   LineLabel(int mj)
   : ln_major(mj)
   {}

   /// print the line number
   void print(ostream & out) const;

   /// print a prompt like [1.2] into buffer
   UCS_string print_prompt(int min_size) const;

   /// true iff two line numbers are equal
   bool operator ==(const LineLabel & other) const;

   /// true iff this line number is smaller than \b other
   bool operator <(const LineLabel & other) const;

   /// true if tis line number is valid (i.e. the line exists)
   bool valid() const
      { return ln_major != -1; }

   /// invalidate the line number
   void clear()
          { ln_major = -1;   ln_minor.clear(); }

   /// return true iff this line number is the header line [0]
   bool is_header_line_number() const
      { return ln_major == 0 && ln_minor.size() == 0; }

   /// increase the line number
   void next();

   /// increase the line number
   void insert();

   /// the integer part of the line number
   int ln_major;

   /// the fractional part of the  line number
   UCS_string ln_minor;
};

/// The Nabla editor
class Nabla
{
public:
   /// edit the function specified in \b cmd (e.g. cmd = "∇FUN")
   static void edit_function(const UCS_string & cmd);

   /// return the line label (sucah as [1]) and the line text
   UCS_string get_label_and_text(int line, bool & is_current) const;

   /// return the number of lines
   int get_line_count() const
      { return lines.size(); }

protected:
   /// constructor
   Nabla(const UCS_string & cmd);

   /// throw a DEFN error with some additional information
   void throw_edit_error(const char * loc);

   /// edit this function
   void edit();

   /// a line label and program text, e.g.  [1.1] TEXT←TEXT
   struct FunLine
      {
        /// constructor
        FunLine(LineLabel lab, const UCS_string & tx)
        : label(lab),
          text(tx),
          stop_flag(false),
          trace_flag(false)
        {}

        /// print the line
        void print(ostream & out) const;

        /// return the line label (sucah as [1]) and the line text
        UCS_string get_label_and_text() const;

        /// the label (like [1.2] of the line
        LineLabel label;

        /// the function text of the line
        UCS_string text;

        /// true if the line has a stop flag (existing function with ∆S)
        bool stop_flag;

        /// true if the line has a trace flag (existing function with ∆T)
        bool trace_flag;
      };

    /// start editing, return source location if \b first_command is bad.
    const char * start();

   /// parse an operation (something like [...], e.g. [⎕3])
   const char * parse_oper(UCS_string & oper, bool initial);

   /// execute an edit operation
   const char * execute_oper();

   /// show function
   const char * execute_show();

   /// delete function
   const char * execute_delete();

   /// edit function line
   const char * execute_edit();

   /// edit function header
   const char * edit_header_line();

   /// edit body line
   const char * edit_body_line();

   /// restore function
   const char * execute_escape();

   /// create a new function.
   const char * open_new_function();

   /// open an existing function, or create a new one.
   const char * open_existing_function();

   /// parse [nn.mm] into a LineLabel;
   LineLabel parse_lineno(UCS_string::iterator & c);

   /// return index of line with label lab, or -1 if not found.
   int find_line(const LineLabel & lab) const;

   /// true if line with label lab exists
   bool line_exists(const LineLabel & lab) const
      { return find_line(lab) != -1; }

   /// the line number (in a .apl script file) of the ∇ that started the editor
   const int defn_line_no;

   /// the header of the function being edited
   UCS_string fun_header;

   /// the symbol for the function being edited
   Symbol * fun_symbol;

   /// the lines of the function.
   vector<FunLine> lines;

   /// editor commands
   enum Ecmd
      {
        ECMD_NOP,    ///< do nothing
        ECMD_SHOW,   ///< show function line(s) idx_from ... idx_to
        ECMD_EDIT,   ///< edit function line edit_from
        ECMD_DELETE,   ///< delete function line(s) edit_from ... idx_to
        ECMD_ESCAPE,   ///< abort editing (discard changes made so far)
      };

   /// the current editor command
   Ecmd ecmd;

   /// optional start of a range for an editor command
   LineLabel edit_from;

   /// optional end of a range for an editor command
   LineLabel edit_to;

   /// true if user has entered a range, i.e. edit_from - edit_to, - edit_to,
   ///  edit_from -
   bool got_minus;

   /// true iff this function existed before opening it
   bool function_existed;

   /// true if the function was modified
   bool modified;

   /// true iff this function shall be closed
   bool do_close;

   /// true iff this function shall be locked
   bool locked;

   /// the line number for the currently edited line
   LineLabel current_line;

   /// the command used to open the function
   const UCS_string first_command;

   /// the text for line \b current_line
   UCS_string current_text;
};

#endif // __NABLA_HH_DEFINED__
