/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2023  Thomas Okken
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

#include "core_display.h"
#include "core_commands2.h"
#include "core_globals.h"
#include "core_helpers.h"
#include "core_main.h"
#include "core_tables.h"
#include "core_variables.h"
#include "shell.h"
#include "shell_spool.h"


/********************/
/* HP-42S font data */
/********************/

#if defined(WINDOWS) && !defined(__GNUC__)
/* Disable warnings:
 * C4838: conversion from 'int' to 'const char' requires a narrowing conversion
 */
#pragma warning(push)
#pragma warning(disable:4838)
#endif


static const char bigchars[130][5] =
    {
        { 0x08, 0x08, 0x2a, 0x08, 0x08 },
        { 0x22, 0x14, 0x08, 0x14, 0x22 },
        { 0x10, 0x20, 0x7f, 0x01, 0x01 },
        { 0x20, 0x40, 0x3e, 0x01, 0x02 },
        { 0x55, 0x2a, 0x55, 0x2a, 0x55 },
        { 0x41, 0x63, 0x55, 0x49, 0x63 },
        { 0x7f, 0x7f, 0x3e, 0x1c, 0x08 },
        { 0x04, 0x7c, 0x04, 0x7c, 0x04 },
        { 0x30, 0x48, 0x45, 0x40, 0x20 },
        { 0x50, 0x58, 0x54, 0x52, 0x51 },
        { 0x0f, 0x08, 0x00, 0x78, 0x28 },
        { 0x51, 0x52, 0x54, 0x58, 0x50 },
        { 0x14, 0x34, 0x1c, 0x16, 0x14 },
        { 0x20, 0x70, 0xa8, 0x20, 0x3f },
        { 0x10, 0x20, 0x7f, 0x20, 0x10 },
        { 0x08, 0x08, 0x2a, 0x1c, 0x08 },
        { 0x08, 0x1c, 0x2a, 0x08, 0x08 },
        { 0x7e, 0x20, 0x20, 0x1e, 0x20 },
        { 0x48, 0x7e, 0x49, 0x41, 0x02 },
        { 0x00, 0x0e, 0x0a, 0x0e, 0x00 },
        { 0x78, 0x16, 0x15, 0x16, 0x78 },
        { 0x7c, 0x0a, 0x11, 0x22, 0x7d },
        { 0x7c, 0x13, 0x12, 0x13, 0x7c },
        { 0x60, 0x50, 0x58, 0x64, 0x42 },
        { 0x3e, 0x2a, 0x2a, 0x22, 0x00 },
        { 0x7e, 0x09, 0x7f, 0x49, 0x41 },
        { 0x60, 0x00, 0x60, 0x00, 0x60 },
        { 0x1f, 0x15, 0x71, 0x50, 0x50 },
        { 0x3c, 0x43, 0x42, 0x43, 0x3c },
        { 0x3c, 0x41, 0x40, 0x41, 0x3c },
        { 0x55, 0x2a, 0x55, 0x2a, 0x55 },
        { 0x3c, 0x3c, 0x3c, 0x3c, 0x3c },
        { 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x5f, 0x00, 0x00 },
        { 0x00, 0x07, 0x00, 0x07, 0x00 },
        { 0x14, 0x7f, 0x14, 0x7f, 0x14 },
        { 0x24, 0x2a, 0x7f, 0x2a, 0x12 },
        { 0x23, 0x13, 0x08, 0x64, 0x62 },
        { 0x36, 0x49, 0x56, 0x20, 0x50 },
        { 0x00, 0x00, 0x07, 0x00, 0x00 },
        { 0x00, 0x1c, 0x22, 0x41, 0x00 },
        { 0x00, 0x41, 0x22, 0x1c, 0x00 },
        { 0x08, 0x2a, 0x1c, 0x2a, 0x08 },
        { 0x08, 0x08, 0x3e, 0x08, 0x08 },
        { 0x00, 0xb0, 0x70, 0x00, 0x00 },
        { 0x08, 0x08, 0x08, 0x08, 0x00 },
        { 0x00, 0x60, 0x60, 0x00, 0x00 },
        { 0x20, 0x10, 0x08, 0x04, 0x02 },
        { 0x3e, 0x51, 0x49, 0x45, 0x3e },
        { 0x00, 0x42, 0x7f, 0x40, 0x00 },
        { 0x62, 0x51, 0x49, 0x49, 0x46 },
        { 0x22, 0x49, 0x49, 0x49, 0x36 },
        { 0x18, 0x14, 0x12, 0x7f, 0x10 },
        { 0x27, 0x45, 0x45, 0x45, 0x39 },
        { 0x3c, 0x4a, 0x49, 0x49, 0x30 },
        { 0x01, 0x71, 0x09, 0x05, 0x03 },
        { 0x36, 0x49, 0x49, 0x49, 0x36 },
        { 0x06, 0x49, 0x49, 0x29, 0x1e },
        { 0x00, 0x36, 0x36, 0x00, 0x00 },
        { 0x00, 0xb6, 0x76, 0x00, 0x00 },
        { 0x08, 0x14, 0x22, 0x41, 0x00 },
        { 0x14, 0x14, 0x14, 0x14, 0x14 },
        { 0x41, 0x22, 0x14, 0x08, 0x00 },
        { 0x02, 0x01, 0x51, 0x09, 0x06 },
        { 0x3e, 0x41, 0x5d, 0x55, 0x5e },
        { 0x7e, 0x09, 0x09, 0x09, 0x7e },
        { 0x7f, 0x49, 0x49, 0x49, 0x36 },
        { 0x3e, 0x41, 0x41, 0x41, 0x22 },
        { 0x7f, 0x41, 0x41, 0x22, 0x1c },
        { 0x7f, 0x49, 0x49, 0x49, 0x41 },
        { 0x7f, 0x09, 0x09, 0x09, 0x01 },
        { 0x3e, 0x41, 0x41, 0x51, 0x72 },
        { 0x7f, 0x08, 0x08, 0x08, 0x7f },
        { 0x00, 0x41, 0x7f, 0x41, 0x00 },
        { 0x30, 0x40, 0x40, 0x40, 0x3f },
        { 0x7f, 0x08, 0x14, 0x22, 0x41 },
        { 0x7f, 0x40, 0x40, 0x40, 0x40 },
        { 0x7f, 0x02, 0x0c, 0x02, 0x7f },
        { 0x7f, 0x04, 0x08, 0x10, 0x7f },
        { 0x3e, 0x41, 0x41, 0x41, 0x3e },
        { 0x7f, 0x09, 0x09, 0x09, 0x06 },
        { 0x3e, 0x41, 0x51, 0x21, 0x5e },
        { 0x7f, 0x09, 0x19, 0x29, 0x46 },
        { 0x26, 0x49, 0x49, 0x49, 0x32 },
        { 0x01, 0x01, 0x7f, 0x01, 0x01 },
        { 0x3f, 0x40, 0x40, 0x40, 0x3f },
        { 0x07, 0x18, 0x60, 0x18, 0x07 },
        { 0x7f, 0x20, 0x18, 0x20, 0x7f },
        { 0x63, 0x14, 0x08, 0x14, 0x63 },
        { 0x03, 0x04, 0x78, 0x04, 0x03 },
        { 0x61, 0x51, 0x49, 0x45, 0x43 },
        { 0x00, 0x7f, 0x41, 0x41, 0x00 },
        { 0x02, 0x04, 0x08, 0x10, 0x20 },
        { 0x00, 0x41, 0x41, 0x7f, 0x00 },
        { 0x04, 0x02, 0x7f, 0x02, 0x04 },
        { 0x80, 0x80, 0x80, 0x80, 0x80 },
        { 0x00, 0x03, 0x04, 0x00, 0x00 },
        { 0x20, 0x54, 0x54, 0x54, 0x78 },
        { 0x7f, 0x44, 0x44, 0x44, 0x38 },
        { 0x38, 0x44, 0x44, 0x44, 0x44 },
        { 0x38, 0x44, 0x44, 0x44, 0x7f },
        { 0x38, 0x54, 0x54, 0x54, 0x58 },
        { 0x00, 0x08, 0x7e, 0x09, 0x02 },
        { 0x18, 0xa4, 0xa4, 0xa4, 0x78 },
        { 0x7f, 0x04, 0x04, 0x04, 0x78 },
        { 0x00, 0x44, 0x7d, 0x40, 0x00 },
        { 0x00, 0x40, 0x80, 0x84, 0x7d },
        { 0x7f, 0x10, 0x28, 0x44, 0x00 },
        { 0x00, 0x41, 0x7f, 0x40, 0x00 },
        { 0x7c, 0x04, 0x38, 0x04, 0x7c },
        { 0x7c, 0x04, 0x04, 0x04, 0x78 },
        { 0x38, 0x44, 0x44, 0x44, 0x38 },
        { 0xfc, 0x24, 0x24, 0x24, 0x18 },
        { 0x18, 0x24, 0x24, 0x24, 0xfc },
        { 0x7c, 0x08, 0x04, 0x04, 0x04 },
        { 0x48, 0x54, 0x54, 0x54, 0x24 },
        { 0x00, 0x04, 0x3f, 0x44, 0x20 },
        { 0x3c, 0x40, 0x40, 0x40, 0x7c },
        { 0x1c, 0x20, 0x40, 0x20, 0x1c },
        { 0x3c, 0x40, 0x30, 0x40, 0x3c },
        { 0x44, 0x28, 0x10, 0x28, 0x44 },
        { 0x1c, 0xa0, 0xa0, 0xa0, 0x7c },
        { 0x44, 0x64, 0x54, 0x4c, 0x44 },
        { 0x08, 0x36, 0x41, 0x41, 0x00 },
        { 0x00, 0x00, 0x7f, 0x00, 0x00 },
        { 0x00, 0x41, 0x41, 0x36, 0x08 },
        { 0x08, 0x04, 0x08, 0x10, 0x08 },
        { 0x7f, 0x08, 0x08, 0x08, 0x08 },
        { 0x28, 0x00, 0x00, 0x00, 0x00 },
        { 0x04, 0x08, 0x70, 0x08, 0x04 }
    };

static const char smallchars[416] =
    {
        0x00, 0x00, 0x00,
        0x5c,
        0x06, 0x00, 0x06,
        0x28, 0x7c, 0x28, 0x7c, 0x28,
        0x08, 0x54, 0x7c, 0x54, 0x20,
        0x24, 0x10, 0x48,
        0x30, 0x4c, 0x50, 0x20, 0x50,
        0x08, 0x04,
        0x38, 0x44,
        0x44, 0x38,
        0x54, 0x38, 0x54,
        0x10, 0x38, 0x10,
        0x40, 0x20,
        0x10, 0x10, 0x10,
        0x40,
        0x60, 0x10, 0x0c,
        0x38, 0x44, 0x38,
        0x48, 0x7c, 0x40,
        0x74, 0x54, 0x5c,
        0x44, 0x54, 0x7c,
        0x1c, 0x10, 0x7c,
        0x5c, 0x54, 0x74,
        0x7c, 0x54, 0x74,
        0x64, 0x14, 0x0c,
        0x7c, 0x54, 0x7c,
        0x5c, 0x54, 0x7c,
        0x28,
        0x40, 0x28,
        0x10, 0x28, 0x44,
        0x28, 0x28, 0x28,
        0x44, 0x28, 0x10,
        0x08, 0x04, 0x54, 0x08,
        0x38, 0x44, 0x54, 0x58,
        0x78, 0x14, 0x78,
        0x7c, 0x54, 0x28,
        0x38, 0x44, 0x44,
        0x7c, 0x44, 0x38,
        0x7c, 0x54, 0x44,
        0x7c, 0x14, 0x04,
        0x7c, 0x44, 0x54, 0x74,
        0x7c, 0x10, 0x7c,
        0x7c,
        0x60, 0x40, 0x7c,
        0x7c, 0x10, 0x28, 0x44,
        0x7c, 0x40, 0x40,
        0x7c, 0x08, 0x10, 0x08, 0x7c,
        0x7c, 0x18, 0x30, 0x7c,
        0x7c, 0x44, 0x7c,
        0x7c, 0x14, 0x1c,
        0x38, 0x44, 0x24, 0x58,
        0x7c, 0x14, 0x6c,
        0x48, 0x54, 0x24,
        0x04, 0x7c, 0x04,
        0x7c, 0x40, 0x7c,
        0x1c, 0x60, 0x1c,
        0x7c, 0x20, 0x10, 0x20, 0x7c,
        0x6c, 0x10, 0x6c,
        0x0c, 0x70, 0x0c,
        0x64, 0x54, 0x4c,
        0x7c, 0x44,
        0x0c, 0x10, 0x60,
        0x44, 0x7c,
        0x10, 0x08, 0x7c, 0x08, 0x10,
        0x40, 0x40, 0x40,
        0x04, 0x08,
        0x10, 0x6c, 0x44,
        0x6c,
        0x44, 0x6c, 0x10,
        0x10, 0x08, 0x10, 0x20, 0x10,
        0x54, 0x28, 0x54, 0x28, 0x54,
        0x10, 0x54, 0x10,
        0x28, 0x10, 0x28,
        0x10, 0x20, 0x7c, 0x04, 0x04, 0x04,
        0x20, 0x40, 0x38, 0x04, 0x08,
        0x44, 0x6c, 0x54, 0x44,
        0x08, 0x78, 0x08, 0x78, 0x08,
        0x50, 0x58, 0x54,
        0x3c, 0x20, 0x00, 0x78, 0x28,
        0x54, 0x58, 0x50,
        0x28, 0x68, 0x38, 0x2c, 0x28,
        0x10, 0x20, 0x7c, 0x20, 0x10,
        0x10, 0x10, 0x54, 0x38, 0x10,
        0x10, 0x38, 0x54, 0x10, 0x10,
        0x78, 0x20, 0x38, 0x20,
        0x1c, 0x14, 0x1c,
        0x1c, 0x08, 0x08,
        0x60, 0x00, 0x60, 0x00, 0x60,
        0x60, 0x50, 0x58, 0x64, 0x40,
        0x74, 0x28, 0x28, 0x74,
        0x34, 0x48, 0x48, 0x34,
        0x34, 0x40, 0x40, 0x34,
        0x7c, 0x12, 0x24, 0x7a,
        0x50, 0x78, 0x54, 0x04,
        0x20, 0x54, 0x40, 0x20,
        0x78, 0x14, 0x7c, 0x54,
        0x38, 0x38, 0x38,
        0x70, 0x2c, 0x70,
        0x7c, 0x7c, 0x38, 0x10,
        0x30, 0x48, 0x78,
        0x7c, 0x50, 0x70,
        0x30, 0x48, 0x48,
        0x70, 0x50, 0x7c,
        0x30, 0x68, 0x58,
        0x10, 0x7c, 0x14,
        0xb0, 0xa8, 0x78,
        0x7c, 0x10, 0x70,
        0x74,
        0x80, 0xf4,
        0x7c, 0x10, 0x68,
        0x7c, 0x40,
        0x78, 0x08, 0x78, 0x08, 0x70,
        0x78, 0x08, 0x70,
        0x38, 0x48, 0x70,
        0xf8, 0x28, 0x38,
        0x38, 0x28, 0xf8,
        0x70, 0x08, 0x08,
        0x58, 0x58, 0x68,
        0x08, 0x7c, 0x48,
        0x38, 0x40, 0x78,
        0x38, 0x60, 0x38,
        0x38, 0x40, 0x30, 0x40, 0x38,
        0x48, 0x30, 0x48,
        0x98, 0xa0, 0x78,
        0x68, 0x58, 0x58,
        0x20, 0x70, 0x20, 0x3c,
        0x7c, 0x54, 0x00, 0x78, 0x48,
    };

