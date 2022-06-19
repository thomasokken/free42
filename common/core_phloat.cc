/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2022  Thomas Okken
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
#include <string.h>
#include <math.h>
#include <float.h>

#include "core_phloat.h"
#include "core_helpers.h"
#include "shell.h"

#ifndef BCD_MATH
// We need these locally for BID128->double conversion
#include "bid_conf.h"
#include "bid_functions.h"
#endif


phloat POS_HUGE_PHLOAT;
phloat NEG_HUGE_PHLOAT;
phloat POS_TINY_PHLOAT;
phloat NEG_TINY_PHLOAT;
phloat NAN_PHLOAT;
phloat NAN_1_PHLOAT;
phloat NAN_2_PHLOAT;


/* Note: this function does not handle infinities or NaN */
static void bcdfloat2string(short *p, char *buf) {
    short exp = p[7];
    bool neg = (exp & 0x8000) != 0;
    exp = ((short) (exp << 3)) >> 3;
    if (neg)
        *buf++ = '-';
    for (int i = 0; i < 7; i++) {
        short d = p[i];
        sprintf(buf, "%04d", d);
        if (i == 0) {
            for (int j = 4; j >= 2; j--)
                buf[j] = buf[j - 1];
            buf[1] = '.';
            buf += 5;
        } else
            buf += 4;
    }
    sprintf(buf, "e%d", exp * 4 - 1);
}

static void bcdfloat_old2new(void *bcd) {
    // Convert old (<= 1.4.51) BCDFloat, where NaN is signalled by
    // (exp & 0x7FFF) == 0x3000, and Infinity is signalled by
    // (exp & 0x7FFF) == 0x3FFF, to the new (>= 1.4.52) BCDFloat, where NaN is
    // signalled by (exp & 0x4000) != 0 and Infinity is signalled by
    // (exp & 0x2000) != 0 (and the exponent field is 2 bits narrower).
    short *p = (short *) bcd;
    short uexp = p[7] & 0x7FFF;
    if (uexp == 0x3000)
        // NaN
        p[7] = 0x4000;
    else if (uexp == 0x3FFF)
        // Infinity
        p[7] = (p[7] & 0x8000) | 0x2000;
    else
        p[7] = p[7] & 0x9FFF;
}


#ifdef BCD_MATH


void phloat_init() {
    BID_UINT128 posinf, neginf, zero, poshuge, neghuge, postiny, negtiny, nan;
    bid128_from_string(&posinf, (char *) "+Inf");
    bid128_from_string(&neginf, (char *) "-Inf");
    int z = 0;
    bid128_from_int32(&zero, &z);
    bid128_nextafter(&poshuge, &posinf, &zero);
    bid128_nextafter(&neghuge, &neginf, &zero);
    bid128_nextafter(&postiny, &zero, &posinf);
    bid128_nextafter(&negtiny, &zero, &neginf);
    POS_HUGE_PHLOAT = poshuge;
    NEG_HUGE_PHLOAT = neghuge;
    POS_TINY_PHLOAT = postiny;
    NEG_TINY_PHLOAT = negtiny;
    bid128_div(&nan, &zero, &zero);
    NAN_PHLOAT = nan;
    bid128_nan(&NAN_1_PHLOAT.val, "1");
    bid128_nan(&NAN_2_PHLOAT.val, "2");
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

    // Special case: "-" by itself. bid128_from_string() doesn't like this,
    // so handling it separately here.
    if (buflen == 1 && buf[0] == '-') {
        *d = 0;
        return 0;
    }

    // First, convert from HP-42S format to bcdfloat format:
    // strip thousands separators, convert comma to dot if
    // appropriate, and convert char(24) to 'E'.
    // Also, reject numbers with more than MAX_MANT_DIGITS digits in the mantissa.
    char buf2[100];
    int buflen2 = 0;
    char sep = flags.f.decimal_point ? ',' : '.';
    int mantdigits = 0;
    int expdigits = 0;
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
        } else if (c >= '0' && c <= '9') {
            if (in_mant) {
                if (++mantdigits > MAX_MANT_DIGITS)
                    return 5;
                if (c != '0')
                    zero = false;
            } else {
                expdigits++;
            }
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

    if (!in_mant && expdigits == 0)
        buf2[buflen2++] = '0';

    buf2[buflen2] = 0;
    BID_UINT128 b;
    bid128_from_string(&b, buf2);
    int r;
    if (bid128_isInf(&r, &b), r)
        return (bid128_isSigned(&r, &b), r) ? 2 : 1;
    if (!zero && (bid128_isZero(&r, &b), r))
        return (bid128_isSigned(&r, &b), r) ? 4 : 3;
    *d = b;
    return 0;
}

