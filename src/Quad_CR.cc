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
NamedObject * obj = Workspace::lookup_existing_name(symbol_name);
   if (obj == 0)                  return Token(TOK_APL_VALUE1, Str0_0(LOC));
   if (!obj->is_user_defined())   return Token(TOK_APL_VALUE1, Str0_0(LOC));

const Function * function = obj->get_function();
   if (function == 0)   return Token(TOK_APL_VALUE1, Str0_0(LOC));

   if (function->get_exec_properties()[0] != 0)
      return Token(TOK_APL_VALUE1, Str0_0(LOC));

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
   if (!A->get_ravel(0).is_integer_cell())   DOMAIN_ERROR;

const PrintContext pctx = Workspace::get_PrintContext(PST_NONE);
Value_P Z = do_CR(A->get_ravel(0).get_int_value(), *B.get(), pctx);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Value_P
Quad_CR::do_CR(APL_Integer a, const Value & B, PrintContext pctx)
{
const char * alpha = "0123456789abcdef";   // alphabet for hex and base64
   switch(a)
      {
        case  0: pctx.set_style(PR_APL);               break;
        case  1: pctx.set_style(PR_APL_FUN);           break;

        case  2: pctx.set_style(PR_BOXED_CHAR);        break;
        case  3: pctx.set_style(PR_BOXED_GRAPHIC);     break;
        case  7: pctx.set_style(PR_BOXED_GRAPHIC1);    break;
        case  9: pctx.set_style(PR_BOXED_GRAPHIC2);    break;

        case 20:  pctx.set_style(PR_NARS);             break;
        case 21:  pctx.set_style(PR_NARS1);            break;
        case 22:  pctx.set_style(PR_NARS2);            break;

        case  4:  a =  3;   goto case_4_8_23_24_25;
        case  8:  a =  7;   goto case_4_8_23_24_25;
        case  23: a = 20;   goto case_4_8_23_24_25;
        case  24: a = 21;   goto case_4_8_23_24_25;
        case  25: a = 22;   goto case_4_8_23_24_25;
        case_4_8_23_24_25:
                { // like 3/7, but enclose B so that the entire B is boxed
                   Value_P B1(LOC);
                   new (&B1->get_ravel(0))
                       PointerCell(B.clone(LOC), B1.getref());
                   B1->check_value(LOC);
                   Value_P Z = do_CR(a, *B1.get(), pctx);
                   Z->check_value(LOC);
                   return Z;
                 }
                 break;

        case  5: alpha = "0123456789ABCDEF";
        case -13:
        case  6:
             {
               // byte-vector → hex
               //
               Shape shape_Z(B.get_shape());
               if (shape_Z.get_rank() == 0)   shape_Z.add_shape_item(1);
               shape_Z.set_shape_item(B.get_rank() - 1, B.get_cols() * 2);
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

        case 10: {
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

        case -12:
        case 11: // Value → CDR conversion
                 //
                 {
                   CDR_string cdr;
                   CDR::to_CDR(cdr, B);
                   const ShapeItem len = cdr.size();
                   Value_P Z(len, LOC);
                   loop(l, len)
                       new (Z->next_ravel()) CharCell((Unicode)(0xFF & cdr[l]));
                   Z->set_default_Spc();
                   Z->check_value(LOC);
                   return Z;
                 }

        case -11:
        case 12: // CDR → Value conversion
                 //
                 {
                   if (B.get_rank() > 1)   RANK_ERROR;

                   CDR_string cdr;
                   loop(b, B.element_count())
                       cdr.append(B.get_ravel(b).get_char_value());
                   Value_P Z = CDR::from_CDR(cdr, LOC);
                   return Z;
                 }

        case -5:
        case -6:
        case 13: // hex → Value conversion. 2 characters per byte, therefore
                 // last axis of B must have even length.
                 //
                 {
                   if (B.get_cols() & 1)   LENGTH_ERROR;

                   Shape shape_Z(B.get_shape());
                   shape_Z.set_shape_item(B.get_rank() - 1,
                                         (B.get_cols() + 1)/ 2);
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

        case -15:
        case 14: // Value → CDR → hex conversion
                 //
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

        case -14:
        case 15: // hex → CDR → Value conversion
                 //
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

        case -17:
        case 16: // byte vector → base64 (RFC 4648)
             {
               if (B.get_rank() > 1)   RANK_ERROR;
               const ShapeItem full_quantums = B.element_count() / 3;
               const ShapeItem len_Z = 4 * ((B.element_count() + 2) / 3);
               Value_P Z(len_Z, LOC);
               alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
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

        case -16:
        case 17: // base64 → byte vector (RFC 4648)
             {
               if (B.get_rank() != 1)   RANK_ERROR;
               if (B.get_cols() & 3)    LENGTH_ERROR;          // length not 4*n
               if (B.get_cols() == 0)   return Str0(LOC);  // empty value

               // figure number of missing chars in final quantum
               //
               int missing;
               if (B.get_ravel(B.get_cols() - 2).get_char_value() == '=')
                  missing = 2;
               else if (B.get_ravel(B.get_cols() - 1).get_char_value() == '=')
                  missing = 1;
               else
                  missing = 0;

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

                     //    b1       b2       b3       b4
                     // --zzzzzz --zzzzzz --zzzzzz --zzzzzz
                     //   111111   112222   223333   333333
                     //
                     const int z1 =  b1 << 2         | (b2 & 0x30) >> 4;
                     const int z2 = (b2 & 0x0F) << 4 | b3 >> 2;
                     const int z3 = (b3 & 0x03) << 6 | b4;

                     if (b1 < 0 || b2 < 0 || q3  == -1 || q4 == -1)
                        DOMAIN_ERROR;

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

        case -19:
        case 18: // UCS → UTF8 byte vector
             {
               UCS_string ucs(B);
               UTF8_string utf(ucs);
               Value_P Z(utf, LOC);
               return Z;
             }

        case -18:
        case 19: // UTF8 byte vector → UCS string
             {
               UTF8_string utf(B);
               UCS_string ucs(utf);
               Value_P Z(ucs, LOC);
               return Z;
             }

        case 26:   // Cell types
             {
               const ShapeItem len = B.element_count();
               Value_P Z(len, LOC);
               loop(l, len)
                   {
                     const Cell & cB = B.get_ravel(l);
                     Cell * cZ = Z->next_ravel_nz();
                     if (cB.is_pointer_cell())
                        {
                          Value_P B_sub = cB.get_pointer_value();
                          Value_P Z_sub = do_CR(a, *B_sub.get(), pctx);
                          new (cZ) PointerCell(Z_sub, Z.getref());
                        }
                     else
                        {
                          new (cZ)   IntCell(cB.get_cell_type());
                        }
                   }
               return Z;
             }
          
        case 27:   // value as int
        case 28:   // value2 as int
             {
               const ShapeItem len = B.element_count();
               Value_P Z(len, LOC);
               loop(l, len)
                   {
                     const Cell & cB = B.get_ravel(l);
                     Cell * cZ = Z->next_ravel_nz();
                     if (cB.is_pointer_cell())
                        {
                          Value_P B_sub = cB.get_pointer_value();
                          Value_P Z_sub = do_CR(a, *B_sub.get(), pctx);
                          new (cZ) PointerCell(Z_sub, Z.getref());
                        }
                     else
                        {
                          APL_Integer data = 0;
                          if (a == 27)   // primary value
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
                                  data = (APL_Integer)
                                         (*(LvalCell *)&cB).get_cell_owner();
                             }
                          new (cZ)   IntCell(data);
                        }
                   }
               return Z;
             }
        default: DOMAIN_ERROR;
      }

   // common code for ⎕CR variants that only differs by print style...
   //
PrintBuffer pb(B, pctx, 0);
   Assert(pb.is_rectangular());

const ShapeItem height = pb.get_height();
const ShapeItem width = pb.get_width(0);

const Shape sh(height, width);
Value_P Z(sh, LOC);

   // prototype
   //
   Z->get_ravel(0).init(CharCell(UNI_ASCII_SPACE), Z.getref(), LOC);

   loop(y, height)
   loop(x, width)
       Z->get_ravel(x + y*width).init(CharCell(pb.get_char(x, y)),
                                      Z.getref(), LOC);

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
               UCS_string res("∇");
               
               loop(u, text.size())
                  {
                     if (text[u] == '\n')
                        {
                          result.push_back(res);
                          res.clear();
                          UCS_string next(text, u + 1, text.size() - u - 1);
                          if (!next.is_comment_or_label() &&
                              u < (text.size() - 1))
                             res.append(UNI_ASCII_SPACE);
                        }
                     else
                        {
                          res.append(text[u]);
                        }
                  }
               if (ufun.get_exec_properties()[0])   res.append(UNI_DEL_TILDE);
               else                                 res.append(UNI_NABLA);
               result.push_back(res);
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

bool short_format = true;   // if false then prolog has been emitted.
bool nested = false;

   // step 1: top-level ravel
   //
   {
     ShapeItem pos = 0;
     V_mode mode = Vm_NONE;
     int max_len = 72 - left.size();
     if (max_len < 40)   max_len = 40;
     UCS_string line;
     ShapeItem count = 0;                   // the number of items on this line
     ShapeItem todo = value.nz_element_count();   // the number of items to do

     loop(p, todo)
        {
          UCS_string lhs(2*(picker.get_level() - 1), UNI_ASCII_SPACE);
          picker.get_indexed(lhs, pos, count);
          lhs.append_utf8("←");

          // compute next item
          //
          UCS_string item;
          const Cell & cell = value.get_ravel(p);
          if (cell.is_pointer_cell())   nested = true;
          const bool may_quote = use_quote(mode, value, p);
          const V_mode next_mode = do_CR10_item(item, cell, mode, may_quote);

           if (p && (line.size() + item.size()) > max_len)
              {
                // next item does not fit in this line. Close the line, append
                // it to result, and start a new line.
                //
                close_mode(line, mode);

                if (short_format)
                   {
                     const UCS_string prolog =
                           compute_prolog(picker.get_level(), left, value);
                     result.push_back(prolog);
                     short_format = false;
                   }

                lhs.append(line);
                pos += count;
                count = 0;

                result.push_back(lhs);
                line.clear();
                mode = Vm_NONE;
              }

          item_separator(line, mode, next_mode);

          mode = next_mode;
          line.append(item);
          ++count;
        }

     // all items done: close mode
     //
     if (mode == Vm_QUOT)       line.append_utf8("'");
     else if (mode == Vm_UCS)   line.append_utf8(")");

     if (short_format)
        {
          UCS_string lhs(2*(picker.get_level() - 1), UNI_ASCII_SPACE);
          lhs.append(left);
          lhs.append_utf8("←");
          lhs.append(shape_rho);
          lhs.append(line);

          result.push_back(lhs);
        }
     else
        {
          UCS_string lhs(2*(picker.get_level() - 1), UNI_ASCII_SPACE);
          picker.get_indexed(lhs, pos, count);
          lhs.append_utf8("←");
          lhs.append(line);

          result.push_back(lhs);
        }
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

   {
     prolog.append(left);
     prolog.append_utf8("←");
     if (pick_level > 1)   prolog.append_utf8("⊂");
     prolog.append_shape(value.get_shape());
     if (default_is_int)   prolog.append_utf8("⍴0 ⍝ prolog ≡");
     else                  prolog.append_utf8("⍴' ' ⍝ prolog ≡");
     prolog.append_number(pick_level);
   }

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
        // for now we add an item in the current mode as
        // placeholder
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
Quad_CR::close_mode(UCS_string & line, V_mode mode)
{
   if      (mode == Vm_QUOT)   line.append_utf8("'");
   else if (mode == Vm_UCS)    line.append_utf8(")");
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

