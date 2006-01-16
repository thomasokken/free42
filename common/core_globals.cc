/*****************************************************************************
 * Free42 -- a free HP-42S calculator clone
 * Copyright (C) 2004-2006  Thomas Okken
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *****************************************************************************/

#include <stdlib.h>

#include "core_globals.h"
#include "core_commands1.h"
#include "core_commands2.h"
#include "core_commands3.h"
#include "core_commands4.h"
#include "core_decimal.h"
#include "core_display.h"
#include "core_helpers.h"
#include "core_main.h"
#include "core_math.h"
#include "core_tables.h"
#include "core_variables.h"
#include "shell.h"


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
    { /* NO_VARIABLES */           "No Variables",            12 }
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

int mode_clall;
int (*mode_interruptible)(int) = NULL;
int mode_stoppable;
int mode_command_entry;
int mode_number_entry;
int mode_alpha_entry;
int mode_shift;
int mode_appmenu;
int mode_plainmenu;
int mode_plainmenu_sticky;
int mode_transientmenu;
int mode_alphamenu;
int mode_commandmenu;
int mode_running;
int mode_getkey;
int mode_disable_stack_lift; /* transient */
int mode_varmenu;
int mode_updown;
int4 mode_sigma_reg;
int mode_goose;

double entered_number;
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
double random_number;

/* NORM & TRACE mode: number waiting to be printed */
int deferred_print = 0;

/* Keystroke buffer - holds keystrokes received while
 * there is a program running.
 */
int keybuf_head = 0;
int keybuf_tail = 0;
int keybuf[16];

int remove_program_catalog = 0;


/*******************/
/* Private globals */
/*******************/

#define MAX_RTNS 8
static int rtn_sp = 0;
static int rtn_prgm[MAX_RTNS];
static int4 rtn_pc[MAX_RTNS];


typedef struct {
    int type;
    int4 rows;
    int4 columns;
} matrix_persister;


static int persist_vartype(vartype *v) GLOBALS_SECT;
static int unpersist_vartype(vartype **v) GLOBALS_SECT;
static void update_label_table(int prgm, int4 pc, int inserted) GLOBALS_SECT;
static void invalidate_lclbls(int prgm_index) GLOBALS_SECT;
static int pc_line_convert(int4 loc, int loc_is_pc) GLOBALS_SECT;


/* TODO: these two routines don't handle matrices > 64k on PalmOS.
 * Then again, if the PalmOS memory manager does not allow malloc(n)
 * for n > 65536, I have a lot more work to do!
 */
static int persist_vartype(vartype *v) {
    if (v == NULL) {
	int type = TYPE_NULL;
	return shell_write_saved_state(&type, sizeof(int));
    }
    switch (v->type) {
	case TYPE_REAL:
	    return shell_write_saved_state(v, sizeof(vartype_real));
	case TYPE_COMPLEX:
	    return shell_write_saved_state(v, sizeof(vartype_complex));
	case TYPE_STRING:
	    return shell_write_saved_state(v, sizeof(vartype_string));
/* TODO: this matrix dump/restore code loses aliasing information. */
	case TYPE_REALMATRIX: {
	    matrix_persister mp;
	    vartype_realmatrix *rm = (vartype_realmatrix *) v;
	    int4 size;
	    mp.type = rm->type;
	    mp.rows = rm->rows;
	    mp.columns = rm->columns;
	    if (!shell_write_saved_state(&mp, sizeof(matrix_persister)))
		return 0;
	    size = mp.rows * mp.columns;
	    if (!shell_write_saved_state(rm->array->data,
					 size * sizeof(double_or_string)))
		return 0;
	    if (!shell_write_saved_state(rm->array->is_string, size))
		return 0;
	    return 1;
	}
	case TYPE_COMPLEXMATRIX: {
	    matrix_persister mp;
	    vartype_complexmatrix *cm = (vartype_complexmatrix *) v;
	    int4 size;
	    mp.type = cm->type;
	    mp.rows = cm->rows;
	    mp.columns = cm->columns;
	    if (!shell_write_saved_state(&mp, sizeof(matrix_persister)))
		return 0;
	    size = mp.rows * mp.columns;
	    if (!shell_write_saved_state(cm->array->data,
					 2 * size * sizeof(double)))
		return 0;
	    return 1;
	}
	default:
	    /* Should not happen */
	    return 1;
    }
}