static short smallchars_offset[127] =
    {
          0,
          3,
          4,
          7,
         12,
         17,
         20,
         25,
         27,
         29,
         31,
         34,
         37,
         39,
         42,
         43,
         46,
         49,
         52,
         55,
         58,
         61,
         64,
         67,
         70,
         73,
         76,
         77,
         79,
         82,
         85,
         88,
         92,
         96,
         99,
        102,
        105,
        108,
        111,
        114,
        118,
        121,
        122,
        125,
        129,
        132,
        137,
        141,
        144,
        147,
        151,
        154,
        157,
        160,
        163,
        166,
        171,
        174,
        177,
        180,
        182,
        185,
        187,
        192,
        195,
        197,
        200,
        201,
        204,
        209,
        214,
        217,
        220,
        226,
        231,
        235,
        240,
        243,
        248,
        251,
        256,
        261,
        266,
        271,
        275,
        278,
        281,
        286,
        291,
        295,
        299,
        303,
        307,
        311,
        315,
        319,
        322,
        325,
        329,
        332,
        335,
        338,
        341,
        344,
        347,
        350,
        353,
        354,
        356,
        359,
        361,
        366,
        369,
        372,
        375,
        378,
        381,
        384,
        387,
        390,
        393,
        398,
        401,
        404,
        407,
        411,
        416,
    };

static char smallchars_map[128] =
    {
        /*   0 */  70,
        /*   1 */  71,
        /*   2 */  72,
        /*   3 */  73,
        /*   4 */  69,
        /*   5 */  74,
        /*   6 */  97,
        /*   7 */  75,
        /*   8 */  93,
        /*   9 */  76,
        /*  10 */  77,
        /*  11 */  78,
        /*  12 */  79,
        /*  13 */ 124,
        /*  14 */  80,
        /*  15 */  81,
        /*  16 */  82,
        /*  17 */  83,
        /*  18 */  92,
        /*  19 */  84,
        /*  20 */  96,
        /*  21 */  91,
        /*  22 */  88,
        /*  23 */  87,
        /*  24 */  37,
        /*  25 */  94,
        /*  26 */  86,
        /*  27 */ 125,
        /*  28 */  89,
        /*  29 */  90,
        /*  30 */  69,
        /*  31 */  95,
        /*  32 */   0,
        /*  33 */   1,
        /*  34 */   2,
        /*  35 */   3,
        /*  36 */   4,
        /*  37 */   5,
        /*  38 */   6,
        /*  39 */   7,
        /*  40 */   8,
        /*  41 */   9,
        /*  42 */  10,
        /*  43 */  11,
        /*  44 */  12,
        /*  45 */  13,
        /*  46 */  14,
        /*  47 */  15,
        /*  48 */  16,
        /*  49 */  17,
        /*  50 */  18,
        /*  51 */  19,
        /*  52 */  20,
        /*  53 */  21,
        /*  54 */  22,
        /*  55 */  23,
        /*  56 */  24,
        /*  57 */  25,
        /*  58 */  26,
        /*  59 */  27,
        /*  60 */  28,
        /*  61 */  29,
        /*  62 */  30,
        /*  63 */  31,
        /*  64 */  32,
        /*  65 */  33,
        /*  66 */  34,
        /*  67 */  35,
        /*  68 */  36,
        /*  69 */  37,
        /*  70 */  38,
        /*  71 */  39,
        /*  72 */  40,
        /*  73 */  41,
        /*  74 */  42,
        /*  75 */  43,
        /*  76 */  44,
        /*  77 */  45,
        /*  78 */  46,
        /*  79 */  47,
        /*  80 */  48,
        /*  81 */  49,
        /*  82 */  50,
        /*  83 */  51,
        /*  84 */  52,
        /*  85 */  53,
        /*  86 */  54,
        /*  87 */  55,
        /*  88 */  56,
        /*  89 */  57,
        /*  90 */  58,
        /*  91 */  59,
        /*  92 */  60,
        /*  93 */  61,
        /*  94 */  62,
        /*  95 */  63,
        /*  96 */  64,
        /*  97 */  98,
        /*  98 */  99,
        /*  99 */ 100,
        /* 100 */ 101,
        /* 101 */ 102,
        /* 102 */ 103,
        /* 103 */ 104,
        /* 104 */ 105,
        /* 105 */ 106,
        /* 106 */ 107,
        /* 107 */ 108,
        /* 108 */ 109,
        /* 109 */ 110,
        /* 110 */ 111,
        /* 111 */ 112,
        /* 112 */ 113,
        /* 113 */ 114,
        /* 114 */ 115,
        /* 115 */ 116,
        /* 116 */ 117,
        /* 117 */ 118,
        /* 118 */ 119,
        /* 119 */ 120,
        /* 120 */ 121,
        /* 121 */ 122,
        /* 122 */ 123,
        /* 123 */  65,
        /* 124 */  66,
        /* 125 */  67,
        /* 126 */  68,
        /* 127 */  85
    };

#if defined(WINDOWS) && !defined(__GNUC__)
#pragma warning(pop)
#endif



static char display[272];

static bool is_dirty = false;
static int dirty_top, dirty_left, dirty_bottom, dirty_right;

static int catalogmenu_section[5];
static int catalogmenu_rows[5];
static int catalogmenu_row[5];
static int catalogmenu_item[5][6];

static int custommenu_length[3][6];
static char custommenu_label[3][6][7];

static arg_struct progmenu_arg[9];
static bool progmenu_is_gto[9];
static int progmenu_length[6];
static char progmenu_label[6][7];

static int appmenu_exitcallback;

/* Menu keys that should respond to certain hardware
 * keyboard keys, in addition to the keymap:
 * 0:none 1:left 2:shift-left 3:right 4:shift-right 5:del
 */
static char special_key[6] = { 0, 0, 0, 0, 0, 0 };


/*******************************/
/* Private function prototypes */
/*******************************/

static void mark_dirty(int top, int left, int bottom, int right);
static void fill_rect(int x, int y, int width, int height, int color);
static int get_cat_index();


bool persist_display() {
    for (int i = 0; i < 5; i++) {
        if (!write_int(catalogmenu_section[i])) return false;
        if (!write_int(catalogmenu_rows[i])) return false;
        if (!write_int(catalogmenu_row[i])) return false;
        for (int j = 0; j < 6; j++)
            if (!write_int(catalogmenu_item[i][j])) return false;
    }
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 6; j++) {
            if (!write_int(custommenu_length[i][j])) return false;
            if (fwrite(custommenu_label[i][j], 1, 7, gfile) != 7) return false;
        }
    }
    for (int i = 0; i < 9; i++)
        if (!write_arg(progmenu_arg + i))
            return false;
    for (int i = 0; i < 9; i++)
        if (!write_bool(progmenu_is_gto[i])) return false;
    for (int i = 0; i < 6; i++) {
        if (!write_int(progmenu_length[i])) return false;
        if (fwrite(progmenu_label[i], 1, 7, gfile) != 7) return false;
    }
    if (fwrite(display, 1, 272, gfile) != 272)
        return false;
    if (!write_int(appmenu_exitcallback)) return false;
    if (fwrite(special_key, 1, 6, gfile) != 6)
        return false;
    return true;
}

bool unpersist_display(int version) {
    if (state_is_portable) {
        for (int i = 0; i < 5; i++) {
            if (!read_int(&catalogmenu_section[i])) return false;
            if (!read_int(&catalogmenu_rows[i])) return false;
            if (!read_int(&catalogmenu_row[i])) return false;
            for (int j = 0; j < 6; j++)
                if (!read_int(&catalogmenu_item[i][j])) return false;
        }
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 6; j++) {
                if (!read_int(&custommenu_length[i][j])) return false;
                if (fread(custommenu_label[i][j], 1, 7, gfile) != 7) return false;
            }
        }
        for (int i = 0; i < 9; i++)
            if (!read_arg(progmenu_arg + i, false))
                return false;
        if (version < 35) {
            int temp;
            for (int i = 0; i < 9; i++) {
                if (!read_int(&temp)) return false;
                progmenu_is_gto[i] = temp != 0;
            }
        } else {
            for (int i = 0; i < 9; i++)
                if (!read_bool(&progmenu_is_gto[i])) return false;
        }
        for (int i = 0; i < 6; i++) {
            if (!read_int(&progmenu_length[i])) return false;
            if (fread(progmenu_label[i], 1, 7, gfile) != 7) return false;
        }
        if (fread(display, 1, 272, gfile) != 272)
            return false;
        if (!read_int(&appmenu_exitcallback)) return false;
        if (version >= 44) {
            if (fread(special_key, 1, 6, gfile) != 6)
                return false;
        } else
            memset(special_key, 0, 6);
    } else {
        int custommenu_cmd[3][6];
        is_dirty = false;
        if (fread(catalogmenu_section, 1, 5 * sizeof(int), gfile)
                != 5 * sizeof(int))
            return false;
        if (fread(catalogmenu_rows, 1, 5 * sizeof(int), gfile)
                != 5 * sizeof(int))
            return false;
        if (fread(catalogmenu_row, 1, 5 * sizeof(int), gfile)
                != 5 * sizeof(int))
            return false;
        if (fread(catalogmenu_item, 1, 30 * sizeof(int), gfile)
                != 30 * sizeof(int))
            return false;

        if (version < 7) {
            /* In version 7, I removed the special handling
             * of FCN catalog assignments (after discovering how
             * the real HP-42S does it and realizing that, for perfect
             * compatibility, I had to do it the same way).
             */
            if (fread(custommenu_cmd, 1, 18 * sizeof(int), gfile)
                    != 18 * sizeof(int))
                return false;
        }
        if (fread(custommenu_length, 1, 18 * sizeof(int), gfile)
                != 18 * sizeof(int))
            return false;
        if (fread(custommenu_label, 1, 126, gfile)
                != 126)
            return false;
        if (version < 7) {
            /* Starting with version 7, FCN catalog assignments are no longer
             * handled specially; instead, commands with shortened key labels
             * (e.g. ENTER/ENTR, ASSIGN/ASGN) are handled by using "meta"
             * characters to indicate the disappearing positions. This is how
             * the HP-42S does it, and since this can even affect programs
             * (e.g. create the line STO "ENTER" by selecting ENTER from the
             * function catalog, and the second E is encoded as meta-E in the
             * program!), we have to do it the same way, for compatibility.
             * I think my original approach was better, but such is life. :-)
             */
            int row, pos;
            for (row = 0; row < 3; row++)
                for (pos = 0; pos < 6; pos++) {
                    int cmd = custommenu_cmd[row][pos];
                    if (cmd != CMD_NONE) {
                        const command_spec *cs = &cmd_array[cmd];
                        string_copy(custommenu_label[row][pos],
                                    &custommenu_length[row][pos],
                                    cs->name, cs->name_length);
                    }
                }
        }

        for (int i = 0; i < 9; i++)
            if (!read_arg(progmenu_arg + i, version < 9))
                return false;
        for (int i = 0; i < 9; i++) {
            int temp;
            if (fread(&temp, 1, sizeof(int), gfile) != sizeof(int))
                return false;
            progmenu_is_gto[i] = temp != 0;
        }
        if (fread(progmenu_length, 1, 6 * sizeof(int), gfile)
                != 6 * sizeof(int))
            return false;
        if (fread(progmenu_label, 1, 42, gfile)
                != 42)
            return false;
        if (fread(display, 1, 272, gfile)
                != 272)
            return false;
        if (fread(&appmenu_exitcallback, 1, sizeof(int), gfile)
                != sizeof(int))
            return false;
    }
    return true;
}

void clear_display() {
    int i;
    for (i = 0; i < 272; i++)
        display[i] = 0;
    mark_dirty(0, 0, 16, 131);
    memset(special_key, 0, 6);
}

void flush_display() {
    if (!is_dirty)
        return;
    shell_blitter(display, 17, dirty_left, dirty_top,
                    dirty_right - dirty_left, dirty_bottom - dirty_top);
    is_dirty = false;
}

void repaint_display() {
    shell_blitter(display, 17, 0, 0, 131, 16);
}

void draw_pixel(int x, int y) {
    display[y * 17 + (x >> 3)] |= 1 << (x & 7);
    mark_dirty(y, x, y + 1, x + 1);
}

void draw_pattern(phloat dx, phloat dy, const char *pattern, int pattern_width){
    int x, y, h, v, hmin, hmax, vmin, vmax;
    x = dx < 0 ? to_int(-floor(-dx + 0.5)) : to_int(floor(dx + 0.5));
    y = dy < 0 ? to_int(-floor(-dy + 0.5)) : to_int(floor(dy + 0.5));
    if (x + pattern_width < 1 || x > 131 || y + 8 < 1 || y > 16)
        return;
    hmin = x < 1 ? 1 - x : 0;
    hmax = x + pattern_width > 132 ? 132 - x : pattern_width;
    vmin = y < 1 ? 1 - y : 0;
    vmax = y + 8 > 17 ? 17 - y : 8;
    x--;
    y--;
    if (flags.f.agraph_control1) {
        if (flags.f.agraph_control0)
            /* dst = dst ^ src */
            for (h = hmin; h < hmax; h++) {
                char c = pattern[h] >> vmin;
                for (v = vmin; v < vmax; v++) {
                    if (c & 1) {
                        int X = h + x;
                        int Y = v + y;
                        display[Y * 17 + (X >> 3)] ^= 1 << (X & 7);
                    }
                    c >>= 1;
                }
            }
        else
            /* dst = dst & ~src */
            for (h = hmin; h < hmax; h++) {
                char c = pattern[h] >> vmin;
                for (v = vmin; v < vmax; v++) {
                    if (c & 1) {
                        int X = h + x;
                        int Y = v + y;
                        display[Y * 17 + (X >> 3)] &= ~(1 << (X & 7));
                    }
                    c >>= 1;
                }
            }
    } else {
        if (flags.f.agraph_control0)
            /* dst = src */
            for (h = hmin; h < hmax; h++) {
                char c = pattern[h] >> vmin;
                for (v = vmin; v < vmax; v++) {
                    int X = h + x;
                    int Y = v + y;
                    if (c & 1)
                        display[Y * 17 + (X >> 3)] |= 1 << (X & 7);
                    else
                        display[Y * 17 + (X >> 3)] &= ~(1 << (X & 7));
                    c >>= 1;
                }
            }
        else
            /* dst = dst | src */
            for (h = hmin; h < hmax; h++) {
                char c = pattern[h] >> vmin;
                for (v = vmin; v < vmax; v++) {
                    if (c & 1) {
                        int X = h + x;
                        int Y = v + y;
                        display[Y * 17 + (X >> 3)] |= 1 << (X & 7);
                    }
                    c >>= 1;
                }
            }
    }
    mark_dirty(y + vmin, x + hmin, y + vmax, x + hmax);
}

