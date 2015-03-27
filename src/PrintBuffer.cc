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

#include <stdio.h>
#include <string.h>

#include "Function.hh"
#include "Output.hh"
#include "Performance.hh"
#include "PointerCell.hh"
#include "PrintBuffer.hh"
#include "PrintOperator.hh"
#include "Value.icc"

/// max sizes for arrays on the stack. Larger values are allocated with new()
enum
{
   PB_MAX_COLS        = 200,
   PB_MAX_ROWS        = 100,
   PB_MAX_ITEMS       = PB_MAX_COLS * PB_MAX_ROWS,
   PB_MAX_BREAKPOINTS = 200,
};

//-----------------------------------------------------------------------------
PrintBuffer::PrintBuffer()
   : complete(true)
{
}
//-----------------------------------------------------------------------------
PrintBuffer::PrintBuffer(const UCS_string & ucs, const ColInfo & ci)
   : col_info(ci),
     complete(true)
{
   buffer.push_back(ucs);
}
//-----------------------------------------------------------------------------
PrintBuffer::PrintBuffer(const Value & value, const PrintContext & _pctx,
                         ostream * out)
   : complete(false)
{
PERFORMANCE_START(start_0)

   // Note: if ostream is non-0 then this value may be incomplete
   // (as indicated by member complete if it is huge. This is to speed
   // up printing if the value is discarded after having been printed

const PrintStyle outer_style = _pctx.get_style();
const bool framed = outer_style & (PST_CS_MASK | PST_CS_OUTER);
PrintContext pctx(_pctx);
   pctx.set_style(PrintStyle(outer_style &~ PST_CS_OUTER));

const ShapeItem ec = value.element_count();

   if (value.is_scalar())
      {
        PERFORMANCE_START(start_1)
        PrintContext pctx1(pctx);
        if (value.get_ravel(0).need_scaling(pctx))   pctx1.set_scaled();

        *this = value.get_ravel(0).character_representation(pctx1);

        // pad the value unless it is framed
        if (value.compute_depth() > 1 && !framed)
           {
             pad_l(UNI_iPAD_U4, 1);
             pad_r(UNI_iPAD_U5, 1);
           }

        add_outer_frame(outer_style);

        if (out)
           {
             UCS_string ucs(*this, value.get_rank(), _pctx.get_PW());
             if (ucs.size())   *out << ucs << endl;
            }
        complete = true;
        PERFORMANCE_END(fs_PrintBuffer1_B, start_1, ec)
        return;
      }

   if (ec == 0)   // empty value of any dimension
      {
        pb_empty(value, pctx, outer_style);
        if (out)
           {
             UCS_string ucs(*this, value.get_rank(), _pctx.get_PW());
             if (ucs.size())   *out << ucs << endl;
            }
        complete = true;
        return;
      }

   if (pctx.get_style() == PR_APL_FUN)
      {
        pb_for_function(value, pctx, outer_style);
        if (out)
           {
             UCS_string ucs(*this, value.get_rank(), _pctx.get_PW());
             if (ucs.size())   *out << ucs << endl;
            }
        complete = true;
        return;
      }

   // non-trivial PrintBuffer
   //
   const ShapeItem cols = value.get_last_shape_item();
   if (cols <= PB_MAX_COLS)   // few columns
      {
        DynArray(bool, scaling, cols);
        DynArray(PrintBuffer, pcols, cols);
        if (ec <= PB_MAX_ITEMS)
           {
             DynArray(PrintBuffer, item_matrix, ec);
             do_PrintBuffer(value, pctx, out, outer_style,
                            scaling, pcols, item_matrix);
           }
        else
           {
             PrintBuffer * item_matrix = new PrintBuffer[ec];
             do_PrintBuffer(value, pctx, out, outer_style,
                            scaling, pcols, item_matrix);
             delete [] item_matrix;
           }
      }
   else                      // many columns
      {
        bool * scaling = new bool[cols];
        PrintBuffer * pcols = new PrintBuffer[cols];
        if (ec <= PB_MAX_ITEMS)
           {
             DynArray(PrintBuffer, item_matrix, ec);
             do_PrintBuffer(value, pctx, out, outer_style,
                            scaling, pcols, item_matrix);
           }
        else
           {
             PrintBuffer * item_matrix = new PrintBuffer[ec];
             do_PrintBuffer(value, pctx, out, outer_style,
                            scaling, pcols, item_matrix);
             delete [] item_matrix;
           }
        delete [] pcols;
        delete [] scaling;
      }

   PERFORMANCE_END(fs_PrintBuffer_B, start_0, ec)
}
//-----------------------------------------------------------------------------
void
PrintBuffer::do_PrintBuffer(const Value & value, const PrintContext & pctx,
                         ostream * out, PrintStyle outer_style,
                         bool * scaling, PrintBuffer * pcols,
                         PrintBuffer * item_matrix)
{
const bool framed = outer_style & (PST_CS_MASK | PST_CS_OUTER);
const ShapeItem ec = value.element_count();
const uint64_t ii_count = interrupt_count;
const bool huge = out && ec > 10000;

   {
     // 1. create a vector with a bool per column that tells if the
     // column needs scaling (i.e. exponential format) or not
     //
     const ShapeItem cols = value.get_last_shape_item();
     const ShapeItem rows = ec/cols;

     loop(x, cols)
         {
           bool need_scaling = false;
           loop(y, rows)
               {
                 if (huge && (ii_count != interrupt_count))   goto interrupted;
                 if (value.get_ravel(x + y*cols).need_scaling(pctx))
                    {
                      need_scaling = true;
                      break;
                    }
               }
           scaling[x] = need_scaling;
         }

     // 2. create a matrix of items.
     //    An item of the matrix is a top-level cell (possibly nested),
     //    therefore we have (⍴,value) items. Items are rectangular.
     //
     PERFORMANCE_START(start_2)
     loop(y, rows)
        {
          ShapeItem max_row_height = 0;

          loop(x, cols)
              {
                if (huge && (ii_count != interrupt_count))   goto interrupted;

                PrintBuffer & item = item_matrix[y*cols + x];
                PrintContext pctx1 = pctx;
                if (scaling[x])   pctx1.set_scaled();
                const Cell & cell = value.get_ravel(x + y*cols);
                item = cell.character_representation(pctx1);

                if (max_row_height < item.get_height())
                   max_row_height = item.get_height();

                Assert1(item.is_rectangular());
              }

          // pad all items to the same height
          //
          loop(x, cols)
             {
                PrintBuffer & item = item_matrix[y*cols + x];
                if (huge && (ii_count != interrupt_count))   goto interrupted;

               item.pad_height(UNI_iPAD_U6, max_row_height);
               Assert1(item.is_rectangular());
             }
        }
     PERFORMANCE_END(fs_PrintBuffer2_B, start_2, ec)

     // 3. align all columns (which pads them to the same width).
     //
     PERFORMANCE_START(start_3)
     loop(x, cols)
        {
          ColInfo col_info_x;
          loop(y, rows)
              {
                if (huge && (ii_count != interrupt_count))   goto interrupted;
                PrintBuffer * item_row = item_matrix + y*cols;
                col_info_x.consider(item_row[x].get_info());
              }

          loop(y, rows)
              {
                if (huge && (ii_count != interrupt_count))   goto interrupted;
                PrintBuffer * item_row = item_matrix + y*cols;
                item_row[x].align(col_info_x);
              }
        }
     PERFORMANCE_END(fs_PrintBuffer3_B, start_3, ec)

     // 4. combine colums, then rows.
     //
     PERFORMANCE_START(start_4)

     int last_col_spacing = 0;    // the col_spacing of the previous column
     bool last_notchar = false;   // the notchar property of the previous column

     loop(x, cols)
        {
          // merge the different items in column x of item_matrix into one
          // PrintBuffer pcol. Insert separator rows as needed.
          //
          PrintBuffer & dest = pcols[x];
          Assert(&dest);
          new (&dest) PrintBuffer;

          // compute the final height of dest and reserve enough rows as to
          // avoid unnecessary copies
          //
          {
            ShapeItem dest_height = 0;
            loop(y, rows)
               {
                 dest_height += separator_rows(y, value.get_shape());
                 dest_height += item_matrix[y*cols + x].get_height();
               }
            dest.buffer.reserve(dest_height);
          }

          loop(y, rows)
              {
                if (huge && (ii_count != interrupt_count))   goto interrupted;

                // insert separator row(s)
                //
                const ShapeItem srows = separator_rows(y, value.get_shape());
                if (srows)   // unless first row
                   {
                    const UCS_string sepa_row(dest.get_width(0), UNI_iPAD_L0);
                    loop(r, srows)   dest.append_ucs(sepa_row);
                   }

                const PrintBuffer & src = item_matrix[y*cols + x];
                dest.add_row(src);
              }

          bool not_char = false;   // determined by get_col_spacing()
	  const int col_spacing = value.get_col_spacing(not_char, x, framed);

          const int32_t max_spacing = (col_spacing > last_col_spacing) 
                                    ?  col_spacing : last_col_spacing;
          int32_t not_char_spaces = 0;

          if (huge && (ii_count != interrupt_count))   goto interrupted;

          if (x)   // subsequent column
             {
               if (last_notchar)
                  {
                    // the previous column was 'notchar' so we add one pad
                    // char to it.
                    //
                    pcols[x - 1].pad_r(UNI_iPAD_U2, 1);
                    ++not_char_spaces;
                  }
               else if (not_char)
                  {
                    // the current column is 'notchar' so we add one pad
                    // char to it.
                    //
                    dest.pad_l(UNI_iPAD_U3, 1);
                    ++not_char_spaces;
                  }

               // we want a total spacing of 'max_spacing'
               // but we deduct the 'not_char_spaces' chars ² and ³
               // that were appended above.
               //
               const int u7_pad_len = max_spacing - not_char_spaces;
               if (u7_pad_len)
                  {
                     pcols[x - 1].pad_r(UNI_iPAD_U7, u7_pad_len);
                  }
             }

          if (huge && (ii_count != interrupt_count))   goto interrupted;

          last_col_spacing = col_spacing;
          last_notchar = not_char;
        }

     // combine columns
     //
     for (int dx = 1; dx < cols; dx += dx)
     for (int xx = dx; xx < cols; xx += dx)
         {
           pcols[xx - dx].append_col(pcols[xx]);
         }
     *this = pcols[0];

     if (value.compute_depth() > 1 && !framed)
        {
          pad_l(UNI_iPAD_U8, 1);
          pad_r(UNI_iPAD_U9, 1);
        }

     if (!is_rectangular())   // should not happen
        {
          Q1(get_height())
          loop(h, get_height())   CERR << "w=" << get_width(h) << "*" << endl;
          loop(h, get_height())   CERR << "*"  << get_line(h) << "*" << endl;
        }

     Assert(is_rectangular());
     add_outer_frame(outer_style);

     PERFORMANCE_END(fs_PrintBuffer4_B, start_4, ec)
   }

   {
     PERFORMANCE_START(start_5)

     if (huge)   // ergo: out
        {
          print_interruptible(*out, value.get_rank(), pctx.get_PW());
        }
     else if (out)
        {
          UCS_string ucs(*this, value.get_rank(), pctx.get_PW());
          if (ucs.size())   *out << ucs << endl;
        }
     complete = true;

     PERFORMANCE_END(fs_PrintBuffer5_B, start_5, ec)
   }

   return;

interrupted:
   attention_raised = false;
   interrupt_raised = false;
   *out << endl << "INTERRUPT" << endl;
}
//-----------------------------------------------------------------------------
void
PrintBuffer::print_interruptible(ostream & out, Rank rank, int quad_PW)
{
   if (get_height() == 0)   return;      // empty PrintBuffer

const int total_width = get_width(0);
const int max_breaks = 2*total_width/quad_PW;

ShapeItem * del = 0;
ShapeItem __breakpoints[PB_MAX_BREAKPOINTS];
ShapeItem * breakpoints = __breakpoints;
   if (max_breaks >= PB_MAX_BREAKPOINTS)
      breakpoints = del = new ShapeItem[max_breaks];
ShapeItem bp_len = 0;

   // print rows, breaking at breakpoints
   //
   loop(row, get_height())
       {
         if (row)   out << endl;   // end previous row
         int col = 0;
         int b = 0;

         while (col < total_width)
            {
              int chunk_len;
              if (row == 0)   // first row: set up breakpoints
                 {
                   chunk_len = get_line(0).compute_chunk_length(quad_PW, col);
                   breakpoints[bp_len++] = chunk_len;
                 }
              else            // subsequent row: re-use breakpoints
                 {
                   chunk_len = breakpoints[b++];
                 }

              if (col)   out << endl << "      ";
              UCS_string trow(get_line(row), col, chunk_len);
              trow.remove_trailing_padchars();

              loop(t, trow.size())
                  {
                     const Unicode uni = trow[t];
                     if (is_iPAD_char(uni))   out << " ";
                     else                     out << uni;
                  }
              col += chunk_len;

              if (interrupt_raised)
                 {
                   out << endl << "INTERRUPT" << endl;
                   attention_raised = false;
                   interrupt_raised = false;
                   delete del;
                   return;
                 }
            }
       }

   out << endl;
   delete del;
}
//-----------------------------------------------------------------------------
void
PrintBuffer::pb_empty(const Value & value, PrintContext pctx, 
                             PrintStyle outer_style)
{
   if (value.get_rank() == 1)   // vector: 1 line
      {
        if (pctx.get_style() == PR_APL_FUN)
           {
             if (value.get_ravel(0).is_character_cell())   // ''
                {
                  UCS_string ucs("''");
                  ColInfo ci;
                  *this = PrintBuffer(ucs, ci);
                  add_outer_frame(outer_style);
                  return;
                }

             if (value.get_ravel(0).is_numeric())   // ⍬
                {
                  UCS_string ucs("⍬");
                  ColInfo ci;
                  *this = PrintBuffer(ucs, ci);
                  add_outer_frame(outer_style);
                  return;
                }
           }

        UCS_string ucs;   // empty
        append_ucs(ucs);
        add_outer_frame(outer_style);
        return;   // 1 row
      }

const Shape sh = value.get_shape().without_axis(value.get_rank()-1);
   if (sh.get_volume() <= 1)   // empty vector
      {
        add_outer_frame(outer_style);
        return;   // 0 rows
      }

   // value has > 0 rows. Compute how many lines we need.
   //
ShapeItem lines = sh.get_volume();
   loop(s, sh.get_rank())
      lines += s * (sh.get_shape_item(sh.get_rank() - s - 1));

   buffer.resize(lines);
   add_outer_frame(outer_style);
}
//-----------------------------------------------------------------------------
void
PrintBuffer::pb_for_function(const Value & value, PrintContext pctx, 
                             PrintStyle outer_style)
{
const ShapeItem ec = value.element_count();
UCS_string ucs;

   if (value.is_char_vector())
      {
        ucs.append(UNI_SINGLE_QUOTE);
        loop(e, ec)
           {
             const Unicode uni = value.get_ravel(e).get_char_value();
             ucs.append(uni);
             if (uni == UNI_SINGLE_QUOTE)   ucs.append(uni);   // ' -> ''
           }
        ucs.append(UNI_SINGLE_QUOTE);
      }
   else
      {
        loop(e, ec)
           {
             PrintBuffer pb = value.get_ravel(e)
                                          .character_representation(pctx);
             if (e)   ucs.append(UNI_ASCII_SPACE);
             ucs.append(UCS_string(pb, 0, pctx.get_PW()));
           }
      }

ColInfo ci;
   *this = PrintBuffer(ucs, ci);
   add_outer_frame(outer_style);
}
//-----------------------------------------------------------------------------
ShapeItem
PrintBuffer::separator_rows(ShapeItem y, const Shape & shape)
{
   if (y == 0)   return 0;

ShapeItem prod = 1;
ShapeItem ret = 0;
   loop(r, shape.get_rank() - 2)
       {
         prod *= shape.get_shape_item(shape.get_rank() - r - 2);
         if (y % prod == 0)   ++ret;
         else                 break;
       }

   return ret;
}
//-----------------------------------------------------------------------------
Unicode
PrintBuffer::get_char(uint32_t x, uint32_t y) const
{ 
   Assert(y < buffer.size());
   Assert(x < buffer[y].size());
   return buffer[y][x];
}
//-----------------------------------------------------------------------------
void
PrintBuffer::set_char(uint32_t x, uint32_t y, Unicode uc)
{
   Assert(y < buffer.size());
   Assert(x < buffer[y].size());
   buffer[y][x] = uc;
}
//-----------------------------------------------------------------------------
void
PrintBuffer::pad_l(Unicode pad, ShapeItem count)
{
   loop(y, get_height())   buffer[y].insert(0, count, pad);
}
//-----------------------------------------------------------------------------
void
PrintBuffer::pad_r(Unicode pad, ShapeItem count)
{
UCS_string ucs(count, pad);
   loop(y, get_height())   buffer[y].append(ucs);
}
//-----------------------------------------------------------------------------
void
PrintBuffer::pad_height(Unicode pad, ShapeItem height)
{
   if (height > get_height())
      {
        UCS_string ucs(get_width(0), pad);
        while (height > get_height())   buffer.push_back(ucs);
      }
}
//-----------------------------------------------------------------------------
void
PrintBuffer::get_frame_chars(PrintStyle pst,
                             Unicode & HORI, Unicode & VERT,
                             Unicode & NW, Unicode & NE,
                             Unicode & SE, Unicode & SW)
{
   switch(pst & PST_CS_MASK)
      {
        case PST_CS_ASCII:
             HORI = UNI_ASCII_MINUS;
             VERT = UNI_ASCII_BAR;
             NW   = UNI_ASCII_FULLSTOP;
             NE   = UNI_ASCII_FULLSTOP;
             SE   = UNI_SINGLE_QUOTE;
             SW   = UNI_SINGLE_QUOTE;
             break;

        case PST_CS_THIN:
             HORI = UNI_LINE_HORI;
             VERT = UNI_LINE_VERT;
             NW   = UNI_LINE_DOWN_LEFT;
             NE   = UNI_LINE_DOWN_RIGHT;
             SE   = UNI_LINE_UP_RIGHT;
             SW   = UNI_LINE_UP_LEFT;
             break;

        case PST_CS_THICK:
             HORI = UNI_LINE_HORI2;
             VERT = UNI_LINE_VERT2;
             NW   = UNI_LINE_DOWN2_LEFT2;
             NE   = UNI_LINE_DOWN2_RIGHT2;
             SE   = UNI_LINE_UP2_RIGHT2;
             SW   = UNI_LINE_UP2_LEFT2;
             break;

        case PST_CS_DOUBLE:
             HORI = UNI_LINE2_HORI;
             VERT = UNI_LINE2_VERT;
             NW   = UNI_LINE2_DOWN_LEFT;
             NE   = UNI_LINE2_DOWN_RIGHT;
             SE   = UNI_LINE2_UP_RIGHT;
             SW   = UNI_LINE2_UP_LEFT;
             break;

        default:
             HORI = Unicode_0;
             VERT = Unicode_0;
             NW   = Unicode_0;
             NE   = Unicode_0;
             SE   = Unicode_0;
             SW   = Unicode_0;
             FIXME;
      }
}
//-----------------------------------------------------------------------------
void
PrintBuffer::add_frame(PrintStyle style, const Shape & shape, uint32_t depth)
{
   Assert(is_rectangular());

Unicode HORI, VERT, NW, NE, SE, SW;
   get_frame_chars(style, HORI, VERT, NW, NE, SE, SW);

   if (get_height() == 0)   // empty
      {
        UCS_string upper;
        upper.append(NE);
        upper.append(NW);
        buffer.push_back(upper);

        UCS_string lower;
        lower.append(SE);
        lower.append(SW);
        buffer.push_back(lower);

        Assert(is_rectangular());
        return;
      }

   // draw | left and right
   //
   loop(y, get_height())
      {
        buffer[y].insert(0, 1, VERT);
        buffer[y].append(VERT);

        // change internal pad characters to ASCII_SPACE so that they will
        // not be removed later and the frame is printed correctly
        //
        buffer[y].map_pad();

      }

   // draw - on top and bottom.
   //
UCS_string hori(get_width(0), HORI);

   buffer.insert(buffer.begin(), hori);
   buffer.push_back(hori);

   // draw the corners
   //
   set_char(0,                0,                NE);
   set_char(0,                get_height() - 1, SE);
   set_char(get_width(0) - 1, 0,                NW);
   set_char(get_width(0) - 1, get_height() - 1, SW);

   // draw other decorators
   //
   if (shape.get_rank() > 0)
      {
        set_char(1, 0, UNI_RIGHT_ARROW);
        if (style & PST_NARS)
           {
             UCS_string ucs;
             ucs.append_number(shape.get_last_shape_item());
             if (ucs.size() < (get_width(0) - 2))
                {
                  loop(u, ucs.size())   set_char(u + 1, 0, ucs[u]);
                }
           }
      }

   if (shape.get_rank() > 1)
      {
        set_char(0, 1, UNI_DOWN_ARROW);
        if (style & PST_NARS)
           {
             UCS_string ucs;
             loop(r, shape.get_rank() - 1)
                {
                  if (r)   ucs.append(VERT);
                  ucs.append_number(shape.get_shape_item(r));
                }
             if (ucs.size() < (get_height() - 2))
                {
                  loop(u, ucs.size())   set_char(0, u + 1, ucs[u]);
                }
           }
      }

   if (depth)
      {
        loop(d, depth - 1)
            if (d + 1 < get_width(0) - 1)
               set_char(1 + d, get_height() - 1, UNI_ELEMENT1);
      }

   if (style & PST_EMPTY_LAST)   // last (X-) dimension is empty
      {
        set_char(1, 0, UNI_CIRCLE_BAR);   // ⊖
      }

   if (style & PST_EMPTY_NLAST)   // a non-last (Y-) dimension is empty
      {
        set_char(0, 1, UNI_CIRCLE_STILE);   // ⌽
      }

   Assert(is_rectangular());
}
//-----------------------------------------------------------------------------
void
PrintBuffer::add_outer_frame(PrintStyle style)
{
   style = PrintStyle(style >> 4 & PST_CS_MASK);
   if (style == PST_CS_NONE)   return;

Unicode HORI, VERT, NW, NE, SE, SW;
   get_frame_chars(style, HORI, VERT, NW, NE, SE, SW);

   if (get_height() == 0)   // empty
      {
        UCS_string upper;
        upper.append(NE);
        upper.append(NW);
        buffer.push_back(upper);

        UCS_string lower;
        lower.append(SE);
        lower.append(SW);
        buffer.push_back(lower);

        Assert(is_rectangular());
        return;
      }

   // draw a bar left and right
   //
   loop(y, get_height())
      {
        buffer[y].insert(0, 1, VERT);
        buffer[y].append(VERT);

        // change internal pad characters to ASCII_SPACE so that they will
        // not be removed later and the frame is printed correctly
        //
        buffer[y].map_pad();
      }

   // draw a bar on top and bottom.
   //
UCS_string hori(get_width(0), HORI);

   buffer.insert(buffer.begin(), hori);
   buffer.push_back(hori);

   // draw the corners
   //
   set_char(0,                0,                NE);
   set_char(0,                get_height() - 1, SE);
   set_char(get_width(0) - 1, 0,                NW);
   set_char(get_width(0) - 1, get_height() - 1, SW);

   Assert(is_rectangular());
}
//-----------------------------------------------------------------------------
ostream &
PrintBuffer::debug(ostream & out, const char * title) const
{
   if (title)   out << title << endl;

   if (get_height() == 0)
      {
        out << UNI_LINE_DOWN_RIGHT << UNI_LINE_DOWN_LEFT << endl
            << UNI_LINE_UP_RIGHT   << UNI_LINE_UP_LEFT
            << "  flags=" << HEX(col_info.flags)
            << "  len="  << col_info.int_len
            << "."   << col_info.fract_len
            << endl << endl;
        return out;
      }

   out << UNI_LINE_DOWN_RIGHT;
   loop(w, get_width(0))   out << UNI_LINE_HORI;
   out << UNI_LINE_DOWN_LEFT << endl;

   loop(y, get_height())
   out << UNI_LINE_VERT << buffer[y] << UNI_LINE_VERT << endl;

   out << UNI_LINE_UP_RIGHT;
   loop(w, get_width(0))   out << UNI_LINE_HORI;
   out << UNI_LINE_UP_LEFT
       << "  flags=" << HEX(col_info.flags)
       << "  ilen="  << col_info.int_len
       << "  flen="   << col_info.fract_len
       << "  rlen="   << col_info.real_len
       << endl << endl;

   return out;
}
//-----------------------------------------------------------------------------
void
PrintBuffer::append_col(const PrintBuffer & pb1)
{
   Assert(get_height() == pb1.get_height());

   loop(h, get_height())   buffer[h].append(pb1.buffer[h]);
}
//-----------------------------------------------------------------------------
void
PrintBuffer::append_ucs(const UCS_string & ucs)
{
   if (buffer.size() == 0)   // empty
      {
        buffer.push_back(ucs);
      }
   else if (ucs.size() < get_width(0))
      {
        UCS_string ucs1(ucs);
        UCS_string pad(get_width(0) - ucs.size(), UNI_iPAD_L1);
        ucs1.append(pad);
        buffer.push_back(ucs1);
      }
   else if (ucs.size() > get_width(0))
      {
        UCS_string pad(ucs.size() - get_width(0), UNI_iPAD_L2);
        loop(h, get_height())   buffer[h].append(pad);
        buffer.push_back(ucs);
      }
   else
      {
        buffer.push_back(ucs);
      }
}
//-----------------------------------------------------------------------------
void
PrintBuffer::append_aligned(const UCS_string & ucs, Unicode align)
{
   Assert(is_rectangular());

int ucs_pos = -1;
   loop(u, ucs.size())
      {
        if (ucs[u] == align)
           {
             ucs_pos = u;
             break;
           }
      }

int this_pos = -1;
   loop(y, buffer.size())
      {
        const UCS_string & row = buffer[y];
        loop(u, row.size())
          {
            if (row[u] == align)
               {
                 this_pos = u;
                 break;
               }
          }
        if (this_pos != -1)   break;
      }

const int ucs_w = ucs.size();
const int this_w = get_width(0);

int ucs_l = 0;    // padding left of ucs
int ucs_r = 0;    // padding right of ucs
int this_l = 0;   // padding left of this
int this_r = 0;   // padding right of this

   if (ucs_pos == -1)       // no align char in ucs
      if (this_pos == -1)   // no align char in this pb
         {
           // no align char at all: align at right border
           //
           //      TTTTttt
           //      UUUUuuu
           //
           if (this_w > ucs_w)   ucs_l = this_w - ucs_w;
           else                  this_l = ucs_w - this_w;
         }
      else                  // align char only in this pb
         {
           //
           //      TTTTttt.ttt
           //      UUUUuuu
           //
           ucs_r = this_w - this_pos;
           if (this_pos > ucs_w)   ucs_l = this_pos - ucs_w;
           else                    this_l = ucs_w - this_pos;
         }
   else                     // align char in ucs
      if (this_pos == -1)   // no align char in this pb
         {
           //
           //      TTTTttt
           //      UUUUuuu.uuu
           //
           this_r = ucs_w - ucs_pos;
           if (ucs_pos > this_w)   this_l = ucs_pos - this_w;
           else                    ucs_l = this_w - ucs_pos;
         }
      else                  // align char in this pb
         {
           //
           //      TTTTttt.tttTTTT
           //      UUUUuuu.uuuUUUU
           //
           if (ucs_pos > this_pos)   this_l = ucs_pos - this_pos;
           else                      ucs_l = this_pos - ucs_pos;
           const int uu = ucs_w - ucs_pos;
           const int tt = this_w - this_pos;
           if (uu > tt)   this_r = uu - tt;
           else           ucs_r = tt - uu;
         }

   Assert(ucs_l >= 0);
   Assert(ucs_r >= 0);
   Assert(this_l >= 0);
   Assert(this_r >= 0);

UCS_string ucs1;

   if (ucs_l > 0)   ucs1.append(UCS_string(ucs_l, UNI_ASCII_SPACE));
   ucs1.append(ucs);
   if (ucs_r > 0)   ucs1.append(UCS_string(ucs_r, UNI_ASCII_SPACE));

   if (this_l > 0)   pad_l(UNI_ASCII_SPACE, this_l);
   if (this_r > 0)   pad_r(UNI_ASCII_SPACE, this_r);

   buffer.push_back(ucs1);

   Assert(is_rectangular());
}
//-----------------------------------------------------------------------------
void
PrintBuffer::add_column(Unicode pad, int32_t pad_count, const PrintBuffer & pb)
{
   if (get_height() != pb.get_height())
      {
         debug(CERR, "this");
         pb.debug(CERR, "pb");
      }

   Assert(get_height() == pb.get_height());

   if (pad_count)
      {
        UCS_string ucs(pad_count, pad);
        loop(y, get_height())   buffer[y].append(ucs);
      }

   loop(y, get_height())   buffer[y].append(pb.buffer[y]);
}
//-----------------------------------------------------------------------------
void PrintBuffer::add_row(const PrintBuffer & pb)
{
   buffer.reserve(buffer.size() + pb.get_height());
   loop(h, pb.get_height())   buffer.push_back(pb.buffer[h]);
}
//-----------------------------------------------------------------------------
void
PrintBuffer::align(ColInfo & cols)
{
   // this PrintBuffer is a (possibly nested) APL value.
   // Align the buffer:
   //
   // to the J (in a column containing complex numbers), or
   // to the decimal point (in a column containing non-complex numbers), or
   // to the left (in a column containing text or nested values).
   //
   // make sure that the item is (and remains) rectangular).
   //
   Assert(is_rectangular());

   if      (cols.flags & has_j)          align_j(cols);
   else if ((cols.flags & CT_NUMERIC))   align_dot(cols);
   else                                  align_left(cols);

   Assert(is_rectangular());
}
//-----------------------------------------------------------------------------
void
PrintBuffer::align_dot(ColInfo & COL_INFO)
{
   Log(LOG_printbuf_align)
      {
        CERR << "before align_dot(), COL_INFO = " << COL_INFO.int_len
             << ":" << COL_INFO.fract_len
             << ":" << COL_INFO.real_len
             << ", this col = " << col_info.int_len
             << ":" << col_info.fract_len
             << ":" << col_info.real_len << endl;
        debug(CERR, 0);
      }

   Assert(buffer.size() > 0);

   Assert(COL_INFO.int_len   >= col_info.int_len);
   Assert(COL_INFO.fract_len >= col_info.fract_len);
   Assert(COL_INFO.real_len  >= col_info.real_len);

   if (col_info.flags & CT_NUMERIC)
      {
        // dot-align numeric items
        if (COL_INFO.int_len > col_info.int_len)
           {
             const size_t diff = COL_INFO.int_len - col_info.int_len;
             pad_l(UNI_iPAD_L3, diff);
             col_info.real_len += diff;
             col_info.int_len = COL_INFO.int_len;
           }

        if (COL_INFO.fract_len > col_info.fract_len)
           {
             pad_fraction(COL_INFO.fract_len, COL_INFO.have_expo());
           }

        if (COL_INFO.real_len > col_info.real_len)
           {
             const size_t diff = COL_INFO.real_len - col_info.real_len;
             if (col_info.have_expo())   // alrady have an expo field
                {
                  pad_r(UNI_iPAD_L4, diff);
                }
             else                        // no expo yet: create one
                {
                  Assert1(diff >= 2);
                  pad_r(UNI_ASCII_E, 1);
                  pad_r(UNI_ASCII_0, 1);
                  pad_r(UNI_iPAD_L4, diff - 2);
                }
             col_info.real_len = COL_INFO.real_len;
           }
      }
   else
      {
        // right-align char items
        size_t LEN = COL_INFO.total_len();
        size_t len = col_info.total_len();
        if (LEN > len)
           {
             const size_t diff = LEN - len;
             pad_l(UNI_iPAD_L5, diff);
             col_info.int_len   = COL_INFO.int_len;
             col_info.fract_len = COL_INFO.fract_len;
             col_info.real_len = COL_INFO.real_len;
           }
      }

   Log(LOG_printbuf_align)   debug(CERR, "after align_dot()");
}
//-----------------------------------------------------------------------------
void
PrintBuffer::align_j(ColInfo & COL_INFO)
{
   Log(LOG_printbuf_align)
      {
        CERR << "before align_j(), COL_INFO = " << COL_INFO.int_len
             << ":" << COL_INFO.fract_len
             << ":" << COL_INFO.real_len
             << ", this col = " << col_info.int_len
             << ":" << col_info.fract_len
             << ":" << col_info.real_len << endl;
        debug(CERR, 0);
      }

   Assert(buffer.size() > 0);

   Assert(COL_INFO.real_len >= col_info.real_len);
   Assert(COL_INFO.imag_len >= col_info.imag_len);

   if (col_info.flags & CT_NUMERIC)
      {
        // J-align numeric items
        if (COL_INFO.real_len > col_info.real_len)
           {
             const size_t diff = COL_INFO.real_len - col_info.real_len;
             pad_l(UNI_iPAD_L3, diff);
             col_info.real_len = COL_INFO.real_len;
           }

        if (COL_INFO.imag_len > col_info.imag_len)
           {
             const size_t diff = COL_INFO.imag_len - col_info.imag_len;
             pad_r(UNI_iPAD_L4, diff);
             col_info.imag_len = COL_INFO.imag_len;
           }
      }
   else
      {
        // right-align char items
        size_t LEN = COL_INFO.real_len + COL_INFO.imag_len;
        size_t len = col_info.real_len + col_info.imag_len;
        if (LEN > len)
           {
             const size_t diff = LEN - len;
             pad_l(UNI_iPAD_L5, diff);
             col_info.real_len = COL_INFO.real_len;
             col_info.imag_len = COL_INFO.imag_len;
           }
      }

   Log(LOG_printbuf_align)   debug(CERR, "after align_j()");
}
//-----------------------------------------------------------------------------
void
PrintBuffer::pad_fraction(int wanted_fract_len, bool want_expo)
{
const int diff = wanted_fract_len - col_info.fract_len;
   Assert1(diff > 0);

      // copy integer part of this PrintBuffer to to new_buf
      //
UCS_string new_buf(buffer[0], 0, col_info.int_len);

      // copy fractional part to new_buf. If the number has no exponent part,
      // then we fill with spaces. Otherwise fill with '0', possibly inserting
      // a decimal point.
      //
      loop(f, col_info.fract_len)
          new_buf.append(buffer[0][col_info.int_len + f]);
      if (!want_expo)
         {
           loop(d, diff)   new_buf.append(UNI_iPAD_L4);
         }
      else if (col_info.fract_len == 0)   // no fractional part yet
         {
           new_buf.append(UNI_ASCII_FULLSTOP);
           loop(d, diff - 1)   new_buf.append(UNI_ASCII_0);
         }
      else
         {
           loop(d, diff)   new_buf.append(UNI_ASCII_0);
         }

   // copy exponent part
   //
   for (int ex = col_info.int_len + col_info.fract_len;
        ex < buffer[0].size(); ++ex)
       new_buf.append(buffer[0][ex]);

   col_info.fract_len = wanted_fract_len;
   col_info.real_len += diff;

   buffer[0] = new_buf;

   // if buffer is multi-len then pad remaining line to new length
   //
   for (ShapeItem h = 1; h < get_height(); ++h)
      {
        const int diff = new_buf.size() - get_width(h);
        if (diff > 0)
           {
             const UCS_string ucs(diff, UNI_iPAD_L4);
             buffer[h].append(ucs);
           }
      }
}
//-----------------------------------------------------------------------------
void
PrintBuffer::align_left(ColInfo & COL_INFO)
{
   Log(LOG_printbuf_align)
      {
        CERR << "before align_left(), COL_INFO = " << COL_INFO.int_len
             << ":" << COL_INFO.fract_len
             << ":" << COL_INFO.real_len
             << ", this col = " << col_info.int_len
             << ":" << col_info.fract_len
             << ":" << col_info.real_len << endl;
        debug(CERR, 0);
      }

   if (col_info.int_len == COL_INFO.int_len)   return;   // no padding needed.

   Assert(col_info.int_len < COL_INFO.int_len);

const size_t diff = COL_INFO.int_len - col_info.int_len;

   if (buffer.size())   pad_r(UNI_iPAD_L3, diff);
   else                 buffer.push_back(UCS_string(diff, UNI_iPAD_L3));

   col_info.int_len = COL_INFO.int_len;

   Log(LOG_printbuf_align)   debug(CERR, "after align_left()");
}
//-----------------------------------------------------------------------------
bool
PrintBuffer::is_rectangular() const
{
   if (get_height())
      {
        const ShapeItem w = get_width(0);
        loop(h, get_height())
           {
             if (get_width(h) != w)    return false;
           }
      }

   return true;
}
//-----------------------------------------------------------------------------
void
ColInfo::consider(ColInfo & other)
{
   flags |= other.flags;
   if (other.imag_len)   flags |= has_j;

const int expo_len = real_len - fract_len - int_len;
const int expo_len1 = other.real_len - other.fract_len - other.int_len;

   if (int_len   < other.int_len)
      {
        real_len += other.int_len - int_len;
           int_len = other.int_len;
      }

   if (fract_len < other.fract_len)
      {
        real_len += other.fract_len - fract_len;
        fract_len = other.fract_len;
      }

   real_len = int_len + fract_len + expo_len;
   if (expo_len < expo_len1)    real_len += expo_len1 - expo_len;

   if (imag_len  < other.imag_len)    imag_len  = other.imag_len;
}
//-----------------------------------------------------------------------------
ostream &
operator << (ostream & out, const PrintBuffer & pb)
{
   out << endl;   return pb.debug(out);
}
//-----------------------------------------------------------------------------
