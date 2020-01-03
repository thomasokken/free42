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
#include <string.h>

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

// File used for reading and writing the state file, and for importing and
// exporting programs. Since only one of these operations can be active at one
// time, having one FILE pointer for all of them is sufficient.
FILE *gfile = NULL;

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
    { /* SUSPICIOUS_OFF */         "Suspicious OFF",          14 },
    { /* RTN_STACK_FULL */         "RTN Stack Full",          14 }
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
    { /* MENU_MODES1 */ MENU_NONE, MENU_MODES2, MENU_MODES3,
                      { { 0x2000 + CMD_DEG,   0, "" },
                        { 0x2000 + CMD_RAD,   0, "" },
                        { 0x2000 + CMD_GRAD,  0, "" },
                        { 0x1000 + CMD_NULL,  0, "" },
                        { 0x2000 + CMD_RECT,  0, "" },
                        { 0x2000 + CMD_POLAR, 0, "" } } },
    { /* MENU_MODES2 */ MENU_NONE, MENU_MODES3, MENU_MODES1,
                      { { 0x1000 + CMD_SIZE,    0, "" },
                        { 0x2000 + CMD_QUIET,   0, "" },
                        { 0x2000 + CMD_CPXRES,  0, "" },
                        { 0x2000 + CMD_REALRES, 0, "" },
                        { 0x2000 + CMD_KEYASN,  0, "" },
                        { 0x2000 + CMD_LCLBL,   0, "" } } },
    { /* MENU_MODES3 */ MENU_NONE, MENU_MODES1, MENU_MODES2,
                      { { 0x1000 + CMD_WSIZE,   0, "" },
                        { 0x1000 + CMD_WSIZE_T, 0, "" },
                        { 0x2000 + CMD_BSIGNED, 0, "" },
                        { 0x2000 + CMD_BWRAP,   0, "" },
                        { 0x1000 + CMD_NULL,    0, "" },
                        { 0x1000 + CMD_BRESET,  0, "" } } },
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
int mode_wsize;

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


/* Version number for the state file.
 * State file versions correspond to application releases as follows:
 * 
 * Version  0: 1.0    first release
 * Version  1: 1.0.13 "IP Hack" option
 * Version  2: 1.0.13 "singular matrix" and matrix "out of range" options
 * Version  3: 1.0.16 "deferred_print" flag for NORM/TRACE printing
 * Version  4: 1.1    BCD conversion table no longer stored in state file
 * Version  5: 1.1    "raw text" option
 * Version  6: 1.1.8  GETKEY across power-cycle
 * Version  7: 1.1.12 FCN catalog assignments now HP-42S-compatible
 * Version  8: 1.1.14 F42 file format and "HP-42S byte counts" option removed
 * Version  9: 1.4    decimal version; removed IP Hack
 * Version 10: 1.4.16 persistent shared matrices
 * Version 11: 1.4.44 "Auto-Repeat" option
 * Version 12: 1.4.52 BIGSTACK (iphone only);
 *                    new BCDFloat format (Inf and NaN flags)
 * 
 *  ========== NOTE: BCD20 Upgrade in Free42 1.4.52 ==========
 *  In version 1.4.52, I upgraded to a new version of BCD20, without realizing
 *  that it uses a slightly different storage format (NaN and Inifinity are now
 *  encoded using two flags in the exponent field, rather than using magical
 *  exponent values; the exponent field was narrowed by 2 bits to accommodate
 *  these flags).
 *  I should have added new code to convert BCDFloat numbers from the old
 *  format to the new at that time. Once I discovered this oversight, 1.4.52-54
 *  were already released.
 *  In 1.4.55, I introduced code to convert old-style BCDFloat to new-style if
 *  the state file version is less than 12, i.e. created by Free42 1.4.51 or
 *  earlier. This means that 1.4.55 will interpret BCDFloat from all previous
 *  versions correctly; however, any state file that has gone through the
 *  transition from <= 1.4.51 Decimal to 1.4.52-54 Decimal may still be
 *  corrupted, and the only way to be safe is to do CLALL and reload all
 *  programs and data in that case.
 *
 * Version 13: 1.4.55 Dynamically sized BIGSTACK (iphone only)
 * Version 14: 1.4.63 Moved BIGSTACK DROP command from index 315 to 329, to fix
 *                    the clash with Underhill's COPAN extensions. The iPhone
 *                    version, when reading a state file with version 12 or 13,
 *                    scans all programs and renumbers DROP where necessary.
 *                    All other versions can ignore this version number change.
 * Version 15: 1.4.63 "Enable Extension" options for COPAN, BIGSTACK, ACCEL,
 *                    LOCAT, HEADING, and HP-41 Time
 * Version 16: 1.4.63 time and date format flags
 * Version 17: 1.4.65 iPhone "OFF enable" flag
 * Version 18: 1.4.79 Replaced BCD20 with Intel's Decimal Floating Point
 *                    Library v.2.1.
 * Version 19: 1.5.14 Removed mode_time_dmy; now using flag 31 instead.
 * Version 20: 2.0.3  Removed "raw text" option, ext_copan, and ext_bigstack.
 * Version 21: 2.0.7  New random number generator.
 * Version 22: 2.0.17 Fixed bug where local GTO/XEQ targets didn't get cleared
 *                    when an END was deleted, and potentially (though never
 *                    reported) also when an END was inserted. Bumping the
 *                    state version so that older state files get their
 *                    potentially-incorrect jump targets cleared, just in case.
 * Version 23: 2.1    Added "prog" extension: SST^, SST->
 * Version 24: 2.2    Large RTN stack; local variables
 * Version 25: 2.4    WSIZE, BSIGNED, BWRAP
 * Version 26: 2.5    Separate and portable core state file
 * Version 27: 2.5.2  Recovery mode for corrupt 2.5 state files
 * Version 28: 2.5.3  Recording platform name and app version in state file
 * Version 29: 2.5.7  SOLVE: Tracking second best guess in order to be able to
 *                    report it accurately in Y, and to provide additional data
 *                    points for distinguishing between zeroes and poles.
 */
#define FREE42_VERSION 29


/*******************/
/* Private globals */
/*******************/

static bool state_bool_is_int;
bool state_is_portable;

typedef struct {
    int4 prgm;
    int4 pc;
} rtn_stack_entry;

typedef struct {
    unsigned char length;
    char name[7];
} rtn_stack_matrix_name_entry;

typedef struct {
    int4 i, j;
} rtn_stack_matrix_ij_entry;

/* Stack pointer vs. level:
 * The stack pointer is the pointer into the actual rtn_stack array, while
 * the stack level is the number of pending returns. The difference between
 * the two is due to the fact that a return may or may not have a saved
 * INDEX matrix name & position associated with it, and that saved matrix
 * state takes up 16 bytes, compared to 8 bytes for a return by itself.
 * I don't want to set aside 24 bytes for every return, and optimize by
 * storing 8 bytes for a plain return (rtn_stack_entry struct) and 24 bytes
 * for a return with INDEX matrix (rtn_stack_entry plus rtn_stack_matrix_name_entry
 * plus rtn_stack_matrix_ij_entry), but that means that the physical stack
 * pointer and the number of pending returns are no longer guaranteed to
 * be in sync, hence the need to track them separately.
 */
#define MAX_RTN_LEVEL 1024
static int rtn_sp = 0;
static int rtn_stack_capacity = 0;
static rtn_stack_entry *rtn_stack = NULL;
static int rtn_level = 0;
static bool rtn_level_0_has_matrix_entry;
static int rtn_stop_level = -1;
static bool rtn_solve_active = false;
static bool rtn_integ_active = false;

#ifdef IPHONE
/* For iPhone, we disable OFF by default, to satisfy App Store
 * policy, but we allow users to enable it using a magic value
 * in the X register. This flag determines OFF behavior.
 */
