/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2015  Dr. Jürgen Sauermann

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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include "../Bif_OPER2_INNER.hh"
#include "../Bif_OPER2_OUTER.hh"
#include "../Native_interface.hh"
#include "../Performance.hh"
#include "../Tokenizer.hh"

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
   : fe_FILE(fp),
     fe_fd(fd),
     fe_errno(0),
     fe_may_read(false),
     fe_may_write(false)
   {}
     
   FILE * fe_FILE;     ///< FILE * returned by fopen()
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

const APL_Integer handle = value.get_ravel(0).get_near_int();

   loop(h, open_files.size())
      {
        if (open_files[h].fe_fd == handle)   return open_files[h];
      }

   DOMAIN_ERROR;
}
//-----------------------------------------------------------------------------
static FILE *
get_FILE(const Value & value)
{
file_entry & fe = get_file(value);
   if (fe.fe_FILE == 0)
      {
        if (fe.fe_may_read && fe.fe_may_write)
         fe.fe_FILE = fdopen(fe.fe_fd, "a+");
        else if (fe.fe_may_read)
         fe.fe_FILE = fdopen(fe.fe_fd, "r");
        else if (fe.fe_may_write)
         fe.fe_FILE = fdopen(fe.fe_fd, "a");
        else
         DOMAIN_ERROR;   // internal error
      }

   return fe.fe_FILE;
}
//-----------------------------------------------------------------------------
static int
get_fd(const Value & value)
{
file_entry & fe = get_file(value);
   return fe.fe_fd;
}
//-----------------------------------------------------------------------------
static Value_P
fds_to_val(const fd_set * fds, int max_fd)
{
ShapeItem fd_count = 0;
   if (fds)
      {
        loop(m, max_fd)   if (FD_ISSET(m, fds))   ++fd_count;
      }

Value_P Z(fd_count, LOC);
   new (&Z->get_ravel(0)) IntCell(0);   // prototype

   if (fds)
      {
        loop(m, max_fd)
            if (FD_ISSET(m, fds))   new (Z->next_ravel())   IntCell(m);
      }

   return Z;
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

   return Token(TOK_APL_VALUE1, IntScalar(out_len, LOC));
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
"   Legend: a - address family, IPv4 address, port (or errno)\n"
"           d - table of dirent structs\n"
"           e - error code (integer as per errno.h)\n"
"           h - file handle (integer)\n"
"           i - integer\n"
"           n - names (nested vector of strings)\n"
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
"File I/O functions:\n"
"\n"
"   Ze ←    FUN[ 4] Bh    fclose(Bh)\n"
"   Ze ←    FUN[ 5] Bh    errno (of last call on Bh)\n"
"   Zi ←    FUN[ 6] Bh    fread(Zi, 1, " << SMALL_BUF << ", Bh) 1 byte per Zi\n"
"   Zi ← Ai FUN[ 6] Bh    fread(Zi, 1, Ai, Bh) 1 byte per Zi\n"
"   Zi ← Ai FUN[ 7] Bh    fwrite(Ai, 1, ⍴Ai, Bh) 1 byte per Ai\n"
"   Zi ←    FUN[ 8] Bh    fgets(Zi, " << SMALL_BUF << ", Bh) 1 byte per Zi\n"
"   Zi ← Ai FUN[ 8] Bh    fgets(Zi, Ai, Bh) 1 byte per Zi\n"
"   Zi ←    FUN[ 9] Bh    fgetc(Zi, Bh) 1 byte\n"
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
"   Zi ← Ac FUN[23] Bh    fwrite(Ac, 1, ⍴Ac, Bh) 1 Unicode per Ac, Output UTF8\n"
"   Zh ← As FUN[24] Bs    popen(Bs, As) command Bs mode As\n"
"   Zh ←    FUN[24] Bs    popen(Bs, \"r\") command Bs\n"
"   Ze ←    FUN[25] Bh    pclose(Bh)\n"
"   Zs ←    FUN[26] Bs    return entire file Bs as byte vector\n"
"   Zs ← As FUN[27] Bs    rename file As to Bs\n"
"   Zd ←    FUN[28] Bs    return content of directory Bs\n"
"   Zn ←    FUN[29] Bs    return file names in directory Bs\n"
"   Zs ←    FUN 30        getcwd()\n"
"   Zn ← As FUN[31] Bs    access(As, Bs) As ∈ 'RWXF'\n"
"   Zh ←    FUN[32] Bi    socket(Bi=AF_INET, SOCK_STREAM, 0)\n"
"   Ze ← Aa FUN[33] Bh    bind(Bh, Aa)\n"
"   Ze ←    FUN[34] Bh    listen(Bh, 10)\n"
"   Ze ← Ai FUN[34] Bh    listen(Bh, Ai)\n"
"   Za ←    FUN[35] Bh    accept(Bh)\n"
"   Ze ← Aa FUN[36] Bh    connect(Bh, Aa)\n"
"   Zi ←    FUN[37] Bh    recv(Bh, Zi, " << SMALL_BUF << ", 0) 1 byte per Zi\n"
"   Zi ← Ai FUN[37] Bh    recv(Bh, Zi, Ai, 0) 1 byte per Zi\n"
"   Zi ← Ai FUN[38] Bh    send(Bh, Ai, ⍴Ai, 0) 1 byte per Ai\n"
"   Zi ← Ac FUN[39] Bh    send(Bh, Ac, ⍴Ac, 0) 1 Unicode per Ac, Output UTF8\n"
"   Zi ←    FUN[40] B     select(B_read, B_write, B_exception, B_timeout)\n"
"   Zi ←    FUN[41] Bh    read(Bh, Zi, " << SMALL_BUF << ") 1 byte per Zi\n"
"   Zi ← Ai FUN[41] Bh    read(Bh, Zi, Ai) 1 byte per Zi\n"
"   Zi ← Ai FUN[42] Bh    write(Bh, Ai, ⍴Ai) 1 byte per Ai\n"
"   Zi ← Ac FUN[43] Bh    write(Bh, Ac, ⍴Ac) 1 Unicode per Ac, Output UTF8\n"
"   Za ←    FUN[44] Bh    getsockname(Bh)\n"
"   Za ←    FUN[45] Bh    getpeername(Bh)\n"
"   Zi ← Ai FUN[46] Bh    getsockopt(Bh, A_level, A_optname, Zi)\n"
"   Ze ← Ai FUN[47] Bh    setsockopt(Bh, A_level, A_optname, A_optval)\n"
"\n"
"Benchmarking functions:\n"
"\n"
"           FUN[200] Bi    clear statistics with ID Bi\n"
"   Zn ←    FUN[201] Bi    get statistics with ID Bi\n"
"           FUN[202] Bs    get monadic parallel threshold for primitive Bs\n"
"        Ai FUN[202] Bs    set monadic parallel threshold for primitive Bs\n"
"           FUN[203] Bs    get dyadic parallel threshold for primitive Bs\n"
"        Ai FUN[203] Bs    set dyadic parallel threshold for primitive Bs\n";

   return Token(TOK_APL_VALUE1, Str0(LOC));
}
//-----------------------------------------------------------------------------
Token
eval_B(Value_P B, const NativeFunction * caller)
{
   if (B->get_rank() > 1)   RANK_ERROR;

   if (!B->get_ravel(0).is_integer_cell())     return list_functions(COUT);

const APL_Integer what = B->get_ravel(0).get_int_value();
   switch(what)
      {
        // what < 0 are "hacker functions" that should no be used by
        // normal mortals.
        //
        case -9: // screen height
             {
               struct winsize ws;
               ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
               return Token(TOK_APL_VALUE1, IntScalar(ws.ws_row, LOC));
             }
        case -8: // screen width
             {
               struct winsize ws;
               ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
               return Token(TOK_APL_VALUE1, IntScalar(ws.ws_col, LOC));
             }
        case -7: // throw a segfault
             {
               CERR << "NOTE: Triggering a segfault (keeping the current "
                       "SIGSEGV handler)..." << endl;

               const APL_Integer result = *(char *)4343;
               CERR << "NOTE: Throwing a segfault failed." << endl;
               return Token(TOK_APL_VALUE1, IntScalar(result, LOC));
             }
                  
        case -6: // throw a segfault
             {
               CERR << "NOTE: Resetting SIGSEGV handler and triggering "
                       "a segfault..." << endl;

               // reset the SSEGV handler
               //
               struct sigaction action;
               memset(&action, 0, sizeof(struct sigaction));
               action.sa_handler = SIG_DFL;
               sigaction(SIGSEGV, &action, 0);
               const APL_Integer result = *(char *)4343;
               CERR << "NOTE: Throwing a segfault failed." << endl;
               return Token(TOK_APL_VALUE1, IntScalar(result, LOC));
             }
                  
        case -5: // return ⎕AV of IBM APL2
             {
               Value_P Z(256, LOC);
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

               return Token(TOK_APL_VALUE1, IntScalar(10*(to - from), LOC));
             }

        case -1: // return CPU cycle counter
             {
               return Token(TOK_APL_VALUE1, IntScalar(cycle_counter(), LOC));
             }

        case 30:   // getcwd()
             {
               char buffer[PATH_MAX + 1];
               char * success = getcwd(buffer, PATH_MAX);
               if (!success)   goto out_errno;

               buffer[PATH_MAX] = 0;
               UCS_string cwd(buffer);

               Value_P Z(cwd, LOC);
               Z->set_default_Spc();
               Z->check_value(LOC);
               return Token(TOK_APL_VALUE1, Z);
             }


        default: break;
      }

   return list_functions(COUT);

out_errno:
   return Token(TOK_APL_VALUE1, IntScalar(-errno, LOC));
}
//-----------------------------------------------------------------------------
Token
eval_AB(Value_P A, Value_P B, const NativeFunction * caller)
{
   if (A->get_rank() > 1)   RANK_ERROR;
   if (B->get_rank() > 1)   RANK_ERROR;

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

                Value_P Z(len, LOC);
                loop(m, len)
                    new (Z->next_ravel())   IntCell(Probe::get_time(probe, m));

                Probe::init(probe);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
             }
             
        default: break;
      }

   return list_functions(COUT);
}
//-----------------------------------------------------------------------------
Token
eval_XB(Value_P X, Value_P B, const NativeFunction * caller)
{
   if (B->get_rank() > 1)   RANK_ERROR;
   if (X->get_rank() > 1)   RANK_ERROR;

const int function_number = X->get_ravel(0).get_near_int();

   switch(function_number)
      {
         case 0:   // list functions
              return list_functions(CERR);

         case 1:   // return (last) errno
              goto out_errno;

         case 2:   // return strerror(B)
              {
                const int b = B->get_ravel(0).get_near_int();
                const char * text = strerror(b);
                const int len = strlen(text);
                Value_P Z(len, LOC);
                loop(t, len)
                    new (&Z->get_ravel(t))   CharCell((Unicode)(text[t]));

                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 3:   // fopen(Bs, "r") filename Bs
              {
                UTF8_string path(*B.get());
                errno = 0;
                FILE * f = fopen(path.c_str(), "r");
                if (f == 0)   return Token(TOK_APL_VALUE1, IntScalar(-1, LOC));

                file_entry fe(f, fileno(f));
                fe.fe_may_read = true;
                open_files.push_back(fe);
                return Token(TOK_APL_VALUE1, IntScalar(fe.fe_fd, LOC));
              }

         case 4:   // fclose(Bh)
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                if (fe.fe_FILE)   fclose(fe.fe_FILE);   // also closes fe.fe_fd
                else              close(fe.fe_fd);

                fe = open_files.back();       // move last file to fe
                open_files.pop_back();        // erase last file
                goto out_errno;
              }

         case 5:   // errno of Bh
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                return Token(TOK_APL_VALUE1, IntScalar(fe.fe_errno, LOC));
              }

         case 6:   // fread(Zi, 1, SMALL_BUF, Bh) 1 byte per Zi
              {
                errno = 0;
                FILE * file = get_FILE(*B.get());
                clearerr(file);

                char buffer[SMALL_BUF];

                const ssize_t len = fread(buffer, 1, SMALL_BUF, file);
                if (len < 0)   goto out_errno;

                Value_P Z(len, LOC);
                new (&Z->get_ravel(0)) IntCell(0);   // prototype
                loop(z, len)   new (Z->next_ravel()) IntCell(buffer[z] & 0xFF);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 8:   // fgets(Zi, SMALL_BUF, Bh) 1 byte per Zi\n"
              {
                errno = 0;
                FILE * file = get_FILE(*B.get());
                clearerr(file);

                char buffer[SMALL_BUF];

                const char * s = fgets(buffer, SMALL_BUF, file);
                const int len = s ? strlen(s) : 0;
                Value_P Z(len, LOC);
                new (&Z->get_ravel(0)) IntCell(0);   // prototype
                loop(z, len)   new (Z->next_ravel()) IntCell(buffer[z] & 0xFF);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 9:   // fgetc(Bh)
              {
                FILE * file = get_FILE(*B.get());
                return Token(TOK_APL_VALUE1, IntScalar(fgetc(file), LOC));
              }

         case 10:   // feof(Bh)
              {
                FILE * file = get_FILE(*B.get());
                return Token(TOK_APL_VALUE1, IntScalar(feof(file), LOC));
              }

         case 11:   // ferror(Bh)
              {
                FILE * file = get_FILE(*B.get());
                return Token(TOK_APL_VALUE1, IntScalar(ferror(file),LOC));
              }

         case 12:   // ftell(Bh)
              {
                FILE * file = get_FILE(*B.get());
                return Token(TOK_APL_VALUE1, IntScalar(ftell(file),LOC));
              }

         case 16:   // fflush(Bh)
              {
                errno = 0;
                FILE * file = get_FILE(*B.get());
                fflush(file);
              }
              goto out_errno;

         case 17:   // fsync(Bh)
              {
                errno = 0;
                const int fd = get_fd(*B.get());
                fsync(fd);
              }
              goto out_errno;

         case 18:   // fstat(Bh)
              {
                errno = 0;
                const int fd = get_fd(*B.get());
                struct stat s;
                const int result = fstat(fd, &s);
                if (result)   goto out_errno;   // fstat failed

                Value_P Z(Value_P(13, LOC));
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
                return Token(TOK_APL_VALUE1, IntScalar(fe.fe_fd,LOC));
              }

         case 25:   // pclose(Bh)
              {
                errno = 0;
                file_entry & fe = get_file(*B.get());
                int err = EBADF;   /* Bad file number */
                if (fe.fe_FILE)   err = pclose(fe.fe_FILE);

                fe = open_files.back();       // move last file to fe
                open_files.pop_back();        // erase last file

                if (err == -1)   goto out_errno;   // pclose() failed
                return Token(TOK_APL_VALUE1, IntScalar(err,LOC));
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

                Value_P Z(len, LOC);
                new (&Z->get_ravel(0)) CharCell(UNI_ASCII_SPACE);   // prototype
                loop(z, len)   new (Z->next_ravel()) CharCell((Unicode)data[z]);
                munmap((char *)data, len);

                Z->set_default_Spc();
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 28:   // read directory Bs
         case 29:   // read file names in directory Bs
              {
                errno = 0;
                UTF8_string path(*B.get());
                DIR * dir = opendir(path.c_str());
                if (dir == 0)   goto out_errno;

                vector<struct dirent> entries;
                for (;;)
                    {
                      dirent * entry = readdir(dir);
                      if (entry == 0)   break;   // directory done

                       // skip . and ..
                       //
                       const int entry_len = strlen(entry->d_name);
                       if (entry->d_name[0] == '.')
                          {
                            if (entry_len == 1 ||
                                (entry_len == 2 && entry->d_name[1] == '.'))
                               continue;
                          }

                      entries.push_back(*entry);
                    }
                closedir(dir);

                Shape shape_Z(entries.size());
                if (function_number == 28)   // 5 by N matrix
                   {
                     shape_Z.add_shape_item(5);
                   }

                Value_P Z(shape_Z, LOC);

                loop(e, entries.size())
                   {
                     const dirent & dent = entries[e];
                     if (function_number == 28)   // full dirent
                        {
                          // all platforms support inode number
                          //
                          new (Z->next_ravel())   IntCell(dent.d_ino);


#ifdef _DIRENT_HAVE_D_OFF
                          new (Z->next_ravel())   IntCell(dent.d_off);
#else
                          new (Z->next_ravel())   IntCell(-1);
#endif


#ifdef _DIRENT_HAVE_D_RECLEN
                          new (Z->next_ravel())   IntCell(dent.d_reclen);
#else
                          new (Z->next_ravel())   IntCell(-1);
#endif


#ifdef _DIRENT_HAVE_D_TYPE
                         new (Z->next_ravel())   IntCell(dent.d_type);
#else
                          new (Z->next_ravel())   IntCell(-1);
#endif
                        }   // function_number == 28

                     UCS_string filename(dent.d_name);
                     Value_P Z_name(filename, LOC);
                     new (Z->next_ravel())   PointerCell(Z_name, Z.getref());
                   }

                Z->set_default_Spc();
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 32:   // socket(Bi=AF_INET, SOCK_STREAM, 0)
              {
                errno = 0;
                APL_Integer domain = AF_INET;
                APL_Integer type = SOCK_STREAM;
                APL_Integer protocol = 0;
                if (B->element_count() > 0)
                   domain = B->get_ravel(0).get_int_value();
                if (B->element_count() > 1)
                   type = B->get_ravel(1).get_int_value();
                if (B->element_count() > 2)
                   protocol = B->get_ravel(2).get_int_value();
                const int sock = socket(domain, type, protocol);
                if (sock == -1)   goto out_errno;

                file_entry fe(0, sock);
                fe.fe_may_read = true;
                fe.fe_may_write = true;
                open_files.push_back(fe);
                return Token(TOK_APL_VALUE1, IntScalar(fe.fe_fd, LOC));
	      }

         case 34:   // listen(Bh, 10)
              {
                const int fd = get_fd(*B.get());
                errno = 0;
                listen(fd, 10);
                goto out_errno;
              }

         case 35:   // accept(Bh)
              {
                const int fd = get_fd(*B.get());
                errno = 0;
                sockaddr_in addr;
                socklen_t alen = sizeof(addr);
                const int sock = accept(fd, (sockaddr *)&addr, &alen);
                if (sock == -1)   goto out_errno;

                file_entry nfe (0, sock);
                open_files.push_back(nfe);

                Value_P Z(4, LOC);
                new (Z->next_ravel())   IntCell(nfe.fe_fd);
                new (Z->next_ravel())   IntCell(addr.sin_family);
                new (Z->next_ravel())   IntCell(ntohl(addr.sin_addr.s_addr));
                new (Z->next_ravel())   IntCell(ntohs(addr.sin_port));
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 37:   // recv(Bh, Zi, SMALL_BUF, 0) 1 byte per Zi
              {
                errno = 0;
                const int fd = get_fd(*B.get());

                char buffer[SMALL_BUF];

                const ssize_t len = recv(fd, buffer, sizeof(buffer), 0);
                if (len < 0)   goto out_errno;

                Value_P Z(len, LOC);
                new (&Z->get_ravel(0)) IntCell(0);   // prototype
                loop(z, len)   new (Z->next_ravel()) IntCell(buffer[z] & 0xFF);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 40:   // select(Br, Bw, Be, Bt)
              {
                fd_set readfds;     FD_ZERO(&readfds);
                fd_set writefds;    FD_ZERO(&writefds);
                fd_set exceptfds;   FD_ZERO(&exceptfds);
                timeval timeout = { 0, 0 };
                fd_set * rd = 0;
                fd_set * wr = 0;
                fd_set * ex = 0;
                timeval * to = 0;
                APL_Integer max_fd = -1;

                if (B->element_count() > 4)   LENGTH_ERROR;
                if (B->element_count() < 1)   LENGTH_ERROR;

                if (B->element_count() >= 4)
                   {
                     const APL_Integer milli = B->get_ravel(3).get_int_value();
                     if (milli < 0)   DOMAIN_ERROR;

                     timeout.tv_sec = milli / 1000;
                     timeout.tv_usec = (milli%1000) * 1000;
                   }

                if (B->element_count() >= 3)
                   {
                      Value_P vex = B->get_ravel(2).get_pointer_value();
                      loop(l, vex->element_count())
                          {
                            const APL_Integer fd =
                                  vex->get_ravel(l).get_int_value();
                            if (fd < 0)                  DOMAIN_ERROR;
                            if (fd > 8*sizeof(fd_set))   DOMAIN_ERROR;
                            FD_SET(fd, &exceptfds);
                            if (max_fd < fd)   max_fd = fd;
                            ex = &exceptfds;
                          }
                   }

                if (B->element_count() >= 2)
                   {
                      Value_P vwr = B->get_ravel(1).get_pointer_value();
                      loop(l, vwr->element_count())
                          {
                            const APL_Integer fd =
                                  vwr->get_ravel(l).get_int_value();
                            if (fd < 0)                  DOMAIN_ERROR;
                            if (fd > 8*sizeof(fd_set))   DOMAIN_ERROR;
                            FD_SET(fd, &writefds);
                            if (max_fd < fd)   max_fd = fd;
                            wr = &writefds;
                          }
                   }

                if (B->element_count() >= 1)
                   {
                      Value_P vrd = B->get_ravel(0).get_pointer_value();
                      loop(l, vrd->element_count())
                          {
                            const APL_Integer fd =
                                  vrd->get_ravel(l).get_int_value();
                            if (fd < 0)                  DOMAIN_ERROR;
                            if (fd > 8*sizeof(fd_set))   DOMAIN_ERROR;
                            FD_SET(fd, &readfds);
                            if (max_fd < fd)   max_fd = fd;
                            rd = &readfds;
                          }
                   }

                const int count = select(max_fd + 1, rd, wr, ex, to);
                if (count < 0)   goto out_errno;

                Value_P Z(5, LOC);
                new (Z->next_ravel())   IntCell(count);
                new (Z->next_ravel())
                    PointerCell(fds_to_val(rd, max_fd), Z.getref());
                new (Z->next_ravel())
                    PointerCell(fds_to_val(wr, max_fd), Z.getref());
                new (Z->next_ravel())
                    PointerCell(fds_to_val(ex, max_fd), Z.getref());
                new (Z->next_ravel())
                    IntCell(timeout.tv_sec*1000 + timeout.tv_usec/1000);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 41:   // read(Bh, Zi, SMALL_BUF) 1 byte per Zi
              {
                errno = 0;
                const int fd = get_fd(*B.get());

                char buffer[SMALL_BUF];

                const ssize_t len = read(fd, buffer, sizeof(buffer));
                if (len < 0)   goto out_errno;

                Value_P Z(len, LOC);
                new (&Z->get_ravel(0)) IntCell(0);   // prototype
                loop(z, len)   new (Z->next_ravel()) IntCell(buffer[z] & 0xFF);
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 44:   // getsockname(Bh, Zi)
              {
                const int fd = get_fd(*B.get());
                errno = 0;
                sockaddr_in addr;
                socklen_t alen = sizeof(addr);
                const int ret = getsockname(fd, (sockaddr *)&addr, &alen);
                if (ret == -1)   goto out_errno;

                Value_P Z(3, LOC);
                new (Z->next_ravel())   IntCell(addr.sin_family);
                new (Z->next_ravel())   IntCell(ntohl(addr.sin_addr.s_addr));
                new (Z->next_ravel())   IntCell(ntohs(addr.sin_port));
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 45:   // getpeername(Bh, Zi)
              {
                const int fd = get_fd(*B.get());
                errno = 0;
                sockaddr_in addr;
                socklen_t alen = sizeof(addr);
                const int ret = getpeername(fd, (sockaddr *)&addr, &alen);
                if (ret == -1)   goto out_errno;

                Value_P Z(3, LOC);
                new (Z->next_ravel())   IntCell(addr.sin_family);
                new (Z->next_ravel())   IntCell(ntohl(addr.sin_addr.s_addr));
                new (Z->next_ravel())   IntCell(ntohs(addr.sin_port));
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }


         case 200:   // clear statistics Bi
         case 201:   // get statistics Bi
              {
                const Pfstat_ID b = (Pfstat_ID)B->get_ravel(0).get_int_value();
                Statistics * stat = Performance::get_statistics(b);
                if (stat == 0)   DOMAIN_ERROR;   // bad statistics ID

                if (function_number == 200)   // reset statistics
                   {
                     stat->reset();
                     return Token(TOK_APL_VALUE1, IntScalar(b, LOC));
                   }

                // get statistics
                //
                 const int t = Performance::get_statistics_type(b);
                 UCS_string stat_name(stat->get_name());
                 Value_P Z1(stat_name, LOC);
                 if (t <= 2)   // cell function statistics
                    {
                       const Statistics_record * r1 = stat->get_first_record();
                       const Statistics_record * rN = stat->get_record();
                       Value_P Z(8, LOC);
                       new (Z->next_ravel())   IntCell(t);
                       new (Z->next_ravel())   PointerCell(Z1, Z.getref());
                       new (Z->next_ravel())   IntCell(r1->get_count());
                       new (Z->next_ravel())   IntCell(r1->get_sum());
                       new (Z->next_ravel())   FloatCell(r1->get_sum2());
                       new (Z->next_ravel())   IntCell(rN->get_count());
                       new (Z->next_ravel())   IntCell(rN->get_sum());
                       new (Z->next_ravel())   FloatCell(rN->get_sum2());
                       Z->check_value(LOC);
                       return Token(TOK_APL_VALUE1, Z);
                    }
                 else           // function statistics
                    {
                       const Statistics_record * r = stat->get_record();
                       Value_P Z(5, LOC);
                       new (Z->next_ravel())   IntCell(t);
                       new (Z->next_ravel())   PointerCell(Z1, Z.getref());
                       new (Z->next_ravel())   IntCell(r->get_count());
                       new (Z->next_ravel())   IntCell(r->get_sum());
                       new (Z->next_ravel())   FloatCell(r->get_sum2());
                       Z->check_value(LOC);
                       return Token(TOK_APL_VALUE1, Z);
                    }
              }

         case 202:   // get monadic parallel threshold
         case 203:   // get dyadicadic parallel threshold
              {
                const Function * fun = 0;
                if (B->element_count() == 3)   // dyadic operator
                   {
                     const Unicode lfun = B->get_ravel(0).get_char_value();
                     const Unicode oper = B->get_ravel(1).get_char_value();
                     if (oper == UNI_ASCII_FULLSTOP)
                        {
                          if (lfun == UNI_RING_OPERATOR)   // ∘.g
                             fun = Bif_OPER2_OUTER::fun;
                          else                             // f.g
                             fun = Bif_OPER2_INNER::fun;
                        }
                   }
                else
                   {
                     const Unicode prim = B->get_ravel(0).get_char_value();
                     const Token tok = Tokenizer::tokenize_function(prim);
                     if (!tok.is_function())   DOMAIN_ERROR;
                     fun = tok.get_function();
                   }
                if (fun == 0)   DOMAIN_ERROR;
                APL_Integer old_threshold;
                if (function_number == 202)
                   {
                     old_threshold = fun->get_monadic_threshold();
                   }
                else
                   {
                     old_threshold = fun->get_dyadic_threshold();
                   }

                 return Token(TOK_APL_VALUE1, IntScalar(old_threshold, LOC));
              }

        default: break;
      }

   CERR << "Bad eval_XB() function number: " << function_number << endl;
   DOMAIN_ERROR;

out_errno:
   return Token(TOK_APL_VALUE1, IntScalar(-errno, LOC));
}
//-----------------------------------------------------------------------------
Token
eval_AXB(const Value_P A, const Value_P X, const Value_P B,
         const NativeFunction * caller)
{
   if (A->get_rank() > 1)   RANK_ERROR;
   if (B->get_rank() > 1)   RANK_ERROR;
   if (X->get_rank() > 1)   RANK_ERROR;

const int function_number = X->get_ravel(0).get_near_int();

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
                return Token(TOK_APL_VALUE1, IntScalar(fe.fe_fd,LOC));
              }

         case 6:   // fread(Zi, 1, Ai, Bh) 1 byte per Zi\n"
              {
                errno = 0;
                const int bytes = A->get_ravel(0).get_near_int();
                FILE * file = get_FILE(*B.get());
                clearerr(file);

                char small_buffer[SMALL_BUF];
                char * buffer = small_buffer;
                char * del = 0;
                if (bytes > sizeof(small_buffer))
                   buffer = del = new char[bytes];

                const size_t len = fread(buffer, 1, bytes, file);
                if (len < 0)   goto out_errno;
                Value_P Z(len, LOC);
                new (&Z->get_ravel(0)) IntCell(0);   // prototype
                loop(z, len)   new (Z->next_ravel()) IntCell(buffer[z] & 0xFF);
                delete [] del;
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 7:   // fwrite(Ai, 1, ⍴Ai, Bh) 1 byte per Zi\n"
              {
                errno = 0;
                const int bytes = A->element_count();
                FILE * file = get_FILE(*B.get());

                char small_buffer[SMALL_BUF];
                char * buffer = small_buffer;
                char * del = 0;
                if (bytes > sizeof(small_buffer))
                   buffer = del = new char[bytes];

                loop(z, bytes)   buffer[z] = A->get_ravel(z).get_near_int();

                const size_t len = fwrite(buffer, 1, bytes, file);
                delete [] del;

                return Token(TOK_APL_VALUE1, IntScalar(len, LOC));
              }

         case 8:   // fgets(Zi, Ai, Bh) 1 byte per Zi\n"
              {
                errno = 0;
                const int bytes = A->get_ravel(0).get_near_int();
                FILE * file = get_FILE(*B.get());
                clearerr(file);

                char small_buffer[SMALL_BUF];
                char * buffer = small_buffer;
                char * del = 0;
                if (bytes > sizeof(buffer))
                   buffer = del = new char[bytes + 1];

                const char * s = fgets(buffer, bytes, file);
                const int len = s ? strlen(s) : 0;
                Value_P Z(len, LOC);
                new (&Z->get_ravel(0)) IntCell(0);   // prototype
                loop(z, len)   new (Z->next_ravel()) IntCell(buffer[z] & 0xFF);
                delete [] del;
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 13:   // fseek(Bh, Ai, SEEK_SET)
              {
                errno = 0;
                FILE * file = get_FILE(*B.get());
                const APL_Integer pos = A->get_ravel(0).get_near_int();
                fseek(file, pos, SEEK_SET);
              }
              goto out_errno;

         case 14:   // fseek(Bh, Ai, SEEK_CUR)
              {
                errno = 0;
                FILE * file = get_FILE(*B.get());
                const APL_Integer pos = A->get_ravel(0).get_near_int();
                fseek(file, pos, SEEK_CUR);
              }
              goto out_errno;

         case 15:   // fseek(Bh, Ai, SEEK_END)
              {
                errno = 0;
                FILE * file = get_FILE(*B.get());
                const APL_Integer pos = A->get_ravel(0).get_near_int();
                fseek(file, pos, SEEK_END);
              }
              goto out_errno;

         case 20:   // mkdir(Bc, Ai)
              {
                const int mask = A->get_ravel(0).get_near_int();
                UTF8_string path(*B.get());
                mkdir(path.c_str(), mask);
              }
              goto out_errno;

         case 22:   // fprintf(Bh, A)
              {
                errno = 0;
                FILE * file = get_FILE(*B.get());
                return do_printf(file, A);
              }

         case 23:   // fwrite(Ac, 1, ⍴Ac, Bh) Unicode Ac Output UTF-8 \n"
              {
                errno = 0;
                FILE * file = get_FILE(*B.get());
                UCS_string text(*A.get());
                UTF8_string utf(text);
                const size_t len = fwrite(utf.c_str(), 1, utf.size(), file); 
                return Token(TOK_APL_VALUE1, IntScalar(len, LOC));
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
                return Token(TOK_APL_VALUE1, IntScalar(fe.fe_fd, LOC));
              }

         case 27:   // rename(As, Bs)
              {
                UTF8_string old_name(*A.get());
                UTF8_string new_name(*B.get());
                errno = 0;
                const int result = rename(old_name.c_str(), new_name.c_str());
                if (result && errno)   goto out_errno;   // errno should be set

               return Token(TOK_APL_VALUE1, IntScalar(result, LOC));
              }

         case 31:   // access
              {
                UTF8_string permissions(*A.get());
                UTF8_string path(*B.get());
                int perms = 0;
                loop(a, permissions.size())
                    {
                      int p = permissions[a] & 0xFF;
                      if      (p == 'R')   perms |= R_OK;
                      else if (p == 'r')   perms |= R_OK;
                      else if (p == 'W')   perms |= W_OK;
                      else if (p == 'w')   perms |= W_OK;
                      else if (p == 'X')   perms |= X_OK;
                      else if (p == 'x')   perms |= X_OK;
                      else if (p == 'F')   perms |= F_OK;
                      else if (p == 'f')   perms |= F_OK;
                      else DOMAIN_ERROR;
                    }

                 errno = 0;
                 const int not_ok = access(path.c_str(), perms);
                 if (not_ok)   goto out_errno;
                 return Token(TOK_APL_VALUE1, IntScalar(0, LOC));
              }

         case 33:   // bind(Bh, Aa)
              {
                const int fd = get_fd(*B.get());
                sockaddr_in addr;
                memset(&addr, 0, sizeof(addr));
                addr.sin_family      =       A->get_ravel(0).get_int_value();
                addr.sin_addr.s_addr = htonl(A->get_ravel(1).get_int_value());
                addr.sin_port        = htons(A->get_ravel(2).get_int_value());
                errno = 0;
                bind(fd, (const sockaddr *)&addr, sizeof(addr));
                goto out_errno;
              }

         case 34:   // listen(Bh, Ai)
              {
                const int fd = get_fd(*B.get());
                APL_Integer backlog = 10;
                if (A->element_count() > 0)
                   backlog = A->get_ravel(0).get_int_value();

                errno = 0;
                listen(fd, backlog);
                goto out_errno;
              }

         case 36:   // connect(Bh, Aa)
              {
                const int fd = get_fd(*B.get());
                errno = 0;
                sockaddr_in addr;
                memset(&addr, 0, sizeof(addr));
                addr.sin_family      =       A->get_ravel(0).get_int_value();
                addr.sin_addr.s_addr = htonl(A->get_ravel(1).get_int_value());
                addr.sin_port        = htons(A->get_ravel(2).get_int_value());
                errno = 0;
                connect(fd, (const sockaddr *)&addr, sizeof(addr));
                goto out_errno;
              }

         case 37:   // recv(Bh, Zi, Ai, 0) 1 byte per Zi
              {
                const int bytes = A->get_ravel(0).get_near_int();
                const int fd = get_fd(*B.get());
                errno = 0;

                char small_buffer[SMALL_BUF];
                char * buffer = small_buffer;
                char * del = 0;
                if (bytes > sizeof(small_buffer))
                   buffer = del = new char[bytes];

                const ssize_t len = recv(fd, buffer, bytes, 0);
                if (len < 0)   goto out_errno;

                Value_P Z(len, LOC);
                new (&Z->get_ravel(0)) IntCell(0);   // prototype
                loop(z, len)   new (Z->next_ravel()) IntCell(buffer[z] & 0xFF);
                delete [] del;
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 38:   // send(Bh, Ai, ⍴Ai, 0) 1 byte per Zi\n"
              {
                errno = 0;
                const int bytes = A->element_count();
                const int fd = get_fd(*B.get());

                char small_buffer[SMALL_BUF];
                char * buffer = small_buffer;
                char * del = 0;
                if (bytes > sizeof(small_buffer))
                   buffer = del = new char[bytes];

                loop(z, bytes)   buffer[z] = A->get_ravel(z).get_near_int();

                const ssize_t len = send(fd, buffer, bytes, 0);
                if (len < 0)   goto out_errno;
                delete [] del;

                return Token(TOK_APL_VALUE1, IntScalar(len, LOC));
              }

         case 39:   // send(Bh, Ac, ⍴Ac, 0) Unicode Ac Output UTF-8 \n"
              {
                UCS_string text(*A.get());
                UTF8_string utf(text);
                const int fd = get_fd(*B.get());
                errno = 0;
                const ssize_t len = send(fd, utf.c_str(), utf.size(), 0); 
                if (len < 0)   goto out_errno;

                return Token(TOK_APL_VALUE1, IntScalar(len, LOC));
              }

         case 41:   // read(Bh, Zi, Ai) 1 byte per Zi
              {
                const int bytes = A->get_ravel(0).get_near_int();
                const int fd = get_fd(*B.get());

                char small_buffer[SMALL_BUF];
                char * buffer = small_buffer;
                char * del = 0;
                if (bytes > sizeof(small_buffer))
                   buffer = del = new char[bytes];

                errno = 0;
                const ssize_t len = read(fd, buffer, bytes);
                if (len < 0)   goto out_errno;

                Value_P Z(len, LOC);
                new (&Z->get_ravel(0)) IntCell(0);   // prototype
                loop(z, len)   new (Z->next_ravel()) IntCell(buffer[z] & 0xFF);
                delete [] del;
                Z->check_value(LOC);
                return Token(TOK_APL_VALUE1, Z);
              }

         case 42:   // write(Bh, Ai, ⍴Ai) 1 byte per Zi\n"
              {
                const int bytes = A->element_count();
                const int fd = get_fd(*B.get());

                char small_buffer[SMALL_BUF];
                char * buffer = small_buffer;
                char * del = 0;
                if (bytes > sizeof(small_buffer))
                   buffer = del = new char[bytes];

                loop(z, bytes)   buffer[z] = A->get_ravel(z).get_near_int();

                errno = 0;
                const ssize_t len = write(fd, buffer, bytes);
                delete [] del;
                if (len < 0)   goto out_errno;

                return Token(TOK_APL_VALUE1, IntScalar(len, LOC));
              }

         case 43:   // write(Bh, Ac, ⍴Ac) Unicode Ac Output UTF-8 \n"
              {
                const int fd = get_fd(*B.get());
                UCS_string text(*A.get());
                UTF8_string utf(text);
                errno = 0;
                const ssize_t len = write(fd, utf.c_str(), utf.size()); 
                if (len < 0)   goto out_errno;
                return Token(TOK_APL_VALUE1, IntScalar(len, LOC));
              }

         case 46:   // getsockopt(Bh, A_level, A_optname, Zi)\n"
              {
                const APL_Integer level = A->get_ravel(0).get_int_value();
                const APL_Integer optname = A->get_ravel(1).get_int_value();
                const int fd = get_fd(*B.get());
                int optval = 0;
                socklen_t olen = sizeof(optval);
                errno = 0;
                const int ret = getsockopt(fd, level, optname, &optval, &olen);
                if (ret < 0)   goto out_errno;
                return Token(TOK_APL_VALUE1, IntScalar(optval, LOC));
              }

         case 47:   // setsockopt(Bh, A_level, A_optname, A_optval)\n"
              {
                const APL_Integer level = A->get_ravel(0).get_int_value();
                const APL_Integer optname = A->get_ravel(1).get_int_value();
                const int optval =  A->get_ravel(2).get_int_value();
                const int fd = get_fd(*B.get());
                errno = 0;
                setsockopt(fd, level, optname, &optval, sizeof(optval));
                goto out_errno;
              }

         case 202:   // set monadic parallel threshold
         case 203:   // set dyadicadic parallel threshold
              {
                const APL_Integer threshold = A->get_ravel(0).get_int_value();
                Function * fun = 0;
                if (B->element_count() == 3)   // dyadic operator
                   {
                     const Unicode oper = B->get_ravel(1).get_char_value();
                     if (oper != UNI_ASCII_FULLSTOP)   DOMAIN_ERROR;
                     fun = Bif_OPER2_INNER::fun;
                   }
                else
                   {
                     const Unicode prim = B->get_ravel(0).get_char_value();
                     const Token tok = Tokenizer::tokenize_function(prim);
                     if (!tok.is_function())   DOMAIN_ERROR;
                     fun = tok.get_function();
                   }
                if (fun == 0)   DOMAIN_ERROR;
                APL_Integer old_threshold;
                if (function_number == 202)
                   {
                     old_threshold = fun->get_monadic_threshold();
                     fun->set_monadic_threshold(threshold);
                   }
                else
                   {
                     old_threshold = fun->get_dyadic_threshold();
                     fun->set_dyadic_threshold(threshold);
                   }

                 return Token(TOK_APL_VALUE1, IntScalar(old_threshold, LOC));
              }

        default: break;
      }

   CERR << "eval_AXB() function number: " << function_number << endl;
   DOMAIN_ERROR;

out_errno:
   return Token(TOK_APL_VALUE1, IntScalar(-errno, LOC));
}
//-----------------------------------------------------------------------------

