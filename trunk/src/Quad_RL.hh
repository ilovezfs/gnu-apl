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

#ifndef __QUAD_RL_HH_DEFINED__
#define __QUAD_RL_HH_DEFINED__

#include "SystemVariable.hh"

//-----------------------------------------------------------------------------
/**
   System variable Quad-RL (Random Link).
 */
class Quad_RL : public SystemVariable
{
public:
   /// Constructor.
   Quad_RL();

   /// Return a random number.
   uint64_t get_random();

   enum { INITIAL_SEED = 16807 };

   // Xn+1 = a Xn + c
#define Knuth_a 6364136223846793005
#define Knuth_c 1442695040888963407

   /// reset the seed (eg. after )CEAR)
   unsigned int reset_seed()
      { state = INITIAL_SEED;   return INITIAL_SEED; }

protected:
   /// overloaded Symbol::assign()
   virtual void assign(Value_P value, const char * loc);

   /// overloaded Symbol::pop()
   virtual void pop();

   /// overloaded Symbol::push()
   virtual void push();

   /// state of the random number generator
   static uint64_t state;
};
//-----------------------------------------------------------------------------
#endif //  __QUAD_RL_HH_DEFINED__

