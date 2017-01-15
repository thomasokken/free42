/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2017  Thomas Okken
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

int finite(double x) {
    return _finite(x);
}

int isinf(double x) {
    return _finite(x) || _isnan(x) ? 0 : x < 0 ? -1 : 1;
}

void sincos(double x, double *sinx, double *cosx) {
    *sinx = sin(x);
    *cosx = cos(x);
}


/******************************************************************************
 * The remainder of this file consists of definitions of the functions        *
 * asinh(), acosh(), atanh(), log1p(), and expm1(), all of which are missing  *
 * from the MSVC++ version of the C library. These definitions were all taken *
 * from the GNU C Library, version 2.2.5, and were originally donated by Sun  *
 * Microsystems.                                                              *
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
/* @(#)s_asinh.c 5.1 93/09/24 */
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

/* asinh(x)
 * Method :
 *  Based on
 *      asinh(x) = sign(x) * log [ |x| + sqrt(x*x+1) ]
 *  we have
 *  asinh(x) := x  if  1+x*x=1,
 *       := sign(x)*(log(x)+ln2)) for large |x|, else
 *       := sign(x)*log(2|x|+1/(|x|+sqrt(x*x+1))) if|x|>2, else
 *       := sign(x)*log1p(|x| + x^2/(1 + sqrt(1+x^2)))
 */

static double
one =  1.00000000000000000000e+00, /* 0x3FF00000, 0x00000000 */
ln2 =  6.93147180559945286227e-01, /* 0x3FE62E42, 0xFEFA39EF */
vast=  1.00000000000000000000e+300;

double asinh(double x)
{
    double t,w;
    int32_t hx,ix;
    GET_HIGH_WORD(hx,x);
    ix = hx&0x7fffffff;
    if(ix>=0x7ff00000) return x+x;  /* x is inf or NaN */
    if(ix< 0x3e300000) {    /* |x|<2**-28 */
        if(vast+x>one) return x;    /* return x inexact except 0 */
    }
    if(ix>0x41b00000) { /* |x| > 2**28 */
        w = log(fabs(x))+ln2;
    } else if (ix>0x40000000) { /* 2**28 > |x| > 2.0 */
        t = fabs(x);
        w = log(2.0*t+one/(sqrt(x*x+one)+t));
    } else {        /* 2.0 > |x| > 2**-28 */
        t = x*x;
        w =log1p(fabs(x)+t/(one+sqrt(one+t)));
    }
    if(hx>0) return w; else return -w;
}

/*---------------------------------------------------------------------------*/
/* @(#)e_acosh.c 5.1 93/09/24 */
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

/* __ieee754_acosh(x)
 * Method :
 *  Based on 
 *      acosh(x) = log [ x + sqrt(x*x-1) ]
 *  we have
 *      acosh(x) := log(x)+ln2, if x is large; else
 *      acosh(x) := log(2x-1/(sqrt(x*x-1)+x)) if x>2; else
 *      acosh(x) := log1p(t+sqrt(2.0*t+t*t)); where t=x-1.
 *
 * Special cases:
 *  acosh(x) is NaN with signal if x<1.
 *  acosh(NaN) is NaN without signal.
 */

#if 0
static double 
one = 1.0;
ln2 = 6.93147180559945286227e-01;  /* 0x3FE62E42, 0xFEFA39EF */
#endif

double acosh(double x)
{   
    double t;
    int32_t hx;
    u_int32_t lx;
    EXTRACT_WORDS(hx,lx,x);
    if(hx<0x3ff00000) {     /* x < 1 */
        return (x-x)/(x-x);
    } else if(hx >=0x41b00000) {    /* x > 2**28 */
        if(hx >=0x7ff00000) {   /* x is inf of NaN */
            return x+x;
        } else 
        return log(x)+ln2;  /* acosh(vast)=log(2x) */
    } else if(((hx-0x3ff00000)|lx)==0) {
        return 0.0;         /* acosh(1) = 0 */
    } else if (hx > 0x40000000) {   /* 2**28 > x > 2 */
        t=x*x;
        return log(2.0*x-one/(x+sqrt(t-one)));
    } else {            /* 1<x<2 */
        t = x-one;
        return log1p(t+sqrt(2.0*t+t*t));
    }
}

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

