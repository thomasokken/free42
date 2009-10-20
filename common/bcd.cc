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

extern "C"
{
#include <stdlib.h>
}

#include "bcd2.h"

char BCD::_buf[64];

BCD sqrt(const BCD& a)
{
    BCD c;
    if (!BCDFloat::sqrt(&a._v, &c._v))
        c._v = BCDFloat::nan();
    return c;
}


#ifndef PALMOS
const char* BCD::asStringFmt(Format fmt, int precision) const
{
    const BCD* bp = this;
    BCD rv;
    if ((fmt & BCDFloat::format_truncate) == 0)
    {
        rv = round(precision);
        bp = &rv;

    }
    bp->_v.asStringFmt(_buf, fmt, precision);
    return _buf;
}
#endif

BCD pow(const BCD& a, int4 n)
{
    int4 m;
    if (n == 0) return 1;
    m = (n < 0) ? -n : n;

    BCD2 s;
    if (m > 1) 
    {
        BCD2 r = a;
        s = 1;
        /* Use binary exponentiation */
        for (;;) 
        {
            if (m & 1) s *= r;
            m >>= 1;
            if (!m) break;
            r *= r;
        }
    } else { s = a; }

    /* Compute the reciprocal if n is negative. */
    BCD s1 = s.asBCD();
    return n < 0 ? 1/s1 : s1;
}
