/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2006  Thomas Okken
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
static int4 arm_shell_get_mem();
static int4 arm_shell_low_battery();
static void arm_shell_powerdown();
//static double arm_shell_random_seed();
static int4 arm_shell_milliseconds();
static void arm_shell_print(arg_shell_print *arg);
static int4 arm_shell_write(arg_shell_write *arg);
static int4 arm_shell_read(arg_shell_read *arg);
static shell_bcd_table_struct *arm_shell_get_bcd_table();
static shell_bcd_table_struct *arm_shell_put_bcd_table(arg_shell_put_bcd_table *arg);
static void arm_shell_release_bcd_table(shell_bcd_table_struct *table);
static void* arm_malloc(size_t size);
static void* arm_realloc(arg_realloc *arg);
static void arm_free(void *ptr);

void core_init(int read_state, int4 version) {
    m = PealLoadFromResources('armc', 1000);
    p_core_init = PealLookupSymbol(m, "arm_core_init");
    p_core_quit = PealLookupSymbol(m, "arm_core_quit");
    p_core_repaint_display = PealLookupSymbol(m, "arm_core_repaint_display");
    p_core_menu = PealLookupSymbol(m, "arm_core_menu");
    p_core_alpha_menu = PealLookupSymbol(m, "arm_core_alpha_menu");
    p_core_hex_menu = PealLookupSymbol(m, "arm_core_hex_menu");
    p_core_keydown = PealLookupSymbol(m, "arm_core_keydown");
    p_core_repeat = PealLookupSymbol(m, "arm_core_repeat");
    p_core_keytimeout1 = PealLookupSymbol(m, "arm_core_keytimeout1");
    p_core_keytimeout2 = PealLookupSymbol(m, "arm_core_keytimeout2");
    p_core_timeout3 = PealLookupSymbol(m, "arm_core_timeout3");
    p_core_keyup = PealLookupSymbol(m, "arm_core_keyup");
    p_core_allows_powerdown = PealLookupSymbol(m, "arm_core_allows_powerdown");
    p_core_powercycle = PealLookupSymbol(m, "arm_core_powercycle");
    p_core_list_programs = PealLookupSymbol(m, "arm_core_list_programs");
    p_core_export_programs = PealLookupSymbol(m, "arm_core_export_programs");
    p_core_import_programs = PealLookupSymbol(m, "arm_core_import_programs");
    p_core_copy = PealLookupSymbol(m, "arm_core_copy");
    p_core_paste = PealLookupSymbol(m, "arm_core_paste");
    p_redisplay = PealLookupSymbol(m, "arm_redisplay");
    p_squeak = PealLookupSymbol(m, "arm_squeak");

    void **p;
    p = (void **) PealLookupSymbol(m, "p_shell_blitter"); *p = arm_shell_blitter;
    p = (void **) PealLookupSymbol(m, "p_shell_beeper"); *p = arm_shell_beeper;
    p = (void **) PealLookupSymbol(m, "p_shell_annunciators"); *p = arm_shell_annunciators;
    p = (void **) PealLookupSymbol(m, "p_shell_wants_cpu"); *p = arm_shell_wants_cpu;
    p = (void **) PealLookupSymbol(m, "p_shell_delay"); *p = arm_shell_delay;
    p = (void **) PealLookupSymbol(m, "p_shell_request_timeout3"); *p = arm_shell_request_timeout3;
    p = (void **) PealLookupSymbol(m, "p_shell_read_saved_state"); *p = arm_shell_read_saved_state;
    p = (void **) PealLookupSymbol(m, "p_shell_write_saved_state"); *p = arm_shell_write_saved_state;
    p = (void **) PealLookupSymbol(m, "p_shell_get_mem"); *p = arm_shell_get_mem;
    p = (void **) PealLookupSymbol(m, "p_shell_low_battery"); *p = arm_shell_low_battery;
    p = (void **) PealLookupSymbol(m, "p_shell_powerdown"); *p = arm_shell_powerdown;
    //p = (void **) PealLookupSymbol(m, "p_shell_random_seed"); *p = arm_shell_random_seed;
    p = (void **) PealLookupSymbol(m, "p_shell_milliseconds"); *p = arm_shell_milliseconds;
    p = (void **) PealLookupSymbol(m, "p_shell_print"); *p = arm_shell_print;
    p = (void **) PealLookupSymbol(m, "p_shell_write"); *p = arm_shell_write;
    p = (void **) PealLookupSymbol(m, "p_shell_read"); *p = arm_shell_read;
    p = (void **) PealLookupSymbol(m, "p_shell_get_bcd_table"); *p = arm_shell_get_bcd_table;
    p = (void **) PealLookupSymbol(m, "p_shell_put_bcd_table"); *p = arm_shell_put_bcd_table;
    p = (void **) PealLookupSymbol(m, "p_shell_release_bcd_table"); *p = arm_shell_release_bcd_table;
    p = (void **) PealLookupSymbol(m, "p_malloc"); *p = arm_malloc;
    p = (void **) PealLookupSymbol(m, "p_realloc"); *p = arm_realloc;
    p = (void **) PealLookupSymbol(m, "p_free"); *p = arm_free;

    arg_core_init arg;
    arg.read_state = ByteSwap16(read_state);
    arg.version = ByteSwap32(version);
    PealCall(m, p_core_init, &arg);
}

