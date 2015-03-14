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

#include "Error.hh"
#include "PointerCell.hh"
#include "PrintOperator.hh"
#include "Value.icc"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
PointerCell::PointerCell(Value_P sub_val, Value & cell_owner)
{
   new (&value._valp()) Value_P(sub_val);
   value2.owner = &cell_owner;

   Assert(value2.owner != sub_val.get());   // typical cut-and-paste error
   cell_owner.increment_pointer_cell_count();
   cell_owner.add_subcount(sub_val->nz_element_count());
}
//-----------------------------------------------------------------------------
void
PointerCell::release(const char * loc)
{
   value2.owner->decrement_pointer_cell_count();
   value2.owner->add_subcount(-get_pointer_value()->nz_element_count());

   ptr_clear(value._valp(), loc);
   new (this) Cell;
}
//-----------------------------------------------------------------------------
bool
PointerCell::equal(const Cell & other, APL_Float qct) const
{
   if (!other.is_pointer_cell())   return false;

Value_P A = get_pointer_value();
Value_P B = other.get_pointer_value();

   if (!A->same_shape(*B))                 return false;

const ShapeItem count = A->element_count();
   loop(c, count)
       if (!A->get_ravel(c).equal(B->get_ravel(c), qct))   return false;

   return true;
}
//-----------------------------------------------------------------------------
bool
PointerCell::greater(const Cell & other) const
{
   switch(other.get_cell_type())
      {
        case CT_CHAR:
        case CT_INT:
        case CT_FLOAT:
        case CT_COMPLEX: return true;
        case CT_POINTER: break;   // continue below
        case CT_CELLREF: DOMAIN_ERROR;
        default:         Assert(0 && "Bad celltype");
      }

   // at this point both cells are pointer cells.
   //
Value_P v1 = get_pointer_value();
Value_P v2 = other.get_pointer_value();

   // compare ranks
   //
   if (v1->get_rank() > v2->get_rank())   return true;
   if (v2->get_rank() > v1->get_rank())   return false;

   // same rank, compare shapes
   //
   loop(r, v1->get_rank())
      {
        if (v1->get_shape_item(r) > v2->get_shape_item(r))   return true;
        if (v2->get_shape_item(r) > v1->get_shape_item(r))   return false;
      }

   // same rank and shape, compare ravel
   //
const Cell * C1 = &v1->get_ravel(0);
const Cell * C2 = &v2->get_ravel(0);
   loop(e, v1->nz_element_count())
      {
        const Comp_result comp = C1++->compare(*C2++);
        if (comp == COMP_GT)   return   true;
        if (comp == COMP_LT)   return  false;
      }

   // everthing equal
   //
   return this > &other;
}
//-----------------------------------------------------------------------------
Value_P
PointerCell::get_pointer_value() const
{
Value_P ret = value._valp();
   return ret;
}
//-----------------------------------------------------------------------------
CellType
PointerCell::deep_cell_types() const
{
   return CellType(CT_POINTER | get_pointer_value()->deep_cell_types());
}
//-----------------------------------------------------------------------------
CellType
PointerCell::deep_cell_subtypes() const
{
   return CellType(CT_POINTER | get_pointer_value()->deep_cell_subtypes());
}
//-----------------------------------------------------------------------------
PrintBuffer
PointerCell::character_representation(const PrintContext & pctx) const
{
Value_P val = get_pointer_value();
   Assert(val);

   if (pctx.get_style() == PR_APL_FUN)   // APL function display
      {
        const ShapeItem ec = val->element_count();
        UCS_string ucs;

        if (val->is_char_vector())
           {
             ucs.append(UNI_SINGLE_QUOTE);
             loop(e, ec)
                {
                  const Unicode uni = val->get_ravel(e).get_char_value();
                  ucs.append(uni);
                  if (uni == UNI_SINGLE_QUOTE)   ucs.append(uni);   // ' -> ''
                }
             ucs.append(UNI_SINGLE_QUOTE);
           }
        else
           {
             ucs.append(UNI_ASCII_L_PARENT);
             loop(e, ec)
                {
                  PrintBuffer pb = val->get_ravel(e).
                        character_representation(pctx);
                  ucs.append(UCS_string(pb, 0,
                             Workspace::get_PrintContext().get_PW()));

                  if (e < ec - 1)   ucs.append(UNI_ASCII_SPACE);
                }
             ucs.append(UNI_ASCII_R_PARENT);
           }

        ColInfo ci;
        return PrintBuffer(ucs, ci);
      }

PrintBuffer ret(*val, pctx, 0);
   ret.get_info().flags &= ~CT_MASK;
   ret.get_info().flags |= CT_POINTER;

   if (pctx.get_style() & PST_CS_INNER)
      {
        const ShapeItem ec = val->element_count();
        if (ec == 0)   // empty value
           {
             int style = pctx.get_style();
             loop(r, val->get_rank())
                {
                  if (val->get_shape_item(r) == 0)   // an empty axis
                     {
                       if (r == val->get_rank() - 1)   style |= PST_EMPTY_LAST;
                       else                            style |= PST_EMPTY_NLAST;
                     }
                }

             // do A←(1⌈⍴A)⍴⊂↑A
             Value_P proto = val->prototype(LOC);

             // compute a shape like ⍴ val but 0 replaced with 1.
             //
             Shape sh(val->get_shape());
             loop(r, sh.get_rank())
                 {
                   if (sh.get_shape_item(r) == 0)   sh.set_shape_item(r, 1);
                 }

             if (sh.get_volume() == 1)   // one prototype
                {
                  ret = PrintBuffer(*proto, pctx, 0);
                  ret.add_frame(PrintStyle(style), proto->get_shape(),
                                proto->compute_depth());
                }
             else                           // several prototypes
                {
                  Value_P proto_reshaped(sh, LOC);
                  Cell * c = &proto_reshaped->get_ravel(0);
                  const ShapeItem len = proto_reshaped->element_count();

                  // store proto in the first ravel item, and copies of proto in
                  // the subsequent ravel items
                  //
                  new (c++) PointerCell(proto, proto_reshaped.getref());
                  loop(rv, len - 1)
                      new (c++) PointerCell(proto->clone(LOC),
                                            proto_reshaped.getref());

                  ret = PrintBuffer(*proto_reshaped, pctx, 0);
                  ret.add_frame(PrintStyle(style), proto_reshaped->get_shape(),
                                proto_reshaped->compute_depth());
                }
           }
       else
           {
             ret.add_frame(pctx.get_style(), val->get_shape(),
                           val->compute_depth());
           }
      }

   ret.get_info().int_len  = ret.get_width(0);
   ret.get_info().real_len = ret.get_width(0);
   return ret;
}
//-----------------------------------------------------------------------------

