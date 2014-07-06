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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "../Native_interface.hh"

   // CONVENTION: all functions must have an axis argument (like X
   // in A fun[X] B); the axis argument is a function number that selects
   // one of several functions provided by this library...
   //

/// the default buffer size if the user does not provide one
enum { SMALL_BUF = 5000 };

class NativeFunction;

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
static Token
do_printf(FILE * out, Value_P A)
{
   // A is expected to be a nested APL value. The first element shall be a 
   // format string, followed by the values for each % field. Result is the
   // number of characters (not bytes!) printed.
   //
int out_len = 0;   // the length of the output produced by all fprintf/fwrite
int a = 0;         // index into A (the next argument to be printed).
const Value & format = *A->get_ravel(a++).get_pointer_value();

   for (int f = 0; f < format.element_count(); /* no f++ */ )
       {
         const Unicode uni = format.get_ravel(f++).get_char_value();
         if (uni != UNI_ASCII_PERCENT)   // not %
            {
              UCS_string ucs(uni);
              UTF8_string utf(ucs);
              fwrite(utf.c_str(), 1, utf.size(), out);
              out_len++;   // count 1 char
              continue;
            }

         // % seen. copy the field in format to fmt and fprintf it with
         // the next argument. That is for format = eg. "...%42.42llf..."
         // we want fmt to be "%42.42llf"
         //
         char fmt[40];
         int fm = 0;        // an index into fmt;
         fmt[fm++] = '%';   // copy the '%'
         for (;;)
             {
               if (f >= format.element_count())
                  {
                    // end of format string reached without seeing the
                    // format char. We print the string and are done.
                    // (This was a mal-formed format string from the user)
                    //
                    fwrite(fmt, 1, fm, out);
                    out_len += fm;
                    goto printf_done;
                  }

               if (fm >= sizeof(fmt))
                  {
                    // end of fmt reached without seeing the format char.
                    // We print the string and are done.
                    // (This may or may not be a mal-formed format string
                    // from the user; we assume it is)
                    //
                    fwrite(fmt, 1, fm, out);
                    out_len += fm;
                    goto field_done;
                  }

               const Unicode un1 = format.get_ravel(f++).get_char_value();
               switch(un1)
                  {
                     // flag chars and field width/precision
                     //
                     case '#':
                     case '0' ... '9':
                     case '-':
                     case ' ':
                     case '+':
                     case '\'':   // SUSE
                     case 'I':    // glibc
                     case '.':

                     // length modifiers
                     //
                     case 'h':
                     case 'l':
                     case 'L':
                     case 'q':
                     case 'j':
                     case 'z':
                     case 't': fmt[fm++] = un1;   fmt[fm] = 0;
                               continue;

                         // conversion specifiers
                         //
                     case 'd':   case 'i':   case 'o':
                     case 'u':   case 'x':   case 'X':   case 'p':
                          {
                            const Cell & cell = A->get_ravel(a++);
                            APL_Integer iv;
                            if (cell.is_integer_cell())
                               {
                                 iv = cell.get_int_value();
                               }
                            else 
                               {
                                 const APL_Float fv = cell.get_real_value();
                                 if (fv < 0)   iv = -int(-fv);
                                 else          iv = int(fv);
                               }
                            fmt[fm++] = un1;   fmt[fm] = 0;
                            out_len += fprintf(out, fmt, iv);
                          }
                          goto field_done;

                     case 'e':   case 'E':   case 'f':   case 'F':
                     case 'g':   case 'G':   case 'a':   case 'A':
                          {
                            const APL_Float fv =
                                            A->get_ravel(a++).get_real_value();
                            fmt[fm++] = un1;   fmt[fm] = 0;
                            out_len += fprintf(out, fmt, fv);
                          }
                          goto field_done;

                     case 's':   // string or char
                          if (A->get_ravel(a).is_character_cell())   goto cval;
                              {
                                Value_P str =
                                        A->get_ravel(a++).get_pointer_value();
                                UCS_string ucs(*str.get());
                                UTF8_string utf(ucs);
                                fwrite(utf.c_str(), 1, utf.size(), out); 
                                out_len += ucs.size();   // not utf.size() !
                              }
                          goto field_done;

                     case 'c':   // single char
                     cval:
                          {
                            const Unicode cv =
                                            A->get_ravel(a++).get_char_value();
                            UCS_string ucs(cv);
                            UTF8_string utf(ucs);
                            fwrite(utf.c_str(), 1, utf.size(), out); 
                            ++out_len;
                          }
                          goto field_done;

                     case 'n':
                          out_len += fprintf(out, "%d", out_len);
                          goto field_done;

                     case 'm':
                          out_len += fprintf(out, "%s", strerror(errno));
                          goto field_done;


                     case '%':
                          if (fm == 0)   // %% is %
                             {
                               fputc('%', out);
                               ++out_len;
                               goto field_done;
                             }
                          /* no break */

                     default:
                        {
                          UCS_string & t4 = Workspace::more_error();
                          t4.clear();
                          t4.append_utf8("invalid format character ");
                          t4.append(un1, LOC);
                          t4.append_utf8(" in function 22 (aka. printf())"
                                         " in module file_io:: ");
                          DOMAIN_ERROR;   // bad format char
                        }
                  }
             }
         field_done: ;
       }

printf_done:

Value_P Z(new Value(LOC));   // scalar result
   new (Z->next_ravel())   IntCell(out_len);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
extern "C" void * get_function_mux(const char * function_name);
static Fun_signature get_signature();
static bool close_fun(Cause cause,          const NativeFunction * caller);
static Token eval_B  (Value_P B,            const NativeFunction * caller);
static Token eval_AB (Value_P A, Value_P B, const NativeFunction * caller);
static Token eval_XB (Value_P X, Value_P B, const NativeFunction * caller);
static Token eval_AXB(Value_P A, Value_P X,
                      Value_P B,            const NativeFunction * caller);

void *
get_function_mux(const char * function_name)
{
   if (!strcmp(function_name, "get_signature"))   return (void *)&get_signature;
   if (!strcmp(function_name, "close_fun"))       return (void *)&close_fun;
   if (!strcmp(function_name, "eval_B"))          return (void *)&eval_B;
   if (!strcmp(function_name, "eval_AB"))         return (void *)&eval_AB;
   if (!strcmp(function_name, "eval_XB"))         return (void *)&eval_XB;
   if (!strcmp(function_name, "eval_AXB"))        return (void *)&eval_AXB;
   return 0;
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
/// an optional function that is called when the native function in the
/// APL interpreter is about to be removed. Return true if the caller shall
/// dlclose() this library
bool
close_fun(Cause cause, const NativeFunction * caller)
{
   return true;
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
"           FUN     ''    print this text on stderr\n"
"        '' FUN     ''    print this text on stdout\n"
"           FUN[ 0] ''    print this text on stderr\n"
"        '' FUN[ 0] ''    print this text on stdout\n"
"\n"
"   Zi ←    FUN[ 1] ''    errno (of last call)\n"
"   Zs ←    FUN[ 2] Be    strerror(Be)\n"
"   Zh ← As FUN[ 3] Bs    fopen(Bs, As) filename Bs mode As\n"
"   Zh ←    FUN[ 3] Bs    fopen(Bs, \"r\") filename Bs\n"
"\n"
"   Ze ←    FUN[ 4] Bh    fclose(Bh)\n"
"   Ze ←    FUN[ 5] Bh    errno (of last call on Bh)\n"
"   Zi ←    FUN[ 6] Bh    fread(Zi, 1, " << SMALL_BUF << ", Bh) 1 byte per Zi\n"
"   Zi ← Ai FUN[ 6] Bh    fread(Zi, 1, Ai, Bh) 1 byte per Zi\n"
"   Zi ← Ai FUN[ 7] Bh    fwrite(Ai, 1, ⍴Ai, Bh) 1 byte per Zi\n"
"   Zi ←    FUN[ 8] Bh    fgets(Zi, " << SMALL_BUF << ", Bh) 1 byte per Zi\n"
"   Zi ← Ai FUN[ 8] Bh    fgets(Zi, Ai, Bh) 1 byte per Zi\n"
"   Zi ←    FUN[ 9] Bh    fgetc(Zi, Bh) 1 byte per Zi\n"
"   Zi ←    FUN[10] Bh    feof(Bh)\n"
"   Zi ←    FUN[11] Bh    ferror(Bh)\n"
"   Zi ←    FUN[12] Bh    ftell(Bh)\n"
"   Zi ← Ai FUN[13] Bh    fseek(Bh, Ai, SEEK_SET)\n"
"   Zi ← Ai FUN[14] Bh    fseek(Bh, Ai, SEEK_CUR)\n"
"   Zi ← Ai FUN[15] Bh    fseek(Bh, Ai, SEEK_END)\n"
"   Zi ←    FUN[16] Bh    fflush(Bh)\n"
"   Zi ←    FUN[17] Bh    fsync(Bh)\n"
"   Zi ←    FUN[18] Bh    fstat(Bh)\n"
"   Zi ←    FUN[19] Bh    unlink(Bc)\n"
"   Zi ←    FUN[20] Bh    mkdir(Bc, 0777)\n"
"   Zi ← Ai FUN[20] Bh    mkdir(Bc, AI)\n"
"   Zi ←    FUN[21] Bh    rmdir(Bc)\n"
"   Zi ← A  FUN[22] 1     printf(         A1, A2...) format A1\n"
"   Zi ← A  FUN[22] 2     fprintf(stderr, A1, A2...) format A1\n"
"   Zi ← A  FUN[22] Bh    fprintf(Bh,     A1, A2...) format A1\n"
"   Zi ← Ac FUN[23] Bh    fwrite(Ac, 1, ⍴Ac, Bh) Unicode Ac Output UTF-8\n"
"   Zh ← As FUN[24] Bs    popen(Bs, As) command Bs mode As\n"
"   Zh ←    FUN[24] Bs    popen(Bs, \"r\") command Bs\n"
"\n"
"   Ze ←    FUN[25] Bh    pclose(Bh)\n"
"   Zs ←    FUN[26] Bs    return entire file Bs as byte vector\n"
"\n";

   return Token(TOK_APL_VALUE1, Str0(LOC));
}
//-----------------------------------------------------------------------------
Token
eval_B(Value_P B, const NativeFunction * caller)
{
   if (!B->get_ravel(0).is_integer_cell())     return list_functions(COUT);

const APL_Integer what = B->get_ravel(0).get_int_value();
   switch(what)
      {
        // what < 0 are "hacker functions" that should no be used by
        // normal mortals.
        //
        case -5: // return ⎕AV of IBM APL2
             {
               Value_P Z(new Value(256, LOC));
               const Unicode * ibm = Avec::IBM_quad_AV();
               loop(c, 256)   new (Z->next_ravel()) CharCell(ibm[c]);
               Z->check_value(LOC);
               return Token(TOK_APL_VALUE1, Z);
             }

        case -4: // clear all probes (ignores B, returns 0)
             Probe::init_all();
             return Token(TOK_APL_VALUE1, IntScalar(0, LOC));

        case -2: // return CPU frequency
             {
               timeval tv = { 0, 100000 }; // 100 ms
               const uint64_t from = cycle_counter();
               select(0, 0, 0, 0, &tv);
               const uint64_t to = cycle_counter();

               Value_P Z(new Value(LOC));
               new (Z->next_ravel())   IntCell(10*(to - from));
               Z->check_value(LOC);
               return Token(TOK_APL_VALUE1, Z);
             }

        case -1: // return CPU cycle counter
             {
               Value_P Z(new Value(LOC));
               new (&Z->get_ravel(0))   IntCell(cycle_counter());
               Z->check_value(LOC);
               return Token(TOK_APL_VALUE1, Z);
             }
      }

   return list_functions(COUT);
}
//-----------------------------------------------------------------------------
Token
eval_AB(Value_P A, Value_P B, const NativeFunction * caller)
{
   if (!B->get_ravel(0).is_integer_cell())     return list_functions(COUT);

const APL_Integer what = B->get_ravel(0).get_int_value();
   switch(what)
      {
        case -3: // read probe A and clear it
             {
               APL_Integer probe = A->get_ravel(0).get_int_value();

               // negative numbers mean take from end (like ↑ and ↓)
               // convention ins that 0, 1, 2, ... are a probe vector (such as
               // one entry per core) while probes at the end are individual
               // probes.
               //
               if (probe < 0)   probe = Probe::PROBE_COUNT + probe;
               if (probe >= Probe::PROBE_COUNT)   // too high
                  return Token(TOK_APL_VALUE1, IntScalar(-1, LOC));

               const int len = Probe::get_length(probe);
               if (len < 0)   return Token(TOK_APL_VALUE1, IntScalar(-1, LOC));

                Value_P Z(new Value(len, LOC));
                loop(m, len)
                    new (Z->next_ravel())   IntCell(Probe::get_time(probe, m));

                Probe::init(probe);
                return Token(TOK_APL_VALUE1, Z);
             }
             
      }

   return list_functions(COUT);
}
//-----------------------------------------------------------------------------
Token
eval_XB(Value_P X, Value_P B, const NativeFunction * caller)
{
const APL_Float qct = Workspace::get_CT();
const int function_number = X->get_ravel(0).get_near_int(qct);

   switch(function_number)
      {
         case 0:   // list functions
              return list_functions(CERR);

         case 1:   // return (last) errno
              goto out_errno;

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
                errno = 0;
                FILE * f = fopen(path.c_str(), "r");
                if (f == 0)   return Token(TOK_APL_VALUE1, IntScalar(-1, LOC));

                file_entry fe(f, fileno(f));
                fe.fe_may_read = true;
                open_files.push_back(fe);
                Value_P Z(new Value(LOC));
                new (&Z->get_ravel(0))   IntCell(fe.fe_fd);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 4:   // fclose(Bh)
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                if (fe.fe_file)   fclose(fe.fe_file);
                else              close(fe.fe_fd);

                fe = open_files.back();       // move last file to fe
                open_files.pop_back();        // erase last file
                goto out_errno;
              }

         case 5:   // errno of Bh
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                Value_P Z(Value_P(new Value(LOC)));
                new (&Z->get_ravel(0))   IntCell(fe.fe_errno);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 6:   // fread(Zi, 1, SMALL_BUF, Bh) 1 byte per Zi\n"
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                clearerr(fe.fe_file);

                char buffer[SMALL_BUF];

                size_t len = fread(buffer, 1, SMALL_BUF, fe.fe_file);
                Value_P Z(new Value(len, LOC));
                new (&Z->get_ravel(0)) IntCell(0);   // prototype
                loop(z, len)   new (&Z->get_ravel(z)) IntCell(buffer[z] & 0xFF);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 8:   // fgets(Zi, SMALL_BUF, Bh) 1 byte per Zi\n"
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                clearerr(fe.fe_file);

                char buffer[SMALL_BUF];

                const char * s = fgets(buffer, SMALL_BUF, fe.fe_file);
                const int len = s ? strlen(s) : 0;
                Value_P Z(new Value(len, LOC));
                new (&Z->get_ravel(0)) IntCell(0);   // prototype
                loop(z, len)   new (&Z->get_ravel(z)) IntCell(buffer[z] & 0xFF);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 9:   // fgetc(Bh)
              {
                file_entry & fe = get_file(*B.get());
                Value_P Z(Value_P(new Value(LOC)));
                new (&Z->get_ravel(0))   IntCell(fgetc(fe.fe_file));
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 10:   // feof(Bh)
              {
                file_entry & fe = get_file(*B.get());
                Value_P Z(Value_P(new Value(LOC)));
                new (&Z->get_ravel(0))   IntCell(feof(fe.fe_file));
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 11:   // ferror(Bh)
              {
                file_entry & fe = get_file(*B.get());
                Value_P Z(Value_P(new Value(LOC)));
                new (&Z->get_ravel(0))   IntCell(ferror(fe.fe_file));
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 12:   // ftell(Bh)
              {
                file_entry & fe = get_file(*B.get());
                Value_P Z(Value_P(new Value(LOC)));
                new (&Z->get_ravel(0))   IntCell(ftell(fe.fe_file));
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 16:   // fflush(Bh)
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                fflush(fe.fe_file);
              }
              goto out_errno;

         case 17:   // fsync(Bh)
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                fsync(fe.fe_fd);
              }
              goto out_errno;

         case 18:   // fstat(Bh)
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                struct stat s;
                const int result = fstat(fe.fe_fd, &s);
                if (result)   goto out_errno;   // fstat failed

                Value_P Z(Value_P(new Value(13, LOC)));
                new (&Z->get_ravel( 0))   IntCell(s.st_dev);
                new (&Z->get_ravel( 1))   IntCell(s.st_ino);
                new (&Z->get_ravel( 2))   IntCell(s.st_mode);
                new (&Z->get_ravel( 3))   IntCell(s.st_nlink);
                new (&Z->get_ravel( 4))   IntCell(s.st_uid);
                new (&Z->get_ravel( 5))   IntCell(s.st_gid);
                new (&Z->get_ravel( 6))   IntCell(s.st_rdev);
                new (&Z->get_ravel( 7))   IntCell(s.st_size);
                new (&Z->get_ravel( 8))   IntCell(s.st_blksize);
                new (&Z->get_ravel( 9))   IntCell(s.st_blocks);
                new (&Z->get_ravel(10))   IntCell(s.st_atime);
                new (&Z->get_ravel(11))   IntCell(s.st_mtime);
                new (&Z->get_ravel(12))   IntCell(s.st_ctime);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 19:   // unlink(Bc)
              {
                UTF8_string path(*B.get());
                unlink(path.c_str());
              }
              goto out_errno;

         case 20:   // mkdir(Bc)
              {
                UTF8_string path(*B.get());
                mkdir(path.c_str(), 0777);
              }
              goto out_errno;

         case 21:   // rmdir(Bc)
              {
                UTF8_string path(*B.get());
                rmdir(path.c_str());
              }
              goto out_errno;

         case 24:   // popen(Bs, "r") command Bs
              {
                UTF8_string path(*B.get());
                errno = 0;
                FILE * f = popen(path.c_str(), "r");
                if (f == 0)
                   {
                     if (errno)   goto out_errno;   // errno may be set or not
                     return Token(TOK_APL_VALUE1, IntScalar(-1, LOC));
                   }

                file_entry fe(f, fileno(f));
                fe.fe_may_read = true;
                open_files.push_back(fe);
                Value_P Z(new Value(LOC));
                new (&Z->get_ravel(0))   IntCell(fe.fe_fd);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 25:   // pclose(Bh)
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                int err = EBADF;   /* Bad file number */
                if (fe.fe_file)   err = pclose(fe.fe_file);

                fe = open_files.back();       // move last file to fe
                open_files.pop_back();        // erase last file

                if (err == -1)   goto out_errno;   // pclose() failed
                Value_P Z(new Value(LOC));
                new (&Z->get_ravel(0))   IntCell(err);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 26:   // read entire file
              {
                errno = 0;
                UTF8_string path(*B.get());
                int fd = open(path.c_str(), O_RDONLY);
                if (fd == -1)   goto out_errno;

                struct stat st;
                if (fstat(fd, &st))
                   {
                     close(fd);
                     goto out_errno;
                   }

                ShapeItem len = st.st_size;
                unsigned char * data = (unsigned char *)mmap(0, len, PROT_READ,
                                MAP_SHARED, fd, 0);
                close(fd);
                if (data == 0)   goto out_errno;

                Value_P Z(new Value(len, LOC));
                loop(z, len)   new (Z->next_ravel()) CharCell((Unicode)data[z]);
                munmap((char *)data, len);

                Z->set_default_Spc();
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }
      }

   CERR << "Bad eval_XB() function number: " << function_number << endl;
   DOMAIN_ERROR;

out_errno:
Value_P Z(Value_P(new Value(LOC)));
   new (&Z->get_ravel(0))   IntCell(errno);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
eval_AXB(const Value_P A, const Value_P X, const Value_P B,
         const NativeFunction * caller)
{
const APL_Float qct = Workspace::get_CT();
const int function_number = X->get_ravel(0).get_near_int(qct);

   switch(function_number)
      {
         case 0:   // list functions
              return list_functions(COUT);

         case 3:   // fopen(Bs, As) filename Bs mode As
              {
                UTF8_string path(*B.get());
                UTF8_string mode(*A.get());
                const char * m = mode.c_str();
                bool read = false;
                bool write = false;
                if      (!strncmp(m, "r+", 2))     read = write = true;
                else if (!strncmp(m, "r" , 1))     read         = true;
                else if (!strncmp(m, "w+", 2))     read = write = true;
                else if (!strncmp(m, "w" , 1))            write = true;
                else if (!strncmp(m, "a+", 2))     read = write = true;
                else if (!strncmp(m, "a" , 1))            write = true;
                else    DOMAIN_ERROR;

                errno = 0;
                FILE * f = fopen(path.c_str(), m);
                if (f == 0)   return Token(TOK_APL_VALUE1, IntScalar(-1, LOC));

                file_entry fe(f, fileno(f));
                fe.fe_may_read = read;
                fe.fe_may_write = write;
                open_files.push_back(fe);
                Value_P Z(new Value(LOC));
                new (&Z->get_ravel(0))   IntCell(fe.fe_fd);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 6:   // fread(Zi, 1, Ai, Bh) 1 byte per Zi\n"
              {
                errno = 0;
                const int bytes = A->get_ravel(0).get_near_int(qct);
                file_entry & fe = get_file(*B.get());
                clearerr(fe.fe_file);


                char small_buffer[SMALL_BUF];
                char * buffer = small_buffer;
                char * del = 0;
                if (bytes > sizeof(small_buffer))
                   buffer = del = new char[bytes];

                size_t len = fread(buffer, 1, bytes, fe.fe_file);
                Value_P Z(new Value(len, LOC));
                new (&Z->get_ravel(0)) IntCell(0);   // prototype
                loop(z, len)   new (&Z->get_ravel(z)) IntCell(buffer[z] & 0xFF);
                delete [] del;
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 7:   // fwrite(Ai, 1, ⍴Ai, Bh) 1 byte per Zi\n"
              {
                errno = 0;
                const int bytes = A->nz_element_count();
                file_entry & fe = get_file(*B.get());

                char small_buffer[SMALL_BUF];
                char * buffer = small_buffer;
                char * del = 0;
                if (bytes > sizeof(small_buffer))
                   buffer = del = new char[bytes];

                loop(z, bytes)   buffer[z] = A->get_ravel(z).get_near_int(qct);

                size_t len = fwrite(buffer, 1, bytes, fe.fe_file);
                delete [] del;

                Value_P Z(new Value(LOC));
                new (&Z->get_ravel(0)) IntCell(len); 
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 8:   // fgets(Zi, Ai, Bh) 1 byte per Zi\n"
              {
                errno = 0;
                const int bytes = A->get_ravel(0).get_near_int(qct);
                file_entry & fe = get_file(*B.get());
                clearerr(fe.fe_file);

                char small_buffer[SMALL_BUF];
                char * buffer = small_buffer;
                char * del = 0;
                if (bytes > sizeof(buffer))
                   buffer = del = new char[bytes + 1];

                const char * s = fgets(buffer, bytes, fe.fe_file);
                const int len = s ? strlen(s) : 0;
                Value_P Z(new Value(len, LOC));
                new (&Z->get_ravel(0)) IntCell(0);   // prototype
                loop(z, len)   new (&Z->get_ravel(z)) IntCell(buffer[z] & 0xFF);
                delete [] del;
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 13:   // fseek(Bh, Ai, SEEK_SET)
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                const APL_Integer pos = A->get_ravel(0).get_near_int(qct);
                fseek(fe.fe_file, pos, SEEK_SET);
              }
              goto out_errno;

         case 14:   // fseek(Bh, Ai, SEEK_CUR)
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                const APL_Integer pos = A->get_ravel(0).get_near_int(qct);
                fseek(fe.fe_file, pos, SEEK_CUR);
              }
              goto out_errno;

         case 15:   // fseek(Bh, Ai, SEEK_END)
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                const APL_Integer pos = A->get_ravel(0).get_near_int(qct);
                fseek(fe.fe_file, pos, SEEK_END);
              }
              goto out_errno;

         case 20:   // mkdir(Bc, Ai)
              {
                const int mask = A->get_ravel(0).get_near_int(qct);
                UTF8_string path(*B.get());
                mkdir(path.c_str(), mask);
              }
              goto out_errno;

         case 22:   // fprintf(Bc, Ai)
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                return do_printf(fe.fe_file, A);
              }

         case 23:   // fwrite(Ac, 1, ⍴Ac, Bh) Unicode Ac Output UTF-8 \n"
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                UCS_string text(*A.get());
                UTF8_string utf(text);
                const size_t l = fwrite(utf.c_str(), 1, utf.size(), fe.fe_file); 
                Value_P Z(new Value(LOC));
                new (&Z->get_ravel(0)) IntCell(l); 
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 24:   // popen(Bs, As) command Bs mode As
              {
                UTF8_string path(*B.get());
                UTF8_string mode(*A.get());
                const char * m = mode.c_str();
                bool read = false;
                bool write = false;
                if      (!strncmp(m, "er", 2))     read = true;
                else if (!strncmp(m, "r" , 1))     read = true;
                else if (!strncmp(m, "ew", 2))     write = true;
                else if (!strncmp(m, "w" , 1))     write = true;
                else    DOMAIN_ERROR;

                errno = 0;
                FILE * f = popen(path.c_str(), m);
                if (f == 0)
                   {
                     if (errno)   goto out_errno;   // errno may be set or not
                     return Token(TOK_APL_VALUE1, IntScalar(-1, LOC));
                   }

                file_entry fe(f, fileno(f));
                fe.fe_may_read = read;
                fe.fe_may_write = write;
                open_files.push_back(fe);
                Value_P Z(new Value(LOC));
                new (&Z->get_ravel(0))   IntCell(fe.fe_fd);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

      }

   CERR << "eval_AXB() function number: " << function_number << endl;
   DOMAIN_ERROR;

out_errno:
Value_P Z(Value_P(new Value(LOC)));
   new (&Z->get_ravel(0))   IntCell(errno);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------

