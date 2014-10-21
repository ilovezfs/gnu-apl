/*
    This file is a port of the 'dgelsy' and 'zgelsy' functions (and the
    functions that they call) from liblapack to C++.

    liblapack has the following license/copyright notice,
    see http://www.netlib.org/lapack/LICENSE:

    Copyright (c) 1992-2011 The University of Tennessee and The University
                            of Tennessee Research Foundation.  All rights
                            reserved.
    Copyright (c) 2000-2011 The University of California Berkeley. All
                            rights reserved.
    Copyright (c) 2006-2011 The University of Colorado Denver.  All rights
                            reserved.


    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer. 
  
    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer listed
      in this license in the documentation and/or other materials
      provided with the distribution.
  
    - Neither the name of the copyright holders nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.
  
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT  
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT  
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
*/

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

#ifndef __GELSY_HH_DEFINED__
#define __GELSY_HH_DEFINED__

#ifndef assert
# define assert(x)
#endif

#include <stdio.h>
#include <stdlib.h>

using namespace std;
typedef double DD;

class ZZ
{
public:
   ZZ()
   : _r(0),
     _i(0)
   {}

   ZZ(double r)
   : _r(r),
     _i(0)
   {}

   ZZ(double r, double i)
   : _r(r),
     _i(i)
   {}

   double real() const       { return _r; }
   void set_real(double x)   { _r = x; }

   double imag() const    { return _i; }
   void set_imag(double x)   { _i = x; }

   void conjugate()   { _i = - _i; }

   bool operator != (double dd) const   { return _r != dd || _i != 0; }
   bool operator == (double dd) const   { return _r == dd && _i == 0; }

   ZZ operator +(const ZZ & zz) const
      { return ZZ(_r + zz._r, _i + zz._i); }

   ZZ operator -(const ZZ & zz) const
      { return ZZ(_r - zz._r, _i - zz._i); }

   ZZ operator -() const
      { return ZZ(-_r, -_i); }

   ZZ operator *(const ZZ & zz) const
      { return ZZ(_r * zz._r - _i * zz._i,
                  _i * zz._r + _r * zz._i); }

   ZZ operator /(double d) const
      { return ZZ(_r/d, _i/d); }

   ZZ operator /(const ZZ & zz) const
      { 
        const double denom = zz._r * zz._r + zz._i * zz._i;
         return ZZ((_r * zz._r + _i * zz._i) / denom,
                   (_i * zz._r - _r * zz._i) / denom);
      }

protected:
  double _r;
  double _i;
};

#define MAX(x, y) ((x) >= (y) ? (x) : (y))
#define MIN(x, y) ((x) <= (y) ? (x) : (y))

/// a single double or complex number. This class contains wrappers to
/// double or complex numbers so that they can be used in templates.
class DZ
{
public:
   static bool is_ZZ(DD x)   { return false; }
   static bool is_ZZ(ZZ z)   { return true;  }

   static double get_real(DD x)   { return x;   }
   static double get_imag(DD x)   { return 0.0; }
   static double get_real(ZZ z)   { return z.real(); }
   static double get_imag(ZZ z)   { return z.imag(); }

   static void set_real(DD & y, DD x)   { y = x; }
   static void set_imag(DD & y, DD x)   { assert(x == 0.0); }
   static void set_real(ZZ & y, DD x)   { y.set_real(x); }
   static void set_imag(ZZ & y, DD x)   { y.set_imag(x); }

   static void conjugate(DD x)     { }
   static void conjugate(ZZ & z)   { z.conjugate(); }

   static DD CONJ(DD x)           { return x; }
   static ZZ CONJ(const ZZ & z)   { return ZZ(z.real(), -z.imag()); }

   static DD inv(DD dd)           { return 1.0 / dd; }

   static inline ZZ inv(const ZZ & zz)
      {
        const double denom = zz.real() * zz.real() + zz.imag() * zz.imag();
        return ZZ(zz.real()/denom, - zz.imag()/denom);
      }
};

template<typename T> double REAL(T x)        { return DZ::get_real(x);    }
template<typename T> double IMAG(T x)        { return DZ::get_imag(x);    }
template<typename T> void  SREAL(T & y, double x)   { DZ::set_real(y, x); }
template<typename T> void  SIMAG(T & y, double x)   { DZ::set_imag(y, x); }

template<typename T> void Sri(T & y, double xr, double xi)
   { SREAL<T>(y, xr);   SIMAG<T>(y, xi); }
