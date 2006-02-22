/**
 * Copyright (c) 2005 voidware ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS," WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */


#include <stdio.h> // XXX
#include "bcdmath.h"

#define BCD_CONST_PI    0
#define BCD_CONST_PI2   1
#define BCD_CONST_PIBY2 2
#define BCD_CONST_PIBY32 3
#define BCD_CONST_SINTAB 4
#define BCD_CONST_COSTAB (BCD_CONST_SINTAB+8)
#define BCD_CONST_LN2    (BCD_CONST_COSTAB+8)
#define BCD_CONST_ONEOTWO (BCD_CONST_LN2+1)
#define BCD_CONST_LN10    (BCD_CONST_ONEOTWO+1)
#define BCD_CONST_ATANLIM (BCD_CONST_LN10+1)
#define BCD_CONST_PIBY32A (BCD_CONST_ATANLIM+1)
#define BCD_CONST_PIBY32B (BCD_CONST_PIBY32A+1)
#define BCD_CONST_POINT5  (BCD_CONST_PIBY32B+1)

typedef unsigned short Dig[P+1];

static Dig constTable[] = 
{
    { 3, 1415, 9265, 3589, 7932, 3846, 2643, 1 }, // pi
    { 6, 2831, 8530, 7179, 5864, 7692, 5287, 1 }, // 2pi
    { 1, 5707, 9632, 6794, 8966, 1923, 1322, 1 }, // pi/2
    { 981, 7477, 424, 6810, 3870, 1957, 6057, 0 }, // pi/32

    /* table for sin & cos */
    { 980, 1714, 329, 5606, 199, 4195, 5639, 0 }, // sin(pi/32)
    { 1950, 9032, 2016, 1282, 6784, 8284, 8685, 0 }, // sin(pi/16)
    { 2902, 8467, 7254, 4623, 6763, 6192, 3758, 0 }, // sin(3pi/32)
    { 3826, 8343, 2365, 897, 7172, 8459, 9840, 0 }, // sin(pi/8)
    { 4713, 9673, 6825, 9976, 4855, 6387, 6259, 0 }, // sin(5pi/32)
    { 5555, 7023, 3019, 6022, 2474, 2830, 8139, 0 }, // sin(3pi/16)
    { 6343, 9328, 4163, 6454, 9821, 5171, 6132, 0 }, // sin(7pi/32)
    { 7071, 678, 1186, 5475, 2440, 844, 3621, 0 }, // sin(pi/4)

    { 9951, 8472, 6672, 1968, 8624, 4836, 9531, 0 }, // cos(pi/32)
    { 9807, 8528, 403, 2304, 4912, 6182, 2361, 0 }, // cos(pi/16)
    { 9569, 4033, 5732, 2088, 6493, 5797, 8870, 0 }, // cos(3pi/32)
    { 9238, 7953, 2511, 2867, 5612, 8183, 1894, 0 }, // cos(pi/8)
    { 8819, 2126, 4348, 3550, 2971, 2756, 8637, 0 }, // cos(5pi/32)
    { 8314, 6961, 2302, 5452, 3707, 8788, 3776, 0 }, // cos(3pi/16)
    { 7730, 1045, 3362, 7369, 6081, 906, 6098, 0 }, // cos(7pi/32)
    { 7071, 678, 1186, 5475, 2440, 844, 3621, 0 }, // cos(pi/4)

    { 6931, 4718, 559, 9453, 941, 7232, 1215, 0 }, // ln(2)
    { 1, 200, 0, 0, 0, 0, 0, 1 }, // 1.02
    { 2, 3025, 8509, 2994, 456, 8401, 7991, 1 }, // ln(10)
    { 1000, 0, 0, 0, 0, 0, 0, 0 },  // 0.1 atan limit

    { 981, 7477, 424, 0, 0, 0, 0, 0 }, // pi/32 part A
    { 6810, 3870, 1957, 6057, 2748, 4465, 1312, (-3)&(NEG-1) }, // pi/32 part B

    { 5000, 0, 0, 0, 0, 0, 0, 0 }, // 1/2
};

BCD pi()
{
    return *(const BCDFloat*)(constTable + BCD_CONST_PI);
}

