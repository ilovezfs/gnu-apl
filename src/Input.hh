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
#include "LineInput.hh"
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

   /// Exit the readline library, editing the history file.
   static void exit_readline();

   /// initialize readline library
   static void init(bool read_history);

   /// get one line
   static void get_line(LineInputMode mode, UCS_string & user_line, bool & eof);

   /// read one line from the user (-terminal).
   static void get_user_line(LineInputMode mode, const UCS_string * prompt,
                             UCS_string & user_line, bool & eof);

protected:
   /// prepare readline and call it
   static char * call_readline(const char *);

   /// set the ^C character to ch
   static void set_intr_char(int ch);

   /// initialize the handler for ^C in readline
   static  int init_readline_control_C();

   /// handle ^C in readline
   static int readline_control_C(int count, int key);
};


#endif // __LINE_INPUT_HH_DEFINED__
