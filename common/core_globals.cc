/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2017  Thomas Okken
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

#include "core_globals.h"
#include "core_commands2.h"
#include "core_commands4.h"
#include "core_display.h"
#include "core_helpers.h"
#include "core_main.h"
#include "core_math1.h"
#include "core_tables.h"
#include "core_variables.h"
#include "shell.h"

#ifndef BCD_MATH
// We need these locally for BID128->double conversion
#include "bid_conf.h"
#include "bid_functions.h"
#endif


error_spec errors[] = {
    { /* NONE */                   NULL,                       0 },
    { /* ALPHA_DATA_IS_INVALID */  "Alpha Data Is Invalid",   21 },
    { /* INSUFFICIENT_MEMORY */    "Insufficient Memory",     19 },
    { /* NOT_YET_IMPLEMENTED */    "Not Yet Implemented",     19 },
    { /* OUT_OF_RANGE */           "Out of Range",            12 },
    { /* DIVIDE_BY_0 */            "Divide by 0",             11 },
    { /* INVALID_TYPE */           "Invalid Type",            12 },
    { /* INVALID_DATA */           "Invalid Data",            12 },
    { /* DIMENSION_ERROR */        "Dimension Error",         15 },
    { /* SIZE_ERROR */             "Size Error",              10 },
    { /* INTERNAL_ERROR */         "Internal Error",          14 },
    { /* NONEXISTENT */            "Nonexistent",             11 },
    { /* RESTRICTED_OPERATION */   "Restricted Operation",    20 },
    { /* YES */                    "Yes",                      3 },
    { /* NO */                     "No",                       2 },
    { /* STOP */                   NULL,                       0 },
    { /* LABEL_NOT_FOUND */        "Label Not Found",         15 },
    { /* NO_REAL_VARIABLES */      "No Real Variables",       17 },
    { /* NO_COMPLEX_VARIABLES */   "No Complex Variables",    20 },
    { /* NO_MATRIX_VARIABLES */    "No Matrix Variables",     19 },
    { /* NO_MENU_VARIABLES */      "No Menu Variables",       17 },
    { /* STAT_MATH_ERROR */        "Stat Math Error",         15 },
    { /* INVALID_FORECAST_MODEL */ "Invalid Forecast Model",  22 },
    { /* SOLVE_INTEG_RTN_LOST */   "Solve/Integ RTN Lost",    20 },
    { /* SINGULAR_MATRIX */        "Singular Matrix",         15 },
    { /* SOLVE_SOLVE */            "Solve(Solve)",            12 },
    { /* INTEG_INTEG */            "Integ(Integ)",            12 },
    { /* RUN */                    NULL,                       0 },
    { /* INTERRUPTED */            "Interrupted",             11 },
    { /* PRINTING_IS_DISABLED */   "Printing Is Disabled",    20 },
    { /* INTERRUPTIBLE */          NULL,                       0 },
    { /* NO_VARIABLES */           "No Variables",            12 },
    { /* SUSPICIOUS_OFF */         "Suspicious OFF",          14 }
};


