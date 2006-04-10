#include "math.h"
#include "mathlib_math.h"

double jumpto__ieee754_sqrt(double x) {
	return __ieee754_sqrt(x);
}

double jumpto__fabs(double x) {
	return __fabs(x);
}

double jumpto__scalbn(double x, int n) {
	return __scalbn(x, n);
}



double acos(double x) {
    return __ieee754_acos(x);
}

double asin(double x) {
    return __ieee754_asin(x);
}

double atan(double x) {
    return __atan(x);
}

double atan2(double y, double x) {
    return __ieee754_atan2(y, x);
}

double cos(double x) {
    return __cos(x);
}

double sin(double x) {
    return __sin(x);
}

double tan(double x) {
    return __tan(x);
}

void sincos(double x, double *sinx, double *cosx) {
    __sincos(x, sinx, cosx);
}

double cosh(double x) {
    return __ieee754_cosh(x);
}

double sinh(double x) {
    return __ieee754_sinh(x);
}

double tanh(double x) {
    return __tanh(x);
}

double acosh(double x) {
    return __ieee754_acosh(x);
}

double asinh(double x) {
    return __asinh(x);
}

double atanh(double x) {
    return __ieee754_atanh(x);
}

double exp(double x) {
    return __ieee754_exp(x);
}

double frexp(double x, int *exponent) {
    return __frexp(x, exponent);
}

double ldexp(double x, int exponent) {
    return __ldexp(x, exponent);
}

double log(double x) {
    return __ieee754_log(x);
}

double log10(double x) {
    return __ieee754_log10(x);
}

double modf(double x, double *intpart) {
    return __modf(x, intpart);
}

double expm1(double x) {
    return __expm1(x);
}

double log1p(double x) {
    return __log1p(x);
}

double logb(double x) {
    return __logb(x);
}

double log2(double x) {
    return __log2(x);
}

double pow(double x, double y) {
    return __ieee754_pow(x, y);
}

double sqrt(double x) {
    return __ieee754_sqrt(x);
}

double hypot(double x, double y) {
    return __ieee754_hypot(x, y);
}

double cbrt(double x) {
    return __cbrt(x);
}

double ceil(double x) {
    return __ceil(x);
}

double fabs(double x) {
    return __fabs(x);
}

double floor(double x) {
    return __floor(x);
}

double fmod(double x, double y) {
    return __ieee754_fmod(x, y);
}

int isinf(double x) {
    return __isinf(x);
}

int finite(double x) {
    return __finite(x);
}

double scalbn(double x, int exponent) {
    return __scalbn(x, exponent);
}

double drem(double x, double y) {
    return __ieee754_remainder(x, y);
}

double significand(double x) {
    return __significand(x);
}

double copysign(double x, double y) {
    return __copysign(x, y);
}

int isnan(double x) {
    return __isnan(x);
}

int ilogb(double x) {
    return __ilogb(x);
}

double rint(double x) {
    return __rint(x);
}

double nextafter(double x, double y) {
    return __nextafter(x, y);
}

double remainder(double x, double y) {
    return __ieee754_remainder(x, y);
}

double scalb(double x, double exponent) {
    return __ieee754_scalb(x, exponent);
}

double round(double x) {
    return __round(x);
}

double trunc(double x) {
    return __trunc(x);
}

unsigned long signbit(double x) {
    return __signbit(x);
}