/* public */
Phloat::Phloat(const char *str) {
    bid128_from_string(&val, (char *) str);
}

/* public */
Phloat::Phloat(int numer, int denom) {
    BID_UINT128 n, d;
    bid128_from_int32(&n, &numer);
    bid128_from_int32(&d, &denom);
    bid128_div(&val, &n, &d);
}

/* public */
Phloat::Phloat(int8 numer, int8 denom) {
    BID_UINT128 n, d;
    bid128_from_int64(&n, &numer);
    bid128_from_int64(&d, &denom);
    bid128_div(&val, &n, &d);
}

/* public */
Phloat::Phloat(int i) {
    bid128_from_int32(&val, &i);
}

/* public */
Phloat::Phloat(int8 i) {
    bid128_from_int64(&val, &i);
}

/* public */
Phloat::Phloat(uint8 i) {
    bid128_from_uint64(&val, &i);
}

/* public */
Phloat::Phloat(double d) {
    BID_UINT64 tmp;
    binary64_to_bid64(&tmp, &d);
    bid64_to_bid128(&val, &tmp);
}

/* public */
Phloat::Phloat(const Phloat &p) {
    val = p.val;
}

/* public */
Phloat Phloat::operator=(int i) {
    bid128_from_int32(&val, &i);
    return *this;
}

/* public */
Phloat Phloat::operator=(int8 i) {
    bid128_from_int64(&val, &i);
    return *this;
}

/* public */
Phloat Phloat::operator=(uint8 i) {
    bid128_from_uint64(&val, &i);
    return *this;
}

/* public */
Phloat Phloat::operator=(double d) {
    BID_UINT64 tmp;
    binary64_to_bid64(&tmp, &d);
    bid64_to_bid128(&val, &tmp);
    return *this;
}

/* public */
Phloat Phloat::operator=(Phloat p) {
    val = p.val;
    return *this;
}

/* public */
bool Phloat::operator==(Phloat p) const {
    int r;
    bid128_quiet_equal(&r, (BID_UINT128 *) &val, &p.val);
    return r != 0;
}

/* public */
bool Phloat::operator!=(Phloat p) const {
    int r;
    bid128_quiet_not_equal(&r, (BID_UINT128 *) &val, &p.val);
    return r != 0;
}

/* public */
bool Phloat::operator<(Phloat p) const {
    int r;
    bid128_quiet_less(&r, (BID_UINT128 *) &val, &p.val);
    return r != 0;
}

/* public */
bool Phloat::operator<=(Phloat p) const {
    int r;
    bid128_quiet_less_equal(&r, (BID_UINT128 *) &val, &p.val);
    return r != 0;
}

/* public */
bool Phloat::operator>(Phloat p) const {
    int r;
    bid128_quiet_greater(&r, (BID_UINT128 *) &val, &p.val);
    return r != 0;
}

/* public */
bool Phloat::operator>=(Phloat p) const {
    int r;
    bid128_quiet_greater_equal(&r, (BID_UINT128 *) &val, &p.val);
    return r != 0;
}

/* public */
Phloat Phloat::operator-() const {
    BID_UINT128 res;
    bid128_negate(&res, (BID_UINT128 *) &val);
    return Phloat(res);
}

/* public */
Phloat Phloat::operator*(Phloat p) const {
    BID_UINT128 res;
    bid128_mul(&res, (BID_UINT128 *) &val, &p.val);
    return Phloat(res);
}

/* public */
Phloat Phloat::operator/(Phloat p) const {
    BID_UINT128 res;
    bid128_div(&res, (BID_UINT128 *) &val, &p.val);
    return Phloat(res);
}

/* public */
Phloat Phloat::operator+(Phloat p) const {
    BID_UINT128 res;
    bid128_add(&res, (BID_UINT128 *) &val, &p.val);
    return Phloat(res);
}

/* public */
Phloat Phloat::operator-(Phloat p) const {
    BID_UINT128 res;
    bid128_sub(&res, (BID_UINT128 *) &val, &p.val);
    return Phloat(res);
}

/* public */
Phloat Phloat::operator*=(Phloat p) {
    BID_UINT128 res;
    bid128_mul(&res, &val, &p.val);
    val = res;
    return *this;
}

/* public */
Phloat Phloat::operator/=(Phloat p) {
    BID_UINT128 res;
    bid128_div(&res, &val, &p.val);
    val = res;
    return *this;
}

/* public */
Phloat Phloat::operator+=(Phloat p) {
    BID_UINT128 res;
    bid128_add(&res, &val, &p.val);
    val = res;
    return *this;
}

