/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2019  Thomas Okken
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

#ifndef CORE_TABLES_H
#define CORE_TABLES_H 1


#include "free42.h"
#include "core_phloat.h"


/************/
/* Commands */
/************/

/* These are indices into the cmdlist array,
 * declared below (except the negative ones!)
 */

#define CMD_NONE         -1
#define CMD_CANCELLED    -2
#define CMD_LINGER1      -3
#define CMD_LINGER2      -4
#define CMD_CLX           0
#define CMD_ENTER         1
#define CMD_SWAP          2
#define CMD_RDN           3
#define CMD_CHS           4
#define CMD_DIV           5
#define CMD_MUL           6
#define CMD_SUB           7
#define CMD_ADD           8
#define CMD_LASTX         9
#define CMD_SILENT_OFF   10
#define CMD_SILENT_ON    11
#define CMD_SIN          12
#define CMD_COS          13
#define CMD_TAN          14
#define CMD_ASIN         15
#define CMD_ACOS         16
#define CMD_ATAN         17
#define CMD_LOG          18
#define CMD_10_POW_X     19
#define CMD_LN           20
#define CMD_E_POW_X      21
#define CMD_SQRT         22
#define CMD_SQUARE       23
#define CMD_INV          24
#define CMD_Y_POW_X      25
#define CMD_PERCENT      26
#define CMD_PI           27
#define CMD_COMPLEX      28
#define CMD_STO          29
#define CMD_STO_DIV      30
#define CMD_STO_MUL      31
#define CMD_STO_SUB      32
#define CMD_STO_ADD      33
#define CMD_RCL          34
#define CMD_RCL_DIV      35
#define CMD_RCL_MUL      36
#define CMD_RCL_SUB      37
#define CMD_RCL_ADD      38
#define CMD_FIX          39
#define CMD_SCI          40
#define CMD_ENG          41
#define CMD_ALL          42
#define CMD_NULL         43
#define CMD_ASTO         44
#define CMD_ARCL         45
#define CMD_CLA          46
#define CMD_DEG          47
#define CMD_RAD          48
#define CMD_GRAD         49
#define CMD_RECT         50
#define CMD_POLAR        51
#define CMD_SIZE         52
#define CMD_QUIET        53
#define CMD_CPXRES       54
#define CMD_REALRES      55
#define CMD_KEYASN       56
#define CMD_LCLBL        57
#define CMD_RDXDOT       58
#define CMD_RDXCOMMA     59
#define CMD_CLSIGMA      60
#define CMD_CLP          61
#define CMD_CLV          62
#define CMD_CLST         63
#define CMD_CLRG         64
#define CMD_DEL          65
#define CMD_CLKEYS       66
#define CMD_CLLCD        67
#define CMD_CLMENU       68
#define CMD_CLALLa       69
#define CMD_TO_DEG       70
#define CMD_TO_RAD       71
#define CMD_TO_HR        72
#define CMD_TO_HMS       73
#define CMD_TO_REC       74
#define CMD_TO_POL       75
#define CMD_IP           76
#define CMD_FP           77
#define CMD_RND          78
#define CMD_ABS          79
#define CMD_SIGN         80
#define CMD_MOD          81
#define CMD_SF           82
#define CMD_CF           83
#define CMD_FS_T         84
#define CMD_FC_T         85
#define CMD_FSC_T        86
#define CMD_FCC_T        87
#define CMD_COMB         88
#define CMD_PERM         89
#define CMD_FACT         90
#define CMD_GAMMA        91
#define CMD_RAN          92
#define CMD_SEED         93
#define CMD_LBL          94
#define CMD_RTN          95
#define CMD_INPUT        96
#define CMD_VIEW         97
#define CMD_AVIEW        98
#define CMD_XEQ          99
#define CMD_PROMPT      100
#define CMD_PSE         101
#define CMD_ISG         102
#define CMD_DSE         103
#define CMD_AIP         104
#define CMD_XTOA        105
#define CMD_AGRAPH      106
#define CMD_PIXEL       107
#define CMD_BEEP        108
#define CMD_TONE        109
#define CMD_MVAR        110
#define CMD_VARMENU     111
#define CMD_GETKEY      112
#define CMD_MENU        113
#define CMD_KEYG        114
#define CMD_KEYX        115
#define CMD_X_EQ_0      116
#define CMD_X_NE_0      117
#define CMD_X_LT_0      118
#define CMD_X_GT_0      119
#define CMD_X_LE_0      120
#define CMD_X_GE_0      121
#define CMD_X_EQ_Y      122
#define CMD_X_NE_Y      123
#define CMD_X_LT_Y      124
#define CMD_X_GT_Y      125
#define CMD_X_LE_Y      126
#define CMD_X_GE_Y      127
#define CMD_PRSIGMA     128
#define CMD_PRP         129
#define CMD_PRV         130
#define CMD_PRSTK       131
#define CMD_PRA         132
#define CMD_PRX         133
#define CMD_PRUSR       134
#define CMD_LIST        135
#define CMD_ADV         136
#define CMD_PRLCD       137
#define CMD_DELAY       138
#define CMD_PON         139
#define CMD_POFF        140
#define CMD_MAN         141
#define CMD_NORM        142
#define CMD_TRACE       143
#define CMD_SIGMAADD    144
#define CMD_SIGMASUB    145
#define CMD_GTO         146
#define CMD_END         147
#define CMD_NUMBER      148
#define CMD_STRING      149
#define CMD_RUN         150
#define CMD_SST         151
#define CMD_GTODOT      152
#define CMD_GTODOTDOT   153
#define CMD_STOP        154
#define CMD_NEWMAT      155
#define CMD_RUP         156
#define CMD_REAL_T      157
#define CMD_CPX_T       158
#define CMD_STR_T       159
#define CMD_MAT_T       160
#define CMD_DIM_T       161
#define CMD_ASSIGNa     162
#define CMD_ASSIGNb     163
#define CMD_ASGN01      164
#define CMD_ASGN02      165
#define CMD_ASGN03      166
#define CMD_ASGN04      167
#define CMD_ASGN05      168
#define CMD_ASGN06      169
#define CMD_ASGN07      170
#define CMD_ASGN08      171
#define CMD_ASGN09      172
#define CMD_ASGN10      173
#define CMD_ASGN11      174
#define CMD_ASGN12      175
#define CMD_ASGN13      176
#define CMD_ASGN14      177
#define CMD_ASGN15      178
#define CMD_ASGN16      179
#define CMD_ASGN17      180
#define CMD_ASGN18      181
#define CMD_ON          182
#define CMD_OFF         183
#define CMD_KEY1G       184
#define CMD_KEY2G       185
#define CMD_KEY3G       186
#define CMD_KEY4G       187
#define CMD_KEY5G       188
#define CMD_KEY6G       189
#define CMD_KEY7G       190
#define CMD_KEY8G       191
#define CMD_KEY9G       192
#define CMD_KEY1X       193
#define CMD_KEY2X       194
#define CMD_KEY3X       195
#define CMD_KEY4X       196
#define CMD_KEY5X       197
#define CMD_KEY6X       198
#define CMD_KEY7X       199
#define CMD_KEY8X       200
#define CMD_KEY9X       201
#define CMD_VMEXEC      202
#define CMD_VMSTO       203
#define CMD_SIGMAREG    204
#define CMD_SIGMAREG_T  205
#define CMD_CLD         206
#define CMD_ACOSH       207
#define CMD_ALENG       208
#define CMD_ALLSIGMA    209
#define CMD_AND         210
#define CMD_AOFF        211
#define CMD_AON         212
#define CMD_AROT        213
#define CMD_ASHF        214
#define CMD_ASINH       215
#define CMD_ATANH       216
#define CMD_ATOX        217
#define CMD_BASEADD     218
#define CMD_BASESUB     219
#define CMD_BASEMUL     220
#define CMD_BASEDIV     221
#define CMD_BASECHS     222
#define CMD_BEST        223
#define CMD_BINM        224
#define CMD_BIT_T       225
#define CMD_BST         226
#define CMD_CORR        227
#define CMD_COSH        228
#define CMD_CROSS       229
#define CMD_CUSTOM      230
#define CMD_DECM        231
#define CMD_DELR        232
#define CMD_DET         233
#define CMD_DIM         234
#define CMD_DOT         235
#define CMD_EDIT        236
#define CMD_EDITN       237
#define CMD_EXITALL     238
#define CMD_EXPF        239
#define CMD_E_POW_X_1   240
#define CMD_FCSTX       241
#define CMD_FCSTY       242
#define CMD_FNRM        243
#define CMD_GETM        244
#define CMD_GROW        245
#define CMD_HEXM        246
#define CMD_HMSADD      247
#define CMD_HMSSUB      248
#define CMD_I_ADD       249
#define CMD_I_SUB       250
#define CMD_INDEX       251
#define CMD_INSR        252
#define CMD_INTEG       253
#define CMD_INVRT       254
#define CMD_J_ADD       255
#define CMD_J_SUB       256
#define CMD_LINF        257
#define CMD_LINSIGMA    258
#define CMD_LN_1_X      259
#define CMD_LOGF        260
#define CMD_MEAN        261
#define CMD_NOT         262
#define CMD_OCTM        263
#define CMD_OLD         264
#define CMD_OR          265
#define CMD_PGMSLV      266
#define CMD_PGMINT      267
#define CMD_POSA        268
#define CMD_PUTM        269
#define CMD_PWRF        270
#define CMD_RCLEL       271
#define CMD_RCLIJ       272
#define CMD_RNRM        273
#define CMD_ROTXY       274
#define CMD_RSUM        275
#define CMD_SWAP_R      276
#define CMD_SDEV        277
#define CMD_SINH        278
#define CMD_SLOPE       279
#define CMD_SOLVE       280
#define CMD_STOEL       281
#define CMD_STOIJ       282
#define CMD_SUM         283
#define CMD_TANH        284
#define CMD_TRANS       285
#define CMD_UVEC        286
#define CMD_WMEAN       287
#define CMD_WRAP        288
#define CMD_X_SWAP      289
#define CMD_XOR         290
#define CMD_YINT        291
#define CMD_TO_DEC      292
#define CMD_TO_OCT      293
#define CMD_LEFT        294
#define CMD_UP          295
#define CMD_DOWN        296
#define CMD_RIGHT       297
#define CMD_PERCENT_CH  298
#define CMD_SIMQ        299
#define CMD_MATA        300
#define CMD_MATB        301
#define CMD_MATX        302
#define CMD_GOTOROW     303
#define CMD_GOTOCOLUMN  304
#define CMD_A_THRU_F    305
#define CMD_CLALLb      306
#define CMD_PGMSLVi     307
#define CMD_PGMINTi     308
#define CMD_VMSTO2      309
#define CMD_VMSOLVE     310
#define CMD_MAX         311
#define CMD_MIN         312
#define CMD_FIND        313
#define CMD_XROM        314
/* Underhill's COPAN extensions (Obsolete) */
#define CMD_OPENF       315
#define CMD_CLOSEF      316
#define CMD_READP       317
#define CMD_WRITEP      318
#define CMD_GETXY       319
#define CMD_PUTXY       320
#define CMD_CLRP        321
#define CMD_CLRD        322
#define CMD_APPD        323
#define CMD_GETN        324
#define CMD_PUTN        325
#define CMD_GETZ        326
#define CMD_PUTZ        327
#define CMD_DELP        328
/* Byron Foster's Bigstack extension (Obsolete) */
#define CMD_DROP        329
/* iPhone hardware support */
#define CMD_ACCEL       330
#define CMD_LOCAT       331
#define CMD_HEADING     332
/* HP-41 Time Module & CX Time */
#define CMD_ADATE       333
#define CMD_ALMCAT      334
#define CMD_ALMNOW      335
#define CMD_ATIME       336
#define CMD_ATIME24     337
#define CMD_CLK12       338
#define CMD_CLK24       339
#define CMD_CLKT        340
#define CMD_CLKTD       341
#define CMD_CLOCK       342
#define CMD_CORRECT     343
#define CMD_DATE        344
#define CMD_DATE_PLUS   345
#define CMD_DDAYS       346
#define CMD_DMY         347
#define CMD_DOW         348
#define CMD_MDY         349
#define CMD_RCLAF       350
#define CMD_RCLSW       351
#define CMD_RUNSW       352
#define CMD_SETAF       353
#define CMD_SETDATE     354
#define CMD_SETIME      355
#define CMD_SETSW       356
#define CMD_STOPSW      357
#define CMD_SW          358
#define CMD_T_PLUS_X    359
#define CMD_TIME        360
#define CMD_XYZALM      361
#define CMD_CLALMA      362
#define CMD_CLALMX      363
#define CMD_CLRALMS     364
#define CMD_RCLALM      365
#define CMD_SWPT        366
/* Intel Decimal Floating-Point Math Library: self-test */
#define CMD_FPTEST      367
/* Programming */
#define CMD_LSTO        368
#define CMD_SST_UP      369
#define CMD_SST_RT      370