static int unpersist_vartype(vartype **v) {
    int type;
    if (shell_read_saved_state(&type, sizeof(int)) != sizeof(int))
	return 0;
    switch (type) {
	case TYPE_NULL: {
	    *v = NULL;
	    return 1;
	}
	case TYPE_REAL: {
	    vartype_real *r = (vartype_real *) new_real(0);
	    int n = sizeof(vartype_real) - sizeof(int);
	    if (r == NULL)
		return 0;
	    if (shell_read_saved_state(&r->type + 1, n) != n) {
		free_vartype((vartype *) r);
		return 0;
	    } else {
		*v = (vartype *) r;
		return 1;
	    }
	}
	case TYPE_COMPLEX: {
	    vartype_complex *c = (vartype_complex *) new_complex(0, 0);
	    int n = sizeof(vartype_complex) - sizeof(int);
	    if (c == NULL)
		return 0;
	    if (shell_read_saved_state(&c->type + 1, n) != n) {
		free_vartype((vartype *) c);
		return 0;
	    } else {
		*v = (vartype *) c;
		return 1;
	    }
	}
	case TYPE_STRING: {
	    vartype_string *s = (vartype_string *) new_string("", 0);
	    int n = sizeof(vartype_string) - sizeof(int);
	    if (s == NULL)
		return 0;
	    if (shell_read_saved_state(&s->type + 1, n) != n) {
		free_vartype((vartype *) s);
		return 0;
	    } else {
		*v = (vartype *) s;
		return 1;
	    }
	}
	case TYPE_REALMATRIX: {
	    matrix_persister mp;
	    int n = sizeof(matrix_persister) - sizeof(int);
	    vartype_realmatrix *rm;
	    int4 size;
	    if (shell_read_saved_state(&mp.type + 1, n) != n)
		return 0;
	    rm = (vartype_realmatrix *) new_realmatrix(mp.rows, mp.columns);
	    if (rm == NULL)
		return 0;
	    size = mp.rows * mp.columns * sizeof(double_or_string);
	    if (shell_read_saved_state(rm->array->data, size) != size) {
		free_vartype((vartype *) rm);
		return 0;
	    }
	    size = mp.rows * mp.columns;
	    if (shell_read_saved_state(rm->array->is_string, size) != size) {
		free_vartype((vartype *) rm);
		return 0;
	    } else {
		*v = (vartype *) rm;
		return 1;
	    }
	}
	case TYPE_COMPLEXMATRIX: {
	    matrix_persister mp;
	    int n = sizeof(matrix_persister) - sizeof(int);
	    vartype_complexmatrix *cm;
	    int4 size;
	    if (shell_read_saved_state(&mp.type + 1, n) != n)
		return 0;
	    cm = (vartype_complexmatrix *)
				    new_complexmatrix(mp.rows, mp.columns);
	    if (cm == NULL)
		return 0;
	    size = 2 * mp.rows * mp.columns * sizeof(double);
	    if (shell_read_saved_state(cm->array->data, size) != size) {
		free_vartype((vartype *) cm);
		return 0;
	    } else {
		*v = (vartype *) cm;
		return 1;
	    }
	}
	default:
	    return 0;
    }
}

