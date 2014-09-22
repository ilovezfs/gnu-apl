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

#ifndef __SYSTEM_VARIABLE_HH_DEFINED__
#define __SYSTEM_VARIABLE_HH_DEFINED__

#include "Parallel.hh"
#include "Symbol.hh"
#include "Id.hh"

#include <sys/time.h>

//-----------------------------------------------------------------------------
/**
    Base class for all system variables (Quad variables).
 */
class SystemVariable : public Symbol
{
public:
   /// Construct a \b SystemVariable with \b Id \b id
   SystemVariable(Id id)
   : Symbol(id_name(id), id)
   {}

   /// overloaded Symbol::print().
   virtual ostream & print(ostream & out) const;

   /// overloaded Symbol::assign().
   virtual void assign(Value_P value, const char * loc);

   /// overloaded Symbol::assign_indexed().
   virtual void assign_indexed(Value_P X, Value_P value);

   /// overloaded Symbol::get_attributes().
   virtual void get_attributes(int mode, Cell * dest) const;

   /// system vars cannot be expunged.
   virtual int expunge() { return 0; }
};
//-----------------------------------------------------------------------------
/**
    A system variable that cannot be localized (push and pop have no effect).
 */
class NL_SystemVariable : public SystemVariable
{
public:
public:
   /// Constructor.
   NL_SystemVariable(Id id)
   : SystemVariable(id)
   {}
protected:
   /// overloaded Symbol::push()
   virtual void push() {}

   /// overloaded Symbol::push_label()
   virtual void push_label(int label) {}

   /// overloaded Symbol::push_function()
   virtual void push_function(Function * function) {}

   /// overloaded Symbol::push_value()
   virtual void push_value(Value_P value) {}

   /// overloaded Symbol::pop()
   virtual void pop() { }
};
//-----------------------------------------------------------------------------
/**
    A read-only system variable (push, pop, and assign are ignored).
 */
class RO_SystemVariable : public NL_SystemVariable
{
public:
   /// Constructor.
   RO_SystemVariable(Id id)
   : NL_SystemVariable(id)
   {}

protected:
   /// overloaded Symbol::assign()
   virtual void assign(Value_P value, const char * loc) {}

   /// overloaded Symbol::assign_indexed()
   virtual void assign_indexed(Value_P X, Value_P value) {}

   /// overloaded Symbol::is_readonly()
   virtual bool is_readonly() const   { return true; }

   /// overloaded Symbol::resolve(). Since push(), pop(), and assign()
   /// do nothing, we can call get_apl_value() directly.
   virtual void resolve(Token & token, bool left)
      { if (!left)   new (&token) Token(TOK_APL_VALUE1, get_apl_value()); }
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-AI (Account Information)
 */
class Quad_AI : public RO_SystemVariable
{
public:
   /// Constructor.
   Quad_AI();

   /// increase the time waiting for user input
   void add_wait(APL_time_us diff)
   { user_wait += diff; }

protected:
   /// when the interpreter was started
   const APL_time_us session_start;

   /// time waiting for user input.
   APL_time_us user_wait;

   /// overloaded Symbol::get_apl_value().
   virtual Value_P get_apl_value() const;
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-ARG (command line arguments of the interpreter)
 */
class Quad_ARG : public RO_SystemVariable
{
public:
   /// Constructor.
   Quad_ARG();

protected:
   /// overloaded Symbol::get_apl_value()
   virtual Value_P get_apl_value() const;
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-AV (Atomic Vector)
 */
class Quad_AV : public RO_SystemVariable
{
public:
   /// Constructor.
   Quad_AV();

   /// return ⎕AV[pos - ⎕IO]
   static Unicode indexed_at(uint32_t pos);

   /// a static ⎕AV
   static Unicode qav[MAX_AV];
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-CT (Comparison Tolerance)
 */
class Quad_CT : public SystemVariable
{
public:
   /// constructor
   Quad_CT();

