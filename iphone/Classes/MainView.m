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
#include <sys/sysctl.h>

#import "MainView.h"
#import "free42.h"
#import "core_main.h"
#import "shell.h"
#import "shell_skin_iphone.h"


///////////////////////////////////////////////////////////////////////////////
/////                         Ye olde C stuphphe                          /////
///////////////////////////////////////////////////////////////////////////////

static void initialize2();

static bool initialized;
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

///////////////////////////////////////////////////////////////////////////////
/////                    Ende ophphe ye olde C stuphphe                   /////
///////////////////////////////////////////////////////////////////////////////


@implementation MainView


- (id)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Note: this does not get called when instantiated from a nib file,
		// so don't bother doing anything here!
    }
    return self;
}


- (void)drawRect:(CGRect)rect {
	if (!initialized)
		[self initialize];
	skin_repaint();
	/*
	CGContextRef myContext = UIGraphicsGetCurrentContext();
	CGFloat color[4];
	color[0] = (random() & 255) / 255.0;
	color[1] = (random() & 255) / 255.0;
	color[2] = (random() & 255) / 255.0;
	color[3] = 1.0;
	CGContextSetFillColor(myContext, color);
	CGContextFillEllipseInRect(myContext, CGRectMake(10, 10, 300, 300));
	*/
}


- (void) dealloc {
	NSLog(@"Shutting down!");
    [super dealloc];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
	[super touchesBegan:touches withEvent:event];
	[self setNeedsDisplay];
}

- (void) initialize {
	initialize2();
	initialized = true;
	if (skin_name == nil) {
		skin_name = @"Realistic";
		long w, h;
		skin_load(skin_name, &w, &h);
		skin_width = w;
		skin_height = h;
	}	
}

@end

///////////////////////////////////////////////////////////////////////////////
/////                   Here beginneth thy olde C code                    /////
///////////////////////////////////////////////////////////////////////////////

static int read_shell_state(int *version);
static void init_shell_state(int version);

#define SHELL_VERSION 0

static FILE* statefile;

static int ann_updown = 0;
static int ann_shift = 0;
static int ann_print = 0;
static int ann_run = 0;
static int ann_battery = 0;
static int ann_g = 0;
static int ann_rad = 0;

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
	if (statefile != NULL)
		fclose(statefile);
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

void shell_blitter(const char *bits, int bytesperline, int x, int y,
				   int width, int height) {
	// TODO
}

void shell_beeper(int frequency, int duration) {
	// TODO
}

void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad) {
    if (updn != -1 && ann_updown != updn) {
		ann_updown = updn;
		skin_repaint_annunciator(1, ann_updown);
    }
    if (shf != -1 && ann_shift != shf) {
		ann_shift = shf;
		skin_repaint_annunciator(2, ann_shift);
    }
    if (prt != -1 && ann_print != prt) {
		ann_print = prt;
		skin_repaint_annunciator(3, ann_print);
    }
    if (run != -1 && ann_run != run) {
		ann_run = run;
		skin_repaint_annunciator(4, ann_run);
    }
    if (g != -1 && ann_g != g) {
		ann_g = g;
		skin_repaint_annunciator(6, ann_g);
    }
    if (rad != -1 && ann_rad != rad) {
		ann_rad = rad;
		skin_repaint_annunciator(7, ann_rad);
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
	// TODO
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
	// TODO
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
