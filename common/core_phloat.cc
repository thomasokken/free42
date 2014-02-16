/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2013  Thomas Okken
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#include "core_phloat.h"
#include "core_helpers.h"
#include "shell.h"
#include "bcd.h"
#include "bcdmath.h"


phloat POS_HUGE_PHLOAT;
phloat NEG_HUGE_PHLOAT;
phloat POS_TINY_PHLOAT;
phloat NEG_TINY_PHLOAT;


#ifdef BCD_MATH


void phloat_init() {
    POS_HUGE_PHLOAT.bcd.d_[0] = 9999;
    POS_HUGE_PHLOAT.bcd.d_[1] = 9999;
    POS_HUGE_PHLOAT.bcd.d_[2] = 9999;
    POS_HUGE_PHLOAT.bcd.d_[3] = 9999;
    POS_HUGE_PHLOAT.bcd.d_[4] = 9999;
    POS_HUGE_PHLOAT.bcd.d_[5] = 9999;
    POS_HUGE_PHLOAT.bcd.d_[6] = 9000;
    POS_HUGE_PHLOAT.bcd.d_[7] = 2500;
    NEG_HUGE_PHLOAT = -POS_HUGE_PHLOAT;
    POS_TINY_PHLOAT.bcd.d_[0] = 1;
    POS_TINY_PHLOAT.bcd.d_[1] = 0;
    POS_TINY_PHLOAT.bcd.d_[2] = 0;
    POS_TINY_PHLOAT.bcd.d_[3] = 0;
    POS_TINY_PHLOAT.bcd.d_[4] = 0;
    POS_TINY_PHLOAT.bcd.d_[5] = 0;
    POS_TINY_PHLOAT.bcd.d_[6] = 0;
    POS_TINY_PHLOAT.bcd.d_[7] = (unsigned short) -2499;
    NEG_TINY_PHLOAT = -POS_TINY_PHLOAT;
}

int string2phloat(const char *buf, int buflen, phloat *d) {
    /* Convert string to phloat.
     * Return values:
     * 0: no error
     * 1: positive overflow
     * 2: negative overflow
     * 3: positive underflow
     * 4: negative underflow
     * 5: other error
     */
    // First, convert from HP-42S format to bcdfloat format:
    // strip thousands separators, convert comma to dot if
    // appropriate, and convert char(24) to 'E'.
    // Also, reject numbers with more than 12 digits in the mantissa.
    char buf2[100];
    int buflen2 = 0;
    char sep = flags.f.decimal_point ? ',' : '.';
    int mantdigits = 0;
    bool in_mant = true;
    bool zero = true;
    for (int i = 0; i < buflen; i++) {
	char c = buf[i];
	if (c == sep)
	    continue;
	if (c == ',')
	    c = '.';
	else if (c == 24) {
	    c = 'E';
	    in_mant = false;
	} else if (in_mant && c >= '0' && c <= '9') {
	    if (++mantdigits > 12)
		return 5;
	    if (c != '0')
		zero = false;
	}
	buf2[buflen2++] = c;
    }

    if (!in_mant && mantdigits == 0) {
	// A number like "E4"; not something the HP-41 or HP42S (or Free42, for
	// that matter) will actually allow you to enter into a program, but
	// the real calcs do accept this kind of thing (synthetically), and
	// supply a mantissa of 1 (just like when you start number entry by
	// pressing EEX (HP-41) or E (HP-42S).
	for (int i = buflen2 - 1; i >= 0; i--)
	    buf2[i + 1] = buf2[i];
	buf2[0] = '1';
	buflen2++;
    }

    buf2[buflen2] = 0;
    BCDFloat b(buf2);
    if (b.isInf())
	return b.neg() ? 2 : 1;
    if (!zero && b.isZero())
	return b.neg() ? 4 : 3;
    d->bcd = b;
    return 0;
}

/* public */
Phloat::Phloat(int numer, int denom) {
    BCDFloat n(numer);
    BCDFloat d(denom);
    BCDFloat::div(&n, &d, &bcd);
}

/* public */
Phloat::Phloat(int i) {
    bcd = BCDFloat(i);
}

/* public */
Phloat::Phloat(int8 i) : bcd(i) {
    // Nothing else to do
}

/* public */
Phloat::Phloat(double d) {
    bcd = double2bcd(d);
}

/* public */
Phloat::Phloat(const Phloat &p) {
    bcd = p.bcd;
}

/* public */
Phloat Phloat::operator=(int i) {
    bcd = BCDFloat(i);
    return *this;
}

/* public */
Phloat Phloat::operator=(int8 i) {
    bcd = BCDFloat(i);
    return *this;
}

/* public */
Phloat Phloat::operator=(double d) {
    bcd = double2bcd(d);
    return *this;
}

/* public */
Phloat Phloat::operator=(Phloat p) {
    bcd = p.bcd;
    return *this;
}

