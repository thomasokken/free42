/* shell.cc: simple stdio shell
 * Use this (or as a template) for simple Model View Control implementations
 * 
 * Copyright (c) 2005 D.Jeff Dionne
 * Copyright (c) 2004-2012  Thomas Okken
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
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include "shell.h"
#include "shell_spool.h"
#include "core_main.h"
#include "core_display.h"

#define SHELL_VERSION 1
#define KEYMAP_MAX_MACRO_LENGTH 16
typedef struct {
    int ctrl;
    int alt;
    int shift; 
    int keycode;
    unsigned char macro[KEYMAP_MAX_MACRO_LENGTH];
} keymap_entry;

static int schedule_timeout3;
static int we_want_cpu;
static char *statefilename;
static char *export_file_name;
static FILE *statefile;
static FILE *export_file;
static FILE *import_file;
static int quit_flag;

static int keymap_length = 0;
static keymap_entry *keymap = NULL;

typedef struct state {
 int ann_updown;
 int ann_shift;
 int ann_print;
 int ann_run;
 int ann_g;
 int ann_rad;
} state_t;

static state_t state;

// My own implementation of fgets, using a file descriptor rather than
// a FILE*. I'm using this because I want to be able to  monitor the input
// stream using select(2) to prevent blocking reads -- and I'm not sure if
// it's kosher to mix fgets(3) and select(2) on the same stream.
static int fdgets(char *buf, int len, int fildes) {
    if (len < 2)
	return -1;
    int n = 0;
    while (n < len - 1) {
	int c = read(fildes, buf + n, 1);
	if (c == 0)
	    // EOF
	    if (n == 0)
		return -1;
	    else {
		buf[n] = 0;
		return fildes;
	    }
	if (c == -1)
	    // error
	    return -1;
	if (buf[n] == 10) {
	    buf[n] = 0;
	    return fildes;
	}
	n++;
    }
    buf[n] = 0;
    return fildes;
}

static int input_pending() {
    fd_set readfds, errorfds;
    FD_ZERO(&readfds);
    FD_SET(0, &readfds);
    FD_ZERO(&errorfds);
    FD_SET(0, &errorfds);
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;
    return select(1, &readfds, NULL, &errorfds, &timeout);
}

void
flush()
{
    char buf[64];

    fflush(stdout);
    fdgets(buf, sizeof(buf), 0);
}

static void show_message(char *msg_type, char *msg)
{
}

static FILE *gif_file;

static void gif_writer(const char *text, int length) {
    fwrite(text, length, 1, gif_file);
}

static void gif_seeker(int pos) {
    fseek(gif_file, pos, SEEK_SET);
}

void shell_blitter(const char *bits, int bytesperline, int x, int y,
			     int width, int height)
{
    gif_file = fopen("display.gif", "w");
    shell_start_gif(gif_writer, 131, 16);
    shell_spool_gif(bits, bytesperline, 0, 0, 131, 16, gif_writer);
    shell_finish_gif(gif_seeker, gif_writer);
    fclose(gif_file);

    printf("d");
    flush();

    char *buf = core_copy();
    printf("x%s", buf);
    flush();
    free(buf);
}

void shell_beeper(int frequency, int duration)
{
}

void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad)
{
    if (updn != -1 && updn != state.ann_updown) {
	printf("%cAnnUpDown", (state.ann_updown = updn) ? 'A' : 'a');
	flush();
    }
    if (shf != -1 && shf != state.ann_shift) {
	printf("%cAnnShift", (state.ann_shift = shf) ? 'A' : 'a');
	flush();
    }
    if (prt != -1 && prt != state.ann_print) {
	printf("%cAnnPrint", (state.ann_print = prt) ? 'A' : 'a');
	flush();
    }
    if (run != -1 && run != state.ann_run) {
	printf("%cAnnRun", (state.ann_run = run) ? 'A' : 'a');
	flush();
    }
    if (g != -1 && g != state.ann_g) {
	printf("%cAnnG", (state.ann_g = g) ? 'A' : 'a');
	flush();
    }
    if (rad != -1 && rad != state.ann_rad) {
	printf("%cAnnRAD", (state.ann_rad = rad) ? 'A' : 'a');
	flush();
    }
}

int shell_wants_cpu()
{
    return input_pending();
}

void shell_delay(int duration)
{
    struct timespec ts;
    ts.tv_sec = duration / 1000;
    ts.tv_nsec = (duration % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void shell_request_timeout3(int delay)
{
    schedule_timeout3 = delay;
}

int4 shell_read_saved_state(void *buf, int4 bufsize) {
    if (statefile == NULL)
	return -1;
    else {
	int4 n = fread(buf, 1, bufsize, statefile);
	if (n != bufsize && ferror(statefile)) {
	    fclose(statefile);
	    statefile = NULL;
	    return -1;
	} else
	    return n;
    }
}

bool shell_write_saved_state(const void *buf, int4 nbytes) {
    if (statefile == NULL)
	return false;
    else {
	int4 n = fwrite(buf, 1, nbytes, statefile);
	if (n != nbytes) {
	    fclose(statefile);
	    remove(statefilename);
	    statefile = NULL;
	    return false;
	} else
	    return true;
    }
}

uint4 shell_get_mem()
{
    return 65536; /* 64k free memory, always */
}

