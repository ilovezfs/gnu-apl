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

#include "UTF8_string.hh"

/// a file mentioned on the command line (via -T or -f) and later on
/// added by )COPY and friends
struct Filename_and_mode
{
   UTF8_string  filename;   ///< dito.
   FILE       * file;       /// file descriptor
   bool         test;       ///< true for -T testfile, false for -f APLfile
   bool         echo;       ///< echo stdin
   int          line_no;    ///< line number in file
   bool         is_script;  /// 
};

/// a structure that contains user preferences from different sources
/// (command line arguments, config files, environment variables ...)
struct UserPreferences
{
   UserPreferences()
   : silent(false),
     emacs_arg(0),
     do_not_echo(false),
     safe_mode(false),
     do_svars(true),
     do_CONT(true),
     do_Color(true),
     requested_id(0),
     requested_par(0),
     requested_cc(CCNT_UNKNOWN),
     daemon(false),
     append_summary(false),
     wait_ms(0),
     randomize_testfiles(false),
      stdin_line_no(1)
   {}

   /// read a preference file and update parameters set there
   void read_config_file(bool sys, bool log_startup);

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

   /// parse command line parameters
   void parse_argv(int argc, const char * argv[]);

   /// expand lumped arguments
   static const char ** expand_argv(int & argc, const char ** argv,
                                    bool & log_startup);

   /// true if no banner/Goodbye is wanted.
   bool silent;

   /// true if emacs mode is wanted
   const char * emacs_arg;

   /// true if no banner/Goodbye is wanted.
   bool do_not_echo;

   /// true if --safe command line option was given
   bool safe_mode;

   /// true if shared variables are enabled
   bool do_svars;

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

   /// return true if the current input file (if any) is a test file
   bool is_validating()
      { return files_todo.size() > 0 && files_todo[0].test; }

   /// randomize the order of test_file_names
   void randomize_files();

   /// files that need to be processed
   vector<Filename_and_mode> files_todo;

   /// the current file
   Filename_and_mode * current_file()
      { return files_todo.size() ? &files_todo[0] : 0; }

   /// the name of the currrent file
   const char * current_filename()
      { return files_todo.size() ? files_todo[0].filename.c_str() : "stdin"; }

   /// the line number of the currrent file
   int current_line_no() const
      { return files_todo.size() ? files_todo[0].line_no : stdin_line_no; }

   void increment_current_line_no()
      { if (files_todo.size()) ++files_todo[0].line_no; else ++stdin_line_no; }

   void open_current_file();

   void close_current_file();

   bool echo_current_file() const
      { return (files_todo.size()) ? files_todo[0].echo : !do_not_echo; }

   bool running_script() const
      { return files_todo.size() > 0 && files_todo[0].is_script; }

protected:
   int stdin_line_no;
};

extern UserPreferences uprefs;

#endif // __USER_PREFERENCES_HH_DEFINED__
