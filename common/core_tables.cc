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

#include <stdlib.h>

#include "core_tables.h"
#include "core_commands1.h"
#include "core_commands2.h"
#include "core_commands3.h"
#include "core_commands4.h"
#include "core_commands5.h"
#include "core_commands6.h"
#include "core_commands7.h"


/* rttypes special cases */

/* ALLT means all types; not just all the types that exist now, but also all
 * types that might be added in the future.
 * For things like ENTER, CLX, PRX, etc.
 */
#define ALLT 0xff

/* Checking performed by the function, maybe because it's complicated,
 * maybe because HP-42S compatibilty requires performing other checks before
 * the parameter type checks.
 */
#define FUNC 0xff

/* Dummy value, to be used only when argcount = 0. Only used for actual
 * existing functions; for unimplemented functions, leave it at 0x00.
 */
#define NA_T 0x00

/* Dummy value, to be used for unimplemented functions.
 */
#define UNIM 0x00

const command_spec cmd_array[] =
{
    { /* CLX */        docmd_clx,         "CLX",                 0x00, 0x00, 0x00, 0x77,  3, ARG_NONE,   1, ALLT },
    { /* ENTER */      docmd_enter,       "ENT\305R",            0x00, 0x00, 0x00, 0x83,  5, ARG_NONE,   1, ALLT },
    { /* SWAP */       docmd_swap,        "X<>Y",                0x00, 0x00, 0x00, 0x71,  4, ARG_NONE,   2, ALLT },
    { /* RDN */        docmd_rdn,         "R\016",               0x00, 0x00, 0x00, 0x75,  2, ARG_NONE,   0, NA_T },
    { /* CHS */        docmd_chs,         "+/-",                 0x00, 0x00, 0x00, 0x54,  3, ARG_NONE,   1, 0x0f },
    { /* DIV */        docmd_div,         "\000",                0x00, 0x00, 0x00, 0x43,  1, ARG_NONE,   2, 0x0f },
    { /* MUL */        docmd_mul,         "\001",                0x00, 0x00, 0x00, 0x42,  1, ARG_NONE,   2, 0x0f },
    { /* SUB */        docmd_sub,         "-",                   0x00, 0x00, 0x00, 0x41,  1, ARG_NONE,   2, 0x0f },
    { /* ADD */        docmd_add,         "+",                   0x00, 0x00, 0x00, 0x40,  1, ARG_NONE,   2, 0x0f },
    { /* LASTX */      docmd_lastx,       "LASTX",               0x00, 0x00, 0x00, 0x76,  5, ARG_NONE,   0, NA_T },
    { /* SILENT_OFF */ NULL,              "",                    0x34, 0x00, 0x00, 0x00,  0, ARG_NONE,   0, NA_T },
    { /* SILENT_ON */  NULL,              "",                    0x34, 0x00, 0x00, 0x00,  0, ARG_NONE,   0, NA_T },
    { /* SIN */        docmd_sin,         "SIN",                 0x00, 0x00, 0x00, 0x59,  3, ARG_NONE,   1, 0x0f },
    { /* COS */        docmd_cos,         "COS",                 0x00, 0x00, 0x00, 0x5a,  3, ARG_NONE,   1, 0x0f },
    { /* TAN */        docmd_tan,         "TAN",                 0x00, 0x00, 0x00, 0x5b,  3, ARG_NONE,   1, 0x0f },
    { /* ASIN */       docmd_asin,        "ASIN",                0x00, 0x00, 0x00, 0x5c,  4, ARG_NONE,   1, 0x0f },
    { /* ACOS */       docmd_acos,        "ACOS",                0x00, 0x00, 0x00, 0x5d,  4, ARG_NONE,   1, 0x0f },
    { /* ATAN */       docmd_atan,        "ATAN",                0x00, 0x00, 0x00, 0x5e,  4, ARG_NONE,   1, 0x0f },
    { /* LOG */        docmd_log,         "LOG",                 0x00, 0x00, 0x00, 0x56,  3, ARG_NONE,   1, 0x0f },
    { /* 10_POW_X */   docmd_10_pow_x,    "10^X",                0x00, 0x00, 0x00, 0x57,  4, ARG_NONE,   1, 0x0f },
    { /* LN */         docmd_ln,          "LN",                  0x00, 0x00, 0x00, 0x50,  2, ARG_NONE,   1, 0x0f },
    { /* E_POW_X */    docmd_e_pow_x,     "E^X",                 0x00, 0x00, 0x00, 0x55,  3, ARG_NONE,   1, 0x0f },
    { /* SQRT */       docmd_sqrt,        "SQRT",                0x00, 0x00, 0x00, 0x52,  4, ARG_NONE,   1, 0x0f },
    { /* SQUARE */     docmd_square,      "X^2",                 0x00, 0x00, 0x00, 0x51,  3, ARG_NONE,   1, 0x0f },
    { /* INV */        docmd_inv,         "1/X",                 0x00, 0x00, 0x00, 0x60,  3, ARG_NONE,   1, 0x0f },
    { /* Y_POW_X */    docmd_y_pow_x,     "Y^X",                 0x00, 0x00, 0x00, 0x53,  3, ARG_NONE,   2, FUNC },
    { /* PERCENT */    docmd_percent,     "%",                   0x00, 0x00, 0x00, 0x4c,  1, ARG_NONE,   2, 0x01 },
    { /* PI */         docmd_pi,          "PI",                  0x00, 0x00, 0x00, 0x72,  2, ARG_NONE,   0, NA_T },
    { /* COMPLEX */    docmd_complex,     "C\317\315PL\305X",    0x00, 0x00, 0xa0, 0x72,  7, ARG_NONE,  -1, 0x00 },
    { /* STO */        docmd_sto,         "STO",                 0x20, 0x81, 0x00, 0x91,  3, ARG_VAR,    1, ALLT },
    { /* STO_DIV */    docmd_sto_div,     "STO\000",             0x00, 0x85, 0x00, 0x95,  4, ARG_VAR,    1, 0x0f },
    { /* STO_MUL */    docmd_sto_mul,     "STO\001",             0x00, 0x84, 0x00, 0x94,  4, ARG_VAR,    1, 0x0f },
    { /* STO_SUB */    docmd_sto_sub,     "STO-",                0x00, 0x83, 0x00, 0x93,  4, ARG_VAR,    1, 0x0f },
    { /* STO_ADD */    docmd_sto_add,     "STO+",                0x00, 0x82, 0x00, 0x92,  4, ARG_VAR,    1, 0x0f },
    { /* RCL */        docmd_rcl,         "RCL",                 0x20, 0x91, 0x00, 0x90,  3, ARG_VAR,    0, NA_T },
    { /* RCL_DIV */    docmd_rcl_div,     "RCL\000",             0x00, 0x95, 0xf2, 0xd4,  4, ARG_VAR,    1, 0x0f },
    { /* RCL_MUL */    docmd_rcl_mul,     "RCL\001",             0x00, 0x94, 0xf2, 0xd3,  4, ARG_VAR,    1, 0x0f },
    { /* RCL_SUB */    docmd_rcl_sub,     "RCL-",                0x00, 0x93, 0xf2, 0xd2,  4, ARG_VAR,    1, 0x0f },
    { /* RCL_ADD */    docmd_rcl_add,     "RCL+",                0x00, 0x92, 0xf2, 0xd1,  4, ARG_VAR,    1, 0x0f },
    { /* FIX */        docmd_fix,         "FIX",                 0x20, 0xd4, 0x00, 0x9c,  3, ARG_NUM11,  0, NA_T },
    { /* SCI */        docmd_sci,         "SCI",                 0x20, 0xd5, 0x00, 0x9d,  3, ARG_NUM11,  0, NA_T },
    { /* ENG */        docmd_eng,         "ENG",                 0x20, 0xd6, 0x00, 0x9e,  3, ARG_NUM11,  0, NA_T },
    { /* ALL */        docmd_all,         "ALL",                 0x00, 0x00, 0xa2, 0x5d,  3, ARG_NONE,   0, NA_T },
    { /* NULL */       docmd_null,        "\316\325\314\314",    0x44, 0x00, 0x00, 0x00,  4, ARG_NONE,   0, NA_T },
    { /* ASTO */       docmd_asto,        "ASTO",                0x00, 0xb2, 0x00, 0x9a,  4, ARG_VAR,    0, NA_T },
    { /* ARCL */       docmd_arcl,        "ARCL",                0x00, 0xb3, 0x00, 0x9b,  4, ARG_VAR,    0, NA_T },
    { /* CLA */        docmd_cla,         "CLA",                 0x00, 0x00, 0x00, 0x87,  3, ARG_NONE,   0, NA_T },
    { /* DEG */        docmd_deg,         "DEG",                 0x00, 0x00, 0x00, 0x80,  3, ARG_NONE,   0, NA_T },
    { /* RAD */        docmd_rad,         "RAD",                 0x00, 0x00, 0x00, 0x81,  3, ARG_NONE,   0, NA_T },
    { /* GRAD */       docmd_grad,        "GRAD",                0x00, 0x00, 0x00, 0x82,  4, ARG_NONE,   0, NA_T },
    { /* RECT */       docmd_rect,        "RECT",                0x00, 0x00, 0xa2, 0x5a,  4, ARG_NONE,   0, NA_T },
    { /* POLAR */      docmd_polar,       "POLAR",               0x00, 0x00, 0xa2, 0x59,  5, ARG_NONE,   0, NA_T },
    { /* SIZE */       docmd_size,        "SIZE",                0x20, 0x00, 0x00, 0x00,  4, ARG_COUNT,  0, NA_T },
    { /* QUIET */      docmd_quiet,       "QUIET",               0x42, 0x00, 0x00, 0x00,  5, ARG_NONE,   0, NA_T },
    { /* CPXRES */     docmd_cpxres,      "C\320\330RES",        0x00, 0x00, 0xa2, 0x6a,  6, ARG_NONE,   0, NA_T },
    { /* REALRES */    docmd_realres,     "R\305\301\314RES",    0x00, 0x00, 0xa2, 0x6b,  7, ARG_NONE,   0, NA_T },
    { /* KEYASN */     docmd_keyasn,      "KEY\301\323\316",     0x00, 0x00, 0xa2, 0x63,  6, ARG_NONE,   0, NA_T },
    { /* LCLBL */      docmd_lclbl,       "LCLBL",               0x00, 0x00, 0xa2, 0x64,  5, ARG_NONE,   0, NA_T },
    { /* RDXDOT */     docmd_rdxdot,      "RDX.",                0x00, 0x00, 0xa2, 0x5b,  4, ARG_NONE,   0, NA_T },
    { /* RDXCOMMA */   docmd_rdxcomma,    "RDX,",                0x00, 0x00, 0xa2, 0x5c,  4, ARG_NONE,   0, NA_T },
    { /* CLSIGMA */    docmd_clsigma,     "CL\005",              0x00, 0x00, 0x00, 0x70,  3, ARG_NONE,   0, NA_T },
    { /* CLP */        docmd_clp,         "CLP",                 0x00, 0xf0, 0x00, 0x00,  3, ARG_PRGM,   0, NA_T },
    { /* CLV */        docmd_clv,         "CLV",                 0x00, 0xb0, 0xf2, 0xd8,  3, ARG_NAMED,  0, NA_T },
    { /* CLST */       docmd_clst,        "CLST",                0x00, 0x00, 0x00, 0x73,  4, ARG_NONE,   0, NA_T },
    { /* CLRG */       docmd_clrg,        "CLRG",                0x00, 0x00, 0x00, 0x8a,  4, ARG_NONE,   0, NA_T },
    { /* DEL */        docmd_del,         "DEL",                 0x43, 0x00, 0x00, 0x00,  3, ARG_COUNT,  0, NA_T },
    { /* CLKEYS */     docmd_clkeys,      "CLK\305Y\323",        0x00, 0x00, 0xa2, 0x62,  6, ARG_NONE,   0, NA_T },
    { /* CLLCD */      docmd_cllcd,       "CLLCD",               0x00, 0x00, 0xa7, 0x63,  5, ARG_NONE,   0, NA_T },
    { /* CLMENU */     docmd_clmenu,      "CLM\305N\325",        0x00, 0x00, 0xa2, 0x6d,  6, ARG_NONE,   0, NA_T },
    { /* CLALLa */     NULL,              "CLALL",               0x42, 0x00, 0x00, 0x00,  5, ARG_NONE,   0, NA_T },
    { /* TO_DEG */     docmd_to_deg,      "\017DEG",             0x00, 0x00, 0x00, 0x6b,  4, ARG_NONE,   1, 0x05 },
    { /* TO_RAD */     docmd_to_rad,      "\017RAD",             0x00, 0x00, 0x00, 0x6a,  4, ARG_NONE,   1, 0x05 },
    { /* TO_HR */      docmd_to_hr,       "\017HR",              0x00, 0x00, 0x00, 0x6d,  3, ARG_NONE,   1, 0x05 },
    { /* TO_HMS */     docmd_to_hms,      "\017HMS",             0x00, 0x00, 0x00, 0x6c,  4, ARG_NONE,   1, 0x05 },
    { /* TO_REC */     docmd_to_rec,      "\017REC",             0x00, 0x00, 0x00, 0x4e,  4, ARG_NONE,  -1, 0x00 },
    { /* TO_POL */     docmd_to_pol,      "\017POL",             0x00, 0x00, 0x00, 0x4f,  4, ARG_NONE,  -1, 0x00 },
    { /* IP */         docmd_ip,          "IP",                  0x00, 0x00, 0x00, 0x68,  2, ARG_NONE,   1, 0x05 },
    { /* FP */         docmd_fp,          "FP",                  0x00, 0x00, 0x00, 0x69,  2, ARG_NONE,   1, 0x05 },
    { /* RND */        docmd_rnd,         "RND",                 0x00, 0x00, 0x00, 0x6e,  3, ARG_NONE,   1, 0x0f },
    { /* ABS */        docmd_abs,         "ABS",                 0x00, 0x00, 0x00, 0x61,  3, ARG_NONE,   1, 0x07 },
    { /* SIGN */       docmd_sign,        "SIGN",                0x00, 0x00, 0x00, 0x7a,  4, ARG_NONE,   1, 0x1f },
    { /* MOD */        docmd_mod,         "MOD",                 0x00, 0x00, 0x00, 0x4b,  3, ARG_NONE,   2, 0x01 },
    { /* SF */         docmd_sf,          "SF",                  0x00, 0xa0, 0x00, 0xa8,  2, ARG_NUM99,  0, NA_T },
    { /* CF */         docmd_cf,          "CF",                  0x00, 0xa1, 0x00, 0xa9,  2, ARG_NUM99,  0, NA_T },
    { /* FS_T */       docmd_fs_t,        "FS?",                 0x00, 0xa4, 0x00, 0xac,  3, ARG_NUM99,  0, NA_T },
    { /* FC_T */       docmd_fc_t,        "FC?",                 0x00, 0xa5, 0x00, 0xad,  3, ARG_NUM99,  0, NA_T },
    { /* FSC_T */      docmd_fsc_t,       "FS?C",                0x00, 0xa2, 0x00, 0xaa,  4, ARG_NUM99,  0, NA_T },
    { /* FCC_T */      docmd_fcc_t,       "FC?C",                0x00, 0xa3, 0x00, 0xab,  4, ARG_NUM99,  0, NA_T },
    { /* COMB */       docmd_comb,        "COMB",                0x00, 0x00, 0xa0, 0x6f,  4, ARG_NONE,   2, 0x01 },
    { /* PERM */       docmd_perm,        "PERM",                0x00, 0x00, 0xa0, 0x70,  4, ARG_NONE,   2, 0x01 },
    { /* FACT */       docmd_fact,        "N!",                  0x00, 0x00, 0x00, 0x62,  2, ARG_NONE,   1, 0x05 },
    { /* GAMMA */      docmd_gamma,       "GAM\315\301",         0x00, 0x00, 0xa0, 0x74,  5, ARG_NONE,   1, 0x05 },
    { /* RAN */        docmd_ran,         "RAN",                 0x00, 0x00, 0xa0, 0x71,  3, ARG_NONE,   0, NA_T },
    { /* SEED */       docmd_seed,        "SEED",                0x00, 0x00, 0xa0, 0x73,  4, ARG_NONE,   1, 0x01 },
    { /* LBL */        docmd_lbl,         "LBL",                 0x21, 0x00, 0x00, 0xcf,  3, ARG_OTHER,  0, NA_T },
    { /* RTN */        docmd_rtn,         "RTN",                 0x00, 0x00, 0x00, 0x85,  3, ARG_NONE,   0, NA_T },
    { /* INPUT */      docmd_input,       "INPUT",               0x21, 0xc5, 0xf2, 0xd0,  5, ARG_VAR,    0, NA_T },
    { /* VIEW */       docmd_view,        "VIEW",                0x00, 0x80, 0x00, 0x98,  4, ARG_VAR,    0, NA_T },
    { /* AVIEW */      docmd_aview,       "AVIEW",               0x00, 0x00, 0x00, 0x7e,  5, ARG_NONE,   0, NA_T },
    { /* XEQ */        docmd_xeq,         "XEQ",                 0x20, 0xa7, 0x00, 0xae,  3, ARG_LBL,    0, NA_T },
    { /* PROMPT */     docmd_prompt,      "PROM\320\324",        0x00, 0x00, 0x00, 0x8e,  6, ARG_NONE,   0, NA_T },
    { /* PSE */        docmd_pse,         "PSE",                 0x00, 0x00, 0x00, 0x89,  3, ARG_NONE,   0, NA_T },
    { /* ISG */        docmd_isg,         "ISG",                 0x00, 0x96, 0x00, 0x96,  3, ARG_REAL,   0, NA_T },
    { /* DSE */        docmd_dse,         "DSE",                 0x00, 0x97, 0x00, 0x97,  3, ARG_REAL,   0, NA_T },
    { /* AIP */        docmd_aip,         "AIP",                 0x00, 0x00, 0xa6, 0x31,  3, ARG_NONE,   1, 0x01 },
    { /* XTOA */       docmd_xtoa,        "XTOA",                0x00, 0x00, 0xa6, 0x6f,  4, ARG_NONE,   1, 0x15 },
    { /* AGRAPH */     docmd_agraph,      "AGRA\320\310",        0x00, 0x00, 0xa7, 0x64,  6, ARG_NONE,  -1, 0x00 },
    { /* PIXEL */      docmd_pixel,       "PIXEL",               0x00, 0x00, 0xa7, 0x65,  5, ARG_NONE,  -1, 0x00 },
    { /* BEEP */       docmd_beep,        "BEEP",                0x00, 0x00, 0x00, 0x86,  4, ARG_NONE,   0, NA_T },
    { /* TONE */       docmd_tone,        "TONE",                0x00, 0xd7, 0x00, 0x9f,  4, ARG_NUM9,   0, NA_T },
    { /* MVAR */       docmd_mvar,        "MVAR",                0x00, 0x90, 0x00, 0x00,  4, ARG_RVAR,   0, NA_T },
    { /* VARMENU */    docmd_varmenu,     "VARM\305\316\325",    0x00, 0xc1, 0xf2, 0xf8,  7, ARG_PRGM,   0, NA_T },
    { /* GETKEY */     docmd_getkey,      "GETK\305\331",        0x00, 0x00, 0xa2, 0x6e,  6, ARG_NONE,   0, NA_T },
    { /* MENU */       docmd_menu,        "MENU",                0x00, 0x00, 0xa2, 0x5e,  4, ARG_NONE,   0, NA_T },
    { /* KEYG */       NULL,              "KEYG",                0x40, 0x00, 0x00, 0x00,  4, ARG_MKEY,   0, NA_T },
    { /* KEYX */       NULL,              "KEYX",                0x40, 0x00, 0x00, 0x00,  4, ARG_MKEY,   0, NA_T },
    { /* X_EQ_0 */     docmd_x_eq_0,      "X=0?",                0x00, 0x00, 0x00, 0x67,  4, ARG_NONE,   1, 0x01 },
    { /* X_NE_0 */     docmd_x_ne_0,      "X\0140?",             0x00, 0x00, 0x00, 0x63,  4, ARG_NONE,   1, 0x01 },
    { /* X_LT_0 */     docmd_x_lt_0,      "X<0?",                0x00, 0x00, 0x00, 0x66,  4, ARG_NONE,   1, 0x01 },
    { /* X_GT_0 */     docmd_x_gt_0,      "X>0?",                0x00, 0x00, 0x00, 0x64,  4, ARG_NONE,   1, 0x01 },
    { /* X_LE_0 */     docmd_x_le_0,      "X\0110?",             0x00, 0x00, 0x00, 0x7b,  4, ARG_NONE,   1, 0x01 },
    { /* X_GE_0 */     docmd_x_ge_0,      "X\0130?",             0x00, 0x00, 0xa2, 0x5f,  4, ARG_NONE,   1, 0x01 },
    { /* X_EQ_Y */     docmd_x_eq_y,      "X=Y?",                0x00, 0x00, 0x00, 0x78,  4, ARG_NONE,   2, ALLT },
    { /* X_NE_Y */     docmd_x_ne_y,      "X\014Y?",             0x00, 0x00, 0x00, 0x79,  4, ARG_NONE,   2, ALLT },
    { /* X_LT_Y */     docmd_x_lt_y,      "X<Y?",                0x00, 0x00, 0x00, 0x44,  4, ARG_NONE,   2, 0x01 },
    { /* X_GT_Y */     docmd_x_gt_y,      "X>Y?",                0x00, 0x00, 0x00, 0x45,  4, ARG_NONE,   2, 0x01 },
    { /* X_LE_Y */     docmd_x_le_y,      "X\011Y?",             0x00, 0x00, 0x00, 0x46,  4, ARG_NONE,   2, 0x01 },
    { /* X_GE_Y */     docmd_x_ge_y,      "X\013Y?",             0x00, 0x00, 0xa2, 0x60,  4, ARG_NONE,   2, 0x01 },
    { /* PRSIGMA */    docmd_prsigma,     "PR\005",              0x00, 0x00, 0xa7, 0x52,  3, ARG_NONE,   0, NA_T },
    { /* PRP */        docmd_prp,         "PRP",                 0x42, 0x00, 0x00, 0x00,  3, ARG_PRGM,   0, NA_T },
    { /* PRV */        docmd_prv,         "PRV",                 0x00, 0xb1, 0xf2, 0xd9,  3, ARG_NAMED,  0, NA_T },
    { /* PRSTK */      docmd_prstk,       "PRST\313",            0x00, 0x00, 0xa7, 0x53,  5, ARG_NONE,   0, NA_T },
    { /* PRA */        docmd_pra,         "PRA",                 0x00, 0x00, 0xa7, 0x48,  3, ARG_NONE,   0, NA_T },
    { /* PRX */        docmd_prx,         "PRX",                 0x00, 0x00, 0xa7, 0x54,  3, ARG_NONE,   1, ALLT },
    { /* PRUSR */      docmd_prusr,       "PRUSR",               0x00, 0x00, 0xa7, 0x61,  5, ARG_NONE,   0, NA_T },
    { /* LIST */       docmd_list,        "LIST",                0x42, 0x00, 0x00, 0x00,  4, ARG_COUNT,  0, NA_T },
    { /* ADV */        docmd_adv,         "ADV",                 0x00, 0x00, 0x00, 0x8f,  3, ARG_NONE,   0, NA_T },
    { /* PRLCD */      docmd_prlcd,       "PRLCD",               0x00, 0x00, 0xa7, 0x62,  5, ARG_NONE,   0, NA_T },
    { /* DELAY */      docmd_delay,       "DELAY",               0x00, 0x00, 0xa7, 0x60,  5, ARG_NONE,   1, 0x01 },
    { /* PON */        docmd_pon,         "P\322ON",             0x00, 0x00, 0xa7, 0x5e,  4, ARG_NONE,   0, NA_T },
    { /* POFF */       docmd_poff,        "P\322OFF",            0x00, 0x00, 0xa7, 0x5f,  5, ARG_NONE,   0, NA_T },
    { /* MAN */        docmd_man,         "MAN",                 0x00, 0x00, 0xa7, 0x5b,  3, ARG_NONE,   0, NA_T },
    { /* NORM */       docmd_norm,        "NORM",                0x00, 0x00, 0xa7, 0x5c,  4, ARG_NONE,   0, NA_T },
    { /* TRACE */      docmd_trace,       "TRACE",               0x00, 0x00, 0xa7, 0x5d,  5, ARG_NONE,   0, NA_T },
    { /* SIGMAADD */   docmd_sigmaadd,    "\005+",               0x00, 0x00, 0x00, 0x47,  2, ARG_NONE,   2, FUNC },
    { /* SIGMASUB */   docmd_sigmasub,    "\005-",               0x00, 0x00, 0x00, 0x48,  2, ARG_NONE,   2, FUNC },
    { /* GTO */        docmd_gto,         "GTO",                 0x20, 0xa6, 0x00, 0x00,  3, ARG_LBL,    0, NA_T },
    { /* END */        docmd_rtn,         "END",                 0x20, 0x00, 0x00, 0x00,  3, ARG_NONE,   0, NA_T },
    { /* NUMBER */     docmd_number,      "",                    0x24, 0x00, 0x00, 0x00,  0, ARG_NONE,   0, NA_T },
    { /* STRING */     docmd_string,      "",                    0x24, 0x00, 0x00, 0x00,  0, ARG_NONE,   0, NA_T },
    { /* RUN */        NULL,              "RUN",                 0x44, 0x00, 0x00, 0x00,  3, ARG_NONE,   0, NA_T },
    { /* SST */        NULL,              "SST",                 0x40, 0x00, 0x00, 0x00,  3, ARG_NONE,   0, NA_T },
    { /* GTODOT */     docmd_gtodot,      "GTO .",               0x42, 0x00, 0x00, 0x00,  5, ARG_OTHER,  0, NA_T },
    { /* GTODOTDOT */  docmd_gtodotdot,   "GTO ..",              0x42, 0x00, 0x00, 0x00,  6, ARG_NONE,   0, NA_T },
    { /* STOP */       docmd_stop,        "STOP",                0x00, 0x00, 0x00, 0x84,  4, ARG_NONE,   0, NA_T },
    { /* NEWMAT */     docmd_newmat,      "NEW\315\301\324",     0x00, 0x00, 0xa6, 0xda,  6, ARG_NONE,   2, 0x01 },
    { /* RUP */        docmd_rup,         "R^",                  0x00, 0x00, 0x00, 0x74,  2, ARG_NONE,   0, NA_T },
    { /* REAL_T */     docmd_real_t,      "RE\301L?",            0x00, 0x00, 0xa2, 0x65,  5, ARG_NONE,   1, ALLT },
    { /* CPX_T */      docmd_cpx_t,       "CPX?",                0x00, 0x00, 0xa2, 0x67,  4, ARG_NONE,   1, ALLT },
    { /* STR_T */      docmd_str_t,       "STR?",                0x00, 0x00, 0xa2, 0x68,  4, ARG_NONE,   1, ALLT },
    { /* MAT_T */      docmd_mat_t,       "MAT?",                0x00, 0x00, 0xa2, 0x66,  4, ARG_NONE,   1, ALLT },
    { /* DIM_T */      docmd_dim_t,       "DIM?",                0x00, 0x00, 0xa6, 0xe7,  4, ARG_NONE,   1, 0x0c },
    { /* ASSIGNa */    NULL,              "AS\323\311GN",        0x40, 0x00, 0x00, 0x00,  6, ARG_NAMED,  0, NA_T },
    { /* ASSIGNb */    NULL,              "",                    0x44, 0x00, 0x00, 0x00,  0, ARG_CKEY,   0, NA_T },
    { /* ASGN01 */     docmd_asgn01,      "",                    0x24, 0x00, 0x00, 0x00,  0, ARG_OTHER,  0, NA_T },
    { /* ASGN02 */     docmd_asgn02,      "",                    0x24, 0x00, 0x00, 0x00,  0, ARG_OTHER,  0, NA_T },
    { /* ASGN03 */     docmd_asgn03,      "",                    0x24, 0x00, 0x00, 0x00,  0, ARG_OTHER,  0, NA_T },
    { /* ASGN04 */     docmd_asgn04,      "",                    0x24, 0x00, 0x00, 0x00,  0, ARG_OTHER,  0, NA_T },
    { /* ASGN05 */     docmd_asgn05,      "",                    0x24, 0x00, 0x00, 0x00,  0, ARG_OTHER,  0, NA_T },
    { /* ASGN06 */     docmd_asgn06,      "",                    0x24, 0x00, 0x00, 0x00,  0, ARG_OTHER,  0, NA_T },
    { /* ASGN07 */     docmd_asgn07,      "",                    0x24, 0x00, 0x00, 0x00,  0, ARG_OTHER,  0, NA_T },
    { /* ASGN08 */     docmd_asgn08,      "",                    0x24, 0x00, 0x00, 0x00,  0, ARG_OTHER,  0, NA_T },
    { /* ASGN09 */     docmd_asgn09,      "",                    0x24, 0x00, 0x00, 0x00,  0, ARG_OTHER,  0, NA_T },
    { /* ASGN10 */     docmd_asgn10,      "",                    0x24, 0x00, 0x00, 0x00,  0, ARG_OTHER,  0, NA_T },
    { /* ASGN11 */     docmd_asgn11,      "",                    0x24, 0x00, 0x00, 0x00,  0, ARG_OTHER,  0, NA_T },
    { /* ASGN12 */     docmd_asgn12,      "",                    0x24, 0x00, 0x00, 0x00,  0, ARG_OTHER,  0, NA_T },
    { /* ASGN13 */     docmd_asgn13,      "",                    0x24, 0x00, 0x00, 0x00,  0, ARG_OTHER,  0, NA_T },
    { /* ASGN14 */     docmd_asgn14,      "",                    0x24, 0x00, 0x00, 0x00,  0, ARG_OTHER,  0, NA_T },
    { /* ASGN15 */     docmd_asgn15,      "",                    0x24, 0x00, 0x00, 0x00,  0, ARG_OTHER,  0, NA_T },
    { /* ASGN16 */     docmd_asgn16,      "",                    0x24, 0x00, 0x00, 0x00,  0, ARG_OTHER,  0, NA_T },
    { /* ASGN17 */     docmd_asgn17,      "",                    0x24, 0x00, 0x00, 0x00,  0, ARG_OTHER,  0, NA_T },
    { /* ASGN18 */     docmd_asgn18,      "",                    0x24, 0x00, 0x00, 0x00,  0, ARG_OTHER,  0, NA_T },
    { /* ON */         docmd_on,          "ON",                  0x00, 0x00, 0xa2, 0x70,  2, ARG_NONE,   0, NA_T },
    { /* OFF */        docmd_off,         "OFF",                 0x00, 0x00, 0x00, 0x8d,  3, ARG_NONE,   0, NA_T },
    { /* KEY1G */      docmd_key1g,       "KEY 1 GTO",           0x24, 0x00, 0x00, 0x00,  9, ARG_LBL,    0, NA_T },
    { /* KEY2G */      docmd_key2g,       "KEY 2 GTO",           0x24, 0x00, 0x00, 0x00,  9, ARG_LBL,    0, NA_T },
    { /* KEY3G */      docmd_key3g,       "KEY 3 GTO",           0x24, 0x00, 0x00, 0x00,  9, ARG_LBL,    0, NA_T },
    { /* KEY4G */      docmd_key4g,       "KEY 4 GTO",           0x24, 0x00, 0x00, 0x00,  9, ARG_LBL,    0, NA_T },
    { /* KEY5G */      docmd_key5g,       "KEY 5 GTO",           0x24, 0x00, 0x00, 0x00,  9, ARG_LBL,    0, NA_T },
    { /* KEY6G */      docmd_key6g,       "KEY 6 GTO",           0x24, 0x00, 0x00, 0x00,  9, ARG_LBL,    0, NA_T },
    { /* KEY7G */      docmd_key7g,       "KEY 7 GTO",           0x24, 0x00, 0x00, 0x00,  9, ARG_LBL,    0, NA_T },
    { /* KEY8G */      docmd_key8g,       "KEY 8 GTO",           0x24, 0x00, 0x00, 0x00,  9, ARG_LBL,    0, NA_T },
    { /* KEY9G */      docmd_key9g,       "KEY 9 GTO",           0x24, 0x00, 0x00, 0x00,  9, ARG_LBL,    0, NA_T },
    { /* KEY1X */      docmd_key1x,       "KEY 1 XEQ",           0x24, 0x00, 0x00, 0x00,  9, ARG_LBL,    0, NA_T },
    { /* KEY2X */      docmd_key2x,       "KEY 2 XEQ",           0x24, 0x00, 0x00, 0x00,  9, ARG_LBL,    0, NA_T },
    { /* KEY3X */      docmd_key3x,       "KEY 3 XEQ",           0x24, 0x00, 0x00, 0x00,  9, ARG_LBL,    0, NA_T },
    { /* KEY4X */      docmd_key4x,       "KEY 4 XEQ",           0x24, 0x00, 0x00, 0x00,  9, ARG_LBL,    0, NA_T },
    { /* KEY5X */      docmd_key5x,       "KEY 5 XEQ",           0x24, 0x00, 0x00, 0x00,  9, ARG_LBL,    0, NA_T },
    { /* KEY6X */      docmd_key6x,       "KEY 6 XEQ",           0x24, 0x00, 0x00, 0x00,  9, ARG_LBL,    0, NA_T },
    { /* KEY7X */      docmd_key7x,       "KEY 7 XEQ",           0x24, 0x00, 0x00, 0x00,  9, ARG_LBL,    0, NA_T },
    { /* KEY8X */      docmd_key8x,       "KEY 8 XEQ",           0x24, 0x00, 0x00, 0x00,  9, ARG_LBL,    0, NA_T },
    { /* KEY9X */      docmd_key9x,       "KEY 9 XEQ",           0x24, 0x00, 0x00, 0x00,  9, ARG_LBL,    0, NA_T },
    { /* VMEXEC */     NULL,              "",                    0x44, 0x00, 0x00, 0x00,  0, ARG_OTHER,  0, NA_T },
    { /* VMSTO */      docmd_vmsto,       "STO",                 0x44, 0x00, 0x00, 0x00,  3, ARG_OTHER,  1, ALLT },
    { /* SIGMAREG */   docmd_sigma_reg,   "\005REG",             0x00, 0xd3, 0x00, 0x99,  4, ARG_NUM99,  0, NA_T },
    { /* SIGMAREG_T */ docmd_sigma_reg_t, "\005R\305G?",         0x00, 0x00, 0xa6, 0x78,  5, ARG_NONE,   0, NA_T },
    { /* CLD */        docmd_cld,         "CLD",                 0x00, 0x00, 0x00, 0x7f,  3, ARG_NONE,   0, NA_T },
    { /* ACOSH */      docmd_acosh,       "ACOSH",               0x00, 0x00, 0xa0, 0x66,  5, ARG_NONE,   1, 0x0f },
    { /* ALENG */      docmd_aleng,       "ALEN\307",            0x00, 0x00, 0xa6, 0x41,  5, ARG_NONE,   0, NA_T },
    { /* ALLSIGMA */   docmd_allsigma,    "ALL\005",             0x00, 0x00, 0xa0, 0xae,  4, ARG_NONE,   0, NA_T },
    { /* AND */        docmd_and,         "AND",                 0x00, 0x00, 0xa5, 0x88,  3, ARG_NONE,   2, 0x01 },
    { /* AOFF */       docmd_aoff,        "AOFF",                0x00, 0x00, 0x00, 0x8b,  4, ARG_NONE,   0, NA_T },
    { /* AON */        docmd_aon,         "AON",                 0x00, 0x00, 0x00, 0x8c,  3, ARG_NONE,   0, NA_T },
    { /* AROT */       docmd_arot,        "AROT",                0x00, 0x00, 0xa6, 0x46,  4, ARG_NONE,   1, 0x01 },
    { /* ASHF */       docmd_ashf,        "ASHF",                0x00, 0x00, 0x00, 0x88,  4, ARG_NONE,   0, NA_T },
    { /* ASINH */      docmd_asinh,       "ASINH",               0x00, 0x00, 0xa0, 0x64,  5, ARG_NONE,   1, 0x0f },
    { /* ATANH */      docmd_atanh,       "AT\301NH",            0x00, 0x00, 0xa0, 0x65,  5, ARG_NONE,   1, 0x0f },
    { /* ATOX */       docmd_atox,        "ATOX",                0x00, 0x00, 0xa6, 0x47,  4, ARG_NONE,   0, NA_T },
    { /* BASEADD */    docmd_baseadd,     "BASE+",               0x00, 0x00, 0xa0, 0xe6,  5, ARG_NONE,   2, 0x01 },
    { /* BASESUB */    docmd_basesub,     "BASE-",               0x00, 0x00, 0xa0, 0xe7,  5, ARG_NONE,   2, 0x01 },
    { /* BASEMUL */    docmd_basemul,     "BASE\001",            0x00, 0x00, 0xa0, 0xe8,  5, ARG_NONE,   2, 0x01 },
    { /* BASEDIV */    docmd_basediv,     "BASE\000",            0x00, 0x00, 0xa0, 0xe9,  5, ARG_NONE,   2, 0x01 },
    { /* BASECHS */    docmd_basechs,     "B\301\323\305+/-",    0x00, 0x00, 0xa0, 0xea,  7, ARG_NONE,   1, 0x01 },
    { /* BEST */       docmd_best,        "BEST",                0x00, 0x00, 0xa0, 0x9f,  4, ARG_NONE,   0, NA_T },
    { /* BINM */       docmd_binm,        "BINM",                0x00, 0x00, 0xa0, 0xe5,  4, ARG_NONE,   0, NA_T },
    { /* BIT_T */      docmd_bit_t,       "BIT?",                0x00, 0x00, 0xa5, 0x8c,  4, ARG_NONE,   2, 0x01 },
    { /* BST */        NULL,              "BST",                 0x40, 0x00, 0x00, 0x00,  3, ARG_NONE,   0, NA_T },
    { /* CORR */       docmd_corr,        "CORR",                0x00, 0x00, 0xa0, 0xa7,  4, ARG_NONE,   0, NA_T },
    { /* COSH */       docmd_cosh,        "COSH",                0x00, 0x00, 0xa0, 0x62,  4, ARG_NONE,   1, 0x0f },
    { /* CROSS */      docmd_cross,       "CROSS",               0x00, 0x00, 0xa6, 0xca,  5, ARG_NONE,   2, FUNC },
    { /* CUSTOM */     docmd_custom,      "CUST\317\315",        0x00, 0x00, 0xa2, 0x6f,  6, ARG_NONE,   0, NA_T },
    { /* DECM */       docmd_decm,        "DECM",                0x00, 0x00, 0xa0, 0xe3,  4, ARG_NONE,   0, NA_T },
    { /* DELR */       docmd_delr,        "DELR",                0x00, 0x00, 0xa0, 0xab,  4, ARG_NONE,   0, NA_T },
    { /* DET */        docmd_det,         "DET",                 0x00, 0x00, 0xa6, 0xcc,  3, ARG_NONE,   1, 0x0c },
    { /* DIM */        docmd_dim,         "DIM",                 0x00, 0xc4, 0xf2, 0xec,  3, ARG_MAT,    2, 0x01 },
    { /* DOT */        docmd_dot,         "DOT",                 0x00, 0x00, 0xa6, 0xcb,  3, ARG_NONE,   2, FUNC },
    { /* EDIT */       docmd_edit,        "EDIT",                0x00, 0x00, 0xa6, 0xe1,  4, ARG_NONE,   1, FUNC },
    { /* EDITN */      docmd_editn,       "EDITN",               0x00, 0xc6, 0xf2, 0xef,  5, ARG_MAT,    0, NA_T },
    { /* EXITALL */    docmd_exitall,     "EXITA\314\314",       0x00, 0x00, 0xa2, 0x6c,  7, ARG_NONE,   0, NA_T },
    { /* EXPF */       docmd_expf,        "EXPF",                0x00, 0x00, 0xa0, 0xa0,  4, ARG_NONE,   0, NA_T },
    { /* E_POW_X_1 */  docmd_e_pow_x_1,   "E^X-\261",            0x00, 0x00, 0x00, 0x58,  5, ARG_NONE,   1, 0x05 },
    { /* FCSTX */      docmd_fcstx,       "FCSTX",               0x00, 0x00, 0xa0, 0xa8,  5, ARG_NONE,   1, FUNC },
    { /* FCSTY */      docmd_fcsty,       "FCSTY",               0x00, 0x00, 0xa0, 0xa9,  5, ARG_NONE,   1, FUNC },
    { /* FNRM */       docmd_fnrm,        "FNRM",                0x00, 0x00, 0xa6, 0xcf,  4, ARG_NONE,   1, 0x0c },
    { /* GETM */       docmd_getm,        "GETM",                0x00, 0x00, 0xa6, 0xe8,  4, ARG_NONE,   2, FUNC },
    { /* GROW */       docmd_grow,        "GROW",                0x00, 0x00, 0xa6, 0xe3,  4, ARG_NONE,   0, NA_T },
    { /* HEXM */       docmd_hexm,        "HEXM",                0x00, 0x00, 0xa0, 0xe2,  4, ARG_NONE,   0, NA_T },
    { /* HMSADD */     docmd_hmsadd,      "HMS+",                0x00, 0x00, 0x00, 0x49,  4, ARG_NONE,   2, 0x01 },
    { /* HMSSUB */     docmd_hmssub,      "HMS-",                0x00, 0x00, 0x00, 0x4a,  4, ARG_NONE,   2, 0x01 },
    { /* I_ADD */      docmd_i_add,       "I+",                  0x00, 0x00, 0xa6, 0xd2,  2, ARG_NONE,   0, NA_T },
    { /* I_SUB */      docmd_i_sub,       "I-",                  0x00, 0x00, 0xa6, 0xd3,  2, ARG_NONE,   0, NA_T },
    { /* INDEX */      docmd_index,       "INDEX",               0x00, 0x87, 0xf2, 0xda,  5, ARG_MAT,    0, NA_T },
    { /* INSR */       docmd_insr,        "INSR",                0x00, 0x00, 0xa0, 0xaa,  4, ARG_NONE,   0, NA_T },
    { /* INTEG */      docmd_integ,       "INTEG",               0x00, 0xb6, 0xf2, 0xea,  5, ARG_RVAR,   0, NA_T },
    { /* INVRT */      docmd_invrt,       "INV\322\324",         0x00, 0x00, 0xa6, 0xce,  5, ARG_NONE,   1, 0x0c },
    { /* J_ADD */      docmd_j_add,       "J+",                  0x00, 0x00, 0xa6, 0xd4,  2, ARG_NONE,   0, NA_T },
    { /* J_SUB */      docmd_j_sub,       "J-",                  0x00, 0x00, 0xa6, 0xd5,  2, ARG_NONE,   0, NA_T },
    { /* LINF */       docmd_linf,        "LINF",                0x00, 0x00, 0xa0, 0xa1,  4, ARG_NONE,   0, NA_T },
    { /* LINSIGMA */   docmd_linsigma,    "LIN\005",             0x00, 0x00, 0xa0, 0xad,  4, ARG_NONE,   0, NA_T },
    { /* LN_1_X */     docmd_ln_1_x,      "LN1+\330",            0x00, 0x00, 0x00, 0x65,  5, ARG_NONE,   1, 0x05 },
    { /* LOGF */       docmd_logf,        "LOGF",                0x00, 0x00, 0xa0, 0xa2,  4, ARG_NONE,   0, NA_T },
    { /* MEAN */       docmd_mean,        "MEAN",                0x00, 0x00, 0x00, 0x7c,  4, ARG_NONE,   0, NA_T },
    { /* NOT */        docmd_not,         "NOT",                 0x00, 0x00, 0xa5, 0x87,  3, ARG_NONE,   1, 0x01 },
    { /* OCTM */       docmd_octm,        "OCTM",                0x00, 0x00, 0xa0, 0xe4,  4, ARG_NONE,   0, NA_T },
    { /* OLD */        docmd_rclel,       "OLD",                 0x00, 0x00, 0xa6, 0xdb,  3, ARG_NONE,   0, NA_T },
    { /* OR */         docmd_or,          "OR",                  0x00, 0x00, 0xa5, 0x89,  2, ARG_NONE,   2, 0x01 },
    { /* PGMSLV */     docmd_pgmslv,      "P\307\315SLV",        0x00, 0xb5, 0xf2, 0xe9,  6, ARG_PRGM,   0, NA_T },
    { /* PGMINT */     docmd_pgmint,      "P\307\315INT",        0x00, 0xb4, 0xf2, 0xe8,  6, ARG_PRGM,   0, NA_T },
    { /* POSA */       docmd_posa,        "POSA",                0x00, 0x00, 0xa6, 0x5c,  4, ARG_NONE,   1, 0x11 },
    { /* PUTM */       docmd_putm,        "PUTM",                0x00, 0x00, 0xa6, 0xe9,  4, ARG_NONE,   1, FUNC },
    { /* PWRF */       docmd_pwrf,        "PWRF",                0x00, 0x00, 0xa0, 0xa3,  4, ARG_NONE,   0, NA_T },
    { /* RCLEL */      docmd_rclel,       "RCLEL",               0x00, 0x00, 0xa6, 0xd7,  5, ARG_NONE,   0, NA_T },
    { /* RCLIJ */      docmd_rclij,       "RCLIJ",               0x00, 0x00, 0xa6, 0xd9,  5, ARG_NONE,   0, NA_T },
    { /* RNRM */       docmd_rnrm,        "RNRM",                0x00, 0x00, 0xa6, 0xed,  4, ARG_NONE,   1, 0x0c },
    { /* ROTXY */      docmd_rotxy,       "ROTXY",               0x00, 0x00, 0xa5, 0x8b,  5, ARG_NONE,   2, 0x01 },
    { /* RSUM */       docmd_rsum,        "RSUM",                0x00, 0x00, 0xa6, 0xd0,  4, ARG_NONE,   1, 0x0c },
    { /* SWAP_R */     docmd_swap_r,      "R<>R",                0x00, 0x00, 0xa6, 0xd1,  4, ARG_NONE,   2, FUNC },
    { /* SDEV */       docmd_sdev,        "SDEV",                0x00, 0x00, 0x00, 0x7d,  4, ARG_NONE,   0, NA_T },
    { /* SINH */       docmd_sinh,        "SINH",                0x00, 0x00, 0xa0, 0x61,  4, ARG_NONE,   1, 0x0f },
    { /* SLOPE */      docmd_slope,       "SLOPE",               0x00, 0x00, 0xa0, 0xa4,  5, ARG_NONE,   0, NA_T },
    { /* SOLVE */      docmd_solve,       "SOLVE",               0x00, 0xb7, 0xf2, 0xeb,  5, ARG_RVAR,   1, FUNC },
    { /* STOEL */      docmd_stoel,       "STOEL",               0x00, 0x00, 0xa6, 0xd6,  5, ARG_NONE,   1, FUNC },
    { /* STOIJ */      docmd_stoij,       "STOIJ",               0x00, 0x00, 0xa6, 0xd8,  5, ARG_NONE,   2, FUNC },
    { /* SUM */        docmd_sum,         "SUM",                 0x00, 0x00, 0xa0, 0xa5,  3, ARG_NONE,   0, NA_T },
    { /* TANH */       docmd_tanh,        "TANH",                0x00, 0x00, 0xa0, 0x63,  4, ARG_NONE,   1, 0x0f },
    { /* TRANS */      docmd_trans,       "TRANS",               0x00, 0x00, 0xa6, 0xc9,  5, ARG_NONE,   1, 0x0c },
    { /* UVEC */       docmd_uvec,        "UVEC",                0x00, 0x00, 0xa6, 0xcd,  4, ARG_NONE,   1, 0x06 },
    { /* WMEAN */      docmd_wmean,       "WM\305\301N",         0x00, 0x00, 0xa0, 0xac,  5, ARG_NONE,   0, NA_T },
    { /* WRAP */       docmd_wrap,        "WRAP",                0x00, 0x00, 0xa6, 0xe2,  4, ARG_NONE,   0, NA_T },
    { /* X_SWAP */     docmd_x_swap,      "X<>",                 0x00, 0x86, 0x00, 0xce,  3, ARG_VAR,    1, ALLT },
    { /* XOR */        docmd_xor,         "XOR",                 0x00, 0x00, 0xa5, 0x8a,  3, ARG_NONE,   2, 0x01 },
    { /* YINT */       docmd_yint,        "YINT",                0x00, 0x00, 0xa0, 0xa6,  4, ARG_NONE,   0, NA_T },
    { /* TO_DEC */     docmd_to_dec,      "\017DEC",             0x00, 0x00, 0x00, 0x5f,  4, ARG_NONE,   1, 0x01 },
    { /* TO_OCT */     docmd_to_oct,      "\017OCT",             0x00, 0x00, 0x00, 0x6f,  4, ARG_NONE,   1, 0x01 },
    { /* LEFT */       docmd_left,        "\020",                0x00, 0x00, 0xa6, 0xdc,  1, ARG_NONE,   0, FUNC },
    { /* UP */         docmd_up,          "^",                   0x00, 0x00, 0xa6, 0xde,  1, ARG_NONE,   0, FUNC },
    { /* DOWN */       docmd_down,        "\016",                0x00, 0x00, 0xa6, 0xdf,  1, ARG_NONE,   0, FUNC },
    { /* RIGHT */      docmd_right,       "\017",                0x00, 0x00, 0xa6, 0xdd,  1, ARG_NONE,   0, FUNC },
    { /* PERCENT_CH */ docmd_percent_ch,  "%CH",                 0x00, 0x00, 0x00, 0x4d,  3, ARG_NONE,   2, 0x01 },
    { /* SIMQ */       docmd_simq,        "SIMQ",                0x4c, 0x00, 0x00, 0x00,  4, ARG_COUNT,  0, NA_T },
    { /* MATA */       docmd_mata,        "MATA",                0x4c, 0x00, 0x00, 0x00,  4, ARG_NONE,   0, NA_T },
    { /* MATB */       docmd_matb,        "MATB",                0x4c, 0x00, 0x00, 0x00,  4, ARG_NONE,   0, NA_T },
    { /* MATX */       docmd_matx,        "MATX",                0x4c, 0x00, 0x00, 0x00,  4, ARG_NONE,   0, NA_T },
    { /* GOTOROW */    NULL,              "GOTO\240\322\357\367",0x44, 0x00, 0x00, 0x00,  8, ARG_COUNT,  0, NA_T },
    { /* GOTOCOLUMN */ NULL,              "GOTO Column",         0x44, 0x00, 0x00, 0x00, 11, ARG_COUNT,  0, NA_T },
    { /* A_THRU_F */   NULL,              "A...F",               0x4c, 0x00, 0x00, 0x00,  5, ARG_NONE,   0, NA_T },
    { /* CLALLb */     docmd_clall,       "CLALL",               0x44, 0x00, 0x00, 0x00,  5, ARG_NONE,   0, NA_T },
    { /* PGMSLVi */    docmd_pgmslvi,     "P\307\315SLV",        0x44, 0x00, 0x00, 0x00,  6, ARG_PRGM,   0, NA_T },
    { /* PGMINTi */    docmd_pgminti,     "P\307\315INT",        0x44, 0x00, 0x00, 0x00,  6, ARG_PRGM,   0, NA_T },
    { /* VMSTO2 */     docmd_vmsto2,      "STO",                 0x44, 0x00, 0x00, 0x00,  3, ARG_OTHER,  1, ALLT },
    { /* VMSOLVE */    docmd_vmsolve,     "SOLVE",               0x44, 0x00, 0x00, 0x00,  5, ARG_OTHER,  0, NA_T },
    { /* MAX */        docmd_max,         "[MAX]",               0x00, 0x00, 0xa6, 0xeb,  5, ARG_NONE,   0, NA_T },
    { /* MIN */        docmd_min,         "[MIN]",               0x00, 0x00, 0xa6, 0xea,  5, ARG_NONE,   0, NA_T },
    { /* FIND */       docmd_find,        "[F\311ND]",           0x00, 0x00, 0xa6, 0xec,  6, ARG_NONE,   1, FUNC },
    { /* XROM */       docmd_xrom,        "XROM",                0x24, 0x00, 0x00, 0x00,  4, ARG_OTHER,  0, NA_T },

    /* Here endeth the original Free42 function table. */

    /* Underhill's COPAN (Obsolete) */
    { /* OPENF */      docmd_xrom,        "OPENF",               0x00, 0x00, 0xa7, 0xc1,  5, ARG_NONE,   0, UNIM },
    { /* CLOSF */      docmd_xrom,        "CLOSF",               0x00, 0x00, 0xa7, 0xc2,  5, ARG_NONE,   0, UNIM },
    { /* READP */      docmd_xrom,        "READP",               0x00, 0x00, 0xa7, 0xc3,  5, ARG_NONE,   0, UNIM },
    { /* WRITP */      docmd_xrom,        "WRITP",               0x00, 0x00, 0xa7, 0xc4,  5, ARG_NONE,   0, UNIM },
    { /* GETXY */      docmd_xrom,        "GETXY",               0x00, 0x00, 0xa7, 0xc5,  5, ARG_NONE,   0, UNIM },
    { /* PUTXY */      docmd_xrom,        "PUTXY",               0x00, 0x00, 0xa7, 0xc6,  5, ARG_NONE,   0, UNIM },
    { /* CLRP */       docmd_xrom,        "CLRP",                0x00, 0x00, 0xa7, 0xc7,  4, ARG_NONE,   0, UNIM },
    { /* CLRD */       docmd_xrom,        "CLRD",                0x00, 0x00, 0xa7, 0xc8,  4, ARG_NONE,   0, UNIM },
    { /* APPD */       docmd_xrom,        "APPD",                0x00, 0x00, 0xa7, 0xc9,  4, ARG_NONE,   0, UNIM },
    { /* GETN */       docmd_xrom,        "GETN",                0x00, 0x00, 0xa7, 0xca,  4, ARG_NONE,   0, UNIM },
    { /* PUTN */       docmd_xrom,        "PUTN",                0x00, 0x00, 0xa7, 0xcb,  4, ARG_NONE,   0, UNIM },
    { /* GETZ */       docmd_xrom,        "GETZ",                0x00, 0x00, 0xa7, 0xcc,  4, ARG_NONE,   0, UNIM },
    { /* PUTZ */       docmd_xrom,        "PUTZ",                0x00, 0x00, 0xa7, 0xcd,  4, ARG_NONE,   0, UNIM },
    { /* DELP */       docmd_xrom,        "DELP",                0x00, 0x00, 0xa7, 0xce,  4, ARG_NONE,   0, UNIM },

    /* Big Stack; additional functions from 4STK */
    { /* DROP */       docmd_drop,        "DROP",                0x00, 0x00, 0xa2, 0x71,  4, ARG_NONE,   1, ALLT },

    /* Accelerometer, GPS, and compass support */
    { /* ACCEL */      docmd_accel,       "ACCEL",               0x00, 0x00, 0xa7, 0xcf,  5, ARG_NONE,   0, NA_T },
    { /* LOCAT */      docmd_locat,       "LOCAT",               0x00, 0x00, 0xa7, 0xd0,  5, ARG_NONE,   0, NA_T },
    { /* HEADING */    docmd_heading,     "H\305\301D\311NG",    0x00, 0x00, 0xa7, 0xd1,  7, ARG_NONE,   0, NA_T },

    /* Time Module & CX Time support*/
    { /* ADATE */      docmd_adate,       "ADATE",               0x00, 0x00, 0xa6, 0x81,  5, ARG_NONE,   1, 0x01 },
    { /* ALMCAT */     docmd_xrom,        "AL\315CAT",           0x04, 0x00, 0xa6, 0x82,  6, ARG_NONE,   0, UNIM },
    { /* ALMNOW */     docmd_xrom,        "AL\315N\317W",        0x04, 0x00, 0xa6, 0x83,  6, ARG_NONE,   0, UNIM },
    { /* ATIME */      docmd_atime,       "ATIME",               0x00, 0x00, 0xa6, 0x84,  5, ARG_NONE,   1, 0x01 },
    { /* ATIME24 */    docmd_atime24,     "AT\311\315\30524",    0x00, 0x00, 0xa6, 0x85,  7, ARG_NONE,   1, 0x01 },
    { /* CLK12 */      docmd_clk12,       "CL\31312",            0x00, 0x00, 0xa6, 0x86,  5, ARG_NONE,   0, NA_T },
    { /* CLK24 */      docmd_clk24,       "CL\31324",            0x00, 0x00, 0xa6, 0x87,  5, ARG_NONE,   0, NA_T },
    { /* CLKT */       docmd_xrom,        "CLKT",                0x04, 0x00, 0xa6, 0x88,  4, ARG_NONE,   0, UNIM },
    { /* CLKTD */      docmd_xrom,        "CL\313TD",            0x04, 0x00, 0xa6, 0x89,  5, ARG_NONE,   0, UNIM },
    { /* CLOCK */      docmd_xrom,        "CL\317\303K",         0x04, 0x00, 0xa6, 0x8a,  5, ARG_NONE,   0, UNIM },
    { /* CORRECT */    docmd_xrom,        "CORR\305\303\324",    0x04, 0x00, 0xa6, 0x8b,  7, ARG_NONE,   0, UNIM },
    { /* DATE */       docmd_date,        "DATE",                0x00, 0x00, 0xa6, 0x8c,  4, ARG_NONE,   0, NA_T },
    { /* DATE_PLUS */  docmd_date_plus,   "DATE+",               0x00, 0x00, 0xa6, 0x8d,  5, ARG_NONE,   2, 0x01 },
    { /* DDAYS */      docmd_ddays,       "DDAYS",               0x00, 0x00, 0xa6, 0x8e,  5, ARG_NONE,   2, 0x01 },
    { /* DMY */        docmd_dmy,         "DMY",                 0x00, 0x00, 0xa6, 0x8f,  3, ARG_NONE,   0, NA_T },
    { /* DOW */        docmd_dow,         "DOW",                 0x00, 0x00, 0xa6, 0x90,  3, ARG_NONE,   1, 0x01 },
    { /* MDY */        docmd_mdy,         "MDY",                 0x00, 0x00, 0xa6, 0x91,  3, ARG_NONE,   0, NA_T },
    { /* RCLAF */      docmd_xrom,        "RCLAF",               0x04, 0x00, 0xa6, 0x92,  5, ARG_NONE,   0, UNIM },
    { /* RCLSW */      docmd_xrom,        "RC\314SW",            0x04, 0x00, 0xa6, 0x93,  5, ARG_NONE,   0, UNIM },
    { /* RUNSW */      docmd_xrom,        "R\325NSW",            0x04, 0x00, 0xa6, 0x94,  5, ARG_NONE,   0, UNIM },
    { /* SETAF */      docmd_xrom,        "SETAF",               0x04, 0x00, 0xa6, 0x95,  5, ARG_NONE,   0, UNIM },
    { /* SETDATE */    docmd_xrom,        "S\305\324DATE",       0x04, 0x00, 0xa6, 0x96,  7, ARG_NONE,   0, UNIM },
    { /* SETIME */     docmd_xrom,        "S\305TIME",           0x04, 0x00, 0xa6, 0x97,  6, ARG_NONE,   0, UNIM },
    { /* SETSW */      docmd_xrom,        "SE\324SW",            0x04, 0x00, 0xa6, 0x98,  5, ARG_NONE,   0, UNIM },
    { /* STOPSW */     docmd_xrom,        "ST\317\320SW",        0x04, 0x00, 0xa6, 0x99,  6, ARG_NONE,   0, UNIM },
    { /* SW */         docmd_xrom,        "SW",                  0x04, 0x00, 0xa6, 0x9a,  2, ARG_NONE,   0, UNIM },
    { /* T_PLUS_X */   docmd_xrom,        "T+X",                 0x04, 0x00, 0xa6, 0x9b,  3, ARG_NONE,   0, UNIM },
    { /* TIME */       docmd_time,        "TIME",                0x00, 0x00, 0xa6, 0x9c,  4, ARG_NONE,   0, NA_T },
    { /* XYZALM */     docmd_xrom,        "XYZALM",              0x04, 0x00, 0xa6, 0x9d,  6, ARG_NONE,   0, UNIM },
    { /* CLALMA */     docmd_xrom,        "CLAL\315A",           0x04, 0x00, 0xa6, 0x9f,  6, ARG_NONE,   0, UNIM },
    { /* CLALMX */     docmd_xrom,        "CLAL\315X",           0x04, 0x00, 0xa6, 0xa0,  6, ARG_NONE,   0, UNIM },
    { /* CLRALMS */    docmd_xrom,        "CLRALMS",             0x04, 0x00, 0xa6, 0xa1,  7, ARG_NONE,   0, UNIM },
    { /* RCLALM */     docmd_xrom,        "RCLALM",              0x04, 0x00, 0xa6, 0xa2,  6, ARG_NONE,   0, UNIM },
    { /* SWPT */       docmd_xrom,        "SWPT",                0x04, 0x00, 0xa6, 0xa3,  4, ARG_NONE,   0, UNIM },

    /* Intel Decimal Floating-Point Math Library: self-test */
    { /* FPTEST */     docmd_fptest,      "FPT\305ST",           0x00, 0x00, 0xa7, 0xd2,  6, ARG_NONE,   0, NA_T },

    /* Programming */
    { /* LSTO */       docmd_lsto,        "LSTO",                0x00, 0xc7, 0xf2, 0xed,  4, ARG_NAMED,  1, ALLT },
    { /* SST_UP */     NULL,              "SST^",                0x40, 0x00, 0x00, 0x00,  4, ARG_NONE,   0, NA_T },
    { /* SST_RT */     NULL,              "SST\017",             0x40, 0x00, 0x00, 0x00,  4, ARG_NONE,   0, NA_T },
    { /* WSIZE */      docmd_wsize,       "WSIZE",               0x00, 0x00, 0xa7, 0xd3,  5, ARG_NONE,   1, 0x01 },
    { /* WSIZE_T */    docmd_wsize_t,     "WS\311Z\305?",        0x00, 0x00, 0xa7, 0xd4,  6, ARG_NONE,   0, NA_T },
    { /* YMD */        docmd_ymd,         "YMD",                 0x00, 0x00, 0xa7, 0xd5,  3, ARG_NONE,   0, NA_T },
    { /* BSIGNED */    docmd_bsigned,     "BS\311GN\305\304",    0x00, 0x00, 0xa7, 0xd6,  7, ARG_NONE,   0, NA_T },
    { /* BWRAP */      docmd_bwrap,       "BWR\301P",            0x00, 0x00, 0xa7, 0xd7,  5, ARG_NONE,   0, NA_T },
    { /* BRESET */     docmd_breset,      "BR\305S\305T",        0x00, 0x00, 0xa7, 0xd8,  6, ARG_NONE,   0, NA_T },
    { /* GETKEY1 */    docmd_getkey1,     "G\305TK\305\3311",    0x00, 0x00, 0xa7, 0xd9,  7, ARG_NONE,   0, NA_T },
    { /* LASTO */      docmd_lasto,       "LASTO",               0x00, 0xf5, 0xf2, 0xc8,  5, ARG_NAMED,  0, NA_T },

    /* Useful X-Fcn functions missing from the 42S */
    { /* ANUM */       docmd_anum,        "ANUM",                0x00, 0x00, 0xa6, 0x42,  4, ARG_NONE,   0, NA_T },
    { /* X<>F */       docmd_x_swap_f,    "X<>F",                0x00, 0x00, 0xa6, 0x6e,  4, ARG_NONE,   1, 0x01 },
    { /* RCLFLAG */    docmd_rclflag,     "RCLFLAG",             0x00, 0x00, 0xa6, 0x60,  7, ARG_NONE,   0, NA_T },
    { /* STOFLAG */    docmd_stoflag,     "STOFLAG",             0x00, 0x00, 0xa6, 0x6d,  7, ARG_NONE,  -1, 0x00 },
    
    /* User-defined functions */
    // Note: a7db-a7dd encode FUNC0-FUNC2, superseded by FUNC 00, FUNC 11, and
    // FUNC 21, and a7e0 encodes RTNERR without argument, and is superseded by
    // RTNERR IND ST X... So we don't need those three XROMs any more, but we
    // can't safely use them, so they should be left unassigned.
    { /* FUNC */       docmd_func,        "FUNC",                0x01, 0x00, 0xf2, 0xe0,  4, ARG_FUNC,   0, NA_T },
    { /* ERRMSG */     docmd_errmsg,      "ERRMSG",              0x00, 0x00, 0xa7, 0xf6,  6, ARG_NONE,   0, NA_T },
    { /* ERRNO */      docmd_errno,       "ERRNO",               0x00, 0x00, 0xa7, 0xf7,  5, ARG_NONE,   0, NA_T },
    { /* RTNYES */     docmd_rtnyes,      "RTNYES",              0x00, 0x00, 0xa7, 0xde,  6, ARG_NONE,   0, NA_T },
    { /* RTNNO */      docmd_rtnno,       "RTNNO",               0x00, 0x00, 0xa7, 0xdf,  5, ARG_NONE,   0, NA_T },
    { /* RTNERR */     docmd_rtnerr,      "RTNERR",              0x01, 0xdc, 0xf2, 0xa0,  6, ARG_NUM9,   0, NA_T },
    { /* STRACE */     docmd_strace,      "STRACE",              0x00, 0x00, 0xa7, 0xe1,  6, ARG_NONE,   0, NA_T },

    /* Big Stack */
    { /* 4STK */       docmd_4stk,        "4STK",                0x00, 0x00, 0xa7, 0xe2,  4, ARG_NONE,   0, NA_T },
    { /* L4STK */      docmd_l4stk,       "L4STK",               0x00, 0x00, 0xa7, 0xe3,  5, ARG_NONE,   0, NA_T },
    { /* NSTK */       docmd_nstk,        "NSTK",                0x00, 0x00, 0xa7, 0xe4,  4, ARG_NONE,   0, NA_T },
    { /* LNSTK */      docmd_lnstk,       "LNSTK",               0x00, 0x00, 0xa7, 0xe5,  5, ARG_NONE,   0, NA_T },
    { /* DEPTH */      docmd_depth,       "DEPTH",               0x00, 0x00, 0xa7, 0xe6,  5, ARG_NONE,   0, NA_T },
    { /* DROPN */      docmd_dropn,       "DR\317PN",            0x00, 0xf1, 0xf2, 0xa1,  5, ARG_NUM9,   0, NA_T },
    { /* DUP */        docmd_dup,         "DUP",                 0x00, 0x00, 0xa7, 0xe7,  3, ARG_NONE,   1, ALLT },
    { /* DUPN */       docmd_dupn,        "DUPN",                0x00, 0xf2, 0xf2, 0xa2,  4, ARG_NUM9,   0, NA_T },
    { /* PICK */       docmd_pick,        "PICK",                0x00, 0xf3, 0xf2, 0xa3,  4, ARG_NUM9,   0, NA_T },
    { /* UNPICK */     docmd_unpick,      "UNPICK",              0x00, 0xf4, 0xf2, 0xa4,  6, ARG_NUM9,   0, NA_T },
    { /* RDNN */       docmd_rdnn,        "R\016N",              0x00, 0xf6, 0xf2, 0xa5,  3, ARG_NUM9,   0, NA_T },
    { /* RUPN */       docmd_rupn,        "R^N",                 0x00, 0xf7, 0xf2, 0xa6,  3, ARG_NUM9,   0, NA_T },

    /* Miscellaneous */
    { /* NOP */        docmd_nop,         "NOP",                 0x00, 0x00, 0x00, 0xf0,  3, ARG_NONE,   0, NA_T },
    /* (Skipping 403 because of single-byte equality checks with CMD_END) */
    { /* DUMMY */      NULL,              "",                    0x44, 0x00, 0x00, 0x00,  0, ARG_OTHER,  0, UNIM },
    { /* FMA */        docmd_fma,         "FMA",                 0x00, 0x00, 0xa7, 0xda,  3, ARG_NONE,   3, 0x01 },
    { /* PGMMENU */    docmd_pgmmenu,     "P\307\315M\305NU",    0x00, 0x00, 0xa7, 0xe8,  7, ARG_NONE,   0, NA_T },
    { /* PMEXEC */     NULL,              "",                    0x44, 0x00, 0x00, 0x00,  0, ARG_OTHER,  0, NA_T },
    { /* PRMVAR */     docmd_prmvar,      "PRMVAR",              0x00, 0x40, 0xf2, 0x30,  6, ARG_PRGM,   0, NA_T },
    { /* VARMNU1 */    docmd_varmnu1,     "V\301\322MN\3251",    0x00, 0x42, 0xf2, 0x31,  7, ARG_PRGM,   0, NA_T },
    { /* X2LINE */     docmd_x2line,      "X2LINE",              0x42, 0x00, 0x00, 0x00,  6, ARG_NONE,   1, 0x13 },
    { /* A2LINE */     docmd_a2line,      "A2LINE",              0x42, 0x00, 0x00, 0x00,  6, ARG_NONE,   0, NA_T },
    { /* RCOMPLX */    docmd_rcomplx,     "RC\317\315P\314X",    0x00, 0x00, 0xa7, 0xf8,  7, ARG_NONE,   0, NA_T },
    { /* PCOMPLX */    docmd_pcomplx,     "PC\317\315P\314X",    0x00, 0x00, 0xa7, 0xf9,  7, ARG_NONE,   0, NA_T },

    /* String & List Functions */
    { /* XSTR */       docmd_xstr,        "XSTR",                0x20, 0x41, 0x00, 0x00,  4, ARG_XSTR,   0, NA_T },
    { /* XASTO */      docmd_xrom,        "XASTO",               0x00, 0x01, 0xf2, 0x11,  5, ARG_VAR,    0, UNIM },
    { /* LXASTO */     docmd_xrom,        "LXASTO",              0x00, 0x02, 0xf2, 0x12,  6, ARG_NAMED,  0, UNIM },
    { /* APPEND */     docmd_xrom,        "APPEND",              0x00, 0x00, 0xa7, 0xe9,  6, ARG_NONE,   0, UNIM },
    { /* EXTEND */     docmd_xrom,        "EXTEND",              0x00, 0x00, 0xa7, 0xea,  6, ARG_NONE,   0, UNIM },
    { /* SUBSTR */     docmd_xrom,        "SUBSTR",              0x00, 0x00, 0xa7, 0xeb,  6, ARG_NONE,   0, UNIM },
    { /* LENGTH */     docmd_xrom,        "LENGTH",              0x00, 0x00, 0xa7, 0xec,  6, ARG_NONE,   0, UNIM },
    { /* HEAD */       docmd_xrom,        "HEAD",                0x00, 0x03, 0xf2, 0x13,  4, ARG_VAR,    0, UNIM },
    { /* REV */        docmd_xrom,        "REV",                 0x00, 0x00, 0xa7, 0xed,  3, ARG_NONE,   0, UNIM },
    { /* POS */        docmd_xrom,        "POS",                 0x00, 0x00, 0xa7, 0xee,  3, ARG_NONE,   0, UNIM },
    { /* S_TO_N */     docmd_xrom,        "S\017N",              0x00, 0x00, 0xa7, 0xef,  3, ARG_NONE,   0, UNIM },
    { /* N_TO_S */     docmd_xrom,        "N\017S",              0x00, 0x00, 0xa7, 0xf0,  3, ARG_NONE,   0, UNIM },
    { /* C_TO_N */     docmd_xrom,        "C\017N",              0x00, 0x00, 0xa7, 0xf1,  3, ARG_NONE,   0, UNIM },
    { /* N_TO_C */     docmd_xrom,        "N\017C",              0x00, 0x00, 0xa7, 0xf2,  3, ARG_NONE,   0, UNIM },
    { /* LIST_T */     docmd_xrom,        "LIST?",               0x00, 0x00, 0xa7, 0xf3,  5, ARG_NONE,   0, UNIM },
    { /* NEWLIST */    docmd_xrom,        "NEWLIST",             0x00, 0x00, 0xa7, 0xf4,  7, ARG_NONE,   0, UNIM },
    { /* NEWSTR */     docmd_xrom,        "NEWSTR",              0x00, 0x00, 0xa7, 0xf5,  6, ARG_NONE,   0, UNIM },

    /* Generalized Comparisons */
    { /* X_EQ_NN */    docmd_x_eq_nn,     "X=?",                 0x00, 0x04, 0xf2, 0x14,  3, ARG_VAR,    1, ALLT },
    { /* X_NE_NN */    docmd_x_ne_nn,     "X\014?",              0x00, 0x05, 0xf2, 0x15,  3, ARG_VAR,    1, ALLT },
    { /* X_LT_NN */    docmd_x_lt_nn,     "X<?",                 0x00, 0x06, 0xf2, 0x16,  3, ARG_VAR,    1, 0x01 },
    { /* X_GT_NN */    docmd_x_gt_nn,     "X>?",                 0x00, 0x07, 0xf2, 0x17,  3, ARG_VAR,    1, 0x01 },
    { /* X_LE_NN */    docmd_x_le_nn,     "X\011?",              0x00, 0x20, 0xf2, 0x18,  3, ARG_VAR,    1, 0x01 },
    { /* X_GE_NN */    docmd_x_ge_nn,     "X\013?",              0x00, 0x21, 0xf2, 0x19,  3, ARG_VAR,    1, 0x01 },
    { /* 0_EQ_NN */    docmd_0_eq_nn,     "0=?",                 0x00, 0x22, 0xf2, 0x1a,  3, ARG_VAR,    0, NA_T },
    { /* 0_NE_NN */    docmd_0_ne_nn,     "0\014?",              0x00, 0x23, 0xf2, 0x1b,  3, ARG_VAR,    0, NA_T },
    { /* 0_LT_NN */    docmd_0_lt_nn,     "0<?",                 0x00, 0x24, 0xf2, 0x1c,  3, ARG_VAR,    0, NA_T },
    { /* 0_GT_NN */    docmd_0_gt_nn,     "0>?",                 0x00, 0x25, 0xf2, 0x1d,  3, ARG_VAR,    0, NA_T },
    { /* 0_LE_NN */    docmd_0_le_nn,     "0\011?",              0x00, 0x26, 0xf2, 0x1e,  3, ARG_VAR,    0, NA_T },
    { /* 0_GE_NN */    docmd_0_ge_nn,     "0\013?",              0x00, 0x27, 0xf2, 0x1f,  3, ARG_VAR,    0, NA_T }
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

int handle(int cmd, arg_struct *arg) {
    const command_spec *cs = cmd_array + cmd;
    if (flags.f.big_stack) {
        if (cs->argcount == -1) {
            if (sp == -1 || sp == 0 && stack[sp]->type != TYPE_COMPLEX && stack[sp]->type != TYPE_COMPLEXMATRIX)
                return ERR_TOO_FEW_ARGUMENTS;
        } else {
            if (sp + 1 < cs->argcount)
                return ERR_TOO_FEW_ARGUMENTS;
        }
    }
    if (cs->argcount > 0 && cs->rttypes != ALLT) {
        // Note that we don't handle argcount = -1 here,
        // that is left up to the functions that use it themselves.
        int argcount = cs->argcount;
        int rttypes = cs->rttypes;
        for (int i = 0; i < argcount; i++) {
            int type = 1 << (stack[sp - i]->type - 1);
            if ((type & rttypes) == 0)
                if (type == 1 << (TYPE_STRING - 1))
                    return ERR_ALPHA_DATA_IS_INVALID;
                else
                    return ERR_INVALID_TYPE;
        }
    }
    return cs->handler(arg);
}
