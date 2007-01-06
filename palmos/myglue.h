/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2007  Thomas Okken
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

#ifndef MYGLUE_H
#define MYGLUE_H 1

#include "free42.h"

BitmapType *MyGlueBmpCreate(Coord width, Coord height, UInt8 depth,
		    ColorTableType *colortableP, UInt16 *error) HELPERS_SECT;
void *MyGlueBmpGetBits(BitmapType *bitmapP) HELPERS_SECT;
Err MyGlueBmpDelete(BitmapType *bitmapP) HELPERS_SECT;

void *MyGlueTblGetItemPtr(const TableType *tableP, Int16 row, Int16 column)
								HELPERS_SECT;

#endif