/* public */
bool Phloat::operator==(Phloat p) const {
    return BCDFloat::equal(&bcd, &p.bcd);
}

/* public */
bool Phloat::operator!=(Phloat p) const {
    return !BCDFloat::equal(&bcd, &p.bcd);
}

/* public */
bool Phloat::operator<(Phloat p) const {
    return BCDFloat::lt(&bcd, &p.bcd);
}

/* public */
bool Phloat::operator<=(Phloat p) const {
    return BCDFloat::le(&bcd, &p.bcd);
}

/* public */
bool Phloat::operator>(Phloat p) const {
    return BCDFloat::gt(&bcd, &p.bcd);
}

/* public */
bool Phloat::operator>=(Phloat p) const {
    return BCDFloat::ge(&bcd, &p.bcd);
}

/* public */
Phloat Phloat::operator-() const {
    Phloat res(*this);
    res.bcd.negate();
    return res;
}

/* public */
Phloat Phloat::operator*(Phloat p) const {
    Phloat res;
    BCDFloat::mul(&bcd, &p.bcd, &res.bcd);
    return res;
}

/* public */
Phloat Phloat::operator/(Phloat p) const {
    Phloat res;
    BCDFloat::div(&bcd, &p.bcd, &res.bcd);
    return res;
}

/* public */
Phloat Phloat::operator+(Phloat p) const {
    Phloat res;
    BCDFloat::add(&bcd, &p.bcd, &res.bcd);
    return res;
}

/* public */
Phloat Phloat::operator-(Phloat p) const {
    Phloat res;
    BCDFloat::sub(&bcd, &p.bcd, &res.bcd);
    return res;
}

/* public */
Phloat Phloat::operator*=(Phloat p) {
    BCDFloat temp;
    BCDFloat::mul(&bcd, &p.bcd, &temp);
    bcd = temp;
    return *this;
}

/* public */
Phloat Phloat::operator/=(Phloat p) {
    BCDFloat temp;
    BCDFloat::div(&bcd, &p.bcd, &temp);
    bcd = temp;
    return *this;
}

/* public */
Phloat Phloat::operator+=(Phloat p) {
    BCDFloat temp;
    BCDFloat::add(&bcd, &p.bcd, &temp);
    bcd = temp;
    return *this;
}

/* public */
Phloat Phloat::operator-=(Phloat p) {
    BCDFloat temp;
    BCDFloat::sub(&bcd, &p.bcd, &temp);
    bcd = temp;
    return *this;
}

/* public */
Phloat Phloat::operator++() {
    // prefix
    const BCDFloat one(1);
    BCDFloat temp;
    BCDFloat::add(&bcd, &one, &temp);
    bcd = temp;
    return *this;
}

/* public */
Phloat Phloat::operator++(int) {
    // postfix
    Phloat old = *this;
    const BCDFloat one(1);
    BCDFloat temp;
    BCDFloat::add(&bcd, &one, &temp);
    bcd = temp;
    return old;
}

/* public */
Phloat Phloat::operator--() {
    // prefix
    const BCDFloat one(1);
    BCDFloat temp;
    BCDFloat::sub(&bcd, &one, &temp);
    bcd = temp;
    return *this;
}

/* public */
Phloat Phloat::operator--(int) {
    // postfix
    Phloat old = *this;
    const BCDFloat one(1);
    BCDFloat temp;
    BCDFloat::sub(&bcd, &one, &temp);
    bcd = temp;
    return old;
}

int p_isinf(Phloat p) {
    if (p.bcd.isInf())
	return p.bcd.neg() ? -1 : 1;
    else
	return 0;
}

int p_isnan(Phloat p) {
    return p.bcd.isNan() ? 1 : 0;
}

int to_digit(Phloat p) {
    BCD res = trunc(fmod(BCD(p.bcd), 10));
    return ifloor(res);
}

static int8 mant(const BCDFloat &b) {
    int8 m = 0;
    int e = b.exp();
    if (e > P)
	e = P;
    for (int i = 0; i < e; i++)
	m = m * 10000 + b.d_[i];
    if (b.neg())
	m = -m;
    return m;
}

char to_char(Phloat p) {
    return (char) mant(p.bcd);
}

int to_int(Phloat p) {
    return (int) mant(p.bcd);
}

int4 to_int4(Phloat p) {
    return (int4) mant(p.bcd);
}

int8 to_int8(Phloat p) {
    return (int8) mant(p.bcd);
}

double to_double(Phloat p) {
    return bcd2double(p.bcd, false);
}

Phloat sin(Phloat p) {
    Phloat res;
    res.bcd = sin(BCD(p.bcd))._v;
    return res;
}

Phloat cos(Phloat p) {
    Phloat res;
    res.bcd = cos(BCD(p.bcd))._v;
    return res;
}

Phloat tan(Phloat p) {
    Phloat res;
    res.bcd = tan(BCD(p.bcd))._v;
    return res;
}

