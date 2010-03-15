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

#ifndef FREE42_H
#define FREE42_H 1


#if defined(PALMOS) && !defined(PALMOS_ARM)

#include <PalmOS.h>
#include <Libraries/PalmOSGlue/PalmOSGlue.h>
#ifndef BCD_MATH
extern "C" {
#include "MathLib.h"
}
#endif

/* Segmentation stuff */
#define SHELL1_SECT __attribute__ ((section ("Shell1")))
#define SHELL2_SECT __attribute__ ((section ("Shell2")))
#define FILESYS_SECT __attribute__ ((section ("FileSys")))
#define MAIN_SECT __attribute__ ((section ("Main")))
#define COMMANDS1_SECT __attribute__ ((section ("Commnds1")))
#define COMMANDS2_SECT __attribute__ ((section ("Commnds2")))
#define COMMANDS3_SECT __attribute__ ((section ("Commnds3")))
#define COMMANDS4_SECT __attribute__ ((section ("Commnds4")))
#define COMMANDS5_SECT __attribute__ ((section ("Commnds5")))
#define COMMANDS6_SECT __attribute__ ((section ("Commnds6")))
#ifdef PALMOS_ARM_SHELL
#define DISPLAY_SECT
#else
#define DISPLAY_SECT __attribute__ ((section ("Display")))
#endif
#define GLOBALS_SECT __attribute__ ((section ("Globals")))
#define HELPERS_SECT __attribute__ ((section ("Helpers")))
#define KEYDOWN_SECT __attribute__ ((section ("KeyDown")))
#define LINALG1_SECT __attribute__ ((section ("LinAlg1")))
#define LINALG2_SECT __attribute__ ((section ("LinAlg2")))
#define MATH1_SECT __attribute__ ((section ("Math1")))
#define MATH2_SECT __attribute__ ((section ("Math2")))
#define PHLOAT_SECT __attribute__ ((section ("Phloat")))
#define STO_RCL_SECT __attribute__ ((section ("StoRcl")))
#define TABLES_SECT __attribute__ ((section ("Tables")))
#define VARIABLES_SECT __attribute__ ((section ("Variabls")))
#define BCD1_SECT __attribute__ ((section ("BcdFlt1")))
#define BCD2_SECT __attribute__ ((section ("BcdFlt2")))
#define int2 Int16
#define uint2 UInt16
#define int4 Int32
#define uint4 UInt32

#else /* !PALMOS || PALMOS_ARM */

#ifndef BCD_MATH
#ifdef PALMOS_ARM
#include "mathlib/math.h"
#else
#include <math.h>
#endif
#endif

#define SHELL1_SECT
#define SHELL2_SECT
#define FILESYS_SECT
#define MAIN_SECT
#define COMMANDS1_SECT
#define COMMANDS2_SECT
#define COMMANDS3_SECT
#define COMMANDS4_SECT
#define COMMANDS5_SECT
#define COMMANDS6_SECT
#define DISPLAY_SECT
#define GLOBALS_SECT
#define HELPERS_SECT
#define KEYDOWN_SECT
#define LINALG1_SECT
#define LINALG2_SECT
#define MATH1_SECT
#define MATH2_SECT
#define PHLOAT_SECT
#define STO_RCL_SECT
#define TABLES_SECT
#define VARIABLES_SECT
#define BCD1_SECT
#define BCD2_SECT
#define int2 short
#define uint2 unsigned short
#define int4 int
#define uint4 unsigned int

#endif /* PALMOS/PALMOS_ARM */


#if defined(WINDOWS) && !defined(__GNUC__)

#define int8 __int64
#define uint8 unsigned __int64
#define LL(x) x

	/* MSVC++ 6.0 lacks a few math functions that Free42 needs.
	 * I've defined workarounds in mathfudge.c. NOTE: my versions
	 * of isnan(), finite(), and isinf() are a bit lame -- I *think*
	 * they handle infinities properly, but definitely not NaNs
	 * (although NaNs shouldn't be much of a problem because the Free42
	 * code mostly tries to avoid them, rather than detect them after
	 * the fact).
	 */
#ifdef __cplusplus
	extern "C" {
#endif
		int isnan(double x);
		int finite(double x);
		int isinf(double x);
		void sincos(double x, double *sinx, double *cosx);
		double asinh(double x);
		double acosh(double x);
		double atanh(double x);
		double expm1(double x);
		double log1p(double x);
#ifdef _WIN32_WCE
		double hypot(double x, double y);
#endif
#ifdef __cplusplus
	}
