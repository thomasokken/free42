/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2006  Thomas Okken
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *****************************************************************************/

#include <stdlib.h>
#include <math.h>

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
    POS_HUGE_PHLOAT.bcd.d_[6] = 9999;
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

void phloat_cleanup() {
    // Nothing to do
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

#if defined(PALMOS) && !defined(PALMOS_ARM)

/* public */
Phloat::Phloat(int4 i) : bcd(i) {
    // Nothing else to do
}

/* public */
Phloat Phloat::operator=(int4 i) {
    bcd = BCDFloat(i);
    return *this;
}

#endif

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

static int8 mant(const BCDFloat &b) PHLOAT_SECT;
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
    return bcd2double(p.bcd);
}

Phloat sin(Phloat p) {
    Phloat res;
    res.bcd = sin(BCD(p.bcd)).ref_->v_;
    return res;
}

Phloat cos(Phloat p) {
    Phloat res;
    res.bcd = cos(BCD(p.bcd)).ref_->v_;
    return res;
}

Phloat tan(Phloat p) {
    Phloat res;
    res.bcd = tan(BCD(p.bcd)).ref_->v_;
    return res;
}

Phloat asin(Phloat p) {
    Phloat res;
    res.bcd = asin(BCD(p.bcd)).ref_->v_;
    return res;
}

Phloat acos(Phloat p) {
    Phloat res;
    res.bcd = acos(BCD(p.bcd)).ref_->v_;
    return res;
}

Phloat atan(Phloat p) {
    Phloat res;
    res.bcd = atan(BCD(p.bcd)).ref_->v_;
    return res;
}

void sincos(Phloat phi, Phloat *s, Phloat *c) {
    BCD p(phi.bcd);
    s->bcd = sin(p).ref_->v_;
    c->bcd = cos(p).ref_->v_;
}

Phloat hypot(Phloat x, Phloat y) {
    Phloat res;
    res.bcd = hypot(BCD(x.bcd), BCD(y.bcd)).ref_->v_;
    return res;
}

Phloat atan2(Phloat x, Phloat y) {
    Phloat res;
    res.bcd = atan2(BCD(x.bcd), BCD(y.bcd)).ref_->v_;
    return res;
}

Phloat sinh(Phloat p) {
    // (exp(x)-exp(-x))/2
    BCDFloat temp1 = exp(BCD(p.bcd)).ref_->v_;
    BCDFloat temp2 = exp(BCD((-p).bcd)).ref_->v_;
    BCDFloat temp3;
    BCDFloat::sub(&temp1, &temp2, &temp3);
    const BCDFloat two(2);
    Phloat res;
    BCDFloat::div(&temp3, &two, &res.bcd);
    return res;
}

Phloat cosh(Phloat p) {
    // (exp(x)+exp(-x))/2
    BCDFloat temp1 = exp(BCD(p.bcd)).ref_->v_;
    BCDFloat temp2 = exp(BCD((-p).bcd)).ref_->v_;
    BCDFloat temp3;
    BCDFloat::add(&temp1, &temp2, &temp3);
    const BCDFloat two(2);
    Phloat res;
    BCDFloat::div(&temp3, &two, &res.bcd);
    return res;
}

Phloat tanh(Phloat p) {
    // (exp(x)-exp(-x))/(exp(x)+exp(-x))
    BCDFloat temp1 = exp(BCD(p.bcd)).ref_->v_;
    BCDFloat temp2 = exp(BCD((-p).bcd)).ref_->v_;
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
    temp1 = sqrt(BCD(temp2)).ref_->v_;
    BCDFloat::add(&temp1, &p.bcd, &temp2);
    Phloat res;
    res.bcd = log(BCD(temp2)).ref_->v_;
    return res;
}

Phloat acosh(Phloat p) {
    // log(sqrt(x^2-1)+x)
    BCDFloat temp1;
    BCDFloat::mul(&p.bcd, &p.bcd, &temp1);
    BCDFloat temp2;
    const BCDFloat one(1);
    BCDFloat::sub(&temp1, &one, &temp2);
    temp1 = sqrt(BCD(temp2)).ref_->v_;
    BCDFloat::add(&temp1, &p.bcd, &temp2);
    Phloat res;
    res.bcd = log(BCD(temp2)).ref_->v_;
    return res;
}

