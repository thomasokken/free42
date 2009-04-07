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

#import <UIKit/UIKit.h>
#include <sys/stat.h>
#include <sys/sysctl.h>

#import "MainView.h"
#import "free42.h"
#import "core_main.h"
#import "core_display.h"
#import "shell.h"
#import "shell_iphone.h"
#import "shell_skin_iphone.h"


///////////////////////////////////////////////////////////////////////////////
/////                         Ye olde C stuphphe                          /////
///////////////////////////////////////////////////////////////////////////////

static void initialize2();
static void quit2();
static void enable_reminder();
static void disable_reminder();
static void shell_keydown();
static void shell_keyup();

static NSString *skin_name;
static int skin_width, skin_height;

static const int FILENAMELEN = 1024;

static struct {
	int printerToTxtFile;
	int printerToGifFile;
	char printerTxtFileName[FILENAMELEN];
	char printerGifFileName[FILENAMELEN];
	int printerGifMaxLength;
	char skinName[FILENAMELEN];
} state;

static int quit_flag = 0;
static int enqueued;

static int ckey = 0;
static int skey;
static unsigned char *macro;
static int mouse_key;

#define WAIT_STATE_NONE 0
#define WAIT_STATE_TIMEOUT1 1
#define WAIT_STATE_TIMEOUT2 2
#define WAIT_STATE_TIMEOUT3 3
#define WAIT_STATE_REPEATER 4

static int wait_state = WAIT_STATE_NONE;
static bool reminder_active = false;

static int ann_updown = 0;
static int ann_shift = 0;
static int ann_print = 0;
static int ann_run = 0;
static int ann_battery = 0;
static int ann_g = 0;
static int ann_rad = 0;

///////////////////////////////////////////////////////////////////////////////
/////                    Ende ophphe ye olde C stuphphe                   /////
///////////////////////////////////////////////////////////////////////////////


static MainView *mainView = nil;

@implementation MainView


- (id)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Note: this does not get called when instantiated from a nib file,
		// so don't bother doing anything here!
    }
    return self;
}


- (void)drawRect:(CGRect)rect {
	if (mainView == nil)
		[self initialize];
	skin_repaint();
}


- (void) dealloc {
	NSLog(@"Shutting down!");
    [super dealloc];
}

- (void) touchesBegan: (NSSet *) touches withEvent: (UIEvent *) event {
	[super touchesBegan:touches withEvent:event];
	if (ckey == 0) {
		UITouch *touch = (UITouch *) [touches anyObject];
		CGPoint p = [touch locationInView:self];
		int x = (int) p.x;
		int y = (int) p.y;
		skin_find_key(x, y, ann_shift != 0, &skey, &ckey);
		if (ckey != 0) {
			[shell_iphone playSound:11];
			macro = skin_find_macro(ckey);
			shell_keydown();
			mouse_key = 1;
		}
	}
}

- (void) touchesEnded: (NSSet *) touches withEvent: (UIEvent *) event {
	[super touchesEnded:touches withEvent:event];
	if (ckey != 0 && mouse_key)
		shell_keyup();
}

- (void) touchesCancelled: (NSSet *) touches withEvent: (UIEvent *) event {
	[super touchesEnded:touches withEvent:event];
	if (ckey != 0 && mouse_key)
		shell_keyup();
}

+ (void) quit {
	quit2();
}

- (void) initialize {
	mainView = self;
	if (skin_name == nil) {
		skin_name = @"Realistic";
		long w, h;
		skin_load(skin_name, &w, &h);
		skin_width = w;
		skin_height = h;
	}	
	initialize2();
}

- (void) reminder {
	if (!reminder_active)
		return;
	int dummy1, dummy2;
	int keep_running = core_keydown(0, &dummy1, &dummy2);
	if (!keep_running)
		reminder_active = false;
	if (quit_flag)
		quit2();
	if (reminder_active)
		[self performSelectorOnMainThread:@selector(reminder) withObject:NULL waitUntilDone:NO];
}

- (void) setWaitState:(int) newState afterDelay:(NSTimeInterval)delay {
	[NSObject cancelPreviousPerformRequestsWithTarget:self];
	wait_state = newState;
	if (newState != WAIT_STATE_NONE)
		[self performSelector:@selector(waitStateExpired) withObject:NULL afterDelay:delay];
}