/*static double one = 1.0, vast = 1e300;*/
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
/* @(#)s_log1p.c 5.1 93/09/24 */
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
/* Modified by Naohiko Shimizu/Tokai University, Japan 1997/08/25,
   for performance improvement on pipelined processors.
*/

/* double log1p(double x)
 *
 * Method :
 *   1. Argument Reduction: find k and f such that
 *          1+x = 2^k * (1+f),
 *     where  sqrt(2)/2 < 1+f < sqrt(2) .
 *
 *      Note. If k=0, then f=x is exact. However, if k!=0, then f
 *  may not be representable exactly. In that case, a correction
 *  term is need. Let u=1+x rounded. Let c = (1+x)-u, then
 *  log(1+x) - log(u) ~ c/u. Thus, we proceed to compute log(u),
 *  and add back the correction term c/u.
 *  (Note: when x > 2**53, one can simply return log(x))
 *
 *   2. Approximation of log1p(f).
 *  Let s = f/(2+f) ; based on log(1+f) = log(1+s) - log(1-s)
 *       = 2s + 2/3 s**3 + 2/5 s**5 + .....,
 *           = 2s + s*R
 *      We use a special Reme algorithm on [0,0.1716] to generate
 *  a polynomial of degree 14 to approximate R The maximum error
 *  of this polynomial approximation is bounded by 2**-58.45. In
 *  other words,
 *              2      4      6      8      10      12      14
 *      R(z) ~ Lp1*s +Lp2*s +Lp3*s +Lp4*s +Lp5*s  +Lp6*s  +Lp7*s
 *      (the values of Lp1 to Lp7 are listed in the program)
 *  and
 *      |      2          14          |     -58.45
 *      | Lp1*s +...+Lp7*s    -  R(z) | <= 2
 *      |                             |
 *  Note that 2s = f - s*f = f - hfsq + s*hfsq, where hfsq = f*f/2.
 *  In order to guarantee error in log below 1ulp, we compute log
 *  by
 *      log1p(f) = f - (hfsq - s*(hfsq+R)).
 *
 *  3. Finally, log1p(x) = k*ln2 + log1p(f).
 *               = k*ln2_hi+(f-(hfsq-(s*(hfsq+R)+k*ln2_lo)))
 *     Here ln2 is split into two floating point number:
 *          ln2_hi + ln2_lo,
 *     where n*ln2_hi is always exact for |n| < 2000.
 *
 * Special cases:
 *  log1p(x) is NaN with signal if x < -1 (including -INF) ;
 *  log1p(+INF) is +INF; log1p(-1) is -INF with signal;
 *  log1p(NaN) is that NaN with no signal.
 *
 * Accuracy:
 *  according to an error analysis, the error is always less than
 *  1 ulp (unit in the last place).
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following
 * constants. The decimal values may be used, provided that the
 * compiler will convert from decimal to binary accurately enough
 * to produce the hexadecimal values shown.
 *
 * Note: Assuming log() return accurate answer, the following
 *   algorithm can be used to compute log1p(x) to within a few ULP:
 *
 *      u = 1+x;
 *      if(u==1.0) return x ; else
 *             return log(u)*(x/(u-1.0));
 *
 *   See HP-15C Advanced Functions Handbook, p.193.
 */

static double
ln2_hi  =  6.93147180369123816490e-01,  /* 3fe62e42 fee00000 */
ln2_lo  =  1.90821492927058770002e-10,  /* 3dea39ef 35793c76 */
two54   =  1.80143985094819840000e+16,  /* 43500000 00000000 */
Lp[] = {0.0, 6.666666666666735130e-01,  /* 3FE55555 55555593 */
 3.999999999940941908e-01,  /* 3FD99999 9997FA04 */
 2.857142874366239149e-01,  /* 3FD24924 94229359 */
 2.222219843214978396e-01,  /* 3FCC71C5 1D8E78AF */
 1.818357216161805012e-01,  /* 3FC74664 96CB03DE */
 1.531383769920937332e-01,  /* 3FC39A09 D078C69F */
 1.479819860511658591e-01};  /* 3FC2F112 DF3E5244 */

/*static double zero = 0.0;*/

