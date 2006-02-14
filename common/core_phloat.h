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
	phloat(int numer, int denom);
	phloat(int i);
	phloat(int8 i);
	phloat(double d);
	phloat(const phloat &p);
	phloat operator=(int i);
	phloat operator=(int8 i);
	phloat operator=(double d);
	phloat operator=(phloat p);

#ifdef PALMOS
	phloat(int4 i);
	phloat operator=(int4 i);
#endif

	// We don't define type cast operators, because they just lead
	// to tons of ambiguities. Defining explicit conversions instead.
	// Note that these conversion routines assume that the value to be
	// converted actually fits in the returned type; if not, the result
	// is undefined, except for to_char(), which will handle the range
	// -128..255 correctly.
	int to_digit() const; // Returns digit in units position
	char to_char() const;
	int to_int() const;
	int4 to_int4() const;
	int8 to_int8() const;
	double to_double() const;

	bool operator==(phloat p) const;
	bool operator!=(phloat p) const;
	bool operator<(phloat p) const;
	bool operator<=(phloat p) const;
	bool operator>(phloat p) const;
	bool operator>=(phloat p) const;
	phloat operator-() const;
	phloat operator*(phloat p) const;
	phloat operator/(phloat p) const;
	phloat operator+(phloat p) const;
	phloat operator-(phloat p) const;
	phloat operator*=(phloat p);
	phloat operator/=(phloat p);
	phloat operator+=(phloat p);
	phloat operator-=(phloat p);
	phloat operator++(); // prefix
	phloat operator++(int); // postfix
	phloat operator--(); // prefix
	phloat operator--(int); // postfix

	// Switch phloat object from dec to bin, or vice versa
	void dec2bin();
	void bin2dec();
};

// I can't simply overload isinf() and isnan(), because the Linux math.h
// defines them as macros.
int p_isinf(phloat p);
int p_isnan(phloat p);

phloat sin(phloat p);
phloat cos(phloat p);
phloat tan(phloat p);
phloat asin(phloat p);
phloat acos(phloat p);
phloat atan(phloat p);
void sincos(phloat phi, phloat *s, phloat *c);
phloat hypot(phloat x, phloat y);
phloat atan2(phloat x, phloat y);
phloat sinh(phloat p);
phloat cosh(phloat p);
phloat tanh(phloat p);
phloat asinh(phloat p);
phloat acosh(phloat p);
phloat atanh(phloat p);
phloat log(phloat p);
phloat log1p(phloat p);
phloat log10(phloat p);
phloat exp(phloat p);
phloat expm1(phloat p);
phloat sqrt(phloat p);
phloat fmod(phloat x, phloat y);
phloat fabs(phloat p);
phloat pow(phloat x, phloat y);
phloat floor(phloat x);

phloat operator*(int x, phloat y);
phloat operator/(int x, phloat y);
phloat operator/(double x, phloat y);
phloat operator+(int x, phloat y);
phloat operator-(int x, phloat y);
bool operator==(int4 x, phloat y);

#endif
