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

#include <stdlib.h>

#include "core_tables.h"
#include "core_commands1.h"
#include "core_commands2.h"
#include "core_commands3.h"
#include "core_commands4.h"
#include "core_commands5.h"
#include "core_commands6.h"
#include "core_commands7.h"


#if !defined(ANDROID) && !defined(IPHONE)
#define docmd_accel docmd_xrom
#define docmd_locat docmd_xrom
#define docmd_heading docmd_xrom
#endif

static const command_spec cmd_array[] =
{
    { /* CLX */         "CLX",                  3, docmd_clx,         0x00000077, ARG_NONE,  FLAG_NONE },
    { /* ENTER */       "ENT\305R",             5, docmd_enter,       0x00000083, ARG_NONE,  FLAG_NONE },
    { /* SWAP */        "X<>Y",                 4, docmd_swap,        0x00000071, ARG_NONE,  FLAG_NONE },
    { /* RDN */         "R\016",                2, docmd_rdn,         0x00000075, ARG_NONE,  FLAG_NONE },
    { /* CHS */         "+/-",                  3, docmd_chs,         0x00000054, ARG_NONE,  FLAG_NONE },
    { /* DIV */         "\000",                 1, docmd_div,         0x00000043, ARG_NONE,  FLAG_NONE },
    { /* MUL */         "\001",                 1, docmd_mul,         0x00000042, ARG_NONE,  FLAG_NONE },
    { /* SUB */         "-",                    1, docmd_sub,         0x00000041, ARG_NONE,  FLAG_NONE },
    { /* ADD */         "+",                    1, docmd_add,         0x00000040, ARG_NONE,  FLAG_NONE },
    { /* LASTX */       "LASTX",                5, docmd_lastx,       0x00000076, ARG_NONE,  FLAG_NONE },
    { /* SILENT_OFF */  "",                     0, NULL,              0x02000000, ARG_NONE,  FLAG_HIDDEN + FLAG_NO_SHOW },
    { /* SILENT_ON */   "",                     0, NULL,              0x02000000, ARG_NONE,  FLAG_HIDDEN + FLAG_NO_SHOW },
    { /* SIN */         "SIN",                  3, docmd_sin,         0x00000059, ARG_NONE,  FLAG_NONE },
    { /* COS */         "COS",                  3, docmd_cos,         0x0000005a, ARG_NONE,  FLAG_NONE },
    { /* TAN */         "TAN",                  3, docmd_tan,         0x0000005b, ARG_NONE,  FLAG_NONE },
    { /* ASIN */        "ASIN",                 4, docmd_asin,        0x0000005c, ARG_NONE,  FLAG_NONE },
    { /* ACOS */        "ACOS",                 4, docmd_acos,        0x0000005d, ARG_NONE,  FLAG_NONE },
    { /* ATAN */        "ATAN",                 4, docmd_atan,        0x0000005e, ARG_NONE,  FLAG_NONE },
    { /* LOG */         "LOG",                  3, docmd_log,         0x00000056, ARG_NONE,  FLAG_NONE },
    { /* 10_POW_X */    "10^X",                 4, docmd_10_pow_x,    0x00000057, ARG_NONE,  FLAG_NONE },
    { /* LN */          "LN",                   2, docmd_ln,          0x00000050, ARG_NONE,  FLAG_NONE },
    { /* E_POW_X */     "E^X",                  3, docmd_e_pow_x,     0x00000055, ARG_NONE,  FLAG_NONE },
    { /* SQRT */        "SQRT",                 4, docmd_sqrt,        0x00000052, ARG_NONE,  FLAG_NONE },
    { /* SQUARE */      "X^2",                  3, docmd_square,      0x00000051, ARG_NONE,  FLAG_NONE },
    { /* INV */         "1/X",                  3, docmd_inv,         0x00000060, ARG_NONE,  FLAG_NONE },
    { /* Y_POW_X */     "Y^X",                  3, docmd_y_pow_x,     0x00000053, ARG_NONE,  FLAG_NONE },
    { /* PERCENT */     "%",                    1, docmd_percent,     0x0000004c, ARG_NONE,  FLAG_NONE },
    { /* PI */          "PI",                   2, docmd_pi,          0x00000072, ARG_NONE,  FLAG_NONE },
    { /* COMPLEX */     "C\317\315PL\305X",     7, docmd_complex,     0x0000a072, ARG_NONE,  FLAG_NONE },
    { /* STO */         "STO",                  3, docmd_sto,         0x01810091, ARG_VAR,   FLAG_NONE },
    { /* STO_DIV */     "STO\000",              4, docmd_sto_div,     0x00850095, ARG_VAR,   FLAG_NONE },
    { /* STO_MUL */     "STO\001",              4, docmd_sto_mul,     0x00840094, ARG_VAR,   FLAG_NONE },
    { /* STO_SUB */     "STO-",                 4, docmd_sto_sub,     0x00830093, ARG_VAR,   FLAG_NONE },
    { /* STO_ADD */     "STO+",                 4, docmd_sto_add,     0x00820092, ARG_VAR,   FLAG_NONE },
    { /* RCL */         "RCL",                  3, docmd_rcl,         0x01910090, ARG_VAR,   FLAG_NONE },
    { /* RCL_DIV */     "RCL\000",              4, docmd_rcl_div,     0x0095f2d4, ARG_VAR,   FLAG_NONE },
    { /* RCL_MUL */     "RCL\001",              4, docmd_rcl_mul,     0x0094f2d3, ARG_VAR,   FLAG_NONE },
    { /* RCL_SUB */     "RCL-",                 4, docmd_rcl_sub,     0x0093f2d2, ARG_VAR,   FLAG_NONE },
    { /* RCL_ADD */     "RCL+",                 4, docmd_rcl_add,     0x0092f2d1, ARG_VAR,   FLAG_NONE },
    { /* FIX */         "FIX",                  3, docmd_fix,         0x01d4009c, ARG_NUM11, FLAG_NONE },
    { /* SCI */         "SCI",                  3, docmd_sci,         0x01d5009d, ARG_NUM11, FLAG_NONE },
    { /* ENG */         "ENG",                  3, docmd_eng,         0x01d6009e, ARG_NUM11, FLAG_NONE },
    { /* ALL */         "ALL",                  3, docmd_all,         0x0000a25d, ARG_NONE,  FLAG_NONE },
    { /* NULL */        "\316\325\314\314",     4, docmd_null,        0x02000000, ARG_NONE,  FLAG_HIDDEN },
    { /* ASTO */        "ASTO",                 4, docmd_asto,        0x00b2009a, ARG_VAR,   FLAG_NONE },
    { /* ARCL */        "ARCL",                 4, docmd_arcl,        0x00b3009b, ARG_VAR,   FLAG_NONE },
    { /* CLA */         "CLA",                  3, docmd_cla,         0x00000087, ARG_NONE,  FLAG_NONE },
    { /* DEG */         "DEG",                  3, docmd_deg,         0x00000080, ARG_NONE,  FLAG_NONE },
    { /* RAD */         "RAD",                  3, docmd_rad,         0x00000081, ARG_NONE,  FLAG_NONE },
    { /* GRAD */        "GRAD",                 4, docmd_grad,        0x00000082, ARG_NONE,  FLAG_NONE },
    { /* RECT */        "RECT",                 4, docmd_rect,        0x0000a25a, ARG_NONE,  FLAG_NONE },
    { /* POLAR */       "POLAR",                5, docmd_polar,       0x0000a259, ARG_NONE,  FLAG_NONE },
    { /* SIZE */        "SIZE",                 4, docmd_size,        0x01000000, ARG_COUNT, FLAG_NONE },
    { /* QUIET */       "QUIET",                5, docmd_quiet,       0x02000000, ARG_NONE,  FLAG_IMMED },
    { /* CPXRES */      "C\320\330RES",         6, docmd_cpxres,      0x0000a26a, ARG_NONE,  FLAG_NONE },
    { /* REALRES */     "R\305\301\314RES",     7, docmd_realres,     0x0000a26b, ARG_NONE,  FLAG_NONE },
    { /* KEYASN */      "KEY\301\323\316",      6, docmd_keyasn,      0x0000a263, ARG_NONE,  FLAG_NONE },
    { /* LCLBL */       "LCLBL",                5, docmd_lclbl,       0x0000a264, ARG_NONE,  FLAG_NONE },
    { /* RDXDOT */      "RDX.",                 4, docmd_rdxdot,      0x0000a25b, ARG_NONE,  FLAG_NONE },
    { /* RDXCOMMA */    "RDX,",                 4, docmd_rdxcomma,    0x0000a25c, ARG_NONE,  FLAG_NONE },
    { /* CLSIGMA */     "CL\005",               3, docmd_clsigma,     0x00000070, ARG_NONE,  FLAG_NONE },
    { /* CLP */         "CLP",                  3, docmd_clp,         0x00f00000, ARG_PRGM,  FLAG_NONE },
    { /* CLV */         "CLV",                  3, docmd_clv,         0x00b0f2d8, ARG_NAMED, FLAG_NONE },
    { /* CLST */        "CLST",                 4, docmd_clst,        0x00000073, ARG_NONE,  FLAG_NONE },
    { /* CLRG */        "CLRG",                 4, docmd_clrg,        0x0000008a, ARG_NONE,  FLAG_NONE },
    { /* DEL */         "DEL",                  3, docmd_del,         0x02000000, ARG_COUNT, FLAG_PRGM_ONLY + FLAG_IMMED },
    { /* CLKEYS */      "CLK\305Y\323",         6, docmd_clkeys,      0x0000a262, ARG_NONE,  FLAG_NONE },
    { /* CLLCD */       "CLLCD",                5, docmd_cllcd,       0x0000a763, ARG_NONE,  FLAG_NONE },
    { /* CLMENU */      "CLM\305N\325",         6, docmd_clmenu,      0x0000a26d, ARG_NONE,  FLAG_NONE },
    { /* CLALLa */      "CLALL",                5, NULL,              0x02000000, ARG_NONE,  FLAG_IMMED },
    { /* TO_DEG */      "\017DEG",              4, docmd_to_deg,      0x0000006b, ARG_NONE,  FLAG_NONE },
    { /* TO_RAD */      "\017RAD",              4, docmd_to_rad,      0x0000006a, ARG_NONE,  FLAG_NONE },
    { /* TO_HR */       "\017HR",               3, docmd_to_hr,       0x0000006d, ARG_NONE,  FLAG_NONE },
    { /* TO_HMS */      "\017HMS",              4, docmd_to_hms,      0x0000006c, ARG_NONE,  FLAG_NONE },
    { /* TO_REC */      "\017REC",              4, docmd_to_rec,      0x0000004e, ARG_NONE,  FLAG_NONE },
    { /* TO_POL */      "\017POL",              4, docmd_to_pol,      0x0000004f, ARG_NONE,  FLAG_NONE },
    { /* IP */          "IP",                   2, docmd_ip,          0x00000068, ARG_NONE,  FLAG_NONE },
    { /* FP */          "FP",                   2, docmd_fp,          0x00000069, ARG_NONE,  FLAG_NONE },
    { /* RND */         "RND",                  3, docmd_rnd,         0x0000006e, ARG_NONE,  FLAG_NONE },
    { /* ABS */         "ABS",                  3, docmd_abs,         0x00000061, ARG_NONE,  FLAG_NONE },
    { /* SIGN */        "SIGN",                 4, docmd_sign,        0x0000007a, ARG_NONE,  FLAG_NONE },
    { /* MOD */         "MOD",                  3, docmd_mod,         0x0000004b, ARG_NONE,  FLAG_NONE },
    { /* SF */          "SF",                   2, docmd_sf,          0x00a000a8, ARG_NUM99, FLAG_NONE },
    { /* CF */          "CF",                   2, docmd_cf,          0x00a100a9, ARG_NUM99, FLAG_NONE },
    { /* FS_T */        "FS?",                  3, docmd_fs_t,        0x00a400ac, ARG_NUM99, FLAG_NONE },
    { /* FC_T */        "FC?",                  3, docmd_fc_t,        0x00a500ad, ARG_NUM99, FLAG_NONE },
    { /* FSC_T */       "FS?C",                 4, docmd_fsc_t,       0x00a200aa, ARG_NUM99, FLAG_NONE },
    { /* FCC_T */       "FC?C",                 4, docmd_fcc_t,       0x00a300ab, ARG_NUM99, FLAG_NONE },
    { /* COMB */        "COMB",                 4, docmd_comb,        0x0000a06f, ARG_NONE,  FLAG_NONE },
    { /* PERM */        "PERM",                 4, docmd_perm,        0x0000a070, ARG_NONE,  FLAG_NONE },
    { /* FACT */        "N!",                   2, docmd_fact,        0x00000062, ARG_NONE,  FLAG_NONE },
    { /* GAMMA */       "GAM\315\301",          5, docmd_gamma,       0x0000a074, ARG_NONE,  FLAG_NONE },
    { /* RAN */         "RAN",                  3, docmd_ran,         0x0000a071, ARG_NONE,  FLAG_NONE },
    { /* SEED */        "SEED",                 4, docmd_seed,        0x0000a073, ARG_NONE,  FLAG_NONE },
    { /* LBL */         "LBL",                  3, docmd_lbl,         0x010000cf, ARG_OTHER, FLAG_PRGM_ONLY },
    { /* RTN */         "RTN",                  3, docmd_rtn,         0x00000085, ARG_NONE,  FLAG_NONE },
    { /* INPUT */       "INPUT",                5, docmd_input,       0x01c5f2d0, ARG_VAR,   FLAG_PRGM_ONLY },
    { /* VIEW */        "VIEW",                 4, docmd_view,        0x00800098, ARG_VAR,   FLAG_NONE },
    { /* AVIEW */       "AVIEW",                5, docmd_aview,       0x0000007e, ARG_NONE,  FLAG_NONE },
    { /* XEQ */         "XEQ",                  3, docmd_xeq,         0x01a700ae, ARG_LBL,   FLAG_NONE },
    { /* PROMPT */      "PROM\320\324",         6, docmd_prompt,      0x0000008e, ARG_NONE,  FLAG_NONE },
    { /* PSE */         "PSE",                  3, docmd_pse,         0x00000089, ARG_NONE,  FLAG_NONE },
    { /* ISG */         "ISG",                  3, docmd_isg,         0x00960096, ARG_REAL,  FLAG_NONE },
    { /* DSE */         "DSE",                  3, docmd_dse,         0x00970097, ARG_REAL,  FLAG_NONE },
    { /* AIP */         "AIP",                  3, docmd_aip,         0x0000a631, ARG_NONE,  FLAG_NONE },
    { /* XTOA */        "XTOA",                 4, docmd_xtoa,        0x0000a66f, ARG_NONE,  FLAG_NONE },
    { /* AGRAPH */      "AGRA\320\310",         6, docmd_agraph,      0x0000a764, ARG_NONE,  FLAG_NONE },
    { /* PIXEL */       "PIXEL",                5, docmd_pixel,       0x0000a765, ARG_NONE,  FLAG_NONE },
    { /* BEEP */        "BEEP",                 4, docmd_beep,        0x00000086, ARG_NONE,  FLAG_NONE },
    { /* TONE */        "TONE",                 4, docmd_tone,        0x00d7009f, ARG_NUM9,  FLAG_NONE },
    { /* MVAR */        "MVAR",                 4, docmd_mvar,        0x00900000, ARG_RVAR,  FLAG_NONE },
    { /* VARMENU */     "VARM\305\316\325",     7, docmd_varmenu,     0x00c1f2f8, ARG_PRGM,  FLAG_NONE },
    { /* GETKEY */      "GETK\305\331",         6, docmd_getkey,      0x0000a26e, ARG_NONE,  FLAG_NONE },
    { /* MENU */        "MENU",                 4, docmd_menu,        0x0000a25e, ARG_NONE,  FLAG_PRGM_ONLY },
    { /* KEYG */        "KEYG",                 4, NULL,              0x02000000, ARG_MKEY,  FLAG_NONE },
    { /* KEYX */        "KEYX",                 4, NULL,              0x02000000, ARG_MKEY,  FLAG_NONE },
    { /* X_EQ_0 */      "X=0?",                 4, docmd_x_eq_0,      0x00000067, ARG_NONE,  FLAG_NONE },
    { /* X_NE_0 */      "X\0140?",              4, docmd_x_ne_0,      0x00000063, ARG_NONE,  FLAG_NONE },
    { /* X_LT_0 */      "X<0?",                 4, docmd_x_lt_0,      0x00000066, ARG_NONE,  FLAG_NONE },
    { /* X_GT_0 */      "X>0?",                 4, docmd_x_gt_0,      0x00000064, ARG_NONE,  FLAG_NONE },
    { /* X_LE_0 */      "X\0110?",              4, docmd_x_le_0,      0x0000007b, ARG_NONE,  FLAG_NONE },
    { /* X_GE_0 */      "X\0130?",              4, docmd_x_ge_0,      0x0000a25f, ARG_NONE,  FLAG_NONE },
    { /* X_EQ_Y */      "X=Y?",                 4, docmd_x_eq_y,      0x00000078, ARG_NONE,  FLAG_NONE },
    { /* X_NE_Y */      "X\014Y?",              4, docmd_x_ne_y,      0x00000079, ARG_NONE,  FLAG_NONE },
    { /* X_LT_Y */      "X<Y?",                 4, docmd_x_lt_y,      0x00000044, ARG_NONE,  FLAG_NONE },
    { /* X_GT_Y */      "X>Y?",                 4, docmd_x_gt_y,      0x00000045, ARG_NONE,  FLAG_NONE },
    { /* X_LE_Y */      "X\011Y?",              4, docmd_x_le_y,      0x00000046, ARG_NONE,  FLAG_NONE },
    { /* X_GE_Y */      "X\013Y?",              4, docmd_x_ge_y,      0x0000a260, ARG_NONE,  FLAG_NONE },
    { /* PRSIGMA */     "PR\005",               3, docmd_prsigma,     0x0000a752, ARG_NONE,  FLAG_NONE },
    { /* PRP */         "PRP",                  3, docmd_prp,         0x02000000, ARG_PRGM,  FLAG_IMMED },
    { /* PRV */         "PRV",                  3, docmd_prv,         0x00b1f2d9, ARG_NAMED, FLAG_NONE },
    { /* PRSTK */       "PRST\313",             5, docmd_prstk,       0x0000a753, ARG_NONE,  FLAG_NONE },
    { /* PRA */         "PRA",                  3, docmd_pra,         0x0000a748, ARG_NONE,  FLAG_NONE },
    { /* PRX */         "PRX",                  3, docmd_prx,         0x0000a754, ARG_NONE,  FLAG_NONE },
    { /* PRUSR */       "PRUSR",                5, docmd_prusr,       0x0000a761, ARG_NONE,  FLAG_NONE },
    { /* LIST */        "LIST",                 4, docmd_list,        0x02000000, ARG_COUNT, FLAG_IMMED },
    { /* ADV */         "ADV",                  3, docmd_adv,         0x0000008f, ARG_NONE,  FLAG_NONE },
    { /* PRLCD */       "PRLCD",                5, docmd_prlcd,       0x0000a762, ARG_NONE,  FLAG_NONE },
    { /* DELAY */       "DELAY",                5, docmd_delay,       0x0000a760, ARG_NONE,  FLAG_NONE },
    { /* PON */         "P\322ON",              4, docmd_pon,         0x0000a75e, ARG_NONE,  FLAG_NONE },
    { /* POFF */        "P\322OFF",             5, docmd_poff,        0x0000a75f, ARG_NONE,  FLAG_NONE },
    { /* MAN */         "MAN",                  3, docmd_man,         0x0000a75b, ARG_NONE,  FLAG_NONE },
    { /* NORM */        "NORM",                 4, docmd_norm,        0x0000a75c, ARG_NONE,  FLAG_NONE },
    { /* TRACE */       "TRACE",                5, docmd_trace,       0x0000a75d, ARG_NONE,  FLAG_NONE },
    { /* SIGMAADD */    "\005+",                2, docmd_sigmaadd,    0x00000047, ARG_NONE,  FLAG_NONE },
    { /* SIGMASUB */    "\005-",                2, docmd_sigmasub,    0x00000048, ARG_NONE,  FLAG_NONE },
    { /* GTO */         "GTO",                  3, docmd_gto,         0x01a60000, ARG_LBL,   FLAG_NONE },
    { /* END */         "END",                  3, docmd_end,         0x01000000, ARG_NONE,  FLAG_NONE },
    { /* NUMBER */      "",                     0, docmd_number,      0x01000000, ARG_NONE,  FLAG_HIDDEN },
    { /* STRING */      "",                     0, docmd_string,      0x01000000, ARG_NONE,  FLAG_HIDDEN },
    { /* RUN */         "RUN",                  3, NULL,              0x02000000, ARG_NONE,  FLAG_HIDDEN },
    { /* SST */         "SST",                  3, NULL,              0x02000000, ARG_NONE,  FLAG_NONE },
    { /* GTODOT */      "GTO .",                5, docmd_gtodot,      0x02000000, ARG_OTHER, FLAG_IMMED },
    { /* GTODOTDOT */   "GTO ..",               6, docmd_gtodotdot,   0x02000000, ARG_NONE,  FLAG_IMMED },
    { /* STOP */        "STOP",                 4, docmd_stop,        0x00000084, ARG_NONE,  FLAG_NONE },
    { /* NEWMAT */      "NEW\315\301\324",      6, docmd_newmat,      0x0000a6da, ARG_NONE,  FLAG_NONE },
    { /* RUP */         "R^",                   2, docmd_rup,         0x00000074, ARG_NONE,  FLAG_NONE },
    { /* REAL_T */      "RE\301L?",             5, docmd_real_t,      0x0000a265, ARG_NONE,  FLAG_NONE },
    { /* CPX_T */       "CPX?",                 4, docmd_cpx_t,       0x0000a267, ARG_NONE,  FLAG_NONE },
    { /* STR_T */       "STR?",                 4, docmd_str_t,       0x0000a268, ARG_NONE,  FLAG_NONE },
    { /* MAT_T */       "MAT?",                 4, docmd_mat_t,       0x0000a266, ARG_NONE,  FLAG_NONE },
    { /* DIM_T */       "DIM?",                 4, docmd_dim_t,       0x0000a6e7, ARG_NONE,  FLAG_NONE },
    { /* ASSIGNa */     "AS\323\311GN",         6, NULL,              0x02000000, ARG_NAMED, FLAG_NONE },
    { /* ASSIGNb */     "",                     0, NULL,              0x02000000, ARG_CKEY,  FLAG_HIDDEN },
    { /* ASGN01 */      "",                     0, docmd_asgn01,      0x01000000, ARG_OTHER, FLAG_HIDDEN },
    { /* ASGN02 */      "",                     0, docmd_asgn02,      0x01000000, ARG_OTHER, FLAG_HIDDEN },
    { /* ASGN03 */      "",                     0, docmd_asgn03,      0x01000000, ARG_OTHER, FLAG_HIDDEN },
    { /* ASGN04 */      "",                     0, docmd_asgn04,      0x01000000, ARG_OTHER, FLAG_HIDDEN },
    { /* ASGN05 */      "",                     0, docmd_asgn05,      0x01000000, ARG_OTHER, FLAG_HIDDEN },
    { /* ASGN06 */      "",                     0, docmd_asgn06,      0x01000000, ARG_OTHER, FLAG_HIDDEN },
    { /* ASGN07 */      "",                     0, docmd_asgn07,      0x01000000, ARG_OTHER, FLAG_HIDDEN },
    { /* ASGN08 */      "",                     0, docmd_asgn08,      0x01000000, ARG_OTHER, FLAG_HIDDEN },
    { /* ASGN09 */      "",                     0, docmd_asgn09,      0x01000000, ARG_OTHER, FLAG_HIDDEN },
    { /* ASGN10 */      "",                     0, docmd_asgn10,      0x01000000, ARG_OTHER, FLAG_HIDDEN },
    { /* ASGN11 */      "",                     0, docmd_asgn11,      0x01000000, ARG_OTHER, FLAG_HIDDEN },
    { /* ASGN12 */      "",                     0, docmd_asgn12,      0x01000000, ARG_OTHER, FLAG_HIDDEN },
    { /* ASGN13 */      "",                     0, docmd_asgn13,      0x01000000, ARG_OTHER, FLAG_HIDDEN },
    { /* ASGN14 */      "",                     0, docmd_asgn14,      0x01000000, ARG_OTHER, FLAG_HIDDEN },
    { /* ASGN15 */      "",                     0, docmd_asgn15,      0x01000000, ARG_OTHER, FLAG_HIDDEN },
    { /* ASGN16 */      "",                     0, docmd_asgn16,      0x01000000, ARG_OTHER, FLAG_HIDDEN },
    { /* ASGN17 */      "",                     0, docmd_asgn17,      0x01000000, ARG_OTHER, FLAG_HIDDEN },
    { /* ASGN18 */      "",                     0, docmd_asgn18,      0x01000000, ARG_OTHER, FLAG_HIDDEN },
    { /* ON */          "ON",                   2, docmd_on,          0x0000a270, ARG_NONE,  FLAG_NONE },
    { /* OFF */         "OFF",                  3, docmd_off,         0x0000008d, ARG_NONE,  FLAG_NONE },
    { /* KEY1G */       "KEY 1 GTO",            9, docmd_key1g,       0x01000000, ARG_LBL,   FLAG_HIDDEN },
    { /* KEY2G */       "KEY 2 GTO",            9, docmd_key2g,       0x01000000, ARG_LBL,   FLAG_HIDDEN },
    { /* KEY3G */       "KEY 3 GTO",            9, docmd_key3g,       0x01000000, ARG_LBL,   FLAG_HIDDEN },
    { /* KEY4G */       "KEY 4 GTO",            9, docmd_key4g,       0x01000000, ARG_LBL,   FLAG_HIDDEN },
    { /* KEY5G */       "KEY 5 GTO",            9, docmd_key5g,       0x01000000, ARG_LBL,   FLAG_HIDDEN },
    { /* KEY6G */       "KEY 6 GTO",            9, docmd_key6g,       0x01000000, ARG_LBL,   FLAG_HIDDEN },
    { /* KEY7G */       "KEY 7 GTO",            9, docmd_key7g,       0x01000000, ARG_LBL,   FLAG_HIDDEN },
    { /* KEY8G */       "KEY 8 GTO",            9, docmd_key8g,       0x01000000, ARG_LBL,   FLAG_HIDDEN },
    { /* KEY9G */       "KEY 9 GTO",            9, docmd_key9g,       0x01000000, ARG_LBL,   FLAG_HIDDEN },
    { /* KEY1X */       "KEY 1 XEQ",            9, docmd_key1x,       0x01000000, ARG_LBL,   FLAG_HIDDEN },
    { /* KEY2X */       "KEY 2 XEQ",            9, docmd_key2x,       0x01000000, ARG_LBL,   FLAG_HIDDEN },
    { /* KEY3X */       "KEY 3 XEQ",            9, docmd_key3x,       0x01000000, ARG_LBL,   FLAG_HIDDEN },
    { /* KEY4X */       "KEY 4 XEQ",            9, docmd_key4x,       0x01000000, ARG_LBL,   FLAG_HIDDEN },
    { /* KEY5X */       "KEY 5 XEQ",            9, docmd_key5x,       0x01000000, ARG_LBL,   FLAG_HIDDEN },
    { /* KEY6X */       "KEY 6 XEQ",            9, docmd_key6x,       0x01000000, ARG_LBL,   FLAG_HIDDEN },
    { /* KEY7X */       "KEY 7 XEQ",            9, docmd_key7x,       0x01000000, ARG_LBL,   FLAG_HIDDEN },
    { /* KEY8X */       "KEY 8 XEQ",            9, docmd_key8x,       0x01000000, ARG_LBL,   FLAG_HIDDEN },
    { /* KEY9X */       "KEY 9 XEQ",            9, docmd_key9x,       0x01000000, ARG_LBL,   FLAG_HIDDEN },
    { /* VMEXEC */      "",                     0, NULL,              0x02000000, ARG_OTHER, FLAG_HIDDEN },
    { /* VMSTO */       "STO",                  3, docmd_vmsto,       0x02000000, ARG_OTHER, FLAG_HIDDEN },
    { /* SIGMAREG */    "\005REG",              4, docmd_sigma_reg,   0x00d30099, ARG_NUM99, FLAG_NONE },
    { /* SIGMAREG_T */  "\005R\305G?",          5, docmd_sigma_reg_t, 0x0000a678, ARG_NONE,  FLAG_NONE },
    { /* CLD */         "CLD",                  3, docmd_cld,         0x0000007f, ARG_NONE,  FLAG_NONE },
    { /* ACOSH */       "ACOSH",                5, docmd_acosh,       0x0000a066, ARG_NONE,  FLAG_NONE },
    { /* ALENG */       "ALEN\307",             5, docmd_aleng,       0x0000a641, ARG_NONE,  FLAG_NONE },
    { /* ALLSIGMA */    "ALL\005",              4, docmd_allsigma,    0x0000a0ae, ARG_NONE,  FLAG_NONE },
    { /* AND */         "AND",                  3, docmd_and,         0x0000a588, ARG_NONE,  FLAG_NONE },
    { /* AOFF */        "AOFF",                 4, docmd_aoff,        0x0000008b, ARG_NONE,  FLAG_NONE },
    { /* AON */         "AON",                  3, docmd_aon,         0x0000008c, ARG_NONE,  FLAG_NONE },
    { /* AROT */        "AROT",                 4, docmd_arot,        0x0000a646, ARG_NONE,  FLAG_NONE },
    { /* ASHF */        "ASHF",                 4, docmd_ashf,        0x00000088, ARG_NONE,  FLAG_NONE },
    { /* ASINH */       "ASINH",                5, docmd_asinh,       0x0000a064, ARG_NONE,  FLAG_NONE },
    { /* ATANH */       "AT\301NH",             5, docmd_atanh,       0x0000a065, ARG_NONE,  FLAG_NONE },
    { /* ATOX */        "ATOX",                 4, docmd_atox,        0x0000a647, ARG_NONE,  FLAG_NONE },
    { /* BASEADD */     "BASE+",                5, docmd_baseadd,     0x0000a0e6, ARG_NONE,  FLAG_NONE },
    { /* BASESUB */     "BASE-",                5, docmd_basesub,     0x0000a0e7, ARG_NONE,  FLAG_NONE },
    { /* BASEMUL */     "BASE\001",             5, docmd_basemul,     0x0000a0e8, ARG_NONE,  FLAG_NONE },
    { /* BASEDIV */     "BASE\000",             5, docmd_basediv,     0x0000a0e9, ARG_NONE,  FLAG_NONE },
    { /* BASECHS */     "B\301\323\305+/-",     7, docmd_basechs,     0x0000a0ea, ARG_NONE,  FLAG_NONE },
    { /* BEST */        "BEST",                 4, docmd_best,        0x0000a09f, ARG_NONE,  FLAG_NONE },
    { /* BINM */        "BINM",                 4, docmd_binm,        0x0000a0e5, ARG_NONE,  FLAG_NONE },
    { /* BIT_T */       "BIT?",                 4, docmd_bit_t,       0x0000a58c, ARG_NONE,  FLAG_NONE },
    { /* BST */         "BST",                  3, NULL,              0x02000000, ARG_NONE,  FLAG_NONE },
    { /* CORR */        "CORR",                 4, docmd_corr,        0x0000a0a7, ARG_NONE,  FLAG_NONE },
    { /* COSH */        "COSH",                 4, docmd_cosh,        0x0000a062, ARG_NONE,  FLAG_NONE },
    { /* CROSS */       "CROSS",                5, docmd_cross,       0x0000a6ca, ARG_NONE,  FLAG_NONE },
    { /* CUSTOM */      "CUST\317\315",         6, docmd_custom,      0x0000a26f, ARG_NONE,  FLAG_NONE },
    { /* DECM */        "DECM",                 4, docmd_decm,        0x0000a0e3, ARG_NONE,  FLAG_NONE },
    { /* DELR */        "DELR",                 4, docmd_delr,        0x0000a0ab, ARG_NONE,  FLAG_NONE },
    { /* DET */         "DET",                  3, docmd_det,         0x0000a6cc, ARG_NONE,  FLAG_NONE },
    { /* DIM */         "DIM",                  3, docmd_dim,         0x00c4f2ec, ARG_MAT,   FLAG_NONE },
    { /* DOT */         "DOT",                  3, docmd_dot,         0x0000a6cb, ARG_NONE,  FLAG_NONE },
    { /* EDIT */        "EDIT",                 4, docmd_edit,        0x0000a6e1, ARG_NONE,  FLAG_NONE },
    { /* EDITN */       "EDITN",                5, docmd_editn,       0x00c6f2ef, ARG_MAT,   FLAG_NONE },
    { /* EXITALL */     "EXITA\314\314",        7, docmd_exitall,     0x0000a26c, ARG_NONE,  FLAG_NONE },
    { /* EXPF */        "EXPF",                 4, docmd_expf,        0x0000a0a0, ARG_NONE,  FLAG_NONE },
    { /* E_POW_X_1 */   "E^X-\261",             5, docmd_e_pow_x_1,   0x00000058, ARG_NONE,  FLAG_NONE },
    { /* FCSTX */       "FCSTX",                5, docmd_fcstx,       0x0000a0a8, ARG_NONE,  FLAG_NONE },
    { /* FCSTY */       "FCSTY",                5, docmd_fcsty,       0x0000a0a9, ARG_NONE,  FLAG_NONE },
    { /* FNRM */        "FNRM",                 4, docmd_fnrm,        0x0000a6cf, ARG_NONE,  FLAG_NONE },
    { /* GETM */        "GETM",                 4, docmd_getm,        0x0000a6e8, ARG_NONE,  FLAG_NONE },
    { /* GROW */        "GROW",                 4, docmd_grow,        0x0000a6e3, ARG_NONE,  FLAG_NONE },
    { /* HEXM */        "HEXM",                 4, docmd_hexm,        0x0000a0e2, ARG_NONE,  FLAG_NONE },
    { /* HMSADD */      "HMS+",                 4, docmd_hmsadd,      0x00000049, ARG_NONE,  FLAG_NONE },
    { /* HMSSUB */      "HMS-",                 4, docmd_hmssub,      0x0000004a, ARG_NONE,  FLAG_NONE },
    { /* I_ADD */       "I+",                   2, docmd_i_add,       0x0000a6d2, ARG_NONE,  FLAG_NONE },
    { /* I_SUB */       "I-",                   2, docmd_i_sub,       0x0000a6d3, ARG_NONE,  FLAG_NONE },
    { /* INDEX */       "INDEX",                5, docmd_index,       0x0087f2da, ARG_MAT,   FLAG_NONE },
    { /* INSR */        "INSR",                 4, docmd_insr,        0x0000a0aa, ARG_NONE,  FLAG_NONE },
    { /* INTEG */       "INTEG",                5, docmd_integ,       0x00b6f2ea, ARG_RVAR,  FLAG_NONE },
    { /* INVRT */       "INV\322\324",          5, docmd_invrt,       0x0000a6ce, ARG_NONE,  FLAG_NONE },
    { /* J_ADD */       "J+",                   2, docmd_j_add,       0x0000a6d4, ARG_NONE,  FLAG_NONE },
    { /* J_SUB */       "J-",                   2, docmd_j_sub,       0x0000a6d5, ARG_NONE,  FLAG_NONE },
    { /* LINF */        "LINF",                 4, docmd_linf,        0x0000a0a1, ARG_NONE,  FLAG_NONE },
    { /* LINSIGMA */    "LIN\005",              4, docmd_linsigma,    0x0000a0ad, ARG_NONE,  FLAG_NONE },
    { /* LN_1_X */      "LN1+\330",             5, docmd_ln_1_x,      0x00000065, ARG_NONE,  FLAG_NONE },
    { /* LOGF */        "LOGF",                 4, docmd_logf,        0x0000a0a2, ARG_NONE,  FLAG_NONE },
    { /* MEAN */        "MEAN",                 4, docmd_mean,        0x0000007c, ARG_NONE,  FLAG_NONE },
    { /* NOT */         "NOT",                  3, docmd_not,         0x0000a587, ARG_NONE,  FLAG_NONE },
    { /* OCTM */        "OCTM",                 4, docmd_octm,        0x0000a0e4, ARG_NONE,  FLAG_NONE },
    { /* OLD */         "OLD",                  3, docmd_old,         0x0000a6db, ARG_NONE,  FLAG_NONE },
    { /* OR */          "OR",                   2, docmd_or,          0x0000a589, ARG_NONE,  FLAG_NONE },
    { /* PGMSLV */      "P\307\315SLV",         6, docmd_pgmslv,      0x00b5f2e9, ARG_PRGM,  FLAG_NONE },
    { /* PGMINT */      "P\307\315INT",         6, docmd_pgmint,      0x00b4f2e8, ARG_PRGM,  FLAG_NONE },
    { /* POSA */        "POSA",                 4, docmd_posa,        0x0000a65c, ARG_NONE,  FLAG_NONE },
    { /* PUTM */        "PUTM",                 4, docmd_putm,        0x0000a6e9, ARG_NONE,  FLAG_NONE },
    { /* PWRF */        "PWRF",                 4, docmd_pwrf,        0x0000a0a3, ARG_NONE,  FLAG_NONE },
    { /* RCLEL */       "RCLEL",                5, docmd_rclel,       0x0000a6d7, ARG_NONE,  FLAG_NONE },
    { /* RCLIJ */       "RCLIJ",                5, docmd_rclij,       0x0000a6d9, ARG_NONE,  FLAG_NONE },
    { /* RNRM */        "RNRM",                 4, docmd_rnrm,        0x0000a6ed, ARG_NONE,  FLAG_NONE },
    { /* ROTXY */       "ROTXY",                5, docmd_rotxy,       0x0000a58b, ARG_NONE,  FLAG_NONE },
    { /* RSUM */        "RSUM",                 4, docmd_rsum,        0x0000a6d0, ARG_NONE,  FLAG_NONE },
    { /* SWAP_R */      "R<>R",                 4, docmd_swap_r,      0x0000a6d1, ARG_NONE,  FLAG_NONE },
    { /* SDEV */        "SDEV",                 4, docmd_sdev,        0x0000007d, ARG_NONE,  FLAG_NONE },
    { /* SINH */        "SINH",                 4, docmd_sinh,        0x0000a061, ARG_NONE,  FLAG_NONE },
    { /* SLOPE */       "SLOPE",                5, docmd_slope,       0x0000a0a4, ARG_NONE,  FLAG_NONE },
    { /* SOLVE */       "SOLVE",                5, docmd_solve,       0x00b7f2eb, ARG_RVAR,  FLAG_NONE },
    { /* STOEL */       "STOEL",                5, docmd_stoel,       0x0000a6d6, ARG_NONE,  FLAG_NONE },
    { /* STOIJ */       "STOIJ",                5, docmd_stoij,       0x0000a6d8, ARG_NONE,  FLAG_NONE },
    { /* SUM */         "SUM",                  3, docmd_sum,         0x0000a0a5, ARG_NONE,  FLAG_NONE },
    { /* TANH */        "TANH",                 4, docmd_tanh,        0x0000a063, ARG_NONE,  FLAG_NONE },
    { /* TRANS */       "TRANS",                5, docmd_trans,       0x0000a6c9, ARG_NONE,  FLAG_NONE },
    { /* UVEC */        "UVEC",                 4, docmd_uvec,        0x0000a6cd, ARG_NONE,  FLAG_NONE },
    { /* WMEAN */       "WM\305\301N",          5, docmd_wmean,       0x0000a0ac, ARG_NONE,  FLAG_NONE },
    { /* WRAP */        "WRAP",                 4, docmd_wrap,        0x0000a6e2, ARG_NONE,  FLAG_NONE },
    { /* X_SWAP */      "X<>",                  3, docmd_x_swap,      0x008600ce, ARG_VAR,   FLAG_NONE },
    { /* XOR */         "XOR",                  3, docmd_xor,         0x0000a58a, ARG_NONE,  FLAG_NONE },
    { /* YINT */        "YINT",                 4, docmd_yint,        0x0000a0a6, ARG_NONE,  FLAG_NONE },
    { /* TO_DEC */      "\017DEC",              4, docmd_to_dec,      0x0000005f, ARG_NONE,  FLAG_NONE },
    { /* TO_OCT */      "\017OCT",              4, docmd_to_oct,      0x0000006f, ARG_NONE,  FLAG_NONE },
    { /* LEFT */        "\020",                 1, docmd_left,        0x0000a6dc, ARG_NONE,  FLAG_NONE },
    { /* UP */          "^",                    1, docmd_up,          0x0000a6de, ARG_NONE,  FLAG_NONE },
    { /* DOWN */        "\016",                 1, docmd_down,        0x0000a6df, ARG_NONE,  FLAG_NONE },
    { /* RIGHT */       "\017",                 1, docmd_right,       0x0000a6dd, ARG_NONE,  FLAG_NONE },
    { /* PERCENT_CH */  "%CH",                  3, docmd_percent_ch,  0x0000004d, ARG_NONE,  FLAG_NONE },
    { /* SIMQ */        "SIMQ",                 4, docmd_simq,        0x02000000, ARG_COUNT, FLAG_HIDDEN + FLAG_NO_PRGM },
    { /* MATA */        "MATA",                 4, docmd_mata,        0x02000000, ARG_NONE,  FLAG_HIDDEN + FLAG_NO_PRGM },
    { /* MATB */        "MATB",                 4, docmd_matb,        0x02000000, ARG_NONE,  FLAG_HIDDEN + FLAG_NO_PRGM },
    { /* MATX */        "MATX",                 4, docmd_matx,        0x02000000, ARG_NONE,  FLAG_HIDDEN + FLAG_NO_PRGM },
    { /* GOTOROW */     "GOTO\240\322\357\367", 8, NULL,              0x02000000, ARG_COUNT, FLAG_HIDDEN },
    { /* GOTOCOLUMN */  "GOTO Column",         11, NULL,              0x02000000, ARG_COUNT, FLAG_HIDDEN },
    { /* A_THRU_F */    "A...F",                5, NULL,              0x02000000, ARG_NONE,  FLAG_HIDDEN + FLAG_NO_PRGM },
    { /* CLALLb */      "CLALL",                5, docmd_clall,       0x02000000, ARG_NONE,  FLAG_HIDDEN },
    { /* PGMSLVi */     "P\307\315SLV",         6, docmd_pgmslvi,     0x02000000, ARG_PRGM,  FLAG_HIDDEN },
    { /* PGMINTi */     "P\307\315INT",         6, docmd_pgminti,     0x02000000, ARG_PRGM,  FLAG_HIDDEN },
    { /* VMSTO2 */      "STO",                  3, docmd_vmsto2,      0x02000000, ARG_OTHER, FLAG_HIDDEN },
    { /* VMSOLVE */     "SOLVE",                5, docmd_vmsolve,     0x02000000, ARG_OTHER, FLAG_HIDDEN },
    { /* MAX */         "[MAX]",                5, docmd_max,         0x0000a6eb, ARG_NONE,  FLAG_NONE },
    { /* MIN */         "[MIN]",                5, docmd_min,         0x0000a6ea, ARG_NONE,  FLAG_NONE },
    { /* FIND */        "[FIND]",               6, docmd_find,        0x0000a6ec, ARG_NONE,  FLAG_NONE },
    { /* XROM */        "XROM",                 4, docmd_xrom,        0x01000000, ARG_OTHER, FLAG_HIDDEN },

    /* Here endeth the original Free42 function table.
     * UPDATE: To support "pure" HP-42S behavior, all extensions can be
     * disabled at runtime, using the core_settings.enable_ext_* flags.
     * When an extension is disabled, its commands disappear from the FCN
     * catalog, are not recognized by XEQ, are displayed as their XROM
     * equivalents in programs, and raise a Nonexistent error when trying to
     * execute them from programs.
     * When a shell disables or enables an extension in response to the user
     * changing a setting in the Preferences dialog, it should call redisplay()
     * to make sure the display reflects the new setting.
     */

    /* Underhill's COPAN (Obsolete) */
    { /* OPENF */       "OPENF",                5, docmd_xrom,        0x0000a7c1, ARG_NONE,  FLAG_NONE },
    { /* CLOSF */       "CLOSF",                5, docmd_xrom,        0x0000a7c2, ARG_NONE,  FLAG_NONE },
    { /* READP */       "READP",                5, docmd_xrom,        0x0000a7c3, ARG_NONE,  FLAG_NONE },
    { /* WRITP */       "WRITP",                5, docmd_xrom,        0x0000a7c4, ARG_NONE,  FLAG_NONE },
    { /* GETXY */       "GETXY",                5, docmd_xrom,        0x0000a7c5, ARG_NONE,  FLAG_NONE },
    { /* PUTXY */       "PUTXY",                5, docmd_xrom,        0x0000a7c6, ARG_NONE,  FLAG_NONE },
    { /* CLRP */        "CLRP",                 4, docmd_xrom,        0x0000a7c7, ARG_NONE,  FLAG_NONE },
    { /* CLRD */        "CLRD",                 4, docmd_xrom,        0x0000a7c8, ARG_NONE,  FLAG_NONE },
    { /* APPD */        "APPD",                 4, docmd_xrom,        0x0000a7c9, ARG_NONE,  FLAG_NONE },
    { /* GETN */        "GETN",                 4, docmd_xrom,        0x0000a7ca, ARG_NONE,  FLAG_NONE },
    { /* PUTN */        "PUTN",                 4, docmd_xrom,        0x0000a7cb, ARG_NONE,  FLAG_NONE },
    { /* GETZ */        "GETZ",                 4, docmd_xrom,        0x0000a7cc, ARG_NONE,  FLAG_NONE },
    { /* PUTZ */        "PUTZ",                 4, docmd_xrom,        0x0000a7cd, ARG_NONE,  FLAG_NONE },
    { /* DELP */        "DELP",                 4, docmd_xrom,        0x0000a7ce, ARG_NONE,  FLAG_NONE },

    /* Byron Foster's DROP for Bigstack (Obsolete) */
    { /* DROP */        "DROP",                 4, docmd_xrom,        0x0000a271, ARG_NONE,  FLAG_NONE },

    /* Accelerometer, GPS, and compass support */
    { /* ACCEL */       "ACCEL",                5, docmd_accel,       0x0000a7cf, ARG_NONE,  FLAG_NONE },
    { /* LOCAT */       "LOCAT",                5, docmd_locat,       0x0000a7d0, ARG_NONE,  FLAG_NONE },
    { /* HEADING */     "H\305\301D\311NG",     7, docmd_heading,     0x0000a7d1, ARG_NONE,  FLAG_NONE },

    /* Time Module & CX Time support*/
    { /* ADATE */       "ADATE",                5, docmd_adate,       0x0000a681, ARG_NONE,  FLAG_NONE },
    { /* ALMCAT */      "AL\315CAT",            6, docmd_xrom,        0x0000a682, ARG_NONE,  FLAG_HIDDEN },
    { /* ALMNOW */      "AL\315N\317W",         6, docmd_xrom,        0x0000a683, ARG_NONE,  FLAG_HIDDEN },
    { /* ATIME */       "ATIME",                5, docmd_atime,       0x0000a684, ARG_NONE,  FLAG_NONE },
    { /* ATIME24 */     "AT\311\315\30524",     7, docmd_atime24,     0x0000a685, ARG_NONE,  FLAG_NONE },
    { /* CLK12 */       "CL\31312",             5, docmd_clk12,       0x0000a686, ARG_NONE,  FLAG_NONE },
    { /* CLK24 */       "CL\31324",             5, docmd_clk24,       0x0000a687, ARG_NONE,  FLAG_NONE },
    { /* CLKT */        "CLKT",                 4, docmd_xrom,        0x0000a688, ARG_NONE,  FLAG_HIDDEN },
    { /* CLKTD */       "CL\313TD",             5, docmd_xrom,        0x0000a689, ARG_NONE,  FLAG_HIDDEN },
    { /* CLOCK */       "CL\317\303K",          5, docmd_xrom,        0x0000a68a, ARG_NONE,  FLAG_HIDDEN },
    { /* CORRECT */     "CORR\305\303\324",     7, docmd_xrom,        0x0000a68b, ARG_NONE,  FLAG_HIDDEN },
    { /* DATE */        "DATE",                 4, docmd_date,        0x0000a68c, ARG_NONE,  FLAG_NONE },
    { /* DATE_PLUS */   "DATE+",                5, docmd_date_plus,   0x0000a68d, ARG_NONE,  FLAG_NONE },
    { /* DDAYS */       "DDAYS",                5, docmd_ddays,       0x0000a68e, ARG_NONE,  FLAG_NONE },
    { /* DMY */         "DMY",                  3, docmd_dmy,         0x0000a68f, ARG_NONE,  FLAG_NONE },
    { /* DOW */         "DOW",                  3, docmd_dow,         0x0000a690, ARG_NONE,  FLAG_NONE },
    { /* MDY */         "MDY",                  3, docmd_mdy,         0x0000a691, ARG_NONE,  FLAG_NONE },
    { /* RCLAF */       "RCLAF",                5, docmd_xrom,        0x0000a692, ARG_NONE,  FLAG_HIDDEN },
    { /* RCLSW */       "RC\314SW",             5, docmd_xrom,        0x0000a693, ARG_NONE,  FLAG_HIDDEN },
    { /* RUNSW */       "R\325NSW",             5, docmd_xrom,        0x0000a694, ARG_NONE,  FLAG_HIDDEN },
    { /* SETAF */       "SETAF",                5, docmd_xrom,        0x0000a695, ARG_NONE,  FLAG_HIDDEN },
    { /* SETDATE */     "S\305\324DATE",        7, docmd_xrom,        0x0000a696, ARG_NONE,  FLAG_HIDDEN },
    { /* SETIME */      "S\305TIME",            6, docmd_xrom,        0x0000a697, ARG_NONE,  FLAG_HIDDEN },
    { /* SETSW */       "SE\324SW",             5, docmd_xrom,        0x0000a698, ARG_NONE,  FLAG_HIDDEN },
    { /* STOPSW */      "ST\317\320SW",         6, docmd_xrom,        0x0000a699, ARG_NONE,  FLAG_HIDDEN },
    { /* SW */          "SW",                   2, docmd_xrom,        0x0000a69a, ARG_NONE,  FLAG_HIDDEN },
    { /* T_PLUS_X */    "T+X",                  3, docmd_xrom,        0x0000a69b, ARG_NONE,  FLAG_HIDDEN },
    { /* TIME */        "TIME",                 4, docmd_time,        0x0000a69c, ARG_NONE,  FLAG_NONE },
    { /* XYZALM */      "XYZALM",               6, docmd_xrom,        0x0000a69d, ARG_NONE,  FLAG_HIDDEN },
    { /* CLALMA */      "CLAL\315A",            6, docmd_xrom,        0x0000a69f, ARG_NONE,  FLAG_HIDDEN },
    { /* CLALMX */      "CLAL\315X",            6, docmd_xrom,        0x0000a6a0, ARG_NONE,  FLAG_HIDDEN },
    { /* CLRALMS */     "CLRALMS",              7, docmd_xrom,        0x0000a6a1, ARG_NONE,  FLAG_HIDDEN },
    { /* RCLALM */      "RCLALM",               6, docmd_xrom,        0x0000a6a2, ARG_NONE,  FLAG_HIDDEN },
    { /* SWPT */        "SWPT",                 4, docmd_xrom,        0x0000a6a3, ARG_NONE,  FLAG_HIDDEN },

    /* Intel Decimal Floating-Point Math Library: self-test */
    { /* FPTEST */     "FPT\305ST",             6, docmd_fptest,      0x0000a7d2, ARG_NONE,  FLAG_NONE },

    /* Programming */
    { /* LSTO */       "LSTO",                  4, docmd_lsto,        0x00c7f2ed, ARG_NAMED, FLAG_NONE },
    { /* SST_UP */     "SST^",                  4, NULL,              0x02000000, ARG_NONE,  FLAG_NONE },
    { /* SST_RT */     "SST\017",               4, NULL,              0x02000000, ARG_NONE,  FLAG_NONE },
    { /* WSIZE */      "WSIZE",                 5, docmd_wsize,       0x0000a7d3, ARG_NONE,  FLAG_NONE },
    { /* WSIZE_T */    "WS\311Z\305?",          6, docmd_wsize_t,     0x0000a7d4, ARG_NONE,  FLAG_NONE },
    { /* YMD */        "YMD",                   3, docmd_ymd,         0x0000a7d5, ARG_NONE,  FLAG_NONE },
    { /* BSIGNED */    "BS\311GN\305\304",      7, docmd_bsigned,     0x0000a7d6, ARG_NONE,  FLAG_NONE },
    { /* BWRAP */      "BWR\301P",              5, docmd_bwrap,       0x0000a7d7, ARG_NONE,  FLAG_NONE },
    { /* BRESET */     "BR\305S\305T",          6, docmd_breset,      0x0000a7d8, ARG_NONE,  FLAG_NONE }
};

