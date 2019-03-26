/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2019  Thomas Okken
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


#ifndef BCD_MATH
#include <math.h>
#endif

#define int2 short
#define uint2 unsigned short
#define int4 int
#define uint4 unsigned int
#define int8 long long
#define uint8 unsigned long long
#define LL(x) x##LL


#if defined(WINDOWS) && !defined(__GNUC__)

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

/* NOTE: In my Linux build, all I have to do is DECLARE sincos(); glibc 2.3.3
 * has it (for C99, I suppose) so I don't have to DEFINE it. On other Unixes
 * (e.g. MacOS X), it may not be provided by the standard libraries; in this
 * case, define the NO_SINCOS symbol, here or in the Makefile.
 * For the Palm build, we don't even need the declaration, since sincos() is
 * provided by MathLib.
 */
extern "C" void sincos(double x, double *sinx, double *cosx);
//#define NO_SINCOS 1

#endif

// the iPhone SDK does not define 'finite' so we create a macro wrapper
#if !defined(BCD_MATH) && defined(IPHONE)
#define finite(x) isfinite(x)
#endif

// Android doesn't appear to provide log2
#if defined(ANDROID)
extern "C" double log2(double x);
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
 * Version 17: 1.4.65 iPhone "OFF enable" flag
 * Version 18: 1.4.79 Replaced BCD20 with Intel's Decimal Floating Point
 *                    Library v.2.1.
 * Version 19: 1.5.14 Removed mode_time_dmy; now using flag 31 instead.
 * Version 20: 2.0.3  Removed "raw text" option, ext_copan, and ext_bigstack.
 * Version 21: 2.0.7  New random number generator.
 * Version 22: 2.0.17 Fixed bug where local GTO/XEQ targets didn't get cleared
 *                    when and END was deleted, and potentially (though never
 *                    reported) also when an END was inserted. Bumping the
 *                    state version so that older state files get their
 *                    potentially-incorrect jump targets cleared, just in case.
 * Version 23: 2.1    Added "prog" extension: SST^, SST->
 */
#define FREE42_MAGIC 0x466b3432
#define FREE42_VERSION 23


#endif
