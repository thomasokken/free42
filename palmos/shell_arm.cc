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

#include <stdlib.h>
#include <PalmOS.h>
extern "C" {
#include "peal.h"
}
#include "endianutils.h"

#include "core_arm.h"
#include "shell.h"
#include "core_main.h"

core_settings_struct core_settings;
core_settings_struct *real_core_settings;

static PealModule *m;
static void *p_core_init;
static void *p_core_quit;
static void *p_core_repaint_display;
static void *p_core_menu;
static void *p_core_alpha_menu;
static void *p_core_hex_menu;
static void *p_core_keydown;
static void *p_core_repeat;
static void *p_core_keytimeout1;
static void *p_core_keytimeout2;
static void *p_core_timeout3;
static void *p_core_keyup;
static void *p_core_allows_powerdown;
static void *p_core_powercycle;
static void *p_core_list_programs;
static void *p_core_export_programs;
static void *p_core_import_programs;
static void *p_core_copy;
static void *p_core_paste;
static void *p_redisplay;
static void *p_squeak;

static void arm_shell_blitter(arg_shell_blitter *arg);
static void arm_shell_beeper(arg_shell_beeper *arg);
static void arm_shell_annunciators(arg_shell_annunciators *arg);
static int4 arm_shell_wants_cpu();
static void arm_shell_delay(int4 duration);
static void arm_shell_request_timeout3(int4 delay);
static int4 arm_shell_read_saved_state(arg_shell_read_saved_state *arg);
static int4 arm_shell_write_saved_state(arg_shell_write_saved_state *arg);
static uint4 arm_shell_get_mem();
static int4 arm_shell_low_battery();
static void arm_shell_powerdown();
static double *arm_shell_random_seed();
static int4 arm_shell_milliseconds();
static void arm_shell_print(arg_shell_print *arg);
static int4 arm_shell_write(arg_shell_write *arg);
static int4 arm_shell_read(arg_shell_read *arg);
static shell_bcd_table_struct *arm_shell_get_bcd_table();
static shell_bcd_table_struct *arm_shell_put_bcd_table(arg_shell_put_bcd_table *arg);
static void arm_shell_release_bcd_table(shell_bcd_table_struct *table);
static void *arm_malloc(size_t size);
static void *arm_realloc(arg_realloc *arg);
static void arm_free(void *ptr);
#ifdef DEBUG
static void arm_logtofile(void *ptr);
static void arm_lognumber(void *ptr);
static void arm_logdouble(void *ptr);
#endif

typedef union {
    arg_core_init init;
    arg_core_keydown keydown;
    arg_core_list_programs list;
    arg_core_export_programs exprt;
    arg_core_copy copy;
    int4 i;
} arg_union;

static arg_union *au;

