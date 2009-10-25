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

#ifndef __bcdfloat_h__
#define __bcdfloat_h__

#include "free42.h"


// Section attribute -- something like
// #define BCD1_SECT __attribute__ ((section ("BcdFlt1")))
// Only needed when building a multi-segment PalmOS executable.
#ifndef BCD1_SECT
#define BCD1_SECT
#endif


#define NEG 0x8000
#define P 7
#define BASE 10000
#define EXPLIMIT (BASE/4)
#define EXPMASK  0x1fff
#define EXPMASKNEG (NEG|EXPMASK)

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
 * -10000 to +9999. the mantissa is stored unsigned and the sign is the
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
 * especially useful for making sure we get the correct rounding at the
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

#ifdef CUSTOM_EXP_CHAR
#define ISEXP(_c) \
((_c) == 'e' || (_c) == 'E' || (_c) == CUSTOM_EXP_CHAR)
#else
#define ISEXP(_c) \
    ((_c) == 'e' || (_c) == 'E')
#endif


struct BCDFloatData
{
    /* store P 4dec `digits', equivalent to P*4 decimal digits.
     * the last place is the exponent.
     */
    unsigned short      d_[P+1];  
};

#define MAX_P        2*(P+1)
#define POS_INF_EXP  0x2000
#define NEG_INF_EXP  0xA000
#define NAN_EXP      0x4000
#define GET_EXP(_d, _p) (((short)((_d)[_p] << 3)) >> 3)
#define SET_EXP(_d, _p, _v) ((_d)[_p] = (_v) & EXPMASK)
#define CLEAR_SIGN(_d, _p) ((_d)[_p] &= ~NEG)
#define NEGATE_SIGN(_d, _p) ((_d)[_p] ^= NEG)
#define GET_SPECIAL(_d, _p) ((_d)[_p]&0x6000)
#define GET_NAN(_d, _p) ((_d)[_p]&NAN_EXP)
#define GET_INF(_d, _p) ((_d)[_p]&0x2000)
#define GET_NEG_BIT(_d, _p) ((_d)[_p]&NEG)

// zero assuming normal (non-special)
#define GET_ZERO_NORM(_d, _p) ((_d)[0] == 0)

// negative assuming non-special
#define GET_NEG_NORM(_d, _p) (GET_NEG_BIT(_d, _p) && !GET_ZERO_NORM(_d,_p))

int bcd_round(unsigned short* d, int pn) BCD1_SECT;
int bcd_round25(unsigned short* d, int pn) BCD1_SECT;
void bcd_uadd(const unsigned short* a,
              const unsigned short* b,
              unsigned short* c,
              int pn) BCD1_SECT;
void bcd_usub(const unsigned short* a,
              const unsigned short* b,
              unsigned short* c,
              int pn) BCD1_SECT;
void bcd_add(const unsigned short* a,
             const unsigned short* b,
             unsigned short* c,
             int pn) BCD1_SECT;
void bcd_sub(const unsigned short* a,
             const unsigned short* b,
             unsigned short* c,
             int pn) BCD1_SECT;
void bcd_mul(const unsigned short* a,
             const unsigned short* b,
             unsigned short* c,
             int pn) BCD1_SECT;
void bcd_div(const unsigned short* a,
             const unsigned short* b,
             unsigned short* c,
             int pn) BCD1_SECT;
int bcd_cmp(const unsigned short* a, 
            const unsigned short* b,
            int pn) BCD1_SECT;
void bcd_fromUInt(unsigned short* d, int pn, uint4 v) BCD1_SECT;
extern int BCDDecade[4];

struct BCDFloat: public BCDFloatData
{
    enum Format
    {
        format_normal = 0,
        format_scimode = 1,
        format_truncate = 2,  // no rounding
    };

    BCDFloat(int d0, int d1, int d2, int d3, int d4, int d5, int d6, int d7)
    {
	d_[0] = d0; d_[1] = d1; d_[2] = d2; d_[3] = d3;
	d_[4] = d4; d_[5] = d5; d_[6] = d6; d_[7] = d7;
    }
    BCDFloat() {} // Warning: not initialised.
    BCDFloat(const char* s) BCD1_SECT;
    BCDFloat(int4 v)
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
    BCDFloat(int v)
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
    BCDFloat(int8) BCD1_SECT;
    BCDFloat(double) BCD1_SECT;
    BCDFloat(uint4 v)
    {
        _init();
        if (v) _fromUInt(v);
    }

    BCDFloat(const BCDFloatData& d) { *this = *(BCDFloat*)&d; }

    // Features
#ifndef PALMOS
    void                asString(char* buf) const
    { _asString(buf, format_normal, 0); }

