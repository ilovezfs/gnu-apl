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

#include <iostream>

#include "Common.hh"
#include "ComplexCell.hh"
#include "FloatCell.hh"
#include "Output.hh"
#include "Value.icc"

using namespace std;

// the implementation of gelsy<T>
#include "LApack.hh"

//-----------------------------------------------------------------------------
Value_P
divide_matrix(ShapeItem rows, Value_P A, Value_P B,
              const Shape & shape_Z, APL_Float qct)
{
const bool need_complex = A->is_complex(qct) || B->is_complex(qct);

Value_P Z(new Value(shape_Z, LOC));

const ShapeItem cols_A = shape_Z.get_rows();
const ShapeItem cols_B = shape_Z.get_cols();

   loop(c, cols_A)
       {
         if (need_complex)
            {
              double  ad[2*rows];
              ZZ * const a = (ZZ *)ad;
              loop(r, rows)
                  {
                    new (a + r) ZZ(A->get_ravel(r*cols_A + c).get_real_value(),
                                   A->get_ravel(r*cols_A + c).get_imag_value());
                  }

              double bd[2*B->element_count()];
              ZZ * const b = (ZZ *)bd;
              ZZ * bb = b;
              loop(rr, cols_B)
              loop(cc, rows)
                 {
                   new (bb++) ZZ(B->get_ravel(rr + cc*cols_B)
                                                  .get_real_value(),
                                   B->get_ravel(rr + cc*cols_B)
                                                  .get_imag_value());
                 }

              {
                const double rcond = 1E-10;

                Matrix<ZZ> B(b, rows, cols_B, /* LDB */ rows);
                Matrix<ZZ> A(a, rows, 1,      /* LDA */ rows);
                const ShapeItem rk = gelsy<ZZ>(B, A, rcond);
                if (rk != cols_B)
                   {
                     DOMAIN_ERROR;
                   }
              }

              // cols_A = rows_Z. We have computed the result for col c of A
              // which is row c of Z.
              //
              loop(r, cols_B)
                  new (&Z->get_ravel(r*cols_A + c))
                      ComplexCell(a[r].real(), a[r].imag());
            }
         else   // real
            {
              double a[rows];
              loop(r, rows)
                 {
                   a[r] = A->get_ravel(r*cols_A + c).get_real_value();
                 }

              double b[B->element_count()];
              double * bb = b;
              loop(rr, B->get_cols())
              loop(cc, B->get_rows())
                 {
                   *bb++ = B->get_ravel(rr + cc*B->get_cols()).get_real_value();
                 }

              {
                const double rcond = 1E-10;

                Matrix<DD> B(b, rows, cols_B, /* LDB */ rows);
                Matrix<DD> A(a, rows, 1,      /* LDA */ rows);
                const ShapeItem rk = gelsy<DD>(B, A, rcond);
                if (rk != cols_B)
                   {
                     DOMAIN_ERROR;
                   }
              }

              // cols_A = rows_Z. We have computed the result for col c of A
              // which is row c of Z.
              //

              loop(r, cols_B)
                  new (&Z->get_ravel(r*cols_A + c)) FloatCell(a[r]);
            }
       }

   return Z;
}