bool off_enable_flag = false;
#endif

typedef struct {
    int type;
    int4 rows;
    int4 columns;
} matrix_persister;

static int array_count;
static int array_list_capacity;
static void **array_list;


static bool array_list_grow();
static int array_list_search(void *array);
static bool persist_vartype(vartype *v);
static bool unpersist_vartype(vartype **v, bool padded);
static void update_label_table(int prgm, int4 pc, int inserted);
static void invalidate_lclbls(int prgm_index, bool force);
static int pc_line_convert(int4 loc, int loc_is_pc);
static bool convert_programs(bool *clear_stack);
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
    if (v == NULL)
        return write_char(TYPE_NULL);
    if (!write_char(v->type))
        return false;
    switch (v->type) {
        case TYPE_REAL: {
            vartype_real *r = (vartype_real *) v;
            return write_phloat(r->x);
        }
        case TYPE_COMPLEX: {
            vartype_complex *c = (vartype_complex *) v;
            return write_phloat(c->re) && write_phloat(c->im);
        }
        case TYPE_STRING: {
            vartype_string *s = (vartype_string *) v;
            return write_char(s->length)
                && fwrite(s->text, 1, s->length, gfile) == s->length;
        }
        case TYPE_REALMATRIX: {
            vartype_realmatrix *rm = (vartype_realmatrix *) v;
            int4 rows = rm->rows;
            int4 columns = rm->columns;
            bool must_write = true;
            if (rm->array->refcount > 1) {
                int n = array_list_search(rm->array);
                if (n == -1) {
                    // A negative row count signals a new shared matrix
                    rows = -rows;
                    if (!array_list_grow())
                        return false;
                    array_list[array_count++] = rm->array;
                } else {
                    // A zero row count means this matrix shares its data
                    // with a previously written matrix
                    rows = 0;
                    columns = n;
                    must_write = false;
                }
            }
            write_int4(rows);
            write_int4(columns);
            if (must_write) {
                int size = rm->rows * rm->columns;
                if (fwrite(rm->array->is_string, 1, size, gfile) != size)
                    return false;
                for (int i = 0; i < size; i++) {
                    if (rm->array->is_string[i]) {
                        char *str = (char *) &rm->array->data[i];
                        if (fwrite(str, 1, 7, gfile) != 7)
                            return false;
                    } else {
                        if (!write_phloat(rm->array->data[i]))
                            return false;
                    }
                }
            }
            return true;
        }
        case TYPE_COMPLEXMATRIX: {
            vartype_complexmatrix *cm = (vartype_complexmatrix *) v;
            int4 rows = cm->rows;
            int4 columns = cm->columns;
            bool must_write = true;
            if (cm->array->refcount > 1) {
                int n = array_list_search(cm->array);
                if (n == -1) {
                    // A negative row count signals a new shared matrix
                    rows = -rows;
                    if (!array_list_grow())
                        return false;
                    array_list[array_count++] = cm->array;
                } else {
                    // A zero row count means this matrix shares its data
                    // with a previously written matrix
                    rows = 0;
                    columns = n;
                    must_write = false;
                }
            }
            write_int4(rows);
            write_int4(columns);
            if (must_write) {
                int size = 2 * cm->rows * cm->columns;
                for (int i = 0; i < size; i++)
                    if (!write_phloat(cm->array->data[i]))
                        return false;
            }
            return true;
        }
        default:
            /* Should not happen */
            return false;
    }
}


// A few declarations to help unpersist_vartype get the offsets right,
// in the case it needs to convert the state file.

struct fake_bcd {
    char data[16];
};

// For coping with bad 2.5 state files. 0 means don't do anything special;
// 1 means check for bad string lengths in real matrices; 2 is bug-
// compatibility mode; and 3 is a signal to switch from mode 1 to mode 2.
// When a bad string length is found in mode 1, this function will switch
// the mode to mode 3 and return false, indicating an error; the caller
// should then clean up what has already been read, rewind the state file,
// and try again in mode 2.

int bug_mode;

