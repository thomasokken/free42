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

#include <stdlib.h>
#include <PalmOS.h>
#include <MemGlue.h>

void *malloc(size_t n) {
    if (n == 0)
	n = 1;
    return MemGluePtrNew(n);
}

void *realloc(void *p, size_t n) {
    void *p_new;
    UInt32 n_old, n2;
    if (p == NULL)
	return MemGluePtrNew(n);
    if (MemPtrResize(p, n) == errNone)
	return p;
    n_old = MemPtrSize(p);
    p_new = MemGluePtrNew(n);
    if (p_new == NULL)
	return NULL;
    n2 = n_old < n ? n_old : n;
    MemMove(p_new, p, n2);
    MemPtrFree(p);
    return p_new;
}

void free(void *p) {
    if (p != NULL)
	MemPtrFree(p);
}
