/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2025  Thomas Okken
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
#include "core_commands7.h"
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

const error_spec errors[] = {
    { /* NONE */                   NULL,                       0 },
    { /* ALPHA_DATA_IS_INVALID */  "Alpha Data Is Invalid",   21 },
    { /* OUT_OF_RANGE */           "Out of Range",            12 },
    { /* DIVIDE_BY_0 */            "Divide by 0",             11 },
    { /* INVALID_TYPE */           "Invalid Type",            12 },
    { /* INVALID_DATA */           "Invalid Data",            12 },
    { /* NONEXISTENT */            "Nonexistent",             11 },
    { /* DIMENSION_ERROR */        "Dimension Error",         15 },
    { /* TOO_FEW_ARGUMENTS */      "Too Few Arguments",       17 },
    { /* SIZE_ERROR */             "Size Error",              10 },
    { /* STACK_DEPTH_ERROR */      "Stack Depth Error",       17 },
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
    { /* SINGULAR_MATRIX */        "Singular Matrix",         15 },
    { /* SOLVE_SOLVE */            "Solve(Solve)",            12 },
    { /* INTEG_INTEG */            "Integ(Integ)",            12 },
    { /* RUN */                    NULL,                       0 },
    { /* INTERRUPTED */            "Interrupted",             11 },
    { /* PRINTING_IS_DISABLED */   "Printing Is Disabled",    20 },
    { /* INTERRUPTIBLE */          NULL,                       0 },
    { /* NO_VARIABLES */           "No Variables",            12 },
    { /* INSUFFICIENT_MEMORY */    "Insufficient Memory",     19 },
    { /* NOT_YET_IMPLEMENTED */    "Not Yet Implemented",     19 },
    { /* INTERNAL_ERROR */         "Internal Error",          14 },
    { /* SUSPICIOUS_OFF */         "Suspicious OFF",          14 },
    { /* RTN_STACK_FULL */         "RTN Stack Full",          14 },
    { /* NUMBER_TOO_LARGE */       "Number Too Large",        16 },
    { /* NUMBER_TOO_SMALL */       "Number Too Small",        16 },
    { /* BIG_STACK_DISABLED */     "Big Stack Disabled",      18 },
    { /* INVALID_CONTEXT */        "Invalid Context",         15 },
    { /* NAME_TOO_LONG */          "Name Too Long",           13 },
    { /* PROGRAM_LOCKED */         "Program Locked",          14 },
    { /* NEXT_PROGRAM_LOCKED */    "Next Program Locked",     19 },
};


const menu_spec menus[] = {
    { /* MENU_ALPHA1 */ MENU_NONE, MENU_ALPHA2, MENU_ALPHA2,
                      { { MENU_ALPHA_ABCDE1, 5, "ABCDE" },
                        { MENU_ALPHA_FGHI,   4, "FGHI"  },
                        { MENU_ALPHA_JKLM,   4, "JKLM"  },
                        { MENU_ALPHA_NOPQ1,  4, "NOPQ"  },
                        { MENU_ALPHA_RSTUV1, 5, "RSTUV" },
                        { MENU_ALPHA_WXYZ,   4, "WXYZ"  } } },
    { /* MENU_ALPHA2 */ MENU_NONE, MENU_ALPHA1, MENU_ALPHA1,
                      { { MENU_ALPHA_PAREN, 5, "( [ {"   },
                        { MENU_ALPHA_ARROW, 3, "\20^\16" },
                        { MENU_ALPHA_COMP,  5, "< = >"   },
                        { MENU_ALPHA_MATH,  4, "MATH"    },
                        { MENU_ALPHA_PUNC1, 4, "PUNC"    },
                        { MENU_ALPHA_MISC1, 4, "MISC"    } } },
    { /* MENU_ALPHA_ABCDE1 */ MENU_ALPHA1, MENU_ALPHA_ABCDE2, MENU_ALPHA_ABCDE2,
                      { { MENU_NONE, 1, "A" },
                        { MENU_NONE, 1, "B" },
                        { MENU_NONE, 1, "C" },
                        { MENU_NONE, 1, "D" },
                        { MENU_NONE, 1, "E" },
                        { MENU_NONE, 1, " " } } },
    { /* MENU_ALPHA_ABCDE2 */ MENU_ALPHA1, MENU_ALPHA_ABCDE1, MENU_ALPHA_ABCDE1,
                      { { MENU_NONE, 1, "\26" },
                        { MENU_NONE, 1, "\24" },
                        { MENU_NONE, 1, "\31" },
                        { MENU_NONE, 1, " "   },
                        { MENU_NONE, 1, " "   },
                        { MENU_NONE, 1, " "   } } },
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
                      { { MENU_NONE, 1, "\25" },
                        { MENU_NONE, 1, "\34" },
                        { MENU_NONE, 1, " "   },
                        { MENU_NONE, 1, " "   },
                        { MENU_NONE, 1, " "   },
                        { MENU_NONE, 1, " "   } } },
    { /* MENU_ALPHA_RSTUV1 */ MENU_ALPHA1, MENU_ALPHA_RSTUV2, MENU_ALPHA_RSTUV2,
                      { { MENU_NONE, 1, "R" },
                        { MENU_NONE, 1, "S" },
                        { MENU_NONE, 1, "T" },
                        { MENU_NONE, 1, "U" },
                        { MENU_NONE, 1, "V" },
                        { MENU_NONE, 1, " " } } },
    { /* MENU_ALPHA_RSTUV2 */ MENU_ALPHA1, MENU_ALPHA_RSTUV1, MENU_ALPHA_RSTUV1,
                      { { MENU_NONE, 1, " "   },
                        { MENU_NONE, 1, " "   },
                        { MENU_NONE, 1, " "   },
                        { MENU_NONE, 1, "\35" },
                        { MENU_NONE, 1, " "   },
                        { MENU_NONE, 1, " "   } } },
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
                      { { MENU_NONE, 1, "\20" },
                        { MENU_NONE, 1, "^"   },
                        { MENU_NONE, 1, "\16" },
                        { MENU_NONE, 1, "\17" },
                        { MENU_NONE, 1, " "   },
                        { MENU_NONE, 1, " "   } } },
    { /* MENU_ALPHA_COMP */ MENU_ALPHA2, MENU_NONE, MENU_NONE,
                      { { MENU_NONE, 1, "="   },
                        { MENU_NONE, 1, "\14" },
                        { MENU_NONE, 1, "<"   },
                        { MENU_NONE, 1, ">"   },
                        { MENU_NONE, 1, "\11" },
                        { MENU_NONE, 1, "\13" } } },
    { /* MENU_ALPHA_MATH */ MENU_ALPHA2, MENU_NONE, MENU_NONE,
                      { { MENU_NONE, 1, "\5"  },
                        { MENU_NONE, 1, "\3"  },
                        { MENU_NONE, 1, "\2"  },
                        { MENU_NONE, 1, "\27" },
                        { MENU_NONE, 1, "\23" },
                        { MENU_NONE, 1, "\21" } } },
    { /* MENU_ALPHA_PUNC1 */ MENU_ALPHA2, MENU_ALPHA_PUNC2, MENU_ALPHA_PUNC2,
                      { { MENU_NONE, 1, ","  },
                        { MENU_NONE, 1, ";"  },
                        { MENU_NONE, 1, ":"  },
                        { MENU_NONE, 1, "!"  },
                        { MENU_NONE, 1, "?"  },
                        { MENU_NONE, 1, "\"" } } },
    { /* MENU_ALPHA_PUNC2 */ MENU_ALPHA2, MENU_ALPHA_PUNC1, MENU_ALPHA_PUNC1,
                      { { MENU_NONE, 1, "\32" },
                        { MENU_NONE, 1, "_"   },
                        { MENU_NONE, 1, "`"   },
                        { MENU_NONE, 1, "'"   },
                        { MENU_NONE, 1, "\10" },
                        { MENU_NONE, 1, "\12" } } },
    { /* MENU_ALPHA_MISC1 */ MENU_ALPHA2, MENU_ALPHA_MISC2, MENU_ALPHA_MISC2,
                      { { MENU_NONE, 1, "$"   },
                        { MENU_NONE, 1, "*"   },
                        { MENU_NONE, 1, "#"   },
                        { MENU_NONE, 1, "/"   },
                        { MENU_NONE, 1, "\37" },
                        { MENU_NONE, 1, " "   } } },
    { /* MENU_ALPHA_MISC2 */ MENU_ALPHA2, MENU_ALPHA_MISC1, MENU_ALPHA_MISC1,
                      { { MENU_NONE, 1, "\22" },
                        { MENU_NONE, 1, "&"   },
                        { MENU_NONE, 1, "@"   },
                        { MENU_NONE, 1, "\\"  },
                        { MENU_NONE, 1, "~"   },
                        { MENU_NONE, 1, "|"   } } },
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
    { /* MENU_MODES1 */ MENU_NONE, MENU_MODES2, MENU_MODES5,
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
    { /* MENU_MODES3 */ MENU_NONE, MENU_MODES4, MENU_MODES2,
                      { { 0x1000 + CMD_WSIZE,   0, "" },
                        { 0x1000 + CMD_WSIZE_T, 0, "" },
                        { 0x2000 + CMD_BSIGNED, 0, "" },
                        { 0x2000 + CMD_BWRAP,   0, "" },
                        { 0x1000 + CMD_NULL,    0, "" },
                        { 0x1000 + CMD_BRESET,  0, "" } } },
    { /* MENU_MODES4 */ MENU_NONE, MENU_MODES5, MENU_MODES3,
                      { { 0x2000 + CMD_MDY,     0, "" },
                        { 0x2000 + CMD_DMY,     0, "" },
                        { 0x2000 + CMD_YMD,     0, "" },
                        { 0x1000 + CMD_NULL,    0, "" },
                        { 0x2000 + CMD_CLK12,   0, "" },
                        { 0x2000 + CMD_CLK24,   0, "" } } },
    { /* MENU_MODES5 */ MENU_NONE, MENU_MODES1, MENU_MODES4,
                      { { 0x2000 + CMD_4STK,    0, "" },
                        { 0x2000 + CMD_NSTK,    0, "" },
                        { 0x2000 + CMD_CAPS,    0, "" },
                        { 0x2000 + CMD_MIXED,   0, "" },
                        { 0x1000 + CMD_NULL,    0, "" },
                        { 0x1000 + CMD_NULL,    0, "" } } },
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
                        { 0x1000 + CMD_PRREG, 0, "" },
                        { 0x1000 + CMD_DELAY, 0, "" } } },
    { /* MENU_PRINT3 */ MENU_NONE, MENU_PRINT1, MENU_PRINT2,
                      { { 0x2000 + CMD_PON,    0, "" },
                        { 0x2000 + CMD_POFF,   0, "" },
                        { 0x2000 + CMD_MAN,    0, "" },
                        { 0x2000 + CMD_NORM,   0, "" },
                        { 0x2000 + CMD_TRACE,  0, "" },
                        { 0x2000 + CMD_STRACE, 0, "" } } },
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
                        { 0,                 1, "\3"   } } }
};


