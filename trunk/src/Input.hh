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

#ifndef __LINE_INPUT_HH_DEFINED__
#define __LINE_INPUT_HH_DEFINED__

#include <stdio.h>
#include <iostream>
#include <fstream>

#include "Assert.hh"
#include "UCS_string.hh"

using namespace std;

/*
 The classes below are used to combine normal user I/O and automatic
 testcase execution. It works like this:

       (CIN)
         |
         |
         V
     +---+---+             +-------------------+
     | Input | <--------   | testcase files(s) +-----+
     +---+---+             +---------+---------+     |
         |                           |               |
         |                           |               |
         V                           |               |
      +--+--+                        |               |
      | APL |                        |               |
      +--+--+                        |               |
         |                           |               |
         |                           |               |
         +-----------------------+   |               |
         |                       |   |               |
         |                       V   V               |
         |                    +--+---+--+            |
         |                    | compare +            |
         |                    +----+----+            |
         |                         |                 |
         |                         |                 |
         V                         V                 V
       (COUT)                testcase.log       summary.log
 
 */

/** the input for the APL interpreter from either the user (via readline)
    or from testcase files.
 */
class Input
{
public:
   /// return the readline version used, 0 if readline was disabled
   static int readline_version();

   /// get one line with trailing blanks removed
   static UCS_string get_line();

   /// get one line from \b filename with trailing blanks removed
   static UTF8 * get_f_line(FILE * filename);

   /// print prompt and return one line from the user
   static UCS_string get_quad_cr_line(UCS_string prompt);

   /// Exit the readline library, editing the history file.
   static void exit_readline();

   /// read one line from the user (-terminal) for the ∇-editor
   static const char * get_user_line_nabla(const UCS_string * prompt);

   /// initialize readline library
   static void init(bool read_history);

   /// true if readline lib is present and shall be used
   static bool use_readline;

   /// number of lines in the readline history
   static int readline_history_len;

   /// location of the readline history
   static UTF8_string readline_history_path;

   /// read one line from input file with CR and LF removed
   static const UTF8 * read_file_line();

protected:
   /// read one line from the user (-terminal).
   static const unsigned char * get_user_line(const UCS_string * prompt);

   static char * call_readline(const char *);

   static void set_intr_char(int ch);

   static  int init_readline_control_C();

   static int readline_control_C(int count, int key);
};


#endif // __LINE_INPUT_HH_DEFINED__
