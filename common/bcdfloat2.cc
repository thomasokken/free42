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

#include "bcdfloat2.h"

int BCDFloat2::decade_[4] = { 1000, 100, 10, 1 };
unsigned short BCDFloat2::posInfD_[P2+1] = { 0,0,0,0,0,0,0,0,0, 0, 0, 0, 0, 0, 0, 0x2000 };
unsigned short BCDFloat2::negInfD_[P2+1] = { 0,0,0,0,0,0,0,0,0, 0, 0, 0, 0, 0, 0, 0xA000 };
unsigned short BCDFloat2::nanD_[P2+1] =    { 0,0,0,0,0,0,0,0,0, 0, 0, 0, 0, 0, 0, 0x4000 };

void BCDFloat2::_init()
{
    int i;
    for (i = 0; i <= P2; ++i) d_[i] = 0;
}

void BCDFloat2::_fromUInt(uint4 v)
{
    /* quicker to deal with cases separately */
    if (v < BASE) 
    {
        d_[0] = v;
        d_[P2] = 1;
    }
    else if (v < ((int4)BASE)*BASE) 
    {
        d_[0] = v/BASE;
        d_[1] = v - d_[0]*((int4)BASE);
        d_[P2] = 2;
    }
    else 
    {
        d_[0] = v/(((int4)BASE)*BASE);
        v -= d_[0]*(((int4)BASE)*BASE);
        d_[1] = v/BASE;
        d_[2] = v - d_[1]*((int4)BASE);
        d_[P2] = 3;
    }
}

int BCDFloat2::_round25() 
{
    // round d_[P2] into the mantissa and mask off digits after 25.
    int i;
    int v;
    if (d_[0] < 10)
        v = d_[P2-1] + (d_[P2] >= 5000);
    else if (d_[0] < 100)
        v = (((((int4) d_[P2-1])+5)*3277)>>15)*10;
    else if (d_[0] < 1000)
        v = (((((int4) d_[P2-1])+50)*5243)>>19)*100;
    else
        v = (((((int4) d_[P2-1])+500)*8389)>>23)*1000;

    i = P2-1;
    while (v >= BASE)
    {
        d_[i] = v - BASE;
        if (!i)
        {
            // shift
            _rshift();
            d_[0] = 1;
            return 1;
        }
        v = d_[--i]+1;
    }
    d_[i] = v;
    return 0;
}

void BCDFloat2::epsilon(int n, BCDFloat2* v)
{
    // generate 10^-n, 
    int m = decade_[(n-1) & 3];
    v->ldexp(m, -(n>>2));
}

void BCDFloat2::add(const BCDFloat2* a, const BCDFloat2* b, BCDFloat2* c)
{
    if (a->isZero()) 
    {
        *c = *b;
        return;
    }
    if (b->isZero()) 
    {
        *c = *a;
        return;
    }

    if (a->isSpecial() || b->isSpecial()) 
    {
        /* inf + inf = inf
         * -inf + (-inf) = -inf
         */
        bool done = false;
        if (!a->isNan() && !b->isNan()) {
            if (a->isInf()) {
                if (b->isInf()) {
                    if (a->d_[P2] == b->d_[P2]) { *c = *a; done = true; }
                }
                else {
                    *c = *a;
                    done = true; // inf + x = inf
                }
            }
            else { // b is inf
                *c = *b;
                done = true; // x + inf = inf
            }
        }

        if (!done) {
            /* all others -> nan */
            *c = nan();
        }
        return;
    }

    int ea = a->exp();
    int eb = b->exp();

    bool na = a->neg();
    bool nb = b->neg();
    bool sub = na != nb;

    if (sub) 
    {
        if (ea >= eb) 
            _usub(a, b, c);
        else 
        {
            _usub(b, a, c);
            na = nb;
        }
    }
    else 
    {
        if (ea >= eb) _uadd(a, b, c);
        else _uadd(b, a, c);
    }
    if (na) c->negate();
}

void BCDFloat2::sub(const BCDFloat2* a, const BCDFloat2* b, BCDFloat2* c)
{
    if (a->isZero()) 
    {
        *c = *b;
        c->negate();
        return;
    }
    if (b->isZero()) 
    {
        *c = *a;
        return;
    }

    if (a->isSpecial() || b->isSpecial()) {
        bool done = false;
        if (!a->isNan() && !b->isNan()) {
            if (a->isInf()) {
                if (b->isInf()) {
                    /* -inf - (inf) = -inf
                     * inf - (-inf) = inf
                     */
                    if (a->d_[P2] != b->d_[P2]) { *c = *a; done = true; }
                }
                else { // a inf && !b inf
                    *c = *a;
                    done = true; // inf - x = inf
                }
            }
            else { // b is inf
                *c = *b;
                c->negate();
                done = true; // x - inf = -inf
            }
        }

        if (!done) {
            /* all others -> nan */
            *c = nan();
        }
        return;
    }

    bool na = a->neg();
    bool nb = b->neg();
    bool sub = (na == nb);

    int ea = a->exp();
    int eb = b->exp();

    if (sub) {
        if (ea >= eb) {
            _usub(a, b, c);
            if (na) c->negate();
        }
        else {
            _usub(b, a, c);
            if (!na) c->negate();
        }
    }
    else {
        if (ea >= eb) _uadd(a, b, c);
        else _uadd(b, a, c);

        if (na) c->negate();
    }
}

