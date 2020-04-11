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

#ifndef CORE_LINALG2_H
#define CORE_LINALG2_H 1

#include "core_globals.h"

int lu_decomp_r(vartype_realmatrix *a, int4 *perm,
                       int (*completion)(int, vartype_realmatrix *,
                                          int4 *, phloat));

int lu_decomp_c(vartype_complexmatrix *a, int4 *perm,
                       int (*completion)(int, vartype_complexmatrix *,
                                          int4 *, phloat, phloat));

int lu_backsubst_rr(vartype_realmatrix *a,
                            int4 *perm,
                            vartype_realmatrix *b,
                            void (*completion)(int, vartype_realmatrix *,
                                    int4 *, vartype_realmatrix *));

int lu_backsubst_rc(vartype_realmatrix *a,
                            int4 *perm,
                            vartype_complexmatrix *b,
                            void (*completion)(int, vartype_realmatrix *,
                                int4 *, vartype_complexmatrix *));

int lu_backsubst_cc(vartype_complexmatrix *a,
                            int4 *perm,
                            vartype_complexmatrix *b,
                            void (*completion)(int, vartype_complexmatrix *,
                                int4 *, vartype_complexmatrix *));

#endif
