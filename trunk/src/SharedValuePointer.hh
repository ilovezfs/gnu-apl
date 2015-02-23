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

#ifndef __SHARED_VALUE_POINTER_HH_DEFINED__
#define __SHARED_VALUE_POINTER_HH_DEFINED__

#ifndef __COMMON_HH_DEFINED__
#error this file must only be included by Common.hh
#endif

class Cell;
class Value;
class Shape;
class CDR_string;
class UCS_string;
class UTF8_string;

#define ptr_clear(p, l) p.reset()

/// a smart pointer to a Value
class Value_P
{
public:
   /// Constructor: 0 pointer
   Value_P()
   : value_p(0)
   {}

   /// a new scalar value
   inline Value_P(const char * loc);

   /// a new scalar value
   inline Value_P(const Cell & cell, const char * loc);

   /// a new vector (rank 1 value) of length len
   inline Value_P(ShapeItem len, const char * loc);

   /// a new value with shape sh
   inline Value_P(const Shape & sh, const char * loc);

   /// a new vector value from a UCS string
   inline Value_P(const UCS_string & ucs, const char * loc);

   /// a new vector value from a UTF8 string
   inline Value_P(const UTF8_string & utf, const char * loc);

   /// a new vector value from a CDR record
   inline Value_P(const CDR_string & cdr, const char * loc);

   /// return the number of Value_P that point to \b value_p
   inline int use_count() const;

   /// Constructor: from other Value_P
   Value_P(const Value_P & other);

   /// copy operator
   Value_P & operator =(const Value_P & other);

   /// Destructor
   inline ~Value_P();

   /// decrement owner-count and reset pointer to 0
   inline void reset();

   /// reset and add value event
   inline void clear(const char * loc);

   /// return a const pointer to the Value (overloaded ->)
   const Value * operator->()  const
      { return value_p; }

   /// return a pointer to the Value (overloaded ->)
   Value * operator->()
      { return value_p; }

   /// return a const reference to the Value
   const Value & operator*()  const
      { return *value_p; }

   /// return a const pointer to the Value
   const Value * get() const
      { return value_p; }

   /// return a pointer to the Value
   Value * get()
      { return value_p; }

   /// return a pointer to the Value
   Value & getref()
      { return *value_p; }

   /// return true if the pointer is invalid
   bool operator!() const
      { return value_p == 0; }

   /// return true if this Value_P points to the same Value as \b other
   bool operator ==(const Value_P & other) const
      { return value_p == other.value_p; }

   /// return true if this Value_P points to a different Value than \b other
   bool operator !=(const Value_P & other) const
      { return value_p != other.value_p; }

protected:
   static inline void decrement_owner_count(Value * & v, const char * loc);
   static inline void increment_owner_count(Value * v, const char * loc);

   /// pointer to the value
   Value * value_p;
};

/// macro to facilitate Value_P in unions
#define VALUE_P(x) /** space for a Value_P **/ char u_ ## x[sizeof(Value_P)]; \
   /** return Value_P **/ Value_P & _ ## x() const { return *(Value_P *) & u_ ## x; }

//-----------------------------------------------------------------------------

#endif // __SHARED_VALUE_POINTER_HH_DEFINED__
