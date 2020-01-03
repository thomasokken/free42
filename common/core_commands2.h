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

#ifndef CORE_COMMANDS2_H
#define CORE_COMMANDS2_H 1

#include "free42.h"
#include "core_globals.h"

int docmd_sf(arg_struct *arg);
int docmd_cf(arg_struct *arg);
int docmd_fs_t(arg_struct *arg);
int docmd_fc_t(arg_struct *arg);
int docmd_fsc_t(arg_struct *arg);
int docmd_fcc_t(arg_struct *arg);
int docmd_comb(arg_struct *arg);
int docmd_perm(arg_struct *arg);
int docmd_fact(arg_struct *arg);
int docmd_gamma(arg_struct *arg);
int docmd_ran(arg_struct *arg);
int docmd_seed(arg_struct *arg);
int docmd_lbl(arg_struct *arg);
int docmd_rtn(arg_struct *arg);
int docmd_input(arg_struct *arg);
int view_helper(arg_struct *arg, int print);
int docmd_view(arg_struct *arg);
int docmd_aview(arg_struct *arg);
int docmd_xeq(arg_struct *arg);
int docmd_prompt(arg_struct *arg);
int docmd_pse(arg_struct *arg);
int docmd_isg(arg_struct *arg);
int docmd_dse(arg_struct *arg);
int docmd_aip(arg_struct *arg);
int docmd_xtoa(arg_struct *arg);
int docmd_agraph(arg_struct *arg);
int docmd_pixel(arg_struct *arg);
int docmd_beep(arg_struct *arg);
int docmd_tone(arg_struct *arg);
int docmd_mvar(arg_struct *arg);
int docmd_varmenu(arg_struct *arg);
int docmd_getkey(arg_struct *arg);
int docmd_menu(arg_struct *arg);
int docmd_x_eq_0(arg_struct *arg);
int docmd_x_ne_0(arg_struct *arg);
int docmd_x_lt_0(arg_struct *arg);
int docmd_x_gt_0(arg_struct *arg);
int docmd_x_le_0(arg_struct *arg);
int docmd_x_ge_0(arg_struct *arg);
int docmd_x_eq_y(arg_struct *arg);
int docmd_x_ne_y(arg_struct *arg);
int docmd_x_lt_y(arg_struct *arg);
int docmd_x_gt_y(arg_struct *arg);
int docmd_x_le_y(arg_struct *arg);
int docmd_x_ge_y(arg_struct *arg);
int docmd_prsigma(arg_struct *arg);
int docmd_prp(arg_struct *arg);
int docmd_prv(arg_struct *arg);
int docmd_prstk(arg_struct *arg);
int docmd_pra(arg_struct *arg);
int docmd_prx(arg_struct *arg);
int docmd_prusr(arg_struct *arg);
int docmd_list(arg_struct *arg);
int docmd_adv(arg_struct *arg);
int docmd_prlcd(arg_struct *arg);
int docmd_delay(arg_struct *arg);
int docmd_pon(arg_struct *arg);
int docmd_poff(arg_struct *arg);
int docmd_man(arg_struct *arg);
int docmd_norm(arg_struct *arg);
int docmd_trace(arg_struct *arg);
int docmd_gto(arg_struct *arg);
int docmd_end(arg_struct *arg);
int docmd_number(arg_struct *arg);
int docmd_string(arg_struct *arg);
int docmd_gtodot(arg_struct *arg);
int docmd_gtodotdot(arg_struct *arg);
int docmd_stop(arg_struct *arg);
int docmd_newmat(arg_struct *arg);
int docmd_rup(arg_struct *arg);
int docmd_real_t(arg_struct *arg);
int docmd_cpx_t(arg_struct *arg);
int docmd_str_t(arg_struct *arg);
int docmd_mat_t(arg_struct *arg);
int docmd_dim_t(arg_struct *arg);
int docmd_asgn01(arg_struct *arg);
int docmd_asgn02(arg_struct *arg);
int docmd_asgn03(arg_struct *arg);
int docmd_asgn04(arg_struct *arg);
int docmd_asgn05(arg_struct *arg);
int docmd_asgn06(arg_struct *arg);
int docmd_asgn07(arg_struct *arg);
int docmd_asgn08(arg_struct *arg);
int docmd_asgn09(arg_struct *arg);
int docmd_asgn10(arg_struct *arg);
int docmd_asgn11(arg_struct *arg);
int docmd_asgn12(arg_struct *arg);
int docmd_asgn13(arg_struct *arg);
int docmd_asgn14(arg_struct *arg);
int docmd_asgn15(arg_struct *arg);
int docmd_asgn16(arg_struct *arg);
int docmd_asgn17(arg_struct *arg);
int docmd_asgn18(arg_struct *arg);
int docmd_on(arg_struct *arg);
int docmd_off(arg_struct *arg);
int docmd_key1g(arg_struct *arg);
int docmd_key2g(arg_struct *arg);
int docmd_key3g(arg_struct *arg);
int docmd_key4g(arg_struct *arg);
int docmd_key5g(arg_struct *arg);
int docmd_key6g(arg_struct *arg);
int docmd_key7g(arg_struct *arg);
int docmd_key8g(arg_struct *arg);
int docmd_key9g(arg_struct *arg);
int docmd_key1x(arg_struct *arg);
int docmd_key2x(arg_struct *arg);
int docmd_key3x(arg_struct *arg);
int docmd_key4x(arg_struct *arg);
int docmd_key5x(arg_struct *arg);
int docmd_key6x(arg_struct *arg);
int docmd_key7x(arg_struct *arg);
int docmd_key8x(arg_struct *arg);
int docmd_key9x(arg_struct *arg);
int docmd_vmsto(arg_struct *arg);
int docmd_vmsto2(arg_struct *arg);
int docmd_sigma_reg(arg_struct *arg);
int docmd_sigma_reg_t(arg_struct *arg);
int docmd_cld(arg_struct *arg);

#endif