Phloat asin(Phloat p) {
    Phloat res;
    res.bcd = asin(BCD(p.bcd))._v;
    return res;
}

Phloat acos(Phloat p) {
    Phloat res;
    res.bcd = acos(BCD(p.bcd))._v;
    return res;
}

Phloat atan(Phloat p) {
    Phloat res;
    res.bcd = atan(BCD(p.bcd))._v;
    return res;
}

void sincos(Phloat phi, Phloat *s, Phloat *c) {
    BCD p(phi.bcd);
    s->bcd = sin(p)._v;
    c->bcd = cos(p)._v;
}

Phloat hypot(Phloat x, Phloat y) {
    Phloat res;
    res.bcd = hypot(BCD(x.bcd), BCD(y.bcd))._v;
    return res;
}

Phloat atan2(Phloat x, Phloat y) {
    Phloat res;
    res.bcd = atan2(BCD(x.bcd), BCD(y.bcd))._v;
    return res;
}

Phloat sinh(Phloat p) {
    // (exp(x)-exp(-x))/2
    BCDFloat temp1 = exp(BCD(p.bcd))._v;
    BCDFloat temp2 = exp(BCD((-p).bcd))._v;
    BCDFloat temp3;
    BCDFloat::sub(&temp1, &temp2, &temp3);
    const BCDFloat two(2);
    Phloat res;
    BCDFloat::div(&temp3, &two, &res.bcd);
    return res;
}

Phloat cosh(Phloat p) {
    // (exp(x)+exp(-x))/2
    BCDFloat temp1 = exp(BCD(p.bcd))._v;
    BCDFloat temp2 = exp(BCD((-p).bcd))._v;
    BCDFloat temp3;
    BCDFloat::add(&temp1, &temp2, &temp3);
    const BCDFloat two(2);
    Phloat res;
    BCDFloat::div(&temp3, &two, &res.bcd);
    return res;
}

Phloat tanh(Phloat p) {
    // (exp(x)-exp(-x))/(exp(x)+exp(-x))
    BCDFloat temp1 = exp(BCD(p.bcd))._v;
    BCDFloat temp2 = exp(BCD((-p).bcd))._v;
    BCDFloat temp3;
    BCDFloat::sub(&temp1, &temp2, &temp3);
    if (temp3.isInf())
	return temp3.neg() ? -1 : 1;
    BCDFloat temp4;
    BCDFloat::add(&temp1, &temp2, &temp4);
    Phloat res;
    BCDFloat::div(&temp3, &temp4, &res.bcd);
    return res;
}

Phloat asinh(Phloat p) {
    // log(sqrt(x^2+1)+x)
    BCDFloat temp1;
    BCDFloat::mul(&p.bcd, &p.bcd, &temp1);
    BCDFloat temp2;
    const BCDFloat one(1);
    BCDFloat::add(&temp1, &one, &temp2);
    temp1 = sqrt(BCD(temp2))._v;
    BCDFloat::add(&temp1, &p.bcd, &temp2);
    Phloat res;
    res.bcd = log(BCD(temp2))._v;
    return res;
}

Phloat acosh(Phloat p) {
    // log(sqrt(x^2-1)+x)
    BCDFloat temp1;
    BCDFloat::mul(&p.bcd, &p.bcd, &temp1);
    BCDFloat temp2;
    const BCDFloat one(1);
    BCDFloat::sub(&temp1, &one, &temp2);
    temp1 = sqrt(BCD(temp2))._v;
    BCDFloat::add(&temp1, &p.bcd, &temp2);
    Phloat res;
    res.bcd = log(BCD(temp2))._v;
    return res;
}

Phloat atanh(Phloat p) {
    // log((1+x)/(1-x))/2
    const BCDFloat one(1);
    BCDFloat temp1, temp2, temp3;
    BCDFloat::add(&one, &p.bcd, &temp1);
    BCDFloat::sub(&one, &p.bcd, &temp2);
    BCDFloat::div(&temp1, &temp2, &temp3);
    temp1 = log(BCD(temp3))._v;
    Phloat res;
    const BCDFloat two(2);
    BCDFloat::div(&temp1, &two, &res.bcd);
    return res;
}

Phloat log(Phloat p) {
    Phloat res;
    res.bcd = log(BCD(p.bcd))._v;
    return res;
}

Phloat log1p(Phloat p) {
    Phloat res;
    res.bcd = ln1p(BCD(p.bcd))._v;
    return res;
}

Phloat log10(Phloat p) {
    Phloat res;
    res.bcd = log10(BCD(p.bcd))._v;
    return res;
}

Phloat exp(Phloat p) {
    Phloat res;
    res.bcd = exp(BCD(p.bcd))._v;
    return res;
}

Phloat expm1(Phloat p) {
    Phloat res;
    res.bcd = expm1(BCD(p.bcd))._v;
    return res;
}

Phloat gamma(Phloat p) {
    --p;
    Phloat res;
    res.bcd = gammaFactorial(BCD(p.bcd))._v;
    return res;
}

