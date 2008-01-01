/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2008  Thomas Okken
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

#ifndef CORE_DISPLAY_H
#define CORE_DISPLAY_H 1

#include "free42.h"
#include "core_phloat.h"
#include "core_globals.h"

bool persist_display() DISPLAY_SECT;
bool unpersist_display(int version) DISPLAY_SECT;
void clear_display() DISPLAY_SECT;
void flush_display() DISPLAY_SECT;
void repaint_display() DISPLAY_SECT;
void draw_pixel(int x, int y) DISPLAY_SECT;
void draw_pattern(phloat dx, phloat dy, const char *pattern, int pattern_width)
								DISPLAY_SECT;
void fly_goose() DISPLAY_SECT;

void squeak() DISPLAY_SECT;
void tone(int n) DISPLAY_SECT;
void draw_char(int x, int y, char c) DISPLAY_SECT;
void get_char(char *bits, char c) DISPLAY_SECT;
void draw_string(int x, int y, const char *s, int length) DISPLAY_SECT;
void clear_row(int row) DISPLAY_SECT;

void display_prgm_line(int row, int line_offset) DISPLAY_SECT;
void display_x(int row) DISPLAY_SECT;
void display_y(int row) DISPLAY_SECT;
void display_incomplete_command(int row) DISPLAY_SECT;
void display_error(int error, int print) DISPLAY_SECT;
void display_command(int row) DISPLAY_SECT;
void draw_varmenu() DISPLAY_SECT;
void display_mem() DISPLAY_SECT;
void show() DISPLAY_SECT;
void redisplay() DISPLAY_SECT;

void print_display() DISPLAY_SECT;
int print_program(int prgm_index, int4 pc, int4 lines, int normal) DISPLAY_SECT;
void print_program_line(int prgm_index, int4 pc) DISPLAY_SECT;
int command2buf(char *buf, int len, int cmd, const arg_struct *arg) DISPLAY_SECT;

#define MENULEVEL_COMMAND   0
#define MENULEVEL_ALPHA     1
#define MENULEVEL_TRANSIENT 2
#define MENULEVEL_PLAIN     3
#define MENULEVEL_APP       4

int appmenu_exitcallback_1(int menuid) COMMANDS3_SECT;
int appmenu_exitcallback_2(int menuid) COMMANDS5_SECT;
int appmenu_exitcallback_3(int menuid) COMMANDS4_SECT;
int appmenu_exitcallback_4(int menuid) COMMANDS4_SECT;
int appmenu_exitcallback_5(int menuid) COMMANDS4_SECT;

void set_menu(int level, int menuid) DISPLAY_SECT;
int set_menu_return_err(int level, int menuid) DISPLAY_SECT;
void set_appmenu_exitcallback(int callback_id) DISPLAY_SECT;
void set_plainmenu(int menuid) DISPLAY_SECT;
void set_catalog_menu(int direction) DISPLAY_SECT;
int *get_front_menu() DISPLAY_SECT;
void set_cat_section(int section) DISPLAY_SECT;
int get_cat_section() DISPLAY_SECT;
void move_cat_row(int direction) DISPLAY_SECT;
void set_cat_row(int row) DISPLAY_SECT;
int get_cat_row() DISPLAY_SECT;
int get_cat_item(int menukey) DISPLAY_SECT;
void update_catalog() DISPLAY_SECT;

void clear_custom_menu() DISPLAY_SECT;
void assign_custom_key(int keynum, const char *name, int length) DISPLAY_SECT;
void get_custom_key(int keynum, char *name, int *length) DISPLAY_SECT;

void clear_prgm_menu() DISPLAY_SECT;
void assign_prgm_key(int keynum, int is_gto, const arg_struct *arg) DISPLAY_SECT;
void do_prgm_menu_key(int keynum) DISPLAY_SECT;

#endif
