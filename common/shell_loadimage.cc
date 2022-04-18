/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2022  Thomas Okken
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>


#include "shell_skin.h"


struct SkinPixmap {
    unsigned char *pixels;
    SkinColor *cmap;
    int depth;
    int width, height, bytesperline;
};

static SkinPixmap pixmap;

struct GifColorMap {
    SkinColor color[256];
    short index[256];
    GifColorMap() {
        for (int i = 0; i < 256; i++) {
            index[i] = -1;
            color[i].r = color[i].g = color[i].b = 0;
        }
    }
};

static int read_byte(int *n) {
    int c = skin_getchar();
    if (c == EOF)
        return 0;
    *n = c;
    return 1;
}

static int read_short(int *n) {
    int c1, c2;
    c1 = skin_getchar();
    if (c1 == EOF)
        return 0;
    c2 = skin_getchar();
    if (c2 == EOF)
        return 0;
    *n = c1 + (c2 << 8);
    return 1;
}


int shell_loadimage() {
    SkinPixmap *pm = &pixmap;
    GifColorMap *lcmap = NULL;
    GifColorMap gcmap;
    int image_colors = 0;

    int sig;

    int info;
    int background;
    int zero;

    int has_global_cmap;
    int bpp;
    int ncolors;
    int size;

    bool mono;
    bool black_used = false;
    bool white_used = false;

    int i, j, type, res;
    unsigned char *ptr;

    if (!read_byte(&sig) || sig != 'G'
            || !read_byte(&sig) || sig != 'I'
            || !read_byte(&sig) || sig != 'F'
            || !read_byte(&sig) || sig != '8'
            || !read_byte(&sig) || (sig != '7' && sig != '9')
            || !read_byte(&sig) || sig != 'a') {
        fprintf(stderr, "GIF signature not found.\n");
        return 0;
    }

    if (!read_short(&pm->width)
            || !read_short(&pm->height)
            || !read_byte(&info)
            || !read_byte(&background)
            || !read_byte(&zero)
            || zero != 0) {
        fprintf(stderr, "Fatally premature EOF.\n");
        return 0;
    }

    has_global_cmap = (info & 128) != 0;
    bpp = (info & 7) + 1;
    ncolors = 1 << bpp;
    size = 0;

    /* Bits 6..4 of info contain one less than the "color resolution",
     * defined as the number of significant bits per RGB component in
     * the source image's color palette. If the source image (from
     * which the GIF was generated) was 24-bit true color, the color
     * resolution is 8, so the value in bits 6..4 is 7. If the source
     * image had an EGA color cube (2x2x2), the color resolution would
     * be 2, etc.
     * Bit 3 of info must be zero in GIF87a; in GIF89a, if it is set,
     * it indicates that the global colormap is sorted, the most
     * important entries being first. In PseudoColor environments this
     * can be used to make sure to get the most important colors from
     * the X server first, to optimize the image's appearance in the
     * event that not all the colors from the colormap can actually be
     * obtained at the same time.
     * The 'zero' field is always 0 in GIF87a; in GIF89a, it indicates
     * the pixel aspect ratio, as (PAR + 15) : 64. If PAR is zero,
     * this means no aspect ratio information is given, PAR = 1 means
     * 1:4 (narrow), PAR = 49 means 1:1 (square), PAR = 255 means
     * slightly over 4:1 (wide), etc.
     */

    pm->cmap = (SkinColor *) malloc(256 * sizeof(SkinColor));
    // TODO - handle memory allocation failure
    if (has_global_cmap) {
        for (i = 0; i < ncolors; i++) {
            int r, g, b;
            if (!read_byte(&r)
                    || !read_byte(&g)
                    || !read_byte(&b)) {
                fprintf(stderr, "Fatally premature EOF.\n");
                goto failed;
            }
            gcmap.color[i].r = r;
            gcmap.color[i].g = g;
            gcmap.color[i].b = b;
        }
    } else {
        for (i = 0; i < ncolors; i++) {
            int k = (i * 255) / (ncolors - 1);
            gcmap.color[i].r = k;
            gcmap.color[i].g = k;
            gcmap.color[i].b = k;
        }
    }

    mono = false;
    if (ncolors <= 2) {
        /* Test for true black & white */
        mono = true;
        for (i = 0; i < ncolors; i++)
            if (!(gcmap.color[i].r == 0
                    && gcmap.color[i].g == 0
                    && gcmap.color[i].b == 0
                || gcmap.color[i].r == 255
                    && gcmap.color[i].g == 255
                    && gcmap.color[i].b == 255)) {
                mono = false;
                break;
            }
    }

    if (mono) {
        pm->depth = 1;
        pm->bytesperline = ((pm->width + 31) >> 3) & ~3;
    } else {
        pm->depth = 8;
        pm->bytesperline = (pm->width + 3) & ~3;
    }

    size = pm->bytesperline * pm->height;
    pm->pixels = (unsigned char *) malloc(size);
    // TODO - handle memory allocation failure

    if (has_global_cmap) {
        if (pm->depth == 1) {
            memset(pm->pixels, background * 255, size);
            if (background == 0)
                black_used = true;
            else
                white_used = true;
        } else {
            memset(pm->pixels, background, size);
            pm->cmap[0] = gcmap.color[background];
            gcmap.index[background] = 0;
            image_colors = 1;
        }
    } else {
        memset(pm->pixels, 0, size);
    }

    while (1) {
        int whatnext;
        if (!read_byte(&whatnext))
            goto unexp_eof;
        if (whatnext == ',') {
            /* Image */
            int ileft, itop, iwidth, iheight;
            int info;

            int lbpp;
            int lncolors;

            int interlaced;
            int h;
            int v;
            int codesize;
            int bytecount;

            short prefix_table[4096];
            short code_table[4096];

            int maxcode;
            int clear_code;
            int end_code;

            int curr_code_size;
            int curr_code;
            int old_code;
            int bits_needed;
            int end_code_seen;

            if (!read_short(&ileft)
                    || !read_short(&itop)
                    || !read_short(&iwidth)
                    || !read_short(&iheight)
                    || !read_byte(&info))
                goto unexp_eof;

            if (itop + iheight > pm->height
                    || ileft + iwidth > pm->width) {
                fprintf(stderr, "Image position and size not contained within screen size!\n");
                goto failed;
            }

            /* Bit 3 of info must be zero in GIF87a; in GIF89a, if it
             * is set, it indicates that the local colormap is sorted,
             * the most important entries being first. In PseudoColor
             * environments this can be used to make sure to get the
             * most important colors from the X server first, to
             * optimize the image's appearance in the event that not
             * all the colors from the colormap can actually be
             * obtained at the same time.
             */
            if ((info & 128) == 0) {
                /* Using global color map */
                lcmap = &gcmap;
                lbpp = bpp;
                lncolors = ncolors;
            } else {
                /* Using local color map */
                lbpp = (info & 7) + 1;
                lncolors = 1 << lbpp;
                lcmap = new (std::nothrow) GifColorMap;
                // TODO - handle memory allocation failure
                for (i = 0; i < lncolors; i++) {
                    int r, g, b;
                    if (!read_byte(&r)
                            || !read_byte(&g)
                            || !read_byte(&b))
                        goto unexp_eof;
                    lcmap->color[i].r = r;
                    lcmap->color[i].g = g;
                    lcmap->color[i].b = b;
                }
            }

            interlaced = (info & 64) != 0;
            h = 0;
            v = 0;
            if (!read_byte(&codesize) || !read_byte(&bytecount))
                goto unexp_eof;

            maxcode = 1 << codesize;
            for (i = 0; i < maxcode + 2; i++) {
                prefix_table[i] = -1;
                code_table[i] = i;
            }
            clear_code = maxcode++;
            end_code = maxcode++;

            curr_code_size = codesize + 1;
            curr_code = 0;
            old_code = -1;
            bits_needed = curr_code_size;
            end_code_seen = 0;

            while (bytecount != 0) {
                for (i = 0; i < bytecount; i++) {
                    int currbyte;
                    int bits_available;

                    if (!read_byte(&currbyte))
                        goto unexp_eof;
                    if (end_code_seen)
                        continue;

                    bits_available = 8;
                    while (bits_available != 0) {
                        int bits_copied = bits_needed < bits_available ?
                                    bits_needed : bits_available;
                        int bits = currbyte >> (8 - bits_available);
                        bits &= 255 >> (8 - bits_copied);
                        curr_code |= bits << (curr_code_size - bits_needed);
                        bits_available -= bits_copied;
                        bits_needed -= bits_copied;

                        if (bits_needed == 0) {
                            if (curr_code == end_code) {
                                end_code_seen = 1;
                            } else if (curr_code == clear_code) {
                                maxcode = (1 << codesize) + 2;
                                curr_code_size = codesize + 1;
                                old_code = -1;
                            } else {
                                unsigned char expanded[4096];
                                int explen;

                                if (curr_code < maxcode) {
                                    if (maxcode < 4096 && old_code != -1) {
                                        int c = curr_code;
                                        while (prefix_table[c] != -1)
                                            c = prefix_table[c];
                                        c = code_table[c];
                                        prefix_table[maxcode] = old_code;
                                        code_table[maxcode] = c;
                                        maxcode++;
                                        if (maxcode == (1 << curr_code_size)
                                                && curr_code_size < 12)
                                            curr_code_size++;
                                    }
                                } else if (curr_code == maxcode) {
                                    /* Once maxcode == 4096, we can't get here
                                     * any more, because we refuse to raise
                                     * curr_code_size above 12 -- so we can
                                     * never read a bigger code than 4095.
                                     */
                                    int c;
                                    if (old_code == -1) {
                                        fprintf(stderr, "Out-of-sequence code in compressed data.\n");
                                        goto done;
                                    }
                                    c = old_code;
                                    while (prefix_table[c] != -1)
                                        c = prefix_table[c];
                                    c = code_table[c];
                                    prefix_table[maxcode] = old_code;
                                    code_table[maxcode] = c;
                                    maxcode++;
                                    if (maxcode == (1 << curr_code_size)
                                            && curr_code_size < 12)
                                        curr_code_size++;
                                } else {
                                    fprintf(stderr, "Out-of-sequence code in compressed data.\n");
                                    goto done;
                                }

                                old_code = curr_code;

                                /* Output curr_code! */
                                explen = 0;
                                while (curr_code != -1) {
                                    expanded[explen++] =
                                        (unsigned char) code_table[curr_code];
                                    curr_code = prefix_table[curr_code];
                                }
                                for (j = explen - 1; j >= 0; j--) {
                                    int pixel = expanded[j];
                                    if (pm->depth == 1) {
                                        int p = lcmap->index[pixel];
                                        if (p == -1) {
                                            unsigned char r = lcmap->color[pixel].r;
                                            unsigned char g = lcmap->color[pixel].g;
                                            unsigned char b = lcmap->color[pixel].b;
                                            if (r == 0 && g == 0 && b == 0) {
                                                p = 0;
                                                black_used = true;
                                            } else if (r == 255 && g == 255 || b == 255) {
                                                p = 1;
                                                white_used = true;
                                            } else {
                                                /* Not black & white; switch to 8-bit */
                                                int newbytesperline = pm->width;
                                                int newsize = newbytesperline * pm->height;
                                                unsigned char *newpixels = (unsigned char *)
                                                            malloc(newsize);
                                                // TODO - handle memory allocation failure
                                                if (!white_used || !black_used)
                                                    memset(newpixels, 0, newsize);
                                                else
                                                    for (int v = 0; v < pm->height; v++) {
                                                        unsigned char *newpixel = newpixels + newbytesperline * v;
                                                        for (int h = 0; h < pm->width; h++) {
                                                            unsigned char px = (pm->pixels[pm->bytesperline * v + (h >> 3)] >> (h & 7)) & 1;
                                                            *newpixel++ = px;
                                                        }
                                                    }
                                                free(pm->pixels);
                                                pm->pixels = newpixels;
                                                pm->bytesperline = newbytesperline;
                                                pm->depth = 8;
                                                if (black_used && white_used) {
                                                    pm->cmap[0].r = 0;
                                                    pm->cmap[0].g = 0;
                                                    pm->cmap[0].b = 0;
                                                    pm->cmap[1].r = 255;
                                                    pm->cmap[1].g = 255;
                                                    pm->cmap[1].b = 255;
                                                    image_colors = 2;
                                                } else if (black_used || white_used) {
                                                    unsigned char c = black_used ? 0 : 255;
                                                    pm->cmap[0].r = c;
                                                    pm->cmap[0].g = c;
                                                    pm->cmap[0].b = c;
                                                    image_colors = 1;
                                                } else {
                                                    image_colors = 0;
                                                }
                                                if (white_used && !black_used)
                                                    for (i = 0; i < 256; i++)
                                                        if (lcmap->index[i] == 1)
                                                            lcmap->index[i] = 0;
                                                goto indexed;
                                            }
                                            lcmap->index[pixel] = p;
                                        }
                                        int x = ileft + h;
                                        int index = pm->bytesperline * (itop + v) + (x >> 3);
                                        unsigned char mask = 1 << (x & 7);
                                        if (p)
                                            pm->pixels[index] |= mask;
                                        else
                                            pm->pixels[index] &= ~mask;
                                    } else if (pm->depth == 8) {
                                        indexed:
                                        int p = lcmap->index[pixel];
                                        if (p == -1) {
                                            unsigned char r = lcmap->color[pixel].r;
                                            unsigned char g = lcmap->color[pixel].g;
                                            unsigned char b = lcmap->color[pixel].b;
                                            for (p = 0; p < image_colors; p++)
                                                if (pm->cmap[p].r == r
                                                        && pm->cmap[p].g == g
                                                        && pm->cmap[p].b == b) {
                                                    lcmap->index[pixel] = p;
                                                    goto found;
                                                }
                                            if (image_colors == 256) {
                                                /* Out of colormap entries; switch to truecolor */
                                                int newbytesperline = pm->width * 3;
                                                unsigned char *newpixels = (unsigned char *)
                                                            malloc(newbytesperline * pm->height);
                                                // TODO - handle memory allocation failure
                                                for (int v = 0; v < pm->height; v++) {
                                                    unsigned char *newpixel = newpixels + newbytesperline * v;
                                                    for (int h = 0; h < pm->width; h++) {
                                                        unsigned char px = pm->pixels[pm->bytesperline * v + h];
                                                        *newpixel++ = pm->cmap[px].r;
                                                        *newpixel++ = pm->cmap[px].g;
                                                        *newpixel++ = pm->cmap[px].b;
                                                    }
                                                }
                                                free(pm->pixels);
                                                pm->pixels = newpixels;
                                                pm->bytesperline = newbytesperline;
                                                pm->depth = 24;
                                                goto truecolor;
                                            }
                                            p = image_colors++;
                                            pm->cmap[p].r = r;
                                            pm->cmap[p].g = g;
                                            pm->cmap[p].b = b;
                                            lcmap->index[pixel] = p;
                                        }
                                        found:
                                        pm->pixels[pm->bytesperline * (itop + v) + ileft + h] = p;
                                    } else {
                                        truecolor:
                                        unsigned char *rgb = pm->pixels + (pm->bytesperline * (itop + v) + 3 * (ileft + h));
                                        *rgb++ = lcmap->color[pixel].r;
                                        *rgb++ = lcmap->color[pixel].g;
                                        *rgb = lcmap->color[pixel].b;
                                    }
                                    if (++h == iwidth) {
                                        h = 0;
                                        if (interlaced) {
                                            switch (v & 7) {
                                                case 0:
                                                    v += 8;
                                                    if (v < iheight)
                                                        break;
                                                    /* Some GIF en/decoders go
                                                     * straight from the '0'
                                                     * pass to the '4' pass
                                                     * without checking the
                                                     * height, and blow up on
                                                     * 2/3/4 pixel high
                                                     * interlaced images.
                                                     */
                                                    if (iheight > 4)
                                                        v = 4;
                                                    else if (iheight > 2)
                                                        v = 2;
                                                    else if (iheight > 1)
                                                        v = 1;
                                                    else
                                                        end_code_seen = 1;
                                                    break;
                                                case 4:
                                                    v += 8;
                                                    if (v >= iheight)
                                                        v = 2;
                                                    break;
                                                case 2:
                                                case 6:
                                                    v += 4;
                                                    if (v >= iheight)
                                                        v = 1;
                                                    break;
                                                case 1:
                                                case 3:
                                                case 5:
                                                case 7:
                                                    v += 2;
                                                    if (v >= iheight)
                                                        end_code_seen = 1;
                                                    break;
                                            }
                                            if (end_code_seen)
                                                break;
                                        } else {
                                            if (++v == iheight) {
                                                end_code_seen = 1;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            curr_code = 0;
                            bits_needed = curr_code_size;
                        }
                    }
                }
                if (!read_byte(&bytecount))
                    goto unexp_eof;
            }

            if (lcmap != &gcmap)
                delete lcmap;
            lcmap = NULL;

        } else if (whatnext == '!') {
            /* Extension block */
            int function_code, byte_count;
            if (!read_byte(&function_code))
                goto unexp_eof;
            if (!read_byte(&byte_count))
                goto unexp_eof;
            while (byte_count != 0) {
                for (i = 0; i < byte_count; i++) {
                    int dummy;
                    if (!read_byte(&dummy))
                        goto unexp_eof;
                }
                if (!read_byte(&byte_count))
                    goto unexp_eof;
            }
        } else if (whatnext == ';') {
            /* Terminator */
            break;
        } else {
            fprintf(stderr, "Unrecognized tag '%c' (0x%02x).\n",
                    whatnext, whatnext);
            goto failed;
        }
    }

    done:
    if (lcmap != NULL && lcmap != &gcmap)
        delete lcmap;
    if (pm->depth == 1 || pm->depth == 24) {
        free(pm->cmap);
        pm->cmap = NULL;
    }

    /* Successfully read the image. Now, write it out: */
    if (pm->depth == 1)
        type = IMGTYPE_MONO;
    else if (pm->depth == 8)
        type = IMGTYPE_COLORMAPPED;
    else /* pm->depth == 24 */
        type = IMGTYPE_TRUECOLOR;

    res = skin_init_image(type, 256, pm->cmap, pm->width, pm->height);
    if (res) {
        ptr = pm->pixels;
        for (i = 0; i < pm->height; i++) {
            skin_put_pixels(ptr);
            ptr += pm->bytesperline;
        }
        skin_finish_image();
    }

    /* Done writing; delete the image and return. */
    if (pm->cmap != NULL)
        free(pm->cmap);
    free(pm->pixels);
    return res;


    unexp_eof:
    fprintf(stderr, "Unexpected EOF.\n");
    goto done;


    failed:
    if (lcmap != NULL && lcmap != &gcmap)
        delete lcmap;
    if (pm->cmap != NULL) {
        free(pm->cmap);
        pm->cmap = NULL;
    }
    if (pm->pixels != NULL) {
        free(pm->pixels);
        pm->pixels = NULL;
    }
    return 0;
}
