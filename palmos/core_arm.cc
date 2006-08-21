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
#include <Core/CoreTraps.h>

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
    arg_core_export_programs *arg = (arg_core_export_programs *) p;
    return core_export_programs(arg->count, arg->indexes, NULL);
}

unsigned long arm_core_import_programs(void *p) {
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

unsigned long arm_redisplay(void *p) {
    redisplay();
    return 0;
}

unsigned long arm_squeak(void *p) {
    squeak();
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
unsigned long p_malloc;
unsigned long p_realloc;
unsigned long p_free;
#ifdef DEBUG
unsigned long p_logtofile;
unsigned long p_lognumber;
unsigned long p_logdouble;
#endif

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
    gCall68KFuncP(gEmulStateP, p_shell_annunciators, &pptr, 4);
}

int shell_wants_cpu() {
    // I'm cheating a bit for the sake of speed. Since this function is called
    // after EVERY program line, the cost of an ARM-to-68K call adds up pretty
    // quickly. Calling EvtEventAvail() directly saves about 50%; only calling
    // it every 10th time saves another 90% (it also means that a program may
    // continue to run 9 steps too far after the user presses R/S -- oh, well).
    static int n = 0;
    if (++n == 10) {
	n = 0;
	return gCall68KFuncP(gEmulStateP, PceNativeTrapNo(sysTrapEvtEventAvail), NULL, 0);
    } else
	return 0;
    //return (int) gCall68KFuncP(gEmulStateP, p_shell_wants_cpu, NULL, 0);
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
    return (int4) gCall68KFuncP(gEmulStateP, p_shell_read_saved_state, &pptr, 4);
}

bool shell_write_saved_state(const void *buf, int4 nbytes) {
    arg_shell_write_saved_state arg;
    arg.buf = (const void *) ByteSwap32(buf);
    arg.nbytes = ByteSwap32(nbytes);
    unsigned long pptr = ByteSwap32(&arg);
    return (bool) gCall68KFuncP(gEmulStateP, p_shell_write_saved_state, &pptr, 4);
}

uint4 shell_get_mem() {
    return (uint4) gCall68KFuncP(gEmulStateP, p_shell_get_mem, NULL, 0);
}

int shell_low_battery() {
    return (int) gCall68KFuncP(gEmulStateP, p_shell_low_battery, NULL, 0);
}

void shell_powerdown() {
    gCall68KFuncP(gEmulStateP, p_shell_powerdown, NULL, 0);
}

double shell_random_seed() {
    double *s = (double *) gCall68KFuncP(gEmulStateP, p_shell_random_seed, NULL, 0 | kPceNativeWantA0);
    union {
	double d;
	struct {
	    int4 x, y;
	} i;
    } u;
    u.d = *s;
    u.i.x = ByteSwap32(u.i.x);
    u.i.y = ByteSwap32(u.i.y);
    return u.d;
}

uint4 shell_milliseconds() {
    return (uint4) gCall68KFuncP(gEmulStateP, p_shell_milliseconds, NULL, 0);
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
    return (int) gCall68KFuncP(gEmulStateP, p_shell_write, &pptr, 4);
}

int4 shell_read(char *buf, int4 buflen) {
    arg_shell_write arg;
    arg.buf = (char *) ByteSwap32(buf);
    arg.buflen = ByteSwap32(buflen);
    unsigned long pptr = ByteSwap32(&arg);
    return (int4) gCall68KFuncP(gEmulStateP, p_shell_read, &pptr, 4);
}

shell_bcd_table_struct *shell_get_bcd_table() {
    return (shell_bcd_table_struct *) gCall68KFuncP(gEmulStateP, p_shell_get_bcd_table, NULL, 0 | kPceNativeWantA0);
}

shell_bcd_table_struct *shell_put_bcd_table(shell_bcd_table_struct *bcdtab,
					    uint4 size) {
    arg_shell_put_bcd_table arg;
    arg.bcdtab = (void *) ByteSwap32(bcdtab);
    arg.size = ByteSwap32(size);
    unsigned long pptr = ByteSwap32(&arg);
    return (shell_bcd_table_struct *) gCall68KFuncP(gEmulStateP, p_shell_put_bcd_table, &pptr, 4 | kPceNativeWantA0);
}

void shell_release_bcd_table(shell_bcd_table_struct *bcdtab) {
    unsigned long pptr = ByteSwap32(bcdtab);
    gCall68KFuncP(gEmulStateP, p_shell_release_bcd_table, &pptr, 4);
}

void *malloc(size_t size) {
    size_t p = size;
    p = ByteSwap32(p);
    return (void *) gCall68KFuncP(gEmulStateP, p_malloc, &p, 4);
}

void *realloc(void *ptr, size_t size) {
    arg_realloc arg;
    arg.ptr = (void *) ByteSwap32(ptr);
    arg.size = ByteSwap32(size);
    unsigned long pptr = ByteSwap32(&arg);
    return (void *) gCall68KFuncP(gEmulStateP, p_realloc, &pptr, 4);
}

void free(void *ptr) {
    int4 p = (int4) ptr;
    p = ByteSwap32(p);
    gCall68KFuncP(gEmulStateP, p_free, &p, 4);
}

#ifdef DEBUG
void logtofile(const char *message) {
    int4 p = (int4) message;
    p = ByteSwap32(p);
    gCall68KFuncP(gEmulStateP, p_logtofile, &p, 4);
}

void lognumber(int4 num) {
    int4 p = (int4) num;
    p = ByteSwap32(p);
    gCall68KFuncP(gEmulStateP, p_lognumber, &p, 4);
}

void logdouble(double num) {
    union {
	double d;
	struct {
	    int4 x, y;
	} i;
    } u;
    u.d = num;
    u.i.x = ByteSwap32(u.i.x);
    u.i.y = ByteSwap32(u.i.y);
    int4 p = (int4) &u;
    p = ByteSwap32(p);
    gCall68KFuncP(gEmulStateP, p_logdouble, &p, 4);
}
#endif


//////////////////////////////////////
///// Cloned from shell_spool.cc /////
//////////////////////////////////////

int hp2ascii(char *dst, const char *src, int srclen) {
    char *esc;
    unsigned char c;
    int s, d = 0;
    for (s = 0; s < srclen; s++) {
	c = src[s];
	if (c >= 130 && c != 138)
	    c &= 127;
	switch (c) {
	    /* NOTE: this code performs the following 12 translations
	     * that are not ASCII, but seem to be widely accepted --
	     * that is, they looked OK when I tried them in several
	     * fonts in Windows and Linux, and in Memo Pad on the Palm:
	     *
	     *   0: 247 (0367) divide
	     *   1: 215 (0327) multiply
	     *   8: 191 (0277) upside-down question mark
	     *  17: 181 (0265) lowercase mu
	     *  18: 163 (0243) sterling
	     *  19: 176 (0260) degree
	     *  20: 197 (0305) Aring
	     *  21: 209 (0321) Ntilde
	     *  22: 196 (0304) Aumlaut
	     *  25: 198 (0306) AE
	     *  28: 214 (0326) Oumlaut
	     *  29: 220 (0334) Uumlaut
	     *
	     * Two additional candidates are these:
	     *
	     *  26: 133 (0205) ellipsis
	     *  31: 149 (0225) bullet
	     *
	     * I'm not using those last two because support for them is not
	     * as good: they exist in Times New Roman and Adobe Courier
	     * (tested on Windows and Linux, respectively) and on the Palm,
	     * but are missing from Windows Fixedsys (the default Notepad
	     * font, so that is a concern!) and X11 lucidatypewriter and
	     * fixed.
	     * Note that 133 and 149 are both in the 128-159 range, that
	     * is, the Ctrl+Meta range, which is unused in many fonts.
	     * Eventually, I should probably support several translation
	     * modes: raw, pure ASCII (only emit codes 32-126 and 10),
	     * non-pure as below, and more aggressive non-pure (using the
	     * ellipsis and fatdot codes, and maybe others). Then again,
	     * maybe not. :-)
	     */
	    case  0:   esc = "\367"; break;
	    case  1:   esc = "\327"; break;
	    case  2:   esc = "\\sqrt"; break;
	    case  3:   esc = "\\int"; break;
	    case  4:   esc = "\\gray1"; break;
	    case  5:   esc = "\\Sigma"; break;
	    case  6:   esc = ">"; break;
	    case  7:   esc = "\\pi"; break;
	    case  8:   esc = "\277"; break;
	    case  9:   esc = "<="; break;
	    case 11:   esc = ">="; break;
	    case 12:   esc = "!="; break;
	    case 13:   esc = "\\r"; break;
	    case 14:   esc = "v"; break;
	    case 15:   esc = "->"; break;
	    case 16:   esc = "<-"; break;
	    case 17:   esc = "\265"; break;
	    case 18:   esc = "\243"; break;
	    case 19:   esc = "\260"; break;
	    case 20:   esc = "\305"; break;
	    case 21:   esc = "\321"; break;
	    case 22:   esc = "\304"; break;
	    case 23:   esc = "\\angle"; break;
	    case 24:   esc = "E"; break;
	    case 25:   esc = "\306"; break;
	    case 26:   esc = "..."; break;
	    case 27:   esc = "\\esc"; break;
	    case 28:   esc = "\326"; break;
	    case 29:   esc = "\334"; break;
	    case 30:   esc = "\\gray2"; break;
	    case 31:   esc = "\\bullet"; break;
	    case '\\': esc = "\\\\"; break;
	    case 127:  esc = "|-"; break;
	    case 128:  esc = ":"; break;
	    case 129:  esc = "y"; break;
	    case 138:  esc = "\\LF"; break;
	    default:   dst[d++] = c; continue;
	}
	while (*esc != 0)
	    dst[d++] = *esc++;
    }
    return d;
}