void BCDFloat2::_uadd(const BCDFloat2* a, const BCDFloat2* b, BCDFloat2* c)
{
    int ea = a->exp();
    int eb = b->exp();
    int d = ea - eb;

    if (d <= P2) // otherwise `b' is insignificant
    {
        int i;
        int ca;
        int v;
        int j = P2-d;
        i = P2-1;

        // copy in first insignificant digit used by final rounding
        v = 0;
        if (d > 0) v = b->d_[j]; 
        c->d_[P2] = v;

        ca = 0;
        while (j > 0) // perform addition of overlapping terms
        {
            v = a->d_[i] + b->d_[--j] + ca;
            ca = 0;
            if (v >= BASE)
            {
                v -= BASE;
                ca = 1;
            }
            c->d_[i] = v;
            --i;
        }

        while (i >= 0) // remainder non-overlap terms
        {
            v = a->d_[i] + ca;
            ca = 0;
            if (v >= BASE)
            {
                v -= BASE;
                ca = 1;
            }
            c->d_[i] = v;
            --i;
        }

        if (ca)
        {
            /* overall carry, shift down and round */
            c->_rshift();
            c->d_[0] = ca;
            ++ea;
        }

        if (c->_round25()) ++ea;
        if (ea > EXPLIMIT) *c = posInf();
        else c->exp(ea);
    }
    else
    {
        *c = *a;
        c->clearSign();
    }
}

void BCDFloat2::_usub(const BCDFloat2* a, const BCDFloat2* b, BCDFloat2* c)
{
    int ea = a->exp();
    int eb = b->exp();
    bool neg = false;

    int d = ea - eb;
    if (d <= P2)
    {
        int i;
        int ca;
        int v;

        int j = P2-d;
        i = P2-1;

	ca = 0;
        c->d_[P2] = 0;
        while (j > 0)
        {
            v = a->d_[i] - b->d_[--j] - ca;
            ca = 0;
            if (v < 0) 
            {
                ca = 1;
                v += BASE;
            }
            c->d_[i] = v;
            --i;
        }

        if (ca)
        {
            while (i >= 0)
            {
                v = a->d_[i];
                if (v)
                {
                    // carry absorbed
                    c->d_[i] = v - 1;
                    
                    // copy remainder
                    while (i > 0) { --i; c->d_[i] = a->d_[i]; }

                    // NB: i == 0
                    break;
                }
                else
                {
                    c->d_[i] = BASE-1;
                    --i;
                }
            }

            if (i < 0) // carry all the way up
            {
                /* overall borrow, need to complement number */
                for (i = P2; i >= 0; --i) 
                {
                    v = BASE-1 - c->d_[i] + ca;
                    ca = 0;
                    if (v >= BASE) 
                    {
                        ca = 1;
                        v -= BASE;
                    }
                    c->d_[i] = v;
                }
                neg = true;
            }
        }
        else
        {
            // copy remainder of number over
            while (i >= 0)
            {
                c->d_[i] = a->d_[i];
                --i;
            }
        }

        int e = a->exp();
        i = 0;
        while (c->d_[i] == 0 && i <= P2) i++;
        if (i > 0) {
            if (i == P2+1)
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
                    c->d_[0] = 0;
                    e = 0;
                }
                else 
                {
                    int j;
                    for (j = 0; j <= P2 - i; j++)
                        c->d_[j] = c->d_[j + i];
                    for (; j <= P2; j++)
                        c->d_[j] = 0;
                }
            }
        }

        // cant happen
        // if (c->_round25()) ++e;

        if (e > EXPLIMIT) *c = posInf();
        else c->exp(e);
        if (neg) c->negate();
    }
    else
    {
        /* `b' is insignificant */
        *c = *a;
        c->clearSign();
    }
}

void BCDFloat2::mul(const BCDFloat2* a, const BCDFloat2* b, BCDFloat2* c)