static int persist_globals() GLOBALS_SECT;
static int persist_globals() {
    int i;
    if (!persist_vartype(reg_x))
	return 0;
    if (!persist_vartype(reg_y))
	return 0;
    if (!persist_vartype(reg_z))
	return 0;
    if (!persist_vartype(reg_t))
	return 0;
    if (!persist_vartype(reg_lastx))
	return 0;
    if (!shell_write_saved_state(&reg_alpha_length, sizeof(int)))
	return 0;
    if (!shell_write_saved_state(reg_alpha, 44))
	return 0;
    if (!shell_write_saved_state(&mode_sigma_reg, sizeof(int4)))
	return 0;
    if (!shell_write_saved_state(&mode_goose, sizeof(int)))
	return 0;
    if (!shell_write_saved_state(&flags, sizeof(flags_struct)))
	return 0;
    if (!shell_write_saved_state(&vars_count, sizeof(int)))
	return 0;
    if (!shell_write_saved_state(vars, vars_count * sizeof(var_struct)))
	return 0;
    for (i = 0; i < vars_count; i++)
	if (!persist_vartype(vars[i].value))
	    return 0;
    if (!shell_write_saved_state(&prgms_count, sizeof(int)))
	return 0;
    if (!shell_write_saved_state(prgms, prgms_count * sizeof(prgm_struct)))
	return 0;
    for (i = 0; i < prgms_count; i++)
	if (!shell_write_saved_state(prgms[i].text, prgms[i].size))
	    return 0;
    if (!shell_write_saved_state(&current_prgm, sizeof(int)))
	return 0;
    if (!shell_write_saved_state(&pc, sizeof(int4)))
	return 0;
    if (!shell_write_saved_state(&prgm_highlight_row, sizeof(int)))
	return 0;
    if (!shell_write_saved_state(&varmenu_length, sizeof(int)))
	return 0;
    if (!shell_write_saved_state(varmenu, 7))
	return 0;
    if (!shell_write_saved_state(&varmenu_rows, sizeof(int)))
	return 0;
    if (!shell_write_saved_state(&varmenu_row, sizeof(int)))
	return 0;
    if (!shell_write_saved_state(varmenu_labellength, 6 * sizeof(int)))
	return 0;
    if (!shell_write_saved_state(varmenu_labeltext, 42))
	return 0;
    if (!shell_write_saved_state(&varmenu_role, sizeof(int)))
	return 0;
    if (!shell_write_saved_state(&rtn_sp, sizeof(int)))
	return 0;
    if (!shell_write_saved_state(&rtn_prgm, MAX_RTNS * sizeof(int)))
	return 0;
    if (!shell_write_saved_state(&rtn_pc, MAX_RTNS * sizeof(int4)))
	return 0;
    return 1;
}

