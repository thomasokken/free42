/**
 * Copyright (c) 2005-2009 voidware ltd.
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

#include <stdio.h>
#include <string.h>
#include "bcdfloat.h"

#ifdef PALMOS
// We take the following three functions from core_phloat.cc, rather than from
// the standard library, because in the case of the PalmOS Decimal build, the
// standard library isn't there.
double pow(double x, double y);
double floor(double x);
double log10(double x);
#else
#include <math.h>
#endif


int BCDDecade[4] = { 1000, 100, 10, 1 };

unsigned short BCDFloat::posInfD_[P+1] = { 0, 0, 0, 0, 0, 0, 0, POS_INF_EXP };
unsigned short BCDFloat::negInfD_[P+1] = { 0, 0, 0, 0, 0, 0, 0, 0xA000 };
unsigned short BCDFloat::nanD_[P+1] =    { 0, 0, 0, 0, 0, 0, 0, 0x4000 };

void BCDFloat::_init()
{
    for (int i = 0; i <= P; ++i) d_[i] = 0;
}

int bcd_round(unsigned short* d, int pn) 
{
    // round d_[P] into the mantissa
    if (d[pn] >= 5000)
    {
        int i = pn-1;
        int v = d[i] + 1;
        while (v >= BASE)
        {
            d[i] = (unsigned short)(v - BASE);
            if (!i)
            {
                // shift
                for (i = pn; i > 0; --i) d[i] = d[i-1];
                d[0] = 1;
                return 1;
            }
            v = d[--i]+1;
        }
        d[i] = v;
    }
    return 0;
}

int bcd_round25(unsigned short* d, int pn) 
{
    // round d_[P] into the mantissa and mask off digits after 25.
    int i = pn-1;
    int v;
    if (d[0] < 10)
        v = d[i] + (d[pn] >= 5000);
    else if (d[0] < 100)
        v = ((((int4) d[i]+5)*3277)>>15)*10;
    else if (d[0] < 1000)
        v = ((((int4) d[i]+50)*5243)>>19)*100;
    else
        v = ((((int4) d[i]+500)*8389)>>23)*1000;

    while (v >= BASE)
    {
        d[i] = (unsigned short)(v - BASE);
        if (!i)
        {
            // shift
            for (i = pn; i > 0; --i) d[i] = d[i-1];
            d[0] = 1;
            return 1;
        }
        v = d[--i]+1;
    }
    d[i] = v;
    return 0;
}



void bcd_uadd(const unsigned short* a,
              const unsigned short* b,
              unsigned short* c,
              int pn)
{
    int ea = GET_EXP(a, pn);
    int eb = GET_EXP(b, pn);
    int d = ea - eb;
    int i;
        
    if (d <= pn) // otherwise `b' is insignificant
    {
        int ca;
        int v;
        int j = pn-d;
        i = pn-1;

        // copy in first insignificant digit used by final rounding
        v = 0;
        if (d > 0) v = b[j]; 
        c[pn] = v;

        ca = 0;
        while (j > 0) // perform addition of overlapping terms
        {
            v = a[i] + b[--j] + ca;
            ca = 0;
            if (v >= BASE)
            {
                v -= BASE;
                ca = 1;
            }
            c[i] = v;
            --i;
        }

        while (i >= 0) // remainder non-overlap terms
        {
            v = a[i] + ca;
            ca = 0;
            if (v >= BASE)
            {
                v -= BASE;
                ca = 1;
            }
            c[i] = v;
            --i;
        }

        if (ca)
        {
            /* overall carry, shift down and round */
            for (i = pn; i > 0; --i) c[i] = c[i-1];
            c[0] = ca;
            ++ea;
        }

        if (bcd_round25(c, pn)) ++ea;
        if (ea > EXPLIMIT)
        {
            // overflow
            for (i = 0; i < pn; ++i) c[i] = 0;
            ea = POS_INF_EXP;
        }
        SET_EXP(c, pn, ea);
    }
    else
    {
        for (i = 0; i <= pn; ++i) c[i] = a[i];
        CLEAR_SIGN(c, pn);
    }
}

