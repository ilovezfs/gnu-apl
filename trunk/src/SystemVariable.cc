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

#include <stdint.h>
#include <stdlib.h>
#include <sys/resource.h>   // for ⎕WA
#include <sys/time.h>

// FreeBSD 8.x lacks utmpx.h
#ifdef HAVE_UTMPX_H
# include <utmpx.h>           // for ⎕UL
#endif

#include "Bif_F12_FORMAT.hh"
#include "CDR.hh"
#include "CharCell.hh"
#include "Common.hh"
#include "FloatCell.hh"
#include "IndexExpr.hh"
#include "IntCell.hh"
#include "LineInput.hh"
#include "main.hh"
#include "Output.hh"
#include "PointerCell.hh"
#include "ProcessorID.hh"
#include "StateIndicator.hh"
#include "SystemVariable.hh"
#include "UserFunction.hh"
#include "UserPreferences.hh"
#include "Value.icc"
#include "Workspace.hh"

UCS_string Quad_QUOTE::prompt;

ShapeItem Quad_SYL::si_depth_limit = 0;
ShapeItem Quad_SYL::value_count_limit = 0;
ShapeItem Quad_SYL::ravel_count_limit = 0;
ShapeItem Quad_SYL::print_length_limit = 0;

Unicode Quad_AV::qav[MAX_AV];

