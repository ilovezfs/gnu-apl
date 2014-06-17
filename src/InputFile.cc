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

#include <limits.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "../config.h"

#include "Common.hh"
#include "Input.hh"
#include "InputFile.hh"
#include "IO_Files.hh"
#include "UserPreferences.hh"

vector<InputFile> InputFile::files_todo;
int InputFile::stdin_line_no = 1;

//-----------------------------------------------------------------------------
void
InputFile::open_current_file()
{
   if (files_todo.size() && files_todo[0].file == 0)
      {
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
        fclose(files_todo[0].file);
        files_todo[0].file = 0;
        files_todo[0].line_no = -1;
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

   for (int done = 0; done < 4*files_todo.size();)
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
   return !uprefs.do_not_echo;
}
//-----------------------------------------------------------------------------