template<typename T> void Sri(T & y, double xr)
   { SREAL<T>(y, xr);   SIMAG<T>(y, 0); }

inline DD ABS(DD a) { return a < 0 ? -a : a; }
inline DD ABS_2(DD a) { return a*a; }
inline DD ABS(ZZ z) { return sqrt(z.real()*z.real() + z.imag()*z.imag()); }
inline DD ABS_2(ZZ z) { return z.real()*z.real() + z.imag()*z.imag(); }

inline double SIGN(double a, double b)
{
   // return abs(a) with the sign of b
   if (b < 0)  return -ABS(a);
   else        return  ABS(a);
}

template<typename T>
inline void exchange(T & x, T & y)
{
const T tmp = x;
  x = y;
  y = tmp;
}
//-----------------------------------------------------------------------------
template<typename T>
class Vector
{
public:
   Vector(T * _data, ShapeItem _len)
     : data(_data),
       len (_len)
   {}

   Vector sub_len(ShapeItem new_len) const
      { assert(new_len <= len);   return Vector(data, new_len); }

   Vector sub_off(ShapeItem off) const
      { assert(off <= len);   return Vector(data + off, len - off); }

   Vector sub_off_len(ShapeItem off, ShapeItem new_len) const
      { assert((off + new_len) <= len);
        return Vector(data + off, new_len); }

   const ShapeItem get_length() const { return len; }

   bool is_ZZ() const   { return DZ::is_ZZ(*data); }

   double norm() const
      {
//     if (len < 1)   return 0.0;
//     double scale = 0.0;
//     double ssq = 1.0;

        double ret = 0.0;
        const T * dj = data;
        loop(j, len)
           {
             const double re = REAL(*dj);
             if (re != 0)
                {
/***
                  const double temp = ABS(re);
                  if (scale < temp)
                     {
                       const double scale_temp = scale/temp;
                       ssq = 1.0 + ssq*scale_temp*scale_temp;
                       scale = temp;
                     }
                  else
                     {
                       const double temp_scale = temp/scale;
                       ssq += temp_scale*temp_scale;
                     }
***/
                  ret += re*re;
                }

             const double im = IMAG(*dj);
             if (im != 0)
                {
/***
                  const double temp = ABS(im);
                  if (scale < temp)
                     {
                       const double scale_temp = scale/temp;
                       ssq = 1.0 + ssq*scale_temp*scale_temp;
                       scale = temp;
                     }
                  else
                     {
                       const double temp_scale = temp/scale;
                       ssq += temp_scale*temp_scale;
                     }
***/
                  ret += im*im;
                }
             ++dj;
           }
        return sqrt(ret);
//      return scale*sqrt(ssq);
      }

   void scale(T factor)
      {
        T * dj = data;
        loop(j, len)
            {
              *dj = *dj * factor;
              ++dj;
            }
      }

   void clear()
      {
        T * dj = data;
        loop(j, len)   *dj++ = 0.0;
      }

   bool is_zero(ShapeItem count) const
      {
        const T * dj = data;
        loop(j, count)
           {
             if (IMAG(*dj) != 0)     return false;
             if (REAL(*dj++) != 0)   return false;
           }
        return true;
      }
 
   void conjugate()
      {
        if (!is_ZZ())   return;
        T * dj = data;
        loop(j, len)   DZ::conjugate(*dj++);
      }

   void add(const Vector & other)
      {
        assert(len == other.len);
        T * dj = data;
        T * sj = other.data;
        loop(j, len)   *dj++ += *sj++;
      }

   T & at(ShapeItem i)
      { assert(i < len);  return *(data + i); }

   const T & at(ShapeItem i) const
      { assert(i < len);  return *(data + i); }

   T sum_prod(const Vector other)
      {
        T sum = 0.0;
        const T * p1 = data;
        const T * p2 = other.data;
        loop(j, len)   sum += *p1++ * *p2++;
      }

   T sum_prod_conj(const Vector other)
      {
        T sum = 0.0;
        const T * p1 = data;
        const T * p2 = other.data;
        loop(j, len)   sum += DZ::CONJ(*p1++ * *p2++);
      }

protected:
   T * const data;
   const ShapeItem len;
};
//-----------------------------------------------------------------------------
template<typename T>
class Matrix
{
public:
   Matrix(T * _data, ShapeItem _rows, ShapeItem _cols, ShapeItem _dx)
     : data(_data),
       rows(_rows),
       cols(_cols),
       dx(_dx)
   {}