/* public */
Phloat Phloat::operator-=(Phloat p) {
    BID_UINT128 res;
    bid128_sub(&res, &val, &p.val);
    val = res;
    return *this;
}

/* public */
Phloat Phloat::operator++() {
    // prefix
    BID_UINT128 one;
    int d1 = 1;
    bid128_from_int32(&one, &d1);
    BID_UINT128 temp;
    bid128_add(&temp, &val, &one);
    val = temp;
    return *this;
}

/* public */
Phloat Phloat::operator++(int) {
    // postfix
    Phloat old = *this;
    BID_UINT128 one;
    int d1 = 1;
    bid128_from_int32(&one, &d1);
    bid128_add(&val, &old.val, &one);
    return old;
}

/* public */
Phloat Phloat::operator--() {
    // prefix
    BID_UINT128 one;
    int d1 = 1;
    bid128_from_int32(&one, &d1);
    BID_UINT128 temp;
    bid128_sub(&temp, &val, &one);
    val = temp;
    return *this;
}

/* public */
Phloat Phloat::operator--(int) {
    // postfix
    Phloat old = *this;
    BID_UINT128 one;
    int d1 = 1;
    bid128_from_int32(&one, &d1);
    bid128_sub(&val, &old.val, &one);
    return old;
}

int p_isinf(Phloat p) {
    int r;
    if (bid128_isInf(&r, &p.val), r)
        return (bid128_isSigned(&r, &p.val), r) ? -1 : 1;
    else
        return 0;
}

int p_isnan(Phloat p) {
    int r;
    bid128_isNaN(&r, &p.val);
    return r;
}

int to_digit(Phloat p) {
    BID_UINT128 ten, res;
    int d10 = 10;
    int ires;
    bid128_from_int32(&ten, &d10);
    bid128_rem(&res, &p.val, &ten);
    int numer_sign, res_sign;
    bid128_isSigned(&numer_sign, &p.val);
    bid128_isSigned(&res_sign, &res);
    if (numer_sign ^ res_sign) {
        BID_UINT128 r2;
        if (res_sign)
            bid128_add(&r2, &res, &ten);
        else
            bid128_sub(&r2, &res, &ten);
        bid128_to_int32_xint(&ires, &r2);
    } else
        bid128_to_int32_xint(&ires, &res);
    return ires;
}

char to_char(Phloat p) {
    int4 res;
    bid128_to_int32_xint(&res, &p.val);
    return (char) res;
}

int to_int(Phloat p) {
    int4 res;
    bid128_to_int32_xint(&res, &p.val);
    return (int) res;
}

int4 to_int4(Phloat p) {
    int4 res;
    bid128_to_int32_xint(&res, &p.val);
    return res;
}

int8 to_int8(Phloat p) {
    int8 res;
    bid128_to_int64_xint(&res, &p.val);
    return res;
}

uint8 to_uint8(Phloat p) {
    uint8 res;
    bid128_to_uint64_xint(&res, &p.val);
    return res;
}

double to_double(Phloat p) {
    double res;
    bid128_to_binary64(&res, &p.val);
    return res;
}

Phloat sin(Phloat p) {
    BID_UINT128 res;
    bid128_sin(&res, &p.val);
    return Phloat(res);
}

Phloat cos(Phloat p) {
    BID_UINT128 res;
    bid128_cos(&res, &p.val);
    return Phloat(res);
}

Phloat tan(Phloat p) {
    BID_UINT128 res;
    bid128_tan(&res, &p.val);
    return Phloat(res);
}

Phloat asin(Phloat p) {
    BID_UINT128 res;
    bid128_asin(&res, &p.val);
    return Phloat(res);
}

Phloat acos(Phloat p) {
    if (p == -1)
        // Intel library bug work-around
        return PI;
    BID_UINT128 res;
    bid128_acos(&res, &p.val);
    return Phloat(res);
}

Phloat atan(Phloat p) {
    BID_UINT128 res;
    bid128_atan(&res, &p.val);
    return Phloat(res);
}

void p_sincos(Phloat phi, Phloat *s, Phloat *c) {
    bid128_sin(&s->val, &phi.val);
    bid128_cos(&c->val, &phi.val);
}

Phloat hypot(Phloat x, Phloat y) {
    BID_UINT128 res;
    bid128_hypot(&res, &x.val, &y.val);
    return Phloat(res);
}

Phloat atan2(Phloat x, Phloat y) {
    BID_UINT128 res;
    bid128_atan2(&res, &x.val, &y.val);
    return Phloat(res);
}