Phloat atanh(Phloat p) {
    // log((1+x)/(1-x))/2
    const BCDFloat one(1);
    BCDFloat temp1, temp2, temp3;
    BCDFloat::add(&one, &p.bcd, &temp1);
    BCDFloat::sub(&one, &p.bcd, &temp2);
    BCDFloat::div(&temp1, &temp2, &temp3);
    temp1 = log(BCD(temp3)).ref_->v_;
    Phloat res;
    const BCDFloat two(2);
    BCDFloat::div(&temp1, &two, &res.bcd);
    return res;
}

Phloat log(Phloat p) {
    Phloat res;
    res.bcd = log(BCD(p.bcd)).ref_->v_;
    return res;
}

Phloat log1p(Phloat p) {
    Phloat res;
    res.bcd = ln1p(BCD(p.bcd)).ref_->v_;
    return res;
}

Phloat log10(Phloat p) {
    Phloat res;
    res.bcd = log10(BCD(p.bcd)).ref_->v_;
    return res;
}

Phloat exp(Phloat p) {
    Phloat res;
    res.bcd = exp(BCD(p.bcd)).ref_->v_;
    return res;
}

Phloat expm1(Phloat p) {
    Phloat res;
    res.bcd = expm1(BCD(p.bcd)).ref_->v_;
    return res;
}

Phloat gamma(Phloat p) {
    --p;
    Phloat res;
    res.bcd = gammaFactorial(BCD(p.bcd)).ref_->v_;
    return res;
}

Phloat sqrt(Phloat p) {
    Phloat res;
    res.bcd = sqrt(BCD(p.bcd)).ref_->v_;
    return res;
}

Phloat fmod(Phloat x, Phloat y) {
    Phloat res;
    res.bcd = fmod(BCD(x.bcd), BCD(y.bcd)).ref_->v_;
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
	int iy = ifloor(y.bcd);
	BCDFloat by(iy);
	if (BCDFloat::equal(&y.bcd, &by)) {
	    res.bcd = pow(BCD(x.bcd), iy).ref_->v_;
	    return res;
	}
    }
    res.bcd = pow(BCD(x.bcd), BCD(y.bcd)).ref_->v_;
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

double bcd2double(BCDFloat b) {
    unsigned short exp = b.d_[P];
    double zero = 0;

#if defined(WINDOWS) && !defined(__GNUC__)
    if (exp == 0x3000)
	return HUGE_VAL; // NaN
    else if (exp == 0x3FFF)
	return HUGE_VAL; // Inf
    else if (exp == 0xBFFF)
	return -HUGE_VAL; // -Inf
#else
    if (exp == 0x3000)
	return 0 / zero; // NaN
    else if (exp == 0x3FFF)
	return 1 / zero; // Inf
    else if (exp == 0xBFFF)
	return -1 / zero; // -Inf
#endif

    if (b.d_[0] == 0)
	return 0;

    bool neg = (exp & 0x8000) != 0;
    exp = (exp << 1) >> 1;

    // Make sure we get exact results for integers
    if (exp > 0 && exp <= 4) {
	int j;
	for (j = exp; j < P; j++)
	    if (b.d_[j] != 0)
		goto noninteger;
	int8 n = 0;
	for (j = 0; j < (short) exp; j++)
	    n = n * 10000 + b.d_[j];
	return (double) (neg ? -n : n);
    }

    // TODO: Using a table-based conversion algorithm, like the one I use in
    // the binary version of string2phloat, I could get better accuracy.
    // The current code returns an inexact result for 23.5, for instance
    // (VC++), while the number *is* exactly representable in a 'double'.

    noninteger:
    double res = 0;
    for (int i = 0; i < P; i++)
	res = res * 10000 + b.d_[i];
    if (neg)
	res = -res;
    return res * pow(10000.0, (double) (exp - P));
}


#else // BCD_MATH


/* binary-to-bcd conversion tables */
static int min_pow2;
static int max_pow2;
static char *pos_pow2mant;
static int *pos_pow2exp;
static char *neg_pow2mant;
static int *neg_pow2exp;
static shell_bcd_table_struct *bcdtab = NULL;


/*********************/
/* Private functions */
/*********************/

static double POS_HUGE_DOUBLE;
static double NEG_HUGE_DOUBLE;
static double POS_TINY_DOUBLE;
static double NEG_TINY_DOUBLE;

static void bcd_add(char *dst_mant, int *dst_exp,
		    const char *src_mant, int src_exp) PHLOAT_SECT;
static int bcd_cmp(const char *dst_mant, int dst_exp,
		   const char *src_mant, int src_exp) PHLOAT_SECT;
static void bcd_sub(char *dst_mant, int *dst_exp,
		    const char *src_mant, int src_exp) PHLOAT_SECT;