void bcd_usub(const unsigned short* a,
              const unsigned short* b,
              unsigned short* c,
              int pn)
{
    int ea = GET_EXP(a, pn);
    int eb = GET_EXP(b, pn);
    bool neg = false;
    int i;

    int d = ea - eb;
    if (d <= pn)
    {
        int ca;
        int v;

        int j = pn-d;
        i = pn-1;

        ca = 0;
        v = 0;

        /* first insignificant digit my be used for rounding */
        if (d > 0)
        {
            v = BASE - b[j];
            ca = 1;
        }
        c[pn] = v;

        while (j > 0)
        {
            v = a[i] - b[--j] - ca;
            ca = 0;
            if (v < 0) 
            {
                ca = 1;
                v += BASE;
            }
            c[i] = v;
            --i;
        }

        if (ca)
        {
            while (i >= 0)
            {
                v = a[i];
                if (v)
                {
                    // carry absorbed
                    c[i] = v - 1;
                    
                    // copy remainder
                    while (i > 0) { --i; c[i] = a[i]; }

                    // NB: i == 0
                    break;
                }
                else
                    c[i--] = BASE-1;
            }

            if (i < 0) // carry all the way up
            {
                /* overall borrow, need to complement number */
                for (i = pn; i >= 0; --i) 
                {
                    v = BASE-1 - c[i] + ca;
                    ca = 0;
                    if (v >= BASE) 
                    {
                        ca = 1;
                        v -= BASE;
                    }
                    c[i] = v;
                }
                neg = true;
            }
        }
        else
        {
            // copy remainder of number over
            while (i >= 0) { c[i] = a[i]; --i; }
        }

        int e = ea;
        i = 0;
        while (c[i] == 0 && i <= pn) i++;

        if (i > 0) 
        {
            if (i == pn+1)
            {
                /* is zero */
                e = 0;
            }
            else 
            {
                e -= i;
                if (e <= -EXPLIMIT) 
                {
                    /* underflow */
                    c[0] = 0;
                    e = 0;
                }
                else 
                {
                    int j;
                    for (j = 0; j <= pn - i; j++) c[j] = c[j + i];
                    for (; j <= pn; j++) c[j] = 0;
                }
            }
        }

        if (bcd_round25(c, pn)) ++e;

        if (e > EXPLIMIT) // not sure this can happen
        {
            for (i = 0; i < pn; ++i) c[i] = 0;
            ea = POS_INF_EXP;
        }
        SET_EXP(c, pn, e);
        if (neg) NEGATE_SIGN(c, pn);
    }
    else
    {
        /* `b' is insignificant */
        for (i = 0; i <= pn; ++i) c[i] = a[i];
        CLEAR_SIGN(c, pn);
    }
}

void bcd_add(const unsigned short* a,
             const unsigned short* b,
             unsigned short* c,
             int pn)
{
    int i;
    if (GET_SPECIAL(a, pn) | GET_SPECIAL(b, pn))
    {
        for (i = 0; i < pn; ++i) c[i] = 0;
        c[pn] = NAN_EXP;

        /* inf + inf = inf
         * -inf + (-inf) = -inf
         */
        if (!(GET_NAN(a,pn) | GET_NAN(b,pn)))
        {
            if (GET_INF(a,pn))
            {
                if (!GET_INF(b,pn) || (a[pn] == b[pn]))
                {
                    // inf + x = inf
                    c[pn] = a[pn];
                }
            }
            else
            {
                // b is inf
                c[pn] = b[pn];
            }
        }
    }
    else
    {
        int ea = GET_EXP(a,pn);
        int eb = GET_EXP(b,pn);

        bool na = GET_NEG_NORM(a,pn);
        bool nb = GET_NEG_NORM(b,pn);
        bool sub = na != nb;

        if (sub) 
        {
            if (ea >= eb) 
                bcd_usub(a, b, c, pn);
            else 
            {
                bcd_usub(b, a, c, pn);
                na = nb;
            }
        }
        else 
        {
            if (ea >= eb) bcd_uadd(a, b, c, pn);
            else bcd_uadd(b, a, c, pn);
        }
        if (na) NEGATE_SIGN(c,pn);
    }
}