double log1p(double x)
{
    double hfsq,f,c,s,z,R,u,z2,z4,z6,R1,R2,R3,R4;
    int32_t k,hx,hu,ax;

    GET_HIGH_WORD(hx,x);
    ax = hx&0x7fffffff;

    k = 1;
    if (hx < 0x3FDA827A) {          /* x < 0.41422  */
        if(ax>=0x3ff00000) {        /* x <= -1.0 */
        if(x==-1.0) return -two54/(x-x);/* log1p(-1)=+inf */
        else return (x-x)/(x-x);    /* log1p(x<-1)=NaN */
        }
        if(ax<0x3e200000) {         /* |x| < 2**-29 */
        if(two54+x>zero         /* raise inexact */
                &&ax<0x3c900000)        /* |x| < 2**-54 */
            return x;
        else
            return x - x*x*0.5;
        }
        if(hx>0||hx<=((int32_t)0xbfd2bec3)) {
        k=0;f=x;hu=1;}  /* -0.2929<x<0.41422 */
    }
    if (hx >= 0x7ff00000) return x+x;
    if(k!=0) {
        if(hx<0x43400000) {
        u  = 1.0+x;
        GET_HIGH_WORD(hu,u);
            k  = (hu>>20)-1023;
            c  = (k>0)? 1.0-(u-x):x-(u-1.0);/* correction term */
        c /= u;
        } else {
        u  = x;
        GET_HIGH_WORD(hu,u);
            k  = (hu>>20)-1023;
        c  = 0;
        }
        hu &= 0x000fffff;
        if(hu<0x6a09e) {
            SET_HIGH_WORD(u,hu|0x3ff00000); /* normalize u */
        } else {
            k += 1;
        SET_HIGH_WORD(u,hu|0x3fe00000); /* normalize u/2 */
            hu = (0x00100000-hu)>>2;
        }
        f = u-1.0;
    }
    hfsq=0.5*f*f;
    if(hu==0) { /* |f| < 2**-20 */
        if(f==zero) {
          if(k==0) return zero;
            else {c += k*ln2_lo; return k*ln2_hi+c;}
        }
        R = hfsq*(1.0-0.66666666666666666*f);
        if(k==0) return f-R; else
                 return k*ln2_hi-((R-(k*ln2_lo+c))-f);
    }
    s = f/(2.0+f);
    z = s*s;
#ifdef DO_NOT_USE_THIS
    R = z*(Lp1+z*(Lp2+z*(Lp3+z*(Lp4+z*(Lp5+z*(Lp6+z*Lp7))))));
#else
    R1 = z*Lp[1]; z2=z*z;
    R2 = Lp[2]+z*Lp[3]; z4=z2*z2;
    R3 = Lp[4]+z*Lp[5]; z6=z4*z2;
    R4 = Lp[6]+z*Lp[7];
    R = R1 + z2*R2 + z4*R3 + z6*R4;
#endif
    if(k==0) return f-(hfsq-s*(hfsq+R)); else
         return k*ln2_hi-((hfsq-(s*(hfsq+R)+(k*ln2_lo+c)))-f);
}

/*---------------------------------------------------------------------------*/
/* @(#)s_expm1.c 5.1 93/09/24 */
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
/* Modified by Naohiko Shimizu/Tokai University, Japan 1997/08/25,
   for performance improvement on pipelined processors.
*/