static void sincosTaylor(const BCD& a, BCD& sa, BCD& ca, int n)
{
    /* calculate sin(a) and cos(a) by taylor series of n terms.
     */

    BCD a2 = a*a;
    ca = 1;
    sa = 1;
    BCD t = 1;

    int i = 1;
    int j = 1;
    while (i < n) {
        ++j;
        t *= a2/j;
        if (i & 1) ca -= t;
        else ca += t;

        ++j;
        t /= j;

        if (i & 1) sa -= t;
        else sa += t;
        
        ++i;
    }
    sa = sa*a;
}

void sincos(const BCD& v, BCD* sinv, BCD* cosv)
{
    /* calculate sin(v), cos(v) or both as requested.
     */
    BCD res;
    BCD a;
    int k;
    int neg = (v < 0);

    /* arrange a >= 0 */
    if (neg) a = -v;
    else a = v;

    /* reduce argument to 0 <= a < 2pi using special means,
     * taking care of large arguments (eg sin(1e22)). its possible
     * that the answer cannot be calculated accurately, in which case
     * modtwopi bails out with NAN.
     */
    a = modtwopi(a);
    if (a.isSpecial()) {
        if (sinv) *sinv = a;
        if (cosv) *cosv = a;
        return;
    }

    /*
     * reduce to k*pi/32 + a, where a < pi/32. use a lookup table
     * for sin(k*pi/32) and cos(k*pi/32). require 8 entries for each.
     */
    BCD piby32(*(const BCDFloat*)(constTable + BCD_CONST_PIBY32));
    k = ifloor(a/piby32);
    if (k > 0) {
        BCD piby32a(*(const BCDFloat*)(constTable + BCD_CONST_PIBY32A));
        BCD piby32b(*(const BCDFloat*)(constTable + BCD_CONST_PIBY32B));
        a = (a - k*piby32a) - k*piby32b;
    }

    /* now a <= pi/32, we can apply taylor series with only 6 terms.
     * the error is then bounded by a^2^(2n+1)/(2n+1)!. ie the
     * first ignored term.
     */
    BCD sa;
    BCD ca;
    sincosTaylor(a, sa, ca, 6);

    BCD sina, cosa;

    k &= 64-1;  // wrap around at 2pi
    int q = k/16; // q is the quadrant.

    k &= 15; // index into table.

    if (k == 0) {
        sina = sa;
        cosa = ca;
    }
    else if (k < 8) {
        BCD sk(*(const BCDFloat*)(constTable + BCD_CONST_SINTAB-1+k));
        BCD ck(*(const BCDFloat*)(constTable + BCD_CONST_COSTAB-1+k));
        sina = sk*ca + ck*sa;
        cosa = ck*ca - sk*sa;
    }
    else {
       BCD sk(*(const BCDFloat*)(constTable + BCD_CONST_SINTAB+15-k));
       BCD ck(*(const BCDFloat*)(constTable + BCD_CONST_COSTAB+15-k));
       sina = ck*ca + sk*sa;
       cosa = sk*ca - ck*sa;
    }

    switch (q) {
    case 0:
        if (sinv) *sinv = sina;
        if (cosv) *cosv = cosa;
        break;
    case 1:
        if (sinv) *sinv = cosa;
        if (cosv) *cosv = -sina;
        break;
    case 2:
        if (sinv) *sinv = -sina;
        if (cosv) *cosv = -cosa;
        break;
    case 3:
        if (sinv) *sinv = -cosa;
        if (cosv) *cosv = sina;
        break;
    }

    if (sinv && neg) *sinv = -*sinv;
}

BCD sin(const BCD& v)
{
    BCD sinv;
    if (v.isSpecial()) return v;

    sincos(v, &sinv, 0);
    return sinv;
}

BCD cos(const BCD& v)
{
    BCD cosv;
    if (v.isSpecial()) return v;

    sincos(v, 0, &cosv);
    return cosv;
}

BCD tan(const BCD& v)
{
    BCD s, c;
    if (v.isSpecial()) return v;

    sincos(v, &s, &c);
    return s/c;
}

static BCD expTaylor(const BCD& a, int n)
{
    BCD t = a;
    BCD s = t + 1;
    int i = 1;
    while (i < n) {
        t = t*a/(++i);
        s += t;
    }
    return s;
}


