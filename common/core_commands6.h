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

#ifndef CORE_COMMANDS6_H
#define CORE_COMMANDS6_H 1

#include "free42.h"
#include "core_globals.h"

int docmd_sin(arg_struct *arg);
int docmd_cos(arg_struct *arg);
int docmd_tan(arg_struct *arg);
int docmd_asin(arg_struct *arg);
int docmd_acos(arg_struct *arg);
int docmd_atan(arg_struct *arg);
int docmd_log(arg_struct *arg);
int docmd_10_pow_x(arg_struct *arg);
int docmd_ln(arg_struct *arg);
int docmd_e_pow_x(arg_struct *arg);
int docmd_sqrt(arg_struct *arg);
int docmd_square(arg_struct *arg);
int docmd_inv(arg_struct *arg);
int docmd_y_pow_x(arg_struct *arg);

#endif