static int unpersist_globals() GLOBALS_SECT;
static int unpersist_globals() {
    int4 n;
    int i;
    free_vartype(reg_x);
    if (!unpersist_vartype(&reg_x))
	return 0;
    free_vartype(reg_y);
    if (!unpersist_vartype(&reg_y))
	return 0;
    free_vartype(reg_z);
    if (!unpersist_vartype(&reg_z))
	return 0;
    free_vartype(reg_t);
    if (!unpersist_vartype(&reg_t))
	return 0;
    free_vartype(reg_lastx);
    if (!unpersist_vartype(&reg_lastx))
	return 0;
    if (shell_read_saved_state(&reg_alpha_length, sizeof(int)) != sizeof(int)) {
	reg_alpha_length = 0;
	return 0;
    }
    if (shell_read_saved_state(reg_alpha, 44) != 44) {
	reg_alpha_length = 0;
	return 0;
    }
    if (shell_read_saved_state(&mode_sigma_reg, sizeof(int4)) != sizeof(int4)) {
	mode_sigma_reg = 11;
	return 0;
    }
    if (shell_read_saved_state(&mode_goose, sizeof(int)) != sizeof(int)) {
	mode_goose = -1;
	return 0;
    }
    if (shell_read_saved_state(&flags, sizeof(flags_struct))
	    != sizeof(flags_struct))
	return 0;
    vars_capacity = 0;
    if (vars != NULL) {
	free(vars);
	vars = NULL;
    }
    if (shell_read_saved_state(&vars_count, sizeof(int)) != sizeof(int)) {
	vars_count = 0;
	return 0;
    }
    n = vars_count * sizeof(var_struct);
    vars = (var_struct *) malloc(n);
    if (vars == NULL) {
	vars_count = 0;
	return 0;
    }
    if (shell_read_saved_state(vars, n) != n) {
	free(vars);
	vars = NULL;
	vars_count = 0;
	return 0;
    }
    vars_capacity = vars_count;
    for (i = 0; i < vars_count; i++)
	vars[i].value = NULL;
    for (i = 0; i < vars_count; i++)
	if (!unpersist_vartype(&vars[i].value)) {
	    purge_all_vars();
	    return 0;
	}
    prgms_capacity = 0;
    if (prgms != NULL) {
	free(prgms);
	prgms = NULL;
    }
    if (shell_read_saved_state(&prgms_count, sizeof(int)) != sizeof(int)) {
	prgms_count = 0;
	return 0;
    }
    n = prgms_count * sizeof(prgm_struct);
    prgms = (prgm_struct *) malloc(n);
    if (prgms == NULL) {
	prgms_count = 0;
	return 0;
    }
    if (shell_read_saved_state(prgms, n) != n) {
	free(prgms);
	prgms = NULL;
	prgms_count = 0;
	return 0;
    }
    prgms_capacity = prgms_count;
    for (i = 0; i < prgms_count; i++) {
	prgms[i].capacity = prgms[i].size;
	prgms[i].text = (unsigned char *) malloc(prgms[i].size);
    }
    for (i = 0; i < prgms_count; i++) {
	if (shell_read_saved_state(prgms[i].text, prgms[i].size)
		!= prgms[i].size) {
	    clear_all_prgms();
	    return 0;
	}
    }
    if (shell_read_saved_state(&current_prgm, sizeof(int)) != sizeof(int)) {
	current_prgm = 0;
	return 0;
    }
    if (shell_read_saved_state(&pc, sizeof(int4)) != sizeof(int4)) {
	pc = -1;
	return 0;
    }
    if (shell_read_saved_state(&prgm_highlight_row, sizeof(int))
							    != sizeof(int)) {
	prgm_highlight_row = 0;
	return 0;
    }
    if (shell_read_saved_state(&varmenu_length, sizeof(int)) != sizeof(int)) {
	varmenu_length = 0;
	return 0;
    }
    if (shell_read_saved_state(varmenu, 7) != 7) {
	varmenu_length = 0;
	return 0;
    }
    if (shell_read_saved_state(&varmenu_rows, sizeof(int)) != sizeof(int)) {
	varmenu_length = 0;
	return 0;
    }
    if (shell_read_saved_state(&varmenu_row, sizeof(int)) != sizeof(int)) {
	varmenu_length = 0;
	return 0;
    }
    if (shell_read_saved_state(varmenu_labellength, 6 * sizeof(int))
	    != 6 * sizeof(int))
	return 0;
    if (shell_read_saved_state(varmenu_labeltext, 42) != 42)
	return 0;
    if (shell_read_saved_state(&varmenu_role, sizeof(int)) != sizeof(int))
	return 0;
    if (shell_read_saved_state(&rtn_sp, sizeof(int)) != sizeof(int))
	return 0;
    if (shell_read_saved_state(rtn_prgm, MAX_RTNS * sizeof(int))
	    != MAX_RTNS * sizeof(int))
	return 0;
    if (shell_read_saved_state(rtn_pc, MAX_RTNS * sizeof(int4))
	    != MAX_RTNS * sizeof(int4))
	return 0;
    rebuild_label_table();
    return 1;
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
    int i, j;
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
	    pc2 += sizeof(double);
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
	    union {
		double d;
		unsigned char b[sizeof(double)];
	    } u;
	    int i;
	    for (i = 0; i < (int) sizeof(double); i++)
		u.b[i] = prgm->text[(*pc)++];
	    arg->val.d = u.d;
	    break;
	}
    }

    if (*command == CMD_NUMBER && arg->type != ARGTYPE_DOUBLE) {
	/* argtype is ARGTYPE_NUM; convert to double */
	arg->val.d = arg->val.num;
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
	int4 n = (int4) arg->val.d;
	if (n == arg->val.d && n != (int4) 0x80000000) {
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
	    union {
		double d;
		unsigned char b[sizeof(double)];
	    } u;
	    int i;
	    u.d = arg->val.d;
	    for (i = 0; i < (int) sizeof(double); i++)
		buf[bufptr++] = u.b[i];
	    break;
	}
    }

    if (bufptr + prgm->size > prgm->capacity) {
	unsigned char *newtext;
	prgm->capacity += 512;
	newtext = (unsigned char *) malloc(prgm->capacity);
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

int solve_active() {
    int i;
    for (i = 0; i < rtn_sp; i++)
	if (rtn_prgm[i] == -2)
	    return 1;
    return 0;
}

int integ_active() {
    int i;
    for (i = 0; i < rtn_sp; i++)
	if (rtn_prgm[i] == -3)
	    return 1;
    return 0;
}

void unwind_stack_until_solve() {
    while (rtn_prgm[--rtn_sp] != -2);
}

static int read_int(int *n) GLOBALS_SECT;
static int write_int(int n) GLOBALS_SECT;
static int read_int4(int4 *n) GLOBALS_SECT;
static int write_int4(int4 n) GLOBALS_SECT;

static int read_int(int *n) {
    return shell_read_saved_state(n, sizeof(int)) == sizeof(int);
}

static int write_int(int n) {
    return shell_write_saved_state(&n, sizeof(int));
}

static int read_int4(int4 *n) {
    return shell_read_saved_state(n, sizeof(int4)) == sizeof(int4);
}

static int write_int4(int4 n) {
    return shell_write_saved_state(&n, sizeof(int4));
}

int load_state(int4 ver) {
    int4 magic;
    int4 version;

    /* The shell has verified the initial magic and version numbers,
     * and loaded the shell state, before we got called.
     */

    if (ver < 2) {
	core_settings.matrix_singularmatrix = 0;
	core_settings.matrix_outofrange = 0;
	core_settings.ip_hack = 0;
    } else {
	if (!read_int(&core_settings.matrix_singularmatrix)) return 0;
	if (!read_int(&core_settings.matrix_outofrange)) return 0;
	if (!read_int(&core_settings.ip_hack)) return 0;
    }
    if (ver < 5)
	core_settings.raw_text = 0;
    else {
	if (!read_int(&core_settings.raw_text)) return 0;
	if (ver < 8) {
	    int dummy;
	    if (!read_int(&dummy)) return 0;
	}
    }

    if (!read_int(&mode_clall)) return 0;
    if (!read_int(&mode_command_entry)) return 0;
    if (!read_int(&mode_number_entry)) return 0;
    if (!read_int(&mode_alpha_entry)) return 0;
    if (!read_int(&mode_shift)) return 0;
    if (!read_int(&mode_appmenu)) return 0;
    if (!read_int(&mode_plainmenu)) return 0;
    if (!read_int(&mode_plainmenu_sticky)) return 0;
    if (!read_int(&mode_transientmenu)) return 0;
    if (!read_int(&mode_alphamenu)) return 0;
    if (!read_int(&mode_commandmenu)) return 0;
    if (!read_int(&mode_running)) return 0;
    if (!read_int(&mode_varmenu)) return 0;
    if (!read_int(&mode_updown)) return 0;

    if (ver < 6)
	mode_getkey = 0;
    else if (!read_int(&mode_getkey))
	return 0;

    if (shell_read_saved_state(&entered_number, sizeof(double))
	    != sizeof(double))
	return 0;
    if (!read_int(&entered_string_length)) return 0;
    if (shell_read_saved_state(entered_string, 15) != 15) return 0;

    if (!read_int(&pending_command)) return 0;
    if (shell_read_saved_state(&pending_command_arg, sizeof(arg_struct))
	    != sizeof(arg_struct))
	return 0;
    if (!read_int(&xeq_invisible)) return 0;

    if (!read_int(&incomplete_command)) return 0;
    if (!read_int(&incomplete_ind)) return 0;
    if (!read_int(&incomplete_alpha)) return 0;
    if (!read_int(&incomplete_length)) return 0;
    if (!read_int(&incomplete_maxdigits)) return 0;
    if (!read_int(&incomplete_argtype)) return 0;
    if (!read_int(&incomplete_num)) return 0;
    if (shell_read_saved_state(incomplete_str, 7) != 7) return 0;
    if (!read_int4(&incomplete_saved_pc)) return 0;
    if (!read_int4(&incomplete_saved_highlight_row)) return 0;

    if (shell_read_saved_state(cmdline, 100) != 100) return 0;
    if (!read_int(&cmdline_length)) return 0;
    if (!read_int(&cmdline_row)) return 0;

    if (!read_int(&matedit_mode)) return 0;
    if (shell_read_saved_state(matedit_name, 7) != 7) return 0;
    if (!read_int(&matedit_length)) return 0;
    if (!unpersist_vartype(&matedit_x)) return 0;
    if (!read_int4(&matedit_i)) return 0;
    if (!read_int4(&matedit_j)) return 0;
    if (!read_int(&matedit_prev_appmenu)) return 0;

    if (shell_read_saved_state(input_name, 11) != 11) return 0;
    if (!read_int(&input_length)) return 0;
    if (shell_read_saved_state(&input_arg, sizeof(arg_struct))
	    != sizeof(arg_struct))
	return 0;

    if (!read_int(&baseapp)) return 0;

    if (shell_read_saved_state(&random_number, sizeof(double))
	    != sizeof(double))
	return 0;

    if (ver < 3) {
	deferred_print = 0;
    } else {
	if (!read_int(&deferred_print)) return 0;
    }

    if (!read_int(&keybuf_head)) return 0;
    if (!read_int(&keybuf_tail)) return 0;
    if (shell_read_saved_state(keybuf, 16 * sizeof(int))
	    != 16 * sizeof(int))
	return 0;

    if (!unpersist_display(ver))
	return 0;
    if (!unpersist_globals())
	return 0;

    if (ver < 4) {
	/* Before state file version 4, I used to save the BCD table in the
	 * state file. As of state file version 4, the Linux and Windows
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
	if (shell_read_saved_state(&min_pow2, sizeof(int)) != sizeof(int))
	    return 0;
	if (shell_read_saved_state(&max_pow2, sizeof(int)) != sizeof(int))
	    return 0;
	n1 = 16 * (max_pow2 + 1);          /* size of pos_pow2mant table */
	n2 = sizeof(int) * (max_pow2 + 1); /* size of pos_pow2exp table  */
	n3 = 16 * (-min_pow2);             /* size of neg_pow2mant table */
	n4 = sizeof(int) * (-min_pow2);    /* size of neg_pow2exp table  */
	n = n1 + n2 + n3 + n4;             /* total number of bytes to skip */
	while (n > 0) {
	    int count = n < 1024 ? n : 1024;
	    if (shell_read_saved_state(dummy, count) != count)
		return 0;
	    n -= count;
	}
    }

    if (!unpersist_math())
	return 0;

    if (!read_int4(&magic)) return 0;
    if (magic != FREE42_MAGIC)
	return 0;
    if (!read_int4(&version)) return 0;
    if (version != ver)
	return 0;

    return 1;
}

void save_state() {
    /* The shell has written the initial magic and version numbers,
     * and the shell state, before we got called.
     */

    if (!write_int(core_settings.matrix_singularmatrix)) return;
    if (!write_int(core_settings.matrix_outofrange)) return;
    if (!write_int(core_settings.ip_hack)) return;
    if (!write_int(core_settings.raw_text)) return;
    if (!write_int(mode_clall)) return;
    if (!write_int(mode_command_entry)) return;
    if (!write_int(mode_number_entry)) return;
    if (!write_int(mode_alpha_entry)) return;
    if (!write_int(mode_shift)) return;
    if (!write_int(mode_appmenu)) return;
    if (!write_int(mode_plainmenu)) return;
    if (!write_int(mode_plainmenu_sticky)) return;
    if (!write_int(mode_transientmenu)) return;
    if (!write_int(mode_alphamenu)) return;
    if (!write_int(mode_commandmenu)) return;
    if (!write_int(mode_running)) return;
    if (!write_int(mode_varmenu)) return;
    if (!write_int(mode_updown)) return;
    if (!write_int(mode_getkey)) return;

    if (!shell_write_saved_state(&entered_number, sizeof(double))) return;
    if (!write_int(entered_string_length)) return;
    if (!shell_write_saved_state(entered_string, 15)) return;

    if (!write_int(pending_command)) return;
    if (!shell_write_saved_state(&pending_command_arg, sizeof(arg_struct)))
	return;
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
    if (!shell_write_saved_state(&input_arg, sizeof(arg_struct))) return;

    if (!write_int(baseapp)) return;

    if (!shell_write_saved_state(&random_number, sizeof(double))) return;

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
    reg_x = new_real(0);
    free_vartype(reg_y);
    reg_y = new_real(0);
    free_vartype(reg_z);
    reg_z = new_real(0);
    free_vartype(reg_t);
    reg_t = new_real(0);
    free_vartype(reg_lastx);
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
    random_number = shell_random_seed();

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
    flags.f.audio_enable = 1; /* The manual disagrees. (TODO) */
    /* flags.f.VIRTUAL_custom_menu = 0; */
    flags.f.decimal_point = 1; /* The manual disagrees. (TODO) */
    flags.f.thousands_separators = 1; /* The manual disagrees. (TODO) */
    flags.f.stack_lift_disable = 0;
    flags.f.f31 = flags.f.f32 = flags.f.f33 = 0;
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
    flags.f.alpha_mode = 0;
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

    mode_clall = 0;
    mode_command_entry = 0;
    mode_number_entry = 0;
    mode_alpha_entry = 0;
    mode_shift = 0;
    mode_commandmenu = MENU_NONE;
    mode_alphamenu = MENU_NONE;
    mode_transientmenu = MENU_NONE;
    mode_plainmenu = MENU_NONE;
    mode_appmenu = MENU_NONE;
    mode_running = 0;
    mode_getkey = 0;
    mode_varmenu = 0;
    prgm_highlight_row = 0;
    varmenu_length = 0;
    mode_updown = 0;
    mode_sigma_reg = 11;
    mode_goose = -1;

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