//=============================================================================
void
SystemVariable::assign(Value_P value, const char * loc)
{
   CERR << "SystemVariable::assign() not (yet) implemented for "
        << get_Id() << endl;
   FIXME;
}
//-----------------------------------------------------------------------------
void
SystemVariable::assign_indexed(Value_P X, Value_P value)
{
   CERR << "SystemVariable::assign_indexed() not (yet) implemented for "
        << get_Id() << endl;
   FIXME;
}
//-----------------------------------------------------------------------------
ostream &
SystemVariable::print(ostream & out) const
{
   return out << get_Id();
}
//-----------------------------------------------------------------------------
void
SystemVariable::get_attributes(int mode, Cell * dest) const
{
   switch(mode)
      {
        case 1: // valences (always 1 0 0 for variables)
                new (dest + 0) IntCell(1);
                new (dest + 1) IntCell(0);
                new (dest + 2) IntCell(0);
                break;

        case 2: // creation time (always 7⍴0 for variables)
                new (dest + 0) IntCell(0);
                new (dest + 1) IntCell(0);
                new (dest + 2) IntCell(0);
                new (dest + 3) IntCell(0);
                new (dest + 4) IntCell(0);
                new (dest + 5) IntCell(0);
                new (dest + 6) IntCell(0);
                break;

        case 3: // execution properties (always 4⍴0 for variables)
                new (dest + 0) IntCell(0);
                new (dest + 1) IntCell(0);
                new (dest + 2) IntCell(0);
                new (dest + 3) IntCell(0);
                break;

        case 4: {
                  Value_P val = get_apl_value();
                  const CDR_type cdr_type = val->get_CDR_type();
                  const int brutto = val->total_size_brutto(cdr_type);
                  const int data = val->data_size(cdr_type);

                  new (dest + 0) IntCell(brutto);
                  new (dest + 1) IntCell(data);
                }
                break;

        default:  Assert(0 && "bad mode");
      }

}
//=============================================================================
Quad_AV::Quad_AV()
   : RO_SystemVariable(ID_Quad_AV)
{
Value_P AV(new Value(MAX_AV, LOC));
   loop(cti, MAX_AV)
       {
         const int av_pos = Avec::get_av_pos(CHT_Index(cti));
         const Unicode uni = Avec::unicode(CHT_Index(cti));

         qav[av_pos] = uni;   // remember unicode for indexed_at()
         new (&AV->get_ravel(av_pos))  CharCell(uni);
       }
   AV->check_value(LOC);

   Symbol::assign(AV, LOC);
}
//-----------------------------------------------------------------------------
Unicode
Quad_AV::indexed_at(uint32_t pos)
{
   if (pos < MAX_AV)   return qav[pos];
   return UNI_AV_MAX;
}
//=============================================================================
Quad_AI::Quad_AI()
   : RO_SystemVariable(ID_Quad_AI),
     session_start(now()),
     user_wait(0)
{
   Symbol::assign(get_apl_value(), LOC);
}
//-----------------------------------------------------------------------------
Value_P
Quad_AI::get_apl_value() const
{
const int total_ms = (now() - session_start)/1000;
const int user_ms  = user_wait/1000;

Value_P ret(new Value(4, LOC));
   new (&ret->get_ravel(0))   IntCell(ProcessorID::get_own_ID());
   new (&ret->get_ravel(1))   IntCell(total_ms - user_ms);
   new (&ret->get_ravel(2))   IntCell(total_ms);
   new (&ret->get_ravel(3))   IntCell(user_ms);

   ret->check_value(LOC);
   return ret;
}
//=============================================================================
Quad_ARG::Quad_ARG()
   : RO_SystemVariable(ID_Quad_ARG)
{
}
//-----------------------------------------------------------------------------
Value_P
Quad_ARG::get_apl_value() const
{
const int argc = uprefs.expanded_argv.size();

Value_P Z(new Value(argc, LOC));
Cell * C = &Z->get_ravel(0);

   loop(a, argc)
      {
        const char * arg = uprefs.expanded_argv[a];
        const int len = strlen(arg);
        Value_P val(new Value(len, LOC));
        loop(l, len)   new (val->next_ravel())   CharCell(Unicode(arg[l]));

        new (C++)   PointerCell(val);
      }

   Z->check_value(LOC);
   return Z;
}
//=============================================================================
Quad_CT::Quad_CT()
   : SystemVariable(ID_Quad_CT)
{
   Symbol::assign(FloatScalar(DEFAULT_Quad_CT, LOC), LOC);
}
//-----------------------------------------------------------------------------
void
Quad_CT::assign(Value_P value, const char * loc)
{
   if (!value->is_scalar_or_len1_vector())
      {
        if (value->get_rank() > 1)   RANK_ERROR;
        else                         LENGTH_ERROR;
      }

const Cell & cell = value->get_ravel(0);
   if (!cell.is_numeric())             DOMAIN_ERROR;
   if (cell.get_imag_value() != 0.0)   DOMAIN_ERROR;

APL_Float val = cell.get_real_value();
   if (val < 0.0)     DOMAIN_ERROR;

   // APL2 "discourages" the use of ⎕CT > 1E¯9.
   // We round down to MAX_Quad_CT (= 1E¯9) instead.
   //
   if (val > MAX_Quad_CT)
      {
        Symbol::assign(FloatScalar(MAX_Quad_CT, LOC), LOC);
      }
   else
      {
        Symbol::assign(value, LOC);
      }
}
//=============================================================================
Value_P
Quad_EM::get_apl_value() const
{
const Error * err = 0;

   for (StateIndicator * si = Workspace::SI_top(); si; si = si->get_parent())
       {
         if (si->get_error().error_code != E_NO_ERROR)
            {
              err = &si->get_error();
              break;
            }
         if (si->get_executable()->get_parse_mode() == PM_FUNCTION)   break;
       }

   if (err == 0)   // no SI entry with non-zero error code
      {
        // return 3 0⍴' '
        //
        Shape sh((ShapeItem)3, (ShapeItem)0);
        Value_P Z(new Value(sh, LOC));
        new (&Z->get_ravel(0))   CharCell(UNI_ASCII_SPACE);
        Z->check_value(LOC);
        return Z;
      }

const UCS_string msg_1 = err->get_error_line_1();
const UCS_string msg_2 = err->get_error_line_2();
const UCS_string msg_3 = err->get_error_line_3();

   // compute max line length of the 3 error lines,
   //
const ShapeItem len_1 = msg_1.size();
const ShapeItem len_2 = msg_2.size();
const ShapeItem len_3 = msg_3.size();

ShapeItem max_len = len_1;
   if (max_len < len_2)   max_len = len_2;
   if (max_len < len_3)   max_len = len_3;

const Shape sh(3, max_len);
Value_P Z(new Value(sh, LOC));
   loop(l, max_len)
      {
        new (&Z->get_ravel(l))
         CharCell((l < len_1) ? msg_1[l] : UNI_ASCII_SPACE);
      }
   loop(l, max_len)
      {
        new (&Z->get_ravel(l + max_len))
         CharCell((l < len_2) ? msg_2[l] : UNI_ASCII_SPACE);
      }
   loop(l, max_len)
      {
        new (&Z->get_ravel(l + 2*max_len))
         CharCell((l < len_3) ? msg_3[l] : UNI_ASCII_SPACE);
      }

   Z->check_value(LOC);
   return Z;
}
//=============================================================================
Value_P
Quad_ET::get_apl_value() const
{
StateIndicator * si = Workspace::SI_top();
ErrorCode ec = E_NO_ERROR;

   for (; si; si = si->get_parent())
       {
         ec = si->get_error().error_code;
         if (ec != E_NO_ERROR)
            {
              break;
            }

         if (si->get_executable()->get_parse_mode() == PM_FUNCTION)   break;
       }

Value_P Z(new Value(2, LOC));

   new (&Z->get_ravel(0)) IntCell(Error::error_major(ec));
   new (&Z->get_ravel(1)) IntCell(Error::error_minor(ec));

   Z->check_value(LOC);
   return Z;
}
//=============================================================================
Quad_FC::Quad_FC() : SystemVariable(ID_Quad_FC)
{
Value_P QFC(new Value(6, LOC));
   new (QFC->next_ravel()) CharCell(UNI_ASCII_FULLSTOP);
   new (QFC->next_ravel()) CharCell(UNI_ASCII_COMMA);
   new (QFC->next_ravel()) CharCell(UNI_STAR_OPERATOR);
   new (QFC->next_ravel()) CharCell(UNI_ASCII_0);
   new (QFC->next_ravel()) CharCell(UNI_ASCII_UNDERSCORE);
   new (QFC->next_ravel()) CharCell(UNI_OVERBAR);
   QFC->check_value(LOC);

   Symbol::assign(QFC, LOC);
}
//-----------------------------------------------------------------------------
void
Quad_FC::push()
{
   Symbol::push();

Value_P QFC(new Value(6, LOC));
   new (QFC->next_ravel()) CharCell(UNI_ASCII_FULLSTOP);
   new (QFC->next_ravel()) CharCell(UNI_ASCII_COMMA);
   new (QFC->next_ravel()) CharCell(UNI_STAR_OPERATOR);
   new (QFC->next_ravel()) CharCell(UNI_ASCII_0);
   new (QFC->next_ravel()) CharCell(UNI_ASCII_UNDERSCORE);
   new (QFC->next_ravel()) CharCell(UNI_OVERBAR);
   QFC->check_value(LOC);

   Symbol::assign(QFC, LOC);
}
//-----------------------------------------------------------------------------
void
Quad_FC::assign(Value_P value, const char * loc)
{
   if (!value->is_scalar_or_vector())   RANK_ERROR;

ShapeItem value_len = value->element_count();
   if (value_len > 6)   value_len = 6;

   loop(c, value_len)
       if (!value->get_ravel(c).is_character_cell())   DOMAIN_ERROR;

   // new value is correct. 
   //
Unicode fc[6] = { UNI_ASCII_FULLSTOP, UNI_ASCII_COMMA,      UNI_STAR_OPERATOR,
                  UNI_ASCII_0,        UNI_ASCII_UNDERSCORE, UNI_OVERBAR };
                  
   loop(c, 6)   if (c < value_len)
         fc[c] = value->get_ravel(c).get_char_value();
                  
   // 0123456789,. are forbidden for ⎕FC[4 + ⎕IO]
   //
   if (Bif_F12_FORMAT::is_control_char(fc[4]))
      fc[4] = UNI_ASCII_SPACE;

UCS_string ucs(fc, 6);
Value_P new_val(new Value(ucs, LOC));
   Symbol::assign(new_val, LOC);
}
//-----------------------------------------------------------------------------
void
Quad_FC::assign_indexed(IndexExpr & IX, Value_P value)
{
   if (IX.value_count() != 1)   INDEX_ERROR;

   // at this point we have a one dimensional index. It it were non-empty,
   // then assign_indexed(Value) would have been called instead. Therefore
   // IX must be an elided index (like in ⎕FC[] ← value)
   //
   Assert1(!IX.values[0]);
   assign(value, LOC);   // ⎕FC[]←value
}
//-----------------------------------------------------------------------------
void
Quad_FC::assign_indexed(Value_P X, Value_P value)
{
   // we don't do scalar extension but require indices to match the value.
   //
ShapeItem ec = X->element_count();
   if (ec != value->element_count())   INDEX_ERROR;

   // ignore extra values.
   //
   if (ec > 6)   ec = 6;

const APL_Integer qio = Workspace::get_IO();
Unicode fc[6];
   {
     Value_P old = get_apl_value();
     loop(e, 6)   fc[e] = old->get_ravel(e).get_char_value();
   }

   loop(e, ec)
      {
        const APL_Integer idx = X->get_ravel(e).get_int_value() - qio;
        if (idx < 0)   continue;
        if (idx > 5)   continue;

        fc[idx] = value->get_ravel(e).get_char_value();
      }

   // 0123456789,. are forbidden for ⎕FC[4 + ⎕IO]
   //
   if (Bif_F12_FORMAT::is_control_char(fc[4]))
      fc[4] = UNI_ASCII_SPACE;

UCS_string ucs(fc, 6);
Value_P new_val(new Value(ucs, LOC));
   Symbol::assign(new_val, LOC);
}
//=============================================================================
Quad_IO::Quad_IO()
   : SystemVariable(ID_Quad_IO)
{
   Symbol::assign(IntScalar(1, LOC), LOC);
}
//-----------------------------------------------------------------------------
void
Quad_IO::assign(Value_P value, const char * loc)
{
   if (!value->is_scalar_or_len1_vector())
      {
        if (value->get_rank() > 1)   RANK_ERROR;
        else                         LENGTH_ERROR;
      }

   if (value->get_ravel(0).get_near_bool(0.1))
      Symbol::assign(IntScalar(1, LOC), LOC);
   else
      Symbol::assign(IntScalar(0, LOC), LOC);
}
//=============================================================================
Quad_L::Quad_L()
 : NL_SystemVariable(ID_Quad_L)
{
   Symbol::assign(IntScalar(0, LOC), LOC);
}
//-----------------------------------------------------------------------------
void
Quad_L::assign(Value_P value, const char * loc)
{
StateIndicator * si = Workspace::SI_top_fun();
   if (si == 0)   return;

   // ignore assignments if error was SYNTAX ERROR or VALUE ERROR
   //
   if (si->get_error().is_syntax_or_value_error())   return;

   si->set_L(value);
}
//-----------------------------------------------------------------------------
Value_P
Quad_L::get_apl_value() const
{
StateIndicator * si = Workspace::SI_top_error();
   if (si)
      {
        Value_P ret = si->get_L();
        if (!!ret)   return  ret;
      }

   VALUE_ERROR;
}
//=============================================================================
Quad_LC::Quad_LC()
   : RO_SystemVariable(ID_Quad_LC)
{
   Symbol::assign(IntScalar(0, LOC), LOC);
}
//-----------------------------------------------------------------------------
Value_P
Quad_LC::get_apl_value() const
{

   // count how many function elements Quad-LC has...
   //
int len = 0;
   for (StateIndicator * si = Workspace::SI_top();
        si; si = si->get_parent())
       {
         if (si->get_executable()->get_parse_mode() == PM_FUNCTION)   ++len;
       }

Value_P Z(new Value(len, LOC));

   for (StateIndicator * si = Workspace::SI_top();
        si; si = si->get_parent())
       {
         if (si->get_executable()->get_parse_mode() == PM_FUNCTION)
            new (Z->next_ravel())   IntCell(si->get_line());
       }

   Z->set_default_Zero();
   Z->check_value(LOC);
   return Z;
}
//=============================================================================
Quad_LX::Quad_LX()
   : NL_SystemVariable(ID_Quad_LX)
{
   Symbol::assign(Str0(LOC), LOC);
}
//-----------------------------------------------------------------------------
void
Quad_LX::assign(Value_P value, const char * loc)
{
   if (value->get_rank() > 1)      RANK_ERROR;
   if (!value->is_char_string())   DOMAIN_ERROR;

   Symbol::assign(value, LOC);
}
//=============================================================================
Quad_PP::Quad_PP()
   : SystemVariable(ID_Quad_PP)
{
Value_P value(new Value(LOC));

   new (&value->get_ravel(0)) IntCell(DEFAULT_Quad_PP);

   value->check_value(LOC);
   Symbol::assign(value, LOC);
}
//-----------------------------------------------------------------------------
void
Quad_PP::assign(Value_P value, const char * loc)
{
   if (!value->is_scalar_or_len1_vector())
      {
        if (value->get_rank() > 1)   RANK_ERROR;
        else                         LENGTH_ERROR;
      }

const Cell & cell = value->get_ravel(0);
APL_Integer val = cell.get_near_int(0.1);
   if (val < MIN_Quad_PP)   DOMAIN_ERROR;

   if (val > MAX_Quad_PP)   val = MAX_Quad_PP;

   Symbol::assign(IntScalar(val, LOC), LOC);
}
//=============================================================================
Quad_PR::Quad_PR()
   : SystemVariable(ID_Quad_PR)
{
   Symbol::assign(Spc(LOC), LOC);
}
//-----------------------------------------------------------------------------
void
Quad_PR::assign(Value_P value, const char * loc)
{
UCS_string ucs = value->get_UCS_ravel();

   if (ucs.size() > 1)   LENGTH_ERROR;

   Symbol::assign(value, LOC);
}
//=============================================================================
Quad_PS::Quad_PS()
   : SystemVariable(ID_Quad_PS)
{
   Symbol::assign(IntScalar(0, LOC), LOC);
}
//-----------------------------------------------------------------------------
void
Quad_PS::assign(Value_P value, const char * loc)
{
   if (!value->is_scalar_or_len1_vector())
      {
        if (value->get_rank() > 1)   RANK_ERROR;
        else                         LENGTH_ERROR;
      }

const Cell & cell = value->get_ravel(0);
const APL_Integer val = cell.get_near_int(0.1);
   switch(val)
      {
        case 0:
        case 1:
        case 2:
        case 3:  Symbol::assign(IntScalar(val, LOC), LOC);   return;
        default: DOMAIN_ERROR;
      }
}
//=============================================================================
Quad_PT::Quad_PT()
   : RO_SystemVariable(ID_Quad_PT)
{
   Symbol::assign(IntScalar(0, LOC), LOC);
}
//-----------------------------------------------------------------------------
Value_P
Quad_PT::get_apl_value() const
{
Value_P Z(new Value(LOC));
   new (&Z->get_ravel(0))   FloatCell(Workspace::get_CT());

   Z->check_value(LOC);
   return Z;
}
//=============================================================================
Quad_PW::Quad_PW()
   : SystemVariable(ID_Quad_PW)
{
   Symbol::assign(IntScalar(DEFAULT_Quad_PW, LOC), LOC);
}
//-----------------------------------------------------------------------------
void
Quad_PW::assign(Value_P value, const char * loc)
{
   if (!value->is_scalar_or_len1_vector())
      {
        if (value->get_rank() > 1)   RANK_ERROR;
        else                         LENGTH_ERROR;
      }

const Cell & cell = value->get_ravel(0);
const APL_Integer val = cell.get_near_int(0.1);

   // min val is 30. Ignore smaller values.
   if (val < 30)   return;

   // max val is system specific. Ignore larger values.
   if (val > MAX_Quad_PW)   return;

   Symbol::assign(value, LOC);
}
//=============================================================================
Quad_Quad::Quad_Quad()
 : SystemVariable(ID_Quad_Quad)
{
}
//-----------------------------------------------------------------------------
void
Quad_Quad::assign(Value_P value, const char * loc)
{
   // write pending LF from  ⍞ (if any)
   Quad_QUOTE::done(true, LOC);

   value->print(COUT);
}
//-----------------------------------------------------------------------------
void
Quad_Quad::resolve(Token & token, bool left)
{
   if (left)   return;   // ⎕←xxx

   // write pending LF from  ⍞ (if any)
   Quad_QUOTE::done(true, LOC);

   COUT << UNI_Quad_Quad << ":" << endl;

bool eof = false;
UCS_string line;
   InputMux::get_line(LIM_Quad_Quad, Workspace::get_prompt(), line, eof,
                      LineHistory::quad_quad_history);

   line.remove_leading_and_trailing_whitespaces();

   move_2(token, Bif_F1_EXECUTE::execute_statement(line), LOC);
}
//=============================================================================
Quad_QUOTE::Quad_QUOTE()
 : SystemVariable(ID_QUOTE_Quad)
{
   // we assign a dummy value so that ⍞ is not undefined.
   //
Value_P dummy(new Value(UCS_string(LOC), LOC));
   dummy->set_complete();
   Symbol::assign(dummy, LOC);
}
//-----------------------------------------------------------------------------
void
Quad_QUOTE::done(bool with_LF, const char * loc)
{
   Log(LOG_cork)
      CERR << "Quad_QUOTE::done(" << with_LF << ") called from " << loc
           << " , buffer = [" << prompt << "]" << endl;

   if (prompt.size())
      {
        if (with_LF)   COUT << endl;
        prompt.clear();
      }
}
//-----------------------------------------------------------------------------
void
Quad_QUOTE::assign(Value_P value, const char * loc)
{
   Log(LOG_cork)
      CERR << "Quad_QUOTE::assign() called, buffer = ["
           << prompt << "]" << endl;

PrintContext pctx(PR_QUOTE_Quad);
PrintBuffer pb(*value, pctx, 0);
   if (pb.get_height() > 1)   // multi line output: flush and restart corking
      {
        loop(y, pb.get_height())
           {
             done(true, LOC);
             prompt = pb.get_line(y).no_pad();
             COUT << prompt;
           }
      }
   else if (pb.get_height() > 0)
      {
        COUT << pb.l1().no_pad() << flush;
        prompt.append(pb.l1());
      }
   else   // empty output
      {
        // nothing to do
      }

   cout << flush;

   Log(LOG_cork)
      CERR << "Quad_QUOTE::assign() done, buffer = ["
           << prompt << "]" << endl;
}
//-----------------------------------------------------------------------------
Value_P
Quad_QUOTE::get_apl_value() const
{
   Log(LOG_cork)
      CERR << "Quad_QUOTE::get_apl_value() called, buffer = ["
           << prompt << "]" << endl;

   // get_quad_cr_line() may call done(), so we save the current prompt.
   //
const UCS_string old_prompt(prompt);

bool eof = false;
UCS_string line;
   InputMux::get_line(LIM_Quote_Quad, old_prompt, line, eof,
                      LineHistory::quote_quad_history);
   done(false, LOC);   // if get_quad_cr_line() has not called it

   if (interrupt_raised)   INTERRUPT;

const UCS_string qpr = Workspace::get_PR();

   if (qpr.size() > 0)   // valid prompt replacement char
      {
        loop(i, line.size())
           {
             if (i >= old_prompt.size())   break;
             if (old_prompt[i] != line[i])   break;
             line[i] = qpr[0];
           }
      }

Value_P Z(new Value(line, LOC));
   Z->check_value(LOC);
   return Z;
}
//=============================================================================
Quad_R::Quad_R()
 : NL_SystemVariable(ID_Quad_R)
{
   Symbol::assign(IntScalar(0, LOC), LOC);
}
//-----------------------------------------------------------------------------
void
Quad_R::assign(Value_P value, const char * loc)
{
StateIndicator * si = Workspace::SI_top_fun();
   if (si == 0)   return;

   // ignore assignments if error was SYNTAX ERROR or VALUE ERROR
   //
   if (si->get_error().is_syntax_or_value_error())   return;

   si->set_R(value);
}
//-----------------------------------------------------------------------------
Value_P
Quad_R::get_apl_value() const
{
StateIndicator * si = Workspace::SI_top_error();
   if (si)
      {
        Value_P ret = si->get_R();
        if (!!ret)   return  ret;
      }

   VALUE_ERROR;
}
//=============================================================================
void
Quad_SYL::assign(Value_P value, const char * loc)
{
   // Quad_SYL is mostly read-only, so we only allow assign_indexed() with
   // certain values.
   //
   if (!!value)   SYNTAX_ERROR;

   // this assign is called from the constructor in order to trigger the
   // creation of a symbol for Quad_SYL.
   //
   Symbol::assign(IntScalar(0, LOC), LOC);
}
//-----------------------------------------------------------------------------
void
Quad_SYL::assign_indexed(IndexExpr & IDX, Value_P value)
{
   //  must be an array index of the form [something; 2]
   //
   if (IDX.value_count() != 2)   INDEX_ERROR;

   // IDX is in reverse order: ⎕SYL[X1;X2]
   //
Value_P X2 = IDX.extract_value(0);

const APL_Float qct = Workspace::get_CT();
const APL_Integer qio = Workspace::get_IO();

   if (!X2)                                             INDEX_ERROR;
   if (X2->element_count() != 1)                        INDEX_ERROR;
   if (!X2->get_ravel(0).is_near_int(qct))              INDEX_ERROR;
   if (X2->get_ravel(0).get_near_int(qct) != qio + 1)   INDEX_ERROR;

Value_P X1 = IDX.extract_value(1);

   assign_indexed(X1, value);
}
//-----------------------------------------------------------------------------
void
Quad_SYL::assign_indexed(Value_P X, Value_P B)
{
const APL_Float qct = Workspace::get_CT();

   if (!(X->is_int_scalar(qct) || X->is_int_vector(qct)))   INDEX_ERROR;
   if (!(B->is_int_scalar(qct) || B->is_int_vector(qct)))   DOMAIN_ERROR;
   if (X->element_count() != B->element_count())            LENGTH_ERROR;

const ShapeItem ec = X->element_count();
const APL_Integer qio = Workspace::get_IO();

   loop(e, ec)
      {
        const APL_Integer x = X->get_ravel(e).get_near_int(qct) - qio;
        const APL_Integer b = B->get_ravel(e).get_near_int(qct);

        if (x == SYL_SI_DEPTH_LIMIT)   // SI depth limit
           {
             const int depth = Workspace::SI_entry_count();
             if (b && (b < (depth + 4)))   DOMAIN_ERROR;    // limit too low
             si_depth_limit = b;
           }
        else if (x == SYL_VALUE_COUNT_LIMIT)   // value count limit
           {
             if (b && (b < (Value::value_count + 10)))    // too low
                DOMAIN_ERROR;
             value_count_limit = b;
           }
        else if (x == SYL_RAVEL_BYTES_LIMIT)   // ravel bytes limit
           {
             const int cells = b / sizeof(Cell);
             if (cells && (cells < (Value::total_ravel_count + 1000)))
                DOMAIN_ERROR;

             ravel_count_limit = cells;
           }
#if CORE_COUNT_WANTED == -3
        else if (x == SYL_CURRENT_CORES)   // number of cores
           {
             setup_cores((CoreCount)b);
           }
#endif
        else if (x == SYL_PRINT_LIMIT)   // print length limit
           {
             if (b < 1000)   DOMAIN_ERROR;
             print_length_limit = b;
           }
        else
           {
             INDEX_ERROR;
           }
      }
}
//-----------------------------------------------------------------------------
Value_P
Quad_SYL::get_apl_value() const
{
const Shape sh(SYL_MAX, 2);
Value_P Z( new Value(sh, LOC));

#define syl2(n, e, v) syl1(n, e, v)
#define syl3(n, e, v) syl1(n, e, v)
#define syl1(n, _e, v) \
  new (Z->next_ravel()) PointerCell(Value_P(new Value(UCS_string(UTF8_string(n)), LOC))); \
  new (Z->next_ravel()) IntCell(v);
#include "SystemLimits.def"

   Z->check_value(LOC);
   return Z;
}
//=============================================================================
Quad_TC::Quad_TC()
   : RO_SystemVariable(ID_Quad_TC)
{
Value_P QCT(new Value(3, LOC));
   new (QCT->next_ravel()) CharCell(UNI_ASCII_BS);
   new (QCT->next_ravel()) CharCell(UNI_ASCII_CR);
   new (QCT->next_ravel()) CharCell(UNI_ASCII_LF);
   QCT->check_value(LOC);

   Symbol::assign(QCT, LOC);
}
//=============================================================================
Quad_TS::Quad_TS()
   : RO_SystemVariable(ID_Quad_TS)
{
   Symbol::assign(IntScalar(0, LOC), LOC);
}
//-----------------------------------------------------------------------------
Value_P
Quad_TS::get_apl_value() const
{
const APL_time_us offset_us = 1000000 * Workspace::get_v_Quad_TZ().get_offset();
const YMDhmsu time(now() + offset_us);

Value_P Z(new Value(7, LOC));
   new (&Z->get_ravel(0)) IntCell(time.year);
   new (&Z->get_ravel(1)) IntCell(time.month);
   new (&Z->get_ravel(2)) IntCell(time.day);
   new (&Z->get_ravel(3)) IntCell(time.hour);
   new (&Z->get_ravel(4)) IntCell(time.minute);
   new (&Z->get_ravel(5)) IntCell(time.second);
   new (&Z->get_ravel(6)) IntCell(time.micro / 1000);

   Z->check_value(LOC);
   return Z;
}
//=============================================================================
Quad_TZ::Quad_TZ()
   : SystemVariable(ID_Quad_TZ)
{
   offset_seconds = compute_offset();

   // ⎕TZ is the offset in hours between GMT and local time
   //
   if (offset_seconds % 3600 == 0)   // full hour
      Symbol::assign(IntScalar(offset_seconds/3600, LOC), LOC);
   else
      Symbol::assign(FloatScalar(offset_seconds/3600, LOC), LOC);
}
//-----------------------------------------------------------------------------
int
Quad_TZ::compute_offset()
{
   // the GNU/Linux timezone variable conflicts with function timezone() on
   // other systems. timezone.tz_minuteswest in gettimeofday() does not
   // sound reliable either.
   // 
   // We compute localtime() and gmtime() and take the difference (hoping
   // that localtime() and gmtime() are more portable.
   // 

   // choose a reference point in time that is the same month in all time zones.
   // we use Monday Jan 2014 in the afternoon
   //
const time_t ref = now()/1000000;

tm * local = localtime(&ref);
const int local_minutes = local->tm_min + 60*local->tm_hour;
const int local_days = local->tm_mday + 31*local->tm_mon + 12*31*local->tm_year;

tm * gmean = gmtime(&ref);
const int gm_minutes    = gmean->tm_min + 60*gmean->tm_hour;
const int gm_days = gmean->tm_mday + 31*gmean->tm_mon + 12*31*gmean->tm_year;
int diff_minutes = local_minutes - gm_minutes;

   if (local_days < gm_days)        diff_minutes -= 24*60;   // local < gmean
   else if (local_days > gm_days)   diff_minutes += 24*60;   // local > gmean

   return 60*diff_minutes;
}
//-----------------------------------------------------------------------------
void
Quad_TZ::assign(Value_P value, const char * loc)
{
   if (!value->is_scalar())   RANK_ERROR;

   // ignore values outside [-12 ... 14], DOMAIN ERROR for bad types.

Cell & cell = value->get_ravel(0);
   if (cell.is_integer_cell())
      {
        const APL_Integer ival = cell.get_int_value();
        if (ival < -12)   return;
        if (ival > 14)    return;
        offset_seconds = ival*3600;
        Symbol::assign(value, LOC);
        return;
      }

   if (cell.is_float_cell())
      {
        const APL_Float fval = cell.get_real_value();
        if (fval < -12.1)   return;
        if (fval > 14.1)    return;
        offset_seconds = int(0.5 + fval*3600);
        Symbol::assign(value, LOC);
        return;
      }

   DOMAIN_ERROR;
}
//=============================================================================
Quad_UL::Quad_UL()
   : RO_SystemVariable(ID_Quad_UL)
{
   Symbol::assign(get_apl_value(), LOC);
}
//-----------------------------------------------------------------------------
Value_P
Quad_UL::get_apl_value() const
{
int user_count = 0;

#ifdef USER_PROCESS
   setutxent();   // rewind utmp file

   for (;;)
       {
         utmpx * entry = getutxent();
         if (entry == 0)   break;   // end of  user accounting database

         if (entry->ut_type == USER_PROCESS)   ++user_count;
       }

   endutxent();   // close utmp file
#else
   user_count = 1;
#endif

Value_P Z(new Value(LOC));
   new (&Z->get_ravel(0))   IntCell(user_count);
   Z->check_value(LOC);
   return Z;
}
//=============================================================================
Quad_WA::Quad_WA()
   : RO_SystemVariable(ID_Quad_WA)
{
   Symbol::assign(IntScalar(0, LOC), LOC);
}
//-----------------------------------------------------------------------------
Value_P
Quad_WA::get_apl_value() const
{
Value_P Z(new Value(LOC));

uint64_t total = total_memory;   // max memory as reported by RLIM_INFINITY
uint64_t proc_mem = 0;            // memory as reported proc/mem_info

   {
     FILE * pm = fopen("/proc/meminfo", "r");

     if (pm)   // /proc/meminfo exists
        {
          for (;;)
              {
                char buffer[2000];
                if (fgets(buffer, sizeof(buffer) - 1, pm) == 0)   break;
                buffer[sizeof(buffer) - 1] = 0;
                if (!strncmp(buffer, "MemFree:", 8))
                   {
                     proc_mem = atoi(buffer + 8);
                     proc_mem *= 1024;
                     break;
                   }
              }

          fclose(pm);
        }
   }

   if (proc_mem == 0)   // nothing from /proc/meminfo
      {
        if (total == RLIM_INFINITY)   // no rlimit
           {
             CERR << "Cannot properly determine ⎕WA (process has no memory"
                     " limit and /proc/meminfo provided no info)" << endl;
             total = 1000000;
           }
        else                          // process has rlimit
           {
             total -= Value::total_ravel_count * sizeof(Cell)
                    + Value::value_count * sizeof(Value);
           }
      }
   else
      {
        if (total == RLIM_INFINITY)   // no rlimit
           {
             total = proc_mem;   // use value from .proc/meminfo
           }
        else                          // process has rlimit
           {
             total -= Value::total_ravel_count * sizeof(Cell)
                    + Value::value_count * sizeof(Value);
             if (total > proc_mem)   total = proc_mem;   // use minimum
           }
      }

   new (&Z->get_ravel(0))   IntCell(total);

   Z->check_value(LOC);
   return Z;
}
//=============================================================================
Quad_X::Quad_X()
 : NL_SystemVariable(ID_Quad_X)
{
   Symbol::assign(IntScalar(0, LOC), LOC);
}
//-----------------------------------------------------------------------------
void
Quad_X::assign(Value_P value, const char * loc)
{
StateIndicator * si = Workspace::SI_top_fun();
   if (si == 0)   return;

   // ignore assignments if error was SYNTAX ERROR or VALUE ERROR
   //
   if (si->get_error().is_syntax_or_value_error())   return;

   si->set_X(value);
}
//-----------------------------------------------------------------------------
Value_P
Quad_X::get_apl_value() const
{
StateIndicator * si = Workspace::SI_top_error();
   if (si)
      {
        Value_P ret = si->get_X();
        if (!!ret)   return ret;
      }

   VALUE_ERROR;
}
//=============================================================================
