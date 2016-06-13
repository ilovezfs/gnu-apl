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

#include "CDR.hh"
#include "Macro.hh"
#include "PointerCell.hh"
#include "Quad_CR.hh"
#include "Symbol.hh"
#include "Workspace.hh"

Quad_CR    Quad_CR   ::_fun;

Quad_CR    * Quad_CR   ::fun = &Quad_CR   ::_fun;

//-----------------------------------------------------------------------------
Token
Quad_CR::eval_B(Value_P B)
{
UCS_string symbol_name(*B.get());

   // remove trailing whitespaces in B
   //
   while (symbol_name.back() <= ' ')   symbol_name.pop_back();

   /*  return an empty character matrix,     if:
    *  1) symbol_name is not known,          or
    *  2) symbol_name is not user defined,   or
    *  3) symbol_name is a function,         or
    *  4) symbol_name is not displayable
    */

   if (symbol_name.size() == 0)   return Token(TOK_APL_VALUE1, Str0_0(LOC));

const Function * function = 0;
   if (symbol_name[0] == UNI_MUE)   // macro
      {
        loop(m, Macro::MAC_COUNT)
            {
              Macro * macro = Macro::get_macro((Macro::Macro_num)m);
              if (symbol_name == macro->get_name())
                 {
                   function = macro;
                   break;
                 }
            }

      }
   else   // maybe user defined function
      {
        NamedObject * obj = Workspace::lookup_existing_name(symbol_name);
        if (obj && obj->is_user_defined())
           {
             function = obj->get_function();
             if (function && function->get_exec_properties()[0])   function = 0;
           }
      }
   if (function == 0)   return Token(TOK_APL_VALUE1, Str0_0(LOC));

   // show the function...
   //
const UCS_string ucs = function->canonical(false);
vector<UCS_string> tlines;
   ucs.to_vector(tlines);
int max_len = 0;
   loop(row, tlines.size())
      {
        if (max_len < tlines[row].size())   max_len = tlines[row].size();
      }

Shape shape_Z;
   shape_Z.add_shape_item(tlines.size());
   shape_Z.add_shape_item(max_len);
   
Value_P Z(shape_Z, LOC);
   loop(row, tlines.size())
      {
        const UCS_string & line = tlines[row];
        loop(col, line.size())
            new (Z->next_ravel()) CharCell(line[col]);

        loop(col, max_len - line.size())
            new (Z->next_ravel()) CharCell(UNI_ASCII_SPACE);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Quad_CR::eval_AB(Value_P A, Value_P B)
{
   if (A->get_rank() > 1)                    RANK_ERROR;
   if (!A->is_scalar_or_len1_vector())       LENGTH_ERROR;
const APL_Integer a = A->get_ravel(0).get_int_value();

PrintContext pctx = Workspace::get_PrintContext(PST_NONE);

Value_P Z = do_CR(a, B.get(), pctx);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Value_P
Quad_CR::do_CR(APL_Integer a, const Value * B, PrintContext pctx)
{
   // some functions a have an inverse (which has its own number, but can
   // also be specified as -a
   //
   if (a < 0)   switch(a)
      {
        case  -5: a = 13;   break;
        case  -6: a = 13;   break;
        case -13: a =  6;   break;
        case -12: a = 11;   break;
        case -11: a = 12;   break;
        case -14: a = 15;   break;
        case -15: a = 14;   break;
        case -16: a = 17;   break;
        case -17: a = 16;   break;
        case -18: a = 19;   break;
        case -19: a = 18;   break;
        default: Workspace::more_error() = "A ⎕CR B with invalid A < 0";
                 DOMAIN_ERROR;
      }
  
bool extra_frame = false;
   switch(a)
      {
        case  4:
        case  8:
        case 23:
        case 24:
        case 25:
        case 29: extra_frame = true;

      }

   switch(a)
      {
        case  0: pctx.set_style(PR_APL);               break;
        case  1: pctx.set_style(PR_APL_FUN);           break;
        case  2: pctx.set_style(PR_BOXED_CHAR);        break;
        case  3: pctx.set_style(PR_BOXED_GRAPHIC);     break;
        case  4: pctx.set_style(PR_BOXED_GRAPHIC);     break;
        case  5: return do_CR5_6("0123456789ABCDEF", *B);   // byte-vector → HEX
        case  6: return do_CR5_6("0123456789abcdef", *B);   // byte-vector → hex
        case  7: pctx.set_style(PR_BOXED_GRAPHIC1);    break;
        case  8: pctx.set_style(PR_BOXED_GRAPHIC1);    break;
        case  9: pctx.set_style(PR_BOXED_GRAPHIC2);    break;
        case 10: return do_CR10(*B);
        case 11: return do_CR11(*B);    // Value → CDR conversion
        case 12: return do_CR12(*B);    // CDR → Value conversion
        case 13: return do_CR13(*B);    // hex → byte-vector
        case 14: return do_CR14(*B);    // Value → CDR → hex conversion
        case 15: return do_CR15(*B);    // hex → CDR → Value conversion
        case 16: return do_CR16(*B);    // byte vector → base64 (RFC 4648)
        case 17: return do_CR17(*B);    // base64 → byte vector (RFC 4648)
        case 18: return do_CR18(*B);    // UCS → UTF8 byte vector
        case 19: return do_CR19(*B);    // UTF8 byte vector → UCS string
        case 20: pctx.set_style(PR_NARS);              break;
        case 21: pctx.set_style(PR_NARS1);             break;
        case 22: pctx.set_style(PR_NARS2);             break;
        case 23: pctx.set_style(PR_NARS);              break;
        case 24: pctx.set_style(PR_NARS1);             break;
        case 25: pctx.set_style(PR_NARS2);             break;
        case 26:   return do_CR26(*B);  // Cell types
        case 27: return do_CR27_28(true,  *B);   // value as int
        case 28: return do_CR27_28(false, *B);   // value2 as int
        case 29: pctx.set_style(PR_BOXED_GRAPHIC3);    break;

        default: Workspace::more_error() = "A ⎕CR B with invalid A";
                 DOMAIN_ERROR;
      }

   // common code for ⎕CR variants that only differ by print style...
   //
   if (extra_frame)
      {
         // enclose B so that the entire B is boxed
         //
         Value_P B1(LOC);
         new (&B1->get_ravel(0)) PointerCell(B->clone(LOC), B1.getref());
         B1->check_value(LOC);
         PrintBuffer pb(*B1, pctx, 0);
         return Value_P(pb, LOC);
      }
   else   // no frame
      {
         PrintBuffer pb(*B, pctx, 0);
         return Value_P(pb, LOC);
      }
}
//-----------------------------------------------------------------------------
Value_P
Quad_CR::do_CR5_6(const char * alpha, const Value & B)
{
Shape shape_Z(B.get_shape());
   if (shape_Z.get_rank() == 0)   // scalar B
      {
        shape_Z.add_shape_item(2);
      }
   else
      {
        shape_Z.set_shape_item(B.get_rank() - 1, B.get_cols()*2);
      }

Value_P Z(shape_Z, LOC);

const Cell * cB = &B.get_ravel(0);
   loop(b, B.element_count())
       {
         const int val = cB++->get_char_value() & 0x00FF;
         const int h = alpha[val >> 4];
         const int l = alpha[val & 0x0F];
         new (Z->next_ravel())   CharCell((Unicode)h);
         new (Z->next_ravel())   CharCell((Unicode)l);
       }

   Z->set_default_Spc();
   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Quad_CR::do_CR10(const Value & B)
{
   // cannot use PrintBuffer because the lines in ucs_vec
   // have different lengths
   //
vector<UCS_string> ucs_vec;
   do_CR10(ucs_vec, B);

Value_P Z(ucs_vec.size(), LOC);
   loop(line, ucs_vec.size())
      {
         Value_P Z_line(ucs_vec[line], LOC);
         new (Z->next_ravel())   PointerCell(Z_line, Z.getref());
      }

   Z->set_default_Spc();
   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
void
Quad_CR::do_CR10(vector<UCS_string> & result, const Value & B)
{
const UCS_string symbol_name(B);
const Symbol * symbol = Workspace::lookup_existing_symbol(symbol_name);
   if (symbol == 0)   DOMAIN_ERROR;

   switch(symbol->get_nc())
      {
        case NC_VARIABLE:
             {
               const Value & value = *symbol->get_apl_value().get();
               do_CR10_var(result, symbol_name, value);
               return;
             }

        case NC_FUNCTION:
        case NC_OPERATOR:
             {
               const Function & ufun = *symbol->get_function();
               const UCS_string text = ufun.canonical(false);
               if (ufun.is_lambda())
                  {
                    UCS_string res = symbol->get_name();
                    res.append(UNI_LEFT_ARROW);
                    res.append(UNI_ASCII_L_CURLY);
                    int t = 0;
                    while (t < text.size())   // skip λ header
                       {
                         const Unicode uni = text[t++];
                         if (uni == UNI_ASCII_LF)   break;
                       }


                    while (t < text.size())   // copy body
                        {
                          const Unicode uni = text[t++];
                          if (uni == UNI_ASCII_LF)   break;
                           res.append(uni);
                        }
                    res.append(UNI_ASCII_R_CURLY);

                    result.push_back(res);
                  }
               else
                  {
                    UCS_string res("∇");
               
                    loop(u, text.size())
                       {
                         if (text[u] == '\n')
                             {
                               result.push_back(res);
                               res.clear();
                               UCS_string next(text, u+1, text.size()-(u+1));
                               if (!next.is_comment_or_label() &&
                                   u < (text.size() - 1))
                                  res.append(UNI_ASCII_SPACE);
                             }
                         else
                             {
                               res.append(text[u]);
                             }
                       }
                    res.append(ufun.get_exec_properties()[0]
                               ? UNI_DEL_TILDE : UNI_NABLA);
                    result.push_back(res);
                  }
               return;
             }

        default: DOMAIN_ERROR;
      }
}
//-----------------------------------------------------------------------------
void
Quad_CR::do_CR10_var(vector<UCS_string> & result, const UCS_string & var_name,
                     const Value & value)
{
Picker picker(var_name);
   do_CR10_rec(result, value, picker, -99);
}
//-----------------------------------------------------------------------------
void
Quad_CR::do_CR10_rec(vector<UCS_string> & result, const Value & value, 
                     Picker & picker, ShapeItem pidx)
{
   /*
      recursively encode var_name with value.
     
      we emit one of 2 formats:
     
      short format: dest ← (⍴ value) ⍴ value
              
      long format:  dest ← (⍴ value) ⍴ 0                 " prolog "
                    dest[] ← partial value ...
              
      short format (the default) requires a reasonably short value

      If value is nested then the short or long format is followed
      by recursive do_CR10_rec() calls for the sub-values.
   */

   picker.push(value.get_shape(), pidx);

UCS_string left;
   picker.get(left);

   // compute shape_rho which is "" for scalars and true vectors
   // of length > 1, "," for true vectors of length 1, and
   // "n1 n2 ... ⍴" otherwise
   //
UCS_string shape_rho;
   {
     if (value.get_rank() == 1)   // true vector
        {
          if      (value.element_count() == 0)   shape_rho.append_utf8("0⍴");
          else if (value.element_count() == 1)   shape_rho.append_utf8(",");
        }
     else if (value.get_rank() > 1)   // matrix or higher
        {
           shape_rho.append_shape(value.get_shape());
           shape_rho.append_utf8("⍴");
         }
   }

   if (value.element_count() == 0)   // empty value
      {
        UCS_string proto_name;
        picker.get(proto_name);
        picker.pop();

        Value_P proto = value.prototype(LOC);
        do_CR10_rec(result, *proto, picker, 0);
        result.back().append_utf8(" ⍝ prototype...");

        // reshape the prototype
        //
        UCS_string reshape = proto_name;
        reshape.append_utf8("←");
        reshape.append(shape_rho);
        reshape.append(proto_name);
        result.push_back(reshape);
        return;
      }

bool nested = false;

   // step 1: top-level ravel. Try short (one-line) ravel and use long
   // (multi-line ravel) if that fails.
   //
   if (short_ravel(result, nested, value, picker, left, shape_rho))
      {
        result.push_back(compute_prolog(picker.get_level(), left, value));

        ShapeItem pos = 0;
        V_mode mode = Vm_NONE;
        UCS_string rhs;
        ShapeItem count = 0;                // the number of items on this line
        ShapeItem todo = value.nz_element_count();  // the number of items to do

        loop(p, todo)
           {
             // recompute line (which changes because count changes)
             //
             UCS_string line(2*(picker.get_level() - 1), UNI_ASCII_SPACE);
             picker.get_indexed(line, pos, count);
             line.append_utf8("←");

             // compute next item
             //
             UCS_string item;
             const Cell & cell = value.get_ravel(p);
             if (cell.is_pointer_cell())   nested = true;
             const bool may_quote = use_quote(mode, value, p);
             const V_mode next_mode = do_CR10_item(item, cell, mode, may_quote);

              if (count && (rhs.size() + item.size() + line.size()) > 76)
                 {
                   // next item does not fit in this line. Close the line,
                   // append it to result, and start a new line.
                   //
                   close_mode(rhs, mode);

                   line.append(rhs);
                   pos += count;
                   count = 0;

                   result.push_back(line);
                   rhs.clear();
                   mode = Vm_NONE;
                 }

             item_separator(rhs, mode, next_mode);

             mode = next_mode;
             rhs.append(item);
             ++count;
           }

     // all items done: close mode
     //
     close_mode(rhs, mode);

     UCS_string line(2*(picker.get_level() - 1), UNI_ASCII_SPACE);
     picker.get_indexed(line, pos, count);
     line.append_utf8("←");
     line.append(rhs);

     result.push_back(line);
   }

   // step 2: nested items
   //
   if (nested)
      {
        loop(p, value.element_count())
           {
             const Cell & cell = value.get_ravel(p);
             if (!cell.is_pointer_cell())   continue;

             const Value & sub_value = *cell.get_pointer_value().get();
             do_CR10_rec(result, sub_value, picker, p);
           }
      }

   picker.pop();
}
//-----------------------------------------------------------------------------
bool
Quad_CR::short_ravel(vector<UCS_string> & result, bool & nested,
                     const Value & value, const Picker & picker,
                     const UCS_string & left, const UCS_string & shape_rho)
{
   // some quick checks beforehand to avoid wasting time attempting to
   // create short ravel
   //
   enum { MAX_SHORT_RAVEL_LEN = 70 };

const ShapeItem value_len = value.nz_element_count();
   if (value_len >= MAX_SHORT_RAVEL_LEN)   return true;  // too long
int len = 0;
   loop(v, value_len)
      {
        const Cell & cell = value.get_ravel(v);
        if (cell.is_integer_cell())   len += 2;   // min. integer (+ separator)
        else if (cell.is_character_cell())   len += 1;
        else if (cell.is_pointer_cell())     len += 1;
        else if (cell.is_complex_cell())     len += 3;
        else                                 len += 2;

        if (len >= MAX_SHORT_RAVEL_LEN)   return true;
      }

   // at this point, a short ravel MAY be possible. Try it
   //
UCS_string line(2*(picker.get_level() - 1), UNI_ASCII_SPACE);  // indent

   line.append(left);
   line.append_utf8("←");
   line.append(shape_rho);

V_mode mode = Vm_NONE;
   loop(v, value_len)
       {
         // compute next item
         //
          UCS_string item;
          const Cell & cell = value.get_ravel(v);
          if (cell.is_pointer_cell())   nested = true;
          const bool may_quote = use_quote(mode, value, v);
          const V_mode next_mode = do_CR10_item(item, cell, mode, may_quote);

          item_separator(line, mode, next_mode);

          mode = next_mode;
          line.append(item);
          if ((line.size() + item.size()) >= MAX_SHORT_RAVEL_LEN)   return true;
       }

   // all items done: close mode
   //
   close_mode(line, mode);

   result.push_back(line);
   return false;
}
//-----------------------------------------------------------------------------
UCS_string
Quad_CR::compute_prolog(int pick_level, const UCS_string & left,
                        const Value & value)
{
   // compute the prolog
   //
UCS_string prolog(2 * (pick_level - 1), UNI_ASCII_SPACE);
Unicode default_char = UNI_ASCII_SPACE;
APL_Integer default_int  = 0;
const bool default_is_int = figure_default(value, default_char, default_int);

   prolog.append(left);
   prolog.append_utf8("←");
   if (pick_level > 1)   prolog.append_utf8("⊂");
   prolog.append_shape(value.get_shape());
   if (default_is_int)   prolog.append_utf8("⍴0 ⍝ prolog ≡");
   else                  prolog.append_utf8("⍴' ' ⍝ prolog ≡");
   prolog.append_number(pick_level);

   return prolog;
}
//-----------------------------------------------------------------------------
bool
Quad_CR::figure_default(const Value & value, Unicode & default_char,
                        APL_Integer & default_int)
{
ShapeItem zeroes = 0;
ShapeItem blanks = 0;
   loop(v, value.nz_element_count())
      {
        const Cell & cell = value.get_ravel(0);
        if (cell.is_integer_cell())
           {
             if (cell.get_int_value() == 0)   ++zeroes;
           }
        else if (cell.is_character_cell())
           {
             if (cell.get_char_value() == UNI_ASCII_SPACE)   ++blanks;
           }
      }

   return zeroes >= blanks;
}
//-----------------------------------------------------------------------------
void
Quad_CR::Picker::get(UCS_string & result)
{
const int level = shapes.size();

   if (level == 1)
      {
        result.append(var_name);
        return;
      }

   result.append_utf8("((⎕IO+");
   if (level == 2 && shapes[0].get_rank() > 1)   // need ⊂ for left ⊃ arg
      {
         Shape sh = shapes[0].offset_to_index(indices[1]);
         result.append_utf8("(⊂");
         result.append_shape(sh);
         result.append_utf8(")");
      }
   else
      {
        loop(s, shapes.size() - 1)
           {
             if (s)   result.append_utf8(" ");
             Shape sh = shapes[s].offset_to_index(indices[s + 1]);
             if (sh.get_rank() > 1)   result.append_utf8("(");
             result.append_shape(sh);
		     if (sh.get_rank() > 1)   result.append_utf8(")");
           }
      }

   result.append_utf8(")⊃");
   result.append(var_name);
   result.append_utf8(")");
}
//-----------------------------------------------------------------------------
void
Quad_CR::Picker::get_indexed(UCS_string & result, ShapeItem pos, ShapeItem len)
{
   // ⊃ is not needed at top level
   //
const bool need_disclose = get_level() > 1;

   // , is only needed if the picked item is not a true vector
   //
const bool need_comma = shapes.back().get_rank() != 1;

   result.append_utf8("(");
   if (need_comma)      result.append_utf8(",");
   if (need_disclose)   result.append_utf8("⊃");
   get(result);
   result.append_utf8(")");
   result.append_utf8("[");
   if (pos)
      {
        result.append_number(pos);
        result.append_utf8("+");
      }
   result.append_utf8("⍳");
   result.append_number(len);
   result.append_utf8("]");
}
//-----------------------------------------------------------------------------
Quad_CR::V_mode
Quad_CR::do_CR10_item(UCS_string & item, const Cell & cell, V_mode mode,
                      bool may_quote)
{
   if (cell.is_character_cell())
      {
        if (may_quote)   // use ''
           {
             const Unicode uni = cell.get_char_value();
             item.append(uni);
             if (uni == UNI_SINGLE_QUOTE)   item.append(uni);
             return Vm_QUOT;
           }
        else             // use UCS()
           {
             item.append_number(cell.get_char_value());
             return Vm_UCS;
           }
      }

   if (cell.is_integer_cell())
      {
        item.append_number(cell.get_int_value());
        return Vm_NUM;
      }

   if (cell.is_float_cell())
      {
        bool scaled = false;
        PrintContext pctx(PR_APL_MIN, MAX_Quad_PP, MAX_Quad_PW);
        item = UCS_string(cell.get_real_value(), scaled, pctx);
        return Vm_NUM;
      }

   if (cell.is_complex_cell())
      {
        bool scaled = false;
        PrintContext pctx(PR_APL_MIN, MAX_Quad_PP, MAX_Quad_PW);
        item = UCS_string(cell.get_real_value(), scaled, pctx);
        item.append_utf8("J");
        scaled = false;
        item.append(UCS_string(cell.get_imag_value(), scaled, pctx));
        return Vm_NUM;
      }

   if (cell.is_pointer_cell())
      {
        // cell is a pointer cell which will be replaced later.
        // For now we add an item in the current mode as placeholder
        //
        if (mode == Vm_QUOT)   { item.append_utf8("∘");     return Vm_QUOT; }
        if (mode == Vm_UCS)    { item.append_utf8("33");    return Vm_UCS;  }
        item.append_utf8("00");
        return Vm_NUM;
      }

   DOMAIN_ERROR;
   return mode;   // not reached
}
//-----------------------------------------------------------------------------
Value_P
Quad_CR::do_CR11(const Value & B)
{
CDR_string cdr;
   CDR::to_CDR(cdr, B);

const ShapeItem len = cdr.size();
Value_P Z(len, LOC);
   loop(l, len)   new (Z->next_ravel()) CharCell((Unicode)(0xFF & cdr[l]));

   Z->set_default_Spc();
   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Quad_CR::do_CR12(const Value & B)
{
   if (B.get_rank() > 1)   RANK_ERROR;

CDR_string cdr;
   loop(b, B.element_count())   cdr.append(B.get_ravel(b).get_char_value());
Value_P Z = CDR::from_CDR(cdr, LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Quad_CR::do_CR13(const Value & B)
{
   // hex → Value conversion. 2 characters per byte in B, therefore
   // last axis of B must have even length.
   //
   if (B.get_cols() & 1)   LENGTH_ERROR;

Shape shape_Z(B.get_shape());
   shape_Z.set_shape_item(B.get_rank() - 1, (B.get_cols() + 1)/ 2);

Value_P Z(shape_Z, LOC);
const Cell * cB = &B.get_ravel(0);
   loop(z, Z->element_count())
       {
         const int n1 = nibble(cB++->get_char_value());
         const int n2 = nibble(cB++->get_char_value());
         if (n1 < 0 || n2 < 0)   DOMAIN_ERROR;
         new (Z->next_ravel()) CharCell(Unicode(16*n1 + n2));
       }

   Z->set_default_Zero();
   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Quad_CR::do_CR14(const Value & B)
{
const char * hex = "0123456789abcdef";
CDR_string cdr;
   CDR::to_CDR(cdr, B);

const ShapeItem len = cdr.size();
Value_P Z(2*len, LOC);
   loop(l, len)
       {
         const Unicode uh = Unicode(hex[0x0F & cdr[l] >> 4]);
         const Unicode ul = Unicode(hex[0x0F & cdr[l]]);
         new (Z->next_ravel()) CharCell(uh);
         new (Z->next_ravel()) CharCell(ul);
       }
   Z->set_default_Spc();
   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Quad_CR::do_CR15(const Value & B)
{
   if (B.get_rank() > 1)   RANK_ERROR;

CDR_string cdr;
const ShapeItem len = B.element_count()/2;
const Cell * cB = &B.get_ravel(0);
   loop(b, len)
       {
         const int n1 = nibble(cB++->get_char_value());
         const int n2 = nibble(cB++->get_char_value());
         if (n1 < 0 || n2 < 0)   DOMAIN_ERROR;
         cdr.append(16*n1 + n2);
       }

   Value_P Z = CDR::from_CDR(cdr, LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Quad_CR::do_CR16(const Value & B)
{
   if (B.get_rank() > 1)   RANK_ERROR;

const ShapeItem full_quantums = B.element_count() / 3;
const ShapeItem len_Z = 4 * ((B.element_count() + 2) / 3);
Value_P Z(len_Z, LOC);

const char *alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                    "abcdefghijklmnopqrstuvwxyz"
                    "0123456789+/";

const Cell * cB = &B.get_ravel(0);
   loop(b, full_quantums)   // encode full quantums
      {
        /*      -- b1 -- -- b2 -- -- b3 --
             z: 11111122 22223333 33444444
         */
        const int b1 = cB++->get_char_value() & 0x00FF;
        const int b2 = cB++->get_char_value() & 0x00FF;
        const int b3 = cB++->get_char_value() & 0x00FF;

        const int z1 = b1 >> 2;
        const int z2 = (b1 & 0x03) << 4 | (b2 & 0xF0) >> 4;
        const int z3 = (b2 & 0x0F) << 2 | (b3 & 0xC0) >> 6;
        const int z4 =  b3 & 0x3F;

        new (Z->next_ravel()) CharCell((Unicode)alpha[z1]);
        new (Z->next_ravel()) CharCell((Unicode)alpha[z2]);
        new (Z->next_ravel()) CharCell((Unicode)alpha[z3]);
        new (Z->next_ravel()) CharCell((Unicode)alpha[z4]);
      }

   // process final bytes
   //
   switch(B.element_count() - 3*full_quantums)
      {
        case 0: break;   // length of B is 3 * N

        case 1:          //  length of B is 3 * N + 1
                {
                  const int b1 = cB++->get_char_value() & 0x00FF;
                  const int b2 = 0;

                  const int z1 = b1 >> 2;
                  const int z2 = (b1 & 0x03) << 4 | (b2 & 0xF0) >> 4;

                  new (Z->next_ravel()) CharCell((Unicode)alpha[z1]);
                  new (Z->next_ravel()) CharCell((Unicode)alpha[z2]);
                  new (Z->next_ravel()) CharCell((Unicode)'=');
                  new (Z->next_ravel()) CharCell((Unicode)'=');
                }
                break;

        case 2:          // two bytes remaining
                {
                  const int b1 = cB++->get_char_value() & 0x00FF;
                  const int b2 = cB++->get_char_value() & 0x00FF;
                  const int b3 = 0;

                  const int z1 = b1 >> 2;
                  const int z2 = (b1 & 0x03) << 4 | (b2 & 0xF0) >> 4;
                  const int z3 = (b2 & 0x0F) << 2 | (b3 & 0xC0) >> 6;

                  new (Z->next_ravel()) CharCell((Unicode)alpha[z1]);
                  new (Z->next_ravel()) CharCell((Unicode)alpha[z2]);
                  new (Z->next_ravel()) CharCell((Unicode)alpha[z3]);
                  new (Z->next_ravel()) CharCell((Unicode)'=');
                }
                break;
      }

   Z->set_default_Spc();
   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Quad_CR::do_CR17(const Value & B)
{
   if (B.get_rank() != 1)   RANK_ERROR;

const int cols = B.get_cols();
   if (cols == 0)   return Str0(LOC);  // empty value
   if (cols & 3)    LENGTH_ERROR;      // length not 4*n

   // figure number of missing chars in final quantum
   //
int missing = 0;
   if      (B.get_ravel(cols - 2).get_char_value() == '=')   missing = 2;
   else if (B.get_ravel(cols - 1).get_char_value() == '=')   missing = 1;

const ShapeItem len_Z = 3 * (B.element_count() / 4) - missing;
const ShapeItem quantums = B.element_count() / 4;

Value_P Z(len_Z, LOC);
const Cell * cB = &B.get_ravel(0);
   loop(q, quantums)
       {
         const int b1 = sixbit(cB++->get_char_value());
         const int b2 = sixbit(cB++->get_char_value());
         const int q3 = sixbit(cB++->get_char_value());
         const int q4 = sixbit(cB++->get_char_value());
         const int b3 = q3 & 0x3F;
         const int b4 = q4 & 0x3F;

         /*    b1       b2       b3       b4
            --zzzzzz --zzzzzz --zzzzzz --zzzzzz
              111111   112222   223333   333333
          */
         const int z1 =  b1 << 2         | (b2 & 0x30) >> 4;
         const int z2 = (b2 & 0x0F) << 4 | b3 >> 2;
         const int z3 = (b3 & 0x03) << 6 | b4;

         if (b1 < 0 || b2 < 0 || q3  == -1 || q4 == -1)   DOMAIN_ERROR;

         if (q < (quantums - 1) || missing == 0)
            {
              new (Z->next_ravel())   CharCell((Unicode)z1);
              new (Z->next_ravel())   CharCell((Unicode)z2);
              new (Z->next_ravel())   CharCell((Unicode)z3);
            }
         else if (missing == 1)   // k6 k2l4 l20000 =
            {
              new (Z->next_ravel())   CharCell((Unicode)z1);
              new (Z->next_ravel())   CharCell((Unicode)z2);
            }
         else                     // k6 k20000 = =
            {
              new (Z->next_ravel())   CharCell((Unicode)z1);
            }
       }

   Z->set_default_Spc();
   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Quad_CR::do_CR18(const Value & B)
{
UCS_string ucs(B);
UTF8_string utf(ucs);
Value_P Z(utf, LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Quad_CR::do_CR19(const Value & B)
{
UTF8_string utf(B);
   for (ShapeItem l = 0; l < utf.size();)
       {
         int len = 0;
         const Unicode uni = UTF8_string::toUni(&utf[l], len, false);
         if (uni == Invalid_Unicode)   DOMAIN_ERROR;

         l += len;
       }

UCS_string ucs(utf);
Value_P Z(ucs, LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Quad_CR::do_CR26(const Value & B)
{
const ShapeItem len = B.element_count();
Value_P Z(len, LOC);
   loop(l, len)
      {
        const Cell & cB = B.get_ravel(l);
        if (cB.is_pointer_cell())
           {
             Value_P B_sub = cB.get_pointer_value();
             Value_P Z_sub = do_CR26(B_sub.getref());
             new (Z->next_ravel()) PointerCell(Z_sub, Z.getref());
           }
        else
           {
             new (Z->next_ravel())   IntCell(cB.get_cell_type());
           }
      }

   Z->set_default_Zero();
   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Quad_CR::do_CR27_28(bool primary, const Value & B)
{
const ShapeItem len = B.element_count();
Value_P Z(len, LOC);
   loop(l, len)
       {
         const Cell & cB = B.get_ravel(l);
         Cell * cZ = Z->next_ravel();
         if (cB.is_pointer_cell())
            {
              Value_P B_sub = cB.get_pointer_value();
              Value_P Z_sub = do_CR27_28(primary, B_sub.getref());
              new (cZ) PointerCell(Z_sub, Z.getref());
            }
         else
            {
              APL_Integer data = 0;
              if (primary)   // primary value
                 {
                   if (cB.get_cell_type() == CT_CHAR)
                      data = cB.get_char_value();
                   else if (cB.get_cell_type() == CT_CELLREF)
                      data = (APL_Integer)cB.get_lval_value();
                   else
                      memcpy(&data, cB.get_u0(), sizeof(data));
                 }
              else   // additional value
                 {
                   if (cB.get_cell_type() == CT_COMPLEX)
                      memcpy(&data, cB.get_u1(), sizeof(data));
                   else if (cB.get_cell_type() == CT_CELLREF)
                      data = (APL_Integer)(*(LvalCell *)&cB).get_cell_owner();
                 }

              new (cZ)   IntCell(data);
            }
       }

   Z->set_default_Zero();
   Z->check_value(LOC);
   return Z;
}
//-----------------------------------------------------------------------------
bool
Quad_CR::use_quote(V_mode mode, const Value & value, ShapeItem pos)
{
int char_len = 0;
int ascii_len = 0;

   for (; pos < value.element_count(); ++pos)
       {
         // if we are in '' mode then a single ASCII char
         // suffices to remain in that mode
         //
         if (ascii_len > 0 && mode == Vm_QUOT) return true;

         // if we are in a non-'' mode then 3 ASCII chars
         // suffice to enter '' mode
         //
         if (ascii_len >= 3)   return true;

         if (!value.get_ravel(pos).is_character_cell())   break;
         ++char_len;
         const Unicode uni = value.get_ravel(pos).get_char_value();
         if (uni >= ' ' && uni <= 0x7E)   ++ascii_len;
         else                             break;
       }

   // if all chars are ASCII then use '' mode
   //
   return (char_len == ascii_len);
}
//-----------------------------------------------------------------------------
void
Quad_CR::close_mode(UCS_string & rhs, V_mode mode)
{
   if      (mode == Vm_QUOT)   rhs.append_utf8("'");
   else if (mode == Vm_UCS)    rhs.append_utf8(")");
}
//-----------------------------------------------------------------------------
void
Quad_CR::item_separator(UCS_string & line, V_mode from_mode, V_mode to_mode)
{
   if (from_mode == to_mode)   // separator in same mode (if any)
      {
        if      (to_mode == Vm_UCS)    line.append_utf8(" ");
        else if (to_mode == Vm_NUM)    line.append_utf8(" ");
      }
   else                // close old mode and open new one
      {
        close_mode(line, from_mode);
        if (from_mode != Vm_NONE)   line.append_utf8(",");

        if      (to_mode == Vm_UCS)    line.append_utf8("(,⎕UCS ");
        else if (to_mode == Vm_QUOT)   line.append_utf8("'");
      }
}
//-----------------------------------------------------------------------------