- (void) waitStateExpired {
	int which = wait_state;
	wait_state = WAIT_STATE_NONE;
	switch (which) {
		case WAIT_STATE_NONE:
			break;
		case WAIT_STATE_TIMEOUT1:
			if (ckey != 0) {
				core_keytimeout1();
				[self setWaitState:WAIT_STATE_TIMEOUT2 afterDelay:1.75];
			}
			break;
		case WAIT_STATE_TIMEOUT2:
			if (ckey != 0)
				core_keytimeout2();
			break;
		case WAIT_STATE_TIMEOUT3:
			core_timeout3(1);
			break;
		case WAIT_STATE_REPEATER:
			int repeat = core_repeat();
			if (repeat != 0)
				[self setWaitState:WAIT_STATE_REPEATER afterDelay:(repeat == 1 ? 0.2 : 0.1)];
			else
				[self setWaitState:WAIT_STATE_TIMEOUT1 afterDelay:0.25];
			break;
	}
}

@end

///////////////////////////////////////////////////////////////////////////////
/////                   Here beginneth thy olde C code                    /////
///////////////////////////////////////////////////////////////////////////////

static int read_shell_state(int *version);
static void init_shell_state(int version);
static int write_shell_state();

#define SHELL_VERSION 0

static FILE* statefile;

static void initialize2() {
	statefile = fopen("config/state", "r");
	int init_mode, version;
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
	core_init(init_mode, version);
	if (statefile != NULL) {
		fclose(statefile);
		statefile = NULL;
	}
}

