/* MathLib: Pilot shared library of IEEE-754 double math functions
 *
 * Library function prototypes for the calling application.  This is
 * the file that the calling application should include in order to
 * get access to the routines in the library; it serves the same
 * function as the file "math.h" normally used on systems with
 * standard math libraries.  Each function in the library has two
 * prototypes listed; the first is for the programmer-friendly
 * wrapper function in MathLib.c, the second is for the raw SYS_TRAP()
 * invocation that actually calls the library routine.
 *
 * Copyright (C) 1997 Rick Huebner
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see file COPYING.LIB.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA
 *
 * Version 1.01, 23 August 1997, Rick Huebner
 */
#ifndef __MATHLIB_H__
#define __MATHLIB_H__

// Library name for use with SysLibFind()
#define MathLibName		"MathLib"
// Values for use with SysLibLoad()
#define LibType			'libr'
#define MathLibCreator	'MthL'

// This is the major version number of the library.  If new functions
// are added to this library, this version number must be incremented
// to protect programs which are compiled with this header from trying
// to invoke the new functions in an old version of MathLib.prc,
// which would be fatal.  Do NOT delete any functions from this list,
// or any older programs which try to use them will die.  Any changes
// other than adding new functions should be reflected in the minor
// part of the version number, e.g. 1.1 if a bug fix, 2.0 if new
// functions added.
#define MathLibVersion 1

// Possible Err values from MathLib functions
typedef enum MathLibErrorCode {
	mlErrNone = 0,
	mlErrOldVersion,	// library is older than version passed to open()
	mlErrNotOpen,		// close() without having called open() first
	mlErrNoMemory		// can't allocate global data block
} MathLibErrorCode;

// Library reference returned by SysLibFind() or SysLibLoad()
extern UInt16 MathLibRef;

/*****************************
 * Library control functions *
 *****************************/
Err MathLibOpen(UInt16 refnum, UInt16 version)		// Initialize library for use
	SYS_TRAP(sysLibTrapOpen);
Err MathLibClose(UInt16 refnum, UInt16* usecountP)// Free library resources when finished
	SYS_TRAP(sysLibTrapClose);
Err MathLibSleep(UInt16 refnum)					// Called by OS when Pilot sleeps
	SYS_TRAP(sysLibTrapSleep);
Err MathLibWake(UInt16 refnum)					// Called by OS when Pilot wakes
	SYS_TRAP(sysLibTrapWake);

/***************************
 * Trigonometric functions *
 ***************************/
double acos(double x);				// Arc cosine of x 
double asin(double x);				// Arc sine of x	
double atan(double x);				// Arc tangent of x	
double atan2(double y, double x);	// Arc tangent of y/x	
double cos(double x);				// Cosine of x	
double sin(double x);				// Sine of x	
double tan(double x);				// Tangent of x	
void   sincos(double x, double *sinx, double *cosx); 	// Sine and cosine of x	

/************************	
 * Hyperbolic functions	*
 ************************/ 
double cosh(double x);				// Hyperbolic cosine of x 
double sinh(double x);				// Hyperbolic sine of x
double tanh(double x);				// Hyperbolic tangent of x
double acosh(double x);				// Hyperbolic arc cosine of x
double asinh(double x);				// Hyperbolic arc sine of x
double atanh(double x);				// Hyperbolic arc tangent of x

/*****************************************
 * Exponential and logarithmic functions *
 *****************************************/
double exp(double x);					// Exponential function of x [pow(e,x)]
double frexp(double x, Int16 *exponent);	// Break x into normalized fraction and an integral power of 2
double ldexp(double x, Int16 exponent);	// x * pow(2,exponent)
double log(double x);					// Natural logarithm of x
double log10(double x);					// Base 10 logarithm of x
double modf(double x, double *intpart);	// Break x into integral and fractional parts
double expm1(double x);					// exp(x) - 1
double log1p(double x);					// log(1+x)
double logb(double x);					// Base 2 signed integral exponent of x
double log2(double x);					// Base 2 logarithm of x

/*******************
 * Power functions *
 *******************/	
double pow(double x, double y);		// x to the y power [x**y]
double sqrt(double x);				// Square root of x [x**0.5]
double hypot(double x, double y);	// sqrt(x*x + y*y)	[hypotenuse of right triangle]
double cbrt(double x);				// Cube root of x	[x**(1/3)]

/************************************************************
 * Nearest integer, absolute value, and remainder functions *
 ************************************************************/
double ceil(double x);				// Smallest integral value not less than x
double fabs(double x);				// Absolute value of x
double floor(double x);				// Largest integral value not greater than x
double fmod(double x, double y);	// Modulo remainder of x/y

/***************************
 * Miscellaneous functions *
 ***************************/