BCD pow(const BCD& a, int4 n)
{
    int4 m;
    if (n == 0) return 1;
    m = (n < 0) ? -n : n;

    BCD s;
    if (m > 1) {
        BCD r = a;
        s = 1;
        /* Use binary exponentiation */
        for (;;) {
            if (m & 1) s *= r;
            m >>= 1;
            if (!m) break;
            r *= r;
        }
    } else { s = a; }

    /* Compute the reciprocal if n is negative. */
    if (n < 0) {
        return 1/s;
    }
    return s;
}

BCD exp(const BCD& v)
{
    /* write v = k*r + n*ln(2)
     * where |k*r| < ln(2)
     *
     * let k be some power of 2, eg 64 so that
     *
     * exp(v) = exp(kr + nln(2)) = exp(kr)*2^n
     *        = exp(r)^64 * 2^n
     *
     * then r is small enough for taylor series.
     */
    if (v.isSpecial()) {
        if (v.isInf() && v.isNeg()) return 0;  // exp(-inf) = 0
        return v;
    }

    BCD ln2(*(const BCDFloat*)(constTable + BCD_CONST_LN2));
    int4 n = ifloor(v/ln2);

    if (n > 33218) {
        /* overflow */
        return BCDFloat::posInf();
    }
    if (n < -33218) {
        /* underflow */
        return 0;
    }

    int k = 64;
    BCD r = (v - n*ln2)/k;
    
    /* error bounded by x^9/9! where x = ln(2)/k */
    BCD er = expTaylor(r, 8);
    er = pow(er, k);

    if (n) {
        er = er*pow(BCD(2), n);
    }
    return er;
}

BCD log(const BCD& v)
{
    /* natural logarithm */
    if (v.isNeg()) return BCDFloat::nan();
    if (v.isSpecial()) return v;
    if (v.isZero()) return BCDFloat::negInf();

    BCD a = v;
    int p10 = a.exponent()*4;
    a.setExponent(0);

    int n;
    int d = a.digit(0);
    if (d >= 1000) { n = 10; p10 -= 1; }
    else if (d >= 100) { n = 100; p10 -= 2; }
    else if (d >= 10) { n = 1000; p10 -= 3; }
    else { n = 10000; p10 -= 4; }

    /* v = a*10^n and 1 <= a < 10 */
    a *= n;

    BCD oneotwo(*(const BCDFloat*)(constTable + BCD_CONST_ONEOTWO)); // 1.02
    int p2 = 2;

    while (a >= oneotwo) { 
        p2 <<= 1;
        a = sqrt(a);  // 7 roots max.
    }
    
    /* use series
     * ln(x) = 2(u+u^3/3+u^5/5 + ...) where u = (x-1)/(x+1) 
     */

    BCD t = (a-1)/(a+1);
    BCD s = t;
    a = t*t;

    int i;
    for (i = 3; i < 12; i += 2) { // only 5 terms needed
        t *= a;
        s += t/i;
    }

    BCD ln10(*(const BCDFloat*)(constTable + BCD_CONST_LN10));
    return s*p2 + p10*ln10;
}

BCD atan(const BCD& v)
{
    bool neg = v.isNeg();

    if (v.isSpecial()) {
        if (v.isInf()) {
            BCD piby2(*(const BCDFloat*)(constTable + BCD_CONST_PIBY2));
            if (neg) return (piby2*3)/2;
            return piby2;
        }
        return v;
    }

    /* arrange for a >= 0 */
    BCD a;
    if (neg) {
        a = -v;
    }
    else a = v;

    /* reduce range to 0 <= a < 1, using atan(x) = pi/2 - atan(1/x)
     */
    bool invert = (a > 1);
    if (invert) {
        a = 1/a;
    }
    
    /* reduce to small enough limit to use taylor series.
     * using
     *  tan(x/2) = tan(x)/(1+sqrt(1+tan(x)^2))
     */
    BCD atanlim(*(const BCDFloat*)(constTable + BCD_CONST_ATANLIM));
    int doubles = 0;
    while (a > atanlim) {
        ++doubles;
        a = a/(1+sqrt(1+a*a));  // at most 3 iterations.
    }

    /* now use taylor series
     * tan(x) = x(1-x^2/3+x^4/5-x^6/7...)
     */
    
    BCD a2 = a*a;
    BCD t = a2;
    BCD s = 1 - t/3;
    int i;
    int j = 5;
    /* perform 9 more terms, error will be the first term not used.
     * ie x^21/21.
     */
    for (i = 2; i < 11; ++i) {
        t *= a2;
        if ((i & 1) == 0) s += t/j;
        else s -= t/j;
        j += 2;
    }
    s = s*a;

    while (doubles) {
        s = s + s;
        --doubles;
    }

    if (invert) {
        BCD piby2(*(const BCDFloat*)(constTable + BCD_CONST_PIBY2));
        s = piby2 - s;
    }

    if (neg) {
        s = -s;
    }
    return s;
}

