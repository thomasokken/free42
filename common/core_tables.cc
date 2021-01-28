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


const command_spec cmd_array[] =
{
    { /* CLX */         "CLX",                  3, ARG_NONE,   1, FLAG_NONE,                   0x00000077, docmd_clx         },
    { /* ENTER */       "ENT\305R",             5, ARG_NONE,   1, FLAG_NONE,                   0x00000083, docmd_enter       },
    { /* SWAP */        "X<>Y",                 4, ARG_NONE,   2, FLAG_NONE,                   0x00000071, docmd_swap        },
    { /* RDN */         "R\016",                2, ARG_NONE,   0, FLAG_NONE,                   0x00000075, docmd_rdn         },
    { /* CHS */         "+/-",                  3, ARG_NONE,   1, FLAG_NONE,                   0x00000054, docmd_chs         },
    { /* DIV */         "\000",                 1, ARG_NONE,   2, FLAG_NONE,                   0x00000043, docmd_div         },
    { /* MUL */         "\001",                 1, ARG_NONE,   2, FLAG_NONE,                   0x00000042, docmd_mul         },
    { /* SUB */         "-",                    1, ARG_NONE,   2, FLAG_NONE,                   0x00000041, docmd_sub         },
    { /* ADD */         "+",                    1, ARG_NONE,   2, FLAG_NONE,                   0x00000040, docmd_add         },
    { /* LASTX */       "LASTX",                5, ARG_NONE,   0, FLAG_NONE,                   0x00000076, docmd_lastx       },
    { /* SILENT_OFF */  "",                     0, ARG_NONE,   0, FLAG_HIDDEN + FLAG_NO_SHOW,  0x02000000, NULL,             },
    { /* SILENT_ON */   "",                     0, ARG_NONE,   0, FLAG_HIDDEN + FLAG_NO_SHOW,  0x02000000, NULL,             },
    { /* SIN */         "SIN",                  3, ARG_NONE,   1, FLAG_NONE,                   0x00000059, docmd_sin         },
    { /* COS */         "COS",                  3, ARG_NONE,   1, FLAG_NONE,                   0x0000005a, docmd_cos         },
    { /* TAN */         "TAN",                  3, ARG_NONE,   1, FLAG_NONE,                   0x0000005b, docmd_tan         },
    { /* ASIN */        "ASIN",                 4, ARG_NONE,   1, FLAG_NONE,                   0x0000005c, docmd_asin        },
    { /* ACOS */        "ACOS",                 4, ARG_NONE,   1, FLAG_NONE,                   0x0000005d, docmd_acos        },
    { /* ATAN */        "ATAN",                 4, ARG_NONE,   1, FLAG_NONE,                   0x0000005e, docmd_atan        },
    { /* LOG */         "LOG",                  3, ARG_NONE,   1, FLAG_NONE,                   0x00000056, docmd_log         },
    { /* 10_POW_X */    "10^X",                 4, ARG_NONE,   1, FLAG_NONE,                   0x00000057, docmd_10_pow_x    },
    { /* LN */          "LN",                   2, ARG_NONE,   1, FLAG_NONE,                   0x00000050, docmd_ln          },
    { /* E_POW_X */     "E^X",                  3, ARG_NONE,   1, FLAG_NONE,                   0x00000055, docmd_e_pow_x     },
    { /* SQRT */        "SQRT",                 4, ARG_NONE,   1, FLAG_NONE,                   0x00000052, docmd_sqrt        },
    { /* SQUARE */      "X^2",                  3, ARG_NONE,   1, FLAG_NONE,                   0x00000051, docmd_square      },
    { /* INV */         "1/X",                  3, ARG_NONE,   1, FLAG_NONE,                   0x00000060, docmd_inv         },
    { /* Y_POW_X */     "Y^X",                  3, ARG_NONE,   2, FLAG_NONE,                   0x00000053, docmd_y_pow_x     },
    { /* PERCENT */     "%",                    1, ARG_NONE,   2, FLAG_NONE,                   0x0000004c, docmd_percent     },
    { /* PI */          "PI",                   2, ARG_NONE,   0, FLAG_NONE,                   0x00000072, docmd_pi          },
    { /* COMPLEX */     "C\317\315PL\305X",     7, ARG_NONE,  -1, FLAG_NONE,                   0x0000a072, docmd_complex     },
    { /* STO */         "STO",                  3, ARG_VAR,    1, FLAG_NONE,                   0x01810091, docmd_sto         },
    { /* STO_DIV */     "STO\000",              4, ARG_VAR,    1, FLAG_NONE,                   0x00850095, docmd_sto_div     },
    { /* STO_MUL */     "STO\001",              4, ARG_VAR,    1, FLAG_NONE,                   0x00840094, docmd_sto_mul     },
    { /* STO_SUB */     "STO-",                 4, ARG_VAR,    1, FLAG_NONE,                   0x00830093, docmd_sto_sub     },
    { /* STO_ADD */     "STO+",                 4, ARG_VAR,    1, FLAG_NONE,                   0x00820092, docmd_sto_add     },
    { /* RCL */         "RCL",                  3, ARG_VAR,    0, FLAG_NONE,                   0x01910090, docmd_rcl         },
    { /* RCL_DIV */     "RCL\000",              4, ARG_VAR,    1, FLAG_NONE,                   0x0095f2d4, docmd_rcl_div     },
    { /* RCL_MUL */     "RCL\001",              4, ARG_VAR,    1, FLAG_NONE,                   0x0094f2d3, docmd_rcl_mul     },
    { /* RCL_SUB */     "RCL-",                 4, ARG_VAR,    1, FLAG_NONE,                   0x0093f2d2, docmd_rcl_sub     },
    { /* RCL_ADD */     "RCL+",                 4, ARG_VAR,    1, FLAG_NONE,                   0x0092f2d1, docmd_rcl_add     },
    { /* FIX */         "FIX",                  3, ARG_NUM11,  0, FLAG_NONE,                   0x01d4009c, docmd_fix         },
    { /* SCI */         "SCI",                  3, ARG_NUM11,  0, FLAG_NONE,                   0x01d5009d, docmd_sci         },
    { /* ENG */         "ENG",                  3, ARG_NUM11,  0, FLAG_NONE,                   0x01d6009e, docmd_eng         },
    { /* ALL */         "ALL",                  3, ARG_NONE,   0, FLAG_NONE,                   0x0000a25d, docmd_all         },
    { /* NULL */        "\316\325\314\314",     4, ARG_NONE,   0, FLAG_HIDDEN,                 0x02000000, docmd_null        },
    { /* ASTO */        "ASTO",                 4, ARG_VAR,    0, FLAG_NONE,                   0x00b2009a, docmd_asto        },
    { /* ARCL */        "ARCL",                 4, ARG_VAR,    0, FLAG_NONE,                   0x00b3009b, docmd_arcl        },
    { /* CLA */         "CLA",                  3, ARG_NONE,   0, FLAG_NONE,                   0x00000087, docmd_cla         },
    { /* DEG */         "DEG",                  3, ARG_NONE,   0, FLAG_NONE,                   0x00000080, docmd_deg         },
    { /* RAD */         "RAD",                  3, ARG_NONE,   0, FLAG_NONE,                   0x00000081, docmd_rad         },
    { /* GRAD */        "GRAD",                 4, ARG_NONE,   0, FLAG_NONE,                   0x00000082, docmd_grad        },
    { /* RECT */        "RECT",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a25a, docmd_rect        },
    { /* POLAR */       "POLAR",                5, ARG_NONE,   0, FLAG_NONE,                   0x0000a259, docmd_polar       },
    { /* SIZE */        "SIZE",                 4, ARG_COUNT,  0, FLAG_NONE,                   0x01000000, docmd_size        },
    { /* QUIET */       "QUIET",                5, ARG_NONE,   0, FLAG_IMMED,                  0x02000000, docmd_quiet       },
    { /* CPXRES */      "C\320\330RES",         6, ARG_NONE,   0, FLAG_NONE,                   0x0000a26a, docmd_cpxres      },
    { /* REALRES */     "R\305\301\314RES",     7, ARG_NONE,   0, FLAG_NONE,                   0x0000a26b, docmd_realres     },
    { /* KEYASN */      "KEY\301\323\316",      6, ARG_NONE,   0, FLAG_NONE,                   0x0000a263, docmd_keyasn      },
    { /* LCLBL */       "LCLBL",                5, ARG_NONE,   0, FLAG_NONE,                   0x0000a264, docmd_lclbl       },
    { /* RDXDOT */      "RDX.",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a25b, docmd_rdxdot      },
    { /* RDXCOMMA */    "RDX,",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a25c, docmd_rdxcomma    },
    { /* CLSIGMA */     "CL\005",               3, ARG_NONE,   0, FLAG_NONE,                   0x00000070, docmd_clsigma     },
    { /* CLP */         "CLP",                  3, ARG_PRGM,   0, FLAG_NONE,                   0x00f00000, docmd_clp         },
    { /* CLV */         "CLV",                  3, ARG_NAMED,  0, FLAG_NONE,                   0x00b0f2d8, docmd_clv         },
    { /* CLST */        "CLST",                 4, ARG_NONE,   0, FLAG_NONE,                   0x00000073, docmd_clst        },
    { /* CLRG */        "CLRG",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000008a, docmd_clrg        },
    { /* DEL */         "DEL",                  3, ARG_COUNT,  0, FLAG_PRGM_ONLY + FLAG_IMMED, 0x02000000, docmd_del         },
    { /* CLKEYS */      "CLK\305Y\323",         6, ARG_NONE,   0, FLAG_NONE,                   0x0000a262, docmd_clkeys      },
    { /* CLLCD */       "CLLCD",                5, ARG_NONE,   0, FLAG_NONE,                   0x0000a763, docmd_cllcd       },
    { /* CLMENU */      "CLM\305N\325",         6, ARG_NONE,   0, FLAG_NONE,                   0x0000a26d, docmd_clmenu      },
    { /* CLALLa */      "CLALL",                5, ARG_NONE,   0, FLAG_IMMED,                  0x02000000, NULL,             },
    { /* TO_DEG */      "\017DEG",              4, ARG_NONE,   1, FLAG_NONE,                   0x0000006b, docmd_to_deg      },
    { /* TO_RAD */      "\017RAD",              4, ARG_NONE,   1, FLAG_NONE,                   0x0000006a, docmd_to_rad      },
    { /* TO_HR */       "\017HR",               3, ARG_NONE,   1, FLAG_NONE,                   0x0000006d, docmd_to_hr       },
    { /* TO_HMS */      "\017HMS",              4, ARG_NONE,   1, FLAG_NONE,                   0x0000006c, docmd_to_hms      },
    { /* TO_REC */      "\017REC",              4, ARG_NONE,  -1, FLAG_NONE,                   0x0000004e, docmd_to_rec      },
    { /* TO_POL */      "\017POL",              4, ARG_NONE,  -1, FLAG_NONE,                   0x0000004f, docmd_to_pol      },
    { /* IP */          "IP",                   2, ARG_NONE,   1, FLAG_NONE,                   0x00000068, docmd_ip          },
    { /* FP */          "FP",                   2, ARG_NONE,   1, FLAG_NONE,                   0x00000069, docmd_fp          },
    { /* RND */         "RND",                  3, ARG_NONE,   1, FLAG_NONE,                   0x0000006e, docmd_rnd         },
    { /* ABS */         "ABS",                  3, ARG_NONE,   1, FLAG_NONE,                   0x00000061, docmd_abs         },
    { /* SIGN */        "SIGN",                 4, ARG_NONE,   1, FLAG_NONE,                   0x0000007a, docmd_sign        },
    { /* MOD */         "MOD",                  3, ARG_NONE,   2, FLAG_NONE,                   0x0000004b, docmd_mod         },
    { /* SF */          "SF",                   2, ARG_NUM99,  0, FLAG_NONE,                   0x00a000a8, docmd_sf          },
    { /* CF */          "CF",                   2, ARG_NUM99,  0, FLAG_NONE,                   0x00a100a9, docmd_cf          },
    { /* FS_T */        "FS?",                  3, ARG_NUM99,  0, FLAG_NONE,                   0x00a400ac, docmd_fs_t        },
    { /* FC_T */        "FC?",                  3, ARG_NUM99,  0, FLAG_NONE,                   0x00a500ad, docmd_fc_t        },
    { /* FSC_T */       "FS?C",                 4, ARG_NUM99,  0, FLAG_NONE,                   0x00a200aa, docmd_fsc_t       },
    { /* FCC_T */       "FC?C",                 4, ARG_NUM99,  0, FLAG_NONE,                   0x00a300ab, docmd_fcc_t       },
    { /* COMB */        "COMB",                 4, ARG_NONE,   2, FLAG_NONE,                   0x0000a06f, docmd_comb        },
    { /* PERM */        "PERM",                 4, ARG_NONE,   2, FLAG_NONE,                   0x0000a070, docmd_perm        },
    { /* FACT */        "N!",                   2, ARG_NONE,   1, FLAG_NONE,                   0x00000062, docmd_fact        },
    { /* GAMMA */       "GAM\315\301",          5, ARG_NONE,   1, FLAG_NONE,                   0x0000a074, docmd_gamma       },
    { /* RAN */         "RAN",                  3, ARG_NONE,   0, FLAG_NONE,                   0x0000a071, docmd_ran         },
    { /* SEED */        "SEED",                 4, ARG_NONE,   1, FLAG_NONE,                   0x0000a073, docmd_seed        },
    { /* LBL */         "LBL",                  3, ARG_OTHER,  0, FLAG_PRGM_ONLY,              0x010000cf, docmd_lbl         },
    { /* RTN */         "RTN",                  3, ARG_NONE,   0, FLAG_NONE,                   0x00000085, docmd_rtn         },
    { /* INPUT */       "INPUT",                5, ARG_VAR,    0, FLAG_PRGM_ONLY,              0x01c5f2d0, docmd_input       },
    { /* VIEW */        "VIEW",                 4, ARG_VAR,    0, FLAG_NONE,                   0x00800098, docmd_view        },
    { /* AVIEW */       "AVIEW",                5, ARG_NONE,   0, FLAG_NONE,                   0x0000007e, docmd_aview       },
    { /* XEQ */         "XEQ",                  3, ARG_LBL,    0, FLAG_NONE,                   0x01a700ae, docmd_xeq         },
    { /* PROMPT */      "PROM\320\324",         6, ARG_NONE,   0, FLAG_NONE,                   0x0000008e, docmd_prompt      },
    { /* PSE */         "PSE",                  3, ARG_NONE,   0, FLAG_NONE,                   0x00000089, docmd_pse         },
    { /* ISG */         "ISG",                  3, ARG_REAL,   0, FLAG_NONE,                   0x00960096, docmd_isg         },
    { /* DSE */         "DSE",                  3, ARG_REAL,   0, FLAG_NONE,                   0x00970097, docmd_dse         },
    { /* AIP */         "AIP",                  3, ARG_NONE,   1, FLAG_NONE,                   0x0000a631, docmd_aip         },
    { /* XTOA */        "XTOA",                 4, ARG_NONE,   1, FLAG_NONE,                   0x0000a66f, docmd_xtoa        },
    { /* AGRAPH */      "AGRA\320\310",         6, ARG_NONE,  -1, FLAG_NONE,                   0x0000a764, docmd_agraph      },
    { /* PIXEL */       "PIXEL",                5, ARG_NONE,  -1, FLAG_NONE,                   0x0000a765, docmd_pixel       },
    { /* BEEP */        "BEEP",                 4, ARG_NONE,   0, FLAG_NONE,                   0x00000086, docmd_beep        },
    { /* TONE */        "TONE",                 4, ARG_NUM9,   0, FLAG_NONE,                   0x00d7009f, docmd_tone        },
    { /* MVAR */        "MVAR",                 4, ARG_RVAR,   0, FLAG_NONE,                   0x00900000, docmd_mvar        },
    { /* VARMENU */     "VARM\305\316\325",     7, ARG_PRGM,   0, FLAG_NONE,                   0x00c1f2f8, docmd_varmenu     },
    { /* GETKEY */      "GETK\305\331",         6, ARG_NONE,   0, FLAG_NONE,                   0x0000a26e, docmd_getkey      },
    { /* MENU */        "MENU",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a25e, docmd_menu        },
    { /* KEYG */        "KEYG",                 4, ARG_MKEY,   0, FLAG_NONE,                   0x02000000, NULL,             },
    { /* KEYX */        "KEYX",                 4, ARG_MKEY,   0, FLAG_NONE,                   0x02000000, NULL,             },
    { /* X_EQ_0 */      "X=0?",                 4, ARG_NONE,   1, FLAG_NONE,                   0x00000067, docmd_x_eq_0      },
    { /* X_NE_0 */      "X\0140?",              4, ARG_NONE,   1, FLAG_NONE,                   0x00000063, docmd_x_ne_0      },
    { /* X_LT_0 */      "X<0?",                 4, ARG_NONE,   1, FLAG_NONE,                   0x00000066, docmd_x_lt_0      },
    { /* X_GT_0 */      "X>0?",                 4, ARG_NONE,   1, FLAG_NONE,                   0x00000064, docmd_x_gt_0      },
    { /* X_LE_0 */      "X\0110?",              4, ARG_NONE,   1, FLAG_NONE,                   0x0000007b, docmd_x_le_0      },
    { /* X_GE_0 */      "X\0130?",              4, ARG_NONE,   1, FLAG_NONE,                   0x0000a25f, docmd_x_ge_0      },
    { /* X_EQ_Y */      "X=Y?",                 4, ARG_NONE,   2, FLAG_NONE,                   0x00000078, docmd_x_eq_y      },
    { /* X_NE_Y */      "X\014Y?",              4, ARG_NONE,   2, FLAG_NONE,                   0x00000079, docmd_x_ne_y      },
    { /* X_LT_Y */      "X<Y?",                 4, ARG_NONE,   2, FLAG_NONE,                   0x00000044, docmd_x_lt_y      },
    { /* X_GT_Y */      "X>Y?",                 4, ARG_NONE,   2, FLAG_NONE,                   0x00000045, docmd_x_gt_y      },
    { /* X_LE_Y */      "X\011Y?",              4, ARG_NONE,   2, FLAG_NONE,                   0x00000046, docmd_x_le_y      },
    { /* X_GE_Y */      "X\013Y?",              4, ARG_NONE,   2, FLAG_NONE,                   0x0000a260, docmd_x_ge_y      },
    { /* PRSIGMA */     "PR\005",               3, ARG_NONE,   0, FLAG_NONE,                   0x0000a752, docmd_prsigma     },
    { /* PRP */         "PRP",                  3, ARG_PRGM,   0, FLAG_IMMED,                  0x02000000, docmd_prp         },
    { /* PRV */         "PRV",                  3, ARG_NAMED,  0, FLAG_NONE,                   0x00b1f2d9, docmd_prv         },
    { /* PRSTK */       "PRST\313",             5, ARG_NONE,   0, FLAG_NONE,                   0x0000a753, docmd_prstk       },
    { /* PRA */         "PRA",                  3, ARG_NONE,   0, FLAG_NONE,                   0x0000a748, docmd_pra         },
    { /* PRX */         "PRX",                  3, ARG_NONE,   1, FLAG_NONE,                   0x0000a754, docmd_prx         },
    { /* PRUSR */       "PRUSR",                5, ARG_NONE,   0, FLAG_NONE,                   0x0000a761, docmd_prusr       },
    { /* LIST */        "LIST",                 4, ARG_COUNT,  0, FLAG_IMMED,                  0x02000000, docmd_list        },
    { /* ADV */         "ADV",                  3, ARG_NONE,   0, FLAG_NONE,                   0x0000008f, docmd_adv         },
    { /* PRLCD */       "PRLCD",                5, ARG_NONE,   0, FLAG_NONE,                   0x0000a762, docmd_prlcd       },
    { /* DELAY */       "DELAY",                5, ARG_NONE,   1, FLAG_NONE,                   0x0000a760, docmd_delay       },
    { /* PON */         "P\322ON",              4, ARG_NONE,   0, FLAG_NONE,                   0x0000a75e, docmd_pon         },
    { /* POFF */        "P\322OFF",             5, ARG_NONE,   0, FLAG_NONE,                   0x0000a75f, docmd_poff        },
    { /* MAN */         "MAN",                  3, ARG_NONE,   0, FLAG_NONE,                   0x0000a75b, docmd_man         },
    { /* NORM */        "NORM",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a75c, docmd_norm        },
    { /* TRACE */       "TRACE",                5, ARG_NONE,   0, FLAG_NONE,                   0x0000a75d, docmd_trace       },
    { /* SIGMAADD */    "\005+",                2, ARG_NONE,   2, FLAG_NONE,                   0x00000047, docmd_sigmaadd    },
    { /* SIGMASUB */    "\005-",                2, ARG_NONE,   2, FLAG_NONE,                   0x00000048, docmd_sigmasub    },
    { /* GTO */         "GTO",                  3, ARG_LBL,    0, FLAG_NONE,                   0x01a60000, docmd_gto         },
    { /* END */         "END",                  3, ARG_NONE,   0, FLAG_NONE,                   0x01000000, docmd_rtn         },
    { /* NUMBER */      "",                     0, ARG_NONE,   0, FLAG_HIDDEN,                 0x01000000, docmd_number      },
    { /* STRING */      "",                     0, ARG_NONE,   0, FLAG_HIDDEN,                 0x01000000, docmd_string      },
    { /* RUN */         "RUN",                  3, ARG_NONE,   0, FLAG_HIDDEN,                 0x02000000, NULL,             },
    { /* SST */         "SST",                  3, ARG_NONE,   0, FLAG_NONE,                   0x02000000, NULL,             },
    { /* GTODOT */      "GTO .",                5, ARG_OTHER,  0, FLAG_IMMED,                  0x02000000, docmd_gtodot      },
    { /* GTODOTDOT */   "GTO ..",               6, ARG_NONE,   0, FLAG_IMMED,                  0x02000000, docmd_gtodotdot   },
    { /* STOP */        "STOP",                 4, ARG_NONE,   0, FLAG_NONE,                   0x00000084, docmd_stop        },
    { /* NEWMAT */      "NEW\315\301\324",      6, ARG_NONE,   2, FLAG_NONE,                   0x0000a6da, docmd_newmat      },
    { /* RUP */         "R^",                   2, ARG_NONE,   0, FLAG_NONE,                   0x00000074, docmd_rup         },
    { /* REAL_T */      "RE\301L?",             5, ARG_NONE,   1, FLAG_NONE,                   0x0000a265, docmd_real_t      },
    { /* CPX_T */       "CPX?",                 4, ARG_NONE,   1, FLAG_NONE,                   0x0000a267, docmd_cpx_t       },
    { /* STR_T */       "STR?",                 4, ARG_NONE,   1, FLAG_NONE,                   0x0000a268, docmd_str_t       },
    { /* MAT_T */       "MAT?",                 4, ARG_NONE,   1, FLAG_NONE,                   0x0000a266, docmd_mat_t       },
    { /* DIM_T */       "DIM?",                 4, ARG_NONE,   1, FLAG_NONE,                   0x0000a6e7, docmd_dim_t       },
    { /* ASSIGNa */     "AS\323\311GN",         6, ARG_NAMED,  0, FLAG_NONE,                   0x02000000, NULL,             },
    { /* ASSIGNb */     "",                     0, ARG_CKEY,   0, FLAG_HIDDEN,                 0x02000000, NULL,             },
    { /* ASGN01 */      "",                     0, ARG_OTHER,  0, FLAG_HIDDEN,                 0x01000000, docmd_asgn01      },
    { /* ASGN02 */      "",                     0, ARG_OTHER,  0, FLAG_HIDDEN,                 0x01000000, docmd_asgn02      },
    { /* ASGN03 */      "",                     0, ARG_OTHER,  0, FLAG_HIDDEN,                 0x01000000, docmd_asgn03      },
    { /* ASGN04 */      "",                     0, ARG_OTHER,  0, FLAG_HIDDEN,                 0x01000000, docmd_asgn04      },
    { /* ASGN05 */      "",                     0, ARG_OTHER,  0, FLAG_HIDDEN,                 0x01000000, docmd_asgn05      },
    { /* ASGN06 */      "",                     0, ARG_OTHER,  0, FLAG_HIDDEN,                 0x01000000, docmd_asgn06      },
    { /* ASGN07 */      "",                     0, ARG_OTHER,  0, FLAG_HIDDEN,                 0x01000000, docmd_asgn07      },
    { /* ASGN08 */      "",                     0, ARG_OTHER,  0, FLAG_HIDDEN,                 0x01000000, docmd_asgn08      },
    { /* ASGN09 */      "",                     0, ARG_OTHER,  0, FLAG_HIDDEN,                 0x01000000, docmd_asgn09      },
    { /* ASGN10 */      "",                     0, ARG_OTHER,  0, FLAG_HIDDEN,                 0x01000000, docmd_asgn10      },
    { /* ASGN11 */      "",                     0, ARG_OTHER,  0, FLAG_HIDDEN,                 0x01000000, docmd_asgn11      },
    { /* ASGN12 */      "",                     0, ARG_OTHER,  0, FLAG_HIDDEN,                 0x01000000, docmd_asgn12      },
    { /* ASGN13 */      "",                     0, ARG_OTHER,  0, FLAG_HIDDEN,                 0x01000000, docmd_asgn13      },
    { /* ASGN14 */      "",                     0, ARG_OTHER,  0, FLAG_HIDDEN,                 0x01000000, docmd_asgn14      },
    { /* ASGN15 */      "",                     0, ARG_OTHER,  0, FLAG_HIDDEN,                 0x01000000, docmd_asgn15      },
    { /* ASGN16 */      "",                     0, ARG_OTHER,  0, FLAG_HIDDEN,                 0x01000000, docmd_asgn16      },
    { /* ASGN17 */      "",                     0, ARG_OTHER,  0, FLAG_HIDDEN,                 0x01000000, docmd_asgn17      },
    { /* ASGN18 */      "",                     0, ARG_OTHER,  0, FLAG_HIDDEN,                 0x01000000, docmd_asgn18      },
    { /* ON */          "ON",                   2, ARG_NONE,   0, FLAG_NONE,                   0x0000a270, docmd_on          },
    { /* OFF */         "OFF",                  3, ARG_NONE,   0, FLAG_NONE,                   0x0000008d, docmd_off         },
    { /* KEY1G */       "KEY 1 GTO",            9, ARG_LBL,    0, FLAG_HIDDEN,                 0x01000000, docmd_key1g       },
    { /* KEY2G */       "KEY 2 GTO",            9, ARG_LBL,    0, FLAG_HIDDEN,                 0x01000000, docmd_key2g       },
    { /* KEY3G */       "KEY 3 GTO",            9, ARG_LBL,    0, FLAG_HIDDEN,                 0x01000000, docmd_key3g       },
    { /* KEY4G */       "KEY 4 GTO",            9, ARG_LBL,    0, FLAG_HIDDEN,                 0x01000000, docmd_key4g       },
    { /* KEY5G */       "KEY 5 GTO",            9, ARG_LBL,    0, FLAG_HIDDEN,                 0x01000000, docmd_key5g       },
    { /* KEY6G */       "KEY 6 GTO",            9, ARG_LBL,    0, FLAG_HIDDEN,                 0x01000000, docmd_key6g       },
    { /* KEY7G */       "KEY 7 GTO",            9, ARG_LBL,    0, FLAG_HIDDEN,                 0x01000000, docmd_key7g       },
    { /* KEY8G */       "KEY 8 GTO",            9, ARG_LBL,    0, FLAG_HIDDEN,                 0x01000000, docmd_key8g       },
    { /* KEY9G */       "KEY 9 GTO",            9, ARG_LBL,    0, FLAG_HIDDEN,                 0x01000000, docmd_key9g       },
    { /* KEY1X */       "KEY 1 XEQ",            9, ARG_LBL,    0, FLAG_HIDDEN,                 0x01000000, docmd_key1x       },
    { /* KEY2X */       "KEY 2 XEQ",            9, ARG_LBL,    0, FLAG_HIDDEN,                 0x01000000, docmd_key2x       },
    { /* KEY3X */       "KEY 3 XEQ",            9, ARG_LBL,    0, FLAG_HIDDEN,                 0x01000000, docmd_key3x       },
    { /* KEY4X */       "KEY 4 XEQ",            9, ARG_LBL,    0, FLAG_HIDDEN,                 0x01000000, docmd_key4x       },
    { /* KEY5X */       "KEY 5 XEQ",            9, ARG_LBL,    0, FLAG_HIDDEN,                 0x01000000, docmd_key5x       },
    { /* KEY6X */       "KEY 6 XEQ",            9, ARG_LBL,    0, FLAG_HIDDEN,                 0x01000000, docmd_key6x       },
    { /* KEY7X */       "KEY 7 XEQ",            9, ARG_LBL,    0, FLAG_HIDDEN,                 0x01000000, docmd_key7x       },
    { /* KEY8X */       "KEY 8 XEQ",            9, ARG_LBL,    0, FLAG_HIDDEN,                 0x01000000, docmd_key8x       },
    { /* KEY9X */       "KEY 9 XEQ",            9, ARG_LBL,    0, FLAG_HIDDEN,                 0x01000000, docmd_key9x       },
    { /* VMEXEC */      "",                     0, ARG_OTHER,  0, FLAG_HIDDEN,                 0x02000000, NULL,             },
    { /* VMSTO */       "STO",                  3, ARG_OTHER,  1, FLAG_HIDDEN,                 0x02000000, docmd_vmsto       },
    { /* SIGMAREG */    "\005REG",              4, ARG_NUM99,  0, FLAG_NONE,                   0x00d30099, docmd_sigma_reg   },
    { /* SIGMAREG_T */  "\005R\305G?",          5, ARG_NONE,   0, FLAG_NONE,                   0x0000a678, docmd_sigma_reg_t },
    { /* CLD */         "CLD",                  3, ARG_NONE,   0, FLAG_NONE,                   0x0000007f, docmd_cld         },
    { /* ACOSH */       "ACOSH",                5, ARG_NONE,   1, FLAG_NONE,                   0x0000a066, docmd_acosh       },
    { /* ALENG */       "ALEN\307",             5, ARG_NONE,   0, FLAG_NONE,                   0x0000a641, docmd_aleng       },
    { /* ALLSIGMA */    "ALL\005",              4, ARG_NONE,   0, FLAG_NONE,                   0x0000a0ae, docmd_allsigma    },
    { /* AND */         "AND",                  3, ARG_NONE,   2, FLAG_NONE,                   0x0000a588, docmd_and         },
    { /* AOFF */        "AOFF",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000008b, docmd_aoff        },
    { /* AON */         "AON",                  3, ARG_NONE,   0, FLAG_NONE,                   0x0000008c, docmd_aon         },
    { /* AROT */        "AROT",                 4, ARG_NONE,   1, FLAG_NONE,                   0x0000a646, docmd_arot        },
    { /* ASHF */        "ASHF",                 4, ARG_NONE,   0, FLAG_NONE,                   0x00000088, docmd_ashf        },
    { /* ASINH */       "ASINH",                5, ARG_NONE,   1, FLAG_NONE,                   0x0000a064, docmd_asinh       },
    { /* ATANH */       "AT\301NH",             5, ARG_NONE,   1, FLAG_NONE,                   0x0000a065, docmd_atanh       },
    { /* ATOX */        "ATOX",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a647, docmd_atox        },
    { /* BASEADD */     "BASE+",                5, ARG_NONE,   2, FLAG_NONE,                   0x0000a0e6, docmd_baseadd     },
    { /* BASESUB */     "BASE-",                5, ARG_NONE,   2, FLAG_NONE,                   0x0000a0e7, docmd_basesub     },
    { /* BASEMUL */     "BASE\001",             5, ARG_NONE,   2, FLAG_NONE,                   0x0000a0e8, docmd_basemul     },
    { /* BASEDIV */     "BASE\000",             5, ARG_NONE,   2, FLAG_NONE,                   0x0000a0e9, docmd_basediv     },
    { /* BASECHS */     "B\301\323\305+/-",     7, ARG_NONE,   1, FLAG_NONE,                   0x0000a0ea, docmd_basechs     },
    { /* BEST */        "BEST",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a09f, docmd_best        },
    { /* BINM */        "BINM",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a0e5, docmd_binm        },
    { /* BIT_T */       "BIT?",                 4, ARG_NONE,   2, FLAG_NONE,                   0x0000a58c, docmd_bit_t       },
    { /* BST */         "BST",                  3, ARG_NONE,   0, FLAG_NONE,                   0x02000000, NULL,             },
    { /* CORR */        "CORR",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a0a7, docmd_corr        },
    { /* COSH */        "COSH",                 4, ARG_NONE,   1, FLAG_NONE,                   0x0000a062, docmd_cosh        },
    { /* CROSS */       "CROSS",                5, ARG_NONE,   2, FLAG_NONE,                   0x0000a6ca, docmd_cross       },
    { /* CUSTOM */      "CUST\317\315",         6, ARG_NONE,   0, FLAG_NONE,                   0x0000a26f, docmd_custom      },
    { /* DECM */        "DECM",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a0e3, docmd_decm        },
    { /* DELR */        "DELR",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a0ab, docmd_delr        },
    { /* DET */         "DET",                  3, ARG_NONE,   1, FLAG_NONE,                   0x0000a6cc, docmd_det         },
    { /* DIM */         "DIM",                  3, ARG_MAT,    2, FLAG_NONE,                   0x00c4f2ec, docmd_dim         },
    { /* DOT */         "DOT",                  3, ARG_NONE,   2, FLAG_NONE,                   0x0000a6cb, docmd_dot         },
    { /* EDIT */        "EDIT",                 4, ARG_NONE,   1, FLAG_NONE,                   0x0000a6e1, docmd_edit        },
    { /* EDITN */       "EDITN",                5, ARG_MAT,    0, FLAG_NONE,                   0x00c6f2ef, docmd_editn       },
    { /* EXITALL */     "EXITA\314\314",        7, ARG_NONE,   0, FLAG_NONE,                   0x0000a26c, docmd_exitall     },
    { /* EXPF */        "EXPF",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a0a0, docmd_expf        },
    { /* E_POW_X_1 */   "E^X-\261",             5, ARG_NONE,   1, FLAG_NONE,                   0x00000058, docmd_e_pow_x_1   },
    { /* FCSTX */       "FCSTX",                5, ARG_NONE,   1, FLAG_NONE,                   0x0000a0a8, docmd_fcstx       },
    { /* FCSTY */       "FCSTY",                5, ARG_NONE,   1, FLAG_NONE,                   0x0000a0a9, docmd_fcsty       },
    { /* FNRM */        "FNRM",                 4, ARG_NONE,   1, FLAG_NONE,                   0x0000a6cf, docmd_fnrm        },
    { /* GETM */        "GETM",                 4, ARG_NONE,   2, FLAG_NONE,                   0x0000a6e8, docmd_getm        },
    { /* GROW */        "GROW",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a6e3, docmd_grow        },
    { /* HEXM */        "HEXM",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a0e2, docmd_hexm        },
    { /* HMSADD */      "HMS+",                 4, ARG_NONE,   2, FLAG_NONE,                   0x00000049, docmd_hmsadd      },
    { /* HMSSUB */      "HMS-",                 4, ARG_NONE,   2, FLAG_NONE,                   0x0000004a, docmd_hmssub      },
    { /* I_ADD */       "I+",                   2, ARG_NONE,   0, FLAG_NONE,                   0x0000a6d2, docmd_i_add       },
    { /* I_SUB */       "I-",                   2, ARG_NONE,   0, FLAG_NONE,                   0x0000a6d3, docmd_i_sub       },
    { /* INDEX */       "INDEX",                5, ARG_MAT,    0, FLAG_NONE,                   0x0087f2da, docmd_index       },
    { /* INSR */        "INSR",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a0aa, docmd_insr        },
    { /* INTEG */       "INTEG",                5, ARG_RVAR,   0, FLAG_NONE,                   0x00b6f2ea, docmd_integ       },
    { /* INVRT */       "INV\322\324",          5, ARG_NONE,   1, FLAG_NONE,                   0x0000a6ce, docmd_invrt       },
    { /* J_ADD */       "J+",                   2, ARG_NONE,   0, FLAG_NONE,                   0x0000a6d4, docmd_j_add       },
    { /* J_SUB */       "J-",                   2, ARG_NONE,   0, FLAG_NONE,                   0x0000a6d5, docmd_j_sub       },
    { /* LINF */        "LINF",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a0a1, docmd_linf        },
    { /* LINSIGMA */    "LIN\005",              4, ARG_NONE,   0, FLAG_NONE,                   0x0000a0ad, docmd_linsigma    },
    { /* LN_1_X */      "LN1+\330",             5, ARG_NONE,   1, FLAG_NONE,                   0x00000065, docmd_ln_1_x      },
    { /* LOGF */        "LOGF",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a0a2, docmd_logf        },
    { /* MEAN */        "MEAN",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000007c, docmd_mean        },
    { /* NOT */         "NOT",                  3, ARG_NONE,   1, FLAG_NONE,                   0x0000a587, docmd_not         },
    { /* OCTM */        "OCTM",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a0e4, docmd_octm        },
    { /* OLD */         "OLD",                  3, ARG_NONE,   0, FLAG_NONE,                   0x0000a6db, docmd_rclel       },
    { /* OR */          "OR",                   2, ARG_NONE,   2, FLAG_NONE,                   0x0000a589, docmd_or          },
    { /* PGMSLV */      "P\307\315SLV",         6, ARG_PRGM,   0, FLAG_NONE,                   0x00b5f2e9, docmd_pgmslv      },
    { /* PGMINT */      "P\307\315INT",         6, ARG_PRGM,   0, FLAG_NONE,                   0x00b4f2e8, docmd_pgmint      },
    { /* POSA */        "POSA",                 4, ARG_NONE,   1, FLAG_NONE,                   0x0000a65c, docmd_posa        },
    { /* PUTM */        "PUTM",                 4, ARG_NONE,   1, FLAG_NONE,                   0x0000a6e9, docmd_putm        },
    { /* PWRF */        "PWRF",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a0a3, docmd_pwrf        },
    { /* RCLEL */       "RCLEL",                5, ARG_NONE,   0, FLAG_NONE,                   0x0000a6d7, docmd_rclel       },
    { /* RCLIJ */       "RCLIJ",                5, ARG_NONE,   0, FLAG_NONE,                   0x0000a6d9, docmd_rclij       },
    { /* RNRM */        "RNRM",                 4, ARG_NONE,   1, FLAG_NONE,                   0x0000a6ed, docmd_rnrm        },
    { /* ROTXY */       "ROTXY",                5, ARG_NONE,   2, FLAG_NONE,                   0x0000a58b, docmd_rotxy       },
    { /* RSUM */        "RSUM",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a6d0, docmd_rsum        },
    { /* SWAP_R */      "R<>R",                 4, ARG_NONE,   2, FLAG_NONE,                   0x0000a6d1, docmd_swap_r      },
    { /* SDEV */        "SDEV",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000007d, docmd_sdev        },
    { /* SINH */        "SINH",                 4, ARG_NONE,   1, FLAG_NONE,                   0x0000a061, docmd_sinh        },
    { /* SLOPE */       "SLOPE",                5, ARG_NONE,   0, FLAG_NONE,                   0x0000a0a4, docmd_slope       },
    { /* SOLVE */       "SOLVE",                5, ARG_RVAR,   1, FLAG_NONE,                   0x00b7f2eb, docmd_solve       },
    { /* STOEL */       "STOEL",                5, ARG_NONE,   1, FLAG_NONE,                   0x0000a6d6, docmd_stoel       },
    { /* STOIJ */       "STOIJ",                5, ARG_NONE,   2, FLAG_NONE,                   0x0000a6d8, docmd_stoij       },
    { /* SUM */         "SUM",                  3, ARG_NONE,   0, FLAG_NONE,                   0x0000a0a5, docmd_sum         },
    { /* TANH */        "TANH",                 4, ARG_NONE,   1, FLAG_NONE,                   0x0000a063, docmd_tanh        },
    { /* TRANS */       "TRANS",                5, ARG_NONE,   1, FLAG_NONE,                   0x0000a6c9, docmd_trans       },
    { /* UVEC */        "UVEC",                 4, ARG_NONE,   1, FLAG_NONE,                   0x0000a6cd, docmd_uvec        },
    { /* WMEAN */       "WM\305\301N",          5, ARG_NONE,   0, FLAG_NONE,                   0x0000a0ac, docmd_wmean       },
    { /* WRAP */        "WRAP",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a6e2, docmd_wrap        },
    { /* X_SWAP */      "X<>",                  3, ARG_VAR,    1, FLAG_NONE,                   0x008600ce, docmd_x_swap      },
    { /* XOR */         "XOR",                  3, ARG_NONE,   2, FLAG_NONE,                   0x0000a58a, docmd_xor         },
    { /* YINT */        "YINT",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a0a6, docmd_yint        },
    { /* TO_DEC */      "\017DEC",              4, ARG_NONE,   1, FLAG_NONE,                   0x0000005f, docmd_to_dec      },
    { /* TO_OCT */      "\017OCT",              4, ARG_NONE,   1, FLAG_NONE,                   0x0000006f, docmd_to_oct      },
    { /* LEFT */        "\020",                 1, ARG_NONE,   1, FLAG_NONE,                   0x0000a6dc, docmd_left        },
    { /* UP */          "^",                    1, ARG_NONE,   1, FLAG_NONE,                   0x0000a6de, docmd_up          },
    { /* DOWN */        "\016",                 1, ARG_NONE,   1, FLAG_NONE,                   0x0000a6df, docmd_down        },
    { /* RIGHT */       "\017",                 1, ARG_NONE,   1, FLAG_NONE,                   0x0000a6dd, docmd_right       },
    { /* PERCENT_CH */  "%CH",                  3, ARG_NONE,   2, FLAG_NONE,                   0x0000004d, docmd_percent_ch  },
    { /* SIMQ */        "SIMQ",                 4, ARG_COUNT,  0, FLAG_HIDDEN + FLAG_NO_PRGM,  0x02000000, docmd_simq        },
    { /* MATA */        "MATA",                 4, ARG_NONE,   0, FLAG_HIDDEN + FLAG_NO_PRGM,  0x02000000, docmd_mata        },
    { /* MATB */        "MATB",                 4, ARG_NONE,   0, FLAG_HIDDEN + FLAG_NO_PRGM,  0x02000000, docmd_matb        },
    { /* MATX */        "MATX",                 4, ARG_NONE,   0, FLAG_HIDDEN + FLAG_NO_PRGM,  0x02000000, docmd_matx        },
    { /* GOTOROW */     "GOTO\240\322\357\367", 8, ARG_COUNT,  0, FLAG_HIDDEN,                 0x02000000, NULL,             },
    { /* GOTOCOLUMN */  "GOTO Column",         11, ARG_COUNT,  0, FLAG_HIDDEN,                 0x02000000, NULL,             },
    { /* A_THRU_F */    "A...F",                5, ARG_NONE,   0, FLAG_HIDDEN + FLAG_NO_PRGM,  0x02000000, NULL,             },
    { /* CLALLb */      "CLALL",                5, ARG_NONE,   0, FLAG_HIDDEN,                 0x02000000, docmd_clall       },
    { /* PGMSLVi */     "P\307\315SLV",         6, ARG_PRGM,   0, FLAG_HIDDEN,                 0x02000000, docmd_pgmslvi     },
    { /* PGMINTi */     "P\307\315INT",         6, ARG_PRGM,   0, FLAG_HIDDEN,                 0x02000000, docmd_pgminti     },
    { /* VMSTO2 */      "STO",                  3, ARG_OTHER,  1, FLAG_HIDDEN,                 0x02000000, docmd_vmsto2      },
    { /* VMSOLVE */     "SOLVE",                5, ARG_OTHER,  1, FLAG_HIDDEN,                 0x02000000, docmd_vmsolve     },
    { /* MAX */         "[MAX]",                5, ARG_NONE,   0, FLAG_NONE,                   0x0000a6eb, docmd_max         },
    { /* MIN */         "[MIN]",                5, ARG_NONE,   0, FLAG_NONE,                   0x0000a6ea, docmd_min         },
    { /* FIND */        "[F\311ND]",            6, ARG_NONE,   1, FLAG_NONE,                   0x0000a6ec, docmd_find        },
    { /* XROM */        "XROM",                 4, ARG_OTHER,  0, FLAG_HIDDEN,                 0x01000000, docmd_xrom        },

    /* Here endeth the original Free42 function table. */

    /* Underhill's COPAN (Obsolete) */
    { /* OPENF */       "OPENF",                5, ARG_NONE,   0, FLAG_NONE,                   0x0000a7c1, docmd_xrom        },
    { /* CLOSF */       "CLOSF",                5, ARG_NONE,   0, FLAG_NONE,                   0x0000a7c2, docmd_xrom        },
    { /* READP */       "READP",                5, ARG_NONE,   0, FLAG_NONE,                   0x0000a7c3, docmd_xrom        },
    { /* WRITP */       "WRITP",                5, ARG_NONE,   0, FLAG_NONE,                   0x0000a7c4, docmd_xrom        },
    { /* GETXY */       "GETXY",                5, ARG_NONE,   0, FLAG_NONE,                   0x0000a7c5, docmd_xrom        },
    { /* PUTXY */       "PUTXY",                5, ARG_NONE,   0, FLAG_NONE,                   0x0000a7c6, docmd_xrom        },
    { /* CLRP */        "CLRP",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a7c7, docmd_xrom        },
    { /* CLRD */        "CLRD",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a7c8, docmd_xrom        },
    { /* APPD */        "APPD",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a7c9, docmd_xrom        },
    { /* GETN */        "GETN",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a7ca, docmd_xrom        },
    { /* PUTN */        "PUTN",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a7cb, docmd_xrom        },
    { /* GETZ */        "GETZ",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a7cc, docmd_xrom        },
    { /* PUTZ */        "PUTZ",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a7cd, docmd_xrom        },
    { /* DELP */        "DELP",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a7ce, docmd_xrom        },

    /* Big Stack; additional functions from 4STK */
    { /* DROP */        "DROP",                 4, ARG_NONE,   1, FLAG_NONE,                   0x0000a271, docmd_drop        },

    /* Accelerometer, GPS, and compass support */
    { /* ACCEL */       "ACCEL",                5, ARG_NONE,   0, FLAG_NONE,                   0x0000a7cf, docmd_accel       },
    { /* LOCAT */       "LOCAT",                5, ARG_NONE,   0, FLAG_NONE,                   0x0000a7d0, docmd_locat       },
    { /* HEADING */     "H\305\301D\311NG",     7, ARG_NONE,   0, FLAG_NONE,                   0x0000a7d1, docmd_heading     },

    /* Time Module & CX Time support*/
    { /* ADATE */       "ADATE",                5, ARG_NONE,   1, FLAG_NONE,                   0x0000a681, docmd_adate       },
    { /* ALMCAT */      "AL\315CAT",            6, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a682, docmd_xrom        },
    { /* ALMNOW */      "AL\315N\317W",         6, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a683, docmd_xrom        },
    { /* ATIME */       "ATIME",                5, ARG_NONE,   1, FLAG_NONE,                   0x0000a684, docmd_atime       },
    { /* ATIME24 */     "AT\311\315\30524",     7, ARG_NONE,   1, FLAG_NONE,                   0x0000a685, docmd_atime24     },
    { /* CLK12 */       "CL\31312",             5, ARG_NONE,   0, FLAG_NONE,                   0x0000a686, docmd_clk12       },
    { /* CLK24 */       "CL\31324",             5, ARG_NONE,   0, FLAG_NONE,                   0x0000a687, docmd_clk24       },
    { /* CLKT */        "CLKT",                 4, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a688, docmd_xrom        },
    { /* CLKTD */       "CL\313TD",             5, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a689, docmd_xrom        },
    { /* CLOCK */       "CL\317\303K",          5, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a68a, docmd_xrom        },
    { /* CORRECT */     "CORR\305\303\324",     7, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a68b, docmd_xrom        },
    { /* DATE */        "DATE",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a68c, docmd_date        },
    { /* DATE_PLUS */   "DATE+",                5, ARG_NONE,   2, FLAG_NONE,                   0x0000a68d, docmd_date_plus   },
    { /* DDAYS */       "DDAYS",                5, ARG_NONE,   2, FLAG_NONE,                   0x0000a68e, docmd_ddays       },
    { /* DMY */         "DMY",                  3, ARG_NONE,   0, FLAG_NONE,                   0x0000a68f, docmd_dmy         },
    { /* DOW */         "DOW",                  3, ARG_NONE,   1, FLAG_NONE,                   0x0000a690, docmd_dow         },
    { /* MDY */         "MDY",                  3, ARG_NONE,   0, FLAG_NONE,                   0x0000a691, docmd_mdy         },
    { /* RCLAF */       "RCLAF",                5, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a692, docmd_xrom        },
    { /* RCLSW */       "RC\314SW",             5, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a693, docmd_xrom        },
    { /* RUNSW */       "R\325NSW",             5, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a694, docmd_xrom        },
    { /* SETAF */       "SETAF",                5, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a695, docmd_xrom        },
    { /* SETDATE */     "S\305\324DATE",        7, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a696, docmd_xrom        },
    { /* SETIME */      "S\305TIME",            6, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a697, docmd_xrom        },
    { /* SETSW */       "SE\324SW",             5, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a698, docmd_xrom        },
    { /* STOPSW */      "ST\317\320SW",         6, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a699, docmd_xrom        },
    { /* SW */          "SW",                   2, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a69a, docmd_xrom        },
    { /* T_PLUS_X */    "T+X",                  3, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a69b, docmd_xrom        },
    { /* TIME */        "TIME",                 4, ARG_NONE,   0, FLAG_NONE,                   0x0000a69c, docmd_time        },
    { /* XYZALM */      "XYZALM",               6, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a69d, docmd_xrom        },
    { /* CLALMA */      "CLAL\315A",            6, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a69f, docmd_xrom        },
    { /* CLALMX */      "CLAL\315X",            6, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a6a0, docmd_xrom        },
    { /* CLRALMS */     "CLRALMS",              7, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a6a1, docmd_xrom        },
    { /* RCLALM */      "RCLALM",               6, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a6a2, docmd_xrom        },
    { /* SWPT */        "SWPT",                 4, ARG_NONE,   0, FLAG_HIDDEN,                 0x0000a6a3, docmd_xrom        },

    /* Intel Decimal Floating-Point Math Library: self-test */
    { /* FPTEST */     "FPT\305ST",             6, ARG_NONE,   0, FLAG_NONE,                   0x0000a7d2, docmd_fptest      },

    /* Programming */
    { /* LSTO */       "LSTO",                  4, ARG_NAMED,  1, FLAG_NONE,                   0x00c7f2ed, docmd_lsto        },
    { /* SST_UP */     "SST^",                  4, ARG_NONE,   0, FLAG_NONE,                   0x02000000, NULL,             },
    { /* SST_RT */     "SST\017",               4, ARG_NONE,   0, FLAG_NONE,                   0x02000000, NULL,             },
    { /* WSIZE */      "WSIZE",                 5, ARG_NONE,   1, FLAG_NONE,                   0x0000a7d3, docmd_wsize       },
    { /* WSIZE_T */    "WS\311Z\305?",          6, ARG_NONE,   0, FLAG_NONE,                   0x0000a7d4, docmd_wsize_t     },
    { /* YMD */        "YMD",                   3, ARG_NONE,   0, FLAG_NONE,                   0x0000a7d5, docmd_ymd         },
    { /* BSIGNED */    "BS\311GN\305\304",      7, ARG_NONE,   0, FLAG_NONE,                   0x0000a7d6, docmd_bsigned     },
    { /* BWRAP */      "BWR\301P",              5, ARG_NONE,   0, FLAG_NONE,                   0x0000a7d7, docmd_bwrap       },
    { /* BRESET */     "BR\305S\305T",          6, ARG_NONE,   0, FLAG_NONE,                   0x0000a7d8, docmd_breset      },
    { /* GETKEY1 */    "G\305TK\305\3311",      7, ARG_NONE,   0, FLAG_NONE,                   0x0000a7d9, docmd_getkey1     },
    { /* LASTO */      "LASTO",                 5, ARG_NAMED,  0, FLAG_NONE,                   0x00f5f2c8, docmd_lasto       },

    /* Useful X-Fcn functions missing from the 42S */
    { /* ANUM */       "ANUM",                  4, ARG_NONE,   0, FLAG_NONE,                   0x0000a642, docmd_anum        },
    { /* X<>F */       "X<>F",                  4, ARG_NONE,   1, FLAG_NONE,                   0x0000a66e, docmd_x_swap_f    },
    { /* RCLFLAG */    "RCLFLAG",               7, ARG_NONE,   0, FLAG_NONE,                   0x0000a660, docmd_rclflag     },
    { /* STOFLAG */    "STOFLAG",               7, ARG_NONE,  -1, FLAG_NONE,                   0x0000a66d, docmd_stoflag     },
    
    /* No-op, stored in raw files as 0xf0, a.k.a. TEXT 0 on the 41C */
    { /* NOP */        "NOP",                   3, ARG_NONE,   0, FLAG_NONE,                   0x000000f0, docmd_nop         },

    /* Fused Multiply-Add */
    { /* FMA */        "FMA",                   3, ARG_NONE,   3, FLAG_NONE,                   0x0000a7da, docmd_fma         },

    /* User-defined functions */
    // Note: a7db-a7dd encode FUNC0-FUNC2, superseded by FUNC 00, FUNC 11, and
    // FUNC 21, and a7e0 encodes RTNERR without argument, and is superseded by
    // RTNERR IND ST X... So we don't need those three XROMs any more, but we
    // can't safely use them, so they should be left unassigned.
    { /* FUNC */       "FUNC",                  4, ARG_FUNC,   0, FLAG_PRGM_ONLY,              0x0000f2e0, docmd_func        },
    { /* RTNYES */     "RTNYES",                6, ARG_NONE,   0, FLAG_NONE,                   0x0000a7de, docmd_rtnyes      },
    { /* RTNNO */      "RTNNO",                 5, ARG_NONE,   0, FLAG_NONE,                   0x0000a7df, docmd_rtnno       },
    { /* RTNERR */     "RTNERR",                6, ARG_NUM9,   0, FLAG_PRGM_ONLY,              0x00dcf2a0, docmd_rtnerr      },
    { /* STRACE */     "STRACE",                6, ARG_NONE,   0, FLAG_NONE,                   0x0000a7e1, docmd_strace      },

    /* Big Stack */
    { /* 4STK */       "4STK",                  4, ARG_NONE,   0, FLAG_NONE,                   0x0000a7e2, docmd_4stk        },
    { /* L4STK */      "L4STK",                 5, ARG_NONE,   0, FLAG_NONE,                   0x0000a7e3, docmd_l4stk       },
    { /* NSTK */       "NSTK",                  4, ARG_NONE,   0, FLAG_NONE,                   0x0000a7e4, docmd_nstk        },
    { /* LNSTK */      "LNSTK",                 5, ARG_NONE,   0, FLAG_NONE,                   0x0000a7e5, docmd_lnstk       },
    { /* DEPTH */      "DEPTH",                 5, ARG_NONE,   0, FLAG_NONE,                   0x0000a7e6, docmd_depth       },
    { /* DROPN */      "DR\317PN",              5, ARG_NUM9,   0, FLAG_NONE,                   0x00f1f2a1, docmd_dropn       },
    { /* DUP */        "DUP",                   3, ARG_NONE,   1, FLAG_NONE,                   0x0000a7e7, docmd_dup         },
    { /* DUPN */       "DUPN",                  4, ARG_NUM9,   0, FLAG_NONE,                   0x00f2f2a2, docmd_dupn        },
    { /* PICK */       "PICK",                  4, ARG_NUM9,   0, FLAG_NONE,                   0x00f3f2a3, docmd_pick        },
    { /* UNPICK */     "UNPICK",                6, ARG_NUM9,   0, FLAG_NONE,                   0x00f4f2a4, docmd_unpick      },
    { /* RDNN */       "R\016N",                3, ARG_NUM9,   0, FLAG_NONE,                   0x00f6f2a5, docmd_rdnn        },
    { /* RUPN */       "R^N",                   3, ARG_NUM9,   0, FLAG_NONE,                   0x00f7f2a6, docmd_rupn        }
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
    const command_spec *cs = &cmd_array[cmd];
    if (flags.f.big_stack) {
        if (cs->argcount == -1) {
            if (sp == -1 || sp == 0 && stack[sp]->type != TYPE_COMPLEX && stack[sp]->type != TYPE_COMPLEXMATRIX)
                return ERR_TOO_FEW_ARGUMENTS;
        } else {
            if (sp + 1 < cs->argcount)
                return ERR_TOO_FEW_ARGUMENTS;
        }
    }
    return cs->handler(arg);
}