void fly_goose() {
    static uint4 lastgoosetime = 0;
    uint4 goosetime = shell_milliseconds();
    if (goosetime < lastgoosetime)
        // shell_millisends() wrapped around
        lastgoosetime = 0;
    if (goosetime - 100 < lastgoosetime)
        /* No goose movements if the most recent one was less than 100 ms
         * ago; in other words, maximum goose speed is 10 positions/second
         */
        return;
    lastgoosetime = goosetime;

    if (mode_goose < 0) {
        clear_row(0);
        mode_goose = (-mode_goose) % 22;
        draw_char(mode_goose, 0, 6);
    } else {
        draw_char(mode_goose, 0, ' ');
        mode_goose = (mode_goose + 1) % 22;
        draw_char(mode_goose, 0, 6);
    }
    flush_display();
}

void squeak() {
    if (flags.f.audio_enable)
        shell_beeper(10);
}

void tone(int n) {
    if (flags.f.audio_enable)
        shell_beeper(n);
}


static void mark_dirty(int top, int left, int bottom, int right) {
    if (is_dirty) {
        if (top < dirty_top)
            dirty_top = top;
        if (left < dirty_left)
            dirty_left = left;
        if (bottom > dirty_bottom)
            dirty_bottom = bottom;
        if (right > dirty_right)
            dirty_right = right;
    } else {
        dirty_top = top;
        dirty_left = left;
        dirty_bottom = bottom;
        dirty_right = right;
        is_dirty = true;
    }
}

void draw_char(int x, int y, char c) {
    int X, Y, h, v;
    unsigned char uc = (unsigned char) c;
    if (x < 0 || x >= 22 || y < 0 || y >= 2)
        return;
    if (uc >= 130)
        uc -= 128;
    X = x * 6;
    Y = y * 8;
    for (v = 0; v < 8; v++) {
        int YY = Y + v;
        for (h = 0; h < 5; h++) {
            int XX = X + h;
            char mask = 1 << (XX & 7);
            if (bigchars[uc][h] & (1 << v))
                display[YY * 17 + (XX >> 3)] |= mask;
            else
                display[YY * 17 + (XX >> 3)] &= ~mask;
        }
    }
    mark_dirty(Y, X, Y + 8, X + 5);
}

const char *get_char(char c) {
    unsigned char uc = (unsigned char) c;
    if (uc >= 130)
        uc -= 128;
    return bigchars[uc];
}

void draw_string(int x, int y, const char *s, int length) {
    while (length != 0 && x < 22) {
        draw_char(x++, y, *s++);
        length--;
    }
}

static void fill_rect(int x, int y, int width, int height, int color) {
    int h, v;
    for (v = y; v < y + height; v++)
        for (h = x; h < x + width; h++)
            if (color)
                display[v * 17 + (h >> 3)] |= 1 << (h & 7);
            else
                display[v * 17 + (h >> 3)] &= ~(1 << (h & 7));
    mark_dirty(y, x, y + height, x + width);
}

static void draw_key(int n, int highlight, int hide_meta,
                            const char *s, int length) {
    int swidth = 0;
    int len = 0;
    int len2;
    int x, i;
    int fatdot = 31;

    /* Note: the SST handling code uses a magic value of 2 in prgm_mode
     * so that we know *not* to suppress menu highlights while stepping. */
    if (flags.f.prgm_mode == 1)
        highlight = 0;

    if (highlight) {
        int f = smallchars_map[fatdot];
        swidth = smallchars_offset[f + 1] - smallchars_offset[f];
    }

    while (len < length) {
        int c, m, cw;
        c = (unsigned char) s[len++];
        if (hide_meta && c >= 128)
            continue;

        c &= 127;
        if (mode_menu_caps && c >= 'a' && c <= 'z')
            c -= 32;
        m = smallchars_map[c];
        cw = smallchars_offset[m + 1] - smallchars_offset[m];
        if (swidth != 0)
            cw++;
        if (swidth + cw > 19) {
            len--;
            break;
        }
        swidth += cw;
    }

    fill_rect(n * 22, 9, 21, 7, 1);
    x = n * 22 + 10 - swidth / 2;
    len2 = highlight ? len + 1 : len;
    for (i = 0; i < len2; i++) {
        int c, m, o, cw, j;
        if (i == len)
            c = fatdot;
        else
            c = (unsigned char) s[i];
        if (hide_meta && c >= 128)
            continue;
        c &= 127;
        if (mode_menu_caps && c >= 'a' && c <= 'z')
            c -= 32;
        m = smallchars_map[c];
        o = smallchars_offset[m];
        cw = smallchars_offset[m + 1] - o;
        for (j = 0; j < cw; j++) {
            int b, k;
            b = smallchars[o + j];
            for (k = 0; k < 8; k++)
                if ((b >> k) & 1)
                    display[(k + 8) * 17 + (x >> 3)] &= ~(1 << (x & 7));
            x++;
        }
        x++;
    }
    /* No need for mark_dirty(); fill_rect() took care of that already. */

    /* Support for automatically mapping physical cursor left,
     * cursor right, and delete keys, to menu keys with legends
     * consisting of arrows, double-head arrows, or the word
     * DEL.
     */
    if (string_equals(s, length, "\20", 1))
        /* <- */
        special_key[n] = 1;
    else if (string_equals(s, length, "<\20", 2)
          || string_equals(s, length, "^", 1))
        /* <<- or up */
        special_key[n] = 2;
    else if (string_equals(s, length, "\17", 1))
        /* -> */
        special_key[n] = 3;
    else if (string_equals(s, length, "\17>", 2)
          || string_equals(s, length, "\16", 1))
        /* ->> or down */
        special_key[n] = 4;
    else if (string_equals(s, length, "DEL", 3))
        special_key[n] = 5;
    else
        special_key[n] = 0;
}

int special_menu_key(int which) {
    for (int i = 0; i < 6; i++)
        if (special_key[i] == which)
            return i + 1;
    return 0;
}

void clear_row(int row) {
    fill_rect(0, row * 8, 131, 8, 0);
}

static int prgmline2buf(char *buf, int len, int4 line, int highlight,
                        int cmd, arg_struct *arg, const char *orig_num,
                        bool shift_left = false,
                        bool highlight_final_end = true,
                        char **xstr = NULL) {
    int bufptr = 0;
    if (line != -1) {
        if (line < 10)
            char2buf(buf, len, &bufptr, '0');
        bufptr += int2string(line, buf + bufptr, len - bufptr);
        if (highlight)
            char2buf(buf, len, &bufptr, 6);
        else
            char2buf(buf, len, &bufptr, ' ');
    }

    if (line == 0) {
        int4 size = core_program_size(current_prgm);
        string2buf(buf, len, &bufptr, "{ ", 2);
        bufptr += int2string(size, buf + bufptr, len - bufptr);
        string2buf(buf, len, &bufptr, "-Byte Prgm }", 12);
    } else if (alpha_active() && mode_alpha_entry && highlight) {
        int append = entered_string_length > 0 && entered_string[0] == 127;
        if (append) {
            string2buf(buf, len, &bufptr, "\177\"", 2);
            string2buf(buf, len, &bufptr, entered_string + 1,
                            entered_string_length - 1);
        } else {
            char2buf(buf, len, &bufptr, '"');
            string2buf(buf, len, &bufptr, entered_string, entered_string_length);
        }
        char2buf(buf, len, &bufptr, '_');
    } else if (highlight_final_end && cmd == CMD_END
                    && current_prgm == prgms_count - 1) {
        string2buf(buf, len, &bufptr, ".END.", 5);
    } else if (cmd == CMD_NUMBER) {
        const char *num;
        if (orig_num != NULL)
            num = orig_num;
        else
            num = phloat2program(arg->val_d);
        int numlen = (int) strlen(num);
        if (bufptr + numlen <= len) {
            memcpy(buf + bufptr, num, numlen);
            bufptr += numlen;
        } else {
            if (shift_left) {
                buf[0] = 26;
                if (numlen >= len - 1) {
                    memcpy(buf + 1, num + numlen - len + 1, len - 1);
                } else {
                    int off = bufptr + numlen - len;
                    memmove(buf + 1, buf + off + 1, bufptr - off - 1);
                    bufptr -= off;
                    memcpy(buf + bufptr, num, len - bufptr);
                }
            } else {
                memcpy(buf + bufptr, num, len - bufptr - 1);
                buf[len - 1] = 26;
            }
            bufptr = len;
        }
    } else if (cmd == CMD_STRING) {
        int append = arg->length > 0 && arg->val.text[0] == 127;
        if (append)
            char2buf(buf, len, &bufptr, 127);
        char2buf(buf, len, &bufptr, '"');
        string2buf(buf, len, &bufptr, arg->val.text + append,
                                     arg->length - append);
        char2buf(buf, len, &bufptr, '"');
    } else if (cmd == CMD_XSTR && xstr != NULL && bufptr + 7 + arg->length > len) {
        *xstr = (char *) malloc(bufptr + 7 + arg->length);
        if (*xstr == NULL)
            goto normal;
        memcpy(*xstr, buf, bufptr);
        bufptr += command2buf(*xstr + bufptr, arg->length + 7, cmd, arg);
    } else {
        normal:
        bufptr += command2buf(buf + bufptr, len - bufptr, cmd, arg);
    }

    return bufptr;
}

void tb_write(textbuf *tb, const char *data, size_t size) {
    if (tb->size + size > tb->capacity) {
        size_t newcapacity = tb->capacity == 0 ? 1024 : (tb->capacity << 1);
        while (newcapacity < tb->size + size)
            newcapacity <<= 1;
        char *newbuf = (char *) realloc(tb->buf, newcapacity);
        if (newbuf == NULL) {
            /* Bummer! Let's just append as much as we can */
            memcpy(tb->buf + tb->size, data, tb->capacity - tb->size);
            tb->size = tb->capacity;
            tb->fail = true;
        } else {
            tb->buf = newbuf;
            tb->capacity = newcapacity;
            memcpy(tb->buf + tb->size, data, size);
            tb->size += size;
        }
    } else {
        memcpy(tb->buf + tb->size, data, size);
        tb->size += size;
    }
}

void tb_indent(textbuf *tb, int indent) {
    for (int i = 0; i < indent; i++)
        tb_write(tb, " ", 1);
}

void tb_write_null(textbuf *tb) {
    char c = 0;
    tb_write(tb, &c, 1);
}

void tb_print_current_program(textbuf *tb) {
    int4 pc = 0;
    int line = 0;
    int cmd;
    arg_struct arg;
    bool end = false;
    char buf[100];
    char utf8buf[500];
    do {
        const char *orig_num;
        if (line > 0) {
            get_next_command(&pc, &cmd, &arg, 0, &orig_num);
            if (cmd == CMD_END)
                end = true;
        }
        char *xstr = NULL;
        int len = prgmline2buf(buf, 100, line, cmd == CMD_LBL, cmd, &arg, orig_num, false, false, &xstr);
        char *buf2 = xstr == NULL ? buf : xstr;
        for (int i = 0; i < len; i++)
            if (buf2[i] == 10)
                buf2[i] = 138;
        int off = 0;
        while (len > 0) {
            int slen = len <= 100 ? len : 100;
            int utf8len = hp2ascii(utf8buf, buf2 + off, slen);
            tb_write(tb, utf8buf, utf8len);
            off += slen;
            len -= slen;
        }
        tb_write(tb, "\r\n", 2);
        free(xstr);
        line++;
    } while (!end);
}

void display_prgm_line(int row, int line_offset) {
    int4 tmppc = pc;
    int4 tmpline = pc2line(pc);
    int cmd;
    arg_struct arg;
    char buf[44];
    int bufptr;
    int len = 22;
    const char *orig_num;

    if (row == -1)
        /* This means use both lines; used by SHOW */
        len = 44;

    if (tmpline != 0) {
        if (line_offset == 0)
            get_next_command(&tmppc, &cmd, &arg, 0, &orig_num);
        else if (line_offset == 1) {
            tmppc += get_command_length(current_prgm, tmppc);
            tmpline++;
            get_next_command(&tmppc, &cmd, &arg, 0, &orig_num);
        } else /* line_offset == -1 */ {
            tmpline--;
            if (tmpline != 0) {
                tmppc = line2pc(tmpline);
                get_next_command(&tmppc, &cmd, &arg, 0, &orig_num);
            }
        }
    } else {
        if (line_offset == 0) {
            /* Nothing to do */
        } else if (line_offset == 1) {
            tmppc = 0;
            get_next_command(&tmppc, &cmd, &arg, 0, &orig_num);
            tmpline++;
        }
        /* Should not get offset == -1 when at line 0! */
    }

    bufptr = prgmline2buf(buf, len, tmpline, line_offset == 0, cmd, &arg, orig_num, row == -1);

    if (row == -1) {
        clear_display();
        if (bufptr <= 22)
            draw_string(0, 0, buf, bufptr);
        else {
            draw_string(0, 0, buf, 22);
            draw_string(0, 1, buf + 22, bufptr - 22);
        }
    } else {
        clear_row(row);
        draw_string(0, row, buf, bufptr);
    }
}

void display_x(int row) {
    char buf[22];
    int bufptr = 0;

    clear_row(row);
    vartype *x = sp >= 0 ? stack[sp] : NULL;
    if (matedit_mode == 2 || matedit_mode == 3) {
        bufptr += int2string(matedit_i + 1, buf + bufptr, 22 - bufptr);
        char2buf(buf, 22, &bufptr, ':');
        bufptr += int2string(matedit_j + 1, buf + bufptr, 22 - bufptr);
        char2buf(buf, 22, &bufptr, '=');
    } else if (input_length > 0) {
        string2buf(buf, 22, &bufptr, input_name, input_length);
        char2buf(buf, 22, &bufptr, '?');
    } else if (flags.f.big_stack) {
        string2buf(buf, 22, &bufptr, "1\200", 2);
    } else {
        string2buf(buf, 22, &bufptr, "x\200", 2);
    }
    if (x != NULL)
        bufptr += vartype2string(x, buf + bufptr, 22 - bufptr);
    draw_string(0, row, buf, bufptr);
}

