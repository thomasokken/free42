/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2016  Thomas Okken
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

#ifndef ANDROID

#include <stdlib.h>

#include "shell_spool.h"
#include "core_main.h"

#endif

int hp2ascii(char *dst, const char *src, int srclen) {
    const char *esc;
    unsigned char c;
    int s, d = 0;
    for (s = 0; s < srclen; s++) {
        c = src[s];
        if (c >= 130 && c != 138)
            c &= 127;
        switch (c) {
            /* NOTE: this code performs the following 12 translations
             * that are not ASCII, but seem to be widely accepted --
             * that is, they looked OK when I tried them in several
             * fonts in Windows and Linux, and in Memo Pad on the Palm:
             *
             *   0: 247 (0367) divide
             *   1: 215 (0327) multiply
             *   8: 191 (0277) upside-down question mark
             *  17: 181 (0265) lowercase mu
             *  18: 163 (0243) sterling
             *  19: 176 (0260) degree
             *  20: 197 (0305) Aring
             *  21: 209 (0321) Ntilde
             *  22: 196 (0304) Aumlaut
             *  25: 198 (0306) AE
             *  28: 214 (0326) Oumlaut
             *  29: 220 (0334) Uumlaut
             *
             * Two additional candidates are these:
             *
             *  26: 133 (0205) ellipsis
             *  31: 149 (0225) bullet
             *
             * I'm not using those last two because support for them is not
             * as good: they exist in Times New Roman and Adobe Courier
             * (tested on Windows and Linux, respectively) and on the Palm,
             * but are missing from Windows Fixedsys (the default Notepad
             * font, so that is a concern!) and X11 lucidatypewriter and
             * fixed.
             * Note that 133 and 149 are both in the 128-159 range, that
             * is, the Ctrl+Meta range, which is unused in many fonts.
             * Eventually, I should probably support several translation
             * modes: raw, pure ASCII (only emit codes 32-126 and 10),
             * non-pure as below, and more aggressive non-pure (using the
             * ellipsis and fatdot codes, and maybe others). Then again,
             * maybe not. :-)
             */
            case  0:   esc = "\367"; break;
            case  1:   esc = "\327"; break;
            case  2:   esc = "\\sqrt"; break;
            case  3:   esc = "\\int"; break;
            case  4:   esc = "\\gray1"; break;
            case  5:   esc = "\\Sigma"; break;
            case  6:   esc = ">"; break;
            case  7:   esc = "\\pi"; break;
            case  8:   esc = "\277"; break;
            case  9:   esc = "<="; break;
            case 11:   esc = ">="; break;
            case 12:   esc = "!="; break;
            case 13:   esc = "\\r"; break;
            case 14:   esc = "v"; break;
            case 15:   esc = "->"; break;
            case 16:   esc = "<-"; break;
            case 17:   esc = "\265"; break;
            case 18:   esc = "\243"; break;
            case 19:   esc = "\260"; break;
            case 20:   esc = "\305"; break;
            case 21:   esc = "\321"; break;
            case 22:   esc = "\304"; break;
            case 23:   esc = "\\angle"; break;
            case 24:   esc = "E"; break;
            case 25:   esc = "\306"; break;
            case 26:   esc = "..."; break;
            case 27:   esc = "\\esc"; break;
            case 28:   esc = "\326"; break;
            case 29:   esc = "\334"; break;
            case 30:   esc = "\\gray2"; break;
            case 31:   esc = "\\bullet"; break;
            case '\\': esc = "\\\\"; break;
            case 127:  esc = "|-"; break;
            case 128:  esc = ":"; break;
            case 129:  esc = "y"; break;
            case 138:  esc = "\\LF"; break;
            default:   dst[d++] = c; continue;
        }
        while (*esc != 0)
            dst[d++] = *esc++;
    }
    return d;
}

#ifndef ANDROID

