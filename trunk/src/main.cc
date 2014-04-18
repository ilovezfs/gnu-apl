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

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "buildtag.hh"
#include "Command.hh"
#include "Common.hh"
#include "Input.hh"
#include "LibPaths.hh"
#include "Logging.hh"
#include "main.hh"
#include "Output.hh"
#include "NativeFunction.hh"
#include "Prefix.hh"
#include "ProcessorID.hh"
#include "Quad_SVx.hh"
#include "TestFiles.hh"
#include "UserFunction.hh"
#include "ValueHistory.hh"
#include "Workspace.hh"
#include "UserPreferences.hh"
#include "Value.hh"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static const char * build_tag[] = { BUILDTAG, 0 };

//-----------------------------------------------------------------------------
/// initialize subsystems that are independent of argv[]
void
init_1(const char * argv0, bool log_startup)
{
rlimit rl;
   getrlimit(RLIMIT_AS, &rl);
   total_memory = rl.rlim_cur;

   Output::init();
   Avec::init();
   LibPaths::init(argv0);
   Value::init();
   VH_entry::init();
}
//-----------------------------------------------------------------------------
/// initialize subsystems that  depend on argv[]
void
init_2(const char * argv0, bool log_startup)
{
   Svar_DB::init(argv0, log_startup, uprefs.do_svars);
   Input::init(true);
}
//-----------------------------------------------------------------------------
/// the opposite of init()
void
cleanup()
{
   ProcessorID::disconnect();

   NativeFunction::cleanup();
   Input::exit_readline();   // write history
   Output::reset_colors();
}
//-----------------------------------------------------------------------------

APL_time_us interrupt_when = 0;
bool interrupt_raised = false;
bool attention_raised = false;

static struct sigaction old_control_C_action;
static struct sigaction new_control_C_action;

static void
control_C(int)
{
APL_time_us when = now();
   attention_raised = true;
   if ((when - interrupt_when) < 500000)   // second ^C within 500 ms
      {
        interrupt_raised = true;
      }

   interrupt_when = when;
}

//-----------------------------------------------------------------------------
static struct sigaction old_SEGV_action;
static struct sigaction new_SEGV_action;

static void
signal_SEGV_handler(int)
{
   CERR << "\n\n====================================================\n"
           "SEGMENTATION FAULT" << endl;

   Backtrace::show(__FILE__, __LINE__);

   CERR << "====================================================\n";

   // count errors
   TestFiles::assert_error();

   Command::cmd_OFF(3);
}

//-----------------------------------------------------------------------------
static void
signal_USR1_handler(int)
{
   CERR << "Got signal USR1" << endl;
}

static struct sigaction old_USR1_action;
static struct sigaction new_USR1_action;

//-----------------------------------------------------------------------------
static struct sigaction old_TERM_action;
static struct sigaction new_TERM_action;

static void
signal_TERM_handler(int)
{
   cleanup();
   sigaction(SIGTERM, &old_TERM_action, 0);
   raise(SIGTERM);
}
//-----------------------------------------------------------------------------
static struct sigaction old_HUP_action;
static struct sigaction new_HUP_action;

static void
signal_HUP_handler(int)
{
   cleanup();
   sigaction(SIGHUP, &old_HUP_action, 0);
   raise(SIGHUP);
}
//-----------------------------------------------------------------------------
void
show_argv(int argc, const char ** argv)
{
   CERR << "argc: " << argc << endl;
   loop(a, argc)   CERR << "  argv[" << a << "]: '" << argv[a] << "'" << endl;
}
//-----------------------------------------------------------------------------
/// print a welcome message (copyright notice)
static void
show_welcome(ostream & out, const char * argv0)
{
char c1[200];
char c2[200];
   snprintf(c1, sizeof(c1), _("Welcome to GNU APL version %s"),build_tag[1]);
   snprintf(c2, sizeof(c2), _("for details run: %s --gpl."), argv0);

const char * lines[] =
{
  "",
  "   ______ _   __ __  __    ___     ____   __ ",
  "  / ____// | / // / / /   /   |   / __ \\ / / ",
  " / / __ /  |/ // / / /   / /| |  / /_/ // /  ",
  "/ /_/ // /|  // /_/ /   / ___ | / ____// /___",
  "\\____//_/ |_/ \\____/   /_/  |_|/_/    /_____/",



  "",
  c1,
  "",
  "Copyright (C) 2008-2014  Dr. Jürgen Sauermann",
  "Banner by FIGlet: www.figlet.org",
  "",
_("This program comes with ABSOLUTELY NO WARRANTY;"),
  c2,
  "",
_("This program is free software, and you are welcome to redistribute it"),
_("according to the GNU Public License (GPL) version 3 or later."),
  "",
  0
};

   // compute max. length
   //
int len = 0;
   for (const char ** l = lines; *l; ++l)
       {
         const char * cl = *l;
         const int clen = strlen(cl);
         if (len < clen)   len = clen;
       }
 
const int left_pad = (80 - len)/2;

   for (const char ** l = lines; *l; ++l)
       {
         const char * cl = *l;
         const int clen = strlen(cl);
         const int pad = left_pad + (len - clen)/2;
         loop(p, pad)   out << " ";
         out << cl << endl;
       }
}
//-----------------------------------------------------------------------------
#ifdef ENABLE_NLS
extern char * bindtextdomain (const char * domainname, const char * dirname);
extern char * textdomain (const char * domainname);