Int16    isinf(double x);					// Return 0 if x is finite or NaN, +1 if +Infinity, or -1 if -Infinity
Int16    finite(double x);				// Return nonzero if x is finite and not NaN
double scalbn(double x, Int16 exponent);	// x * pow(2,exponent)
double drem(double x, double y);		// Remainder of x/y
double significand(double x);			// Fractional part of x after dividing out ilogb(x)
double copysign(double x, double y);	// Return x with its sign changed to match y's
Int16    isnan(double x);					// Return nonzero if x is NaN (Not a Number)
Int16    ilogb(double x);					// Binary exponent of non-zero x
double rint(double x);					// Integral value nearest x in direction of prevailing rounding mode
double nextafter(double x, double y);	// Next machine double value after x in the direction towards y
double remainder(double x, double y);	// Remainder of integer division x/y with infinite precision
double scalb(double x, double exponent);// x * pow(2,exponent)
double round(double x);					// Round x to nearest integral value away from zero
double trunc(double x);					// Round x to nearest integral value not larger than x
UInt32  signbit(double x);				// Return signbit of x's machine representation

/****************************************
 * Prototypes for the system traps that *
 * actually perform the library calls,  *
 * in the format mandated by the OS.    *
 ****************************************/
Err MathLibACos(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom);
Err MathLibASin(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+1);
Err MathLibATan(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+2);
Err MathLibATan2(UInt16 refnum, double y, double x, double *result)	SYS_TRAP(sysLibTrapCustom+3);
Err MathLibCos(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+4);
Err MathLibSin(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+5);
Err MathLibTan(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+6);
Err MathLibSinCos(UInt16 refnum, double x, double *sinx, double *cosx)	SYS_TRAP(sysLibTrapCustom+7);
Err MathLibCosH(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+8);
Err MathLibSinH(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+9);
Err MathLibTanH(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+10);
Err MathLibACosH(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+11);
Err MathLibASinH(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+12);
Err MathLibATanH(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+13);
Err MathLibExp(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+14);
Err MathLibFrExp(UInt16 refnum, double x, double *fraction, Int16 *exponent)	SYS_TRAP(sysLibTrapCustom+15);
Err MathLibLdExp(UInt16 refnum, double x, Int16 exponent, double *result)	SYS_TRAP(sysLibTrapCustom+16);
Err MathLibLog(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+17);
Err MathLibLog10(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+18);
Err MathLibModF(UInt16 refnum, double x, double *intpart, double *fracpart)	SYS_TRAP(sysLibTrapCustom+19);
Err MathLibExpM1(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+20);
Err MathLibLog1P(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+21);
Err MathLibLogB(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+22);
Err MathLibLog2(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+23);
Err MathLibPow(UInt16 refnum, double x, double y, double *result)	SYS_TRAP(sysLibTrapCustom+24);
Err MathLibSqrt(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+25);
Err MathLibHypot(UInt16 refnum, double x, double y, double *result)	SYS_TRAP(sysLibTrapCustom+26);
Err MathLibCbrt(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+27);
Err MathLibCeil(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+28);
Err MathLibFAbs(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+29);
Err MathLibFloor(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+30);
Err MathLibFMod(UInt16 refnum, double x, double y, double *result)	SYS_TRAP(sysLibTrapCustom+31);
Err MathLibIsInf(UInt16 refnum, double x, Int16 *result)	SYS_TRAP(sysLibTrapCustom+32);
Err MathLibFinite(UInt16 refnum, double x, Int16 *result)	SYS_TRAP(sysLibTrapCustom+33);
Err MathLibScalBN(UInt16 refnum, double x, Int16 exponent, double *result)	SYS_TRAP(sysLibTrapCustom+34);
Err MathLibDRem(UInt16 refnum, double x, double y, double *result)	SYS_TRAP(sysLibTrapCustom+35);
Err MathLibSignificand(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+36);
Err MathLibCopySign(UInt16 refnum, double x, double y, double *result)	SYS_TRAP(sysLibTrapCustom+37);
Err MathLibIsNaN(UInt16 refnum, double x, Int16 *result)	SYS_TRAP(sysLibTrapCustom+38);
Err MathLibILogB(UInt16 refnum, double x, Int16 *result)	SYS_TRAP(sysLibTrapCustom+39);
Err MathLibRInt(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+40);
Err MathLibNextAfter(UInt16 refnum, double x, double y, double *result)	SYS_TRAP(sysLibTrapCustom+41);
Err MathLibRemainder(UInt16 refnum, double x, double y, double *result)	SYS_TRAP(sysLibTrapCustom+42);
Err MathLibScalB(UInt16 refnum, double x, double exponent, double *result)	SYS_TRAP(sysLibTrapCustom+43);
Err MathLibRound(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+44);
Err MathLibTrunc(UInt16 refnum, double x, double *result)	SYS_TRAP(sysLibTrapCustom+45);
Err MathLibSignBit(UInt16 refnum, double x, UInt32 *result)	SYS_TRAP(sysLibTrapCustom+46);
 
#endif // __MATHLIB_H__