Phloat sqrt(Phloat p) {
    Phloat res;
    res.bcd = sqrt(BCD(p.bcd))._v;
    return res;
}

Phloat fmod(Phloat x, Phloat y) {
    Phloat res;
    res.bcd = fmod(BCD(x.bcd), BCD(y.bcd))._v;
    return res;
}

Phloat fabs(Phloat p) {
    Phloat res(p);
    if (!res.bcd.isNan() && res.bcd.neg())
	res.bcd.negate();
    return res;
}

Phloat pow(Phloat x, Phloat y) {
    Phloat res;
    if (!y.bcd.isSpecial()) {
	int iy = BCDFloat::ifloor(&y.bcd);
	BCDFloat by(iy);
	if (BCDFloat::equal(&y.bcd, &by)) {
	    res.bcd = pow(BCD(x.bcd), iy)._v;
	    return res;
	}
    }
    res.bcd = pow(BCD(x.bcd), BCD(y.bcd))._v;
    return res;
}

Phloat floor(Phloat p) {
    Phloat res;
    BCDFloat::floor(&p.bcd, &res.bcd);
    return res;
}

Phloat operator*(int x, Phloat y) {
    Phloat res;
    BCDFloat bx(x);
    BCDFloat::mul(&bx, &y.bcd, &res.bcd);
    return res;
}

Phloat operator/(int x, Phloat y) {
    Phloat res;
    BCDFloat bx(x);
    BCDFloat::div(&bx, &y.bcd, &res.bcd);
    return res;
}

Phloat operator/(double x, Phloat y) {
    Phloat res;
    BCDFloat bx = double2bcd(x);
    BCDFloat::div(&bx, &y.bcd, &res.bcd);
    return res;
}

Phloat operator+(int x, Phloat y) {
    Phloat res;
    BCDFloat bx(x);
    BCDFloat::add(&bx, &y.bcd, &res.bcd);
    return res;
}

Phloat operator-(int x, Phloat y) {
    Phloat res;
    BCDFloat bx(x);
    BCDFloat::sub(&bx, &y.bcd, &res.bcd);
    return res;
}

bool operator==(int4 x, Phloat y) {
    BCDFloat bx(x);
    return BCDFloat::equal(&bx, &y.bcd);
}

Phloat PI(BCDFloat(3, 1415, 9265, 3589, 7932, 3846, 2643, 1));

BCDFloat double2bcd(double d, bool round /* = false */) {
    BCDFloat res(d);
    if (round && !res.isSpecial()) {
	// This is used when converting programs from a Free42 Binary state
	// file. Number literals in programs are rounded to 12 digits, so
	// what you see really is what you get; without this hack, you'd
	// get stuff like 0.9 turning into 0.8999999+ but still *looking*
	// like 0.9!
	int i;
	for (i = 4; i < P; i++)
	    res.d_[i] = 0;
	unsigned short s = res.d_[0];
	unsigned short d;
	if (s < 10)
	    d = 10;
	else if (s < 100)
	    d = 100;
	else if (s < 1000)
	    d = 1000;
	else
	    d = 10000;
	s = res.d_[3];
	unsigned short r = s % d;
	if (r >= d >> 1)
	    s += d;
	s -= r;
	bool carry = s >= 10000;
	if (carry)
	    s -= 10000;
	res.d_[3] = s;
	for (i = 2; carry && i >= 0; i--) {
	    s = res.d_[i] + 1;
	    carry = s >= 10000;
	    if (carry)
		s -= 10000;
	    res.d_[i] = s;
	}
	if (carry) {
	    for (i = 3; i >= 0; i--)
		res.d_[i + 1] = res.d_[i];
	    res.d_[0] = 1;
	    res.d_[P]++;
	    // No need to check if the exponent is overflowing; the range
	    // of 'double' is too small to cause such problems here.
	}
    }
    return res;
}

double bcd2double(BCDFloat b, bool old_bcd) {
    if (old_bcd)
	bcdfloat_old2new(b.d_);

    double zero = 0;
    bool neg = b.neg();

#if defined(WINDOWS) && !defined(__GNUC__)
    // No support for NaN or infinities
    if (b.isNan())
	return HUGE_VAL;
    else if (b.isInf())
	return neg ? -HUGE_VAL : HUGE_VAL;
#else
    if (b.isNan())
	return 0 / zero;
    else if (b.isInf())
	return neg ? -1 / zero : 1 / zero;
#endif

    if (b.d_[0] == 0)
	return 0;

    short exp = (((short) b.d_[P]) << 3) >> 3;

    char decstr[35];
    char *cp = decstr;
    if (neg)
	*cp++ = '-';
    for (int i = 0; i < P; i++) {
	short d = b.d_[i];
	sprintf(cp, "%04d", d);
	if (i == 0) {
	    for (int j = 4; j >= 2; j--)
		cp[j] = cp[j - 1];
	    cp[1] = '.';
	    cp += 5;
	} else
	    cp += 4;
    }
    sprintf(cp, "e%d", exp * 4 - 1);
    double res;
    sscanf(decstr, "%le", &res);
    return res;
}


