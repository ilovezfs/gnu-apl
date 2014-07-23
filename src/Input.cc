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

#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "Command.hh"
#include "Common.hh"   // for HAVE_LIBREADLINE etc.
#include "Input.hh"
#include "InputFile.hh"
#include "IO_Files.hh"
#include "main.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include <signal.h>
#include "SystemVariable.hh"
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

#define _FUNCTION_DEF
#include <readline/readline.h>
#include <readline/history.h>

#else // ! HAVE_LIBREADLINE

bool Input::use_readline = false;

static void add_history(const char * line) {}
static void rl_initialize() {}
static void stifle_history(int) {}
static void read_history(const char *) {}
static void rl_stuff_char(char) {}
static char * readline(const char *) {}
void rl_list_funmap_names() {}
int rl_bind_key(int, int) {}
int rl_parse_and_bind(const char *) {}

const char *  rl_readline_name = "GnuAPL";
int rl_catch_signals = 1;
void (*rl_startup_hook)() = 0;

#endif //  HAVE_LIBREADLINE

// a function to be used if readline() does not exist or
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
//-----------------------------------------------------------------------------
void
Input::init(bool do_read_history)
{
   if (use_readline)
      {
        rl_readline_name = strdup("GnuAPL");
        rl_initialize();
        if (do_read_history)
           {
             stifle_history(readline_history_len);
             read_history(readline_history_path.c_str());
           }

        rl_startup_hook = Input::init_readline_control_C;
//      rl_function_dumper(1);
      }
}
//-----------------------------------------------------------------------------
int
Input::readline_version()
{
#if HAVE_LIBREADLINE
   return rl_readline_version;
#else
   return 0;
#endif
}
//-----------------------------------------------------------------------------
UCS_string
Input::get_line()
{
   Quad_QUOTE::done(true, LOC);
   InputFile::increment_current_line_no();

const char * input_type = "?";
const UTF8 * buf = IO_Files::get_file_line();

   if (buf)   // we got a line from a test file.
      {
        input_type = "-T";
        CIN << Workspace::get_prompt() << buf << endl;
      }

const APL_time_us from = now();
int control_D_count = 0;
   while (buf == 0)
      {
        input_type = "U";
        Output::set_color_mode(Output::COLM_INPUT);
        buf = get_user_line(&Workspace::get_prompt());

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
   if (s == 0)   return 0;// end of file

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
   write_history(readline_history_path.c_str());
#endif
}
//-----------------------------------------------------------------------------
const unsigned char *
Input::get_user_line(const UCS_string * prompt)
{
   if (start_input)   (*start_input)();

const APL_time_us from = now();
char * line;
   if (!use_readline)
      {
        line = no_readline(prompt);
      }
   else if (prompt)
      {
        UTF8_string prompt_utf(*prompt);
        line = call_readline(prompt_utf.c_str());
      }
   else
      {
        line = call_readline(0);
      }

   if (end_input)   (*end_input)();

const APL_time_us to = now();
   Workspace::add_wait(to - from);

UTF8 * l = (UTF8 *)line;
   if (l)
      {
        while (*l && *l <= ' ')   ++l;   // skip leading whitespace

        if (use_readline && *l)   add_history(line);
      }

   return l;
}
//-----------------------------------------------------------------------------
const char *
Input::get_user_line_nabla(const UCS_string * prompt)
{
   InputFile::increment_current_line_no();

const UTF8 * line = IO_Files::get_file_line();
   if (line)   return (const char *)line;
   return (const char *)get_user_line(prompt);
}
//-----------------------------------------------------------------------------
UCS_string
Input::get_quad_cr_line(UCS_string ucs_prompt)
{
   if (InputFile::is_validating())
      {
        Quad_QUOTE::done(true, LOC);

        const UTF8 * line = IO_Files::get_file_line();
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
            rl_stuff_char(utf_prompt[s] & 0x00FF);

        cout << '\r';
        line = call_readline(0);
      }

   if (line == 0)   return UCS_string();

const UTF8_string uline(line);
   return UCS_string(uline);
}
//-----------------------------------------------------------------------------
const UTF8 *
Input::read_file_line()
{
static char buf[2000];
int b = 0;
   for (;b < sizeof(buf) - 1; ++b)
       {
         int cc = fgetc(InputFile::current_file()->file);
         if (cc == EOF)   // end of file
            {
              if (b == 0)   return 0;
              break;
            }

         if (cc == '\n' || cc == 2)   // end of line or ^B
            {
              break;
            }

         if (cc == '\r')   continue;   // ignore carrige returns

          buf[b] = cc;
       }

   buf[b] = 0;

   Log(LOG_test_execution)
      CERR << "read_file_line() -> " << buf << endl;

   return (UTF8 *)buf;
}
//-----------------------------------------------------------------------------
char *
Input::call_readline(const char * prompt)
{
   // deinstall ^C handler...
   //
static struct sigaction control_C_action;
   memset(&control_C_action, 0, sizeof(struct sigaction));
   control_C_action.sa_handler = SIG_IGN;
   sigaction(SIGINT,  &control_C_action, 0);

   set_intr_char(0);   // none
char * ret = readline(prompt);
   set_intr_char(3);   // ^C

   control_C_action.sa_handler = &control_C;
   sigaction(SIGINT,  &control_C_action, 0);

   return ret;
}
//-----------------------------------------------------------------------------
void
Input::set_intr_char(int ch)
{
struct termios tios;
   tcgetattr(STDIN_FILENO, &tios);
   tios.c_cc[VINTR] = ch;
   tios.c_lflag &= ~ECHOCTL;
   tcsetattr(STDIN_FILENO, TCSANOW, &tios);
}
//-----------------------------------------------------------------------------
int
Input::init_readline_control_C()
{
// CERR << "init_readline_control_C()" << endl;

static char cmd[] = "set echo-control-characters off";
   rl_parse_and_bind(strdup("set echo-control-characters off"));
   rl_bind_key(CTRL('C'), &readline_control_C);

   // it seems to suffice if this this function is called once (i.e. after
   // first time where readline() is being called
   //
   rl_startup_hook = 0;
   return 0;
}
//-----------------------------------------------------------------------------
int
Input::readline_control_C(int count, int key)
{
   control_C(SIGINT);
   rl_crlf();
   rl_delete_text(0, rl_end);
   rl_done = 1;

   attention_raised = false;
   interrupt_raised = false;
   interrupt_when = 0;
   return 0;
}
//-----------------------------------------------------------------------------
