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

#import <sys/time.h>
#import "Free42AppDelegate.h"
#import "free42.h"
#import "shell.h"

#define FILENAMELEN 256

static Free42AppDelegate *delegate = NULL;

static char statefilename[FILENAMELEN];
static FILE *statefile = NULL;
static char export_file_name[FILENAMELEN];
static FILE *export_file = NULL;
static FILE *import_file = NULL;

static void show_message(char *title, char *message);

	
@implementation Free42AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	NSLog(@"Application did finish launching!!!");
	delegate = self;
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
	NSLog(@"Application will terminate!!!");
}

@end


static void show_message(char *title, char *message) {
	// TODO!
	fprintf(stderr, "%s\n", message);
}

double shell_random_seed() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((tv.tv_sec * 1000000L + tv.tv_usec) & 0xffffffffL) / 4294967296.0;
}

void shell_beeper(int frequency, int duration) {
	// TODO!
	NSBeep();
}

int shell_low_battery() {
	// TODO!
	return 0;
}

uint4 shell_milliseconds() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint4) (tv.tv_sec * 1000L + tv.tv_usec / 1000);
}

void shell_delay(int duration) {
	struct timespec ts;
	ts.tv_sec = duration / 1000;
	ts.tv_nsec = (duration % 1000) * 1000000;
	nanosleep(&ts, NULL);
}

uint4 shell_get_mem() {
	// TODO!
	return 42;
}

void shell_blitter(const char *bits, int bytesperline, int x, int y, int width, int height) {
	// TODO!
}

void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad) {
	// TODO!
}

void shell_powerdown() {
	// TODO!
}

void shell_print(const char *text, int length,
				 const char *bits, int bytesperline,
				 int x, int y, int width, int height) {
	// TODO!
}

void shell_request_timeout3(int delay) {
	// TODO!
}

shell_bcd_table_struct *shell_get_bcd_table() {
	return NULL;
}

shell_bcd_table_struct *shell_put_bcd_table(shell_bcd_table_struct *bcdtab,
                                            uint4 size) {
	return bcdtab;
}

void shell_release_bcd_table(shell_bcd_table_struct *bcdtab) {
	free(bcdtab);
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
		show_message("Message", "An error occurred; import was terminated prematurely.");
		return -1;
	} else
		return nread;
}

int shell_wants_cpu() {
	// TODO!
	static bool want = false;
	want = !want;
	return want ? 1 : 0;
}
