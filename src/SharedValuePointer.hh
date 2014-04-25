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

#ifndef __SHARED_VALUE_POINTER_HH_DEFINED__
#define __SHARED_VALUE_POINTER_HH_DEFINED__

#ifndef __COMMON_HH_DEFINED__
#error this file must only be included by Common.hh
#endif

class Value;

#define ptr_clear(p, l) p.reset()

class Value_P
{
public:
   /// Constructor: 0 pointer
   Value_P()
   : value_p(0)
   {}

   /// Constructor: from Value *
   inline Value_P(Value * val);

   /// Constructor: from Value *
   inline Value_P(Value * val, const char * loc);

   inline int use_count() const;

   /// Constructor: from other Value_P
   Value_P(const Value_P & other);

   /// copy operator
   Value_P & operator =(const Value_P & other);

   /// Destructor
   inline ~Value_P();

   inline void reset();

   inline void clear(const char * loc);

   const Value * operator->()  const
      { return value_p; }

   Value * operator->()
      { return value_p; }

   const Value & operator*()  const
      { return *value_p; }

   const Value * get() const
      { return value_p; }

   Value * get()
      { return value_p; }

 bool operator!() const
      { return value_p == 0; }


   bool operator ==(const Value_P & other) const
      { return value_p == other.value_p; }

   bool operator !=(const Value_P & other) const
      { return value_p != other.value_p; }

protected:
   Value * value_p;
};

/// macro to facilitate Value_P in unions
#define VALUE_P(x) char u_ ## x[sizeof(Value_P)]; \
   Value_P & _ ## x() const { return *(Value_P *) & u_ ## x; }

//-----------------------------------------------------------------------------

#endif // __SHARED_VALUE_POINTER_HH_DEFINED__