/********************/
/* Public functions */
/********************/

void phloat_init() {
    double d, pd, nd, tmp;
    char temp_bcd_mantissa[20];
    int temp_bcd_exponent;
    int e, i;
    int carry;
    uint4 n1, n2, n3, n4, size;
    char *base;

    /**************************************************************/
    /* First, try if the BCD table is available without doing any */
    /* hard work. If it isn't, we'll compute it from scratch and  */
    /* try to save it so we don't have to do this again later.    */
    /**************************************************************/

    shell_bcd_table_struct *bcdtab = shell_get_bcd_table();
    if (bcdtab != NULL) {
	char *base = (char *) (bcdtab + 1);
	POS_HUGE_DOUBLE = bcdtab->pos_huge_double;
	NEG_HUGE_DOUBLE = bcdtab->neg_huge_double;
	POS_TINY_DOUBLE = bcdtab->pos_tiny_double;
	NEG_TINY_DOUBLE = bcdtab->neg_tiny_double;
	POS_HUGE_PHLOAT = POS_HUGE_DOUBLE;
	NEG_HUGE_PHLOAT = NEG_HUGE_DOUBLE;
	POS_TINY_PHLOAT = POS_TINY_DOUBLE;
	NEG_TINY_PHLOAT = NEG_TINY_DOUBLE;
	max_pow2 = bcdtab->max_pow2;
	min_pow2 = bcdtab->min_pow2;
	pos_pow2mant = base;
	pos_pow2exp = (int *) (base + bcdtab->pos_pow2exp_offset);
	neg_pow2mant = base + bcdtab->neg_pow2mant_offset;
	neg_pow2exp = (int *) (base + bcdtab->neg_pow2exp_offset);
	return;
    }


    /****************************************************************/
    /* Since I can't find a portable API or macro that gives me     */
    /* the largest positive and negative numbers (no, I don't want  */
    /* to hard-code IEEE-754 -- I'm paranoid), I just find them out */
    /* myself.                                                      */
    /****************************************************************/

    pd = 1;
    while (1) {
	tmp = pd * 2;
	if (isinf(tmp) || tmp <= pd)
	    break;
	pd = tmp;
    }
    POS_HUGE_DOUBLE = pd;
    while (1) {
	pd = pd / 2;
	tmp = POS_HUGE_DOUBLE + pd;
	if (isinf(tmp))
	    continue;
	if (POS_HUGE_DOUBLE == tmp)
	    break;
	POS_HUGE_DOUBLE = tmp;
    }

    nd = -1;
    while (1) {
	tmp = nd * 2;
	if (isinf(tmp) || tmp >= nd)
	    break;
	nd = tmp;
    }
    NEG_HUGE_DOUBLE = nd;
    while (1) {
	nd = nd / 2;
	tmp = NEG_HUGE_DOUBLE + nd;
	if (isinf(tmp))
	    continue;
	if (NEG_HUGE_DOUBLE == tmp)
	    break;
	NEG_HUGE_DOUBLE = tmp;
    }


    /*************************************************************************/
    /* I need the smallest nonzero doubles so I have something to substitute */
    /* when I encounter something like 1e-499 in a "raw" file I'm importing. */
    /* Normally, underflows just go to zero, but in the case of a number     */
    /* literal in a program, that seems wrong.                               */
    /*************************************************************************/

    pd = 1;
    while (1) {
	tmp = pd / 2;
#if defined(WINDOWS) && !defined(__GNUC__)
	/* Without this, Visual C++ 6.0 generates code that leaves the loop
	 * one iteration too late. Looks like an optimizer bug or something.
	 */
	fabs(tmp);
#endif
	if (tmp == 0)
	    break;
	pd = tmp;
    }
    POS_TINY_DOUBLE = pd;
    nd = -1;
    while (1) {
	tmp = nd / 2;
#if defined(WINDOWS) && !defined(__GNUC__)
	/* Without this, Visual C++ 6.0 generates code that leaves the loop
	 * one iteration too late. Looks like an optimizer bug or something.
	 */
	fabs(tmp);
#endif
	if (tmp == 0)
	    break;
	nd = tmp;
    }
    NEG_TINY_DOUBLE = nd;


    /*****************************************************************/
    /* Compute BCD representations of all representable powers of 2. */
    /* These will be used for binary-to-decimal and decimal-to-      */
    /* binary conversions.                                           */
    /*****************************************************************/

    d = 1;
    max_pow2 = 0;
    while (finite(d)) {
	max_pow2++;
	d *= 2;
    }
    /* Yes, I deliberately look for max_pow2 such that it is actually
     * the lowest power of 2 that is *too big* for a 'double'. Of course,
     * this value is never going to get used in double-to-bcd conversion,
     * but it is needed as a sentinel in the bcd-to-double conversion
     * (that code looks starts out by looking for the bcd value of the
     * highest nonzero bit in the number to be converted, and the process
     * of finding that value will overshoot the desired power by 1 before
     * settling on the right value).
     */

    d = 1;
    min_pow2 = -1;
    while (d != 0) {
	min_pow2++;
	d /= 2;
    }


    n1 = 16 * (max_pow2 + 1);
    n2 = sizeof(int) * (max_pow2 + 1);
    n3 = 16 * min_pow2;
    n4 = sizeof(int) * min_pow2;
    size = sizeof(shell_bcd_table_struct) + n1 + n2 + n3 + n4;
    bcdtab = (shell_bcd_table_struct *) malloc(size);
    bcdtab->pos_huge_double = POS_HUGE_DOUBLE;
    bcdtab->neg_huge_double = NEG_HUGE_DOUBLE;
    bcdtab->pos_tiny_double = POS_TINY_DOUBLE;
    bcdtab->neg_tiny_double = NEG_TINY_DOUBLE;
    bcdtab->max_pow2 = max_pow2;
    bcdtab->min_pow2 = -min_pow2;
    bcdtab->pos_pow2exp_offset = n1;
    bcdtab->neg_pow2mant_offset = n1 + n2;
    bcdtab->neg_pow2exp_offset = n1 + n2 + n3;

    base = (char *) (bcdtab + 1);
    pos_pow2mant = (char *) base;
    pos_pow2exp = (int *) (base + n1);
    neg_pow2mant = (char *) (base + n1 + n2);
    neg_pow2exp = (int *) (base + n1 + n2 + n3);


    temp_bcd_mantissa[0] = 1;
    for (i = 1; i < 20; i++)
	temp_bcd_mantissa[i] = 0;
    temp_bcd_exponent = 0;

    for (e = 0; e <= max_pow2; e++) {
	if (e != 0) {
	    int carry = 0;
	    for (i = 19; i >= 0; i--) {
		char c = temp_bcd_mantissa[i] * 2 + carry;
		if (c < 10) {
		    temp_bcd_mantissa[i] = c;
		    carry = 0;
		} else {
		    temp_bcd_mantissa[i] = c - 10;
		    carry = 1;
		}
	    }
	    if (carry) {
		/* Need to shift mantissa 1 position to the right.
		 * Apply rounding on last digit.
		 */
		int carry2 = 0;
		if (temp_bcd_mantissa[19] >= 5) {
		    carry2 = 1;
		    for (i = 18; i >= 0; i--) {
			char c = temp_bcd_mantissa[i] + carry2;
			if (c < 10) {
			    temp_bcd_mantissa[i] = c;
			    carry2 = 0;
			    break;
			} else {
			    temp_bcd_mantissa[i] = c - 10;
			    carry2 = 1;
			}
		    }
		}
		for (i = 19; i >= 1; i--)
		    temp_bcd_mantissa[i] = temp_bcd_mantissa[i - 1];
		temp_bcd_mantissa[0] = carry + carry2;
		temp_bcd_exponent++;
	    }
	}

	carry = temp_bcd_mantissa[16] >= 5;
	for (i = 15; i >= 0; i--) {
	    char c = temp_bcd_mantissa[i] + carry;
	    if (c < 10) {
		pos_pow2mant[16 * e + i] = c;
		carry = 0;
	    } else {
		pos_pow2mant[16 * e + i] = c - 10;
		carry = 1;
	    }
	}
	pos_pow2exp[e] = temp_bcd_exponent;
	if (carry) {
	    /* Don't shift, but redo the copy with a 1 digit offset.
	     * If we would shift here, we would have to round the mantissa
	     * twice (once for going from 20 digits to 16, then again for
	     * losing a digit when dividing by 10).
	     */
	    carry = temp_bcd_mantissa[17] >= 5;
	    for (i = 15; i > 0; i--) {
		char c = temp_bcd_mantissa[i + 1] + carry;
		if (c < 10) {
		    pos_pow2mant[16 * e + i] = c;
		    carry = 0;
		} else {
		    pos_pow2mant[16 * e + i] = c - 10;
		    carry = 1;
		}
	    }
	    pos_pow2mant[16 * e] = carry;
	    pos_pow2exp[e] = temp_bcd_exponent + 1;
	}
    }

    temp_bcd_mantissa[0] = 1;
    for (i = 1; i < 20; i++)
	temp_bcd_mantissa[i] = 0;
    temp_bcd_exponent = 0;

    for (e = 0; e < min_pow2; e++) {
	int carry = 0;
	for (i = 19; i >= 0; i--) {
	    char c = temp_bcd_mantissa[i] * 5 + carry;
	    if (c < 10) {
		temp_bcd_mantissa[i] = c;
		carry = 0;
	    } else {
		temp_bcd_mantissa[i] = c % 10;
		carry = c / 10;
	    }
	}
	temp_bcd_exponent--;
	if (carry) {
	    /* Need to shift mantissa 1 position to the right.
	     * Apply rounding on last digit.
	     */
	    int carry2 = 0;
	    if (temp_bcd_mantissa[19] >= 5) {
		carry2 = 1;
		for (i = 18; i >= 0; i--) {
		    char c = temp_bcd_mantissa[i] + carry2;
		    if (c < 10) {
			temp_bcd_mantissa[i] = c;
			carry2 = 0;
			break;
		    } else {
			temp_bcd_mantissa[i] = c - 10;
			carry2 = 1;
		    }
		}
	    }
	    for (i = 19; i >= 1; i--)
		temp_bcd_mantissa[i] = temp_bcd_mantissa[i - 1];
	    temp_bcd_mantissa[0] = carry + carry2;
	    temp_bcd_exponent++;
	}

	carry = temp_bcd_mantissa[16] >= 5;
	for (i = 15; i >= 0; i--) {
	    char c = temp_bcd_mantissa[i] + carry;
	    if (c < 10) {
		neg_pow2mant[16 * e + i] = c;
		carry = 0;
	    } else {
		neg_pow2mant[16 * e + i] = c - 10;
		carry = 1;
	    }
	}
	neg_pow2exp[e] = temp_bcd_exponent;
	if (carry) {
	    /* Don't shift, but redo the copy with a 1 digit offset.
	     * If we would shift here, we would have to round the mantissa
	     * twice (once for going from 20 digits to 16, then again for
	     * losing a digit when dividing by 10).
	     */
	    carry = temp_bcd_mantissa[17] >= 5;
	    for (i = 15; i > 0; i--) {
		char c = temp_bcd_mantissa[i + 1] + carry;
		if (c < 10) {
		    neg_pow2mant[16 * e + i] = c;
		    carry = 0;
		} else {
		    neg_pow2mant[16 * e + i] = c - 10;
		    carry = 1;
		}
	    }
	    neg_pow2mant[16 * e] = carry;
	    neg_pow2exp[e] = temp_bcd_exponent + 1;
	}
    }

    min_pow2 = -min_pow2;
    bcdtab = shell_put_bcd_table(bcdtab, size);
    base = (char *) (bcdtab + 1);
    pos_pow2mant = (char *) base;
    pos_pow2exp = (int *) (base + n1);
    neg_pow2mant = (char *) (base + n1 + n2);
    neg_pow2exp = (int *) (base + n1 + n2 + n3);

    POS_HUGE_PHLOAT = POS_HUGE_DOUBLE;
    NEG_HUGE_PHLOAT = NEG_HUGE_DOUBLE;
    POS_TINY_PHLOAT = POS_TINY_DOUBLE;
    NEG_TINY_PHLOAT = NEG_TINY_DOUBLE;
}

