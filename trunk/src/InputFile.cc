/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2016  Dr. Jürgen Sauermann

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

#include <limits.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "../config.h"

#include "Common.hh"
#include "InputFile.hh"
#include "IO_Files.hh"
#include "PrintOperator.hh"
#include "UserFunction.hh"
#include "UserPreferences.hh"

vector<InputFile> InputFile::files_todo;
int InputFile::stdin_line_no = 1;

//-----------------------------------------------------------------------------
void
InputFile::open_current_file()
{
   if (files_todo.size() && files_todo[0].file == 0)
      {
        if (!strcmp(current_filename(), "-"))
           files_todo[0].file = stdin;
        else if (!strcmp(current_filename(), "stdin"))
           files_todo[0].file = stdin;
        else
             files_todo[0].file = fopen(current_filename(), "r");

        files_todo[0].line_no = 0;
      }
}
//-----------------------------------------------------------------------------
void
InputFile::close_current_file()
{
   if (files_todo.size() && files_todo[0].file)
      {
        if (files_todo[0].file != stdin)
           {
             fclose(files_todo[0].file);
             files_todo[0].file = 0;
             files_todo[0].line_no = -1;
           }
      }
}
//-----------------------------------------------------------------------------
void
InputFile::randomize_files()
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

   for (size_t done = 0; done < 4*files_todo.size();)
       {
         const int n1 = random() % files_todo.size();
         const int n2 = random() % files_todo.size();
         if (n1 == n2)   continue;

         InputFile f1 = files_todo[n1];
         InputFile f2 = files_todo[n2];

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
bool
InputFile::echo_current_file()
{
   if (files_todo.size()) return files_todo[0].echo;
   return uprefs.echo_CIN || !uprefs.do_not_echo;
}
//-----------------------------------------------------------------------------
bool
InputFile::check_filter(const UTF8_string & line)
{
bool ret = in_matched;
UCS_string ucs_line(line);
   ucs_line.remove_lt_spaces();   // remove leading and trailing spaces

   if (in_function)
      {
        if (ucs_line.size() && ucs_line[0] == UNI_NABLA)   // end of a function
           {
             in_function = false;
             in_variable = false;
             in_matched  = false;
           }
        return ret;
      }

   if (in_variable)
      {
        if (ucs_line.size() == 0)                          // end of a vriable
           {
             in_function = false;
             in_variable = false;
             in_matched  = false;
           }
        return ret;
      }

   if (ucs_line.size() && (ucs_line[0] == UNI_NABLA))   // new function
      {
        in_function = true;
        in_variable = false;
        in_matched  = false;
        ucs_line = ucs_line.drop(1);
        UserFunction_header uh(ucs_line);
        const UCS_string fun_name = uh.get_name();
        loop(n, object_filter.size())
           {
              if (fun_name == object_filter[n])
                 {
                   in_matched = true;
                   return true;
                 }
           }
        return false;
      }

   if (ucs_line.size() && (Avec::is_quad(ucs_line[0]) ||
             Avec::is_first_symbol_char(ucs_line[0])))   // new variable
      {
        in_function = false;
        in_variable = true;
        in_matched  = false;
        loop(u, ucs_line.size())
           {
             if (ucs_line[u] != UNI_LEFT_ARROW)   continue;   // not ←

             ucs_line.shrink(u);
             ucs_line.remove_lt_spaces();
             loop(n, object_filter.size())
                 {
                   if (ucs_line == object_filter[n])
                      {
                        in_matched = true;
                        return true;
                      }
                 }
           }
      }

   return ret;
}
//-----------------------------------------------------------------------------

