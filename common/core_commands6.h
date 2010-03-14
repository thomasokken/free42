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

#ifndef CORE_COMMANDS6_H
#define CORE_COMMANDS6_H 1

#include "free42.h"
#include "core_globals.h"

int docmd_sin(arg_struct *arg) COMMANDS6_SECT;
int docmd_cos(arg_struct *arg) COMMANDS6_SECT;
int docmd_tan(arg_struct *arg) COMMANDS6_SECT;
int docmd_asin(arg_struct *arg) COMMANDS6_SECT;
int docmd_acos(arg_struct *arg) COMMANDS6_SECT;
int docmd_atan(arg_struct *arg) COMMANDS6_SECT;
int docmd_log(arg_struct *arg) COMMANDS6_SECT;
int docmd_10_pow_x(arg_struct *arg) COMMANDS6_SECT;
int docmd_ln(arg_struct *arg) COMMANDS6_SECT;
int docmd_e_pow_x(arg_struct *arg) COMMANDS6_SECT;
int docmd_sqrt(arg_struct *arg) COMMANDS6_SECT;
int docmd_square(arg_struct *arg) COMMANDS6_SECT;
int docmd_inv(arg_struct *arg) COMMANDS6_SECT;
int docmd_y_pow_x(arg_struct *arg) COMMANDS6_SECT;

#ifdef IPHONE
int docmd_accel(arg_struct *arg) COMMANDS6_SECT;
int docmd_locat(arg_struct *arg) COMMANDS6_SECT;
int docmd_heading(arg_struct *arg) COMMANDS6_SECT;
#endif

int docmd_adate(arg_struct *arg) COMMANDS6_SECT;
int docmd_almcat(arg_struct *arg) COMMANDS6_SECT;
int docmd_almnow(arg_struct *arg) COMMANDS6_SECT;
int docmd_atime(arg_struct *arg) COMMANDS6_SECT;
int docmd_atime24(arg_struct *arg) COMMANDS6_SECT;
int docmd_clk12(arg_struct *arg) COMMANDS6_SECT;
int docmd_clk24(arg_struct *arg) COMMANDS6_SECT;
int docmd_clkt(arg_struct *arg) COMMANDS6_SECT;
int docmd_clktd(arg_struct *arg) COMMANDS6_SECT;
int docmd_clock(arg_struct *arg) COMMANDS6_SECT;
int docmd_correct(arg_struct *arg) COMMANDS6_SECT;
int docmd_date(arg_struct *arg) COMMANDS6_SECT;
int docmd_date_plus(arg_struct *arg) COMMANDS6_SECT;
int docmd_ddays(arg_struct *arg) COMMANDS6_SECT;
int docmd_dmy(arg_struct *arg) COMMANDS6_SECT;
int docmd_dow(arg_struct *arg) COMMANDS6_SECT;
int docmd_mdy(arg_struct *arg) COMMANDS6_SECT;
int docmd_rclaf(arg_struct *arg) COMMANDS6_SECT;
int docmd_rclsw(arg_struct *arg) COMMANDS6_SECT;
int docmd_runsw(arg_struct *arg) COMMANDS6_SECT;
int docmd_setaf(arg_struct *arg) COMMANDS6_SECT;
int docmd_setdate(arg_struct *arg) COMMANDS6_SECT;
int docmd_setime(arg_struct *arg) COMMANDS6_SECT;
int docmd_setsw(arg_struct *arg) COMMANDS6_SECT;
int docmd_stopsw(arg_struct *arg) COMMANDS6_SECT;
int docmd_sw(arg_struct *arg) COMMANDS6_SECT;
int docmd_t_plus_x(arg_struct *arg) COMMANDS6_SECT;
int docmd_time(arg_struct *arg) COMMANDS6_SECT;
int docmd_xyzalm(arg_struct *arg) COMMANDS6_SECT;
int docmd_clalma(arg_struct *arg) COMMANDS6_SECT;
int docmd_clalmx(arg_struct *arg) COMMANDS6_SECT;
int docmd_clralms(arg_struct *arg) COMMANDS6_SECT;
int docmd_rclalm(arg_struct *arg) COMMANDS6_SECT;
int docmd_swpt(arg_struct *arg) COMMANDS6_SECT;

#endif
