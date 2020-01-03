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
            case  0:   esc = "\303\267"; break;     // division sign
            case  1:   esc = "\303\227"; break;     // multiplication sign
            case  2:   esc = "\342\210\232"; break; // square root sign
            case  3:   esc = "\342\210\253"; break; // integral sign
            case  4:   esc = "\342\226\222"; break; // gray rectangle
            case  5:   esc = "\316\243"; break;     // uppercase sigma
            case  6:   esc = "\342\226\270"; break; // small right-pointing triangle
            case  7:   esc = "\317\200"; break;     // lowercase pi
            case  8:   esc = "\302\277"; break;     // upside-down question mark
            case  9:   esc = "\342\211\244"; break; // less-than-or-equals sign
            case 11:   esc = "\342\211\245"; break; // greater-than-or-equals sign
            case 12:   esc = "\342\211\240"; break; // not-equals sign
            case 13:   esc = "\342\206\265"; break; // down-then-left arrow
            case 14:   esc = "\342\206\223"; break; // downward-pointing arrow
            case 15:   esc = "\342\206\222"; break; // right-pointing arrow
            case 16:   esc = "\342\206\220"; break; // left-pointing arrow
            case 17:   esc = "\316\274"; break;     // lowercase mu
            case 18:   esc = "\302\243"; break;     // pound sterling sign
            case 19:   esc = "\302\260"; break;     // degree symbol
            case 20:   esc = "\303\205"; break;     // uppercase a with ring
            case 21:   esc = "\303\221"; break;     // uppercase n with tilde
            case 22:   esc = "\303\204"; break;     // uppercase a with umlaut
            case 23:   esc = "\342\210\241"; break; // measured angle symbol
            case 24:   esc = "\341\264\207"; break; // small-caps e
            case 25:   esc = "\303\206"; break;     // uppercase ae ligature
            case 26:   esc = "\342\200\246"; break; // ellipsis
            case 27:   esc = "[ESC]"; break;        // EC symbol
            case 28:   esc = "\303\226"; break;     // uppercase o with umlaut
            case 29:   esc = "\303\234"; break;     // uppercase u with umlaut
            case 30:   esc = "\342\226\222"; break; // gray rectangle
            case 31:   esc = "\342\200\242"; break; // bullet
            case 94:   esc = "\342\206\221"; break; // upward-pointing arrow
            case 127:  esc = "\342\224\234"; break; // append sign
            case 128:  esc = ":"; break;            // thin colon
            case 129:  esc = "\312\217"; break;     // small-caps y
            case 138:  esc = "[LF]"; break;         // LF symbol
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
    char buf[1000];
    int d = hp2ascii(buf, text, length);
    writer(buf, d);
    newliner();
}

void shell_spool_bitmap_to_txt(const char *bits, int bytesperline,
                               int x, int y, int width, int height,
                               file_writer writer, file_newliner newliner) {
    for (int v = 0; v < height; v += 2) {
        for (int h = 0; h < width; h += 2) {
            int k = 0;
            for (int vv = 0; vv < 2; vv++) {
                int vvv = v + vv + y;
                if (vvv >= height)
                    break;
                for (int hh = 0; hh < 2; hh++) {
                    int hhh = h + hh + x;
                    if (hhh >= width)
                        break;
                    if ((bits[vvv * bytesperline + (hhh >> 3)] & (1 << (hhh & 7))) != 0)
                        k += 1 << (hh + 2 * vv);
                }
            }
            switch (k) {
                case  0: writer("\302\240", 2); break;
                case  1: writer("\342\226\230", 3); break;
                case  2: writer("\342\226\235", 3); break;
                case  3: writer("\342\226\200", 3); break;
                case  4: writer("\342\226\226", 3); break;
                case  5: writer("\342\226\214", 3); break;
                case  6: writer("\342\226\236", 3); break;
                case  7: writer("\342\226\233", 3); break;
                case  8: writer("\342\226\227", 3); break;
                case  9: writer("\342\226\232", 3); break;
                case 10: writer("\342\226\220", 3); break;
                case 11: writer("\342\226\234", 3); break;
                case 12: writer("\342\226\204", 3); break;
                case 13: writer("\342\226\231", 3); break;
                case 14: writer("\342\226\237", 3); break;
                case 15: writer("\342\226\210", 3); break;
            }
        }
        newliner();
    }
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