static int read_shell_state(int *ver) {
    int magic;
    int version;
    int state_size;
    int state_version;
	
    if (shell_read_saved_state(&magic, sizeof(int)) != sizeof(int))
		return 0;
    if (magic != FREE42_MAGIC)
		return 0;
	
    if (shell_read_saved_state(&version, sizeof(int)) != sizeof(int))
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
	
    if (shell_read_saved_state(&state_size, sizeof(int)) != sizeof(int))
		return 0;
    if (shell_read_saved_state(&state_version, sizeof(int)) != sizeof(int))
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

static void init_shell_state(int version) {
    switch (version) {
        case -1:
            state.printerToTxtFile = 0;
            state.printerToGifFile = 0;
            state.printerTxtFileName[0] = 0;
            state.printerGifFileName[0] = 0;
            state.printerGifMaxLength = 256;
            state.skinName[0] = 0;
            /* fall through */
        case 0:
            /* current version (SHELL_VERSION = 0),
             * so nothing to do here since everything
             * was initialized from the state file.
             */
            ;
    }
}

static void quit2() {
	mkdir("config", 0755);
    statefile = fopen("config/state", "w");
    if (statefile != NULL)
        write_shell_state();
    core_quit();
    if (statefile != NULL)
        fclose(statefile);
	exit(0);
}

static void enable_reminder() {
	if (reminder_active)
		return;
	reminder_active = true;
	[mainView performSelectorOnMainThread:@selector(reminder) withObject:NULL waitUntilDone:NO];
}

static void disable_reminder() {
	// Can't cancel stuff that was scheduled using performSelectorOnMainThread
	// so just clearing this flag; reminder() checks it so even though we can't
	// prevent reminder() from being called, at least we prevent it from doing
	// anything.
	reminder_active = false;
}

static void shell_keydown() {
    int repeat, keep_running;
    if (skey == -1)
		skey = skin_find_skey(ckey);
	skin_set_pressed_key(skey, mainView);
    if (wait_state == WAIT_STATE_TIMEOUT3 && (macro != NULL || ckey != 28 /* KEY_SHIFT */)) {
		[mainView setWaitState:WAIT_STATE_NONE afterDelay:0];
		[NSObject cancelPreviousPerformRequestsWithTarget:mainView];
		core_timeout3(0);
    }
	
    if (macro != NULL) {
		if (*macro == 0) {
			squeak();
			return;
		}
		bool one_key_macro = macro[1] == 0 || (macro[2] == 0 && macro[0] == 28);
		if (!one_key_macro) {
			skin_display_set_enabled(false);
		}
		while (*macro != 0) {
			keep_running = core_keydown(*macro++, &enqueued, &repeat);
			if (*macro != 0 && !enqueued)
				core_keyup();
		}
		if (!one_key_macro) {
			skin_display_set_enabled(true);
			skin_repaint_display(mainView);
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
		quit2();
    if (keep_running)
		enable_reminder();
    else {
		disable_reminder();
		if (repeat != 0)
			[mainView setWaitState:WAIT_STATE_REPEATER afterDelay:(repeat == 1 ? 1.0 : 0.5)];
		else if (!enqueued)
			[mainView setWaitState:WAIT_STATE_TIMEOUT1 afterDelay:0.25];
		else
			[mainView setWaitState:WAIT_STATE_NONE afterDelay:0];
    }
}

static void shell_keyup() {
	skin_set_pressed_key(-1, mainView);
    ckey = 0;
    skey = -1;
	[mainView setWaitState:WAIT_STATE_NONE afterDelay:0];
    if (!enqueued) {
		int keep_running = core_keyup();
		if (quit_flag)
			quit2();
		if (keep_running)
			enable_reminder();
		else
			disable_reminder();
    }
}

static int write_shell_state() {
    int magic = FREE42_MAGIC;
    int version = FREE42_VERSION;
    int state_size = sizeof(state);
    int state_version = SHELL_VERSION;
	
    if (!shell_write_saved_state(&magic, sizeof(int)))
        return 0;
    if (!shell_write_saved_state(&version, sizeof(int)))
        return 0;
    if (!shell_write_saved_state(&state_size, sizeof(int)))
        return 0;
    if (!shell_write_saved_state(&state_version, sizeof(int)))
        return 0;
    if (!shell_write_saved_state(&state, sizeof(state)))
        return 0;
	
    return 1;
}

void shell_blitter(const char *bits, int bytesperline, int x, int y, int width, int height) {
	skin_display_blitter(bits, bytesperline, x, y, width, height, mainView);
}

void shell_beeper(int frequency, int duration) {
	const int cutoff_freqs[] = { 164, 220, 243, 275, 293, 324, 366, 418, 438, 550 };
	for (int i = 0; i < 10; i++) {
		if (frequency <= cutoff_freqs[i]) {
			[shell_iphone playSound:i];
			shell_delay(250);
			return;
		}
	}
	[shell_iphone playSound:10];
	shell_delay(125);
}

void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad) {
	NSLog(@"shell_annunciators(updn=%d, shf=%d, prt=%d, run=%d, g=%d, rad=%d", updn, shf, prt, run, g, rad);
    if (updn != -1 && ann_updown != updn) {
		ann_updown = updn;
		skin_update_annunciator(1, ann_updown, mainView);
    }
    if (shf != -1 && ann_shift != shf) {
		ann_shift = shf;
		skin_update_annunciator(2, ann_shift, mainView);
    }
    if (prt != -1 && ann_print != prt) {
		ann_print = prt;
		skin_update_annunciator(3, ann_print, mainView);
    }
    if (run != -1 && ann_run != run) {
		ann_run = run;
		skin_update_annunciator(4, ann_run, mainView);
    }
    if (g != -1 && ann_g != g) {
		ann_g = g;
		skin_update_annunciator(6, ann_g, mainView);
    }
    if (rad != -1 && ann_rad != rad) {
		ann_rad = rad;
		skin_update_annunciator(7, ann_rad, mainView);
    }
}

int shell_wants_cpu() {
	// TODO
	return 0;
}

void shell_delay(int duration) {
    struct timespec ts;
    ts.tv_sec = duration / 1000;
    ts.tv_nsec = (duration % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void shell_request_timeout3(int delay) {
	[mainView setWaitState:WAIT_STATE_TIMEOUT3 afterDelay:(delay / 1000.0)];
}

int shell_read_saved_state(void *buf, int bufsize) {
    if (statefile == NULL)
		return -1;
    else {
		int n = fread(buf, 1, bufsize, statefile);
		if (n != bufsize && ferror(statefile)) {
			fclose(statefile);
			statefile = NULL;
			return -1;
		} else
			return n;
    }
}

bool shell_write_saved_state(const void *buf, int nbytes) {
    if (statefile == NULL)
		return false;
    else {
		int n = fwrite(buf, 1, nbytes, statefile);
		if (n != nbytes) {
			fclose(statefile);
			remove("config/state");
			statefile = NULL;
			return false;
		} else
			return true;
    }
}

unsigned int shell_get_mem() {
	int mib[2];
	unsigned int memsize;
	size_t len;
	
	// Retrieve the available system memory
	
	mib[0] = CTL_HW;
	mib[1] = HW_USERMEM;
	len = sizeof(memsize);
	sysctl(mib, 2, &memsize, &len, NULL, 0);
	
	return memsize;
}

int shell_low_battery() {
	// TODO
	return 0;
}

void shell_powerdown() {
	quit_flag = 1;
}

double shell_random_seed() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((tv.tv_sec * 1000000L + tv.tv_usec) & 0xffffffffL) / 4294967296.0;
}

unsigned int shell_milliseconds() {
	struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned int) (tv.tv_sec * 1000L + tv.tv_usec / 1000);
}

void shell_print(const char *text, int length,
				 const char *bits, int bytesperline,
				 int x, int y, int width, int height) {
	// TODO
}

int shell_write(const char *buf, int buflen) {
	return 0;
}

int shell_read(char *buf, int buflen) {
	return -1;
}

shell_bcd_table_struct *shell_get_bcd_table() {
	return NULL;
}

shell_bcd_table_struct *shell_put_bcd_table(shell_bcd_table_struct *bcdtab,
											unsigned int size) {
	return bcdtab;
}

void shell_release_bcd_table(shell_bcd_table_struct *bcdtab) {
	free(bcdtab);
}
