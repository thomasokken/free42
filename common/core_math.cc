/*****************************************************************************
 * Free42 -- a free HP-42S calculator clone
 * Copyright (C) 2004-2005  Thomas Okken
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *****************************************************************************/

#include <stdlib.h>

#include "core_math.h"
#include "core_commands2.h"
#include "core_decimal.h"
#include "core_display.h"
#include "core_globals.h"
#include "core_helpers.h"
#include "core_main.h"
#include "core_variables.h"
#include "shell.h"

#define SOLVE_VERSION 4
#define INTEG_VERSION 2
#define NUM_SHADOWS 10

/* Solver */
typedef struct {
    int version;
    char prgm_name[7];
    int prgm_length;
    char active_prgm_name[7];
    int active_prgm_length;
    char var_name[7];
    int var_length;
    int keep_running;
    int prev_prgm;
    int4 prev_pc;
    int state;
    int which;
    int toggle;
    int retry_counter;
    double retry_value;
    double x1, x2, x3;
    double fx1, fx2;
    double prev_x, curr_x, curr_f;
    double xm, fxm;
    char shadow_name[NUM_SHADOWS][7];
    int shadow_length[NUM_SHADOWS];
    double shadow_value[NUM_SHADOWS];
    uint4 last_disp_time;
} solve_state;

static solve_state solve;

#if 0 /* Simpson's Rule */

/* Integrator */
typedef struct {
    int version;
    char prgm_name[7];
    int prgm_length;
    char active_prgm_name[7];
    int active_prgm_length;
    char var_name[7];
    int var_length;
    int keep_running;
    int prev_prgm;
    int4 prev_pc;
    int state;
    double llim, ulim, acc;
    double h, integral, prev_integral;
    int i, n;
} integ_state;

#else /* Romberg */

#define ROMB_MAX 20

/* Integrator */
typedef struct {
    int version;
    char prgm_name[7];
    int prgm_length;
    char active_prgm_name[7];
    int active_prgm_length;
    char var_name[7];
    int var_length;
    int keep_running;
    int prev_prgm;
    int4 prev_pc;
    int state;
    double llim, ulim, acc;
    double a, b, eps;
    int n, m, i;
    double h, sum;
    double c[ROMB_MAX+1];
    int nsteps;
    double p;
    double t, u;
    double prev_int;
} integ_state;

#endif

static integ_state integ;


static void reset_solve() MATH_SECT;
static void reset_integ() MATH_SECT;


int persist_math() {
    int size = sizeof(solve_state);
    solve.version = SOLVE_VERSION;
    if (!shell_write_saved_state(&size, sizeof(int))) return 0;
    if (!shell_write_saved_state(&solve, sizeof(solve_state))) return 0;
    size = sizeof(integ_state);
    integ.version = INTEG_VERSION;
    if (!shell_write_saved_state(&size, sizeof(int))) return 0;
    if (!shell_write_saved_state(&integ, sizeof(integ_state))) return 0;
    return 1;
}

int unpersist_math() {
    int size, success;
    void *dummy;

    if (shell_read_saved_state(&size, sizeof(int)) != sizeof(int))
	return 0;
    if (size == sizeof(solve_state)) {
	if (shell_read_saved_state(&solve, size) != size)
	    return 0;
	if (solve.version != SOLVE_VERSION)
	    reset_solve();
    } else {
	dummy = malloc(size);
	if (dummy == NULL)
	    return 0;
	success = shell_read_saved_state(dummy, size) == size;
	free(dummy);
	if (!success)
	    return 0;
	reset_solve();
    }

    if (shell_read_saved_state(&size, sizeof(int)) != sizeof(int))
	return 0;
    if (size == sizeof(integ_state)) {
	if (shell_read_saved_state(&integ, size) != size)
	    return 0;
	if (integ.version != INTEG_VERSION)
	    reset_integ();
    } else {
	dummy = malloc(size);
	if (dummy == NULL)
	    return 0;
	success = shell_read_saved_state(dummy, size) == size;
	free(dummy);
	if (!success)
	    return 0;
	reset_integ();
    }

    return 1;
}

void reset_math() {
    reset_solve();
    reset_integ();
}

double math_random() {
    /* TODO Check if this is adequate -- there are probably much
     * better RNGs out there. This one is taken from the Blackjack
     * program in the HP-41C Standard Applications book.
     */
    random_number = random_number * 9821 + 0.211327;
    random_number -= floor(random_number);
    return random_number;
}

int math_asinh_acosh(double xre, double xim,
			    double *yre, double *yim, int do_asinh) {

    /* TODO: review; and deal with overflows in intermediate results */
    double are, aim, br, bphi;

    /* a = z ^ 2 +- 1 */
    are = xre * xre - xim * xim;
    if (do_asinh) are += 1; else are -= 1;
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
    return ERR_NONE;
}

