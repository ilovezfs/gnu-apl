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

#include "Assert.hh"
#include "Bif_F12_SORT.hh"
#include "Cell.hh"
#include "Heapsort.hh"
#include "Value.icc"
#include "Workspace.hh"

Bif_F12_SORT_ASC  Bif_F12_SORT_ASC::fun;     // ⍋
Bif_F12_SORT_DES  Bif_F12_SORT_DES::fun;     // ⍒

//-----------------------------------------------------------------------------
bool
CollatingCache::greater_vec(const Cell * ca, const Cell * cb,
                            const void * comp_arg)
{
CollatingCache & cache = *(CollatingCache *)comp_arg;
const Rank rank = cache.get_rank();

   loop(r, rank)
      {
        loop(c, cache.get_comp_len())
           {
             const APL_Integer a = ca[c].get_int_value();
             const APL_Integer b = cb[c].get_int_value();
             const int d = cache[a].compare(cache[b], true, rank - r - 1);

              if (d)   return d > 0;
           }
      }

   return ca > cb;
}
//-----------------------------------------------------------------------------
bool
CollatingCache::smaller_vec(const Cell * ca, const Cell * cb,
                            const void * comp_arg)
{
CollatingCache & cache = *(CollatingCache *)comp_arg;
const Rank rank = cache.get_rank();

   loop(r, rank)
      {
        loop(c, cache.get_comp_len())
           {
             const APL_Integer a = ca[c].get_int_value();
             const APL_Integer b = cb[c].get_int_value();
             const int d = cache[a].compare(cache[b], true, rank - r - 1);

              if (d)   return d < 0;
           }
      }

   return ca > cb;
}
//-----------------------------------------------------------------------------
int
CollatingCacheEntry::compare(const CollatingCacheEntry & other,
                             bool ascending, Rank axis) const
{
   return  (ascending) ? get_pos(axis) - other.get_pos(axis)
                       : other.get_pos(axis) - get_pos(axis);
}
//=============================================================================
Token
Bif_F12_SORT::sort(Value_P B, bool ascending)
{
   if (B->is_scalar())   RANK_ERROR;

const ShapeItem len_BZ = B->get_shape_item(0);
   if (len_BZ == 0)   return Token(TOK_APL_VALUE1, Idx0(LOC));

const ShapeItem comp_len = B->element_count()/len_BZ;
DynArray(const Cell *, array, len_BZ);
   loop(bz, len_BZ)   array[bz] = &B->get_ravel(bz*comp_len);

   if (ascending)
      Heapsort<const Cell *>::sort(&array[0], len_BZ, &comp_len, &Cell::greater_vec);
   else
      Heapsort<const Cell *>::sort(&array[0], len_BZ, &comp_len, &Cell::smaller_vec);

Value_P Z(new Value(len_BZ, LOC));

const APL_Integer qio = Workspace::get_IO();
const Cell * base = &B->get_ravel(0);
   loop(bz, len_BZ)
       new (Z->next_ravel())   IntCell(qio + (array[bz] - base)/comp_len);

   Z->set_default_Zero();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Bif_F12_SORT::sort_collating(Value_P A, Value_P B, bool ascending)
{
   if (A->is_scalar())   RANK_ERROR;
   if (A->NOTCHAR())     DOMAIN_ERROR;

const APL_Integer qio = Workspace::get_IO();
   if (B->NOTCHAR())     DOMAIN_ERROR;
   if (B->is_scalar())   return Token(TOK_APL_VALUE1, IntScalar(qio, LOC));

const ShapeItem len_BZ = B->get_shape_item(0);
   if (len_BZ == 0)   return Token(TOK_APL_VALUE1, Idx0(LOC));
   if (len_BZ == 1)   return Token(TOK_APL_VALUE1, IntScalar(qio, LOC));

Value_P B1(new Value(B->get_shape(), LOC));
const ShapeItem ec_B = B->element_count();
const ShapeItem comp_len = ec_B/len_BZ;
CollatingCache cc_cache(A->get_rank(), comp_len);
   loop(b, ec_B)
      {
        const Unicode uni = B->get_ravel(b).get_char_value();
        const ShapeItem b1 = collating_cache(uni, A, cc_cache);
        new (&B1->get_ravel(b)) IntCell(b1);
      }

DynArray(const Cell *, array, len_BZ);
   loop(bz, len_BZ)   array[bz] = &B1->get_ravel(bz*comp_len);

   if (ascending)
      Heapsort<const Cell *>::sort(&array[0], len_BZ, &cc_cache,
                                   &CollatingCache::greater_vec);
   else
      Heapsort<const Cell *>::sort(&array[0], len_BZ, &cc_cache,
                                   &CollatingCache::smaller_vec);

Value_P Z(new Value(len_BZ, LOC));

const Cell * base = &B1->get_ravel(0);
   loop(bz, len_BZ)
       new (&Z->get_ravel(bz)) IntCell(qio + (array[bz] - base)/comp_len);

   Z->set_default_Zero();

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
ShapeItem
Bif_F12_SORT::collating_cache(Unicode uni, Value_P A, CollatingCache & cache)
{
   // first check if uni is already in the cache...
   //
   loop(s, cache.size())
      {
        if (uni == cache[s].ce_char)   return s;
      }

   // uni is not in the cache. See if it is in the collating sequence.
   //
const ShapeItem ec_A = A->element_count();
CollatingCacheEntry entry(uni, A->get_shape());
   loop(a, ec_A)
      {
        if (uni != A->get_ravel(a).get_char_value())   continue;

        ShapeItem aq = a;
        loop(r, entry.ce_shape.get_rank())
           {
             const Rank axis = entry.ce_shape.get_rank() - r - 1;
             const ShapeItem ar = aq % A->get_shape_item(axis);
             if (entry.ce_shape.get_shape_item(axis) > ar)
                entry.ce_shape.set_shape_item(axis, ar);
             aq /= A->get_shape_item(axis);
           }
      }

   cache.push_back(entry);
   return cache.size() - 1;
}
//-----------------------------------------------------------------------------