#endif
#else

#define int8 long long
#define uint8 unsigned long long
#define LL(x) x##LL

/* NOTE: In my Linux build, all I have to do is DECLARE sincos(); glibc 2.3.3
 * has it (for C99, I suppose) so I don't have to DEFINE it. On other Unixes
 * (e.g. MacOS X), it may not be provided by the standard libraries; in this
 * case, define the NO_SINCOS symbol, here or in the Makefile.
 * For the Palm build, we don't even need the declaration, since sincos() is
 * provided by MathLib.
 */
#ifndef PALMOS
extern "C" void sincos(double x, double *sinx, double *cosx) HELPERS_SECT;
#endif
//#define NO_SINCOS 1

#endif

// the iPhone SDK does not define 'finite' so we create a macro wrapper
#if !defined(BCD_MATH) && defined(IPHONE)
#define finite(x) isfinite(x)
#endif

#if defined(PALMOS) && defined(BCD_MATH)
// Compatibility hacks for the sake of core_phloat and bcdfloat. I don't want
// to depend on MathLib for this stuff, especially since I can make do with
// simple partial implementations.
int isinf(double d) PHLOAT_SECT;
int isnan(double d) PHLOAT_SECT;
double pow(double x, double y) PHLOAT_SECT;
double floor(double x) PHLOAT_SECT;
double log10(double x) PHLOAT_SECT;
#endif


#define uint unsigned int

/* Magic number and version number for the state file.
 * State file versions correspond to application releases as follows:
 * 
 * Version  0: 1.0    first release
 * Version  1: 1.0.13 "IP Hack" option
 * Version  2: 1.0.13 "singular matrix" and matrix "out of range" options
 * Version  3: 1.0.16 "deferred_print" flag for NORM/TRACE printing
 * Version  4: 1.1    BCD conversion table no longer stored in state file
 * Version  5: 1.1    "raw text" option
 * Version  6: 1.1.8  GETKEY across power-cycle
 * Version  7: 1.1.12 FCN catalog assignments now HP-42S-compatible
 * Version  8: 1.1.14 F42 file format and "HP-42S byte counts" option removed
 * Version  9: 1.4    decimal version; removed IP Hack
 * Version 10: 1.4.16 persistent shared matrices
 * Version 11: 1.4.44 "Auto-Repeat" option
 * Version 12: 1.4.52 BIGSTACK (iphone only);
 *                    new BCDFloat format (Inf and NaN flags)
 * 
 *  ========== NOTE: BCD20 Upgrade in Free42 1.4.52 ==========
 *  In version 1.4.52, I upgraded to a new version of BCD20, without realizing
 *  that it uses a slightly different storage format (NaN and Inifinity are now
 *  encoded using two flags in the exponent field, rather than using magical
 *  exponent values; the exponent field was narrowed by 2 bits to accomodate
 *  these flags).
 *  I should have added new code to convert BCDFloat numbers from the old
 *  format to the new at that time. Once I discovered this oversight, 1.4.52-54
 *  were already released.
 *  In 1.4.55, I introduced code to convert old-style BCDFloat to new-style if
 *  the state file version is less than 12, i.e. created by Free42 1.4.51 or
 *  earlier. This means that 1.4.55 will interpret BCDFloat from all previous
 *  versions correctly; however, any state file that has gone through the
 *  transition from <= 1.4.51 Decimal to 1.4.52-54 Decimal may still be
 *  corrupted, and the only way to be safe is to do CLALL and reload all
 *  programs and data in that case.
 *
 * Version 13: 1.4.55 Dynamically sized BIGSTACK (iphone only)
 * Version 14: 1.4.63 Moved BIGSTACK DROP command from index 315 to 329, to fix
 *                    the clash with Underhill's COPAN extensions. The iPhone
 *                    version, when reading a state file with version 12 or 13,
 *                    scans all programs and renumbers DROP where necessary.
 *                    All other versions can ignore this version number change.
 * Version 15: 1.4.63 "Enable Extension" options for COPAN, BIGSTACK, ACCEL,
 *                    LOCAT, HEADING, and HP-41 Time
 * Version 16: 1.4.63 time and date format flags
 */
#define FREE42_MAGIC 0x466b3432
#define FREE42_VERSION 16


#endif
