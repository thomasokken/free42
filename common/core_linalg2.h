/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2009  Thomas Okken
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

#ifndef CORE_LINALG2_H
#define CORE_LINALG2_H 1

#include "core_globals.h"

int lu_decomp_r(vartype_realmatrix *a, int4 *perm,
		       int (*completion)(int, vartype_realmatrix *,
					  int4 *, phloat)) LINALG2_SECT;

int lu_decomp_c(vartype_complexmatrix *a, int4 *perm,
		       int (*completion)(int, vartype_complexmatrix *,
					  int4 *, phloat, phloat)) LINALG2_SECT;

int lu_backsubst_rr(vartype_realmatrix *a,
			    int4 *perm,
			    vartype_realmatrix *b,
			    void (*completion)(int, vartype_realmatrix *,
				    int4 *, vartype_realmatrix *)) LINALG2_SECT;

int lu_backsubst_rc(vartype_realmatrix *a,
			    int4 *perm,
			    vartype_complexmatrix *b,
			    void (*completion)(int, vartype_realmatrix *,
				int4 *, vartype_complexmatrix *)) LINALG2_SECT;

int lu_backsubst_cc(vartype_complexmatrix *a,
			    int4 *perm,
			    vartype_complexmatrix *b,
			    void (*completion)(int, vartype_complexmatrix *,
				int4 *, vartype_complexmatrix *)) LINALG2_SECT;

#endif