   /// return the current comparison tolerance. MUST NOT CALL get_apl_value()
   /// because it can be called from parallel Cell functions
   APL_Float current() const
      { return get_first_cell()->get_real_value(); }

protected:
   /// overloaded Symbol::assign()
   virtual void assign(Value_P value, const char * loc);

   // overloaded Symbol::push()
   virtual void push()
      {
        Symbol::push();
        Symbol::assign(FloatScalar(DEFAULT_Quad_CT, LOC), LOC);
      }
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-EM (Event Message)
 */
class Quad_EM : public RO_SystemVariable
{
public:
   /// Constructor.
   Quad_EM()
   : RO_SystemVariable(ID_Quad_EM)
   {}

protected:
   /// overloaded Symbol::get_apl_value()
   virtual Value_P get_apl_value() const;
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-ET (Event Type).
 */
class Quad_ET : public RO_SystemVariable
{
public:
   /// Constructor.
   Quad_ET() : RO_SystemVariable(ID_Quad_ET)
   {}

protected:
   /// overloaded Symbol::get_apl_value().
   virtual Value_P get_apl_value() const;
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-FC (Format Control).
 */
class Quad_FC : public SystemVariable
{
public:
   /// Constructor.
   Quad_FC();

   /// return the current format control (used by Workspace::get_FC())
   const UCS_string current() const
      { return UCS_string(*get_apl_value()); }

protected:
   /// overloaded Symbol::assign().
   virtual void assign(Value_P value, const char * loc);

   /// overloaded Symbol::assign_indexed().
   virtual void assign_indexed(Value_P X, Value_P value);

   /// overloaded Symbol::assign_indexed().
   virtual void assign_indexed(IndexExpr & IX, Value_P value);

   // overloaded Symbol::push()
   virtual void push();
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-IO (Index Origin).
 */
class Quad_IO : public SystemVariable
{
public:
   /// Constructor.
   Quad_IO();

   /// Return the current index origin. MUST NOT CALL get_apl_value()
   /// because it can be called from parallel Cell functions
   APL_Integer current() const
      { return get_first_cell()->get_int_value(); }

protected:
   /// overloaded Symbol::assign().
   virtual void assign(Value_P value, const char * loc);

   // overloaded Symbol::push()
   virtual void push()
      {
        Symbol::push();
        Symbol::assign(IntScalar(1, LOC), LOC);
      }
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-L (Left Argument).
 */
class Quad_L : public NL_SystemVariable
{
public:
   /// Constructor.
   Quad_L();

   /// overloaded Symbol::save()
   virtual void save(ostream & out) const {}

protected:
   /// overloaded Symbol::assign()
   virtual void assign(Value_P value, const char * loc);

   /// overloaded Symbol::get_apl_value().
   virtual Value_P get_apl_value() const;
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-LC (Line Counter).
 */
class Quad_LC : public RO_SystemVariable
{
public:
   /// Constructor.
   Quad_LC();

protected:
   /// overloaded Symbol::get_apl_value().
   virtual Value_P get_apl_value() const;
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-LX (Latent Expression).
 */
class Quad_LX : public NL_SystemVariable
{
public:
   /// Constructor.
   Quad_LX();

protected:
   /// overloaded Symbol::assign().
   virtual void assign(Value_P value, const char * loc);

   /// overloaded Symbol::assign_indexed()
   virtual void assign_indexed(Value_P X, Value_P value) {}

};
//-----------------------------------------------------------------------------
/**
   System variable Quad-PP (Printing Precision).
 */
class Quad_PP : public SystemVariable
{
public:
   /// Constructor.
   Quad_PP();

   /// Return the current print precision. MUST NOT CALL get_apl_value()
   /// because it can be called from parallel Cell functions
   APL_Integer current() const
      { return get_first_cell()->get_int_value(); }

protected:
   /// overloaded Symbol::assign().
   virtual void assign(Value_P value, const char * loc);

