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

#ifndef CORE_VARIABLES_H
#define CORE_VARIABLES_H 1

#include "core_phloat.h"

vartype *new_real(phloat value);
vartype *new_complex(phloat re, phloat im);
vartype *new_string(const char *s, int slen);
vartype *new_realmatrix(int4 rows, int4 columns);
vartype *new_complexmatrix(int4 rows, int4 columns);
vartype *new_matrix_alias(vartype *m);
void free_vartype(vartype *v);
void clean_vartype_pools();
vartype *dup_vartype(const vartype *v);
int disentangle(vartype *v);
int lookup_var(const char *name, int namelength);
vartype *recall_var(const char *name, int namelength);
bool ensure_var_space(int n);
int store_var(const char *name, int namelength, vartype *value, bool local = false);
void purge_var(const char *name, int namelength);
void purge_all_vars();
int vars_exist(int real, int cpx, int matrix);
int contains_no_strings(const vartype_realmatrix *rm);
int matrix_copy(vartype *dst, const vartype *src);

#endif
