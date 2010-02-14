/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2010  Thomas Okken
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

#ifndef SKIN2PRC_H
#define SKIN2PRC_H 1


#ifdef PALMOS

#include <PalmOS.h>

#define int2 Int16
#define uint2 UInt16
#define int4 Int32
#define uint4 UInt32

#else

#define int2 short
#define uint2 unsigned short
#define int4 int
#define uint4 unsigned int

typedef struct {
    unsigned char index, r, g, b;
} RGBColorType;

#endif


typedef struct {
    int2 x, y;
    uint2 off_bitmap, on_bitmap;
} AnnunciatorSpec;

typedef struct {
    int2 code;
    int2 shifted_code;
    int2 sens_x, sens_y, sens_width, sens_height;
    int2 x, y;
    uint2 up_bitmap, down_bitmap;
} KeySpec;

typedef struct {
    char version[4];
    char name[32];
    int2 density;
    uint2 skin_bitmap;
    int2 display_x, display_y, display_xscale, display_yscale;
    RGBColorType display_bg, display_fg;
    AnnunciatorSpec annunciator[7];
    int2 nkeys, sections;
    KeySpec key[1];
} SkinSpec;

/* Macro information follows SkinSpec; the address of this information may be
 * computed like
 *
 * SkinSpec *ss1 = WHATEVER_IT_IS;
 * SkinSpec *ss2 = (SkinSpec2 *) (ss1->key + ss1->nkeys);
 */

typedef struct {
    int2 code;
    unsigned char macro[32];
} MacroSpec;

typedef struct {
    int2 nmacros;
    MacroSpec macro[1];
} SkinSpec2;

/* Before version 3, the maximum macro length was 16, and the macro array
 * was length 18; these structures help in dealing with such old skins
 */

typedef struct {
    int2 code;
    unsigned char macro[18];
} MacroSpec_V2;

typedef struct {
    int2 nmacros;
    MacroSpec_V2 macro[1];
} SkinSpec2_V2;

/* About skin versions:
 * The original SkinSpec structure did not have the 'version' member; its first
 * member was 'name'. Such skins can be recognized by the fact that the first
 * byte of SkinSpec is nonzero; for skins *with* the 'version' member, the
 * convention if that version[0] must be 0. Version[1] is used to hold the
 * actual version number.
 * Version 0: 1.1.16 Pluggable skin support comes to Free42/PalmOS
 * Version 1: 1.2.5  Sections, Macros
 * Version 2: 1.4.9  Added support for tall (320x450) skins
 * Version 3: 1.4.17 Added CShift support; increased macro length limit to 31.
 */

#endif