/* By how much do the variables, programs, and labels
 * arrays grow when they are full
 */
#define VARS_INCREMENT 25
#define PRGMS_INCREMENT 10
#define LABELS_INCREMENT 10

/* Registers */
vartype **stack = NULL;
int sp = -1;
int stack_capacity = 0;
vartype *lastx = NULL;
int reg_alpha_length = 0;
char reg_alpha[44];

/* Flags */
flags_struct flags;
const char *virtual_flags =
    /* 00-49 */ "00000000000000000000000000010000000000000000111111"
    /* 50-99 */ "00010000000000010000000001000000000000000000000000";

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
int (*mode_interruptible)(bool) = NULL;
bool mode_stoppable;
bool mode_command_entry;
char mode_number_entry;
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
bool mode_getkey1;
bool mode_pause = false;
bool mode_disable_stack_lift; /* transient */
bool mode_caller_stack_lift_disabled;
bool mode_varmenu;
bool mode_updown;
int4 mode_sigma_reg;
int mode_goose;
bool mode_time_clktd;
bool mode_time_clk24;
int mode_wsize;
bool mode_menu_caps;
#if defined(ANDROID) || defined(IPHONE)
bool mode_popup_unknown = true;
#endif

phloat entered_number;
int entered_string_length;
char entered_string[15];

int pending_command;
arg_struct pending_command_arg;
int xeq_invisible;

/* Multi-keystroke commands -- edit state */
/* Relevant when mode_command_entry != 0 */
int incomplete_command;
bool incomplete_ind;
bool incomplete_alpha;
int incomplete_length;
int incomplete_maxdigits;
int incomplete_argtype;
int incomplete_num;
char incomplete_str[22];
int4 incomplete_saved_pc;
int4 incomplete_saved_highlight_row;

/* Command line handling temporaries */
char cmdline[100];
int cmdline_length;
int cmdline_row;

/* Matrix editor / matrix indexing */
int matedit_mode; /* 0=off, 1=index, 2=edit, 3=editn */
int matedit_level;
char matedit_name[7];
int matedit_length;
vartype *matedit_x;
int4 matedit_i;
int4 matedit_j;
int matedit_prev_appmenu;
int4 *matedit_stack = NULL;
int matedit_stack_depth = 0;
bool matedit_is_list;

/* INPUT */
char input_name[11];
int input_length;
arg_struct input_arg;

/* ERRMSG/ERRNO */
int lasterr = 0;
int lasterr_length;
char lasterr_text[22];

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
 * (Versions before 26 (2.5) are no longer supported.)
 *
 * Version 26: 2.5    Separate and portable core state file
 * Version 27: 2.5.2  Recovery mode for corrupt 2.5 state files
 * Version 28: 2.5.3  Recording platform name and app version in state file
 * Version 29: 2.5.7  SOLVE: Tracking second best guess in order to be able to
 *                    report it accurately in Y, and to provide additional data
 *                    points for distinguishing between zeroes and poles.
 * Version 30: 2.5.23 Private local variables
 * Version 31: 2.5.24 Merge RTN and FRT functionality
 * Version 32: 2.5.24 Replace FUNC[012] with FUNC [0-4][0-4]
 * Version 33: 3.0    Big stack; parameterized RTNERR
 * Version 34: 3.0    Long strings
 * Version 35: 3.0    Changing 'int' to 'bool' where appropriate
 * Version 36-38:     Plus42 stuff
 * Version 39: 3.0.3  ERRMSG/ERRNO
 * Version 40: 3.0.3  Longer incomplete_str buffer
 * Version 41: 3.0.3  Plus42 stuff
 * Version 42: 3.0.6  CAPS/Mixed for menus
 * Version 43: 3.0.7  Plus42 stuff
 * Version 44: 3.0.8  cursor left, cursor right, del key handling
 * Version 45: 3.0.12 SOLVE secant impatience
 * Version 46: 3.1    CSLD?
 * Version 47: 3.1    Back-port of Plus42 RTN stack; FUNC stack hiding
 * Version 48: 3.1    Matrix editor nested lists
 * Version 49: 3.1.13 Program locking
 */
#define FREE42_VERSION 49


/*******************/
/* Private globals */
/*******************/

struct rtn_stack_entry {
    int4 prgm;
    int4 pc;
    int4 get_prgm() {
        int4 p = prgm & 0x1fffffff;
        if ((p & 0x10000000) != 0)
            p |= 0xe0000000;
        return p;
    }
    void set_prgm(int4 prgm) {
        this->prgm = prgm & 0x1fffffff;
    }
    bool has_matrix() {
        return (prgm & 0x80000000) != 0;
    }
    void set_has_matrix(bool state) {
        if (state)
            prgm |= 0x80000000;
        else
            prgm &= 0x7fffffff;
    }
    bool has_func() {
        return (prgm & 0x40000000) != 0;
    }
    void set_has_func(bool state) {
        if (state)
            prgm |= 0x40000000;
        else
            prgm &= 0xbfffffff;
    }
    bool is_csld() {
        return (prgm & 0x20000000) != 0;
    }
    void set_csld() {
        if (flags.f.stack_lift_disable)
            prgm |= 0x20000000;
        else
            prgm &= 0xdfffffff;
    }
};

#define MAX_RTN_LEVEL 1024
static int rtn_stack_capacity = 0;
static rtn_stack_entry *rtn_stack = NULL;
static int rtn_level = 0;
static bool rtn_level_0_has_matrix_entry;
static bool rtn_level_0_has_func_state;
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

struct matrix_persister {
    int type;
    int4 rows;
    int4 columns;
};

static int array_count;
static int array_list_capacity;
static void **array_list;


static bool array_list_grow();
static int array_list_search(void *array);
static bool persist_vartype(vartype *v);
static bool unpersist_vartype(vartype **v);
static void update_label_table(int prgm, int4 pc, int inserted);
static void invalidate_lclbls(int prgm_index, bool force);
static int pc_line_convert(int4 loc, int loc_is_pc);

#ifdef BCD_MATH
#define bin_dec_mode_switch() ( state_file_number_format == NUMBER_FORMAT_BINARY )
#else
#define bin_dec_mode_switch() ( state_file_number_format != NUMBER_FORMAT_BINARY )
#endif


