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

#ifndef __HEAPSORT_HH_DEFINED__
#define __HEAPSORT_HH_DEFINED__

#include <stdint.h>

template<typename T>
class Heapsort
{
public:
   /// a function to compare two items. The function returns true if item_a
   /// is larger than item_b. comp_arg is some additional argument guiding
   /// the comparison
   typedef bool (*greater_fun)(T item_a, T item_b, const void * comp_arg);

   /// sort a
   static void sort(T * a, int64_t heapsize, const void * comp_arg,
                    greater_fun gf)
      {
        // Sort a[] into a heap.
        //
        init_heap(a, heapsize, comp_arg, gf);

        // here a[] is a heap, with a[0] being the largest node.

        for (int k = heapsize - 1; k > 0; k--)
            {
              // The root a[0] is the largest element (in a[0]..a[k]).
              // Store the root, replace it by another element.
              //
              const T t = a[k];   a[k] = a[0];   a[0] = t;

              // Sort a[] into a heap again.
              //
              make_heap(a, k, 0, comp_arg, gf);
            }
      }

protected:
   /// sort a[] into a heap
   static void init_heap(T * array, int64_t heapsize,
                         const void * comp_arg, greater_fun gf)
      {
        for (int64_t p = heapsize/2 - 1; p >= 0; --p)
            make_heap(array, heapsize, p, comp_arg, gf);

         // here a is a heap, i.e. a[i] >= a[2i+1], a[2i+2]
      }

   /// sort subtree starting at a[i] into a heap
   static void make_heap(T * a, int64_t heapsize, int64_t i,
                         const void * comp_arg, greater_fun gf)
      {
        const int64_t l = 2*i + 1;   // left  child of i.
        const int64_t r = l + 1;     // right child of i.
        int64_t max = i;   // assume parent is the max.

        // set max to the position of the largest of a[i], a[l], and a[r]
        //
        if ((l < heapsize) && (*gf)(a[l], a[max], comp_arg))
           max = l;   // left child is larger

        if ((r < heapsize) && (*gf)(a[r], a[max], comp_arg))
           max = r;   // right child is larger

        if (max != i)   // parent was not the max: exchange it.
           {
             const T t = a[max];   a[max] = a[i];   a[i] = t;
             make_heap(a, heapsize, max, comp_arg, gf);
           }
      }
};

#endif // __HEAPSORT_HH_DEFINED__
