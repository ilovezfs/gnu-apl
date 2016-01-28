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

   //
   // the file_io native function is now obsolete. It has been replaced by ⎕FIO
   // and the native function has become a wrapper to ⎕FIO
   //

#include "../Quad_FIO.hh"

class NativeFunction;

enum { SMALL_BUF = Quad_FIO::SMALL_BUF };

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

   return Quad_FIO::fun->eval_B(B);
}
//-----------------------------------------------------------------------------
Token
eval_AB(Value_P A, Value_P B, const NativeFunction * caller)
{
   return Quad_FIO::fun->eval_AB(A, B);
}
//-----------------------------------------------------------------------------
Token
eval_XB(Value_P X, Value_P B, const NativeFunction * caller)
{
   return Quad_FIO::fun->eval_XB(X, B);
}
//-----------------------------------------------------------------------------
Token
eval_AXB(const Value_P A, const Value_P X, const Value_P B,
         const NativeFunction * caller)
{
   return Quad_FIO::fun->eval_AXB(A, X, B);
}
//-----------------------------------------------------------------------------

