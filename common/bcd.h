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

#ifndef __bcd_h__
#define __bcd_h__

#include "bcdfloat.h"

struct BCD
{
    typedef BCDFloat::Format Format;

    // Constructors
    BCD() {}
    BCD(const char* s) : _v(s) {}
#if defined(PALMOS) && !defined(PALMOS_ARM)
    BCD(int v) : _v(v) {}
#endif
    BCD(int4 v) : _v(v) {}
    BCD(uint4 v) : _v(v) {}
    BCD(const BCDFloatData& bf) : _v(bf) {}

    int                 exponent() const { return _v.exp(); }
    void                setExponent(int v) { _v.exp(v); }
    int                 digit(int n) const { return _v.d_[n]; }
    void                digit(int n, int v)  { _v.d_[n] = v; }

    // Arithmetic
    friend BCD          operator+(const BCD& a, const BCD& b)
    {
        BCD c;
        BCDFloat::add(&a._v, &b._v, &c._v);
        return c;
    }
    friend BCD          operator-(const BCD& a, const BCD& b)
    {
        BCD c;
        BCDFloat::sub(&a._v, &b._v, &c._v);
        return c;
    }
    friend BCD          operator*(const BCD& a, const BCD& b)
    {
        BCD c;
        BCDFloat::mul(&a._v, &b._v, &c._v);
        return c;
    }
    friend BCD          operator/(const BCD& a, const BCD& b)
    {
        BCD c;
        BCDFloat::div(&a._v, &b._v, &c._v);
        return c;
    }
    void                operator+=(const BCD& b)
    {
        BCD c;
        BCDFloat::add(&_v, &b._v, &c._v);
        _v = c._v;
    }
    void                operator-=(const BCD& b)
    {
        BCD c;
        BCDFloat::sub(&_v, &b._v, &c._v);
        _v = c._v;
    }
    void                operator*=(const BCD& b)
    {
        BCD c;
        BCDFloat::mul(&_v, &b._v, &c._v);
        _v = c._v;
    }
    void                operator/=(const BCD& b)
    {
        BCD c;
        BCDFloat::div(&_v, &b._v, &c._v);
        _v = c._v;
    }
    BCD                 operator-() const
    {
        BCD c(*this);
        c.negate();
        return c;
    }
    void                operator++() { *this += 1; }
    void                operator--() { *this -= 1; }

    friend int4         ifloor(const BCD& a)
                        { return BCDFloat::ifloor(&a._v); }
    friend int4         itrunc(const BCD& a)
                        { return BCDFloat::itrunc(&a._v); }
    friend BCD          floor(const BCD& a)
    {
        /* floor, largest integer <= a.
         * eg floor(2.1) = 2.
         *    floor(-2.1) = -3.
         */
        BCD t;
        if (a.isSpecial()) return a;
        BCDFloat::floor(&a._v, &t._v);
        return t;
    }

    friend BCD          trunc(const BCD& a)
    {
        /* truncate towards zero.
         * trunc(2.1) = 2.
         * trunc(-2.1) = -2
         */
        if (a.isSpecial()) return a;
        BCD t;
        BCDFloat::trunc(&a._v, &t._v);
        return t;
    }

    friend BCD          fabs(const BCD& a) { return (a.isNeg()) ? -a : a; }

    friend BCD          frac(const BCD& a)
    {
        if (a.isSpecial()) return a;
        return a - trunc(a);
    }
#ifndef PALMOS
    const char*         asString() const
    {
        _v.asString(_buf);
        return _buf;
    }
#endif

    static BCD          epsilon(int n)
    {
        BCD v;
        BCDFloat::epsilon(n, &v._v);
        return v;
    }

#ifndef PALMOS
    const char*         asStringFmt(Format fmt, int precision) const;
#endif

    bool                isZero() const { return _v.isZero(); }
    bool                isNeg() const { return _v.neg(); }
    bool                isSpecial() const
                                { return _v.isSpecial(); }
    bool                isInf() const
                                { return  _v.isInf(); }
    bool                isNan() const
                                { return _v.isNan(); }
    bool                isInteger() const { return _v.isInteger(); }

    void                negate() { _v.negate(); }
    BCD                 round(int digits) const
    {
        BCD rv;
        _v._roundDigits(digits, &rv._v);
        return rv;
    }

    static BCD          inf() { return BCDFloat::posInf(); }


    // Comparision
    friend bool         operator==(const BCD& a, const BCD& b)
                        { return BCDFloat::equal(&a._v, &b._v); }
    friend bool         operator!=(const BCD& a, const BCD& b)
                        { return !BCDFloat::equal(&a._v, &b._v); }
    friend bool         operator<(const BCD& a, const BCD& b)
                        { return BCDFloat::lt(&a._v, &b._v); }
    friend bool         operator<=(const BCD& a, const BCD& b)
                        { return BCDFloat::le(&a._v, &b._v); }
    friend bool         operator>(const BCD& a, const BCD& b)
                        { return BCDFloat::gt(&a._v, &b._v); }
    friend bool         operator>=(const BCD& a, const BCD& b)
                        { return BCDFloat::ge(&a._v, &b._v); }

    BCDFloat            _v;
    static char         _buf[64];
};

BCD pow(const BCD& a, int4 n) BCD1_SECT;

inline bool operator==(const BCDFloatData& a, const BCDFloatData& b)
{
    return *(BCD*)&a == *(BCD*)&b;
}

inline bool operator<(const BCDFloatData& a, const BCDFloatData& b)
{
    return *(BCD*)&a < *(BCD*)&b;
}

inline bool operator<=(const BCDFloatData& a, const BCDFloatData& b)
{
    return *(BCD*)&a <= *(BCD*)&b;
}

inline bool operator>(const BCDFloatData& a, const BCDFloatData& b)
{
    return *(BCD*)&a > *(BCD*)&b;
}

inline bool operator>=(const BCDFloatData& a, const BCDFloatData& b)
{
    return *(BCD*)&a >= *(BCD*)&b;
}

inline BCD operator-(const BCDFloatData& a, const BCDFloatData& b)
{
    return *(BCD*)&a - *(BCD*)&b;
}

#endif // __bcd_h__
