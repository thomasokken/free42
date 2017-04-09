/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2017  Thomas Okken
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

#ifndef CORE_DISPLAY_H
#define CORE_DISPLAY_H 1

#include "free42.h"
#include "core_phloat.h"
#include "core_globals.h"

bool persist_display();
bool unpersist_display(int version);
void clear_display();
void flush_display();
void repaint_display();
void draw_pixel(int x, int y);
void draw_pattern(phloat dx, phloat dy, const char *pattern, int pattern_width);
void fly_goose();

void squeak();
void tone(int n);
void draw_char(int x, int y, char c);
void get_char(char *bits, char c);
void draw_string(int x, int y, const char *s, int length);
void clear_row(int row);

void display_prgm_line(int row, int line_offset);
void display_x(int row);
void display_y(int row);
void display_incomplete_command(int row);
void display_error(int error, int print);
void display_command(int row);
void draw_varmenu();
void display_mem();
void show();
void redisplay();

void print_display();
int print_program(int prgm_index, int4 pc, int4 lines, int normal);
void print_program_line(int prgm_index, int4 pc);
int command2buf(char *buf, int len, int cmd, const arg_struct *arg);

typedef struct {
    char *buf;
    size_t size;
    size_t capacity;
    bool fail;
} textbuf;

void tb_write(textbuf *tb, const char *data, size_t size);
void tb_write_null(textbuf *tb);
void tb_print_current_program(textbuf *tb);

#define MENULEVEL_COMMAND   0
#define MENULEVEL_ALPHA     1
#define MENULEVEL_TRANSIENT 2
#define MENULEVEL_PLAIN     3
#define MENULEVEL_APP       4

int appmenu_exitcallback_1(int menuid, bool exitall);
int appmenu_exitcallback_2(int menuid, bool exitall);
int appmenu_exitcallback_3(int menuid, bool exitall);
int appmenu_exitcallback_4(int menuid, bool exitall);
int appmenu_exitcallback_5(int menuid, bool exitall);

void set_menu(int level, int menuid);
int set_menu_return_err(int level, int menuid, bool exitall);
void set_appmenu_exitcallback(int callback_id);
void set_plainmenu(int menuid);
void set_catalog_menu(int direction);
int *get_front_menu();
void set_cat_section(int section);
int get_cat_section();
void move_cat_row(int direction);
void set_cat_row(int row);
int get_cat_row();
int get_cat_item(int menukey);
void update_catalog();

void clear_custom_menu();
void assign_custom_key(int keynum, const char *name, int length);
void get_custom_key(int keynum, char *name, int *length);

void clear_prgm_menu();
void assign_prgm_key(int keynum, int is_gto, const arg_struct *arg);
void do_prgm_menu_key(int keynum);

#endif