Phloat sinh(Phloat p) {
    BID_UINT128 res;
    bid128_sinh(&res, &p.val);
    return Phloat(res);
}

Phloat cosh(Phloat p) {
    BID_UINT128 res;
    bid128_cosh(&res, &p.val);
    return Phloat(res);
}

Phloat tanh(Phloat p) {
    BID_UINT128 res;
    bid128_tanh(&res, &p.val);
    return Phloat(res);
}

Phloat asinh(Phloat p) {
    BID_UINT128 res;
    bid128_asinh(&res, &p.val);
    return Phloat(res);
}

Phloat acosh(Phloat p) {
    BID_UINT128 res;
    bid128_acosh(&res, &p.val);
    return Phloat(res);
}

Phloat atanh(Phloat p) {
    BID_UINT128 res;
    bid128_atanh(&res, &p.val);
    return Phloat(res);
}

Phloat log(Phloat p) {
    BID_UINT128 res;
    bid128_log(&res, &p.val);
    return Phloat(res);
}

Phloat log1p(Phloat p) {
    BID_UINT128 res;
    bid128_log1p(&res, &p.val);
    return Phloat(res);
}

Phloat log10(Phloat p) {
    BID_UINT128 res;
    bid128_log10(&res, &p.val);
    return Phloat(res);
}

Phloat exp(Phloat p) {
    BID_UINT128 res;
    bid128_exp(&res, &p.val);
    return Phloat(res);
}

Phloat expm1(Phloat p) {
    BID_UINT128 res;
    bid128_expm1(&res, &p.val);
    return Phloat(res);
}

Phloat tgamma(Phloat p) {
    BID_UINT128 res;
    bid128_tgamma(&res, &p.val);
    return Phloat(res);
}

Phloat sqrt(Phloat p) {
    BID_UINT128 res;
    bid128_sqrt(&res, &p.val);
    return Phloat(res);
}

Phloat fmod(Phloat x, Phloat y) {
    BID_UINT128 res;
    bid128_rem(&res, &x.val, &y.val);
    int numer_sign, denom_sign, res_sign;
    bid128_isSigned(&numer_sign, &x.val);
    bid128_isSigned(&denom_sign, &y.val);
    bid128_isSigned(&res_sign, &res);
    if (numer_sign ^ res_sign) {
        BID_UINT128 r2;
        if (denom_sign ^ res_sign)
            bid128_add(&r2, &res, &y.val);
        else
            bid128_sub(&r2, &res, &y.val);
        return Phloat(r2);
    } else
        return Phloat(res);
}

Phloat fabs(Phloat p) {
    BID_UINT128 res;
    bid128_abs(&res, &p.val);
    return Phloat(res);
}

Phloat pow(Phloat y, Phloat x) {
    BID_UINT128 tmp, res;
    bid128_round_integral_negative(&tmp, &x.val);
    int r;
    bid128_quiet_equal(&r, &tmp, &x.val);
    if (r != 0) {
        // Integral power. Use repeated squaring for these, at
        // least as long as the calculations are exact. We make sure
        // of this by scaling the number to make the mantissa the
        // smallest possible integer, and then check whether it
        // grows beyond 10^34-1.
        if (x < -2147483647.0 || x > 2147483647.0)
            goto inexact;
        int4 ex = to_int4(x);
        // Handle base of zero
        bid128_isZero(&r, &y.val);
        if (r != 0) {
            if (ex < 0) {
                BID_UINT128 zero;
                int izero = 0;
                bid128_from_int32(&zero, &izero);
                bid128_div(&res, &zero, &zero); // 0/0 -> NaN
                return res;
            } else if (ex == 0)
                return 1;
            else
                return 0;
        }
        // Handle negative base
        bool result_negative;
        BID_UINT128 yy;
        bid128_isSigned(&r, &y.val);
        if (r != 0) {
            result_negative = (ex & 1) != 0;
            bid128_negate(&yy, &y.val);
        } else {
            result_negative = false;
            yy = y.val;
        }
        // Handle negative exponent
        int ione = 1;
        bid128_from_int32(&res, &ione);
        if (ex < 0) {
            bid128_div(&tmp, &res, &yy);
            yy = tmp;
            ex = -ex;
        }
        // Scale mantissa to smallest possible integer
        int scale;
        bid128_ilogb(&scale, &yy);
        if (scale != 0) {
            scale = -scale;
            bid128_scalbn(&tmp, &yy, &scale);
            scale = -scale;
            yy = tmp;
        }
        while (true) {
            bid128_round_integral_negative(&tmp, &yy);
            bid128_quiet_equal(&r, &tmp, &yy);
            if (r != 0)
                break;
            r = 1;
            bid128_scalbn(&tmp, &yy, &r);
            yy = tmp;
            scale--;
        }
        int8 final_scale = scale;
        final_scale *= ex;
        if (final_scale > 6144 || final_scale < -6209)
            // Out of range, but let bid128_pow() deal with it
            goto inexact;
        scale = (int) final_scale;
        // Only perform repeated squaring if scaled mantissa != 1
        bid128_quiet_equal(&r, &res, &yy);
        if (r == 0) {
            // Check if exponent so large that result can't possibly be exact
            if (ex > 112)
                goto inexact;
            bid128_ilogb(&r, &yy);
            if (ex * r > 33)
                goto inexact;
            // Perform exponentiation by repeated squaring
            while (true) {
                if ((ex & 1) != 0) {
                    bid128_mul(&tmp, &res, &yy);
                    res = tmp;
                    bid128_ilogb(&r, &res);
                    if (r > 33)
                        goto inexact;
                }
                ex >>= 1;
                if (ex == 0)
                    break;
                bid128_mul(&tmp, &yy, &yy);
                yy = tmp;
            }
        }
        bid128_scalbn(&tmp, &res, &scale);
        if (!result_negative)
            return Phloat(tmp);
        bid128_negate(&res, &tmp);
        return Phloat(res);
    } else {
        inexact:
        bid128_pow(&res, &y.val, &x.val);
        return Phloat(res);
    }
}

