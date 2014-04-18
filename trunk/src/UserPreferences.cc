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
#include <sys/time.h>

#include "../config.h"

#include "buildtag.hh"
static const char * build_tag[] = { BUILDTAG, 0 };

#include "Common.hh"
#include "Input.hh"
#include "TestFiles.hh"
#include "UserPreferences.hh"
#include "Value.hh"

UserPreferences uprefs;

//-----------------------------------------------------------------------------
void
UserPreferences::open_current_file()
{
   if (files_todo.size() && files_todo[0].file == 0)
      {
        files_todo[0].file = fopen(current_filename(), "r");
        files_todo[0].line_no = 0;
      }
}
//-----------------------------------------------------------------------------

void
UserPreferences::close_current_file()
{
   if (files_todo.size() && files_todo[0].file)
      {
        fclose(files_todo[0].file);
        files_todo[0].file = 0;
        files_todo[0].line_no = -1;
      }
}
//-----------------------------------------------------------------------------
void
UserPreferences::usage(const char * prog)
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
"    -f file ...          read APL input from file(s)\n"
"    --id proc            use processor ID proc (default: first unused > 1000)\n"), prog);
   CERR << cc;

#ifdef DYNAMIC_LOG_WANTED
   snprintf(cc, sizeof(cc),
_("    -l num               turn log facility num (1-%d) ON\n"), LID_MAX - 1);
   CERR << cc;
#endif

#if CORE_COUNT_WANTED == -2
   CERR <<