#else // BCD_MATH


void phloat_init() {
    POS_HUGE_PHLOAT = DBL_MAX;
    NEG_HUGE_PHLOAT = -POS_HUGE_PHLOAT;
#ifndef WINDOWS
    POS_TINY_PHLOAT = nextafter(0.0, 1.0);
#else
    double d = 1;
    while (1) {
	double d2 = d / 2;
	if (d2 == 0)
	    break;
	d = d2;
    }
    POS_TINY_PHLOAT = d;
#endif
    NEG_TINY_PHLOAT = -POS_TINY_PHLOAT;
}

int string2phloat(const char *buf, int buflen, phloat *d) {
    /* Convert string to phloat.
     * Return values:
     * 0: no error
     * 1: positive overflow
     * 2: negative overflow
     * 3: positive underflow
     * 4: negative underflow
     * 5: other error
     */
    char mantissa[16] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
    int mant_sign = 0;
    int skipping_zeroes = 1;
    int seen_dot = 0;
    int mant_pos = 0;
    int exp_offset = -1;
    int in_exp = 0;
    int exp = 0;
    int exp_sign = 0;
    int i, mant_digits = 0;
    char dot = flags.f.decimal_point ? '.' : ',';
    char sep = flags.f.decimal_point ? ',' : '.';
    int is_zero = 1;
    double res;

    for (i = 0; i < buflen; i++) {
	char c = buf[i];
	if (c == 24) {
	    in_exp = 1;
	    if (mant_digits == 0) {
		mant_digits++;
		mantissa[mant_pos++] = 1;
		is_zero = 0;
		exp_offset = 0;
	    }
	    continue;
	}
	if (in_exp) {
	    if (c == '-')
		exp_sign = 1;
	    else
		exp = exp * 10 + (c - '0');
	} else {
	    if (c == sep)
		continue;
	    if (c == dot) {
		seen_dot = 1;
		skipping_zeroes = 0;
		continue;
	    }
	    if (c == '-') {
		mant_sign = 1;
		continue;
	    }
	    /* Once we get here, c should be a digit */
	    if (++mant_digits > 12)
		/* Too many digits! We only allow the user to enter 12. */
		return 5;
	    if (c == '0' && skipping_zeroes)
		continue;
	    skipping_zeroes = 0;
	    mantissa[mant_pos++] = c - '0';
	    if (c != '0')
		is_zero = 0;
	    if (!seen_dot)
		exp_offset = mant_pos - 1;
	}
    }

    if (is_zero) {
	*d = 0;
	return 0;
    }

    if (exp_sign)
	exp = -exp;
    exp += exp_offset;

    /* Get rid of leading zeroes in mantissa
     * (the loop above removes redundant leading zeroes, e.g. the first two
     * of 0012.345, but in the case of 0.001, the two zeroes following the
     * decimal are not redundant, and they end up in the mantissa.
     */
    if (mantissa[0] == 0) {
	int leadingzeroes = 0;
	int i;
	while (mantissa[leadingzeroes] == 0)
	    leadingzeroes++;
	for (i = 0; i < mant_pos - leadingzeroes; i++)
	    mantissa[i] = mantissa[i + leadingzeroes];
	for (i = mant_pos - leadingzeroes; i < mant_pos; i++)
	    mantissa[i] = 0;
	exp -= leadingzeroes;
    }

    /* 'mantissa' now contains the normalized bcd mantissa;
     * 'exp' contains the normalized signed exponent,
     * and 'mant_sign' contains the mantissa's sign.
     */
    char decstr[35];
    char *cp = decstr;
    if (mant_sign)
	*cp++ = '-';
    for (i = 0; i < 16; i++) {
	*cp++ = mantissa[i] + '0';
	if (i == 0)
	    *cp++ = '.';
    }
    sprintf(cp, "e%d", exp);
    sscanf(decstr, "%le", &res);
    if (isinf(res))
	return mant_sign ? 2 : 1;
    if (res == 0.0)
	return mant_sign ? 4 : 3;
    *d = res;
    return 0;
}

