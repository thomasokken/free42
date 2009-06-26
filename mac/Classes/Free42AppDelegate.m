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
#import <pthread.h>
#import "free42.h"
#import "shell.h"
#import "shell_skin.h"
#import "core_main.h"
#import "core_display.h"
#import "Free42AppDelegate.h"
#import "ProgramListDataSource.h"


static Free42AppDelegate *instance = NULL;

state_type state;
char free42dirname[FILENAMELEN];

static int quit_flag = 0;
static int enqueued;
static int keep_running = 0;
static int we_want_cpu = 0;
static bool is_running = false;
static pthread_mutex_t is_running_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t is_running_cond = PTHREAD_COND_INITIALIZER;

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

static bool timeout_active = false;
static int timeout_which;
static bool timeout3_active = false;
static bool repeater_active = false;

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
static void shell_keydown();
static void shell_keyup();

@implementation Free42AppDelegate

@synthesize mainWindow;
@synthesize calcView;
@synthesize printWindow;
@synthesize preferencesWindow;
@synthesize selectProgramsWindow;
@synthesize programListView;
@synthesize programListDataSource;
@synthesize aboutWindow;
@synthesize aboutVersion;
@synthesize aboutCopyright;

- (void) startRunner {
	[self performSelectorInBackground:@selector(runner) withObject:NULL];
}

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
		[self startRunner];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
	state.mainWindowX = (int) mainWindow.frame.origin.x;
	state.mainWindowY = (int) mainWindow.frame.origin.y;
	state.mainWindowKnown = 1;
    statefile = fopen(statefilename, "w");
    if (statefile != NULL)
        write_shell_state();
    core_quit();
    if (statefile != NULL)
        fclose(statefile);
}

- (IBAction) showAbout:(id)sender {
	const char *version = [Free42AppDelegate getVersion];
	[aboutVersion setStringValue:[NSString stringWithFormat:@"Free42 %s", version]];
	[aboutCopyright setStringValue:@"© 2004-2009 Thomas Okken"];
	[aboutWindow makeKeyAndOrderFront:self];
}

- (IBAction) showPreferences:(id)sender {
	[preferencesWindow makeKeyAndOrderFront:self];
}

- (IBAction) importPrograms:(id)sender {
	NSOpenPanel* openDlg = [NSOpenPanel openPanel];
	[openDlg setCanChooseFiles:YES];
	[openDlg setCanChooseDirectories:NO];
	if ([openDlg runModalForDirectory:nil file:nil] == NSOKButton) {
		NSArray* files = [openDlg filenames];
		for (int i = 0; i < [files count]; i++) {
			NSString* fileName = [files objectAtIndex:i];
			char cFileName[1024];
			[fileName getCString:cFileName maxLength:1024 encoding:NSUTF8StringEncoding];
			import_file = fopen(cFileName, "r");
			if (import_file == NULL) {
				char buf[1000];
				int err = errno;
				snprintf(buf, 1000, "Could not open \"%s\" for reading:\n%s (%d)",
						 cFileName, strerror(err), err);
				show_message("Message", buf);
			} else {
				core_import_programs(NULL);
				redisplay();
				if (import_file != NULL) {
					fclose(import_file);
					import_file = NULL;
				}
			}
		}
	}
}

- (IBAction) exportPrograms:(id)sender {
	char buf[10000];
	int count = core_list_programs(buf, 10000);
	[programListDataSource setProgramNames:buf count:count];
	[programListView reloadData];
	[selectProgramsWindow makeKeyAndOrderFront:self];
}

- (IBAction) exportProgramsCancel:(id)sender {
	[selectProgramsWindow orderOut:self];
}

- (IBAction) exportProgramsOK:(id)sender {
	[selectProgramsWindow orderOut:self];
	bool *selection = [programListDataSource getSelection];
	int count = [programListDataSource numberOfRowsInTableView:nil];
	NSSavePanel *saveDlg = [NSSavePanel savePanel];
	if ([saveDlg runModalForDirectory:nil file:nil] == NSOKButton) {
		NSString *fileName = [saveDlg filename];
		char cFileName[1024];
		[fileName getCString:cFileName maxLength:1024 encoding:NSUTF8StringEncoding];
		export_file = fopen(cFileName, "w");
		if (export_file == NULL) {
			char buf[1000];
			int err = errno;
			snprintf(buf, 1000, "Could not open \"%s\" for writing:\n%s (%d)",
					 cFileName, strerror(err), err);
			show_message("Message", buf);
		} else {
			int *indexes = (int *) malloc(count * sizeof(int));
			int selectionSize = 0;
			for (int i = 0; i < count; i++)
				if (selection[i])
					indexes[selectionSize++] = i;
			core_export_programs(selectionSize, indexes, NULL);
			free(indexes);
			if (export_file != NULL) {
				fclose(export_file);
				export_file = NULL;
			}
		}
	}
}

