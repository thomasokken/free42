/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2021  Thomas Okken
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

#ifndef CORE_COMMANDS7_H
#define CORE_COMMANDS7_H 1

#include "free42.h"
#include "core_globals.h"

int docmd_accel(arg_struct *arg);
int docmd_locat(arg_struct *arg);
int docmd_heading(arg_struct *arg);

int docmd_adate(arg_struct *arg);
int docmd_atime(arg_struct *arg);
int docmd_atime24(arg_struct *arg);
int docmd_clk12(arg_struct *arg);
int docmd_clk24(arg_struct *arg);
int docmd_date(arg_struct *arg);
int docmd_date_plus(arg_struct *arg);
int docmd_ddays(arg_struct *arg);
int docmd_dmy(arg_struct *arg);
int docmd_dow(arg_struct *arg);
int docmd_mdy(arg_struct *arg);
int docmd_time(arg_struct *arg);
int docmd_ymd(arg_struct *arg);
int docmd_getkey1(arg_struct *arg);

int docmd_fptest(arg_struct *arg);

int docmd_lsto(arg_struct *arg);
int docmd_lasto(arg_struct *arg);
int docmd_wsize(arg_struct *arg);
int docmd_wsize_t(arg_struct *arg);
int docmd_bsigned(arg_struct *arg);
int docmd_bwrap(arg_struct *arg);
int docmd_breset(arg_struct *arg);

int docmd_nop(arg_struct *arg);
int docmd_fma(arg_struct *arg);
int docmd_func(arg_struct *arg);
int docmd_errmsg(arg_struct *arg);
int docmd_errno(arg_struct *arg);
int docmd_rtnyes(arg_struct *arg);
int docmd_rtnno(arg_struct *arg);
int docmd_rtnerr(arg_struct *arg);
int docmd_strace(arg_struct *arg);
int docmd_varmnu1(arg_struct *arg);
int docmd_x2line(arg_struct *arg);
int docmd_a2line(arg_struct *arg);
int docmd_rcomplx(arg_struct *arg);
int docmd_pcomplx(arg_struct *arg);

int docmd_4stk(arg_struct *arg);
int docmd_l4stk(arg_struct *arg);
int docmd_nstk(arg_struct *arg);
int docmd_lnstk(arg_struct *arg);
int docmd_depth(arg_struct *arg);
int docmd_drop(arg_struct *arg);
int docmd_dropn(arg_struct *arg);
int docmd_dup(arg_struct *arg);
int docmd_dupn(arg_struct *arg);
int docmd_pick(arg_struct *arg);
int docmd_unpick(arg_struct *arg);
int docmd_rdnn(arg_struct *arg);
int docmd_rupn(arg_struct *arg);

int docmd_pgmmenu(arg_struct *arg);
int docmd_prmvar(arg_struct *arg);

int docmd_x_eq_nn(arg_struct *arg);
int docmd_x_ne_nn(arg_struct *arg);
int docmd_x_lt_nn(arg_struct *arg);
int docmd_x_gt_nn(arg_struct *arg);
int docmd_x_le_nn(arg_struct *arg);
int docmd_x_ge_nn(arg_struct *arg);
int docmd_0_eq_nn(arg_struct *arg);
int docmd_0_ne_nn(arg_struct *arg);
int docmd_0_lt_nn(arg_struct *arg);
int docmd_0_gt_nn(arg_struct *arg);
int docmd_0_le_nn(arg_struct *arg);
int docmd_0_ge_nn(arg_struct *arg);

int docmd_xstr(arg_struct *arg);

#endif
