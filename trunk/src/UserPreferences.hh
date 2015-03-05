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

#ifndef __USER_PREFERENCES_HH_DEFINED__
#define __USER_PREFERENCES_HH_DEFINED__

#include <vector>

#include "Parallel.hh"
#include "UTF8_string.hh"

/// a structure that contains user preferences from different sources
/// (command line arguments, config files, environment variables ...)
struct UserPreferences
{
   UserPreferences()
   : silent(false),
     emacs_mode(false),
     emacs_arg(0),
     do_not_echo(false),
     safe_mode(false),
     user_do_svars(true),
     system_do_svars(true),
     do_CONT(true),
     do_Color(true),
     requested_id(0),
     requested_par(0),
     requested_cc(CCNT_UNKNOWN),
     daemon(false),
     append_summary(false),
     wait_ms(0),
     randomize_testfiles(false),
     user_profile(0),
     backup_before_save(false),
     script_argc(0),
     line_history_path(".apl.history"),
     line_history_len(500),
     nabla_to_history(1),   // if function was modified
     control_Ds_to_exit(0),
     raw_cin(false)
   {}

   /// read a \b preference file and update parameters set there
   void read_config_file(bool sys, bool log_startup);

   /// read a \b parallel_thresholds file and update parameters set there
   void read_threshold_file(bool sys, bool log_startup);

   /// print possible command line options and exit
   static void usage(const char * prog);

   /// return " (default)" if yes is true
   static const char * is_default(bool yes)
      { return yes ? " (default)" : ""; }

   /// print how the interpreter was configured (via ./configure) and exit
   static void show_configure_options();

   /// print the GPL
   static void show_GPL(ostream & out);

   /// show version information
   static void show_version(ostream & out);

   /// parse command line parameters (before reading preference files)
   bool parse_argv_1();

   /// parse command line parameters (after reading preference files)
   void parse_argv_2(bool logit);

   /// expand lumped arguments
   void expand_argv(int argc, const char ** argv);

   /// argv/argc at startup
   vector<const char *>original_argv;

   /// argv/argc after expand_argv
   vector<const char *>expanded_argv;

   /// true if no banner/Goodbye is wanted.
   bool silent;

   /// true if emacs mode is wanted
   bool emacs_mode;

   /// an argument for emacs mode
   const char * emacs_arg;

   /// true if no banner/Goodbye is wanted.
   bool do_not_echo;

   /// true if --safe command line option was given
   bool safe_mode;

   /// true if shared variables are wanted by the user
   bool user_do_svars;

   /// true if shared variables are enabled by the system. This is initially
   /// the same as user_do_svars, but can become false if something goes wrong
   bool system_do_svars;

   /// load workspace CONTINUE on start-up
   bool do_CONT;

  /// output coloring enabled
   bool do_Color;

   /// desired --id (⎕AI[1] and shared variable functions)
   int requested_id;

   /// desired --par (⎕AI[1] and shared variable functions)
   int requested_par;

   /// desired core count
   CoreCount requested_cc;

   /// run as deamon
   bool daemon;

   /// append test results to summary.log rather than overriding it
   bool append_summary;

   /// wait at start-up
   int wait_ms;

   /// randomize the order of testfiles
   bool randomize_testfiles;

   /// the profile to be used (in the preferences file)
   int user_profile;

   /// something to be executed at startup (--LX)
   UTF8_string latent_expression;

   /// a workspace to be loaded at startup
   UTF8_string initial_workspace;

   /// backup on )SAVE
   bool backup_before_save;

   /// the argument number of the APL script name (if run from a script)
   /// in expanded_argv, or 0 if apl is started directly.
   int script_argc;

   /// location of the input line history
   UTF8_string line_history_path;

   /// number of lines in the input line history
   int line_history_len;

   /// when function body shall go into the history
   int nabla_to_history;

   /// name of a user-provided keyboard layout file
   UTF8_string keyboard_layout_file;

   /// number of control-Ds to exit (0 = never)
   int control_Ds_to_exit;

   /// send no ESC sequences on stderr
   bool raw_cin;

protected:
   /// open a user-supplied config file (in $HOME or gnu-apl.d)
   FILE * open_user_file(const char * fname, char * opened_filename,
                         bool sys, bool log_startup);

   /// set the parallel threshold of function \b fun to \b threshold
   static void set_threshold(Function & fun, int padic, int macn,
                             ShapeItem threshold);

   /// return true if file \b filename is an APL script (has execute permission
   /// and starts with #!
   static bool is_APL_script(const char * filename);
};

extern UserPreferences uprefs;

#endif // __USER_PREFERENCES_HH_DEFINED__