_("    --cc count           use count cores (default: all)\n");
#endif

   snprintf(cc, sizeof(cc), _(
"    --par proc           use processor parent ID proc (default: no parent)\n"
"    -w milli             wait milli milliseconds at startup\n"
"    --noCIN              do not echo input(for scripting)\n"
"    --rawCIN             do not use the readline lib for input\n"
"    --[no]Color          start with ]XTERM ON [OFF])\n"
"    --noCONT             do not load CONTINUE workspace on startup)\n"
"    --emacs              run in emacs mode\n"
"    --[no]SV             [do not] start APnnn (a shared variable server)\n"
"    --cfg                show ./configure options used and exit\n"
"    --gpl                show license (GPL) and exit\n"
"    --silent             do not show the welcome message\n"
"    --safe               safe mode (no shared vars, no native functions)\n"
"    -s, --script         same as --silent --noCIN --noCONT --noColor -f -\n"
"    -v, --version        show version information and exit\n"
"    -T testcases ...     run testcases\n"
"    --TM mode            test mode (for -T files):\n"
"                         0:   (default) exit after last testcase\n"
"                         1:   exit after last testcase if no error\n"
"                         2:   continue (don't exit) after last testcase\n"
"                         3:   stop testcases after first error (don't exit)\n"
"                         4:   exit after first error\n"
"    --TR                 randomize order of testfiles\n"
"    --TS                 append to (rather than override) summary.log\n"
"    --                   end of options for %s\n"), prog1);
   CERR << cc << endl;
}
//-----------------------------------------------------------------------------
void
UserPreferences::show_GPL(ostream & out)
{
   out <<
"\n"
"---------------------------------------------------------------------------\n"
"\n"
"    This program is GNU APL, a free implementation of the\n"
"    ISO/IEC Standard 13751, \"Programming Language APL, Extended\"\n"
"\n"
"    Copyright (C) 2008-2014  Dr. Jürgen Sauermann\n"
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
void
UserPreferences::parse_argv(int argc, const char * argv[])
{
   // execve() puts the script name (and optional args at the end of argv.
   // For example, argv might look like this:
   //
   // /usr/bin/apl -apl-options... -- ./scriptname -script-options
   //
   // If a script name is present then strip it off and remember its place.
   //
int script_argc = argc;   // $0 of the apl script
   for (int a = argc - 2; a >= 0; --a)
       {
         if (!strcmp(argv[a], "--"))   // found script name
            {
              script_argc = a + 1;
              argc = a;
              Filename_and_mode fam =
                 { UTF8_string(argv[script_argc]), 0, false, !do_not_echo };
              files_todo.push_back(fam);
            }
       }


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
              exit(0);
            }
         else if (!strcmp(opt, "--Color"))
            {
              do_Color = true;
            }
         else if (!strcmp(opt, "--noColor"))
            {
              do_Color = false;
            }
         else if (!strcmp(opt, "-d"))
            {
              daemon = true;
            }
         else if (!strcmp(opt, "-f"))
            {
              for (; a < argc; ++a)
                  {
                    if (!strcmp(argv[a], "--"))   // end of -f arguments
                       {
                         a = argc;
                         break;

                       }

                    const char * filename = argv[a];
                    if (!strcmp(argv[a], "-"))   // "-" shall mean  stdin
                       {
                         if (script_argc < argc)   filename = argv[script_argc];
                         else                      filename = "-";
                       }
                    Filename_and_mode fam =
                             { UTF8_string(filename), 0, false, !do_not_echo };
                    files_todo.push_back(fam);
                  }
            }
         else if (!strcmp(opt, "--gpl"))
            {
              show_GPL(cout);
              exit(0);
            }
         else if (!strcmp(opt, "-h") || !strcmp(opt, "--help"))
            {
              usage(argv[0]);
              exit(0);
            }
         else if (!strcmp(opt, "--id"))
            {
              ++a;
              if (!val)
                 {
                   CERR << "--id without processor number" << endl;
                   exit(2);
                 }

              requested_id = atoi(val);
            }
         else if (!strcmp(opt, "-l"))
            {
              ++a;
#ifdef DYNAMIC_LOG_WANTED
              if (val)   Log_control(LogId(atoi(val)), true);
              else
                 {
                   CERR << _("-l without log facility") << endl;
                   exit(3);
                 }
#else
   if (val && atoi(val) == LID_startup)   ;
   else  CERR << _("the -l option was ignored (requires ./configure "
                   "DYNAMIC_LOG_WANTED=yes)") << endl;
#endif // DYNAMIC_LOG_WANTED
            }
         else if (!strcmp(opt, "--noCIN"))
            {
              do_not_echo = true;
            }
         else if (!strcmp(opt, "--rawCIN"))
            {
              Input::use_readline = false;
            }
         else if (!strcmp(opt, "--noCONT"))
            {
              do_CONT = false;
            }
         else if (!strcmp(opt, "--emacs"))
            {
              emacs_mode = true;
            }
         else if (!strcmp(opt, "--SV"))
            {
              do_svars = true;
            }
         else if (!strcmp(opt, "--noSV"))
            {
              do_svars = false;
            }
#if CORE_COUNT_WANTED == -2
         else if (!strcmp(opt, "--cc"))
            {
              ++a;
              if (!val)
                 {
                   CERR << "--cc without core count" << endl;
                   exit(4);
                 }
              requested_cc = (CoreCount)atoi(val);
            }
#endif
         else if (!strcmp(opt, "--par"))
            {
              ++a;
              if (!val)
                 {
                   CERR << "--par without processor number" << endl;
                   exit(5);
                 }
              requested_par = atoi(val);
            }
         else if (!strcmp(opt, "-s") || !strcmp(opt, "--script"))
            {
              do_CONT = false;             // --noCONT
              do_Color = false;            // --noColor
              do_not_echo = true;             // -noCIN
              silent = true;                  // --silent
            }
         else if (!strcmp(opt, "--silent"))
            {
              silent = true;
            }
         else if (!strcmp(opt, "--safe"))
            {
              safe_mode = true;
              do_svars = false;
            }
         else if (!strcmp(opt, "-T"))
            {
              do_CONT = false;

              // TestFiles::open_next_testfile() will exit if it sees "-"
              //
              TestFiles::need_total = true;
              for (; a < argc; ++a)
                  {
                    if (!strcmp(argv[a], "--"))   // end of -T arguments
                       {
                         a = argc;
                         break;

                       }
                    Filename_and_mode fam =
                             { UTF8_string(argv[a]), 0, true, true };
                    files_todo.push_back(fam);
                  }

              // 
              if (is_validating())
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
                   exit(6);
                 }
            }
         else if (!strcmp(opt, "--TR"))
            {
              randomize_testfiles = true;
            }
         else if (!strcmp(opt, "--TS"))
            {
              append_summary = true;
            }
         else if (!strcmp(opt, "-v") || !strcmp(opt, "--version"))
            {
              show_version(cout);
              exit(0);
            }
         else if (!strcmp(opt, "-w"))
            {
              ++a;
              if (val)   wait_ms = atoi(val);
              else
                 {
                   CERR << _("-w without milli(seconds)") << endl;
                   exit(7);
                 }
            }
         else
            {
              CERR << _("unknown option '") << opt << "'" << endl;
              usage(argv[0]);
              exit(8);
            }
       }

   if (randomize_testfiles)   randomize_files();

   // count number of testfiles
   //
   TestFiles::testcase_count = 0;
   loop(f, files_todo.size())
      {
        if (files_todo[f].test)   ++TestFiles::testcase_count;
      }
}
//-----------------------------------------------------------------------------
/**
    When APL is started as a script. then several options might be grouped
    into a single option (and one string argv[a] can contain several options
    separated by spaces.

    expand_argv() expands such grouped options into individual options.

    expand_argv() also checks for -l LID_startup so that startup messages
    emitted before parsing the command line options become visioble (provided
    that dynamic logging is on).
 **/