int shell_low_battery()
{
    return 0;
}

void shell_powerdown() {
    /* We defer the actual shutdown so the emulator core can
     * return from core_keyup() or core_keydown() and isn't
     * asked to save its state while still in the middle of
     * executing the OFF instruction...
     */
    quit_flag = 1;
}

int8 shell_random_seed() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000LL + tv.tv_usec / 1000;
}

uint4 shell_milliseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint4) (tv.tv_sec * 1000L + tv.tv_usec / 1000);
}

void shell_get_time_date(uint4 *time, uint4 *date, int *weekday) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tms;
    localtime_r(&tv.tv_sec, &tms);
    if (time != NULL)
	*time = ((tms.tm_hour * 100 + tms.tm_min) * 100 + tms.tm_sec) * 100 + tv.tv_usec / 10000;
    if (date != NULL)
	*date = ((tms.tm_year + 1900) * 100 + tms.tm_mon + 1) * 100 + tms.tm_mday;
    if (weekday != NULL)
	*weekday = tms.tm_wday;
}

int shell_decimal_point() {
    return 1;
}

int shell_write(const char *buf, int4 buflen) {
    int4 written;
    if (export_file == NULL)
	return 0;
    written = fwrite(buf, 1, buflen, export_file);
    if (written != buflen) {
	char buf[1000];
	fclose(export_file);
	export_file = NULL;
	snprintf(buf, 1000, "Writing \"%s\" failed.", export_file_name);
	show_message("Message", buf);
	return 0;
    } else
	return 1;
}

int shell_read(char *buf, int4 buflen) {
    int4 nread;
    if (import_file == NULL)
	return -1;
    nread = fread(buf, 1, buflen, import_file);
    if (nread != buflen && ferror(import_file)) {
	fclose(import_file);
	import_file = NULL;
	show_message("Message",
		"An error occurred; import was terminated prematurely.");
	return -1;
    } else
	return nread;
}

void shell_print(const char *text, int length,
		 const char *bits, int bytesperline,
		 int x, int y, int width, int height) {
}

static FILE *logfile = NULL;

void shell_log(const char *message) {
    if (logfile == NULL)
        logfile = fopen("/tmp/free42-widget.log", "w");
    fprintf(logfile, "%s\n", message);
    fflush(logfile);
}

