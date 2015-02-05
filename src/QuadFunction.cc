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

#include <unistd.h>

#include "ArrayIterator.hh"
#include "Avec.hh"
#include "Bif_F12_FORMAT.hh"
#include "CDR.hh"
#include "CharCell.hh"
#include "ComplexCell.hh"
#include "FloatCell.hh"
#include "IndexIterator.hh"
#include "IntCell.hh"
#include "LineInput.hh"
#include "Output.hh"
#include "PointerCell.hh"
#include "PrintOperator.hh"
#include "QuadFunction.hh"
#include "Quad_FX.hh"
#include "Quad_TF.hh"
#include "Tokenizer.hh"
#include "UserFunction.hh"
#include "Value.icc"
#include "Workspace.hh"

extern char **environ;

Quad_AF    Quad_AF::fun;
Quad_AT    Quad_AT::fun;
Quad_CR    Quad_CR::fun;
Quad_DL    Quad_DL::fun;
Quad_EA    Quad_EA::fun;
Quad_EC    Quad_EC::fun;
Quad_ENV   Quad_ENV::fun;
Quad_ES    Quad_ES::fun;
Quad_EX    Quad_EX::fun;
Quad_INP   Quad_INP::fun;
Quad_NA    Quad_NA::fun;
Quad_NC    Quad_NC::fun;
Quad_NL    Quad_NL::fun;
Quad_SI    Quad_SI::fun;
Quad_UCS   Quad_UCS::fun;
Quad_STOP  Quad_STOP::fun;            // S∆
Quad_TRACE Quad_TRACE::fun;           // T∆