const char **
UserPreferences::expand_argv(int & argc, const char ** argv, bool & log_startup)
{
bool need_expand = false;
int end_of_arg = -1;   // position of --

   // check for -l LID_startup
   loop(a, argc - 1)
      {
        if (!strcmp(argv[a], "--"))   break;
        if (!strcmp(argv[a], "-l") && atoi(argv[a + 1]) == LID_startup)
           {
             log_startup = true;
             break;
           }
      }

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
   // We fix this common mistake here. The downside is, or course, that option
   // names cannnot be script names. We ignore options -h and --help since
   // they exit immediately.
   //
   for (int a = 1; a < (argc - 1); ++a)
       {
         const char * opt = argvec[a];
         const char * next = argvec[a + 1];
         if (!strcmp(opt, "-f") && ( !strcmp(next, "-d")        ||
                                     !strcmp(next, "--id")      ||
                                     !strcmp(next, "-l")        ||
                                     !strcmp(next, "--noCIN")   ||
                                     !strcmp(next, "--noCONT")  ||
                                     !strcmp(next, "--Color")   ||
                                     !strcmp(next, "--noColor") ||
                                     !strcmp(next, "--SV")      ||
                                     !strcmp(next, "--noSV")    ||
                                     !strcmp(next, "--par")     ||
                                     !strcmp(next, "-s")        ||
                                     !strcmp(next, "--script")  ||
                                     !strcmp(next, "--silent")  ||
                                     !strcmp(next, "-T")        ||
                                     !strcmp(next, "--TM")      ||
                                     !strcmp(next, "-w")))
            {
              // there is another known option after -f
              //
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
UserPreferences::randomize_files()
{
   {
     timeval now;
     gettimeofday(&now, 0);
     srandom(now.tv_sec + now.tv_usec);
   }

   // check that all file are test files
   //
   loop(f, files_todo.size())
      {
        if (files_todo[f].test == false)   // not a test file
           {
             CERR << "Cannot randomise testfiles when a mix of -T"
                     " and -f is used (--TR ignored)" << endl;
             return;
           }
      }

   for (int done = 0; done < 4*files_todo.size();)
       {
         const int n1 = random() % files_todo.size();
         const int n2 = random() % files_todo.size();
         if (n1 == n2)   continue;

         Filename_and_mode f1 = files_todo[n1];
         Filename_and_mode f2 = files_todo[n2];

         const char * ff1 = strrchr(f1.filename.c_str(), '/');
         if (!ff1++)   ff1 = f1.filename.c_str();
         const char * ff2 = strrchr(f2.filename.c_str(), '/');
         if (!ff2++)   ff2 = f2.filename.c_str();

         // the order of files named AAA... or ZZZ... shall not be changed
         //
         if (strncmp(ff1, "AAA", 3) == 0)   continue;
         if (strncmp(ff1, "ZZZ", 3) == 0)   continue;
         if (strncmp(ff2, "AAA", 3) == 0)   continue;
         if (strncmp(ff2, "ZZZ", 3) == 0)   continue;

         // at this point f1 and f2 shall be swapped.
         //
         files_todo[n1] = f2;
         files_todo[n2] = f1;
         ++done;
       }
}
//-----------------------------------------------------------------------------
void
UserPreferences::show_version(ostream & out)
{
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

   out << "  Readline:       " << HEX4(Input::readline_version()) << endl
       << endl;

   Output::set_color_mode(Output::COLM_OUTPUT);
}
//-----------------------------------------------------------------------------
void
UserPreferences::show_configure_options()
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

#ifdef HAVE_LIBREADLINE
   "    libreadline is used (default)"
#else
   "    libreadline is not used (disabled or not present)"
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

#ifdef VALUE_HISTORY_WANTED
   "    VALUE_HISTORY_WANTED=yes"
#else
   "    VALUE_HISTORY_WANTED=no (default)"
#endif
   << endl <<

   "    CORE_COUNT_WANTED=" << CORE_COUNT_WANTED <<
#if   CORE_COUNT_WANTED == -3
   "  (⎕SYL)"
#elif CORE_COUNT_WANTED == -2
   "  (argv (--cc))"
#elif CORE_COUNT_WANTED == -1
   "  (all)"
#elif CORE_COUNT_WANTED == 0
   "  (default: (sequential))"
#endif
   << endl <<

#ifdef VF_TRACING_WANTED
   "    VF_TRACING_WANTED=yes"
#else
   "    VF_TRACING_WANTED=no (default)"
#endif
   << endl <<

#ifdef VISIBLE_MARKERS_WANTED
   "    VISIBLE_MARKERS_WANTED=yes"
#else
   "    VISIBLE_MARKERS_WANTED=no (default)"
#endif
   << endl

   << endl;


   show_version(CERR);
}
//-----------------------------------------------------------------------------