menu_spec menus[] = {
    { /* MENU_ALPHA1 */ MENU_NONE, MENU_ALPHA2, MENU_ALPHA2,
                      { { MENU_ALPHA_ABCDE1, 5, "ABCDE" },
                        { MENU_ALPHA_FGHI,   4, "FGHI"  },
                        { MENU_ALPHA_JKLM,   4, "JKLM"  },
                        { MENU_ALPHA_NOPQ1,  4, "NOPQ"  },
                        { MENU_ALPHA_RSTUV1, 5, "RSTUV" },
                        { MENU_ALPHA_WXYZ,   4, "WXYZ"  } } },
    { /* MENU_ALPHA2 */ MENU_NONE, MENU_ALPHA1, MENU_ALPHA1,
                      { { MENU_ALPHA_PAREN, 5, "( [ {"     },
                        { MENU_ALPHA_ARROW, 3, "\020^\016" },
                        { MENU_ALPHA_COMP,  5, "< = >"     },
                        { MENU_ALPHA_MATH,  4, "MATH"      },
                        { MENU_ALPHA_PUNC1, 4, "PUNC"      },
                        { MENU_ALPHA_MISC1, 4, "MISC"      } } },
    { /* MENU_ALPHA_ABCDE1 */ MENU_ALPHA1, MENU_ALPHA_ABCDE2, MENU_ALPHA_ABCDE2,
                      { { MENU_NONE, 1, "A" },
                        { MENU_NONE, 1, "B" },
                        { MENU_NONE, 1, "C" },
                        { MENU_NONE, 1, "D" },
                        { MENU_NONE, 1, "E" },
                        { MENU_NONE, 1, " " } } },
    { /* MENU_ALPHA_ABCDE2 */ MENU_ALPHA1, MENU_ALPHA_ABCDE1, MENU_ALPHA_ABCDE1,
                      { { MENU_NONE, 1, "\026" },
                        { MENU_NONE, 1, "\024" },
                        { MENU_NONE, 1, "\031" },
                        { MENU_NONE, 1, " "    },
                        { MENU_NONE, 1, " "    },
                        { MENU_NONE, 1, " "    } } },
    { /* MENU_ALPHA_FGHI */ MENU_ALPHA1, MENU_NONE, MENU_NONE,
                      { { MENU_NONE, 1, "F" },
                        { MENU_NONE, 1, "G" },
                        { MENU_NONE, 1, "H" },
                        { MENU_NONE, 1, "I" },
                        { MENU_NONE, 1, " " },
                        { MENU_NONE, 1, " " } } },
    { /* MENU_ALPHA_JKLM */ MENU_ALPHA1, MENU_NONE, MENU_NONE,
                      { { MENU_NONE, 1, "J" },
                        { MENU_NONE, 1, "K" },
                        { MENU_NONE, 1, "L" },
                        { MENU_NONE, 1, "M" },
                        { MENU_NONE, 1, " " },
                        { MENU_NONE, 1, " " } } },
    { /* MENU_ALPHA_NOPQ1 */ MENU_ALPHA1, MENU_ALPHA_NOPQ2, MENU_ALPHA_NOPQ2,
                      { { MENU_NONE, 1, "N" },
                        { MENU_NONE, 1, "O" },
                        { MENU_NONE, 1, "P" },
                        { MENU_NONE, 1, "Q" },
                        { MENU_NONE, 1, " " },
                        { MENU_NONE, 1, " " } } },
    { /* MENU_ALPHA_NOPQ2 */ MENU_ALPHA1, MENU_ALPHA_NOPQ1, MENU_ALPHA_NOPQ1,
                      { { MENU_NONE, 1, "\025" },
                        { MENU_NONE, 1, "\034" },
                        { MENU_NONE, 1, " "    },
                        { MENU_NONE, 1, " "    },
                        { MENU_NONE, 1, " "    },
                        { MENU_NONE, 1, " "    } } },
    { /* MENU_ALPHA_RSTUV1 */ MENU_ALPHA1, MENU_ALPHA_RSTUV2, MENU_ALPHA_RSTUV2,
                      { { MENU_NONE, 1, "R" },
                        { MENU_NONE, 1, "S" },
                        { MENU_NONE, 1, "T" },
                        { MENU_NONE, 1, "U" },
                        { MENU_NONE, 1, "V" },
                        { MENU_NONE, 1, " " } } },
    { /* MENU_ALPHA_RSTUV2 */ MENU_ALPHA1, MENU_ALPHA_RSTUV1, MENU_ALPHA_RSTUV1,
                      { { MENU_NONE, 1, " "    },
                        { MENU_NONE, 1, " "    },
                        { MENU_NONE, 1, " "    },
                        { MENU_NONE, 1, "\035" },
                        { MENU_NONE, 1, " "    },
                        { MENU_NONE, 1, " "    } } },
    { /* MENU_ALPHA_WXYZ */ MENU_ALPHA1, MENU_NONE, MENU_NONE,
                      { { MENU_NONE, 1, "W" },
                        { MENU_NONE, 1, "X" },
                        { MENU_NONE, 1, "Y" },
                        { MENU_NONE, 1, "Z" },
                        { MENU_NONE, 1, " " },
                        { MENU_NONE, 1, " " } } },
    { /* MENU_ALPHA_PAREN */ MENU_ALPHA2, MENU_NONE, MENU_NONE,
                      { { MENU_NONE, 1, "(" },
                        { MENU_NONE, 1, ")" },
                        { MENU_NONE, 1, "[" },
                        { MENU_NONE, 1, "]" },
                        { MENU_NONE, 1, "{" },
                        { MENU_NONE, 1, "}" } } },
    { /* MENU_ALPHA_ARROW */ MENU_ALPHA2, MENU_NONE, MENU_NONE,
                      { { MENU_NONE, 1, "\020" },
                        { MENU_NONE, 1, "^"    },
                        { MENU_NONE, 1, "\016" },
                        { MENU_NONE, 1, "\017" },
                        { MENU_NONE, 1, " "    },
                        { MENU_NONE, 1, " "    } } },
    { /* MENU_ALPHA_COMP */ MENU_ALPHA2, MENU_NONE, MENU_NONE,
                      { { MENU_NONE, 1, "="    },
                        { MENU_NONE, 1, "\014" },
                        { MENU_NONE, 1, "<"    },
                        { MENU_NONE, 1, ">"    },
                        { MENU_NONE, 1, "\011" },
                        { MENU_NONE, 1, "\013" } } },
    { /* MENU_ALPHA_MATH */ MENU_ALPHA2, MENU_NONE, MENU_NONE,
                      { { MENU_NONE, 1, "\005" },
                        { MENU_NONE, 1, "\003" },
                        { MENU_NONE, 1, "\002" },
                        { MENU_NONE, 1, "\027" },
                        { MENU_NONE, 1, "\023" },
                        { MENU_NONE, 1, "\021" } } },
    { /* MENU_ALPHA_PUNC1 */ MENU_ALPHA2, MENU_ALPHA_PUNC2, MENU_ALPHA_PUNC2,
                      { { MENU_NONE, 1, ","  },
                        { MENU_NONE, 1, ";"  },
                        { MENU_NONE, 1, ":"  },
                        { MENU_NONE, 1, "!"  },
                        { MENU_NONE, 1, "?"  },
                        { MENU_NONE, 1, "\"" } } },
    { /* MENU_ALPHA_PUNC2 */ MENU_ALPHA2, MENU_ALPHA_PUNC1, MENU_ALPHA_PUNC1,
                      { { MENU_NONE, 1, "\032" },
                        { MENU_NONE, 1, "_"    },
                        { MENU_NONE, 1, "`"    },
                        { MENU_NONE, 1, "'"    },
                        { MENU_NONE, 1, "\010" },
                        { MENU_NONE, 1, "\012" } } },
    { /* MENU_ALPHA_MISC1 */ MENU_ALPHA2, MENU_ALPHA_MISC2, MENU_ALPHA_MISC2,
                      { { MENU_NONE, 1, "$"    },
                        { MENU_NONE, 1, "*"    },
                        { MENU_NONE, 1, "#"    },
                        { MENU_NONE, 1, "/"    },
                        { MENU_NONE, 1, "\037" },
                        { MENU_NONE, 1, " "    } } },
    { /* MENU_ALPHA_MISC2 */ MENU_ALPHA2, MENU_ALPHA_MISC1, MENU_ALPHA_MISC1,
                      { { MENU_NONE, 1, "\022" },
                        { MENU_NONE, 1, "&"    },
                        { MENU_NONE, 1, "@"    },
                        { MENU_NONE, 1, "\\"   },
                        { MENU_NONE, 1, "~"    },
                        { MENU_NONE, 1, "|"    } } },
    { /* MENU_ST */ MENU_NONE, MENU_NONE, MENU_NONE,
                      { { MENU_NONE, 4, "ST L" },
                        { MENU_NONE, 4, "ST X" },
                        { MENU_NONE, 4, "ST Y" },
                        { MENU_NONE, 4, "ST Z" },
                        { MENU_NONE, 4, "ST T" },
                        { MENU_NONE, 0, ""     } } },
    { /* MENU_IND_ST */ MENU_NONE, MENU_NONE, MENU_NONE,
                      { { MENU_NONE, 3, "IND"  },
                        { MENU_NONE, 4, "ST L" },
                        { MENU_NONE, 4, "ST X" },
                        { MENU_NONE, 4, "ST Y" },
                        { MENU_NONE, 4, "ST Z" },
                        { MENU_NONE, 4, "ST T" } } },
    { /* MENU_IND */ MENU_NONE, MENU_NONE, MENU_NONE,
                      { { MENU_NONE, 3, "IND"  },
                        { MENU_NONE, 0, "" },
                        { MENU_NONE, 0, "" },
                        { MENU_NONE, 0, "" },
                        { MENU_NONE, 0, "" },
                        { MENU_NONE, 0, "" } } },
    { /* MENU_MODES1 */ MENU_NONE, MENU_MODES2, MENU_MODES2,
                      { { 0x2000 + CMD_DEG,   0, "" },
                        { 0x2000 + CMD_RAD,   0, "" },
                        { 0x2000 + CMD_GRAD,  0, "" },
                        { 0x1000 + CMD_NULL,  0, "" },
                        { 0x2000 + CMD_RECT,  0, "" },
                        { 0x2000 + CMD_POLAR, 0, "" } } },
    { /* MENU_MODES2 */ MENU_NONE, MENU_MODES1, MENU_MODES1,
                      { { 0x1000 + CMD_SIZE,    0, "" },
                        { 0x2000 + CMD_QUIET,   0, "" },
                        { 0x2000 + CMD_CPXRES,  0, "" },
                        { 0x2000 + CMD_REALRES, 0, "" },
                        { 0x2000 + CMD_KEYASN,  0, "" },
                        { 0x2000 + CMD_LCLBL,   0, "" } } },
    { /* MENU_DISP */ MENU_NONE, MENU_NONE, MENU_NONE,
                      { { 0x2000 + CMD_FIX,      0, "" },
                        { 0x2000 + CMD_SCI,      0, "" },
                        { 0x2000 + CMD_ENG,      0, "" },
                        { 0x2000 + CMD_ALL,      0, "" },
                        { 0x2000 + CMD_RDXDOT,   0, "" },
                        { 0x2000 + CMD_RDXCOMMA, 0, "" } } },
    { /* MENU_CLEAR1 */ MENU_NONE, MENU_CLEAR2, MENU_CLEAR2,
                      { { 0x1000 + CMD_CLSIGMA, 0, "" },
                        { 0x1000 + CMD_CLP,     0, "" },
                        { 0x1000 + CMD_CLV,     0, "" },
                        { 0x1000 + CMD_CLST,    0, "" },
                        { 0x1000 + CMD_CLA,     0, "" },
                        { 0x1000 + CMD_CLX,     0, "" } } },
    { /* MENU_CLEAR2 */ MENU_NONE, MENU_CLEAR1, MENU_CLEAR1,
                      { { 0x1000 + CMD_CLRG,   0, "" },
                        { 0x1000 + CMD_DEL,    0, "" },
                        { 0x1000 + CMD_CLKEYS, 0, "" },
                        { 0x1000 + CMD_CLLCD,  0, "" },
                        { 0x1000 + CMD_CLMENU, 0, "" },
                        { 0x1000 + CMD_CLALLa, 0, "" } } },
    { /* MENU_CONVERT1 */ MENU_NONE, MENU_CONVERT2, MENU_CONVERT2,
                      { { 0x1000 + CMD_TO_DEG, 0, "" },
                        { 0x1000 + CMD_TO_RAD, 0, "" },
                        { 0x1000 + CMD_TO_HR,  0, "" },
                        { 0x1000 + CMD_TO_HMS, 0, "" },
                        { 0x1000 + CMD_TO_REC, 0, "" },
                        { 0x1000 + CMD_TO_POL, 0, "" } } },
    { /* MENU_CONVERT2 */ MENU_NONE, MENU_CONVERT1, MENU_CONVERT1,
                      { { 0x1000 + CMD_IP,   0, "" },
                        { 0x1000 + CMD_FP,   0, "" },
                        { 0x1000 + CMD_RND,  0, "" },
                        { 0x1000 + CMD_ABS,  0, "" },
                        { 0x1000 + CMD_SIGN, 0, "" },
                        { 0x1000 + CMD_MOD,  0, "" } } },
    { /* MENU_FLAGS */ MENU_NONE, MENU_NONE, MENU_NONE,
                      { { 0x1000 + CMD_SF,    0, "" },
                        { 0x1000 + CMD_CF,    0, "" },
                        { 0x1000 + CMD_FS_T,  0, "" },
                        { 0x1000 + CMD_FC_T,  0, "" },
                        { 0x1000 + CMD_FSC_T, 0, "" },
                        { 0x1000 + CMD_FCC_T, 0, "" } } },
    { /* MENU_PROB */ MENU_NONE, MENU_NONE, MENU_NONE,
                      { { 0x1000 + CMD_COMB,  0, "" },
                        { 0x1000 + CMD_PERM,  0, "" },
                        { 0x1000 + CMD_FACT,  0, "" },
                        { 0x1000 + CMD_GAMMA, 0, "" },
                        { 0x1000 + CMD_RAN,   0, "" },
                        { 0x1000 + CMD_SEED,  0, "" } } },
    { /* MENU_CUSTOM1 */ MENU_NONE, MENU_CUSTOM2, MENU_CUSTOM3,
                      { { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" } } },
    { /* MENU_CUSTOM2 */ MENU_NONE, MENU_CUSTOM3, MENU_CUSTOM1,
                      { { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" } } },
    { /* MENU_CUSTOM3 */ MENU_NONE, MENU_CUSTOM1, MENU_CUSTOM2,
                      { { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" } } },
    { /* MENU_PGM_FCN1 */ MENU_NONE, MENU_PGM_FCN2, MENU_PGM_FCN4,
                      { { 0x1000 + CMD_LBL,   0, "" },
                        { 0x1000 + CMD_RTN,   0, "" },
                        { 0x1000 + CMD_INPUT, 0, "" },
                        { 0x1000 + CMD_VIEW,  0, "" },
                        { 0x1000 + CMD_AVIEW, 0, "" },
                        { 0x1000 + CMD_XEQ,   0, "" } } },
    { /* MENU_PGM_FCN2 */ MENU_NONE, MENU_PGM_FCN3, MENU_PGM_FCN1,
                      { { MENU_PGM_XCOMP0,     3, "X?0" },
                        { MENU_PGM_XCOMPY,     3, "X?Y" },
                        { 0x1000 + CMD_PROMPT, 0, ""    },
                        { 0x1000 + CMD_PSE,    0, ""    },
                        { 0x1000 + CMD_ISG,    0, ""    },
                        { 0x1000 + CMD_DSE,    0, ""    } } },
    { /* MENU_PGM_FCN3 */ MENU_NONE, MENU_PGM_FCN4, MENU_PGM_FCN2,
                      { { 0x1000 + CMD_AIP,    0, "" },
                        { 0x1000 + CMD_XTOA,   0, "" },
                        { 0x1000 + CMD_AGRAPH, 0, "" },
                        { 0x1000 + CMD_PIXEL,  0, "" },
                        { 0x1000 + CMD_BEEP,   0, "" },
                        { 0x1000 + CMD_TONE,   0, "" } } },
    { /* MENU_PGM_FCN4 */ MENU_NONE, MENU_PGM_FCN1, MENU_PGM_FCN3,
                      { { 0x1000 + CMD_MVAR,    0, "" },
                        { 0x1000 + CMD_VARMENU, 0, "" },
                        { 0x1000 + CMD_GETKEY,  0, "" },
                        { 0x1000 + CMD_MENU,    0, "" },
                        { 0x1000 + CMD_KEYG,    0, "" },
                        { 0x1000 + CMD_KEYX,    0, "" } } },
    { /* MENU_PGM_XCOMP0 */ MENU_PGM_FCN2, MENU_NONE, MENU_NONE,
                      { { 0x1000 + CMD_X_EQ_0, 0, "" },
                        { 0x1000 + CMD_X_NE_0, 0, "" },
                        { 0x1000 + CMD_X_LT_0, 0, "" },
                        { 0x1000 + CMD_X_GT_0, 0, "" },
                        { 0x1000 + CMD_X_LE_0, 0, "" },
                        { 0x1000 + CMD_X_GE_0, 0, "" } } },
    { /* MENU_PGM_XCOMPY */ MENU_PGM_FCN2, MENU_NONE, MENU_NONE,
                      { { 0x1000 + CMD_X_EQ_Y, 0, "" },
                        { 0x1000 + CMD_X_NE_Y, 0, "" },
                        { 0x1000 + CMD_X_LT_Y, 0, "" },
                        { 0x1000 + CMD_X_GT_Y, 0, "" },
                        { 0x1000 + CMD_X_LE_Y, 0, "" },
                        { 0x1000 + CMD_X_GE_Y, 0, "" } } },
    { /* MENU_PRINT1 */ MENU_NONE, MENU_PRINT2, MENU_PRINT3,
                      { { 0x1000 + CMD_PRSIGMA, 0, "" },
                        { 0x1000 + CMD_PRP,     0, "" },
                        { 0x1000 + CMD_PRV,     0, "" },
                        { 0x1000 + CMD_PRSTK,   0, "" },
                        { 0x1000 + CMD_PRA,     0, "" },
                        { 0x1000 + CMD_PRX,     0, "" } } },
    { /* MENU_PRINT2 */ MENU_NONE, MENU_PRINT3, MENU_PRINT1,
                      { { 0x1000 + CMD_PRUSR, 0, "" },
                        { 0x1000 + CMD_LIST,  0, "" },
                        { 0x1000 + CMD_ADV,   0, "" },
                        { 0x1000 + CMD_PRLCD, 0, "" },
                        { 0x1000 + CMD_NULL,  0, "" },
                        { 0x1000 + CMD_DELAY, 0, "" } } },
    { /* MENU_PRINT3 */ MENU_NONE, MENU_PRINT1, MENU_PRINT2,
                      { { 0x2000 + CMD_PON,   0, "" },
                        { 0x2000 + CMD_POFF,  0, "" },
                        { 0x1000 + CMD_NULL,  0, "" },
                        { 0x2000 + CMD_MAN,   0, "" },
                        { 0x2000 + CMD_NORM,  0, "" },
                        { 0x2000 + CMD_TRACE, 0, "" } } },
    { /* MENU_TOP_FCN */ MENU_NONE, MENU_NONE, MENU_NONE,
                      { { 0x1000 + CMD_SIGMAADD, 0, "" },
                        { 0x1000 + CMD_INV,      0, "" },
                        { 0x1000 + CMD_SQRT,     0, "" },
                        { 0x1000 + CMD_LOG,      0, "" },
                        { 0x1000 + CMD_LN,       0, "" },
                        { 0x1000 + CMD_XEQ,      0, "" } } },
    { /* MENU_CATALOG */ MENU_NONE, MENU_NONE, MENU_NONE,
                      { { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" } } },
    { /* MENU_BLANK */ MENU_NONE, MENU_NONE, MENU_NONE,
                      { { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" } } },
    { /* MENU_PROGRAMMABLE */ MENU_NONE, MENU_NONE, MENU_NONE,
                      { { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" } } },
    { /* MENU_VARMENU */ MENU_NONE, MENU_NONE, MENU_NONE,
                      { { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" },
                        { 0, 0, "" } } },
    { /* MENU_STAT1 */ MENU_NONE, MENU_STAT2, MENU_STAT2,
                      { { 0x1000 + CMD_SIGMAADD, 0, ""     },
                        { 0x1000 + CMD_SUM,      0, ""     },
                        { 0x1000 + CMD_MEAN,     0, ""     },
                        { 0x1000 + CMD_WMEAN,    0, ""     },
                        { 0x1000 + CMD_SDEV,     0, ""     },
                        { MENU_STAT_CFIT,        4, "CFIT" } } },
    { /* MENU_STAT2 */ MENU_NONE, MENU_STAT1, MENU_STAT1,
                      { { 0x2000 + CMD_ALLSIGMA,   0, "" },
                        { 0x2000 + CMD_LINSIGMA,   0, "" },
                        { 0x1000 + CMD_NULL,       0, "" },
                        { 0x1000 + CMD_NULL,       0, "" },
                        { 0x1000 + CMD_SIGMAREG,   0, "" },
                        { 0x1000 + CMD_SIGMAREG_T, 0, "" } } },
    { /* MENU_STAT_CFIT */ MENU_STAT1, MENU_NONE, MENU_NONE,
                      { { 0x1000 + CMD_FCSTX, 0, ""     },
                        { 0x1000 + CMD_FCSTY, 0, ""     },
                        { 0x1000 + CMD_SLOPE, 0, ""     },
                        { 0x1000 + CMD_YINT,  0, ""     },
                        { 0x1000 + CMD_CORR,  0, ""     },
                        { MENU_STAT_MODL,     4, "MODL" } } },
    { /* MENU_STAT_MODL */ MENU_STAT_CFIT, MENU_NONE, MENU_NONE,
                      { { 0x2000 + CMD_LINF, 0, "" },
                        { 0x2000 + CMD_LOGF, 0, "" },
                        { 0x2000 + CMD_EXPF, 0, "" },
                        { 0x2000 + CMD_PWRF, 0, "" },
                        { 0x1000 + CMD_NULL, 0, "" },
                        { 0x1000 + CMD_BEST, 0, "" } } },
    { /* MENU_MATRIX1 */ MENU_NONE, MENU_MATRIX2, MENU_MATRIX3,
                      { { 0x1000 + CMD_NEWMAT, 0, "" },
                        { 0x1000 + CMD_INVRT,  0, "" },
                        { 0x1000 + CMD_DET,    0, "" },
                        { 0x1000 + CMD_TRANS,  0, "" },
                        { 0x1000 + CMD_SIMQ,   0, "" },
                        { 0x1000 + CMD_EDIT,   0, "" } } },
    { /* MENU_MATRIX2 */ MENU_NONE, MENU_MATRIX3, MENU_MATRIX1,
                      { { 0x1000 + CMD_DOT,   0, "" },
                        { 0x1000 + CMD_CROSS, 0, "" },
                        { 0x1000 + CMD_UVEC,  0, "" },
                        { 0x1000 + CMD_DIM,   0, "" },
                        { 0x1000 + CMD_INDEX, 0, "" },
                        { 0x1000 + CMD_EDITN, 0, "" } } },
    { /* MENU_MATRIX3 */ MENU_NONE, MENU_MATRIX1, MENU_MATRIX2,
                      { { 0x1000 + CMD_STOIJ, 0, "" },
                        { 0x1000 + CMD_RCLIJ, 0, "" },
                        { 0x1000 + CMD_STOEL, 0, "" },
                        { 0x1000 + CMD_RCLEL, 0, "" },
                        { 0x1000 + CMD_PUTM,  0, "" },
                        { 0x1000 + CMD_GETM,  0, "" } } },
    { /* MENU_MATRIX_SIMQ */ MENU_MATRIX1, MENU_NONE, MENU_NONE,
                      { { 0x1000 + CMD_MATA, 0, "" },
                        { 0x1000 + CMD_MATB, 0, "" },
                        { 0x1000 + CMD_MATX, 0, "" },
                        { 0x1000 + CMD_NULL, 0, "" },
                        { 0x1000 + CMD_NULL, 0, "" },
                        { 0x1000 + CMD_NULL, 0, "" } } },
    { /* MENU_MATRIX_EDIT1 */ MENU_NONE, MENU_MATRIX_EDIT2, MENU_MATRIX_EDIT2,
                      { { 0x1000 + CMD_LEFT,    0, "" },
                        { 0x1000 + CMD_OLD,     0, "" },
                        { 0x1000 + CMD_UP,      0, "" },
                        { 0x1000 + CMD_DOWN,    0, "" },
                        { 0x1000 + CMD_GOTOROW, 0, "" },
                        { 0x1000 + CMD_RIGHT,   0, "" } } },
    { /* MENU_MATRIX_EDIT2 */ MENU_NONE, MENU_MATRIX_EDIT1, MENU_MATRIX_EDIT1,
                      { { 0x1000 + CMD_INSR, 0, "" },
                        { 0x1000 + CMD_NULL, 0, "" },
                        { 0x1000 + CMD_DELR, 0, "" },
                        { 0x1000 + CMD_NULL, 0, "" },
                        { 0x2000 + CMD_WRAP, 0, "" },
                        { 0x2000 + CMD_GROW, 0, "" } } },
    { /* MENU_BASE */ MENU_NONE, MENU_NONE, MENU_NONE,
                      { { 0x1000 + CMD_A_THRU_F, 0, ""      },
                        { 0x2000 + CMD_HEXM,     0, ""      },
                        { 0x2000 + CMD_DECM,     0, ""      },
                        { 0x2000 + CMD_OCTM,     0, ""      },
                        { 0x2000 + CMD_BINM,     0, ""      },
                        { MENU_BASE_LOGIC,       5, "LOGIC" } } },
    { /* MENU_BASE_A_THRU_F */ MENU_BASE, MENU_NONE, MENU_NONE,
                      { { 0, 1, "A" },
                        { 0, 1, "B" },
                        { 0, 1, "C" },
                        { 0, 1, "D" },
                        { 0, 1, "E" },
                        { 0, 1, "F" } } },
    { /* MENU_BASE_LOGIC */ MENU_BASE, MENU_NONE, MENU_NONE,
                      { { 0x1000 + CMD_AND,   0, "" },
                        { 0x1000 + CMD_OR,    0, "" },
                        { 0x1000 + CMD_XOR,   0, "" },
                        { 0x1000 + CMD_NOT,   0, "" },
                        { 0x1000 + CMD_BIT_T, 0, "" },
                        { 0x1000 + CMD_ROTXY, 0, "" } } },
    { /* MENU_SOLVE */ MENU_NONE, MENU_NONE, MENU_NONE,
                      { { 0x1000 + CMD_MVAR,   0, "" },
                        { 0x1000 + CMD_NULL,   0, "" },
                        { 0x1000 + CMD_NULL,   0, "" },
                        { 0x1000 + CMD_NULL,   0, "" },
                        { 0x1000 + CMD_PGMSLV, 0, "" },
                        { 0x1000 + CMD_SOLVE,  0, "" } } },
    { /* MENU_INTEG */ MENU_NONE, MENU_NONE, MENU_NONE,
                      { { 0x1000 + CMD_MVAR,   0, "" },
                        { 0x1000 + CMD_NULL,   0, "" },
                        { 0x1000 + CMD_NULL,   0, "" },
                        { 0x1000 + CMD_NULL,   0, "" },
                        { 0x1000 + CMD_PGMINT, 0, "" },
                        { 0x1000 + CMD_INTEG,  0, "" } } },
    { /* MENU_INTEG_PARAMS */ MENU_NONE, MENU_NONE, MENU_NONE,
                      { { 0,                 4, "LLIM" },
                        { 0,                 4, "ULIM" },
                        { 0,                 3, "ACC"  },
                        { 0x1000 + CMD_NULL, 0, ""     },
                        { 0x1000 + CMD_NULL, 0, ""     },
                        { 0,                 1, "\003" } } }
};


/* By how much do the variables, programs, and labels
 * arrays grow when they are full
 */
#define VARS_INCREMENT 25
#define PRGMS_INCREMENT 10
#define LABELS_INCREMENT 10

/* Registers */
vartype *reg_x = NULL;
vartype *reg_y = NULL;
vartype *reg_z = NULL;
vartype *reg_t = NULL;
vartype *reg_lastx = NULL;
int reg_alpha_length = 0;
char reg_alpha[44];

/* Flags */
flags_struct flags;

/* Variables */
int vars_capacity = 0;
int vars_count = 0;
var_struct *vars = NULL;

/* Programs */
int prgms_capacity = 0;
int prgms_count = 0;
prgm_struct *prgms = NULL;
int labels_capacity = 0;
int labels_count = 0;
label_struct *labels = NULL;

int current_prgm = -1;
int4 pc;
int prgm_highlight_row = 0;

int varmenu_length;
char varmenu[7];
int varmenu_rows;
int varmenu_row;
int varmenu_labellength[6];
char varmenu_labeltext[6][7];
int varmenu_role;

bool mode_clall;
int (*mode_interruptible)(int) = NULL;
bool mode_stoppable;
bool mode_command_entry;
bool mode_number_entry;
bool mode_alpha_entry;
bool mode_shift;
int mode_appmenu;
int mode_plainmenu;
bool mode_plainmenu_sticky;
int mode_transientmenu;
int mode_alphamenu;
int mode_commandmenu;
bool mode_running;
bool mode_getkey;
bool mode_pause = false;
bool mode_disable_stack_lift; /* transient */
bool mode_varmenu;
bool mode_updown;
int4 mode_sigma_reg;
int mode_goose;
bool mode_time_clktd;
bool mode_time_clk24;

phloat entered_number;
int entered_string_length;
char entered_string[15];

int pending_command;
arg_struct pending_command_arg;
int xeq_invisible;

/* Multi-keystroke commands -- edit state */
/* Relevant when mode_command_entry != 0 */
int incomplete_command;
int incomplete_ind;
int incomplete_alpha;
int incomplete_length;
int incomplete_maxdigits;
int incomplete_argtype;
int incomplete_num;
char incomplete_str[7];
int4 incomplete_saved_pc;
int4 incomplete_saved_highlight_row;

/* Command line handling temporaries */
char cmdline[100];
int cmdline_length;
int cmdline_row;

/* Matrix editor / matrix indexing */
int matedit_mode; /* 0=off, 1=index, 2=edit, 3=editn */
char matedit_name[7];
int matedit_length;
vartype *matedit_x;
int4 matedit_i;
int4 matedit_j;
int matedit_prev_appmenu;

/* INPUT */
char input_name[11];
int input_length;
arg_struct input_arg;

/* BASE application */
int baseapp = 0;

/* Random number generator */
int8 random_number_low, random_number_high;

/* NORM & TRACE mode: number waiting to be printed */
int deferred_print = 0;

/* Keystroke buffer - holds keystrokes received while
 * there is a program running.
 */
int keybuf_head = 0;
int keybuf_tail = 0;
int keybuf[16];

int remove_program_catalog = 0;

int state_file_number_format;

/* No user interaction: we keep track of whether or not the user
 * has pressed any keys since powering up, and we don't allow
 * programmatic OFF until they have. The reason is that we want
 * to prevent nastiness like
 *
 *   LBL "YIKES"  SF 11  OFF  GTO "YIKES"
 *
 * from locking the user out.
 */
bool no_keystrokes_yet;


/*******************/
/* Private globals */
/*******************/

static bool state_bool_is_int;

#define MAX_RTNS 8
static int rtn_sp = 0;
static int rtn_prgm[MAX_RTNS];
static int4 rtn_pc[MAX_RTNS];

#ifdef IPHONE
/* For iPhone, we disable OFF by default, to satisfy App Store
 * policy, but we allow users to enable it using a magic value
 * in the X register. This flag determines OFF behavior.
 */
static bool off_enable_flag = false;
#endif

typedef struct {
    int type;
    int4 rows;
    int4 columns;
} matrix_persister;

static int array_count;
static int array_list_capacity;
static void **array_list;


static bool read_int(int *n);
static bool write_int(int n);
static bool read_int4(int4 *n);
static bool write_int4(int4 n);
static bool read_bool(bool *n);
static bool write_bool(bool n);

static bool array_list_grow();
static int array_list_search(void *array);
static bool persist_vartype(vartype *v);
static bool unpersist_vartype(vartype **v, bool padded);
static void update_label_table(int prgm, int4 pc, int inserted);
static void invalidate_lclbls(int prgm_index);
static int pc_line_convert(int4 loc, int loc_is_pc);
static bool convert_programs();
#ifdef BCD_MATH
static void update_decimal_in_programs();
#endif

#ifdef BCD_MATH
#define bin_dec_mode_switch() ( state_file_number_format == NUMBER_FORMAT_BINARY )
#else
#define bin_dec_mode_switch() ( state_file_number_format != NUMBER_FORMAT_BINARY )
#endif


static bool array_list_grow() {
    if (array_count < array_list_capacity)
        return true;
    array_list_capacity += 10;
    void **p = (void **) realloc(array_list,
                                 array_list_capacity * sizeof(void *));
    if (p == NULL)
        return false;
    array_list = p;
    return true;
}

static int array_list_search(void *array) {
    for (int i = 0; i < array_count; i++)
        if (array_list[i] == array)
            return i;
    return -1;
}

static bool persist_vartype(vartype *v) {
    if (v == NULL) {
        int type = TYPE_NULL;
        return shell_write_saved_state(&type, sizeof(int));
    }
    switch (v->type) {
        case TYPE_REAL: {
            if (!shell_write_saved_state(v, sizeof(int)))
                return false;
            vartype_real *r = (vartype_real *) v;
            return shell_write_saved_state(&r->x, sizeof(phloat));
        }
        case TYPE_COMPLEX: {
            if (!shell_write_saved_state(v, sizeof(int)))
                return false;
            vartype_complex *c = (vartype_complex *) v;
            return shell_write_saved_state(&c->re, 2 * sizeof(phloat));
        }
        case TYPE_STRING:
            return shell_write_saved_state(v, sizeof(vartype_string));
        case TYPE_REALMATRIX: {
            matrix_persister mp;
            vartype_realmatrix *rm = (vartype_realmatrix *) v;
            mp.type = rm->type;
            mp.rows = rm->rows;
            mp.columns = rm->columns;
            int4 size = mp.rows * mp.columns;
            bool must_write = true;
            if (rm->array->refcount > 1) {
                int n = array_list_search(rm->array);
                if (n == -1) {
                    // A negative row count signals a new shared matrix
                    mp.rows = -mp.rows;
                    if (!array_list_grow())
                        return false;
                    array_list[array_count++] = rm->array;
                } else {
                    // A zero row count means this matrix shares its data
                    // with a previously written matrix
                    mp.rows = 0;
                    mp.columns = n;
                    must_write = false;
                }
            }
            if (!shell_write_saved_state(&mp, sizeof(matrix_persister)))
                return false;
            if (must_write) {
                if (!shell_write_saved_state(rm->array->data,
                                            size * sizeof(phloat)))
                    return false;
                if (!shell_write_saved_state(rm->array->is_string, size))
                    return false;
            }
            return true;
        }
        case TYPE_COMPLEXMATRIX: {
            matrix_persister mp;
            vartype_complexmatrix *cm = (vartype_complexmatrix *) v;
            mp.type = cm->type;
            mp.rows = cm->rows;
            mp.columns = cm->columns;
            int4 size = mp.rows * mp.columns;
            bool must_write = true;
            if (cm->array->refcount > 1) {
                int n = array_list_search(cm->array);
                if (n == -1) {
                    // A negative row count signals a new shared matrix
                    mp.rows = -mp.rows;
                    if (!array_list_grow())
                        return false;
                    array_list[array_count++] = cm->array;
                } else {
                    // A zero row count means this matrix shares its data
                    // with a previously written matrix
                    mp.rows = 0;
                    mp.columns = n;
                    must_write = false;
                }
            }
            if (!shell_write_saved_state(&mp, sizeof(matrix_persister)))
                return false;
            if (must_write) {
                if (!shell_write_saved_state(cm->array->data,
                                            2 * size * sizeof(phloat)))
                    return false;
            }
            return true;
        }
        default:
            /* Should not happen */
            return true;
    }
}


// A few declarations to help unpersist_vartype get the offsets right,
// in the case it needs to convert the state file.

struct fake_bcd {
    char data[16];
};

static bool unpersist_vartype(vartype **v, bool padded) {
    int type;
    if (shell_read_saved_state(&type, sizeof(int)) != sizeof(int))
        return false;
    switch (type) {
        case TYPE_NULL: {
            *v = NULL;
            return true;
        }
        case TYPE_REAL: {
            vartype_real *r = (vartype_real *) new_real(0);
            if (r == NULL)
                return false;
            if (bin_dec_mode_switch()) {
                #ifdef BCD_MATH
                    if (padded) {
                        int4 dummy;
                        if (shell_read_saved_state(&dummy, 4) != 4) {
                            free_vartype((vartype *) r);
                            return false;
                        }
                    }
                    double x;
                    if (shell_read_saved_state(&x, 8) != 8) {
                        free_vartype((vartype *) r);
                        return false;
                    }
                    r->x = x;
                #else
                    BID_UINT128 x;
                    if (shell_read_saved_state(&x, 16) != 16) {
                        free_vartype((vartype *) r);
                        return false;
                    }
                    r->x = decimal2double(&x);
                #endif
            } else {
                #ifndef BCD_MATH
                    if (padded) {
                        int4 dummy;
                        if (shell_read_saved_state(&dummy, 4) != 4) {
                            free_vartype((vartype *) r);
                            return false;
                        }
                    }
                #endif
                if (shell_read_saved_state(&r->x, sizeof(phloat))
                        != sizeof(phloat)) {
                    free_vartype((vartype *) r);
                    return false;
                }
                #ifdef BCD_MATH
                    update_decimal(&r->x.val);
                #endif
            }
            *v = (vartype *) r;
            return true;
        }
        case TYPE_COMPLEX: {
            vartype_complex *c = (vartype_complex *) new_complex(0, 0);
            if (c == NULL)
                return false;
            if (bin_dec_mode_switch()) {
                #ifdef BCD_MATH
                    if (padded) {
                        int4 dummy;
                        if (shell_read_saved_state(&dummy, 4) != 4) {
                            free_vartype((vartype *) c);
                            return false;
                        }
                    }
                    double parts[2];
                    if (shell_read_saved_state(parts, 16) != 16) {
                        free_vartype((vartype *) c);
                        return false;
                    }
                    c->re = parts[0];
                    c->im = parts[1];
                #else
                    BID_UINT128 parts[2];
                    if (shell_read_saved_state(parts, 32) != 32) {
                        free_vartype((vartype *) c);
                        return false;
                    }
                    c->re = decimal2double(parts);
                    c->im = decimal2double(parts + 1);
                #endif
            } else {
                #ifndef BCD_MATH
                    if (padded) {
                        int4 dummy;
                        if (shell_read_saved_state(&dummy, 4) != 4) {
                            free_vartype((vartype *) c);
                            return false;
                        }
                    }
                #endif
                if (shell_read_saved_state(&c->re, 2 * sizeof(phloat))
                        != 2 * sizeof(phloat)) {
                    free_vartype((vartype *) c);
                    return false;
                }
                #ifdef BCD_MATH
                    update_decimal(&c->re.val);
                    update_decimal(&c->im.val);
                #endif
            }
            *v = (vartype *) c;
            return true;
        }
        case TYPE_STRING: {
            vartype_string *s = (vartype_string *) new_string("", 0);
            int n = sizeof(vartype_string) - sizeof(int);
            if (s == NULL)
                return false;
            if (shell_read_saved_state(&s->type + 1, n) != n) {
                free_vartype((vartype *) s);
                return false;
            } else {
                *v = (vartype *) s;
                return true;
            }
        }
        case TYPE_REALMATRIX: {
            matrix_persister mp;
            int n = sizeof(matrix_persister) - sizeof(int);
            if (shell_read_saved_state(&mp.type + 1, n) != n)
                return false;
            if (mp.rows == 0) {
                // Shared matrix
                vartype *m = new_matrix_alias((vartype *) array_list[mp.columns]);
                if (m == NULL)
                    return false;
                else {
                    *v = m;
                    return true;
                }
            }
            bool shared = mp.rows < 0;
            if (shared)
                mp.rows = -mp.rows;
            vartype_realmatrix *rm = (vartype_realmatrix *) new_realmatrix(mp.rows, mp.columns);
            if (rm == NULL)
                return false;
            if (bin_dec_mode_switch()) {
                int4 size = mp.rows * mp.columns;
                #ifdef BCD_MATH
                    int phsz = sizeof(double);
                #else
                    int phsz = sizeof(fake_bcd);
                #endif
                int4 tsz = size * phsz;
                char *temp = (char *) malloc(tsz);
                if (temp == NULL) {
                    free_vartype((vartype *) rm);
                    return false;
                }
                if (shell_read_saved_state(temp, tsz) != tsz) {
                    free(temp);
                    free_vartype((vartype *) rm);
                    return false;
                }
                if (shell_read_saved_state(rm->array->is_string, size) != size) {
                    free(temp);
                    free_vartype((vartype *) rm);
                    return false;
                }
                #ifdef BCD_MATH
                    for (int4 i = 0; i < size; i++) {
                        if (rm->array->is_string[i]) {
                            char *src = temp + i * phsz;
                            char *dst = (char *) (rm->array->data + i);
                            for (int j = 0; j < 7; j++)
                                *dst++ = *src++;
                        } else {
                            rm->array->data[i] = ((double *) temp)[i];
                        }
                    }
                #else
                    for (int4 i = 0; i < size; i++) {
                        if (rm->array->is_string[i]) {
                            char *src = temp + i * phsz;
                            char *dst = (char *) (rm->array->data + i);
                            for (int j = 0; j < 7; j++)
                                *dst++ = *src++;
                        } else {
                            rm->array->data[i] = decimal2double((char *) (temp + phsz * i));
                        }
                    }
                #endif
                free(temp);
            } else {
                int4 size = mp.rows * mp.columns * sizeof(phloat);
                if (shell_read_saved_state(rm->array->data, size) != size) {
                    free_vartype((vartype *) rm);
                    return false;
                }
                size = mp.rows * mp.columns;
                if (shell_read_saved_state(rm->array->is_string, size) != size) {
                    free_vartype((vartype *) rm);
                    return false;
                }
                #ifdef BCD_MATH
                    if (state_file_number_format != NUMBER_FORMAT_BID128)
                        for (int4 i = 0; i < size; i++)
                            if (!rm->array->is_string[i])
                                update_decimal(&rm->array->data[i].val);
                #endif
            }
            if (shared) {
                if (!array_list_grow()) {
                    free_vartype((vartype *) rm);
                    return false;
                }
                array_list[array_count++] = rm;
            }
            *v = (vartype *) rm;
            return true;
        }
        case TYPE_COMPLEXMATRIX: {
            matrix_persister mp;
            int n = sizeof(matrix_persister) - sizeof(int);
            if (shell_read_saved_state(&mp.type + 1, n) != n)
                return false;
            if (mp.rows == 0) {
                // Shared matrix
                vartype *m = new_matrix_alias((vartype *) array_list[mp.columns]);
                if (m == NULL)
                    return false;
                else {
                    *v = m;
                    return true;
                }
            }
            bool shared = mp.rows < 0;
            if (shared)
                mp.rows = -mp.rows;
            vartype_complexmatrix *cm = (vartype_complexmatrix *)
                                        new_complexmatrix(mp.rows, mp.columns);
            if (cm == NULL)
                return false;
            if (bin_dec_mode_switch()) {
                int4 size = 2 * mp.rows * mp.columns;
                for (int4 i = 0; i < size; i++)
                    if (!read_phloat(cm->array->data + i)) {
                        free_vartype((vartype *) cm);
                        return false;
                    }
            } else {
                int4 size = 2 * mp.rows * mp.columns * sizeof(phloat);
                if (shell_read_saved_state(cm->array->data, size) != size) {
                    free_vartype((vartype *) cm);
                    return false;
                }
                #ifdef BCD_MATH
                    if (state_file_number_format != NUMBER_FORMAT_BID128) {
                        size = mp.rows * mp.columns;
                        for (int4 i = 0; i < size; i++)
                            update_decimal(&cm->array->data[i].val);
                    }
                #endif
            }
            if (shared) {
                if (!array_list_grow()) {
                    free_vartype((vartype *) cm);
                    return false;
                }
                array_list[array_count++] = cm;
            }
            *v = (vartype *) cm;
            return true;
        }
        default:
            return false;
    }
}

static bool persist_globals() {
    int i;
    array_count = 0;
    array_list_capacity = 0;
    array_list = NULL;
    bool ret = false;

    if (!persist_vartype(reg_x))
        goto done;
    if (!persist_vartype(reg_y))
        goto done;
    if (!persist_vartype(reg_z))
        goto done;
    if (!persist_vartype(reg_t))
        goto done;
    if (!persist_vartype(reg_lastx))
        goto done;
    if (!write_int(reg_alpha_length))
        goto done;
    if (!shell_write_saved_state(reg_alpha, 44))
        goto done;
    if (!write_int4(mode_sigma_reg))
        goto done;
    if (!write_int(mode_goose))
        goto done;
    if (!write_bool(mode_time_clktd))
        goto done;
    if (!write_bool(mode_time_clk24))
        goto done;
    if (!shell_write_saved_state(&flags, sizeof(flags_struct)))
        goto done;
    if (!write_int(vars_count))
        goto done;
    for (i = 0; i < vars_count; i++)
        if (!shell_write_saved_state(vars + i, sizeof(var_struct_32bit)))
            goto done;
    for (i = 0; i < vars_count; i++)
        if (!persist_vartype(vars[i].value))
            goto done;
    if (!write_int(prgms_count))
        goto done;
    for (i = 0; i < prgms_count; i++)
        if (!shell_write_saved_state(prgms + i, sizeof(prgm_struct_32bit)))
            goto done;
    for (i = 0; i < prgms_count; i++)
        if (!shell_write_saved_state(prgms[i].text, prgms[i].size))
            goto done;
    if (!write_int(current_prgm))
        goto done;
    if (!write_int4(pc))
        goto done;
    if (!write_int(prgm_highlight_row))
        goto done;
    if (!write_int(varmenu_length))
        goto done;
    if (!shell_write_saved_state(varmenu, 7))
        goto done;
    if (!write_int(varmenu_rows))
        goto done;
    if (!write_int(varmenu_row))
        goto done;
    if (!shell_write_saved_state(varmenu_labellength, 6 * sizeof(int)))
        goto done;
    if (!shell_write_saved_state(varmenu_labeltext, 42))
        goto done;
    if (!write_int(varmenu_role))
        goto done;
    if (!write_int(rtn_sp))
        goto done;
    if (!shell_write_saved_state(&rtn_prgm, MAX_RTNS * sizeof(int)))
        goto done;
    if (!shell_write_saved_state(&rtn_pc, MAX_RTNS * sizeof(int4)))
        goto done;
#ifdef IPHONE
    if (!write_bool(off_enable_flag))
        goto done;
#endif
    ret = true;

    done:
    free(array_list);
    return ret;
}

static bool unpersist_globals(int4 ver) {
    int i;
    array_count = 0;
    array_list_capacity = 0;
    array_list = NULL;
    bool ret = false;
#ifdef WINDOWS
    bool padded = ver < 18;
#else
    bool padded = false;
#endif
    char tmp_dmy = 2;

    free_vartype(reg_x);
    if (!unpersist_vartype(&reg_x, padded))
        goto done;
    free_vartype(reg_y);
    if (!unpersist_vartype(&reg_y, padded))
        goto done;
    free_vartype(reg_z);
    if (!unpersist_vartype(&reg_z, padded))
        goto done;
    free_vartype(reg_t);
    if (!unpersist_vartype(&reg_t, padded))
        goto done;
    free_vartype(reg_lastx);
    if (!unpersist_vartype(&reg_lastx, padded))
        goto done;

    if (ver >= 12 && ver < 20) {
        /* BIGSTACK -- obsolete */
        bool bigstack;
        if (!read_bool(&bigstack))
            goto done;
    }    
    
    if (!read_int(&reg_alpha_length)) {
        reg_alpha_length = 0;
        goto done;
    }
    if (shell_read_saved_state(reg_alpha, 44) != 44) {
        reg_alpha_length = 0;
        goto done;
    }
    if (!read_int4(&mode_sigma_reg)) {
        mode_sigma_reg = 11;
        goto done;
    }
    if (!read_int(&mode_goose)) {
        mode_goose = -1;
        goto done;
    }
    if (ver >= 16) {
        if (!read_bool(&mode_time_clktd)) {
            mode_time_clktd = false;
            goto done;
        }
        if (!read_bool(&mode_time_clk24)) {
            mode_time_clk24 = false;
            goto done;
        }
        if (ver < 19) {
            bool dmy;
            if (!read_bool(&dmy))
                goto done;
            tmp_dmy = dmy ? 1 : 0;
        }
    }
    if (shell_read_saved_state(&flags, sizeof(flags_struct))
            != sizeof(flags_struct))
        goto done;
    if (tmp_dmy != 2)
        flags.f.dmy = tmp_dmy;
    vars_capacity = 0;
    if (vars != NULL) {
        free(vars);
        vars = NULL;
    }
    if (!read_int(&vars_count)) {
        vars_count = 0;
        goto done;
    }
    vars = (var_struct *) malloc(vars_count * sizeof(var_struct));
    if (vars == NULL) {
        vars_count = 0;
        goto done;
    }
    for (i = 0; i < vars_count; i++)
        if (shell_read_saved_state(vars + i, sizeof(var_struct_32bit)) != sizeof(var_struct_32bit)) {
            free(vars);
            vars = NULL;
            vars_count = 0;
            goto done;
        }
    vars_capacity = vars_count;
    for (i = 0; i < vars_count; i++)
        vars[i].value = NULL;
    for (i = 0; i < vars_count; i++)
        if (!unpersist_vartype(&vars[i].value, padded)) {
            purge_all_vars();
            goto done;
        }

    // Purging zero-length var that may have been created by buggy INTEG
    purge_var("", 0);

    prgms_capacity = 0;
    if (prgms != NULL) {
        free(prgms);
        prgms = NULL;
    }
    if (!read_int(&prgms_count)) {
        prgms_count = 0;
        goto done;
    }
    prgms = (prgm_struct *) malloc(prgms_count * sizeof(prgm_struct));
    if (prgms == NULL) {
        prgms_count = 0;
        goto done;
    }
    for (i = 0; i < prgms_count; i++)
        if (shell_read_saved_state(prgms + i, sizeof(prgm_struct_32bit)) != sizeof(prgm_struct_32bit)) {
            free(prgms);
            prgms = NULL;
            prgms_count = 0;
            goto done;
        }
    prgms_capacity = prgms_count;
    for (i = 0; i < prgms_count; i++) {
        prgms[i].capacity = prgms[i].size;
        prgms[i].text = (unsigned char *) malloc(prgms[i].size);
        // TODO - handle memory allocation failure
    }
    for (i = 0; i < prgms_count; i++) {
        if (shell_read_saved_state(prgms[i].text, prgms[i].size)
                != prgms[i].size) {
            clear_all_prgms();
            goto done;
        }
    }
    if (!read_int(&current_prgm)) {
        current_prgm = 0;
        goto done;
    }
    if (!read_int4(&pc)) {
        pc = -1;
        goto done;
    }
    if (!read_int(&prgm_highlight_row)) {
        prgm_highlight_row = 0;
        goto done;
    }
    if (!read_int(&varmenu_length)) {
        varmenu_length = 0;
        goto done;
    }
    if (shell_read_saved_state(varmenu, 7) != 7) {
        varmenu_length = 0;
        goto done;
    }
    if (!read_int(&varmenu_rows)) {
        varmenu_length = 0;
        goto done;
    }
    if (!read_int(&varmenu_row)) {
        varmenu_length = 0;
        goto done;
    }
    if (shell_read_saved_state(varmenu_labellength, 6 * sizeof(int))
            != 6 * sizeof(int))
        goto done;
    if (shell_read_saved_state(varmenu_labeltext, 42) != 42)
        goto done;
    if (!read_int(&varmenu_role))
        goto done;
    if (!read_int(&rtn_sp))
        goto done;
    if (shell_read_saved_state(rtn_prgm, MAX_RTNS * sizeof(int))
            != MAX_RTNS * sizeof(int))
        goto done;
    if (shell_read_saved_state(rtn_pc, MAX_RTNS * sizeof(int4))
            != MAX_RTNS * sizeof(int4))
        goto done;
#ifdef IPHONE
    if (ver >= 17)
        if (!read_bool(&off_enable_flag))
            goto done;
#endif

    if (bin_dec_mode_switch())
        if (!convert_programs()) {
            clear_all_prgms();
            goto done;
        }

#ifdef BCD_MATH
    if (state_file_number_format == NUMBER_FORMAT_BCD20_OLD
            || state_file_number_format == NUMBER_FORMAT_BCD20_NEW)
        update_decimal_in_programs();
#endif

    rebuild_label_table();
    ret = true;

    done:
    free(array_list);
    return ret;
}

void clear_all_prgms() {
    if (prgms != NULL) {
        int i;
        for (i = 0; i < prgms_count; i++)
            if (prgms[i].text != NULL)
                free(prgms[i].text);
        free(prgms);
    }
    prgms = NULL;
    prgms_capacity = 0;
    prgms_count = 0;
    if (labels != NULL)
        free(labels);
    labels = NULL;
    labels_capacity = 0;
    labels_count = 0;
}

int clear_prgm(const arg_struct *arg) {
    int prgm_index;
    if (arg->type == ARGTYPE_LBLINDEX)
        prgm_index = labels[arg->val.num].prgm;
    else if (arg->type == ARGTYPE_STR) {
        if (arg->length == 0) {
            if (current_prgm < 0 || current_prgm >= prgms_count)
                return ERR_INTERNAL_ERROR;
            prgm_index = current_prgm;
        } else {
            int i;
            for (i = labels_count - 1; i >= 0; i--)
                if (string_equals(arg->val.text, arg->length,
                                 labels[i].name, labels[i].length))
                    goto found;
            return ERR_LABEL_NOT_FOUND;
            found:
            prgm_index = labels[i].prgm;
        }
    }
    return clear_prgm_by_index(prgm_index);
}

int clear_prgm_by_index(int prgm_index) {
    int i, j;
    if (prgm_index < 0 || prgm_index >= prgms_count)
        return ERR_LABEL_NOT_FOUND;
    clear_all_rtns();
    if (prgm_index == current_prgm)
        pc = -1;
    else if (current_prgm > prgm_index)
        current_prgm--;
    free(prgms[prgm_index].text);
    for (i = prgm_index; i < prgms_count - 1; i++)
        prgms[i] = prgms[i + 1];
    prgms_count--;
    i = j = 0;
    while (j < labels_count) {
        if (j > i)
            labels[i] = labels[j];
        j++;
        if (labels[i].prgm > prgm_index) {
            labels[i].prgm--;
            i++;
        } else if (labels[i].prgm < prgm_index)
            i++;
    }
    labels_count = i;
    if (prgms_count == 0 || prgm_index == prgms_count) {
        int saved_prgm = current_prgm;
        int saved_pc = pc;
        goto_dot_dot();
        current_prgm = saved_prgm;
        pc = saved_pc;
    }
    update_catalog();
    return ERR_NONE;
}

void clear_prgm_lines(int4 count) {
    int4 frompc, deleted, i, j;
    if (pc == -1)
        pc = 0;
    frompc = pc;
    while (count > 0) {
        int command;
        arg_struct arg;
        get_next_command(&pc, &command, &arg, 0);
        if (command == CMD_END) {
            pc -= 2;
            break;
        }
        count--;
    }
    deleted = pc - frompc;

    for (i = pc; i < prgms[current_prgm].size; i++)
        prgms[current_prgm].text[i - deleted] = prgms[current_prgm].text[i];
    prgms[current_prgm].size -= deleted;
    pc = frompc;

    i = j = 0;
    while (j < labels_count) {
        if (j > i)
            labels[i] = labels[j];
        j++;
        if (labels[i].prgm == current_prgm) {
            if (labels[i].pc < frompc)
                i++;
            else if (labels[i].pc >= frompc + deleted) {
                labels[i].pc -= deleted;
                i++;
            }
        } else
            i++;
    }
    labels_count = i;

    invalidate_lclbls(current_prgm);
    clear_all_rtns();
}

void goto_dot_dot() {
    int command;
    arg_struct arg;
    if (prgms_count != 0) {
        /* Check if last program is empty */
        pc = 0;
        current_prgm = prgms_count - 1;
        get_next_command(&pc, &command, &arg, 0);
        if (command == CMD_END) {
            pc = -1;
            return;
        }
    }
    if (prgms_count == prgms_capacity) {
        prgm_struct *newprgms;
        int i;
        prgms_capacity += 10;
        newprgms = (prgm_struct *) malloc(prgms_capacity * sizeof(prgm_struct));
        // TODO - handle memory allocation failure
        for (i = 0; i < prgms_count; i++)
            newprgms[i] = prgms[i];
        if (prgms != NULL)
            free(prgms);
        prgms = newprgms;
    }
    current_prgm = prgms_count++;
    prgms[current_prgm].capacity = 0;
    prgms[current_prgm].size = 0;
    prgms[current_prgm].lclbl_invalid = 1;
    prgms[current_prgm].text = NULL;
    command = CMD_END;
    arg.type = ARGTYPE_NONE;
    store_command(0, command, &arg);
    pc = -1;
}

int mvar_prgms_exist() {
    int i;
    for (i = 0; i < labels_count; i++)
        if (label_has_mvar(i))
            return 1;
    return 0;
}

int label_has_mvar(int lblindex) {
    int saved_prgm;
    int4 pc;
    int command;
    arg_struct arg;
    if (labels[lblindex].length == 0)
        return 0;
    saved_prgm = current_prgm;
    current_prgm = labels[lblindex].prgm;
    pc = labels[lblindex].pc;
    pc += get_command_length(current_prgm, pc);
    get_next_command(&pc, &command, &arg, 0);
    current_prgm = saved_prgm;
    return command == CMD_MVAR;
}

int get_command_length(int prgm_index, int4 pc) {
    prgm_struct *prgm = prgms + prgm_index;
    int4 pc2 = pc;
    int command = prgm->text[pc2++];
    int argtype = prgm->text[pc2++];
    command |= (argtype & 240) << 4;
    argtype &= 15;

    if ((command == CMD_GTO || command == CMD_XEQ)
            && (argtype == ARGTYPE_NUM || argtype == ARGTYPE_LCLBL))
        pc2 += 4;
    switch (argtype) {
        case ARGTYPE_NUM:
        case ARGTYPE_NEG_NUM:
        case ARGTYPE_IND_NUM: {
            while ((prgm->text[pc2++] & 128) == 0);
            break;
        }
        case ARGTYPE_STK:
        case ARGTYPE_IND_STK:
        case ARGTYPE_COMMAND:
        case ARGTYPE_LCLBL:
            pc2++;
            break;
        case ARGTYPE_STR:
        case ARGTYPE_IND_STR: {
            pc2 += prgm->text[pc2] + 1;
            break;
        }
        case ARGTYPE_DOUBLE:
            pc2 += sizeof(phloat);
            break;
    }
    return pc2 - pc;
}

void get_next_command(int4 *pc, int *command, arg_struct *arg, int find_target){
    prgm_struct *prgm = prgms + current_prgm;
    int i;
    int4 target_pc;
    int4 orig_pc = *pc;

    *command = prgm->text[(*pc)++];
    arg->type = prgm->text[(*pc)++];
    *command |= (arg->type & 240) << 4;
    arg->type &= 15;

    if ((*command == CMD_GTO || *command == CMD_XEQ)
            && (arg->type == ARGTYPE_NUM
                || arg->type == ARGTYPE_LCLBL)) {
        if (find_target) {
            target_pc = 0;
            for (i = 0; i < 4; i++)
                target_pc = (target_pc << 8) | prgm->text[(*pc)++];
            if (target_pc != -1) {
                arg->target = target_pc;
                find_target = 0;
            }
        } else
            (*pc) += 4;
    } else {
        find_target = 0;
        arg->target = -1;
    }

    switch (arg->type) {
        case ARGTYPE_NUM:
        case ARGTYPE_NEG_NUM:
        case ARGTYPE_IND_NUM: {
            int4 num = 0;
            unsigned char c;
            do {
                c = prgm->text[(*pc)++];
                num = (num << 7) | (c & 127);
            } while ((c & 128) == 0);
            if (arg->type == ARGTYPE_NEG_NUM) {
                arg->type = ARGTYPE_NUM;
                num = -num;
            }
            arg->val.num = num;
            break;
        }
        case ARGTYPE_STK:
        case ARGTYPE_IND_STK:
            arg->val.stk = prgm->text[(*pc)++];
            break;
        case ARGTYPE_COMMAND:
            arg->val.cmd = prgm->text[(*pc)++];
            break;
        case ARGTYPE_LCLBL:
            arg->val.lclbl = prgm->text[(*pc)++];
            break;
        case ARGTYPE_STR:
        case ARGTYPE_IND_STR: {
            arg->length = prgm->text[(*pc)++];
            for (i = 0; i < arg->length; i++)
                arg->val.text[i] = prgm->text[(*pc)++];
            break;
        }
        case ARGTYPE_DOUBLE: {
            unsigned char *b = (unsigned char *) &arg->val_d;
            for (int i = 0; i < (int) sizeof(phloat); i++)
                *b++ = prgm->text[(*pc)++];
            break;
        }
    }

    if (*command == CMD_NUMBER && arg->type != ARGTYPE_DOUBLE) {
        /* argtype is ARGTYPE_NUM; convert to phloat */
        arg->val_d = arg->val.num;
        arg->type = ARGTYPE_DOUBLE;
    }
    
    if (find_target) {
        target_pc = find_local_label(arg);
        arg->target = target_pc;
        for (i = 5; i >= 2; i--) {
            prgm->text[orig_pc + i] = target_pc;
            target_pc >>= 8;
        }
        prgm->lclbl_invalid = 0;
    }
}

void rebuild_label_table() {
    /* TODO -- this is *not* efficient; inserting and deleting ENDs and
     * global LBLs should not cause every single program to get rescanned!
     * But, I don't feel like dealing with that at the moment, so just
     * this ugly brute force approach for now.
     */
    int prgm_index;
    int4 pc;
    labels_count = 0;
    for (prgm_index = 0; prgm_index < prgms_count; prgm_index++) {
        prgm_struct *prgm = prgms + prgm_index;
        pc = 0;
        while (pc < prgm->size) {
            int command = prgm->text[pc];
            int argtype = prgm->text[pc + 1];
            command |= (argtype & 240) << 4;
            argtype &= 15;

            if (command == CMD_END
                        || (command == CMD_LBL && argtype == ARGTYPE_STR)) {
                label_struct *newlabel;
                if (labels_count == labels_capacity) {
                    label_struct *newlabels;
                    int i;
                    labels_capacity += 50;
                    newlabels = (label_struct *)
                                malloc(labels_capacity * sizeof(label_struct));
                    // TODO - handle memory allocation failure
                    for (i = 0; i < labels_count; i++)
                        newlabels[i] = labels[i];
                    if (labels != NULL)
                        free(labels);
                    labels = newlabels;
                }
                newlabel = labels + labels_count++;
                if (command == CMD_END)
                    newlabel->length = 0;
                else {
                    int i;
                    newlabel->length = prgm->text[pc + 2];
                    for (i = 0; i < newlabel->length; i++)
                        newlabel->name[i] = prgm->text[pc + 3 + i];
                }
                newlabel->prgm = prgm_index;
                newlabel->pc = pc;
            }
            pc += get_command_length(prgm_index, pc);
        }
    }
}

static void update_label_table(int prgm, int4 pc, int inserted) {
    int i;
    for (i = 0; i < labels_count; i++) {
        if (labels[i].prgm > prgm)
            return;
        if (labels[i].prgm == prgm && labels[i].pc >= pc)
            labels[i].pc += inserted;
    }
}

static void invalidate_lclbls(int prgm_index) {
    prgm_struct *prgm = prgms + prgm_index;
    if (!prgm->lclbl_invalid) {
        int4 pc2 = 0;
        while (pc2 < prgm->size) {
            int command = prgm->text[pc2];
            int argtype = prgm->text[pc2 + 1];
            command |= (argtype & 240) << 4;
            argtype &= 15;
            if ((command == CMD_GTO || command == CMD_XEQ)
                    && (argtype == ARGTYPE_NUM || argtype == ARGTYPE_LCLBL)) {
                /* A dest_pc value of -1 signals 'unknown',
                 * -2 means 'nonexistent', and anything else is
                 * the pc where the destination label is found.
                 */
                int4 pos;
                for (pos = pc2 + 2; pos < pc2 + 6; pos++)
                    prgm->text[pos] = 255;
            }
            pc2 += get_command_length(prgm_index, pc2);
        }
        prgm->lclbl_invalid = 1;
    }
}

void delete_command(int4 pc) {
    prgm_struct *prgm = prgms + current_prgm;
    int command = prgm->text[pc];
    int argtype = prgm->text[pc + 1];
    int length = get_command_length(current_prgm, pc);
    int4 pos;

    command |= (argtype & 240) << 4;
    argtype &= 15;

    if (command == CMD_END) {
        int4 newsize;
        prgm_struct *nextprgm;
        if (current_prgm == prgms_count - 1)
            /* Don't allow deletion of last program's END. */
            return;
        nextprgm = prgm + 1;
        prgm->size -= 2;
        newsize = prgm->size + nextprgm->size;
        if (newsize > prgm->capacity) {
            int4 newcapacity = (newsize + 511) & ~511;
            unsigned char *newtext = (unsigned char *) malloc(newcapacity);
            // TODO - handle memory allocation failure
            for (pos = 0; pos < prgm->size; pos++)
                newtext[pos] = prgm->text[pos];
            free(prgm->text);
            prgm->text = newtext;
            prgm->capacity = newcapacity;
        }
        for (pos = 0; pos < nextprgm->size; pos++)
            prgm->text[prgm->size++] = nextprgm->text[pos];
        free(nextprgm->text);
        for (pos = current_prgm + 1; pos < prgms_count - 1; pos++)
            prgms[pos] = prgms[pos + 1];
        prgms_count--;
        rebuild_label_table();
        invalidate_lclbls(current_prgm);
        clear_all_rtns();
        draw_varmenu();
        return;
    }

    for (pos = pc; pos < prgm->size - length; pos++)
        prgm->text[pos] = prgm->text[pos + length];
    prgm->size -= length;
    if (command == CMD_LBL && argtype == ARGTYPE_STR)
        rebuild_label_table();
    else
        update_label_table(current_prgm, pc, -length);
    invalidate_lclbls(current_prgm);
    clear_all_rtns();
    draw_varmenu();
}

void store_command(int4 pc, int command, arg_struct *arg) {
    unsigned char buf[100];
    int bufptr = 0;
    int i;
    int4 pos;
    prgm_struct *prgm = prgms + current_prgm;

    /* We should never be called with pc = -1, but just to be safe... */
    if (pc == -1)
        pc = 0;

    if (arg->type == ARGTYPE_NUM && arg->val.num < 0) {
        arg->type = ARGTYPE_NEG_NUM;
        arg->val.num = -arg->val.num;
    } else if (command == CMD_NUMBER) {
        /* arg.type is always ARGTYPE_DOUBLE for CMD_NUMBER, but for storage
         * efficiency, we handle integers specially and store them as
         * ARGTYPE_NUM or ARGTYPE_NEG_NUM instead.
         */
        int4 n = to_int4(arg->val_d);
        if (n == arg->val_d && n != (int4) 0x80000000) {
            if (n >= 0) {
                arg->val.num = n;
                arg->type = ARGTYPE_NUM;
            } else {
                arg->val.num = -n;
                arg->type = ARGTYPE_NEG_NUM;
            }
        }
    } else if (arg->type == ARGTYPE_LBLINDEX) {
        int li = arg->val.num;
        arg->length = labels[li].length;
        for (i = 0; i < arg->length; i++)
            arg->val.text[i] = labels[li].name[i];
        arg->type = ARGTYPE_STR;
    }

    buf[bufptr++] = command & 255;
    buf[bufptr++] = arg->type | ((command & ~255) >> 4);

    /* If the program is nonempty, it must already contain an END,
     * since that's the very first thing that gets stored in any new
     * program. In this case, we need to split the program.
     */
    if (command == CMD_END && prgm->size > 0) {
        prgm_struct *new_prgm;
        if (prgms_count == prgms_capacity) {
            prgm_struct *new_prgms;
            int i;
            prgms_capacity += 10;
            new_prgms = (prgm_struct *)
                            malloc(prgms_capacity * sizeof(prgm_struct));
            // TODO - handle memory allocation failure
            for (i = 0; i <= current_prgm; i++)
                new_prgms[i] = prgms[i];
            for (i = current_prgm + 1; i < prgms_count; i++)
                new_prgms[i + 1] = prgms[i];
            free(prgms);
            prgms = new_prgms;
            prgm = prgms + current_prgm;
        } else {
            for (i = prgms_count - 1; i > current_prgm; i--)
                prgms[i + 1] = prgms[i];
        }
        prgms_count++;
        new_prgm = prgm + 1;
        new_prgm->size = prgm->size - pc;
        new_prgm->capacity = (new_prgm->size + 511) & ~511;
        new_prgm->text = (unsigned char *) malloc(new_prgm->capacity);
        // TODO - handle memory allocation failure
        for (i = pc; i < prgm->size; i++)
            new_prgm->text[i - pc] = prgm->text[i];
        current_prgm++;

        /* Truncate the previously 'current' program and append an END.
         * No need to check the size against the capacity and grow the
         * program; since it contained an END before, it still has the
         * capacity for one now;
         */
        prgm->size = pc;
        prgm->text[prgm->size++] = CMD_END;
        prgm->text[prgm->size++] = ARGTYPE_NONE;
        if (flags.f.trace_print || flags.f.normal_print)
            print_program_line(current_prgm - 1, pc);

        rebuild_label_table();
        invalidate_lclbls(current_prgm);
        invalidate_lclbls(current_prgm - 1);
        clear_all_rtns();
        draw_varmenu();
        return;
    }

    if ((command == CMD_GTO || command == CMD_XEQ)
            && (arg->type == ARGTYPE_NUM || arg->type == ARGTYPE_LCLBL))
        for (i = 0; i < 4; i++)
            buf[bufptr++] = 255;
    switch (arg->type) {
        case ARGTYPE_NUM:
        case ARGTYPE_NEG_NUM:
        case ARGTYPE_IND_NUM: {
            int4 num = arg->val.num;
            char tmpbuf[5];
            int tmplen = 0;
            while (num > 127) {
                tmpbuf[tmplen++] = num & 127;
                num >>= 7;
            }
            tmpbuf[tmplen++] = num;
            tmpbuf[0] |= 128;
            while (--tmplen >= 0)
                buf[bufptr++] = tmpbuf[tmplen];
            break;
        }
        case ARGTYPE_STK:
        case ARGTYPE_IND_STK:
            buf[bufptr++] = arg->val.stk;
            break;
        case ARGTYPE_STR:
        case ARGTYPE_IND_STR: {
            buf[bufptr++] = arg->length;
            for (i = 0; i < arg->length; i++)
                buf[bufptr++] = arg->val.text[i];
            break;
        }
        case ARGTYPE_LCLBL:
            buf[bufptr++] = arg->val.lclbl;
            break;
        case ARGTYPE_DOUBLE: {
            unsigned char *b = (unsigned char *) &arg->val_d;
            for (int i = 0; i < (int) sizeof(phloat); i++)
                buf[bufptr++] = *b++;
            break;
        }
    }

    if (bufptr + prgm->size > prgm->capacity) {
        unsigned char *newtext;
        prgm->capacity += 512;
        newtext = (unsigned char *) malloc(prgm->capacity);
        // TODO - handle memory allocation failure
        for (pos = 0; pos < pc; pos++)
            newtext[pos] = prgm->text[pos];
        for (pos = pc; pos < prgm->size; pos++)
            newtext[pos + bufptr] = prgm->text[pos];
        if (prgm->text != NULL)
            free(prgm->text);
        prgm->text = newtext;
    } else {
        for (pos = prgm->size - 1; pos >= pc; pos--)
            prgm->text[pos + bufptr] = prgm->text[pos];
    }
    for (pos = 0; pos < bufptr; pos++)
        prgm->text[pc + pos] = buf[pos];
    prgm->size += bufptr;
    if (command != CMD_END && (flags.f.trace_print || flags.f.normal_print))
        print_program_line(current_prgm, pc);
    
    if (command == CMD_END ||
            (command == CMD_LBL && arg->type == ARGTYPE_STR))
        rebuild_label_table();
    else
        update_label_table(current_prgm, pc, bufptr);
    invalidate_lclbls(current_prgm);
    clear_all_rtns();
    draw_varmenu();
}

void store_command_after(int4 *pc, int command, arg_struct *arg) {
    if (*pc == -1)
        *pc = 0;
    else if (prgms[current_prgm].text[*pc] != CMD_END)
        *pc += get_command_length(current_prgm, *pc);
    store_command(*pc, command, arg);
}

static int pc_line_convert(int4 loc, int loc_is_pc) {
    int4 pc = 0;
    int4 line = 1;
    prgm_struct *prgm = prgms + current_prgm;

    while (1) {
        if (loc_is_pc) {
            if (pc >= loc)
                return line;
        } else {
            if (line >= loc)
                return pc;
        }
        if (prgm->text[pc] == CMD_END)
            return loc_is_pc ? line : pc;
        pc += get_command_length(current_prgm, pc);
        line++;
    }
}

int4 pc2line(int4 pc) {
    if (pc == -1)
        return 0;
    else
        return pc_line_convert(pc, 1);
}

int4 line2pc(int4 line) {
    if (line == 0)
        return -1;
    else
        return pc_line_convert(line, 0);
}

int4 find_local_label(const arg_struct *arg) {
    int4 orig_pc = pc;
    int4 search_pc;
    int wrapped = 0;
    prgm_struct *prgm = prgms + current_prgm;

    if (orig_pc == -1)
        orig_pc = 0;
    search_pc = orig_pc;

    while (!wrapped || search_pc < orig_pc) {
        int command, argtype;
        if (search_pc >= prgm->size - 2) {
            if (orig_pc == 0)
                break;
            search_pc = 0;
            wrapped = 1;
        }
        command = prgm->text[search_pc];
        argtype = prgm->text[search_pc + 1];
        command |= (argtype & 240) << 4;
        argtype &= 15;
        if (command == CMD_LBL && argtype == arg->type) {
            if (argtype == ARGTYPE_NUM) {
                int num = 0;
                unsigned char c;
                int pos = search_pc + 2;
                do {
                    c = prgm->text[pos++];
                    num = (num << 7) | (c & 127);
                } while ((c & 128) == 0);
                if (num == arg->val.num)
                    return search_pc;
            } else {
                char lclbl = prgm->text[search_pc + 2];
                if (lclbl == arg->val.lclbl)
                    return search_pc;
            }
        }
        search_pc += get_command_length(current_prgm, search_pc);
    }

    return -2;
}

int find_global_label(const arg_struct *arg, int *prgm, int4 *pc) {
    int i;
    const char *name = arg->val.text;
    int namelen = arg->length;
    for (i = labels_count - 1; i >= 0; i--) {
        int j;
        char *labelname;
        if (labels[i].length != namelen)
            continue;
        labelname = labels[i].name;
        for (j = 0; j < namelen; j++)
            if (labelname[j] != name[j])
                goto nomatch;
        *prgm = labels[i].prgm;
        *pc = labels[i].pc;
        return 1;
        nomatch:;
    }
    return 0;
}

int push_rtn_addr(int prgm, int4 pc) {
    int err = ERR_NONE;
    if (rtn_sp == MAX_RTNS) {
        int i;
        if (rtn_prgm[0] == -2 || rtn_prgm[0] == -3)
            err = ERR_SOLVE_INTEG_RTN_LOST;
        for (i = 0; i < MAX_RTNS - 1; i++) {
            rtn_prgm[i] = rtn_prgm[i + 1];
            rtn_pc[i] = rtn_pc[i + 1];
        }
        rtn_sp--;
    }
    rtn_prgm[rtn_sp] = prgm;
    rtn_pc[rtn_sp] = pc;
    rtn_sp++;
    return err;
}

void pop_rtn_addr(int *prgm, int4 *pc) {
    if (rtn_sp == 0) {
        *prgm = -1;
        *pc = -1;
    } else {
        rtn_sp--;
        *prgm = rtn_prgm[rtn_sp];
        *pc = rtn_pc[rtn_sp];
    }
}

void clear_all_rtns() {
    rtn_sp = 0;
}

bool solve_active() {
    int i;
    for (i = 0; i < rtn_sp; i++)
        if (rtn_prgm[i] == -2)
            return true;
    return false;
}

bool integ_active() {
    int i;
    for (i = 0; i < rtn_sp; i++)
        if (rtn_prgm[i] == -3)
            return true;
    return false;
}

void unwind_stack_until_solve() {
    while (rtn_prgm[--rtn_sp] != -2);
}

static bool read_int(int *n) {
    return shell_read_saved_state(n, sizeof(int)) == sizeof(int);
}

static bool write_int(int n) {
    return shell_write_saved_state(&n, sizeof(int));
}

static bool read_int4(int4 *n) {
    return shell_read_saved_state(n, sizeof(int4)) == sizeof(int4);
}

static bool write_int4(int4 n) {
    return shell_write_saved_state(&n, sizeof(int4));
}

static bool read_int8(int8 *n) {
    return shell_read_saved_state(n, sizeof(int8)) == sizeof(int8);
}

static bool write_int8(int8 n) {
    return shell_write_saved_state(&n, sizeof(int8));
}

static bool read_bool(bool *b) {
    if (state_bool_is_int) {
        int t;
        if (!read_int(&t))
            return false;
        if (t != 0 && t != 1)
            return false;
        *b = t != 0;
        return true;
    } else {
        return shell_read_saved_state(b, sizeof(bool)) == sizeof(bool);
    }
}

static bool write_bool(bool b) {
    return shell_write_saved_state(&b, sizeof(bool));
}

bool read_phloat(phloat *d) {
    if (bin_dec_mode_switch()) {
        #ifdef BCD_MATH
            double dbl;
            if (shell_read_saved_state(&dbl, sizeof(double)) != sizeof(double))
                return false;
            *d = dbl;
            return true;
        #else
            char data[16];
            if (shell_read_saved_state(data, 16) != 16)
                return false;
            *d = decimal2double(data);
            return true;
        #endif
    } else {
        if (shell_read_saved_state(d, sizeof(phloat)) != sizeof(phloat))
            return false;
        #ifdef BCD_MATH
            update_decimal(&d->val);
        #endif
        return true;
    }
}

bool write_phloat(phloat d) {
    return shell_write_saved_state(&d, sizeof(phloat));
}

bool load_state(int4 ver) {
    int4 magic;
    int4 version;

    /* The shell has verified the initial magic and version numbers,
     * and loaded the shell state, before we got called.
     */

    state_bool_is_int = ver < 9;

    if (ver < 9) {
        state_file_number_format = NUMBER_FORMAT_BINARY;
    } else {
        bool state_is_decimal;
        if (!read_bool(&state_is_decimal)) return false;
        if (!state_is_decimal)
            state_file_number_format = NUMBER_FORMAT_BINARY;
        else if (ver < 12)
            state_file_number_format = NUMBER_FORMAT_BCD20_OLD;
        else if (ver < 18)
            state_file_number_format = NUMBER_FORMAT_BCD20_NEW;
        else
            state_file_number_format = NUMBER_FORMAT_BID128;
    }

    if (ver < 2) {
        core_settings.matrix_singularmatrix = false;
        core_settings.matrix_outofrange = false;
    } else {
        if (!read_bool(&core_settings.matrix_singularmatrix)) return false;
        if (!read_bool(&core_settings.matrix_outofrange)) return false;
        if (ver < 9) {
            int dummy;
            if (!read_int(&dummy)) return false;
        }
    }
    if (ver >= 5 && ver < 20) {
        bool bdummy;
        if (!read_bool(&bdummy)) return false;
        if (ver < 8) {
            int dummy;
            if (!read_int(&dummy)) return false;
        }
    }
    if (ver < 11)
        core_settings.auto_repeat = true;
    else
        if (!read_bool(&core_settings.auto_repeat)) return false;
    if (ver < 15) {
        #if defined(ANDROID) || defined(IPHONE)
            core_settings.enable_ext_accel = true;
            core_settings.enable_ext_locat = true;
            core_settings.enable_ext_heading = true;
        #else
            core_settings.enable_ext_accel = false;
            core_settings.enable_ext_locat = false;
            core_settings.enable_ext_heading = false;
        #endif
        core_settings.enable_ext_time = true;
    } else {
        if (ver < 20) {
            bool dummy;
            if (!read_bool(&dummy)) return false;
            if (!read_bool(&dummy)) return false;
        }
        if (!read_bool(&core_settings.enable_ext_accel)) return false;
        if (!read_bool(&core_settings.enable_ext_locat)) return false;
        if (!read_bool(&core_settings.enable_ext_heading)) return false;
        if (!read_bool(&core_settings.enable_ext_time)) return false;
    }
    #if defined (FREE42_FPTEST)
        core_settings.enable_ext_fptest = true;
    #else
        core_settings.enable_ext_fptest = false;
    #endif

    if (!read_bool(&mode_clall)) return false;
    if (!read_bool(&mode_command_entry)) return false;
    if (!read_bool(&mode_number_entry)) return false;
    if (!read_bool(&mode_alpha_entry)) return false;
    if (!read_bool(&mode_shift)) return false;
    if (!read_int(&mode_appmenu)) return false;
    if (!read_int(&mode_plainmenu)) return false;
    if (!read_bool(&mode_plainmenu_sticky)) return false;
    if (!read_int(&mode_transientmenu)) return false;
    if (!read_int(&mode_alphamenu)) return false;
    if (!read_int(&mode_commandmenu)) return false;
    if (!read_bool(&mode_running)) return false;
    if (!read_bool(&mode_varmenu)) return false;
    if (!read_bool(&mode_updown)) return false;

    if (ver < 6)
        mode_getkey = false;
    else if (!read_bool(&mode_getkey))
        return false;

    if (!read_phloat(&entered_number)) return false;
    if (!read_int(&entered_string_length)) return false;
    if (shell_read_saved_state(entered_string, 15) != 15) return false;

    if (!read_int(&pending_command)) return false;
    if (!read_arg(&pending_command_arg, ver < 9)) return false;
    if (!read_int(&xeq_invisible)) return false;

    if (!read_int(&incomplete_command)) return false;
    if (!read_int(&incomplete_ind)) return false;
    if (!read_int(&incomplete_alpha)) return false;
    if (!read_int(&incomplete_length)) return false;
    if (!read_int(&incomplete_maxdigits)) return false;
    if (!read_int(&incomplete_argtype)) return false;
    if (!read_int(&incomplete_num)) return false;
    if (shell_read_saved_state(incomplete_str, 7) != 7) return false;
    if (!read_int4(&incomplete_saved_pc)) return false;
    if (!read_int4(&incomplete_saved_highlight_row)) return false;

    if (shell_read_saved_state(cmdline, 100) != 100) return false;
    if (!read_int(&cmdline_length)) return false;
    if (!read_int(&cmdline_row)) return false;

    if (!read_int(&matedit_mode)) return false;
    if (shell_read_saved_state(matedit_name, 7) != 7) return false;
    if (!read_int(&matedit_length)) return false;
    if (!unpersist_vartype(&matedit_x, ver < 18)) return false;
    if (!read_int4(&matedit_i)) return false;
    if (!read_int4(&matedit_j)) return false;
    if (!read_int(&matedit_prev_appmenu)) return false;

    if (shell_read_saved_state(input_name, 11) != 11) return false;
    if (!read_int(&input_length)) return false;
    if (!read_arg(&input_arg, ver < 9)) return false;

    if (!read_int(&baseapp)) return false;

    if (ver < 21) {
        phloat random_number;
        if (!read_phloat(&random_number))
            return false;
        random_number_low = to_int8(random_number * 1000000) * 10 + 1;
        random_number_high = 0;
    } else {
        if (!read_int8(&random_number_low)) return false;
        if (!read_int8(&random_number_high)) return false;
    }

    if (ver < 3) {
        deferred_print = 0;
    } else {
        if (!read_int(&deferred_print)) return false;
    }

    if (!read_int(&keybuf_head)) return false;
    if (!read_int(&keybuf_tail)) return false;
    if (shell_read_saved_state(keybuf, 16 * sizeof(int))
            != 16 * sizeof(int))
        return false;

    if (!unpersist_display(ver))
        return false;
    if (!unpersist_globals(ver))
        return false;

    if (ver < 4) {
        /* Before state file version 4, I used to save the BCD table in the
         * state file. As of state file version 4, the Unix and Windows
         * versions don't do that any more because they don't need to
         * (generating the table on startup is fast enough); the PalmOS version
         * now persists the BCD table in a database, which is faster because it
         * doesn't need to be loaded and saved each time the application is
         * started and stopped.
         * This code is to skip the saved BCD table in state versions up to
         * and including version 3.
         */
        int min_pow2, max_pow2;
        uint4 n1, n2, n3, n4, n;
        char dummy[1024];
        if (!read_int(&min_pow2))
            return false;
        if (!read_int(&max_pow2))
            return false;
        n1 = 16 * (max_pow2 + 1);          /* size of pos_pow2mant table */
        n2 = sizeof(int) * (max_pow2 + 1); /* size of pos_pow2exp table  */
        n3 = 16 * (-min_pow2);             /* size of neg_pow2mant table */
        n4 = sizeof(int) * (-min_pow2);    /* size of neg_pow2exp table  */
        n = n1 + n2 + n3 + n4;             /* total number of bytes to skip */
        while (n > 0) {
            int count = n < 1024 ? n : 1024;
            if (shell_read_saved_state(dummy, count) != count)
                return false;
            n -= count;
        }
    }

#ifdef BCD_MATH
    if (!unpersist_math(state_file_number_format != NUMBER_FORMAT_BID128))
        return false;
#else
    if (!unpersist_math(state_file_number_format != NUMBER_FORMAT_BINARY))
        return false;
#endif

    if (!read_int4(&magic)) return false;
    if (magic != FREE42_MAGIC)
        return false;
    if (!read_int4(&version)) return false;
    if (version != ver)
        return false;

    return true;
}

void save_state() {
    /* The shell has written the initial magic and version numbers,
     * and the shell state, before we got called.
     */

    #ifdef BCD_MATH
        if (!write_bool(true)) return;
    #else
        if (!write_bool(false)) return;
    #endif
    if (!write_bool(core_settings.matrix_singularmatrix)) return;
    if (!write_bool(core_settings.matrix_outofrange)) return;
    if (!write_bool(core_settings.auto_repeat)) return;
    if (!write_bool(core_settings.enable_ext_accel)) return;
    if (!write_bool(core_settings.enable_ext_locat)) return;
    if (!write_bool(core_settings.enable_ext_heading)) return;
    if (!write_bool(core_settings.enable_ext_time)) return;
    if (!write_bool(mode_clall)) return;
    if (!write_bool(mode_command_entry)) return;
    if (!write_bool(mode_number_entry)) return;
    if (!write_bool(mode_alpha_entry)) return;
    if (!write_bool(mode_shift)) return;
    if (!write_int(mode_appmenu)) return;
    if (!write_int(mode_plainmenu)) return;
    if (!write_bool(mode_plainmenu_sticky)) return;
    if (!write_int(mode_transientmenu)) return;
    if (!write_int(mode_alphamenu)) return;
    if (!write_int(mode_commandmenu)) return;
    if (!write_bool(mode_running)) return;
    if (!write_bool(mode_varmenu)) return;
    if (!write_bool(mode_updown)) return;
    if (!write_bool(mode_getkey)) return;

    if (!write_phloat(entered_number)) return;
    if (!write_int(entered_string_length)) return;
    if (!shell_write_saved_state(entered_string, 15)) return;

    if (!write_int(pending_command)) return;
    if (!write_arg(&pending_command_arg)) return;
    if (!write_int(xeq_invisible)) return;

    if (!write_int(incomplete_command)) return;
    if (!write_int(incomplete_ind)) return;
    if (!write_int(incomplete_alpha)) return;
    if (!write_int(incomplete_length)) return;
    if (!write_int(incomplete_maxdigits)) return;
    if (!write_int(incomplete_argtype)) return;
    if (!write_int(incomplete_num)) return;
    if (!shell_write_saved_state(incomplete_str, 7)) return;
    if (!write_int4(incomplete_saved_pc)) return;
    if (!write_int4(incomplete_saved_highlight_row)) return;

    if (!shell_write_saved_state(cmdline, 100)) return;
    if (!write_int(cmdline_length)) return;
    if (!write_int(cmdline_row)) return;

    if (!write_int(matedit_mode)) return;
    if (!shell_write_saved_state(matedit_name, 7)) return;
    if (!write_int(matedit_length)) return;
    if (!persist_vartype(matedit_x)) return;
    if (!write_int4(matedit_i)) return;
    if (!write_int4(matedit_j)) return;
    if (!write_int(matedit_prev_appmenu)) return;

    if (!shell_write_saved_state(input_name, 11)) return;
    if (!write_int(input_length)) return;
    if (!write_arg(&input_arg)) return;

    if (!write_int(baseapp)) return;

    if (!write_int8(random_number_low)) return;
    if (!write_int8(random_number_high)) return;

    if (!write_int(deferred_print)) return;

    if (!write_int(keybuf_head)) return;
    if (!write_int(keybuf_tail)) return;
    if (!shell_write_saved_state(keybuf, 16 * sizeof(int))) return;

    if (!persist_display())
        return;
    if (!persist_globals())
        return;
    if (!persist_math())
        return;

    if (!write_int4(FREE42_MAGIC)) return;
    if (!write_int4(FREE42_VERSION)) return;
}

void hard_reset(int bad_state_file) {
    vartype *regs;

    /* Clear stack */
    free_vartype(reg_x);
    free_vartype(reg_y);
    free_vartype(reg_z);
    free_vartype(reg_t);
    free_vartype(reg_lastx);
    reg_x = new_real(0);
    reg_y = new_real(0);
    reg_z = new_real(0);
    reg_t = new_real(0);
    reg_lastx = new_real(0);

    /* Clear alpha */
    reg_alpha_length = 0;

    /* Clear variables */
    purge_all_vars();
    regs = new_realmatrix(25, 1);
    store_var("REGS", 4, regs);

    /* Clear programs */
    if (prgms != NULL) {
        free(prgms);
        prgms = NULL;
        prgms_capacity = 0;
        prgms_count = 0;
    }
    if (labels != NULL) {
        free(labels);
        labels = NULL;
        labels_capacity = 0;
        labels_count = 0;
    }
    goto_dot_dot();

    pending_command = CMD_NONE;

    matedit_mode = 0;
    input_length = 0;
    baseapp = 0;
    random_number_low = 0;
    random_number_high = 0;

    flags.f.f00 = flags.f.f01 = flags.f.f02 = flags.f.f03 = flags.f.f04 = 0;
    flags.f.f05 = flags.f.f06 = flags.f.f07 = flags.f.f08 = flags.f.f09 = 0;
    flags.f.f10 = 0;
    flags.f.auto_exec = 0;
    flags.f.double_wide_print = 0;
    flags.f.lowercase_print = 0;
    flags.f.f14 = 0;
    flags.f.trace_print = 0;
    flags.f.normal_print = 0;
    flags.f.f17 = flags.f.f18 = flags.f.f19 = flags.f.f20 = 0;
    flags.f.printer_enable = 0;
    flags.f.numeric_data_input = 0;
    flags.f.alpha_data_input = 0;
    flags.f.range_error_ignore = 0;
    flags.f.error_ignore = 0;
    flags.f.audio_enable = 1;
    /* flags.f.VIRTUAL_custom_menu = 0; */
    flags.f.decimal_point = shell_decimal_point(); // HP-42S sets this to 1 on hard reset
    flags.f.thousands_separators = 1;
    flags.f.stack_lift_disable = 0;
    flags.f.dmy = 0;
    flags.f.f32 = flags.f.f33 = 0;
    flags.f.agraph_control1 = 0;
    flags.f.agraph_control0 = 0;
    flags.f.digits_bit3 = 0;
    flags.f.digits_bit2 = 1;
    flags.f.digits_bit1 = 0;
    flags.f.digits_bit0 = 0;
    flags.f.fix_or_all = 1;
    flags.f.eng_or_all = 0;
    flags.f.grad = 0;
    flags.f.rad = 0;
    flags.f.continuous_on = 0;
    /* flags.f.VIRTUAL_solving = 0; */
    /* flags.f.VIRTUAL_integrating = 0; */
    /* flags.f.VIRTUAL_variable_menu = 0; */
    /* flags.f.VIRTUAL_alpha_mode = 0; */
    /* flags.f.VIRTUAL_low_battery = 0; */
    flags.f.message = 1;
    flags.f.two_line_message = 0;
    flags.f.prgm_mode = 0;
    /* flags.f.VIRTUAL_input = 0; */
    flags.f.f54 = 0;
    flags.f.printer_exists = 0;
    flags.f.lin_fit = 1;
    flags.f.log_fit = 0;
    flags.f.exp_fit = 0;
    flags.f.pwr_fit = 0;
    flags.f.all_sigma = 1;
    flags.f.log_fit_invalid = 0;
    flags.f.exp_fit_invalid = 0;
    flags.f.pwr_fit_invalid = 0;
    flags.f.f64 = 0;
    /* flags.f.VIRTUAL_matrix_editor = 0; */
    flags.f.grow = 0;
    flags.f.f67 = 0;
    flags.f.base_bit0 = 0;
    flags.f.base_bit1 = 0;
    flags.f.base_bit2 = 0;
    flags.f.base_bit3 = 0;
    flags.f.local_label = 0;
    flags.f.polar = 0;
    flags.f.real_result_only = 0;
    /* flags.f.VIRTUAL_programmable_menu = 0; */
    flags.f.matrix_edge_wrap = 0;
    flags.f.matrix_end_wrap = 0;
    flags.f.f78 = flags.f.f79 = flags.f.f80 = flags.f.f81 = flags.f.f82 = 0;
    flags.f.f83 = flags.f.f84 = flags.f.f85 = flags.f.f86 = flags.f.f87 = 0;
    flags.f.f88 = flags.f.f89 = flags.f.f90 = flags.f.f91 = flags.f.f92 = 0;
    flags.f.f93 = flags.f.f94 = flags.f.f95 = flags.f.f96 = flags.f.f97 = 0;
    flags.f.f98 = flags.f.f99 = 0;

    mode_clall = false;
    mode_command_entry = false;
    mode_number_entry = false;
    mode_alpha_entry = false;
    mode_shift = false;
    mode_commandmenu = MENU_NONE;
    mode_alphamenu = MENU_NONE;
    mode_transientmenu = MENU_NONE;
    mode_plainmenu = MENU_NONE;
    mode_appmenu = MENU_NONE;
    mode_running = false;
    mode_getkey = false;
    mode_pause = false;
    mode_varmenu = false;
    prgm_highlight_row = 0;
    varmenu_length = 0;
    mode_updown = false;
    mode_sigma_reg = 11;
    mode_goose = -1;
    mode_time_clktd = false;
    mode_time_clk24 = false;

    core_settings.auto_repeat = true;
    #if defined(ANDROID) || defined(IPHONE)
        core_settings.enable_ext_accel = true;
        core_settings.enable_ext_locat = true;
        core_settings.enable_ext_heading = true;
    #else
        core_settings.enable_ext_accel = false;
        core_settings.enable_ext_locat = false;
        core_settings.enable_ext_heading = false;
    #endif
    core_settings.enable_ext_time = true;
    #if defined (FREE42_FPTEST)
        core_settings.enable_ext_fptest = true;
    #else
        core_settings.enable_ext_fptest = false;
    #endif

    reset_math();

    clear_display();
    clear_custom_menu();
    clear_prgm_menu();
    if (bad_state_file)
        draw_string(0, 0, "State File Corrupt", 18);
    else
        draw_string(0, 0, "Memory Clear", 12);
    display_x(1);
    flush_display();
}

struct dec_arg_struct {
    unsigned char type;
    unsigned char length;
    int4 target;
    union {
        int4 num;
        char text[15];
        char stk;
        int cmd;
        char lclbl;
    } val;
    fake_bcd val_d;
};

struct bin_arg_struct {
    unsigned char type;
    unsigned char length;
    int4 target;
    union {
        int4 num;
        char text[15];
        char stk;
        int cmd;
        char lclbl;
    } val;
    double val_d;
};

bool read_arg(arg_struct *arg, bool old) {
    if (old) {
        // Prior to core state version 9, the arg_struct type saved a bit of
        // by using a union to hold the argument value.
        // In version 9, we switched from using 'double' as our main numeric
        // data type to 'phloat' -- but since 'phloat' is a class, with a
        // constructor, it cannot be a member of a union.
        // So, I had to change the 'val' member from a union to a struct.
        // Of course, this means that the arg_struct layout is now different,
        // and when deserializing a pre-9 state file, I must make sure to
        // deserialize an old-stype arg_struct and then convert it to a
        // new one.
        struct {
            unsigned char type;
            unsigned char length;
            int4 target;
            union {
                int4 num;
                char text[15];
                char stk;
                int cmd; /* For backward compatibility only! */
                char lclbl;
                double d;
            } val;
        } old_arg;
        if (shell_read_saved_state(&old_arg, sizeof(old_arg))
                != sizeof(old_arg))
            return false;
        arg->type = old_arg.type;
        arg->length = old_arg.length;
        arg->target = old_arg.target;
        char *d = (char *) &arg->val;
        char *s = (char *) &old_arg.val;
        for (unsigned int i = 0; i < sizeof(old_arg.val); i++)
            *d++ = *s++;
        arg->val_d = old_arg.val.d;
        return true;
    } else if (bin_dec_mode_switch()) {
        #ifdef BCD_MATH
            bin_arg_struct ba;
            if (shell_read_saved_state(&ba, sizeof(bin_arg_struct))
                        != sizeof(bin_arg_struct))
                return false;
            arg->type = ba.type;
            arg->length = ba.length;
            arg->target = ba.target;
            char *d = (char *) &arg->val;
            char *s = (char *) &ba.val;
            for (unsigned int i = 0; i < sizeof(ba.val); i++)
                *d++ = *s++;
            arg->val_d = ba.val_d;
        #else
            dec_arg_struct da;
            if (shell_read_saved_state(&da, sizeof(dec_arg_struct))
                        != sizeof(dec_arg_struct))
                return false;
            arg->type = da.type;
            arg->length = da.length;
            arg->target = da.target;
            char *d = (char *) &arg->val;
            char *s = (char *) &da.val;
            for (unsigned int i = 0; i < sizeof(da.val); i++)
                *d++ = *s++;
            arg->val_d = decimal2double(da.val_d.data);
        #endif
        return true;
    } else {
        #if BCD_MATH
        // For explanation, see the comment in write_arg()
        if (shell_read_saved_state(arg, sizeof(dec_arg_struct))
                != sizeof(dec_arg_struct))
            return false;
        int offset = sizeof(arg_struct) - sizeof(dec_arg_struct);
        if (offset != 0) {
            char *s = ((char *) arg) + sizeof(dec_arg_struct);
            char *d = ((char *) arg) + sizeof(arg_struct);
            for (unsigned int i = 0; i < 16; i++)
                *--d = *--s;
        }
        return true;
        #else
        return shell_read_saved_state(arg, sizeof(arg_struct))
            == sizeof(arg_struct);
        #endif
    }
}

bool write_arg(const arg_struct *arg) {
#ifdef BCD_MATH
    // The introduction of BID_UINT128 changed the alignment in arg_struct.
    // BCDFloat was an array of 8 shorts, so was aligned on a 2-byte boundary,
    // while BID_UINT128 is aligned on a 16-byte boundary. For compatibility,
    // we eliminate the extra 8 bytes while writing.
    int firstpartlen = sizeof(dec_arg_struct) - 16;
    if (!shell_write_saved_state(arg, firstpartlen))
        return false;
    return shell_write_saved_state(&arg->val_d, 16);
#else
    return shell_write_saved_state(arg, sizeof(arg_struct));
#endif
}

static bool convert_programs() {
    // This function is called if the setting of mode_decimal recorded in the
    // state file does not match the current setting (i.e., if we're a binary
    // Free42 and the state file was written by the decimal version, or vice
    // versa).
    // This function looks for floating-point number literals (command =
    // CMD_NUMBER with arg.type = ARGTYPE_DOUBLE) and converts them from double
    // to Phloat or the other way around.

    int saved_prgm = current_prgm;
    int4 saved_pc = pc;
    int i;

    // Since converting programs can cause instructions to move, I have to
    // update all stored PC values to correct for this. PCs are stored in the
    // 'pc' and 'rtn_pc[]' globals. I copy those values into a local array,
    // which I then sort by program index and pc; this allows me to do the
    // updates very efficiently later on.
    int mod_prgm[MAX_RTNS + 2];
    int4 mod_pc[MAX_RTNS + 2];
    int mod_sp[MAX_RTNS + 2];
    int mod_count = 0;
    for (i = 0; i < rtn_sp; i++) {
        int prgm = rtn_prgm[i];
        if (prgm == -2 || prgm == -3) {
            // Return-to-solve and return-to-integ
            // On a binary/decimal mode switch, unpersist_math() discards all
            // the SOLVE and INTEG state. If SOLVE or INTEG are actually
            // active, we have to clear the RTN stack, too.
            rtn_sp = 0;
            mod_count = 0;
            break;
        }
        mod_prgm[mod_count] = prgm;
        mod_pc[mod_count] = rtn_pc[i];
        mod_sp[mod_count] = i;
        mod_count++;
    }
    if (saved_pc > 0) {
        mod_prgm[mod_count] = current_prgm;
        mod_pc[mod_count] = saved_pc;
        mod_sp[mod_count] = -1;
        mod_count++;
    }
    if (incomplete_saved_pc > 0) {
        mod_prgm[mod_count] = current_prgm;
        mod_pc[mod_count] = incomplete_saved_pc;
        mod_sp[mod_count] = -2;
        mod_count++;
    }
    mod_count--;

    for (i = 0; i < mod_count; i++)
        for (int j = i + 1; j <= mod_count; j++)
            if (mod_prgm[i] < mod_prgm[j]
                    || mod_prgm[i] == mod_prgm[j] && mod_pc[i] < mod_pc[j]) {
                int tmp = mod_prgm[i];
                mod_prgm[i] = mod_prgm[j];
                mod_prgm[j] = tmp;
                int4 tmp4 = mod_pc[i];
                mod_pc[i] = mod_pc[j];
                mod_pc[j] = tmp4;
                tmp = mod_sp[i];
                mod_sp[i] = mod_sp[j];
                mod_sp[j] = tmp;
            }

    for (i = 0; i < prgms_count; i++) {
        current_prgm = i;
        pc = 0;
        int4 oldpc = 0;
        prgm_struct *prgm = prgms + i;
        prgm->lclbl_invalid = 1;
        while (true) {
            while (mod_count >= 0 && current_prgm == mod_prgm[mod_count]
                                  && oldpc >= mod_pc[mod_count]) {
                // oldpc should never be greater than mod_pc[mod_count]; this
                // means that something is out of whack, because we have an old
                // PC value that does not actually coincide with the beginning
                // of an instruction.
                int s = mod_sp[mod_count];
                if (s == -1)
                    saved_pc = pc;
                else if (s == -2)
                    incomplete_saved_pc = pc;
                else
                    rtn_pc[s] = pc;
                mod_count--;
            }
            int4 prevpc = pc;
            int command = prgm->text[pc++];
            int argtype = prgm->text[pc++];
            command |= (argtype & 240) << 4;
            argtype &= 15;

            if (command == CMD_END)
                break;
            if ((command == CMD_GTO || command == CMD_XEQ)
                    && (argtype == ARGTYPE_NUM || argtype == ARGTYPE_LCLBL)) {
                // Invalidate local label offsets
                prgm->text[pc++] = 255;
                prgm->text[pc++] = 255;
                prgm->text[pc++] = 255;
                prgm->text[pc++] = 255;
            }
            switch (argtype) {
                case ARGTYPE_NUM:
                case ARGTYPE_NEG_NUM:
                case ARGTYPE_IND_NUM: {
                    while ((prgm->text[pc++] & 128) == 0);
                    break;
                }
                case ARGTYPE_STK:
                case ARGTYPE_IND_STK:
                case ARGTYPE_COMMAND:
                case ARGTYPE_LCLBL:
                    pc++;
                    break;
                case ARGTYPE_STR:
                case ARGTYPE_IND_STR: {
                    pc += prgm->text[pc] + 1;
                    break;
                }
                case ARGTYPE_DOUBLE:
                    #ifdef BCD_MATH
                        double d;
                        int j;
                        unsigned char *b = (unsigned char *) &d;
                        for (j = 0; j < (int) sizeof(double); j++)
                            *b++ = prgm->text[pc++];
                        pc -= sizeof(double);

                        int growth = sizeof(phloat) - sizeof(double);
                        int4 pos;
                        if (prgm->size + growth > prgm->capacity) {
                            unsigned char *newtext;
                            prgm->capacity += 512;
                            newtext = (unsigned char *) malloc(prgm->capacity);
                            if (newtext == NULL)
                                // Failed to grow program; abort.
                                return false;
                            for (pos = 0; pos < pc; pos++)
                                newtext[pos] = prgm->text[pos];
                            for (pos = pc; pos < prgm->size; pos++)
                                newtext[pos + growth] = prgm->text[pos];
                            if (prgm->text != NULL)
                                free(prgm->text);
                            prgm->text = newtext;
                        } else {
                            for (pos = prgm->size - 1; pos >= pc; pos--)
                                prgm->text[pos + growth] = prgm->text[pos];
                        }
                        prgm->size += growth;
                        oldpc -= growth;

                        phloat p(d);
                        b = (unsigned char *) &p;
                        for (j = 0; j < (int) sizeof(phloat); j++)
                            prgm->text[pc++] = *b++;
                    #else
                        fake_bcd dec;
                        int j;
                        unsigned char *b = (unsigned char *) &dec;
                        for (j = 0; j < (int) sizeof(fake_bcd); j++)
                            *b++ = prgm->text[pc++];
                        double dbl = decimal2double(dec.data, true);

                        pc -= sizeof(fake_bcd);
                        b = (unsigned char *) &dbl;
                        for (j = 0; j < (int) sizeof(double); j++)
                            prgm->text[pc++] = *b++;

                        int shrinkage = sizeof(fake_bcd) - sizeof(double);
                        prgm->size -= shrinkage;
                        for (int4 pos = pc; pos < prgm->size; pos++)
                            prgm->text[pos] = prgm->text[pos + shrinkage];
                        oldpc += shrinkage;
                    #endif
                    break;
            }
            oldpc += pc - prevpc;
        }
    }

    current_prgm = saved_prgm;
    pc = saved_pc;

    return true;
}

#ifdef BCD_MATH
static void update_decimal_in_programs() {
    // This function is called after reading a decimal state file,
    // if the number format in that state file doesn't match the current one.

    int saved_prgm = current_prgm;
    int4 saved_pc = pc;
    int i, j;

    for (i = 0; i < prgms_count; i++) {
        current_prgm = i;
        pc = 0;
        prgm_struct *prgm = prgms + i;
        while (true) {
            int command = prgm->text[pc++];
            int argtype = prgm->text[pc++];
            command |= (argtype & 240) << 4;
            argtype &= 15;

            if (command == CMD_END)
                break;
            if ((command == CMD_GTO || command == CMD_XEQ)
                    && (argtype == ARGTYPE_NUM || argtype == ARGTYPE_LCLBL)) {
                // Skip local label offsets
                pc += 4;
            }
            switch (argtype) {
                case ARGTYPE_NUM:
                case ARGTYPE_NEG_NUM:
                case ARGTYPE_IND_NUM: {
                    while ((prgm->text[pc++] & 128) == 0);
                    break;
                }
                case ARGTYPE_STK:
                case ARGTYPE_IND_STK:
                case ARGTYPE_COMMAND:
                case ARGTYPE_LCLBL:
                    pc++;
                    break;
                case ARGTYPE_STR:
                case ARGTYPE_IND_STR: {
                    pc += prgm->text[pc] + 1;
                    break;
                }
                case ARGTYPE_DOUBLE:
                    BID_UINT128 dec;
                    char *p = (char *) &dec;
                    for (j = 0; j < 16; j++)
                        *p++ = prgm->text[pc++];
                    update_decimal(&dec);
                    p -= 16;
                    pc -= 16;
                    for (j = 0; j < 16; j++)
                        prgm->text[pc++] = *p++;
                    break;
            }
        }
    }

    current_prgm = saved_prgm;
    pc = saved_pc;
}
#endif

#ifdef ANDROID
void reinitialize_globals() {
    /* The Android version may call core_init() after core_quit(), in other
     * words, the globals may live for more than one session. This caused
     * crashes in the initial builds, because of course global initializers
     * are only invoked once, and core_quit() did not bother to clean things
     * up so that core_init() would be able to run safely.
     * In my defense, this wasn't sloppy coding; core_quit() does deallocate
     * everything -- I've tested Free42 for memory leaks using POSE many
     * times, and it is solid in that regard. The dangling pointers left
     * by core_quit() are never a problem as long as core_init() and
     * core_quit() are only called once per process.
     * Anyway: the following are re-initializations of some globals that
     * could cause double-free() memory corruption, or other (less fatal, but
     * still annoying) misbehaviors if left as they are.
     */
    reg_x = NULL;
    reg_y = NULL;
    reg_z = NULL;
    reg_t = NULL;
    reg_lastx = NULL;
    reg_alpha_length = 0;
    vars_capacity = 0;
    vars_count = 0;
    vars = NULL;
    prgms_capacity = 0;
    prgms_count = 0;
    prgms = NULL;
    labels_capacity = 0;
    labels_count = 0;
    labels = NULL;
    current_prgm = -1;
    prgm_highlight_row = 0;
    mode_interruptible = NULL;
    mode_pause = false;
    baseapp = 0;
    deferred_print = 0;
    keybuf_head = 0;
    keybuf_tail = 0;
    remove_program_catalog = 0;
    rtn_sp = 0;
}
#endif

#ifdef IPHONE
bool off_enabled() {
    if (off_enable_flag)
        return true;
    if (reg_x->type != TYPE_STRING)
        return false;
    vartype_string *str = (vartype_string *) reg_x;
    off_enable_flag = str->length == 6
                      && str->text[0] == 'Y'
                      && str->text[1] == 'E'
                      && str->text[2] == 'S'
                      && str->text[3] == 'O'
                      && str->text[4] == 'F'
                      && str->text[5] == 'F';
    return off_enable_flag;
}
#endif