void core_quit() {
    PealCall(m, p_core_quit, NULL);
    PealUnload(m);
}

void core_repaint_display() {
    PealCall(m, p_core_repaint_display, NULL);
}

int core_menu() {
    int4 ret = PealCall(m, p_core_menu, NULL);
    return (int) ByteSwap32(ret);
}

int core_alpha_menu() {
    int4 ret = PealCall(m, p_core_alpha_menu, NULL);
    return (int) ByteSwap32(ret);
}

int core_hex_menu() {
    int4 ret = PealCall(m, p_core_hex_menu, NULL);
    return (int) ByteSwap32(ret);
}

int core_keydown(int key, int *enqueued, int *repeat) {
    arg_core_keydown arg;
    arg.key = ByteSwap16(key);
    int4 ret = PealCall(m, p_core_keydown, &arg);
    *enqueued = ByteSwap16(arg.enqueued);
    *repeat = ByteSwap16(arg.repeat);
    return (int) ByteSwap32(ret);
}

int core_repeat() {
    int4 ret = PealCall(m, p_core_repeat, NULL);
    return (int) ByteSwap32(ret);
}

void core_keytimeout1() {
    PealCall(m, p_core_keytimeout1, NULL);
}

void core_keytimeout2() {
    PealCall(m, p_core_keytimeout2, NULL);
}

void core_timeout3(int repaint) {
    int4 p = repaint;
    p = ByteSwap32(p);
    PealCall(m, p_core_timeout3, (void *) p);
}

int core_keyup() {
    int4 ret = PealCall(m, p_core_keyup, NULL);
    return (int) ByteSwap32(ret);
}

int core_allows_powerdown(int *want_cpu) {
    struct {
	int4 x;
    } int_aligner;
    int4 ret = (int) PealCall(m, p_core_allows_powerdown, &int_aligner.x);
    *want_cpu = (int) ByteSwap32(int_aligner.x);
    return (int) ByteSwap32(ret);
}

int core_powercycle() {
    int4 ret = PealCall(m, p_core_powercycle, NULL);
    return (int) ByteSwap32(ret);
}

int core_list_programs(char *buf, int bufsize) {
    arg_core_list_programs arg;
    arg.buf = (char *) ByteSwap32(buf);
    arg.bufsize = ByteSwap16(bufsize);
    int4 ret = PealCall(m, p_core_list_programs, &arg);
    return (int) ByteSwap32(ret);
}

int core_export_programs(int count, const int *indexes,
			 int (*progress_report)(const char *)) {
    arg_core_export_programs arg;
    arg.count = ByteSwap16(count);
    arg.indexes = (const int2 *) ByteSwap32(indexes);
    // TODO: progress report callback
    int4 ret = PealCall(m, p_core_export_programs, &arg);
    return (int) ByteSwap32(ret);
}

void core_import_programs(int (*progress_report)(const char *)) {
    // TODO: progress report callback
    PealCall(m, p_core_import_programs, NULL);
}

void core_copy(char *buf, int buflen) {
    arg_core_copy arg;
    arg.buf = (char *) ByteSwap32(buf);
    arg.buflen = ByteSwap16(buflen);
    PealCall(m, p_core_copy, &arg);
}

void core_paste(const char *s) {
    const char *ss = (const char *) ByteSwap32(s);
    PealCall(m, p_core_paste, (void *) ss);
}

void redisplay() {
    PealCall(m, p_redisplay, NULL);
}

void squeak() {
    PealCall(m, p_squeak, NULL);
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

static int4 arm_shell_get_mem() {
    return shell_get_mem();
}

static int4 arm_shell_low_battery() {
    return shell_low_battery();
}

static void arm_shell_powerdown() {
    shell_powerdown();
}

//static double arm_shell_random_seed();

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
