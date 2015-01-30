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

#include <string.h>

#include "IndexExpr.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include "Symbol.hh"
#include "Token.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
Token::Token(const Token & other)
   : tag(TOK_VOID)
{
   copy_1(*this, other, "Token::Token(other)");
}
//-----------------------------------------------------------------------------
Token::Token(TokenTag tg, IndexExpr & idx)
{
   Assert(&idx);

   if (tg == TOK_PINDEX)
      {
        // this token is a partial index in the prefix parser.
        // use it as is.
        //
        tag = TOK_PINDEX;
        value.index_val = &idx;
      }
   else
      {
        // this token is a complete index
        //
        Assert(tg == TOK_INDEX);
        if (idx.value_count() < 2)   // [idx] or []
           {
             tag = TOK_AXES;
             if (idx.value_count() == 0)   // []
                {
                  new (&value._apl_val()) Value_P;
                }
             else                          // [x]
                {
                  new (&value._apl_val())   Value_P(idx.extract_value(0));
                }
           }
        else                         // [idx; ...]
           {
             tag = TOK_INDEX;
             value.index_val = &idx;
           }
      }
}
//-----------------------------------------------------------------------------
ostream &
operator << (ostream & out, TokenTag tag)
{
   if (tag > 0xFFFF)   out << HEX(tag);
   else                out << HEX4(tag);

   return out;
}
//-----------------------------------------------------------------------------
ostream &
operator << (ostream & out, TokenClass tc)
{
#define tcc(x) case x: out << #x;   break;
   switch(tc)
      {
        tcc(TC_INVALID)
        tcc(TC_END)
        tcc(TC_RETURN)
        tcc(TC_VALUE)
        tcc(TC_INDEX)
        tcc(TC_PINDEX)
        tcc(TC_SYMBOL)
        tcc(TC_FUN0)
        tcc(TC_FUN12)
        tcc(TC_OPER1)
        tcc(TC_OPER2)
        tcc(TC_R_PARENT)
        tcc(TC_L_PARENT)
        tcc(TC_R_BRACK)
        tcc(TC_L_BRACK)
        tcc(TC_R_ARROW)
        tcc(TC_ASSIGN)
        tcc(TC_LINE)
        tcc(TC_VOID)
        tcc(TC_DIAMOND)
        tcc(TC_NUMERIC)
        tcc(TC_SPACE)
        tcc(TC_NEWLINE)
        tcc(TC_COLON)
        tcc(TC_QUOTE)
        tcc(TC_OFF)
        tcc(TC_SI_LEAVE)

        default: break;
      }
#undef tcc

   return out;
}
//-----------------------------------------------------------------------------
ostream &
operator << (ostream & out, const Token & token)
{
   if (token.get_tag() == TOK_CHARACTER)
      {
        out << "CHAR«" << token.get_char_val() << "»";
        return out;
      }

   if (token.get_tag() == TOK_APL_VALUE1 ||
       token.get_tag() == TOK_APL_VALUE2 ||
       token.get_tag() == TOK_APL_VALUE3)
      {
        if      (token.get_tag() == TOK_APL_VALUE1)   out << "VALUE1";
        else if (token.get_tag() == TOK_APL_VALUE2)   out << "VALUE2";
        else if (token.get_tag() == TOK_APL_VALUE3)   out << "VALUE3";
        else                                          out << "VALUE???";
        Value_P value = token.get_apl_val();
        Assert(value);
        const Depth depth = value->compute_depth();
        out << "«";
        for (Depth d = 0; d < depth; ++d)   out << "≡";

        if (value->get_rank())   out << value->get_shape();

        const PrintContext pctx(PR_APL);
        PrintBuffer pb(*value, pctx, 0);
        bool more = pb.get_height() > 1;
        if (pb.get_height() > 0)
           {
             UCS_string ucs = pb.get_line(0).no_pad();
             if (ucs.size() > 20)
                {
                  ucs.shrink(20);
                  more = true;
                }
             out << ucs;
           }
        if (more)   out << "...";
        return out << "»";
      }

   if (token.get_tag() == TOK_ERROR)
      {
        return out << Error::error_name(ErrorCode(token.get_int_val()));
      }

   if (token.get_tag() == TOK_BRANCH)
      {
        return out << token.get_Id() << token.get_int_val();
      }

   if (token.get_tag() == TOK_NOBRANCH)
      {
        return out << token.get_Id() << token.get_int_val();
      }

   if (token.get_Id() > ID::No_ID2)   return out << token.get_Id();

   switch(token.get_Class())
      {
        case TC_END:      return out << "END";
        case TC_RETURN:
             if (token.get_tag() == TOK_RETURN_EXEC)
                          return out << "RETURN ⍎";
             if (token.get_tag() == TOK_RETURN_STATS)
                          return out << "RETURN ◊";
             if (token.get_tag() == TOK_RETURN_VOID)
                          return out << "RETURN ∇FUN";
             if (token.get_tag() == TOK_RETURN_SYMBOL)
                          return out << "RETURN Z←FUN";
             return out << "RETURN ???";

        case TC_VALUE:    return token.print_value(out);

        case TC_INDEX:
        case TC_PINDEX:
             if (token.get_tag() == TOK_INDEX)
                return out << token.get_index_val();
             else if (token.get_tag() == TOK_AXES)
                return out << *token.get_axes();
             else
                FIXME;

        case TC_SYMBOL:   if (token.get_tag() == TOK_LSYMB)
                             {
                               token.get_sym_ptr()->print(out << "'");
                               out << UNI_LEFT_ARROW;
                             }
                          else if (token.get_tag() == TOK_LSYMB2)
                             {
                               token.get_sym_ptr()->print(out << "'(... ");
                               out << ")←";
                             }
                          else
                             {
                               token.get_sym_ptr()->print(out << "'");
                             }
                          return out;
        case TC_FUN0:
        case TC_FUN12:
        case TC_OPER1:
        case TC_OPER2:    return token.print_function(out);
        case TC_LINE:     return out << "\n[" << token.get_fun_line() << "] ";
        case TC_NUMERIC:  return out << "Numeric";
        case TC_NEWLINE:  return out << "{LFeed}";

        default: break;
      }

   return out <<  "{-unknown Token " << token.get_tag() << "-}";
}
//-----------------------------------------------------------------------------
void
Token::ChangeTag(TokenTag new_tag)
{
   Assert((tag & TV_MASK) == (new_tag & TV_MASK));
   // tag is ia const TokenTag, so we cheat a little here.
   (TokenTag &)tag = new_tag;
}
//-----------------------------------------------------------------------------
void Token::extract_apl_val(const char * loc) const
{
   if (is_apl_val())   ptr_clear(value._apl_val(), loc);
}
//-----------------------------------------------------------------------------
ostream &
Token::print_function(ostream & out) const
{
   switch(tag)
      {
        case TOK_Quad_TC:
        case TOK_Quad_TS:
        case TOK_Quad_UL:
        case TOK_Quad_WA:
        case TOK_Quad_AF:
        case TOK_Quad_CR:
        case TOK_Quad_DL:
        case TOK_Quad_EC:
        case TOK_Quad_EX:
        case TOK_Quad_SVQ:
        case TOK_Quad_SVR:
        case TOK_Quad_SVS:
        case TOK_Quad_UCS:
        case TOK_Quad_AT:
        case TOK_Quad_EA:
        case TOK_Quad_SVC:
        case TOK_Quad_TF:
        case TOK_Quad_ES:
        case TOK_Quad_FX:
        case TOK_Quad_NA:
        case TOK_Quad_NL:
        case TOK_Quad_SI:
        case TOK_Quad_SVO:      return print_quad(out);

        case TOK_F1_EXECUTE:    return out << UNI_EXECUTE;

        case TOK_F2_LESS:       return out << UNI_ASCII_LESS;
        case TOK_F2_FIND:       return out << UNI_EPSILON_UBAR;
        case TOK_F2_EQUAL:      return out << UNI_ASCII_EQUAL;
        case TOK_F2_GREATER:    return out << UNI_ASCII_GREATER;
        case TOK_ASSIGN:        return out << UNI_LEFT_ARROW;
        case TOK_F2_AND:        return out << UNI_AND;
        case TOK_F2_OR:         return out << UNI_OR;
        case TOK_F2_INDEX:      return out << UNI_SQUISH_Quad;
        case TOK_F2_LEQ:        return out << UNI_LESS_OR_EQUAL;
        case TOK_F2_MEQ:        return out << UNI_MORE_OR_EQUAL;
        case TOK_F2_UNEQ:       return out << UNI_NOT_EQUAL;
        case TOK_F2_NOR:        return out << UNI_NOR;
        case TOK_F2_NAND:       return out << UNI_NAND;

        case TOK_F12_BINOM:     return out << UNI_ASCII_EXCLAM;
        case TOK_F12_CIRCLE:    return out << UNI_CIRCLE;
        case TOK_F12_COMMA:     return out << UNI_ASCII_COMMA;
        case TOK_F12_COMMA1:    return out << UNI_COMMA_BAR;
        case TOK_F12_DECODE:    return out << UNI_UP_TACK;
        case TOK_F12_DIVIDE:    return out << UNI_DIVIDE;
        case TOK_F12_DOMINO:    return out << UNI_Quad_DIVIDE;
        case TOK_F12_DROP:      return out << UNI_DOWN_ARROW;
        case TOK_F12_ELEMENT:   return out << UNI_ELEMENT;
        case TOK_F12_ENCODE:    return out << UNI_DOWN_TACK;
        case TOK_F12_EQUIV:     return out << UNI_EQUIVALENT;
        case TOK_F12_FORMAT:    return out << UNI_FORMAT;
        case TOK_F12_INDEX_OF:  return out << UNI_IOTA;
        case TOK_F12_LOGA:      return out << UNI_LOGARITHM;
        case TOK_F12_MINUS:     return out << UNI_ASCII_MINUS;
        case TOK_F12_PARTITION: return out << UNI_SUBSET;
        case TOK_F12_PICK:      return out << UNI_SUPERSET;
        case TOK_F12_PLUS:      return out << UNI_ASCII_PLUS;
        case TOK_F12_POWER:     return out << UNI_STAR_OPERATOR;
        case TOK_F12_RHO:       return out << UNI_RHO;
        case TOK_F12_RND_DN:    return out << UNI_LEFT_FLOOR;
        case TOK_F12_RND_UP:    return out << UNI_LEFT_CEILING;
        case TOK_F12_ROLL:      return out << UNI_ASCII_QUESTION;
        case TOK_F12_ROTATE:    return out << UNI_CIRCLE_STILE;
        case TOK_F12_ROTATE1:   return out << UNI_CIRCLE_BAR;
        case TOK_F12_SORT_ASC:  return out << UNI_SORT_ASCENDING;
        case TOK_F12_SORT_DES:  return out << UNI_SORT_DECENDING;
        case TOK_F12_STILE:     return out << UNI_DIVIDES;
        case TOK_F12_TAKE:      return out << UNI_UP_ARROW;
        case TOK_F12_TIMES:     return out << UNI_MULTIPLY;
        case TOK_F12_TRANSPOSE: return out << UNI_TRANSPOSE;
        case TOK_F12_WITHOUT:   return out << UNI_TILDE_OPERATOR;

        case TOK_JOT:           return out << UNI_RING_OPERATOR;

        case TOK_OPER1_EACH:
        case TOK_OPER1_REDUCE:
        case TOK_OPER1_REDUCE1:
        case TOK_OPER1_SCAN:
        case TOK_OPER1_SCAN1:
        case TOK_OPER2_OUTER:
        case TOK_OPER2_INNER: return out << get_Id();

        case TOK_FUN0:      return get_function()->print(out << "USER-F0 ");
        case TOK_FUN2:      return get_function()->print(out << "USER-F2 ");
        case TOK_OPER1:     return get_function()->print(out << "USER-OP1 ");
        case TOK_OPER2:     return get_function()->print(out << "USER-OP2 ");

        default: break;
      }

   // unknown tag.

   out << "{ unknown function Token " << tag;

   if (get_Id() != ID::No_ID)   // tag has an Id
      {
        out << ", Id = " << get_Id();
      }
   return out <<  ", }";
}
//-----------------------------------------------------------------------------
ostream &
Token::print_value(ostream & out) const
{
   switch(tag)
      {
        case TOK_VARIABLE:  return value.sym_ptr->print(out);
        case TOK_FUN0:      return out <<  "{ fun/0 }";
        case TOK_CHARACTER: return out << value.char_val;
        case TOK_INTEGER:   return out << value.int_vals[0];
        case TOK_REAL:      return out << value.float_vals[0];

        case TOK_COMPLEX:   return out << value.float_vals[0] << "J"
                                       << value.float_vals[1];

        case TOK_APL_VALUE1:
        case TOK_APL_VALUE3:
             {
               Value_P v = value._apl_val();
               if (v->get_rank() == 0)   out << "''";
               loop(r, v->get_rank())
                   {
                     if (r)   out <<  " ";
                     out << v->get_shape_item(r);
                   }
               out << UNI_RHO;
               loop(e, v->element_count())
                 {
                   if (e == 4)
                      {
                        out << "...";
                        break;
                      }

                   const PrintContext pctx(PR_APL, 2, 80);
                   out << UCS_string(v->get_ravel(e)
                                       .character_representation(pctx), 0, 80)
                       << " ";
                 }
               return out;
             }

        case TOK_Quad_Quad: return out << UNI_Quad_Quad;

        case TOK_Quad_AI:
        case TOK_Quad_AV:
        case TOK_Quad_CT:
        case TOK_Quad_EM:
        case TOK_Quad_ET:
        case TOK_Quad_FC:
        case TOK_Quad_IO:
        case TOK_Quad_LC:
        case TOK_Quad_LX:
        case TOK_Quad_L:
        case TOK_Quad_PP:
        case TOK_Quad_PR:
        case TOK_Quad_PS:
        case TOK_Quad_PW:
        case TOK_Quad_R:
        case TOK_Quad_RL:
        case TOK_Quad_SVE:
        case TOK_Quad_TZ:
        case TOK_Quad_WA:   return print_quad(out);

        default: break;
      }

   out << "{ unknown value Token " << tag;

   if (get_Id() != ID::No_ID)   // tag has an Id
      {
        out << ", Id = " << get_Id();
      }

   return out <<  ", }";
}
//-----------------------------------------------------------------------------
void
Token::show_trace(ostream & out, const UCS_string & fun_name, 
                  Function_Line line) const
{
UCS_string fn = fun_name;
   fn.append_utf8("[");
   fn.append_number(line);
   fn.append_utf8("] ");

   out << fn;

   switch(get_tag())
      {
        case TOK_APL_VALUE1:
        case TOK_APL_VALUE2:
        case TOK_APL_VALUE3:
             break;   // continue below

        case TOK_BRANCH:
             out << "→" << get_int_val() << endl;
             return;

        case TOK_NOBRANCH:
             out << "→⍬" << endl;
             return;

        case TOK_ESCAPE:
             out << "→" << endl;
             return;

        case TOK_VOID:
        case TOK_NO_VALUE:
             out << endl;
             return;

        default: Q1(*this)
                 FIXME;
      }

   // print a value

PrintContext pctx = Workspace::get_PrintContext();
const Value & val = *get_apl_val();
   if (val.get_rank() == 0)   // scalar
      {
        pctx.set_style(PR_APL_MIN);
      }
   else if (val.get_rank() == 1)   // vector
      {
        if (val.element_count() == 0 &&   // empty vector
            (val.get_ravel(0).is_character_cell() ||
             val.get_ravel(0).is_numeric()))
           {
             out << endl;
             return;
           }
            
        pctx.set_style(PR_APL_MIN);
      }
   else                  // matrix or higher
      {
        pctx.set_style((PrintStyle)(pctx.get_style() | PST_NO_FRACT_0));
      }

PrintBuffer pb(val, pctx, 0);
const UCS_string indent(fn.size(), UNI_ASCII_SPACE);
   loop(l, pb.get_height())
      {
        if (l)   out << indent;
        out << pb.get_line(l).no_pad() << endl;
      }

   if (pb.get_height() == 0)   out << endl;
}
//-----------------------------------------------------------------------------
ostream &
Token::print_quad(ostream & out) const
{
   return out << UNI_Quad_Quad << get_Id();
}
//-----------------------------------------------------------------------------
UCS_string
Token::canonical(PrintStyle style) const
{
UCS_string ucs;

   switch(get_Class())
      {
        case TC_ASSIGN:
             ucs.append(UNI_LEFT_ARROW);
             break;

        case TC_R_ARROW:
             ucs.append(UNI_RIGHT_ARROW);
             break;

        case TC_L_BRACK:
             if (get_tag() == TOK_L_BRACK)
                ucs.append(UNI_ASCII_L_BRACK);
             else if (get_tag() == TOK_SEMICOL)
                ucs.append(UNI_ASCII_SEMICOLON);
             else
                Assert(0);
                break;

        case TC_R_BRACK:
             ucs.append(UNI_ASCII_R_BRACK);
             break;

        case TC_END:
             ucs.append(UNI_DIAMOND);
             break;

        case TC_RETURN:                                                  break;
        case TC_LINE:
             ucs.append(UNI_ASCII_LF);
             break;

        case TC_VALUE:
             {
               PrintContext pctx(style, 16, 80);
               PrintBuffer pbuf(*get_apl_val(), pctx, 0);
               if (pbuf.get_height() == 0)   return ucs;
               return pbuf.l1();
             }

        case TC_SYMBOL:
             ucs.append(get_sym_ptr()->get_name());
             break;


        case TC_R_PARENT:
        case TC_L_PARENT:
        case TC_R_CURLY:
        case TC_L_CURLY:
             return UCS_string(UTF8_string(ID::name(get_Id())));

        case TC_FUN0:
        case TC_FUN12:
        case TC_OPER1:
        case TC_OPER2:
             if (get_Id() == ID::No_ID)   return get_function()->get_name();
             return UCS_string(UTF8_string(ID::name(get_Id())));

        default:
             CERR << "Token: " << HEX4(tag) << " " << *this
                  << " at " << LOC << endl;
             Q1((void *)get_Class())
             Backtrace::show(__FILE__, __LINE__);
             FIXME;
      }

   return ucs;
}
//-----------------------------------------------------------------------------
UCS_string
Token::tag_name() const
{
   switch(get_tag())
      {
#define TD(tag, _tc, _tv, _id) case tag: return UCS_string( #tag );
#include "Token.def"
      }

char cc[40];
   snprintf(cc, sizeof(cc), "0x%X", get_tag());
UCS_string ucs(cc);
   return ucs;
}
//-----------------------------------------------------------------------------
int
Token::error_info(UCS_string & ucs) const
{
UCS_string can = canonical(PR_APL_FUN).remove_pad();

const Unicode c1 = ucs.back();
const Unicode c2 = can.size() ? can[0] : Invalid_Unicode;

   // conditions when we don't need a space
   //
bool need_space = ! (Avec::no_space_after(c1) || Avec::no_space_before(c2));

   if (need_space)   ucs.append(UNI_ASCII_SPACE);

   ucs.append(can);
   return need_space ? -can.size() : can.size();
}
//-----------------------------------------------------------------------------
const char * 
Token::short_class_name(TokenClass cls)
{
   switch(cls)
      {
        case TC_ASSIGN:    return "←";
        case TC_R_ARROW:   return "→";
        case TC_L_BRACK:   return "[";
        case TC_R_BRACK:   return "]";
        case TC_END:       return "END";
        case TC_FUN0:      return "F0";
        case TC_FUN12:     return "F12";
        case TC_INDEX:     return "IDX";
        case TC_OPER1:     return "OP1";
        case TC_OPER2:     return "OP2";
        case TC_L_PARENT:  return "(";
        case TC_R_PARENT:  return ")";
        case TC_RETURN:    return "RET";
        case TC_SYMBOL:    return "SYM";
        case TC_VALUE:     return "VAL";

        case TC_PINDEX:    return "PIDX";
        case TC_VOID:      return "VOID";

        case TC_OFF:       return "OFF";
        case TC_SI_LEAVE:  return "LEAVE";
        case TC_LINE:      return "LINE";
        case TC_DIAMOND:   return "◊";
        case TC_NUMERIC:   return "NUMB";
        case TC_SPACE:     return "SPACE";
        case TC_NEWLINE:   return "LF";
        case TC_COLON:     return ":";
        case TC_QUOTE:     return "QUOTE";
        case TC_L_CURLY:   return "{";
        case TC_R_CURLY:   return "}";

        case TC_INVALID:   return "INV";
        default:           break;
      }

   return "???";
}
//-----------------------------------------------------------------------------
void
Token::print_token_list(ostream & out, const Source<Token> & src)
{
const int len = src.rest();
   loop(t, len)   out << "`" << src[t] << "  ";
   out << endl;
}
//-----------------------------------------------------------------------------
ostream &
operator << (ostream & out, const Token_string & tos)
{
   out << "[" << tos.size() << " token]: ";
   Token::print_token_list(out, tos);
   out << endl;
   return out;
}
//-----------------------------------------------------------------------------
const char *
Token::class_name(TokenClass tc)
{
#define tcn(x) case x: return #x;

   switch(tc)
      {
        tcn( TC_ASSIGN   )
        tcn( TC_R_ARROW  )
        tcn( TC_L_BRACK  )
        tcn( TC_R_BRACK  )
        tcn( TC_END      )
        tcn( TC_FUN0     )
        tcn( TC_FUN12    )
        tcn( TC_INDEX    )
        tcn( TC_OPER1    )
        tcn( TC_OPER2    )
        tcn( TC_L_PARENT )
        tcn( TC_R_PARENT )
        tcn( TC_RETURN   )
        tcn( TC_SYMBOL   )
        tcn( TC_VALUE    )

        tcn( TC_PINDEX   )
        tcn( TC_VOID     )

        tcn( TC_OFF      )
        tcn( TC_SI_LEAVE )
        tcn( TC_LINE     )
        tcn( TC_DIAMOND  )
        tcn( TC_NUMERIC  )
        tcn( TC_SPACE    )
        tcn( TC_NEWLINE  )
        tcn( TC_COLON    )
        tcn( TC_QUOTE    )
        tcn( TC_L_CURLY  )
        tcn( TC_R_CURLY  )

        tcn( TC_INVALID  )
        default: break;
      }

   return "*** Obscure token class ***";
}
//-----------------------------------------------------------------------------
inline void 
Token::copy_N(const Token & src)
{
   tag = src.tag;
   switch(src.get_ValueType())
      {
        case TV_NONE:  value.int_vals[0]   = 0;
                       value.int_vals[1]   = src.value.int_vals[1];     break;

        case TV_CHAR:  value.char_val      = src.value.char_val;        break;
        case TV_INT:   value.int_vals[0]   = src.value.int_vals[0];
                       value.int_vals[1]   = src.value.int_vals[1];     break;
        case TV_FLT:   value.float_vals[0] = src.value.float_vals[0];   break;
        case TV_CPX:   value.float_vals[0] = src.value.float_vals[0];
                       value.float_vals[1] = src.value.float_vals[1];   break;
        case TV_SYM:   value.sym_ptr       = src.value.sym_ptr;         break;
        case TV_LIN:   value.fun_line      = src.value.fun_line;        break;
        case TV_VAL:   value._apl_val()    = src.value._apl_val();      break;
        case TV_INDEX: value.index_val     = src.value.index_val;       break;
        case TV_FUN:   value.function      = src.value.function;        break;
        default:       FIXME;
      }
}
//-----------------------------------------------------------------------------
void
copy_1(Token & dst, const Token & src, const char * loc)
{
   dst.clear(loc);
   if (src.is_apl_val())
      {
        Value * val = src.get_apl_val_pointer();
        if (val)
           {
             ADD_EVENT(val, VHE_TokCopy1, src.value_use_count(), loc);
           }
        else
           {
             ADD_EVENT(val, VHE_TokCopy1, -1, loc);
           }
      }

   dst.copy_N(src);
}
//-----------------------------------------------------------------------------
void
move_1(Token & dst, Token & src, const char * loc)
{
   dst.clear(loc);
   dst.copy_N(src);

   if (src.is_apl_val())
      {
        Value * val = src.get_apl_val_pointer();
	ADD_EVENT(val, VHE_TokMove1, src.value_use_count() - 1, loc);
        src.clear(loc);
      }
}
//-----------------------------------------------------------------------------
void
move_2(Token & dst, const Token & src, const char * loc)
{
   dst.clear(loc);
   dst.copy_N(src);

   if (src.is_apl_val())
      {
        Value * val = src.get_apl_val_pointer();
        ADD_EVENT(val, VHE_TokMove2, src.value_use_count() - 1, loc);
        ((Token &)src).clear(loc);
      }
}
//-----------------------------------------------------------------------------
void
Token_string::print(ostream & out, int from) const
{
   loop(t, size() - from)
       {
         const Token & tok = (*this)[from + t];
         out << "`" << tok << "  ";
       }

   out << endl;
}
//-----------------------------------------------------------------------------