void shell_spool_txt(const char *text, int length,
                     file_writer writer, file_newliner newliner) {
    if (core_settings.raw_text)
        writer(text, length);
    else {
        char buf[1000];
        int d = hp2ascii(buf, text, length);
        writer(buf, d);
    }
    newliner();
}

typedef struct {
    int codesize;
    int bytecount;
    char buf[255];

    short prefix_table[4096];
    short code_table[4096];
    short hash_next[4096];
    short hash_head[256];

    int maxcode;
    int clear_code;
    int end_code;

    int curr_code_size;
    int prefix;
    int currbyte;
    int bits_needed;
    int initial_clear;
    int really_done;

    int width;
    int height;
} gif_data;

static gif_data *g;


int shell_start_gif(file_writer writer, int width, int provisional_height) {
    char buf[29];
    char *p = buf, c;
    int height = provisional_height;
    int i;

    /* NOTE: the height will be set to the *actual* height once we know
     * what that is, i.e., when shell_finish_gif() is called. We populate
     * it using the maximum height (as set in the preferences dialog) so
     * that even incomplete GIF files will be viewable, just in case the
     * user is impatient and wants to take a peek.
     */


    /* GIF Header */

    *p++ = 'G';
    *p++ = 'I';
    *p++ = 'F';
    *p++ = '8';
    *p++ = '7';
    *p++ = 'a';

    /* Screen descriptor */

    *p++ = width & 255;
    *p++ = width >> 8;
    *p++ = height & 255;
    *p++ = height >> 8;
    *p++ = (char) 0xF0;
    *p++ = 0;
    *p++ = 0;

    /* Global color map */

    *p++ = (char) 255;
    *p++ = (char) 255;
    *p++ = (char) 255;
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;

    /* Image Descriptor */

    *p++ = ',';
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;
    *p++ = width & 255;
    *p++ = width >> 8;
    *p++ = height & 255;
    *p++ = height >> 8;
    *p++ = 0x00;

    /* Write GIF header & descriptors */

    writer(buf, 29);


    /* Initialize GIF encoder */

    if (g == NULL) {
        g = (gif_data *) malloc(sizeof(gif_data));
        if (g == NULL)
            return 0;
    }

    g->codesize = 2;
    g->bytecount = 0;
    g->maxcode = 1 << g->codesize;
    for (i = 0; i < g->maxcode; i++) {
        g->prefix_table[i] = -1;
        g->code_table[i] = i;
        g->hash_next[i] = -1;
    }
    for (i = 0; i < 256; i++)
        g->hash_head[i] = -1;

    g->clear_code = g->maxcode++;
    g->end_code = g->maxcode++;

    g->curr_code_size = g->codesize + 1;
    g->prefix = -1;
    g->currbyte = 0;
    g->bits_needed = 8;
    g->initial_clear = 1;
    g->really_done = 0;

    g->width = width;
    g->height = 0;

    c = g->codesize;
    writer(&c, 1);

    return 1;
}