BCD atan2(const BCD& y, const BCD& x)
{
    BCD r;
    if (x.isZero()) {
        r = BCD(*(const BCDFloat*)(constTable + BCD_CONST_PIBY2));
        if (y.isNeg()) { r = -r; }
    }
    else {
        r = atan(y/x);
        if (x.isNeg()) {
            BCD pi(*(const BCDFloat*)(constTable + BCD_CONST_PI));
            if (y.isNeg()) {
                r = r - pi;
            }
            else {
                r = r + pi;
            }
        }
    }
    return r;
}

BCD asin(const BCD& v)
{
    if (v.isSpecial()) return v;

    if (v < -1 || v > 1) {
        return BCDFloat::nan();
    }
    
    BCD r = atan(v/(1+sqrt(1-v*v)));
    r = r + r;
    return r;
}

BCD acos(const BCD& v)
{
    if (v.isSpecial()) return v;

    if (v < -1 || v > 1) {
        return BCDFloat::nan();
    }
    
    BCD r = atan((1-v)/sqrt(1-v*v));
    r = r + r;
    return r;
}

BCD pow(const BCD& x, const BCD& y)
{
    if (y.isZero()) {
        if (x.isZero()) return BCDFloat::nan();
        return 1;
    }
    
    /* check for x^n */
    if (y.isInteger()) {
        int4 n = ifloor(y);
        if (n) return pow(x, n);
        
        /* otherwise power is too large */
        if (x == 1) return x;
        
        bool a = (fabs(x) > 1);
        if (a == y.isNeg()) return 0;
        return BCDFloat::posInf();
    }

    /* otherwise use logs */
    return exp(log(x)*y);
} 

/* here are the first 1060 decimal digits of 1/2pi */
static unsigned short inv2pi[] = {
    1591, 5494, 3091, 8953, 3576, 8883,
    7633, 7251, 4362,  344, 5964, 5740, 4564, 4874, 7667, 3440, 5889,
    6797, 6342, 2653, 5090, 1138,  276, 6253,  859, 5607, 2842, 7267,
    5795, 8036, 8929, 1184, 6114, 5786, 5287, 7967, 4107, 3169, 9839,
    2292, 3996, 6937, 4090, 7757, 3077, 7463, 9692, 5307, 6887, 1739,
    2896, 2173, 9766, 1693, 3623, 9024, 1723, 6290, 1183, 2380, 1142,
    2269, 9755, 7159, 4046, 1890,  869,  267, 3956, 1204, 8941,  936,
    9378, 4408, 5528, 7230, 9994, 6443, 4002, 4867, 2347, 7394, 5961,
     898, 3230, 9678, 3074, 9061, 6698, 6462, 8046, 9944, 8652, 1878,
    8157, 4786, 5669, 6424, 1038, 9958, 7413, 9348, 6099, 8386, 8099,
    1999, 6244, 2875, 5851, 7117, 8858, 4311, 1751, 8767, 1605, 4654,
    7536, 9880,  973, 9460, 3647, 5933, 3768,  593,  249, 4496, 6353,
     532, 7156, 7755,  322,  324, 7778, 1639, 7166,  229, 4674, 8119,
    5981, 6584,  606,  168,  303, 5998, 1339, 1198, 7498, 8327, 8665,
    4435, 2797, 5507,   16, 2406, 7756, 4388, 8495, 7131,  880, 1221,
    9937, 6147, 6813, 7776, 4737, 8906, 3306, 8046, 4579, 7848, 1761,
    3124, 2731, 4069, 9607, 7502, 4500, 2977, 5985, 7089,  569,  279,
    6785, 1315, 2521,   16, 3177, 4602,  924, 8116,  624,  561, 4562,
     314, 6484,  892, 4845, 9191, 4352, 1157, 5407, 5562,   87, 1526,
    6068,  221, 7159, 1407, 5747, 4582, 7225, 9774, 6285, 3998, 7515,
    5329, 3908, 1398, 1772, 4093, 5825, 4797,  733, 2871, 9040, 6999,
    7590, 7657, 7078, 4934, 7039, 3589, 8280, 8717, 3425, 6403, 6689,
    5116, 6254, 5705, 9433, 2763, 1268, 6500, 2612, 2717, 9711, 5321,
    1259, 9504, 3866, 7945,  376, 2556,  836, 3171, 1695, 2597, 5812,
    8224, 9416, 2333, 4314, 5106, 1235
};

