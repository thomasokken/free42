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

#ifndef CORE_COMMANDS4_H
#define CORE_COMMANDS4_H 1

#include "free42.h"
#include "core_globals.h"

int docmd_insr(arg_struct *arg);
int docmd_invrt(arg_struct *arg);
int docmd_j_add(arg_struct *arg);
int docmd_j_sub(arg_struct *arg);
int docmd_ln_1_x(arg_struct *arg);
int docmd_old(arg_struct *arg);
int docmd_posa(arg_struct *arg);
int docmd_putm(arg_struct *arg);
int docmd_rclel(arg_struct *arg);
int docmd_rclij(arg_struct *arg);
int docmd_rnrm(arg_struct *arg);
int docmd_rsum(arg_struct *arg);
int docmd_swap_r(arg_struct *arg);
int docmd_sinh(arg_struct *arg);
int docmd_stoel(arg_struct *arg);
int docmd_stoij(arg_struct *arg);
int docmd_tanh(arg_struct *arg);
int docmd_trans(arg_struct *arg);
int docmd_wrap(arg_struct *arg);
int docmd_x_swap(arg_struct *arg);
int docmd_left(arg_struct *arg);
int docmd_up(arg_struct *arg);
int docmd_down(arg_struct *arg);
int docmd_right(arg_struct *arg);
int docmd_percent_ch(arg_struct *arg);
int docmd_simq(arg_struct *arg);
int docmd_mata(arg_struct *arg);
int docmd_matb(arg_struct *arg);
int docmd_matx(arg_struct *arg);
int docmd_max(arg_struct *arg);
int docmd_min(arg_struct *arg);
int docmd_find(arg_struct *arg);
int docmd_xrom(arg_struct *arg);

#endif
