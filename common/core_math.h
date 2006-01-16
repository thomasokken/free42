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

#ifndef CORE_MATH_H
#define CORE_MATH_H 1

#include "core_globals.h"

int persist_math() MATH_SECT;
int unpersist_math() MATH_SECT;
void reset_math() MATH_SECT;

double math_random() MATH_SECT;
int math_asinh_acosh(double xre, double xim,
			    double *yre, double *yim, int do_asinh) MATH_SECT;
int math_atanh(double xre, double xim, double *yre, double *yim) MATH_SECT;
int math_gamma(double x, double *gamma) MATH_SECT;

void put_shadow(const char *name, int length, double value) MATH_SECT;
int get_shadow(const char *name, int length, double *value) MATH_SECT;
void remove_shadow(const char *name, int length) MATH_SECT;
void set_solve_prgm(const char *name, int length) MATH_SECT;
int start_solve(const char *name, int length, double x1, double x2) MATH_SECT;
int return_to_solve(int failure) MATH_SECT;

void set_integ_prgm(const char *name, int length) MATH_SECT;
void get_integ_prgm(char *name, int *length) MATH_SECT;
void set_integ_var(const char *name, int length) MATH_SECT;
void get_integ_var(char *name, int *length) MATH_SECT;
int start_integ(const char *name, int length) MATH_SECT;
int return_to_integ(int failure) MATH_SECT;

#endif