{
    int na = a->neg();
    int nb = b->neg();

    bool az = a->isZero();
    bool bz = b->isZero();

    if (a->isSpecial() || b->isSpecial()) 
    {
        bool done = false;
        if (!a->isNan() && !b->isNan()) 
        {
            if (a->isInf()) 
            {
                if (b->isInf()) 
                {
                    *c = posInf();
                    done = true;
                }
                else 
                {
                    if (!bz) 
                    {   // inf * 0 = nan
                        *c = posInf();
                        done = true; // inf * x = inf, x != 0
                    }
                }
            }
            else 
            { // b is inf
                if (!az)
                { // 0 * inf = nan
                    *c = posInf();
                    done = true; // x * inf = inf, x != 0
                }
            }
        }

        if (!done) 
            /* all others -> nan */
            *c = nan();
        else 
            if (na != nb) c->negate();            

        return;
    }

    c->_init();

    // quit now if either is zero.
    if (az || bz) return; 

    int ca;
    int i, j;
    int4 u, v;

    int ea = a->exp();
    int eb = b->exp();
    BCDFloat2 acc;

    for (i = P2-1; i >= 0; --i) 
    {
        c->_round25(); // wont carry
        c->_rshift();
        c->d_[0] = 0;
        
        u = a->d_[i];
        if (!u) continue;

        ca = 0;
        for (j = P2; j > 0; --j) 
        {
            v = b->d_[j-1] * u + ca;
            ca = 0;
            if (v >= BASE) 
            {
                ca = v / BASE;
                v = v - ca*((int4)BASE);
            }
            acc.d_[j] = v;
        }
        acc.d_[0] = ca;

        /* now add acc into c */
        ca = 0;
        for (j = P2; j >= 0; --j) 
        {
            v = c->d_[j] + acc.d_[j] + ca;
            ca = 0;
            if (v >= BASE) 
            {
                ca = 1;
                v -= BASE;
            }
            c->d_[j] = v;
        }

        /* won't be any overall carry */
    }

    if (!c->d_[0]) 
    {
        c->_lshift();
        c->d_[P2] = 0;
    }
    else 
    {
        ++ea;
        if (c->_round25()) ++ea;
    }

    ea += eb - 1;
    if (ea <= -EXPLIMIT) c->_init();
    else 
    {
        if (ea > EXPLIMIT) *c = posInf();
        else c->exp(ea);

        /* fix sign */
        if (na != nb) c->negate();
    }
}

void BCDFloat2::div(const BCDFloat2* a, const BCDFloat2* b, BCDFloat2* c)
{
    int na = a->neg();
    int nb = b->neg();

    if (a->isSpecial() || b->isSpecial()) {
        bool done = false;
        if (!a->isNan() && !b->isNan()) {
            if (a->isInf()) {
                if (b->isInf()) {
                    /* inf / inf = nan */
                }
                else {
                    *c = posInf();
                    if (na != nb) c->negate();            
                    done = true; // inf / x = inf
                }
            }
            else { // b is inf
                /* x/inf = 0 */
                c->_init();
                done = true;
            }
        }

        if (!done) {
            /* all others -> nan */
            *c = nan();
        }
        return;
    }

    int4 u, v;
    int ca;
    int j = 0;
    int i;
    int4 q;

    bool az = a->isZero();
    bool bz = b->isZero();
    if (az || bz) {
        if (az) {
            if (bz) *c = nan();
            else c->_init();
        }
        else { // bz && !az
            *c = posInf();
            if (a->neg()) c->negate();
        }
    }
    else {
        int ea = a->exp();
        int eb = b->exp();


        BCDFloat2 acc;
        BCDFloat2 b1;

        u = ((int4)BASE)/(b->d_[0]+1);

        if (u != 1) {
            /* prenormialise `a' and move into acc using spare digit */
            ca = 0;
            for (i = P2; i > 0; --i) {
                v = a->d_[i-1]*u + ca;
                ca = 0;
                if (v >= BASE) {
                    ca = v/BASE;
                    v -= ca*((int4)BASE);
                }
                acc.d_[i] = v;
            }
            acc.d_[0] = ca;

            /* prenormalise `b' into b1 */
            ca = 0;
            for (i = P2-1; i >= 0; --i) {
                v = b->d_[i]*u + ca;
                ca = 0;
                if (v >= BASE) {
                    ca = v/BASE;
                    v -= ca*((int4)BASE);
                }
                b1.d_[i] = v;
            }
        }
        else {
            /* u is often 1 */
            for (i = P2-1; i >= 0; --i) {
                acc.d_[i+1] = a->d_[i];
                b1.d_[i] = b->d_[i];
            }
            acc.d_[0] = 0;
        }

        for (;;) {
            if (acc.d_[0] == b1.d_[0]) q = BASE-1;
            else {
                v = acc.d_[0]*((int4)BASE) + acc.d_[1];
                q = v/b1.d_[0];

                while (b1.d_[1]*q > ((v - q*b1.d_[0])*BASE + acc.d_[2])) {
                    --q;
                }
            }

            if (!q && !j) {
                /* first quotient digit is zero. can gain extra
                 * accuracy by ignoring this and adjusting exponent.
                 */
                --ea;
            }
            else {
                if (j == P2) {
                    c->d_[j] = q;
                    break;
                }

                ca = 0;
                for (i = P2; i > 0; --i) {
                    v = acc.d_[i] - b1.d_[i-1]*q - ca;
                    ca = 0;
                    if (v < 0) {
                        ca = (-v + BASE-1)/BASE;
                        v += ca*((int4)BASE);
                    }
                    acc.d_[i] = v;
                }
                v = acc.d_[0] - ca;

                if (v) {
                    /* the infamous add back correction */
                    ca = 0;
                    for (i = P2; i > 0; --i) {
                        v = acc.d_[i] + b1.d_[i-1] + ca;
                        ca = 0;
                        if (v >= BASE) {
                            ca = 1;
                            v -= BASE;
                        }
                        acc.d_[i] = v;
                    }
                    q--;
                }
                if (q == 0 && j == 0)
                    --ea;
                else
                    c->d_[j++] = q;
            }

            acc._lshift();
            acc.d_[P2] = 0;
        }

        if (c->_round25()) ++ea;
        
        ea -= eb - 1;
        if (ea <= -EXPLIMIT) c->_init();
        else {
            if (ea > EXPLIMIT) *c = posInf();
            else c->exp(ea);
            if (na != nb) c->negate();
        }
    }
}


