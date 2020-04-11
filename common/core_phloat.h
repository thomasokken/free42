/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2020  Thomas Okken
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see http://www.gnu.org/licenses/.
 *****************************************************************************/

#ifndef CORE_PHLOAT_H
#define CORE_PHLOAT_H 1


#include "free42.h"
#ifdef BCD_MATH
#include "bid_conf.h"
#include "bid_functions.h"
#endif

// A little hack to allow storing 6-character strings in a phloat
struct hp_string {
    char text[6];
    unsigned char length;
};
#define phloat_text(x) (((hp_string *) &(x))->text)
#define phloat_length(x) (((hp_string *) &(x))->length)

#ifdef BCD_MATH
#define MAX_MANT_DIGITS 34
#else
#define MAX_MANT_DIGITS 16
#endif


#ifndef BCD_MATH


#define phloat double

#define p_isinf(x) (isinf(x) ? (x) > 0 ? 1 : -1 : 0)
#define p_isnan isnan
#define p_sincos(x, s, c) { *(s) = sin(x); *(c) = cos(x); }
#define to_digit(x) ((int) fmod((x), 10.0))
#define to_char(x) ((char) (x))
#define to_int(x) ((int) (x))
#define to_int4(x) ((int4) (x))
#define to_int8(x) ((int8) (x))
#define to_uint8(x) ((uint8) (x))
#define to_double(x) ((double) (x))

#define PI 3.1415926535897932384626433
#define P 7

double decimal2double(void *data, bool pin_magnitude = false);


#else // BCD_MATH


#define phloat Phloat

class Phloat {
    public:
        BID_UINT128 val;

        Phloat() {}
        Phloat(const BID_UINT128 &b) : val(b) {}
        Phloat(const char *str);
        Phloat(int numer, int denom);
        Phloat(int8 numer, int8 denom);
        Phloat(int i);
        Phloat(int8 i);
        Phloat(uint8 i);
        Phloat(double d);
        Phloat(const Phloat &p);
        Phloat operator=(const BID_UINT128 &b) { val = b; return *this; }
        Phloat operator=(int i);
        Phloat operator=(int8 i);
        Phloat operator=(uint8 i);
        Phloat operator=(double d);
        Phloat operator=(Phloat p);
        bool operator==(Phloat p) const;
        bool operator!=(Phloat p) const;
        bool operator<(Phloat p) const;
        bool operator<=(Phloat p) const;
        bool operator>(Phloat p) const;
        bool operator>=(Phloat p) const;
        Phloat operator-() const;
        Phloat operator*(Phloat p) const;
        Phloat operator/(Phloat p) const;
        Phloat operator+(Phloat p) const;
        Phloat operator-(Phloat p) const;
        Phloat operator*=(Phloat p);
        Phloat operator/=(Phloat p);
        Phloat operator+=(Phloat p);
        Phloat operator-=(Phloat p);
        Phloat operator++(); // prefix
        Phloat operator++(int); // postfix
        Phloat operator--(); // prefix
        Phloat operator--(int); // postfix
};

// I can't simply overload isinf() and isnan(), because the Linux math.h
// defines them as macros.
int p_isinf(Phloat p);
int p_isnan(Phloat p);

// We don't define type cast operators, because they just lead
// to tons of ambiguities. Defining explicit conversions instead.
// Note that these conversion routines assume that the value to be
// converted actually fits in the returned type; if not, the result
// is undefined, except for to_char(), which will handle the range
// -128..255 correctly.
int to_digit(Phloat p); // Returns digit in units position
char to_char(Phloat p);
int to_int(Phloat p);
int4 to_int4(Phloat p);
int8 to_int8(Phloat p);
uint8 to_uint8(Phloat p);
double to_double(Phloat p);

Phloat sin(Phloat p);
Phloat cos(Phloat p);
Phloat tan(Phloat p);
Phloat asin(Phloat p);
Phloat acos(Phloat p);
Phloat atan(Phloat p);
void p_sincos(Phloat phi, Phloat *s, Phloat *c);
Phloat hypot(Phloat x, Phloat y);
Phloat atan2(Phloat x, Phloat y);
Phloat sinh(Phloat p);
Phloat cosh(Phloat p);
Phloat tanh(Phloat p);
Phloat asinh(Phloat p);
Phloat acosh(Phloat p);
Phloat atanh(Phloat p);
Phloat log(Phloat p);
Phloat log1p(Phloat p);
Phloat log10(Phloat p);
Phloat exp(Phloat p);
Phloat expm1(Phloat p);
Phloat tgamma(Phloat p);
Phloat sqrt(Phloat p);
Phloat fmod(Phloat x, Phloat y);
Phloat fabs(Phloat p);
Phloat pow(Phloat x, Phloat y);
Phloat floor(Phloat x);

Phloat operator*(int x, Phloat y);
Phloat operator/(int x, Phloat y);
Phloat operator/(double x, Phloat y);
Phloat operator+(int x, Phloat y);
Phloat operator-(int x, Phloat y);
bool operator==(int4 x, Phloat y);

extern Phloat PI;

void update_decimal(BID_UINT128 *val);


#endif // BCD_MATH


extern phloat POS_HUGE_PHLOAT;
extern phloat NEG_HUGE_PHLOAT;
extern phloat POS_TINY_PHLOAT;
extern phloat NEG_TINY_PHLOAT;
extern phloat NAN_PHLOAT;

void phloat_init();
int phloat2string(phloat d, char *buf, int buflen,
                  int base_mode, int digits, int dispmode,
                  int thousandssep, int max_mant_digits = 12);
int string2phloat(const char *buf, int buflen, phloat *d);


#endif
