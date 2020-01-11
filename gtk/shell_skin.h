///////////////////////////////////////////////////////////////////////////////
// Free42 -- an HP-42S calculator simulator
// Copyright (C) 2004-2020  Thomas Okken
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see http://www.gnu.org/licenses/.
///////////////////////////////////////////////////////////////////////////////

#ifndef SHELL_SKIN_H
#define SHELL_SKIN_H 1

#include <gtk/gtk.h>

void skin_menu_update(GtkWidget *w);
void skin_load(int *width, int *height);

typedef struct {
    unsigned char r, g, b, pad;
} SkinColor;

#define IMGTYPE_MONO 1
#define IMGTYPE_GRAY 2
#define IMGTYPE_COLORMAPPED 3
#define IMGTYPE_TRUECOLOR 4

int skin_getchar();
void skin_rewind();
int skin_init_image(int type, int ncolors, const SkinColor *colors,
                    int width, int height);
void skin_put_pixels(unsigned const char *data);
void skin_finish_image();

void skin_repaint(cairo_t *cr);
void skin_repaint_annunciator(cairo_t *cr, int which, bool state);
void skin_invalidate_annunciator(GdkWindow *win, int which);
void skin_find_key(int x, int y, bool cshift, int *key, int *code);
int skin_find_skey(int ckey);
unsigned char *skin_find_macro(int ckey, bool *is_name);
unsigned char *skin_keymap_lookup(guint keyval, bool printable,
                                  bool ctrl, bool alt, bool shift, bool cshift,
                                  bool *exact);
void skin_repaint_key(cairo_t *cr, int key, bool state);
void skin_invalidate_key(GdkWindow *win, int key);
void skin_display_invalidater(GdkWindow *win, const char *bits, int bytesperline,
                                int x, int y, int width, int height);
void skin_repaint_display(cairo_t *cr);
void skin_invalidate_display(GdkWindow *win);
void skin_display_set_enabled(bool enable);

#endif