void display_y(int row) {
    char buf[20];
    int len;
    clear_row(row);
    vartype *y = sp >= 1 ? stack[sp - 1] : NULL;
    if (flags.f.big_stack)
        draw_string(0, row, "2\200", 2);
    else
        draw_string(0, row, "\201\200", 2);
    if (y != NULL) {
        len = vartype2string(y, buf, 20);
        if (len > 20) {
            draw_string(2, row, buf, 19);
            draw_char(21, row, 26);
        } else
            draw_string(2, row, buf, len);
    }
}

void display_incomplete_command(int row) {
    char buf[40];
    int bufptr = 0;
    const command_spec *cmd = &cmd_array[incomplete_command];

    if (flags.f.prgm_mode && (cmd->flags & FLAG_IMMED) == 0) {
        int line = pc2line(pc);
        if (line < 10)
            char2buf(buf, 40, &bufptr, '0');
        bufptr += int2string(line, buf + bufptr, 40 - bufptr);
        char2buf(buf, 40, &bufptr, 6);
    }

    if (incomplete_command == CMD_ASSIGNb) {
        string2buf(buf, 40, &bufptr, "ASSIGN \"", 8);
        string2buf(buf, 40, &bufptr, pending_command_arg.val.text,
                                     pending_command_arg.length);
        string2buf(buf, 40, &bufptr, "\" TO _", 6);
        goto done;
    }

    if (incomplete_argtype == ARG_MKEY) {
        /* KEYG/KEYX */
        string2buf(buf, 40, &bufptr, "KEY _", 5);
        goto done;
    }

    if (incomplete_command == CMD_SIMQ)
        string2buf(buf, 40, &bufptr, "Number of Unknowns ", 19);
    else {
        string2buf(buf, 40, &bufptr, cmd->name, cmd->name_length);
        char2buf(buf, 40, &bufptr, ' ');
    }

    if (incomplete_ind)
        string2buf(buf, 40, &bufptr, "IND ", 4);
    if (incomplete_alpha) {
        char2buf(buf, 40, &bufptr, '"');
        string2buf(buf, 40, &bufptr, incomplete_str, incomplete_length);
        char2buf(buf, 40, &bufptr, '_');
    } else {
        int d = 1;
        int i;
        for (i = 0; i < incomplete_length - 1; i++)
            d *= 10;
        for (i = 0; i < incomplete_maxdigits; i++) {
            if (i < incomplete_length) {
                char2buf(buf, 40, &bufptr,
                                    (char) ('0' + (incomplete_num / d) % 10));
                d /= 10;
            } else
                char2buf(buf, 40, &bufptr, '_');
        }
    }

    done:
    clear_row(row);
    if (bufptr <= 22)
        draw_string(0, row, buf, bufptr);
    else {
        draw_char(0, row, 26);
        draw_string(1, row, buf + (bufptr - 21), 21);
    }
}

void display_error(int error, bool print) {
    clear_row(0);
    int err_len;
    const char *err_text;
    if (error == -1) {
        err_len = lasterr_length;
        err_text = lasterr_text;
    } else {
        err_len = errors[error].length;
        err_text = errors[error].text;
    }
    draw_string(0, 0, err_text, err_len);
    flags.f.message = 1;
    flags.f.two_line_message = 0;
    if (print && (flags.f.trace_print || flags.f.normal_print)
            && flags.f.printer_exists)
        print_text(err_text, err_len, true);
}

void display_command(int row) {
    char buf[22];
    int bufptr = 0;
    const command_spec *cmd = &cmd_array[pending_command];
    int catsect;
    int hide = pending_command == CMD_VMEXEC
            || pending_command == CMD_PMEXEC
            || (pending_command == CMD_XEQ
                && xeq_invisible
                && get_front_menu() == MENU_CATALOG
                && ((catsect = get_cat_section()) == CATSECT_PGM
                    || catsect == CATSECT_PGM_ONLY));

    if (pending_command >= CMD_ASGN01 && pending_command <= CMD_ASGN18)
        string2buf(buf, 22, &bufptr, "ASSIGN ", 7);
    else if (!hide) {
        if (pending_command == CMD_SIMQ)
            string2buf(buf, 22, &bufptr, "Number of Unknowns ", 19);
        else {
            string2buf(buf, 22, &bufptr, cmd->name, cmd->name_length);
            char2buf(buf, 22, &bufptr, ' ');
        }
    }

    if (cmd->argtype == ARG_NONE)
        goto done;

    if (pending_command_arg.type == ARGTYPE_IND_NUM
            || pending_command_arg.type == ARGTYPE_IND_STK
            || pending_command_arg.type == ARGTYPE_IND_STR)
        string2buf(buf, 22, &bufptr, "IND ", 4);

    if (pending_command_arg.type == ARGTYPE_NUM
            || pending_command_arg.type == ARGTYPE_IND_NUM) {
        int d = 1, i;
        int leadingzero = 1;
        for (i = 0; i < pending_command_arg.length - 1; i++)
            d *= 10;
        for (i = 0; i < pending_command_arg.length; i++) {
            int digit = (pending_command_arg.val.num / d) % 10;
            if (digit != 0 || i >= pending_command_arg.length - 2)
                leadingzero = 0;
            if (!leadingzero)
                char2buf(buf, 22, &bufptr, (char) ('0' + digit));
            d /= 10;
        }
    } else if (pending_command_arg.type == ARGTYPE_STK
            || pending_command_arg.type == ARGTYPE_IND_STK) {
        string2buf(buf, 22, &bufptr, "ST ", 3);
        char2buf(buf, 22, &bufptr, pending_command_arg.val.stk);
    } else if (pending_command_arg.type == ARGTYPE_STR
            || pending_command_arg.type == ARGTYPE_IND_STR) {
        char2buf(buf, 22, &bufptr, '"');
        string2buf(buf, 22, &bufptr, pending_command_arg.val.text,
                                     pending_command_arg.length);
        char2buf(buf, 22, &bufptr, '"');
    } else if (pending_command_arg.type == ARGTYPE_LBLINDEX) {
        int labelindex = pending_command_arg.val.num;
        if (labels[labelindex].length == 0)
            if (labelindex == labels_count - 1)
                string2buf(buf, 22, &bufptr, ".END.", 5);
            else
                string2buf(buf, 22, &bufptr, "END", 3);
        else {
            char2buf(buf, 22, &bufptr, '"');
            string2buf(buf, 22, &bufptr, labels[labelindex].name,
                                         labels[labelindex].length);
            char2buf(buf, 22, &bufptr, '"');
        }
     } else if (pending_command_arg.type == ARGTYPE_XSTR) {
         char2buf(buf, 22, &bufptr, '"');
         string2buf(buf, 22, &bufptr, pending_command_arg.val.xstr,
                    pending_command_arg.length);
         char2buf(buf, 22, &bufptr, '"');
    } else /* ARGTYPE_LCLBL */ {
        char2buf(buf, 22, &bufptr, pending_command_arg.val.lclbl);
    }

    if (pending_command >= CMD_ASGN01 && pending_command <= CMD_ASGN18) {
        int keynum = pending_command - CMD_ASGN01 + 1;
        string2buf(buf, 22, &bufptr, " TO ", 4);
        char2buf(buf, 22, &bufptr, (char) ('0' + keynum / 10));
        char2buf(buf, 22, &bufptr, (char) ('0' + keynum % 10));
    }

done:
    clear_row(row);
    draw_string(0, row, buf, bufptr);
}

static int set_appmenu(int menuid, bool exitall) {
    if (mode_appmenu != MENU_NONE && appmenu_exitcallback != 0) {
        /* We delegate the set_menu() call to the callback,
         * but only once. If the callback wants to stay active,
         * it will have to call set_appmenu_callback() itself
         * to reinstate itself.
         */
        int cb = appmenu_exitcallback;
        appmenu_exitcallback = 0;
        /* NOTE: I don't use a 'traditional' callback pointer
         * because appmenu_exitcallback has to be persistable,
         * and pointers to code do not have that property.
         */
        switch (cb) {
            case 1: return appmenu_exitcallback_1(menuid, exitall);
            case 2: return appmenu_exitcallback_2(menuid, exitall);
            case 3: return appmenu_exitcallback_3(menuid, exitall);
            case 4: return appmenu_exitcallback_4(menuid, exitall);
            case 5: return appmenu_exitcallback_5(menuid, exitall);
            default: return ERR_INTERNAL_ERROR;
        }
    } else {
        mode_appmenu = menuid;
        appmenu_exitcallback = 0;
        return ERR_NONE;
    }
}

void draw_varmenu() {
    arg_struct arg;
    int saved_prgm, prgm;
    int4 pc, pc2;
    int command, i, row, key;
    int num_mvars = 0;

    if (mode_appmenu != MENU_VARMENU)
        return;
    arg.type = ARGTYPE_STR;
    arg.length = varmenu_length;
    for (i = 0; i < arg.length; i++)
        arg.val.text[i] = varmenu[i];
    if (!find_global_label(&arg, &prgm, &pc)) {
        set_appmenu(MENU_NONE, false);
        varmenu_length = 0;
        return;
    }
    saved_prgm = current_prgm;
    current_prgm = prgm;
    pc += get_command_length(prgm, pc);
    pc2 = pc;
    while (get_next_command(&pc, &command, &arg, 0, NULL), command == CMD_MVAR)
        num_mvars++;
    if (num_mvars == 0) {
        current_prgm = saved_prgm;
        set_appmenu(MENU_NONE, false);
        varmenu_length = 0;
        return;
    }

    varmenu_rows = (num_mvars + 5) / 6;
    if (varmenu_row >= varmenu_rows)
        varmenu_row = varmenu_rows - 1;
    shell_annunciators(varmenu_rows > 1, -1, -1, -1, -1, -1);

    row = 0;
    key = 0;
    while (get_next_command(&pc2, &command, &arg, 0, NULL), command == CMD_MVAR) {
        if (row == varmenu_row) {
            varmenu_labellength[key] = arg.length;
            for (i = 0; i < arg.length; i++)
                varmenu_labeltext[key][i] = arg.val.text[i];
            draw_key(key, 0, 0, arg.val.text, arg.length);
        }
        if (key++ == 5) {
            if (row++ == varmenu_row)
                break;
            else
                key = 0;
        }
    }
    current_prgm = saved_prgm;
    while (key < 6) {
        varmenu_labellength[key] = 0;
        draw_key(key, 0, 0, "", 0);
        key++;
    }
}

static int fcn_cat[] = {
    CMD_ABS,      CMD_ACOS,      CMD_ACOSH,    CMD_ADV,        CMD_AGRAPH,  CMD_AIP,
    CMD_ALENG,    CMD_ALL,       CMD_ALLSIGMA, CMD_AND,        CMD_AOFF,    CMD_AON,
    CMD_ARCL,     CMD_AROT,      CMD_ASHF,     CMD_ASIN,       CMD_ASINH,   CMD_ASSIGNa,
    CMD_ASTO,     CMD_ATAN,      CMD_ATANH,    CMD_ATOX,       CMD_AVIEW,   CMD_BASEADD,
    CMD_BASESUB,  CMD_BASEMUL,   CMD_BASEDIV,  CMD_BASECHS,    CMD_BEEP,    CMD_BEST,
    CMD_BINM,     CMD_BIT_T,     CMD_BST,      CMD_CF,         CMD_CLA,     CMD_CLALLa,
    CMD_CLD,      CMD_CLKEYS,    CMD_CLLCD,    CMD_CLMENU,     CMD_CLP,     CMD_CLRG,
    CMD_CLST,     CMD_CLV,       CMD_CLX,      CMD_CLSIGMA,    CMD_COMB,    CMD_COMPLEX,
    CMD_CORR,     CMD_COS,       CMD_COSH,     CMD_CPXRES,     CMD_CPX_T,   CMD_CROSS,
    CMD_CUSTOM,   CMD_DECM,      CMD_DEG,      CMD_DEL,        CMD_DELAY,   CMD_DELR,
    CMD_DET,      CMD_DIM,       CMD_DIM_T,    CMD_DOT,        CMD_DSE,     CMD_EDIT,
    CMD_EDITN,    CMD_END,       CMD_ENG,      CMD_ENTER,      CMD_EXITALL, CMD_EXPF,
    CMD_E_POW_X,  CMD_E_POW_X_1, CMD_FC_T,     CMD_FCC_T,      CMD_FCSTX,   CMD_FCSTY,
    CMD_FIX,      CMD_FNRM,      CMD_FP,       CMD_FS_T,       CMD_FSC_T,   CMD_GAMMA,
    CMD_GETKEY,   CMD_GETM,      CMD_GRAD,     CMD_GROW,       CMD_GTO,     CMD_HEXM,
    CMD_HMSADD,   CMD_HMSSUB,    CMD_I_ADD,    CMD_I_SUB,      CMD_INDEX,   CMD_INPUT,
    CMD_INSR,     CMD_INTEG,     CMD_INVRT,    CMD_IP,         CMD_ISG,     CMD_J_ADD,
    CMD_J_SUB,    CMD_KEYASN,    CMD_KEYG,     CMD_KEYX,       CMD_LASTX,   CMD_LBL,
    CMD_LCLBL,    CMD_LINF,      CMD_LINSIGMA, CMD_LIST,       CMD_LN,      CMD_LN_1_X,
    CMD_LOG,      CMD_LOGF,      CMD_MAN,      CMD_MAT_T,      CMD_MEAN,    CMD_MENU,
    CMD_MOD,      CMD_MVAR,      CMD_FACT,     CMD_NEWMAT,     CMD_NORM,    CMD_NOT,
    CMD_OCTM,     CMD_OFF,       CMD_OLD,      CMD_ON,         CMD_OR,      CMD_PERM,
    CMD_PGMINT,   CMD_PGMSLV,    CMD_PI,       CMD_PIXEL,      CMD_POLAR,   CMD_POSA,
    CMD_PRA,      CMD_PRLCD,     CMD_POFF,     CMD_PROMPT,     CMD_PON,     CMD_PRP,
    CMD_PRSTK,    CMD_PRUSR,     CMD_PRV,      CMD_PRX,        CMD_PRSIGMA, CMD_PSE,
    CMD_PUTM,     CMD_PWRF,      CMD_QUIET,    CMD_RAD,        CMD_RAN,     CMD_RCL,
    CMD_RCL_ADD,  CMD_RCL_SUB,   CMD_RCL_MUL,  CMD_RCL_DIV,    CMD_RCLEL,   CMD_RCLIJ,
    CMD_RDXCOMMA, CMD_RDXDOT,    CMD_REALRES,  CMD_REAL_T,     CMD_RECT,    CMD_RND,
    CMD_RNRM,     CMD_ROTXY,     CMD_RSUM,     CMD_RTN,        CMD_SWAP_R,  CMD_RUP,
    CMD_RDN,      CMD_SCI,       CMD_SDEV,     CMD_SEED,       CMD_SF,      CMD_SIGN,
    CMD_SIN,      CMD_SINH,      CMD_SIZE,     CMD_SLOPE,      CMD_SOLVE,   CMD_SQRT,
    CMD_SST,      CMD_STO,       CMD_STO_ADD,  CMD_STO_SUB,    CMD_STO_MUL, CMD_STO_DIV,
    CMD_STOEL,    CMD_STOIJ,     CMD_STOP,     CMD_STR_T,      CMD_SUM,     CMD_TAN,
    CMD_TANH,     CMD_TONE,      CMD_TRACE,    CMD_TRANS,      CMD_UVEC,    CMD_VARMENU,
    CMD_VIEW,     CMD_WMEAN,     CMD_WRAP,     CMD_X_SWAP,     CMD_SWAP,    CMD_X_LT_0,
    CMD_X_LT_Y,   CMD_X_LE_0,    CMD_X_LE_Y,   CMD_X_EQ_0,     CMD_X_EQ_Y,  CMD_X_NE_0,
    CMD_X_NE_Y,   CMD_X_GT_0,    CMD_X_GT_Y,   CMD_X_GE_0,     CMD_X_GE_Y,  CMD_XEQ,
    CMD_XOR,      CMD_XTOA,      CMD_SQUARE,   CMD_YINT,       CMD_Y_POW_X, CMD_INV,
    CMD_10_POW_X, CMD_ADD,       CMD_SUB,      CMD_MUL,        CMD_DIV,     CMD_CHS,
    CMD_SIGMAADD, CMD_SIGMASUB,  CMD_SIGMAREG, CMD_SIGMAREG_T, CMD_TO_DEC,  CMD_TO_DEG,
    CMD_TO_HMS,   CMD_TO_HR,     CMD_TO_OCT,   CMD_TO_POL,     CMD_TO_RAD,  CMD_TO_REC,
    CMD_LEFT,     CMD_UP,        CMD_DOWN,     CMD_RIGHT,      CMD_PERCENT, CMD_PERCENT_CH,
    CMD_FIND,     CMD_MAX,       CMD_MIN,      CMD_NULL,       CMD_NULL,    CMD_NULL
};

