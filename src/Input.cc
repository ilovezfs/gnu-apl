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
#include "Common.hh"   // for HAVE_LIBREADLINE etc.
#include "Input.hh"
#include "main.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include "SystemVariable.hh"
#include "TestFiles.hh"
#include "UTF8_string.hh"
#include "Workspace.hh"

int Input::readline_history_len = 500;
UTF8_string Input::readline_history_path(".apl.history");

/// an optional function called before printing a prompt and reading input
extern void (*start_input)();

/// an optional function called after having read a line of input
extern void (*end_input)();

void (*start_input)() = 0;
void (*end_input)() = 0;

#if HAVE_LIBREADLINE

bool Input::use_readline = true;

namespace readline_lib
{
#include <readline/readline.h>
#include <readline/history.h>
};   // namespace readline_lib

#else // ! HAVE_LIBREADLINE

bool Input::use_readline = false;

namespace readline_lib
{
static void add_history(const char * line) {}
static void rl_initialize() {}
static void stifle_history(int) {}
static void read_history(const char *) {}
static void rl_stuff_char(char) {}
static char * readline(const char *) {}
const char *  rl_readline_name = "GnuAPL";

};   // namespace readline_lib

#endif //  HAVE_LIBREADLINE

// a function to be used if readline_lib::readline() does not exist or
// is not wanted
static char *
no_readline(const UCS_string * prompt)
{
   if (prompt)
      {
        CIN << '\r' << *prompt << flush;
        UTF8_string prompt_utf(*prompt);
        loop(p, prompt_utf.size())
           {
             const int cc = prompt_utf[prompt_utf.size() - p - 1];
             ungetc(cc & 0xFF, stdin);
           }
      }

static char buffer[MAX_INPUT_LEN];   // returned as result
const char * s = fgets(buffer, sizeof(buffer) - 1, stdin);
   if (s == 0)   return 0;

int len = strlen(buffer);
   if (len && buffer[len - 1] == '\n')   buffer[--len] = 0;
   if (len && buffer[len - 1] == '\r')   buffer[--len] = 0;
   return buffer;
}


const char * Input::input_file_name = 0;
FILE * Input::input_file_FILE = 0;

bool Input::stdin_from_file = false;

//-----------------------------------------------------------------------------
void
Input::init(bool read_history)
{
   if (use_readline)
      {
        readline_lib::rl_readline_name = "GnuAPL";
        readline_lib::rl_initialize();
        if (read_history)
           {
             readline_lib::stifle_history(readline_history_len);
             readline_lib::read_history(readline_history_path.c_str());
           }

//      readline_lib::rl_function_dumper(1);
      }
}
//-----------------------------------------------------------------------------
int
Input::readline_version()
{
#if HAVE_LIBREADLINE
   return readline_lib::rl_readline_version;
#else
   return 0;
#endif
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
        CIN << Workspace::get_prompt() << buf << endl;
      }
   else if (input_file_FILE)   // a -f file is open
      {
        input_type = "-f";
        buf = get_f_line(input_file_FILE);
        if (buf)   CIN << Workspace::get_prompt() << buf << endl;
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

const APL_time_us from = now();
int control_D_count = 0;
   while (buf == 0)
      {
        input_type = "U";
        buf = get_user_line(&Workspace::get_prompt(), false);

        if (buf == 0)   // ^D or end of file
           {
             ++control_D_count;
             if (control_D_count < 5)
                {
                  CIN << endl;
                  COUT << "      ^D or end-of-input detected ("
                       << control_D_count << "). Use )OFF to leave APL!"
                       << endl;
                }

             if (control_D_count > 10 && (now() - from)/control_D_count < 10000)
                {
                  // we got 10 or more times buf == 0 at a rate of 10 ms or
                  // faster. That looks like end-of-input rather than ^D
                  // typed by the user. Abort the interpreter.
                  //
                  CIN << endl;
                  COUT << "      *** end of input" << endl;
                  Command::cmd_OFF(2);
                }
           }
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
static char buffer[MAX_INPUT_LEN];

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
#if HAVE_LIBREADLINE
   readline_lib::write_history(readline_history_path.c_str());
#endif
}
//-----------------------------------------------------------------------------
const unsigned char *
Input::get_user_line(const UCS_string * prompt, bool from_nabla)
{
   if (start_input)   (*start_input)();

   if (!from_nabla)   Output::set_color_mode(Output::COLM_INPUT);

const APL_time_us from = now();
char * line;
   if (!use_readline)
      {
        line = no_readline(prompt);
      }
   else if (prompt)
      {
        UTF8_string prompt_utf(*prompt);
        line = readline_lib::readline(prompt_utf.c_str());
      }
   else
      {
        line = readline_lib::readline(0);
      }

   if (end_input)   (*end_input)();

const APL_time_us to = now();
   Workspace::add_wait(to - from);

UTF8 * l = (UTF8 *)line;
   if (l)
      {
        while (*l && *l <= ' ')   ++l;   // skip leading whitespace

        if (use_readline && *l)   readline_lib::add_history(line);
      }

   return l;
}
//-----------------------------------------------------------------------------
const char *
Input::get_user_line_nabla(const UCS_string * prompt)
{
const UTF8 * line = TestFiles::get_testcase_line();
   if (line)   return (const char *)line;

   if (input_file_FILE)   // a -f file is open
      {
        line = get_f_line(input_file_FILE);
        if (line)   return (const char *)line;
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
             input_file_FILE = stdin;
           }
        else
           {
             input_file_FILE = fopen(input_file_name, "r");
           }
        if (input_file_FILE)
           {
             line = get_f_line(input_file_FILE);
             if (line)   return (const char *)line;
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
   return (const char *)get_user_line(prompt, true);


const APL_time_us from = now();
   if (!use_readline)
      {
        line = (UTF8 *)no_readline(prompt);
      }
   else
     {
       if (prompt)
          {
            UTF8_string prompt_utf(*prompt);
            loop(p, prompt_utf.size())
            readline_lib::rl_stuff_char(prompt_utf[p] & 0x00FF);
          }

       line = (UTF8 *)readline_lib::readline(0);
     }

   while (*line && *line <= ' ')   ++line;

const APL_time_us to = now();
   Workspace::add_wait(to - from);

   return (const char *)line;
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
             ret.append_utf8(line);
             return ret;
           }

        // no more testcase lines: fall through to terminal input
      }

UTF8_string utf_prompt(ucs_prompt);

const char * line;
   if (!use_readline)
      {
        line = no_readline(&ucs_prompt);
      }
   else
      {
        loop(s, utf_prompt.size())
            readline_lib::rl_stuff_char(utf_prompt[s] & 0x00FF);

        cout << '\r';
        line = readline_lib::readline(0);
      }

const UTF8_string uline(line);
   return UCS_string(uline);
}
//-----------------------------------------------------------------------------
