// 15 August 1997, Rick Huebner:  Small changes made to adapt for MathLib

/* @(#)e_rem_pio2.c 5.1 93/09/24 */
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

#if defined(LIBM_SCCS) && !defined(lint)
static char rcsid[] = "$NetBSD: e_rem_pio2.c,v 1.8 1995/05/10 20:46:02 jtc Exp $";
#endif

/* __ieee754_rem_pio2(x,y)
 * 
 * return the remainder of x rem pi/2 in y[0]+y[1] 
 * use __kernel_rem_pio2()
 */

#include "mathlib_math.h"
#include "mathlib_math_private.h"

/*
 * Table of constants for 2/pi, 396 Hex digits (476 decimal) of 2/pi 
 */
#ifdef _NO_STATIC_ARRAYS_
#ifdef __STDC__
static const int32_t two_over_pi[] = {
#else
static int32_t two_over_pi[] = {
#endif
0xA2F983, 0x6E4E44, 0x1529FC, 0x2757D1, 0xF534DD, 0xC0DB62, 
0x95993C, 0x439041, 0xFE5163, 0xABDEBB, 0xC561B7, 0x246E3A, 
0x424DD2, 0xE00649, 0x2EEA09, 0xD1921C, 0xFE1DEB, 0x1CB129, 
0xA73EE8, 0x8235F5, 0x2EBB44, 0x84E99C, 0x7026B4, 0x5F7E41, 
0x3991D6, 0x398353, 0x39F49C, 0x845F8B, 0xBDF928, 0x3B1FF8, 
0x97FFDE, 0x05980F, 0xEF2F11, 0x8B5A0A, 0x6D1F6D, 0x367ECF, 
0x27CB09, 0xB74F46, 0x3F669E, 0x5FEA2D, 0x7527BA, 0xC7EBE5, 
0xF17B3D, 0x0739F7, 0x8A5292, 0xEA6BFB, 0x5FB11F, 0x8D5D08, 
0x560330, 0x46FC7B, 0x6BABF0, 0xCFBC20, 0x9AF436, 0x1DA9E3, 
0x91615E, 0xE61B08, 0x659985, 0x5F14A0, 0x68408D, 0xFFD880, 
0x4D7327, 0x310606, 0x1556CA, 0x73A8C9, 0x60E27B, 0xC08C6B, 
};

#ifdef __STDC__
static const int32_t npio2_hw[] = {
#else
static int32_t npio2_hw[] = {
#endif
0x3FF921FB, 0x400921FB, 0x4012D97C, 0x401921FB, 0x401F6A7A, 0x4022D97C,
0x4025FDBB, 0x402921FB, 0x402C463A, 0x402F6A7A, 0x4031475C, 0x4032D97C,
0x40346B9C, 0x4035FDBB, 0x40378FDB, 0x403921FB, 0x403AB41B, 0x403C463A,
0x403DD85A, 0x403F6A7A, 0x40407E4C, 0x4041475C, 0x4042106C, 0x4042D97C,
0x4043A28C, 0x40446B9C, 0x404534AC, 0x4045FDBB, 0x4046C6CB, 0x40478FDB,
0x404858EB, 0x404921FB,
};
#endif	// _NO_STATIC_ARRAYS_

/*
 * invpio2:  53 bits of 2/pi
 * pio2_1:   first  33 bit of pi/2
 * pio2_1t:  pi/2 - pio2_1
 * pio2_2:   second 33 bit of pi/2
 * pio2_2t:  pi/2 - (pio2_1+pio2_2)
 * pio2_3:   third  33 bit of pi/2
 * pio2_3t:  pi/2 - (pio2_1+pio2_2+pio2_3)
 */

#ifdef __STDC__
static const double 
#else
static double 
#endif
zero =  0.00000000000000000000e+00, /* 0x00000000, 0x00000000 */
half =  5.00000000000000000000e-01, /* 0x3FE00000, 0x00000000 */
two24 =  1.67772160000000000000e+07, /* 0x41700000, 0x00000000 */
invpio2 =  6.36619772367581382433e-01, /* 0x3FE45F30, 0x6DC9C883 */
pio2_1  =  1.57079632673412561417e+00, /* 0x3FF921FB, 0x54400000 */
pio2_1t =  6.07710050650619224932e-11, /* 0x3DD0B461, 0x1A626331 */
pio2_2  =  6.07710050630396597660e-11, /* 0x3DD0B461, 0x1A600000 */
pio2_2t =  2.02226624879595063154e-21, /* 0x3BA3198A, 0x2E037073 */
pio2_3  =  2.02226624871116645580e-21, /* 0x3BA3198A, 0x2E000000 */
pio2_3t =  8.47842766036889956997e-32; /* 0x397B839A, 0x252049C1 */

#ifdef __STDC__
	int32_t __ieee754_rem_pio2(double x, double *y)
#else
	int32_t __ieee754_rem_pio2(x,y)
	double x,y[];
#endif
{
	int32_t two_over_pi[66], npio2_hw[32];
	double z,w,t,r,fn;
	double tx[3];
	int32_t e0,i,j,nx,n,ix,hx;
	u_int32_t low;

	// Yeah, I know this stinks, but shared libraries can't have any static arrays
	// or other global data, so I had to turn all this crud into runtime assignments
	// to force it into the code segment.  I don't like it any more than you do.
	two_over_pi[0]=0xA2F983;  two_over_pi[1]=0x6E4E44;  two_over_pi[2]=0x1529FC;  two_over_pi[3]=0x2757D1;  two_over_pi[4]=0xF534DD;  two_over_pi[5]=0xC0DB62;
	two_over_pi[6]=0x95993C;  two_over_pi[7]=0x439041;  two_over_pi[8]=0xFE5163;  two_over_pi[9]=0xABDEBB;  two_over_pi[10]=0xC561B7; two_over_pi[11]=0x246E3A; 
	two_over_pi[12]=0x424DD2; two_over_pi[13]=0xE00649; two_over_pi[14]=0x2EEA09; two_over_pi[15]=0xD1921C; two_over_pi[16]=0xFE1DEB; two_over_pi[17]=0x1CB129; 
	two_over_pi[18]=0xA73EE8; two_over_pi[19]=0x8235F5; two_over_pi[20]=0x2EBB44; two_over_pi[21]=0x84E99C; two_over_pi[22]=0x7026B4; two_over_pi[23]=0x5F7E41; 
	two_over_pi[24]=0x3991D6; two_over_pi[25]=0x398353; two_over_pi[26]=0x39F49C; two_over_pi[27]=0x845F8B; two_over_pi[28]=0xBDF928; two_over_pi[29]=0x3B1FF8; 
	two_over_pi[30]=0x97FFDE; two_over_pi[31]=0x05980F; two_over_pi[32]=0xEF2F11; two_over_pi[33]=0x8B5A0A; two_over_pi[34]=0x6D1F6D; two_over_pi[35]=0x367ECF; 
	two_over_pi[36]=0x27CB09; two_over_pi[37]=0xB74F46; two_over_pi[38]=0x3F669E; two_over_pi[39]=0x5FEA2D; two_over_pi[40]=0x7527BA; two_over_pi[41]=0xC7EBE5; 
	two_over_pi[42]=0xF17B3D; two_over_pi[43]=0x0739F7; two_over_pi[44]=0x8A5292; two_over_pi[45]=0xEA6BFB; two_over_pi[46]=0x5FB11F; two_over_pi[47]=0x8D5D08; 
	two_over_pi[48]=0x560330; two_over_pi[49]=0x46FC7B; two_over_pi[50]=0x6BABF0; two_over_pi[51]=0xCFBC20; two_over_pi[52]=0x9AF436; two_over_pi[53]=0x1DA9E3; 
	two_over_pi[54]=0x91615E; two_over_pi[55]=0xE61B08; two_over_pi[56]=0x659985; two_over_pi[57]=0x5F14A0; two_over_pi[58]=0x68408D; two_over_pi[59]=0xFFD880; 
	two_over_pi[60]=0x4D7327; two_over_pi[61]=0x310606; two_over_pi[62]=0x1556CA; two_over_pi[63]=0x73A8C9; two_over_pi[64]=0x60E27B; two_over_pi[65]=0xC08C6B; 
	npio2_hw[0]=0x3FF921FB;  npio2_hw[1]=0x400921FB;  npio2_hw[2]=0x4012D97C;  npio2_hw[3]=0x401921FB;  npio2_hw[4]=0x401F6A7A;  npio2_hw[5]=0x4022D97C;
	npio2_hw[6]=0x4025FDBB;  npio2_hw[7]=0x402921FB;  npio2_hw[8]=0x402C463A;  npio2_hw[9]=0x402F6A7A;  npio2_hw[10]=0x4031475C; npio2_hw[11]=0x4032D97C;
	npio2_hw[12]=0x40346B9C; npio2_hw[13]=0x4035FDBB; npio2_hw[14]=0x40378FDB; npio2_hw[15]=0x403921FB; npio2_hw[16]=0x403AB41B; npio2_hw[17]=0x403C463A;
	npio2_hw[18]=0x403DD85A; npio2_hw[19]=0x403F6A7A; npio2_hw[20]=0x40407E4C; npio2_hw[21]=0x4041475C; npio2_hw[22]=0x4042106C; npio2_hw[23]=0x4042D97C;
	npio2_hw[24]=0x4043A28C; npio2_hw[25]=0x40446B9C; npio2_hw[26]=0x404534AC; npio2_hw[27]=0x4045FDBB; npio2_hw[28]=0x4046C6CB; npio2_hw[29]=0x40478FDB;
	npio2_hw[30]=0x404858EB; npio2_hw[31]=0x404921FB;
	
	GET_HIGH_WORD(hx,x);		/* high word of x */
	ix = hx&0x7fffffff;
	if(ix<=0x3fe921fb)   /* |x| ~<= pi/4 , no need for reduction */
	    {y[0] = x; y[1] = 0; return 0;}
	if(ix<0x4002d97c) {  /* |x| < 3pi/4, special case with n=+-1 */
	    if(hx>0) { 
		z = x - pio2_1;
		if(ix!=0x3ff921fb) { 	/* 33+53 bit pi is good enough */
		    y[0] = z - pio2_1t;
		    y[1] = (z-y[0])-pio2_1t;
		} else {		/* near pi/2, use 33+33+53 bit pi */
		    z -= pio2_2;
		    y[0] = z - pio2_2t;
		    y[1] = (z-y[0])-pio2_2t;
		}
		return 1;
	    } else {	/* negative x */
		z = x + pio2_1;
		if(ix!=0x3ff921fb) { 	/* 33+53 bit pi is good enough */
		    y[0] = z + pio2_1t;
		    y[1] = (z-y[0])+pio2_1t;
		} else {		/* near pi/2, use 33+33+53 bit pi */
		    z += pio2_2;
		    y[0] = z + pio2_2t;
		    y[1] = (z-y[0])+pio2_2t;
		}
		return -1;
	    }
	}
	if(ix<=0x413921fb) { /* |x| ~<= 2^19*(pi/2), medium size */
	    t  = __fabs(x);
	    n  = (int32_t) (t*invpio2+half);
	    fn = (double)n;
	    r  = t-fn*pio2_1;
	    w  = fn*pio2_1t;	/* 1st round good to 85 bit */
	    if(n<32&&ix!=npio2_hw[n-1]) {	
		y[0] = r-w;	/* quick check no cancellation */
	    } else {
	        u_int32_t high;
	        j  = ix>>20;
	        y[0] = r-w; 
		GET_HIGH_WORD(high,y[0]);
	        i = j-((high>>20)&0x7ff);
	        if(i>16) {  /* 2nd iteration needed, good to 118 */
		    t  = r;
		    w  = fn*pio2_2;	
		    r  = t-w;
		    w  = fn*pio2_2t-((t-r)-w);	
		    y[0] = r-w;
		    GET_HIGH_WORD(high,y[0]);
		    i = j-((high>>20)&0x7ff);
		    if(i>49)  {	/* 3rd iteration need, 151 bits acc */
		    	t  = r;	/* will cover all possible cases */
		    	w  = fn*pio2_3;	
		    	r  = t-w;
		    	w  = fn*pio2_3t-((t-r)-w);	
		    	y[0] = r-w;
		    }
		}
	    }
	    y[1] = (r-y[0])-w;
	    if(hx<0) 	{y[0] = -y[0]; y[1] = -y[1]; return -n;}
	    else	 return n;
	}
    /* 
     * all other (large) arguments
     */
	if(ix>=0x7ff00000) {		/* x is inf or NaN */
	    y[0]=y[1]=x-x; return 0;
	}
    /* set z = scalbn(|x|,ilogb(x)-23) */
	GET_LOW_WORD(low,x);
	SET_LOW_WORD(z,low);
	e0 	= (ix>>20)-1046;	/* e0 = ilogb(z)-23; */
	SET_HIGH_WORD(z, ix - ((int32_t)(e0<<20)));
	for(i=0;i<2;i++) {
		tx[i] = (double)((int32_t)(z));
		z     = (z-tx[i])*two24;
	}
	tx[2] = z;
	nx = 3;
	while(tx[nx-1]==zero) nx--;	/* skip zero term */
	n  =  __kernel_rem_pio2(tx,y,e0,nx,2,two_over_pi);
	if(hx<0) {y[0] = -y[0]; y[1] = -y[1]; return -n;}
	return n;
}