void phloat_cleanup() {
    shell_release_bcd_table(bcdtab);
}

static void bcd_add(char *dst_mant, int *dst_exp,
		    const char *src_mant, int src_exp) {
    int i;

    if (dst_mant[0] == 0 || src_exp > *dst_exp + 16) {
	for (i = 0; i < 16; i++)
	    dst_mant[i] = src_mant[i];
	*dst_exp = src_exp;
    } else if (src_exp == *dst_exp) {
	int carry;
	add_matching:
	carry = 0;
	for (i = 15; i >= 0; i--) {
	    char c = src_mant[i] + dst_mant[i] + carry;
	    if (c < 10) {
		dst_mant[i] = c;
		carry = 0;
	    } else {
		dst_mant[i] = c - 10;
		carry = 1;
	    }
	}
	if (carry) {
	    /* Need to shift mantissa 1 position to the right.
	     * Apply rounding on last digit.
	     */
	    int carry2 = 0;
	    if (dst_mant[15] >= 5) {
		carry2 = 1;
		for (i = 14; i >= 0; i--) {
		    char c = dst_mant[i] + carry2;
		    if (c < 10) {
			dst_mant[i] = c;
			carry2 = 0;
			break;
		    } else {
			dst_mant[i] = c - 10;
			carry2 = 1;
		    }
		}
	    }
	    for (i = 15; i >= 1; i--)
		dst_mant[i] = dst_mant[i - 1];
	    dst_mant[0] = carry + carry2;
	    (*dst_exp)++;
	}
    } else if (src_exp > *dst_exp) {
	/* The case that src_exp is greater than *dst_exp+16 is handled
	 * as if dst==0; the case being handled here is src_exp-*dst_exp
	 * between 1 and 16.
	 */
	int shift = src_exp - *dst_exp;
	/* Shift dst so its exponent matches src */
	int carry = dst_mant[16 - shift] >= 5;
	for (i = 15; i >= 0; i--)
	    dst_mant[i] = i < shift ? 0 : dst_mant[i - shift];
	if (carry) {
	    for (i = 15; i >= 0; i--) {
		char c = dst_mant[i] + carry;
		if (c < 10) {
		    dst_mant[i] = c;
		    carry = 0;
		    break;
		} else {
		    dst_mant[i] = c - 10;
		    carry = 1;
		}
	    }
	}
	/* Carry is always 0 here, because the leftmost digit
	 * is 0 + carry, which never leads to the carry being propagated
	 * further.
	 */
	*dst_exp = src_exp;
	/* Now that dst is aligned with src, proceed using the code
	 * for the dst_exp == src_exp case.
	 */
	goto add_matching;
    } else /* src_exp < *dst_exp */ {
	int carry;
	int shift = *dst_exp - src_exp;
	if (shift > 16)
	    /* The number being added is too small to matter */
	    return;
	carry = src_mant[16 - shift] >= 5;
	for (i = 15; i >= 0; i--) {
	    char c = dst_mant[i] + carry;
	    if (i >= shift)
		c += src_mant[i - shift];
	    if (c < 10) {
		dst_mant[i] = c;
		carry = 0;
		if (i <= shift)
		    break;
	    } else {
		dst_mant[i] = c - 10;
		carry = 1;
	    }
	}
	if (carry) {
	    /* Need to shift mantissa 1 position to the right.
	     * Apply rounding on last digit.
	     */
	    int carry2 = 0;
	    if (dst_mant[15] >= 5) {
		carry2 = 1;
		for (i = 14; i >= 0; i--) {
		    char c = dst_mant[i] + carry2;
		    if (c < 10) {
			dst_mant[i] = c;
			carry2 = 0;
			break;
		    } else {
			dst_mant[i] = c - 10;
			carry2 = 1;
		    }
		}
	    }
	    for (i = 15; i >= 1; i--)
		dst_mant[i] = dst_mant[i - 1];
	    dst_mant[0] = carry + carry2;
	    (*dst_exp)++;
	}
    }
}

