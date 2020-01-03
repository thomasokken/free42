/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2020  Thomas Okken
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
#define _REENTRANT 1
#include <math.h>
#endif

#define int2 short
#define uint2 unsigned short
#define int4 int
#define uint4 unsigned int
#define int8 long long
#define uint8 unsigned long long
#define uint unsigned int

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
/* I have tested big-endian state file compatibility in Fedora 12
 * running on qemu-system-ppc. I found that I needed to explicitly
 * set F42_BIG_ENDIAN for it to work; apparently the __BYTE_ORDER__
 * macro is not defined in such old compilers.
 * Also see the comment about setting BID_BIG_ENDIAN in
 * gtk/build-intel-lib.sh.
 */
#define F42_BIG_ENDIAN 1
#endif


#if defined(WINDOWS) && !defined(BCD_MATH) && !defined(__GNUC__)

        /* MSVC++ 2008 lacks a few math functions that Free42 needs.
         * I've defined workarounds in mathfudge.c.
         */
#ifdef __cplusplus
        extern "C" {
#endif
                int isnan(double x);
                int isinf(double x);
                double atanh(double x);
                /* These are in the library, but not declared in math.h */
                double asinh(double x);
                double acosh(double x);
                double expm1(double x);
                double log1p(double x);
                double tgamma(double x);
#ifdef __cplusplus
        }
#endif

#endif


/* Magic number "24kF" for the state file. */
#define FREE42_MAGIC 0x466b3432
#define FREE42_MAGIC_STR "24kF"


#endif