Phloat floor(Phloat p) {
    BID_UINT128 res;
    bid128_round_integral_negative(&res, &p.val);
    return Phloat(res);
}

Phloat fma(Phloat x, Phloat y, Phloat z) {
    BID_UINT128 res;
    bid128_fma(&res, &x.val, &y.val, &z.val);
    return Phloat(res);
}

Phloat operator*(int x, Phloat y) {
    BID_UINT128 xx, res;
    bid128_from_int32(&xx, &x);
    bid128_mul(&res, &xx, &y.val);
    return Phloat(res);
}

Phloat operator/(int x, Phloat y) {
    BID_UINT128 xx, res;
    bid128_from_int32(&xx, &x);
    bid128_div(&res, &xx, &y.val);
    return Phloat(res);
}

Phloat operator/(double x, Phloat y) {
    BID_UINT128 xx, res;
    BID_UINT64 tmp;
    binary64_to_bid64(&tmp, &x);
    bid64_to_bid128(&xx, &tmp);
    bid128_div(&res, &xx, &y.val);
    return Phloat(res);
}

Phloat operator+(int x, Phloat y) {
    BID_UINT128 xx, res;
    bid128_from_int32(&xx, &x);
    bid128_add(&res, &xx, &y.val);
    return Phloat(res);
}

Phloat operator-(int x, Phloat y) {
    BID_UINT128 xx, res;
    bid128_from_int32(&xx, &x);
    bid128_sub(&res, &xx, &y.val);
    return Phloat(res);
}

bool operator==(int4 x, Phloat y) {
    BID_UINT128 xx;
    bid128_from_int32(&xx, &x);
    int r;
    bid128_quiet_equal(&r, &xx, &y.val);
    return r != 0;
}

Phloat PI("3.141592653589793238462643383279503");

void update_decimal(BID_UINT128 *val) {
    if (state_file_number_format == NUMBER_FORMAT_BID128)
        return;
    short *p = (short *) val;
    if (state_file_number_format == NUMBER_FORMAT_BCD20_OLD)
        bcdfloat_old2new(p);
    char decstr[35];
    bcdfloat2string(p, decstr);
    bid128_from_string(val, decstr);
}


#else // BCD_MATH