keymap_entry *parse_keymap_entry(char *line, int lineno) {
    char *p;
    static keymap_entry entry;

    p =  strchr(line, '#');
    if (p != NULL)
	*p = 0;
    p = strchr(line, '\n');
    if (p != NULL)
	*p = 0;
    p = strchr(line, '\r');
    if (p != NULL)
	*p = 0;

    p = strchr(line, ':');
    if (p != NULL) {
	char *val = p + 1;
	char *tok;
	int ctrl = 0;
	int alt = 0;
	int shift = 0;
	int keycode = 0;
	int done = 0;
	unsigned char macro[KEYMAP_MAX_MACRO_LENGTH];
	int macrolen = 0;

	/* Parse keycode */
	*p = 0;
	tok = strtok(line, " \t");
	while (tok != NULL) {
	    if (done) {
		fprintf(stderr, "Keymap, line %d: Excess tokens in key spec.\n", lineno);
		return NULL;
	    }
	    if (strcasecmp(tok, "ctrl") == 0)
		ctrl = 1;
	    else if (strcasecmp(tok, "alt") == 0)
		alt = 1;
	    else if (strcasecmp(tok, "shift") == 0)
		shift = 1;
	    else {
		char *endptr;
		long k = strtol(tok, &endptr, 10);
		if (k < 1 || *endptr != 0) {
		    fprintf(stderr, "Keymap, line %d: Bad keycode.\n", lineno);
		    return NULL;
		}
		keycode = k;
		done = 1;
	    }
	    tok = strtok(NULL, " \t");
	}
	if (!done) {
	    fprintf(stderr, "Keymap, line %d: Unrecognized keycode.\n", lineno);
	    return NULL;
	}

	/* Parse macro */
	tok = strtok(val, " \t");
	memset(macro, 0, KEYMAP_MAX_MACRO_LENGTH);
	while (tok != NULL) {
	    char *endptr;
	    long k = strtol(tok, &endptr, 10);
	    if (*endptr != 0 || k < 1 || k > 255) {
		fprintf(stderr, "Keymap, line %d: Bad value (%s) in macro.\n", lineno, tok);
		return NULL;
	    } else if (macrolen == KEYMAP_MAX_MACRO_LENGTH) {
		fprintf(stderr, "Keymap, line %d: Macro too long (max=%d).\n", lineno, KEYMAP_MAX_MACRO_LENGTH);
		return NULL;
	    } else
		macro[macrolen++] = (unsigned char) k;
	    tok = strtok(NULL, " \t");
	}

	entry.ctrl = ctrl;
	entry.alt = alt;
	entry.shift = shift;
	entry.keycode = keycode;
	memcpy(entry.macro, macro, KEYMAP_MAX_MACRO_LENGTH);
	return &entry;
    } else
	return NULL;
}

static void read_key_map() {
    FILE *keymapfile = fopen("keymap.txt", "r");
    int kmcap = 0;
    char line[1024];
    int lineno = 0;

    if (keymapfile == NULL) return;

    while (fgets(line, 1024, keymapfile) != NULL) {
    keymap_entry *entry = parse_keymap_entry(line, ++lineno);
    if (entry == NULL)
	continue;
    /* Create new keymap entry */
    if (keymap_length == kmcap) {
	kmcap += 50;
	keymap = (keymap_entry *) realloc(keymap, kmcap * sizeof(keymap_entry));
	// TODO - handle memory allocation failure
    }
    memcpy(keymap + (keymap_length++), entry, sizeof(keymap_entry));
    }

    fclose(keymapfile);
}

unsigned char *skin_keymap_lookup(int keycode, int ctrl, int alt, int shift) {
    int i;
    for (i = 0; i < keymap_length; i++) {
	keymap_entry *entry = keymap + i;
	if (keycode == entry->keycode
		&& ctrl == entry->ctrl
		&& alt == entry->alt
		&& shift == entry->shift)
	    return entry->macro;
    }
    return NULL;
}

static int read_shell_state(int4 *ver) {
    int4 magic;
    int4 version;
    int4 state_size;
    int4 state_version;

    if (shell_read_saved_state(&magic, sizeof(int4)) != sizeof(int4))
    return 0;
    if (magic != FREE42_MAGIC)
    return 0;

    if (shell_read_saved_state(&version, sizeof(int4)) != sizeof(int4))
    return 0;
    if (version == 0) {
    /* State file version 0 does not contain shell state,
     * only core state, so we just hard-init the shell.
     */
    *ver = version;
    return 1;
    } else if (version > FREE42_VERSION)
    /* Unknown state file version */
    return 0;

    if (shell_read_saved_state(&state_size, sizeof(int4)) != sizeof(int4))
    return 0;
    if (shell_read_saved_state(&state_version, sizeof(int4)) != sizeof(int4))
    return 0;
    if (state_version < 0 || state_version > SHELL_VERSION)
    /* Unknown shell state version */
    return 0;
    if (shell_read_saved_state(&state, state_size) != state_size)
    return 0;

    *ver = version;
    return 1;
}

static int write_shell_state() {
    int4 magic = FREE42_MAGIC;
    int4 version = FREE42_VERSION;
    int4 state_size = sizeof(state_t);
    int4 state_version = SHELL_VERSION;

    if (!shell_write_saved_state(&magic, sizeof(int4)))
    return 0;
    if (!shell_write_saved_state(&version, sizeof(int4)))
    return 0;
    if (!shell_write_saved_state(&state_size, sizeof(int4)))
    return 0;
    if (!shell_write_saved_state(&state_version, sizeof(int4)))
    return 0;
    if (!shell_write_saved_state(&state, sizeof(state_t)))
    return 0;

    return 1;
}

