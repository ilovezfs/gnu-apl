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

#include "Error.hh"
#include "NativeFunction.hh"
#include "Symbol.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
NativeFunction::NativeFunction(void * so_handle, UCS_string apl_name)
   : Function(ID_USER_SYMBOL, TOK_FUN2),
     handle(so_handle),
     name(apl_name),
     valid(false)
{
   // create an entry in the symbol table
   //
Symbol * sym = Workspace::lookup_symbol(apl_name);
   Assert(sym);
   if (!sym->can_be_defined())   return;

   /// read function pointers...
   //
#define fun(f, a, m) f_eval_ ## f = (Token (*) a )dlsym(handle, #m);
   fun(            , ()                              , xxx);
   fun(B           , (Vr B)                          , xxx)
   fun(AB          , (Vr A, Vr B)                    , xxx)
   fun(LB          , (Fr LO, Vr B)                   , xxx)
   fun(ALB         , (Vr A, Fr LO, Vr B)             , xxx)
   fun(LRB         , (Fr LO, Fr RO, Vr B)            , xxx)
   fun(ALRB        , (Vr A, Fr LO, Fr RO, Vr B)      , xxx)
   fun(XB          , (Vr X, Vr B)                    , _Z7eval_XBRK5ValueS1_)
   fun(AXB         , (Vr A, Vr X, Vr B)              , _Z8eval_AXBRK5ValueS1_S1_)
   fun(LXB         , (Fr LO, Vr X, Vr B)             , xxx)
   fun(ALXB        , (Vr A, Fr LO, Vr X, Vr B)       , xxx)
   fun(LRXB        , (Fr LO, Fr RO, Vr X, Vr B)      , xxx)
   fun(ALRXB       , (Vr A, Fr LO, Fr RO, Vr X, Vr B), xxx)
   fun(fill_B      , (Vr B)                          , xxx)
   fun(fill_AB     , (Vr A, Vr B)                    , xxx)
   fun(identity_fun, (Vr B, Axis axis)               , xxx)
#undef fun

   // check for mandatory properties in shared linrary...
   //
   {
     void * fun = dlsym(handle, "_Z10has_resultv");
     if (!fun)
        {
          CERR << "shared library is lacking the mandatory "
                  "function has_result() !"
               << endl;
          return;
        }

     b_has_result = ((bool (*)())fun)();
   }

   valid = true;
   if (is_operator())   sym->set_nc(NC_OPERATOR, this);
   else                 sym->set_nc(NC_FUNCTION, this);
}
//-----------------------------------------------------------------------------
NativeFunction::~NativeFunction()
{
   if (handle)   dlclose(handle);
}
//-----------------------------------------------------------------------------
NativeFunction *
NativeFunction::fix(void * so_handle, const UCS_string function_name)
{
NativeFunction * new_function = new NativeFunction(so_handle, function_name);

   if (!new_function->valid)   // something went wrong
      {
        delete new_function;
        return 0;
      }

   return new NativeFunction(so_handle, function_name);
}
//-----------------------------------------------------------------------------
int
NativeFunction::has_result() const
{
   return b_has_result;
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
   if (f_eval_)   return (*f_eval_)();

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_B(Value_P B)
{
   if (f_eval_B)   return (*f_eval_B)(*B.get());

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_AB(Value_P A, Value_P B)
{
   if (f_eval_AB)   return (*f_eval_AB)(*A.get(), *B.get());

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_LB(Token & LO, Value_P B)
{
   if (f_eval_LB)   return (*f_eval_LB)(*LO.get_function(), *B.get());

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_ALB(Value_P A, Token & LO, Value_P B)
{
   if (f_eval_ALB)   return (*f_eval_ALB)(*A.get(), *LO.get_function(),
                                          *B.get());

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_LRB(Token & LO, Token & RO, Value_P B)
{
   if (f_eval_LRB)   return (*f_eval_LRB)(*LO.get_function(),
                                          *RO.get_function(), *B.get());

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_ALRB(Value_P A, Token & LO, Token & RO, Value_P B)
{
   if (f_eval_ALRB)   return (*f_eval_ALRB)(*A.get(), *LO.get_function(),
                                            *RO.get_function(), *B.get());

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_XB(Value_P X, Value_P B)
{
   // call axis variant if present, or else the non-axis variant.
   //
   if (f_eval_XB)   return (*f_eval_XB)(*X.get(), *B.get());
   else             return eval_B(B);
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_AXB(Value_P A, Value_P X, Value_P B)
{
   // call axis variant if present, or else the non-axis variant.
   //
   if (f_eval_AXB)   return (*f_eval_AXB)(*A.get(), *X.get(), *B.get());
   else              return eval_AB(A, B);
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_LXB(Token & LO, Value_P X, Value_P B)
{
   // call axis variant if present, or else the non-axis variant.
   //
   if (f_eval_LXB)   return (*f_eval_LXB)(*LO.get_function(),
                                          *X.get(), *B.get());
   else              return eval_LB(LO, B);
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_ALXB(Value_P A, Token & LO, Value_P X, Value_P B)
{
   // call axis variant if present, or else the non-axis variant.
   //
   if (f_eval_ALXB)   return (*f_eval_ALXB)(*A.get(), *LO.get_function(),
                                            *X.get(), *B.get());
   else               return eval_ALB(A, LO, B);
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_LRXB(Token & LO, Token & RO, Value_P X, Value_P B)
{
   // call axis variant if present, or else the non-axis variant.
   //
   if (f_eval_LRXB)   return (*f_eval_LRXB)(*LO.get_function(),
                                            *RO.get_function(),
                                            *X.get(), *B.get());
   else                return eval_LRB(LO, RO, B);
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_ALRXB(Value_P A, Token & LO, Token & RO, Value_P X, Value_P B)
{
   // call axis variant if present, or else the non-axis variant.
   //
   if (f_eval_ALRXB)   return (*f_eval_ALRXB)(*A.get(), *LO.get_function(),
                                                        *RO.get_function(),
                                                        *X.get(), *B.get());
   else                return eval_ALRB(A, LO, RO, B);
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_fill_B(Value_P B)
{
   if (f_eval_fill_B)   return (*f_eval_fill_B)(*B.get());

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_fill_AB(Value_P A, Value_P B)
{
   if (f_eval_fill_AB)   return (*f_eval_fill_AB)(*A.get(), *B.get());

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------
Token
NativeFunction::eval_identity_fun(Value_P B, Axis axis)
{
   if (f_eval_identity_fun)   return (*f_eval_identity_fun)(*B.get(), axis);

   SYNTAX_ERROR;
}
//-----------------------------------------------------------------------------

