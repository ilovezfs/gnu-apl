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

#ifndef __Quad_FUNCTION_HH_DEFINED__
#define __Quad_FUNCTION_HH_DEFINED__

#include "PrimitiveFunction.hh"
#include "StateIndicator.hh"

//-----------------------------------------------------------------------------
/** The various Quad functions.
 */
class QuadFunction : public PrimitiveFunction
{
public:
   /// Constructor.
   QuadFunction(TokenTag tag) : PrimitiveFunction(tag) {}

   /// overloaded Function::has_alpha()
   virtual bool has_alpha() const   { return true; }

   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B) { VALENCE_ERROR; }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B) { VALENCE_ERROR; }

   /// overloaded Function::has_result()
   virtual bool has_result() const   { return true; }
};
//-----------------------------------------------------------------------------
/**
   The system function ⎕AF (Atomic Function)
 */
class Quad_AF : public QuadFunction
{
public:
   /// Constructor.
   Quad_AF() : QuadFunction(TOK_Quad_AF) {}

   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   static Quad_AF           fun;          ///< Built-in function.

protected:
};
//-----------------------------------------------------------------------------
/**
   The system function ⎕AT (Attributes)
 */
class Quad_AT : public QuadFunction
{
public:
   /// Constructor.
   Quad_AT() : QuadFunction(TOK_Quad_AT) {}

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   static Quad_AT fun;          ///< Built-in function.

protected:
};
//-----------------------------------------------------------------------------
/**
   The system function ⎕CR (Character Representation).
 */
class Quad_CR : public QuadFunction
{
public:
   /// Constructor.
   Quad_CR() : QuadFunction(TOK_Quad_CR) {}

   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   /// compute \b a ⎕CR \b B
   static Value_P do_CR(APL_Integer a, const Value & B, PrintContext pctx);

   static Quad_CR fun;          ///< Built-in function.

   /// portable variable encoding of value named name (varname or varname ⊂)
   static void do_CR10_var(vector<UCS_string> & result,
                           const UCS_string & name, const UCS_string & pick,
                           const Value & value);

protected:
   /// 10 ⎕CR symbol_name (variable or function name)
   static void do_CR10(vector<UCS_string> & result, const Value & symbol_name);
};
//-----------------------------------------------------------------------------
/**
   The system function ⎕DL (Delay).
 */
class Quad_DL : public QuadFunction
{
public:
   /// Constructor.
   Quad_DL() : QuadFunction(TOK_Quad_DL) {}

   static Quad_DL fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);
};
//-----------------------------------------------------------------------------
/**
   The system function ⎕EA (Execute Alternate)
 */
class Quad_EA : public QuadFunction
{
public:
   /// Constructor.
   Quad_EA() : QuadFunction(TOK_Quad_EA) {}

   static Quad_EA fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   /// end of context handler for ⎕EA
   static bool eoc_B_done(Token & token, EOC_arg & arg);

   /// end of context handler for ⎕EA
   static bool eoc_A_and_B_done(Token & token, EOC_arg & arg);
};
//-----------------------------------------------------------------------------
/**
   The system function ⎕EC (Execute Controlled)
 */
class Quad_EC : public QuadFunction
{
public:
   /// Constructor.
   Quad_EC() : QuadFunction(TOK_Quad_EC) {}

   static Quad_EC           fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   /// end of context handler for ⎕EC
   static bool eoc(Token & token, EOC_arg & arg);
};
//-----------------------------------------------------------------------------
/**
   The system function ⎕ENV (ENvironment Variables)
 */
class Quad_ENV : public QuadFunction
{
public:
   /// Constructor.
   Quad_ENV() : QuadFunction(TOK_Quad_ENV) {}

   static Quad_ENV fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);
};
//-----------------------------------------------------------------------------
/**
   The system function ⎕ES (Event Simulate).
 */
class Quad_ES : public QuadFunction
{
public:
   /// Constructor.
   Quad_ES() : QuadFunction(TOK_Quad_ES) {}

   static Quad_ES           fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   /// common inplementation for eval_AB() and eval_B()
   Token event_simulate(const UCS_string * A, Value_P B, Error & error);

   /// compute error code for B
   static ErrorCode get_error_code(Value_P B);
};
//-----------------------------------------------------------------------------
/**
   The system function ⎕EX (Expunge).
 */
class Quad_EX : public QuadFunction
{
public:
   /// Constructor.
   Quad_EX() : QuadFunction(TOK_Quad_EX) {}