void vartype_string::trim1() {
    if (length > SSLENV + 1) {
        memmove(t.ptr, t.ptr + 1, --length);
    } else if (length == SSLENV + 1) {
        char temp[SSLENV];
        memcpy(temp, t.ptr + 1, --length);
        free(t.ptr);
        memcpy(t.buf, temp, length);
    } else if (length > 0) {
        memmove(t.buf, t.buf + 1, --length);
    }
}

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
            return write_int4(s->length)
                && fwrite(s->txt(), 1, s->length, gfile) == s->length;
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
                    if (rm->array->is_string[i] == 0) {
                        if (!write_phloat(rm->array->data[i]))
                            return false;
                    } else {
                        char *text;
                        int4 len;
                        get_matrix_string(rm, i, &text, &len);
                        if (!write_int4(len))
                            return false;
                        if (fwrite(text, 1, len, gfile) != len)
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
        case TYPE_LIST: {
            vartype_list *list = (vartype_list *) v;
            int4 size = list->size;
            int data_index = -1;
            bool must_write = true;
            if (list->array->refcount > 1) {
                int n = array_list_search(list->array);
                if (n == -1) {
                    // data_index == -2 indicates a new shared list
                    data_index = -2;
                    if (!array_list_grow())
                        return false;
                    array_list[array_count++] = list->array;
                } else {
                    // data_index >= 0 refers to a previously shared list
                    data_index = n;
                    must_write = false;
                }
            }
            write_int4(size);
            write_int(data_index);
            if (must_write) {
                for (int4 i = 0; i < list->size; i++)
                    if (!persist_vartype(list->array->data[i]))
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

struct old_vartype_string {
    int type;
    int length;
    char text[6];
};

// For coping with bad 2.5 state files. 0 means don't do anything special;
// 1 means check for bad string lengths in real matrices; 2 is bug-
// compatibility mode; and 3 is a signal to switch from mode 1 to mode 2.
// When a bad string length is found in mode 1, this function will switch
// the mode to mode 3 and return false, indicating an error; the caller
// should then clean up what has already been read, rewind the state file,
// and try again in mode 2.

int bug_mode;

// Using a global for 'ver' so we don't have to pass it around all the time

int4 ver;

static bool unpersist_vartype(vartype **v) {
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
            int4 len;
            if (ver < 34) {
                char c;
                if (!read_char(&c))
                    return false;
                len = c;
            } else {
                if (!read_int4(&len))
                    return false;
            }
            vartype_string *s = (vartype_string *) new_string(NULL, len);
            if (s == NULL)
                return false;
            if (fread(s->txt(), 1, len, gfile) != len) {
                free_vartype((vartype *) s);
                return false;
            }
            *v = (vartype *) s;
            return true;
        }
        case TYPE_REALMATRIX: {
            int4 rows, columns;
            if (!read_int4(&rows) || !read_int4(&columns))
                return false;
            if (rows == 0) {
                // Shared matrix
                vartype *m = dup_vartype((vartype *) array_list[columns]);
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
            int4 i;
            for (i = 0; i < size; i++) {
                success = false;
                if (rm->array->is_string[i] == 0) {
                    if (!read_phloat(&rm->array->data[i]))
                        break;
                } else {
                    rm->array->is_string[i] = 1;
                    if (bug_mode == 0) {
                        if (ver < 34) {
                            // 6 bytes of text followed by length byte
                            char *t = (char *) &rm->array->data[i];
                            if (fread(t + 1, 1, 7, gfile) != 7)
                                break;
                            t[0] = t[7];
                        } else {
                            // 4-byte length followed by n bytes of text
                            int4 len;
                            if (!read_int4(&len))
                                break;
                            if (len > SSLENM) {
                                int4 *p = (int4 *) malloc(len + 4);
                                if (p == NULL)
                                    break;
                                if (fread(p + 1, 1, len, gfile) != len) {
                                    free(p);
                                    break;
                                }
                                *p = len;
                                *(int4 **) &rm->array->data[i] = p;
                                rm->array->is_string[i] = 2;
                            } else {
                                char *t = (char *) &rm->array->data[i];
                                *t = len;
                                if (fread(t + 1, 1, len, gfile) != len)
                                    break;
                            }
                        }
                    } else if (bug_mode == 1) {
                        // Could be as above, or could be length-prefixed.
                        // Read 7 bytes, and if byte 7 looks plausible,
                        // carry on; otherwise, set bug_mode to 3, signalling
                        // we should start over in bug-compatibility mode.
                        char *t = (char *) &rm->array->data[i];
                        if (fread(t + 1, 1, 7, gfile) != 7)
                            break;
                        if (t[7] < 0 || t[7] > 6) {
                            bug_mode = 3;
                            break;
                        }
                        t[0] = t[7];
                    } else {
                        // bug_mode == 2, means this has to be a file with
                        // length-prefixed strings in matrices. Bear in
                        // mind that the prefixes are bogus, so for reading,
                        // clamp them to the 0..6 range, but for advancing
                        // in the file, take them at face value.
                        unsigned char len;
                        if (fread(&len, 1, 1, gfile) != 1)
                            break;
                        unsigned char reallen = len > 6 ? 6 : len;
                        char *t = (char *) &rm->array->data[i];
                        if (fread(t + 1, 1, reallen, gfile) != reallen)
                            break;
                        t[0] = reallen;
                        len -= reallen;
                        if (len > 0 && fseek(gfile, len, SEEK_CUR) != 0)
                            break;
                    }
                }
                success = true;
            }
            if (!success) {
                memset(rm->array->is_string + i, 0, size - i);
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
                vartype *m = dup_vartype((vartype *) array_list[columns]);
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
        case TYPE_LIST: {
            int4 size;
            int data_index;
            if (!read_int4(&size) || !read_int(&data_index))
                return false;
            if (data_index >= 0) {
                // Shared list
                vartype *m = dup_vartype((vartype *) array_list[data_index]);
                if (m == NULL)
                    return false;
                else {
                    *v = m;
                    return true;
                }
            }
            bool shared = data_index == -2;
            vartype_list *list = (vartype_list *) new_list(size);
            if (list == NULL)
                return false;
            if (shared) {
                if (!array_list_grow()) {
                    free_vartype((vartype *) list);
                    return false;
                }
                array_list[array_count++] = list;
            }
            for (int4 i = 0; i < size; i++) {
                if (!unpersist_vartype(&list->array->data[i])) {
                    free_vartype((vartype *) list);
                    return false;
                }
            }
            *v = (vartype *) list;
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

    if (!write_int(sp))
        goto done;
    for (int i = 0; i <= sp; i++)
        if (!persist_vartype(stack[i]))
            goto done;
    if (!persist_vartype(lastx))
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
    if (!write_bool(mode_menu_caps))
        goto done;
    if (fwrite(&flags, 1, sizeof(flags_struct), gfile) != sizeof(flags_struct))
        goto done;
    if (!write_int(prgms_count))
        goto done;
    for (i = 0; i < prgms_count; i++)
        core_export_programs(1, &i, NULL);
    for (i = 0; i < prgms_count; i++)
        if (!write_bool(prgms[i].locked))
            goto done;
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
            || !write_int2(vars[i].flags)
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
    if (!write_int(rtn_level))
        goto done;
    if (!write_bool(rtn_level_0_has_matrix_entry))
        goto done;
    if (!write_bool(rtn_level_0_has_func_state))
        goto done;
    int saved_prgm;
    saved_prgm = current_prgm;
    for (i = rtn_level - 1; i >= 0; i--) {
        current_prgm = rtn_stack[i].get_prgm();
        int4 line = rtn_stack[i].pc;
        if (current_prgm >= 0)
            line = pc2line(line);
        if (!write_int4(rtn_stack[i].prgm) || !write_int4(line))
            goto done;
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

bool loading_state = false;

static bool unpersist_globals() {
    int i;
    array_count = 0;
    array_list_capacity = 0;
    array_list = NULL;
    bool ret = false;
    char tmp_dmy = 2;

    if (ver < 33) {
        sp = 3;
    } else {
        if (!read_int(&sp)) {
            sp = -1;
            goto done;
        }
    }
    stack_capacity = sp + 1;
    if (stack_capacity < 4)
        stack_capacity = 4;
    stack = (vartype **) malloc(stack_capacity * sizeof(vartype *));
    if (stack == NULL) {
        stack_capacity = 0;
        sp = -1;
        goto done;
    }
    for (int i = 0; i <= sp; i++) {
        if (!unpersist_vartype(&stack[i]) || stack[i] == NULL) {
            for (int j = 0; j < i; j++)
                free_vartype(stack[j]);
            free(stack);
            stack = NULL;
            sp = -1;
            stack_capacity = 0;
            goto done;
        }
    }
    if (ver < 33) {
        vartype *tmp = stack[REG_X];
        stack[REG_X] = stack[REG_T];
        stack[REG_T] = tmp;
        tmp = stack[REG_Y];
        stack[REG_Y] = stack[REG_Z];
        stack[REG_Z] = tmp;
    }

    free_vartype(lastx);
    if (!unpersist_vartype(&lastx))
        goto done;

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
    if (!read_bool(&mode_time_clktd)) {
        mode_time_clktd = false;
        goto done;
    }
    if (!read_bool(&mode_time_clk24)) {
        mode_time_clk24 = false;
        goto done;
    }
    if (!read_int(&mode_wsize)) {
        mode_wsize = 36;
        goto done;
    }
    if (ver >= 42) {
        if (!read_bool(&mode_menu_caps)) {
            mode_menu_caps = false;
            goto done;
        }
    } else
        mode_menu_caps = false;
    if (fread(&flags, 1, sizeof(flags_struct), gfile)
            != sizeof(flags_struct))
        goto done;
    if (tmp_dmy != 2)
        flags.f.dmy = tmp_dmy;
    if (ver < 33)
        flags.f.big_stack = 0;

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
    loading_state = true;
    core_import_programs(nprogs, NULL);
    loading_state = false;
    if (ver >= 49)
        for (i = 0; i < nprogs; i++)
            if (!read_bool(&prgms[i].locked))
                goto done;
    if (!read_int(&current_prgm)) {
        current_prgm = 0;
        goto done;
    }
    if (!read_int4(&pc)) {
        pc = -1;
        goto done;
    }
    pc = line2pc(pc);
    incomplete_saved_pc = line2pc(incomplete_saved_pc);
    if (!read_int(&prgm_highlight_row)) {
        prgm_highlight_row = 0;
        goto done;
    }

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
        if (!read_char((char *) &vars[i].length))
            goto vars_fail;
        if (fread(vars[i].name, 1, vars[i].length, gfile) != vars[i].length)
            goto vars_fail;
        if (!read_int2(&vars[i].level))
            goto vars_fail;
        if (ver < 30) {
            bool hidden, hiding;
            if (!read_bool(&hidden) || !read_bool(&hiding))
                goto vars_fail;
            vars[i].flags = (hidden ? VAR_HIDDEN : 0)
                            | (hiding ? VAR_HIDING : 0);
        } else {
            if (!read_int2(&vars[i].flags))
                goto vars_fail;
        }
        if (!unpersist_vartype(&vars[i].value)) {
            vars_fail:
            for (int j = 0; j < i; j++)
                free_vartype(vars[j].value);
            free(vars);
            vars = NULL;
            vars_count = 0;
            goto done;
        }
    }
    vars_capacity = vars_count;

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
    char c;
    for (i = 0; i < 6; i++) {
        if (!read_char(&c)
                || fread(varmenu_labeltext[i], 1, c, gfile) != c)
            goto done;
        varmenu_labellength[i] = c;
    }
    if (!read_int(&varmenu_role))
        goto done;
    if (ver < 47) {
        // rtn_sp; obsolete
        int dummy;
        if (!read_int(&dummy))
            goto done;
    }
    if (!read_int(&rtn_level))
        goto done;
    if (!read_bool(&rtn_level_0_has_matrix_entry))
        goto done;
    if (ver >= 31) {
        if (!read_bool(&rtn_level_0_has_func_state))
            goto done;
    } else {
        rtn_level_0_has_func_state = false;
    }
    rtn_stack_capacity = 16;
    while (rtn_level > rtn_stack_capacity)
        rtn_stack_capacity <<= 1;
    rtn_stack = (rtn_stack_entry *) realloc(rtn_stack, rtn_stack_capacity * sizeof(rtn_stack_entry));
    if (ver >= 47) {
        int saved_prgm = current_prgm;
        for (i = rtn_level - 1; i >= 0; i--) {
            int4 line;
            if (!read_int4(&rtn_stack[i].prgm)) goto done;
            if (!read_int4(&line)) goto done;
            current_prgm = rtn_stack[i].get_prgm();
            if (current_prgm >= 0)
                line = line2pc(line);
            rtn_stack[i].pc = line;
        }
        current_prgm = saved_prgm;
    } else {
        int saved_prgm = current_prgm;
        for (int lvl = rtn_level - 1; lvl >= -1; lvl--) {
            bool matrix_entry_follows;
            if (lvl == -1) {
                matrix_entry_follows = rtn_level_0_has_matrix_entry;
            } else {
                int4 prgm, line;
                if (!read_int4(&prgm) || !read_int4(&line))
                    goto done;
                rtn_stack[lvl].prgm = prgm;
                matrix_entry_follows = rtn_stack[lvl].has_matrix();
                current_prgm = rtn_stack[lvl].get_prgm();
                if (current_prgm >= 0)
                    line = line2pc(line);
                rtn_stack[lvl].pc = line;
            }
            if (matrix_entry_follows) {
                char m_len;
                char m_name[7];
                int4 m_i, m_j;
                if (!read_char(&m_len)
                        || fread(m_name, 1, m_len, gfile) != m_len
                        || !read_int4(&m_i)
                        || !read_int4(&m_j))
                    goto done;
                vartype_list *list = (vartype_list *) new_list(4);
                if (list == NULL)
                    goto done;
                list->array->data[0] = new_string(m_name, m_len);
                list->array->data[1] = new_real(0);
                list->array->data[2] = new_real(m_i);
                list->array->data[3] = new_real(m_j);
                for (int i = 0; i < 4; i++)
                    if (list->array->data[i] == NULL) {
                        free_vartype((vartype *) list);
                        goto done;
                    }
                int pos = vars_count - 1;
                int m_lvl = -2;
                for (; pos >= 0; pos--) {
                    var_struct *vs = vars + pos;
                    if (vs->level < lvl + 1 && string_equals(vs->name, vs->length, m_name, m_len)) {
                        m_lvl = vs->level;
                        break;
                    }
                }
                if (m_lvl == -2) {
                    // shouldn't happen
                    free_vartype((vartype *) list);
                    goto done;
                }
                ((vartype_real *) list->array->data[1])->x = m_lvl;
                for (pos = 0; pos < vars_count; pos++)
                    if (vars[pos].level >= lvl + 1)
                        break;
                if (vars_count == vars_capacity) {
                    int newcap = vars_capacity + 8;
                    var_struct *newvars = (var_struct *) realloc(vars, newcap * sizeof(var_struct));
                    if (newvars == NULL) {
                        free_vartype((vartype *) list);
                        goto done;
                    }
                    vars = newvars;
                    vars_capacity = newcap;
                }
                memmove(vars + pos + 1, vars + pos, (vars_count - pos) * sizeof(var_struct));
                memcpy(vars[pos].name, "MAT", 3);
                vars[pos].length = 3;
                vars[pos].level = lvl + 1;
                vars[pos].flags = VAR_PRIVATE;
                vars[pos].value = (vartype *) list;
                vars_count++;
            }
        }
        current_prgm = saved_prgm;
    }
    if (!read_bool(&rtn_solve_active))
        goto done;
    if (!read_bool(&rtn_integ_active))
        goto done;

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
    else { // arg->type == ARGTYPE_STR
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
        get_next_command(&pc, &command, &arg, 0, NULL);
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
        get_next_command(&pc, &command, &arg, 0, NULL);
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
    prgms[current_prgm].lclbl_invalid = true;
    prgms[current_prgm].locked = false;
    prgms[current_prgm].text = NULL;
    command = CMD_END;
    arg.type = ARGTYPE_NONE;
    store_command(0, command, &arg, NULL);
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
    get_next_command(&pc, &command, &arg, 0, NULL);
    current_prgm = saved_prgm;
    return command == CMD_MVAR;
}

int get_command_length(int prgm_index, int4 pc) {
    prgm_struct *prgm = prgms + prgm_index;
    int4 pc2 = pc;
    int command = prgm->text[pc2++];
    int argtype = prgm->text[pc2++];
    command |= (argtype & 112) << 4;
    bool have_orig_num = command == CMD_NUMBER && (argtype & 128) != 0;
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
        case ARGTYPE_XSTR: {
            int xl = prgm->text[pc2++];
            xl += prgm->text[pc2++] << 8;
            pc2 += xl;
            break;
        }
    }
    if (have_orig_num)
        while (prgm->text[pc2++]);
    return pc2 - pc;
}

void get_next_command(int4 *pc, int *command, arg_struct *arg, int find_target, const char **num_str) {
    prgm_struct *prgm = prgms + current_prgm;
    int i;
    int4 target_pc;
    int4 orig_pc = *pc;

    *command = prgm->text[(*pc)++];
    arg->type = prgm->text[(*pc)++];
    *command |= (arg->type & 112) << 4;
    bool have_orig_num = *command == CMD_NUMBER && (arg->type & 128) != 0;
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
        case ARGTYPE_XSTR: {
            int xstr_len = prgm->text[(*pc)++];
            xstr_len += prgm->text[(*pc)++] << 8;
            arg->length = xstr_len;
            arg->val.xstr = (const char *) (prgm->text + *pc);
            (*pc) += xstr_len;
            break;
        }
    }

    if (*command == CMD_NUMBER) {
        if (have_orig_num) {
            char *p = (char *) &prgm->text[*pc];
            if (num_str != NULL)
                *num_str = p;
            /* Make sure the decimal stored in the program matches
             * the current setting of flag 28.
             */
            char wrong_dot = flags.f.decimal_point ? ',' : '.';
            char right_dot = flags.f.decimal_point ? '.' : ',';
            int numlen = 1;
            while (*p != 0) {
                if (*p == wrong_dot)
                    *p = right_dot;
                p++;
                numlen++;
            }
            *pc += numlen;
        } else {
            if (num_str != NULL)
                *num_str = NULL;
        }
        if (arg->type != ARGTYPE_DOUBLE) {
            /* argtype is ARGTYPE_NUM; convert to phloat */
            arg->val_d = arg->val.num;
            arg->type = ARGTYPE_DOUBLE;
        }
    }

    if (find_target) {
        target_pc = find_local_label(arg);
        arg->target = target_pc;
        for (i = 5; i >= 2; i--) {
            prgm->text[orig_pc + i] = target_pc;
            target_pc >>= 8;
        }
        prgm->lclbl_invalid = false;
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
            command |= (argtype & 112) << 4;
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
            command |= (argtype & 112) << 4;
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
        prgm->lclbl_invalid = true;
    }
}

void delete_command(int4 pc) {
    prgm_struct *prgm = prgms + current_prgm;
    int command = prgm->text[pc];
    int argtype = prgm->text[pc + 1];
    int length = get_command_length(current_prgm, pc);
    int4 pos;

    command |= (argtype & 112) << 4;
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

bool store_command(int4 pc, int command, arg_struct *arg, const char *num_str) {
    unsigned char buf[100];
    int bufptr = 0;
    int xstr_len;
    int i;
    int4 pos;
    prgm_struct *prgm = prgms + current_prgm;

    if (flags.f.prgm_mode && prgm->locked) {
        display_error(ERR_PROGRAM_LOCKED);
        return false;
    }

    /* We should never be called with pc = -1, but just to be safe... */
    if (pc == -1)
        pc = 0;

    if (arg->type == ARGTYPE_NUM && arg->val.num < 0) {
        arg->type = ARGTYPE_NEG_NUM;
        arg->val.num = -arg->val.num;
    } else if (command == CMD_NUMBER) {
        /* Store the string representation of the number, unless it matches
         * the canonical representation, or unless the number is zero.
         */
        if (num_str != NULL) {
            if (arg->val_d == 0) {
                num_str = NULL;
            } else {
                const char *ap = phloat2program(arg->val_d);
                const char *bp = num_str;
                bool equal = true;
                while (1) {
                    char a = *ap++;
                    char b = *bp++;
                    if (a == 0) {
                        if (b != 0)
                            equal = false;
                        break;
                    } else if (b == 0) {
                        goto notequal;
                    }
                    if (a != b) {
                        if (a == 24) {
                            if (b != 'E' && b != 'e')
                                goto notequal;
                        } else if (a == '.' || a == ',') {
                            if (b != '.' && b != ',')
                                goto notequal;
                        } else {
                            notequal:
                            equal = false;
                            break;
                        }
                    }
                }
                if (equal)
                    num_str = NULL;
            }
        }
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
    buf[bufptr++] = arg->type | ((command & 0x700) >> 4) | (command != CMD_NUMBER || num_str == NULL ? 0 : 128);

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
        return true;
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
            buf[bufptr++] = (unsigned char) arg->length;
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
        case ARGTYPE_XSTR: {
            xstr_len = arg->length;
            if (xstr_len > 65535)
                xstr_len = 65535;
            buf[bufptr++] = xstr_len;
            buf[bufptr++] = xstr_len >> 8;
            // Not storing the text in 'buf' because it may not fit;
            // we'll handle that separately when copying the buffer
            // into the program.
            bufptr += xstr_len;
            break;
        }
    }

    if (command == CMD_NUMBER && num_str != NULL) {
        const char *p = num_str;
        char c;
        const char wrong_dot = flags.f.decimal_point ? ',' : '.';
        const char right_dot = flags.f.decimal_point ? '.' : ',';
        while ((c = *p++) != 0) {
            if (c == wrong_dot)
                c = right_dot;
            else if (c == 'E' || c == 'e')
                c = 24;
            buf[bufptr++] = c;
        }
        buf[bufptr++] = 0;
    }

    if (bufptr + prgm->size > prgm->capacity) {
        unsigned char *newtext;
        prgm->capacity += bufptr + 512;
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
    if (arg->type == ARGTYPE_XSTR) {
        int instr_len = bufptr - xstr_len;
        memcpy(prgm->text + pc, buf, instr_len);
        memcpy(prgm->text + pc + instr_len, arg->val.xstr, xstr_len);
    } else {
        memcpy(prgm->text + pc, buf, bufptr);
    }
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
    if (!loading_state)
        draw_varmenu();
    return true;
}

void store_command_after(int4 *pc, int command, arg_struct *arg, const char *num_str) {
    int4 oldpc = *pc;
    if (*pc == -1)
        *pc = 0;
    else if (!prgms[current_prgm].is_end(*pc))
        *pc += get_command_length(current_prgm, *pc);
    if (!store_command(*pc, command, arg, num_str))
        *pc = oldpc;
}

static bool ensure_prgm_space(int n) {
    prgm_struct *prgm = prgms + current_prgm;
    if (prgm->size + n <= prgm->capacity)
        return true;
    int4 newcapacity = prgm->size + n;
    unsigned char *newtext = (unsigned char *) realloc(prgm->text, newcapacity);
    if (newtext == NULL)
        return false;
    prgm->text = newtext;
    prgm->capacity = newcapacity;
    return true;
}

int x2line() {
    if (prgms[current_prgm].locked)
        return ERR_PROGRAM_LOCKED;
    switch (stack[sp]->type) {
        case TYPE_REAL: {
            if (!ensure_prgm_space(2 + sizeof(phloat)))
                return ERR_INSUFFICIENT_MEMORY;
            vartype_real *r = (vartype_real *) stack[sp];
            arg_struct arg;
            arg.type = ARGTYPE_DOUBLE;
            arg.val_d = r->x;
            store_command_after(&pc, CMD_NUMBER, &arg, NULL);
            return ERR_NONE;
        }
        case TYPE_COMPLEX: {
            if (!ensure_prgm_space(6 + 2 * sizeof(phloat)))
                return ERR_INSUFFICIENT_MEMORY;
            vartype_complex *c = (vartype_complex *) stack[sp];
            arg_struct arg;
            arg.type = ARGTYPE_DOUBLE;
            arg.val_d = c->re;
            store_command_after(&pc, CMD_NUMBER, &arg, NULL);
            arg.type = ARGTYPE_DOUBLE;
            arg.val_d = c->im;
            store_command_after(&pc, CMD_NUMBER, &arg, NULL);
            arg.type = ARGTYPE_NONE;
            store_command_after(&pc, CMD_RCOMPLX, &arg, NULL);
            return ERR_NONE;
        }
        case TYPE_STRING: {
            vartype_string *s = (vartype_string *) stack[sp];
            int len = s->length;
            if (len > 65535)
                len = 65535;
            if (!ensure_prgm_space(4 + len))
                return ERR_INSUFFICIENT_MEMORY;
            arg_struct arg;
            arg.type = ARGTYPE_XSTR;
            arg.length = len;
            arg.val.xstr = s->txt();
            store_command_after(&pc, CMD_XSTR, &arg, NULL);
            return ERR_NONE;
        }
        default:
            return ERR_INTERNAL_ERROR;
    }
}

int a2line(bool append) {
    if (prgms[current_prgm].locked)
        return ERR_PROGRAM_LOCKED;
    if (reg_alpha_length == 0) {
        squeak();
        return ERR_NONE;
    }
    if (!ensure_prgm_space(reg_alpha_length + ((reg_alpha_length - 2) / 14 + 1) * 3))
        return ERR_INSUFFICIENT_MEMORY;
    const char *p = reg_alpha;
    int len = reg_alpha_length;
    int maxlen = 15;

    arg_struct arg;
    if (append) {
        maxlen = 14;
    } else if (p[0] == 0x7f || (p[0] & 128) != 0) {
        arg.type = ARGTYPE_NONE;
        store_command_after(&pc, CMD_CLA, &arg, NULL);
        maxlen = 14;
    }

    while (len > 0) {
        int len2 = len;
        if (len2 > maxlen)
            len2 = maxlen;
        arg.type = ARGTYPE_STR;
        if (maxlen == 15) {
            arg.length = len2;
            memcpy(arg.val.text, p, len2);
        } else {
            arg.length = len2 + 1;
            arg.val.text[0] = 127;
            memcpy(arg.val.text + 1, p, len2);
        }
        store_command_after(&pc, CMD_STRING, &arg, NULL);
        p += len2;
        len -= len2;
        maxlen = 14;
    }
    return ERR_NONE;
}

int prgm_lock(bool lock) {
    if (!flags.f.prgm_mode)
        return ERR_RESTRICTED_OPERATION;
    prgms[current_prgm].locked = lock;
    return ERR_NONE;
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
        if (prgm->is_end(pc))
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

int4 global_pc2line(int prgm, int4 pc) {
    int saved_prgm = current_prgm;
    current_prgm = prgm;
    int4 res = pc2line(pc);
    current_prgm = saved_prgm;
    return res;
}

int4 global_line2pc(int prgm, int4 line) {
    int saved_prgm = current_prgm;
    current_prgm = prgm;
    int4 res = line2pc(line);
    current_prgm = saved_prgm;
    return res;
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
        command |= (argtype & 112) << 4;
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

static int find_global_label_2(const arg_struct *arg, int *prgm, int4 *pc, int *idx) {
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
        if (prgm != NULL)
            *prgm = labels[i].prgm;
        if (pc != NULL)
            *pc = labels[i].pc;
        if (idx != NULL)
            *idx = i;
        return 1;
        nomatch:;
    }
    return 0;
}

int find_global_label(const arg_struct *arg, int *prgm, int4 *pc) {
    return find_global_label_2(arg, prgm, pc, NULL);
}

int find_global_label_index(const arg_struct *arg, int *idx) {
    return find_global_label_2(arg, NULL, NULL, idx);
}

int push_rtn_addr(int prgm, int4 pc) {
    if (rtn_level == MAX_RTN_LEVEL)
        return ERR_RTN_STACK_FULL;
    if (rtn_level == rtn_stack_capacity) {
        int new_rtn_stack_capacity = rtn_stack_capacity + 16;
        rtn_stack_entry *new_rtn_stack = (rtn_stack_entry *) realloc(rtn_stack, new_rtn_stack_capacity * sizeof(rtn_stack_entry));
        if (new_rtn_stack == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        rtn_stack_capacity = new_rtn_stack_capacity;
        rtn_stack = new_rtn_stack;
    }
    rtn_stack[rtn_level].set_prgm(prgm);
    rtn_stack[rtn_level].pc = pc;
    rtn_level++;
    if (prgm == -2)
        rtn_solve_active = true;
    else if (prgm == -3)
        rtn_integ_active = true;
    return ERR_NONE;
}

int push_indexed_matrix() {
    if (rtn_level == 0 ? rtn_level_0_has_matrix_entry : rtn_stack[rtn_level - 1].has_matrix())
        return ERR_NONE;
    vartype_list *list = (vartype_list *) new_list(4 + matedit_stack_depth);
    if (list == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    list->array->data[0] = new_string(matedit_name, matedit_length);
    list->array->data[1] = new_real(matedit_level);
    list->array->data[2] = new_real(matedit_i);
    list->array->data[3] = new_real(matedit_is_list ? -1 : matedit_j);
    for (int i = 0; i < matedit_stack_depth; i++)
        list->array->data[4 + i] = new_real(matedit_stack[i]);
    for (int i = 0; i < 4 + matedit_stack_depth; i++)
        if (list->array->data[i] == NULL) {
            free_vartype((vartype *) list);
            return ERR_INSUFFICIENT_MEMORY;
        }
    store_private_var("MAT", 3, (vartype *) list);
    if (rtn_level == 0)
        rtn_level_0_has_matrix_entry = true;
    else
        rtn_stack[rtn_level - 1].set_has_matrix(true);
    matedit_mode = 0;
    free(matedit_stack);
    matedit_stack = NULL;
    matedit_stack_depth = 0;
    return ERR_NONE;
}

void maybe_pop_indexed_matrix(const char *name, int len) {
    if (rtn_level == 0 ? !rtn_level_0_has_matrix_entry : !rtn_stack[rtn_level - 1].has_matrix())
        return;
    if (!string_equals(matedit_name, matedit_length, name, len))
        return;
    vartype_list *list = (vartype_list *) recall_and_purge_private_var("MAT", 3);
    if (list == NULL)
        return;
    int newdepth = list->size - 4;
    int4 *newstack = newdepth == 0 ? NULL : (int4 *) malloc(newdepth * sizeof(int4));
    // TODO Handle memory allocation failure
    vartype_string *s = (vartype_string *) list->array->data[0];
    string_copy(matedit_name, &matedit_length, s->txt(), s->length);
    matedit_level = to_int4(((vartype_real *) list->array->data[1])->x);
    matedit_i = to_int4(((vartype_real *) list->array->data[2])->x);
    matedit_j = to_int4(((vartype_real *) list->array->data[3])->x);
    matedit_is_list = matedit_j == -1;
    if (matedit_is_list)
        matedit_j = 0;
    matedit_stack_depth = newdepth;
    free(matedit_stack);
    matedit_stack = newstack;
    for (int i = 0; i < newdepth; i++)
        matedit_stack[i] = to_int4(((vartype_real *) list->array->data[i + 4])->x);
    matedit_mode = 1;
    free_vartype((vartype *) list);
    if (rtn_level == 0)
        rtn_level_0_has_matrix_entry = false;
    else
        rtn_stack[rtn_level - 1].set_has_matrix(false);
}

/* Layout of STK list used to save state for FUNC and LNSTK/L4STK:
 * 0: Mode: [0-4][0-4] for FUNC, or -1 for stand-alone LNSTK/L4STK;
 * 1: State: "ABCDE<Text>" where A=Caller Big Stack, B=LNSTK/L4STK after FUNC,
 *    C=CSLD, D=F25, E=ERRNO, <Text>=ERRMSG
 *    For stand-alone LNSTK/L4STK, only A is present.
 * 2: Saved stack. For stand-alone LNSTK/L4STK that didn't have to
 *    change the actual stack mode, this is empty; for stand-alone
 *    LNSTK, this is also empty; for all other cases, this is the entire stack.
 * 3: Saved LASTX (FUNC only)
 */

int push_func_state(int n) {
    if (!program_running())
        return ERR_RESTRICTED_OPERATION;
    int inputs = n / 10;
    if (sp + 1 < inputs)
        return ERR_TOO_FEW_ARGUMENTS;

    vartype *stk = recall_private_var("STK", 3);
    if (stk != NULL)
        return ERR_INVALID_CONTEXT;
    if (!ensure_var_space(1))
        return ERR_INSUFFICIENT_MEMORY;

    /* Create the STK list */
    stk = new_list(4);
    if (stk == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    vartype_list *slist = (vartype_list *) stk;
    slist->array->data[0] = new_real(n);
    slist->array->data[1] = new_string(NULL, lasterr == -1 ? 5 + lasterr_length : 5);
    int i;
    for (i = 0; i < 2; i++)
        if (slist->array->data[i] == NULL) {
            nomem1:
            free_vartype(stk);
            return ERR_INSUFFICIENT_MEMORY;
        }

    /* Create the new stack
     * Note that we're creating a list here, while the actual stack is just
     * a plain array of (vartype *). We'll swap the array in this list with
     * the one for the RPN stack once all allocations have been done, and then
     * use this list to store the old stack in the STK list.
     */
    int newdepth = flags.f.big_stack ? inputs : 4;
    // Allocating size 4 because the stack must always have capacity >= 4
    vartype_list *tlist = (vartype_list *) new_list(4);
    tlist->size = newdepth;
    if (tlist == NULL)
        goto nomem1;
    for (i = 0; i < newdepth; i++) {
        tlist->array->data[newdepth - 1 - i] = i < inputs ? dup_vartype(stack[sp - i]) : new_real(0);
        if (tlist->array->data[newdepth - 1 - i] == NULL) {
            nomem2:
            free_vartype((vartype *) tlist);
            goto nomem1;
        }
    }

    vartype *newlastx = new_real(0);
    if (newlastx == NULL)
        goto nomem2;

    /* OK, we have everything we need. Now move it all into place... */
    vartype_string *s = (vartype_string *) slist->array->data[1];
    s->txt()[0] = flags.f.big_stack ? '1' : '0';
    s->txt()[1] = '0';
    s->txt()[2] = sp != -1 && is_csld() ? '1' : '0';
    s->txt()[3] = flags.f.error_ignore ? '1' : '0';
    s->txt()[4] = (char) lasterr;
    if (lasterr == -1)
        memcpy(s->txt() + 5, lasterr_text, lasterr_length);
    vartype **tmpstk = tlist->array->data;
    int4 tmpdepth = tlist->size;
    tlist->array->data = stack;
    tlist->size = sp + 1;
    stack = tmpstk;
    stack_capacity = 4;
    sp = tmpdepth - 1;
    slist->array->data[2] = (vartype *) tlist;
    slist->array->data[3] = lastx;
    lastx = newlastx;

    store_private_var("STK", 3, stk);
    flags.f.error_ignore = 0;
    lasterr = ERR_NONE;

    if (rtn_level == 0)
        rtn_level_0_has_func_state = true;
    else
        rtn_stack[rtn_level - 1].set_has_func(true);
    return ERR_NONE;
}

int push_stack_state(bool big) {
    vartype *stk = recall_private_var("STK", 3);
    if (stk != NULL) {
        /* LNSTK/L4STK after FUNC */
        vartype_list *slist = (vartype_list *) stk;
        vartype_string *s = (vartype_string *) slist->array->data[1];
        if (s->length == 1 || s->txt()[1] != '0')
            /* LNSTK/L4STK after LNSTK/L4STK: not allowed */
            return ERR_INVALID_CONTEXT;
        if ((bool) flags.f.big_stack == big) {
            /* Nothing to do */
        } else if (big) {
            /* Assuming we're being called right after FUNC, so
             * the stack contains the parameters, padded to depth 4
             * with zeros. We just remove the padding now.
             */
            vartype_real *mode = (vartype_real *) slist->array->data[0];
            int inputs = to_int(mode->x) / 10;
            int excess = 4 - inputs;
            if (excess > 0) {
                for (int i = 0; i < excess; i++)
                    free_vartype(stack[i]);
                memmove(stack, stack + excess, inputs * sizeof(vartype *));
                sp = inputs - 1;
            }
            flags.f.big_stack = true;
        } else {
            /* Just a plain switch to 4STK using truncation to 4 levels.
             * FUNC has already taken care of saving the entire stack,
             * so there's no need to do that here.
             */
            int err = docmd_4stk(NULL);
            if (err != ERR_NONE)
                return err;
        }
        s->txt()[1] = '1';
        return ERR_NONE;
    } else {
        /* Stand-alone LNSTK/L4STK */

        bool save_stk = flags.f.big_stack && !big;

        /* Create the STK list */
        stk = new_list(3);
        if (stk == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        vartype_list *slist = (vartype_list *) stk;
        slist->array->data[0] = new_real(-1);
        slist->array->data[1] = new_string(flags.f.big_stack ? "1" : "0", 1);
        slist->array->data[2] = NULL;
        int i;
        for (i = 0; i < 2; i++)
            if (slist->array->data[i] == NULL) {
                nomem:
                free_vartype(stk);
                return ERR_INSUFFICIENT_MEMORY;
            }

        /* When switching from NSTK to 4STK, save the stack */
        if (save_stk) {
            vartype_list *tlist = (vartype_list *) new_list(4);
            if (tlist == NULL)
                goto nomem;
            for (i = 0; i < 4; i++) {
                tlist->array->data[3 - i] = i <= sp ? dup_vartype(stack[sp - i]) : new_real(0);
                if (tlist->array->data[3 - i] == NULL) {
                    free_vartype((vartype *) tlist);
                    goto nomem;
                }
            }
            vartype **tmpstk = tlist->array->data;
            int4 tmpdepth = tlist->size;
            tlist->array->data = stack;
            tlist->size = sp + 1;
            stack = tmpstk;
            stack_capacity = tmpdepth;
            sp = tmpdepth - 1;
            slist->array->data[2] = (vartype *) tlist;
        }

        store_private_var("STK", 3, stk);
        flags.f.big_stack = big;

        if (rtn_level == 0)
            rtn_level_0_has_func_state = true;
        else
            rtn_stack[rtn_level - 1].set_has_func(true);
        return ERR_NONE;
    }
}

/* FUNC and L4STK save the stack in a list, but this has one undesirable
 * side effect: if the state is saved and restored, the array inside the
 * list will be created without any spare room. This is bad when restoring
 * the stack, because there is the assumption that the stack array will
 * always have capacity >= 4. This function makes sure the array has that
 * capacity.
 */
static bool ensure_list_capacity_4(vartype_list *list) {
    int4 size = list->size;
    if (size < 4) {
        vartype **newdata = (vartype **) realloc(list->array->data, 4 * sizeof(vartype *));
        if (newdata == NULL)
            return false;
        list->array->data = newdata;
    }
    return true;
}

int pop_func_state(bool error) {
    if (rtn_level == 0) {
        if (!rtn_level_0_has_func_state)
            return ERR_NONE;
    } else {
        if (!rtn_stack[rtn_level - 1].has_func())
            return ERR_NONE;
    }

    vartype_list *stk = (vartype_list *) recall_private_var("STK", 3);
    if (stk == NULL)
        // Older FD/ST-based FUNC logic, pre-47. Too difficult to deal with, so
        // we punt. Note that this can only happen if a user upgrades from <47
        // to >=47 while execution is stopped with old FUNC data on the stack.
        // That seems like a reasonable scenario to ignore.
        return ERR_INVALID_DATA;

    vartype **stk_data = stk->array->data;
    int n = to_int(((vartype_real *) stk_data[0])->x);
    vartype_string *state = (vartype_string *) stk_data[1];
    bool big = state->txt()[0] == '1';
    if (big && !core_settings.allow_big_stack)
        return ERR_BIG_STACK_DISABLED;

    int err = ERR_NONE;
    if (false) {
        error:
        free_vartype(lastx);
        lastx = stk_data[3];
        stk_data[3] = NULL;
        goto done;
    }

    if (n == -1) {
        // Stand-alone LNSTK/L4STK
        if (big && !flags.f.big_stack && stk_data[2] != NULL) {
            // Extend the stack back to its original size, restoring its
            // original contents, but only those above level 4.
            vartype_list *tlist = (vartype_list *) stk_data[2];
            if (!ensure_list_capacity_4(tlist))
                return ERR_INSUFFICIENT_MEMORY;
            while (tlist->size < 4)
                tlist->array->data[tlist->size++] = NULL;
            for (int i = 0; i < 4; i++) {
                free_vartype(tlist->array->data[tlist->size - 1 - i]);
                tlist->array->data[tlist->size - 1 - i] = stack[sp - i];
                stack[sp - i] = NULL;
            }
            vartype **tmpstk = stack;
            int tmpsize = sp + 1;
            stack = tlist->array->data;
            stack_capacity = tlist->size;
            sp = stack_capacity - 1;
            tlist->array->data = tmpstk;
            tlist->size = tmpsize;
        } else if (!big && flags.f.big_stack) {
            if (sp < 3) {
                int extra = 3 - sp;
                vartype *zeros[4];
                bool nomem = false;
                for (int i = 0; i < extra; i++) {
                    zeros[i] = new_real(0);
                    if (zeros[i] == NULL)
                        nomem = true;
                }
                if (nomem || !ensure_stack_capacity(extra)) {
                    for (int i = 0; i < extra; i++)
                        free_vartype(zeros[i]);
                    big = true; // because the switch back to 4STK has failed
                    err = ERR_INSUFFICIENT_MEMORY;
                } else {
                    memmove(stack + extra, stack, (sp + 1) * sizeof(vartype *));
                    for (int i = 0; i < extra; i++)
                        stack[i] = zeros[i];
                    sp = 3;
                }
            } else if (sp > 3) {
                int excess = sp - 3;
                for (int i = 0; i < excess; i++)
                    free_vartype(stack[i]);
                memmove(stack, stack + excess, 4 * sizeof(vartype *));
                sp = 3;
            }
        }
    } else {
        // FUNC, with or without LNSTK/L4STK
        vartype_list *tlist = (vartype_list *) stk_data[2];
        if (!ensure_list_capacity_4(tlist)) {
            err = ERR_INSUFFICIENT_MEMORY;
            goto error;
        }

        vartype **tmpstk = stack;
        int tmpsize = sp + 1;
        stack = tlist->array->data;
        stack_capacity = tlist->size;
        sp = stack_capacity - 1;
        if (stack_capacity < 4)
            stack_capacity = 4;
        tlist->array->data = tmpstk;
        tlist->size = tmpsize;

        if (error)
            goto error;

        int inputs = n / 10;
        int outputs = n % 10;
        if (tmpsize < outputs) {
            // One could make the case that this should be an error.
            // I chose to be lenient and pad the result set with zeros instead.
            int deficit = outputs - tmpsize;
            memmove(tmpstk + deficit, tmpstk, tmpsize * sizeof(vartype *));
            bool nomem = false;
            for (int i = 0; i < deficit; i++) {
                vartype *zero = new_real(0);
                tmpstk[i] = zero;
                if (zero == NULL)
                    nomem = true;
            }
            tmpsize += deficit;
            tlist->size = tmpsize;
            if (nomem) {
                err = ERR_INSUFFICIENT_MEMORY;
                goto error;
            }
        }

        bool do_lastx = inputs > 0;
        if (n == 1 && state->txt()[2] == '1')
            // RCL-like function with stack lift disabled
            inputs = 1;
        int growth = outputs - inputs;
        if (big) {
            flags.f.big_stack = true;
            if (!ensure_stack_capacity(growth)) {
                err = ERR_INSUFFICIENT_MEMORY;
                goto error;
            }
            free_vartype(lastx);
            if (do_lastx) {
                lastx = stack[sp];
                stack[sp] = NULL;
            } else {
                lastx = stk_data[3];
                stk_data[3] = NULL;
            }
            for (int i = 0; i < inputs; i++) {
                free_vartype(stack[sp - i]);
                stack[sp - i] = NULL;
            }
            sp -= inputs;
            sp += outputs;
            for (int i = 0; i < outputs; i++) {
                stack[sp - i] = tmpstk[tmpsize - i - 1];
                tmpstk[tmpsize - i - 1] = NULL;
            }
        } else {
            vartype *tdups[4];
            int n_tdups = -growth;
            for (int i = 0; i < n_tdups; i++) {
                tdups[i] = dup_vartype(stack[0]);
                if (tdups[i] == NULL) {
                    for (int j = 0; j < i; j++)
                        free_vartype(tdups[j]);
                    err = ERR_INSUFFICIENT_MEMORY;
                    goto error;
                }
            }
            free_vartype(lastx);
            if (do_lastx) {
                lastx = stack[sp];
                stack[sp] = NULL;
            } else {
                lastx = stk_data[3];
                stk_data[3] = NULL;
            }
            for (int i = 0; i < inputs; i++) {
                free_vartype(stack[sp - i]);
                stack[sp - i] = NULL;
            }
            if (growth > 0) {
                for (int i = 0; i < growth; i++)
                    free_vartype(stack[i]);
                memmove(stack, stack + growth, (4 - outputs) * sizeof(vartype *));
            } else if (growth < 0) {
                int shrinkage = -growth;
                memmove(stack + shrinkage, stack, (4 - inputs) * sizeof(vartype *));
                for (int i = 0; i < shrinkage; i++)
                    stack[i] = tdups[i];
            }
            for (int i = 0; i < outputs; i++) {
                stack[sp - i] = tmpstk[tmpsize - i - 1];
                tmpstk[tmpsize - i - 1] = NULL;
            }
        }

        flags.f.error_ignore = state->txt()[3] == '1';
        lasterr = (signed char) state->txt()[4];
        if (lasterr == -1) {
            lasterr_length = state->length - 5;
            memcpy(lasterr_text, state->txt() + 5, lasterr_length);
        }
    }

    done:
    if (rtn_level == 0)
        rtn_level_0_has_func_state = false;
    else
        rtn_stack[rtn_level - 1].set_has_func(false);

    flags.f.big_stack = big;
    print_trace();
    return err;
}

void step_out() {
    if (rtn_level > 0)
        rtn_stop_level = rtn_level - 1;
}

void step_over() {
    if (rtn_level >= 0)
        rtn_stop_level = rtn_level;
}

bool should_i_stop_at_this_level() {
    bool stop = rtn_stop_level >= rtn_level;
    if (stop)
        rtn_stop_level = -1;
    return stop;
}

static void remove_locals() {
    if (matedit_mode == 3 && matedit_level >= rtn_level)
        leave_matrix_editor();
    int last = -1;
    for (int i = vars_count - 1; i >= 0; i--) {
        if (vars[i].level == -1)
            continue;
        if (vars[i].level < rtn_level)
            break;
        if ((matedit_mode == 1 || matedit_mode == 3)
                && vars[i].level == matedit_level
                && string_equals(vars[i].name, vars[i].length, matedit_name, matedit_length)) {
            if (matedit_mode == 3) {
                set_appmenu_exitcallback(0);
                set_menu(MENULEVEL_APP, MENU_NONE);
            }
            matedit_mode = 0;
            free(matedit_stack);
            matedit_stack = NULL;
            matedit_stack_depth = 0;
        }
        if ((vars[i].flags & VAR_HIDING) != 0) {
            for (int j = i - 1; j >= 0; j--)
                if ((vars[j].flags & VAR_HIDDEN) != 0 && string_equals(vars[i].name, vars[i].length, vars[j].name, vars[j].length)) {
                    vars[j].flags &= ~VAR_HIDDEN;
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

int rtn(int err) {
    // NOTE: 'err' should be one of ERR_NONE, ERR_YES, or ERR_NO.
    // For any actual *error*, i.e. anything that should actually
    // stop program execution, use rtn_with_error() instead.
    int newprgm;
    int4 newpc;
    bool stop;
    pop_rtn_addr(&newprgm, &newpc, &stop);
    if (newprgm == -3) {
        return return_to_integ(stop);
    } else if (newprgm == -2) {
        return return_to_solve(0, stop);
    } else if (newprgm == -1) {
        if (pc >= prgms[current_prgm].size)
            /* It's an END; go to line 0 */
            pc = -1;
        if (err != ERR_NONE)
            display_error(err);
        return ERR_STOP;
    } else {
        current_prgm = newprgm;
        pc = newpc;
        if (err == ERR_NO) {
            int command;
            arg_struct arg;
            get_next_command(&pc, &command, &arg, 0, NULL);
            if (command == CMD_END)
                pc = newpc;
        }
        return stop ? ERR_STOP : ERR_NONE;
    }
}

int rtn_with_error(int err) {
    bool stop;
    if (solve_active() && (err == ERR_OUT_OF_RANGE
                            || err == ERR_DIVIDE_BY_0
                            || err == ERR_INVALID_DATA
                            || err == ERR_STAT_MATH_ERROR)) {
        stop = unwind_stack_until_solve();
        return return_to_solve(1, stop);
    }
    int newprgm;
    int4 newpc;
    pop_rtn_addr(&newprgm, &newpc, &stop);
    if (newprgm >= 0) {
        // Stop on the calling XEQ, not the RTNERR
        current_prgm = newprgm;
        int line = pc2line(newpc);
        set_old_pc(line2pc(line - 1));
    }
    return err;
}

void pop_rtn_addr(int *prgm, int4 *pc, bool *stop) {
    if (rtn_level == 0 ? rtn_level_0_has_matrix_entry : rtn_stack[rtn_level - 1].has_matrix()) {
        vartype_list *list = (vartype_list *) recall_and_purge_private_var("MAT", 3);
        if (list != NULL) {
            int newdepth = list->size - 4;
            int4 *newstack = newdepth == 0 ? NULL : (int4 *) malloc(newdepth * sizeof(vartype *));
            // TODO: Handle memory allocation failure
            vartype_string *s = (vartype_string *) list->array->data[0];
            string_copy(matedit_name, &matedit_length, s->txt(), s->length);
            matedit_level = to_int4(((vartype_real *) list->array->data[1])->x);
            matedit_i = to_int4(((vartype_real *) list->array->data[2])->x);
            matedit_j = to_int4(((vartype_real *) list->array->data[3])->x);
            matedit_is_list = matedit_j == -1;
            if (matedit_is_list)
                matedit_j = 0;
            matedit_stack_depth = newdepth;
            free(matedit_stack);
            matedit_stack = newstack;
            for (int i = 0; i < matedit_stack_depth; i++)
                matedit_stack[i] = to_int4(((vartype_real *) list->array->data[i + 4])->x);
            matedit_mode = 1;
            free_vartype((vartype *) list);
        }
        if (rtn_level == 0)
            rtn_level_0_has_matrix_entry = false;
        else
            rtn_stack[rtn_level - 1].set_has_matrix(false);
    }
    remove_locals();
    if (rtn_level == 0) {
        *prgm = -1;
        *pc = -1;
        rtn_stop_level = -1;
        rtn_level_0_has_func_state = false;
    } else {
        rtn_level--;
        *prgm = rtn_stack[rtn_level].get_prgm();
        *pc = rtn_stack[rtn_level].pc;
        if (rtn_stop_level >= rtn_level) {
            *stop = true;
            rtn_stop_level = -1;
        } else
            *stop = false;
        if (*prgm == -2)
            rtn_solve_active = false;
        else if (*prgm == -3)
            rtn_integ_active = false;
    }
}

static void get_saved_stack_mode(int *m) {
    if (rtn_level == 0) {
        if (!rtn_level_0_has_func_state)
            return;
    } else {
        if (!rtn_stack[rtn_level - 1].has_func())
            return;
    }
    vartype_list *stk = (vartype_list *) recall_private_var("STK", 3);
    if (stk == NULL)
        return;
    vartype **stk_data = stk->array->data;
    *m = ((vartype_string *) stk_data[1])->txt()[0] == '1';
}

void clear_all_rtns() {
    int dummy1;
    int4 dummy2;
    bool dummy3;
    int st_mode = -1;
    while (rtn_level > 0) {
        get_saved_stack_mode(&st_mode);
        pop_rtn_addr(&dummy1, &dummy2, &dummy3);
    }
    get_saved_stack_mode(&st_mode);
    pop_rtn_addr(&dummy1, &dummy2, &dummy3);
    if (st_mode == 0) {
        arg_struct dummy_arg;
        docmd_4stk(&dummy_arg);
    } else if (st_mode == 1) {
        docmd_nstk(NULL);
    }
    if (mode_plainmenu == MENU_PROGRAMMABLE)
        set_menu(MENULEVEL_PLAIN, MENU_NONE);
    if (varmenu_role == 3)
        varmenu_role = 0;
}

int get_rtn_level() {
    return rtn_level;
}

void save_csld() {
    if (rtn_level == 0)
        mode_caller_stack_lift_disabled = flags.f.stack_lift_disable;
    else
        rtn_stack[rtn_level - 1].set_csld();
}

bool is_csld() {
    if (rtn_level == 0)
        return mode_caller_stack_lift_disabled;
    else
        return rtn_stack[rtn_level - 1].is_csld();
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
    int st_mode = -1;
    do {
        get_saved_stack_mode(&st_mode);
        pop_rtn_addr(&prgm, &pc, &stop);
    } while (prgm != -2);
    if (st_mode == 0) {
        arg_struct dummy_arg;
        docmd_4stk(&dummy_arg);
    } else if (st_mode == 1) {
        docmd_nstk(NULL);
    }
    return stop;
}

bool read_bool(bool *b) {
    return read_char((char *) b);
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
    int4 m;
    if (!read_int4(&m))
        return false;
    *n = (int) m;
    return true;
}

bool write_int(int n) {
    return write_int4(n);
}

bool read_int2(int2 *n) {
    #ifdef F42_BIG_ENDIAN
        char buf[2];
        if (fread(buf, 1, 2, gfile) != 2)
            return false;
        char *dst = (char *) n;
        for (int i = 0; i < 2; i++)
            dst[i] = buf[1 - i];
        return true;
    #else
        return fread(n, 1, 2, gfile) == 2;
    #endif
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
        char buf[4];
        if (fread(buf, 1, 4, gfile) != 4)
            return false;
        char *dst = (char *) n;
        for (int i = 0; i < 4; i++)
            dst[i] = buf[3 - i];
        return true;
    #else
        return fread(n, 1, 4, gfile) == 4;
    #endif
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
        char buf[8];
        if (fread(buf, 1, 8, gfile) != 8)
            return false;
        char *dst = (char *) n;
        for (int i = 0; i < 8; i++)
            dst[i] = buf[7 - i];
        return true;
    #else
        return fread(n, 1, 8, gfile) == 8;
    #endif
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
            #ifdef BCD_MATH
                char buf[8];
                if (fread(buf, 1, 8, gfile) != 8)
                    return false;
                double dbl;
                char *dst = (char *) &dbl;
                for (int i = 0; i < 8; i++)
                    dst[i] = buf[7 - i];
                d->assign17digits(dbl);
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
        #else
            #ifdef BCD_MATH
                double dbl;
                if (fread(&dbl, 1, 8, gfile) != 8)
                    return false;
                d->assign17digits(dbl);
                return true;
            #else
                char data[16];
                if (fread(data, 1, 16, gfile) != 16)
                    return false;
                *d = decimal2double(data);
                return true;
            #endif
        #endif
    } else {
        #ifdef F42_BIG_ENDIAN
            #ifdef BCD_MATH
                char buf[16];
                if (fread(buf, 1, 16, gfile) != 16)
                    return false;
                char *dst = (char *) d;
                for (int i = 0; i < 16; i++)
                    dst[i] = buf[15 - i];
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
        #else
            if (fread(d, 1, sizeof(phloat), gfile) != sizeof(phloat))
                return false;
            return true;
        #endif
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

bool read_arg(arg_struct *arg) {
    if (!read_char((char *) &arg->type))
        return false;
    char c;
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
            // Serializing 'length' as a char for backward compatibility.
            // Values > 255 only happen for XSTR, and those are never
            // serialized.
            if (!read_char(&c))
                return false;
            arg->length = c & 255;
            return fread(arg->val.text, 1, arg->length, gfile) == arg->length;
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

bool write_arg(const arg_struct *arg) {
    int type = arg->type;
    if (type == ARGTYPE_XSTR)
        // This type is always used immediately, so no need to persist it;
        // also, persisting it would be difficult, since this variant uses
        // a pointer to the actual text, which is context-dependent and
        // would be impossible to restore.
        type = ARGTYPE_NONE;

    if (!write_char(type))
        return false;
    switch (type) {
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
            return write_char((char) arg->length)
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

static bool load_state2(bool *clear, bool *too_new) {
    int4 magic;
    int4 version;
    *clear = false;
    *too_new = false;

    /* The shell has verified the initial magic and version numbers,
     * and loaded the shell state, before we got called.
     */

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

    if (ver > FREE42_VERSION) {
        *too_new = true;
        return false;
    }
    if (ver < 26)
        // Pre-2.5, non-portable state. Not supported any more.
        return false;

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

    bool state_is_decimal;
    if (!read_bool(&state_is_decimal)) return false;
    if (!state_is_decimal)
        state_file_number_format = NUMBER_FORMAT_BINARY;
    else
        state_file_number_format = NUMBER_FORMAT_BID128;

    bool bdummy;
    if (!read_bool(&bdummy)) return false;
    if (!read_bool(&bdummy)) return false;
    if (!read_bool(&bdummy)) return false;

    if (!read_bool(&mode_clall)) return false;
    if (!read_bool(&mode_command_entry)) return false;
    if (!read_char(&mode_number_entry)) return false;
    if (!read_bool(&mode_alpha_entry)) return false;
    if (!read_bool(&mode_shift)) return false;
    if (!read_int(&mode_appmenu)) return false;
    if (!read_int(&mode_plainmenu)) return false;
    if (!read_bool(&mode_plainmenu_sticky)) return false;
    if (!read_int(&mode_transientmenu)) return false;
    if (!read_int(&mode_alphamenu)) return false;
    if (!read_int(&mode_commandmenu)) return false;
    if (ver < 33) {
        if (mode_appmenu > MENU_MODES3)
            mode_appmenu += 2;
        if (mode_plainmenu > MENU_MODES3)
            mode_plainmenu += 2;
        if (mode_transientmenu > MENU_MODES3)
            mode_transientmenu += 2;
        if (mode_alphamenu > MENU_MODES3)
            mode_alphamenu += 2;
        if (mode_commandmenu > MENU_MODES3)
            mode_commandmenu += 2;
    }
    if (!read_bool(&mode_running)) return false;
    if (ver < 46)
        mode_caller_stack_lift_disabled = false;
    else if (!read_bool(&mode_caller_stack_lift_disabled))
        return false;
    if (!read_bool(&mode_varmenu)) return false;
    if (!read_bool(&mode_updown)) return false;

    if (!read_bool(&mode_getkey))
        return false;

    if (!read_phloat(&entered_number)) return false;
    if (!read_int(&entered_string_length)) return false;
    if (fread(entered_string, 1, 15, gfile) != 15) return false;

    if (!read_int(&pending_command)) return false;
    if (!read_arg(&pending_command_arg)) return false;
    if (!read_int(&xeq_invisible)) return false;

    if (!read_int(&incomplete_command)) return false;
    if (ver < 35) {
        int temp;
        if (!read_int(&temp)) return false;
        incomplete_ind = temp != 0;
        if (!read_int(&temp)) return false;
        incomplete_alpha = temp != 0;
    } else {
        if (!read_bool(&incomplete_ind)) return false;
        if (!read_bool(&incomplete_alpha)) return false;
    }
    if (!read_int(&incomplete_length)) return false;
    if (!read_int(&incomplete_maxdigits)) return false;
    if (!read_int(&incomplete_argtype)) return false;
    if (!read_int(&incomplete_num)) return false;
    int isl = ver < 40 ? 7 : 22;
    if (fread(incomplete_str, 1, isl, gfile) != isl) return false;
    if (!read_int4(&incomplete_saved_pc)) return false;
    if (!read_int4(&incomplete_saved_highlight_row)) return false;

    if (fread(cmdline, 1, 100, gfile) != 100) return false;
    if (!read_int(&cmdline_length)) return false;
    if (!read_int(&cmdline_row)) return false;

    if (!read_int(&matedit_mode)) return false;
    if (ver < 47)
        matedit_level = -2; // This is handled later in this function
    else
        if (!read_int(&matedit_level)) return false;
    if (fread(matedit_name, 1, 7, gfile) != 7) return false;
    if (!read_int(&matedit_length)) return false;
    if (!unpersist_vartype(&matedit_x)) return false;
    if (!read_int4(&matedit_i)) return false;
    if (!read_int4(&matedit_j)) return false;
    if (!read_int(&matedit_prev_appmenu)) return false;
    if (ver < 48) {
        matedit_stack = NULL;
        matedit_stack_depth = 0;
        matedit_is_list = false;
    } else {
        if (!read_int(&matedit_stack_depth)) return false;
        if (matedit_stack_depth == 0) {
            matedit_stack = NULL;
        } else {
            matedit_stack = (int4 *) malloc(matedit_stack_depth * sizeof(int4));
            if (matedit_stack == NULL) {
                matedit_stack_depth = 0;
                return false;
            }
            for (int i = 0; i < matedit_stack_depth; i++)
                if (!read_int4(matedit_stack + i)) {
                    free(matedit_stack);
                    matedit_stack = NULL;
                    matedit_stack_depth = 0;
                    return false;
                }
        }
        if (!read_bool(&matedit_is_list)) return false;
    }

    if (fread(input_name, 1, 11, gfile) != 11) return false;
    if (!read_int(&input_length)) return false;
    if (!read_arg(&input_arg)) return false;

    if (ver < 39) {
        lasterr = 0;
    } else {
        if (!read_int(&lasterr)) return false;
        if (!read_int(&lasterr_length)) return false;
        if (fread(lasterr_text, 1, 22, gfile) != 22) return false;
    }

    if (!read_int(&baseapp)) return false;

    if (!read_int8(&random_number_low)) return false;
    if (!read_int8(&random_number_high)) return false;

    if (!read_int(&deferred_print)) return false;

    if (!read_int(&keybuf_head)) return false;
    if (!read_int(&keybuf_tail)) return false;
    for (int i = 0; i < 16; i++)
        if (!read_int(&keybuf[i]))
            return false;

    if (!unpersist_display(ver))
        return false;
    if (!unpersist_globals())
        return false;

    /* When loading older states, figure out matedit_level.
     * This is needed for EDITN and INDEX to keep working across
     * upgrades. Note that we're not dealing with saved matrix
     * info on the RTN stack, though, so this provides only
     * partial compatibility, covering only the most common
     * scenarios.
     */
    if (matedit_level == -2 && (matedit_mode == 1 || matedit_mode == 3)) {
        matedit_level = -1;
        for (int i = vars_count - 1; i >= 0; i--) {
            var_struct *vs = vars + i;
            if ((vs->flags & (VAR_HIDDEN | VAR_PRIVATE)) != 0)
                continue;
            if (string_equals(matedit_name, matedit_length, vs->name, vs->length)) {
                matedit_level = vs->level;
                break;
            }
        }
    }

    if (!unpersist_math(ver))
        return false;

    if (!read_int4(&magic)) return false;
    if (magic != FREE42_MAGIC)
        return false;
    if (!read_int4(&version)) return false;
    if (version != ver)
        return false;

    return true;
}

// See the comment for bug_mode at its declaration...

bool load_state(int4 ver_p, bool *clear, bool *too_new) {
    bug_mode = 0;
    ver = ver_p;
    long fpos = ftell(gfile);
    if (load_state2(clear, too_new))
        return true;
    if (bug_mode != 3)
        return false;
    // bug_mode == 3 is the signal that the file looks screwy
    // in the way caused by the buggy string-in-matrix writing
    // in version 2.5
    core_cleanup();
    fseek(gfile, fpos, SEEK_SET);
    bug_mode = 2;
    return load_state2(clear, too_new);
}

void save_state(bool *success) {
    *success = false;
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
    if (!write_char(mode_number_entry)) return;
    if (!write_bool(mode_alpha_entry)) return;
    if (!write_bool(mode_shift)) return;
    if (!write_int(mode_appmenu)) return;
    if (!write_int(mode_plainmenu)) return;
    if (!write_bool(mode_plainmenu_sticky)) return;
    if (!write_int(mode_transientmenu)) return;
    if (!write_int(mode_alphamenu)) return;
    if (!write_int(mode_commandmenu)) return;
    if (!write_bool(mode_running)) return;
    if (!write_bool(mode_caller_stack_lift_disabled)) return;
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
    if (!write_bool(incomplete_ind)) return;
    if (!write_bool(incomplete_alpha)) return;
    if (!write_int(incomplete_length)) return;
    if (!write_int(incomplete_maxdigits)) return;
    if (!write_int(incomplete_argtype)) return;
    if (!write_int(incomplete_num)) return;
    if (fwrite(incomplete_str, 1, 22, gfile) != 22) return;
    if (!write_int4(pc2line(incomplete_saved_pc))) return;
    if (!write_int4(incomplete_saved_highlight_row)) return;

    if (fwrite(cmdline, 1, 100, gfile) != 100) return;
    if (!write_int(cmdline_length)) return;
    if (!write_int(cmdline_row)) return;

    if (!write_int(matedit_mode)) return;
    if (!write_int(matedit_level)) return;
    if (fwrite(matedit_name, 1, 7, gfile) != 7) return;
    if (!write_int(matedit_length)) return;
    if (!persist_vartype(matedit_x)) return;
    if (!write_int4(matedit_i)) return;
    if (!write_int4(matedit_j)) return;
    if (!write_int(matedit_prev_appmenu)) return;
    if (!write_int(matedit_stack_depth)) return;
    for (int i = 0; i < matedit_stack_depth; i++)
        if (!write_int4(matedit_stack[i])) return;
    if (!write_bool(matedit_is_list)) return;

    if (fwrite(input_name, 1, 11, gfile) != 11) return;
    if (!write_int(input_length)) return;
    if (!write_arg(&input_arg)) return;

    if (!write_int(lasterr)) return;
    if (!write_int(lasterr_length)) return;
    if (fwrite(lasterr_text, 1, 22, gfile) != 22) return;

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
    *success = true;
}

// Reason:
// 0 = Memory Clear
// 1 = State File Corrupt
// 2 = State File Too New
void hard_reset(int reason) {
    vartype *regs;

    /* Clear stack */
    for (int i = 0; i <= sp; i++)
        free_vartype(stack[i]);
    free(stack);
    free_vartype(lastx);
    sp = 3;
    stack_capacity = 4;
    stack = (vartype **) malloc(stack_capacity * sizeof(vartype *));
    for (int i = 0; i <= sp; i++)
        stack[i] = new_real(0);
    lastx = new_real(0);

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
    matedit_stack_depth = 0;
    free(matedit_stack);
    matedit_stack = NULL;
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
    flags.f.decimal_point = number_format()[0] != ','; // HP-42S sets this to 1 on hard reset
    flags.f.thousands_separators = 1;
    flags.f.stack_lift_disable = 0;
    int df = shell_date_format();
    flags.f.dmy = df == 1;
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
    flags.f.ymd = df == 2;
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
    flags.f.big_stack = 0;
    flags.f.f81 = flags.f.f82 = flags.f.f83 = flags.f.f84 = 0;
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
    mode_caller_stack_lift_disabled = false;
    mode_varmenu = false;
    prgm_highlight_row = 0;
    varmenu_length = 0;
    mode_updown = false;
    mode_sigma_reg = 11;
    mode_goose = -1;
    mode_time_clktd = false;
    mode_time_clk24 = shell_clk24();
    mode_wsize = 36;
    mode_menu_caps = false;

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

#ifdef IPHONE
bool off_enabled() {
    if (off_enable_flag)
        return true;
    if (sp == -1 || stack[sp]->type != TYPE_STRING)
        return false;
    vartype_string *str = (vartype_string *) stack[sp];
    off_enable_flag = string_equals(str->txt(), str->length, "YESOFF", 6);
    return off_enable_flag;
}
#endif