static int bcd_cmp(const char *dst_mant, int dst_exp,
		   const char *src_mant, int src_exp) {
    /* Note: both numbers should be normalized, that is, the leftmost
     * digit should only be zero if the entire number is zero.
     */
    int i;
    if (dst_exp > src_exp)
	return 1;
    else if (dst_exp < src_exp)
	return -1;
    for (i = 0; i < 16; i++)
	if (dst_mant[i] > src_mant[i])
	    return 1;
	else if (dst_mant[i] < src_mant[i])
	    return -1;
    return 0;
}

static void bcd_sub(char *dst_mant, int *dst_exp,
		    const char *src_mant, int src_exp) {
    /* Note: this function should only be called when dst >= src
     * (the bcd_cmp() function can be used to ascertain this)
     */
    int i;
    int exp_offset = *dst_exp - src_exp;
    int borrow;
    if (exp_offset > 16)
	/* The number to be subtracted is too small to make a difference. */
	return;
    if (exp_offset == 0)
	borrow = 0;
    else
	borrow = src_mant[16 - exp_offset] >= 5;

    /* Note: no check for exp_offset < 0 -- we require dst >= src ! */

    for (i = 15; i >= 0; i--) {
	char c;
	if (i < exp_offset)
	    c = dst_mant[i] - borrow;
	else
	    c = dst_mant[i] - src_mant[i - exp_offset] - borrow;
	if (c >= 0) {
	    dst_mant[i] = c;
	    borrow = 0;
	    if (i <= exp_offset)
		break;
	} else {
	    dst_mant[i] = c + 10;
	    borrow = 1;
	}
    }

    /* Note: no check for borrow != 0 at this point;
     * we require dst >= src !
     */

    /* Normalize dst */
    exp_offset = -1;
    for (i = 0; i < 15; i++)
	if (dst_mant[i] != 0) {
	    exp_offset = i;
	    break;
	}
    if (exp_offset <= 0)
	/* exp_offset == -1 means dst == 0 now;
	 * exp_offset == 0 means dst is nonzero and normalized.
	 */
	return;
    for (i = 0; i < 16 - exp_offset; i++)
	dst_mant[i] = dst_mant[i + exp_offset];
    for (i = 16 - exp_offset; i < 16; i++)
	dst_mant[i] = 0;
    *dst_exp -= exp_offset;
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
    int d_exp, lastr;
    int is_zero = 1;
    double d_mant, d_bit, res;
    int temp_exp;

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

    /* We now perform the decimal-to-binary conversion by subtracting
     * bcd representations of powers of two, using the same lookup table
     * that is used for the binary-to-decimal conversion.
     * First, we must look for the greatest power of two that is less than
     * or equal to our number...
     */

    d_exp = (int) (((double) exp) / log10(2.0));
    if (d_exp < min_pow2)
	d_exp = min_pow2;
    else if (d_exp > max_pow2)
	d_exp = max_pow2;

    lastr = 0;
    while (1) {
	int r;
	char *bit_mant;
	int bit_exp;
	if (d_exp >= 0) {
	    bit_mant = pos_pow2mant + 16 * d_exp;
	    bit_exp = pos_pow2exp[d_exp];
	} else {
	    bit_mant = neg_pow2mant + 16 * (-1 - d_exp);
	    bit_exp = neg_pow2exp[-1 - d_exp];
	}
	r = bcd_cmp(mantissa, exp, bit_mant, bit_exp);
	if (r == 0)
	    break;
	if (r < 0 && lastr > 0) {
	    d_exp--;
	    break;
	}
	if (r > 0 && lastr < 0)
	    break;
	if (r < 0) {
	    d_exp--;
	    if (d_exp < min_pow2)
		/* Too small! */
		return mant_sign ? 4 : 3;
	    lastr = r;
	} else {
	    d_exp++;
	    if (d_exp > max_pow2)
		/* Too big! */
		return mant_sign ? 2 : 1;
	    lastr = r;
	}
    }

    /* 'd_exp' is now the greatest power of 2 which is less than or
     * equal to our decimal number.
     * Now we subtract this, and successively smaller powers of 2, from
     * the decimal number, until we hit zero or until the contribution
     * from the powers of 2 to the final result becomes too small.
     */
    d_mant = 0;
    d_bit = 1;
    temp_exp = d_exp;
    while (1) {
	char *bit_mant;
	int bit_exp;
	int r;

	if (d_mant + d_bit == d_mant)
	    /* We have reached full precision. */
	    break;

	if (temp_exp >= 0) {
	    bit_mant = pos_pow2mant + 16 * temp_exp;
	    bit_exp = pos_pow2exp[temp_exp];
	} else {
	    bit_mant = neg_pow2mant + 16 * (-1 - temp_exp);
	    bit_exp = neg_pow2exp[-1 - temp_exp];
	}

	r = bcd_cmp(mantissa, exp, bit_mant, bit_exp);
	if (r == 0) {
	    d_mant += d_bit;
	    break;
	} else if (r > 0) {
	    d_mant += d_bit;
	    bcd_sub(mantissa, &exp, bit_mant, bit_exp);
	}
	d_bit /= 2;
	temp_exp--;
    }

    res = ldexp(d_mant, (short) d_exp);
    if (mant_sign)
	res = -res;
    if (res == 0)
	return mant_sign ? 4 : 3;
    else if (p_isinf(res))
	return mant_sign ? 2 : 1;
    else {
	*d = res;
	return 0;
    }
}

