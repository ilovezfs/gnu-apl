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

#define Function APL_Function   // make realine happy
#include "Command.hh"
#include "Common.hh"   // for HAVE_LIBREADLINE etc.
#include "Input.hh"
#include "InputFile.hh"
#include "IO_Files.hh"
#include "LineInput.hh"
#include "main.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include <signal.h>
#include "SystemVariable.hh"
#include "UTF8_string.hh"
#include "UserPreferences.hh"
#include "Workspace.hh"
#undef Function

/// an optional function called before printing a prompt and reading input
extern void (*start_input)();

/// an optional function called after having read a line of input
extern void (*end_input)();

void (*start_input)() = 0;
void (*end_input)() = 0;

#if HAVE_LIBREADLINE

#  if defined(HAVE_READLINE_READLINE_H)
#    include <readline/readline.h>
#  elif defined(HAVE_READLINE_H)
#    include <readline.h>
#  else /* !defined(HAVE_READLINE_H) */
extern char *readline ();
#  endif /* !defined(HAVE_READLINE_H) */
char *cmdline = NULL;

#ifdef HAVE_READLINE_HISTORY
#  if defined(HAVE_READLINE_HISTORY_H)
#    include <readline/history.h>
#  elif defined(HAVE_HISTORY_H)
#    include <history.h>
#  else /* !defined(HAVE_HISTORY_H) */
extern void add_history ();
extern int write_history ();
extern int read_history ();
#  endif /* defined(HAVE_READLINE_HISTORY_H) */
/* no history */
#endif /* HAVE_READLINE_HISTORY */

#else // ! HAVE_LIBREADLINE

static void   add_history(const char *)    { }
static void   read_history(const char *)   { }

static void   rl_initialize()              { }
static void   stifle_history(int)          { }
static void   rl_stuff_char(char)          { }
static void   rl_list_funmap_names()       { }

static char * readline(const char *)       { return 0; }
static int    rl_bind_key(int, int)        { return 0; }
static int    rl_crlf()                    { return 0; }
static int    rl_delete_text(int, int)     { return 0; }


static const char * rl_readline_name = "GnuAPL";
static int          rl_done = 0;
void              (*rl_startup_hook)() = 0;

static int readline_dummy()
{
int ret = 0;
   ret += *rl_readline_name;
   ret += rl_done;
   (*rl_startup_hook)();
   readline_dummy();
   readline(0);
   rl_bind_key(0, 0);
   rl_crlf();
   rl_delete_text(0, 0);
   add_history(0);
   read_history(0);

   rl_initialize();
   stifle_history(0);
   rl_stuff_char(0);
   rl_list_funmap_names();
   return ret;
}

#endif //  don't HAVE_LIBREADLINE

//-----------------------------------------------------------------------------
void
Input::init(bool do_read_history)
{
#if HAVE_LIBREADLINE
   rl_readline_name = strdup("GnuAPL");
   rl_initialize();
   if (do_read_history)
      {
        stifle_history(uprefs.line_history_len);
        read_history(uprefs.line_history_path.c_str());
      }

#ifdef ADVANCED_readline
   rl_startup_hook = (rl_hook_func_t *)(Input::init_readline_control_C);
#endif

// rl_function_dumper(1);
#endif
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
void
Input::exit_readline()
{
#if HAVE_LIBREADLINE
   write_history(uprefs.line_history_path.c_str());
#endif
}
//-----------------------------------------------------------------------------
void
Input::get_user_line(LineInputMode mode, const UCS_string & prompt,
                     UCS_string & user_line, bool & eof)
{
   Output::set_color_mode(Output::COLM_INPUT);

   if (start_input)   (*start_input)();

const APL_time_us from = now();
const char * line;

UTF8_string prompt_utf(prompt);
   line = call_readline(prompt_utf.c_str());

   Workspace::add_wait(now() - from);

   if (end_input)   (*end_input)();

   if (line == 0)
      {
        user_line.clear();
        eof = true;
        return;
      }

UTF8_string user_line_utf(line);
   user_line = UCS_string(user_line_utf);
}
//-----------------------------------------------------------------------------
char *
Input::call_readline(const char * prompt)
{
#ifdef ADVANCED_readline

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

#else

char * ret = readline(prompt);

#endif

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

#ifdef ADVANCED_readline

// the functions below are not present in the readline lib shipped with OS-X

//-----------------------------------------------------------------------------
int
Input::init_readline_control_C()
{
#if HAVE_LIBREADLINE
// CERR << "init_readline_control_C()" << endl;

   rl_bind_key(CTRL('C'), &readline_control_C);

   // it seems to suffice if this this function is called once (i.e. after
   // first time where readline() is being called
   //
   rl_startup_hook = 0;
#endif

   return 0;
}
//-----------------------------------------------------------------------------
int
Input::readline_control_C(int count, int key)
{
#if HAVE_LIBREADLINE
   control_C(SIGINT);
   rl_crlf();
   rl_delete_text(0, rl_end);
   rl_done = 1;

   attention_raised = false;
   interrupt_raised = false;
#endif

   return 0;
}
//-----------------------------------------------------------------------------
#endif