   Matrix(const Matrix & other, ShapeItem new_col_count)
     : data(other.data),
       rows(other.rows),
       cols(new_col_count),
       dx(other.dx)
   { assert(new_col_count <= other.cols); }

   // return the sub-matrix starting at row y and column x
   //
   Matrix sub_yx(ShapeItem row, ShapeItem col)
      {
        assert(col <= cols && row <= rows);
        return Matrix(&at(row, col), rows - row, cols - col, dx);
      }

   // return the sub-matrix of size new_len_y: new_len_x
   // starting at row 0 and column 0
   //
   Matrix sub_len(ShapeItem new_rows, ShapeItem new_cols)
      {
        assert(new_rows <= rows);
        assert(new_cols <= cols);
        return Matrix(data, new_rows, new_cols, dx);
      }

   bool is_ZZ() const   { return DZ::is_ZZ(*data); }

   const ShapeItem get_row_count() const
      { return rows; }

   const ShapeItem get_column_count() const
      { return cols; }

   Vector<T> get_column(ShapeItem c) 
      {
        return Vector<T>(data + c*dx, rows);
      }

   const Vector<T> get_column(ShapeItem c) const
      {
        return Vector<T>(data + c*dx, rows);
      }

   T & at(ShapeItem i, ShapeItem j)
      { assert(i < rows);   assert(j < cols); return *(data + i + j*dx); }

   const T & at(ShapeItem i, ShapeItem j) const
      { assert(i < rows);   assert(j < cols); return *(data + i + j*dx); }

   T & diag(ShapeItem i) const
      { assert(i < rows);   assert(i < cols); return *(data + i*(1 + dx)); }

   void exchange_columns(ShapeItem c1, ShapeItem c2)
      {
        assert(c1 < cols);
        assert(c2 < cols);
        T * p1 = data + c1*dx;
        T * p2 = data + c2*dx;
        loop(r, rows)   exchange(*p1++, *p2++);
      }

   ShapeItem get_dx() const   { return dx; }

   /// return the length of the largest element
   double max_norm() const
      {
        const ShapeItem len = rows * cols;
        if (is_ZZ())   // complex max. norm
           {
             double ret2 = 0;
             loop(l, len)
                {
                  const double a2 = ABS_2(data[l]);
                  if (ret2 < a2)   ret2 = a2;
                }
             return sqrt(ret2);
           }
        else           // real max norm
           {
             double ret = 0;
             loop(l, len)
                {
                  const double a = DZ::get_real(data[l]);
                  if (ret < a)         ret = a;
                  else if (ret < -a)   ret = -a;
                }
             return ret;
           }
      }

   void scale(T factor)
      {
        T * dj = data;
        const ShapeItem len = rows*cols;
        loop(j, len)
            {
               *dj = *dj * factor;
               ++dj;
            }
      }

protected:
   T * const data;
   const ShapeItem rows;
   const ShapeItem cols;
   const ShapeItem dx;
};
//=============================================================================

#include <math.h>
#include <stdio.h>
#include <string.h>

//=============================================================================

   // numerical limnits
   //
#define dlamch_E 1.11022e-16
#define dlamch_S 2.22507e-308
#define dlamch_P 2.22045e-16
static const double small_number = dlamch_S / dlamch_P;
static const double big_number   = 1.0 / small_number;
static const double safe_min     =  dlamch_S / dlamch_E;
static const double inv_safe_min = 1.0 / safe_min;
static const double tol3z        = sqrt(dlamch_E);

