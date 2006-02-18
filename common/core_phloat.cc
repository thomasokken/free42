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

#include <math.h>
#include "core_phloat.h"


#ifndef PHLOAT_IS_DOUBLE

/* public */
Phloat::Phloat(int numer, int denom) {
    // PHLOAT_TODO: This constructor is meant to be used to allow exact decimal
    // number literals, e.g. Phloat(4, 100) to represent 0.04 exactly.
    // Because of this, it should always be created as a decimal-mode Phloat;
    // it should only be *used* in decimal mode, and its dec2bin() and
    // bin2dec() methods should never be called.
    ph.d = numer / denom;
}

/* public */
Phloat::Phloat(int i) {
    ph.d = i;
}

/* public */
Phloat::Phloat(int8 i) {
    ph.d = (double) i;
}

/* public */
Phloat::Phloat(double d) {
    ph.d = d;
}

/* public */
Phloat::Phloat(const Phloat &p) {
    ph = p.ph;
}

/* public */
Phloat Phloat::operator=(int i) {
    ph.d = i;
    return *this;
}

/* public */
Phloat Phloat::operator=(int8 i) {
    ph.d = (double) i;
    return *this;
}

/* public */
Phloat Phloat::operator=(double d) {
    ph.d = d;
    return *this;
}

/* public */
Phloat Phloat::operator=(Phloat p) {
    ph = p.ph;
    return *this;
}

#ifdef PALMOS

/* public */
Phloat::Phloat(int4 i) {
    ph.d = i;
}

/* public */
Phloat Phloat::operator=(int4 i) {
    ph.d = i;
    return *this;
}

#endif

/* public */
bool Phloat::operator==(Phloat p) const {
    return ph.d == p.ph.d;
}

/* public */
bool Phloat::operator!=(Phloat p) const {
    return ph.d != p.ph.d;
}

/* public */
bool Phloat::operator<(Phloat p) const {
    return ph.d < p.ph.d;
}

/* public */
bool Phloat::operator<=(Phloat p) const {
    return ph.d <= p.ph.d;
}

/* public */
bool Phloat::operator>(Phloat p) const {
    return ph.d > p.ph.d;
}

/* public */
bool Phloat::operator>=(Phloat p) const {
    return ph.d >= p.ph.d;
}

/* public */
Phloat Phloat::operator-() const {
    return Phloat(-ph.d);
}

/* public */
Phloat Phloat::operator*(Phloat p) const {
    return ph.d * p.ph.d;
}

/* public */
Phloat Phloat::operator/(Phloat p) const {
    return ph.d / p.ph.d;
}

/* public */
Phloat Phloat::operator+(Phloat p) const {
    return ph.d + p.ph.d;
}

/* public */
Phloat Phloat::operator-(Phloat p) const {
    return ph.d - p.ph.d;
}

/* public */
Phloat Phloat::operator*=(Phloat p) {
    ph.d *= p.ph.d;
    return *this;
}

/* public */
Phloat Phloat::operator/=(Phloat p) {
    ph.d /= p.ph.d;
    return *this;
}

/* public */
Phloat Phloat::operator+=(Phloat p) {
    ph.d += p.ph.d;
    return *this;
}

/* public */
Phloat Phloat::operator-=(Phloat p) {
    ph.d -= p.ph.d;
    return *this;
}

/* public */
Phloat Phloat::operator++() {
    // prefix
    ++ph.d;
    return *this;
}

/* public */
Phloat Phloat::operator++(int) {
    // postfix
    Phloat old = *this;
    ++ph.d;
    return old;
}

/* public */
Phloat Phloat::operator--() {
    // prefix
    --ph.d;
    return *this;
}

/* public */
Phloat Phloat::operator--(int) {
    // postfix
    Phloat old = *this;
    --ph.d;
    return old;
}

/* public */
void Phloat::dec2bin() {
    //
}

/* public */
void Phloat::bin2dec() {
    //
}

