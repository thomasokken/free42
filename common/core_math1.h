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

#ifndef CORE_MATH1_H
#define CORE_MATH1_H 1

#include "free42.h"
#include "core_phloat.h"

bool persist_math();
bool unpersist_math(int ver, bool discard);
void reset_math();

void put_shadow(const char *name, int length, phloat value);
int get_shadow(const char *name, int length, phloat *value);
void remove_shadow(const char *name, int length);
void set_solve_prgm(const char *name, int length);
int start_solve(const char *name, int length, phloat x1, phloat x2);
int return_to_solve(int failure, bool stop);

void set_integ_prgm(const char *name, int length);
void get_integ_prgm(char *name, int *length);
void set_integ_var(const char *name, int length);
void get_integ_var(char *name, int *length);
int start_integ(const char *name, int length);
int return_to_integ(int failure, bool stop);

#endif