double bcd2double(short *p, bool old_bcd) {
    if (old_bcd)
	bcdfloat_old2new(p);

    short exp = p[P];
    bool neg = (exp & 0x8000) != 0;
    double zero = 0;

#if defined(WINDOWS) && !defined(__GNUC__)
    if ((exp & 0x4000) != 0)
	return HUGE_VAL; // NaN
    else if ((exp & 0x2000) != 0)
	return neg ? -HUGE_VAL : HUGE_VAL; // -Inf or Inf
#else
    if ((exp & 0x4000) != 0)
	return 0 / zero; // NaN
    else if ((exp & 0x2000) != 0)
	return neg ? -1 / zero : 1 / zero; // -Inf or Inf
#endif

    if (p[0] == 0)
	return 0;

    exp = ((short) (exp << 3)) >> 3;

    char decstr[35];
    char *cp = decstr;
    if (neg)
	*cp++ = '-';
    for (int i = 0; i < P; i++) {
	short d = p[i];
	sprintf(cp, "%04d", d);
	if (i == 0) {
	    for (int j = 4; j >= 2; j--)
		cp[j] = cp[j - 1];
	    cp[1] = '.';
	    cp += 5;
	} else
	    cp += 4;
    }
    sprintf(cp, "e%d", exp * 4 - 1);
    double res;
    sscanf(decstr, "%le", &res);
    return res;
}


#endif // BCD_MATH


