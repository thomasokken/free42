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

#ifndef CORE_MATH2_H
#define CORE_MATH2_H 1

#include "core_phloat.h"

phloat math_random() MATH2_SECT;
int math_asinh_acosh(phloat xre, phloat xim,
			    phloat *yre, phloat *yim, int do_asinh) MATH2_SECT;
int math_atanh(phloat xre, phloat xim, phloat *yre, phloat *yim) MATH2_SECT;
int math_gamma(phloat x, phloat *gamma) MATH2_SECT;

#endif
