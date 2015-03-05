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

/// heapsort an array of items of type \b T
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
        // turn a[] into a heap, i.e. a[i] > a[2i] and a[i] > a[2i+1]
        // for all i.
        //
        for (int64_t p = heapsize/2 - 1; p >= 0; --p)
            make_heap(a, heapsize, p, comp_arg, gf);

        // here a[] is a heap (and therefore a[0] is the largest element)

        for (--heapsize; heapsize > 0; heapsize--)
            {
              // The root a[0] is the largest element in a[0] ... a[k].
              // Exchange a[0] and a[k], decrease the heap size,
              // and re-establish the heap property of the new a[0].
              //
              const T t = a[heapsize];   a[heapsize] = a[0];   a[0] = t;

              // re-establish the heap property of the new a[0]
              //
              make_heap(a, heapsize, 0, comp_arg, gf);
            }
      }

protected:
   /// establish the heap property of the subtree with root a[i]
   static void make_heap(T * a, int64_t heapsize, int64_t parent,
                         const void * comp_arg, greater_fun gf)
      {
        for (;;)
           {
             const int64_t left = 2*parent + 1;   // left  child of parent.
             const int64_t right = left + 1;      // right child of parent.
             int64_t max = parent;                // assume parent is the max.

             // set max to the position of the largest of a[i], a[l], and a[r]
             //
             if ((left < heapsize) && (*gf)(a[left], a[max], comp_arg))
                max = left;   // left child is larger

             if ((right < heapsize) && (*gf)(a[right], a[max], comp_arg))
                max = right;   // right child is larger

             if (max == parent)   return; // parent was the max: done

             // left or right was the max. exchange and continue
             const T t = a[max];   a[max] = a[parent];   a[parent] = t;
             parent = max;
           }
      }
};

#endif // __HEAPSORT_HH_DEFINED__
