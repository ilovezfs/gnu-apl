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

#ifndef __INPUT_FILE_HH_DEFINED__
#define __INPUT_FILE_HH_DEFINED__

/// an input file and its properties. the file can be an apl script(.asp) file
/// or a testcase (.tc) file. The file names initially come from the command
/// line, but can be extended by )COPY commands 
struct InputFile
{
   friend class IO_Files;

   /// constructor for vector<>::resize()
   InputFile() {}   // for resize()

   /// Normal constructor
   InputFile(const UTF8_string & _filename, FILE * _file,
                     bool _test, bool _echo, bool _is_script, bool _with_LX)
   : file     (_file),
     filename (_filename),
     test     (_test),
     echo     (_echo),
     is_script(_is_script),
     with_LX  (_with_LX),
     line_no  (0)
   {}

   /// set the current line number
   void set_line_no(int num)
      { line_no = num; }

   /// the current file
   static InputFile * current_file()
      { return files_todo.size() ? &files_todo[0] : 0; }

   /// the name of the currrent file
   static const char * current_filename()
      { return files_todo.size() ? files_todo[0].filename.c_str() : "stdin"; }

   /// the line number of the currrent file
   static int current_line_no()
      { return files_todo.size() ? files_todo[0].line_no : stdin_line_no; }

   /// increment the line number of the current file
   static void increment_current_line_no()
      { if (files_todo.size()) ++files_todo[0].line_no; else ++stdin_line_no; }

   /// return true iff input comes from a script (as opposed to running
   /// interactively)
   static bool running_script()
      { return files_todo.size() > 0 && files_todo[0].is_script; }

   /// return true iff the current input file exists and is a test file
   static bool is_validating()
      { return files_todo.size() > 0 && files_todo[0].test; }

   /// the number of testcase (.tc) files
   static int testcase_file_count()
      {
        int count = 0;
        loop(f, files_todo.size())
            {
              if (files_todo[f].test)   ++count;
            }
        return count;
      }

   /// true if echo (of the input) is on for the current file
   static bool echo_current_file();

   /// open current file unless already open
   static void open_current_file();

   /// close the current file and perform some cleanup
   static void close_current_file();

   /// randomize the order of test_file_names
   static void randomize_files();

   /// files that need to be processed
   static vector<InputFile> files_todo;

   FILE       * file;       ///< file descriptor
   UTF8_string  filename;   ///< dito.

protected:
   bool         test;       ///< true for -T testfile, false for -f APLfile
   bool         echo;       ///< echo stdin
   bool         is_script;  ///< script (override existing functions)
   bool         with_LX;    ///< execute ⎕LX at the end
   int          line_no;    ///< line number in file

   /// line number in stdin
   static int stdin_line_no;
};

#endif // __INPUT_FILE_HH_DEFINED__