static void
init_NLS()
{
   Log(LOG_startup)   CERR << "NLS is enabled." << endl;

const char * locale = setlocale(LC_ALL, "");
   if (locale == 0)
      {
        CERR <<
"*** function locale() has returned 0 which indicates a problem\n"
"with your locale settings. This will most likely prevent APL characters\n"
"from being entered or displayed properly\n"
"\n"
"Please fix your locales or ./configure GNU APL with --disable-nls\n"
              << endl;
        return;
      }

   Log(LOG_startup)   CERR << "locale is: " << locale << "." << endl;

   // ensure that numbers are scanned and printed in
   // a defined APL-compatible manner, regardless of the locale
   //
   setlocale(LC_NUMERIC, "C");

const char * dir = bindtextdomain(PACKAGE, LOCALEDIR);

   if (dir)
      {
        Log(LOG_startup)   CERR << "locale directory is: "
                                << dir << "." << endl;
      }
   else
      {
        CERR << "*** bindtextdomain(" << PACKAGE << ", " << LOCALEDIR
             << " failed: " << strerror(errno) << endl;
        return;
      }

const char * dom = textdomain(PACKAGE);
   if (dom)
      {
        Log(LOG_startup)   CERR << "locale domain is: " << dom << "." << endl;
      }
   else
      {
        CERR << "*** textdomain(" << PACKAGE << ", " << LOCALEDIR
             << " failed: " << strerror(errno) << endl;
        return;
      }
}
#endif
//-----------------------------------------------------------------------------