void bcd_sub(const unsigned short* a,
             const unsigned short* b,
             unsigned short* c,
             int pn)
{
    int i;
    if (GET_SPECIAL(a, pn) | GET_SPECIAL(b, pn))
    {
        for (i = 0; i < pn; ++i) c[i] = 0;

        /* all others -> nan */
        c[pn] = NAN_EXP;
            
        if (!(GET_NAN(a,pn) | GET_NAN(b,pn)))
        {
            if (GET_INF(a,pn))
            {
                if (!GET_INF(b,pn) || (a[pn] != b[pn]))
                {
                    /* -inf - (inf) = -inf
                     * inf - (-inf) = inf
                     */
                    c[pn] = a[pn];
                }
            }
            else 
            {
                // b is inf
                c[pn] = NEG_INF_EXP;
            }
        }
    }
    else
    {
        bool na = GET_NEG_NORM(a,pn);
        bool nb = GET_NEG_NORM(b,pn);
        bool sub = (na == nb);

        int ea = GET_EXP(a,pn);
        int eb = GET_EXP(b,pn);

        if (sub) 
        {
            if (ea >= eb) 
            {
                bcd_usub(a, b, c, pn);
                if (na) NEGATE_SIGN(c,pn);
            }
            else 
            {
                bcd_usub(b, a, c, pn);
                if (!na) NEGATE_SIGN(c,pn);
            }
        }
        else 
        {
            if (ea >= eb) bcd_uadd(a, b, c, pn);
            else bcd_uadd(b, a, c, pn);
            if (na) NEGATE_SIGN(c,pn);
        }
    }
}

void bcd_mul(const unsigned short* a,
             const unsigned short* b,
             unsigned short* c,
             int pn)
{

    // zero flags assuming not special
    bool az = GET_ZERO_NORM(a,pn);
    bool bz = GET_ZERO_NORM(b,pn);

    // neg flag assuming non-zero and non-special
    int na = GET_NEG_BIT(a,pn);
    int nb = GET_NEG_BIT(b,pn);

    int i;
    for (i = 0; i <= pn; ++i) c[i] = 0;

    if (GET_SPECIAL(a,pn) | GET_SPECIAL(b,pn))
    {
        /* all others -> nan */
        c[pn] = NAN_EXP;

        if (!(GET_NAN(a,pn) | GET_NAN(b,pn)))
        {
            if ((GET_INF(a,pn) && (GET_INF(b,pn) || (!bz))) || !az)
            {
                // inf * inf -> inf
                // inf * non-zero -> inf
                // non-zero * inf -> inf
                c[pn] = POS_INF_EXP;
                if (na != nb) NEGATE_SIGN(c, pn);
            }
        }
    }
    else if (!az && !bz)
    {        
        int ca;
        int i, j;
        int4 u, v;

        int ea = GET_EXP(a,pn);
        int eb = GET_EXP(b,pn);

        unsigned short acc[MAX_P+1];
        for (i = 0; i <= pn; ++i) acc[i] = 0;

        for (i = pn-1; i >= 0; --i) 
        {
            bcd_round(c, pn);
            for (j = pn; j > 0; --j) c[j] = c[j-1];
            c[0] = 0;
        
            u = a[i];
            if (!u) continue;

            ca = 0;
            for (j = pn; j > 0; --j) 
            {
                v = b[j-1] * u + ca;
                ca = 0;
                if (v >= BASE) 
                {
                    ca = v / BASE;
                    v = v - ca*BASE;
                }
                acc[j] = v;
            }
            acc[0] = ca;

            /* now add acc into c */
            ca = 0;
            for (j = pn; j >= 0; --j) 
            {
                v = c[j] + acc[j] + ca;
                ca = 0;
                if (v >= BASE) 
                {
                    ca = 1;
                    v -= BASE;
                }
                c[j] = v;
            }
            /* won't be any overall carry */
        }

        if (!c[0]) 
        {
            for (i = 0; i < pn; ++i) c[i] = c[i+1]; 
            c[pn] = 0;
        }
        else 
        {
            ++ea;
            if (bcd_round25(c,pn)) ++ea;
        }

        ea += eb - 1;
        if (ea <= -EXPLIMIT || ea > EXPLIMIT)
        {
            for (i = 0; i <= pn; ++i) c[i] = 0; // underflow
            if (ea > EXPLIMIT) c[pn] = POS_INF_EXP;  // overflow           
        }
        else 
        {
            SET_EXP(c,pn,ea);
            
            /* fix sign */
            if (na != nb) NEGATE_SIGN(c,pn);
        }
    }
}

