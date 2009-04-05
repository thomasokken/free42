/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2009  Thomas Okken
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

#import <UIKit/UIKit.h>
#import "shell_skin.h"

void skin_load(NSString *skinname, long *width, long *height);

void skin_repaint();
/*
void skin_repaint_annunciator(HDC hdc, HDC memdc, int which, int state);
void skin_find_key(int x, int y, bool cshift, int *skey, int *ckey);
int skin_find_skey(int ckey);
unsigned char *skin_find_macro(int ckey);
unsigned char *skin_keymap_lookup(int keycode, bool ctrl, bool alt, bool shift, bool cshift, bool *exact);
void skin_repaint_key(HDC hdc, HDC memdc, int key, int state);
void skin_display_blitter(HDC hdc, const char *bits, int bytesperline, int x, int y,
						  int width, int height);
void skin_repaint_display(HDC hdc, HDC memdc);
void skin_display_set_enabled(bool enable);
*/