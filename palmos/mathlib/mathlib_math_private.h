/* MathLib: Pilot shared library of IEEE-754 double math functions
 *
 * Minimum subset of GCC math_private.h file; just enough to allow proper
 * compilation of the GCC source code with as few changes as possible.
.* These frequently used extraction and insertion macros have been
 * rewritten to generate more efficient code under CodeWarrior.
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
 * Version 1.0, 15 August 1997, Rick Huebner
 */
// Stub out unused macros
#define weak_alias(x,y)
#define strong_alias(x,y)

#define __HI32(x) *((u_int32_t *) &x)
#define __LO32(x) *((u_int32_t *) &x + 1)

/* Get two 32 bit ints from a double.  */
#define EXTRACT_WORDS(ix0,ix1,d)                                \
do {															\
	(ix0) = __HI32(d);											\
	(ix1) = __LO32(d);											\
} while (0)

/* Get the more significant 32 bit int from a double.  */
#define GET_HIGH_WORD(i,d)                                      \
do {                                                            \
	(i) = __HI32(d);											\
} while (0)

/* Get the less significant 32 bit int from a double.  */
#define GET_LOW_WORD(i,d)                                     	\
do {                                                            \
	(i) = __LO32(d);											\
} while (0)

/* Set a double from two 32 bit ints.  */
#define INSERT_WORDS(d,ix0,ix1)                                 \
do {                                                            \
	__HI32(d) = (ix0);											\
	__LO32(d) = (ix1);											\
} while (0)

/* Set the more significant 32 bits of a double from an int.  */
#define SET_HIGH_WORD(d,v)		                                \
do {                                                            \
	__HI32(d) = (v);											\
} while (0)

/* Set the less significant 32 bits of a double from an int.  */
#define SET_LOW_WORD(d,v)		                                \
do {                                                            \
	__LO32(d) = (v);											\
} while (0)