/* expm1(x)
 * Returns exp(x)-1, the exponential of x minus 1.
 *
 * Method
 *   1. Argument reduction:
 *  Given x, find r and integer k such that
 *
 *               x = k*ln2 + r,  |r| <= 0.5*ln2 ~ 0.34658
 *
 *      Here a correction term c will be computed to compensate
 *  the error in r when rounded to a floating-point number.
 *
 *   2. Approximating expm1(r) by a special rational function on
 *  the interval [0,0.34658]:
 *  Since
 *      r*(exp(r)+1)/(exp(r)-1) = 2+ r^2/6 - r^4/360 + ...
 *  we define R1(r*r) by
 *      r*(exp(r)+1)/(exp(r)-1) = 2+ r^2/6 * R1(r*r)
 *  That is,
 *      R1(r**2) = 6/r *((exp(r)+1)/(exp(r)-1) - 2/r)
 *           = 6/r * ( 1 + 2.0*(1/(exp(r)-1) - 1/r))
 *           = 1 - r^2/60 + r^4/2520 - r^6/100800 + ...
 *      We use a special Reme algorithm on [0,0.347] to generate
 *  a polynomial of degree 5 in r*r to approximate R1. The
 *  maximum error of this polynomial approximation is bounded
 *  by 2**-61. In other words,
 *      R1(z) ~ 1.0 + Q1*z + Q2*z**2 + Q3*z**3 + Q4*z**4 + Q5*z**5
 *  where   Q1  =  -1.6666666666666567384E-2,
 *      Q2  =   3.9682539681370365873E-4,
 *      Q3  =  -9.9206344733435987357E-6,
 *      Q4  =   2.5051361420808517002E-7,
 *      Q5  =  -6.2843505682382617102E-9;
 *      (where z=r*r, and the values of Q1 to Q5 are listed below)
 *  with error bounded by
 *      |                  5           |     -61
 *      | 1.0+Q1*z+...+Q5*z   -  R1(z) | <= 2
 *      |                              |
 *
 *  expm1(r) = exp(r)-1 is then computed by the following
 *  specific way which minimize the accumulation rounding error:
 *                 2     3
 *                r     r    [ 3 - (R1 + R1*r/2)  ]
 *        expm1(r) = r + --- + --- * [--------------------]
 *                    2     2    [ 6 - r*(3 - R1*r/2) ]
 *
 *  To compensate the error in the argument reduction, we use
 *      expm1(r+c) = expm1(r) + c + expm1(r)*c
 *             ~ expm1(r) + c + r*c
 *  Thus c+r*c will be added in as the correction terms for
 *  expm1(r+c). Now rearrange the term to avoid optimization
 *  screw up:
 *              (      2                                    2 )
 *              ({  ( r    [ R1 -  (3 - R1*r/2) ]  )  }    r  )
 *   expm1(r+c)~r - ({r*(--- * [--------------------]-c)-c} - --- )
 *                  ({  ( 2    [ 6 - r*(3 - R1*r/2) ]  )  }    2  )
 *                      (                                             )
 *
 *         = r - E
 *   3. Scale back to obtain expm1(x):
 *  From step 1, we have
 *     expm1(x) = either 2^k*[expm1(r)+1] - 1
 *          = or     2^k*[expm1(r) + (1-2^-k)]
 *   4. Implementation notes:
 *  (A). To save one multiplication, we scale the coefficient Qi
 *       to Qi*2^i, and replace z by (x^2)/2.
 *  (B). To achieve maximum accuracy, we compute expm1(x) by
 *    (i)   if x < -56*ln2, return -1.0, (raise inexact if x!=inf)
 *    (ii)  if k=0, return r-E
 *    (iii) if k=-1, return 0.5*(r-E)-0.5
 *        (iv)  if k=1 if r < -0.25, return 2*((r+0.5)- E)
 *                 else      return  1.0+2.0*(r-E);
 *    (v)   if (k<-2||k>56) return 2^k(1-(E-r)) - 1 (or exp(x)-1)
 *    (vi)  if k <= 20, return 2^k((1-2^-k)-(E-r)), else
 *    (vii) return 2^k(1-((E+2^-k)-r))
 *
 * Special cases:
 *  expm1(INF) is INF, expm1(NaN) is NaN;
 *  expm1(-INF) is -1, and
 *  for finite argument, only expm1(0)=0 is exact.
 *
 * Accuracy:
 *  according to an error analysis, the error is always less than
 *  1 ulp (unit in the last place).
 *
 * Misc. info.
 *  For IEEE double
 *      if x >  7.09782712893383973096e+02 then expm1(x) overflow
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following
 * constants. The decimal values may be used, provided that the
 * compiler will convert from decimal to binary accurately enough
 * to produce the hexadecimal values shown.
 */

#define one Q[0]
static double
/*vast      = 1.0e+300,*/
tiny        = 1.0e-300,
o_threshold = 7.09782712893383973096e+02,/* 0x40862E42, 0xFEFA39EF */
/*ln2_hi        = 6.93147180369123816490e-01,*/ /* 0x3fe62e42, 0xfee00000 */
/*ln2_lo        = 1.90821492927058770002e-10,*/ /* 0x3dea39ef, 0x35793c76 */
invln2      = 1.44269504088896338700e+00,/* 0x3ff71547, 0x652b82fe */
    /* scaled coefficients related to expm1 */
Q[]  =  {1.0, -3.33333333333331316428e-02, /* BFA11111 111110F4 */
   1.58730158725481460165e-03, /* 3F5A01A0 19FE5585 */
  -7.93650757867487942473e-05, /* BF14CE19 9EAADBB7 */
   4.00821782732936239552e-06, /* 3ED0CFCA 86E65239 */
  -2.01099218183624371326e-07}; /* BE8AFDB7 6E09C32D */