/*
===============================================================================
HP-42S program storage format
Suffixes of nn work as follows: 0-65 are 00-101; 66-7F are A-J, T, Z, Y, X, L,
M, N, O, P, Q, \append, a, b, c, d, e; 80-FF are IND versions.
Fn starts an n-character alpha string; the 42S uses special initial bytes (in
the 80-FF range) to encode those of its extensions to the 41C instruction set
that take a parameter; parameterless extensions are encoded using XROM
instructions (2-byte instructions with 1st byte of A0-A7).
Dunno yet how the offsets work (LBL "", END, GTO nn, XEQ nn)

TODO: what about 1F (W ""), AF & B0 (SPARE)?

Quick instruction length finder: 00-8F are 1 byte, except 1D-1F, which are
followed by a string (Fn plus n bytes of text, for a total of n+2 bytes).
90-BF are 2 bytes (but what about AF & B0 (SPARE)?)
C0-CD: if byte 3 is Fn, then it's a global label with a total of n+3 bytes (the 
string has an extra byte prepended which the 41C uses for key assignment); if
byte 3 is not Fn (TODO: which values are allowed & what do they mean?) it is an
END, 3 bytes.
D0-EF: 3 bytes.
Fn: string, n+1 bytes. This includes 42S extensions with parameters (42S
extensions without parameters are encoded using XROM instructions (A[0-7] nn),
always 2 bytes).

CLX          77
ENTER        83
SWAP         71
RDN          75
CHS          54
DIV          43
MUL          42
SUB          41
ADD          40
LASTX        76
SILENT_OFF   n/a
SILENT_ON    n/a
SIN          59
COS          5A
TAN          5B
ASIN         5C
ACOS         5D
ATAN         5E
LOG          56
10_POW_X     57
LN           50
E_POW_X      55
SQRT         52
SQUARE       51
INV          60
Y_POW_X      53
PERCENT      4C
PI           72
COMPLEX      A0 72
STO          91 nn (STO 00-15: 3n; STO "": Fn 81; STO IND "": Fn 89)
STO_DIV      95 nn (STO/ "": Fn 85; STO/ IND "": Fn 8D)
STO_MUL      94 nn (STO* "": Fn 84; STO* IND "": Fn 8C)
STO_SUB      93 nn (STO- "": Fn 83; STO- IND "": Fn 8B)
STO_ADD      92 nn (STO+ "": Fn 82; STO+ IND "": Fn 8A)
RCL          90 nn (RCL 00-15: 2n; RCL "": Fn 91; RCL IND "": Fn 99)
RCL_DIV      F2 D4 nn (RCL/ "": Fn 95; RCL/ IND "": Fn 9D)
RCL_MUL      F2 D3 nn (RCL* "": Fn 94; RCL* IND "": Fn 9C)
RCL_SUB      F2 D2 nn (RCL- "": Fn 93; RCL- IND "": Fn 9B)
RCL_ADD      F2 D1 nn (RCL+ "": Fn 92; RCL+ IND "": Fn 9A)
FIX          9C nn (FIX 10: F1 D5; FIX 11: F1 E5) (FIX IND "": Fn DC)
SCI          9D nn (SCI 10: F1 D6; SCI 11: F1 E6) (SCI IND "": Fn DD)
ENG          9E nn (ENG 10: F1 D7; ENG 11: F1 E7) (ENG IND "": Fn DE)
ALL          A2 5D
NULL         00
ASTO         9A nn (ASTO "": Fn B2; ASTO IND "": Fn BA)
ARCL         9B nn (ARCL "": Fn B3; ARCL IND "": Fn BB)
CLA          87
DEG          80
RAD          81
GRAD         82
RECT         A2 5A
POLAR        A2 59
SIZE         F3 F7 nn nn
QUIET        A2 69 (ill)
CPXRES       A2 6A
REALRES      A2 6B
KEYASN       A2 63
LCLBL        A2 64
RDXDOT       A2 5B
RDXCOMMA     A2 5C
CLSIGMA      70
CLP          Fn F0
CLV          F2 D8 nn (IND only) (CLV "": Fn B0; CLV IND "": Fn B8)
CLST         73
CLRG         8A
DEL          F3 F6 nn nn (ill)
CLKEYS       A2 62
CLLCD        A7 63
CLMENU       A2 6D
CLALLa       n/a
TO_DEG       6B
TO_RAD       6A
TO_HR        6D
TO_HMS       6C
TO_REC       4E
TO_POL       4F
IP           68
FP           69
RND          6E
ABS          61
SIGN         7A
MOD          4B
SF           A8 nn (SF IND "": Fn A8)
CF           A9 nn (CF IND "": Fn A9)
FS_T         AC nn (FS? IND "": Fn AC)
FC_T         AD nn (FC? IND "": Fn AD)
FSC_T        AA nn (FS?C IND "": Fn AA)
FCC_T        AB nn (FC?C IND "": Fn AB)
COMB         A0 6F
PERM         A0 70
FACT         62
GAMMA        A0 74
RAN          A0 71
SEED         A0 73
LBL          CF nn (LBL 00-14: 01-0F; LBL "": Cm mm Fn) (note that CE and CF
              are X<> nn and LBL nn, so that limits the possible values of mmm;
              the label name has an extra byte prepended which the 41C uses for
              key assignment)
RTN          85
INPUT        F2 D0 nn (INPUT IND: F2 EE nn; INPUT "": Fn C5;
                        INPUT IND "": Fn CD)
VIEW         98 nn (VIEW "": Fn 80; VIEW IND nn: Fn 88)
AVIEW        7E
XEQ          Em mm nn (XEQ IND nn: AE nn (nn bit 7 set);
                        XEQ "": 1E Fn; XEQ IND "": Fn AF)
PROMPT       8E
PSE          89
ISG          96 nn (ISG "": Fn 96; ISG IND nn: Fn 9E)
DSE          97 nn (DSE "": Fn 97; DSE IND nn: Fn 9F)
AIP          A6 31
XTOA         A6 6F
AGRAPH       A7 64
PIXEL        A7 65
BEEP         86
TONE         9F nn (TONE IND "": Fn DF)
MVAR         Fn 90 (MVAR IND "": Fn 98 (ill (?)))
VARMENU      F2 F8 nn (IND only) (VARMENU "": Fn C1; VARMENU IND "": Fn C9)
GETKEY       A2 6E
MENU         A2 5E
KEYG         n/a
KEYX         n/a
X_EQ_0       67
X_NE_0       63
X_LT_0       66
X_GT_0       64
X_LE_0       7B
X_GE_0       A2 5F
X_EQ_Y       78
X_NE_Y       79
X_LT_Y       44
X_GT_Y       45
X_LE_Y       46
X_GE_Y       A2 60
PRSIGMA      A7 52
PRP          A7 4D (ill)
PRV          F2 D9 nn (IND only) (PRV "": Fn B1; PRV IND "": Fn B9)
PRSTK        A7 53
PRA          A7 48
PRX          A7 54
PRUSR        A7 61
LIST         A7 47 (ill)
ADV          8F
PRLCD        A7 62
DELAY        A7 60
PON          A7 5E
POFF         A7 5F
MAN          A7 5B
NORM         A7 5C
TRACE        A7 5D
SIGMAADD     47
SIGMASUB     48
GTO          Dm mm nn (GTO 00-14: B1-BF; GTO IND nn: AE nn (nn bit 7 clear);
                        GTO "": 1D Fn; GTO IND "": Fn AE)
END          Cm mm ?? (mmm < E00; ?? is not Fn (cuz that's LBL ""), but what?)
NUMBER       0-9: 10-19; .: 1A; E: 1B; -: 1C; conseq num lines sep by NULL (00)
STRING       Fn, except when n > 0 and the next byte has bit 7 set (or at least
             is one of the special values that define HP-42S extensions with
             parameters) (but for the purpose of instruction length finding, it
             makes no difference: that is always n+1)
RUN          n/a
SST          n/a
GTODOT       F3 F2 nn nn (GTO . "": Fn F4) (ill)
GTODOTDOT    Fn F3 (TODO: shouldn't that be F1 F3?) (ill)
STOP         84
NEWMAT       A6 DA
RUP          74
REAL_T       A2 65
CPX_T        A2 67
STR_T        A2 68
MAT_T        A2 66
DIM_T        A6 E7
ASSIGNa      n/a
ASSIGNb      n/a
ASGN01       Fn C0 name 00 
ASGN02       Fn C0 name 01
ASGN03       Fn C0 name 02
ASGN04       Fn C0 name 03
ASGN05       Fn C0 name 04
ASGN06       Fn C0 name 05
ASGN07       Fn C0 name 06
ASGN08       Fn C0 name 07
ASGN09       Fn C0 name 08
ASGN10       Fn C0 name 09
ASGN11       Fn C0 name 0a
ASGN12       Fn C0 name 0b
ASGN13       Fn C0 name 0c
ASGN14       Fn C0 name 0d
ASGN15       Fn C0 name 0e
ASGN16       Fn C0 name 0f
ASGN17       Fn C0 name 10
ASGN18       Fn C0 name 11
ON           A2 70
OFF          8D
KEY1G        F3 E3 01 nn ("": Fn C3 01; IND "": Fn CB 01)
KEY2G        F3 E3 02 nn ("": Fn C3 02; IND "": Fn CB 02)
KEY3G        F3 E3 03 nn ("": Fn C3 03; IND "": Fn CB 03)
KEY4G        F3 E3 04 nn ("": Fn C3 04; IND "": Fn CB 04)
KEY5G        F3 E3 05 nn ("": Fn C3 05; IND "": Fn CB 05)
KEY6G        F3 E3 06 nn ("": Fn C3 06; IND "": Fn CB 06)
KEY7G        F3 E3 07 nn ("": Fn C3 07; IND "": Fn CB 07)
KEY8G        F3 E3 08 nn ("": Fn C3 08; IND "": Fn CB 08)
KEY9G        F3 E3 09 nn ("": Fn C3 09; IND "": Fn CB 09)
KEY1X        F3 E2 01 nn ("": Fn C2 01; IND "": Fn CA 01)
KEY2X        F3 E2 02 nn ("": Fn C2 02; IND "": Fn CA 02)
KEY3X        F3 E2 03 nn ("": Fn C2 03; IND "": Fn CA 03)
KEY4X        F3 E2 04 nn ("": Fn C2 04; IND "": Fn CA 04)
KEY5X        F3 E2 05 nn ("": Fn C2 05; IND "": Fn CA 05)
KEY6X        F3 E2 06 nn ("": Fn C2 06; IND "": Fn CA 06)
KEY7X        F3 E2 07 nn ("": Fn C2 07; IND "": Fn CA 07)
KEY8X        F3 E2 08 nn ("": Fn C2 08; IND "": Fn CA 08)
KEY9X        F3 E2 09 nn ("": Fn C2 09; IND "": Fn CA 09)
VMEXEC       n/a
VMSTO        n/a
SIGMAREG     99 nn (SigmaREG IND "": Fn DB)
SIGMAREG_T   A6 78
CLD          7F
ACOSH        A0 66
ALENG        A6 41
ALLSIGMA     A0 AE
AND          A5 88
AOFF         8B
AON          8C
AROT         A6 46
ASHF         88
ASINH        A0 64
ATANH        A0 65
ATOX         A6 47
BASEADD      A0 E6
BASESUB      A0 E7
BASEMUL      A0 E8
BASEDIV      A0 E9
BASECHS      A0 EA
BEST         A0 9F
BINM         A0 E5
BIT_T        A5 8C
BST          n/a
CORR         A0 A7
COSH         A0 62
CROSS        A6 CA
CUSTOM       A2 6F
DECM         A0 E3
DELR         A0 AB
DET          A6 CC
DIM          F2 EC nn (IND only) (DIM "": Fn C4; DIM IND "": Fn CC)
DOT          A6 CB
EDIT         A6 E1
EDITN        F2 EF nn (IND only) (EDITN "": Fn C6; EDITN IND "": Fn CE)
EXITALL      A2 6C
EXPF         A0 A0
E_POW_X_1    58
FCSTX        A0 A8
FCSTY        A0 A9
FNRM         A6 CF
GETM         A6 E8
GROW         A6 E3
HEXM         A0 E2
HMSADD       49
HMSSUB       4A
I_ADD        A6 D2
I_SUB        A6 D3
INDEX        F2 DA nn (IND only) (INDEX "": Fn 87; INDEX IND "": Fn 8F)
INSR         A0 AA
INTEG        F2 EA nn (IND only) (INTEG "": Fn B6; INTEG IND "": Fn BE)
INVRT        A6 CE
J_ADD        A6 D4
J_SUB        A6 D5
LINF         A0 A1
LINSIGMA     A0 AD
LN_1_X       65
LOGF         A0 A2
MEAN         7C
NOT          A5 87
OCTM         A0 E4
OLD          A6 DB
OR           A5 89
PGMSLV       F2 E9 nn (IND only) (PGMSLV "": Fn B5; PGMSLV IND "": Fn BD)
PGMINT       F2 E8 nn (IND only) (PGMINT "": Fn B4; PGMINT IND "": Fn BC)
POSA         A6 5C
PUTM         A6 E9
PWRF         A0 A3
RCLEL        A6 D7
RCLIJ        A6 D9
RNRM         A6 ED
ROTXY        A5 8B
RSUM         A6 D0
SWAP_R       A6 D1
SDEV         7D
SINH         A0 61
SLOPE        A0 A4
SOLVE        F2 EB nn (IND only) (SOLVE "": Fn B7; SOLVE IND "": Fn BF)
STOEL        A6 D6
STOIJ        A6 D8
SUM          A0 A5
TANH         A0 63
TRANS        A6 C9
UVEC         A6 CD
WMEAN        A0 AC
WRAP         A6 E2
X_SWAP       CE nn (X<> "": Fn 86; X<> IND "": Fn 8E)
XOR          A5 8A
YINT         A0 A6
TO_DEC       5F
TO_OCT       6F
LEFT         A6 DC
UP           A6 DE
DOWN         A6 DF
RIGHT        A6 DD
PERCENT_CH   4D
SIMQ         n/a
MATA         A6 E4 (ill)
MATB         A6 E5 (ill)
MATX         A6 E6 (ill)
GOTOROW      A6 E0 (ill)
GOTOCOLUMN   n/a
A_THRU_F     n/a
CLALLb       A2 61 (ill)
PGMSLVi      n/a
PGMINTi      n/a
VMSTO2       n/a
VMSOLVE      n/a
MAX          A6 EB
MIN          A6 EA
FIND         A6 EC

To be added:
W            1F Fn (TODO: what's this?)
SPARE1       AF (TODO: what's this?)
SPARE2       B0 (TODO: what's this?)
XFCN         Fn F1 (TODO: what's this?) (apparently, always says "Nonexistent")
XROM         A[0-7] nn (bits 2-0 of byte 1 plus bits 7-6 of byte 2 are the ROM
             ID; bits 5-0 of byte 2 are the instruction number. The instruction
             is displayed as XROM nn,mm with nn and mm in 2 decimal digits.
             When executed, always says "Nonexistent".
             Note: when decoding functions, the check for XROM should come
             *last*, because all the parameterless HP-42S extensions are
             encoded in XROM space.
===============================================================================
*/

const command_spec *cmdlist(int index) {
    return cmd_array + index;
}