#define CMD_SENTINEL    371


/* command_spec.argtype */

#define ARG_NONE      0 /* No argument */
#define ARG_VAR       1 /* Variable: num, stk, var, ind */
#define ARG_REAL      2 /* Variable: num, stk, var, ind; real scalars only */
#define ARG_NAMED     3 /* Variable: var, ind */
#define ARG_NUM9      4 /* num (0..9), ind */
#define ARG_NUM11     5 /* num (0..11), ind */
#define ARG_NUM99     6 /* num (0..99), ind */
#define ARG_COUNT     7 /* numeric-only (SIMQ, DEL, SIZE, LIST) */
#define ARG_LBL       8 /* Label: num, lclbl, global, ind */
#define ARG_CKEY      9 /* Key in custom menu */
#define ARG_MKEY     10 /* Key in programmable menu (KEYG/KEYX) */
#define ARG_PRGM     11 /* Alpha label (CATSECT_PGM) */
#define ARG_RVAR     12 /* Variable (real only) (MVAR, INTEG, SOLVE) */
#define ARG_MAT      13 /* Variable (matrix only) (EDITN, INDEX) */
#define ARG_OTHER    14 /* Weirdos */


/* command_spec.flags */

#define FLAG_NONE      0  /* Boring! */
#define FLAG_PRGM_ONLY 1  /* Only allowed in program mode (LBL, DEL, ...) */
#define FLAG_IMMED     2  /* Executes in program mode (DEL, GTO.nnn, ...) */
#define FLAG_HIDDEN    4  /* Cannot be activated using XEQ "NAME" (SIMQ, ...) */
#define FLAG_NO_PRGM   8  /* Cannot be programmed (SIMQ, MATA, ...) */
#define FLAG_NO_SHOW  16  /* Do not show after keytimeout1 */