static void
init_OMP()
{
#ifdef MULTICORE

# ifdef STATIC_CORE_COUNT

CoreCount core_count_wanted = STATIC_CORE_COUNT;

# elif CORE_COUNT_WANTED == -1  // all available cores

const CoreCount core_count_wanted = max_cores();

# elif CORE_COUNT_WANTED == -2  // --cc option

const CoreCount core_count_wanted = (uprefs.requested_cc == CCNT_UNKNOWN)
                            ? max_cores()        // -cc option not given
                            : uprefs.requested_cc;   // --cc option value

# elif CORE_COUNT_WANTED == -3  // ⎕SYL, initially 1

const CoreCount core_count_wanted = CCNT_MIN;

#else

const CoreCount core_count_wanted = CCNT_MIN;

#endif

   omp_set_dynamic(false);
const CoreCount cores_available = setup_cores(core_count_wanted);

#endif // MULTICORE
}
//-----------------------------------------------------------------------------
// read user preference file(s) if present
static void
read_config_file(bool sys, bool log)
{
char filename[PATH_MAX + 1] = "/etc/gnu-apl.d/preferences";   // fallback filename


   if (sys)   // try /etc/gnu-apl/preferences
      {
#ifdef SYSCONFDIR
        if (strlen(SYSCONFDIR))
           snprintf(filename, PATH_MAX, "%s/gnu-apl.d/preferences", SYSCONFDIR);
#endif
      }
   else       // try $HOME/.gnu_apl
      {
        const char * HOME = getenv("HOME");
        if (HOME == 0)
           {
             if (log)
                CERR << "environment variable 'HOME' is not defined!" << endl;
             return;
           }
        snprintf(filename, PATH_MAX, "%s/.gnu-apl/preferences", HOME);
      }

   filename[PATH_MAX] = 0;

FILE * f = fopen(filename, "r");
   if (f == 0)
      {
         if (log)
            CERR << "config file " << filename
                 << " is not present/readable" << endl;
         return;
      }

   if (log)
      CERR << "Reading config file " << filename << " ..." << endl;

int line = 0;
   for (;;)
       {
         enum { BUFSIZE = 200 };
         char buffer[BUFSIZE + 1];
         const char * s = fgets(buffer, BUFSIZE, f);
         if (s == 0)   break;   // end of file

         buffer[BUFSIZE] = 0;
         ++line;

         // skip leading spaces
         //
         while (*s && *s <= ' ')   ++s;
         if (*s == 0)     continue;   // empty line
         if (*s == '#')   continue;   // comment line
         char opt[BUFSIZE] = { 0 };
         char arg[BUFSIZE] = { 0 };
         int d[100];
         const int count = sscanf(s, "%s"
                   " %s %X %X %X %X %X %X %X %X %X"
                   " %X %X %X %X %X %X %X %X %X %X"
                   " %X %X %X %X %X %X %X %X %X %X"
                   " %X %X %X %X %X %X %X %X %X %X"
                   " %X %X %X %X %X %X %X %X %X %X"
                   " %X %X %X %X %X %X %X %X %X %X"
                   " %X %X %X %X %X %X %X %X %X %X"
                   " %X %X %X %X %X %X %X %X %X %X"
                   " %X %X %X %X %X %X %X %X %X %X"
                   " %X %X %X %X %X %X %X %X %X %X",
              opt, arg,  d+ 1, d+ 2, d+ 3, d+ 4, d+ 5, d+ 6, d+ 7, d+ 8, d+ 9,
                   d+10, d+11, d+12, d+13, d+14, d+15, d+16, d+17, d+18, d+19,
                   d+20, d+21, d+22, d+23, d+24, d+25, d+26, d+27, d+28, d+29,
                   d+30, d+31, d+32, d+33, d+34, d+35, d+36, d+37, d+38, d+39,
                   d+40, d+41, d+42, d+43, d+44, d+45, d+46, d+47, d+48, d+49,
                   d+50, d+51, d+52, d+53, d+54, d+55, d+56, d+57, d+58, d+59,
                   d+60, d+61, d+62, d+63, d+64, d+65, d+66, d+67, d+68, d+69,
                   d+70, d+71, d+72, d+73, d+74, d+75, d+76, d+77, d+78, d+79,
                   d+80, d+81, d+82, d+83, d+84, d+85, d+86, d+87, d+88, d+89,
                   d+90, d+91, d+92, d+93, d+94, d+95, d+96, d+97, d+98, d+99);

         if (count < 2)
            {
              CERR << "bad tag or value at line " << line
                   << " of config file " << filename << " (ignored)" << endl;
              continue;
            }
         d[0] = strtol(arg, 0, 16);
         const bool yes = !strcasecmp(arg, "YES"     )
                       || !strcasecmp(arg, "ENABLED" )
                       || !strcasecmp(arg, "ON"      );

         const bool no  = !strcasecmp(arg, "NO"      )
                       || !strcasecmp(arg, "DISABLED")
                       || !strcasecmp(arg, "OFF"     ) ;

         const bool yes_no  = yes || no;

         if (!strcasecmp(opt, "Color"))
            {
              if (yes_no)   // user said "Yes" or "No"
                 {
                   // if the user said "No" then use_curses is not touched.
                   // if the user said "Yes" (which is an obsolete setting)
                   // then use_curses is cleared because at the time when
                   // "Yes" was valid, curses was not yet implemented.
                   //
                   uprefs.do_Color = yes;
                   if (yes)   Output::use_curses = false;
                 }
              if (!strcasecmp(arg, "ANSI"))
                 {
                   uprefs.do_Color = true;
                   Output::use_curses = false;
                 }
              if (!strcasecmp(arg, "CURSES"))
                 {
                   uprefs.do_Color = true;
                   Output::use_curses = true;
                 }
            }
         else if (yes_no && !strcasecmp(opt, "Welcome"))
            {
              uprefs.silent = no;
            }
         else if (yes_no && !strcasecmp(opt, "SharedVars"))
            {
              uprefs.do_svars = yes;
            }
         else if (!strcasecmp(opt, "Logging"))
            {
              Log_control(LogId(d[0]), true);
            }
         else if (!strcasecmp(opt, "CIN-SEQUENCE"))
            {
              loop(p, count - 1)   Output::color_CIN[p] = (char)(d[p] & 0xFF);
              Output::color_CIN[count - 1] = 0;
            }
         else if (!strcasecmp(opt, "COUT-SEQUENCE"))
            {
              loop(p, count - 1)   Output::color_COUT[p] = (char)(d[p] & 0xFF);
              Output::color_COUT[count - 1] = 0;
            }
         else if (!strcasecmp(opt, "CERR-SEQUENCE"))
            {
              loop(p, count - 1)   Output::color_CERR[p] = (char)(d[p] & 0xFF);
              Output::color_CERR[count - 1] = 0;
            }
         else if (!strcasecmp(opt, "UERR-SEQUENCE"))
            {
              loop(p, count - 1)   Output::color_UERR[p] = (char)(d[p] & 0xFF);
              Output::color_UERR[count - 1] = 0;
            }
         else if (!strcasecmp(opt, "RESET-SEQUENCE"))
            {
              loop(p, count - 1)   Output::color_RESET[p] = (char)(d[p] & 0xFF);
              Output::color_RESET[count - 1] = 0;
            }
         else if (!strcasecmp(opt, "CLEAR-EOL-SEQUENCE"))
            {
              loop(p, count - 1)   Output::clear_EOL[p] = (char)(d[p] & 0xFF);
              Output::clear_EOL[count - 1] = 0;
            }
         else if (!strcasecmp(opt, "CIN-FOREGROUND"))
            {
              Output::color_CIN_foreground = atoi(arg);
            }
         else if (!strcasecmp(opt, "CIN-BACKGROUND"))
            {
              Output::color_CIN_background = atoi(arg);
            }
         else if (!strcasecmp(opt, "COUT-FOREGROUND"))
            {
              Output::color_COUT_foreground = atoi(arg);
            }
         else if (!strcasecmp(opt, "COUT-BACKGROUND"))
            {
              Output::color_COUT_background = atoi(arg);
            }
         else if (!strcasecmp(opt, "CERR-FOREGROUND"))
            {
              Output::color_CERR_foreground = atoi(arg);
            }
         else if (!strcasecmp(opt, "CERR-BACKGROUND"))
            {
              Output::color_CERR_background = atoi(arg);
            }
         else if (!strcasecmp(opt, "UERR-FOREGROUND"))
            {
              Output::color_UERR_foreground = atoi(arg);
            }
         else if (!strcasecmp(opt, "UERR-BACKGROUND"))
            {
              Output::color_UERR_background = atoi(arg);
            }
         else if (!strncasecmp(opt, "LIBREF-", 7))
            {
              const int lib_ref = atoi(opt + 7);
              if (lib_ref < 0 || lib_ref > 9)
                 {
                   CERR << "bad library reference number " << lib_ref
                        << " at line " << line << " of config file "
                        << filename << " (ignored)" << endl;
                   continue;
                 }

              LibPaths::set_lib_dir((LibRef)lib_ref, arg,
                                    sys ? LibPaths::LibDir::CS_PREF_SYS
                                        : LibPaths::LibDir::CS_PREF_HOME);
            }
         else if (!strcasecmp(opt, "READLINE_HISTORY_LEN"))
            {
              Input::readline_history_len = atoi(arg);
            }
         else if (!strcasecmp(opt, "READLINE_HISTORY_PATH"))
            {
              Input::readline_history_path = UTF8_string(arg);
            }
       }

   fclose(f);
}
//-----------------------------------------------------------------------------
int
main(int argc, const char * _argv[])
{
   {
     // make curses happy
     //
     const char * term = getenv("TERM");
     if (term == 0 || *term == 0)   setenv("TERM", "dumb", 1);
   }

bool log_startup = false;
const char ** argv = UserPreferences::expand_argv(argc, _argv, log_startup);
   Quad_ARG::argc = argc;   // remember argc for ⎕ARG
   Quad_ARG::argv = argv;   // remember argv for ⎕ARG

#ifdef DYNAMIC_LOG_WANTED
   if (log_startup)   Log_control(LID_startup, true);
#endif // DYNAMIC_LOG_WANTED

const char * argv0 = argv[0];
   init_1(argv0, log_startup);

   read_config_file(true, log_startup);   // read /etc/gnu-apl.d/preferences
   read_config_file(false, log_startup);   // read $HOME/.gnu_apl

   // struct sigaction differs between GNU/Linux and other systems, which
   // causes direct bracket assignment to not compile on some machines.
   //
   // We therefore memset everything to 0 and then set the handler (which
   // should be compatible on GNU/Linux and other systems.
   //
   memset(&new_control_C_action, 0, sizeof(struct sigaction));
   memset(&new_USR1_action,      0, sizeof(struct sigaction));
   memset(&new_SEGV_action,      0, sizeof(struct sigaction));
   memset(&new_TERM_action,      0, sizeof(struct sigaction));
   memset(&new_HUP_action,       0, sizeof(struct sigaction));

   new_control_C_action.sa_handler = &control_C;
   new_USR1_action .sa_handler = &signal_USR1_handler;
   new_SEGV_action .sa_handler = &signal_SEGV_handler;
   new_TERM_action .sa_handler = &signal_TERM_handler;
   new_HUP_action  .sa_handler = &signal_HUP_handler;

   sigaction(SIGINT,  &new_control_C_action, &old_control_C_action);
   sigaction(SIGUSR1, &new_USR1_action,      &old_USR1_action);
   sigaction(SIGSEGV, &new_SEGV_action,      &old_SEGV_action);
   sigaction(SIGTERM, &new_TERM_action,      &old_TERM_action);
   sigaction(SIGHUP,  &new_HUP_action,       &old_HUP_action);

   // init NLS so that usage() will be translated
   //
#ifdef ENABLE_NLS
   init_NLS();
#endif

   uprefs.parse_argv(argc, argv);

   if (uprefs.emacs_mode)
      {
        // CIN = U+F00C0 = UTF8 F3 B0 83 80 ...
        Output::color_CIN[0] = 0xF3;
        Output::color_CIN[1] = 0xB0;
        Output::color_CIN[2] = 0x83;
        Output::color_CIN[3] = 0x80;
        Output::color_CIN[4] = 0;

        // COUT = U+F00C1 = UTF8 F3 B0 83 81 ...
        Output::color_COUT[0] = 0xF3;
        Output::color_COUT[1] = 0xB0;
        Output::color_COUT[2] = 0x83;
        Output::color_COUT[3] = 0x81;
        Output::color_COUT[4] = 0;

        // CERR = U+F00C2 = UTF8 F3 B0 83 82 ...
        Output::color_CERR[0] = 0xF3;
        Output::color_CERR[1] = 0xB0;
        Output::color_CERR[2] = 0x83;
        Output::color_CERR[3] = 0x82;
        Output::color_CERR[4] = 0;

        // UERR = U+F00C3 = UTF8 F3 B0 83 83 ...
        Output::color_UERR[0] = 0xF3;
        Output::color_UERR[1] = 0xB0;
        Output::color_UERR[2] = 0x83;
        Output::color_UERR[3] = 0x83;
        Output::color_UERR[4] = 0;

        // no clear_EOL
        Output::clear_EOL[0] = 0;
      }


   if (uprefs.daemon)
      {
        const pid_t pid = fork();
        if (pid)   // parent
           {
             Log(LOG_startup)
                CERR << "parent pid = " << getpid()
                     << " child pid = " << pid << endl;

             exit(0);   // parent returns
           }

        Log(LOG_startup)
           CERR << "child forked (pid" << getpid() << ")" << endl;
      }

   if (uprefs.wait_ms)   usleep(1000*uprefs.wait_ms);

   init_2(argv0, log_startup);

   if (!uprefs.silent)   show_welcome(cout, argv0);

   if (log_startup)   CERR << "PID is " << getpid() << endl;
   Log(LOG_argc_argv || log_startup)   show_argv(argc, argv);

   // init OMP (will do nothing if OMP is not configured)
   init_OMP();

   if (ProcessorID::init(log_startup))
      {
        // error message printed in ProcessorID::init()
        return 8;
      }

   if (uprefs.do_Color)   Output::toggle_color("ON");

   if (uprefs.do_CONT)
      {
         UCS_string cont("CONTINUE");
         UTF8_string filename =
            LibPaths::get_lib_filename(LIB0, cont, true, "xml");

         if (access(filename.c_str(), F_OK) == 0)
            {
              UCS_string load_cmd(")LOAD CONTINUE");
              Command::process_line(load_cmd);
            }
      }

   for (;;)
       {
         Token t = Workspace::immediate_execution(
                       TestFiles::test_mode == TestFiles::TM_EXIT_AFTER_ERROR);
         if (t.get_tag() == TOK_OFF)   Command::cmd_OFF(0);
       }

   /* not reached */
}
//-----------------------------------------------------------------------------