static int ext_time_cat[] = {
    CMD_ADATE,     CMD_ATIME, CMD_ATIME24, CMD_CLK12, CMD_CLK24, CMD_DATE,
    CMD_DATE_PLUS, CMD_DDAYS, CMD_DMY,     CMD_DOW,   CMD_MDY,   CMD_TIME,
    CMD_YMD,       CMD_NULL,  CMD_NULL,    CMD_NULL,  CMD_NULL,  CMD_NULL
};

static int ext_xfcn_cat[] = {
    CMD_ANUM, CMD_RCLFLAG, CMD_STOFLAG, CMD_X_SWAP_F, CMD_NULL, CMD_NULL
};

static int ext_base_cat[] = {
    CMD_BRESET, CMD_BSIGNED, CMD_BWRAP, CMD_WSIZE, CMD_WSIZE_T, CMD_A_THRU_F_2
};

static int ext_prgm_cat[] = {
    CMD_CPXMAT_T, CMD_ERRMSG,  CMD_ERRNO,   CMD_FUNC,    CMD_GETKEY1, CMD_LSTO,
    CMD_LASTO,    CMD_LCLV,    CMD_NOP,     CMD_PGMMENU, CMD_PGMVAR,  CMD_RTNERR,
    CMD_RTNNO,    CMD_RTNYES,  CMD_SKIP,    CMD_SST_UP,  CMD_SST_RT,  CMD_TYPE_T,
    CMD_VARMNU1,  -2 /* 0? */, -3 /* X? */, CMD_NULL,    CMD_NULL,    CMD_NULL
};

static int ext_str_cat[] = {
    CMD_APPEND,    CMD_C_TO_N,  CMD_EXTEND, CMD_HEAD,    CMD_LENGTH, CMD_TO_LIST,
    CMD_FROM_LIST, CMD_LIST_T,  CMD_LXASTO, CMD_NEWLIST, CMD_NEWSTR, CMD_N_TO_C,
    CMD_N_TO_S,    CMD_NN_TO_S, CMD_POS,    CMD_REV,     CMD_SUBSTR, CMD_S_TO_N,
    CMD_XASTO,     CMD_XSTR,    CMD_XVIEW,  CMD_NULL,    CMD_NULL,   CMD_NULL
};

static int ext_stk_cat[] = {
    CMD_4STK,   CMD_DEPTH, CMD_DROP, CMD_DROPN, CMD_DUP,  CMD_DUPN,
    CMD_L4STK,  CMD_LNSTK, CMD_NSTK, CMD_PICK,  CMD_RUPN, CMD_RDNN,
    CMD_UNPICK, CMD_NULL,  CMD_NULL, CMD_NULL,  CMD_NULL, CMD_NULL
};

#if defined(ANDROID) || defined(IPHONE)
#ifdef FREE42_FPTEST
static int ext_misc_cat[] = {
    CMD_A2LINE,  CMD_A2PLINE, CMD_CAPS,    CMD_FMA,    CMD_HEIGHT, CMD_MIXED,
    CMD_PCOMPLX, CMD_PRREG,   CMD_RCOMPLX, CMD_STRACE, CMD_WIDTH,  CMD_X2LINE,
    CMD_ACCEL,   CMD_LOCAT,   CMD_HEADING, CMD_FPTEST, CMD_NULL,   CMD_NULL
};
#define MISC_CAT_ROWS 3
#else
static int ext_misc_cat[] = {
    CMD_A2LINE,  CMD_A2PLINE, CMD_CAPS,    CMD_FMA,    CMD_HEIGHT, CMD_MIXED,
    CMD_PCOMPLX, CMD_PRREG,   CMD_RCOMPLX, CMD_STRACE, CMD_WIDTH,  CMD_X2LINE,
    CMD_ACCEL,   CMD_LOCAT,   CMD_HEADING, CMD_NULL,   CMD_NULL,   CMD_NULL
};
#define MISC_CAT_ROWS 3
#endif
#else
#ifdef FREE42_FPTEST
static int ext_misc_cat[] = {
    CMD_A2LINE,  CMD_A2PLINE, CMD_CAPS,    CMD_FMA,    CMD_HEIGHT, CMD_MIXED,
    CMD_PCOMPLX, CMD_PRREG,   CMD_RCOMPLX, CMD_STRACE, CMD_WIDTH,  CMD_X2LINE,
    CMD_FPTEST,  CMD_NULL,    CMD_NULL,    CMD_NULL,   CMD_NULL,   CMD_NULL
};
#define MISC_CAT_ROWS 3
#else
static int ext_misc_cat[] = {
    CMD_A2LINE,  CMD_A2PLINE, CMD_CAPS,    CMD_FMA,    CMD_HEIGHT, CMD_MIXED,
    CMD_PCOMPLX, CMD_PRREG,   CMD_RCOMPLX, CMD_STRACE, CMD_WIDTH,  CMD_X2LINE
};
#define MISC_CAT_ROWS 2
#endif
#endif

static int ext_0_cmp_cat[] = {
    CMD_0_EQ_NN, CMD_0_NE_NN, CMD_0_LT_NN, CMD_0_GT_NN, CMD_0_LE_NN, CMD_0_GE_NN
};

static int ext_x_cmp_cat[] = {
    CMD_X_EQ_NN, CMD_X_NE_NN, CMD_X_LT_NN, CMD_X_GT_NN, CMD_X_LE_NN, CMD_X_GE_NN
};

static void draw_catalog() {
    int catsect = get_cat_section();
    int catindex = get_cat_index();
    if (catsect == CATSECT_TOP) {
        draw_top:
        draw_key(0, 0, 0, "FCN", 3);
        draw_key(1, 0, 0, "PGM", 3);
        draw_key(2, 0, 0, "REAL", 4);
        draw_key(3, 0, 0, "CPX", 3);
        draw_key(4, 0, 0, "MAT", 3);
        draw_key(5, 0, 0, "MEM", 3);
        mode_updown = true;
        shell_annunciators(1, -1, -1, -1, -1, -1);
    } else if (catsect == CATSECT_EXT_1) {
        draw_key(0, 0, 0, "TIME", 4);
        draw_key(1, 0, 0, "XFCN", 4);
        draw_key(2, 0, 0, "BASE", 4);
        draw_key(3, 0, 0, "PRGM", 4);
        draw_key(4, 0, 0, "STR", 3);
        draw_key(5, 0, 0, "STK", 3);
        mode_updown = true;
        shell_annunciators(1, -1, -1, -1, -1, -1);
    } else if (catsect == CATSECT_EXT_2) {
        draw_key(0, 0, 0, "MISC", 4);
        draw_key(1, 0, 0, "", 0);
        draw_key(2, 0, 0, "", 0);
        draw_key(3, 0, 0, "", 0);
        draw_key(4, 0, 0, "", 0);
        draw_key(5, 0, 0, "", 0);
        mode_updown = true;
        shell_annunciators(1, -1, -1, -1, -1, -1);
    } else if (catsect == CATSECT_PGM
            || catsect == CATSECT_PGM_ONLY
            || catsect == CATSECT_PGM_SOLVE
            || catsect == CATSECT_PGM_INTEG
            || catsect == CATSECT_PGM_MENU) {
        /* Show menu of alpha labels */
        int lcount = 0;
        int i, j, k = -1;
        if (catsect == CATSECT_PGM || catsect == CATSECT_PGM_ONLY) {
            int lastprgm = -1;
            for (i = 0; i < labels_count; i++) {
                if (labels[i].length > 0 || labels[i].prgm != lastprgm)
                    lcount++;
                lastprgm = labels[i].prgm;
            }
        } else /* CATSECT_PGM_SOLVE, CATSECT_PGM_INTEG, CATSECT_PGM_MENU */ {
            for (i = 0; i < labels_count; i++)
                if (label_has_mvar(i))
                    lcount++;
        }
        catalogmenu_rows[catindex] = (lcount + 5) / 6;
        if (catalogmenu_row[catindex] >= catalogmenu_rows[catindex])
            catalogmenu_row[catindex] = catalogmenu_rows[catindex] - 1;
        j = -1;
        for (i = labels_count - 1; i >= 0; i--) {
            int show_this_label;
            if (catsect == CATSECT_PGM || catsect == CATSECT_PGM_ONLY) {
                show_this_label = labels[i].length > 0 || i == 0
                                    || labels[i - 1].prgm != labels[i].prgm;
            } else {
                show_this_label = label_has_mvar(i);
            }
            if (show_this_label) {
                j++;
                if (j / 6 == catalogmenu_row[catindex]) {
                    int len = labels[i].length;
                    k = j % 6;
                    if (len == 0) {
                        if (i == labels_count - 1)
                            draw_key(k, 0, 0, ".END.", 5);
                        else
                            draw_key(k, 0, 0, "END", 3);
                    } else
                        draw_key(k, 0, 0, labels[i].name, labels[i].length);
                    catalogmenu_item[catindex][k] = i;
                    if (k == 5)
                        break;
                }
            }
        }
        while (k < 5) {
            draw_key(++k, 0, 0, "", 0);
            catalogmenu_item[catindex][k] = -1;
        }
        mode_updown = catalogmenu_rows[catindex] > 1;
        shell_annunciators(mode_updown, -1, -1, -1, -1, -1);
    } else if (catsect == CATSECT_FCN
            || catsect >= CATSECT_EXT_TIME && catsect <= CATSECT_EXT_X_CMP) {
        int *subcat;
        int subcat_rows;
        switch (catsect) {
            case CATSECT_FCN: subcat = fcn_cat; subcat_rows = 43; break;
            case CATSECT_EXT_TIME: subcat = ext_time_cat; subcat_rows = 3; break;
            case CATSECT_EXT_XFCN: subcat = ext_xfcn_cat; subcat_rows = 1; break;
            case CATSECT_EXT_BASE: subcat = ext_base_cat; subcat_rows = 1; break;
            case CATSECT_EXT_PRGM: subcat = ext_prgm_cat; subcat_rows = 4; break;
            case CATSECT_EXT_STR: subcat = ext_str_cat; subcat_rows = 4; break;
            case CATSECT_EXT_STK: subcat = ext_stk_cat; subcat_rows = 3; break;
            case CATSECT_EXT_MISC: subcat = ext_misc_cat; subcat_rows = MISC_CAT_ROWS; break;
            case CATSECT_EXT_0_CMP: subcat = ext_0_cmp_cat; subcat_rows = 1; break;
            case CATSECT_EXT_X_CMP: subcat = ext_x_cmp_cat; subcat_rows = 1; break;
        }

        int desired_row = catalogmenu_row[catindex];
        if (desired_row >= subcat_rows)
            desired_row = 0;
        for (int i = 0; i < 6; i++) {
            int cmd = subcat[desired_row * 6 + i];
            catalogmenu_item[catindex][i] = cmd;
            if (cmd == -2)
                draw_key(i, 0, 1, "0?", 2);
            else if (cmd == -3)
                draw_key(i, 0, 1, "X?", 2);
            else
                draw_key(i, 0, 1, cmd_array[cmd].name,
                                  cmd_array[cmd].name_length);
        }
        catalogmenu_rows[catindex] = subcat_rows;
        mode_updown = subcat_rows > 1;
        shell_annunciators(mode_updown ? 1 : 0, -1, -1, -1, -1, -1);
    } else {
        int vcount = 0;
        int i, j, k = -1;
        int show_real = 1;
        int show_str = 1;
        int show_cpx = 1;
        int show_mat = 1;
        int show_list = 1;

        switch (catsect) {
            case CATSECT_REAL:
            case CATSECT_REAL_ONLY:
                show_cpx = show_mat = show_list = 0; break;
            case CATSECT_CPX:
                show_real = show_str = show_mat = show_list = 0; break;
            case CATSECT_MAT:
            case CATSECT_MAT_ONLY:
                show_real = show_str = show_cpx = show_list = 0; break;
            case CATSECT_MAT_LIST:
            case CATSECT_MAT_LIST_ONLY:
                show_real = show_str = show_cpx = 0; break;
            case CATSECT_LIST_STR_ONLY:
                show_real = show_cpx = show_mat = 0; break;
        }

        for (i = 0; i < vars_count; i++) {
            int type = vars[i].value->type;
            if ((vars[i].flags & (VAR_HIDDEN | VAR_PRIVATE)) != 0)
                continue;
            switch (type) {
                case TYPE_REAL:
                    if (show_real) vcount++;
                    break;
                case TYPE_STRING:
                    if (show_str) vcount++;
                    break;
                case TYPE_COMPLEX:
                    if (show_cpx) vcount++;
                    break;
                case TYPE_REALMATRIX:
                case TYPE_COMPLEXMATRIX:
                    if (show_mat) vcount++;
                    break;
                case TYPE_LIST:
                    if (show_list) vcount++;
                    break;
            }
        }
        if (vcount == 0) {
            /* We should only get here if the 'plainmenu' catalog is
             * in operation; the other catalogs only operate during
             * command entry mode, or are label catalogs -- so in those
             * cases, it is possible to prevent empty catalogs from
             * being displayed *in advance* (i.e., check if any real
             * variables exist before enabling MENU_CATALOG with
             * catalogmenu_section = CATSECT_REAL, etc.).
             * When a catalog becomes empty while displayed, we move
             * to the top level silently. The "No XXX Variables" message
             * is only displayed if the user actively tries to enter
             * an empty catalog section.
             */
            set_cat_section(CATSECT_TOP);
            goto draw_top;
        }

        catalogmenu_rows[catindex] = (vcount + 5) / 6;
        if (catalogmenu_row[catindex] >= catalogmenu_rows[catindex])
            catalogmenu_row[catindex] = catalogmenu_rows[catindex] - 1;
        j = -1;
        for (i = vars_count - 1; i >= 0; i--) {
            if ((vars[i].flags & (VAR_HIDDEN | VAR_PRIVATE)) != 0)
                continue;
            int type = vars[i].value->type;
            switch (type) {
                case TYPE_REAL:
                    if (show_real) break; else continue;
                case TYPE_STRING:
                    if (show_str) break; else continue;
                case TYPE_COMPLEX:
                    if (show_cpx) break; else continue;
                case TYPE_REALMATRIX:
                case TYPE_COMPLEXMATRIX:
                    if (show_mat) break; else continue;
                case TYPE_LIST:
                    if (show_list) break; else continue;
                default:
                    continue;
            }
            j++;
            if (j / 6 == catalogmenu_row[catindex]) {
                k = j % 6;
                draw_key(k, 0, 0, vars[i].name, vars[i].length);
                catalogmenu_item[catindex][k] = i;
                if (k == 5)
                    break;
            }
        }
        while (k < 5) {
            draw_key(++k, 0, 0, "", 0);
            catalogmenu_item[catindex][k] = -1;
        }
        mode_updown = catalogmenu_rows[catindex] > 1;
        shell_annunciators(mode_updown, -1, -1, -1, -1, -1);
    }
}

