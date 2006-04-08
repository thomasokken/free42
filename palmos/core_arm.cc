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
#include "endianutils.h"
#include "pealstub.h"

#include "shell.h"
#include "core_arm.h"
#include "core_main.h"
#include "core_display.h"


////////////////////////////////
///// Calls FROM the shell /////
////////////////////////////////

unsigned long arm_core_init(void *p) {
    arg_core_init *arg = (arg_core_init *) p;
    core_init(arg->read_state, arg->version);
    return 0;
}

unsigned long arm_core_quit(void *p) {
    core_quit();
    return 0;
}

unsigned long arm_core_repaint_display(void *p) {
    core_repaint_display();
    return 0;
}

unsigned long arm_core_menu(void *p) {
    return core_menu();
}

unsigned long arm_core_alpha_menu(void *p) {
    return core_alpha_menu();
}

unsigned long arm_core_hex_menu(void *p) {
    return core_hex_menu();
}

unsigned long arm_core_keydown(void *p) {
    arg_core_keydown *arg = (arg_core_keydown *) p;
    int enqueued, repeat;
    int ret = core_keydown(arg->key, &enqueued, &repeat);
    arg->enqueued = enqueued;
    arg->repeat = repeat;
    return ret;
}

unsigned long arm_core_repeat(void *p) {
    return core_repeat();
}

unsigned long arm_core_keytimeout1(void *p) {
    core_keytimeout1();
    return 0;
}

unsigned long arm_core_keytimeout2(void *p) {
    core_keytimeout2();
    return 0;
}

unsigned long arm_core_timeout3(void *p) {
    core_timeout3((int) p);
    return 0;
}

unsigned long arm_core_keyup(void *p) {
    return core_keyup();
}

unsigned long arm_core_allows_powerdown(void *p) {
    return core_allows_powerdown((int *) p);
}

unsigned long arm_core_powercycle(void *p) {
    return core_powercycle();
}

unsigned long arm_core_list_programs(void *p) {
    arg_core_list_programs *arg = (arg_core_list_programs *) p;
    return core_list_programs(arg->buf, arg->bufsize);
}

unsigned long arm_core_export_programs(void *p) {
    // TODO: progress report callback
    arg_core_export_programs *arg = (arg_core_export_programs *) p;
    int indexes[100];
    if (arg->count > 100)
	arg->count = 100;
    for (int i = 0; i < arg->count; i++)
	indexes[i] = arg->indexes[i];
    return core_export_programs(arg->count, indexes, NULL);
}

unsigned long arm_core_import_programs(void *p) {
    // TODO: progress report callback
    core_import_programs(NULL);
    return 0;
}

unsigned long arm_core_copy(void *p) {
    arg_core_copy *arg = (arg_core_copy *) p;
    core_copy(arg->buf, arg->buflen);
    return 0;
}

unsigned long arm_core_paste(void *p) {
    core_paste((const char *) p);
    return 0;
}


//////////////////////////////
///// Calls TO the shell /////
//////////////////////////////

unsigned long p_shell_blitter;
unsigned long p_shell_beeper;
unsigned long p_shell_annunciators;
unsigned long p_shell_wants_cpu;
unsigned long p_shell_delay;
unsigned long p_shell_request_timeout3;
unsigned long p_shell_read_saved_state;
unsigned long p_shell_write_saved_state;
unsigned long p_shell_get_mem;
unsigned long p_shell_low_battery;
unsigned long p_shell_powerdown;
unsigned long p_shell_random_seed;
unsigned long p_shell_milliseconds;
unsigned long p_shell_print;
unsigned long p_shell_write;
unsigned long p_shell_read;
unsigned long p_shell_get_bcd_table;
unsigned long p_shell_put_bcd_table;
unsigned long p_shell_release_bcd_table;

void shell_blitter(const char *bits, int bytesperline, int x, int y,
			     int width, int height) {
    arg_shell_blitter arg;
    arg.bits = (const char *) ByteSwap32(bits);
    arg.bytesperline = ByteSwap16(bytesperline);
    arg.x = ByteSwap16(x);
    arg.y = ByteSwap16(y);
    arg.width = ByteSwap16(width);
    arg.height = ByteSwap16(height);
    unsigned long pptr = ByteSwap32(&arg);
    gCall68KFuncP(gEmulStateP, p_shell_blitter, &pptr, 4);
}

void shell_beeper(int frequency, int duration) {
    arg_shell_beeper arg;
    arg.frequency = ByteSwap16(frequency);
    arg.duration = ByteSwap16(duration);
    unsigned long pptr = ByteSwap32(&arg);
    gCall68KFuncP(gEmulStateP, p_shell_beeper, &pptr, 4);
}

void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad) {
    arg_shell_annunciators arg;
    arg.updn = ByteSwap16(updn);
    arg.shf = ByteSwap16(shf);
    arg.prt = ByteSwap16(prt);
    arg.run = ByteSwap16(run);
    arg.g = ByteSwap16(g);
    arg.rad = ByteSwap16(rad);
    unsigned long pptr = ByteSwap32(&arg);
    gCall68KFuncP(gEmulStateP, p_shell_beeper, &pptr, 4);
}