int math_atanh(double xre, double xim, double *yre, double *yim) {

    double are, aim, bre, bim, cre, cim, h;

    /* TODO: review, and deal with overflows in intermediate results */
    if (xim == 0 && (xre == 1 || xre == -1))
	return ERR_INVALID_DATA;

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
 * 	Since gamma(1+s)=s*gamma(s), for x in [0,8], we may
 * 	reduce x to a number in [1.5,2.5] by
 * 		lgamma(1+s) = log(s) + lgamma(s)
 *	for example,
 *		lgamma(7.3) = log(6.3) + lgamma(6.3)
 *			    = log(6.3*5.3) + lgamma(5.3)
 *			    = log(6.3*5.3*4.3*3.3*2.3) + lgamma(2.3)
 *   2. Polynomial approximation of lgamma around its
 *	minimun ymin=1.461632144968362245 to maintain monotonicity.
 *	On [ymin-0.23, ymin+0.27] (i.e., [1.23164,1.73163]), use
 *		Let z = x-ymin;
 *		lgamma(x) = -1.214862905358496078218 + z^2*poly(z)
 *	where
 *		poly(z) is a 14 degree polynomial.
 *   2. Rational approximation in the primary interval [2,3]
 *	We use the following approximation:
 *		s = x-2.0;
 *		lgamma(x) = 0.5*s + s*P(s)/Q(s)
 *	with accuracy
 *		|P/Q - (lgamma(x)-0.5s)| < 2**-61.71
 *	Our algorithms are based on the following observation
 *
 *                             zeta(2)-1    2    zeta(3)-1    3
 * lgamma(2+s) = s*(1-Euler) + --------- * s  -  --------- * s  + ...
 *                                 2                 3
 *
 *	where Euler = 0.5771... is the Euler constant, which is very
 *	close to 0.5.
 *
 *   3. For x>=8, we have
 *	lgamma(x)~(x-0.5)log(x)-x+0.5*log(2pi)+1/(12x)-1/(360x**3)+....
 *	(better formula:
 *	   lgamma(x)~(x-0.5)*(log(x)-1)-.5*(log(2pi)-1) + ...)
 *	Let z = 1/x, then we approximation
 *		f(z) = lgamma(x) - (x-0.5)(log(x)-1)
 *	by
 *	  			    3       5             11
 *		w = w0 + w1*z + w2*z  + w3*z  + ... + w6*z
 *	where
 *		|w - f(z)| < 2**-58.74
 *
 *   4. For negative x, since (G is gamma function)
 *		-x*G(-x)*G(x) = pi/sin(pi*x),
 * 	we have
 * 		G(x) = pi/(sin(pi*x)*(-x)*G(-x))
 *	since G(-x) is positive, sign(G(x)) = sign(sin(pi*x)) for x<0
 *	Hence, for x<0, signgam = sign(sin(pi*x)) and
 *		lgamma(x) = log(|Gamma(x)|)
 *			  = log(pi/(|x*sin(pi*x)|)) - lgamma(-x);
 *	Note: one should avoid compute pi*(-x) directly in the
 *	      computation of sin(pi*(-x)).
 *
 *   5. Special Cases
 *		lgamma(2+s) ~ s*(1-Euler) for tiny s
 *		lgamma(1)=lgamma(2)=0
 *		lgamma(x) ~ -log(x) for tiny x
 *		lgamma(0) = lgamma(inf) = inf
 *	 	lgamma(-integer) = +-inf
 *
 */

/* NOTE (ThO): if the 'const' in this code is not commented out, the non-
 * Multilink PalmOS build of Free42 malfunctions in GAMMA (wrong results,
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

static double sin_pi(double x) MATH_SECT;
static double sin_pi(double x) {
	double y,z, absx;
	int4 n;

	absx = x < 0 ? -x : x;

	if(absx < 0.25) return sin(pi*x);
	y = -x;		/* x is assume negative */

    /*
     * argument reduction, make sure inexact flag not raised if input
     * is an integer
     */
	z = floor(y);
	if(z!=y) {				/* inexact anyway */
	    y  *= 0.5;
	    y   = 2.0*(y - floor(y));		/* y = |x| mod 2.0 */
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

static int math_lgamma(double x, double *gam, int *signgam) MATH_SECT;
static int math_lgamma(double x, double *gam, int *signgam) {
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
	*signgam = 1;
	if (x == 0)
	    return ERR_INVALID_DATA;
	absx = x < 0 ? -x : x;
	if (absx < 8.47032947254300339068e-22) {
	    /* |x|<2**-70, return -log(|x|) */
	    *signgam = -1;
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
		*signgam = -1;
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
		p1 = t0+w*(t3+w*(t6+w*(t9 +w*t12)));	/* parallel comp */
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
	else if(x < 8) { 			/* x < 8.0 */
	    i = (int4)x;
	    t = zero;
	    y = x-(double)i;
	    p = y*(s0+y*(s1+y*(s2+y*(s3+y*(s4+y*(s5+y*s6))))));
	    q = one+y*(r1+y*(r2+y*(r3+y*(r4+y*(r5+y*r6)))));
	    r = half*y+p/q;
	    z = one;	/* lgamma(1+s) = log(s) + lgamma(s) */
	    switch(i) {
	    case 7: z *= (y+6.0);	/* FALLTHRU */
	    case 6: z *= (y+5.0);	/* FALLTHRU */
	    case 5: z *= (y+4.0);	/* FALLTHRU */
	    case 4: z *= (y+3.0);	/* FALLTHRU */
	    case 3: z *= (y+2.0);	/* FALLTHRU */
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

int math_gamma(double x, double *gamma) {
    double lgam;
    int sign, err;
    if (x == 0 || (x < 0 && x == floor(x)))
	return ERR_INVALID_DATA;
    err = math_lgamma(x, &lgam, &sign);
    if (err != ERR_NONE)
	return err;
    *gamma = exp(lgam);
    if (isinf(*gamma)) {
	if (flags.f.range_error_ignore)
	    *gamma = sign < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    } else {
	if (sign < 0)
	    *gamma = -(*gamma);
    }
    return ERR_NONE;
}

static void reset_solve() {
    int i;
    for (i = 0; i < NUM_SHADOWS; i++)
	solve.shadow_length[i] = 0;
    solve.prgm_length = 0;
    solve.active_prgm_length = 0;
    solve.state = 0;
}

static int find_shadow(const char *name, int length) MATH_SECT;
static int find_shadow(const char *name, int length) {
    int i;
    for (i = 0; i < NUM_SHADOWS; i++)
	if (string_equals(solve.shadow_name[i], solve.shadow_length[i],
			  name, length))
	    return i;
    return -1;
}

void put_shadow(const char *name, int length, double value) {
    int i = find_shadow(name, length);
    if (i == -1) {
	for (i = 0; i < NUM_SHADOWS; i++)
	    if (solve.shadow_length[i] == 0)
		goto do_insert;
	/* No empty slots available. Remove slot 0 (the oldest) and
	 * move all subsequent ones down, freeing up slot NUM_SHADOWS - 1
	 */
	for (i = 0; i < NUM_SHADOWS - 1; i++) {
	    string_copy(solve.shadow_name[i], &solve.shadow_length[i],
			solve.shadow_name[i + 1], solve.shadow_length[i + 1]);
	    solve.shadow_value[i] = solve.shadow_value[i + 1];
	}
    }
    do_insert:
    string_copy(solve.shadow_name[i], &solve.shadow_length[i], name, length);
    solve.shadow_value[i] = value;
}

int get_shadow(const char *name, int length, double *value) {
    int i = find_shadow(name, length);
    if (i == -1)
	return 0;
    *value = solve.shadow_value[i];
    return 1;
}

void remove_shadow(const char *name, int length) {
    int i = find_shadow(name, length);
    int j;
    if (i == -1)
	return;
    for (j = i; j < NUM_SHADOWS - 1; j++) {
	string_copy(solve.shadow_name[i], &solve.shadow_length[i],
		    solve.shadow_name[i + 1], solve.shadow_length[i + 1]);
	solve.shadow_value[i] = solve.shadow_value[i + 1];
    }
    solve.shadow_length[NUM_SHADOWS - 1] = 0;
}

void set_solve_prgm(const char *name, int length) {
    string_copy(solve.prgm_name, &solve.prgm_length, name, length);
}

static int call_solve_fn(int which, int state) MATH_SECT;
static int call_solve_fn(int which, int state) {
    int err, i;
    arg_struct arg;
    vartype *v = recall_var(solve.var_name, solve.var_length);
    double x = which == 1 ? solve.x1 : which == 2 ? solve.x2 : solve.x3;
    solve.prev_x = solve.curr_x;
    solve.curr_x = x;
    if (v == NULL || v->type != TYPE_REAL) {
	v = new_real(x);
	if (v == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	store_var(solve.var_name, solve.var_length, v);
    } else
	((vartype_real *) v)->x = x;
    solve.which = which;
    solve.state = state;
    arg.type = ARGTYPE_STR;
    arg.length = solve.active_prgm_length;
    for (i = 0; i < arg.length; i++)
	arg.val.text[i] = solve.active_prgm_name[i];
    err = docmd_gto(&arg);
    if (err != ERR_NONE) {
	free_vartype(v);
	return err;
    }
    push_rtn_addr(-2, 0);
    return ERR_RUN;
}

int start_solve(const char *name, int length, double x1, double x2) {
    if (solve_active())
	return ERR_SOLVE_SOLVE;
    string_copy(solve.var_name, &solve.var_length, name, length);
    string_copy(solve.active_prgm_name, &solve.active_prgm_length,
		solve.prgm_name, solve.prgm_length);
    solve.prev_prgm = current_prgm;
    solve.prev_pc = pc;
    if (x1 == x2) {
	if (x1 == 0) {
	    x2 = 1;
	    solve.retry_counter = 0;
	} else {
	    x2 = x1 * 1.000001;
	    if (isinf(x2))
		x2 = x1 * 0.999999;
	    solve.retry_counter = -10;
	}
    } else {
	solve.retry_counter = 10;
	solve.retry_value = fabs(x1) < fabs(x2) ? x1 : x2;
    }
    if (x1 < x2) {
	solve.x1 = x1;
	solve.x2 = x2;
    } else {
	solve.x1 = x2;
	solve.x2 = x1;
    }
    solve.last_disp_time = 0;
    solve.toggle = 1;
    solve.keep_running = program_running();
    return call_solve_fn(1, 1);
}

typedef struct {
    char *text;
    int length;
} message_spec;

#define SOLVE_ROOT          0
#define SOLVE_SIGN_REVERSAL 1
#define SOLVE_EXTREMUM      2
#define SOLVE_BAD_GUESSES   3
#define SOLVE_CONSTANT      4

static const message_spec solve_message[] MATH_SECT = {
    { NULL,             0 },
    { "Sign Reversal", 13 },
    { "Extremum",       8 },
    { "Bad Guess(es)", 13 },
    { "Constant?",      9 }
};

static int finish_solve(int message) MATH_SECT;
static int finish_solve(int message) {
    vartype *v, *new_x, *new_y, *new_z, *new_t;
    arg_struct arg;
    int dummy, print;

    solve.state = 0;

    v = recall_var(solve.var_name, solve.var_length);
    ((vartype_real *) v)->x = solve.which == 1 ? solve.x1 :
				solve.which == 2 ? solve.x2 : solve.x3;
    new_x = dup_vartype(v);
    new_y = new_real(solve.prev_x);
    new_z = new_real(solve.curr_f);
    new_t = new_real(message);
    if (new_x == NULL || new_y == NULL || new_z == NULL || new_t == NULL) {
	free_vartype(new_x);
	free_vartype(new_y);
	free_vartype(new_z);
	free_vartype(new_t);
	return ERR_INSUFFICIENT_MEMORY;
    }
    free_vartype(reg_x);
    free_vartype(reg_y);
    free_vartype(reg_z);
    free_vartype(reg_t);
    reg_x = new_x;
    reg_y = new_y;
    reg_z = new_z;
    reg_t = new_t;

    current_prgm = solve.prev_prgm;
    pc = solve.prev_pc;

    arg.type = ARGTYPE_STR;
    string_copy(arg.val.text, &dummy, solve.var_name, solve.var_length);
    arg.length = solve.var_length;

    print = flags.f.trace_print && flags.f.printer_exists;

    if (!solve.keep_running) {
	view_helper(&arg, print);
	if (message != 0) {
	    clear_row(1);
	    draw_string(0, 1, solve_message[message].text,
			      solve_message[message].length);
	    flags.f.message = 1;
	    flags.f.two_line_message = 1;
	    flush_display();
	}
    } else {
	if (print) {
	    char namebuf[8], valbuf[22];
	    int namelen = 0, vallen;
	    string2buf(namebuf, 8, &namelen, solve.var_name, solve.var_length);
	    char2buf(namebuf, 8, &namelen, '=');
	    vallen = vartype2string(v, valbuf, 22);
	    print_wide(namebuf, namelen, valbuf, vallen);
	}
    }

    if (print && message != 0)
	print_lines(solve_message[message].text,
		    solve_message[message].length, 1);

    return solve.keep_running ? ERR_NONE : ERR_STOP;
}

int return_to_solve(int failure) {
    double f, slope, s, prev_f = solve.curr_f;
    uint4 now_time;

    if (solve.state == 0)
	return ERR_INTERNAL_ERROR;
    if (!failure) {
	if (reg_x->type == TYPE_REAL) {
	    f = ((vartype_real *) reg_x)->x;
	    solve.curr_f = f;
	    if (f == 0)
		return finish_solve(SOLVE_ROOT);
	} else {
	    solve.curr_f = POS_HUGE_DOUBLE;
	    failure = 1;
	}
    } else
	solve.curr_f = POS_HUGE_DOUBLE;

    if (!failure && solve.retry_counter != 0) {
	if (solve.retry_counter > 0)
	    solve.retry_counter--;
	else
	    solve.retry_counter++;
    }

    now_time = shell_milliseconds();
    if (!solve.keep_running && solve.state > 1
				&& now_time >= solve.last_disp_time + 250) {
	/* Put on a show so the user won't think we're just drinking beer
	 * while they're waiting anxiously for the solver to converge...
	 */
	char buf[22];
	int bufptr = 0, i;
	solve.last_disp_time = now_time;
	clear_display();
	bufptr = double2string(solve.curr_x, buf, 22, 0, 0, 3,
				    flags.f.thousands_separators);
	for (i = bufptr; i < 21; i++)
	    buf[i] = ' ';
	buf[21] = failure ? '?' : solve.curr_f > 0 ? '+' : '-';
	draw_string(0, 0, buf, 22);
	bufptr = double2string(solve.prev_x, buf, 22, 0, 0, 3,
				    flags.f.thousands_separators);
	for (i = bufptr; i < 21; i++)
	    buf[i] = ' ';
	buf[21] = prev_f == POS_HUGE_DOUBLE ? '?' : prev_f > 0 ? '+' : '-';
	draw_string(0, 1, buf, 22);
	flush_display();
	flags.f.message = 1;
	flags.f.two_line_message = 1;
    }

    switch (solve.state) {

	case 1:
	    /* first evaluation of x1 */
	    if (failure) {
		if (solve.retry_counter > 0)
		    solve.retry_counter = -solve.retry_counter;
		return call_solve_fn(2, 2);
	    } else {
		solve.fx1 = f;
		return call_solve_fn(2, 3);
	    }

	case 2:
	    /* first evaluation of x2 after x1 was unsuccessful */
	    if (failure)
		return finish_solve(SOLVE_BAD_GUESSES);
	    solve.fx2 = f;
	    solve.x1 = (solve.x1 + solve.x2) / 2;
	    if (solve.x1 == solve.x2)
		return finish_solve(SOLVE_BAD_GUESSES);
	    return call_solve_fn(1, 3);

	case 3:
	    /* make sure f(x1) != f(x2) */
	    if (failure) {
		if (solve.which == 1)
		    solve.x1 = (solve.x1 + solve.x2) / 2;
		else
		    solve.x2 = (solve.x1 + solve.x2) / 2;
		if (solve.x1 == solve.x2)
		    return finish_solve(SOLVE_BAD_GUESSES);
		return call_solve_fn(solve.which, 3);
	    }
	    if (solve.which == 1)
		solve.fx1 = f;
	    else
		solve.fx2 = f;
	    if (solve.fx1 == solve.fx2) {
		/* If f(x1) == f(x2), we assume we're in a local flat spot.
		 * We extend the interval exponentially until we have two
		 * values of x, both of which are evaluated successfully,
		 * and yielding different values; from that moment on, we can
		 * apply the secant method.
		 */
		int which;
		double x;
		if (solve.toggle) {
		    x = solve.x2 + 100 * (solve.x2 - solve.x1);
		    if (isinf(x)) {
			if (solve.retry_counter != 0)
			    goto retry_solve;
			return finish_solve(SOLVE_CONSTANT);
		    }
		    which = 2;
		    solve.x2 = x;
		} else {
		    x = solve.x1 - 100 * (solve.x2 - solve.x1);
		    if (isinf(x)) {
			if (solve.retry_counter != 0)
			    goto retry_solve;
			return finish_solve(SOLVE_CONSTANT);
		    }
		    which = 1;
		    solve.x1 = x;
		}
		solve.toggle = !solve.toggle;
		return call_solve_fn(which, 3);
	    }
	    /* When we get here, f(x1) != f(x2), and we can start
	     * applying the secant method.
	     */
	    goto do_secant;

	case 4:
	    /* secant method, evaluated x3 */
	case 5:
	    /* just performed bisection */
	    if (failure) {
		if (solve.x3 > solve.x2) {
		    /* Failure outside [x1, x2]; approach x2 */
		    solve.x3 = (solve.x2 + solve.x3) / 2;
		    if (solve.x3 == solve.x2)
			return finish_solve(SOLVE_EXTREMUM);
		} else if (solve.x3 < solve.x1) {
		    /* Failure outside [x1, x2]; approach x1 */
		    solve.x3 = (solve.x1 + solve.x3) / 2;
		    if (solve.x3 == solve.x1)
			return finish_solve(SOLVE_EXTREMUM);
		} else {
		    /* Failure inside [x1, x2];
		     * alternately approach x1 and x2 */
		    if (solve.toggle) {
			double old_x3 = solve.x3;
			if (solve.x3 <= (solve.x1 + solve.x2) / 2)
			    solve.x3 = (solve.x1 + solve.x3) / 2;
			else
			    solve.x3 = (solve.x2 + solve.x3) / 2;
			if (solve.x3 == old_x3)
			    return finish_solve(SOLVE_SIGN_REVERSAL);
		    } else
			solve.x3 = solve.x1 + solve.x2 - solve.x3;
		    solve.toggle = !solve.toggle;
		    if (solve.x3 == solve.x1 || solve.x3 == solve.x2)
			return finish_solve(SOLVE_SIGN_REVERSAL);
		}
		return call_solve_fn(3, 4);
	    } else if (solve.fx1 > 0 && solve.fx2 > 0) {
		if (solve.fx1 > solve.fx2) {
		    if (f >= solve.fx1 && solve.state != 5)
			goto do_bisection;
		    solve.x1 = solve.x3;
		    solve.fx1 = f;
		} else {
		    if (f >= solve.fx2 && solve.state != 5)
			goto do_bisection;
		    solve.x2 = solve.x3;
		    solve.fx2 = f;
		}
	    } else if (solve.fx1 < 0 && solve.fx2 < 0) {
		if (solve.fx1 < solve.fx2) {
		    if (f <= solve.fx1 && solve.state != 5)
			goto do_bisection;
		    solve.x1 = solve.x3;
		    solve.fx1 = f;
		} else {
		    if (f <= solve.fx2 && solve.state != 5)
			goto do_bisection;
		    solve.x2 = solve.x3;
		    solve.fx2 = f;
		}
	    } else {
		/* f(x1) and f(x2) have opposite signs; assuming f is
		 * continuous on the interval [x1, x2], there is at least
		 * one root. We use x3 to replace x1 or x2 and narrow the
		 * interval, even if f(x3) is actually worse than f(x1) and
		 * f(x2). This way we're guaranteed to home in on the root
		 * (but of course we'll get stuck if we encounter a
		 * discontinuous sign reversal instead, e.g. 1/x at x = 0).
		 * Such is life.
		 */
		if ((solve.fx1 > 0 && f > 0) || (solve.fx1 < 0 && f < 0)) {
		    solve.x1 = solve.x3;
		    solve.fx1 = f;
		} else {
		    solve.x2 = solve.x3;
		    solve.fx2 = f;
		}
	    }
	    if (solve.x2 < solve.x1) {
		/* Make sure x1 is always less than x2 */
		double tmp = solve.x1;
		solve.x1 = solve.x2;
		solve.x2 = tmp;
		tmp = solve.fx1;
		solve.fx1 = solve.fx2;
		solve.fx2 = tmp;
	    }
	    do_secant:
	    if (solve.fx1 == solve.fx2)
		return finish_solve(SOLVE_EXTREMUM);
	    if ((solve.fx1 > 0 && solve.fx2 < 0)
		    || (solve.fx1 < 0 && solve.fx2 > 0))
		goto do_ridders;
	    slope = (solve.fx2 - solve.fx1) / (solve.x2 - solve.x1);
	    if (isinf(slope)) {
		solve.x3 = (solve.x1 + solve.x2) / 2;
		if (solve.x3 == solve.x1 || solve.x3 == solve.x2)
		    return finish_solve(SOLVE_ROOT);
		else
		    return call_solve_fn(3, 4);
	    } else if (slope == 0) {
		/* Underflow caused by x2 - x1 being too big.
		 * We're changing the calculation sequence to steer
		 * clear of trouble.
		 */
		solve.x3 = solve.x1 - solve.fx1 * (solve.x2 - solve.x1)
						/ (solve.fx2 - solve.fx1);

		goto finish_secant;
	    } else {
		int inf;
		solve.x3 = solve.x1 - solve.fx1 / slope;
		finish_secant:
		if ((inf = isinf(solve.x3)) != 0) {
		    if (solve.retry_counter != 0)
			goto retry_solve;
		    return finish_solve(SOLVE_EXTREMUM);
		}
		/* The next two 'if' statements deal with the case that the
		 * secant extrapolation returns one of the points we already
		 * had. We assume this means no improvement is possible (TODO:
		 * this is not necessarily true; Kahan's 34C article has an
		 * example of this -- the thing to be careful of is that the
		 * 'bad' value is so bad that the secant becomes excessively
		 * steep). We fudge the 'solve' struct a bit to make sure we
		 * don't return the 'bad' value as the root.
		 */
		if (solve.x3 == solve.x1) {
		    solve.which = 1;
		    solve.curr_f = solve.fx1;
		    solve.prev_x = solve.x2;
		    return finish_solve(SOLVE_ROOT);
		}
		if (solve.x3 == solve.x2) {
		    solve.which = 2;
		    solve.curr_f = solve.fx2;
		    solve.prev_x = solve.x1;
		    return finish_solve(SOLVE_ROOT);
		}
		/* If we're extrapolating, make sure we don't race away from
		 * the current interval too quickly */
		if (solve.x3 < solve.x1) {
		    double min = solve.x1 - 100 * (solve.x2 - solve.x1);
		    if (solve.x3 < min)
			solve.x3 = min;
		} else if (solve.x3 > solve.x2) {
		    double max = solve.x2 + 100 * (solve.x2 - solve.x1);
		    if (solve.x3 > max)
			solve.x3 = max;
		} else {
		    /* If we're interpolating, make sure we actually make
		     * some progress. Enforce a minimum distance between x3
		     * and the edges of the interval.
		     */
		    double eps = (solve.x2 - solve.x1) / 10;
		    if (solve.x3 < solve.x1 + eps)
			solve.x3 = solve.x1 + eps;
		    else if (solve.x3 > solve.x2 - eps)
			solve.x3 = solve.x2 - eps;
		}
		return call_solve_fn(3, 4);
	    }

	    retry_solve:
	    /* We hit infinity without finding two values of x where f(x) has
	     * opposite signs, but we got to infinity suspiciously quickly.
	     * If we started with two guesses, we now retry with only the lower
	     * of the two; if we started with one guess, we now retry with
	     * starting guesses of 0 and 1.
	     */
	    if (solve.retry_counter > 0) {
		solve.x1 = solve.retry_value;
		solve.x2 = solve.x1 * 1.000001;
		if (isinf(solve.x2))
		    solve.x2 = solve.x1 * 0.999999;
		if (solve.x1 > solve.x2) {
		    double tmp = solve.x1;
		    solve.x1 = solve.x2;
		    solve.x2 = tmp;
		}
		solve.retry_counter = -10;
	    } else {
		solve.x1 = 0;
		solve.x2 = 1;
		solve.retry_counter = 0;
	    }
	    return call_solve_fn(1, 1);

	    do_bisection:
	    solve.x3 = (solve.x1 + solve.x2) / 2;
	    return call_solve_fn(3, 5);

	case 6:
	    /* Ridders' method, evaluated midpoint */
	    if (failure)
		goto do_bisection;
	    s = sqrt(f * f - solve.fx1 * solve.fx2);
	    if (s == 0)
		/* Mathematically impossible, but numerically possible if
		 * the function is so close to zero that f^2 underflows.
		 * We could handle this better but this seems adequate.
		 */
		return finish_solve(SOLVE_ROOT);
	    solve.xm = solve.x3;
	    solve.fxm = f;
	    if (solve.fx1 < solve.fx2)
		s = -s;
	    solve.x3 = solve.xm + (solve.xm - solve.x1) * (solve.fxm / s);
	    if (solve.x3 == solve.x1 || solve.x3 == solve.x2)
		return finish_solve(SOLVE_ROOT);
	    return call_solve_fn(3, 7);

	case 7:
	    /* Ridders' method, evaluated xnew */
	    if (failure)
		goto do_bisection;
	    if ((f > 0 && solve.fxm < 0) || (f < 0 && solve.fxm > 0)) {
		if (solve.xm < solve.x3) {
		    solve.x1 = solve.xm;
		    solve.fx1 = solve.fxm;
		    solve.x2 = solve.x3;
		    solve.fx2 = f;
		} else {
		    solve.x1 = solve.x3;
		    solve.fx1 = f;
		    solve.x2 = solve.xm;
		    solve.fx2 = solve.fxm;
		}
	    } else if ((f > 0 && solve.fx1 < 0) || (f < 0 && solve.fx1 > 0)) {
		solve.x2 = solve.x3;
		solve.fx2 = f;
	    } else /* f > 0 && solve.fx2 < 0 || f < 0 && solve.fx2 > 0 */ {
		solve.x1 = solve.x3;
		solve.fx1 = f;
	    }
	    do_ridders:
	    solve.x3 = (solve.x1 + solve.x2) / 2;
	    if (solve.x3 == solve.x1 || solve.x3 == solve.x2)
		return finish_solve(SOLVE_ROOT);
	    else
		return call_solve_fn(3, 6);

	default:
	    return ERR_INTERNAL_ERROR;
    }
}

static void reset_integ() {
    integ.prgm_length = 0;
    integ.active_prgm_length = 0;
    integ.state = 0;
}

void set_integ_prgm(const char *name, int length) {
    string_copy(integ.prgm_name, &integ.prgm_length, name, length);
}

void get_integ_prgm(char *name, int *length) {
    string_copy(name, length, integ.prgm_name, integ.prgm_length);
}

void set_integ_var(const char *name, int length) {
    string_copy(integ.var_name, &integ.var_length, name, length);
}

void get_integ_var(char *name, int *length) {
    string_copy(name, length, integ.var_name, integ.var_length);
}

#if 0 /* Simpson's Rule */

static int call_integ_fn() MATH_SECT;
static int call_integ_fn() {
    int err, i;
    arg_struct arg;
    double x = integ.llim + integ.h * integ.i;
    vartype *v = recall_var(integ.var_name, integ.var_length);
    if (v == NULL || v->type != TYPE_REAL) {
	v = new_real(x);
	if (v == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	store_var(integ.var_name, integ.var_length, v);
    } else
	((vartype_real *) v)->x = x;
    arg.type = ARGTYPE_STR;
    arg.length = integ.active_prgm_length;
    for (i = 0; i < arg.length; i++)
	arg.val.text[i] = integ.active_prgm_name[i];
    err = docmd_gto(&arg);
    if (err != ERR_NONE) {
	free_vartype(v);
	return err;
    }
    push_rtn_addr(-3, 0);
    return ERR_RUN;
}

int start_integ(const char *name, int length) {
    vartype *v;
    if (integ_active())
	return ERR_INTEG_INTEG;
    v = recall_var("LLIM", 4);
    if (v == NULL)
	return ERR_NONEXISTENT;
    else if (v->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else if (v->type != TYPE_REAL)
	return ERR_INVALID_TYPE;
    integ.llim = ((vartype_real *) v)->x;
    v = recall_var("ULIM", 4);
    if (v == NULL)
	return ERR_NONEXISTENT;
    else if (v->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else if (v->type != TYPE_REAL)
	return ERR_INVALID_TYPE;
    integ.ulim = ((vartype_real *) v)->x;
    v = recall_var("ACC", 3);
    if (v == NULL)
	integ.acc = 0;
    else if (v->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else if (v->type != TYPE_REAL)
	return ERR_INVALID_TYPE;
    else
	integ.acc = ((vartype_real *) v)->x;
    if (integ.acc < 0)
	integ.acc = 0;
    string_copy(integ.var_name, &integ.var_length, name, length);
    string_copy(integ.active_prgm_name, &integ.active_prgm_length,
		integ.prgm_name, integ.prgm_length);
    integ.prev_prgm = current_prgm;
    integ.prev_pc = pc;
    integ.integral = 0;
    integ.n = 10;
    integ.i = 0;
    integ.h = (integ.ulim - integ.llim) / integ.n;
    integ.state = 1;
    integ.keep_running = program_running();
    if (!integ.keep_running) {
	clear_row(0);
	draw_string(0, 0, "Integrating", 11);
	flush_display();
	flags.f.message = 1;
	flags.f.two_line_message = 0;
    }
    return call_integ_fn();
}

static int finish_integ() MATH_SECT;
static int finish_integ() {
    vartype *v;
    int saved_trace = flags.f.trace_print;
    integ.state = 0;

    v = new_real(integ.integral);
    if (v == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    flags.f.trace_print = 0;
    recall_result(v);
    flags.f.trace_print = saved_trace;

    current_prgm = integ.prev_prgm;
    pc = integ.prev_pc;

    if (!integ.keep_running) {
	char buf[22];
	int bufptr = 0;
	string2buf(buf, 22, &bufptr, "\003=", 2);
	bufptr += vartype2string(v, buf + bufptr, 22 - bufptr);
	clear_row(0);
	draw_string(0, 0, buf, bufptr);
	flush_display();
	flags.f.message = 1;
	flags.f.two_line_message = 0;
	if (flags.f.trace_print && flags.f.printer_exists)
	    print_wide(buf, 2, buf + 2, bufptr - 2);
	return ERR_STOP;
    } else
	return ERR_NONE;
}

int return_to_integ(int failure) {
    /* TODO -- better algorithm... This one uses Simpson's rule, initially
     * with 10 steps (h = (ulim - llim) / 10), then keeps halving the step
     * size until two successive approximations are no more than 'acc'
     * apart. The HP-42S uses a smarter algorithm which uses local error
     * estimates to refine the step size only where needed (i.e. where the
     * derivative of the integrand is high).
     */

    double y;
    if (failure || reg_x->type != TYPE_REAL)
	y = 0;
    else
	y = ((vartype_real *) reg_x)->x;

    switch (integ.state) {
	case 0:
	    return ERR_INTERNAL_ERROR;
	case 1:
	case 2:
	    if (integ.i == 0 || integ.i == integ.n)
		integ.integral += (integ.h * 0.3333333333333333333333333) * y;
	    else if (integ.i % 2 == 0)
		integ.integral += (integ.h * 0.6666666666666666666666667) * y;
	    else
		integ.integral += (integ.h * 1.333333333333333333333333) * y;
	    if (integ.i == integ.n) {
		if (integ.state == 2) {
		    double eps = integ.prev_integral - integ.integral;
		    if (eps < 0)
			eps = -eps;
		    if (eps <= integ.acc)
			return finish_integ();
		}
		integ.prev_integral = integ.integral;
		integ.integral = 0;
		integ.n *= 2;
		integ.i = 0;
		integ.h = (integ.ulim - integ.llim) / integ.n;
		integ.state = 2;
	    } else
		integ.i++;
	    return call_integ_fn();
	default:
	    return ERR_INTERNAL_ERROR;
    }
}

#else /* Romberg */

static int call_integ_fn() MATH_SECT;
static int call_integ_fn() {
    int err, i;
    arg_struct arg;
    double x = integ.u;
    vartype *v = recall_var(integ.var_name, integ.var_length);
    if (v == NULL || v->type != TYPE_REAL) {
	v = new_real(x);
	if (v == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	store_var(integ.var_name, integ.var_length, v);
    } else
	((vartype_real *) v)->x = x;
    arg.type = ARGTYPE_STR;
    arg.length = integ.active_prgm_length;
    for (i = 0; i < arg.length; i++)
	arg.val.text[i] = integ.active_prgm_name[i];
    err = docmd_gto(&arg);
    if (err != ERR_NONE) {
	free_vartype(v);
	return err;
    }
    push_rtn_addr(-3, 0);
    return ERR_RUN;
}

int start_integ(const char *name, int length) {
    vartype *v;
    if (integ_active())
	return ERR_INTEG_INTEG;
    v = recall_var("LLIM", 4);
    if (v == NULL)
	return ERR_NONEXISTENT;
    else if (v->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else if (v->type != TYPE_REAL)
	return ERR_INVALID_TYPE;
    integ.llim = ((vartype_real *) v)->x;
    v = recall_var("ULIM", 4);
    if (v == NULL)
	return ERR_NONEXISTENT;
    else if (v->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else if (v->type != TYPE_REAL)
	return ERR_INVALID_TYPE;
    integ.ulim = ((vartype_real *) v)->x;
    v = recall_var("ACC", 3);
    if (v == NULL)
	integ.acc = 0;
    else if (v->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else if (v->type != TYPE_REAL)
	return ERR_INVALID_TYPE;
    else
	integ.acc = ((vartype_real *) v)->x;
    if (integ.acc < 0)
	integ.acc = 0;
    string_copy(integ.var_name, &integ.var_length, name, length);
    string_copy(integ.active_prgm_name, &integ.active_prgm_length,
		integ.prgm_name, integ.prgm_length);
    integ.prev_prgm = current_prgm;
    integ.prev_pc = pc;

    integ.a = integ.llim;
    integ.b = integ.ulim - integ.llim;
    integ.h = 2;
    integ.c[0] = 0;
    integ.prev_int = 0;
    integ.nsteps = 1;
    integ.n = 0;
    integ.state = 1;

    integ.keep_running = program_running();
    if (!integ.keep_running) {
	clear_row(0);
	draw_string(0, 0, "Integrating", 11);
	flush_display();
	flags.f.message = 1;
	flags.f.two_line_message = 0;
    }
    return return_to_integ(0);
}

static int finish_integ() MATH_SECT;
static int finish_integ() {
    vartype *x, *y;
    int saved_trace = flags.f.trace_print;
    integ.state = 0;

    x = new_real(integ.c[integ.i] * integ.b * 0.75);
    y = new_real(integ.eps);
    if (x == NULL || y == NULL) {
	free_vartype(x);
	free_vartype(y);
	return ERR_INSUFFICIENT_MEMORY;
    }
    flags.f.trace_print = 0;
    recall_two_results(x, y);
    flags.f.trace_print = saved_trace;

    current_prgm = integ.prev_prgm;
    pc = integ.prev_pc;

    if (!integ.keep_running) {
	char buf[22];
	int bufptr = 0;
	string2buf(buf, 22, &bufptr, "\003=", 2);
	bufptr += vartype2string(x, buf + bufptr, 22 - bufptr);
	clear_row(0);
	draw_string(0, 0, buf, bufptr);
	flush_display();
	flags.f.message = 1;
	flags.f.two_line_message = 0;
	if (flags.f.trace_print && flags.f.printer_exists)
	    print_wide(buf, 2, buf + 2, bufptr - 2);
	return ERR_STOP;
    } else
	return ERR_NONE;
}


/* approximate integral of `f' between `a' and `b' subject to a given
 * error. Use Romberg method with refinement substitution, x = (3u-u^3)/2
 * which prevents endpoint evaluation and causes non-uniform sampling.
 */

int return_to_integ(int failure) {
    switch (integ.state) {
	case 0:
	    return ERR_INTERNAL_ERROR;

	case 1:
	    integ.state = 2;

	    loop1:

	    integ.p = integ.h / 2 - 1;
	    integ.sum = 0.0;
	    integ.i = 0;

		loop2:

		integ.t = 1 - integ.p * integ.p;
		integ.u = integ.p + integ.t * integ.p / 2;
		integ.u = (integ.u * integ.b + integ.b) / 2 + integ.a;
		return call_integ_fn();

	case 2:
		if (!failure && reg_x->type == TYPE_REAL)
		    integ.sum += integ.t * ((vartype_real *) reg_x)->x;
		integ.p += integ.h;
		if (++integ.i < integ.nsteps)
		    goto loop2;

	    integ.p = 4;
	    integ.t = integ.c[0];
	    integ.c[0] = (integ.c[0] + integ.h * integ.sum) / 2.0;
	    for (integ.i = 0; integ.i <= integ.n; integ.i++) {
		integ.u = integ.c[integ.i + 1];
		integ.c[integ.i + 1] =
			(integ.p * integ.c[integ.i] - integ.t) / (integ.p - 1);
		integ.t = integ.u;
		integ.p *= 4;
	    }

	    /* NOTE: I'm delaying the convergence check until the 5th
	     * iteration, just to make sure we don't jump to conclusions
	     * after having only sampled a handful of data points.
	     * Maybe that's cheating, but it's necessary to make certain
	     * nasty cases (x*exp(-x), exp(-x)/x) work right.
	     */
	    if (integ.n > 3) {
		integ.eps = (integ.c[integ.i] - integ.prev_int)
							* integ.b * 0.75;
		if (integ.eps < 0)
		    integ.eps = -integ.eps;
		if (integ.eps <= integ.acc)
		    return finish_integ();
	    }

	    integ.prev_int = integ.c[integ.i];
	    integ.nsteps <<= 1;
	    integ.h /= 2.0;

	    if (++integ.n < ROMB_MAX)
		goto loop1;
	    else
		return finish_integ();

	default:
	    return ERR_INTERNAL_ERROR;
    }
}

#endif /* Simpson's Rule / Romberg */
