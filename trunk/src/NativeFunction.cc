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


#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "Error.hh"
#include "makefile.h"
#include "NativeFunction.hh"
#include "Symbol.hh"
#include "Workspace.hh"

vector<NativeFunction *> NativeFunction::valid_functions;

//-----------------------------------------------------------------------------
NativeFunction::NativeFunction(const UCS_string & so_name,
                               const UCS_string & apl_name)
   : Function(ID_USER_SYMBOL, TOK_FUN2),
     handle(0),
     name(apl_name),
     so_path(so_name),
     valid(false),
     close_fun(0)
{
   open_so_file();
   if (handle == 0)   return;
   Workspace::more_error() = UCS_string("shared library ");
   Workspace::more_error().append(so_name);

   // get the function multiplexer
   //
void * fmux = dlsym(handle, "get_function_mux");
     if (!fmux)
        {
          CERR << "shared library " << so_name << " is lacking the mandatory "
                  "function get_function_mux() !" << endl;
          Workspace::more_error().append_utf8(
                                  "is invalid (no get_function_mux())");
          return;
        }

void * (*get_function_mux)(const char *) = (void * (*)(const char *))fmux;

   // get the mandatory function get_signature() which returns
   //  the function signature
   //
   {
     void * get_sig = get_function_mux("get_signature");
     if (!get_sig)
        {
          CERR << "shared library is lacking the mandatory "
                  "function signature() !" << endl;
          Workspace::more_error().append_utf8(
                                  "is invalid (no get_signature())");
          return;
        }


     signature = ((Fun_signature (*)())get_sig)();
   }

   // get the optional function close_fun(), which is called before
   // this function disappears
   {
     void * cfun = get_function_mux("close_fun");
     if (cfun)   close_fun = (bool (*)(Cause, const NativeFunction *))cfun;
     else        close_fun = 0;
   }

   // create an entry in the symbol table
   //
Symbol * sym = Workspace::lookup_symbol(apl_name);
   Assert(sym);
const char * why_not = sym->cant_be_defined();
   if (why_not)
      {
        Workspace::more_error() = UCS_string(why_not);
        return;
      }

   /// read function pointers...
   //
#define Th const NativeFunction * th

#define ev(fun, args) f_ ## fun = (Token (*) args ) get_function_mux(#fun)

   ev(eval_        , (                                Th));

   ev(eval_B       , (                          Vr B, Th));
   ev(eval_AB      , (Vr A,                     Vr B, Th));
   ev(eval_XB      , (                    Vr X, Vr B, Th));
   ev(eval_AXB     , (Vr A,               Vr X, Vr B, Th));

   ev(eval_LB      , (      Fr LO,              Vr B, Th));
   ev(eval_ALB     , (Vr A, Fr LO,              Vr B, Th));
   ev(eval_LXB     , (      Fr LO,        Vr X, Vr B, Th));
   ev(eval_ALXB    , (Vr A, Fr LO,        Vr X, Vr B, Th));

   ev(eval_LRB     , (      Fr LO, Fr RO,       Vr B, Th));
   ev(eval_LRXB    , (      Fr LO, Fr RO, Vr X, Vr B, Th));
   ev(eval_ALRB    , (Vr A, Fr LO, Fr RO,       Vr B, Th));
   ev(eval_ALRXB   , (Vr A, Fr LO, Fr RO, Vr X, Vr B, Th));

   ev(eval_fill_B  , (                          Vr B, Th));
   ev(eval_fill_AB , (Vr A,                     Vr B, Th));
   ev(eval_ident_Bx, (Vr B,             Axis x,       Th));
#undef ev

   // compute function tag based on the signature
   //
   if      (signature & SIG_RO)   tag = TOK_OPER2;
   else if (signature & SIG_LO)   tag = TOK_OPER1;
   else if (signature & SIG_B )   tag = TOK_FUN2;
   else                           tag = TOK_FUN0;

   if (is_operator())   sym->set_nc(NC_OPERATOR, this);
   else                 sym->set_nc(NC_FUNCTION, this);

   Workspace::more_error().clear();
   valid = true;
   valid_functions.push_back(this);
}
//-----------------------------------------------------------------------------
NativeFunction::~NativeFunction()
{
  Log(LOG_UserFunction__enter_leave)
      CERR << "Native function " << get_name() << " deleted." << endl;

   loop(v, valid_functions.size())
      {
        if (valid_functions[v] == this)
           {
             valid_functions.erase(valid_functions.begin() + v);
           }
      }
}
//-----------------------------------------------------------------------------
void
NativeFunction::try_one_file(const char * filename)
{
UCS_string & t4 = Workspace::more_error();
const int t4_len = t4.size();
   t4.append_utf8("    file ");
   t4.append_utf8(filename);

   if (access(filename, R_OK) != 0)
      {
        while (t4.size() < t4_len + 44)     t4.append_utf8(" ");
        t4.append_utf8(" (");
        t4.append_utf8(strerror(errno));
        t4.append_utf8(")\n");
        return;
      }

   handle = dlopen(filename, RTLD_NOW);
   if (handle == 0)
      {
        const char * err = dlerror();
        if (strrchr(err, ':'))   err = 1 + strrchr(err, ':');

        while (t4.size() < t4_len + 44)     t4.append_utf8(" ");
        t4.append_utf8(" (");
        t4.append_utf8(err);
        t4.append_utf8(" )\n");
      }
}
//-----------------------------------------------------------------------------
void
NativeFunction::open_so_file()
{
   // prepare a )MORE error message containing the file names tried.
   //
UCS_string & t4 = Workspace::more_error();
   t4.clear();
   t4.append_utf8("Could not find shared library '");
   t4.append(so_path);
   t4.append_utf8("'\n"
                  "The following directories and file names were tried:\n");

   // if the name starts with / (or \ on Windows) or .
   // then take it as is without changes.
   //
   if (so_path[0] == UNI_ASCII_SLASH     ||
       so_path[0] == UNI_ASCII_BACKSLASH ||
       so_path[0] == UNI_ASCII_FULLSTOP)
      {
        UTF8_string filename(so_path);
        try_one_file(filename.c_str());

        if (handle == 0)   t4.append_utf8(
            "NOTE: Filename extensions are NOT automatically added "
            "when a full path\n"
            "      (i.e. a path starting with / or .) is used.");
        return;
      }

   // otherwise try Makefile__pkglibdir . /usr/lib/apl and /usr/local/lib/apl,
   // avoiding duplicates
   //
UTF8_string utf_so_path(so_path);
const char * dirs[] =
{
  Makefile__pkglibdir,    // the normal case
  "/usr/lib/apl",
  "/usr/local/lib/apl",
  ".",
  "./native",             // if make install was not performed
  "./emacs_mode",         // if make install was not performed
};

   // most likely Makefile__pkglibdir is /usr/lib/apl or /usr/local/lib/apl.
   // don't try them twice.
   //
   if (!strcmp(Makefile__pkglibdir, dirs[1]))   dirs[1] = 0;
   if (!strcmp(Makefile__pkglibdir, dirs[2]))   dirs[2] = 0;

   loop(d, sizeof(dirs) / sizeof(*dirs))
       {
         if (dirs[d] == 0)   continue;

         UTF8_string dir_so_path(dirs[d]);
         dir_so_path.append(UTF8_string("/"));
         dir_so_path.append(utf_so_path);

         UTF8_string dir_only(dir_so_path);
         dir_only[strrchr(dir_only.c_str(), '/') - dir_only.c_str()] = 0;
         if (access(dir_only.c_str(), R_OK | X_OK) != 0)
            {
              t4.append_utf8("    directory ");
              t4.append_utf8(dir_only.c_str());
              t4.append_utf8("\n");
              continue;   // new directory
            }

         // try filename, then filename.so, and then filename.dylib
         // unless the filename has an extension already.
         //
         const char * exts[] = { ".so", ".dylib", "" };

         bool has_extension = false;
         loop(e, sizeof(exts)/sizeof(*exts))
            {
              if (*exts[e])   // not "no extension"
                 {
                   const char * ext = exts[e];
                   const char * end = dir_so_path.c_str();
                   end += strlen(end) - strlen(ext);
                   if (!strcmp(exts[e],  end))
                      {
                        has_extension = true;
                        break;
                      }
                       
                 }
            }

         loop(e, sizeof(exts)/sizeof(*exts))
             {
               if (has_extension && *exts[e])   continue;

               UTF8_string filename(dir_so_path);
               if (exts[e])   filename.append(UTF8_string(exts[e]));

               try_one_file(filename.c_str());
             }
       }
}
//-----------------------------------------------------------------------------
void
NativeFunction::cleanup()
{
   // delete in reverse construction order
   //
   loop(v, valid_functions.size())
      {
        NativeFunction & fun = *valid_functions.back();
        if (fun.close_fun && fun.handle)
           {
             const bool do_dlclose = (*fun.close_fun)(CAUSE_SHUTDOWN, &fun);
             if (do_dlclose)
                {
                  dlclose(fun.handle);
                  fun.handle = 0;
                }
           }
        delete valid_functions[v];
      }
}
//-----------------------------------------------------------------------------
void
NativeFunction::destroy()
{
   if (close_fun)   (*close_fun)(CAUSE_ERASED, this);
   if (close_fun && handle)
      {
        const bool do_dlclose = (*close_fun)(CAUSE_ERASED, this);
        if (do_dlclose)
           {
             dlclose(handle);
             handle = 0;
           }
      }
   delete this;
}
//-----------------------------------------------------------------------------
NativeFunction *
NativeFunction::fix(const UCS_string & so_name,
                    const UCS_string & function_name)
{
   // if the function already exists then return it.
   //
   loop(v, valid_functions.size())
      {
        if (so_name == valid_functions[v]->so_path)   return valid_functions[v];
      }

NativeFunction * new_function = new NativeFunction(so_name, function_name);

   if (!new_function->valid)   // something went wrong
      {
        delete new_function;
        return 0;
      }

   return new_function;
}
//-----------------------------------------------------------------------------
bool
NativeFunction::has_result() const
{
   return !!(signature & SIG_Z);
}
//-----------------------------------------------------------------------------
bool
NativeFunction::is_operator() const
{
   // this function is an operator if one of the eval functions with a
   // function argument is present...
   //
   return f_eval_LB
       || f_eval_ALB
       || f_eval_LRB
       || f_eval_ALRB
       || f_eval_LXB
       || f_eval_ALXB
       || f_eval_LRXB
       || f_eval_ALRXB;
}
//-----------------------------------------------------------------------------
void
NativeFunction::print_properties(ostream & out, int indent) const
{
UCS_string ind(indent, UNI_ASCII_SPACE);
   out << ind << "Native Function " << endl;
}
//-----------------------------------------------------------------------------
UCS_string
NativeFunction::canonical(bool with_lines) const
{
   return so_path;
}
//-----------------------------------------------------------------------------
ostream &
NativeFunction::print(std::ostream & out) const
{
   if (is_operator())   out << "native Operator " << name << endl;
   else                 out << "native Function " << name << endl;

   return out;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_()
{
   if (f_eval_)   return (*f_eval_)(this);

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_B(Value_P B)
{
   if (f_eval_B)   return (*f_eval_B)(B, this);

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_AB(Value_P A, Value_P B)
{
   if (f_eval_AB)   return (*f_eval_AB)(A, B, this);

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_LB(Token & LO, Value_P B)
{
   if (f_eval_LB)   return (*f_eval_LB)(*LO.get_function(), B, this);

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_ALB(Value_P A, Token & LO, Value_P B)
{
   if (f_eval_ALB)   return (*f_eval_ALB)(A, *LO.get_function(), B, this);

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_LRB(Token & LO, Token & RO, Value_P B)
{
   if (f_eval_LRB)   return (*f_eval_LRB)(*LO.get_function(),
                                          *RO.get_function(), B, this);

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_ALRB(Value_P A, Token & LO, Token & RO, Value_P B)
{
   if (f_eval_ALRB)   return (*f_eval_ALRB)(A, *LO.get_function(),
                                               *RO.get_function(), B, this);

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_XB(Value_P X, Value_P B)
{
   // call axis variant if present, or else the non-axis variant.
   //
   if (f_eval_XB)   return (*f_eval_XB)(X, B, this);
   else             return eval_B(B);
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_AXB(Value_P A, Value_P X, Value_P B)
{
   // call axis variant if present, or else the non-axis variant.
   //
   if (f_eval_AXB)   return (*f_eval_AXB)(A, X, B, this);
   else              return eval_AB(A, B);
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_LXB(Token & LO, Value_P X, Value_P B)
{
   // call axis variant if present, or else the non-axis variant.
   //
   if (f_eval_LXB)   return (*f_eval_LXB)(*LO.get_function(), X, B, this);
   else              return eval_LB(LO, B);
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_ALXB(Value_P A, Token & LO, Value_P X, Value_P B)
{
   // call axis variant if present, or else the non-axis variant.
   //
   if (f_eval_ALXB)   return (*f_eval_ALXB)(A, *LO.get_function(), X, B, this);
   else               return eval_ALB(A, LO, B);
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_LRXB(Token & LO, Token & RO, Value_P X, Value_P B)
{
   // call axis variant if present, or else the non-axis variant.
   //
   if (f_eval_LRXB)   return (*f_eval_LRXB)(*LO.get_function(),
                                            *RO.get_function(), X, B, this);
   else                return eval_LRB(LO, RO, B);
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_ALRXB(Value_P A, Token & LO, Token & RO, Value_P X, Value_P B)
{
   // call axis variant if present, or else the non-axis variant.
   //
   if (f_eval_ALRXB)   return (*f_eval_ALRXB)(A, *LO.get_function(),
                                                 *RO.get_function(), X, B, this);
   else                return eval_ALRB(A, LO, RO, B);
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_fill_B(Value_P B)
{
   if (f_eval_fill_B)   return (*f_eval_fill_B)(B, this);

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_fill_AB(Value_P A, Value_P B)
{
   if (f_eval_fill_AB)   return (*f_eval_fill_AB)(A, B, this);

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_identity_fun(Value_P B, Axis axis)
{
   if (f_eval_ident_Bx)   return (*f_eval_ident_Bx)(B, axis, this);

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------

