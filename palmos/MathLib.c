/* MathLib: Pilot shared library of IEEE-754 double math functions
 *
 * Convenience functions for the calling application.  These functions
 * provide a programmer-friendly wrapper around the raw system trap
 * invocations which actually call the library routines.  The idea
 * is to allow the programmer to say:
 *    y = sqrt(x);
 * instead of requiring:
 *	   MathLibSqrt(MathLibRef, x, &y);
 * like the system trap interface requires.  The system trap form is
 * not only harder to read, but can't directly replace the traditional
 * function call in ported code, and can't be nested inside an expression.
 * Just add this source file to your project or makefile, and include
 * "MathLib.h" in any source file that needs to call these.
 *
 * The downside to these routines is that they'll take up some space
 * in your program, though CodeWarrior at least is smart enough to
 * only link in the ones which you actually use, so it doesn't really
 * cost you that much.  In fact, if you call these enough they'll pay
 * for themselves, since "x=sqrt(x)" generates much less code than
 * calling MathLibSqrt() directly.
 *
 * Released to the public domain; use as you like.
 *
 * Version 1.1.01, 6 February 2003, Rick Huebner
 *    Corrected licensing of this file; no executable code changes.
 */
#include <PalmOS.h>

#include "MathLib.h"

// Library reference returned by SysLibFind() or SysLibLoad()
UInt16 MathLibRef;



double acos(double x) {
	double result;
	MathLibACos(MathLibRef, x, &result);
	return result;
}

double asin(double x) {
	double result;
	MathLibASin(MathLibRef, x, &result);
	return result;
}

double atan(double x) {
	double result;
	MathLibATan(MathLibRef, x, &result);
	return result;
}

double atan2(double y, double x) {
	double result;
	MathLibATan2(MathLibRef, y, x, &result);
	return result;
}

double cos(double x) {
	double result;
	MathLibCos(MathLibRef, x, &result);
	return result;
}

double sin(double x) {
	double result;
	MathLibSin(MathLibRef, x, &result);
	return result;
}

double tan(double x) {
	double result;
	MathLibTan(MathLibRef, x, &result);
	return result;
}

void sincos(double x, double *sinx, double *cosx) {
	MathLibSinCos(MathLibRef, x, sinx, cosx);
}

double cosh(double x) {
	double result;
	MathLibCosH(MathLibRef, x, &result);
	return result;
}

double sinh(double x) {
	double result;
	MathLibSinH(MathLibRef, x, &result);
	return result;
}

double tanh(double x) {
	double result;
	MathLibTanH(MathLibRef, x, &result);
	return result;
}

double acosh(double x) {
	double result;
	MathLibACosH(MathLibRef, x, &result);
	return result;
}

double asinh(double x) {
	double result;
	MathLibASinH(MathLibRef, x, &result);
	return result;
}

double atanh(double x) {
	double result;
	MathLibATanH(MathLibRef, x, &result);
	return result;
}

double exp(double x) {
	double result;
	MathLibExp(MathLibRef, x, &result);
	return result;
}

double frexp(double x, Int16 *exponent) {
	double fraction;
	MathLibFrExp(MathLibRef, x, &fraction, exponent);
	return fraction;
}

double ldexp(double x, Int16 exponent) {
	double result;
	MathLibLdExp(MathLibRef, x, exponent, &result);
	return result;
}

double log(double x) {
	double result;
	MathLibLog(MathLibRef, x, &result);
	return result;
}

double log10(double x) {
	double result;
	MathLibLog10(MathLibRef, x, &result);
	return result;
}

double modf(double x, double *intpart) {
	double fraction;
	MathLibModF(MathLibRef, x, intpart, &fraction);
	return fraction;
}

double expm1(double x) {
	double result;
	MathLibExpM1(MathLibRef, x, &result);
	return result;
}

double log1p(double x) {
	double result;
	MathLibLog1P(MathLibRef, x, &result);
	return result;
}

double logb(double x) {
	double result;
	MathLibLogB(MathLibRef, x, &result);
	return result;
}

double log2(double x) {
	double result;
	MathLibLog2(MathLibRef, x, &result);
	return result;
}

double pow(double x, double y) {
	double result;
	MathLibPow(MathLibRef, x, y, &result);
	return result;
}

double sqrt(double x) {
	double result;
	MathLibSqrt(MathLibRef, x, &result);
	return result;
}

double hypot(double x, double y) {
	double result;
	MathLibHypot(MathLibRef, x, y, &result);
	return result;
}

double cbrt(double x) {
	double result;
	MathLibCbrt(MathLibRef, x, &result);
	return result;
}

double ceil(double x) {
	double result;
	MathLibCeil(MathLibRef, x, &result);
	return result;
}

double fabs(double x) {
	double result;
	MathLibFAbs(MathLibRef, x, &result);
	return result;
}

double floor(double x) {
	double result;
	MathLibFloor(MathLibRef, x, &result);
	return result;
}

double fmod(double x, double y) {
	double result;
	MathLibFMod(MathLibRef, x, y, &result);
	return result;
}

Int16 isinf(double x) {
	Int16 result;
	MathLibIsInf(MathLibRef, x, &result);
	return result;
}

Int16 finite(double x) {
	Int16 result;
	MathLibFinite(MathLibRef, x, &result);
	return result;
}

double scalbn(double x, Int16 exponent) {
	double result;
	MathLibScalBN(MathLibRef, x, exponent, &result);
	return result;
}

double drem(double x, double y) {
	double result;
	MathLibDRem(MathLibRef, x, y, &result);
	return result;
}

double significand(double x) {
	double result;
	MathLibSignificand(MathLibRef, x, &result);
	return result;
}

double copysign(double x, double y) {
	double result;
	MathLibCopySign(MathLibRef, x, y, &result);
	return result;
}

Int16 isnan(double x) {
	Int16 result;
	MathLibIsNaN(MathLibRef, x, &result);
	return result;
}

Int16 ilogb(double x) {
	Int16 result;
	MathLibILogB(MathLibRef, x, &result);
	return result;
}

double rint(double x) {
	double result;
	MathLibRInt(MathLibRef, x, &result);
	return result;
}

double nextafter(double x, double y) {
	double result;
	MathLibNextAfter(MathLibRef, x, y, &result);
	return result;
}

double remainder(double x, double y) {
	double result;
	MathLibRemainder(MathLibRef, x, y, &result);
	return result;
}

double scalb(double x, double exponent) {
	double result;
	MathLibScalB(MathLibRef, x, exponent, &result);
	return result;
}

double round(double x) {
	double result;
	MathLibRound(MathLibRef, x, &result);
	return result;
}

double trunc(double x) {
	double result;
	MathLibTrunc(MathLibRef, x, &result);
	return result;
}

UInt32 signbit(double x) {
	UInt32 result;
	MathLibSignBit(MathLibRef, x, &result);
	return result;
}
