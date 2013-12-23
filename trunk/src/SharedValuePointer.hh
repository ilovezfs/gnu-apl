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

/**
 !!  Currently GNU APL supports 3 different methods for the memory management
 !!  of APL values (i.e. objects of class Value):
 !!
 !!  1. the original method where ownership of a value is claimed by flags.
 !!     If the last owner (corresponding to a bit in  VF_need_clone) clears
 !!     a bit then it is supposed to delete the value.
 !!
 !!     This method has caused a number of segfaults (due to Values released
 !!     too early) in the past and is being phased out for that reason.
 !!
 !!  2. the second method uses std::tr1::shared_ptr<Value> pointers that
 !!     take care of deleting a Value when the last shared_ptr<Value> is
 !!     deleted.
 !!
 !!  3. the third method uses our own shared pointer class that uses a
 !!     reference counter in the Value object itself.
 !!
 !!  Currently all methods are working and Methods 1 is used by default
 !!  (which can be changed below). Methods 1 and 2 will disappear soon.
 **/

#define SHARED_POINTER_METHOD 1

class Value;

#if SHARED_POINTER_METHOD == 1 //////////////////////////////////////////////

#define ptr_clear(p, l) p.clear(l)

class Value_P
{
public:
   /// Constructor: 0 pointer
   Value_P()
   : value_p(0)
   {}

   /// Constructor: from Value *
   Value_P(Value * val)
     : value_p(val)
     {
       if (value_p)
          {
            ADD_EVENT(value_p, VHE_PtrNew, use_count(), LOC);
          }
     }

   /// Constructor: from Value * with deleter
   Value_P(Value * val, void (*Deleter)(Value *))
     : value_p(val)
     {
       if (value_p)
          {
            ADD_EVENT(value_p, VHE_PtrNew, use_count(), "static Value_P");
          }
     }

   /// Constructor: from other Value_P
   Value_P(const Value_P & other)
   : value_p(other.value_p)
   {
     if (value_p)
        {
          ADD_EVENT(value_p, VHE_PtrCopy1, use_count(), LOC);
        }
   }

   /// copy operator
   Value_P & operator =(const Value_P & other)
   {
      if (value_p)   // override existing pointer
         {
           if (value_p == other.value_p)   return *this;   // same pointer

           ADD_EVENT(value_p, VHE_PtrClr, other.use_count(), LOC);
         }
          
      value_p = other.value_p;
      if (value_p)
         {
           ADD_EVENT(value_p, VHE_PtrCopy3, use_count(), LOC);
         }

      return *this;
   }

   int use_count() const
      { return 99; }

   void destruct()
      {
        if (value_p)
           {
             const int count = use_count() - 1;
             ADD_EVENT(value_p, VHE_PtrDel, count, LOC);
          }
      }

   /// Destructor for normal Value_P objects
   ~Value_P()   { destruct(); }

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


   bool operator ==(const Value_P other) const
      { return value_p == other.value_p; }

   bool operator !=(const Value_P other) const
      { return value_p != other.value_p; }

   void clear(const char * loc)
     {
       if (!value_p)   return;

       const int count = use_count() - 1;
       ADD_EVENT(value_p, VHE_PtrClr, count, loc);

       value_p = 0;
    }

protected:
   Value * value_p;
};

#elif SHARED_POINTER_METHOD == 2 //////////////////////////////////////////////

#define ptr_clear(p, l) p.reset()

#include "Backtrace.hh"
#include <tr1/memory>

typedef std::tr1::shared_ptr<Value> Value_P;

#elif SHARED_POINTER_METHOD == 3 //////////////////////////////////////////////

#define ptr_clear(p, l) p.reset()

int  get_owner_count(const Value * v);
void increment_owner_count(Value * v, const char * loc);
void decrement_owner_count(Value * v, const char * loc);

class Value_P
{
public:
   /// Constructor: 0 pointer
   Value_P()
   : value_p(0)
   {}

   /// Constructor: from Value *
   Value_P(Value * val)
     : value_p(val)
     {
       if (value_p)
          {
            increment_owner_count(value_p, LOC);
            ADD_EVENT(value_p, VHE_PtrNew, use_count(), LOC);
          }
     }

   /// Constructor: from Value * with deleter
   Value_P(Value * val, void (*Deleter)(Value *))
     : value_p(val)
     {
       if (value_p)
          {
            increment_owner_count(value_p, "static Value_P");
            ADD_EVENT(value_p, VHE_PtrNew, use_count(), "static Value_P");
          }
     }

   /// Constructor: from other Value_P
   Value_P(const Value_P & other)
   : value_p(other.value_p)
   {
     if (value_p)
        {
          increment_owner_count(value_p, LOC);
          ADD_EVENT(value_p, VHE_PtrCopy1, use_count(), LOC);
        }
   }

   /// copy operator
   Value_P & operator =(const Value_P & other)
   {
      if (value_p)   // override existing pointer
         {
           if (value_p == other.value_p)   return *this;   // same pointer

           decrement_owner_count(value_p, LOC);
           ADD_EVENT(value_p, VHE_PtrClr, other.use_count(), LOC);
         }
          
      value_p = other.value_p;
      if (value_p)
         {
           increment_owner_count(value_p, LOC);
           ADD_EVENT(value_p, VHE_PtrCopy3, use_count(), LOC);
         }

      return *this;
   }

   int use_count() const
      { return get_owner_count(value_p); }

   void destruct()
      {
        if (value_p)
           {
             const int count = use_count() - 1;
             decrement_owner_count(value_p, LOC);
             ADD_EVENT(value_p, VHE_PtrDel, count, LOC);
          }
      }

   /// Destructor for normal Value_P objects
   ~Value_P()   { destruct(); }

   void reset()
      {
        if (value_p)
           {
             decrement_owner_count(value_p, LOC);
             value_p = 0;
           }
      }

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

   void clear(const char * loc)
     {
       if (value_p)
          {
            const int count = use_count() - 1;
            decrement_owner_count(value_p, LOC);
            ADD_EVENT(value_p, VHE_PtrClr, count, loc);

            value_p = 0;
          }
    }

protected:
   Value * value_p;
};

#else /////////////////////////////////////////////////////////////////////////

# error SHARED_POINTER_METHOD must be 1, 2 or 3!

#endif // SHARED_POINTER_METHOD

/// macro to facilitate Value_P in unions
#define VALUE_P(x) char u_ ## x[sizeof(Value_P)]; \
   Value_P & _ ## x() const { return *(Value_P *) & u_ ## x; }

//-----------------------------------------------------------------------------

#endif // __SHARED_VALUE_POINTER_HH_DEFINED__