int BCDFloat2::cmp(const BCDFloat2 *a, const BCDFloat2 *b)
{
    if (a->isNan() || b->isNan())
	// NaNs are not equal to anything, even themselves;
	// ALL comparisions involving them should return 'false'
	// (except '!=', which should return 'true').
	// So, I return a special value here.
	return 2;

    bool sa = a->neg();
    bool sb = b->neg();
    if (sa != sb)
	return sa ? -1 : 1;

    // If a and b are both negative, switch them. This way, I can
    // ignore signs in the remainder of this method.
    if (sa) {
	const BCDFloat2 *tmp = a;
	a = b;
	b = tmp;
    }

    if (a->isInf())
	return b->isInf() ? 0 : 1;
    if (b->isInf())
	return -1;

    if (a->isZero())
	return b->isZero() ? 0 : -1;
    if (b->isZero())
	return 1;

    int ea = a->exp();
    int eb = b->exp();
    if (ea != eb)
	return ea < eb ? -1 : 1;

    for (int i = 0; i < P2; i++) {
	int da = a->d_[i];
	int db = b->d_[i];
	if (da != db)
	    return da < db ? -1 : 1;
    }
    return 0;
}

bool BCDFloat2::trunc(const BCDFloat2* a, BCDFloat2* c)
{
    /* truncate towards zero.
     * trunc(2.1) = 2.
     * trunc(-2.1) = -2
     */
    *c = *a;
    int e = c->exp();
    int i;
    for (i = P2-1; i >= 0; --i) 
        if (i >= e) c->d_[i] = 0;

    return true;
}

bool BCDFloat2::floor(const BCDFloat2* a, BCDFloat2* c)
{
    /* floor, largest integer <= a.
     * eg floor(2.1) = 2.
     *    floor(-2.1) = -3.
     */

    *c = *a;
    int e = c->exp();
    int i;
    bool changed = false;
    for (i = P2-1; i >= 0; --i) 
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
        for (i = P2-1; i >= 0; --i) 
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

int4 BCDFloat2::asInt() const
{
    // if we fit in an int, return it otherwise 0
    int ea = exp();
    int4 v = 0;
    int i = 0;
    while (i < ea && i < P2) 
    {
        if (v > 214748L) return 0; // too large, bail out.
        v*= BASE;
        v += d_[i];
        ++i;
    }
    if (neg()) v = -v;
    return v;
}

bool BCDFloat2::isInteger() const
{
    if (isZero())
        return true;

    int e = exp();
    int i;
    for (i = P2-1; i >= 0; --i) 
        if (d_[i]) return e > i;
    return false;
}

bool BCDFloat2::sqrt(const BCDFloat2* a, BCDFloat2* r)
{
    // use single precision version as approx.
    BCDFloat a1;
    BCDFloat r1;
    a->asBCD(&a1);
    bool res = BCDFloat::sqrt(&a1, &r1);
    
    BCDFloat2 rr(r1);
    BCDFloat2 t, t2;
    BCDFloat2 two(2);
    if (res)
    {
        if (!rr.isSpecial())
        {
            for (int i = 0; i < 3; ++i)
            {
                div(a,&rr,&t);              // t = a/x
                add(&t, &rr, &t2);          // t2 = (a/x) + x
                div(&t2, &two, &rr);        // x = (t2)/2
            }
        }
        *r = rr;
    }
    return res;
}
