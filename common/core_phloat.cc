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


/* public */
phloat::phloat(int i) {
    ph.d = i;
}

/* public */
phloat::phloat(int numer, int denom) {
    // PHLOAT_TODO: This constructor is meant to be used to allow exact decimal
    // number literals, e.g. phloat(4, 100) to represent 0.04 exactly.
    // Because of this, it should always be created as a decimal-mode phloat;
    // it should only be *used* in decimal mode, and its dec2bin() and
    // bin2dec() methods should never be called.
    ph.d = numer / denom;
}

/* public */
phloat::phloat(int8 i) {
    ph.d = i;
}

/* public */
phloat::phloat(double d) {
    ph.d = d;
}

/* public */
phloat::phloat(const phloat &p) {
    ph = p.ph;
}

/* public */
phloat phloat::operator=(int i) {
    ph.d = i;
    return *this;
}

/* public */
phloat phloat::operator=(int8 i) {
    ph.d = i;
    return *this;
}

/* public */
phloat phloat::operator=(double d) {
    ph.d = d;
    return *this;
}

/* public */
phloat phloat::operator=(phloat p) {
    ph = p.ph;
    return *this;
}

/* public */
int phloat::to_digit() const {
    // TODO: more efficient implementation for decimal
    return (int) fmod(ph.d, 10);
}

/* public */
char phloat::to_char() const {
    // TODO: more efficient implementation for decimal
    return (char) ph.d;
}

/* public */
int phloat::to_int() const {
    // TODO: more efficient implementation for decimal
    return (int) ph.d;
}

/* public */
int4 phloat::to_int4() const {
    // TODO: more efficient implementation for decimal
    return (int4) ph.d;
}

/* public */
int8 phloat::to_int8() const {
    // TODO: more efficient implementation for decimal
    return (int8) ph.d;
}

/* public */
double phloat::to_double() const {
    // TODO: more efficient implementation for decimal
    return ph.d;
}

/* public */
bool phloat::operator==(phloat p) const {
    return ph.d == p.ph.d;
}

/* public */
bool phloat::operator!=(phloat p) const {
    return ph.d != p.ph.d;
}

/* public */
bool phloat::operator<(phloat p) const {
    return ph.d < p.ph.d;
}

/* public */
bool phloat::operator<=(phloat p) const {
    return ph.d <= p.ph.d;
}

/* public */
bool phloat::operator>(phloat p) const {
    return ph.d > p.ph.d;
}

/* public */
bool phloat::operator>=(phloat p) const {
    return ph.d >= p.ph.d;
}

/* public */
phloat phloat::operator-() const {
    return phloat(-ph.d);
}

/* public */
phloat phloat::operator*(phloat p) const {
    return ph.d * p.ph.d;
}

/* public */
phloat phloat::operator/(phloat p) const {
    return ph.d / p.ph.d;
}

/* public */
phloat phloat::operator+(phloat p) const {
    return ph.d + p.ph.d;
}

/* public */
phloat phloat::operator-(phloat p) const {
    return ph.d - p.ph.d;
}

/* public */
phloat phloat::operator*=(phloat p) {
    ph.d *= p.ph.d;
    return *this;
}

/* public */
phloat phloat::operator/=(phloat p) {
    ph.d /= p.ph.d;
    return *this;
}

/* public */
phloat phloat::operator+=(phloat p) {
    ph.d += p.ph.d;
    return *this;
}

/* public */
phloat phloat::operator-=(phloat p) {
    ph.d -= p.ph.d;
    return *this;
}

/* public */
phloat phloat::operator++() {
    // prefix
    ++ph.d;
    return *this;
}

/* public */
phloat phloat::operator++(int) {
    // postfix
    phloat old = *this;
    ++ph.d;
    return old;
}

/* public */
phloat phloat::operator--() {
    // prefix
    --ph.d;
    return *this;
}

/* public */
phloat phloat::operator--(int) {
    // postfix
    phloat old = *this;
    --ph.d;
    return old;
}

/* public */
void phloat::dec2bin() {
    //
}

/* public */
void phloat::bin2dec() {
    //
}

int p_isinf(phloat p) {
    return isinf(p.ph.d);
}

int p_isnan(phloat p) {
    return isnan(p.ph.d);
}

phloat sin(phloat p) {
    return phloat(sin(p.ph.d));
}

phloat cos(phloat p) {
    return phloat(cos(p.ph.d));
}

phloat tan(phloat p) {
    return phloat(tan(p.ph.d));
}

phloat asin(phloat p) {
    return phloat(asin(p.ph.d));
}

phloat acos(phloat p) {
    return phloat(acos(p.ph.d));
}

phloat atan(phloat p) {
    return phloat(atan(p.ph.d));
}

void sincos(phloat phi, phloat *s, phloat *c) {
    sincos(phi.ph.d, &s->ph.d, &c->ph.d);
}

phloat hypot(phloat x, phloat y) {
    return phloat(hypot(x.ph.d, y.ph.d));
}

phloat atan2(phloat x, phloat y) {
    return phloat(atan2(x.ph.d, y.ph.d));
}

phloat sinh(phloat p) {
    return phloat(sinh(p.ph.d));
}

phloat cosh(phloat p) {
    return phloat(cosh(p.ph.d));
}

phloat tanh(phloat p) {
    return phloat(tanh(p.ph.d));
}

phloat asinh(phloat p) {
    return phloat(asinh(p.ph.d));
}

phloat acosh(phloat p) {
    return phloat(acosh(p.ph.d));
}

phloat atanh(phloat p) {
    return phloat(atanh(p.ph.d));
}

phloat log(phloat p) {
    return phloat(log(p.ph.d));
}

phloat log1p(phloat p) {
    return phloat(log1p(p.ph.d));
}

phloat log10(phloat p) {
    return phloat(log10(p.ph.d));
}

phloat exp(phloat p) {
    return phloat(exp(p.ph.d));
}

phloat expm1(phloat p) {
    return phloat(expm1(p.ph.d));
}

phloat sqrt(phloat p) {
    return phloat(sqrt(p.ph.d));
}

phloat fmod(phloat x, phloat y) {
    return phloat(fmod(x.ph.d, y.ph.d));
}

phloat fabs(phloat p) {
    return phloat(fabs(p.ph.d));
}

phloat pow(phloat x, phloat y) {
    return phloat(pow(x.ph.d, y.ph.d));
}

phloat floor(phloat p) {
    return phloat(floor(p.ph.d));
}

phloat operator*(int x, phloat y) {
    return phloat(x * y.ph.d);
}

phloat operator/(int x, phloat y) {
    return phloat(x / y.ph.d);
}

phloat operator/(double x, phloat y) {
    return phloat(x / y.ph.d);
}

phloat operator+(int x, phloat y) {
    return phloat(x + y.ph.d);
}

phloat operator-(int x, phloat y) {
    return phloat(x - y.ph.d);
}

bool operator==(int4 x, phloat y) {
    return x == y.ph.d;
}
