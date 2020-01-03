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

#ifndef SHELL_SPOOL_H
#define SHELL_SPOOL_H 1

#ifndef ANDROID

#include "free42.h"

typedef void (*file_seeker)(int4 pos);
typedef void (*file_writer)(const char *text, int length);
typedef void (*file_newliner)();

#endif

/* hp2ascii()
 *
 * Converts characters outside the range 32-126 and 10 to their ASCII
 * or extended ASCII equivalents; substitutes escape sequences for characters
 * for which no reasonable equivalents exist, and for all character codes
 * above 130.
 */
int hp2ascii(char *dst, const char *src, int srclen);

#ifndef ANDROID

/* shell_spool_txt()
 *
 * Shell helper that writes plain text to a file, optionally translating
 * HP-42S characters to their closest ASCII equivalents or escape sequences.
 * The actual I/O is all done through callbacks so that this routine is OS
 * independent.
 */
void shell_spool_txt(const char *text, int length,
                     file_writer writer, file_newliner newliner);

/* shell_spool_bitmap_to_txt()
 *
 * Shell helper that writes a bitmap to a text stream, using Unicode 2x2
 * graphics block characters. Used by PRLCD.
 */
void shell_spool_bitmap_to_txt(const char *bits, int bytesperline,
                               int x, int y, int width, int height,
                               file_writer writer, file_newliner newliner);

/* shell_start_gif()
 *
 * Shell helper for writing bitmaps to GIF files.
 * This call writes the GIF header (except for the length, which isn't known
 * yet) and initialized the GIF encoder.
 * Returns 1 on success; 0 if it failed because of a memory allocation
 * failure.
 */
int shell_start_gif(file_writer writer, int width, int provisional_height);

/* shell_spool_gif()
 *
 * Shell helper that writes bitmaps to a GIF file.
 * The actual I/O is all done through callbacks so that this routine is OS
 * independent.
 */
void shell_spool_gif(const char *bits, int bytesperline,
                     int x, int y, int width, int height,
                     file_writer writer);

/* shell_finish_gif()
 *
 * Shell helper for writing bitmaps to GIF files.
 * This call flushes the GIF encoder, and writes the final image height to the
 * GIF header. After this call, the caller should close the output file.
 */
void shell_finish_gif(file_seeker seeker, file_writer writer);

/* shell_spool_exit()
 *
 * Cleans up spooler's private data. Call this just before application exit.
 */
void shell_spool_exit();

#endif

#endif