void phloat_init() {
    POS_HUGE_PHLOAT = DBL_MAX;
    NEG_HUGE_PHLOAT = -POS_HUGE_PHLOAT;
#ifdef WINDOWS
    double d = 1;
    while (1) {
        double d2 = d / 2;
        if (d2 == 0)
            break;
        d = d2;
    }
    POS_TINY_PHLOAT = d;
    *(int8 *) &NAN_1_PHLOAT = 0x7ff8000000000001;
    *(int8 *) &NAN_2_PHLOAT = 0x7ff8000000000002;
#else
    POS_TINY_PHLOAT = nextafter(0.0, 1.0);
    NAN_1_PHLOAT = nan("1");
    NAN_2_PHLOAT = nan("2");
#endif
    NEG_TINY_PHLOAT = -POS_TINY_PHLOAT;
    NAN_PHLOAT = nan("");
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
            if (++mant_digits > MAX_MANT_DIGITS)
                /* Too many digits! We only allow the user to enter 16 (binary) or 34 (decimal). */
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

double decimal2double(void *data, bool pin_magnitude /* = false */) {
    if (state_file_number_format == NUMBER_FORMAT_BID128) {
        double res;
        BID_UINT128 *b, b2;
        if ((((size_t) data) & 15) != 0) {
            //b2 = *((BID_UINT128 *) data);
            memcpy(&b2, data, 16);
            b = &b2;
        } else
            b = (BID_UINT128 *) data;
        bid128_to_binary64(&res, b);
        if (isnan(res) || !pin_magnitude)
            return res;
        int r;
        if (res == 0 && !(bid128_isZero(&r, b), r))
            return (bid128_isSigned(&r, b), r) ? NEG_TINY_PHLOAT : POS_TINY_PHLOAT;
        int inf = isinf(res);
        return inf == 0 ? res : inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
    }

    // BCD20_OLD or BCD20_NEW
    short *p = (short *) data;
    if (state_file_number_format == NUMBER_FORMAT_BCD20_OLD)
        bcdfloat_old2new(p);

    short exp = p[7];
    bool neg = (exp & 0x8000) != 0;
    double zero = 0;

    if ((exp & 0x4000) != 0) // NaN
        return 0 / zero;
    else if ((exp & 0x2000) != 0) // -Inf or Inf
        if (pin_magnitude)
            return neg ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
        else
            return neg ? -1 / zero : 1 / zero;

    if (p[0] == 0)
        return 0;

    char decstr[35];
    bcdfloat2string(p, decstr);
    double res;
    sscanf(decstr, "%le", &res);
    if (isnan(res) || !pin_magnitude)
        return res;
    else if (res == 0)
        return neg ? NEG_TINY_PHLOAT : POS_TINY_PHLOAT;
    else
        return res;
}


#endif // BCD_MATH


int phloat2string(phloat pd, char *buf, int buflen, int base_mode, int digits,
                         int dispmode, int thousandssep, int max_mant_digits,
                         const char *format) {
    int group1, group2;
    char dec, sep;
    if (format == NULL) {
        dec = flags.f.decimal_point ? '.' : ',';
        sep = flags.f.decimal_point ? ',' : '.';
        group1 = group2 = 3;
    } else {
        dec = format[0];
        thousandssep = format[1] != 0;
        if (thousandssep) {
            sep = format[1];
            group1 = format[2] - '0';
            group2 = format[3] - '0';
        }
    }

    if (pd == 0)
        pd = 0; // Suppress signed zero

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

    /* base_mode: 0=only decimal, 1=all bases, 2=SHOW */
    int base = get_base();
    if (base_mode == 1 && base != 10 || base_mode == 2 && base <= 8) {
        uint8 n;
        int inexact, shift;
        bool too_big = false;
        char binbuf[64];
        int binbufptr = 0;

        int wsize = effective_wsize();
        phloat high, low;
        if (flags.f.base_signed) {
            high = pow(phloat(2), wsize - 1);
            low = -high;
            high--;
        } else {
            high = pow(phloat(2), wsize) - 1;
            low = 0;
        }
        if (pd > high || pd < low) {
            if (flags.f.base_wrap) {
                too_big = true;
                phloat ipd = pd < 0 ? -floor(-pd) : floor(pd);
                inexact = base_mode == 1 && pd != ipd;
                phloat d = pow(phloat(2), wsize);
                phloat r = fmod(ipd, d);
                if (r < 0)
                    r += d;
                n = (int8) to_uint8(r);
                if (flags.f.base_signed) {
                    int8 m = 1LL << (wsize - 1);
                    if ((n & m) != 0)
                        n |= -1LL << (wsize - 1);
                    else
                        n &= (1LL << (wsize - 1)) - 1;
                } else {
                    if (wsize < 64)
                        n &= (1ULL << wsize) - 1;
                }
            } else {
                if (base_mode == 2)
                    goto decimal_after_all;
                else {
                    if (!flags.f.base_signed && pd < 0)
                        string2buf(buf, buflen, &chars_so_far, "<Negative>", 10);
                    else
                        string2buf(buf, buflen, &chars_so_far, "<Too Big>", 9);
                    return chars_so_far;
                }
            }
        } else {
            if (flags.f.base_signed) {
                int8 sn = to_int8(pd);
                inexact = base_mode == 1 && pd != sn;
                n = (uint8) sn;
            } else {
                n = to_uint8(pd);
                inexact = base_mode == 1 && pd != n;
            }
        }
        if (wsize < 64)
            n &= (1ULL << wsize) - 1;

        uint8 mask;
        mask = 0xfffff00000000000ULL;
        if (inexact)
            mask <<= 1;
        if (too_big)
            mask <<= 1;
        if (base_mode == 2 && base == 2 && (n & mask) != 0) {
            // More than 44 bits; won't fit. Use hex instead.
            string2buf(buf, buflen, &chars_so_far, "hex ", 4);
            base = 16;
        }
        shift = base == 2 ? 1 : base == 8 ? 3 : 4;
        while (n != 0) {
            int digit = (int) (n & (base - 1));
            char c = digit < 10 ? '0' + digit : 'A' + digit - 10;
            binbuf[binbufptr++] = c;
            n >>= shift;
        }
        if (binbufptr == 0)
            binbuf[binbufptr++] = '0';

        if (too_big)
            char2buf(buf, buflen, &chars_so_far, 26);
        while (binbufptr > 0)
            char2buf(buf, buflen, &chars_so_far, binbuf[--binbufptr]);
        if (inexact)
            char2buf(buf, buflen, &chars_so_far, dec);
        return chars_so_far;

        decimal_after_all:;
    }

    char bcd_mantissa[MAX_MANT_DIGITS];
    memset(bcd_mantissa, 0, MAX_MANT_DIGITS);
    int bcd_exponent = 0;
    int bcd_mantissa_sign = 0;

    char decstr[50];

#ifndef BCD_MATH
    double d = to_double(pd);
    sprintf(decstr, "%.15e", d);
#else
    bid128_to_string(decstr, &pd.val);
#endif

    char *p = decstr;
    int mant_index = 0;
    bcd_mantissa_sign = 0;
    bool seen_dot = false;
    bool in_leading_zeroes = true;
    int exp_offset = -1;

    while (*p != 0) {
        char c = *p++;
        if (c == '-') {
            bcd_mantissa_sign = 1;
            continue;
        }
        if (c == '+')
            continue;
        if (c == '.') {
            seen_dot = true;
            continue;
        }
        if (c == 'e' || c == 'E') {
            if (!in_leading_zeroes) {
                sscanf(p, "%d", &bcd_exponent);
                bcd_exponent += exp_offset;
            }
            break;
        }
        // Can only be decimal digit at this point
        if (c == '0') {
            if (in_leading_zeroes)
                continue;
        } else
            in_leading_zeroes = false;
        if (!seen_dot)
            exp_offset++;
        if (mant_index < MAX_MANT_DIGITS)
            bcd_mantissa[mant_index++] = c - '0';
    }

    int max_int_digits = max_mant_digits;
    int max_frac_digits = MAX_MANT_DIGITS + max_int_digits - 1;

    if (dispmode == 0 || dispmode == 3) {

        /* FIX and ALL modes */

        char norm_ip[MAX_MANT_DIGITS];
        memset(norm_ip, 0, max_int_digits);
        char norm_fp[MAX_MANT_DIGITS * 2 - 1];
        memset(norm_fp, 0, max_frac_digits);

        int i;
        int int_digits, frac_digits;

        int digits2;
        if (dispmode == 0)
            digits2 = digits;
        else
            digits2 = max_int_digits - 1;

        if (bcd_exponent >= max_int_digits || -bcd_exponent > digits2 + 1)
            goto do_sci;
        for (i = 0; i < MAX_MANT_DIGITS; i++) {
            if (i <= bcd_exponent)
                norm_ip[max_int_digits - 1 - bcd_exponent + i] = bcd_mantissa[i];
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
            int visdigits = max_int_digits - 1 - bcd_exponent;
            if (visdigits > digits)
                visdigits = digits;
            carry = norm_fp[visdigits] >= 5;
            for (i = visdigits; i < max_frac_digits; i++)
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
            for (i = max_int_digits - 1; i >= 0; i--) {
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
        } else if (max_int_digits < MAX_MANT_DIGITS) {
            /* ALL mode: for HP-42S compatibility, round to max_int_digits
             * digits before proceeding.
             */
            int f = 1000;
            for (i = 0; i < max_int_digits + max_frac_digits; i++) {
                char c = i < max_int_digits ? norm_ip[i] : norm_fp[i - max_int_digits];
                if (c != 0 && f == 1000)
                    f = i;
                if (i == f + max_int_digits) {
                    int carry = c >= 5;
                    if (carry) {
                        int j;
                        for (j = i - 1; j >= 0; j--) {
                            char c2 = j < max_int_digits ? norm_ip[j] : norm_fp[j - max_int_digits];
                            c2++;
                            if (c2 < 10)
                                carry = 0;
                            else {
                                c2 -= 10;
                                carry = 1;
                            }
                            if (j < max_int_digits)
                                norm_ip[j] = c2;
                            else
                                norm_fp[j - max_int_digits] = c2;
                            if (!carry)
                                break;
                        }
                        if (carry)
                            /* Rounding is making the integer part max_int_digits + 1 digits
                             * long; must go to SCI mode.
                             */
                            goto do_sci;
                    }
                }
                if (i >= f + max_int_digits) {
                    if (i < max_int_digits)
                        norm_ip[i] = 0;
                    else
                        norm_fp[i - max_int_digits] = 0;
                }
            }
        }

        /* Check if the number is still within bounds for FIX or ALL */
        if (pd != 0) {
            /* Make sure that nonzero numbers are not
             * displayed as zero because of the rounding.
             */
            for (i = 0; i < max_int_digits; i++)
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
                for (i = max_int_digits - 1; i < max_frac_digits; i++)
                    if (norm_fp[i] != 0)
                        goto do_sci;
            }
        }

        int_digits = 1;
        for (i = 0; i < max_int_digits; i++)
            if (norm_ip[i] != 0) {
                int_digits = max_int_digits - i;
                break;
            }

        if (bcd_mantissa_sign)
            char2buf(buf, buflen, &chars_so_far, '-');

        for (i = int_digits - 1; i >= 0; i--) {
            if (thousandssep && i >= group1 - 1 && (i - group1 + 1) % group2 == 0 && i != int_digits - 1)
                if (sep == ' ')
                    // U+2009: Unicode thin space
                    string2buf(buf, buflen, &chars_so_far, "\342\200\211", 3);
                else
                    char2buf(buf, buflen, &chars_so_far, sep);
            char2buf(buf, buflen, &chars_so_far, (char)('0' + norm_ip[max_int_digits - 1 - i]));
        }

        if (dispmode == 0)
            frac_digits = digits;
        else {
            frac_digits = 0;
            for (i = 0; i < max_frac_digits; i++)
                if (norm_fp[i] != 0)
                    frac_digits = i + 1;
        }
        if (frac_digits + int_digits > max_int_digits)
            frac_digits = max_int_digits - int_digits;

        if (frac_digits > 0 || (dispmode == 0 && thousandssep)) {
            char2buf(buf, buflen, &chars_so_far, dec);
            for (i = 0; i < frac_digits; i++)
                char2buf(buf, buflen, &chars_so_far, (char) ('0' + norm_fp[i]));
        }

        return chars_so_far;

    } else {

        /* SCI and ENG modes */
        /* Also fall-through from FIX and ALL */

        int m_digits;
        int carry;
        char norm_mantissa[MAX_MANT_DIGITS];
        int norm_exponent, e3;
        int i;

        do_sci:

        for (i = 0; i < MAX_MANT_DIGITS; i++)
            norm_mantissa[i] = bcd_mantissa[i];
        norm_exponent = bcd_exponent;

        if (dispmode == 3) {
            /* Round to max_int_digits digits before doing anything else;
             * this is needed to handle mantissas like 9.99999999999999,
             * which would otherwise end up getting displayed as
             * 10.0000000000 instead of 10.
             */

            sci_all_round:

            carry = max_int_digits < MAX_MANT_DIGITS
                    && norm_mantissa[max_int_digits] >= 5;
            for (i = max_int_digits; i < MAX_MANT_DIGITS; i++)
                norm_mantissa[i] = 0;
            if (carry) {
                for (i = max_int_digits - 1; i >= 0; i--) {
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
                for (i = 0; i < MAX_MANT_DIGITS - 1; i++)
                    norm_mantissa[i + 1] = bcd_mantissa[i];
                norm_mantissa[0] = 0;
                norm_exponent = bcd_exponent + 1;
                goto sci_all_round;
            }
            m_digits = 0;
            for (i = max_int_digits - 1; i >= 0; i--)
                if (norm_mantissa[i] != 0) {
                    m_digits = i;
                    break;
                }
        } else {
            m_digits = digits;

            sci_round:
            carry = m_digits + 1 < MAX_MANT_DIGITS
                    && norm_mantissa[m_digits + 1] >= 5;
            for (i = m_digits + 1; i < MAX_MANT_DIGITS; i++)
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
                for (i = 0; i < MAX_MANT_DIGITS - 1; i++)
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
                char2buf(buf, buflen, &chars_so_far, dec);
        }

        char2buf(buf, buflen, &chars_so_far, 24);
        i = int2string(norm_exponent, buf + chars_so_far,
                                            buflen - chars_so_far);
        chars_so_far += i;

        return chars_so_far;
    }
}