double expm1(double x)
{
    double y,hi,lo,c,t,e,hxs,hfx,r1,h2,h4,R1,R2,R3;
    int32_t k,xsb;
    u_int32_t hx;

    GET_HIGH_WORD(hx,x);
    xsb = hx&0x80000000;        /* sign bit of x */
    if(xsb==0) y=x; else y= -x; /* y = |x| */
    hx &= 0x7fffffff;       /* high word of |x| */

    /* filter out vast and non-finite argument */
    if(hx >= 0x4043687A) {          /* if |x|>=56*ln2 */
        if(hx >= 0x40862E42) {      /* if |x|>=709.78... */
                if(hx>=0x7ff00000) {
            u_int32_t low;
            GET_LOW_WORD(low,x);
            if(((hx&0xfffff)|low)!=0)
                 return x+x;     /* NaN */
            else return (xsb==0)? x:-1.0;/* exp(+-inf)={inf,-1} */
            }
            if(x > o_threshold) return vast*vast; /* overflow */
        }
        if(xsb!=0) { /* x < -56*ln2, return -1.0 with inexact */
        if(x+tiny<0.0)      /* raise inexact */
        return tiny-one;    /* return -1 */
        }
    }

    /* argument reduction */
    if(hx > 0x3fd62e42) {       /* if  |x| > 0.5 ln2 */
        if(hx < 0x3FF0A2B2) {   /* and |x| < 1.5 ln2 */
        if(xsb==0)
            {hi = x - ln2_hi; lo =  ln2_lo;  k =  1;}
        else
            {hi = x + ln2_hi; lo = -ln2_lo;  k = -1;}
        } else {
        k  = (int32_t) (invln2*x+((xsb==0)?0.5:-0.5));
        t  = k;
        hi = x - t*ln2_hi;  /* t*ln2_hi is exact here */
        lo = t*ln2_lo;
        }
        x  = hi - lo;
        c  = (hi-x)-lo;
    }
    else if(hx < 0x3c900000) {      /* when |x|<2**-54, return x */
        t = vast+x; /* return x with inexact flags when x!=0 */
        return x - (t-(vast+x));
    }
    else k = 0;

    /* x is now in primary range */
    hfx = 0.5*x;
    hxs = x*hfx;
#ifdef DO_NOT_USE_THIS
    r1 = one+hxs*(Q1+hxs*(Q2+hxs*(Q3+hxs*(Q4+hxs*Q5))));
#else
    R1 = one+hxs*Q[1]; h2 = hxs*hxs;
    R2 = Q[2]+hxs*Q[3]; h4 = h2*h2;
    R3 = Q[4]+hxs*Q[5];
    r1 = R1 + h2*R2 + h4*R3;
#endif
    t  = 3.0-r1*hfx;
    e  = hxs*((r1-t)/(6.0 - x*t));
    if(k==0) return x - (x*e-hxs);      /* c is 0 */
    else {
        e  = (x*(e-c)-c);
        e -= hxs;
        if(k== -1) return 0.5*(x-e)-0.5;
        if(k==1) {
            if(x < -0.25) return -2.0*(e-(x+0.5));
            else          return  one+2.0*(x-e);
        }
        if (k <= -2 || k>56) {   /* suffice to return exp(x)-1 */
            u_int32_t high;
            y = one-(e-x);
        GET_HIGH_WORD(high,y);
        SET_HIGH_WORD(y,high+(k<<20));  /* add k to y's exponent */
            return y-one;
        }
        t = one;
        if(k<20) {
            u_int32_t high;
            SET_HIGH_WORD(t,0x3ff00000 - (0x200000>>k));  /* t=1-2^-k */
            y = t-(e-x);
        GET_HIGH_WORD(high,y);
        SET_HIGH_WORD(y,high+(k<<20));  /* add k to y's exponent */
       } else {
            u_int32_t high;
        SET_HIGH_WORD(t,((0x3ff-k)<<20));   /* 2^-k */
            y = x-(e+t);
            y += one;
        GET_HIGH_WORD(high,y);
        SET_HIGH_WORD(y,high+(k<<20));  /* add k to y's exponent */
        }
    }
    return y;
}

/*---------------------------------------------------------------------------*/