void display_mem() {
    uint8 bytes = shell_get_mem();
    char buf[20];
    int buflen;
    clear_display();
    draw_string(0, 0, "Available Memory:", 17);
    buflen = ulong2string(bytes, buf, 20);
    draw_string(0, 1, buf, buflen);
    draw_string(buflen + 1, 1, "Bytes", 5);
    flush_display();
}

static int procrustean_phloat2string(phloat d, char *buf, int buflen) {
    char tbuf[100];
    int tbuflen = phloat2string(d, tbuf, 100, 0, 0, 3,
                                flags.f.thousands_separators, MAX_MANT_DIGITS);
    if (tbuflen <= buflen) {
        memcpy(buf, tbuf, tbuflen);
        return tbuflen;
    }
    if (flags.f.thousands_separators) {
        tbuflen = phloat2string(d, tbuf, 100, 0, 0, 3, 0, MAX_MANT_DIGITS);
        if (tbuflen <= buflen) {
            memcpy(buf, tbuf, tbuflen);
            return tbuflen;
        }
    }
    int epos = 0;
    while (epos < tbuflen && tbuf[epos] != 24)
        epos++;
    if (epos == tbuflen) {
        int dpos = buflen - 2;
        char dec = flags.f.decimal_point ? '.' : ',';
        while (dpos >= 0 && tbuf[dpos] != dec)
            dpos--;
        if (dpos != -1) {
            memcpy(buf, tbuf, buflen - 1);
            buf[buflen - 1] = 26;
            return buflen;
        }
        tbuflen = phloat2string(d, tbuf, 100, 0, MAX_MANT_DIGITS - 1, 1, 0, MAX_MANT_DIGITS);
        epos = 0;
        int zero_since = -1;
        while (epos < tbuflen && tbuf[epos] != 24) {
            if (tbuf[epos] == '0') {
                if (zero_since == -1)
                    zero_since = epos;
            } else {
                zero_since = -1;
            }
            epos++;
        }
        if (zero_since != -1) {
            memmove(tbuf + zero_since, tbuf + epos, tbuflen - epos);
            tbuflen -= epos - zero_since;
            epos = zero_since;
        }
        if (tbuflen <= buflen) {
            memcpy(buf, tbuf, tbuflen);
            return tbuflen;
        }
    }
    int expsize = tbuflen - epos;
    memcpy(buf, tbuf, buflen - expsize - 1);
    buf[buflen - expsize - 1] = 26;
    memcpy(buf + buflen - expsize, tbuf + epos, expsize);
    return buflen;
}

void show() {
    if (flags.f.prgm_mode)
        display_prgm_line(-1, 0);
    else if (alpha_active()) {
        clear_display();
        if (reg_alpha_length <= 22)
            draw_string(0, 0, reg_alpha, reg_alpha_length);
        else {
            draw_string(0, 0, reg_alpha, 22);
            draw_string(0, 1, reg_alpha + 22, reg_alpha_length - 22);
        }
    } else {
        char buf[45];
        int bufptr;
        vartype *rx;
        int x_type;
        if (sp >= 0) {
            rx = stack[sp];
            x_type = rx->type;
        } else {
            x_type = TYPE_NULL;
        }
        clear_display();
        switch (x_type) {
            case TYPE_REAL: {
                bufptr = phloat2string(((vartype_real *) rx)->x, buf, 45,
                                       2, 0, 3,
                                       flags.f.thousands_separators, MAX_MANT_DIGITS);
                if (bufptr == 45)
                    bufptr = phloat2string(((vartype_real *) rx)->x, buf,
                                           44, 2, 0, 3, 0, MAX_MANT_DIGITS);
                show_one_or_two_lines:
                if (bufptr <= 22)
                    draw_string(0, 0, buf, bufptr);
                else {
                    draw_string(0, 0, buf, 22);
                    draw_string(0, 1, buf + 22, bufptr - 22);
                }
                break;
            }
            case TYPE_STRING: {
                vartype_string *s = (vartype_string *) rx;
                bufptr = 0;
                char2buf(buf, 44, &bufptr, '"');
                string2buf(buf, 44, &bufptr, s->txt(), s->length);
                if (bufptr < 44)
                    char2buf(buf, 44, &bufptr, '"');
                goto show_one_or_two_lines;
            }
            case TYPE_COMPLEX: {
                vartype_complex *c = (vartype_complex *) rx;
                phloat x, y;
                if (flags.f.polar) {
                    generic_r2p(c->re, c->im, &x, &y);
                    if (p_isinf(x))
                        x = POS_HUGE_PHLOAT;
                } else {
                    x = c->re;
                    y = c->im;
                }
                bufptr = procrustean_phloat2string(x, buf, 22);
                draw_string(0, 0, buf, bufptr);
                bufptr = procrustean_phloat2string(y, buf, 21);
                if (flags.f.polar) {
                    draw_char(0, 1, 23);
                    draw_string(1, 1, buf, bufptr);
                } else {
                    if (buf[0] == '-') {
                        draw_string(0, 1, "-i", 2);
                        draw_string(2, 1, buf + 1, bufptr - 1);
                    } else {
                        draw_char(0, 1, 'i');
                        draw_string(1, 1, buf, bufptr);
                    }
                }
                break;
            }
            case TYPE_REALMATRIX: {
                vartype_realmatrix *rm = (vartype_realmatrix *) rx;
                phloat *d = rm->array->data;
                bufptr = vartype2string(rx, buf, 22);
                draw_string(0, 0, buf, bufptr);
                draw_string(0, 1, "1:1=", 4);
                bufptr = 0;
                if (rm->array->is_string[0] != 0) {
                    char *text;
                    int4 len;
                    get_matrix_string(rm, 0, &text, &len);
                    char2buf(buf, 18, &bufptr, '"');
                    string2buf(buf, 18, &bufptr, text, len);
                    if (bufptr < 18)
                        char2buf(buf, 18, &bufptr, '"');
                } else {
                    bufptr = phloat2string(*d, buf, 18,
                                           0, 0, 3,
                                           flags.f.thousands_separators);
                }
                draw_string(4, 1, buf, bufptr);
                break;
            }
            case TYPE_COMPLEXMATRIX: {
                vartype_complexmatrix *cm = (vartype_complexmatrix *) rx;
                vartype_complex c;
                bufptr = vartype2string(rx, buf, 22);
                draw_string(0, 0, buf, bufptr);
                draw_string(0, 1, "1:1=", 4);
                c.type = TYPE_COMPLEX;
                c.re = cm->array->data[0];
                c.im = cm->array->data[1];
                bufptr = vartype2string((vartype *) &c, buf, 18);
                draw_string(4, 1, buf, bufptr);
                break;
            }
        }
    }
    flush_display();
}

void redisplay() {
    int menu_id;
    int avail_rows = 2;
    int i;

    if (mode_clall) {
        clear_display();
        draw_string(0, 0, "Clear All Memory?", 17);
        draw_key(0, 0, 0, "YES", 3);
        draw_key(1, 0, 0, "", 0);
        draw_key(2, 0, 0, "", 0);
        draw_key(3, 0, 0, "", 0);
        draw_key(4, 0, 0, "", 0);
        draw_key(5, 0, 0, "NO", 2);
        flush_display();
        return;
    }

    if (flags.f.two_line_message)
        return;
    if (flags.f.message)
        clear_row(1);
    else
        clear_display();

    if (mode_commandmenu != MENU_NONE)
        menu_id = mode_commandmenu;
    else if (mode_alphamenu != MENU_NONE)
        menu_id = mode_alphamenu;
    else if (mode_transientmenu != MENU_NONE)
        menu_id = mode_transientmenu;
    else if (mode_plainmenu != MENU_NONE)
        menu_id = mode_plainmenu;
    else if (mode_appmenu != MENU_NONE)
        menu_id = mode_appmenu;
    else
        menu_id = MENU_NONE;
    if (menu_id == MENU_CATALOG) {
        draw_catalog();
        avail_rows = 1;
    } else if (menu_id == MENU_VARMENU) {
        draw_varmenu();
        if (varmenu_length == 0) {
            redisplay();
            return;
        }
        avail_rows = 1;
    } else if (menu_id == MENU_CUSTOM1 || menu_id == MENU_CUSTOM2
            || menu_id == MENU_CUSTOM3) {
        int r = menu_id - MENU_CUSTOM1;
        if (flags.f.local_label
                && !(mode_command_entry && incomplete_argtype == ARG_CKEY)) {
            for (i = 0; i < 5; i++) {
                char c = (r == 0 ? 'A' : 'F') + i;
                draw_key(i, 0, 0, &c, 1);
            }
            draw_key(5, 0, 0, "XEQ", 3);
        } else {
            for (i = 0; i < 6; i++) {
                draw_key(i, 0, 1, custommenu_label[r][i],
                                  custommenu_length[r][i]);
            }
        }
        avail_rows = 1;
    } else if (menu_id == MENU_PROGRAMMABLE) {
        for (i = 0; i < 6; i++)
            draw_key(i, 0, 0, progmenu_label[i], progmenu_length[i]);
        avail_rows = 1;
    } else if (menu_id != MENU_NONE) {
        const menu_spec *m = menus + menu_id;
        for (i = 0; i < 6; i++) {
            const menu_item_spec *mi = m->child + i;
            if (mi->menuid == MENU_NONE || (mi->menuid & 0x3000) == 0)
                draw_key(i, 0, 0, mi->title, mi->title_length);
            else {
                int cmd_id = mi->menuid & 0xfff;
                const command_spec *cmd = &cmd_array[cmd_id];
                int is_flag = (mi->menuid & 0x2000) != 0;
                if (is_flag) {
                    /* Take a closer look at the command ID and highlight
                     * the menu item if appropriate -- that is, clear 'is_flag'
                     * if highlighting is *not* appropriate
                     */
                    switch (cmd_id) {
                        case CMD_FIX:
                            is_flag = flags.f.fix_or_all
                                        && !flags.f.eng_or_all;
                            break;
                        case CMD_SCI:
                            is_flag = !flags.f.fix_or_all
                                        && !flags.f.eng_or_all;
                            break;
                        case CMD_ENG:
                            is_flag = !flags.f.fix_or_all
                                        && flags.f.eng_or_all;
                            break;
                        case CMD_ALL:
                            is_flag = flags.f.fix_or_all
                                        && flags.f.eng_or_all;
                            break;
                        case CMD_RDXDOT:
                            is_flag = flags.f.decimal_point;
                            break;
                        case CMD_RDXCOMMA:
                            is_flag = !flags.f.decimal_point;
                            break;
                        case CMD_DEG:
                            is_flag = !flags.f.rad && !flags.f.grad;
                            break;
                        case CMD_RAD:
                            is_flag = flags.f.rad;
                            break;
                        case CMD_GRAD:
                            is_flag = !flags.f.rad && flags.f.grad;
                            break;
                        case CMD_POLAR:
                            is_flag = flags.f.polar;
                            break;
                        case CMD_RECT:
                            is_flag = !flags.f.polar;
                            break;
                        case CMD_QUIET:
                            is_flag = !flags.f.audio_enable;
                            break;
                        case CMD_CPXRES:
                            is_flag = !flags.f.real_result_only;
                            break;
                        case CMD_REALRES:
                            is_flag = flags.f.real_result_only;
                            break;
                        case CMD_KEYASN:
                            is_flag = !flags.f.local_label;
                            break;
                        case CMD_LCLBL:
                            is_flag = flags.f.local_label;
                            break;
                        case CMD_BSIGNED:
                            is_flag = flags.f.base_signed;
                            break;
                        case CMD_BWRAP:
                            is_flag = flags.f.base_wrap;
                            break;
                        case CMD_MDY:
                            is_flag = !flags.f.ymd && !flags.f.dmy;
                            break;
                        case CMD_DMY:
                            is_flag = !flags.f.ymd && flags.f.dmy;
                            break;
                        case CMD_YMD:
                            is_flag = flags.f.ymd;
                            break;
                        case CMD_CLK12:
                            is_flag = !mode_time_clk24;
                            break;
                        case CMD_CLK24:
                            is_flag = mode_time_clk24;
                            break;
                        case CMD_4STK:
                            is_flag = !flags.f.big_stack;
                            break;
                        case CMD_NSTK:
                            is_flag = flags.f.big_stack;
                            break;
                        case CMD_CAPS:
                            is_flag = mode_menu_caps;
                            break;
                        case CMD_MIXED:
                            is_flag = !mode_menu_caps;
                            break;
                        case CMD_PON:
                            is_flag = flags.f.printer_exists;
                            break;
                        case CMD_POFF:
                            is_flag = !flags.f.printer_exists;
                            break;
                        case CMD_MAN:
                            is_flag = !flags.f.trace_print
                                        && !flags.f.normal_print;
                            break;
                        case CMD_NORM:
                            is_flag = !flags.f.trace_print
                                        && flags.f.normal_print;
                            break;
                        case CMD_TRACE:
                            is_flag = flags.f.trace_print
                                        && !flags.f.normal_print;
                            break;
                        case CMD_STRACE:
                            is_flag = flags.f.trace_print
                                        && flags.f.normal_print;
                            break;
                        case CMD_ALLSIGMA:
                            is_flag = flags.f.all_sigma;
                            break;
                        case CMD_LINSIGMA:
                            is_flag = !flags.f.all_sigma;
                            break;
                        case CMD_LINF:
                            is_flag = flags.f.lin_fit;
                            break;
                        case CMD_LOGF:
                            is_flag = flags.f.log_fit;
                            break;
                        case CMD_EXPF:
                            is_flag = flags.f.exp_fit;
                            break;
                        case CMD_PWRF:
                            is_flag = flags.f.pwr_fit;
                            break;
                        case CMD_WRAP:
                            is_flag = !flags.f.grow;
                            break;
                        case CMD_GROW:
                            is_flag = flags.f.grow;
                            break;
                        case CMD_BINM:
                            is_flag = get_base() == 2;
                            break;
                        case CMD_OCTM:
                            is_flag = get_base() == 8;
                            break;
                        case CMD_DECM:
                            is_flag = get_base() == 10;
                            break;
                        case CMD_HEXM:
                            is_flag = get_base() == 16;
                            break;
                    }
                }
                draw_key(i, is_flag, 1, cmd->name, cmd->name_length);
            }
        }
        avail_rows = 1;
    }

    if (!flags.f.prgm_mode &&
            (mode_command_entry || pending_command != CMD_NONE)) {
        int cmd_row;
        if (menu_id == MENU_NONE) {
            cmd_row = 1;
            avail_rows = 1;
        } else {
            cmd_row = 0;
            avail_rows = 0;
        }
        if (mode_command_entry)
            display_incomplete_command(cmd_row);
        else
            display_command(cmd_row);
    }

    if (!alpha_active() && !flags.f.prgm_mode) {
        if (avail_rows == 1) {
            if (!flags.f.message)
                display_x(0);
        } else if (avail_rows == 2) {
            if (!flags.f.message)
                display_y(0);
            display_x(1);
        }
    } else if (flags.f.prgm_mode && avail_rows != 0) {
        if (mode_command_entry) {
            if (avail_rows == 1)
                display_incomplete_command(0);
            else {
                display_prgm_line(0, -1);
                display_incomplete_command(1);
            }
        } else {
            if (avail_rows == 1) {
                if (!flags.f.message)
                    display_prgm_line(0, 0);
            } else if (avail_rows == 2) {
                if (!flags.f.message) {
                    if (pc == -1)
                        prgm_highlight_row = 0;
                    else {
                        if (prgms[current_prgm].text[pc] == CMD_END)
                            prgm_highlight_row = 1;
                    }
                    if (prgm_highlight_row == 0) {
                        display_prgm_line(0, 0);
                        display_prgm_line(1, 1);
                    } else {
                        display_prgm_line(0, -1);
                        display_prgm_line(1, 0);
                    }
                } else
                    display_prgm_line(1, 0);
            }
        }
    } else if (alpha_active() && avail_rows != 0 && !flags.f.message) {
        int avail = mode_alpha_entry ? 21 : 22;
        if (reg_alpha_length <= avail) {
            draw_string(0, 0, reg_alpha, reg_alpha_length);
            if (mode_alpha_entry)
                draw_char(reg_alpha_length, 0, '_');
        } else {
            avail--;
            draw_char(0, 0, 26);
            draw_string(1, 0, reg_alpha + reg_alpha_length - avail, avail);
            if (mode_alpha_entry)
                draw_char(21, 0, '_');
        }
    }

    flush_display();
}