   // overloaded Symbol::push()
   virtual void push()
      {
        Symbol::push();
        Symbol::assign(IntScalar(DEFAULT_Quad_PP, LOC), LOC);
      }
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-PR (Prompt Replacement).
 */
class Quad_PR : public SystemVariable
{
public:
   /// Constructor.
   Quad_PR();

   /// Return the current prompt replacement.
   const UCS_string current() const
      { return  UCS_string(*get_apl_value()); }

protected:
   /// overloaded Symbol::assign().
   virtual void assign(Value_P value, const char * loc);

   // overloaded Symbol::push()
   virtual void push()
      {
        Symbol::push();
        Symbol::assign(Spc(LOC), LOC);
      }
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-PS (Print Style). This variable controls the formatting
   of APL values (classical APL or DISPLAY style).
 */
class Quad_PS : public SystemVariable
{
public:
   /// Constructor.
   Quad_PS();

   /// Return the current print style. MUST NOT CALL get_apl_value()
   /// because it can be called from parallel Cell functions
   PrintStyle current() const
      { switch (get_first_cell()->get_int_value())
           {
             case 0: return PR_APL;
             case 1: return PR_APL_FUN;
             case 2: return PR_BOXED_CHAR;
             case 3: return PR_BOXED_GRAPHIC;
             default: return PST_NONE;
           }
      }

protected:
   /// overloaded Symbol::assign().
   virtual void assign(Value_P value, const char * loc);

   // overloaded Symbol::push()
   virtual void push()
      {
        Symbol::push();
        Symbol::assign(IntScalar(0, LOC), LOC);
      }
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-PT (Print Tolerance), about 10^-⎕PP
 */
class Quad_PT : public RO_SystemVariable
{
public:
   /// constructor
   Quad_PT();

protected:
   virtual Value_P get_apl_value() const;
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-PW (Print Width).
 */
class Quad_PW : public SystemVariable
{
public:
   /// Constructor.
   Quad_PW();

   /// return the current ⎕PW. MUST NOT CALL get_apl_value()
   /// because it can be called from parallel Cell functions
   APL_Integer current() const
      { return get_first_cell()->get_int_value(); }

protected:
   /// overloaded Symbol::assign().
   virtual void assign(Value_P value, const char * loc);

   // overloaded Symbol::push()
   virtual void push()
      {
        Symbol::push();
        Symbol::assign(IntScalar(DEFAULT_Quad_PW, LOC), LOC);
      }
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-Quad (Evaluated Input/Output).
 */
class Quad_Quad : public SystemVariable
{
public:
   /// Constructor.
   Quad_Quad();

   /// overloaded Symbol::resolve().
   virtual void resolve(Token & token, bool left);

protected:
   virtual void assign(Value_P value, const char * loc);

   // should never be called due to overloaded resolve()
   virtual Value_P get_apl_value() const
      { Assert(0);   /* not reached */ return Value_P(); }
};
//-----------------------------------------------------------------------------
/**
   System variable Quote-Quad (Evaluated Input/Output).
 */
class Quad_QUOTE : public SystemVariable
{
public:
   /// Constructor.
   Quad_QUOTE();

   /// end of ⍞ (output, but no input: clear prompt)
   static void done(bool with_LF, const char * loc);

protected:
   /// overloaded Symbol::assign().
   virtual void assign(Value_P value, const char * loc);

   /// overloaded Symbol::get_apl_value().
   virtual Value_P get_apl_value() const;

   /// last line of output
   static UCS_string prompt;
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-R (Right Argment).
 */
class Quad_R : public NL_SystemVariable
{
public:
   /// Constructor.
   Quad_R();

   /// overloaded Symbol::save()
   virtual void save(ostream & out) const {}

protected:
   /// overloaded Symbol::assign()
   virtual void assign(Value_P value, const char * loc);

