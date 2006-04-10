#ifndef MATH_H
#define MATH_H

#ifdef __cplusplus
extern "C" {
#endif

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
double frexp(double x, int *exponent);	// Break x into normalized fraction and an integral power of 2
double ldexp(double x, int exponent);	// x * pow(2,exponent)
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
int  isinf(double x);					// Return 0 if x is finite or NaN, +1 if +Infinity, or -1 if -Infinity
int  finite(double x);				// Return nonzero if x is finite and not NaN
double scalbn(double x, int exponent);	// x * pow(2,exponent)
double drem(double x, double y);		// Remainder of x/y
double significand(double x);			// Fractional part of x after dividing out ilogb(x)
double copysign(double x, double y);	// Return x with its sign changed to match y's
int  isnan(double x);					// Return nonzero if x is NaN (Not a Number)
int  ilogb(double x);					// Binary exponent of non-zero x
double rint(double x);					// Integral value nearest x in direction of prevailing rounding mode
double nextafter(double x, double y);	// Next machine double value after x in the direction towards y
double remainder(double x, double y);	// Remainder of integer division x/y with infinite precision
double scalb(double x, double exponent);// x * pow(2,exponent)
double round(double x);					// Round x to nearest integral value away from zero
double trunc(double x);					// Round x to nearest integral value not larger than x
unsigned long signbit(double x);				// Return signbit of x's machine representation
 
#ifdef __cplusplus
}
#endif

#endif // MATH_H