//-----------------------------------------------------------------------------
inline ShapeItem max_pos(const double * vec, ShapeItem len)
{
   // return the offset of the largest element in vec
   //
ShapeItem imax = 0;
double dmax = ABS(vec[0]); 
   for (ShapeItem j = 1; j < len; ++j)
       {
         const double dj = ABS(vec[j]);
         if (dmax < dj )   { dmax = dj;   imax = j; }
       }

   return imax;
}
//-----------------------------------------------------------------------------
template<typename T>
T larfg(ShapeItem N, T & ALPHA, Vector<T> &x)
{
   assert(N > 0);

   if (N == 1)   return 0.0;

double xnorm = x.norm();
double alpha_r = REAL(ALPHA);
double alpha_i = IMAG(ALPHA);

   if (xnorm == 0.0 && alpha_i == 0.0)   return 0.0;

double beta = -SIGN(sqrt(alpha_r*alpha_r + alpha_i*alpha_i + xnorm*xnorm),
                    alpha_r);

int kcnt = 0;
   if (ABS(beta) < safe_min)
       {
         while (ABS(beta) < safe_min)
            {
              ++kcnt;
              x.scale(inv_safe_min);
              beta = beta * inv_safe_min;
              alpha_r *= inv_safe_min;
              alpha_i *= inv_safe_min;
            }

         xnorm = x.norm();
         Sri<T>(ALPHA, alpha_r, alpha_i);
         beta = -SIGN(sqrt(alpha_r*alpha_r +
                           alpha_i*alpha_i +
                           xnorm*xnorm), alpha_r);
       }

T tau;
   Sri<T>(tau, (beta - alpha_r)/beta, -alpha_i/beta);
const T factor = DZ::inv(ALPHA - beta);
   x.scale(factor);

   loop(k, kcnt)   beta *= safe_min;
   Sri<T>(ALPHA, beta);

   return tau;
}
//-----------------------------------------------------------------------------
template<typename T>
void trsm(int M, int N, const Matrix<T> & A, Matrix<T> & B)
{
   // only: SIDE   = 'Left'              - lside == true
   //       UPLO   = 'Upper'             - upper = true
   //       TRANSA = 'No transpose'      - 
   //       DIAG   = 'Non-unit' and      - nounit = true
   //       ALPHA  = 1.0 is implemented!
   //
   loop(j_0, N)
       {
         for (ShapeItem k_0 = M - 1; k_0 >= 0; --k_0)
             {
               T & B_kj = B.at(k_0, j_0);
               if (B_kj != 0.0)
                  {
                    B_kj = B_kj / A.diag(k_0);
                    loop(i_0, k_0)
                        B.at(i_0, j_0) = B.at(i_0, j_0) - B_kj * A.at(i_0, k_0);
                  }
             }
       }
}
//-----------------------------------------------------------------------------
template<typename T>
int ila_lc(ShapeItem M, ShapeItem N, const Matrix<T> & A)
{
   for (ShapeItem col = N - 1; col > 0; --col)
       {
         const Vector<T> column = A.get_column(col);
         if (!column.is_zero(M))   return col + 1;

         /* the above is the same as:

         for (ShapeItem row = 0; row < M; ++row)
             {
               if (REAL(A[row + col*LDA]) != 0)   return col + 1;
               if (IMAG(A[row + col*LDA]) != 0)   return col + 1;
             }

          */
       }

   return 1;
}
//-----------------------------------------------------------------------------
template<typename T>
inline void gemv(int M, int N, const Matrix<T> & A, const Vector<T> &x,
                 Vector<T> &y)
{
   loop(j, N)
       {
         T temp = 0;
         loop(i, M)   temp = temp + DZ::CONJ(A.at(i, j)) * x.at(i);
         y.at(j) = temp;
       }
}
//-----------------------------------------------------------------------------
template<typename T>
void gerc(int M, int N, T ALPHA, const Vector<T> &x, const Vector<T> &y,
          Matrix<T> & A)
{
   if (M == 0 || N == 0 || ALPHA == 0.0)   return;

   loop(j, N)
      {
        T Y_j = y.at(j);
        if (Y_j != 0.0)
           {
             DZ::conjugate(Y_j);
             Y_j = Y_j * ALPHA;
             loop(i, M)   A.at(i, j) = A.at(i, j) + Y_j * x.at(i);
           }
      }
}
//-----------------------------------------------------------------------------
template<typename T>
void larf(Vector<T> & v, T tau, Matrix<T> & c)
{
const ShapeItem M = c.get_row_count();
const ShapeItem N = c.get_column_count();

   // only SIDE == "Left" is implemented
   //
ShapeItem  lastv = 0;
ShapeItem  lastc = 0;

   if (tau != 0.0)
     {
       // Look for the last non-zero row in V
       //
       lastv = M;
       while (lastv > 0 && v.at(lastv - 1) == 0.0)   --lastv;

       // Scan for the last non-zero column in C(1:lastv,:)
       //
       lastc = ila_lc<T>(lastv, N, c);
     }

   if (lastv)
      {
        DynArray(double, gemv_data, 2*N);   // double instead of T for OSX
        Vector<T> gemv_result((T *)gemv_data, N);

        gemv<T>(lastv, lastc, c, v, gemv_result);
        gerc<T>(lastv, lastc, -tau, v, gemv_result, c);
      }
}
//-----------------------------------------------------------------------------
template<typename T>
void laqp2(Matrix<T> & A, ShapeItem * pivot, T * tau, double * vn1)
{
const ShapeItem M = A.get_row_count();
const ShapeItem N = A.get_column_count();

double * vn2 = vn1 + N;

   loop(i_0, N)
      {
        T & tau_i = tau[i_0];

        // Determine the i'th pivot column and swap if necessary.
        //
        const int pvt_0 = i_0 + max_pos(vn1 + i_0, N - i_0);

        if (pvt_0 != i_0)
           {
             A.exchange_columns(pvt_0, i_0);
             exchange(pivot[pvt_0], pivot[i_0]);
             vn1[pvt_0] = vn1[i_0];
             vn2[pvt_0] = vn2[i_0];
           }

        // Generate elementary reflector H(i).
        //
        {
          if (i_0 < (M - 1))
             {
               Vector<T> x(&A.at(i_0 + 1, i_0), M - i_0 - 1);
               T & alpha = *(&x.at(0) - 1);   // one before x
               tau_i = larfg<T>(M - i_0, alpha, x);
             }
          else
             {
               Vector<T> x(&A.at(M - 1, i_0), 1);
               T & alpha = x.at(0);           // at x
               tau_i = larfg<T>(1, alpha, x);
             }
        }

        if (i_0 < (N - 1))
           {
             // Apply H(i)**H to A(offset+i:m,i+1:n) from the left.
             //
             const T Aii = A.diag(i_0);   // save diag
             A.diag(i_0) = 1.0;
                Vector<T> v(&A.diag(i_0), M - i_0);
                Matrix<T> c = A.sub_yx(i_0, i_0 + 1);
                larf<T>(v, DZ::CONJ(tau_i), c);
             A.diag(i_0) = Aii;           // restore diag
           }

        // Update partial column norms.
        //
        for (ShapeItem j_0 = i_0 + 1; j_0 < N; ++j_0)
            {
              if (vn1[j_0] != 0)
                 {
                   double temp = ABS(A.at(i_0, j_0)) / vn1[j_0];
                   temp = 1.0 - temp*temp;
                   temp = MAX(temp, 0.0);

                   double temp2 = vn1[j_0] / vn2[j_0];
                   temp2 = temp * temp2 * temp2;
                   if (temp2 <= tol3z)
                      {
                        if (i_0 < (M - 1))
                           {
                             Vector<T> x(&A.at(i_0 + 1, j_0), M - i_0 - 1);
                             vn1[j_0] = x.norm();
                             vn2[j_0] = vn1[j_0];
                           }
                        else
                           {
                             vn1[j_0] = 0.0;
                             vn2[j_0] = 0.0;
                           }
                      }
                   else
                      {
                        vn1[j_0] *= sqrt(temp);
                      }
                 }
            }
      }
}
//-----------------------------------------------------------------------------
template<typename T>
void laic1_max(double SEST, T alpha, T GAMMA, double &SESTPR, T &S, T &C)
{
const double eps = dlamch_E;
const double abs_alpha = ABS(alpha);
const double abs_gamma = ABS(GAMMA);
const double abs_estimate = ABS(SEST);

   //    Estimating largest singular value ...
   //

   //    special cases
   //
   if (SEST == 0.0)
      {
        const double s1 = MAX(abs_gamma, abs_alpha);
        if (s1 == 0.0)
           {
             S = 0.0;
             C = 1.0;
             SESTPR = 0.0;
           }
        else
           {
             S = alpha / s1;
             C = GAMMA / s1;

             const double tmp = sqrt(ABS_2(S) + ABS_2(C));
             S = S / tmp;
             C = C / tmp;
             SESTPR = s1*tmp;
           }
        return;
      }

   if (abs_gamma <= eps*abs_estimate)
      {
        S = 1.0;
        C = 0.0;
        double tmp = MAX(abs_estimate, abs_alpha);
        const double s1 = abs_estimate / tmp;
        const double s2 = abs_alpha / tmp;
        SESTPR = tmp*sqrt(s1*s1 + s2*s2);
        return;
      }

   if (abs_alpha <= eps*abs_estimate)
      {
        const double s1 = abs_gamma;
        const double s2 = abs_estimate;
        if (s1 <= s2)
           {
             S = 1.0;
             C = 0.0;
             SESTPR = s2;
           }
        else
           {
             S = 0.0;
             C = 1.0;
             SESTPR = s1;
           }
        return;
      }

   if (abs_estimate <= eps*abs_alpha || abs_estimate <= eps*abs_gamma)
      {
        const double s1 = abs_gamma;
        const double s2 = abs_alpha;
        if (s1 <= s2)
           {
             const double tmp = s1 / s2;
             const double scl = sqrt(1.0 + tmp*tmp);
             SESTPR = s2*scl;
             S = (alpha / s2) / scl;
             C = (GAMMA / s2) / scl;
           }
        else
           {
             const double tmp = s2 / s1;
             const double scl = sqrt(1.0 + tmp*tmp);
             SESTPR = s1*scl;
             S = (alpha / s1) / scl;
             C = (GAMMA / s1) / scl;
           }
        return;
      }

   // normal case
   //
const double zeta1 = abs_alpha / abs_estimate;
const double zeta2 = abs_gamma / abs_estimate;
const double b = (1.0 - zeta1*zeta1 - zeta2*zeta2)*0.5;
const double zeta1_2 = zeta1*zeta1;
double t;

   if (b > 0.0)  t = zeta1_2 / (b + sqrt(b*b + zeta1_2));
   else          t = sqrt(b*b + zeta1_2) - b;

const T sine = -( alpha / abs_estimate ) / t;
const T cosine = -( GAMMA / abs_estimate ) / ( 1.0 + t);
const double tmp = sqrt(ABS_2(sine) + ABS_2(cosine));
   S = sine / tmp;
   C = cosine / tmp;
   SESTPR = sqrt(t + 1.0) * abs_estimate;
}
//-----------------------------------------------------------------------------
template<typename T>
void laic1_min(double SEST, T alpha, T GAMMA, double &SESTPR, T &S, T &C)
{
const double eps = dlamch_E;
const double abs_alpha = ABS(alpha);
const double abs_gamma = ABS(GAMMA);
const double abs_estimate = ABS(SEST);

   //    Estimating smallest singular value ...
   //

   //    special cases
   //
   if (SEST == 0.0)
      {
        SESTPR = 0.0;
        T sine = 1.0;
        T cosine = 0.0;
        if (abs_gamma > 0.0 || abs_alpha > 0.0)
           {
             sine   = -DZ::CONJ(GAMMA);
             cosine =  DZ::CONJ(alpha);
           }

        const double abs_sine = ABS(sine);
        const double abs_cosine = ABS(cosine);
        const double s1 = MAX(abs_sine, abs_cosine);
        const T sine_s1 = sine / s1;
        const T cosine_s1 = cosine / s1;
        S = sine_s1;
        C = cosine_s1;
        const double tmp = sqrt(ABS_2(sine_s1) + ABS_2(cosine_s1));
        S = S / tmp;
        C = C / tmp;
        return;
      }

   if (abs_gamma <= eps*abs_estimate)
      {
        S = 0.0;
        C = 1.0;
        SESTPR = abs_gamma;
        return;
      }

   if (abs_alpha <= eps*abs_estimate)
      {
        const double s1 = abs_gamma;
        const double s2 = abs_estimate;

        if (s1 <= s2)
          {
            S = 0.0;
            C = 1.0;
            SESTPR = s1;
          }
        else
          {
            S = 1.0;
            C = 0.0;
            SESTPR = s2;
          }
        return;
      }

   if (abs_estimate <= eps*abs_alpha || abs_estimate <= eps*abs_gamma)
      {
        const double s1 = abs_gamma;
        const double s2 = abs_alpha;
        const T conj_gamma = DZ::CONJ(GAMMA);
        const T conj_alpha = DZ::CONJ(alpha);

        if (s1 <= s2)
           {
             const double tmp = s1 / s2;
             const double scl = sqrt(1.0 + tmp*tmp);

             SESTPR = abs_estimate*(tmp/scl);
             S = -(conj_gamma / s2 ) / scl;
             C =  (conj_alpha / s2 ) / scl;
           }
        else
           {
             const double tmp = s2 / s1;
             const double scl = sqrt(1.0 + tmp*tmp);

             SESTPR = abs_estimate / scl;
             S = -( conj_gamma / s1 ) / scl;
             C = ( conj_alpha  / s1 ) / scl;
           }
        return;
      }

   // normal case
   //
const double zeta1 = abs_alpha / abs_estimate;
const double zeta2 = abs_gamma / abs_estimate;

const double norma_1 = 1.0 + zeta1*zeta1 + zeta1*zeta2;
const double norma_2 = zeta1*zeta2 + zeta2*zeta2;
const double norma = MAX(norma_1, norma_2);

const double test = 1.0 + 2.0*(zeta1-zeta2)*(zeta1+zeta2);
T sine;
T cosine;
   if (test >= 0.0 )
      {
        // root is close to zero, compute directly
        //
        const double b = (zeta1*zeta1 + zeta2*zeta2 + 1.0)*0.5;
        const double zeta2_2 = zeta2*zeta2;
        const double t = zeta2_2 / (b + sqrt(ABS(b*b - zeta2_2)));
        sine = (alpha / abs_estimate) / (1.0 - t);
        cosine = -( GAMMA / abs_estimate ) / t;
        SESTPR = sqrt(t + 4.0*eps*eps*norma)*abs_estimate;
      }
   else
      {
        // root is closer to ONE, shift by that amount
        //
        const double b = (zeta2*zeta2 + zeta1*zeta1 - 1.0)*0.5;
        const double zeta1_2 = zeta1*zeta1;
        double t;
        if (b >= 0.0)   t = -zeta1_2 / (b+sqrt(b*b + zeta1_2));
        else            t = b - sqrt(b*b + zeta1_2);

        sine = -( alpha / abs_estimate ) / t;
        cosine = -( GAMMA / abs_estimate ) / (1.0 + t);
        SESTPR = sqrt(1.0 + t + 4.0*eps*eps*norma)*abs_estimate;
      }

const double tmp = sqrt(ABS_2(sine) + ABS_2(cosine));
   S = sine / tmp;
   C = cosine / tmp;
}
//-----------------------------------------------------------------------------
template<typename T>
void unm2r(ShapeItem K, Matrix<T> & A, const T * tau, Matrix<T> & c)
{
const ShapeItem M = c.get_row_count();

   // only SIDE == "Left" is implemented
   // only TRANS = 'T' or 'C' is implemented
   // thus NOTRANS is always false
   //
   loop(i_0, K)
       {
         // H(i) or H(i)**H is applied to C(i:m,1:n)
         //
         const int MM = M - i_0;

         // Apply H(i) or H(i)**H
         //
         const T taui = DZ::CONJ(tau[i_0]);

         const T Aii = A.diag(i_0);
         A.diag(i_0) = 1.0;
            Vector<T> v(&A.diag(i_0), MM);
            Matrix<T> c1 = c.sub_yx(i_0, 0);
            larf<T>(v, taui, c1);
         A.diag(i_0) = Aii;
       }
}
//-----------------------------------------------------------------------------
template<typename T>
void geqp3(Matrix<T> & A, ShapeItem * pivot, T * tau)
{
const ShapeItem M = A.get_row_count();
const ShapeItem N = A.get_column_count();

   // init column permutaion for pivoting
   // so we simply create the identical permutation
   //
   loop(j_0, N)   pivot[j_0] = j_0;

   // (no fixed columns)

   // Factorize free columns
   // ======================
   //
   {
     // Initialize partial column norms.
     // the first N elements of WORK store the exact column norms.
     //
     DynArray(double, vn12, 2*N);   // == vn1, vn2

     for (int j_0 = 0; j_0 < N; ++j_0)
         {
           Vector<T> x(&A.at(0, j_0), M);
           vn12[N + j_0] = vn12[j_0] = x.norm();
         }

     // Use unblocked code to factor the last or only block
     //
     laqp2<T>(A, pivot, tau, vn12);
   }
}
//-----------------------------------------------------------------------------
template<typename T>
int estimate_rank(const Matrix<T> & A, double rcond)
{
   // Determine RANK using incremental condition estimation...

const ShapeItem N = A.get_column_count();

   // store minima in work_min[ 0 ... N]
   // store maxima in work_max == work_min[N ... 2N]
   //
DynArray(double, work_1, 4*N);   // double instead of T for OSX
T * work_min = (T *)work_1;
T * work_max = work_min + N;

   work_min[0] = 1.0;
   work_max[0] = 1.0;

double smax = ABS(A.diag(0));
double smin = smax;

   if (smax == 0.0)   return 0;

   for (int RANK = 1; RANK < N; ++RANK)
       {
         double sminpr = 0.0;
         double smaxpr = 0.0;
         T s1 = 0.0;
         T s2 = 0.0;
         T c1 = 0.0;
         T c2 = 0.0;
         const T gamma = A.diag(RANK);

         T alpha_min = 0.0;
         T alpha_max = 0.0;
         for (int r = 0; r < RANK; ++r)
             {
               alpha_min = alpha_min + DZ::CONJ(work_min[r] * A.at(r, RANK));
               alpha_max = alpha_max + DZ::CONJ(work_max[r] * A.at(r, RANK));
             }

         laic1_min<T>(smin, alpha_min, gamma, sminpr, s1, c1);
         laic1_max<T>(smax, alpha_max, gamma, smaxpr, s2, c2);

         if (smaxpr*rcond > sminpr)   return RANK;

         loop(cI, RANK)
              {
                work_min[cI] = work_min[cI] * s1;
                work_max[cI] = work_max[cI] * s2;
              }
         work_min[RANK] = c1;
         work_max[RANK] = c2;
         smin = sminpr;
         smax = smaxpr;
       }

   return N;
}
//-----------------------------------------------------------------------------
template<typename T>
int scaled_gelsy(Matrix<T> & A, Matrix<T> & B, double rcond)
{
   // gelsy is optimized for (and restricted to) the following conditions:
   //
   // 0 < N <= M  →  min_NM = N and max_MN = M
   // 0 < NRHS
   //
const ShapeItem N    = A.get_column_count();
const ShapeItem NRHS = B.get_column_count();

   // Compute QR factorization with column pivoting of A:
   // A * P = Q * R
   //
DynArray(ShapeItem, pivot, N);

DynArray(double, tau1, 2*N);   // double instead of T for OSX
T * tau = (T *)tau1;

   geqp3<T>(A, pivot, tau);

   // Details of Householder rotations stored in WORK(1:MN).
   //
   // Determine RANK using incremental condition estimation
   //
   {
     const int RANK = estimate_rank(A, rcond);
     if (RANK < N)   return RANK;
   }

   // from here on, RANK == N. We leave RANK in the comments but use N un
   // the code.

   // Logically partition R = [ R11 R12 ]
   //                         [  0  R22 ]
   // where R11 = R(1:RANK,1:RANK)
   // [R11,R12] = [ T11, 0 ] * Y
   //

   // Details of Householder rotations stored in WORK(MN+1:2*MN)
   // 
   // B(1:M, 1:NRHS) := Q**H * B(1:M,1:NRHS)
   // 
   unm2r<T>(N, A, tau, B);

   // B(1:RANK, 1:NRHS) := inv(T11) * B(1:RANK,1:NRHS)
   //
   {
     Matrix<T> B1 = B.sub_len(B.get_dx(), NRHS);
     trsm<T>(N, NRHS, A, B1);
   }

   // workspace: 2*MN+NRHS
   //
   // B(1:N,1:NRHS) := P * B(1:N,1:NRHS)
   //
   loop(j_0, NRHS)
      {
        DynArray(double, tmp1, 2*N);   // double instead of T for OSX
        T * tmp = (T *)tmp1;
        loop(i_0, N)   tmp[pivot[i_0]] = B.at(i_0, j_0);
        loop(i_0, N)   B.at(i_0, j_0) = tmp[i_0];
      }

   return N;
}
//-----------------------------------------------------------------------------
template<typename T>
int gelsy(Matrix<T> & A, Matrix<T> &B, double rcond)
{
const ShapeItem M    = A.get_row_count();
const ShapeItem N    = A.get_column_count();
const ShapeItem NRHS = B.get_column_count();

   // APL is responsible for handling the empty cases
   //
   Assert(M && N && NRHS && N <= M);

   // For better precision, scale A and B so that their max. element lies
   // between small_number and big_number. Then call scaled_gelsy() and
   // scale the result back by the same factors.
   //
double scale_A = 1.0;
   {
     const double norm_A = A.max_norm();

     if (norm_A == 0.0)                return 0;   // A is rank-deficient
     else if (norm_A < small_number)   A.scale(scale_A = big_number);
     else if (norm_A > big_number)     A.scale(scale_A = small_number);
   }

double scale_B = 1.0;
   {
     const double norm_B = B.max_norm();

     if (norm_B == 0.0)                /* OK */ ;
     else if (norm_B < small_number)   B.scale(scale_B = big_number);
     else if (norm_B > big_number)     B.scale(scale_B = small_number);
   }

   {
     const int RANK = scaled_gelsy(A, B, rcond);
     if (RANK < N)   return RANK;   // this is an error
   }

   // Undo scaling
   //
   if (scale_A != 1.0)
      {
        Matrix<T> A1 = A.sub_len(N, N);
        Matrix<T> B1 = B.sub_len(N, NRHS);
        B1.scale(1/scale_A);
        A1.scale(scale_A);
      }

   if (scale_B != 1.0)
      {
        Matrix<T> B1 = B.sub_len(N, NRHS);
        B1.scale(scale_B);
      }

   return N;   // success
}
//=============================================================================

#endif // __GELSY_HH_DEFINED__