   /// overloaded Symbol::get_apl_value().
   virtual Value_P get_apl_value() const;
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-SYL (System Limits).
 */
class Quad_SYL : public NL_SystemVariable
{
public:
   /// Constructor.
   Quad_SYL() : NL_SystemVariable(ID_Quad_SYL)
      { assign(Value_P(), LOC); }

   /// overloaded Symbol::assign()
   virtual void assign(Value_P value, const char * loc);

   /// overloaded Symbol::assign_indexed()
   virtual void assign_indexed(Value_P X, Value_P value);

   /// overloaded Symbol::assign_indexed()
   virtual void assign_indexed(IndexExpr & IDX, Value_P value);

   virtual Value_P get_apl_value() const;

   /// maximum depth of SI stack
   static ShapeItem si_depth_limit;

   /// maximum number of values
   static ShapeItem value_count_limit;

   /// maximum number of ravel bytes
   static ShapeItem ravel_count_limit;

   /// maximum number of ravel bytes in APL printout
   static ShapeItem print_length_limit;

   /// the system limits
   enum SYL_INDEX
      {
#define syl3(n, e, v) syl1(n, e, v)
#define syl2(n, e, v) syl1(n, e, v)
#define syl1(_n, e, _v) SYL_ ## e, ///< dito
#include "SystemLimits.def"

        SYL_MAX                      ///< max. system limit value (excluding)
      };

protected:
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-TC (Terminal Control Characters)
 */
class Quad_TC : public RO_SystemVariable
{
public:
   /// Constructor.
   Quad_TC();
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-TS (Time Stamp).
 */
class Quad_TS : public RO_SystemVariable
{
public:
   /// Constructor.
   Quad_TS();

   /// overloaded Symbol::get_apl_value().
   virtual Value_P get_apl_value() const;
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-TZ (Time Zone).
 */
class Quad_TZ : public SystemVariable
{
public:
   /// Constructor.
   Quad_TZ();

   /// return the offset from GMT of the current timezone (in seconds).
   /// we make this int64_t rather than int so that conversion to usec
   /// does not overflow
   int64_t get_offset() const   { return offset_seconds; }

protected:
   /// overloaded Symbol::assign().
   virtual void assign(Value_P value, const char * loc);

   /// compute the offset (in seconds) from GMT
   static int compute_offset();

   // overloaded Symbol::push()
   virtual void push()
      {
        Symbol::push();
        offset_seconds = compute_offset();
        if (offset_seconds % 3600 == 0)   // full hour
           Symbol::assign(IntScalar(offset_seconds/3600, LOC), LOC);
        else
           Symbol::assign(FloatScalar(offset_seconds/3600, LOC), LOC);
      }

   virtual void pop()
      {
        Symbol::pop();
        offset_seconds = 3600 * get_apl_value()->get_ravel(0).get_int_value();
      }

   /// the offset from GMT of the current timezone (in seconds)
   int offset_seconds;
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-UL (User Load).
 */
class Quad_UL : public RO_SystemVariable
{
public:
   /// Constructor.
   Quad_UL();

protected:
   /// overloaded Symbol::get_apl_value().
   virtual Value_P get_apl_value() const;
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-WA (Workspace Available).
 */
class Quad_WA : public RO_SystemVariable
{
public:
   /// Constructor.
   Quad_WA();

protected:
   /// overloaded Symbol::get_apl_value().
   virtual Value_P get_apl_value() const;
};
//-----------------------------------------------------------------------------
/**
   System variable Quad-X (Axis Argument).
 */
class Quad_X : public NL_SystemVariable
{
public:
   /// Constructor.
   Quad_X();

   /// overloaded Symbol::save()
   virtual void save(ostream & out) const {}

protected:
   /// overloaded Symbol::assign()
   virtual void assign(Value_P value, const char * loc);

   /// overloaded Symbol::get_apl_value().
   virtual Value_P get_apl_value() const;
};
//-----------------------------------------------------------------------------
#endif // __SYSTEM_VARIABLE_HH_DEFINED__