static bool unpersist_vartype(vartype **v, bool padded) {
    if (state_is_portable) {
        char type;
        if (!read_char(&type))
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
                if (!read_phloat(&r->x)) {
                    free_vartype((vartype *) r);
                    return false;
                }
                *v = (vartype *) r;
                return true;
            }
            case TYPE_COMPLEX: {
                vartype_complex *c = (vartype_complex *) new_complex(0, 0);
                if (c == NULL)
                    return false;
                if (!read_phloat(&c->re) || !read_phloat(&c->im)) {
                    free_vartype((vartype *) c);
                    return false;
                }
                *v = (vartype *) c;
                return true;
            }
            case TYPE_STRING: {
                vartype_string *s = (vartype_string *) new_string("", 0);
                if (s == NULL)
                    return false;
                char len;
                if (!read_char(&len) || fread(s->text, 1, len, gfile) != len) {
                    free_vartype((vartype *) s);
                    return false;
                }
                s->length = len;
                *v = (vartype *) s;
                return true;
            }
            case TYPE_REALMATRIX: {
                int4 rows, columns;
                if (!read_int4(&rows) || !read_int4(&columns))
                    return false;
                if (rows == 0) {
                    // Shared matrix
                    vartype *m = new_matrix_alias((vartype *) array_list[columns]);
                    if (m == NULL)
                        return false;
                    else {
                        *v = m;
                        return true;
                    }
                }
                bool shared = rows < 0;
                if (shared)
                    rows = -rows;
                vartype_realmatrix *rm = (vartype_realmatrix *) new_realmatrix(rows, columns);
                if (rm == NULL)
                    return false;
                int4 size = rows * columns;
                if (fread(rm->array->is_string, 1, size, gfile) != size) {
                    free_vartype((vartype *) rm);
                    return false;
                }
                bool success = true;
                for (int4 i = 0; i < size; i++) {
                    if (rm->array->is_string[i]) {
                        char *dst = (char *) &rm->array->data[i];
                        if (bug_mode == 0) {
                            // 6 bytes of text followed by length byte
                            if (fread(dst, 1, 7, gfile) != 7) {
                                success = false;
                                break;
                            }
                        } else if (bug_mode == 1) {
                            // Could be as above, or could be length-prefixed.
                            // Read 7 bytes, and if byte 7 looks plausible,
                            // carry on; otherwise, set bug_mode to 3, signalling
                            // we should start over in bug-compatibility mode.
                            if (fread(dst, 1, 7, gfile) != 7) {
                                success = false;
                                break;
                            }
                            if (dst[6] < 0 || dst[6] > 6) {
                                bug_mode = 3;
                                success = false;
                                break;
                            }
                        } else {
                            // bug_mode == 2, means this has to be a file with
                            // length-prefixed strings in matrices. Bear in
                            // mind that the prefixes are bogus, so for reading,
                            // clamp them to the 0..6 range, but for advancing
                            // in the file, take them at face value.
                            unsigned char len;
                            if (fread(&len, 1, 1, gfile) != 1) {
                                success = false;
                                break;
                            }
                            unsigned char reallen = len > 6 ? 6 : len;
                            dst[0] = len;
                            if (fread(dst + 1, 1, reallen, gfile) != reallen) {
                                success = false;
                                break;
                            }
                            len -= reallen;
                            if (len > 0 && fseek(gfile, len, SEEK_CUR) != 0) {
                                success = false;
                                break;
                            }
                        }
                    } else {
                        if (!read_phloat(&rm->array->data[i])) {
                            success = false;
                            break;
                        }
                    }
                }
                if (!success) {
                    free_vartype((vartype *) rm);
                    return false;
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
                int4 rows, columns;
                if (!read_int4(&rows) || !read_int4(&columns))
                    return false;
                if (rows == 0) {
                    // Shared matrix
                    vartype *m = new_matrix_alias((vartype *) array_list[columns]);
                    if (m == NULL)
                        return false;
                    else {
                        *v = m;
                        return true;
                    }
                }
                bool shared = rows < 0;
                if (shared)
                    rows = -rows;
                vartype_complexmatrix *cm = (vartype_complexmatrix *) new_complexmatrix(rows, columns);
                if (cm == NULL)
                    return false;
                int4 size = 2 * rows * columns;
                for (int4 i = 0; i < size; i++) {
                    if (!read_phloat(&cm->array->data[i])) {
                        free_vartype((vartype *) cm);
                        return false;
                    }
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
    
    // !state_is_portable
    int type;
    if (fread(&type, 1, sizeof(int), gfile) != sizeof(int))
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
                        if (fread(&dummy, 1, 4, gfile) != 4) {
                            free_vartype((vartype *) r);
                            return false;
                        }
                    }
                    double x;
                    if (fread(&x, 1, 8, gfile) != 8) {
                        free_vartype((vartype *) r);
                        return false;
                    }
                    r->x = x;
                #else
                    BID_UINT128 x;
                    if (fread(&x, 1, 16, gfile) != 16) {
                        free_vartype((vartype *) r);
                        return false;
                    }
                    r->x = decimal2double(&x);
                #endif
            } else {
                #ifndef BCD_MATH
                    if (padded) {
                        int4 dummy;
                        if (fread(&dummy, 1, 4, gfile) != 4) {
                            free_vartype((vartype *) r);
                            return false;
                        }
                    }
                #endif
                if (fread(&r->x, 1, sizeof(phloat), gfile)
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
                        if (fread(&dummy, 1, 4, gfile) != 4) {
                            free_vartype((vartype *) c);
                            return false;
                        }
                    }
                    double parts[2];
                    if (fread(parts, 1, 16, gfile) != 16) {
                        free_vartype((vartype *) c);
                        return false;
                    }
                    c->re = parts[0];
                    c->im = parts[1];
                #else
                    BID_UINT128 parts[2];
                    if (fread(parts, 1, 32, gfile) != 32) {
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
                        if (fread(&dummy, 1, 4, gfile) != 4) {
                            free_vartype((vartype *) c);
                            return false;
                        }
                    }
                #endif
                if (fread(&c->re, 1, 2 * sizeof(phloat), gfile)
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
            if (fread(&s->type + 1, 1, n, gfile) != n) {
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
            if (fread(&mp.type + 1, 1, n, gfile) != n)
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
                if (fread(temp, 1, tsz, gfile) != tsz) {
                    free(temp);
                    free_vartype((vartype *) rm);
                    return false;
                }
                if (fread(rm->array->is_string, 1, size, gfile) != size) {
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
                if (fread(rm->array->data, 1, size, gfile) != size) {
                    free_vartype((vartype *) rm);
                    return false;
                }
                size = mp.rows * mp.columns;
                if (fread(rm->array->is_string, 1, size, gfile) != size) {
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
            if (fread(&mp.type + 1, 1, n, gfile) != n)
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
                if (fread(cm->array->data, 1, size, gfile) != size) {
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
    if (fwrite(reg_alpha, 1, 44, gfile) != 44)
        goto done;
    if (!write_int4(mode_sigma_reg))
        goto done;
    if (!write_int(mode_goose))
        goto done;
    if (!write_bool(mode_time_clktd))
        goto done;
    if (!write_bool(mode_time_clk24))
        goto done;
    if (!write_int(mode_wsize))
        goto done;
    if (fwrite(&flags, 1, sizeof(flags_struct), gfile) != sizeof(flags_struct))
        goto done;
    if (!write_int(prgms_count))
        goto done;
    for (i = 0; i < prgms_count; i++)
        core_export_programs(1, &i, NULL);
    if (!write_int(current_prgm))
        goto done;
    if (!write_int4(pc2line(pc)))
        goto done;
    if (!write_int(prgm_highlight_row))
        goto done;
    if (!write_int(vars_count))
        goto done;
    for (i = 0; i < vars_count; i++) {
        if (!write_char(vars[i].length)
            || fwrite(vars[i].name, 1, vars[i].length, gfile) != vars[i].length
            || !write_int2(vars[i].level)
            || !write_bool(vars[i].hidden)
            || !write_bool(vars[i].hiding)
            || !persist_vartype(vars[i].value))
            goto done;
    }
    if (!write_int(varmenu_length))
        goto done;
    if (fwrite(varmenu, 1, 7, gfile) != 7)
        goto done;
    if (!write_int(varmenu_rows))
        goto done;
    if (!write_int(varmenu_row))
        goto done;
    for (i = 0; i < 6; i++)
        if (!write_char(varmenu_labellength[i])
                || fwrite(varmenu_labeltext[i], 1, varmenu_labellength[i], gfile) != varmenu_labellength[i])
            goto done;
    if (!write_int(varmenu_role))
        goto done;
    if (!write_int(rtn_sp))
        goto done;
    if (!write_int(rtn_level))
        goto done;
    if (!write_bool(rtn_level_0_has_matrix_entry))
        goto done;
    int saved_prgm;
    saved_prgm = current_prgm;
    for (i = rtn_sp - 1; i >= 0; i--) {
        bool matrix_entry_follows = i == 1 && rtn_level_0_has_matrix_entry;
        if (matrix_entry_follows) {
            i++;
        } else {
            int4 p = rtn_stack[i].prgm;
            matrix_entry_follows = p < 0;
            p = p & 0x7fffffff;
            if ((p & 0x40000000) != 0)
                p |= 0x80000000;
            current_prgm = p;
            int4 l = rtn_stack[i].pc;
            if (current_prgm >= 0)
                l = pc2line(l);
            if (!write_int4(rtn_stack[i].prgm)
                    || !write_int4(l))
                goto done;
        }
        if (matrix_entry_follows) {
            rtn_stack_matrix_name_entry *e1 = (rtn_stack_matrix_name_entry *) &rtn_stack[--i];
            rtn_stack_matrix_ij_entry *e2 = (rtn_stack_matrix_ij_entry *) &rtn_stack[--i];
            if (!write_char(e1->length)
                    || fwrite(e1->name, 1, e1->length, gfile) != e1->length
                    || !write_int4(e2->i)
                    || !write_int4(e2->j))
                goto done;
        }
    }
    current_prgm = saved_prgm;
    if (!write_bool(rtn_solve_active))
        goto done;
    if (!write_bool(rtn_integ_active))
        goto done;
    ret = true;

    done:
    free(array_list);
    return ret;
}

static bool suppress_varmenu_update = false;

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

    // Hack to deal with bad Android state files
    if (reg_x == NULL)
        goto done;
    // End of hack

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
    if (fread(reg_alpha, 1, 44, gfile) != 44) {
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
    } else {
        mode_time_clktd = false;
        mode_time_clk24 = false;
    }
    if (ver >= 25) {
        if (!read_int(&mode_wsize)) {
            mode_wsize = 36;
            goto done;
        }
    } else
        mode_wsize = 36;
    if (fread(&flags, 1, sizeof(flags_struct), gfile)
            != sizeof(flags_struct))
        goto done;
    if (tmp_dmy != 2)
        flags.f.dmy = tmp_dmy;
    if (ver < 25) {
        flags.f.base_signed = 1;
        flags.f.base_wrap = 0;
    }

    if (!state_is_portable) {
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
            if (fread(vars + i, 1, 12, gfile) != 12) {
                free(vars);
                vars = NULL;
                vars_count = 0;
                goto done;
            }
        if (ver < 24)
            for (i = 0; i < vars_count; i++) {
                vars[i].level = -1;
                vars[i].hidden = false;
                vars[i].hiding = false;
            }
        for (i = 0; i < vars_count; i++)
            if (!unpersist_vartype(&vars[i].value, padded)) {
                for (int j = 0; j < i; j++)
                    free_vartype(vars[j].value);
                free(vars);
                vars = NULL;
                vars_count = 0;
                goto done;
            }
        vars_capacity = vars_count;

        // Purging zero-length var that may have been created by buggy INTEG
        purge_var("", 0);
    }

    prgms_count = 0;
    prgms_capacity = 0;
    if (prgms != NULL) {
        free(prgms);
        prgms = NULL;
    }
    int nprogs;
    if (!read_int(&nprogs)) {
        goto done;
    }
    if (state_is_portable) {
        suppress_varmenu_update = true;
        core_import_programs(nprogs, NULL);
        suppress_varmenu_update = false;
    } else {
        prgms_count = nprogs;
        prgms = (prgm_struct *) malloc(prgms_count * sizeof(prgm_struct));
        if (prgms == NULL) {
            prgms_count = 0;
            goto done;
        }
        for (i = 0; i < prgms_count; i++)
            if (fread(prgms + i, 1, sizeof(prgm_struct_32bit), gfile) != sizeof(prgm_struct_32bit)) {
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
            if (fread(prgms[i].text, 1, prgms[i].size, gfile)
                    != prgms[i].size) {
                clear_all_prgms();
                goto done;
            }
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
    if (state_is_portable) {
        pc = line2pc(pc);
        incomplete_saved_pc = line2pc(incomplete_saved_pc);
    }
    if (!read_int(&prgm_highlight_row)) {
        prgm_highlight_row = 0;
        goto done;
    }
    
    if (state_is_portable) {
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
        for (i = 0; i < vars_count; i++) {
            if (!read_char((char *) &vars[i].length)
                || fread(vars[i].name, 1, vars[i].length, gfile) != vars[i].length
                || !read_int2(&vars[i].level)
                || !read_bool(&vars[i].hidden)
                || !read_bool(&vars[i].hiding)
                || !unpersist_vartype(&vars[i].value, false)) {
                for (int j = 0; j < i; j++)
                    free_vartype(vars[j].value);
                free(vars);
                vars = NULL;
                vars_count = 0;
                goto done;
            }
        }
        vars_capacity = vars_count;
    }
    
    if (!read_int(&varmenu_length)) {
        varmenu_length = 0;
        goto done;
    }
    if (fread(varmenu, 1, 7, gfile) != 7) {
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
    if (state_is_portable) {
        char c;
        for (i = 0; i < 6; i++) {
            if (!read_char(&c)
                    || fread(varmenu_labeltext[i], 1, c, gfile) != c)
                goto done;
            varmenu_labellength[i] = c;
        }
    } else {
        if (fread(varmenu_labellength, 1, 6 * sizeof(int), gfile)
                != 6 * sizeof(int))
            goto done;
        if (fread(varmenu_labeltext, 1, 42, gfile) != 42)
            goto done;
    }
    if (!read_int(&varmenu_role))
        goto done;
    if (!read_int(&rtn_sp))
        goto done;
    if (ver >= 24) {
        if (!read_int(&rtn_level))
            goto done;
        if (!read_bool(&rtn_level_0_has_matrix_entry))
            goto done;
        rtn_stack_capacity = 16;
        while (rtn_sp > rtn_stack_capacity)
            rtn_stack_capacity <<= 1;
        rtn_stack = (rtn_stack_entry *) realloc(rtn_stack, rtn_stack_capacity * sizeof(rtn_stack_entry));
        if (state_is_portable) {
            int saved_prgm = current_prgm;
            for (i = rtn_sp - 1; i >= 0; i--) {
                bool matrix_entry_follows = i == 1 && rtn_level_0_has_matrix_entry;
                if (matrix_entry_follows) {
                    i++;
                } else {
                    int4 tprgm, p, l;
                    if (!read_int4(&tprgm) || !read_int4(&l))
                        goto done;
                    matrix_entry_follows = tprgm < 0;
                    p = tprgm & 0x7fffffff;
                    if ((p & 0x40000000) != 0)
                        p |= 0x80000000;
                    current_prgm = p;
                    if (p >= 0)
                        l = line2pc(l);
                    rtn_stack[i].prgm = tprgm;
                    rtn_stack[i].pc = l;
                }
                if (matrix_entry_follows) {
                    rtn_stack_matrix_name_entry *e1 = (rtn_stack_matrix_name_entry *) &rtn_stack[--i];
                    rtn_stack_matrix_ij_entry *e2 = (rtn_stack_matrix_ij_entry *) &rtn_stack[--i];
                    if (!read_char((char *) &e1->length)
                            || fread(e1->name, 1, e1->length, gfile) != e1->length
                            || !read_int4(&e2->i)
                            || !read_int4(&e2->j))
                        goto done;
                }
            }
            current_prgm = saved_prgm;
        } else {
            int sz = rtn_sp * sizeof(rtn_stack_entry);
            if (fread(rtn_stack, 1, sz, gfile) != sz)
                goto done;
        }
        if (!read_bool(&rtn_solve_active))
            goto done;
        if (!read_bool(&rtn_integ_active))
            goto done;
    } else {
        rtn_level = rtn_sp;
        rtn_level_0_has_matrix_entry = false;
        rtn_stack_capacity = 16;
        rtn_stack = (rtn_stack_entry *) realloc(rtn_stack, rtn_stack_capacity * sizeof(rtn_stack_entry));
        rtn_solve_active = false;
        rtn_integ_active = false;
        for (i = 0; i < 8; i++) {
            int prgm;
            if (fread(&prgm, 1, sizeof(int), gfile) != sizeof(int))
                goto done;
            rtn_stack[i].prgm = prgm & 0x7fffffff;
            if (i < rtn_sp)
                if (prgm == -2)
                    rtn_solve_active = true;
                else if (prgm == -3)
                    rtn_integ_active = true;
        }
        for (i = 0; i < 8; i++)
            if (fread(&rtn_stack[i].pc, 1, sizeof(int4), gfile) != sizeof(int4))
                goto done;
    }
#ifdef IPHONE
    if (ver >= 17 && ver <= 25)
        if (!read_bool(&off_enable_flag))
            goto done;
#endif

    if (!state_is_portable) {
        if (bin_dec_mode_switch()) {
            bool clear_stack;
            if (!convert_programs(&clear_stack)) {
                clear_all_prgms();
                goto done;
            }
            if (clear_stack)
                clear_all_rtns();
        } else {
            if (ver < 22)
                for (i = 0; i < prgms_count; i++)
                    invalidate_lclbls(i, true);
        }
        #ifdef BCD_MATH
            if (state_file_number_format == NUMBER_FORMAT_BCD20_OLD
                    || state_file_number_format == NUMBER_FORMAT_BCD20_NEW)
                update_decimal_in_programs();
        #endif
        rebuild_label_table();
    }
    
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
        goto_dot_dot(false);
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

    invalidate_lclbls(current_prgm, false);
    clear_all_rtns();
}

void goto_dot_dot(bool force_new) {
    int command;
    arg_struct arg;
    if (prgms_count != 0 && !force_new) {
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
            && (argtype == ARGTYPE_NUM || argtype == ARGTYPE_STK
                                       || argtype == ARGTYPE_LCLBL))
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
                || arg->type == ARGTYPE_LCLBL
                || arg->type == ARGTYPE_STK)) {
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

static void invalidate_lclbls(int prgm_index, bool force) {
    prgm_struct *prgm = prgms + prgm_index;
    if (force || !prgm->lclbl_invalid) {
        int4 pc2 = 0;
        while (pc2 < prgm->size) {
            int command = prgm->text[pc2];
            int argtype = prgm->text[pc2 + 1];
            command |= (argtype & 240) << 4;
            argtype &= 15;
            if ((command == CMD_GTO || command == CMD_XEQ)
                    && (argtype == ARGTYPE_NUM || argtype == ARGTYPE_STK
                                               || argtype == ARGTYPE_LCLBL)) {
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
        invalidate_lclbls(current_prgm, true);
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
    invalidate_lclbls(current_prgm, false);
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
        if (flags.f.printer_exists && (flags.f.trace_print || flags.f.normal_print))
            print_program_line(current_prgm - 1, pc);

        rebuild_label_table();
        invalidate_lclbls(current_prgm, true);
        invalidate_lclbls(current_prgm - 1, true);
        clear_all_rtns();
        draw_varmenu();
        return;
    }

    if ((command == CMD_GTO || command == CMD_XEQ)
            && (arg->type == ARGTYPE_NUM || arg->type == ARGTYPE_STK 
                                         || arg->type == ARGTYPE_LCLBL))
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
    if (command != CMD_END && flags.f.printer_exists && (flags.f.trace_print || flags.f.normal_print))
        print_program_line(current_prgm, pc);
    
    if (command == CMD_END ||
            (command == CMD_LBL && arg->type == ARGTYPE_STR))
        rebuild_label_table();
    else
        update_label_table(current_prgm, pc, bufptr);
    invalidate_lclbls(current_prgm, false);
    clear_all_rtns();
    if (!suppress_varmenu_update)
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
        if (command == CMD_LBL && (argtype == arg->type
                                || argtype == ARGTYPE_STK)) {
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
            } else if (argtype == ARGTYPE_STK) {
                // Synthetic LBL ST T etc.
                // Allow GTO ST T and GTO 112
                char stk = prgm->text[search_pc + 2];
                if (arg->type == ARGTYPE_STK) {
                    if (stk = arg->val.stk)
                        return search_pc;
                } else if (arg->type == ARGTYPE_NUM) {
                    int num = 0;
                    switch (stk) {
                        case 'T': num = 112; break;
                        case 'Z': num = 113; break;
                        case 'Y': num = 114; break;
                        case 'X': num = 115; break;
                        case 'L': num = 116; break;
                    }
                    if (num == arg->val.num)
                        return search_pc;
                }
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
    if (rtn_level == MAX_RTN_LEVEL)
        return ERR_RTN_STACK_FULL;
    if (rtn_sp == rtn_stack_capacity) {
        int new_rtn_stack_capacity = rtn_stack_capacity + 16;
        rtn_stack_entry *new_rtn_stack = (rtn_stack_entry *) realloc(rtn_stack, new_rtn_stack_capacity * sizeof(rtn_stack_entry));
        if (new_rtn_stack == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        rtn_stack_capacity = new_rtn_stack_capacity;
        rtn_stack = new_rtn_stack;
    }
    rtn_stack[rtn_sp].prgm = prgm & 0x7fffffff;
    rtn_stack[rtn_sp].pc = pc;
    rtn_sp++;
    rtn_level++;
    if (prgm == -2)
        rtn_solve_active = true;
    else if (prgm == -3)
        rtn_integ_active = true;
    return ERR_NONE;
}

int push_indexed_matrix(const char *name, int len) {
    if (matedit_mode == 0 || matedit_mode == 2)
        return ERR_NONE;
    if (!string_equals(name, len, matedit_name, matedit_length))
        return ERR_NONE;
    if (matedit_mode == 3)
        return ERR_RESTRICTED_OPERATION;
    
    if (rtn_level == 0) {
        if (rtn_level_0_has_matrix_entry)
            return ERR_NONE;
        if (rtn_sp + 2 > rtn_stack_capacity) {
            int new_rtn_stack_capacity = rtn_stack_capacity + 16;
            rtn_stack_entry *new_rtn_stack = (rtn_stack_entry *) realloc(rtn_stack, new_rtn_stack_capacity * sizeof(rtn_stack_entry));
            if (new_rtn_stack == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            rtn_stack_capacity = new_rtn_stack_capacity;
            rtn_stack = new_rtn_stack;
        }
        rtn_level_0_has_matrix_entry = true;
        rtn_sp += 2;
        rtn_stack_matrix_name_entry e1;
        int dlen;
        string_copy(e1.name, &dlen, name, len);
        e1.length = dlen;
        memcpy(&rtn_stack[rtn_sp - 1], &e1, sizeof(e1));
        rtn_stack_matrix_ij_entry e2;
        e2.i = matedit_i;
        e2.j = matedit_j;
        memcpy(&rtn_stack[rtn_sp - 2], &e2, sizeof(e2));
    } else {
        if ((rtn_stack[rtn_sp - 1].prgm & 0x80000000) != 0)
            return ERR_NONE;
        if (rtn_sp + 2 > rtn_stack_capacity) {
            int new_rtn_stack_capacity = rtn_stack_capacity + 16;
            rtn_stack_entry *new_rtn_stack = (rtn_stack_entry *) realloc(rtn_stack, new_rtn_stack_capacity * sizeof(rtn_stack_entry));
            if (new_rtn_stack == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            rtn_stack_capacity = new_rtn_stack_capacity;
            rtn_stack = new_rtn_stack;
        }
        rtn_sp += 2;
        rtn_stack[rtn_sp - 1] = rtn_stack[rtn_sp - 3];
        rtn_stack[rtn_sp - 1].prgm |= 0x80000000;
        rtn_stack_matrix_name_entry e1;
        int dlen;
        string_copy(e1.name, &dlen, name, len);
        e1.length = dlen;
        memcpy(&rtn_stack[rtn_sp - 2], &e1, sizeof(e1));
        rtn_stack_matrix_ij_entry e2;
        e2.i = matedit_i;
        e2.j = matedit_j;
        memcpy(&rtn_stack[rtn_sp - 3], &e2, sizeof(e2));
    }
    matedit_mode = 0;
    return ERR_NONE;
}

void step_out() {
    if (rtn_sp > 0)
        rtn_stop_level = rtn_level - 1;
}

void step_over() {
    if (rtn_sp >= 0)
        rtn_stop_level = rtn_level;
}

bool should_i_stop_at_this_level() {
    bool stop = rtn_stop_level >= rtn_level;
    if (stop)
        rtn_stop_level = -1;
    return stop;
}

static void remove_locals() {
    int last = -1;
    for (int i = vars_count - 1; i >= 0; i--) {
        if (vars[i].level == -1)
            continue;
        if (vars[i].level < rtn_level)
            break;
        if ((matedit_mode == 1 || matedit_mode == 3)
                && string_equals(vars[i].name, vars[i].length, matedit_name, matedit_length)) {
            if (matedit_mode == 3) {
                set_appmenu_exitcallback(0);
                set_menu(MENULEVEL_APP, MENU_NONE);
            }
            matedit_mode = 0;
        }
        if (vars[i].hiding) {
            for (int j = i - 1; j >= 0; j--)
                if (vars[j].hidden && string_equals(vars[i].name, vars[i].length, vars[j].name, vars[j].length)) {
                    vars[j].hidden = false;
                    break;
                }
        }
        free_vartype(vars[i].value);
        vars[i].length = 100;
        last = i;
    }
    if (last == -1)
        return;
    int from = last;
    int to = last;
    while (from < vars_count) {
        if (vars[from].length != 100)
            vars[to++] = vars[from];
        from++;
    }
    vars_count -= from - to;
    update_catalog();
}

void pop_rtn_addr(int *prgm, int4 *pc, bool *stop) {
    remove_locals();
    if (rtn_level == 0) {
        *prgm = -1;
        *pc = -1;
        rtn_stop_level = -1;
        if (rtn_level_0_has_matrix_entry) {
            rtn_level_0_has_matrix_entry = false;
            restore_indexed_matrix:
            rtn_stack_matrix_name_entry e1;
            memcpy(&e1, &rtn_stack[--rtn_sp], sizeof(e1));
            string_copy(matedit_name, &matedit_length, e1.name, e1.length);
            rtn_stack_matrix_ij_entry e2;
            memcpy(&e2, &rtn_stack[--rtn_sp], sizeof(e2));
            matedit_i = e2.i;
            matedit_j = e2.j;
            matedit_mode = 1;
        }
    } else {
        rtn_sp--;
        rtn_level--;
        int4 tprgm = rtn_stack[rtn_sp].prgm;
        *prgm = tprgm & 0x7fffffff;
        // Fix sign, or -2 and -3 won't work!
        if ((tprgm & 0x40000000) != 0)
            *prgm |= 0x80000000;
        *pc = rtn_stack[rtn_sp].pc;
        if (rtn_stop_level >= rtn_level) {
            *stop = true;
            rtn_stop_level = -1;
        } else
            *stop = false;
        if (*prgm == -2)
            rtn_solve_active = false;
        else if (*prgm == -3)
            rtn_integ_active = false;
        if ((tprgm & 0x80000000) != 0)
            goto restore_indexed_matrix;
    }
}

void pop_indexed_matrix(const char *name, int namelen) {
    if (rtn_level == 0) {
        if (rtn_level_0_has_matrix_entry) {
            rtn_stack_matrix_name_entry e1;
            memcpy(&e1, &rtn_stack[rtn_sp - 1], sizeof(e1));
            if (string_equals(e1.name, e1.length, name, namelen)) {
                rtn_level_0_has_matrix_entry = false;
                string_copy(matedit_name, &matedit_length, e1.name, e1.length);
                rtn_stack_matrix_ij_entry e2;
                memcpy(&e2, &rtn_stack[rtn_sp - 2], sizeof(e2));
                matedit_i = e2.i;
                matedit_j = e2.j;
                matedit_mode = 1;
                rtn_sp -= 2;
            }
        }
    } else {
        int4 tprgm = rtn_stack[rtn_sp - 1].prgm;
        if ((tprgm & 0x80000000) != 0) {
            rtn_stack_matrix_name_entry e1;
            memcpy(&e1, &rtn_stack[rtn_sp - 2], sizeof(e1));
            if (string_equals(e1.name, e1.length, name, namelen)) {
                string_copy(matedit_name, &matedit_length, e1.name, e1.length);
                rtn_stack_matrix_ij_entry e2;
                memcpy(&e2, &rtn_stack[rtn_sp - 3], sizeof(e2));
                matedit_i = e2.i;
                matedit_j = e2.j;
                matedit_mode = 1;
                rtn_stack[rtn_sp - 3].prgm = tprgm & 0x7fffffff;
                rtn_stack[rtn_sp - 3].pc = rtn_stack[rtn_sp - 1].pc;
                rtn_sp -= 2;
            }
        }
    }
}

void clear_all_rtns() {
    int dummy1;
    int4 dummy2;
    bool dummy3;
    while (rtn_level > 0)
        pop_rtn_addr(&dummy1, &dummy2, &dummy3);
    pop_rtn_addr(&dummy1, &dummy2, &dummy3);
}

int get_rtn_level() {
    return rtn_level;
}

bool solve_active() {
    return rtn_solve_active;
}

bool integ_active() {
    return rtn_integ_active;
}

bool unwind_stack_until_solve() {
    int prgm;
    int4 pc;
    bool stop;
    do {
        pop_rtn_addr(&prgm, &pc, &stop);
    } while (prgm != -2);
    return stop;
}

bool read_bool(bool *b) {
    if (state_bool_is_int) {
        int t;
        if (!read_int(&t))
            return false;
        if (t != 0 && t != 1)
            return false;
        *b = t != 0;
        return true;
    } else {
        return read_char((char *) b);
    }
}

bool write_bool(bool b) {
    return fputc((char) b, gfile) != EOF;
}

bool read_char(char *c) {
    int i = fgetc(gfile);
    *c = (char) i;
    return i != EOF;
}

bool write_char(char c) {
    return fputc(c, gfile) != EOF;
}

bool read_int(int *n) {
    if (state_is_portable) {
        int4 m;
        if (!read_int4(&m))
            return false;
        *n = (int) m;
        return true;
    } else
        return fread(n, 1, sizeof(int), gfile) == sizeof(int);
}

bool write_int(int n) {
    return write_int4(n);
}

bool read_int2(int2 *n) {
    #ifdef F42_BIG_ENDIAN
        if (state_is_portable) {
            char buf[2];
            if (fread(buf, 1, 2, gfile) != 2)
                return false;
            char *dst = (char *) n;
            for (int i = 0; i < 2; i++)
                dst[i] = buf[1 - i];
            return true;
        }
    #endif
        return fread(n, 1, 2, gfile) == 2;
}

bool write_int2(int2 n) {
    #ifdef F42_BIG_ENDIAN
        char buf[2];
        char *src = (char *) &n;
        for (int i = 0; i < 2; i++)
            buf[i] = src[1 - i];
        return fwrite(buf, 1, 2, gfile) == 2;
    #else
        return fwrite(&n, 1, 2, gfile) == 2;
    #endif
}

bool read_int4(int4 *n) {
    #ifdef F42_BIG_ENDIAN
        if (state_is_portable) {
            char buf[4];
            if (fread(buf, 1, 4, gfile) != 4)
                return false;
            char *dst = (char *) n;
            for (int i = 0; i < 4; i++)
                dst[i] = buf[3 - i];
            return true;
        }
    #endif
        return fread(n, 1, 4, gfile) == 4;
}

bool write_int4(int4 n) {
    #ifdef F42_BIG_ENDIAN
        char buf[4];
        char *src = (char *) &n;
        for (int i = 0; i < 4; i++)
            buf[i] = src[3 - i];
        return fwrite(buf, 1, 4, gfile) == 4;
    #else
        return fwrite(&n, 1, 4, gfile) == 4;
    #endif
}

bool read_int8(int8 *n) {
    #ifdef F42_BIG_ENDIAN
        if (state_is_portable) {
            char buf[8];
            if (fread(buf, 1, 8, gfile) != 8)
                return false;
            char *dst = (char *) n;
            for (int i = 0; i < 8; i++)
                dst[i] = buf[7 - i];
            return true;
        }
    #endif
    return fread(n, 1, 8, gfile) == 8;
}

bool write_int8(int8 n) {
    #ifdef F42_BIG_ENDIAN
        char buf[8];
        char *src = (char *) &n;
        for (int i = 0; i < 8; i++)
            buf[i] = src[7 - i];
        return fwrite(buf, 1, 8, gfile) == 8;
    #else
        return fwrite(&n, 1, 8, gfile) == 8;
    #endif
}

bool read_phloat(phloat *d) {
    if (bin_dec_mode_switch()) {
        #ifdef F42_BIG_ENDIAN
            if (state_is_portable) {
                #ifdef BCD_MATH
                    char buf[8];
                    if (fread(buf, 1, 8, gfile) != 8)
                        return false;
                    double dbl;
                    char *dst = (char *) &dbl;
                    for (int i = 0; i < 8; i++)
                        dst[i] = buf[7 - i];
                    *d = dbl;
                    return true;
                #else
                    char buf[16], data[16];
                    if (fread(buf, 1, 16, gfile) != 16)
                        return false;
                    for (int i = 0; i < 16; i++)
                        data[i] = buf[15 - i];
                    *d = decimal2double(data);
                    return true;
                #endif
            }
        #endif
        #ifdef BCD_MATH
            double dbl;
            if (fread(&dbl, 1, 8, gfile) != 8)
                return false;
            *d = dbl;
            return true;
        #else
            char data[16];
            if (fread(data, 1, 16, gfile) != 16)
                return false;
            *d = decimal2double(data);
            return true;
        #endif
    } else {
        #ifdef F42_BIG_ENDIAN
            if (state_is_portable) {
                #ifdef BCD_MATH
                    char buf[16];
                    if (fread(buf, 1, 16, gfile) != 16)
                        return false;
                    char *dst = (char *) d;
                    for (int i = 0; i < 16; i++)
                        dst[i] = buf[15 - i];
                    update_decimal(&d->val);
                    return true;
                #else
                    char buf[8];
                    if (fread(buf, 1, 8, gfile) != 8)
                        return false;
                    char *dst = (char *) d;
                    for (int i = 0; i < 8; i++)
                        dst[i] = buf[7 - i];
                    return true;
                #endif
            }
        #endif
        if (fread(d, 1, sizeof(phloat), gfile) != sizeof(phloat))
            return false;
        #ifdef BCD_MATH
            update_decimal(&d->val);
        #endif
        return true;
    }
}

bool write_phloat(phloat d) {
    #ifdef F42_BIG_ENDIAN
        #ifdef BCD_MATH
            char buf[16];
            char *src = (char *) &d;
            for (int i = 0; i < 16; i++)
                buf[i] = src[15 - i];
            return fwrite(buf, 1, 16, gfile) == 16;
        #else
            char buf[8];
            char *src = (char *) &d;
            for (int i = 0; i < 8; i++)
                buf[i] = src[7 - i];
            return fwrite(buf, 1, 8, gfile) == 8;
        #endif
    #else
        return fwrite(&d, 1, sizeof(phloat), gfile) == sizeof(phloat);
    #endif
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
    if (state_is_portable) {
        if (!read_char((char *) &arg->type))
            return false;
        switch (arg->type) {
            case ARGTYPE_NONE:
                return true;
            case ARGTYPE_NUM:
            case ARGTYPE_NEG_NUM:
            case ARGTYPE_IND_NUM:
            case ARGTYPE_LBLINDEX:
                return read_int4(&arg->val.num);
            case ARGTYPE_STK:
            case ARGTYPE_IND_STK:
                return read_char(&arg->val.stk);
            case ARGTYPE_STR:
            case ARGTYPE_IND_STR:
                return read_char((char *) &arg->length)
                && fread(arg->val.text, 1, arg->length, gfile) == arg->length;
            case ARGTYPE_COMMAND:
                return read_int(&arg->val.cmd);
            case ARGTYPE_LCLBL:
                return read_char(&arg->val.lclbl);
            case ARGTYPE_DOUBLE:
                return read_phloat(&arg->val_d);
            default:
                // Should never happen
                return false;
        }
    }
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
        if (fread(&old_arg, 1, sizeof(old_arg), gfile)
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
        if (fread(&ba, 1, sizeof(bin_arg_struct), gfile)
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
        if (fread(&da, 1, sizeof(dec_arg_struct), gfile)
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
        if (fread(arg, 1, sizeof(dec_arg_struct), gfile)
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
        return fread(arg, 1, sizeof(arg_struct), gfile)
        == sizeof(arg_struct);
#endif
    }
}

bool write_arg(const arg_struct *arg) {
    if (!write_char(arg->type))
        return false;
    switch (arg->type) {
        case ARGTYPE_NONE:
            return true;
        case ARGTYPE_NUM:
        case ARGTYPE_NEG_NUM:
        case ARGTYPE_IND_NUM:
        case ARGTYPE_LBLINDEX:
            return write_int4(arg->val.num);
        case ARGTYPE_STK:
        case ARGTYPE_IND_STK:
            return write_char(arg->val.stk);
        case ARGTYPE_STR:
        case ARGTYPE_IND_STR:
            return write_char(arg->length)
                && fwrite(arg->val.text, 1, arg->length, gfile) == arg->length;
        case ARGTYPE_COMMAND:
            return write_int(arg->val.cmd);
        case ARGTYPE_LCLBL:
            return write_char(arg->val.lclbl);
        case ARGTYPE_DOUBLE:
            return write_phloat(arg->val_d);
        default:
            // Should never happen
            return false;
    }
}

static bool load_state2(int4 ver, bool *clear, bool *too_new) {
    int4 magic;
    int4 version;
    *clear = false;
    *too_new = false;

    /* The shell has verified the initial magic and version numbers,
     * and loaded the shell state, before we got called.
     */

    state_bool_is_int = ver < 9;
    state_is_portable = ver >= 26;

    if (state_is_portable) {
        int4 magic;
        if (!read_int4(&magic))
            return false;
        if (magic != FREE42_MAGIC)
            return false;
        if (!read_int4(&ver)) {
            // A state file containing nothing after the magic number
            // is considered empty, and results in a hard reset. This
            // is *not* an error condition; such state files are used
            // when creating a new state in the States window.
            *clear = true;
            return false;
        }
    }

    if (ver > FREE42_VERSION) {
        *too_new = true;
        return false;
    }

    if (bug_mode == 0 && ver == 26)
        bug_mode = 1;

    if (ver >= 28) {
        // Embedded version information. No need to read this; it's just
        // there for troubleshooting purposes. All we need to do here is
        // skip it.
        while (true) {
            char c;
            if (!read_char(&c))
                return false;
            if (c == 0)
                break;
        }
    }
    
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

    if (ver >= 2) {
        bool bdummy;
        if (!read_bool(&bdummy)) return false;
        if (!read_bool(&bdummy)) return false;
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
    if (ver >= 11) {
        bool dummy;
        if (!read_bool(&dummy)) return false;
    }
    if (ver >= 15 && ver <= 25) {
        bool dummy;
        if (ver < 20) {
            if (!read_bool(&dummy)) return false;
            if (!read_bool(&dummy)) return false;
        }
        if (!read_bool(&dummy)) return false;
        if (!read_bool(&dummy)) return false;
        if (!read_bool(&dummy)) return false;
        if (!read_bool(&dummy)) return false;
    }
    if (ver >= 23 && ver <= 25) {
        bool dummy;
        if (!read_bool(&dummy)) return false;
    }

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
    if (ver < 25) {
        if (mode_appmenu > MENU_MODES2)
            mode_appmenu++;
        if (mode_plainmenu > MENU_MODES2)
            mode_plainmenu++;
        if (mode_transientmenu > MENU_MODES2)
            mode_transientmenu++;
        if (mode_alphamenu > MENU_MODES2)
            mode_alphamenu++;
        if (mode_commandmenu > MENU_MODES2)
            mode_commandmenu++;
    }
    if (!read_bool(&mode_running)) return false;
    if (!read_bool(&mode_varmenu)) return false;
    if (!read_bool(&mode_updown)) return false;

    if (ver < 6)
        mode_getkey = false;
    else if (!read_bool(&mode_getkey))
        return false;

    if (!read_phloat(&entered_number)) return false;
    if (!read_int(&entered_string_length)) return false;
    if (fread(entered_string, 1, 15, gfile) != 15) return false;

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
    if (fread(incomplete_str, 1, 7, gfile) != 7) return false;
    if (!read_int4(&incomplete_saved_pc)) return false;
    if (!read_int4(&incomplete_saved_highlight_row)) return false;

    if (fread(cmdline, 1, 100, gfile) != 100) return false;
    if (!read_int(&cmdline_length)) return false;
    if (!read_int(&cmdline_row)) return false;

    if (!read_int(&matedit_mode)) return false;
    if (fread(matedit_name, 1, 7, gfile) != 7) return false;
    if (!read_int(&matedit_length)) return false;
    if (!unpersist_vartype(&matedit_x, ver < 18)) return false;
    if (!read_int4(&matedit_i)) return false;
    if (!read_int4(&matedit_j)) return false;
    if (!read_int(&matedit_prev_appmenu)) return false;

    if (fread(input_name, 1, 11, gfile) != 11) return false;
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
    if (state_is_portable) {
        for (int i = 0; i < 16; i++)
            if (!read_int(&keybuf[i]))
                return false;
    } else {
        if (fread(keybuf, 1, 16 * sizeof(int), gfile)
                != 16 * sizeof(int))
            return false;
    }

    if (!unpersist_display(ver))
        return false;
    if (!unpersist_globals(ver))
        return false;

    if (ver < 4) {
        /* Before state file version 4, I used to save the BCD table in the
         * state file. We don't use the BCD table any more, so all this code
         * does is skip it.
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
            if (fread(dummy, 1, count, gfile) != count)
                return false;
            n -= count;
        }
    }

#ifdef BCD_MATH
    if (!unpersist_math(ver, state_file_number_format != NUMBER_FORMAT_BID128))
        return false;
#else
    if (!unpersist_math(ver, state_file_number_format != NUMBER_FORMAT_BINARY))
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

// See the comment for bug_mode at its declaration...

bool load_state(int4 ver, bool *clear, bool *too_new) {
    bug_mode = 0;
    long fpos = ftell(gfile);
    if (load_state2(ver, clear, too_new))
        return true;
    if (bug_mode != 3)
        return false;
    // bug_mode == 3 is the signal that the file looks screwy
    // in the way caused by the buggy string-in-matrix writing
    // in version 2.5
    core_cleanup();
    fseek(gfile, fpos, SEEK_SET);
    bug_mode = 2;
    return load_state2(ver, clear, too_new);
}

void save_state() {
    if (!write_int4(FREE42_MAGIC) || !write_int4(FREE42_VERSION))
        return;

    // Write app version and platform, for troubleshooting purposes
    const char *platform = shell_platform();
    char c;
    do {
        c = *platform++;
        write_char(c);
    } while (c != 0);

    #ifdef BCD_MATH
        if (!write_bool(true)) return;
    #else
        if (!write_bool(false)) return;
    #endif
    if (!write_bool(core_settings.matrix_singularmatrix)) return;
    if (!write_bool(core_settings.matrix_outofrange)) return;
    if (!write_bool(core_settings.auto_repeat)) return;
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
    if (fwrite(entered_string, 1, 15, gfile) != 15) return;

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
    if (fwrite(incomplete_str, 1, 7, gfile) != 7) return;
    if (!write_int4(pc2line(incomplete_saved_pc))) return;
    if (!write_int4(incomplete_saved_highlight_row)) return;

    if (fwrite(cmdline, 1, 100, gfile) != 100) return;
    if (!write_int(cmdline_length)) return;
    if (!write_int(cmdline_row)) return;

    if (!write_int(matedit_mode)) return;
    if (fwrite(matedit_name, 1, 7, gfile) != 7) return;
    if (!write_int(matedit_length)) return;
    if (!persist_vartype(matedit_x)) return;
    if (!write_int4(matedit_i)) return;
    if (!write_int4(matedit_j)) return;
    if (!write_int(matedit_prev_appmenu)) return;

    if (fwrite(input_name, 1, 11, gfile) != 11) return;
    if (!write_int(input_length)) return;
    if (!write_arg(&input_arg)) return;

    if (!write_int(baseapp)) return;

    if (!write_int8(random_number_low)) return;
    if (!write_int8(random_number_high)) return;

    if (!write_int(deferred_print)) return;

    if (!write_int(keybuf_head)) return;
    if (!write_int(keybuf_tail)) return;
    for (int i = 0; i < 16; i++)
        if (!write_int(keybuf[i]))
            return;

    if (!persist_display())
        return;
    if (!persist_globals())
        return;
    if (!persist_math())
        return;

    if (!write_int4(FREE42_MAGIC)) return;
    if (!write_int4(FREE42_VERSION)) return;
}

// Reason:
// 0 = Memory Clear
// 1 = State File Corrupt
// 2 = State File Too New
void hard_reset(int reason) {
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

    /* Clear RTN stack */
    if (rtn_stack != NULL)
        free(rtn_stack);
    rtn_stack_capacity = 16;
    rtn_stack = (rtn_stack_entry *) malloc(rtn_stack_capacity * sizeof(rtn_stack_entry));
    rtn_sp = 0;
    rtn_level = 0;
    rtn_stop_level = -1;
    rtn_solve_active = false;
    rtn_integ_active = false;

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
    goto_dot_dot(false);

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
    /* flags.f.VIRTUAL_continuous_on = 0; */
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
    flags.f.shift_state = 0;
    /* flags.f.VIRTUAL_matrix_editor = 0; */
    flags.f.grow = 0;
    flags.f.ymd = 0;
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
    flags.f.base_signed = 1;
    flags.f.base_wrap = 0;
    flags.f.f80 = flags.f.f81 = flags.f.f82 = flags.f.f83 = flags.f.f84 = 0;
    flags.f.f85 = flags.f.f86 = flags.f.f87 = flags.f.f88 = flags.f.f89 = 0;
    flags.f.f90 = flags.f.f91 = flags.f.f92 = flags.f.f93 = flags.f.f94 = 0;
    flags.f.f95 = flags.f.f96 = flags.f.f97 = flags.f.f98 = flags.f.f99 = 0;

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
    mode_wsize = 36;

    reset_math();

    clear_display();
    clear_custom_menu();
    clear_prgm_menu();
    switch (reason) {
        case 0:
            draw_string(0, 0, "Memory Clear", 12);
            break;
        case 1:
            draw_string(0, 0, "State File Corrupt", 18);
            break;
        case 2:
            draw_string(0, 0, "State File Too New", 18);
            break;
    }
    display_x(1);
    flush_display();
}

static bool convert_programs(bool *clear_stack) {
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
    bool success = false;
    *clear_stack = false;

    // Since converting programs can cause instructions to move, I have to
    // update all stored PC values to correct for this. PCs are stored in the
    // 'pc' and 'rtn_pc[]' globals. I copy those values into a local array,
    // which I then sort by program index and pc; this allows me to do the
    // updates very efficiently later on.
    int *mod_prgm = (int *) malloc((rtn_level + 2) * sizeof(int));
    int4 *mod_pc = (int4 *) malloc((rtn_level + 2) * sizeof(int4));
    int *mod_sp = (int *) malloc((rtn_level + 2) * sizeof(int));
    if (mod_prgm == NULL || mod_pc == NULL || mod_sp == NULL) {
        end:
        free(mod_prgm);
        free(mod_pc);
        free(mod_sp);
        return success;
    }
    int mod_count = 0;
    int sp = rtn_sp;
    if (rtn_solve_active || rtn_integ_active) {
        *clear_stack = true;
    } else {
        for (i = 0; i < rtn_level; i++) {
            sp--;
            int4 prgm = rtn_stack[sp].prgm;
            mod_prgm[mod_count] = prgm & 0x7fffffff;
            mod_pc[mod_count] = rtn_stack[sp].pc;
            mod_sp[mod_count] = sp;
            if ((prgm & 0x80000000) != 0)
                sp -= 2;
            mod_count++;
        }
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
                    rtn_stack[sp].pc = pc;
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
                    && (argtype == ARGTYPE_NUM || argtype == ARGTYPE_STK
                                               || argtype == ARGTYPE_LCLBL)) {
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
                                goto end;
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
    success = true;
    goto end;
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
                    && (argtype == ARGTYPE_NUM || argtype == ARGTYPE_STK
                                               || argtype == ARGTYPE_LCLBL)) {
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