   static Quad_EX fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   /// disassociate name from value, return 0 on failure or 1 on success.
   int expunge(UCS_string name);
};
//-----------------------------------------------------------------------------
/**
   The system function ⎕INP (input from script, aka. HERE document)
 */
class Quad_INP : public QuadFunction
{
public:
   /// Constructor.
   Quad_INP() : QuadFunction(TOK_Quad_INP) {}

   static Quad_INP fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   /// overloaded Function::eval_XB().
   virtual Token eval_XB(Value_P X, Value_P B);

   /// read input until end_marker seen; maybe ⍎ esc1...esc2.
   static bool eoc_INP(Token & token, EOC_arg & arg);
};
//-----------------------------------------------------------------------------
/**
   The system function ⎕NA (Name Association).
 */
class Quad_NA : public QuadFunction
{
public:
   /// Constructor.
   Quad_NA() : QuadFunction(TOK_Quad_NA) {}

   static Quad_NA fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B)   { TODO; }

   /// overloaded Function::eval_B()
   virtual Token eval_B(Value_P B)   { TODO; }
};
//-----------------------------------------------------------------------------
/**
   The system function ⎕NC (Name class).
 */
class Quad_NC : public QuadFunction
{
public:
   /// Constructor.
   Quad_NC() : QuadFunction(TOK_Quad_NC) {}

   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   /// return the ⎕NC for variable name \b var
   static APL_Integer get_NC(const UCS_string var);

   static Quad_NC fun;          ///< Built-in function.

protected:
};
//-----------------------------------------------------------------------------
/**
   The system function ⎕NL (Name List).
 */
class Quad_NL : public QuadFunction
{
public:
   /// Constructor.
   Quad_NL() : QuadFunction(TOK_Quad_NL) {}

   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B)
      { return do_quad_NL(Value_P(), B); }

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B)
      { return do_quad_NL(A, B); }

   static Quad_NL           fun;          ///< Built-in function.

protected:
   /// return A ⎕NL B
   Token do_quad_NL(Value_P A, Value_P B);
};
//-----------------------------------------------------------------------------
/**
   The system function ⎕SI (State Indicator)
 */
class Quad_SI : public QuadFunction
{
public:
   /// Constructor.
   Quad_SI() : QuadFunction(TOK_Quad_SI) {}

   /// overloaded Function::eval_AB().
   virtual Token eval_AB(Value_P A, Value_P B);

   /// overloaded Function::eval_AB().
   virtual Token eval_B(Value_P B);

   /// Built-in function.
   static Quad_SI fun;

protected:
};
//-----------------------------------------------------------------------------
/**
   The system function ⎕UCS (Universal Character Set)
 */
class Quad_UCS : public QuadFunction
{
public:
   /// Constructor.
   Quad_UCS() : QuadFunction(TOK_Quad_UCS) {}

   /// overloaded Function::eval_B().
   virtual Token eval_B(Value_P B);

   /// Built-in function.
   static Quad_UCS fun;

protected:
};
//-----------------------------------------------------------------------------
/// functions common for ⎕STOP and ⎕TRACE
class Stop_Trace : public QuadFunction
{
protected:
   /// constructor
   Stop_Trace(TokenTag tag)
   : QuadFunction (tag)
   {}

   /// find UserFunction named \b fun_name
   UserFunction * locate_fun(const Value & fun_name);

   /// return integers in lines
   Token reference(const vector<Function_Line> & lines, bool assigned);

   /// return assign lines in new_value to stop or trace vector in ufun
   void assign(UserFunction * ufun, const Value & new_value, bool stop);
};
//-----------------------------------------------------------------------------
/// ⎕STOP
class Quad_STOP : public Stop_Trace
{
public:
   /// Constructor
   Quad_STOP()
   : Stop_Trace(TOK_Quad_STOP)
   {}

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// Built-in function
   static Quad_STOP fun;
};
//-----------------------------------------------------------------------------
/// ⎕TRACE
class Quad_TRACE : public Stop_Trace
{
public:
   /// Constructor
   Quad_TRACE()
   : Stop_Trace(TOK_Quad_TRACE)
   {}

   /// Overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B);

   /// Overloaded Function::eval_B()
   virtual Token eval_B(Value_P B);

   /// Built-in function
   static Quad_TRACE fun;
};
//-----------------------------------------------------------------------------




#endif // __Quad_FUNCTION_HH_DEFINED__
