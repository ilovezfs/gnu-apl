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

#ifndef __CDR_STRING_HH_DEFINED__
#define __CDR_STRING_HH_DEFINED__

#include "Simple_string.hh"

/// the header of a CDR record
struct CDR_header
{
   uint32_t be_ptr;    ///< a value to figure the endian of the machine
   uint32_t be_nb;     ///< number of bytes in big endian
   int32_t be_nelm;   ///< number of elements in big endian
   uint8_t  type;      ///< CDR_type
   uint8_t  rank;      ///< rank of the value
   uint8_t  fill[2];   ///< fill bytes

   // some compilers do not allow zero -sized arrays like dim[].
   // Since dim is not used we just ignore it.
   //
// uint32_t dim[0];    ///< shape of the value

   /// return the number of bytes in host endian
   uint32_t get_nb() const
      { return get_be32((const uint8_t *)&be_nb); }

   /// return the number of elements in host endian
   uint32_t get_nelm() const
      { return get_be32((const uint8_t *)&be_nelm); }

   /// convert big endian 32 bit value to 32 bit host value (== ntohl())
   static uint32_t get_be32(const uint8_t * data)
      {
        uint32_t ret  =  *data++;  ret <<= 8;
                 ret |= *data++;   ret <<= 8;
                 ret |= *data++;   ret <<= 8;
                 ret |= *data++;   return ret;

      }
};

/// a string containing a CDR record
class CDR_string : public Simple_string<uint8_t>
{
public:
   /// Constructor: An uninitialized CDR structure
   CDR_string()
   : Simple_string<uint8_t>((const uint8_t *)0, 0)
   {}

   /// Constructor: CDR structure from uint8_t * and length
   CDR_string(const uint8_t * data, int len)
   : Simple_string<uint8_t>(data, len)
   {}

   /// this cdr as a const CDR_header pointer
   const CDR_header & header() const
      { return *(const CDR_header *)get_items(); }

   /// this cdr as a CDR_header pointer
   CDR_header & header()
      { return *(CDR_header *)get_items(); }

   /// return the tag of this CDR
   int get_ptr() const   { return get_4(0); }

   /// return the CDR type of this CDR
   CDR_type get_type() const
      { return CDR_type(header().type); }

   /// return the rank of this CDR
   int get_rank() const
      { return header().rank; }

   /// return the ravel of this CDR
   const uint8_t * ravel() const
      { return items + 16 + 4*get_rank(); }

   /// return true if this CDR is bool or integer
   bool is_integer() const
      { return get_type() == CDR_BOOL1 || get_type() == CDR_INT32; }

   /// return true if this CDR is character
   bool is_character() const
      { return get_type() == CDR_CHAR8 || get_type() == CDR_CHAR32; }

   /// return non-zero if this CDR is malformed
   int check() const
      {
        uint32_t cnt = 1;
        for (int r = 0; r < get_rank(); ++r)   cnt *= get_4(16 + 4*r);
        if (items == 0)                          return 1;
        if (size() < 32)                         return 2;
        if (get_ptr() != 0x00002020)             return 3;
        if (header().get_nb() != size())         return 4;
        if (header().get_nelm() != cnt)          return 5;
        if (uint32_t(get_type()) > CDR_NEST32)   return 6;
        return 0;
      }

   /// print properties
   void debug(ostream & out, const char * loc = 0) const
      {
        const int error = check();
        out << "CDR record";
        if (loc)   out << " at " << loc;
        if (error)   out << "(error " << error << ")";
        out << ":" << endl;
        loop(i, size())   cerr << " " << hex << setfill('0')
                               << setw(2) << (get_items()[i] & 0xFF);
              cerr << dec << setfill(' ') << endl;
      }

protected:
   /// return 4 bytes of the header (the header is always big endian)
   uint32_t get_4(unsigned int offset) const
      { return CDR_header::get_be32(items + offset); }
};

#endif // __CDR_STRING_HH_DEFINED__