void bcd_div(const unsigned short* a,
             const unsigned short* b,
             unsigned short* c,
             int pn)
{

    int as = GET_SPECIAL(a,pn);
    int bs = GET_SPECIAL(b,pn);
        
    // zero flags assuming not special
    int az = GET_ZERO_NORM(a,pn) && !as;
    int bz = GET_ZERO_NORM(b,pn) && !bs;

    // neg assuming non-special
    int na = GET_NEG_BIT(a,pn) && !az;
    int nb = GET_NEG_BIT(b,pn) && !bz;
    int i;

    if (as | bs | az | bz)
    {
        for (i = 0; i < pn; ++i) c[i] = 0;

        /* all others -> nan */
        c[pn] = NAN_EXP;

        if (!(GET_NAN(a,pn) | GET_NAN(b,pn)))
        {
            if (GET_INF(a,pn) | bz)
            {
                // inf/inf -> nan
                if (!GET_INF(b,pn) && !az)
                {
                    // inf/x -> inf
                    if (na == nb) c[pn] = POS_INF_EXP;
                    else c[pn] = NEG_INF_EXP;
                }
            }
            else if (GET_INF(b,pn) | az)
            { 
                // x/inf -> 0
                c[pn] = 0;
            }
        }
    }
    else
    {
        int4 u, v;
        int ca;
        int j = 0;
        int4 q;

        int ea = GET_EXP(a,pn);
        int eb = GET_EXP(b,pn);

        unsigned short acc[MAX_P+1];
        unsigned short b1[MAX_P+1];
        
        u = BASE/(b[0]+1);

        if (u != 1) 
        {
            /* prenormialise `a' and move into acc using spare digit */
            ca = 0;
            for (i = pn; i > 0; --i) 
            {
                v = a[i-1]*u + ca;
                ca = 0;
                if (v >= BASE) 
                {
                    ca = v/BASE;
                    v -= ca*BASE;
                }
                acc[i] = v;
            }
            acc[0] = ca;

            /* prenormalise `b' into b1 */
            ca = 0;
            for (i = pn-1; i >= 0; --i) 
            {
                v = b[i]*u + ca;
                ca = 0;
                if (v >= BASE) 
                {
                    ca = v/BASE;
                    v -= ca*BASE;
                }
                b1[i] = v;
            }
        }
        else 
        {
            /* u is often 1 */
            for (i = pn-1; i >= 0; --i) 
            {
                acc[i+1] = a[i];
                b1[i] = b[i];
            }
            acc[0] = 0;
        }

        for (;;) 
        {
            if (acc[0] == b1[0]) q = BASE-1;
            else 
            {
                v = acc[0]*BASE + acc[1];
                q = v/b1[0];
                while (b1[1]*q > ((v - q*b1[0])*BASE + acc[2])) --q;
            }

            if (!q && !j) 
            {
                /* first quotient digit is zero. can gain extra
                 * accuracy by ignoring this and adjusting exponent.
                 */
                --ea;
            }
            else 
            {
                if (j == pn) 
                {
                    c[j] = q;
                    break;
                }

                ca = 0;
                for (i = pn; i > 0; --i) 
                {
                    v = acc[i] - b1[i-1]*q - ca;
                    ca = 0;
                    if (v < 0) 
                    {
                        ca = (-v + BASE-1)/BASE;
                        v += ca*BASE;
                    }
                    acc[i] = v;
                }
                v = acc[0] - ca;

                if (v) 
                {
                    /* the infamous add back correction */
                    ca = 0;
                    for (i = pn; i > 0; --i) 
                    {
                        v = acc[i] + b1[i-1] + ca;
                        ca = 0;
                        if (v >= BASE) 
                        {
                            ca = 1;
                            v -= BASE;
                        }
                        acc[i] = v;
                    }
                    q--;
                }
                if (q == 0 && j == 0)
                    --ea;
                else
                    c[j++] = q;
            }

            // left shift
            for (i = 0; i < pn; ++i) acc[i] = acc[i+1]; 
            acc[pn] = 0;
        }

        if (bcd_round25(c, pn)) ++ea;
        
        ea -= eb - 1;
        if (ea <= -EXPLIMIT || ea > EXPLIMIT)
        {
            for (i = 0; i <= pn; ++i) c[i] = 0;
            if (ea > EXPLIMIT) c[pn] = POS_INF_EXP;
        }
        else SET_EXP(c, pn, ea);
        if (na != nb) NEGATE_SIGN(c, pn);
    }
}

int bcd_cmp(const unsigned short* a, 
            const unsigned short* b,
            int pn)
{
    // zero flags assuming not special
    int az = GET_ZERO_NORM(a,pn);
    int bz = GET_ZERO_NORM(b,pn);

    // neg 
    int na = GET_NEG_BIT(a,pn) && !az;
    int nb = GET_NEG_BIT(b,pn) && !bz;

    if (na != nb)
        return na ? -1 : 1;

    if (GET_NAN(a,pn) | GET_NAN(b,pn))
    {
        // NaNs are not equal to anything, even themselves;
        // ALL comparisions involving them should return 'false'
        // (except '!=', which should return 'true').
        // So, I return a special value here.
        return 2;
    }
    
    // If a and b are both negative, switch them. This way, I can
    // ignore signs in the remainder of this method.
    if (na) 
    {
        const unsigned short* t = a;
        a = b;
        b = t;
    }

    if (GET_INF(a, pn)) return GET_INF(b,pn) ? 0 : 1;
    if (GET_INF(b,pn)) return -1;
        
    if (az)
        return bz ? 0 : -1;

    if (bz)
        return 1;
    
    int ea = GET_EXP(a,pn);
    int eb = GET_EXP(b,pn);
    if (ea != eb)
        return ea < eb ? -1 : 1;

    for (int i = 0; i < pn; i++) {
        int da = a[i];
        int db = b[i];
        if (da != db)
            return da < db ? -1 : 1;
    }
    return 0;
}