void
cleanup()
{
    statefile = fopen("state", "w");

    write_shell_state();
    core_quit();
    fclose(statefile);
}

void
sighand(int)
{
    exit(0);
}

int
main(int argc, char *argv[])
{
    char cmd[65536];
    unsigned char *macro;
    int keyno, ctrl, alt, shift;
    int repeat;
    int queue;
    int version;
    int keep_running;

    if ((statefile = fopen("state", "r"))) {
	if (read_shell_state(&version)) {
	    core_init(1, version);
	} else core_init(0,8);
	fclose(statefile);
    } else core_init(0, 8);
    keep_running = core_powercycle();

    signal(SIGPIPE, sighand);
    signal(SIGHUP, sighand);
    signal(SIGINT, sighand);
    atexit(cleanup);

    read_key_map();

    printf("%cAnnUpDown", state.ann_updown ? 'A' : 'a');
    flush();
    printf("%cAnnShift", state.ann_shift ? 'A' : 'a');
    flush();
    printf("%cAnnPrint", state.ann_print ? 'A' : 'a');
    flush();
    printf("%cAnnRun", state.ann_run ? 'A' : 'a');
    flush();
    printf("%cAnnG", state.ann_g ? 'A' : 'a');
    flush();
    printf("%cAnnRAD", state.ann_rad ? 'A' : 'a');
    flush();

    while (true) {
	if (keep_running) {
	    // The core wants the CPU back, so we should avoid
	    // blocking here, so I use select() to check if there is
	    // any input pending on fd 0.
	    if (!input_pending()) {
		int dummy1, dummy2;
		keep_running = core_keydown(0, &dummy1, &dummy2);
		continue;
	    }
	}

	if (fdgets(cmd, sizeof(cmd), 0) == -1)
	    break;

	keyno=atoi(cmd + 1);
	switch(cmd[0]) {

	case 'C':
	    keep_running = core_keydown(keyno + 1, &queue, &repeat);
	    break;
	case 'U':
	    if (!queue)
		keep_running = core_keyup();
	    break;
	case 'K':
	    ctrl = (keyno & 0x10000) != 0;
	    alt = (keyno & 0x20000) != 0;
	    shift = (keyno & 0x40000) != 0;
	    keyno &= 0xffff;
	    macro = skin_keymap_lookup(keyno, ctrl, alt, shift);
	    if (macro == NULL || (macro[0] != 36 || macro[1] != 0)
		    && (macro[0] != 28 || macro[1] != 36 || macro[2] != 0)) {
		// The test above is to make sure that whatever mapping is in
		// effect for R/S will never be overridden by the special cases
		// for the ALPHA and A..F menus.
		if (!ctrl && !alt) {
		    if (core_alpha_menu() && keyno >= 32 && keyno <= 126) {
			if (keyno >= 'a' && keyno <= 'z')
			    keyno = keyno + 'A' - 'a';
			else if (keyno >= 'A' && keyno <= 'Z')
			    keyno = keyno + 'a' - 'A';
			keep_running = core_keydown(keyno + 1024, &queue, &repeat);
			if (!queue)
			    keep_running = core_keyup();
			goto done;
		    } else if (core_hex_menu() && ((keyno >= 'a' && keyno <= 'f') || (keyno >= 'A' && keyno <= 'F'))) {
			if (keyno >= 'a' && keyno <= 'f')
			    keyno -= 'a' - 1;
			else
			    keyno -= 'A' - 1;
			keep_running = core_keydown(keyno, &queue, &repeat);
			if (!queue)
			    keep_running = core_keyup();
			goto done;
		    }
		}
	    }
	    for (; macro && *macro; macro++) {
		keyno = *macro;
		keep_running = core_keydown(keyno, &queue, &repeat);
		if (!queue)
		    keep_running = core_keyup();
	    }
	    done:
	    break;
	case 'P':
            char c;
            char *p;
            for (p = cmd + 1; (c = *p) != 0; p++) {
                if (c == 31)
                    *p = 13;
                else if (c == 30)
                    *p = 10;
            }
            core_paste(cmd + 1);
	    redisplay();
	    break;
	case 'e':
	case 'E':
	    if (strstr(cmd, "invSingular")) {
		core_settings.matrix_singularmatrix = cmd[0] == 'E';
	    }
	    if (strstr(cmd, "matrixOverflow")) {
		core_settings.matrix_outofrange = cmd[0] == 'E';
	    }
	    break;
	}
    }
}