/* double precision version of 2pi */
static unsigned short pi2dp[] = {
 6, 2831, 8530, 7179, 5864, 7692, 5286,
 7665, 5900, 5768, 3943, 3879, 8750, 2116, 4194
};

BCD modtwopi(const BCD& a)
{
    BCD pi2(*(const BCDFloat*)(constTable + BCD_CONST_PI2));
    if (a < pi2) return a;

    unsigned short xd[2*P+1];
    int i;

    /* copy digits of manstissa as double precision */
    for (i = 0; i < P; ++i) {
        xd[i] = a.ref_->v_.d_[i];
    }
    while (i <= 2*P) { // clear extended digits.
        xd[i] = 0;
        ++i;
    }
    int ex = a.exponent();

    unsigned short bd[2*P+1];
    int eb;
    int excess = 0;

    /* see if the exponent is large enough to consider it separately */
    if (ex > P) {

        /* yes. consider our number as X * BASE^E where X is an integer.
         * in this case we can calculate f = (BASE^E) mod 2pi accurately
         * and form X*f.
         */

        excess = ex - P;  // remaining exponent
        ex = P;

        unsigned short fd[2*P+1];
        int ef;

        /* do we have enough table? */
        if (excess + 2*P+1 > (int)(sizeof(inv2pi)/sizeof(inv2pi[0]))) {
            /* oh dear, digits required off end of table. give up.
             */
            return BCDFloat::nan();
        }

        /* find base^ex mod 2pi using the table */
        BCDFloat::mul2(inv2pi + excess, 0, pi2dp, 1, fd, ef);

        /* multiply exponent excess mod 2pi into X. note, all 
         * calculations here must be double precision. this is because
         * we are only interested in the fractional part and the
         * integer part is the size of 1 precision already.
         */
        BCDFloat::mul2(fd, ef, xd, ex, bd, eb);

        for (i = 0; i < 2*P+1; ++i) {
            xd[i] = bd[i];
        }
        ex = eb;
    }


    /* need to divide by 2pi and extract the fractional part.
     * this is done by multiplying by 1/2pi using our table digits
     * we had to have for the exponent extraction.
     */
    BCDFloat::mul2(xd, ex, inv2pi, 0, bd, eb);

    BCDFloat b;
    BCDFloat c;

    /* pick out final single precision fraction */
    for (i = 0; i < P; ++i) b.d_[i] = bd[i+eb];

    /* and multiply by 2pi to get answer */
    BCDFloat::mul(&b, &pi2.ref_->v_, &c);        
    return c;
}

BCD log10(const BCD& v)
{
    /* XXX FIXME */
    BCD ln10(*(const BCDFloat*)(constTable + BCD_CONST_LN10));
    return log(v) / ln10;
}

BCD hypot(const BCD& a, const BCD& b)
{
    /* XXX FIXME */
    return sqrt(a * a + b * b);
}

BCD fmod(const BCD& a, const BCD& b)
{
    /* XXX FIXME */
    BCD c = a - b * trunc(a / b);
    if (a == trunc(a) && b == trunc(b) && !(c == trunc(c))) {
	// Numerator and denominator are both integral;
	// in this case we force the result to be integral as well.
	BCD half(*(const BCDFloat*)(constTable + BCD_CONST_POINT5));
	if (c < 0)
	    c = trunc(c - half);
	else
	    c = trunc(c + half);
    }
    return c;
}
