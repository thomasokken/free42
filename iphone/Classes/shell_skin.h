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

typedef struct {
    unsigned char r, g, b, pad;
} SkinColor;

#define IMGTYPE_MONO 1
#define IMGTYPE_GRAY 2
#define IMGTYPE_COLORMAPPED 3
#define IMGTYPE_TRUECOLOR 4

#define KEYMAP_MAX_MACRO_LENGTH 31
typedef struct {
    bool ctrl;
    bool alt;
    bool shift; 
    bool cshift; 
    int keycode;
    unsigned char macro[KEYMAP_MAX_MACRO_LENGTH + 1];
} keymap_entry;

keymap_entry *parse_keymap_entry(char *line, int lineno);

int skin_getchar();
void skin_rewind();
int skin_init_image(int type, int ncolors, const SkinColor *colors,
                    int width, int height);
void skin_put_pixels(unsigned const char *data);
void skin_finish_image();