void print_display() {
    shell_print(NULL, 0, display, 17, 0, 0, 131, 16);
}

struct prp_data_struct {
    char buf[100];
    int len;
    int saved_prgm;
    int cmd;
    arg_struct arg;
    int4 line;
    int4 pc;
    int4 lines;
    int width;
    int first;
    bool trace;
    bool normal;
    bool full_xstr;
};

static prp_data_struct *prp_data;
static int print_program_worker(bool interrupted);

int print_program(int prgm_index, int4 pc, int4 lines, bool normal) {
    prp_data_struct *dat = (prp_data_struct *) malloc(sizeof(prp_data_struct));
    if (dat == NULL)
        return ERR_INSUFFICIENT_MEMORY;

    shell_annunciators(-1, -1, 1, -1, -1, -1);
    dat->len = 0;
    dat->saved_prgm = current_prgm;
    dat->cmd = CMD_NONE;
    dat->line = pc2line(pc);
    dat->pc = pc;
    dat->lines = lines;
    dat->width = flags.f.double_wide_print ? 12 : 24;
    dat->first = 1;
    if (normal) {
        dat->trace = false;
        dat->normal = true;
        dat->full_xstr = false;
    } else {
        dat->trace = flags.f.trace_print;
        dat->normal = flags.f.normal_print;
        dat->full_xstr = true;
    }

    current_prgm = prgm_index;
    prp_data = dat;

    if (normal) {
        /* Printing just one line for NORM and TRACE mode;
         * we don't do the 'interruptible' thing in this case.
         */
        int err;
        while ((err = print_program_worker(false)) == ERR_INTERRUPTIBLE);
        return err;
    } else {
        print_text(NULL, 0, true);
        mode_interruptible = print_program_worker;
        mode_stoppable = true;
        return ERR_INTERRUPTIBLE;
    }
}

static int print_program_worker(bool interrupted) {
    prp_data_struct *dat = prp_data;
    int printed = 0;

    if (interrupted)
        goto done;

    do {
        const char *orig_num;
        if (dat->line == 0)
            dat->pc = 0;
        else
            get_next_command(&dat->pc, &dat->cmd, &dat->arg, 0, &orig_num);

        char *xstr = NULL;
        if (dat->trace) {
            if (dat->cmd == CMD_LBL || dat->first) {
                if (dat->len > 0) {
                    print_lines(dat->buf, dat->len, true);
                    printed = 1;
                }
                if (!dat->first)
                    print_text(NULL, 0, true);
                dat->first = 0;
                dat->buf[0] = ' ';
                dat->len = 1 + prgmline2buf(dat->buf + 1, 100 - 1, dat->line,
                                            dat->cmd == CMD_LBL, dat->cmd,
                                            &dat->arg, orig_num);
                if (dat->cmd == CMD_LBL || dat->cmd == CMD_END
                        || dat->lines == 1) {
                    print_lines(dat->buf, dat->len, true);
                    printed = 1;
                    dat->len = 0;
                }
            } else {
                int i, len2;
                if (dat->len > 0) {
                    dat->buf[dat->len++] = ' ';
                    dat->buf[dat->len++] = ' ';
                }
                len2 = prgmline2buf(dat->buf + dat->len, 100 - dat->len, -1, 0,
                                    dat->cmd, &dat->arg, orig_num,
                                    false, true, dat->full_xstr ? &xstr : NULL);
                if (dat->len > 0 && dat->len + len2 > dat->width) {
                    /* Break line before current instruction */
                    print_lines(dat->buf, dat->len - 2, true);
                    printed = 1;
                    if (xstr == NULL) {
                        for (i = 0; i < len2; i++)
                            dat->buf[i] = dat->buf[dat->len + i];
                        dat->len = len2;
                    } else {
                        goto print_xstr;
                    }
                } else if (xstr != NULL) {
                    print_xstr:
                    int plen = (len2 / dat->width) * dat->width;
                    print_lines(xstr, plen, true);
                    memcpy(dat->buf, xstr + plen, len2 - plen);
                    dat->len = len2 - plen;
                } else
                    dat->len += len2;
                if (dat->lines == 1 || dat->cmd == CMD_END) {
                    print_lines(dat->buf, dat->len, true);
                    printed = 1;
                } else if (dat->len >= dat->width) {
                    len2 = (dat->len / dat->width) * dat->width;
                    print_lines(dat->buf, len2, true);
                    printed = 1;
                    for (i = len2; i < dat->len; i++)
                        dat->buf[i - len2] = dat->buf[i];
                    dat->len -= len2;
                }
            }
        } else {
            dat->len = prgmline2buf(dat->buf, 100, dat->line,
                                    dat->cmd == CMD_LBL, dat->cmd, &dat->arg,
                                    orig_num, false, true,
                                    dat->full_xstr ? &xstr : NULL);
            char *buf2 = xstr == NULL ? dat->buf : xstr;
            if (dat->normal) {
                /* In normal mode, programs are printed right-justified;
                 * we pad the instuctions to a minimum of 8 characters so
                 * the listing won't look too ragged.
                 * First, find the beginning of the instruction -- it starts
                 * right after the first space or 'goose' (6) character.
                 */
                int p = 0;
                while (buf2[p] != ' ' && buf2[p] != 6)
                    p++;
                while (dat->len < p + 9)
                    buf2[dat->len++] = ' ';
                /* Insert blank line above LBLs */
                if (dat->cmd == CMD_LBL && !dat->first)
                    print_text(NULL, 0, true);
                dat->first = 0;
            }
            print_lines(buf2, dat->len, !dat->normal);
            printed = 1;
        }
        free(xstr);
        dat->line++;
        dat->lines--;

    } while (!printed);

    if (dat->lines != 0 && dat->cmd != CMD_END)
        return ERR_INTERRUPTIBLE;

    done:
    current_prgm = dat->saved_prgm;
    free(dat);
    shell_annunciators(-1, -1, 0, -1, -1, -1);
    return ERR_STOP;
}

void print_program_line(int prgm_index, int4 pc) {
    print_program(prgm_index, pc, 1, true);
}

int command2buf(char *buf, int len, int cmd, const arg_struct *arg) {
    int bufptr = 0;

    int4 xrom_arg;
    if ((cmd_array[cmd].code1 & 0xf8) == 0xa0 && (cmd_array[cmd].flags & FLAG_HIDDEN) != 0) {
        xrom_arg = (cmd_array[cmd].code1 << 8) | cmd_array[cmd].code2;
        cmd = CMD_XROM;
    } else if (cmd == CMD_XROM) {
        if (arg->type == ARGTYPE_NUM) {
            xrom_arg = arg->val.num;
        } else {
            string2buf(buf, len, &bufptr, "XROM 0x", 7);
            for (int i = 0; i < arg->length; i++) {
                char2buf(buf, len, &bufptr, "0123456789abcdef"[(arg->val.text[i] >> 4) & 15]);
                char2buf(buf, len, &bufptr, "0123456789abcdef"[arg->val.text[i] & 15]);
            }
            return bufptr;
        }
    }

    const command_spec *cmdspec = &cmd_array[cmd];
    if (cmd >= CMD_ASGN01 && cmd <= CMD_ASGN18) {
        string2buf(buf, len, &bufptr, "ASSIGN ", 7);
    } else {
        for (int i = 0; i < cmdspec->name_length; i++) {
            int c = (unsigned char) cmdspec->name[i];
            if (c >= 130 && c != 138)
                c &= 127;
            char2buf(buf, len, &bufptr, c);
        }
    }

    if (cmd == CMD_XROM) {
        int n = xrom_arg & 0x7FF;
        int rom = n >> 6;
        int instr = n & 63;
        char2buf(buf, len, &bufptr, ' ');
        char2buf(buf, len, &bufptr, '0' + rom / 10);
        char2buf(buf, len, &bufptr, '0' + rom % 10);
        char2buf(buf, len, &bufptr, ',');
        char2buf(buf, len, &bufptr, '0' + instr / 10);
        char2buf(buf, len, &bufptr, '0' + instr % 10);
    } else if (cmdspec->argtype != ARG_NONE) {
        if (cmdspec->name_length > 0)
            char2buf(buf, len, &bufptr, ' ');
        if (arg->type == ARGTYPE_IND_NUM
                || arg->type == ARGTYPE_IND_STK
                || arg->type == ARGTYPE_IND_STR)
            string2buf(buf, len, &bufptr, "IND ", 4);
        if (arg->type == ARGTYPE_NUM
                || arg->type == ARGTYPE_IND_NUM) {
            int digits = arg->type == ARGTYPE_IND_NUM ? 2
                        : cmdspec->argtype == ARG_NUM9 ? 1
                        : 2;
            int d = 1, i;
            for (i = 0; i < digits - 1; i++)
                d *= 10;
            while (arg->val.num >= d * 10) {
                d *= 10;
                digits++;
            }
            for (i = 0; i < digits; i++) {
                char2buf(buf, len, &bufptr,
                                (char) ('0' + (arg->val.num / d) % 10));
                d /= 10;
            }
        } else if (arg->type == ARGTYPE_STK
                || arg->type == ARGTYPE_IND_STK) {
            string2buf(buf, len, &bufptr, "ST ", 3);
            char2buf(buf, len, &bufptr, arg->val.stk);
        } else if (arg->type == ARGTYPE_STR
                || arg->type == ARGTYPE_IND_STR) {
            char2buf(buf, len, &bufptr, '"');
            string2buf(buf, len, &bufptr, arg->val.text,
                                            arg->length);
            char2buf(buf, len, &bufptr, '"');
        } else if (arg->type == ARGTYPE_LCLBL) {
            char2buf(buf, len, &bufptr, arg->val.lclbl);
        } else if (arg->type == ARGTYPE_LBLINDEX) {
            label_struct *lbl = labels + arg->val.num;
            if (lbl->length == 0) {
                if (arg->val.num == labels_count - 1)
                    string2buf(buf, len, &bufptr, ".END.", 5);
                else
                    string2buf(buf, len, &bufptr, "END", 3);
            } else {
                char2buf(buf, len, &bufptr, '"');
                string2buf(buf, len, &bufptr, lbl->name, lbl->length);
                char2buf(buf, len, &bufptr, '"');
            }
        } else if (arg->type == ARGTYPE_XSTR) {
            char2buf(buf, len, &bufptr, '"');
            string2buf(buf, len, &bufptr, arg->val.xstr,
                                            arg->length);
            char2buf(buf, len, &bufptr, '"');
        } else /* ARGTYPE_COMMAND; for backward compatibility only */ {
            const command_spec *cs = &cmd_array[arg->val.cmd];
            char2buf(buf, len, &bufptr, '"');
            string2buf(buf, len, &bufptr, cs->name, cs->name_length);
            char2buf(buf, len, &bufptr, '"');
        }
    }
    if (cmd >= CMD_ASGN01 && cmd <= CMD_ASGN18) {
        int keynum = cmd - CMD_ASGN01 + 1;
        string2buf(buf, len, &bufptr, " TO ", 4);
        char2buf(buf, len, &bufptr, (char) ('0' + keynum / 10));
        char2buf(buf, len, &bufptr, (char) ('0' + keynum % 10));
    }

    return bufptr;
}