- (IBAction) doCopy:(id)sender {
	NSPasteboard *pb = [NSPasteboard generalPasteboard];
	NSArray *types = [NSArray arrayWithObjects: NSStringPboardType, nil];
	[pb declareTypes:types owner:self];
	char buf[100];
	core_copy(buf, 100);
	NSString *txt = [NSString stringWithCString:buf encoding:NSUTF8StringEncoding];
	[pb setString:txt forType:NSStringPboardType];
}

- (IBAction) doPaste:(id)sender {
	NSPasteboard *pb = [NSPasteboard generalPasteboard];
	NSArray *types = [NSArray arrayWithObjects: NSStringPboardType, nil];
	NSString *bestType = [pb availableTypeFromArray:types];
	if (bestType != nil) {
		NSString *txt = [pb stringForType:NSStringPboardType];
		char buf[100];
		[txt getCString:buf maxLength:100 encoding:NSUTF8StringEncoding];
		core_paste(buf);
		redisplay();
	}
}

static char version[32] = "";

+ (const char *) getVersion {
	if (version[0] == 0) {
		NSString *path = [[NSBundle mainBundle] pathForResource:@"VERSION" ofType:nil];
		const char *cpath = [path cStringUsingEncoding:NSUTF8StringEncoding];
		FILE *vfile = fopen(cpath, "r");
		fscanf(vfile, "%s", version);
		fclose(vfile);
	}
	return version;
}

- (IBAction) menuNeedsUpdate:(NSMenu *)menu {
	skin_menu_update(menu);
}

- (void) selectSkin:(id)sender {
	NSMenuItem *item = (NSMenuItem *) sender;
	NSString *name = [item title];
	[name getCString:state.skinName maxLength:FILENAMELEN encoding:NSUTF8StringEncoding];
	long w, h;
	skin_load(&w, &h);
	core_repaint_display();
	NSSize sz;
	sz.width = w;
	sz.height = h;
	NSRect frame = [mainWindow frame];
	NSPoint p;
	p.x = frame.origin.x;
	p.y = frame.origin.y + frame.size.height;
	[mainWindow setContentSize:sz];
	[mainWindow setFrameTopLeftPoint:p];
}

- (void) mouseDown3 {
	macro = skin_find_macro(ckey);
	shell_keydown();
	mouse_key = 1;
}

- (void) mouseDown2 {
	we_want_cpu = 1;
	pthread_mutex_lock(&is_running_mutex);
	while (is_running)
		pthread_cond_wait(&is_running_cond, &is_running_mutex);
	pthread_mutex_unlock(&is_running_mutex);
	we_want_cpu = 0;
	[self performSelectorOnMainThread:@selector(mouseDown3) withObject:NULL waitUntilDone:NO];
}

- (void) mouseUp3 {
	shell_keyup();
}

- (void) mouseUp2 {
	we_want_cpu = 1;
	pthread_mutex_lock(&is_running_mutex);
	while (is_running)
		pthread_cond_wait(&is_running_cond, &is_running_mutex);
	pthread_mutex_unlock(&is_running_mutex);
	we_want_cpu = 0;
	[self performSelectorOnMainThread:@selector(mouseUp3) withObject:NULL waitUntilDone:NO];
}

- (void) runner {
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	int dummy1, dummy2;
	is_running = true;
	keep_running = core_keydown(0, &dummy1, &dummy2);
	pthread_mutex_lock(&is_running_mutex);
	is_running = false;
	pthread_cond_signal(&is_running_cond);
	pthread_mutex_unlock(&is_running_mutex);
	if (quit_flag)
		[self performSelectorOnMainThread:@selector(quit) withObject:NULL waitUntilDone:NO];
	else if (keep_running && !we_want_cpu)
		[self performSelectorOnMainThread:@selector(startRunner) withObject:NULL waitUntilDone:NO];
	[pool release];
}

- (void) setTimeout:(int) which {
	[NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(timeout_callback) object:NULL];
	timeout_which = which;
	timeout_active = true;
	[self performSelector:@selector(timeout_callback) withObject:NULL afterDelay:(which == 1 ? 0.25 : 1.75)];
}

- (void) cancelTimeout {
	[NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(timeout_callback) object:NULL];
	timeout_active = false;
}