int p_isinf(Phloat p) {
    return isinf(p.ph.d);
}

int p_isnan(Phloat p) {
    return isnan(p.ph.d);
}

int to_digit(Phloat p) {
    // TODO: more efficient implementation for decimal
    return (int) fmod(p.ph.d, 10);
}

char to_char(Phloat p) {
    // TODO: more efficient implementation for decimal
    return (char) p.ph.d;
}

int to_int(Phloat p) {
    // TODO: more efficient implementation for decimal
    return (int) p.ph.d;
}

int4 to_int4(Phloat p) {
    // TODO: more efficient implementation for decimal
    return (int4) p.ph.d;
}

int8 to_int8(Phloat p) {
    // TODO: more efficient implementation for decimal
    return (int8) p.ph.d;
}

double to_double(Phloat p) {
    // TODO: more efficient implementation for decimal
    return p.ph.d;
}

Phloat sin(Phloat p) {
    return Phloat(sin(p.ph.d));
}

Phloat cos(Phloat p) {
    return Phloat(cos(p.ph.d));
}

Phloat tan(Phloat p) {
    return Phloat(tan(p.ph.d));
}

Phloat asin(Phloat p) {
    return Phloat(asin(p.ph.d));
}

Phloat acos(Phloat p) {
    return Phloat(acos(p.ph.d));
}

Phloat atan(Phloat p) {
    return Phloat(atan(p.ph.d));
}

void sincos(Phloat phi, Phloat *s, Phloat *c) {
    sincos(phi.ph.d, &s->ph.d, &c->ph.d);
}

Phloat hypot(Phloat x, Phloat y) {
    return Phloat(hypot(x.ph.d, y.ph.d));
}

Phloat atan2(Phloat x, Phloat y) {
    return Phloat(atan2(x.ph.d, y.ph.d));
}

Phloat sinh(Phloat p) {
    return Phloat(sinh(p.ph.d));
}

Phloat cosh(Phloat p) {
    return Phloat(cosh(p.ph.d));
}

Phloat tanh(Phloat p) {
    return Phloat(tanh(p.ph.d));
}

Phloat asinh(Phloat p) {
    return Phloat(asinh(p.ph.d));
}

Phloat acosh(Phloat p) {
    return Phloat(acosh(p.ph.d));
}

Phloat atanh(Phloat p) {
    return Phloat(atanh(p.ph.d));
}

Phloat log(Phloat p) {
    return Phloat(log(p.ph.d));
}

Phloat log1p(Phloat p) {
    return Phloat(log1p(p.ph.d));
}

Phloat log10(Phloat p) {
    return Phloat(log10(p.ph.d));
}

Phloat exp(Phloat p) {
    return Phloat(exp(p.ph.d));
}

Phloat expm1(Phloat p) {
    return Phloat(expm1(p.ph.d));
}

Phloat sqrt(Phloat p) {
    return Phloat(sqrt(p.ph.d));
}

Phloat fmod(Phloat x, Phloat y) {
    return Phloat(fmod(x.ph.d, y.ph.d));
}

Phloat fabs(Phloat p) {
    return Phloat(fabs(p.ph.d));
}

Phloat pow(Phloat x, Phloat y) {
    return Phloat(pow(x.ph.d, y.ph.d));
}

Phloat floor(Phloat p) {
    return Phloat(floor(p.ph.d));
}

Phloat operator*(int x, Phloat y) {
    return Phloat(x * y.ph.d);
}

Phloat operator/(int x, Phloat y) {
    return Phloat(x / y.ph.d);
}

Phloat operator/(double x, Phloat y) {
    return Phloat(x / y.ph.d);
}

Phloat operator+(int x, Phloat y) {
    return Phloat(x + y.ph.d);
}

Phloat operator-(int x, Phloat y) {
    return Phloat(x - y.ph.d);
}

bool operator==(int4 x, Phloat y) {
    return x == y.ph.d;
}

#endif
