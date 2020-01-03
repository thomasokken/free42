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

#ifndef SHELL_LOADIMAGE_H
#define SHELL_LOADIMAGE_H 1

/* NOTE: This function obtains the image file data using skin_getchar() and
 * skin_rewind(), and it writes the in-memory pixmap using skin_init_image(),
 * skin_putpixels(), and skin_finish_image().
 * These functions should be declared in shell_skin.h.
 */
int shell_loadimage();

#endif
