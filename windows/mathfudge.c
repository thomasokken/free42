/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2020  Thomas Okken
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see http://www.gnu.org/licenses/.
 *****************************************************************************/

#include <math.h>
#include <float.h>
#include "free42.h"

int isnan(double x) {
    return _isnan(x);
}

int isinf(double x) {
    return _finite(x) || _isnan(x) ? 0 : x < 0 ? -1 : 1;
}


/******************************************************************************
 * The remainder of this file consists of the definition of the function      *
 * atanh(), which is missing from the MSVC++ 2008 version of the C library.   *
 * This definition was taken from the GNU C Library, version 2.2.5, and was   *
 * originally donated by Sun Microsystems.                                    *
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/*
 * from: @(#)fdlibm.h 5.1 93/09/24
 * Id: math_private.h,v 1.1.1.4 2001/09/25 21:58:02 jakub Exp
 */

#if 0
#include <endian.h>
#include <sys/types.h>
#else
#define int32_t int
#define u_int32_t unsigned int
#endif

/* The original fdlibm code used statements like:
    n0 = ((*(int*)&one)>>29)^1;     * index of high word *
    ix0 = *(n0+(int*)&x);           * high word of x *
    ix1 = *((1-n0)+(int*)&x);       * low word of x *
   to dig two 32 bit words out of the 64 bit IEEE floating point
   value.  That is non-ANSI, and, moreover, the gcc instruction
   scheduler gets it wrong.  We instead use the following macros.
   Unlike the original code, we determine the endianness at compile
   time, not at run time; I don't see much benefit to selecting
   endianness at run time.  */

/* A union which permits us to convert between a double and two 32 bit
   ints.  */

#if 0 /* __FLOAT_WORD_ORDER == BIG_ENDIAN */

typedef union
{
  double value;
  struct
  {
    u_int32_t msw;
    u_int32_t lsw;
  } parts;
} ieee_double_shape_type;

#endif

#if 1 /* __FLOAT_WORD_ORDER == LITTLE_ENDIAN */

typedef union
{
  double value;
  struct
  {
    u_int32_t lsw;
    u_int32_t msw;
  } parts;
} ieee_double_shape_type;

#endif

/* Get two 32 bit ints from a double.  */

#define EXTRACT_WORDS(ix0,ix1,d)                \
do {                                \
  ieee_double_shape_type ew_u;                  \
  ew_u.value = (d);                     \
  (ix0) = ew_u.parts.msw;                   \
  (ix1) = ew_u.parts.lsw;                   \
} while (0)

/* Get the more significant 32 bit int from a double.  */

#define GET_HIGH_WORD(i,d)                  \
do {                                \
  ieee_double_shape_type gh_u;                  \
  gh_u.value = (d);                     \
  (i) = gh_u.parts.msw;                     \
} while (0)

/* Get the less significant 32 bit int from a double.  */

#define GET_LOW_WORD(i,d)                   \
do {                                \
  ieee_double_shape_type gl_u;                  \
  gl_u.value = (d);                     \
  (i) = gl_u.parts.lsw;                     \
} while (0)

/* Set a double from two 32 bit ints.  */

#define INSERT_WORDS(d,ix0,ix1)                 \
do {                                \
  ieee_double_shape_type iw_u;                  \
  iw_u.parts.msw = (ix0);                   \
  iw_u.parts.lsw = (ix1);                   \
  (d) = iw_u.value;                     \
} while (0)

/* Set the more significant 32 bits of a double from an int.  */

#define SET_HIGH_WORD(d,v)                  \
do {                                \
  ieee_double_shape_type sh_u;                  \
  sh_u.value = (d);                     \
  sh_u.parts.msw = (v);                     \
  (d) = sh_u.value;                     \
} while (0)

/* Set the less significant 32 bits of a double from an int.  */

#define SET_LOW_WORD(d,v)                   \
do {                                \
  ieee_double_shape_type sl_u;                  \
  sl_u.value = (d);                     \
  sl_u.parts.lsw = (v);                     \
  (d) = sl_u.value;                     \
} while (0)

/* A union which permits us to convert between a float and a 32 bit
   int.  */

typedef union
{
  float value;
  u_int32_t word;
} ieee_float_shape_type;

/* Get a 32 bit int from a float.  */

#define GET_FLOAT_WORD(i,d)                 \
do {                                \
  ieee_float_shape_type gf_u;                   \
  gf_u.value = (d);                     \
  (i) = gf_u.word;                      \
} while (0)

/* Set a float from a 32 bit int.  */

#define SET_FLOAT_WORD(d,i)                 \
do {                                \
  ieee_float_shape_type sf_u;                   \
  sf_u.word = (i);                      \
  (d) = sf_u.value;                     \
} while (0)

/*---------------------------------------------------------------------------*/
/* @(#)e_atanh.c 5.1 93/09/24 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice 
 * is preserved.
 * ====================================================
 */

/* __ieee754_atanh(x)
 * Method :
 *    1.Reduced x to positive by atanh(-x) = -atanh(x)
 *    2.For x>=0.5
 *                  1              2x                          x
 *  atanh(x) = --- * log(1 + -------) = 0.5 * log1p(2 * --------)
 *                  2             1 - x                      1 - x
 *  
 *  For x<0.5
 *  atanh(x) = 0.5*log1p(2x+2x*x/(1-x))
 *
 * Special cases:
 *  atanh(x) is NaN if |x| > 1 with signal;
 *  atanh(NaN) is that NaN with no signal;
 *  atanh(+-1) is +-INF with signal.
 *
 */

static double one = 1.0, vast = 1e300;
static double zero = 0.0;

double atanh(double x)
{
    double t;
    int32_t hx,ix;
    u_int32_t lx;
    EXTRACT_WORDS(hx,lx,x);
    ix = hx&0x7fffffff;
    if ((ix|((lx|(-(int32_t)lx))>>31))>0x3ff00000) /* |x|>1 */
        return (x-x)/(x-x);
    if(ix==0x3ff00000) 
        return x/zero;
    if(ix<0x3e300000&&(vast+x)>zero) return x;  /* x<2**-28 */
    SET_HIGH_WORD(x,ix);
    if(ix<0x3fe00000) {     /* x < 0.5 */
        t = x+x;
        t = 0.5*log1p(t+t*x/(one-x));
    } else 
        t = 0.5*log1p((x+x)/(one-x));
    if(hx>=0) return t; else return -t;
}

/*---------------------------------------------------------------------------*/
