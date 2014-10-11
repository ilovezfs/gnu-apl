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

#ifndef __CDR_HH_DEFINED__
#define __CDR_HH_DEFINED__

#include "CDR_string.hh"

class Value;

/** a class for creating the Common Data Representation of an APL value.
 **/

//-----------------------------------------------------------------------------
class CDR
{
public:
   /// convert \b value into a CDR_string
   static void to_CDR(CDR_string & result, const Value & value);

   /// create a value from a CDR_string, throwing
   /// DOMAIN ERROR if cdr is ill-formed
   static Value_P from_CDR(const CDR_string & cdr, const char * loc);

   /// return 2 bytes starting at \b data in network byte order
   static uint32_t get_2_be(const uint8_t * data)
      {
        uint32_t ret =  *data++;   ret <<= 8;
                 ret |= *data++;   return ret;
      }

   /// return 4 bytes starting at \b data in network byte order
   static uint32_t get_4_be(const uint8_t * data)
      { return CDR_header::get_be32(data); }

   /// return 8 bytes starting at \b data in network byte order
   static uint64_t get_8_be(const uint8_t * data)
      {
        uint64_t ret = get_4_be(data);   ret <<= 32;
                 return ret | get_4_be(data + 4);
      }

protected:
   /// fill result with the bytes of the CDR of \b value
   static void fill(CDR_string & result, int type, int len, const Value & val);
};
//-----------------------------------------------------------------------------

#endif // __CDR_HH_DEFINED__
