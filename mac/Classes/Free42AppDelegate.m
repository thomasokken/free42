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

#import <sys/stat.h>
#import <sys/time.h>
#import "free42.h"
#import "shell.h"
#import "shell_skin.h"
#import "core_main.h"
#import "core_display.h"
#import "Free42AppDelegate.h"


static Free42AppDelegate *instance = NULL;

state_type state;
char free42dirname[FILENAMELEN];

static int quit_flag = 0;
static int enqueued;

static char statefilename[FILENAMELEN];
static char printfilename[FILENAMELEN];
static FILE *statefile = NULL;
static char export_file_name[FILENAMELEN];
static FILE *export_file = NULL;
static FILE *import_file = NULL;

static int ckey = 0;
static int skey;
static unsigned char *macro;
static int mouse_key;
static int timeout_active = 0;
static int timeout3_active = 0;

static int ann_updown = 0;
static int ann_shift = 0;
static int ann_print = 0;
static int ann_run = 0;
//static int ann_battery = 0;
static int ann_g = 0;
static int ann_rad = 0;

static void show_message(char *title, char *message);
static void init_shell_state(int4 ver);
static int read_shell_state(int4 *ver);
static int write_shell_state();

static void enable_reminder();
static void disable_reminder();

	
@implementation Free42AppDelegate

@synthesize mainWindow;
@synthesize printWindow;
@synthesize preferencesWindow;
@synthesize selectProgramsWindow;
@synthesize aboutWindow;

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	instance = self;

	/*****************************************************/
	/***** Try to create the $HOME/.free42 directory *****/
	/*****************************************************/
	
	int free42dir_exists = 0;
	char *home = getenv("HOME");
	struct stat st;
	char keymapfilename[FILENAMELEN];
	
	snprintf(free42dirname, FILENAMELEN, "%s/.free42", home);
	if (stat(free42dirname, &st) == -1 || !S_ISDIR(st.st_mode)) {
		mkdir(free42dirname, 0755);
		if (stat(free42dirname, &st) == 0 && S_ISDIR(st.st_mode))
			free42dir_exists = 1;
	} else
		free42dir_exists = 1;
	
	if (free42dir_exists) {
		snprintf(statefilename, FILENAMELEN, "%s/.free42/state", home);
		snprintf(printfilename, FILENAMELEN, "%s/.free42/print", home);
		snprintf(keymapfilename, FILENAMELEN, "%s/.free42/keymap", home);
	} else {
		statefilename[0] = 0;
		printfilename[0] = 0;
		keymapfilename[0] = 0;
	}
	
	
	/****************************/
	/***** Read the key map *****/
	/****************************/
	
	// TODO!
	
	
	/***********************************************************/
	/***** Open the state file and read the shell settings *****/
	/***********************************************************/
	
	int4 version;
	int init_mode;
	if (free42dir_exists)
		statefile = fopen(statefilename, "r");
	else
		statefile = NULL;
	if (statefile != NULL) {
		if (read_shell_state(&version)) {
			init_mode = 1;
		} else {
			init_shell_state(-1);
			init_mode = 2;
		}
	} else {
		init_shell_state(-1);
		init_mode = 0;
	}
	
	long win_width, win_height;
	skin_load(&win_width, &win_height);
	NSSize sz;
	sz.width = win_width;
	sz.height = win_height;
	[mainWindow setContentSize:sz];
	
	if (state.mainWindowKnown) {
		NSPoint pt;
		pt.x = state.mainWindowX;
		pt.y = state.mainWindowY;
		[mainWindow setFrameOrigin:pt];
	}
	
	[mainWindow makeKeyAndOrderFront:self];
	
	core_init(init_mode, version);
	if (statefile != NULL) {
		fclose(statefile);
		statefile = NULL;
	}
	if (core_powercycle())
		enable_reminder();
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
	state.mainWindowX = (int) mainWindow.frame.origin.x;
	state.mainWindowY = (int) mainWindow.frame.origin.y;
	state.mainWindowKnown = 1;
}

- (void)repeater_callback {
	// TODO!
}

- (void)timeout1_callback {
	// TODO!
}
	
- (void)timeout2_callback {
	// TODO!
}

- (void)timeout3_callback {
	bool keep_running = core_timeout3(1);
    timeout3_active = 0;
    if (keep_running)
        enable_reminder();
}

@end