int phloat2string(phloat pd, char *buf, int buflen, int base_mode, int digits,
			 int dispmode, int thousandssep) {
    int chars_so_far = 0;

    if (p_isnan(pd)) {
	string2buf(buf, buflen, &chars_so_far, "<Not a Number>", 14);
	return chars_so_far;
    }

    if (p_isinf(pd)) {
	char2buf(buf, buflen, &chars_so_far, '<');
	if (pd < 0)
	    char2buf(buf, buflen, &chars_so_far, '-');
	string2buf(buf, buflen, &chars_so_far, "Infinity>", 9);
	return chars_so_far;
    }

    /* base_mode: 0=only decimal, 1=all bases, 2=decimal or binary (SHOW) */
    int base = get_base();
    if (base_mode == 1 && base != 10 || base_mode == 2 && base == 2) {
	int8 n;
	int inexact, shift;
	char binbuf[36];
	int binbufptr = 0;

	if (pd > 34359738367.0 || pd < -34359738368.0) {
	    if (base_mode == 2)
		goto decimal_after_all;
	    else {
		string2buf(buf, buflen, &chars_so_far, "<Too Big>", 9);
		return chars_so_far;
	    }
	}

	n = to_int8(pd);
	inexact = base_mode == 1 && pd != n;
	n &= LL(0xfffffffff);
	shift = base == 2 ? 1 : base == 8 ? 3 : 4;
	while (n != 0) {
	    int digit = (int) (n & (base - 1));
	    char c = digit < 10 ? '0' + digit : 'A' + digit - 10;
	    binbuf[binbufptr++] = c;
	    n >>= shift;
	}
	if (binbufptr == 0)
	    binbuf[binbufptr++] = '0';

	while (binbufptr > 0)
	    char2buf(buf, buflen, &chars_so_far, binbuf[--binbufptr]);
	if (inexact)
	    char2buf(buf, buflen, &chars_so_far,
		     (char) (flags.f.decimal_point ? '.' : ','));
	return chars_so_far;

	decimal_after_all:;
    }

    char bcd_mantissa[16] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
    int bcd_exponent = 0;
    int bcd_mantissa_sign = 0;

    if (pd < 0) {
	pd = -pd;
	bcd_mantissa_sign = 1;
    }

#ifndef BCD_MATH

    char decstr[32];
    double d = to_double(pd);
    sprintf(decstr, "%.15e", d);
    char *p = decstr;
    int mant_index = 0;
    bcd_mantissa_sign = 0;

    while (*p != 0) {
	char c = *p++;
	if (c == '-') {
	    bcd_mantissa_sign = 1;
	    continue;
	}
	if (c == '.')
	    continue;
	if (c == 'e') {
	    sscanf(p, "%d", &bcd_exponent);
	    break;
	}
	// Can only be decimal digit at this point
	bcd_mantissa[mant_index++] = c - '0';
    }

#else // BCD_MATH

    if (pd != 0) {
	int offset, pos = 0;
	if (pd.bcd.d_[0] >= 1000)
	    offset = 0;
	else if (pd.bcd.d_[0] >= 100)
	    offset = 1;
	else if (pd.bcd.d_[0] >= 10)
	    offset = 2;
	else
	    offset = 3;
	bcd_exponent = pd.bcd.exp() * 4 - 1 - offset;
	for (int i = 0; i < 5; i++) {
	    short s = pd.bcd.d_[i];
	    for (int j = 0; j < 4; j++) {
		if (pos == 16)
		    break;
		if (offset == 0)
		    bcd_mantissa[pos++] = s / 1000;
		else
		    offset--;
		s = (s % 1000) * 10;
	    }
	}
    }

#endif // BCD_MATH

    if (dispmode == 0 || dispmode == 3) {

	/* FIX and ALL modes */

	char norm_ip[12];
	char norm_fp[27];

	int i;
	int int_digits, frac_digits;

	int digits2;
	if (dispmode == 0)
	    digits2 = digits;
	else
	    digits2 = 11;

	for (i = 0; i < 12; i++)
	    norm_ip[i] = 0;
	for (i = 0; i < 27; i++)
	    norm_fp[i] = 0;

	if (bcd_exponent > 11 || -bcd_exponent > digits2 + 1)
	    goto do_sci;
	for (i = 0; i < 16; i++) {
	    if (i <= bcd_exponent)
		norm_ip[11 - bcd_exponent + i] = bcd_mantissa[i];
	    else
		norm_fp[-1 - bcd_exponent + i] = bcd_mantissa[i];
	}

	if (dispmode == 0) {
	    /* NOTE: I don't simply round norm_fp[digits], because of a
	     * circumstance that cannot happen on a real HP-42S: there
	     * may not be enough positions in the display to show a number
	     * at the requested accuracy (e.g. 123456123456.5 at FIX 01)
	     * so I first calculate how many positions I actually have
	     * available, and then I round to that. As a result of this
	     * rounding, the number may overflow the FIX representation,
	     * in which case I fall back on SCI.
	     */
	    int carry;
	    int visdigits = 11 - bcd_exponent;
	    if (visdigits > digits)
		visdigits = digits;
	    carry = norm_fp[visdigits] >= 5;
	    for (i = visdigits; i < 27; i++)
		norm_fp[i] = 0;
	    if (!carry)
		goto done_rounding;
	    for (i = visdigits - 1; i >= 0; i--) {
		char c = norm_fp[i] + 1;
		if (c < 10) {
		    norm_fp[i] = c;
		    goto done_rounding;
		} else
		    norm_fp[i] = c - 10;
	    }
	    for (i = 11; i >= 0; i--) {
		char c = norm_ip[i] + 1;
		if (c < 10) {
		    norm_ip[i] = c;
		    goto done_rounding;
		} else
		    norm_ip[i] = c - 10;
	    }
	    /* If we get here, the carry went past the 12th integer digit,
	     * and we have to use SCI mode instead.
	     */
	    goto do_sci;
	    done_rounding:;
	} else {
	    /* ALL mode: for HP-42S compatibility, round to 12
	     * digits before proceeding.
	     */
	    int f = 1000;
	    for (i = 0; i < 39; i++) {
		char c = i < 12 ? norm_ip[i] : norm_fp[i - 12];
		if (c != 0 && f == 1000)
		    f = i;
		if (i == f + 12) {
		    int carry = c >= 5;
		    if (carry) {
			int j;
			for (j = i - 1; j >= 0; j--) {
			    char c2 = j < 12 ? norm_ip[j] : norm_fp[j - 12];
			    c2++;
			    if (c2 < 10)
				carry = 0;
			    else {
				c2 -= 10;
				carry = 1;
			    }
			    if (j < 12)
				norm_ip[j] = c2;
			    else
				norm_fp[j - 12] = c2;
			    if (!carry)
				break;
			}
			if (carry)
			    /* Rounding is making the integer part 13 digits
			     * long; must go to SCI mode.
			     */
			    goto do_sci;
		    }
		}
		if (i >= f + 12) {
		    if (i < 12)
			norm_ip[i] = 0;
		    else
			norm_fp[i - 12] = 0;
		}
	    }
	}

	/* Check if the number is still within bounds for FIX or ALL */
	if (pd != 0) {
	    /* Make sure that nonzero numbers are not
	     * displayed as zero because of the rounding.
	     */
	    for (i = 0; i < 12; i++)
		if (norm_ip[i] != 0)
		    goto fix_ok;
	    for (i = 0; i < digits2; i++)
		if (norm_fp[i] != 0)
		    goto fix_ok;
	    /* Uh-oh, the number is nonzero, but its rounded representation
	     * is zero. That's not good; use SCI mode instead.
	     */
	    goto do_sci;
	    fix_ok:
	    if (dispmode == 3) {
		/* Make sure we're not throwing away anything in ALL mode */
		for (i = 11; i < 27; i++)
		    if (norm_fp[i] != 0)
			goto do_sci;
	    }
	}

	int_digits = 1;
	for (i = 0; i < 12; i++)
	    if (norm_ip[i] != 0) {
		int_digits = 12 - i;
		break;
	    }

	if (bcd_mantissa_sign)
	    char2buf(buf, buflen, &chars_so_far, '-');

	for (i = int_digits - 1; i >= 0; i--) {
	    if (thousandssep && i % 3 == 2 && i != int_digits - 1)
		char2buf(buf, buflen, &chars_so_far,
				(char) (flags.f.decimal_point ? ',' : '.'));
	    char2buf(buf, buflen, &chars_so_far, (char)('0' + norm_ip[11 - i]));
	}

	if (dispmode == 0)
	    frac_digits = digits;
	else {
	    frac_digits = 0;
	    for (i = 0; i < 27; i++)
		if (norm_fp[i] != 0)
		    frac_digits = i + 1;
	}
	if (frac_digits + int_digits > 12)
	    frac_digits = 12 - int_digits;

	if (frac_digits > 0 || (dispmode == 0 && thousandssep)) {
	    char2buf(buf, buflen, &chars_so_far,
				(char) (flags.f.decimal_point ? '.' : ','));
	    for (i = 0; i < frac_digits; i++)
		char2buf(buf, buflen, &chars_so_far, (char) ('0' + norm_fp[i]));
	}

	return chars_so_far;

    } else {

	/* SCI and ENG modes */
	/* Also fall-through from FIX and ALL */

	int m_digits;
	int carry;
	char norm_mantissa[16];
	int norm_exponent, e3;
	int i;

	do_sci:

	for (i = 0; i < 16; i++)
	    norm_mantissa[i] = bcd_mantissa[i];
	norm_exponent = bcd_exponent;
	
	if (dispmode == 3) {
	    /* Round to 12 digits before doing anything else;
	     * this is needed to handle mantissas like 9.99999999999999,
	     * which would otherwise end up getting displayed as
	     * 10.0000000000 instead of 10.
	     */

	    sci_all_round:

	    carry = norm_mantissa[12] >= 5;
	    for (i = 12; i < 16; i++)
		norm_mantissa[i] = 0;
	    if (carry) {
		for (i = 11; i >= 0; i--) {
		    char c = norm_mantissa[i] + carry;
		    if (c < 10) {
			norm_mantissa[i] = c;
			carry = 0;
			break;
		    } else {
			norm_mantissa[i] = c - 10;
			carry = 1;
		    }
		}
	    }
	    if (carry) {
		/* Don't round by an additional digit: that would mean
		* we're rounding the same number twice, which is bad
		* (think about what happens when you round one digit off
		* 0.45, twice -- you get first 0.5, then 1... Oops).
		* So, we start over.
		*/
		for (i = 0; i < 15; i++)
		    norm_mantissa[i + 1] = bcd_mantissa[i];
		norm_mantissa[0] = 0;
		norm_exponent = bcd_exponent + 1;
		goto sci_all_round;
	    }
	    m_digits = 0;
	    for (i = 11; i >= 0; i--)
		if (norm_mantissa[i] != 0) {
		    m_digits = i;
		    break;
		}
	} else {
	    m_digits = digits;

	    sci_round:
	    carry = norm_mantissa[m_digits + 1] >= 5;
	    for (i = m_digits + 1; i < 16; i++)
		norm_mantissa[i] = 0;
	    if (carry) {
		for (i = m_digits; i >= 0; i--) {
		    char c = norm_mantissa[i] + carry;
		    if (c < 10) {
			norm_mantissa[i] = c;
			carry = 0;
			break;
		    } else {
			norm_mantissa[i] = c - 10;
			carry = 1;
		    }
		}
	    }
	    if (carry) {
		/* Don't round by an additional digit: that would mean
		* we're rounding the same number twice, which is bad
		* (think about what happens when you round one digit off
		* 0.45, twice -- you get first 0.5, then 1... Oops).
		* So, we start over.
		*/
		for (i = 0; i < 15; i++)
		    norm_mantissa[i + 1] = bcd_mantissa[i];
		norm_mantissa[0] = 0;
		norm_exponent = bcd_exponent + 1;
		goto sci_round;
	    }
	}

	if (bcd_mantissa_sign)
	    char2buf(buf, buflen, &chars_so_far, '-');
	if (dispmode == 2) {
	    e3 = norm_exponent % 3;
	    if (e3 < 0)
		e3 += 3;
	    if (m_digits < e3)
		m_digits = e3;
	    norm_exponent -= e3;
	} else
	    e3 = 0;
	for (i = 0; i <= m_digits; i++) {
	    char2buf(buf, buflen, &chars_so_far,
				    (char) ('0' + norm_mantissa[i]));
	    if (i == e3)
		char2buf(buf, buflen, &chars_so_far,
				    (char) (flags.f.decimal_point ? '.' : ','));
	}

	char2buf(buf, buflen, &chars_so_far, 24);
	i = int2string(norm_exponent, buf + chars_so_far,
					    buflen - chars_so_far);
	chars_so_far += i;

	return chars_so_far;
    }
}

void bcdfloat_old2new(void *bcd) {
    // Convert old (<= 1.4.51) BCDFloat, where NaN is signalled by
    // (exp & 0x7FFF) == 0x3000, and Infinity is signalled by
    // (exp & 0x7FFF) == 0x3FFF, to the new (>= 1.4.52) BCDFloat, where NaN is
    // signalled by (exp & 0x4000) != 0 and Infinity is signalled by
    // (exp & 0x2000) != 0 (and the exponent field is 2 bits narrower).
    short *p = (short *) bcd;
    short uexp = p[P] & 0x7FFF;
    if (uexp == 0x3000)
	// NaN
	p[P] = 0x4000;
    else if (uexp == 0x3FFF)
	// Infinity
	p[P] = (p[P] & 0x8000) | 0x2000;
    else
	p[P] = p[P] & 0x9FFF;
}
