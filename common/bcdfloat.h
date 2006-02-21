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

#ifndef __bcdfloat_h__
#define __bcdfloat_h__


#ifdef PALMOS
#define int4 Int32
#define uint4 UInt32
#else
#define int4 int
#define uint4 unsigned int
#endif

#if defined(WINDOWS) && !defined(__GNUC__)
#define int8 __int64
#define uint8 unsigned __int64
#define LL(x) x
#else
#define int8 long long
#define uint8 unsigned long long
#define LL(x) x##LL
#endif


#define NEG 0x8000
#define P 7
#define BASE 10000
#define EXPLIMIT (BASE/4)

/**
 * Here is the BCD float class.
 *
 * the BCD float is made of `P' 4dec digits. a 4dec digit is a chunk
 * of 4 decimal digits stored a a machine `short'. rather than use
 * a nibble per digit and base 10, we are using base 10,000 and thus
 * work with 4 decimal digits in one go.
 *
 * this is much more efficient because, not only is it faster, but
 * we are able to leverage the 16*16->32 and 32/16->16 multiply and
 * divide integer operations found on most modern processors as well
 * as 16 bit add & subtract.
 *
 * the exponent is stored in the last slot (ie P+1) and ranges from 
 * -9999 to +9999. the mantissa is stored unsigned and the sign is the
 * top bit of the exponent.
 *
 * there are a number of important considerations for this representation:
 *
 * the postion of the radix must always correspond with the end of a 4dec
 * word. therefore up to one extra 4dec is wasted in the representation.
 * for example, the number 12345.67 will store as [0001][2345].[6700] and
 * utilise 3 4decs rather than two. this is the drawback of using 4decs.
 * bytes would waste up to 1 nibble and nibbles dont waste anything which
 * is why they are normally chosen for short digit (eg 6) representations.
 * for long digit (eg 20) this method is preferable.
 *
 * whence we take P=6 which always has the capacity for 21 decimal digits
 * of mantissa (and up to 24 when it gets lucky) and at P=7 we have
 * 1 spare 4dec word.
 *
 * the trick of storing the mantissa as the P+1'th slot is cunning.
 * most of the arithmetic proceeds by extracting the sign and exponent
 * before working with the mantissa. thus for the purposes of the internal
 * calculations, we can let the mantissa spill out into the last `digit'
 * and effectively have one extra digit of working precision. this is
 * especially usefull for making sure we get the correct rounding at the
 * end when we put back the exponent. and also for accommodating any overall
 * numeric carry overflow before the number is shifted down.
 *
 * it has been suggested that, rather than use an extra 16 bit word, 5 words
 * are used for 20 digits without waste. this is possible but it involves
 * multiplying all 5 terms by 10, 100 or 1000 with carry, in order to align
 * the radix for operation. i didnt want this overhead, but it is true that
 * it would result in smaller representation. but dont forget that the
 * overall representation in size will be rounded up to a multiple of 4 bytes
 * anyway.
 *
 * lastly, floating point code is notorious for hiding bugs, often for years.
 * this code is new and there will be some "simple" cases that come out
 * plain wrong. thats the way with floating point. even CPU makers get it
 * wrong sometimes.
 */

struct BCDFloat
{
    BCDFloat() { _init(); }
    BCDFloat(const char* s);
    BCDFloat(int4);
    BCDFloat(int8);

    // Features
    void                asString(char* buf) const;
    int                 exp() const { return ((short)(d_[P] << 1)) >> 1; }
    void                exp(int v) { d_[P] = (v & (NEG-1)); }
    bool                neg() const { return d_[0] != 0 && (d_[P]& NEG) != 0; }
    void                negate() { d_[P] = d_[P] ^ NEG; }
    bool                isZero() const { return d_[0] == 0; }
    bool                isSpecial() const { return (d_[P]&0x7000) == 0x3000; } 
    bool                isNan() const { return (d_[P]&0x7fff) == 0x3000; }
    bool                isInf() const { return (d_[P]&0x7fff) == 0x3fff; }
    bool                isInteger() const;

    static void         add(const BCDFloat* a, const BCDFloat* b, BCDFloat* c);
    static void         sub(const BCDFloat* a, const BCDFloat* b, BCDFloat* c);
    static void         mul(const BCDFloat* a, const BCDFloat* b, BCDFloat* c);
    static void         div(const BCDFloat* a, const BCDFloat* b, BCDFloat* c);
    static bool         sqrt(const BCDFloat* a, BCDFloat* ra);

    static bool         lt(const BCDFloat* a, const BCDFloat* b);
    static bool         le(const BCDFloat* a, const BCDFloat* b);
    static bool         gt(const BCDFloat* a, const BCDFloat* b)
                                { return lt(b, a); }
    static bool         ge(const BCDFloat* a, const BCDFloat* b)
                                { return le(b, a); }
    static bool         equal(const BCDFloat* a, const BCDFloat* b);
    static int4         ifloor(const BCDFloat* a);
    static bool         floor(const BCDFloat* a, BCDFloat* c);
    static bool         trunc(const BCDFloat* a, BCDFloat* c);

    void                _init();
    static void         _uadd(const BCDFloat* a, const BCDFloat* b,
                              BCDFloat* c);
    static void         _usub(const BCDFloat* a, const BCDFloat* b,
                              BCDFloat* c);
    int                 _round();
    void                _rshift();
    void                _lshift();
    const BCDFloat&     _round20() const;
    void                _asString(char* buf) const;

    static const BCDFloat& posInf() { return *(BCDFloat*)posInfD_; }
    static const BCDFloat& negInf() { return *(BCDFloat*)negInfD_; }
    static const BCDFloat& nan() { return *(BCDFloat*)nanD_; }

    static void         mul2(unsigned short* ad, int ea,
                             unsigned short* bd, int eb,
                             unsigned short* cd, int& ec);
    
    /* store P 4dec `digits', equivalent to P*4 decimal digits.
     * the last place is the exponent.
     */
    unsigned short      d_[P+1];  

    static BCDFloat       roundedVal_;
    static BCDFloat       rounding_;
    static unsigned short posInfD_[P+1];
    static unsigned short negInfD_[P+1];
    static unsigned short nanD_[P+1];
    static int            decade_[4];
};

inline void BCDFloat::_rshift()
{
    int i;
    for (i = P; i > 0; --i) d_[i] = d_[i-1];
}

inline void BCDFloat::_lshift()
{
    int i;
    for (i = 0; i < P; ++i) d_[i] = d_[i+1]; 
}

#endif 


