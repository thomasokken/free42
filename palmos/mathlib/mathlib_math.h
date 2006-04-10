/* MathLib: Pilot shared library of IEEE-754 double math functions
 *
 * This version of math.h is designed for inclusion by the GCC math
 * functions, not the calling application program.  Its purpose is to
 * allow proper compilation of the GCC source code with as few 
 * changes as possible.
 *
 * Copyright (C) 1997 Rick Huebner
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see file COPYING.LIB.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA
 *
 * Version 1.0, 15 August 1997, Rick Huebner
 */

// Typedefs used in math routines
typedef long int32_t;
typedef unsigned long u_int32_t;

// Prototypes for primary math functions, copied verbatim from GCC source
double __ieee754_acos(double x);
double __ieee754_asin(double x);
double __atan(double x);
double __ieee754_atan2(double y, double x);
double __cos(double x);
double __sin(double x);
double __tan(double x);
void   __sincos(double x, double *sinx, double *cosx);
double __ieee754_cosh(double x);
double __ieee754_sinh(double x);
double __tanh(double x);
double __ieee754_acosh(double x);
double __asinh(double x);
double __ieee754_atanh(double x);
double __ieee754_exp(double x);
double __frexp(double x, int *eptr);
double __ldexp(double value, int exp);
double __ieee754_log(double x);
double __ieee754_log10(double x);
double __modf(double x, double *iptr);
double __expm1(double x);
double __log1p(double x);
double __logb(double x);
double __log2(double x);
double __ieee754_pow(double x, double y);
double __ieee754_sqrt(double x);
double __ieee754_hypot(double x, double y);
double __cbrt(double x);
double __ceil(double x);
double __fabs(double x);
double __floor(double x);
double __ieee754_fmod(double x, double y);
int    __isinf(double x);
int    __finite(double x);
double __scalbn(double x, int n);
double __ieee754_remainder(double x, double p);
double __significand(double x);
double __copysign(double x, double y);
int    __isnan(double x);
int    __ilogb(double x);
double __rint(double x);
double __nextafter(double x, double y);
double __ieee754_scalb(double x, double fn);
double __round(double x);
double __trunc(double x);
int    __signbit(double x);

// Prototypes for special routines used by primary math functions
double __kernel_cos(double x, double y);
double __kernel_sin(double x, double y, int iy);
double __kernel_tan(double x, double y, int iy);
int32_t __ieee754_rem_pio2(double x, double *y);
int __kernel_rem_pio2(double *x, double *y, int e0, int nx, int prec, const int32_t *ipio2);

// Prototypes for our "jump island" stubs to avoid 32KB pc-rel jsr limit
double jumpto__ieee754_sqrt(double x);
double jumpto__fabs(double x);
double jumpto__scalbn(double x, int n);