//=============================================================================
Token
Quad_AF::eval_B(Value_P B)
{
const ShapeItem ec = B->element_count();
Value_P Z(new Value(B->get_shape(), LOC));

   loop(v, ec)
       {
         const Cell & cell_B = B->get_ravel(v);

         if (cell_B.is_character_cell())   // Unicode to AV index
            {
              const Unicode uni = cell_B.get_char_value();
              int32_t pos = Avec::find_av_pos(uni);
              if (pos < 0)   new (&Z->get_ravel(v)) IntCell(MAX_AV);
              else           new (&Z->get_ravel(v)) IntCell(pos);
              continue;
            }

         if (cell_B.is_integer_cell())
            {
              const APL_Integer idx = cell_B.get_int_value();
              new (&Z->get_ravel(v))   CharCell(Quad_AV::indexed_at(idx));
              continue;
            }

         DOMAIN_ERROR;
       }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_AT::eval_AB(Value_P A, Value_P B)
{
   // A should be an integer scalar 1, 2, 3, or 4
   //
   if (A->get_rank() > 0)   RANK_ERROR;
const APL_Integer mode = A->get_ravel(0).get_int_value();
   if (mode < 1)   DOMAIN_ERROR;
   if (mode > 4)   DOMAIN_ERROR;

const ShapeItem cols = B->get_cols();
const ShapeItem rows = B->get_rows();
   if (rows == 0)   LENGTH_ERROR;

const int mode_vec[] = { 3, 7, 4, 2 };
const int mode_len = mode_vec[mode - 1];
Shape shape_Z(rows);
   shape_Z.add_shape_item(mode_len);

Value_P Z(new Value(shape_Z, LOC));

   loop(r, rows)
      {
        // get the symbol name by stripping trailing spaces
        //
        const ShapeItem b = r*cols;   // start of the symbol name
        UCS_string symbol_name;
        loop(c, cols)
           {
            const Unicode uni = B->get_ravel(b + c).get_char_value();
            if (uni == UNI_ASCII_SPACE)   break;
            symbol_name.append(uni);
           }

        NamedObject * obj = Workspace::lookup_existing_name(symbol_name);
        if (obj == 0)   throw_symbol_error(symbol_name, LOC);

        const Function * function = obj->get_function();
        if (function)             // user defined or system function.
           {
             function->get_attributes(mode, &Z->get_ravel(r*mode_len));
             continue;
           }

        Symbol * symbol = obj->get_symbol();
        if (symbol)               // user defined or system var.
           {
             symbol->get_attributes(mode, &Z->get_ravel(r*mode_len));
             continue;
           }

        // neither function nor variable (e.g. unused name)
        VALUE_ERROR;

        if (Avec::is_quad(symbol_name[0]))   // system function or variable
           {
             int l;
             const Token t(Workspace::get_quad(symbol_name, l));
             if (t.get_Class() == TC_SYMBOL)   // system variable
                {
                  // Assert() if symbol_name is not a function
                  t.get_sym_ptr();
                }
             else
                {
                  // throws SYNTAX_ERROR if t is not a function
                  t.get_function();
                }
           }
        else                                   // user defined
           {
             Symbol * symbol = Workspace::lookup_existing_symbol(symbol_name);
             if (symbol == 0)   VALUE_ERROR;

             symbol->get_attributes(mode, &Z->get_ravel(r*mode_len));
           }
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
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
   
Value_P Z(new Value(shape_Z, LOC));
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
        case  0:  pctx.set_style(PR_APL);              break;
        case  1:  pctx.set_style(PR_APL_FUN);          break;
        case  2:  pctx.set_style(PR_BOXED_CHAR);       break;
        case  3:  pctx.set_style(PR_BOXED_GRAPHIC);    break;

        case  4:
        case  8: { // like 3/7, but enclose B so that the entire B is boxed
                   Value_P B1(new Value(LOC));
                   new (&B1->get_ravel(0))
                       PointerCell(B.clone(LOC), B1.getref());
                   Value_P Z = do_CR(a - 1, *B1.get(), pctx);
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
               Value_P Z(new Value(shape_Z, LOC));

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

        case  7: pctx.set_style(PR_BOXED_GRAPHIC1);   break;
        case  9: pctx.set_style(PR_BOXED_GRAPHIC2);   break;

        case 10: {
                   vector<UCS_string> ucs_vec;
                   do_CR10(ucs_vec, B);

                   Value_P Z(new Value(ucs_vec.size(), LOC));
                   loop(line, ucs_vec.size())
                      {
                        Value_P Z_line(new Value(ucs_vec[line], LOC));
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
                   Value_P Z(new Value(len, LOC));
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
                   Value_P Z(new Value(shape_Z, LOC));
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
                   Value_P Z(new Value(2*len, LOC));
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
               Value_P Z(new Value(len_Z, LOC));
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

               Value_P Z(new Value(len_Z, LOC));
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
               Value_P Z(new Value(utf, LOC));
               return Z;
             }

        case -18:
        case 19: // UTF8 byte vector → UCS string
             {
               UTF8_string utf(B);
               UCS_string ucs(utf);
               Value_P Z(new Value(ucs, LOC));
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
Value_P Z(new Value(sh, LOC));

   // prototype
   //
   Z->get_ravel(0).init(CharCell(UNI_ASCII_SPACE), Z.getref());

   loop(y, height)
   loop(x, width)
       Z->get_ravel(x + y*width).init(CharCell(pb.get_char(x, y)), Z.getref());

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
               const UCS_string pick;
               do_CR10_var(result, symbol_name, pick, value);
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
                 const UCS_string & pick, const Value & value)
{
   // recursively encode var_name with value.
   //
   // we emit one of 2 formats:
   //
   // short format: var_name ← (⍴ value) ⍴ value             " shortlog "
   //         
   // long format:  var_name ← (⍴ value) ⍴ value             " prolog "
   //               var_name[] ← partial value
   //               var_name ← (⍴ var_name) ⍴ var_name       " epilog "
   //         
   // short format (the default) requires a short and not nested value
   //
bool short_format = true;   // if false then prolog has been mitted.
bool nested = false;

UCS_string name(var_name);
   if (pick.size())
      {
         name.clear();
         name.append_utf8("((⎕IO+");
         name.append(pick);
         name.append_utf8(")⊃");
         name.append(var_name);
         name.append_utf8(")");
      }

   // compute the prolog
   //
UCS_string prolog(name);
   {
     prolog.append_utf8("←");
     if (pick.size())   prolog.append_utf8("⊂");
     prolog.append_number(value.element_count());
     prolog.append_utf8("⍴0");
   }

UCS_string shape_rho;
   {
     if (!value.is_scalar())
        {
          loop(r, value.get_rank())
              {
                if (r)   shape_rho.append_utf8(" ");
                shape_rho.append_number(value.get_shape_item(r));
              }

           shape_rho.append_utf8("⍴");
         }
   }

UCS_string epilog(name);
   {
     epilog.append_utf8("←");
     epilog.append(shape_rho);
     epilog.append(name);
   }

   enum V_mode
      {
        Vm_NONE,
        Vm_NUM,
        Vm_QUOT,
        Vm_UCS
      };

   if (value.element_count() == 0)   // empty value
      {
        Value_P proto = value.prototype(LOC);
        do_CR10_var(result, var_name, pick, *proto);
        UCS_string reshape(name);
        reshape.append_utf8("←");
        reshape.append(shape_rho);
        reshape.append(name);

        result.push_back(reshape);
        return;
      }

   // step 1: plain numbers and characters
   //
   {
     ShapeItem pos = 0;
     V_mode mode = Vm_NONE;
     int max_len = 72 - pick.size();
     if (max_len < 40)   max_len = 40;
     UCS_string line;
     int count = 0;
     loop(p, value.element_count())
        {
          UCS_string pref = name;
          pref.append_utf8("[");
          if (pos)
             {
               pref.append_number(pos);
               pref.append_utf8("+");
             }
          pref.append_utf8("⍳");
          pref.append_number(count);
          pref.append_utf8("]←");

          // compute next item
          //
          UCS_string next;
          V_mode next_mode;
          const Cell & cell = value.get_ravel(p);
          if (cell.is_character_cell())
             {
               // see if we should use '' or ⎕UCS
               //
               bool may_quote = false;
               {
                int char_len = 0;
                int ascii_len = 0;
                for (ShapeItem pp = p; pp < value.element_count(); ++pp)
                    {
                      // if we are in '' mode then a single ASCII char
                      // suffices to remain in that mode
                      //
                      if (ascii_len > 0 && mode == Vm_QUOT)
                         {
                           may_quote = true;
                           break;
                         }

                      // if we are in a non-'' mode then 3 ASCII chars
                      // suffice to enter '' mode
                      //
                      if (ascii_len >= 3)
                         {
                           may_quote = true;
                           break;
                         }

                      if (value.get_ravel(pp).is_character_cell())
                         {
                           ++char_len;
                           const Unicode uni =
                                         value.get_ravel(pp).get_char_value();
                           if (uni >= ' ' && uni <= 0x7E)   ++ascii_len;
                           else                             break;
                         }
                      else                                  break;
                    }

                 // if all chars are ASCII then use '' mode
                 //
                 if (char_len == ascii_len)   may_quote = true;
               }

               if (may_quote)
                   {
                     next_mode = Vm_QUOT;
                     const Unicode uni = cell.get_char_value();
                     next.append(uni);
                     if (uni == UNI_SINGLE_QUOTE)   next.append(uni);
                   }
                else
                   {
                     next_mode = Vm_UCS;
                     next.append_number(cell.get_char_value());
                   }
             }
          else if (cell.is_integer_cell())
             {
               next_mode = Vm_NUM;
               next.append_number(cell.get_int_value());
             }
          else if (cell.is_float_cell())
             {
               next_mode = Vm_NUM;
               bool scaled = false;
               PrintContext pctx(PR_APL_MIN, MAX_Quad_PP, MAX_Quad_PW);
               next = UCS_string(cell.get_real_value(), scaled, pctx);
             }
          else if (cell.is_complex_cell())
             {
               next_mode = Vm_NUM;
               bool scaled = false;
               PrintContext pctx(PR_APL_MIN, MAX_Quad_PP, MAX_Quad_PW);
               next = UCS_string(cell.get_real_value(), scaled, pctx);
               next.append_utf8("J");
               scaled = false;
               next.append(UCS_string(cell.get_imag_value(), scaled, pctx));
             }
          else if (cell.is_pointer_cell())
             {
               // cell is a pointer cell which will be replaced later.
               // for now we add an item in the current mode as
               // placeholder
               //
               if (mode == Vm_NONE)
                  {
                    mode = Vm_QUOT;
                    next.append_utf8("'");
                  }
               next_mode = mode;
               if      (mode == Vm_NUM)    next.append_utf8(" 0");
               else if (mode == Vm_QUOT)   next.append_utf8("∘");
               else if (mode == Vm_UCS)    next.append_utf8(" 33");

               nested = true;
               if (short_format)
                  {
                    result.push_back(prolog);
                    short_format = false;
                  }
             }
          else DOMAIN_ERROR;

           if (p && (pref.size() + line.size() + next.size()) > max_len)
              {
                // next item does not fit in this line.
                //
                 if (mode == Vm_QUOT)       line.append_utf8("'");
                 else if (mode == Vm_UCS)   line.append_utf8(")");

                  if (short_format)
                     {
                       result.push_back(prolog);
                       short_format = false;
                     }

                 pref.append(line);
                 pos += count;
                 count = 0;

                 result.push_back(pref);
                 line.clear();
                 mode = Vm_NONE;
              }

          const bool mode_changed = (mode != next_mode);
          if (mode_changed)   // close old mode
             {
               if      (mode == Vm_QUOT)   line.append_utf8("'");
               else if (mode == Vm_UCS)    line.append_utf8(")");
             }

          if (mode_changed)   // open new mode
             {
               if (mode != Vm_NONE)   line.append_utf8(",");

               if      (next_mode == Vm_UCS)    line.append_utf8("(,⎕UCS ");
               else if (next_mode == Vm_QUOT)   line.append_utf8("'");
               else if (next_mode == Vm_NUM)    line.append_utf8("");
             }
          else                // separator in same mode
             {
               if      (next_mode == Vm_QUOT)   ;
               else if (next_mode == Vm_UCS)    line.append_utf8(" ");
               else if (next_mode == Vm_NUM)    line.append_utf8(",");
             }

          mode = next_mode;
          line.append(next);
          ++count;
          next.clear();
        }

     // all items done: close mode
     //
     if (mode == Vm_QUOT)       line.append_utf8("'");
     else if (mode == Vm_UCS)   line.append_utf8(")");

     if (short_format)
        {
          UCS_string pref = name;
          pref.append_utf8("←");
          pref.append(shape_rho);
          pref.append(line);

          result.push_back(pref);
        }
     else
        {
          UCS_string pref = name;
          pref.append_utf8("[");
          pref.append_number(pos);
          pref.append_utf8("+⍳");
          pref.append_number(count);
          pref.append_utf8("]←");
          pref.append(line);

          result.push_back(pref);
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

             UCS_string sub_pick = pick;
             if (pick.size())   sub_pick.append_utf8(" ");
             sub_pick.append_number(p);

             Value_P sub_value = cell.get_pointer_value();
             do_CR10_var(result, var_name, sub_pick, *sub_value.get());
           }
      }

   // step 3: reshape to final result
   //
   if (!short_format)   // may need epilog
      {
        // if value is a scalar or a vector with != 1 elements then
        // epilog is not needed.
        //
        if (value.get_rank() > 1 ||
            (value.is_vector() && value.element_count() == 1))

            result.push_back(epilog);
      }
}
//=============================================================================
Token
Quad_DL::eval_B(Value_P B)
{
const APL_time_us start = now();

   // B should be an integer or real scalar
   //
   if (B->get_rank() > 0)                 RANK_ERROR;
   if (!B->get_ravel(0).is_real_cell())   DOMAIN_ERROR;

const APL_time_us end = start + 1000000 * B->get_ravel(0).get_real_value();
   if (end < start)                            DOMAIN_ERROR;
   if (end > start + 31*24*60*60*1000000ULL)   DOMAIN_ERROR;   // > 1 month

   for (;;)
       {
         // compute time remaining.
         //
         const APL_time_us remaining_us =  end - now();
         if (remaining_us <= 0)   break;

         const int wait_sec  = remaining_us/1000000;
         const int wait_usec = remaining_us%1000000;
         timeval tv = { wait_sec, wait_usec };
         if (select(0, 0, 0, 0, &tv) == 0)   break;
       }

   // return time elapsed.
   //
Value_P Z(new Value(LOC));
   new (&Z->get_ravel(0)) FloatCell(0.000001*(now() - start));

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_EA::eval_AB(Value_P A, Value_P B)
{
ExecuteList * fun = 0;
const UCS_string statement_B(*B.get());

   try
      {
        fun = ExecuteList::fix(statement_B, false, LOC);
      }
   catch (...)
      {
      }

   if (fun == 0)   // ExecuteList::fix() failed
      {
        // syntax error in B: try to fix A. This will reset ⎕EM and ⎕ET even
        // though we whould keep them (lrm p. 178). Therefore we save the error
        // info from fixing B
        //
        const Error error_B = *Workspace::get_error();
        try
           {
             const UCS_string statement_A(*A.get());
             fun = ExecuteList::fix(statement_A, false, LOC);
           }
       catch (...)
           {
           }

        if (fun == 0)   SYNTAX_ERROR;   // syntax error in A and B: give up.

        // A could be fixed: execute it.
        //
        Log(LOG_UserFunction__execute)   fun->print(CERR);

        Workspace::push_SI(fun, LOC);
        *Workspace::get_error() = error_B;
        Workspace::SI_top()->set_safe_execution(true);

        // install end of context handler. The handler will do nothing when
        // B succeeds, but will execute A if not.
        //
        StateIndicator * si = Workspace::SI_top();

        si->set_eoc_handler(eoc_A_and_B_done);
        si->get_eoc_arg().A = A;
        si->get_eoc_arg().B = B;

        return Token(TOK_SI_PUSHED);
      }

   Assert(fun);

   Log(LOG_UserFunction__execute)   fun->print(CERR);

   Workspace::push_SI(fun, LOC);
   Workspace::SI_top()->set_safe_execution(true);

   // install end of context handler. The handler will do nothing when
   // B succeeds, but will execute A if not.
   //
   {
     Value_P dummy_Z;
     EOC_arg arg(dummy_Z, B, A);

     Workspace::SI_top()->set_eoc_handler(eoc_B_done);
     Workspace::SI_top()->get_eoc_arg() = arg;
   }

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
bool
Quad_EA::eoc_B_done(Token & token, EOC_arg & arg)
{
StateIndicator * si = Workspace::SI_top();

   // in A ⎕EA B, ⍎B was executed and may or may not have failed.
   //
   if (token.get_tag() != TOK_ERROR)   // ⍎B succeeded
      {
        si->set_safe_execution(false);
        // do not clear ⎕EM and ⎕ET; they should remain visible.
        return false;   // ⍎B successful.
      }

   // in A ⎕EA B, ⍎B has failed...
   //
   // lrm p. 178: "⎕EM and ⎕ET are set, execution of B is abandoned without
   // an error message, and the expression represented by A is executed."
   //
Value_P A = arg.A;
Value_P B = arg.B;
const UCS_string statement_A(*A.get());

ExecuteList * fun = 0;
   try
      {
         fun = ExecuteList::fix(statement_A, false, LOC);
      }
   catch (...)
      {
      }
        
   // give up if if syntax error in A (and B has failed before)
   //
   if (fun == 0)   SYNTAX_ERROR;

   // A could be ⎕FXed: execute it.
   //
   Assert(fun);

   Log(LOG_UserFunction__execute)   fun->print(CERR);

   Workspace::pop_SI(LOC);
   Workspace::push_SI(fun, LOC);
   Workspace::SI_top()->get_error().init((ErrorCode)(token.get_int_val()), LOC);

   Workspace::SI_top()->set_safe_execution(true);

   // install end of context handler for the result of ⍎A. 
   //
   {
     Value_P dummy_Z;
     EOC_arg arg(dummy_Z, B, A);
     StateIndicator * si1 = Workspace::SI_top();
     si1->set_eoc_handler(eoc_A_and_B_done);
     si1->get_eoc_arg() = arg;
   }

   return true;
}
//-----------------------------------------------------------------------------
bool
Quad_EA::eoc_A_and_B_done(Token & token, EOC_arg & arg)
{
   // in A ⎕EA B, ⍎B has failed, and ⍎A was executed and may or
   // may not have failed.
   //
StateIndicator * si = Workspace::SI_top();
   Assert(si);
   si->set_safe_execution(false);

   if (token.get_tag() != TOK_ERROR)   // ⍎A succeeded
      {
        StateIndicator * si1 = si->get_parent();
        si1->get_error() = si->get_error();
        return false;   // ⍎A successful.
      }

Value_P A = arg.A;
Value_P B = arg.B;
const UCS_string statement_A(*A.get());

   // here both ⍎B and ⍎A failed. ⎕EM shows only B, but should show
   // A ⎕EA B instead.
   //

   // display of errors was disabled since ⎕EM was incorrect.
   // now we have fixed ⎕EM and can display it.
   //
   si->get_error().print_em(UERR, LOC);
   si->get_error().clear_error_line_1();

UCS_string ucs_2(6, UNI_ASCII_SPACE);
int left_caret = 6;
int right_caret;

const UCS_string statement_B(*B.get());

   right_caret = left_caret + statement_A.size() + 3;

   ucs_2.append(UNI_SINGLE_QUOTE);
   ucs_2.append(statement_A);
   ucs_2.append(UNI_SINGLE_QUOTE);

   ucs_2.append(UNI_ASCII_SPACE);

   ucs_2.append(UNI_Quad_Quad);
   ucs_2.append(UNI_ASCII_E);
   ucs_2.append(UNI_ASCII_A);

   ucs_2.append(UNI_ASCII_SPACE);

   ucs_2.append(UNI_SINGLE_QUOTE);
   ucs_2.append(statement_B);
   ucs_2.append(UNI_SINGLE_QUOTE);

   si->get_error().set_error_line_2(ucs_2, left_caret, right_caret);

   UERR << si->get_error().get_error_line_2() << endl
        << si->get_error().get_error_line_3() << endl;

   return false;
}
//=============================================================================
Token
Quad_EC::eval_B(Value_P B)
{
const UCS_string statement_B(*B.get());

ExecuteList * fun = 0;

   try
      {
        fun = ExecuteList::fix(statement_B, false, LOC);
      }
   catch (...)
      {
      }

   if (fun == 0)
      {
        // syntax error in B
        //
        Value_P Z(new Value(3, LOC));
        Value_P Z1(new Value(2, LOC));
        Value_P Z2(new Value(Error::error_name(E_SYNTAX_ERROR), LOC));
        new (&Z1->get_ravel(0))   IntCell(Error::error_major(E_SYNTAX_ERROR));
        new (&Z1->get_ravel(1))   IntCell(Error::error_minor(E_SYNTAX_ERROR));

        new (&Z->get_ravel(0)) IntCell(0);        // return code = error
        new (&Z->get_ravel(1)) PointerCell(Z1, Z.getref());   // ⎕ET value
        new (&Z->get_ravel(2)) PointerCell(Z2, Z.getref());   // ⎕EM

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   Assert(fun);

   Log(LOG_UserFunction__execute)   fun->print(CERR);

   Workspace::push_SI(fun, LOC);
   Workspace::SI_top()->set_safe_execution(true);

   // install end of context handler.
   // The handler will create the result after executing B.
   //
   {
     Value_P dummy_B;
     EOC_arg arg(dummy_B);

     Workspace::SI_top()->set_eoc_handler(eoc);
     Workspace::SI_top()->get_eoc_arg() = arg;
   }

   return Token(TOK_SI_PUSHED);
}
//-----------------------------------------------------------------------------
bool
Quad_EC::eoc(Token & result_B, EOC_arg & arg)
{
StateIndicator * si = Workspace::SI_top();
   si->set_safe_execution(false);

Value_P Z(new Value(3, LOC));
Value_P Z2;

int result_type = 0;
ErrorCode ec = E_NO_ERROR;

   switch(result_B.get_tag())
      {
       case TOK_ERROR:
            {
              result_type = 0;
              const Error & err = si->get_error();

              ec = ErrorCode(result_B.get_int_val());

              UCS_string row_1 = Error::error_name(ec);
              UCS_string row_2 = err.get_error_line_2();
              UCS_string row_3 = err.get_error_line_3();
              int cols = row_1.size();
              if (cols < row_2.size())   cols = row_2.size();
              if (cols < row_3.size())   cols = row_3.size();

              const Shape sh_Z2(3, cols);
              Z2 = Value_P(new Value(sh_Z2, LOC));   // 3 line message like ⎕EM
              Cell * C2 = &Z2->get_ravel(0);
              loop(c, cols)
                 if (c < row_1.size())   new (C2++) CharCell(row_1[c]);
                 else                    new (C2++) CharCell(UNI_ASCII_SPACE);
              loop(c, cols)
                 if (c < row_2.size())   new (C2++) CharCell(row_2[c]);
                 else                    new (C2++) CharCell(UNI_ASCII_SPACE);
              loop(c, cols)
                 if (c < row_3.size())   new (C2++) CharCell(row_3[c]);
                 else                    new (C2++) CharCell(UNI_ASCII_SPACE);
            }
            break;

        case TOK_APL_VALUE1:
        case TOK_APL_VALUE3:
             result_type = 1;
             Z2 = result_B.get_apl_val();
             break;

        case TOK_APL_VALUE2:
             result_type = 2;
             Z2 = result_B.get_apl_val();
             break;

        case TOK_NO_VALUE:
        case TOK_VOID:
             result_type = 3;
             Z2 = Idx0(LOC);  // 0⍴0
             break;

        case TOK_BRANCH:
             result_type = 4;
             Z2 = Value_P(new Value(LOC));
             new (&Z2->get_ravel(0))   IntCell(result_B.get_int_val());
             break;

        case TOK_ESCAPE:
             result_type = 5;
             Z2 = Idx0(LOC);  // 0⍴0
             break;

        default: CERR << "unexpected result tag " << result_B.get_tag()
                      << " in Quad_EC::eoc()" << endl;
                 Assert(0);
      }

Value_P Z1(new Value(2, LOC));
   new (&Z1->get_ravel(0)) IntCell(Error::error_major(ec));
   new (&Z1->get_ravel(1)) IntCell(Error::error_minor(ec));

   new (&Z->get_ravel(0)) IntCell(result_type);
   new (&Z->get_ravel(1)) PointerCell(Z1, Z.getref());
   new (&Z->get_ravel(2)) PointerCell(Z2, Z.getref());

   Z->check_value(LOC);
   move_2(result_B, Token(TOK_APL_VALUE1, Z), LOC);

   return false;
}
//=============================================================================
Token
Quad_ENV::eval_B(Value_P B)
{
   if (!B->is_char_string())   DOMAIN_ERROR;

const ShapeItem ec_B = B->element_count();

vector<const char *> evars;

   for (char **e = environ; *e; ++e)
       {
         const char * env = *e;

         // check if env starts with B.
         //
         bool match = true;
         loop(b, ec_B)
            {
              if (B->get_ravel(b).get_char_value() != Unicode(env[b]))
                 {
                   match = false;
                   break;
                 }
            }

         if (match)   evars.push_back(env);
       }

const Shape sh_Z(evars.size(), 2);
Value_P Z(new Value(sh_Z, LOC));

   loop(e, evars.size())
      {
        const char * env = evars[e];
        UCS_string ucs;
        while (*env)
           {
             if (*env != '=')   ucs.append(Unicode(*env++));
             else               break;
           }
        ++env;   // skip '='

        Value_P varname(new Value(ucs, LOC));

        ucs.clear();
        while (*env)   ucs.append(Unicode(*env++));

        Value_P varval(new Value(ucs, LOC));

        new (Z->next_ravel()) PointerCell(varname, Z.getref());
        new (Z->next_ravel()) PointerCell(varval, Z.getref());
      }

   Z->set_default_Spc();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_ES::eval_AB(Value_P A, Value_P B)
{
const UCS_string ucs(*A.get());
Error error(E_NO_ERROR, LOC);
const Token ret = event_simulate(&ucs, B, error);
   if (error.error_code == E_NO_ERROR)   return ret;

   throw error;
}
//-----------------------------------------------------------------------------
Token
Quad_ES::eval_B(Value_P B)
{
Error error(E_NO_ERROR, LOC);
const Token ret = event_simulate(0, B, error);
   if (error.error_code == E_NO_ERROR)   return ret;

   throw error;
}
//-----------------------------------------------------------------------------
Token
Quad_ES::event_simulate(const UCS_string * A, Value_P B, Error & error)
{
   // B is empty: no action
   //
   if (B->element_count() == 0)   return Token();

   error.init(get_error_code(B), error.throw_loc);

   if (error.error_code == E_NO_ERROR)   // B = 0 0: reset ⎕ET and ⎕EM.
      {
        Workspace::clear_error(LOC);
        return Token();
      }

   if (error.error_code == E_ASSERTION_FAILED)   // B = 0 ASSERTION_FAILED
      {
        Assert(0 && "simulated ASSERTION_FAILED in ⎕ES");
      }

   // at this point we shall throw the error. Add some error details.
   //
   // set up error message 1
   //
   if (A)                                 // A ⎕ES B
      {
        error.error_message_1 = *A;
      }
   else if (error.error_code == E_USER_DEFINED_ERROR)   // ⎕ES with character B
      {
        error.error_message_1 = UCS_string(*B.get());
      }
   else if (error.is_known())             //  ⎕ES B with known major/minor B
      {
        /* error_message_1 already OK */ ;
      }
   else                                   //  ⎕ES B with unknown major/minor B
      {
        error.error_message_1.clear();
      }

   error.show_locked = true;

   Assert(Workspace::SI_top());
   if (StateIndicator * si = Workspace::SI_top()->get_parent())
      {
        const UserFunction * ufun = si->get_executable()->get_ufun();
        if (ufun)
           {
             // lrm p 282: When ⎕ES is executed from within a defined function
             // and B is not empty, the event action is generated as though
             // the function were primitive.
             //
             error.error_message_2 = UCS_string(6, UNI_ASCII_SPACE);
             error.error_message_2.append(ufun->get_name());
             error.left_caret = 6;
             error.right_caret = -1;
             Workspace::pop_SI(LOC);
             Workspace::SI_top()->get_error() = error;
             error.print_em(UERR, LOC);
             return Token();
           }
      }

   Workspace::SI_top()->update_error_info(error);
   return Token();
}
//-----------------------------------------------------------------------------
ErrorCode
Quad_ES::get_error_code(Value_P B)
{
   // B shall be one of:                  → Error code
   //
   // 1. empty, or                        → No error
   // 2. a 2 element integer vector, or   → B[0]/N[1]
   // 3. a simple character vector.       → user defined
   //
   // Otherwise, throw an error

   if (B->get_rank() > 1)   RANK_ERROR;

   if (B->element_count() == 0)   return E_NO_ERROR;
   if (B->is_char_string())       return E_USER_DEFINED_ERROR;
   if (B->element_count() != 2)   LENGTH_ERROR;

const APL_Integer major = B->get_ravel(0).get_int_value();
const APL_Integer minor = B->get_ravel(1).get_int_value();

   return ErrorCode(major << 16 | (minor & 0xFFFF));
}
//=============================================================================
Token
Quad_EX::eval_B(Value_P B)
{
   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem var_count = B->get_rows();
vector<UCS_string> vars(var_count);
   B->to_varnames(vars, false);

Value_P Z(var_count > 1 ? new Value(var_count, LOC) : new Value(LOC));

   loop(z, var_count)   new (Z->next_ravel()) IntCell(expunge(vars[z]));

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
int
Quad_EX::expunge(UCS_string name)
{
   if (name.size() == 0)   return 0;

Symbol * symbol = Workspace::lookup_existing_symbol(name);
   if (symbol == 0)   return 0;
   return symbol->expunge();
}
//=============================================================================
Token
Quad_INP::eval_AB(Value_P A, Value_P B)
{
EOC_arg arg(B);
quad_INP & _arg = arg.u.u_quad_INP;
   _arg.lines = 0;

   // B is the end of document marker for a 'HERE document', similar
   // to ENDCAT in cat << ENDCAT.
   //
   // make sure that B is a non-empty string and extract it.
   //
   if (B->get_rank() > 1)         RANK_ERROR;
   if (B->element_count() == 0)   LENGTH_ERROR;

   _arg.end_marker = new UCS_string(B->get_UCS_ravel());

   // A is either one string (esc1 == esc2) or two (nested) strings
   // for esc1 and esc2 respectively.
   //
   if (A->compute_depth() == 2)   // two (nested) strings
      {
        if (A->get_rank() != 1)        RANK_ERROR;
        if (A->element_count() != 2)   LENGTH_ERROR;

        UCS_string ** esc[2] = { &_arg.esc1, &_arg.esc2 };
        loop(e, 2)
           {
             const Cell & cell = A->get_ravel(e);
             if (cell.is_pointer_cell())   // char vector
                {
                  *esc[e] =
                      new UCS_string(cell.get_pointer_value()->get_UCS_ravel());
                }
             else                          // char scalar
                {
                  *esc[e] = new UCS_string(1, cell.get_char_value());
                }
           }

        if (_arg.esc1->size() == 0)   LENGTH_ERROR;
      }
   else                       // one string 
      {
        _arg.esc1 = _arg.esc2 = new UCS_string(A->get_UCS_ravel());
      }

Token tok(TOK_FIRST_TIME);
   eoc_INP(tok, arg);
   return tok;
}
//-----------------------------------------------------------------------------
Token
Quad_INP::eval_B(Value_P B)
{
EOC_arg arg(B);
quad_INP & _arg = arg.u.u_quad_INP;
   _arg.lines = 0;

   // B is the end of document marker for a 'HERE document', similar
   // to ENDCAT in cat << ENDCAT.
   //
   // make sure that B is a non-empty string and extract it.
   //
   if (B->get_rank() > 1)         RANK_ERROR;
   if (B->element_count() == 0)   LENGTH_ERROR;

   _arg.end_marker = new UCS_string(B->get_UCS_ravel());

   _arg.esc1 = _arg.esc2 = 0;

Token tok(TOK_FIRST_TIME);
   eoc_INP(tok, arg);
   return tok;
}
//-----------------------------------------------------------------------------
bool
Quad_INP::eoc_INP(Token & token, EOC_arg & _arg)
{
   if (token.get_tag() == TOK_ERROR)
      {
         CERR << "Error in ⎕INP" << endl;
         UCS_string err("*** ");
         err.append(Error::error_name((ErrorCode)(token.get_int_val())));
         err.append_utf8(" in ⎕INP ***");
         Value_P val(new Value(err, LOC));
         move_2(token, Token(TOK_APL_VALUE1, val), LOC);
      }

   // _arg may be pop_SI()ed below so we need a copy of it
   //
quad_INP arg = _arg.u.u_quad_INP;

   if (token.get_tag() != TOK_FIRST_TIME)
      {
        // token is the result of ⍎ exec and arg.lines has at least 2 lines.
        // append the value in token and then the last line to the second
        // last line and pop the last line.
        //
        Value_P result = token.get_apl_val();

        UCS_string_list * last = arg.lines;
        Assert(last);                       // remember last
        arg.lines = arg.lines->prev;        // pop last from lines
        Assert(arg.lines);                  // must be 'before' below

        // if the result is a simple char string then insert it between
        // the last two lines. Otherwise insert a 2-dimensional ⍕result
        // indented by the length of arg.lines.string (== before below)
        // 
        if (result->is_char_string())
           {
             // simple char string: insert result
             //
             arg.lines->string.append(result->get_UCS_ravel());
           }
        else
           {
             Value_P matrix = Bif_F12_FORMAT::monadic_format(result);
             const Cell * cm = &matrix->get_ravel(0);
             const int plen = arg.lines->string.size();   // indent

             // append the first row of the matrix to 'before'
             //
             loop(r, matrix->get_cols())
                 arg.lines->string.append(cm++->get_char_value());

             // append subsequent rows of the matrix to a new line
             //
             loop(r, matrix->get_rows() - 1)
                {
                  UCS_string row(plen, UNI_ASCII_SPACE);
                  loop(r, matrix->get_cols())
                      row.append(cm++->get_char_value());

                  arg.lines = new UCS_string_list(row, arg.lines);
                }
           }

        arg.lines->string.append(last->string);            // append last line

        delete last;
      }

   for (;;)   // read lines until end reached.
       {
         bool eof = false;
         UCS_string line;
         UCS_string prompt;
         InputMux::get_line(LIM_Quad_INP, prompt, line, eof,
                            LineHistory::quad_INP_history);
         arg.done = (line.substr_pos(*arg.end_marker) != -1);

         if (arg.esc1)   // start marker defined (dyadic ⎕INP)
            {
              UCS_string exec;     // line esc1 ... esc2 (excluding)

              const int e1_pos = line.substr_pos(*arg.esc1);
              if (e1_pos != -1)   // and the start marker is present.
                 {
                   // push characters left of exec
                   //
                   {
                     const UCS_string before(line, 0, e1_pos);
                     arg.lines = new UCS_string_list(before, arg.lines);
                   }

                   exec = line.drop(e1_pos + arg.esc1->size());
                   exec.remove_lt_spaces();   // no leading and trailing spaces
                   line.clear();

                   if (arg.esc2 == 0)
                      {
                        // empty esc2 means exec is the rest of the line.
                        // nothing to do.
                      }
                   else
                      {
                        // non-empty esc2: search for it
                        //
                        const int e2_pos = exec.substr_pos(*arg.esc2);
                        if (e2_pos == -1)   // esc2 not found: exec rest of line
                           {
                             // esc2 not in exec: take rest of line
                             // i.e. nothing to do.
                           }
                        else
                           {
                             const int rest = e2_pos + arg.esc2->size();
                             line = UCS_string(exec, rest, exec.size() - rest);
                             exec.shrink(e2_pos);
                           }
                      }
                 }

              if (exec.size())
                 {
                   // if SI is already pushed (i.e. this is a subsequent
                   // call to eoc_INP() then pop it.
                   //
                   if (token.get_tag() != TOK_FIRST_TIME)
                      Workspace::pop_SI(LOC);

                   arg.lines = new UCS_string_list(line, arg.lines);

                   move_2(token, Bif_F1_EXECUTE::execute_statement(exec), LOC);
                   Assert(token.get_tag() == TOK_SI_PUSHED);

                   StateIndicator * si = Workspace::SI_top();
                   si->set_eoc_handler(eoc_INP);
                   si->get_eoc_arg().u.u_quad_INP = arg;
                   return true;   // continue
                 }
            }

         if (arg.done)   break;

         arg.lines = new UCS_string_list(line, arg.lines);
       }

const ShapeItem zlen = UCS_string_list::length(arg.lines);
Value_P Z(new Value(ShapeItem(zlen), LOC));
Cell * cZ = &Z->get_ravel(0) + zlen;

   loop(z, zlen)
      {
        UCS_string_list * node = arg.lines;
        arg.lines = arg.lines->prev;
        Value_P ZZ(new Value(node->string, LOC));
        ZZ->check_value(LOC);
        new (--cZ)   PointerCell(ZZ, Z.getref());
        delete node;
      }

   delete arg.end_marker;
   delete arg.esc1;
   if (arg.esc2 != arg.esc1)   delete arg.esc2;

   if (Z->is_empty())   // then Z←⊂''
      {
        Shape sh(0);   // length 0 vector
        Value_P ZZ(new Value(sh, LOC));
        ZZ->set_default_Spc();
        new (&Z->get_ravel(0)) PointerCell(ZZ, Z.getref());
      }

   Z->check_value(LOC);
   move_2(token, Token(TOK_APL_VALUE1, Z), LOC);
   return false;   // continue
}
//-----------------------------------------------------------------------------
Token
Quad_INP::eval_XB(Value_P X, Value_P B)
{
   if (X->element_count() != 1)
      {
        if (X->get_rank() > 1)   RANK_ERROR;
        else                     LENGTH_ERROR;
      }

APL_Integer x = X->get_ravel(0).get_int_value();
   if (x == 0)   return eval_B(B);
   if (x > 1)    DOMAIN_ERROR;

   // B is the end of document marker for a 'HERE document', similar
   // to ENDCAT in cat << ENDCAT.
   //
   // make sure that B is a non-empty string and extract it.
   //
   if (B->get_rank() > 1)         RANK_ERROR;
   if (B->element_count() == 0)   LENGTH_ERROR;

UCS_string end_marker(B->get_UCS_ravel());

vector<UCS_string> lines;
Parser parser(PM_EXECUTE, LOC);

   for (;;)
      {
         bool eof = false;
         UCS_string line;
         UCS_string prompt;
         InputMux::get_line(LIM_Quad_INP, prompt, line, eof,
                            LineHistory::quad_INP_history);
         if (eof)   break;
         const int end_pos = line.substr_pos(end_marker);
         if (end_pos != -1)  break;

         Token_string tos;
         ErrorCode ec = parser.parse(line, tos);
         if (ec != E_NO_ERROR)
            {
              throw_apl_error(ec, LOC);
            }

         if ((tos.size() & 1) == 0)   LENGTH_ERROR;

         // expect APL values at even positions and , at odd positions
         //
         loop(t, tos.size())
            {
              Token & tok = tos[t];
              if (t & 1)   // , or ⍪
                 {
                   if (tok.get_tag() != TOK_F12_COMMA &&
                       tok.get_tag() != TOK_F12_COMMA1)   DOMAIN_ERROR;
                 }
              else         // value
                 {
                   if (tok.get_Class() != TC_VALUE)   DOMAIN_ERROR;
                 }

            }
         lines.push_back(line);
      }

Value_P Z(new Value(lines.size(), LOC));
   loop(z, lines.size())
      {
         Token_string tos;
         parser.parse(lines[z], tos);
         const ShapeItem val_count = (tos.size() + 1)/2;
         Value_P ZZ(new Value(val_count, LOC));
         loop(v, val_count)
            {
              new (ZZ->next_ravel()) PointerCell(tos[2*v].get_apl_val(),
                                                 ZZ.getref());
            }

         new (Z->next_ravel()) PointerCell(ZZ, Z.getref());
      }

   if (lines.size() == 0)   // empty result
      {
        Value_P ZZ(new Value(UCS_string(), LOC));
        new(&Z->get_ravel(0)) PointerCell(ZZ, Z.getref());
      }
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_NC::eval_B(Value_P B)
{
   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem var_count = B->get_rows();
vector<UCS_string> vars(var_count);
   B->to_varnames(vars, false);

Value_P Z(var_count > 1 ? new Value(var_count, LOC) : new Value(LOC));

   loop(v, var_count)   new (Z->next_ravel())   IntCell(get_NC(vars[v]));

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
APL_Integer
Quad_NC::get_NC(const UCS_string ucs)
{
   if (ucs.size() == 0)   return -1;   // invalid name

   if (Avec::is_quad(ucs[0]))   // quad fun or var
      {
        int len = 0;
        const Token t = Workspace::get_quad(ucs, len);
        if (len < 2)                      return -1;   // invalid quad
        if (t.get_Class() == TC_SYMBOL)   return  5;   // quad variable
        else                              return  6;   // quad function

        // cases 0, 1, or 4 cannot happen for Quad symbols
      }

Symbol * symbol = Workspace::lookup_existing_symbol(ucs);

   if (!symbol)
      {
        // symbol not found. Distinguish between invalid and unused names
        //
        if (!Avec::is_first_symbol_char(ucs[0]))   return -1;   // invalid
        loop (u, ucs.size())
           {
              if (!Avec::is_symbol_char(ucs[u]))   return -1;   // invalid
           }
        return 0;   // unused
      }

const NameClass nc = symbol->get_nc();

   // return nc, but map shared variables to regular variables
   //
   return nc == NC_SHARED_VAR ? NC_VARIABLE : nc;
}
//=============================================================================
Token
Quad_NL::do_quad_NL(Value_P A, Value_P B)
{
   if (!!A && !A->is_char_string())   DOMAIN_ERROR;
   if (B->get_rank() > 1)             RANK_ERROR;

UCS_string first_chars;
   if (!!A)   first_chars = UCS_string(*A);

   // 1. create a bitmap of ⎕NC values requested in B
   //
int requested_NCs = 0;
   {
     loop(b, B->element_count())
        {
          const APL_Integer bb = B->get_ravel(b).get_int_value();
          if (bb < 1)   DOMAIN_ERROR;
          if (bb > 6)   DOMAIN_ERROR;
          requested_NCs |= 1 << bb;
        }
   }

   // 2, build a name table, starting with user defined names
   //
vector<UCS_string> names;
   {
     const uint32_t symbol_count = Workspace::symbols_allocated();
     DynArray(Symbol *, table, symbol_count);
     Workspace::get_all_symbols(&table[0], symbol_count);

     loop(s, symbol_count)
        {
          Symbol * symbol = table[s];
          if (symbol->is_erased())   continue;   // erased symbol

          NameClass nc = symbol->get_nc();
          if (nc == NC_SHARED_VAR)   nc = NC_VARIABLE;

          if (!(requested_NCs & 1 << nc))   continue;   // name class not in B

          if (first_chars.size())
             {
               const Unicode first_char = symbol->get_name()[0];
               if (!first_chars.contains(first_char))   continue;
             }

          names.push_back(symbol->get_name());
        }
   }

   // 3, append ⎕-vars and ⎕-functions to name table (unless prevented by A)
   //
   if (first_chars.size() == 0 || first_chars.contains(UNI_Quad_Quad))
      {
#define ro_sv_def(x) { Symbol * symbol = &Workspace::get_v_ ## x();           \
                       UCS_string ucs = symbol->get_name();           \
                       if (ucs[0] == UNI_Quad_Quad && requested_NCs & 1 << 5) \
                       names.push_back(ucs); }

#define rw_sv_def(x) { Symbol * symbol = &Workspace::get_v_ ## x();           \
                       UCS_string ucs = symbol->get_name();           \
                       if (ucs[0] == UNI_Quad_Quad && requested_NCs & 1 << 5) \
                       names.push_back(ucs); }

#define sf_def(x)    { if (requested_NCs & 1 << 6) \
                          names.push_back((x::fun.get_name())); }
#include "SystemVariable.def"
      }


   // 4. compute length of longest name
   //
ShapeItem longest = 0;
   loop(n, names.size())
      {
        if (longest < names[n].size())
           longest = names[n].size();
      }

const Shape shZ(names.size(), longest);
Value_P Z(new Value(shZ, LOC));

   // 5. construct result. The number of symbols is small and ⎕NL is
   // (or should) not be a perfomance // critical function, so we can
   // use a simple (find smallest and print) // approach.
   //
   for (int count = names.size(); count; --count)
      {
        // find smallest.
        //
        uint32_t smallest = count - 1;
        loop(t, count - 1)
           {
             if (names[smallest].compare(names[t]) > 0)   smallest = t;
           }
        // copy name to result, padded with spaces.
        //
        loop(l, longest)
           {
             const UCS_string & ucs = names[smallest];
             new (Z->next_ravel())
                 CharCell(l < ucs.size() ? ucs[l] : UNI_ASCII_SPACE);
           }

        // remove smalles from table
        //
        names[smallest] = names[count - 1];
      }

   Z->set_default_Spc();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_SI::eval_AB(Value_P A, Value_P B)
{
   if (A->element_count() != 1)   // not scalar-like
      {
        if (A->get_rank() > 1)   RANK_ERROR;
        else                     LENGTH_ERROR;
      }
APL_Integer a = A->get_ravel(0).get_int_value();
const ShapeItem len = Workspace::SI_entry_count();
   if (a >= len)   DOMAIN_ERROR;
   if (a < -len)   DOMAIN_ERROR;
   if (a < 0)   a += len;   // negative a counts backwards from end

const StateIndicator * si = 0;
   for (si = Workspace::SI_top(); si; si = si->get_parent())
       {
         if (si->get_level() == a)   break;   // found
       }

   Assert(si);

   if (B->element_count() != 1)   // not scalar-like
      {
        if (B->get_rank() > 1)   RANK_ERROR;
        else                     LENGTH_ERROR;
      }

const Function_PC PC = Function_PC(si->get_PC() - 1);
const Executable * exec = si->get_executable();
const ParseMode pm = exec->get_parse_mode();
const UCS_string & fun_name = exec->get_name();
const Function_Line fun_line = exec->get_line(PC);

Value_P Z;

const APL_Integer b = B->get_ravel(0).get_int_value();
   switch(b)
      {
        case 1:  Z = Value_P(new Value(fun_name, LOC));
                 break;

        case 2:  Z = Value_P(new Value(LOC));
                new (&Z->get_ravel(0)) IntCell(fun_line);
                break;

        case 3:  {
                   UCS_string fun_and_line(fun_name);
                   fun_and_line.append(UNI_ASCII_L_BRACK);
                   fun_and_line.append_number(fun_line);
                   fun_and_line.append(UNI_ASCII_R_BRACK);
                   Z = Value_P(new Value(fun_and_line, LOC)); 
                 }
                 break;

        case 4:  if (si->get_error().error_code)
                    {
                      const UCS_string & text =
                                        si->get_error().get_error_line_2();
                      Z = Value_P(new Value(text, LOC));
                    }
                 else
                    {
                      const UCS_string text = exec->statement_text(PC);
                      Z = Value_P(new Value(text, LOC));
                    }
                 break;

        case 5: Z = Value_P(new Value(LOC));
                new (&Z->get_ravel(0)) IntCell(PC);                      break;

        case 6: Z = Value_P(new Value(LOC));
                new (&Z->get_ravel(0)) IntCell(pm);                      break;

        default: DOMAIN_ERROR;
      }

   Z->set_default_Zero();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Quad_SI::eval_B(Value_P B)
{
   if (B->element_count() != 1)   // not scalar-like
      {
        if (B->get_rank() > 1)   RANK_ERROR;
        else                     LENGTH_ERROR;
      }

const APL_Integer b = B->get_ravel(0).get_int_value();
const ShapeItem len = Workspace::SI_entry_count();

   if (b < 1)   DOMAIN_ERROR;
   if (b > 4)   DOMAIN_ERROR;

   // at this point we should not fail...
   //
Value_P Z(new Value(len, LOC));

ShapeItem z = 0;
   for (const StateIndicator * si = Workspace::SI_top();
        si; si = si->get_parent())
       {
         Cell * cZ = &Z->get_ravel(len - ++z);

         const Function_PC PC = Function_PC(si->get_PC() - 1);
         const Executable * exec = si->get_executable();
         const ParseMode pm = exec->get_parse_mode();
         const UCS_string & fun_name = exec->get_name();
         const Function_Line fun_line = exec->get_line(PC);

         switch (b)
           {
             case 1:  new (cZ) PointerCell(
                                Value_P(new Value(fun_name, LOC)), Z.getref());
                      break;

             case 2:  new (cZ) IntCell(fun_line);
                      break;

             case 3:  {
                        UCS_string fun_and_line(fun_name);
                        fun_and_line.append(UNI_ASCII_L_BRACK);
                        fun_and_line.append_number(fun_line);
                        fun_and_line.append(UNI_ASCII_R_BRACK);
                        new (cZ) PointerCell(Value_P(
                                    new Value(fun_and_line, LOC)), Z.getref()); 
                      }
                      break;

             case 4:  if (si->get_error().error_code)
                         {
                           const UCS_string & text =
                                        si->get_error().get_error_line_2();
                           new (cZ) PointerCell(Value_P(
                                            new Value(text, LOC)), Z.getref()); 
                         }
                      else
                         {
                           const UCS_string text = exec->statement_text(PC);
                           new (cZ) PointerCell(Value_P(
                                            new Value(text, LOC)), Z.getref()); 
                         }
                      break;

             case 5:  new (cZ) IntCell(PC);                             break;
             case 6:  new (cZ) IntCell(pm);                             break;
           }

       }

   Z->set_default_Zero();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_UCS::eval_B(Value_P B)
{
Value_P Z(new Value(B->get_shape(), LOC));

const ShapeItem ec = B->nz_element_count();
   loop(v, ec)
       {
         const Cell & cell_B = B->get_ravel(v);

         if (cell_B.is_character_cell())   // char to Unicode
            {
              const Unicode uni = cell_B.get_char_value();
              new (&Z->get_ravel(v)) IntCell(uni);
              continue;
            }

         if (cell_B.is_integer_cell())
            {
              const APL_Integer bint = cell_B.get_int_value();
              new (&Z->get_ravel(v))   CharCell(Unicode(bint));
              continue;
            }

         Q1(cell_B.get_cell_type())
         DOMAIN_ERROR;
       }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
UserFunction *
Stop_Trace::locate_fun(const Value & fun_name)
{
UCS_string fun_name_ucs(fun_name);
   if (fun_name_ucs.size() == 0)   LENGTH_ERROR;

Symbol * fun_symbol = Workspace::lookup_existing_symbol(fun_name_ucs);
   if (fun_symbol == 0)
      {
        CERR << "symbol " << fun_name_ucs << " not found" << endl;
        return 0;
      }

Function * fun = fun_symbol->get_function();
   if (fun_symbol == 0)
      {
        CERR << "symbol " << fun_name_ucs << " is not a function" << endl;
        return 0;
      }

UserFunction * ufun = fun->get_ufun1();
   if (ufun == 0)
      {
        CERR << "symbol " << fun_name_ucs
             << " is not a defined function" << endl;
        return 0;
      }

   return ufun;
}
//-----------------------------------------------------------------------------
Token
Stop_Trace::reference(const vector<Function_Line> & lines, bool assigned)
{
Value_P Z(new Value(lines.size(), LOC));

   loop(z, lines.size())   new (Z->next_ravel()) IntCell(lines[z]);

   Z->set_default_Zero();
   if (assigned)   return Token(TOK_APL_VALUE2, Z);
   else            return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
void
Stop_Trace::assign(UserFunction * ufun, const Value & new_value, bool stop)
{
DynArray(Function_Line, lines, new_value.element_count());
int line_count = 0;

const APL_Float qct = Workspace::get_CT();
   loop(l, new_value.element_count())
      {
         APL_Integer line = new_value.get_ravel(l).get_near_int(qct);
         if (line < 1)   continue;
         lines[line_count++] = (Function_Line)line;
      }

   ufun->set_trace_stop(lines, line_count, stop);
}
//=============================================================================
Token
Quad_STOP::eval_AB(Value_P A, Value_P B)
{
   // Note: Quad_STOP::eval_AB can be called directly or via S∆. If
   //
   // 1. called via S∆   then A is the function and B are the lines.
   // 2. called directly then B is the function and A are the lines.
   //
UserFunction * ufun = locate_fun(*A);
   if (ufun)   // case 1.
      {
        assign(ufun, *B, true);
        return reference(ufun->get_stop_lines(), true);
      }

   // case 2.
   //
   ufun = locate_fun(*B);
   if (ufun == 0)   DOMAIN_ERROR;

   assign(ufun, *A, true);
   return reference(ufun->get_stop_lines(), true);
}
//-----------------------------------------------------------------------------
Token
Quad_STOP::eval_B(Value_P B)
{
UserFunction * ufun = locate_fun(*B);
   if (ufun == 0)   DOMAIN_ERROR;

   return reference(ufun->get_stop_lines(), false);
}
//=============================================================================
Token
Quad_TRACE::eval_AB(Value_P A, Value_P B)
{
   // Note: Quad_TRACE::eval_AB can be called directly or via S∆. If
   //
   // 1. called via S∆   then A is the function and B are the lines.
   // 2. called directly then B is the function and A are the lines.
   //
UserFunction * ufun = locate_fun(*A);
   if (ufun)   // case 1.
      {
        assign(ufun, *B, false);
        return reference(ufun->get_trace_lines(), true);
      }

   // case 2.
   //
   ufun = locate_fun(*B);
   if (ufun == 0)   DOMAIN_ERROR;

   assign(ufun, *A, false);
   return reference(ufun->get_trace_lines(), true);
}
//-----------------------------------------------------------------------------
Token
Quad_TRACE::eval_B(Value_P B)
{
UserFunction * ufun = locate_fun(*B);
   if (ufun == 0)   DOMAIN_ERROR;

   return reference(ufun->get_trace_lines(), false);
}
//=============================================================================

