/*****************************************************************************
 * Free42 -- a free HP-42S calculator clone
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

#ifndef CORE_PHLOAT_H
#define CORE_PHLOAT_H 1


#include "free42.h"

class phloat {
    public:
	union {
	    // For binary mode
	    double d;
	    // For decimal mode
	    unsigned char b[8];
	    // For storing user-mode strings
	    struct {
		char text[6];
		unsigned char length;
	    } s;
	} ph;

	phloat() {}
	phloat(int numer, int denom) PHLOAT_SECT;
	phloat(int i) PHLOAT_SECT;
	phloat(int8 i) PHLOAT_SECT;
	phloat(double d) PHLOAT_SECT;
	phloat(const phloat &p) PHLOAT_SECT;
	phloat operator=(int i) PHLOAT_SECT;
	phloat operator=(int8 i) PHLOAT_SECT;
	phloat operator=(double d) PHLOAT_SECT;
	phloat operator=(phloat p) PHLOAT_SECT;

#ifdef PALMOS
	phloat(int4 i) PHLOAT_SECT;
	phloat operator=(int4 i) PHLOAT_SECT;
#endif

	// We don't define type cast operators, because they just lead
	// to tons of ambiguities. Defining explicit conversions instead.
	// Note that these conversion routines assume that the value to be
	// converted actually fits in the returned type; if not, the result
	// is undefined, except for to_char(), which will handle the range
	// -128..255 correctly.
	int to_digit() const PHLOAT_SECT; // Returns digit in units position
	char to_char() const PHLOAT_SECT;
	int to_int() const PHLOAT_SECT;
	int4 to_int4() const PHLOAT_SECT;
	int8 to_int8() const PHLOAT_SECT;
	double to_double() const PHLOAT_SECT;

	bool operator==(phloat p) const PHLOAT_SECT;
	bool operator!=(phloat p) const PHLOAT_SECT;
	bool operator<(phloat p) const PHLOAT_SECT;
	bool operator<=(phloat p) const PHLOAT_SECT;
	bool operator>(phloat p) const PHLOAT_SECT;
	bool operator>=(phloat p) const PHLOAT_SECT;
	phloat operator-() const PHLOAT_SECT;
	phloat operator*(phloat p) const PHLOAT_SECT;
	phloat operator/(phloat p) const PHLOAT_SECT;
	phloat operator+(phloat p) const PHLOAT_SECT;
	phloat operator-(phloat p) const PHLOAT_SECT;
	phloat operator*=(phloat p) PHLOAT_SECT;
	phloat operator/=(phloat p) PHLOAT_SECT;
	phloat operator+=(phloat p) PHLOAT_SECT;
	phloat operator-=(phloat p) PHLOAT_SECT;
	phloat operator++() PHLOAT_SECT; // prefix
	phloat operator++(int) PHLOAT_SECT; // postfix
	phloat operator--() PHLOAT_SECT; // prefix
	phloat operator--(int) PHLOAT_SECT; // postfix

	// Switch phloat object from dec to bin, or vice versa
	void dec2bin() PHLOAT_SECT;
	void bin2dec() PHLOAT_SECT;
};

// I can't simply overload isinf() and isnan(), because the Linux math.h
// defines them as macros.
int p_isinf(phloat p) PHLOAT_SECT;
int p_isnan(phloat p) PHLOAT_SECT;

phloat sin(phloat p) PHLOAT_SECT;
phloat cos(phloat p) PHLOAT_SECT;
phloat tan(phloat p) PHLOAT_SECT;
phloat asin(phloat p) PHLOAT_SECT;
phloat acos(phloat p) PHLOAT_SECT;
phloat atan(phloat p) PHLOAT_SECT;
void sincos(phloat phi, phloat *s, phloat *c) PHLOAT_SECT;
phloat hypot(phloat x, phloat y) PHLOAT_SECT;
phloat atan2(phloat x, phloat y) PHLOAT_SECT;
phloat sinh(phloat p) PHLOAT_SECT;
phloat cosh(phloat p) PHLOAT_SECT;
phloat tanh(phloat p) PHLOAT_SECT;
phloat asinh(phloat p) PHLOAT_SECT;
phloat acosh(phloat p) PHLOAT_SECT;
phloat atanh(phloat p) PHLOAT_SECT;
phloat log(phloat p) PHLOAT_SECT;
phloat log1p(phloat p) PHLOAT_SECT;
phloat log10(phloat p) PHLOAT_SECT;
phloat exp(phloat p) PHLOAT_SECT;
phloat expm1(phloat p) PHLOAT_SECT;
phloat sqrt(phloat p) PHLOAT_SECT;
phloat fmod(phloat x, phloat y) PHLOAT_SECT;
phloat fabs(phloat p) PHLOAT_SECT;
phloat pow(phloat x, phloat y) PHLOAT_SECT;
phloat floor(phloat x) PHLOAT_SECT;

phloat operator*(int x, phloat y) PHLOAT_SECT;
phloat operator/(int x, phloat y) PHLOAT_SECT;
phloat operator/(double x, phloat y) PHLOAT_SECT;
phloat operator+(int x, phloat y) PHLOAT_SECT;
phloat operator-(int x, phloat y) PHLOAT_SECT;
bool operator==(int4 x, phloat y) PHLOAT_SECT;

#endif