/* Builtin cmd arg types */

/* These represent data types used when a command
 * is executing. Compare the ARG_ macros; those
 * represent the data types that a command is *capable*
 * of accepting, and are used during command entry.
 */

#define ARGTYPE_NONE      0
#define ARGTYPE_NUM       1
#define ARGTYPE_NEG_NUM   2 /* Used internally */
#define ARGTYPE_STK       3
#define ARGTYPE_STR       4
#define ARGTYPE_IND_NUM   5
#define ARGTYPE_IND_STK   6
#define ARGTYPE_IND_STR   7
#define ARGTYPE_COMMAND   8 /* For backward compatibility only! */
#define ARGTYPE_LCLBL     9
#define ARGTYPE_DOUBLE   10
#define ARGTYPE_LBLINDEX 11


typedef struct {
    unsigned char type;
    unsigned char length;
    int4 target;
    union {
        int4 num;
        char text[15];
        char stk;
        int cmd; /* For backward compatibility only! */
        char lclbl;
    } val;
    // This used to be a member of the 'val' union, but once I changed it
    // from 'double' to 'phloat', that was no longer possible.
    phloat val_d;
} arg_struct;


typedef struct {
    char name[12];
    int name_length;
    int (*handler)(arg_struct *arg);
    uint4 hp42s_code;
    int argtype;
    int flags;
} command_spec;


const command_spec *cmdlist(int index);


#endif
