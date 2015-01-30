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

#include <stdlib.h>

#include "Common.hh"
#include "Command.hh"
#include "IndexExpr.hh"
#include "InputFile.hh"
#include "IO_Files.hh"
#include "Output.hh"
#include "UTF8_string.hh"
#include "Workspace.hh"

int IO_Files::testcase_count = 0;
int IO_Files::testcases_done = 0;
int IO_Files::total_errors = 0;
int IO_Files::apl_errors = 0;
const char * IO_Files::last_apl_error_loc = "";
int IO_Files::last_apl_error_line = -1;
int IO_Files::assert_errors = 0;
int IO_Files::diff_errors = 0;
int IO_Files::parse_errors = 0;
IO_Files::TestMode IO_Files::test_mode = TM_EXIT_AFTER_LAST;
bool IO_Files::need_total = false;
ofstream IO_Files::current_testreport;

//-----------------------------------------------------------------------------
void
IO_Files::get_file_line(UTF8_string & line, bool & eof)
{
   while (InputFile::current_file())   // as long as we have input files
      {
        if (!InputFile::current_file()->file)
           {
             open_next_file();
             if (!InputFile::current_file())         break;   // no more files
             if (!InputFile::current_file()->file)   break;   // no more files
           }

        // read a line with CR and LF removed.
        //
        eof = false;
        read_file_line(line, eof);
        if (eof)   // end of file reached: do some global checks
           {
             if (InputFile::current_file() &&
                 InputFile::current_file()->with_LX)
                {
                  InputFile::current_file()->with_LX = false;
                  UCS_string LX = Workspace::get_LX();
                  if (LX.size())   // ⎕LX pending
                     {
                       LX.append_utf8(" ⍝ ⎕LX");
                       line = UTF8_string(LX);
                       eof = false;
                       return;
                     }
                }
             if (end_of_current_file())   continue;   // try again.
              else                        break;      // done
           }

        current_testreport << "----> " << line.c_str() << endl;
        return;
      }

   // arrive here when all testfiles have been read.
   // Maybe print a testcase total summary
   //
   if (need_total)   // we had a -T option
      {
        print_summary();
        need_total = false;   // forget the -T option

        if ((test_mode == TM_EXIT_AFTER_LAST) ||
            (test_mode == TM_EXIT_AFTER_LAST_IF_OK && !error_count()))
          {
            CERR << "Exiting (test_mode " << test_mode << ")" << endl;
            cleanup(true);
            if (total_errors)   Command::cmd_OFF(1);
            else                Command::cmd_OFF(0);
          }
      }

   eof = true;
}
//-----------------------------------------------------------------------------
void
IO_Files::read_file_line(UTF8_string & file_line, bool & eof)
{
   for (;;)
       {
         int cc = fgetc(InputFile::current_file()->file);
         if (cc == EOF)   // end of file
            {
              if (file_line.size())   break;   // EOF, but we have chars

              eof = true;
              return;
            }

         if (cc == '\n' || cc == 2)   // end of line or ^B
            {
              break;
            }

         if (cc == '\r')   continue;   // ignore carrige returns

          file_line.append((UTF8)cc);
       }

   Log(LOG_test_execution)
      CERR << "read_file_line() -> " << file_line << endl;
}
//-----------------------------------------------------------------------------
void
IO_Files::next_file()
{
   if (InputFile::current_file() &&
       InputFile::current_file()->file)
      {
        end_of_current_file();
      }
   else
      {
        CERR << "]NEXTFILE: no file" << endl;
      }
   open_next_file();
}
//-----------------------------------------------------------------------------
bool
IO_Files::end_of_current_file()
{
   if (InputFile::is_validating())   // running a .tc file
      {
        // we expect )SI to be clear after a testcase has finished and
        // complain if it is not.
        //
        if (Workspace::SI_entry_count() > 1)
           {
             CERR << endl << ")SI not cleared at the end of "
                  << InputFile::current_filename() << ":" << endl;
             Workspace::list_SI(CERR, SIM_SIS);
             CERR << endl;

             if (current_testreport.is_open())
                {
                  current_testreport << endl
                                     << ")SI not cleared at the end of "
                                     << InputFile::current_filename()<< ":"
                                     << endl;
                  Workspace::list_SI(current_testreport, SIM_SIS);
                  current_testreport << endl;
                }
             apl_error(LOC);
           }

        // check for stale values and indices
        //
        if (current_testreport.is_open())
           {
             if (Value::print_incomplete(current_testreport))
                {
                  current_testreport
                     << " (automatic check for incomplete values failed)"
                     << endl;
                  apl_error(LOC);
                }

             if (Value::print_stale(current_testreport))
                {
                  current_testreport
                     << " (automatic check for stale values failed,"
                        " offending Value erased)." << endl;

                  apl_error(LOC);
                  Value::erase_stale(LOC);
                }

             if (IndexExpr::print_stale(current_testreport))
                {
                  current_testreport
                     << " (automatic check for stale indices failed,"
                        " offending IndexExpr erased)." << endl;
                  apl_error(LOC);
                  IndexExpr::erase_stale(LOC);
                }
           }
      }
             
   InputFile::close_current_file();
   ++testcases_done;

   Log(LOG_test_execution)
      CERR << "closed testcase file " << InputFile::current_filename()
           << endl;

   if (InputFile::is_validating())   // running a .tc file
      {
        ofstream summary("testcases/summary.log", ios_base::app);
        summary << error_count() << " ";

        if (error_count())
           {
             total_errors += error_count();
             summary << "(" << apl_errors    << " APL, ";
             if (apl_errors)
                summary << "    loc=" << last_apl_error_loc 
                        << " .tc line="  << last_apl_error_line << endl
                   << "    ";

             summary << assert_errors << " assert, "
                     << diff_errors   << " diff, "
                     << parse_errors  << " parse) ";
           }

        summary << InputFile::current_filename() << endl;

        if ((test_mode == TM_STOP_AFTER_ERROR ||
             test_mode == TM_EXIT_AFTER_ERROR) && error_count())
           {
             CERR << endl
                  << "Stopping test execution since an error has occurred"
                  << endl
                  << "The error count is " << error_count() << endl
                  << "Failed testcase is " << InputFile::current_filename()
                  << endl 
                  << endl;
                  
             InputFile::files_todo.resize(0);
             return false;
           }
      }

   InputFile::files_todo.erase(InputFile::files_todo.begin());

   Output::reset_dout();
   reset_errors();
   return true;   // continue processing
}
//-----------------------------------------------------------------------------
void
IO_Files::print_summary()
{
ofstream summary("testcases/summary.log", ios_base::app);

   summary << "======================================="
              "=======================================" << endl
           << total_errors << " errors in " << testcases_done
           << "(" << testcase_count << ")"
           << " testcase files" << endl;

   CERR    << endl
           << "======================================="
           "=======================================" << endl
           << total_errors << " errors in " << testcases_done
           << "(" << testcase_count << ")"
           << " testcase files" << endl;
}
//-----------------------------------------------------------------------------
void
IO_Files::open_next_file()
{
   if (InputFile::current_file() == 0)
      {
        CERR << "Workspace::open_next_file(): no more files" << endl;
        return;
      }

   if (InputFile::current_file()->file)
      {
        CERR << "Workspace::open_next_file(): already open" << endl;
        return;
      }

     for (;;)
         {
           if (!InputFile::current_file())   break;   // no more files

           char log_name[FILENAME_MAX];
           snprintf(log_name, sizeof(log_name) - 1,  "%s.log",
                    InputFile::current_filename());

           if (InputFile::current_file()->test)
              {
                CERR << "  #######################################"
                        "#####################################\n"
                     << " ########################################"
                        "######################################\n"
                     << " ##    Testfile: " << left << setw(60)
                     << InputFile::current_filename() << "##\n"
                     << " ##    Log file: " << setw(60) << log_name << "##\n"
                     << " ########################################"
                        "######################################" << endl
                     << "  #######################################"
                        "#####################################\n" << endl
                     << right;
              }

           InputFile::open_current_file();
           if (InputFile::current_file()->file == 0)
              {
                CERR << "could not open "
                     << InputFile::current_filename() << endl;
                InputFile::files_todo.erase(InputFile::files_todo.begin());
                continue;
              }

           Log(LOG_test_execution)
              CERR << "openened testcase file "
                   << InputFile::current_filename() << endl;

           Output::reset_dout();
           reset_errors();

           current_testreport.close();

           if (InputFile::current_file()->test)
              {
                current_testreport.open(log_name,
                                        ofstream::out | ofstream::trunc);
                if (!current_testreport.is_open())
                   {
                     CERR << "could not open testcase log file " << log_name
                          << "; producing no .log file" << endl;
                   }
              }

           return;
         }
}
//-----------------------------------------------------------------------------
void
IO_Files::expect_apl_errors(const UCS_string & arg)
{
const int cnt = arg.atoi();
   if (apl_errors == cnt)
      {
        Log(LOG_test_execution)
           CERR << "APL errors reset from (expected) " << apl_errors
                << " to 0" << endl;
        apl_errors = 0;
        last_apl_error_line = -1;
        last_apl_error_loc = "";
      }
   else
      {
        Log(LOG_test_execution)
           CERR << "*** Not reseting APL errors (got " << apl_errors
                << " expecing " << cnt << endl;
      }
}
//-----------------------------------------------------------------------------
void
IO_Files::syntax_error()
{
   if (!InputFile::current_file())         return;
   if (!InputFile::current_file()->file)   return;

   ++parse_errors;

   Log(LOG_test_execution)
      CERR << "parse errors incremented to " << parse_errors << endl;

   current_testreport << "**\n** Parse Error ********\n**" << endl;
}
//-----------------------------------------------------------------------------
void
IO_Files::apl_error(const char * loc)
{
   if (!InputFile::current_file())         return;
   if (!InputFile::current_file()->file)   return;

   ++apl_errors;
   last_apl_error_loc = loc;
   last_apl_error_line = InputFile::current_line_no();

   Log(LOG_test_execution)
      CERR << "APL errors incremented to " << apl_errors << endl;
}
//-----------------------------------------------------------------------------
void
IO_Files::assert_error()
{
   if (!InputFile::current_file())         return;
   if (!InputFile::current_file()->file)   return;

   ++assert_errors;
   Log(LOG_test_execution)
      CERR << "Assert errors incremented to " << assert_errors << endl;
}
//-----------------------------------------------------------------------------
void
IO_Files::diff_error()
{
   if (!InputFile::current_file())         return;
   if (!InputFile::current_file()->file)   return;

   ++diff_errors;
   Log(LOG_test_execution)
      CERR << "Diff errors incremented to " << diff_errors << endl;
}
//-----------------------------------------------------------------------------
