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

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/resource.h>
#include <signal.h>

#include "../config.h"   // for _WANTED macros
#include "buildtag.hh"
#include "Command.hh"
#include "Common.hh"
#include "Input.hh"
#include "Logging.hh"
#include "main.hh"
#include "Output.hh"
#include "Prefix.hh"
#include "ProcessorID.hh"
#include "Quad_SVx.hh"
#include "TestFiles.hh"
#include "UserFunction.hh"
#include "Value.hh"
#include "Workspace.hh"

   // global flags
   //
bool silent = false;
bool do_not_echo = false;


//-----------------------------------------------------------------------------
/// initialize subsystems
void
init()
{
rlimit rl;
   getrlimit(RLIMIT_AS, &rl);
   total_memory = rl.rlim_cur;

   Avec::init();
   Value::init();
//   PrefixNode::print_all(cerr);
}
//-----------------------------------------------------------------------------
/// the opposite of init()
void
cleanup()
{
   ProcessorID::disconnect();

   Input::exit_readline();   // write history
   Output::reset_colors();
}
//-----------------------------------------------------------------------------

APL_time interrupt_when = 0;
bool interrupt_raised = false;
bool attention_raised = false;

static void
control_C(int)
{
APL_time when = now();
   attention_raised = true;
   if ((when - interrupt_when) < 500000)   // second ^C within 500 ms
      {
        interrupt_raised = true;
      }

   interrupt_when = when;
}

static struct sigaction old_control_C_action;
static struct sigaction new_control_C_action;

//-----------------------------------------------------------------------------
static void
seg_fault(int)
{
   do_Assert(0, "SEGMENTATION FAULT ", 0, 0);
   CERR << "\n\n====================================================\n";
   Command::cmd_OFF(3);
}

static struct sigaction old_segfault_action;
static struct sigaction new_segfault_action;

//-----------------------------------------------------------------------------
static void
signal_USR1_handler(int)
{
   CERR << "Got signal USR1" << endl;
}

static struct sigaction old_USR1_action;
static struct sigaction new_USR1_action;

//-----------------------------------------------------------------------------
static char APL_bin_path[PATH_MAX + 1] = "";
const char * get_APL_bin_path()   { return APL_bin_path; }

static void
set_APL_bin_path(const char * argv0)
{
   if (strchr(argv0, '/') == 0)
      {
         // if argv0 contains no / then realpath() seems to prepend the current
         // directory to argv0 (which is wrong since argv0 may be in $PATH).
         //
         // we fix this by searching argv0 in $PATH
         //
         const char * path = getenv("PATH");   // must NOT be modified
         
         if (path)
            {
              const size_t alen = strlen(argv0);
              const size_t plen = strlen(path);
              char path1[plen + 1];
              strncpy(path1, path, sizeof(path1));
              char * next = path1;
              for (;;)
                  {
                    char * semi = strchr(next, ':');
                    if (semi)   *semi = 0;
                    char filename[plen + alen + 10];
                    snprintf(filename, sizeof(filename), "%s/%s",
                             next, argv0);

                    if (access(filename, X_OK) == 0)
                       {
                         strncpy(APL_bin_path, next, sizeof(APL_bin_path));
                         return;
                       }

                    if (semi == 0)   break;
                    next = semi + 1;
                  }
            }
      }

const void * unused = realpath(argv0, APL_bin_path);
   APL_bin_path[PATH_MAX] = 0;
char * slash =   strrchr(APL_bin_path, '/');
   if (slash)   *slash = 0;

   // if we have a PWD and it is a prefix of APL_bin_path then replace PWD
   // by './'
   //
const char * PWD = getenv("PWD");
   if (PWD)   // we have a pwd
      {
        const int PWD_len = strlen(PWD);
        if (!strncmp(PWD, APL_bin_path, PWD_len) && PWD_len > 1)
           {
             strcpy(APL_bin_path + 1, APL_bin_path + PWD_len);
             APL_bin_path[0] = '.';
           }
      }
}
//-----------------------------------------------------------------------------
static char APL_lib_root[PATH_MAX + 10] = "";
const char * get_APL_lib_root()   { return APL_lib_root; }

/// return true if files dir/workspaces amd dir/wslib1 exist (not necessarily
/// being direcories
static bool
is_lib_root(const char * dir)
{
char filename[PATH_MAX + 1];

   snprintf(filename, sizeof(filename), "%s/workspaces", dir);
   if (access(filename, F_OK))   return false;

   snprintf(filename, sizeof(filename), "%s/wslib1", dir);
   if (access(filename, F_OK))   return false;

   return true;
}

