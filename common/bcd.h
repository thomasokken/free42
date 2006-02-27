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

#ifndef __abcd_h__
#define __abcd_h__

#include "bcdfloat.h"

struct BCDRef
{
    BCDFloat            v_;
    BCDRef*             next_;

    void                _free() { next_ = pool_;  pool_ = this; }
    static BCDRef*      _alloc() BCD_SECT;
    static BCDRef*      pool_;
};

struct BCD
{
    // Constructors
    BCD() { ref_ = 0; }
    BCD(BCDRef* r) : ref_(r) {}
    BCD(const char* s) { ref_ = BCDRef::_alloc(); ref_->v_ = BCDFloat(s); }
#ifdef PALMOS
    BCD(int v) { ref_ = BCDRef::_alloc(); ref_->v_ = BCDFloat(v); }
#endif
    BCD(int4 v) { ref_ = BCDRef::_alloc(); ref_->v_ = BCDFloat(v); }
    BCD(const BCD& v) { ref_ = 0; *this = v; }
    BCD(const BCDFloat& v) { ref_ = BCDRef::_alloc(); ref_->v_ = v; }
    BCD(double v) { ref_ = BCDRef::_alloc(); ref_->v_ = BCDFloat(v); }
    BCD(const int8& v) { ref_ = BCDRef::_alloc(); ref_->v_ = BCDFloat(v); }

    BCD&                operator=(const BCD& v)
    {
        _purge();
        if (v.ref_) {
            ref_ = BCDRef::_alloc();
            ref_->v_ = v.ref_->v_;
        }
        return *this;
    }

    int16                 exponent() const { return ref_->v_.exp(); }
    void                setExponent(int16 v) { ref_->v_.exp(v); }
    int16                 digit(int16 n) const { return ref_->v_.d_[n]; }

    // Arithmetic
    friend BCD          operator+(const BCD& a, const BCD& b)
    {
        BCDRef* ref = BCDRef::_alloc();
        BCDFloat::add(&a.ref_->v_, &b.ref_->v_, &ref->v_);
        return ref;
    }
    friend BCD          operator-(const BCD& a, const BCD& b)
    {
        BCDRef* ref = BCDRef::_alloc();
        BCDFloat::sub(&a.ref_->v_, &b.ref_->v_, &ref->v_);
        return ref;
    }
    friend BCD          operator*(const BCD& a, const BCD& b)
    {
        BCDRef* ref = BCDRef::_alloc();
        BCDFloat::mul(&a.ref_->v_, &b.ref_->v_, &ref->v_);
        return ref;
    }
    friend BCD          operator/(const BCD& a, const BCD& b)
    {
        BCDRef* ref = BCDRef::_alloc();
        BCDFloat::div(&a.ref_->v_, &b.ref_->v_, &ref->v_);
        return ref;
    }
    void                operator+=(const BCD& b)
    {
        BCDFloat c;
        BCDFloat::add(&ref_->v_, &b.ref_->v_, &c);
        ref_->v_ = c;
    }
    void                operator-=(const BCD& b)
    {
        BCDFloat c;
        BCDFloat::sub(&ref_->v_, &b.ref_->v_, &c);
        ref_->v_ = c;
    }
    void                operator*=(const BCD& b)
    {
        BCDFloat c;
        BCDFloat::mul(&ref_->v_, &b.ref_->v_, &c);
        ref_->v_ = c;
    }
    void                operator/=(const BCD& b)
    {
        BCDFloat c;
        BCDFloat::div(&ref_->v_, &b.ref_->v_, &c);
        ref_->v_ = c;
    }
    BCD                 operator-() const
    {
        BCDRef* t = BCDRef::_alloc();
        t->v_ = ref_->v_;
        t->v_.negate();
        return t;
    }
    void                operator++() 
    {
        *this += 1;
    }
    void                operator--()
    {
        *this -= 1;
    }

    friend int4         ifloor(const BCD& a)
                        { return BCDFloat::ifloor(&a.ref_->v_); }
    friend BCD          floor(const BCD&) BCD_SECT;
    friend BCD          trunc(const BCD&) BCD_SECT;
    friend BCD          fabs(const BCD&) BCD_SECT;
    friend BCD          frac(const BCD&) BCD_SECT;
#ifndef PALMOS
    const char*         asString() const;
#endif
    bool                isZero() const { return !ref_ || ref_->v_.isZero(); }
    bool                isNeg() const { return ref_ && ref_->v_.neg(); }
    bool                isSpecial() const
                                { return ref_ && ref_->v_.isSpecial(); }
    bool                isInf() const
                                { return ref_ && ref_->v_.isInf(); }
    bool                isNan() const
                                { return ref_ && ref_->v_.isNan(); }
    bool                isInteger() const
                                { return !ref_ || ref_->v_.isInteger(); }

    // Comparision
    friend bool         operator==(const BCD& a, const BCD& b)
                        { return BCDFloat::equal(&a.ref_->v_, &b.ref_->v_); }
    friend bool         operator<(const BCD& a, const BCD& b)
                        { return BCDFloat::lt(&a.ref_->v_, &b.ref_->v_); }
    friend bool         operator<=(const BCD& a, const BCD& b)
                        { return BCDFloat::le(&a.ref_->v_, &b.ref_->v_); }
    friend bool         operator>(const BCD& a, const BCD& b)
                        { return BCDFloat::gt(&a.ref_->v_, &b.ref_->v_); }
    friend bool         operator>=(const BCD& a, const BCD& b)
                        { return BCDFloat::ge(&a.ref_->v_, &b.ref_->v_); }

    // Destructor
    ~BCD() { _purge(); }

    void _purge() { if (ref_) { ref_->_free(); ref_ = 0; } }

    BCDRef*             ref_;
    static char         buf_[64];
};

BCD sqrt(const BCD&) BCD_SECT;

#endif 