static void shell_keydown() {
    int repeat, keep_running;
    if (skey == -1)
        skey = skin_find_skey(ckey);
    skin_set_pressed_key(skey);
    if (timeout3_active && (macro != NULL || ckey != 28 /* KEY_SHIFT */)) {
		[NSObject cancelPreviousPerformRequestsWithTarget:instance selector:@selector(timeout3_callback) object:NULL];
        timeout3_active = 0;
        core_timeout3(0);
    }
	
    if (macro != NULL) {
        if (*macro == 0) {
            squeak();
            return;
        }
        bool one_key_macro = macro[1] == 0 || (macro[2] == 0 && macro[0] == 28);
        if (!one_key_macro)
            skin_display_set_enabled(false);
        while (*macro != 0) {
            keep_running = core_keydown(*macro++, &enqueued, &repeat);
            if (*macro != 0 && !enqueued)
                core_keyup();
        }
        if (!one_key_macro) {
            skin_display_set_enabled(true);
            skin_repaint_display();
			/*
            skin_repaint_annunciator(1, ann_updown);
            skin_repaint_annunciator(2, ann_shift);
            skin_repaint_annunciator(3, ann_print);
            skin_repaint_annunciator(4, ann_run);
            skin_repaint_annunciator(5, ann_battery);
            skin_repaint_annunciator(6, ann_g);
            skin_repaint_annunciator(7, ann_rad);
			*/
            repeat = 0;
        }
    } else
        keep_running = core_keydown(ckey, &enqueued, &repeat);
	
    if (quit_flag)
        [NSApp terminate];
    if (keep_running)
        enable_reminder();
    else {
        disable_reminder();
        if (timeout_active) {
            [NSObject cancelPreviousPerformRequestsWithTarget:instance selector:@selector(timeout1_callback) object:NULL];
            [NSObject cancelPreviousPerformRequestsWithTarget:instance selector:@selector(timeout2_callback) object:NULL];
            [NSObject cancelPreviousPerformRequestsWithTarget:instance selector:@selector(repeater_callback) object:NULL];
			timeout_active = 0;
		}
        if (repeat != 0) {
			[instance performSelector:@selector(repeater_callback) withObject:NULL afterDelay:(repeat == 1 ? 1.0 : 0.5)];
            timeout_active = 1;
        } else if (!enqueued) {
			[instance performSelector:@selector(timeout1_callback) withObject:NULL afterDelay:0.25];
            timeout_active = 1;
        }
    }
}

static void shell_keyup() {
    skin_set_pressed_key(-1);
    ckey = 0;
    skey = -1;
    if (timeout_active) {
		[NSObject cancelPreviousPerformRequestsWithTarget:instance selector:@selector(timeout1_callback) object:NULL];
		[NSObject cancelPreviousPerformRequestsWithTarget:instance selector:@selector(timeout2_callback) object:NULL];
		[NSObject cancelPreviousPerformRequestsWithTarget:instance selector:@selector(repeater_callback) object:NULL];
        timeout_active = 0;
    }
    if (!enqueued) {
        int keep_running = core_keyup();
        if (quit_flag)
            [NSApp terminate];
        if (keep_running)
            enable_reminder();
        else
            disable_reminder();
    }
}

void calc_mousedown(int x, int y) {
	skin_find_key(x, y, ann_shift != 0, &skey, &ckey);
	if (ckey != 0) {
		macro = skin_find_macro(ckey);
		shell_keydown();
		mouse_key = 1;
	}
}

void calc_mouseup() {
	if (ckey != 0 && mouse_key)
		shell_keyup();
}

static void enable_reminder() {
	// TODO!
}

static void disable_reminder() {
	// TODO!
}

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
	skin_display_blitter(bits, bytesperline, x, y, width, height);
}

void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad) {
	if (updn != -1 && ann_updown != updn) {
		ann_updown = updn;
		skin_update_annunciator(1, ann_updown);
	}
	if (shf != -1 && ann_shift != shf) {
		ann_shift = shf;
		skin_update_annunciator(2, ann_shift);
	}
	if (prt != -1 && ann_print != prt) {
		ann_print = prt;
		skin_update_annunciator(3, ann_print);
	}
	if (run != -1 && ann_run != run) {
		ann_run = run;
		skin_update_annunciator(4, ann_run);
	}
	if (g != -1 && ann_g != g) {
		ann_g = g;
		skin_update_annunciator(6, ann_g);
	}
	if (rad != -1 && ann_rad != rad) {
		ann_rad = rad;
		skin_update_annunciator(7, ann_rad);
	}
}

void shell_powerdown() {
	quit_flag = 1;
}

void shell_print(const char *text, int length,
				 const char *bits, int bytesperline,
				 int x, int y, int width, int height) {
	// TODO!
}

void shell_request_timeout3(int delay) {
	[NSObject cancelPreviousPerformRequestsWithTarget:instance selector:@selector(timeout3_callback) object:NULL];
	[instance performSelector:@selector(timeout3_callback) withObject:NULL afterDelay:(delay / 1000.0)];
	timeout3_active = 1;
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

static void init_shell_state(int4 version) {
    switch (version) {
        case -1:
            state.printerToTxtFile = 0;
            state.printerToGifFile = 0;
            state.printerTxtFileName[0] = 0;
            state.printerGifFileName[0] = 0;
            state.printerGifMaxLength = 256;
            state.mainWindowKnown = 0;
            state.printWindowKnown = 0;
            state.skinName[0] = 0;
            /* fall through */
        case 0:
            /* current version (SHELL_VERSION = 4),
             * so nothing to do here since everything
             * was initialized from the state file.
             */
            ;
    }
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
		init_shell_state(-1);
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
	
	init_shell_state(state_version);
	*ver = version;
	return 1;
}

static int write_shell_state() {
	int4 magic = FREE42_MAGIC;
	int4 version = FREE42_VERSION;
	int4 state_size = sizeof(state_type);
	int4 state_version = SHELL_VERSION;
	
	if (!shell_write_saved_state(&magic, sizeof(int4)))
		return 0;
	if (!shell_write_saved_state(&version, sizeof(int4)))
		return 0;
	if (!shell_write_saved_state(&state_size, sizeof(int4)))
		return 0;
	if (!shell_write_saved_state(&state_version, sizeof(int4)))
		return 0;
	if (!shell_write_saved_state(&state, sizeof(state_type)))
		return 0;
	
	return 1;
}