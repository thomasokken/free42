/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2010  Thomas Okken
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

#ifndef CORE_COMMANDS3_H
#define CORE_COMMANDS3_H 1

#include "free42.h"
#include "core_globals.h"

int matedit_get_dim(int4 *rows, int4 *columns) COMMANDS3_SECT;

int docmd_acosh(arg_struct *arg) COMMANDS3_SECT;
int docmd_aleng(arg_struct *arg) COMMANDS3_SECT;
int docmd_aoff(arg_struct *arg) COMMANDS3_SECT;
int docmd_aon(arg_struct *arg) COMMANDS3_SECT;
int docmd_arot(arg_struct *arg) COMMANDS3_SECT;
int docmd_ashf(arg_struct *arg) COMMANDS3_SECT;
int docmd_asinh(arg_struct *arg) COMMANDS3_SECT;
int docmd_atanh(arg_struct *arg) COMMANDS3_SECT;
int docmd_atox(arg_struct *arg) COMMANDS3_SECT;
int docmd_cosh(arg_struct *arg) COMMANDS3_SECT;
int docmd_cross(arg_struct *arg) COMMANDS3_SECT;
int docmd_custom(arg_struct *arg) COMMANDS3_SECT;
int docmd_delr(arg_struct *arg) COMMANDS3_SECT;
int docmd_det(arg_struct *arg) COMMANDS3_SECT;
int docmd_dim(arg_struct *arg) COMMANDS3_SECT;
int docmd_dot(arg_struct *arg) COMMANDS3_SECT;
int docmd_edit(arg_struct *arg) COMMANDS3_SECT;
int docmd_editn(arg_struct *arg) COMMANDS3_SECT;
int docmd_exitall(arg_struct *arg) COMMANDS3_SECT;
int docmd_e_pow_x_1(arg_struct *arg) COMMANDS3_SECT;
int docmd_fnrm(arg_struct *arg) COMMANDS3_SECT;
int docmd_getm(arg_struct *arg) COMMANDS3_SECT;
int docmd_grow(arg_struct *arg) COMMANDS3_SECT;
int docmd_hmsadd(arg_struct *arg) COMMANDS3_SECT;
int docmd_hmssub(arg_struct *arg) COMMANDS3_SECT;
int docmd_i_add(arg_struct *arg) COMMANDS3_SECT;
int docmd_i_sub(arg_struct *arg) COMMANDS3_SECT;
void matedit_goto(int4 row, int4 column) COMMANDS3_SECT;
int docmd_index(arg_struct *arg) COMMANDS3_SECT;
int docmd_uvec(arg_struct *arg) COMMANDS3_SECT;

#endif