double bcd2double(const short *p) {
    short exp = p[P];
    double zero = 0;

#if defined(WINDOWS) && !defined(__GNUC__)
    if (exp == 0x3000)
	return HUGE_VAL; // NaN
    else if (exp == 0x3FFF)
	return HUGE_VAL; // Inf
    else if (exp == (short) 0xBFFF)
	return -HUGE_VAL; // -Inf
#else
    if (exp == 0x3000)
	return 0 / zero; // NaN
    else if (exp == 0x3FFF)
	return 1 / zero; // Inf
    else if (exp == (short) 0xBFFF)
	return -1 / zero; // -Inf
#endif

    if (p[0] == 0)
	return 0;

    bool neg = (exp & 0x8000) != 0;
    exp = ((short) (exp << 1)) >> 1;

    // Make sure we get exact results for integers
    if (exp > 0 && exp <= 5) {
	int j;
	for (j = exp; j < P; j++)
	    if (p[j] != 0)
		goto noninteger;
	int8 n = 0;
	for (j = 0; j < exp; j++)
	    n = n * 10000 + p[j];
	return (double) (neg ? -n : n);
    }

    // TODO: Using a table-based conversion algorithm, like the one I use in
    // the binary version of string2phloat, I could get better accuracy.
    // The current code returns an inexact result for 23.5, for instance
    // (VC++), while the number *is* exactly representable in a 'double'.

    noninteger:
    double res = 0;
    for (int i = 0; i < P; i++)
	res = res * 10000 + p[i];
    if (neg)
	res = -res;
    return res * pow(10000.0, (double) (exp - P));
}