- (void) timeout_callback {
	timeout_active = false;
	if (ckey != 0) {
		if (timeout_which == 1) {
			core_keytimeout1();
			[self setTimeout:2];
		} else if (timeout_which == 2) {
			core_keytimeout2();
		}
	}
}

- (void) cancelTimeout3 {
	[NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(timeout3_callback) object:NULL];
	timeout3_active = false;
}

- (void) setTimeout3: (int) delay {
	[self cancelTimeout3];
	[self performSelector:@selector(timeout3_callback) withObject:NULL afterDelay:(delay / 1000.0)];
	timeout3_active = true;
}

- (void) timeout3_callback {
	timeout3_active = false;
	keep_running = core_timeout3(1);
	if (keep_running)
		[self startRunner];
}

- (void) setRepeater: (int) delay {
	[NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(repeater_callback) object:NULL];
	[self performSelector:@selector(repeater_callback) withObject:NULL afterDelay:(delay / 1000.0)];
	repeater_active = true;
}

- (void) cancelRepeater {
	[NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(repeater_callback) object:NULL];
	repeater_active = false;
}

- (void) repeater_callback {
	int repeat = core_repeat();
	if (repeat != 0)
		[self setRepeater:(repeat == 1 ? 200 : 100)];
	else
		[self setTimeout:1];
}

// The following is some wrapper code, to allow functions called by core_keydown()
// while it is running in the background, to run in the main thread.

static union {
	struct {
		int delay;
	} shell_request_timeout3_args;
	struct {
		const char *text;
		int length;
		const char *bits;
		int bytesperline;
		int x, y, width, height;
	} shell_print_args;
} helper_args;

static pthread_mutex_t shell_helper_mutex = PTHREAD_MUTEX_INITIALIZER;

- (void) shell_request_timeout3_helper {
	[self setTimeout3:helper_args.shell_request_timeout3_args.delay];
	pthread_mutex_unlock(&shell_helper_mutex);
}

- (void) shell_print_helper {
	// TODO
	pthread_mutex_unlock(&shell_helper_mutex);
}

@end

static void shell_keydown() {
    int repeat;
    if (skey == -1)
		skey = skin_find_skey(ckey);
	skin_set_pressed_key(skey);
    if (timeout3_active && (macro != NULL || ckey != 28 /* KEY_SHIFT */)) {
		[instance cancelTimeout3];
		core_timeout3(0);
    }
	
	// We temporarily set we_want_cpu to 'true', to force the calls
	// to core_keydown() in this function to return quickly. This is
	// necessary since this function runs on the main thread, and we
	// can't peek ahead in the event queue while core_keydown() is
	// hogging the CPU on the main thread. (The lack of something like
	// EventAvail is an annoying omission of the iPhone API.)
	
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
			we_want_cpu = 1;
			keep_running = core_keydown(*macro++, &enqueued, &repeat);
			we_want_cpu = 0;
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
    } else {
		we_want_cpu = 1;
		keep_running = core_keydown(ckey, &enqueued, &repeat);
		we_want_cpu = 0;
	}
	
    if (quit_flag)
		[NSApp terminate];
    else if (keep_running)
		[instance startRunner];
    else {
		[instance cancelTimeout];
		[instance cancelRepeater];
		if (repeat != 0)
			[instance setRepeater:(repeat == 1 ? 1000 : 500)];
		else if (!enqueued)
			[instance setTimeout:1];
    }
}

static void shell_keyup() {
	skin_set_pressed_key(-1);
    ckey = 0;
    skey = -1;
	[instance cancelTimeout];
	[instance cancelRepeater];
    if (!enqueued) {
		keep_running = core_keyup();
		if (quit_flag)
			[NSApp terminate];
		else if (keep_running)
			[instance startRunner];
    } else if (keep_running) {
		[instance startRunner];
	}
}

void calc_mousedown(int x, int y) {
	if (ckey == 0) {
		skin_find_key(x, y, ann_shift != 0, &skey, &ckey);
		if (ckey != 0) {
			if (is_running)
				[instance performSelectorInBackground:@selector(mouseDown2) withObject:NULL];
			else
				[instance mouseDown3];
		}
	}
}

void calc_mouseup() {
	if (ckey != 0 && mouse_key) {
		if (is_running)
			[instance performSelectorInBackground:@selector(mouseUp2) withObject:NULL];
		else
			[instance mouseUp3];
	}
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
	pthread_mutex_lock(&shell_helper_mutex);
	helper_args.shell_request_timeout3_args.delay = delay;
	[instance performSelectorOnMainThread:@selector(shell_request_timeout3_helper) withObject:NULL waitUntilDone:NO];
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
	return we_want_cpu;
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