void bcd_fromUInt(unsigned short* d, int pn, uint4 v)
{
    /* quicker to deal with cases separately */
    if (v < BASE) 
    {
        d[0] = v;
        d[pn] = 1;
    }
    else if (v < ((int4) BASE)*BASE) 
    {
        d[0] = (unsigned short)(v/BASE);
        d[1] = (unsigned short)(v - d[0]*BASE);
        d[pn] = 2;
    }
    else 
    {
        d[0] = (unsigned short)(v/(((int4) BASE)*BASE));
        v -= d[0]*(((int4) BASE)*BASE);
        d[1] = (unsigned short)(v/BASE);
        d[2] = (unsigned short)(v - d[1]*BASE);
        d[pn] = 3;
    }
}

BCDFloat::BCDFloat(const char* s)
{
    /* make a BCD float from a string.
     */
    _init();

    bool neg = false;
    if (*s == '-') 
    {
        neg = true;
        ++s;
    }

    int i;
    int j = 0;
    int e = 0;
    
    /* find the end of the input string */
    const char* endp = s;
    const char* startp = 0;
    const char* point = 0;
    bool begun = false;
    while (*endp && !ISEXP(*endp)) 
    {
        if (*endp == '.') 
        {
            if (point) break;
            point = endp;
            if (!startp) {
                startp = endp;
                begun = true;
            }
        }
        else 
        {
            if (*endp < '0' || *endp > '9') break;
            if (*endp != '0' && !begun) 
            {
                startp = endp;
                begun = true;
            }
        }
        ++endp;
    }

    if (!startp)
    {
        // check for Inf before fail
        //if (!strnicmp(s, "inf", 3))
	if ((s[0] == 'i' || s[0] == 'I') && (s[1] == 'n' || s[1] == 'N') && (s[2] == 'f' || s[2] == 'F'))
        {
            *this = posInf();
            if (neg) negate();
        }
        return;
    }

    bool eneg = false;
    if (startp) 
    {
        if (ISEXP(*endp)) 
        {
            const char* p = endp + 1;
            if (*p == '-') {
                eneg = true;
                ++p;
            }
            else if (*p == '+') ++p;  // allow E+1234 

	e = 0;
	char c;
	// Skip leading zeroes
	while ((c = *p++) == '0');
	// Accept up to 6 digits; the reason why I don't stop after
	// 5 digits is because I want to make sure that 0.1e100000
	// is converted to Inf, and not 1e9999
	for (i = 0; i < 6; ++i) {
	    if (c >= '0' && c <= '9')
		e = e * 10 + c - '0';
	    else
		break;
	    c = *p++;
	}
	if (eneg) e = -e;
    }

        /* represent the decimal point by adjusting the exponent */
        if (point) 
            e += (int)(point - startp);
        else 
            e += (int)(endp - startp);

        /* calculate the decade offset of the exponent remainder */
        if (e >= 0) 
        {
            i = (e&3);
            if (i) i = 4-i;
        }
        else 
            i = ((-e) & 3);
        
        e += i;

        /* convert to 4dec */
        e >>= 2;
    
        const char* p = startp;
        bool leading_zero = true;
        while (p != endp) 
        {
            int d = 0;
            while (i < 4) 
            {
                if (*p != '.') 
                {
                    d += (*p - '0')*BCDDecade[i];
                    ++i;
                }
                ++p;
                if (p == endp) break;
            }
            i = 0;
            if (leading_zero && d == 0) 
            {
                e--;
                continue;
            }
            leading_zero = false;
            d_[j] = d;
            if (++j > P)
                // full up.
                break;
        }

        if (leading_zero)
            e = 0;
        while (j <= P)
            d_[j++] = 0;

        if (_round25())
            e++;

        if (e <= -EXPLIMIT) _init();
        else 
        {
            if (e > EXPLIMIT) *this = posInf();
            else exp(e);
            if (neg) negate();
        }
    }
}