static void
set_APL_lib_root()
{
const char * path = getenv("APL_LIB_ROOT");
   if (path)
      {
        const void * unused = realpath(path, APL_lib_root);
        return;
      }

   // search from "." to "." for  a valid lib-root
   //
int last_len = 2*PATH_MAX;
   APL_lib_root[0] = '.';
   APL_lib_root[1] = 0;
   for (;;)
       {
         const void * unused = realpath(APL_lib_root, APL_lib_root);
         int len = strlen(APL_lib_root);

         if (is_lib_root(APL_lib_root))   return;   // lib-rot found

         // if length has not decreased then we stop in order to
         // avoid an endless loop.
         //
         if (len >= last_len)   break;
         last_len = len;

         // move up by appending /..
         //
         APL_lib_root[len++] = '/';
         APL_lib_root[len++] = '.';
         APL_lib_root[len++] = '.';
         APL_lib_root[len++] = 0;
       }

   // no lib-root found. use ".";
   //
const void * unused = realpath(".", APL_lib_root);
}

void
set_APL_lib_root(const char * new_root)
{
const void * unused = realpath(new_root, APL_lib_root);
}
//-----------------------------------------------------------------------------
/**
    When APL is started as a script. then several options might be grouped
    into a single option. expand_argv() expands such grouped options into
    individual options
 **/
const char **
expand_argv(int & argc, const char ** argv)
{
bool need_expand = false;
int end_of_arg = -1;   // position of --

   // check if any argument contains spaces and remember the '--' option
   //
   loop(a, argc)
      {
        if (end_of_arg != -1)   // '--' seen
           {
             scriptname = argv[a];
             end_of_arg = -1;
           }

        if (strchr(argv[a], ' '))     need_expand = true;
        if (!strcmp(argv[a], "--"))   end_of_arg = a;
      }

   // if no argument had a space then return the original argv 
   //
   if (!need_expand)   return argv;

vector<const char *>argvec;

   end_of_arg = -1;
   loop(a, argc)
      {
        if (end_of_arg != -1)   // '--' seen
           {
             scriptname = argv[a];
             end_of_arg = -1;
           }
        end_of_arg = -1;

        if (strchr(argv[a], ' ') == 0)   // no space in this arg
           {
             argvec.push_back(argv[a]);
             if (!strcmp(argv[a], "--"))   end_of_arg = a;
             continue;
           }

        // arg has spaces, i.e. it may have multiple options
        //
        const char * arg = argv[a];

        for (;;)
            {
              while (*arg == ' ')   ++arg;   // skip leading spaces
              if (*arg == 0)        break;   // nothing left
              if (*arg == '#')      break;   // comment

              const char * sp = strchr(arg, ' ');
              if (sp == 0)   sp = arg + strlen(arg);
              const int new_len = sp - arg;   // excluding terminating 0
              if (!strncmp(arg, "--", new_len))   end_of_arg = a;

              char * new_arg = new char[new_len + 1];
              memcpy(new_arg, arg, new_len);
              new_arg[new_len] = 0;
              argvec.push_back(new_arg);
              arg = sp;
            }
      }

   argc = argvec.size();

   // the user may not be aware of how 'execve' calls an interpreter script
   // and may not have given -f as the last option in the script file.
   // In that situation argv is something like:
   //
   // "-f" "-other-options..." "script-name"
   //
   // instead of the expected:
   //
   // "-other-options..." "-f" "script-name"
   //
   // We fix this common mistake here. The downsize is, or course, that option
   // names cannnot be script names. We ignore options -h and --help since
   // they exit immediately.
   //
   for (int a = 1; a < (argc - 1); ++a)
       {
         const char * opt = argvec[a];
         const char * next = argvec[a + 1];
         if (!strcmp(opt, "-f") && ( !strcmp(next, "-d")     ||
                                     !strcmp(next, "--id")   ||
                                     !strcmp(next, "-l")     ||
                                     !strcmp(next, "--noSV") ||
                                     !strcmp(next, "-w")     ||
                                     !strcmp(next, "-T")     ||
                                     !strcmp(next, "--TM")))
            {
              for (int aa = a; aa < (argc - 2); ++aa)
                 argvec[aa] = argvec[aa + 1];
              argvec[argc - 2] = opt;   // put the -f before last
              break;   // for a...
            }
       }

const char ** ret = new const char *[argc + 1];
   loop(a, argc)   ret[a] = argvec[a];
   ret[argc] = 0;
   return ret;
}
//-----------------------------------------------------------------------------
void
show_argv(int argc, const char ** argv)
{
   CERR << "argc: " << argc << endl;
   loop(a, argc)   CERR << "  argv[" << a << "]: '" << argv[a] << "'" << endl;
}
//-----------------------------------------------------------------------------
static void
show_version(ostream & out)
{
const char * build_tag[] = { BUILDTAG, 0 };
   out << "BUILDTAG:" << endl
       << "---------" << endl;
   for (const char ** bt = build_tag; *bt; ++bt)
       {
         switch(bt - build_tag)
            {
              case 0:  out << "  Project:        ";   break;
              case 1:  out << "  Version / SVN:  ";   break;
              case 2:  out << "  Build Date:     ";   break;
              case 3:  out << "  Build OS:       ";   break;
              case 4:  out << "  config.status:  ";   break;
              default: out << "  [" << bt - build_tag << "] ";
            }
         out << *bt << endl;
       }
   out << endl;
   Output::set_color_mode(Output::COLM_OUTPUT);
}
//-----------------------------------------------------------------------------
/// print how this interpreter was configured (via ./configure) and exit
const char *
is_default(bool yes)
{
   return yes ? _(" (default)") : "";
}

