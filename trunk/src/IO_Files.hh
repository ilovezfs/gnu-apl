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

#ifndef __IO_FILES_HH_DEFINED__
#define __IO_FILES_HH_DEFINED__

#include <stdio.h>
#include <iostream>
#include <fstream>

#include "Assert.hh"
#include "UTF8_string.hh"

using namespace std;

/*
 The classes below are used to combine normal user I/O and automatic
 testcase execution. It works like this:

       (cin)
         |
         |
         V
     +---+---+             +-------------------+
     | Input | <--------   | testcase files(s) |
     +---+---+             +---------+---------+
         |                           |
         |                           |
         V                           |
      +--+--+                        |
      | APL |                        |
      +--+--+                        |
         |                           |
         |                           |
         +-----------------------+   |
         |                       |   |
         |                       V   V
         |                    +--+---+--+
         |                    | compare |
         |                    +----+----+
         |                         |
         |                         |
         V                         V
       (cout)                (test results)
 
 */

/** handling of Input files. IO_Files reads lines from files until they
    are all executed. The output of the APL interpreter is compared against
    the testcase files and mismatches are recorded.
 */
class IO_Files
{
   friend int main(int argc, const char *argv[]);
   friend struct UserPreferences;

public:
   /// count an error
   static void reset_errors()
      {
        apl_errors = 0;
        assert_errors = 0;
        diff_errors = 0;
        parse_errors = 0;
      }

   /// return the total number of errors
   static int error_count()
      { return apl_errors + assert_errors + diff_errors + parse_errors; }

   /// count and report a parse error
   static void syntax_error();

   /// reset APL errors, expecting cnt
   static void expect_apl_errors(const UCS_string & arg);

   /// count an APL error
   static void apl_error(const char * loc);

   /// count a failed assertion
   static void assert_error();

   /// count a output diff error
   static void diff_error();

   /// get one line from the current file, open the next file if necceessary
   static const UTF8 * get_file_line() ;

   /// open the next test file 
   static void open_next_file();

   /// close current testcase file (prematurely) and opem mext one
   static void next_file();

   /// return the current test report
   static ofstream & get_current_testreport()
      { return current_testreport; }

protected:
   /// dito (close files, print errors, summary etc).
   static bool end_of_current_file();

   /// how to handle test results
   static enum TestMode
      {
        /// exit() after last testcase file
        TM_EXIT_AFTER_LAST       = 0,

        /// exit() after last testcase file if no error
        TM_EXIT_AFTER_LAST_IF_OK = 1,

        /// continue in APL interpreter after last testcase file
        TM_STAY_AFTER_LAST       = 2,

        /// stop test execution after the first error (stay in APL interpreter)
        TM_STOP_AFTER_ERROR      = 3,

        /// exit() after the first error
        TM_EXIT_AFTER_ERROR      = 4,
      } test_mode;   ///< the desired test mode as per --TM n

   /// write testcases summary file
   static void print_summary();

   /// true until total error count is printed.
   static bool need_total;

   /// the number of testcases provided
   static int testcase_count;

   /// the number of testcases executed
   static int testcases_done;

   /// the number of parse errors when executing test file
   static int parse_errors;

   /// the tital number of errors in all test files
   static int total_errors;

   /// the number of APL errors when executing in current file
   static int apl_errors;

   /// the source location where the last APL error was thrown
   static const char * last_apl_error_loc;

   /// the testcase file line that has triggered the last APL error
   static int last_apl_error_line;

   /// the number of output differences when executing test file
   static int diff_errors;

   /// the number of failed assertions when executing test file
   static int assert_errors;

   /// the current test report
   static ofstream current_testreport;
};

#endif // __IO_FILES_HH_DEFINED__