BCDFloat::BCDFloat(int8 n) {
    _init();
    int i, z = 5;
    bool neg = n < 0;
    if (neg)
	n = -n;
    for (i = 4; i >= 0; i--) {
	int d = (int) (n % BASE);
	d_[i] = d;
	n /= BASE;
	if (d != 0)
	    z = i;
    }
    if (z < 5) {
	if (z > 0) {
	    for (i = 0; i < 5 - z; i++)
		d_[i] = d_[i + z];
	    for (i = 5 - z; i < 5; i++)
		d_[i] = 0;
	}
	d_[P] = 5 - z;
	if (neg)
	    negate();
    }
}

BCDFloat::BCDFloat(double d) {
    if (isnan(d)) {
	*this = BCDFloat::nan();
	return;
    }
    int inf = isinf(d);
    if (inf > 0) {
	*this = BCDFloat::posInf();
	return;
    } else if (inf < 0) {
	*this = BCDFloat::negInf();
	return;
    }

    // Return exact results if d is representable exactly as a long long.
    // TODO: there are additional cases we should try to handle exactly, e.g.
    // 0.375. Basically, the idea would be to do the conversion by summing
    // exact equivalents for the values of d's nonzero bits, just like the
    // way the table-based binary-to-bcd conversion in the Free42 Binary
    // build works. This approach returns exact results for a very large set
    // of cases, including all representable integers.
    int8 n = (int8) d;
    if (d == n) {
	*this = BCDFloat(n);
	return;
    }

    bool neg = d < 0;
    if (neg)
	d = -d;

    if (d == 0) {
	_init();
    } else {
	int exp = (((int) ::floor(log10(d))) & ~3) >> 2;
	d_[P] = (exp + 1) & EXPMASK;
	double m = pow(100.0, (double) exp);
	d /= m;
	d /= m;
	for (int i = 0; i < P; i++) {
	    short s = (short) d;
	    d_[i] = s;
	    d = (d - s) * 10000;
	}
	if (neg)
	    negate();
    }
    if (_round25())
    {
        // increase exponent
        int e = exp();
        ++e;
        if (e == EXPLIMIT) 
        {
            // Out of range; return Inf
            *this = !neg ? posInf() : negInf();
        }
        else
        {
            exp(e);
        }
    }
}
void BCDFloat::_roundDigits(unsigned int precision, BCDFloat* v) const
{
    if (!isSpecial() && !isZero() && precision < 25)
    {
        bool n = neg();

        BCDFloat rv;

        // adjust precision to number of digits before 4radix.
        if (d_[0] >= 10) --precision;
        if (d_[0] >= 100) --precision;
        if (d_[0] >= 1000) --precision;
        --precision; // units

        int m = BCDDecade[precision & 3] * 5;
        rv.ldexp(m, exp() - (precision>>2) - 1);

        // ignores signs
        _uadd(this, &rv, v);  

        // restore sign if needed
        if (n) v->setSign();
    }
    else
        *v = *this;
}

#ifndef PALMOS
void BCDFloat::_asString(char* buf, Format fmt, int precision) const
{
    char* p = buf;
    if (isSpecial())
    {
        if (isNan()) strcpy(p, "NaN");
        else 
        { // inf
            if (neg()) *p++ = '-';
            strcpy(p, "Inf");
        }
    }
    else 
    {
        int i;
        bool scimode = (fmt & format_scimode) != 0;
        if (neg()) *p++ = '-';
        char* point = 0;
        int v;
        int eadj = 0;
        int e = exp();
        int pr = 0;  // significant digits printed
        i = 0;
        if (e > 0) 
        {
            /* if the exponent is less than our digits, we can
             * print out the number like an integer.
             */
            if (e <= 5 && !scimode)
            {
                while (i < e) 
                {
                    v = d_[i];            
                    if (!i) sprintf(p, "%d", v);            
                    else sprintf(p, "%04d", v);            
                    while (*p) { ++p; ++pr; }
                    ++i;
                }
                e -= i;
            }
            else 
            {
                /* otherwise we have a larger number. print out
                 * as scientific form.
                 */
                v = d_[0];
                char tb[8];
                sprintf(tb, "%d", v);
                char* q = tb;
                *p++ = *q++;
                ++pr;
                point = p; 
                *p++ = '.';
                while (*q) 
                {
                    *p++ = *q++;
                    ++pr;
                    ++eadj;
                }
                ++i;
                --e;
            }
        }
        else 
        {
            /* otherwise have small number */
            *p++ = '0';
            *p = 0;
        }

        // here we have i = number of 4digits printed
        // e = remains of exponent

        int n = P;
        while (!d_[n-1] && n > i) --n;

        if (i < n) 
        {
            if (!point) 
            {
                point = p;
                *p++ = '.';
            }

            for (; i < n; ++i) 
            {
                v = d_[i];
                sprintf(p, "%04d", v); 
                pr += 4;
                p += 4;
                if (precision && pr >= precision)
                {
                    int over = pr - precision;
                    p -= over;
                    *p = 0;
                    break;
                }
            }
        }

        /* tidy up */
        if (point) 
            while (p > point && (p[-1] == '0' || p[-1] == '.')) *--p = 0;

        // print exponent
        if (e || scimode) 
#ifdef CUSTOM_EXP_CHAR
            sprintf(p, "%c%d", CUSTOM_EXP_CHAR, e*4+eadj);
#else
            sprintf(p, "e%d", e*4+eadj);
#endif
     }
}
#endif // !PALMOS