static void
show_configure_options()
{
   enum { value_size = sizeof(Value),
          cell_size  = sizeof(Cell),
          header_size = value_size - cell_size * SHORT_VALUE_LENGTH_WANTED,
        };

   CERR << endl << _("configurable options:") << endl <<
                   "---------------------" << endl <<

   "    ASSERT_LEVEL_WANTED=" << ASSERT_LEVEL_WANTED
        << is_default(ASSERT_LEVEL_WANTED == 1) << endl <<

#ifdef DYNAMIC_LOG_WANTED
   "    DYNAMIC_LOG_WANTED=yes"
#else
   "    DYNAMIC_LOG_WANTED=no (default)"
#endif
   << endl <<

#ifdef ENABLE_NLS
   "    NLS enabled"
#else
   "    NLS disabled"
#endif
   << endl <<

   "    MAX_RANK_WANTED="     << MAX_RANK_WANTED
        << is_default(MAX_RANK_WANTED == 8)
   << endl <<

   "    SHORT_VALUE_LENGTH_WANTED=" << SHORT_VALUE_LENGTH_WANTED
        << is_default(SHORT_VALUE_LENGTH_WANTED == 1)
        << _(", therefore:") << endl <<
   "        sizeof(Value)       : "  << value_size  << " bytes" << endl <<
   "        sizeof(Cell)        :  " << cell_size   << " bytes" << endl <<
   "        sizeof(Value header): "  << header_size << " bytes" << endl <<
   endl <<

#ifdef VALUE_CHECK_WANTED
   "    VALUE_CHECK_WANTED=yes"
#else
   "    VALUE_CHECK_WANTED=no (default)"
#endif
   << endl <<

#ifdef VF_TRACING_WANTED
   "    VF_TRACING_WANTED=yes"
#else
   "    VF_TRACING_WANTED=no (default)"
#endif
   << endl;


   show_version(CERR);
}
//-----------------------------------------------------------------------------
/// print possible command line options and exit
void
usage(const char * prog)
{
const char * prog1 = strrchr(prog, '/');
   if (prog1)   ++prog1;
   else         prog1 = prog;

char cc[4000];
   snprintf(cc, sizeof(cc), 

_("usage: %s [options]\n"
"    options: \n"
"    -h, --help           print this help\n"
"    -d                   run in the background (i.e. as daemon)\n"
"    -f file              read input from file (after all -T files)\n"
"    --id proc            use processor ID proc (default: first unused > 1000)\n"), prog);
   CERR << cc;

#ifdef DYNAMIC_LOG_WANTED
   snprintf(cc, sizeof(cc), 
_("    -l num               turn log facility num (1-%d) ON\n"), LID_MAX-1);
   CERR << cc;

#endif

   snprintf(cc, sizeof(cc), _(
"    --par proc           use processor parent ID proc (default: no parent)\n"
"    -w milli             wait milli milliseconds at startup\n"
"    --noCIN              do not echo input(for scripting)\n"
"    --noCONT             do not load CONTINUE workspace on startup)\n"
"    --noSV               do not start APnnn (a shared variable server)\n"
"    --cfg                show ./configure options used and exit\n"
"    --gpl                show license (GPL) and exit\n"
"    --silent             do not show the welcome message\n"
"    -s, --script         same as --silent --noCIN --noCONT -f -\n"
"    -v, --version        show version information and exit\n"
"    -T testcases ...     run testcases\n"
"    --TM mode            test mode (for -T files):\n"
"                         0:   (default) exit after last testcase\n"
"                         1:   exit after last testcase if no error\n"
"                         2:   continue (don't exit) after last testcase\n"
"                         3:   stop testcases after first error (don't exit)\n"
"                         4:   exit after first error\n"
"    --TS                 append to (rather than override) summary.log\n"
"    --                   end of options for %s\n"), prog1);
   CERR << cc << endl;
}
//-----------------------------------------------------------------------------
/// print the GPL
static void
show_GPL(ostream & out)
{
   out <<
"\n"
"---------------------------------------------------------------------------\n"
"\n"
"    This program is GNU APL, a free implementation of the\n"
"    ISO/IEC Standard 13751, \"Programming Language APL, Extended\"\n"
"\n"
"    Copyright (C) 2008-2013  Dr. Jürgen Sauermann\n"
"\n"
"    This program is free software: you can redistribute it and/or modify\n"
"    it under the terms of the GNU General Public License as published by\n"
"    the Free Software Foundation, either version 3 of the License, or\n"
"    (at your option) any later version.\n"
"\n"
"    This program is distributed in the hope that it will be useful,\n"
"    but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"    GNU General Public License for more details.\n"
"\n"
"    You should have received a copy of the GNU General Public License\n"
"    along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"
"\n"
"---------------------------------------------------------------------------\n"
"\n";
}
//-----------------------------------------------------------------------------
/// print a welcome message (copyright notice)
static void
show_welcome(ostream & out)
{
char c1[200];
char c2[200];
   snprintf(c1, sizeof(c1), _("Welcome to GNU APL version %s"),PACKAGE_VERSION);
   snprintf(c2, sizeof(c2), _("for details run: %s --gpl."), progname);

const char * lines[] =
{
  "",
  c1,
  "",
  "Copyright (C) 2008-2013  Dr. Jürgen Sauermann",
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
int
main(int argc, const char * _argv[])
{
const char ** argv = expand_argv(argc, _argv);
bool do_CONT = true;   // load workspace CONTINUE on start-up
int requested_id = 0;

   Quad_ARG::argc = argc;
   Quad_ARG::argv = argv;

   progname = argv[0];
   set_APL_bin_path(progname);
   set_APL_lib_root();

   // struct sigaction differs between GNU/Linux and other systems, which
   // causes direct bracket assignment to not compile on some machines.
   //
   // We therefore memset everything to 0 and then set the handler (which
   // should be compatible on GNU/Linux and other systems.
   //
   memset(&new_control_C_action, 0, sizeof(struct sigaction));
   memset(&new_USR1_action,      0, sizeof(struct sigaction));
   memset(&new_segfault_action,  0, sizeof(struct sigaction));

   new_control_C_action.sa_handler = &control_C;
   new_USR1_action     .sa_handler = &signal_USR1_handler;
   new_segfault_action .sa_handler = &seg_fault;

   sigaction(SIGINT,  &new_control_C_action, &old_control_C_action);
   sigaction(SIGUSR1, &new_USR1_action,      &old_USR1_action);
// sigaction(SIGSEGV, &new_segfault_action,  &old_segfault_action);

   init();

Workspace w;
bool do_sv = true;
bool daemon = false;
bool append_summary = false;
int wait_ms = 0;
int option_errors = 0;

   // init NLS so that usage() will be translated
   //
#ifdef ENABLE_NLS
   init_NLS();
#endif

   for (int a = 1; a < argc; )
       {
         const char * opt = argv[a++];
         const char * val = (a < argc) ? argv[a] : 0;

         if (!strcmp(opt, "--"))   // end of arguments
            {
              a = argc;
              break;
            }
         else if (!strcmp(opt, "--cfg"))
            {
              show_configure_options();
              return 0;
            }
         else if (!strcmp(opt, "-d"))
            {
              daemon = true;
            }
         else if (!strcmp(opt, "-f"))
            {
              ++a;
              if (val)   Input::input_file_name = val;
              else
                 {
                   CERR << _("-f without filename") << endl;
                   return 1;
                 }
            }
         else if (!strcmp(opt, "--gpl"))
            {
              show_GPL(cout);
              return 0;
            }
         else if (!strcmp(opt, "-h") || !strcmp(opt, "--help"))
            {
              usage(argv[0]);
              return 0;
            }
         else if (!strcmp(opt, "--id"))
            {
              // skip this option (handled by ProcessorID::init() below)
              //
              requested_id = val ? atoi(val) : 0;
              ++a;
              continue;
            }
#ifdef DYNAMIC_LOG_WANTED
         else if (!strcmp(opt, "-l"))
            {
              ++a;
              if (val)   Log_control(LogId(atoi(val)), true);
              else
                 {
                   CERR << _("-l without log facility") << endl;
                   return 1;
                 }
            }
#endif // DYNAMIC_LOG_WANTED
         else if (!strcmp(opt, "--noCIN"))
            {
              do_not_echo = true;
            }
         else if (!strcmp(opt, "--noCONT"))
            {
              do_CONT = false;
            }
         else if (!strcmp(opt, "--noSV"))
            {
              do_sv = false;
            }
         else if (!strcmp(opt, "--par"))
            {
              // skip this option (handled by ProcessorID::init() below)
              //
              ++a;
              continue;
            }
         else if (!strcmp(opt, "-s") || !strcmp(opt, "--script"))
            {
              do_CONT = false;                // --noCONT
              do_not_echo = true;             // -noCIN
              silent = true;                  // --silent
              Input::input_file_name = "-";   // -f -
            }
         else if (!strcmp(opt, "--silent"))
            {
              silent = true;
            }
         else if (!strcmp(opt, "-T"))
            {
              do_CONT = false;

              // TestFiles::open_next_testfile() will exit if it sees "-"
              //
              TestFiles::need_total = true;
              for (; a < argc; ++a)
                  {
                    if (!strcmp(argv[a], "--"))   // end of arguments
                       {
                         a = argc;
                         break;
                         
                       }
                    TestFiles::test_file_names.push_back(argv[a]);
                  }

              // 
              if (TestFiles::is_validating())
                 {
                   // truncate summary.log, unless append_summary is desired
                   //
                   if (!append_summary)
                      {
                        ofstream summary("testcases/summary.log",
                                         ios_base::trunc);
                      }
                   TestFiles::open_next_testfile();
                 }
            }
         else if (!strcmp(opt, "--TM"))
            {
              ++a;
              if (val)
                 {
                   const int mode = atoi(val);
                   TestFiles::test_mode = TestFiles::TestMode(mode);
                 }
              else
                 {
                   CERR << _("--TM without test mode") << endl;
                   return 1;
                 }
            }
         else if (!strcmp(opt, "--TS"))
            {
              append_summary = true;
            }
         else if (!strcmp(opt, "-v") || !strcmp(opt, "--version"))
            {
              show_version(cout);
              return 0;
            }
         else if (!strcmp(opt, "-w"))
            {
              ++a;
              if (val)   wait_ms = atoi(val);
              else
                 {
                   CERR << _("-w without milli(seconds)") << endl;
                   return 1;
                 }
            }
         else
            {
              CERR << _("unknown option '") << opt << "'" << endl;
              usage(argv[0]);
              return 1;
            }
       }

   if (daemon)
      {
        if (fork())   return 0;   // parent returns
        Log(LOG_startup)   CERR << "process forked" << endl;
      }

   if (wait_ms)   usleep(1000*wait_ms);

   if (!silent)   show_welcome(cout);

   Log(LOG_startup)   CERR << "PID is " << getpid() << endl;
   Log(LOG_argc_argv)   show_argv(argc, argv);

   TestFiles::testcase_count = TestFiles::test_file_names.size();

   if (ProcessorID::init(argc, argv, do_sv))
      {
        COUT << _("*** Another APL interpreter with --id ")
             << requested_id <<  _(" is already running") << endl;
        return 4;
      }

   if (do_CONT)
      {
         UCS_string cont("CONTINUE.xml");
         vector<UCS_string> lib_file;
         lib_file.push_back(cont);
         UTF8_string path = Command::get_lib_file_path(lib_file);
         if (access((const char *)path.c_str(), F_OK) == 0)
            {
              UCS_string load_cmd(")LOAD CONTINUE");
              Command::process_line(load_cmd);
            }
      }

   for (;;)
       {
         Token t = w.immediate_execution(
                   TestFiles::test_mode == TestFiles::TM_EXIT_AFTER_ERROR);
         if (t.get_tag() == TOK_OFF)   Command::cmd_OFF(0);
       }

   /* not reached */
}
//-----------------------------------------------------------------------------
