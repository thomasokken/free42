/*****************************************************************************
 * Free42 -- a free HP-42S calculator clone
 * Copyright (C) 2004-2005  Thomas Okken
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

#ifndef SHELL2_H
#define SHELL2_H 1


#include "free42.h"
#include "filesys.h"
#include "skin2prc.h"


#define PRINT_LINES 3000
#define PRINT_BYTESPERLINE 20
#define PRINT_SIZE 60000


#define SHELL_VERSION 3

typedef struct {
    Boolean extras;
    Boolean printerToMemo;
    Boolean printerToTxtFile;
    Boolean printerToGifFile;
    char printerMemoName[FILENAMELEN];
    char printerTxtFileName[FILENAMELEN];
    char printerGifFileName[FILENAMELEN];
    int printerGifMaxLength;
    int soundVolume;
    char skinName[32];
} state_type;

extern state_type state;

extern BitmapType *disp_bitmap;
extern char *disp_bits_v3;
extern BitmapType *print_bitmap;
extern int want_to_run;
extern Int32 timeout3time;
extern char *printout;
extern int4 printout_top;
extern int4 printout_bottom;
extern int4 printout_pos;
extern int can_draw;
extern int softkey;

extern int updownAnn;
extern int shiftAnn;
extern int printAnn;
extern int runAnn;
extern int batteryAnn;
extern int gAnn;
extern int radAnn;

extern SkinSpec *skin;


void open_math_lib() SHELL_SECT;
void load_skin() SHELL_SECT;
void unload_skin() SHELL_SECT;
void close_math_lib() SHELL_SECT;
void misc_cleanup() SHELL_SECT;
int feature_set_3_5_present() SHELL_SECT;
int feature_set_4_0_present() SHELL_SECT;
int feature_set_high_density_present() SHELL_SECT;
int graffiti_2_present() SHELL_SECT;
void init_shell_state(int4 version) SHELL_SECT;
int read_shell_state(int4 *ver) SHELL_SECT;
int write_shell_state() SHELL_SECT;
void set_colors(RGBColorType *bg, RGBColorType *fg) SHELL_SECT;
void restore_colors() SHELL_SECT;
void set_coord_sys() SHELL_SECT;
void restore_coord_sys() SHELL_SECT;
void repaint_annunciator(int i) SHELL_SECT;
void repaint_printout() SHELL_SECT;
void draw_softkey(int state) SHELL_SECT;
Boolean form_handler(EventType *e) SHELL_SECT;
Boolean calcgadget_handler(struct FormGadgetTypeInCallback *gadgetP,
			    UInt16 cmd, void *paramP) SHELL_SECT;
Boolean printgadget_handler(struct FormGadgetTypeInCallback *gadgetP,
			    UInt16 cmd, void *paramP) SHELL_SECT;
Boolean handle_event(EventType *e) SHELL_SECT;

void show_message(char *message) SHELL_SECT;
void set_field_text(FieldType *fld, const char *text) SHELL_SECT;

void print_to_memo(const char *text, int length) SHELL_SECT;
void print_to_txt(const char *text, int length) SHELL_SECT;
void print_to_gif(const char *bits, short bytesperline, short x, short y,
		  short width, short height) SHELL_SECT;
void close_memo() SHELL_SECT;
void close_txt() SHELL_SECT;
void close_gif(int reset_sequence) SHELL_SECT;

#endif
