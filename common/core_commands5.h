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

#ifndef CORE_COMMANDS5_H
#define CORE_COMMANDS5_H 1

#include "free42.h"
#include "core_globals.h"

int docmd_binm(arg_struct *arg);
int docmd_octm(arg_struct *arg);
int docmd_decm(arg_struct *arg);
int docmd_hexm(arg_struct *arg);
int docmd_linf(arg_struct *arg);
int docmd_logf(arg_struct *arg);
int docmd_expf(arg_struct *arg);
int docmd_pwrf(arg_struct *arg);
int docmd_allsigma(arg_struct *arg);
int docmd_and(arg_struct *arg);
int docmd_baseadd(arg_struct *arg);
int docmd_basesub(arg_struct *arg);
int docmd_basemul(arg_struct *arg);
int docmd_basediv(arg_struct *arg);
int docmd_basechs(arg_struct *arg);
int docmd_best(arg_struct *arg);
int docmd_bit_t(arg_struct *arg);
int docmd_corr(arg_struct *arg);
int docmd_fcstx(arg_struct *arg);
int docmd_fcsty(arg_struct *arg);
int docmd_mean(arg_struct *arg);
int docmd_sdev(arg_struct *arg);
int docmd_slope(arg_struct *arg);
int docmd_sum(arg_struct *arg);
int docmd_wmean(arg_struct *arg);
int docmd_yint(arg_struct *arg);

int docmd_integ(arg_struct *arg);
int docmd_linsigma(arg_struct *arg);
int docmd_not(arg_struct *arg);
int docmd_or(arg_struct *arg);
int docmd_pgmint(arg_struct *arg);
int docmd_pgmslv(arg_struct *arg);
int docmd_pgminti(arg_struct *arg);
int docmd_pgmslvi(arg_struct *arg);
int docmd_rotxy(arg_struct *arg);
int docmd_solve(arg_struct *arg);
int docmd_vmsolve(arg_struct *arg);
int docmd_xor(arg_struct *arg);
int docmd_to_dec(arg_struct *arg);
int docmd_to_oct(arg_struct *arg);
int docmd_sigmaadd(arg_struct *arg);
int docmd_sigmasub(arg_struct *arg);

#endif
