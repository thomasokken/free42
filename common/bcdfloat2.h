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

#ifndef __bcdfloat2_h__
#define __bcdfloat2_h__

#include "bcdfloat.h"

#define P2 (2*P+1)

struct BCDFloatData2
{
    /* store P 4dec `digits', equivalent to P*4 decimal digits.
     * the last place is the exponent.
     */
    unsigned short      d_[P2+1];  
};

struct BCDFloat2: public BCDFloatData2
{
    BCDFloat2() {} // Warning: not initialised.
    BCDFloat2(int4 v)
    {
        _init();
        if (v)
        {
            bool neg = v < 0;
            if (neg)
                v = -v;
            _fromUInt((uint4)v);
            if (neg) negate();
        }
    }
#if defined(PALMOS) && !defined(PALMOS_ARM)
    BCDFloat2(int v)
    {
        _init();
        if (v)
        {
            bool neg = v < 0;
            if (neg)
                v = -v;
            _fromUInt((uint4)v);
            if (neg) negate();
        }
    }
#endif

    BCDFloat2(uint4 v)
    {
        _init();
        if (v) _fromUInt(v);
    }

    BCDFloat2(const BCDFloatData2& d) { *this = *(BCDFloat2*)&d; }

    BCDFloat2(const BCDFloatData& d)
    {
        int i;
        for (i = 0; i < P; ++i) d_[i] = d.d_[i];
        while (i < P2) d_[i++] = 0;
        d_[P2] = d.d_[P];
    }

    // Features
    int                 exp() const { return ((short)(d_[P2] << 3)) >> 3; }
    void                exp(int v) { d_[P2] = v & EXPMASK; }
    bool                neg() const
    { return (d_[P2]& NEG) && (d_[0] != 0 || isInf()); }
            
    void                setSign() { d_[P2] |= NEG; }
    void                clearSign() { d_[P2] &= ~NEG; }
    void                negate() { d_[P2] ^= NEG; }
    bool                isSpecial() const { return (d_[P2]&0x6000) != 0; } 

    bool                isZero() const
                        { return d_[0] == 0 && !isSpecial(); }

    bool                isNan() const { return (d_[P2]&0x4000) != 0; }
    bool                isInf() const { return (d_[P2]&0x2000) != 0; }
    bool                isInteger() const BCD_SECT;

    void                asBCD(BCDFloat* v) const
    {
        for (int i = 0; i <= P; ++i) v->d_[i] = d_[i];
        int e = d_[P2];
        if (v->_round25()) ++e; // XX overflow?
        v->d_[P] = e;
    }

    void                ldexp(unsigned int mant, int e)
    {
        // load the exp as a 4-block
        _init();
        _fromUInt(mant);
        exp(e);
    }

    static void         add(const BCDFloat2* a, const BCDFloat2* b, BCDFloat2* c) BCD_SECT;
    static void         sub(const BCDFloat2* a, const BCDFloat2* b, BCDFloat2* c) BCD_SECT;
    static void         mul(const BCDFloat2* a, const BCDFloat2* b, BCDFloat2* c) BCD_SECT;
    static void         div(const BCDFloat2* a, const BCDFloat2* b, BCDFloat2* c) BCD_SECT;
    static bool         sqrt(const BCDFloat2* a, BCDFloat2* ra) BCD_SECT;
    static int          cmp(const BCDFloat2 *a, const BCDFloat2 *b) BCD_SECT;
    static bool         lt(const BCDFloat2* a, const BCDFloat2* b)
    {
        /* true iff a < b */
        return cmp(a, b) == -1;
    }

    static bool         le(const BCDFloat2* a, const BCDFloat2* b)
    {
        /* true iff a <= b */
        int res = cmp(a, b);
        return res == -1 || res == 0;
    }

    static bool         gt(const BCDFloat2* a, const BCDFloat2* b)
    {
        /* true iff a > b */
        return cmp(a, b) == 1;
    }

    static bool         ge(const BCDFloat2* a, const BCDFloat2* b)
    {
        /* true iff a >= b */
        int res = cmp(a, b);
        return res == 1 || res == 0;
    }

    static bool         equal(const BCDFloat2* a, const BCDFloat2* b)
    {
        return cmp(a, b) == 0;
    }

    static int4         ifloor(const BCDFloat2* x) 
    {
        BCDFloat2 a;
        floor(x, &a);
        return a.asInt();
    }

    static int4         itrunc(const BCDFloat2* x) 
    {
        BCDFloat2 a;
        trunc(x, &a);
        return a.asInt();
    }

    static bool         floor(const BCDFloat2* a, BCDFloat2* c) BCD_SECT;
    static bool         trunc(const BCDFloat2* a, BCDFloat2* c) BCD_SECT;

    void                _init() BCD_SECT;
    static void         _uadd(const BCDFloat2* a, const BCDFloat2* b,
                              BCDFloat2* c) BCD_SECT;
    static void         _usub(const BCDFloat2* a, const BCDFloat2* b,
                              BCDFloat2* c) BCD_SECT;
    void                _rshift() BCD_SECT;
    void                _lshift() BCD_SECT;
    int                 _round25()BCD_SECT;
    void                _fromUInt(uint4) BCD_SECT;


    static const BCDFloat2& posInf() { return *(BCDFloat2*)posInfD_; }
    static const BCDFloat2& negInf() { return *(BCDFloat2*)negInfD_; }
    static const BCDFloat2& nan() { return *(BCDFloat2*)nanD_; }
    static void         epsilon(int n, BCDFloat2* v) BCD_SECT;

    int4                asInt() const BCD_SECT;
    

    static unsigned short posInfD_[P2+1];
    static unsigned short negInfD_[P2+1];
    static unsigned short nanD_[P2+1];
    static int            decade_[4];
};

inline void BCDFloat2::_rshift()
{
    int i;
    for (i = P2; i > 0; --i) d_[i] = d_[i-1];
}

inline void BCDFloat2::_lshift()
{
    int i;
    for (i = 0; i < P2; ++i) d_[i] = d_[i+1]; 
}

#endif 