#endif // BCD_MATH


int phloat2string(phloat pd, char *buf, int buflen, int base_mode, int digits,
			 int dispmode, int thousandssep) {
    int chars_so_far = 0;
    int base;

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

    /* base_mode: 0=only decimal, 1=all bases, 2=all bases (show) */
    if (base_mode != 0 && (base = get_base()) != 10) {
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

    double d = to_double(pd);
    double mantissa;
#if defined(PALMOS) && !defined(PALMOS_ARM)
    Int16 exp;
#else
    int exp;
#endif
    int still_zero = 1;

    mantissa = frexp(d, &exp);
    while (mantissa != 0) {
	int bit;
	mantissa *= 2;
	exp--;
	bit = (int) mantissa;
	mantissa -= bit;
	if (bit) {
	    char *bit_mant;
	    int bit_exp;
	    if (exp >= 0) {
		bit_mant = pos_pow2mant + 16 * exp;
		bit_exp = pos_pow2exp[exp];
	    } else {
		bit_mant = neg_pow2mant + 16 * (-1 - exp);
		bit_exp = neg_pow2exp[-1 - exp];
	    }
	    if (!still_zero && bit_exp < bcd_exponent - 17)
		/* We have reached the full 16 decimal digits precision. */
		break;
	    bcd_add(bcd_mantissa, &bcd_exponent, bit_mant, bit_exp);
	    still_zero = 0;
	}
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

#if defined(PALMOS) && defined(BCD_MATH)

// Compatibility stuff - note that some of these are only partial
// implementations. These are just hacks so that core_phloat and bcdfloat can
// be compiled and run on Palms without MathLib.

union double_words {
    double d;
    struct {
	int4 hx, lx;
    } w;
};

int isinf(double x) {
    double_words dw;
    dw.d = x;
    dw.w.lx |= (dw.w.hx & 0x7fffffff) ^ 0x7ff00000;
    dw.w.lx |= -dw.w.lx;
    return ~(dw.w.lx >> 31) & (dw.w.hx >> 30);
}

int isnan(double x) {
    double_words dw;
    dw.d = x;
    dw.w.hx &= 0x7fffffff;
    dw.w.hx |= (uint4)(dw.w.lx|(-dw.w.lx))>>31;
    dw.w.hx = 0x7ff00000 - dw.w.hx;
    return (int)(((uint4)dw.w.hx)>>31);
}

double pow(double x, double y) {
    // Only handles integer exponents
    int4 exp = (int4) y;
    bool neg = exp < 0;
    if (neg)
	exp = -exp;
    double res = 1, factor = x;
    while (exp != 0) {
	if ((exp & 1) != 0)
	    res *= factor;
	exp >>= 1;
	factor *= factor;
    }
    return neg ? 1 / res : res;
}

double floor(double x) {
    // Yes, this is adequate, believe it or not: this only gets called on the
    // result of our hacked log10() (see below), and that function implicitly
    // "floors" its return value.
    return x;
}

double log10(double x) {
    // TODO: faster implementation. It doesn't matter all that much, since this
    // function is only called for the state file conversion, but still, that
    // conversion shouldn't take any longer than necessary either.
    if (x <= 0) {
	double zero = 0;
	return x == 0 ? -1 / zero : 0 / zero;
    }
    int res = 0;
    if (x < 1) {
	while (x < 1) {
	    x *= 10;
	    res--;
	}
    } else {
	while (x >= 10) {
	    x /= 10;
	    res++;
	}
    }
    return (double) res;
}

#endif