void core_init(int read_state, int4 version) {
    au = (arg_union *) malloc(sizeof(arg_union));

    m = PealLoadFromResources('armc', 1000);
    p_core_init = PealLookupSymbol(m, "_Z13arm_core_initPv");
    p_core_quit = PealLookupSymbol(m, "_Z13arm_core_quitPv");
    p_core_repaint_display = PealLookupSymbol(m, "_Z24arm_core_repaint_displayPv");
    p_core_menu = PealLookupSymbol(m, "_Z13arm_core_menuPv");
    p_core_alpha_menu = PealLookupSymbol(m, "_Z19arm_core_alpha_menuPv");
    p_core_hex_menu = PealLookupSymbol(m, "_Z17arm_core_hex_menuPv");
    p_core_keydown = PealLookupSymbol(m, "_Z16arm_core_keydownPv");
    p_core_repeat = PealLookupSymbol(m, "_Z15arm_core_repeatPv");
    p_core_keytimeout1 = PealLookupSymbol(m, "_Z20arm_core_keytimeout1Pv");
    p_core_keytimeout2 = PealLookupSymbol(m, "_Z20arm_core_keytimeout2Pv");
    p_core_timeout3 = PealLookupSymbol(m, "_Z17arm_core_timeout3Pv");
    p_core_keyup = PealLookupSymbol(m, "_Z14arm_core_keyupPv");
    p_core_allows_powerdown = PealLookupSymbol(m, "_Z25arm_core_allows_powerdownPv");
    p_core_powercycle = PealLookupSymbol(m, "_Z19arm_core_powercyclePv");
    p_core_list_programs = PealLookupSymbol(m, "_Z22arm_core_list_programsPv");
    p_core_export_programs = PealLookupSymbol(m, "_Z24arm_core_export_programsPv");
    p_core_import_programs = PealLookupSymbol(m, "_Z24arm_core_import_programsPv");
    p_core_copy = PealLookupSymbol(m, "_Z13arm_core_copyPv");
    p_core_paste = PealLookupSymbol(m, "_Z14arm_core_pastePv");
    p_redisplay = PealLookupSymbol(m, "_Z13arm_redisplayPv");
    p_squeak = PealLookupSymbol(m, "_Z10arm_squeakPv");

    void **p;
    p = (void **) PealLookupSymbol(m, "p_shell_blitter"); *p = (void *) ByteSwap32(arm_shell_blitter);
    p = (void **) PealLookupSymbol(m, "p_shell_beeper"); *p = (void *) ByteSwap32(arm_shell_beeper);
    p = (void **) PealLookupSymbol(m, "p_shell_annunciators"); *p = (void *) ByteSwap32(arm_shell_annunciators);
    p = (void **) PealLookupSymbol(m, "p_shell_wants_cpu"); *p = (void *) ByteSwap32(arm_shell_wants_cpu);
    p = (void **) PealLookupSymbol(m, "p_shell_delay"); *p = (void *) ByteSwap32(arm_shell_delay);
    p = (void **) PealLookupSymbol(m, "p_shell_request_timeout3"); *p = (void *) ByteSwap32(arm_shell_request_timeout3);
    p = (void **) PealLookupSymbol(m, "p_shell_read_saved_state"); *p = (void *) ByteSwap32(arm_shell_read_saved_state);
    p = (void **) PealLookupSymbol(m, "p_shell_write_saved_state"); *p = (void *) ByteSwap32(arm_shell_write_saved_state);
    p = (void **) PealLookupSymbol(m, "p_shell_get_mem"); *p = (void *) ByteSwap32(arm_shell_get_mem);
    p = (void **) PealLookupSymbol(m, "p_shell_low_battery"); *p = (void *) ByteSwap32(arm_shell_low_battery);
    p = (void **) PealLookupSymbol(m, "p_shell_powerdown"); *p = (void *) ByteSwap32(arm_shell_powerdown);
    p = (void **) PealLookupSymbol(m, "p_shell_random_seed"); *p = (void *) ByteSwap32(arm_shell_random_seed);
    p = (void **) PealLookupSymbol(m, "p_shell_milliseconds"); *p = (void *) ByteSwap32(arm_shell_milliseconds);
    p = (void **) PealLookupSymbol(m, "p_shell_print"); *p = (void *) ByteSwap32(arm_shell_print);
    p = (void **) PealLookupSymbol(m, "p_shell_write"); *p = (void *) ByteSwap32(arm_shell_write);
    p = (void **) PealLookupSymbol(m, "p_shell_read"); *p = (void *) ByteSwap32(arm_shell_read);
    p = (void **) PealLookupSymbol(m, "p_shell_get_bcd_table"); *p = (void *) ByteSwap32(arm_shell_get_bcd_table);
    p = (void **) PealLookupSymbol(m, "p_shell_put_bcd_table"); *p = (void *) ByteSwap32(arm_shell_put_bcd_table);
    p = (void **) PealLookupSymbol(m, "p_shell_release_bcd_table"); *p = (void *) ByteSwap32(arm_shell_release_bcd_table);
    p = (void **) PealLookupSymbol(m, "p_malloc"); *p = (void *) ByteSwap32(arm_malloc);
    p = (void **) PealLookupSymbol(m, "p_realloc"); *p = (void *) ByteSwap32(arm_realloc);
    p = (void **) PealLookupSymbol(m, "p_free"); *p = (void *) ByteSwap32(arm_free);
#ifdef DEBUG
    p = (void **) PealLookupSymbol(m, "p_logtofile"); *p = (void *) ByteSwap32(arm_logtofile);
    p = (void **) PealLookupSymbol(m, "p_lognumber"); *p = (void *) ByteSwap32(arm_lognumber);
    p = (void **) PealLookupSymbol(m, "p_logdouble"); *p = (void *) ByteSwap32(arm_logdouble);
#endif

    real_core_settings = (core_settings_struct *) PealLookupSymbol(m, "core_settings");

    au->init.read_state = ByteSwap16(read_state);
    au->init.version = ByteSwap32(version);
    PealCall(m, p_core_init, au);
}

void core_quit() {
    PealCall(m, p_core_quit, NULL);
    PealUnload(m);
    free(au);
}

void core_repaint_display() {
    PealCall(m, p_core_repaint_display, NULL);
}

int core_menu() {
    return (int) PealCall(m, p_core_menu, NULL);
}

int core_alpha_menu() {
    return (int) PealCall(m, p_core_alpha_menu, NULL);
}

int core_hex_menu() {
    return (int) PealCall(m, p_core_hex_menu, NULL);
}

int core_keydown(int key, int *enqueued, int *repeat) {
    au->keydown.key = ByteSwap16(key);
    int ret = (int) PealCall(m, p_core_keydown, au);
    *enqueued = ByteSwap16(au->keydown.enqueued);
    *repeat = ByteSwap16(au->keydown.repeat);
    return ret;
}

int core_repeat() {
    return (int) PealCall(m, p_core_repeat, NULL);
}

void core_keytimeout1() {
    PealCall(m, p_core_keytimeout1, NULL);
}

void core_keytimeout2() {
    PealCall(m, p_core_keytimeout2, NULL);
}