static int get_cat_index() {
    if (mode_commandmenu != MENU_NONE)
        return 0;
    else if (mode_alphamenu != MENU_NONE)
        return 1;
    else if (mode_transientmenu != MENU_NONE)
        return 2;
    else if (mode_plainmenu != MENU_NONE)
        return 3;
    else if (mode_appmenu != MENU_NONE)
        return 4;
    else
        return -1;
}

void set_menu(int level, int menuid) {
    int err = set_menu_return_err(level, menuid, false);
    if (err != ERR_NONE) {
        display_error(err, true);
        flush_display();
    }
}

int set_menu_return_err(int level, int menuid, bool exitall) {
    int err;

    switch (level) {
        case MENULEVEL_COMMAND:
            mode_commandmenu = menuid;
            goto lbl_03;
        case MENULEVEL_ALPHA:
            mode_alphamenu = menuid;
            goto lbl_02;
        case MENULEVEL_TRANSIENT:
            mode_transientmenu = menuid;
            goto lbl_01;
        case MENULEVEL_PLAIN:
            mode_plainmenu = menuid;
            goto lbl_00;
        case MENULEVEL_APP:
            err = set_appmenu(menuid, exitall);
            if (err != ERR_NONE)
                return err;
    }
            mode_plainmenu = MENU_NONE;
    lbl_00: mode_transientmenu = MENU_NONE;
    lbl_01: mode_alphamenu = MENU_NONE;
    lbl_02: mode_commandmenu = MENU_NONE;
    lbl_03:

    int newmenu = get_front_menu();
    if (newmenu != MENU_NONE) {
        if (newmenu == MENU_CATALOG) {
            int index = get_cat_index();
            mode_updown = index != -1 && catalogmenu_rows[index] > 1;
        } else if (newmenu == MENU_PROGRAMMABLE) {
            /* The programmable menu's up/down annunciator is on if the UP
             * and/or DOWN keys have been assigned to.
             * This is something the original HP-42S doesn't do, but I couldn't
             * resist this little improvement, perfect compatibility be damned.
             * In my defense, the Programming Examples and Techniques book,
             * bottom of page 34, does state that this should work.
             * Can't say whether the fact that it doesn't work on the real
             * HP-42S is a bug, or whether the coders and the documentation
             * writers just had a misunderstanding.
             */
            mode_updown = progmenu_arg[6].type != ARGTYPE_NONE
                        || progmenu_arg[7].type != ARGTYPE_NONE;
        } else {
            /* The up/down annunciator for catalogs depends on how many
             * items they contain; this is handled in draw_catalog().
             */
            mode_updown = newmenu == MENU_VARMENU
                                ? varmenu_rows > 1
                                : menus[newmenu].next != MENU_NONE;
        }
    } else
        mode_updown = false;
    shell_annunciators(mode_updown, -1, -1, -1, -1, -1);
    return ERR_NONE;
}

void set_appmenu_exitcallback(int callback_id) {
    appmenu_exitcallback = callback_id;
}

void set_plainmenu(int menuid) {
    mode_commandmenu = MENU_NONE;
    mode_alphamenu = MENU_NONE;
    mode_transientmenu = MENU_NONE;

    if (menuid == mode_plainmenu) {
        mode_plainmenu_sticky = 1;
        redisplay();
    } else if (menuid == MENU_CUSTOM1
            || menuid == MENU_CUSTOM2
            || menuid == MENU_CUSTOM3) {
        mode_plainmenu = menuid;
        mode_plainmenu_sticky = 1;
        redisplay();
        mode_updown = 1;
        shell_annunciators(1, -1, -1, -1, -1, -1);
    } else {
        /* Even if it's a different menu than the current one, it should
         * still stick if it belongs to the same group.
         */
        if (mode_plainmenu != MENU_NONE) {
            int menu1 = mode_plainmenu;
            int menu2 = menuid;
            int parent;
            while ((parent = menus[menu1].parent) != MENU_NONE)
                menu1 = parent;
            while ((parent = menus[menu2].parent) != MENU_NONE)
                menu2 = parent;
            if (menu1 == menu2)
                mode_plainmenu_sticky = 1;
            else if (menus[menu1].next == MENU_NONE)
                mode_plainmenu_sticky = 0;
            else {
                int m = menu1;
                mode_plainmenu_sticky = 0;
                do {
                    m = menus[m].next;
                    if (m == menu2) {
                        mode_plainmenu_sticky = 1;
                        break;
                    }
                } while (m != menu1);
            }
        } else
            mode_plainmenu_sticky = 0;
        if (!mode_plainmenu_sticky) {
            mode_plainmenu = menuid;
            if (mode_plainmenu == MENU_CATALOG)
                set_cat_section(CATSECT_TOP);
            redisplay();
        }
        mode_updown = mode_plainmenu == MENU_CATALOG
                || mode_plainmenu != MENU_NONE
                        && menus[mode_plainmenu].next != MENU_NONE;
        shell_annunciators(mode_updown, -1, -1, -1, -1, -1);
    }
}

void set_catalog_menu(int section) {
    mode_commandmenu = MENU_CATALOG;
    move_cat_row(0);
    if (section == CATSECT_VARS_ONLY && incomplete_command == CMD_HEAD)
        section = CATSECT_LIST_STR_ONLY;
    set_cat_section(section);
    switch (section) {
        case CATSECT_TOP:
        case CATSECT_FCN:
        case CATSECT_PGM:
        case CATSECT_PGM_ONLY:
        case CATSECT_EXT_1:
        case CATSECT_EXT_TIME:
        case CATSECT_EXT_XFCN:
        case CATSECT_EXT_BASE:
        case CATSECT_EXT_PRGM:
        case CATSECT_EXT_STR:
        case CATSECT_EXT_STK:
        case CATSECT_EXT_2:
        case CATSECT_EXT_MISC:
        case CATSECT_EXT_0_CMP:
        case CATSECT_EXT_X_CMP:
            return;
        case CATSECT_REAL:
        case CATSECT_REAL_ONLY:
            if (!vars_exist(CATSECT_REAL))
                mode_commandmenu = MENU_NONE;
            return;
        case CATSECT_CPX:
            if (!vars_exist(CATSECT_CPX))
                mode_commandmenu = MENU_NONE;
            return;
        case CATSECT_MAT:
        case CATSECT_MAT_ONLY:
            if (!vars_exist(CATSECT_MAT))
                mode_commandmenu = MENU_NONE;
            return;
        case CATSECT_MAT_LIST:
        case CATSECT_MAT_LIST_ONLY:
            if (!vars_exist(CATSECT_MAT_LIST))
                mode_commandmenu = MENU_NONE;
            return;
        case CATSECT_LIST_STR_ONLY:
            if (!vars_exist(CATSECT_LIST_STR_ONLY))
                mode_commandmenu = MENU_NONE;
            return;
        case CATSECT_VARS_ONLY:
            if (!vars_exist(-1))
                mode_commandmenu = MENU_NONE;
            return;
        case CATSECT_PGM_SOLVE:
        case CATSECT_PGM_INTEG:
        case CATSECT_PGM_MENU:
        default:
            mode_commandmenu = MENU_NONE;
            return;
    }
}

int get_front_menu() {
    if (mode_commandmenu != MENU_NONE)
        return mode_commandmenu;
    if (mode_alphamenu != MENU_NONE)
        return mode_alphamenu;
    if (mode_transientmenu != MENU_NONE)
        return mode_transientmenu;
    if (mode_plainmenu != MENU_NONE)
        return mode_plainmenu;
    return mode_appmenu;
}

void set_cat_section(int section) {
    int index = get_cat_index();
    if (index != -1)
        catalogmenu_section[index] = section;
}

int get_cat_section() {
    int index = get_cat_index();
    if (index != -1)
        return catalogmenu_section[index];
    else
        return CATSECT_TOP;
}

void move_cat_row(int direction) {
    int index = get_cat_index();
    if (index == -1)
        return;
    if (direction == 0)
        catalogmenu_row[index] = 0;
    else if (direction == -1) {
        catalogmenu_row[index]--;
        if (catalogmenu_row[index] < 0)
            catalogmenu_row[index] = catalogmenu_rows[index] - 1;
    } else {
        catalogmenu_row[index]++;
        if (catalogmenu_row[index] >= catalogmenu_rows[index])
            catalogmenu_row[index] = 0;
    }
}

void set_cat_row(int row) {
    int index = get_cat_index();
    if (index == -1)
        return;
    catalogmenu_row[index] = row;
}

int get_cat_row() {
    int index = get_cat_index();
    if (index == -1)
        return 0;
    else
        return catalogmenu_row[index];
}

int get_cat_item(int menukey) {
    int index = get_cat_index();
    if (index == -1)
        return -1;
    else
        return catalogmenu_item[index][menukey];
}

void update_catalog() {
    int *the_menu;
    if (mode_commandmenu != MENU_NONE)
        the_menu = &mode_commandmenu;
    else if (mode_alphamenu != MENU_NONE)
        the_menu = &mode_alphamenu;
    else if (mode_transientmenu != MENU_NONE)
        the_menu = &mode_transientmenu;
    else if (mode_plainmenu != MENU_NONE)
        the_menu = &mode_plainmenu;
    else if (mode_appmenu != MENU_NONE)
        the_menu = &mode_appmenu;
    else
        return;
    if (*the_menu != MENU_CATALOG)
        return;
    switch (get_cat_section()) {
        case CATSECT_TOP:
        case CATSECT_FCN:
        case CATSECT_EXT_1:
        case CATSECT_EXT_TIME:
        case CATSECT_EXT_XFCN:
        case CATSECT_EXT_BASE:
        case CATSECT_EXT_PRGM:
        case CATSECT_EXT_STR:
        case CATSECT_EXT_STK:
        case CATSECT_EXT_2:
        case CATSECT_EXT_MISC:
        case CATSECT_EXT_0_CMP:
        case CATSECT_EXT_X_CMP:
            return;
        case CATSECT_PGM:
        case CATSECT_PGM_ONLY:
            break;
        case CATSECT_REAL:
            if (!vars_exist(CATSECT_REAL))
                set_cat_section(CATSECT_TOP);
            break;
        case CATSECT_CPX:
            if (!vars_exist(CATSECT_CPX))
                set_cat_section(CATSECT_TOP);
            break;
        case CATSECT_MAT:
            if (!vars_exist(CATSECT_MAT))
                set_cat_section(CATSECT_TOP);
            break;
        case CATSECT_MAT_LIST:
            if (!vars_exist(CATSECT_MAT_LIST))
                set_cat_section(CATSECT_TOP);
            break;
        case CATSECT_REAL_ONLY:
            if (!vars_exist(CATSECT_REAL)) {
                *the_menu = MENU_NONE;
                redisplay();
                return;
            }
            break;
        case CATSECT_MAT_ONLY:
            if (!vars_exist(CATSECT_MAT)) {
                *the_menu = MENU_NONE;
                redisplay();
                return;
            }
            break;
        case CATSECT_MAT_LIST_ONLY:
            if (!vars_exist(CATSECT_MAT_LIST)) {
                *the_menu = MENU_NONE;
                redisplay();
                return;
            }
            break;
        case CATSECT_LIST_STR_ONLY:
            if (!vars_exist(CATSECT_LIST_STR_ONLY)) {
                *the_menu = MENU_NONE;
                redisplay();
                return;
            }
            break;
        case CATSECT_VARS_ONLY:
            if (!vars_exist(-1)) {
                *the_menu = MENU_NONE;
                redisplay();
                return;
            }
            break;
        case CATSECT_PGM_SOLVE:
        case CATSECT_PGM_INTEG:
        case CATSECT_PGM_MENU:
            if (!mvar_prgms_exist()) {
                *the_menu = MENU_NONE;
                display_error(ERR_NO_MENU_VARIABLES, false);
                redisplay();
                return;
            }
            break;
    }
    draw_catalog();
}

void clear_custom_menu() {
    int row, key;
    for (row = 0; row < 3; row++)
        for (key = 0; key < 6; key++)
            custommenu_length[row][key] = 0;
}

void assign_custom_key(int keynum, const char *name, int length) {
    int row = (keynum - 1) / 6;
    int key = (keynum - 1) % 6;
    int i;
    custommenu_length[row][key] = length;
    for (i = 0; i < length; i++)
        custommenu_label[row][key][i] = name[i];
}

void get_custom_key(int keynum, char *name, int *length) {
    int row = (keynum - 1) / 6;
    int key = (keynum - 1) % 6;
    string_copy(name, length, custommenu_label[row][key],
                              custommenu_length[row][key]);
}

void clear_prgm_menu() {
    int i;
    for (i = 0; i < 9; i++)
        progmenu_arg[i].type = ARGTYPE_NONE;
    for (i = 0; i < 6; i++)
        progmenu_length[i] = 0;
}

void assign_prgm_key(int keynum, bool is_gto, const arg_struct *arg) {
    int length, i;
    keynum--;
    progmenu_arg[keynum] = (arg_struct) *arg;
    progmenu_is_gto[keynum] = is_gto;
    length = reg_alpha_length;
    if (keynum < 6) {
        if (length > 7)
            length = 7;
        for (i = 0; i < length; i++)
            progmenu_label[keynum][i] = reg_alpha[i];
        progmenu_length[keynum] = length;
    }
}

void do_prgm_menu_key(int keynum) {
    int err, oldprgm;
    int4 oldpc;
    keynum--;
    if (keynum == 8)
        set_menu(MENULEVEL_PLAIN, MENU_NONE);
    if (progmenu_arg[keynum].type == ARGTYPE_NONE) {
        if (keynum < 6)
            pending_command = CMD_NULL;
        else if (keynum == 8)
            pending_command = CMD_CANCELLED;
        return;
    }
    if ((flags.f.trace_print || flags.f.normal_print) && flags.f.printer_exists)
        print_command(progmenu_is_gto[keynum] ? CMD_GTO : CMD_XEQ,
                                                &progmenu_arg[keynum]);
    oldprgm = current_prgm;
    oldpc = pc;
    set_running(true);
    progmenu_arg[keynum].target = -1; /* Force docmd_gto() to search */
    err = docmd_gto(&progmenu_arg[keynum]);
    if (err != ERR_NONE) {
        set_running(false);
        display_error(err, true);
        flush_display();
        return;
    }
    if (!progmenu_is_gto[keynum]) {
        err = push_rtn_addr(oldprgm, oldpc == -1 ? 0 : oldpc);
        if (err != ERR_NONE) {
            current_prgm = oldprgm;
            pc = oldpc;
            set_running(false);
            display_error(err, true);
            flush_display();
            return;
        }
    }
}
