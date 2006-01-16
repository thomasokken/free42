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

#ifndef CORE_COMMANDS3_H
#define CORE_COMMANDS3_H 1

#include "free42.h"
#include "core_globals.h"

int matedit_get_dim(int4 *rows, int4 *columns) COMMANDS3_SECT;

int docmd_acosh(arg_struct *arg) COMMANDS3_SECT;
int docmd_aleng(arg_struct *arg) COMMANDS3_SECT;
int docmd_allsigma(arg_struct *arg) COMMANDS3_SECT;
int docmd_and(arg_struct *arg) COMMANDS3_SECT;
int docmd_aoff(arg_struct *arg) COMMANDS3_SECT;
int docmd_aon(arg_struct *arg) COMMANDS3_SECT;
int docmd_arot(arg_struct *arg) COMMANDS3_SECT;
int docmd_ashf(arg_struct *arg) COMMANDS3_SECT;
int docmd_asinh(arg_struct *arg) COMMANDS3_SECT;
int docmd_atanh(arg_struct *arg) COMMANDS3_SECT;
int docmd_atox(arg_struct *arg) COMMANDS3_SECT;
int docmd_baseadd(arg_struct *arg) COMMANDS3_SECT;
int docmd_basesub(arg_struct *arg) COMMANDS3_SECT;
int docmd_basemul(arg_struct *arg) COMMANDS3_SECT;
int docmd_basediv(arg_struct *arg) COMMANDS3_SECT;
int docmd_basechs(arg_struct *arg) COMMANDS3_SECT;
int docmd_best(arg_struct *arg) COMMANDS3_SECT;
int docmd_binm(arg_struct *arg) COMMANDS3_SECT;
int docmd_bit_t(arg_struct *arg) COMMANDS3_SECT;
int docmd_corr(arg_struct *arg) COMMANDS3_SECT;
int docmd_cosh(arg_struct *arg) COMMANDS3_SECT;
int docmd_cross(arg_struct *arg) COMMANDS3_SECT;
int docmd_custom(arg_struct *arg) COMMANDS3_SECT;
int docmd_decm(arg_struct *arg) COMMANDS3_SECT;
int docmd_delr(arg_struct *arg) COMMANDS3_SECT;
int docmd_det(arg_struct *arg) COMMANDS3_SECT;
int docmd_dim(arg_struct *arg) COMMANDS3_SECT;
int docmd_dot(arg_struct *arg) COMMANDS3_SECT;
int docmd_edit(arg_struct *arg) COMMANDS3_SECT;
int docmd_editn(arg_struct *arg) COMMANDS3_SECT;
int docmd_exitall(arg_struct *arg) COMMANDS3_SECT;
int docmd_expf(arg_struct *arg) COMMANDS3_SECT;
int docmd_e_pow_x_1(arg_struct *arg) COMMANDS3_SECT;
int docmd_fcstx(arg_struct *arg) COMMANDS3_SECT;
int docmd_fcsty(arg_struct *arg) COMMANDS3_SECT;
int docmd_fnrm(arg_struct *arg) COMMANDS3_SECT;
int docmd_getm(arg_struct *arg) COMMANDS3_SECT;
int docmd_grow(arg_struct *arg) COMMANDS3_SECT;
int docmd_hexm(arg_struct *arg) COMMANDS3_SECT;
int docmd_hmsadd(arg_struct *arg) COMMANDS3_SECT;
int docmd_hmssub(arg_struct *arg) COMMANDS3_SECT;
int docmd_i_add(arg_struct *arg) COMMANDS3_SECT;
int docmd_i_sub(arg_struct *arg) COMMANDS3_SECT;
void matedit_goto(int4 row, int4 column) COMMANDS3_SECT;
int docmd_index(arg_struct *arg) COMMANDS3_SECT;
int docmd_octm(arg_struct *arg) COMMANDS3_SECT;
int docmd_mean(arg_struct *arg) COMMANDS3_SECT;
int docmd_sdev(arg_struct *arg) COMMANDS3_SECT;
int docmd_slope(arg_struct *arg) COMMANDS3_SECT;
int docmd_sum(arg_struct *arg) COMMANDS3_SECT;
int docmd_uvec(arg_struct *arg) COMMANDS3_SECT;
int docmd_wmean(arg_struct *arg) COMMANDS3_SECT;
int docmd_yint(arg_struct *arg) COMMANDS3_SECT;

#endif
