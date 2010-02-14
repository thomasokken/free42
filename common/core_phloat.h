/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2010  Thomas Okken
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
#include "bcdfloat.h"
#endif

// A little hack to allow storing 6-character strings in a phloat
struct hp_string {
    char text[6];
    unsigned char length;
};
#define phloat_text(x) (((hp_string *) &(x))->text)
#define phloat_length(x) (((hp_string *) &(x))->length)


#ifndef BCD_MATH


#define phloat double

#define p_isinf isinf
#define p_isnan isnan
#define to_digit(x) ((int) fmod((x), 10.0))
#define to_char(x) ((char) (x))
#define to_int(x) ((int) (x))
#define to_int4(x) ((int4) (x))
#define to_int8(x) ((int8) (x))
#define to_double(x) ((double) (x))

#define PI 3.1415926535897932384626433
#define P 7
double bcd2double(short *p, bool old_bcd);


#else // BCD_MATH


#define phloat Phloat

class Phloat {
    public:
	BCDFloat bcd;

	Phloat() {}
	Phloat(BCDFloat b) : bcd(b) {}
	Phloat(int numer, int denom) PHLOAT_SECT;
	Phloat(int i) PHLOAT_SECT;
	Phloat(int8 i) PHLOAT_SECT;
	Phloat(double d) PHLOAT_SECT;
	Phloat(const Phloat &p) PHLOAT_SECT;
	Phloat operator=(int i) PHLOAT_SECT;
	Phloat operator=(int8 i) PHLOAT_SECT;
	Phloat operator=(double d) PHLOAT_SECT;
	Phloat operator=(Phloat p) PHLOAT_SECT;

#if defined(PALMOS) && !defined(PALMOS_ARM)
	Phloat(int4 i) PHLOAT_SECT;
	Phloat operator=(int4 i) PHLOAT_SECT;
#endif

	bool operator==(Phloat p) const PHLOAT_SECT;
	bool operator!=(Phloat p) const PHLOAT_SECT;
	bool operator<(Phloat p) const PHLOAT_SECT;
	bool operator<=(Phloat p) const PHLOAT_SECT;
	bool operator>(Phloat p) const PHLOAT_SECT;
	bool operator>=(Phloat p) const PHLOAT_SECT;
	Phloat operator-() const PHLOAT_SECT;
	Phloat operator*(Phloat p) const PHLOAT_SECT;
	Phloat operator/(Phloat p) const PHLOAT_SECT;
	Phloat operator+(Phloat p) const PHLOAT_SECT;
	Phloat operator-(Phloat p) const PHLOAT_SECT;
	Phloat operator*=(Phloat p) PHLOAT_SECT;
	Phloat operator/=(Phloat p) PHLOAT_SECT;
	Phloat operator+=(Phloat p) PHLOAT_SECT;
	Phloat operator-=(Phloat p) PHLOAT_SECT;
	Phloat operator++() PHLOAT_SECT; // prefix
	Phloat operator++(int) PHLOAT_SECT; // postfix
	Phloat operator--() PHLOAT_SECT; // prefix
	Phloat operator--(int) PHLOAT_SECT; // postfix
};

// I can't simply overload isinf() and isnan(), because the Linux math.h
// defines them as macros.
int p_isinf(Phloat p) PHLOAT_SECT;
int p_isnan(Phloat p) PHLOAT_SECT;

// We don't define type cast operators, because they just lead
// to tons of ambiguities. Defining explicit conversions instead.
// Note that these conversion routines assume that the value to be
// converted actually fits in the returned type; if not, the result
// is undefined, except for to_char(), which will handle the range
// -128..255 correctly.
int to_digit(Phloat p) PHLOAT_SECT; // Returns digit in units position
char to_char(Phloat p) PHLOAT_SECT;
int to_int(Phloat p) PHLOAT_SECT;
int4 to_int4(Phloat p) PHLOAT_SECT;
int8 to_int8(Phloat p) PHLOAT_SECT;
double to_double(Phloat p) PHLOAT_SECT;

Phloat sin(Phloat p) PHLOAT_SECT;
Phloat cos(Phloat p) PHLOAT_SECT;
Phloat tan(Phloat p) PHLOAT_SECT;
Phloat asin(Phloat p) PHLOAT_SECT;
Phloat acos(Phloat p) PHLOAT_SECT;
Phloat atan(Phloat p) PHLOAT_SECT;
void sincos(Phloat phi, Phloat *s, Phloat *c) PHLOAT_SECT;
Phloat hypot(Phloat x, Phloat y) PHLOAT_SECT;
Phloat atan2(Phloat x, Phloat y) PHLOAT_SECT;
Phloat sinh(Phloat p) PHLOAT_SECT;
Phloat cosh(Phloat p) PHLOAT_SECT;
Phloat tanh(Phloat p) PHLOAT_SECT;
Phloat asinh(Phloat p) PHLOAT_SECT;
Phloat acosh(Phloat p) PHLOAT_SECT;
Phloat atanh(Phloat p) PHLOAT_SECT;
Phloat log(Phloat p) PHLOAT_SECT;
Phloat log1p(Phloat p) PHLOAT_SECT;
Phloat log10(Phloat p) PHLOAT_SECT;
Phloat exp(Phloat p) PHLOAT_SECT;
Phloat expm1(Phloat p) PHLOAT_SECT;
Phloat gamma(Phloat p) PHLOAT_SECT;
Phloat sqrt(Phloat p) PHLOAT_SECT;
Phloat fmod(Phloat x, Phloat y) PHLOAT_SECT;
Phloat fabs(Phloat p) PHLOAT_SECT;
Phloat pow(Phloat x, Phloat y) PHLOAT_SECT;
Phloat floor(Phloat x) PHLOAT_SECT;

Phloat operator*(int x, Phloat y) PHLOAT_SECT;
Phloat operator/(int x, Phloat y) PHLOAT_SECT;
Phloat operator/(double x, Phloat y) PHLOAT_SECT;
Phloat operator+(int x, Phloat y) PHLOAT_SECT;
Phloat operator-(int x, Phloat y) PHLOAT_SECT;
bool operator==(int4 x, Phloat y) PHLOAT_SECT;

extern Phloat PI;

BCDFloat double2bcd(double d, bool round = false) PHLOAT_SECT;
double bcd2double(BCDFloat b, bool old_bcd) PHLOAT_SECT;


#endif // BCD_MATH


extern phloat POS_HUGE_PHLOAT;
extern phloat NEG_HUGE_PHLOAT;
extern phloat POS_TINY_PHLOAT;
extern phloat NEG_TINY_PHLOAT;

void phloat_init() PHLOAT_SECT;
void phloat_cleanup() PHLOAT_SECT;
int phloat2string(phloat d, char *buf, int buflen,
		  int base_mode, int digits, int dispmode,
		  int thousandssep) PHLOAT_SECT;
int string2phloat(const char *buf, int buflen, phloat *d) PHLOAT_SECT;

void bcdfloat_old2new(void *bcd) PHLOAT_SECT;


#endif