static unsigned int isqrt(unsigned int v) BCD1_SECT;
static unsigned int isqrt(unsigned int v)
{
    /* integer root for x<= 9999 */
    unsigned int x = 0;
    int b = 1<<7;
    do {
        x ^= b;  
        if (x*x > v) x ^= b;
    } while (b >>=1);
    return x;
}

bool BCDFloat::sqrt(const BCDFloat* a, BCDFloat* r)
{
    if (a->neg()) return false;
    if (a->isInf()) 
    {
        *r = *a;  // sqrt(inf) = inf
        return true;
    }
    if (a->isNan()) return false;

    BCDFloat acc;
    BCDFloat t;
    int rs;
    int as;
    int ts;
    int4 v;
    int rodd;
    int4 q;

    BCDFloat u;
    int us = 0;

    r->_init();

    int e = a->exp();

    v = isqrt(a->d_[0]);

    rodd = !(e & 1);
    r->d_[0] = v;
    if (rodd) r->d_[0] *= 100;
    rs = 1;

    as = 0;
    acc.d_[0] = a->d_[0] - v*v;
    if (acc.d_[0]) ++as;

    int i;
    int j = 1;
    int4 ca;

    for (;;) 
    {
        /* bring in the next digit */
        acc.d_[as] = j < P ? a->d_[j++] : 0;

        q = 0;
        if (acc.d_[0]) 
        {
            ++as;
        
            /* t = 200*r if even, t=2*r if odd */
            int m = rodd ? 2 : 200;
            ca = 0;
            ts = rs;
            for (i = rs; i > 0; --i) 
            {
                v = ((int4) r->d_[i-1])*m + ca;
                ca = 0;
                if (v >= BASE) 
                {
                    ca = v/BASE;
                    v -= ca*((int4)BASE);
                }
                t.d_[i] = v;
            }
            t.d_[i] = ca;
            ++ts;

            while (!t.d_[0]) 
            {
                for (i = 0; i < ts; ++i) t.d_[i] = t.d_[i+1];
                --ts;
            }

            if (ts > P) 
            {
                /* rarely, the tempory size can become bigger than
                 * we can handle. this can only happen on the last
                 * digit. if so, stop.
                 */
                break;
            }

            q = 0;
            if (ts == as) 
            {
                if (ts > 1)
                    q = (((int4) acc.d_[0])*((int4)BASE) + acc.d_[1])/(((int4) t.d_[0])*((int4)BASE)+t.d_[1]);
                else
                    q = ((int4)acc.d_[0])/t.d_[0];
            }
            else if (as > ts) 
            {
                q = (((int4) acc.d_[0])*((int4)BASE) + acc.d_[1])/t.d_[0];
            }

            if (q) 
            {
                if (q > 99) q = 99;
        
                /* t = t + q */
                t.d_[ts-1] += q;  // cant carry

                for (;;) 
                {
                    /* u = t*q */
                    ca = 0;
                    us = ts;
                    for (i = ts; i > 0; --i) 
                    {
                        v = t.d_[i-1]*q + ca;
                        ca = 0;
                        if (v >= BASE) 
                        {
                            ca = v/BASE;
                            v -= ca*((int4)BASE);
                        }
                        u.d_[i] = v;
                    }
                    u.d_[i] = ca;
                    if (ca) ++us;
                    else 
                        for (i = 0; i < us; ++i) u.d_[i] = u.d_[i+1];
                
                    /* determine whether u > acc. if so then q was too
                     * big.
                     */
                    bool fail = us > as;
                    if (!fail && us == as)
                    {
                        for (i = 0; i < as; ++i) 
                        {
                            int d = u.d_[i] - acc.d_[i];
                            if (d > 0) 
                            {
                                fail = true;
                                break;
                            }
                            else if (d < 0)
                                break;
                        }
                    }

                    if (!fail) break;

                    /* decrease q by 1 and try again */
                    q -= 1;
                    --t.d_[ts-1]; // adjust for new q
                }
            }
        }

        if (rodd) 
        {
            /* can accommodate 2 more digits in current size */
            r->d_[rs-1] += q;
            rodd = 0;
            if (rs == P) break;
        }
        else 
        {
            r->d_[rs++] = q*100;
            rodd = 1;
        }
        
        if (q) 
        {
            /* acc = acc - u.
             * wont borrow because u <= acc.
             */
            int k;
            ca = 0;
            k = us;
            for (i = as-1; i >= 0; --i) 
            {
                v = acc.d_[i] - ca;
                if (k > 0) v -= u.d_[--k];
                ca = 0;
                if (v < 0) 
                {
                    v += BASE;
                    ca = 1;
                }
                acc.d_[i] = v;
            }

            while (!acc.d_[0]) 
            {
                for (i = 0; i < as; ++i) acc.d_[i] = acc.d_[i+1];            
                if (!--as) break;
            }
        }
    }
    e = e >= -1 ? (e + 1) / 2 : e / 2;
    if (r->_round25()) ++e;
    r->exp(e);
    return true;
}

