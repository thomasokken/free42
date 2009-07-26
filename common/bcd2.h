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

#ifndef __bcd2_h__
#define __bcd2_h__

#include "bcdfloat2.h"
#include "bcd.h"

struct BCD2
{

    // Constructors
    BCD2() {}
    BCD2(int4 v) : _v(v) {}
    BCD2(uint4 v) : _v(v) {}

    BCD2(const BCDFloatData2& bf) : _v(bf) {}

    BCD2(const BCDFloatData& bf) : _v(bf) {}
    BCD2(const BCD& b) : _v(b._v) {}

    int                 exponent() const { return _v.exp(); }
    void                setExponent(int v) { _v.exp(v); }
    int                 digit(int n) const { return _v.d_[n]; }
    void                digit(int n, int v)  { _v.d_[n] = v; }
    BCD                 asBCD() const
    {
        BCD v;
        _v.asBCD(&v._v);
        return v;
    }

    // Arithmetic
    friend BCD2          operator+(const BCD2& a, const BCD2& b)
    {
        BCD2 c;
        BCDFloat2::add(&a._v, &b._v, &c._v);
        return c;
    }
    friend BCD2          operator-(const BCD2& a, const BCD2& b)
    {
        BCD2 c;
        BCDFloat2::sub(&a._v, &b._v, &c._v);
        return c;
    }
    friend BCD2          operator*(const BCD2& a, const BCD2& b)
    {
        BCD2 c;
        BCDFloat2::mul(&a._v, &b._v, &c._v);
        return c;
    }
    friend BCD2          operator/(const BCD2& a, const BCD2& b)
    {
        BCD2 c;
        BCDFloat2::div(&a._v, &b._v, &c._v);
        return c;
    }
    void                operator+=(const BCD2& b)
    {
        BCD2 c;
        BCDFloat2::add(&_v, &b._v, &c._v);
        _v = c._v;
    }
    void                operator-=(const BCD2& b)
    {
        BCD2 c;
        BCDFloat2::sub(&_v, &b._v, &c._v);
        _v = c._v;
    }
    void                operator*=(const BCD2& b)
    {
        BCD2 c;
        BCDFloat2::mul(&_v, &b._v, &c._v);
        _v = c._v;
    }
    void                operator/=(const BCD2& b)
    {
        BCD2 c;
        BCDFloat2::div(&_v, &b._v, &c._v);
        _v = c._v;
    }
    BCD2                 operator-() const
    {
        BCD2 c(*this);
        c.negate();
        return c;
    }
    void                operator++() { *this += 1; }
    void                operator--() { *this -= 1; }

    friend int4         ifloor(const BCD2& a)
                        { return BCDFloat2::ifloor(&a._v); }
    friend int4         itrunc(const BCD2& a)
                        { return BCDFloat2::itrunc(&a._v); }
    friend BCD2          floor(const BCD2& a)
    {
        /* floor, largest integer <= a.
         * eg floor(2.1) = 2.
         *    floor(-2.1) = -3.
         */
        BCD2 t;
        if (a.isSpecial()) return a;
        BCDFloat2::floor(&a._v, &t._v);
        return t;
    }

    friend BCD2          trunc(const BCD2& a)
    {
        /* truncate towards zero.
         * trunc(2.1) = 2.
         * trunc(-2.1) = -2
         */
        if (a.isSpecial()) return a;
        BCD2 t;
        BCDFloat2::trunc(&a._v, &t._v);
        return t;
    }

    friend BCD2          fabs(const BCD2& a) { return (a.isNeg()) ? -a : a; }
    friend BCD2          frac(const BCD2& a)
    {
        if (a.isSpecial()) return a;
        return a - trunc(a);
    }
    static BCD2          epsilon(int n)
    {
        BCD2 v;
        BCDFloat2::epsilon(n, &v._v);
        return v;
    }

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

    static BCD2          inf() { return BCDFloat2::posInf(); }


    // Comparision
    friend bool         operator==(const BCD2& a, const BCD2& b)
                        { return BCDFloat2::equal(&a._v, &b._v); }
    friend bool         operator!=(const BCD2& a, const BCD2& b)
                        { return !BCDFloat2::equal(&a._v, &b._v); }
    friend bool         operator<(const BCD2& a, const BCD2& b)
                        { return BCDFloat2::lt(&a._v, &b._v); }
    friend bool         operator<=(const BCD2& a, const BCD2& b)
                        { return BCDFloat2::le(&a._v, &b._v); }
    friend bool         operator>(const BCD2& a, const BCD2& b)
                        { return BCDFloat2::gt(&a._v, &b._v); }
    friend bool         operator>=(const BCD2& a, const BCD2& b)
                        { return BCDFloat2::ge(&a._v, &b._v); }

    BCDFloat2            _v;
};

inline bool operator==(const BCDFloatData2& a, const BCDFloatData2& b)
{
    return *(BCD2*)&a == *(BCD2*)&b;
}

inline bool operator<(const BCDFloatData2& a, const BCDFloatData2& b)
{
    return *(BCD2*)&a < *(BCD2*)&b;
}

inline bool operator<=(const BCDFloatData2& a, const BCDFloatData2& b)
{
    return *(BCD2*)&a <= *(BCD2*)&b;
}

inline bool operator>(const BCDFloatData2& a, const BCDFloatData2& b)
{
    return *(BCD2*)&a > *(BCD2*)&b;
}

inline bool operator>=(const BCDFloatData2& a, const BCDFloatData2& b)
{
    return *(BCD2*)&a >= *(BCD2*)&b;
}

inline BCD2 operator-(const BCDFloatData2& a, const BCDFloatData2& b)
{
    return *(BCD2*)&a - *(BCD2*)&b;
}

inline BCD2 sqrt(const BCD2& a)
{
    BCD2 c;
    if (!BCDFloat2::sqrt(&a._v, &c._v))
        c._v = BCDFloat2::nan();
    return c;
}

#endif // __bcd2_h__