int shell_wants_cpu() {
    int4 ret = gCall68KFuncP(gEmulStateP, p_shell_wants_cpu, NULL, 0);
    return (int) ByteSwap32(ret);
}

void shell_delay(int duration) {
    int4 p = duration;
    p = ByteSwap32(p);
    gCall68KFuncP(gEmulStateP, p_shell_delay, &p, 4);
}

void shell_request_timeout3(int delay) {
    int4 p = delay;
    p = ByteSwap32(p);
    gCall68KFuncP(gEmulStateP, p_shell_request_timeout3, &p, 4);
}

int4 shell_read_saved_state(void *buf, int4 bufsize) {
    arg_shell_read_saved_state arg;
    arg.buf = (void *) ByteSwap32(buf);
    arg.bufsize = ByteSwap32(bufsize);
    unsigned long pptr = ByteSwap32(&arg);
    int4 ret = gCall68KFuncP(gEmulStateP, p_shell_read_saved_state, &pptr, 4);
    return ByteSwap32(ret);
}

bool shell_write_saved_state(const void *buf, int4 nbytes) {
    arg_shell_write_saved_state arg;
    arg.buf = (const void *) ByteSwap32(buf);
    arg.nbytes = ByteSwap32(nbytes);
    unsigned long pptr = ByteSwap32(&arg);
    int4 ret = gCall68KFuncP(gEmulStateP, p_shell_write_saved_state, &pptr, 4);
    return (bool) ByteSwap32(ret);
}

int4 shell_get_mem() {
    int4 ret = gCall68KFuncP(gEmulStateP, p_shell_get_mem, NULL, 0);
    return (bool) ByteSwap32(ret);
}

int shell_low_battery() {
    int4 ret = gCall68KFuncP(gEmulStateP, p_shell_low_battery, NULL, 0);
    return (int) ByteSwap32(ret);
}

void shell_powerdown() {
    gCall68KFuncP(gEmulStateP, p_shell_powerdown, NULL, 0);
}

double shell_random_seed() {
    // TODO: how are doubles returned?
    return 0;
}

uint4 shell_milliseconds() {
    uint4 ret = gCall68KFuncP(gEmulStateP, p_shell_milliseconds, NULL, 0);
    return ByteSwap32(ret);
}

void shell_print(const char *text, int length,
		 const char *bits, int bytesperline,
		 int x, int y, int width, int height) {
    arg_shell_print arg;
    arg.text = (const char *) ByteSwap32(text);
    arg.length = ByteSwap16(length);
    arg.bits = (const char *) ByteSwap32(bits);
    arg.bytesperline = ByteSwap16(bytesperline);
    arg.x = ByteSwap16(x);
    arg.y = ByteSwap16(y);
    arg.width = ByteSwap16(width);
    arg.height = ByteSwap16(height);
    unsigned long pptr = ByteSwap32(&arg);
    gCall68KFuncP(gEmulStateP, p_shell_print, &pptr, 4);
}

int shell_write(const char *buf, int4 buflen) {
    arg_shell_write arg;
    arg.buf = (const char *) ByteSwap32(buf);
    arg.buflen = ByteSwap32(buflen);
    unsigned long pptr = ByteSwap32(&arg);
    int4 ret = gCall68KFuncP(gEmulStateP, p_shell_write, &pptr, 4);
    return (int) ByteSwap32(ret);
}

int4 shell_read(char *buf, int4 buflen) {
    arg_shell_write arg;
    arg.buf = (char *) ByteSwap32(buf);
    arg.buflen = ByteSwap32(buflen);
    unsigned long pptr = ByteSwap32(&arg);
    int4 ret = gCall68KFuncP(gEmulStateP, p_shell_read, &pptr, 4);
    return ByteSwap32(ret);
}

shell_bcd_table_struct *shell_get_bcd_table() {
    int4 ret = gCall68KFuncP(gEmulStateP, p_shell_get_bcd_table, NULL, 0 | kPceNativeWantA0);
    return (shell_bcd_table_struct *) ByteSwap32(ret);
}

shell_bcd_table_struct *shell_put_bcd_table(shell_bcd_table_struct *bcdtab,
					    uint4 size) {
    arg_shell_put_bcd_table arg;
    arg.bcdtab = (void *) ByteSwap32(bcdtab);
    arg.size = ByteSwap32(size);
    unsigned long pptr = ByteSwap32(&arg);
    int4 ret = gCall68KFuncP(gEmulStateP, p_shell_put_bcd_table, &pptr, 4 | kPceNativeWantA0);
    return (shell_bcd_table_struct *) ByteSwap32(ret);
}

void shell_release_bcd_table(shell_bcd_table_struct *bcdtab) {
    unsigned long pptr = ByteSwap32(bcdtab);
    gCall68KFuncP(gEmulStateP, p_shell_release_bcd_table, &pptr, 4);
}
