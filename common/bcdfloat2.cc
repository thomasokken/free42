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

void BCDFloat2::_init()
{
    for (int i = 0; i <= P2; ++i) d_[i] = 0;
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
    BCDFloat2 two(2U);
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