void shell_spool_gif(const char *bits, int bytesperline,
                     int x, int y, int width, int height,
                     file_writer writer) {
    int v, h;
    g->height += height;

    /* Encode Image Data */

    for (v = y; v < y + height || (v == y && height == 0); v++) {
        int done = v == y && height == 0;
        for (h = 0; h < g->width; h++) {
            int new_code;
            unsigned char hash_code;
            int hash_index;
            int pixel;

            if (g->really_done) {
                new_code = g->end_code;
                goto emit;
            } else if (done) {
                new_code = g->prefix;
                goto emit;
            }

            if (h < width)
                pixel = ((bits[bytesperline * v + (h >> 3)]) >> (h & 7)) & 1;
            else
                pixel = 0;

            /* Look for concat(prefix, pixel) in string table */
            if (g->prefix == -1) {
                g->prefix = pixel;
                goto no_emit;
            }
            
            /* Compute hash code
             * TODO: There's a lot of room for improvement here!
             * I'm getting search percentages of over 30%; looking for
             * something in single digits.
             */
            {
                unsigned long x = (((long) g->prefix) << 20)
                                        + (((long) pixel) << 12);
                unsigned char b1, b2, b3;
                x /= 997;
                b1 = (unsigned char) (x >> 16);
                b2 = (unsigned char) (x >> 8);
                b3 = (unsigned char) x;
                hash_code = b1 ^ b2 ^ b3;
            }
            hash_index = g->hash_head[hash_code];
            while (hash_index != -1) {
                if (g->prefix_table[hash_index] == g->prefix
                         && g->code_table[hash_index] == pixel) {
                    g->prefix = hash_index;
                    goto no_emit;
                }
                hash_index = g->hash_next[hash_index];
            }
            
            /* Not found: */
            if (g->maxcode < 4096) {
                g->prefix_table[g->maxcode] = g->prefix;
                g->code_table[g->maxcode] = pixel;
                g->hash_next[g->maxcode] = g->hash_head[hash_code];
                g->hash_head[hash_code] = g->maxcode;
                g->maxcode++;
            }
            new_code = g->prefix;
            g->prefix = pixel;
            
            emit: {
                int outcode = g->initial_clear ? g->clear_code
                                    : g->really_done ? g->end_code : new_code;
                int bits_available = g->curr_code_size;
                while (bits_available != 0) {
                    int bits_copied = g->bits_needed < bits_available ?
                                g->bits_needed : bits_available;
                    int bits = outcode >> (g->curr_code_size - bits_available);
                    bits &= 255 >> (8 - bits_copied);
                    g->currbyte |= bits << (8 - g->bits_needed);
                    bits_available -= bits_copied;
                    g->bits_needed -= bits_copied;
                    if (g->bits_needed == 0 ||
                                    (bits_available == 0 && g->really_done)) {
                        g->buf[g->bytecount++] = g->currbyte;
                        if (g->bytecount == 255) {
                            char c = g->bytecount;
                            writer(&c, 1);
                            writer(g->buf, g->bytecount);
                            g->bytecount = 0;
                        }
                        if (bits_available == 0 && g->really_done)
                            goto data_done;
                        g->currbyte = 0;
                        g->bits_needed = 8;
                    }
                }

                if (done) {
                    done = 0;
                    g->really_done = 1;
                    goto emit;
                }
                if (g->initial_clear) {
                    g->initial_clear = 0;
                    goto emit;
                } else {
                    if (g->maxcode > (1 << g->curr_code_size)) {
                        g->curr_code_size++;
                    } else if (new_code == g->clear_code) {
                        int i;
                        g->maxcode = (1 << g->codesize) + 2;
                        g->curr_code_size = g->codesize + 1;
                        for (i = 0; i < 256; i++)
                            g->hash_head[i] = -1;
                    } else if (g->maxcode == 4096) {
                        new_code = g->clear_code;
                        goto emit;
                    }
                }
            }

            no_emit:;
        }
    }

    data_done:
    ;
}

void shell_finish_gif(file_seeker seeker, file_writer writer) {
    char c;

    /* Flush the encoder and write any remaining data */

    shell_spool_gif(NULL, 0, 0, 0, 0, 0, writer);

    if (g->bytecount > 0) {
        c = g->bytecount;
        writer(&c, 1);
        writer(g->buf, g->bytecount);
    }
    c = 0;
    writer(&c, 1);

    /* GIF Trailer */

    c = ';';
    writer(&c, 1);

    /* Update the 'height' fields in the header, now that at last
     * we know what the final height is */

    seeker(8);
    c = g->height & 255;
    writer(&c, 1);
    c = g->height >> 8;
    writer(&c, 1);

    seeker(26);
    c = g->height & 255;
    writer(&c, 1);
    c = g->height >> 8;
    writer(&c, 1);

    /* All done! */
}

void shell_spool_exit() {
    if (g != NULL) {
        free(g);
        g = NULL;
    }
}

#endif
