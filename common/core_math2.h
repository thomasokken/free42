/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2011  Thomas Okken
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

#ifndef CORE_MATH2_H
#define CORE_MATH2_H 1

#include "core_phloat.h"

phloat math_random() MATH2_SECT;
int math_asinh_acosh(phloat xre, phloat xim,
			    phloat *yre, phloat *yim, int do_asinh) MATH2_SECT;
int math_atanh(phloat xre, phloat xim, phloat *yre, phloat *yim) MATH2_SECT;
int math_gamma(phloat x, phloat *gamma) MATH2_SECT;

#endif
