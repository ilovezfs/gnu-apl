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

#ifndef __QUAD_FIO_HH_DEFINED__
#define __QUAD_FIO_HH_DEFINED__

#include "QuadFunction.hh"

//-----------------------------------------------------------------------------

/**
   The system function Quad-FIO (File I/O)
 */
class Quad_FIO : public QuadFunction
{
public:
   /// the default buffer size if the user does not provide one
   enum { SMALL_BUF = 5000 };

   /// Constructor.
   Quad_FIO();

   static Quad_FIO * fun;   ///< Built-in function.
   static Quad_FIO  _fun;   ///< Built-in function.

   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   /// overloaded Function::eval_AXB().
   virtual Token eval_XB(Value_P X, Value_P B);

   /// overloaded Function::eval_AXB().
   virtual Token eval_AXB(Value_P A, Value_P X, Value_P B);

   /// close all open files
   void clear();
protected:
   struct file_entry
      {
        file_entry(FILE * fp, int fd)
        : fe_FILE(fp),
          fe_fd(fd),
          fe_errno(0),
          fe_may_read(false),
          fe_may_write(false)
        {}

        UTF8_string path;      ///< filename
        FILE * fe_FILE;        ///< FILE * returned by fopen()
        int    fe_fd;          ///< file desriptor == fileno(file)
        int    fe_errno;       ///< errno for last operation
        bool   fe_may_read;    ///< file open for reading
        bool   fe_may_write;   ///< file open for writing
      };

   file_entry & get_file(const Value & value);

   FILE * get_FILE(const Value & value);

   int get_fd(const Value & value)
       {
         file_entry & fe = get_file(value);   // may throw DOMAIN ERROR
         return fe.fe_fd;
       }

   Token list_functions(ostream & out);

   Value_P fds_to_val(const fd_set * fds, int max_fd);

   Token do_printf(FILE * out, Value_P A);

   vector<file_entry> open_files;
};
//-----------------------------------------------------------------------------
#endif //  __QUAD_FIO_HH_DEFINED__

