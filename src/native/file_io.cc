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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "../Native_interface.hh"

   // CONVENTION: all functions must have an axis argument (like X
   // in A fun[X] B); the axis argument is a function number that selects
   // one of several functions provided by this library...
   //

//-----------------------------------------------------------------------------
struct file_entry
{
   file_entry(FILE * fp, int fd)
   : fe_file(fp),
     fe_fd(fd),
     fe_errno(0),
     fe_may_read(false),
     fe_may_write(false)
   {}
     
   FILE * fe_file;     ///< FILE * returned by fopen
   int    fe_fd;       ///< file desriptor == fileno(file)
   int    fe_errno;    ///< errno for last operation
   bool   fe_may_read;    ///< file open for reading
   bool   fe_may_write;   ///< file open for writing
};

/// all files currently open
vector<file_entry> open_files;

static file_entry &
get_file(const Value & value)
{
   if (open_files.size() == 0)   // first access: init stdin, stdout, and stderr
      {
        file_entry f0(stdin, STDIN_FILENO);
        open_files.push_back(f0);
        file_entry f1(stdout, STDOUT_FILENO);
        open_files.push_back(f1);
        file_entry f2(stderr, STDERR_FILENO);
        open_files.push_back(f2);
      }

const APL_Float qct = Workspace::get_CT();
const APL_Integer handle = value.get_ravel(0).get_near_int(qct);


   loop(h, open_files.size())
      {
        if (open_files[h].fe_fd == handle)   return open_files[h];
      }

   DOMAIN_ERROR;
}
//-----------------------------------------------------------------------------
/// a mandatory function that returns the signature of the eval_XXX()
/// function(s) provided by this library.
///
Fun_signature
get_signature()
{
   return SIG_Z_A_F2_B;
}
//-----------------------------------------------------------------------------
static Token
list_functions(ostream & out)
{
   out << 
"   Functions provided by this library.\n"
"   Assumes 'lib_file_io.so'  ⎕FX  'FUN'\n"
"\n"
"   Legend: e - error code\n"
"           i - integer\n"
"           h - file handle (integer)\n"
"           s - string\n"
"           A1, A2, ...  nested vector with elements A1, A2, ...\n"
"\n"
"           FUN    ''    print this text on stderr\n"
"        '' FUN    ''    print this text on stdout\n"
"           FUN[0] ''    print this text on stderr\n"
"        '' FUN[0] ''    print this text on stdout\n"
"\n"
"   Zi ←    FUN[1] ''    errno (of last call)\n"
"   Zs ←    FUN[2] Be    strerror(Be)\n"
"   Zh ← As FUN[3] Bs    fopen(Bs, As) filename Bs mode As\n"
"   Zh ←    FUN[3] Bs    fopen(Bs, \"r\") filename Bs\n"
"\n"
"   Ze ←    FUN[4] Bh    fclose(Bh)\n"
"   Ze ←    FUN[5] Bh    errno (of last call on Bh)\n"
"   Zi ← Ai FUN[6] Bh    fread(Zi, 1, Ai, Bh) 1 byte per Zi\n"
"   Zi ← Ai FUN[7] Bh    fwrite(Ai, 1, ⍴Ai, Bh) 1 byte per Zi\n"
"   Zi ←    FUN[8] Bh    fgets(Zi, 2000, Bh) 1 byte per Zi\n"
"\n"
   ;

   return Token(TOK_APL_VALUE1, Value::Str0_P);
}
//-----------------------------------------------------------------------------
Token
eval_B(Value_P B)
{
   return list_functions(CERR);
}
//-----------------------------------------------------------------------------
Token
eval_AB(Value_P A, Value_P B)
{
   return list_functions(COUT);
}
//-----------------------------------------------------------------------------
Token
eval_XB(Value_P X, Value_P B)
{
const APL_Float qct = Workspace::get_CT();
const int function_number = X->get_ravel(0).get_near_int(qct);

   switch(function_number)
      {
         case 0:   // list functions
              return list_functions(CERR);

         case 1:   // return (last) errno
              {
                Value_P Z(Value_P(new Value(LOC)));
                new (&Z->get_ravel(0))   IntCell(errno);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 2:   // return strerror(B)
              {
                const int b = B->get_ravel(0).get_near_int(qct);
                const char * text = strerror(b);
                const int len = strlen(text);
                Value_P Z(new Value(len, LOC));
                loop(t, len)
                    new (&Z->get_ravel(t))   CharCell((Unicode)(text[t]));

                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 3:   // fopen(Bs, \"r\") filename Bs
              {
                UTF8_string path(*B.get());
                FILE * f = fopen((const char *)(path.c_str()), "r");
                if (f == 0)   return Token(TOK_APL_VALUE1, Value::Minus_One_P);

                file_entry fe(f, fileno(f));
                fe.fe_may_read = true;
                open_files.push_back(fe);
                return Token(TOK_APL_VALUE1, Value::Zero_P);
              }
      }

   CERR << "Bad eval_XB() function number: " << function_number << endl;
   DOMAIN_ERROR;
}
//-----------------------------------------------------------------------------
Token
eval_AXB(const Value_P A, const Value_P X, const Value_P B)
{
const APL_Float qct = Workspace::get_CT();
const int function_number = X->get_ravel(0).get_near_int(qct);

   switch(function_number)
      {
         case 0:   // list functions
              return list_functions(COUT);

      }

   CERR << "eval_AXB() function number: " << function_number << endl;
   DOMAIN_ERROR;
}
//-----------------------------------------------------------------------------