bool core_timeout3(int repaint) {
    int4 p = repaint;
    p = ByteSwap32(p);
    return (bool) PealCall(m, p_core_timeout3, (void *) p);
}

int core_keyup() {
    return (int) PealCall(m, p_core_keyup, NULL);
}

int core_allows_powerdown(int *want_cpu) {
    int ret = (int) PealCall(m, p_core_allows_powerdown, au);
    *want_cpu = (int) ByteSwap32(au->i);
    return ret;
}

int core_powercycle() {
    return (int) PealCall(m, p_core_powercycle, NULL);
}

int core_list_programs(char *buf, int bufsize) {
    au->list.buf = (char *) ByteSwap32(buf);
    au->list.bufsize = ByteSwap16(bufsize);
    return (int) PealCall(m, p_core_list_programs, au);
}

int core_export_programs(int count, const int *indexes,
			 int (*progress_report)(const char *)) {
    int4 *indexes4 = (int4 *) malloc(count * sizeof(int4));
    // TODO - handle memory allocation failure
    for (int i = 0; i < count; i++)
	indexes4[i] = ByteSwap32(indexes[i]);
    au->exprt.count = ByteSwap16(count);
    au->exprt.indexes = (const int4 *) ByteSwap32(indexes4);
    int ret = (int) PealCall(m, p_core_export_programs, au);
    free(indexes4);
    return ret;
}

void core_import_programs(int (*progress_report)(const char *)) {
    PealCall(m, p_core_import_programs, NULL);
}

void core_copy(char *buf, int buflen) {
    au->copy.buf = (char *) ByteSwap32(buf);
    au->copy.buflen = ByteSwap16(buflen);
    PealCall(m, p_core_copy, au);
}

void core_paste(const char *s) {
    PealCall(m, p_core_paste, (void *) s);
}

void redisplay() {
    PealCall(m, p_redisplay, NULL);
}

void squeak() {
    PealCall(m, p_squeak, NULL);
}

void put_core_settings() {
    *real_core_settings = core_settings;
}

void get_core_settings() {
    core_settings = *real_core_settings;
}

static void arm_shell_blitter(arg_shell_blitter *arg) {
    shell_blitter(arg->bits, arg->bytesperline, arg->x, arg->y, arg->width, arg->height);
}

static void arm_shell_beeper(arg_shell_beeper *arg) {
    shell_beeper(arg->frequency, arg->duration);
}

static void arm_shell_annunciators(arg_shell_annunciators *arg) {
    shell_annunciators(arg->updn, arg->shf, arg->prt, arg->run, arg->g, arg->rad);
}

static int4 arm_shell_wants_cpu() {
    return shell_wants_cpu();
}

static void arm_shell_delay(int4 duration) {
    shell_delay(duration);
}

static void arm_shell_request_timeout3(int4 delay) {
    shell_request_timeout3(delay);
}

static int4 arm_shell_read_saved_state(arg_shell_read_saved_state *arg) {
    return shell_read_saved_state(arg->buf, arg->bufsize);
}

static int4 arm_shell_write_saved_state(arg_shell_write_saved_state *arg) {
    return shell_write_saved_state(arg->buf, arg->nbytes);
}

static uint4 arm_shell_get_mem() {
    return shell_get_mem();
}

static int4 arm_shell_low_battery() {
    return shell_low_battery();
}

static void arm_shell_powerdown() {
    shell_powerdown();
}

static double *arm_shell_random_seed() {
    static double s = shell_random_seed();
    return &s;
}

static int4 arm_shell_milliseconds() {
    return shell_milliseconds();
}

static void arm_shell_print(arg_shell_print *arg) {
    shell_print(arg->text, arg->length, arg->bits, arg->bytesperline, arg->x, arg->y, arg->width, arg->height);
}

static int4 arm_shell_write(arg_shell_write *arg) {
    return shell_write(arg->buf, arg->buflen);
}

static int4 arm_shell_read(arg_shell_read *arg) {
    return shell_read(arg->buf, arg->buflen);
}

static shell_bcd_table_struct *arm_shell_get_bcd_table() {
    return shell_get_bcd_table();
}

static shell_bcd_table_struct *arm_shell_put_bcd_table(arg_shell_put_bcd_table *arg) {
    return shell_put_bcd_table((shell_bcd_table_struct *) arg->bcdtab, arg->size);
}

static void arm_shell_release_bcd_table(shell_bcd_table_struct *table) {
    shell_release_bcd_table(table);
}

static void* arm_malloc(size_t size) {
    return malloc(size);
}

static void* arm_realloc(arg_realloc *arg) {
    return realloc(arg->ptr, arg->size);
}

static void arm_free(void *ptr) {
    free(ptr);
}

#ifdef DEBUG
static void arm_logtofile(void *ptr) {
    logtofile((const char *) ptr);
}

static void arm_lognumber(void *ptr) {
    lognumber((int4) ptr);
}

static void arm_logdouble(void *ptr) {
    logdouble(*(double *) ptr);
}
#endif
