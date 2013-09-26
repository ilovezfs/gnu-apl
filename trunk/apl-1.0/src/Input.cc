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

#include <unistd.h>

#include "Command.hh"
#include "Common.hh"
#include "Input.hh"
#include "main.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include "SystemVariable.hh"
#include "TestFiles.hh"
#include "UTF8_string.hh"
#include "Workspace.hh"

namespace readline_lib
{
#include <readline/readline.h>
#include <readline/history.h>
};

const char * Input::input_file_name = 0;
FILE * Input::input_file_FILE = 0;

Input Input::the_input;

bool Input::stdin_from_file = false;

//-----------------------------------------------------------------------------
Input::Input()
{
   readline_lib::stifle_history(500);
   readline_lib::read_history(".apl.history");
}
//-----------------------------------------------------------------------------
UCS_string
Input::get_line()
{
   Quad_QUOTE::done(true, LOC);

const char * input_type = "?";
const UTF8 * buf = TestFiles::get_testcase_line();

   if (buf)   // we got a line from a test file.
      {
        input_type = "-T";
        CIN << Workspace::the_workspace->prompt << buf << endl;
      }
   else if (input_file_FILE)   // a -f file is open
      {
        input_type = "-f";
        buf = get_f_line(input_file_FILE);
        if (buf)   CIN << Workspace::the_workspace->prompt << buf << endl;
      }
   else if (input_file_name)   // -f present, but not opened yet
      {
        // A special case is -f - where we open stdin
        //
        stdin_from_file = !strcmp(input_file_name, "-");
        if (stdin_from_file && scriptname)
           {
             input_file_FILE = fopen(scriptname, "r");
           }
        else if (stdin_from_file)
           {
            input_file_FILE = stdin;
           }
        else
           {
             input_file_FILE = fopen(input_file_name, "r");
           }

        if (input_file_FILE)
           {
             input_type = "-f";
             buf = get_f_line(input_file_FILE);
           }
        else
           {
             CERR << "cannot open file " << input_file_name
                  << " (from -f) at " LOC << endl;
             input_file_name = 0;
           }
      }

   if (buf == 0)   // all reads from files failed
      {
        input_type = "U";
        buf = get_user_line(Workspace::the_workspace->prompt.c_str());
      }

   if (buf == 0)   // ^D or end of file
      {
        CIN << endl;
        COUT << "^D (use command )OFF to leave the APL interpreter!)" << endl;
        return UCS_string();
      }

size_t len = strlen((const char *)buf);

   // strip trailing blanks.
   //
   while (len && (buf[len - 1] <= ' '))   --len;
   if (len == 0)   return UCS_string();

   // strip leading blanks.
   //
const UTF8 * buf1 = buf;
   while (len && buf1[0] <= ' ')   { ++buf1;  --len; }

UTF8_string utf(buf1, len);

   Log(LOG_get_line)
      CERR << input_type << " '" << utf << "'" << endl;

   return UCS_string(utf);
}
//-----------------------------------------------------------------------------
UTF8 *
Input::get_f_line(FILE * file)
{
static char buffer[2000];

const char * s = fgets(buffer, sizeof(buffer) - 1, file);
   if (s == 0)   // end of file
      {
        input_file_name = 0;
        input_file_FILE = 0;
        stdin_from_file = false;
        return 0;
      }

   buffer[sizeof(buffer) - 1] = 0;   // make strlen happy.
size_t len = strlen(buffer);
   if (buffer[len - 1] == '\n')   buffer[--len] = 0;
   if (buffer[len - 1] == '\r')   buffer[--len] = 0;

   return (UTF8 *)&buffer;
}
//-----------------------------------------------------------------------------
void
Input::exit_readline()
{
   readline_lib::write_history(".apl.history");
}
//-----------------------------------------------------------------------------
const unsigned char *
Input::get_user_line(const char * prompt)
{
   Output::set_color_mode(Output::COLM_INPUT);

const APL_time from = now();
char * line = readline_lib::readline(prompt);

const APL_time to = now();
   Workspace::the_workspace->add_wait(to - from);

UTF8 * l = (UTF8 *)line;
   if (l)
      {
        while (*l && *l <= ' ')   ++l;   // skip leading whitespace

        if (*l)   readline_lib::add_history(line);
      }

   return l;
}
//-----------------------------------------------------------------------------
const unsigned char *
Input::get_user_line_1(const char * prompt)
{
const UTF8 * line = TestFiles::get_testcase_line();
   if (line)   return line;

   if (input_file_FILE)   // a -f file is open
      {
        line = get_f_line(input_file_FILE);
        if (line)   return line;
      }

   if (input_file_name)   // -f present, but not opened yet
      {
        // we have a -f option with a filename to rerad from
        // we open the file and read from it as long as possible.
        // get_f_line() will set input_file_name and input_file_FILE to 0 when
        // it reaches EOF.
        //
        // A special case is -f - where we open stdin
        //
        stdin_from_file = !strcmp(input_file_name, "-");
        if (stdin_from_file)
           {
Q(stdin)
             input_file_FILE = stdin;
           }
        else
           {
             input_file_FILE = fopen(input_file_name, "r");
           }
        if (input_file_FILE)
           {
             line = get_f_line(input_file_FILE);
             if (line)   return line;
           }
        else
           {
             CERR << "cannot open file " << input_file_name
                  << "(from -f) at " LOC << endl;
             input_file_name = 0;
           }
      }

   // all reads from files failed.
   //
   while (*prompt)   readline_lib::rl_stuff_char(*prompt++ & 0x00FF);

   line = (UTF8 *)readline_lib::readline(0);

   while (*line && *line <= ' ')   ++line;

   return line;
}
//-----------------------------------------------------------------------------
UCS_string
Input::get_quad_cr_line(UCS_string ucs_prompt)
{
   if (TestFiles::is_validating())
      {
        Quad_QUOTE::done(true, LOC);

        const UTF8 * line = TestFiles::get_testcase_line();
        if (line)
           {
             // for each leading backspace in line: remove last prompt char
             while (line[0] == UNI_ASCII_BS && ucs_prompt.size() > 0)
                {
                  ++line;
                  ucs_prompt.shrink(ucs_prompt.size() - 1);
                }

             UCS_string ret(ucs_prompt);
             ret.app(line);
             return ret;
           }

        // no more testcase lines: fall through to terminal input
      }

UTF8_string utf_prompt(ucs_prompt);
   loop(s, utf_prompt.size())
      readline_lib::rl_stuff_char(utf_prompt[s] & 0x00FF);

   cout << '\r';
const char * line = readline_lib::readline(0);

const UTF8_string uline(line);
   return UCS_string(uline);
}
//-----------------------------------------------------------------------------