bool BCDFloat::trunc(const BCDFloat* a, BCDFloat* c)
{
    /* truncate towards zero.
     * trunc(2.1) = 2.
     * trunc(-2.1) = -2
     */
    *c = *a;
    int e = c->exp();
    int i;
    for (i = P-1; i >= 0; --i) 
        if (i >= e) c->d_[i] = 0;

    return true;
}

bool BCDFloat::floor(const BCDFloat* a, BCDFloat* c)
{
    /* floor, largest integer <= a.
     * eg floor(2.1) = 2.
     *    floor(-2.1) = -3.
     */

    *c = *a;
    int e = c->exp();
    int i;
    bool changed = false;
    for (i = P-1; i >= 0; --i) 
    {
        if (i >= e) 
        {
            if (c->d_[i]) changed = true;
            c->d_[i] = 0;
        }
    }
    
    if (c->neg() && changed) 
    {
        /* need to subtract 1 */
        for (i = P-1; i >= 0; --i) 
        {
            if (c->d_[i]) 
            {
                ++c->d_[i];
                break;
            }
        }
    }
    return true;
}

int4 BCDFloat::asInt() const
{
    // if we fit in an int, return it otherwise 0
    int ea = exp();
    int4 v = 0;
    int i = 0;
    while (i < ea && i < P) 
    {
        if (v > 214748L) return 0; // too large, bail out.
        v*= BASE;
        v += d_[i];
        ++i;
    }
    if (neg()) v = -v;
    return v;
}

bool BCDFloat::isInteger() const
{
    if (isZero())
        return true;

    int e = exp();
    int i;
    for (i = P-1; i >= 0; --i) 
        if (d_[i]) return e > i;
    return false;
}

void BCDFloat::mul2(const unsigned short* ad, int ea,
                    const unsigned short* bd, int eb,
                    unsigned short* cd, int& ec)
{
    int ca;
    int i, j;
    int4 u, v;

    unsigned short acc[2*P+1];

    for (i = 0; i < 2*P; ++i) cd[i] = 0;
    int cc = 0;
    for (i = 2*P-1; i >= 0; --i) 
    {
        for (j = 2*P; j > 0; --j) cd[j] = cd[j-1];
        cd[0] = cc; cc = 0;
        u = ad[i];
        if (!u) continue;

        ca = 0;
        for (j = 2*P; j > 0; --j) 
        {
            v = bd[j-1]*u + ca;
            ca = 0;
            if (v >= BASE) 
            {
                ca = v / BASE;
                v = v - ca*((int4)BASE);
            }
            acc[j] = v;
        }
        acc[0] = ca;

        /* now add acc into c */
        for (j = 2*P; j >= 0; --j) 
        {
            v = cd[j] + acc[j] + cc;
            cc = 0;
            if (v >= BASE) 
            {
                cc = 1;
                v -= BASE;
            }
            cd[j] = v;
        }
    }
    cd[0] += cc;  // carry?
    ec = ea + eb;
}
