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

#include "core_globals.h"
#include "core_math2.h"

phloat math_random() {
#ifdef BCD_MATH
    const Phloat n1("9821");
    const Phloat n2("0.211327");
    random_number = random_number * n1 + n2;
#else
    random_number = random_number * 9821 + 0.211327;
#endif
    random_number -= floor(random_number);
    return random_number;
}

int math_tan(phloat x, phloat *y, bool rad) {
    if (rad || flags.f.rad) {
        *y = tan(x);
    } else if (flags.f.grad) {
        bool neg = false;
        if (x < 0) {
            x = -x;
            neg = true;
        }
        // [0 200[
        x = fmod(x, 200);
        if (x == 100)
            goto infinite;
        // TAN(x+100gon) = -TAN(100gon-x)
        if (x > 100) {
            x = 200 - x;
            neg = !neg;
        }
        // to improve accuracy for x close to 100gon
        if (x > 89)
            *y = 1 / tan((100 - x) / (200 / PI));
        else
            *y = tan(x / (200 / PI)); 
        if (neg)
            *y = -(*y);
    } else {
        bool neg = false;
        if (x < 0) {
            x = -x;
            neg = true;
        }
        // [0 180[
        x = fmod(x, 180);
        if (x == 90)
            goto infinite;
        // TAN(x+90°) = -TAN(90°-x)
        if (x > 90) {
            x = 180 - x;
            neg = !neg;
        }
        // to improve accuracy for x close to 90°
        if (x > 80)
            *y = 1 / tan((90 - x) / (180 / PI));
        else
            *y = tan(x / (180 / PI)); 
        if (neg)
            *y = -(*y);
    }
    if (p_isnan(*y) || p_isinf(*y) != 0) {
        infinite:
        if (flags.f.range_error_ignore)
            *y = POS_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    return ERR_NONE;
}

int math_asinh(phloat xre, phloat xim, phloat *yre, phloat *yim) {

    if (xim == 0) {
        *yre = asinh(xre);
        *yim = 0;
        return ERR_NONE;
    } else if (xre == 0) {
        if (xim > 1) {
            *yre = acosh(xim);
            *yim = PI / 2;
        } else if (xim < -1) {
            *yre = -acosh(-xim);
            *yim = -PI / 2;
        } else {
            *yre = 0;
            *yim = asin(xim);
        }
        return ERR_NONE;
    }

    /* TODO: review; and deal with overflows in intermediate results */
    phloat are, aim, br, bphi;

    /* If re(x)<0, we calculate asinh(x)=-asinh(-x); this avoids the loss of
     * significance in x+sqrt(x^2+1) when x is large and sqrt(x^2+1) approaches
     * -x.
     */
    int neg = xre < 0;
    if (neg) {
        xre = -xre;
        xim = -xim;
    }

    /* a = x ^ 2 + 1 */
    are = xre * xre - xim * xim + 1;
    aim = 2 * xre * xim;

    /* b = sqrt(a) */
    br = sqrt(hypot(are, aim));
    bphi = atan2(aim, are) / 2;

    /* a = b + x */
    sincos(bphi, &aim, &are);
    are = are * br + xre;
    aim = aim * br + xim;

    /* y = log(a) */
    *yre = log(hypot(are, aim));
    *yim = atan2(aim, are);
    if (neg) {
        *yre = -*yre;
        *yim = -*yim;
    }
    return ERR_NONE;
}

int math_acosh(phloat xre, phloat xim, phloat *yre, phloat *yim) {

    if (xim == 0) {
        if (xre >= 1) {
            *yre = acosh(xre);
            *yim = 0;
        } else if (xre <= -1) {
            *yre = acosh(-xre);
            *yim = PI;
        } else {
            *yre = 0;
            *yim = acos(xre);
        }
        return ERR_NONE;
    } else if (xre == 0) {
        if (xim > 0) {
            *yre = asinh(xim);
            *yim = PI / 2;
        } else {
            *yre = -asinh(xim);
            *yim = -PI / 2;
        }
        return ERR_NONE;
    }

    /* TODO: review; and deal with overflows in intermediate results */
    phloat ar, aphi, are, aim, br, bphi, bre, bim, cre, cim;

    /* a = sqrt(x + 1) */
    ar = sqrt(hypot(xre + 1, xim));
    aphi = atan2(xim, xre + 1) / 2;
    sincos(aphi, &aim, &are);
    are *= ar;
    aim *= ar;

    /* b = sqrt(x - 1) */
    br = sqrt(hypot(xre - 1, xim));
    bphi = atan2(xim, xre - 1) / 2;
    sincos(bphi, &bim, &bre);
    bre *= br;
    bim *= br;

    /* c = x + a * b */
    cre = xre + are * bre - aim * bim;
    cim = xim + are * bim + aim * bre;

    /* y = log(c) */
    *yre = log(hypot(cre, cim));
    *yim = atan2(cim, cre);
    return ERR_NONE;
}

int math_atanh(phloat xre, phloat xim, phloat *yre, phloat *yim) {

    if (xim == 0) {
        if (xre == 1 || xre == -1)
            return ERR_INVALID_DATA;
        else if (xre > -1 && xre < 1) {
            *yre = atanh(xre);
            *yim = 0;
            return ERR_NONE;
        } else {
            *yre = atanh(1 / xre);
            if (xre > 1)
                *yim = -PI / 2;
            else
                *yim = PI / 2;
            return ERR_NONE;
        }
    } else if (xre == 0) {
        *yre = 0;
        *yim = atan(xim);
        return ERR_NONE;
    }

    phloat are, aim, bre, bim, cre, cim, h;

    /* TODO: review, and deal with overflows in intermediate results */

    /* a = 1 + x */
    are = 1 + xre;
    aim = xim;

    /* b = 1 - x */
    bre = 1 - xre;
    bim = - xim;

    /* c = a / b */
    h = hypot(bre, bim);
    cre = (are * bre + aim * bim) / h / h;
    cim = (aim * bre - are * bim) / h / h;

    /* y = log(c) / 2 */
    *yre = log(hypot(cre, cim)) / 2;
    *yim = atan2(cim, cre) / 2;

    /* Theoretically, you could go out of range, but in practice,
     * you can't get close enough to the critical values to cause
     * trouble.
     */
    return ERR_NONE;
}

#ifdef BCD_MATH

int math_gamma(phloat phx, phloat *phgamma) {
    if (phx == 0 || (phx < 0 && phx == floor(phx)))
        return ERR_INVALID_DATA;
    *phgamma = gamma(phx);
    int inf = p_isinf(*phgamma);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            *phgamma = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    return ERR_NONE;
}

#else // BCD_MATH

/**************************************************************/
/* The following is code to compute the gamma function,       */
/* copied from the GNU C Library, version 2.2.5, and modified */
/* to conform to the coding conventions used in Free42.       */
/* Dependencies on the IEEE-754 format have been removed.     */
/* It is included here because on PalmOS, MathLib does        */
/* not provide gamma at all, and on Linux, I'm not sure       */
/* how it works (the lack of documentation does not help!).   */
/* Better safe than sorry, and it's not that big anyway.      */
/**************************************************************/

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

/* __ieee754_lgamma_r(x, signgamp)
 * Reentrant version of the logarithm of the Gamma function
 * with user provide pointer for the sign of Gamma(x).
 *
 * Method:
 *   1. Argument Reduction for 0 < x <= 8
 *      Since gamma(1+s)=s*gamma(s), for x in [0,8], we may
 *      reduce x to a number in [1.5,2.5] by
 *              lgamma(1+s) = log(s) + lgamma(s)
 *      for example,
 *              lgamma(7.3) = log(6.3) + lgamma(6.3)
 *                          = log(6.3*5.3) + lgamma(5.3)
 *                          = log(6.3*5.3*4.3*3.3*2.3) + lgamma(2.3)
 *   2. Polynomial approximation of lgamma around its
 *      minimun ymin=1.461632144968362245 to maintain monotonicity.
 *      On [ymin-0.23, ymin+0.27] (i.e., [1.23164,1.73163]), use
 *              Let z = x-ymin;
 *              lgamma(x) = -1.214862905358496078218 + z^2*poly(z)
 *      where
 *              poly(z) is a 14 degree polynomial.
 *   2. Rational approximation in the primary interval [2,3]
 *      We use the following approximation:
 *              s = x-2.0;
 *              lgamma(x) = 0.5*s + s*P(s)/Q(s)
 *      with accuracy
 *              |P/Q - (lgamma(x)-0.5s)| < 2**-61.71
 *      Our algorithms are based on the following observation
 *
 *                             zeta(2)-1    2    zeta(3)-1    3
 * lgamma(2+s) = s*(1-Euler) + --------- * s  -  --------- * s  + ...
 *                                 2                 3
 *
 *      where Euler = 0.5771... is the Euler constant, which is very
 *      close to 0.5.
 *
 *   3. For x>=8, we have
 *      lgamma(x)~(x-0.5)log(x)-x+0.5*log(2pi)+1/(12x)-1/(360x**3)+....
 *      (better formula:
 *         lgamma(x)~(x-0.5)*(log(x)-1)-.5*(log(2pi)-1) + ...)
 *      Let z = 1/x, then we approximation
 *              f(z) = lgamma(x) - (x-0.5)(log(x)-1)
 *      by
 *                                  3       5             11
 *              w = w0 + w1*z + w2*z  + w3*z  + ... + w6*z
 *      where
 *              |w - f(z)| < 2**-58.74
 *
 *   4. For negative x, since (G is gamma function)
 *              -x*G(-x)*G(x) = pi/sin(pi*x),
 *      we have
 *              G(x) = pi/(sin(pi*x)*(-x)*G(-x))
 *      since G(-x) is positive, sign(G(x)) = sign(sin(pi*x)) for x<0
 *      Hence, for x<0, signgam = sign(sin(pi*x)) and
 *              lgamma(x) = log(|Gamma(x)|)
 *                        = log(pi/(|x*sin(pi*x)|)) - lgamma(-x);
 *      Note: one should avoid compute pi*(-x) directly in the
 *            computation of sin(pi*(-x)).
 *
 *   5. Special Cases
 *              lgamma(2+s) ~ s*(1-Euler) for tiny s
 *              lgamma(1)=lgamma(2)=0
 *              lgamma(x) ~ -log(x) for tiny x
 *              lgamma(0) = lgamma(inf) = inf
 *              lgamma(-integer) = +-inf
 *
 */

/* NOTE (ThO): if the 'const' in this code is not commented out, the
 * PalmOS build of Free42 malfunctions in GAMMA (wrong results,
 * memory access weirdness). This isn't the first time I've noticed
 * m68k-palmos-gcc acting weird in the presence of const globals.
 * It probably puts them in the wrong section or something. I should debug
 * this, and add section attributes where needed, but since Free42 doesn't
 * use that much global space anyway, and the variables below are only
 * something like 500 bytes altogether, I just work around the issue by
 * putting them in the read/write global area.
 */
static /*const*/ double
half=  5.00000000000000000000e-01, /* 0x3FE00000, 0x00000000 */
one =  1.00000000000000000000e+00, /* 0x3FF00000, 0x00000000 */
pi  =  3.14159265358979311600e+00, /* 0x400921FB, 0x54442D18 */
a0  =  7.72156649015328655494e-02, /* 0x3FB3C467, 0xE37DB0C8 */
a1  =  3.22467033424113591611e-01, /* 0x3FD4A34C, 0xC4A60FAD */
a2  =  6.73523010531292681824e-02, /* 0x3FB13E00, 0x1A5562A7 */
a3  =  2.05808084325167332806e-02, /* 0x3F951322, 0xAC92547B */
a4  =  7.38555086081402883957e-03, /* 0x3F7E404F, 0xB68FEFE8 */
a5  =  2.89051383673415629091e-03, /* 0x3F67ADD8, 0xCCB7926B */
a6  =  1.19270763183362067845e-03, /* 0x3F538A94, 0x116F3F5D */
a7  =  5.10069792153511336608e-04, /* 0x3F40B6C6, 0x89B99C00 */
a8  =  2.20862790713908385557e-04, /* 0x3F2CF2EC, 0xED10E54D */
a9  =  1.08011567247583939954e-04, /* 0x3F1C5088, 0x987DFB07 */
a10 =  2.52144565451257326939e-05, /* 0x3EFA7074, 0x428CFA52 */
a11 =  4.48640949618915160150e-05, /* 0x3F07858E, 0x90A45837 */
tc  =  1.46163214496836224576e+00, /* 0x3FF762D8, 0x6356BE3F */
tf  = -1.21486290535849611461e-01, /* 0xBFBF19B9, 0xBCC38A42 */
/* tt = -(tail of tf) */
tt  = -3.63867699703950536541e-18, /* 0xBC50C7CA, 0xA48A971F */
t0  =  4.83836122723810047042e-01, /* 0x3FDEF72B, 0xC8EE38A2 */
t1  = -1.47587722994593911752e-01, /* 0xBFC2E427, 0x8DC6C509 */
t2  =  6.46249402391333854778e-02, /* 0x3FB08B42, 0x94D5419B */
t3  = -3.27885410759859649565e-02, /* 0xBFA0C9A8, 0xDF35B713 */
t4  =  1.79706750811820387126e-02, /* 0x3F9266E7, 0x970AF9EC */
t5  = -1.03142241298341437450e-02, /* 0xBF851F9F, 0xBA91EC6A */
t6  =  6.10053870246291332635e-03, /* 0x3F78FCE0, 0xE370E344 */
t7  = -3.68452016781138256760e-03, /* 0xBF6E2EFF, 0xB3E914D7 */
t8  =  2.25964780900612472250e-03, /* 0x3F6282D3, 0x2E15C915 */
t9  = -1.40346469989232843813e-03, /* 0xBF56FE8E, 0xBF2D1AF1 */
t10 =  8.81081882437654011382e-04, /* 0x3F4CDF0C, 0xEF61A8E9 */
t11 = -5.38595305356740546715e-04, /* 0xBF41A610, 0x9C73E0EC */
t12 =  3.15632070903625950361e-04, /* 0x3F34AF6D, 0x6C0EBBF7 */
t13 = -3.12754168375120860518e-04, /* 0xBF347F24, 0xECC38C38 */
t14 =  3.35529192635519073543e-04, /* 0x3F35FD3E, 0xE8C2D3F4 */
u0  = -7.72156649015328655494e-02, /* 0xBFB3C467, 0xE37DB0C8 */
u1  =  6.32827064025093366517e-01, /* 0x3FE4401E, 0x8B005DFF */
u2  =  1.45492250137234768737e+00, /* 0x3FF7475C, 0xD119BD6F */
u3  =  9.77717527963372745603e-01, /* 0x3FEF4976, 0x44EA8450 */
u4  =  2.28963728064692451092e-01, /* 0x3FCD4EAE, 0xF6010924 */
u5  =  1.33810918536787660377e-02, /* 0x3F8B678B, 0xBF2BAB09 */
v1  =  2.45597793713041134822e+00, /* 0x4003A5D7, 0xC2BD619C */
v2  =  2.12848976379893395361e+00, /* 0x40010725, 0xA42B18F5 */
v3  =  7.69285150456672783825e-01, /* 0x3FE89DFB, 0xE45050AF */
v4  =  1.04222645593369134254e-01, /* 0x3FBAAE55, 0xD6537C88 */
v5  =  3.21709242282423911810e-03, /* 0x3F6A5ABB, 0x57D0CF61 */
s0  = -7.72156649015328655494e-02, /* 0xBFB3C467, 0xE37DB0C8 */
s1  =  2.14982415960608852501e-01, /* 0x3FCB848B, 0x36E20878 */
s2  =  3.25778796408930981787e-01, /* 0x3FD4D98F, 0x4F139F59 */
s3  =  1.46350472652464452805e-01, /* 0x3FC2BB9C, 0xBEE5F2F7 */
s4  =  2.66422703033638609560e-02, /* 0x3F9B481C, 0x7E939961 */
s5  =  1.84028451407337715652e-03, /* 0x3F5E26B6, 0x7368F239 */
s6  =  3.19475326584100867617e-05, /* 0x3F00BFEC, 0xDD17E945 */
r1  =  1.39200533467621045958e+00, /* 0x3FF645A7, 0x62C4AB74 */
r2  =  7.21935547567138069525e-01, /* 0x3FE71A18, 0x93D3DCDC */
r3  =  1.71933865632803078993e-01, /* 0x3FC601ED, 0xCCFBDF27 */
r4  =  1.86459191715652901344e-02, /* 0x3F9317EA, 0x742ED475 */
r5  =  7.77942496381893596434e-04, /* 0x3F497DDA, 0xCA41A95B */
r6  =  7.32668430744625636189e-06, /* 0x3EDEBAF7, 0xA5B38140 */
w0  =  4.18938533204672725052e-01, /* 0x3FDACFE3, 0x90C97D69 */
w1  =  8.33333333333329678849e-02, /* 0x3FB55555, 0x5555553B */
w2  = -2.77777777728775536470e-03, /* 0xBF66C16C, 0x16B02E5C */
w3  =  7.93650558643019558500e-04, /* 0x3F4A019F, 0x98CF38B6 */
w4  = -5.95187557450339963135e-04, /* 0xBF4380CB, 0x8C0FE741 */
w5  =  8.36339918996282139126e-04, /* 0x3F4B67BA, 0x4CDAD5D1 */
w6  = -1.63092934096575273989e-03; /* 0xBF5AB89D, 0x0B9E43E4 */

static /*const*/ double zero=  0.00000000000000000000e+00;

static double sin_pi(double x) {
        double y,z, absx;
        int4 n;

        absx = x < 0 ? -x : x;

        if(absx < 0.25) return sin(pi*x);
        y = -x;         /* x is assume negative */

    /*
     * argument reduction, make sure inexact flag not raised if input
     * is an integer
     */
        z = floor(y);
        if(z!=y) {                              /* inexact anyway */
            y  *= 0.5;
            y   = 2.0*(y - floor(y));           /* y = |x| mod 2.0 */
            n   = (int4) (y*4.0);
        } else {
            if(absx + 1 == absx) {
                y = zero; n = 0;                 /* y must be even */
            } else {
                n = ((int) y) & 1;
                y  = n;
                n<<= 2;
            }
        }
        switch (n) {
            case 0:   y =  sin(pi*y); break;
            case 1:
            case 2:   y =  cos(pi*(0.5-y)); break;
            case 3:
            case 4:   y =  sin(pi*(one-y)); break;
            case 5:
            case 6:   y = -cos(pi*(y-1.5)); break;
            default:  y =  sin(pi*(y-2.0)); break;
            }
        return -y;
}

static int math_lgamma(double x, double *gam, int *sgngam) {
        double t,y,z,nadj,p,p1,p2,p3,q,r,w;
        double absx;
        int i, neg = 0;

    /* purge off +-inf, NaN, +-0, and negative arguments */
    /* ThO: I removed the code that deals with inf and nan (Free42 does
     * not allow such values to propagate, so they should never be presented
     * as arguments to a function; if they are, what needs to be fixed is
     * the function that *returns* inf or nan, not the way functions deal
     * with such incoming arguments.
     */
        *sgngam = 1;
        if (x == 0)
            return ERR_INVALID_DATA;
        absx = x < 0 ? -x : x;
        if (absx < 8.47032947254300339068e-22) {
            /* |x|<2**-70, return -log(|x|) */
            *sgngam = -1;
            *gam = -log(absx);
            return ERR_NONE;
        }
        if (x < 0) {
            if (absx == floor(absx))
                /* -integer */
                return ERR_INVALID_DATA;
            t = sin_pi(x);
            if (t == zero)
                return ERR_INVALID_DATA;
            if (t < 0) {
                *sgngam = -1;
                nadj = log(pi / (t*x));
            } else
                nadj = log(pi / (-t*x));
            x = -x;
            neg = 1;
        }

    /* purge off 1 and 2 */
        if (x == 1 || x == 2) r = 0;
    /* for x < 2.0 */
        else if (x < 2) {
            if(x <= 0.900000095367431529603) {
                /* lgamma(x) = lgamma(x+1)-log(x) */
                r = -log(x);
                if(x >= 0.7315998077392578125) {y = one-x; i= 0;}
                else if(x >= 0.231639981269836425781) {y= x-(tc-one); i=1;}
                else {y = x; i=2;}
            } else {
                r = zero;
                if(x >= 1.73163127899169921875) {y=2.0-x;i=0;} /* [1.7316,2] */
                else if(x >= 1.231632232666015625) {y=x-tc;i=1;} /* [1.23,1.73] */
                else {y=x-one;i=2;}
            }
            switch(i) {
              case 0:
                z = y*y;
                p1 = a0+z*(a2+z*(a4+z*(a6+z*(a8+z*a10))));
                p2 = z*(a1+z*(a3+z*(a5+z*(a7+z*(a9+z*a11)))));
                p  = y*p1+p2;
                r  += (p-0.5*y); break;
              case 1:
                z = y*y;
                w = z*y;
                p1 = t0+w*(t3+w*(t6+w*(t9 +w*t12)));    /* parallel comp */
                p2 = t1+w*(t4+w*(t7+w*(t10+w*t13)));
                p3 = t2+w*(t5+w*(t8+w*(t11+w*t14)));
                p  = z*p1-(tt-w*(p2+y*p3));
                r += (tf + p); break;
              case 2:
                p1 = y*(u0+y*(u1+y*(u2+y*(u3+y*(u4+y*u5)))));
                p2 = one+y*(v1+y*(v2+y*(v3+y*(v4+y*v5))));
                r += (-0.5*y + p1/p2);
            }
        }
        else if(x < 8) {                        /* x < 8.0 */
            i = (int4)x;
            t = zero;
            y = x-(double)i;
            p = y*(s0+y*(s1+y*(s2+y*(s3+y*(s4+y*(s5+y*s6))))));
            q = one+y*(r1+y*(r2+y*(r3+y*(r4+y*(r5+y*r6)))));
            r = half*y+p/q;
            z = one;    /* lgamma(1+s) = log(s) + lgamma(s) */
            switch(i) {
            case 7: z *= (y+6.0);       /* FALLTHRU */
            case 6: z *= (y+5.0);       /* FALLTHRU */
            case 5: z *= (y+4.0);       /* FALLTHRU */
            case 4: z *= (y+3.0);       /* FALLTHRU */
            case 3: z *= (y+2.0);       /* FALLTHRU */
                    r += log(z); break;
            }
    /* 8.0 <= x < 2**58 */
        } else if (x < 288230376151711744.0) {
            t = log(x);
            z = one/x;
            y = z*z;
            w = w0+z*(w1+y*(w2+y*(w3+y*(w4+y*(w5+y*w6)))));
            r = (x-half)*(t-one)+w;
        } else
    /* 2**58 <= x <= inf */
            r =  x*(log(x)-one);
        if (neg) r = nadj - r;
        *gam = r;
        return ERR_NONE;
}

/***************************************************************/
/* Here ends the borrowed GNU C Library code (Gamma function). */
/***************************************************************/

int math_gamma(phloat phx, phloat *phgamma) {
    double x = to_double(phx);
    double gam;
    double lgam;
    int sign, err;
    if (x == 0 || (x < 0 && x == floor(x)))
        return ERR_INVALID_DATA;
    err = math_lgamma(x, &lgam, &sign);
    if (err != ERR_NONE)
        return err;
    gam = exp(lgam);
    if (p_isinf(gam)) {
        if (flags.f.range_error_ignore)
            *phgamma = sign < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    } else {
        *phgamma = sign < 0 ? -gam : gam;
    }
    return ERR_NONE;
}

#endif // BCD_MATH
