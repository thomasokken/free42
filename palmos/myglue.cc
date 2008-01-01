/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2008  Thomas Okken
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

#define ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS 1
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS 1
#define ALLOW_ACCESS_TO_INTERNALS_OF_TABLES 1

#include "myglue.h"
#include "shell2.h"

BitmapType *MyGlueBmpCreate(Coord width, Coord height, UInt8 depth,
			    ColorTableType *colortableP, UInt16 *error) {
    if (feature_set_3_5_present())
	return BmpCreate(width, height, depth, colortableP, error);
    else {
	int rowbytes = ((width + 15) & ~15) >> 3;
	int size = rowbytes * height;
	BitmapTypeV1 *bm = (BitmapTypeV1 *)
				MemPtrNew(sizeof(BitmapTypeV1) + size);
	bm->width = width;
	bm->height = height;
	bm->rowBytes = rowbytes;
	bm->flags.compressed = 0;
	bm->flags.hasColorTable = 0;
	bm->flags.hasTransparency = 0;
	bm->flags.indirect = 0;
	bm->flags.forScreen = 0;
	bm->flags.directColor = 0;
	bm->flags.indirectColorTable = 0;
	bm->flags.noDither = 0;
	bm->flags.reserved = 0;
	bm->pixelSize = 1;
	bm->version = BitmapVersionOne;
	bm->nextDepthOffset = 0;
	bm->reserved[0] = 0;
	bm->reserved[1] = 0;
	return (BitmapType *) bm;
    }
}

void *MyGlueBmpGetBits(BitmapType *bitmapP) {
    if (feature_set_3_5_present())
	return BmpGetBits(bitmapP);
    else
	/* Note: you'd think I'd simply use BmpGlueGetBits(), but while
	 * testing with PalmOS 3.3 on POSE, I noticed it was lying: it
	 * returned an offset of 10 bytes, while sizeof(BitmapTypeV1) is
	 * 16 bytes. Maybe I'm not constructing the bitmap right (see
	 * MyGlueBmpCreate(), above), but I can't see anything wrong with
	 * it, and PalmOS 3.3 seems perfectly happy to *render* them, too.
	 * So, just this little hack... It seems to do the job, so,
	 * what the heck.
	 */
	return ((char *) bitmapP) + 16;
}

Err MyGlueBmpDelete(BitmapType *bitmapP) {
    if (feature_set_3_5_present())
	return BmpDelete(bitmapP);
    else
	return MemPtrFree(bitmapP);
}

void *MyGlueTblGetItemPtr(const TableType *tableP, Int16 row, Int16 column) {
    if (feature_set_3_5_present())
        return TblGetItemPtr(tableP, row, column);
    else
	return tableP->items[row * tableP->numColumns + column].ptr;
}