    void                asStringFmt(char* buf,
                                    Format fmt,
                                    int precision) const
    {
        _asString(buf, fmt, precision); 
    }
#endif 
    int                 exp() const { return GET_EXP(d_, P); }
    void                exp(int v) { SET_EXP(d_, P, v); }
    bool                neg() const
    { return (d_[P]& NEG) && (d_[0] != 0 || isInf()); }
            
    void                setSign() { d_[P] |= NEG; }
    void                clearSign() { CLEAR_SIGN(d_, P); }
    void                negate() { NEGATE_SIGN(d_, P); }
    bool                isSpecial() const { return GET_SPECIAL(d_,P) != 0; } 

    bool                isZero() const
                        { return d_[0] == 0 && !isSpecial(); }

    bool                isNan() const { return GET_NAN(d_,P) != 0; }
    bool                isInf() const { return GET_INF(d_,P) != 0; }
    bool                isInteger() const BCD1_SECT;

    void                ldexp(unsigned int mant, int e)
    {
        // load the exp as a 4-block
        _init();
        _fromUInt(mant);
        exp(e);
    }

    static void         add(const BCDFloat* a, const BCDFloat* b, BCDFloat* c)
    {
        bcd_add(a->d_, b->d_, c->d_, P);
    }

    static void         sub(const BCDFloat* a, const BCDFloat* b, BCDFloat* c)
    {
        bcd_sub(a->d_, b->d_, c->d_, P);
    }

    static void         mul(const BCDFloat* a, const BCDFloat* b, BCDFloat* c)
    {
        bcd_mul(a->d_, b->d_, c->d_, P);
    }

    static void         div(const BCDFloat* a, const BCDFloat* b, BCDFloat* c)
    {
        bcd_div(a->d_, b->d_, c->d_, P);
    }
    
    static bool         sqrt(const BCDFloat* a, BCDFloat* ra) BCD1_SECT;

    static bool         lt(const BCDFloat* a, const BCDFloat* b)
    {
        /* true iff a < b */
        return bcd_cmp(a->d_, b->d_, P) < 0;
    }
    static bool         le(const BCDFloat* a, const BCDFloat* b)
    {
        /* true iff a <= b */
        return bcd_cmp(a->d_, b->d_, P) <= 0;
    }
    static bool         gt(const BCDFloat* a, const BCDFloat* b)
    {
        /* true iff a > b */
        return bcd_cmp(a->d_, b->d_, P) > 0;
    }

    static bool         ge(const BCDFloat* a, const BCDFloat* b)
    {
        /* true iff a >= b */
        return bcd_cmp(a->d_, b->d_, P) >= 0;
    }

    static bool         equal(const BCDFloat* a, const BCDFloat* b)
    {
        return bcd_cmp(a->d_, b->d_, P) == 0;
    }

    static int4         ifloor(const BCDFloat* x) 
    {
        BCDFloat a;
        floor(x, &a);
        return a.asInt();
    }

    static int4         itrunc(const BCDFloat* x) 
    {
        BCDFloat a;
        trunc(x, &a);
        return a.asInt();
    }
    static bool         floor(const BCDFloat* a, BCDFloat* c) BCD1_SECT;
    static bool         trunc(const BCDFloat* a, BCDFloat* c) BCD1_SECT;

    void                _init() BCD1_SECT;
    static void         _uadd(const BCDFloat* a,
                              const BCDFloat* b,
                              BCDFloat* c)
    {
        bcd_uadd(a->d_, b->d_, c->d_, P);
    }

    int                 _round25() { return bcd_round25(d_, P); }
#ifndef PALMOS
    void                _asString(char* buf, Format fmt, int precision) const;
#endif
    void                _fromUInt(uint4 v) { bcd_fromUInt(d_, P, v); }
    void                _roundDigits(unsigned int precision, BCDFloat* v) const BCD1_SECT;
    static const BCDFloat& posInf() { return *(BCDFloat*)posInfD_; }
    static const BCDFloat& negInf() { return *(BCDFloat*)negInfD_; }
    static const BCDFloat& nan() { return *(BCDFloat*)nanD_; }

    static void         epsilon(int n, BCDFloat* v)
    {
        // generate 10^-n, 
        int m = BCDDecade[(n-1) & 3];
        v->ldexp(m, -(n>>2));
    }

    static void         mul2(const unsigned short* ad, int ea,
                             const unsigned short* bd, int eb,
                             unsigned short* cd, int& ec) BCD1_SECT;
    int4                asInt() const BCD1_SECT;
    

    static unsigned short posInfD_[P+1];
    static unsigned short negInfD_[P+1];
    static unsigned short nanD_[P+1];
};

#endif 
