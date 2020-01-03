/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2020  Thomas Okken
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
#import <sys/stat.h>
#import <sys/sysctl.h>

#import <AudioToolbox/AudioServices.h>
#import <CoreLocation/CoreLocation.h>
#import <CoreMotion/CoreMotion.h>

#import "CalcView.h"
#import "PrintView.h"
#import "RootViewController.h"
#import "Free42AppDelegate.h"
#import "free42.h"
#import "core_main.h"
#import "core_display.h"
#import "shell.h"
#import "shell_spool.h"
#import "shell_skin_iphone.h"

// For "audio enable" flag
#import "core_globals.h"


///////////////////////////////////////////////////////////////////////////////
/////                         Ye olde C stuphphe                          /////
///////////////////////////////////////////////////////////////////////////////

#if 0
class Tracer {
private:
    const char *name;
public:
    Tracer(const char *name) {
        this->name = name;
        NSLog(@"%@ : ENTERING %s", [NSThread currentThread], name);
    }
    ~Tracer() {
        NSLog(@"%@ : EXITING %s", [NSThread currentThread], name);
    }
};
#define TRACE(x) Tracer T(x)
#else
#define TRACE(x)
#endif
        
static void quit2(bool really_quit);
static void shell_keydown();
static void shell_keyup();

static int read_shell_state(int *version);
static void init_shell_state(int version);
static int write_shell_state();

state_type state;
FILE* statefile;

static int quit_flag = 0;
static int enqueued;
static int keep_running = 0;
static int we_want_cpu = 0;

static int ckey = 0;
static int skey;
static unsigned char *macro;
static bool macro_is_name;
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
static bool ann_print_timeout_active = false;

static FILE *print_txt = NULL;
static FILE *print_gif = NULL;
static char print_gif_name[FILENAMELEN];
static int gif_seq = -1;
static int gif_lines;

static void show_message(const char *title, const char *message);
static void txt_writer(const char *text, int length);
static void txt_newliner();
static void gif_seeker(int4 pos);
static void gif_writer(const char *text, int length);
static bool is_file(const char *name);

///////////////////////////////////////////////////////////////////////////////
/////                    Ende ophphe ye olde C stuphphe                   /////
///////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
/////   Accelerometer, Location Services, and Compass support    /////
//////////////////////////////////////////////////////////////////////

static double loc_lat = 0, loc_lon = 0, loc_lat_lon_acc = -1, loc_elev = 0, loc_elev_acc = -1;
static double hdg_mag = 0, hdg_true = 0, hdg_acc = -1, hdg_x = 0, hdg_y = 0, hdg_z = 0;

@interface HardwareDelegate : NSObject <CLLocationManagerDelegate> {}
- (void)locationManager:(CLLocationManager *)manager didUpdateToLocation:(CLLocation *)newLocation fromLocation:(CLLocation *)oldLocation;
- (void)locationManager:(CLLocationManager *)manager didFailWithError:(NSError *)error;
- (void)locationManager:(CLLocationManager *)manager didUpdateHeading:(CLHeading *)newHeading;
- (BOOL)locationManagerShouldDisplayHeadingCalibration:(CLLocationManager *)manager;
@end

@implementation HardwareDelegate

- (void)locationManager:(CLLocationManager *)manager didUpdateToLocation:(CLLocation *)newLocation fromLocation:(CLLocation *)oldLocation {
    loc_lat = newLocation.coordinate.latitude;
    loc_lon = newLocation.coordinate.longitude;
    loc_lat_lon_acc = newLocation.horizontalAccuracy;
    loc_elev = newLocation.altitude;
    loc_elev_acc = newLocation.verticalAccuracy;
    NSLog(@"Location received: %g %g", loc_lat, loc_lon);
}

- (void)locationManager:(CLLocationManager *)manager didFailWithError:(NSError *)error {
    NSLog(@"Location error received: %@", [error localizedDescription]);
}

- (void)locationManager:(CLLocationManager *)manager didUpdateHeading:(CLHeading *)newHeading {
    hdg_mag = newHeading.magneticHeading;
    hdg_true = newHeading.trueHeading;
    hdg_acc = newHeading.headingAccuracy;
    hdg_x = newHeading.x;
    hdg_y = newHeading.y;
    hdg_z = newHeading.z;
    NSLog(@"Heading received: %g", hdg_mag);
}

- (BOOL)locationManagerShouldDisplayHeadingCalibration:(CLLocationManager *)manager {
    return YES;
}

@end


static CalcView *calcView = nil;

@implementation CalcView


- (id) initWithFrame:(CGRect)frame {
    TRACE("initWithFrame");
    if (self = [super initWithFrame:frame]) {
        // Note: this does not get called when instantiated from a nib file,
        // so don't bother doing anything here!
    }
    return self;
}

- (void) showMainMenu {
    UIActionSheet *menu =
    [[UIActionSheet alloc] initWithTitle:@"Main Menu"
                                delegate:self cancelButtonTitle:@"Cancel" destructiveButtonTitle:nil
                       otherButtonTitles:@"Show Print-Out", @"Program Import & Export", @"States", @"Preferences", @"Select Skin", @"Copy", @"Paste", @"About Free42", nil];
    
    [menu showInView:self];
    [menu release];
}

- (void) showImportExportMenu {
    UIActionSheet *menu =
    [[UIActionSheet alloc] initWithTitle:@"Import & Export Menu"
                                delegate:self cancelButtonTitle:@"Cancel" destructiveButtonTitle:nil
                       otherButtonTitles:@"HTTP Server", @"Import Programs", @"Export Programs", @"Share Programs", @"Back", nil];
    
    [menu showInView:self];
    [menu release];
}

- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex {
    if ([[actionSheet title] isEqualToString:@"Main Menu"]) {
        switch (buttonIndex) {
            case 0:
                // Show Print-Out
                [RootViewController showPrintOut];
                break;
            case 1:
                // Program Import & Export
                [self showImportExportMenu];
                break;
            case 2:
                // States
                [RootViewController showStates:nil];
                break;
            case 3:
                // Preferences
                [RootViewController showPreferences];
                break;
            case 4:
                // Select Skin
                [RootViewController showSelectSkin];
                break;
            case 5:
                // Copy
                [self doCopy];
                break;
            case 6:
                // Paste
                [self doPaste];
                break;
            case 7:
                // About Free42
                [RootViewController showAbout];
                break;
            case 8:
                // Cancel
                break;
        }
    } else {
        switch (buttonIndex) {
            case 0:
                // HTTP Server
                [RootViewController showHttpServer];
                break;
            case 1:
                // Import Programs
                [RootViewController doImport];
                break;
            case 2:
                // Export Programs
                [RootViewController doExport:NO];
                break;
            case 3:
                // Share Programs
                [RootViewController doExport:YES];
                break;
            case 4:
                // Back
                [self showMainMenu];
                break;
            case 5:
                // Cancel
                break;
        }
    }
}

- (void) drawRect:(CGRect)rect {
    TRACE("drawRect");
    skin_repaint(&rect);
}

- (void) dealloc {
    TRACE("dealloc");
    NSLog(@"Shutting down!");
    [super dealloc];
}

- (void) touchesBegan: (NSSet *) touches withEvent: (UIEvent *) event {
    TRACE("touchesBegan");
    [super touchesBegan:touches withEvent:event];
    UITouch *touch = (UITouch *) [touches anyObject];
    CGPoint p = [touch locationInView:self];
    int x = (int) p.x;
    int y = (int) p.y;
    if (ckey == 0) {
        skin_find_key(x, y, ann_shift != 0, &skey, &ckey);
        if (ckey == 0) {
            if (skin_in_menu_area(x, y))
                [self showMainMenu];
        } else {
            if (state.keyClicks > 0)
                [RootViewController playSound:state.keyClicks + 10];
            if (state.hapticFeedback > 0) {
                UIImpactFeedbackStyle s;
                switch (state.hapticFeedback) {
                    case 1: s = UIImpactFeedbackStyleLight; break;
                    case 2: s = UIImpactFeedbackStyleMedium; break;
                    case 3: s = UIImpactFeedbackStyleHeavy; break;
                }
                UIImpactFeedbackGenerator *fbgen = [[UIImpactFeedbackGenerator alloc] initWithStyle:s];
                [fbgen impactOccurred];
            }
            macro = skin_find_macro(ckey, &macro_is_name);
            shell_keydown();
            mouse_key = 1;
        }
    }
}

- (void) touchesEnded: (NSSet *) touches withEvent: (UIEvent *) event {
    TRACE("touchesEnded");
    [super touchesEnded:touches withEvent:event];
    if (ckey != 0 && mouse_key)
        shell_keyup();
}

- (void) touchesCancelled: (NSSet *) touches withEvent: (UIEvent *) event {
    TRACE("touchesCancelled");
    [super touchesCancelled:touches withEvent:event];
    if (ckey != 0 && mouse_key)
        shell_keyup();
}

+ (void) repaint {
    TRACE("repaint");
    [calcView setNeedsDisplay];
}

+ (void) quit {
    TRACE("quit");
    quit2(true);
}

- (void) quitB {
    TRACE("quitB");
    quit2(true);
}

- (void) layoutSubviews {
    long w, h;
    skin_load(&w, &h);
    core_repaint_display();
    [CalcView repaint];
}

+ (BOOL) isPortrait {
    return calcView.bounds.size.height > calcView.bounds.size.width;
}

+ (CGFloat) width {
    return calcView.bounds.size.width;
}

+ (CGFloat) height {
    return calcView.bounds.size.height;
}

+ (void) enterBackground {
    TRACE("enterBackground");
    quit2(false);
}

+ (void) leaveBackground {
    TRACE("leaveBackground");
    keep_running = core_powercycle();
    if (keep_running)
        [calcView startRunner];
}

- (void) doCopy {
    char *buf = core_copy();
    NSString *txt = [NSString stringWithUTF8String:buf];
    UIPasteboard *pb = [UIPasteboard generalPasteboard];
    [pb setString:txt];
    free(buf);
}

- (void) doPaste {
    UIPasteboard *pb = [UIPasteboard generalPasteboard];
    NSString *txt = [pb string];
    if (txt != nil) {
        const char *buf = [txt UTF8String];
        core_paste(buf);
    }
}

static struct timeval runner_end_time;

- (void) startRunner {
    TRACE("startRunner");
    gettimeofday(&runner_end_time, NULL);
    runner_end_time.tv_usec += 10000; // run for up to 10 ms
    if (runner_end_time.tv_usec >= 1000000) {
        runner_end_time.tv_usec -= 1000000;
        runner_end_time.tv_sec++;
    }
    int dummy1, dummy2;
    keep_running = core_keydown(0, &dummy1, &dummy2);
    if (quit_flag)
        [self quitB];
    else if (keep_running && !we_want_cpu)
        [self performSelector:@selector(startRunner) withObject:NULL afterDelay:0];
}

- (void) awakeFromNib {
    TRACE("awakeFromNib");
    [super awakeFromNib];
    calcView = self;
    statefile = fopen("config/state", "r");
    int init_mode, version;
    char core_state_file_name[FILENAMELEN];
    int core_state_file_offset;
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
    if (init_mode == 1) {
        if (version > 25) {
            snprintf(core_state_file_name, FILENAMELEN, "config/%s.f42", state.coreName);
            core_state_file_offset = 0;
        } else {
            strcpy(core_state_file_name, "config/state");
            core_state_file_offset = (int) ftell(statefile);
        }
        fclose(statefile);
    }  else {
        // The shell state was missing or corrupt, but there
        // may still be a valid core state...
        snprintf(core_state_file_name, FILENAMELEN, "config/%s.f42", state.coreName);
        struct stat st;
        if (stat(core_state_file_name, &st) == 0) {
            // Core state "Untitled.f42" exists; let's try to read it
            core_state_file_offset = 0;
            init_mode = 1;
            version = 26;
        }
    }

    long w, h;
    skin_load(&w, &h);
    
    core_init(init_mode, version, core_state_file_name, core_state_file_offset);
    keep_running = core_powercycle();
    if (keep_running)
        [self startRunner];
    if (shell_always_on(-1))
        [UIApplication sharedApplication].idleTimerDisabled = YES;
}

+ (void) loadState:(const char *)name {
    if (strcmp(name, state.coreName) != 0) {
        char corefilename[FILENAMELEN];
        snprintf(corefilename, FILENAMELEN, "config/%s.f42", state.coreName);
        core_save_state(corefilename);
    }
    core_cleanup();
    strcpy(state.coreName, name);
    char corefilename[FILENAMELEN];
    snprintf(corefilename, FILENAMELEN, "config/%s.f42", state.coreName);
    core_init(1, 26, corefilename, 0);
    if (core_powercycle())
        [calcView startRunner];
}

- (void) setTimeout:(int) which {
    TRACE("setTimeout");
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(timeout_callback) object:NULL];
    timeout_which = which;
    timeout_active = true;
    [self performSelector:@selector(timeout_callback) withObject:NULL afterDelay:(which == 1 ? 0.25 : 1.75)];
}

- (void) cancelTimeout {
    TRACE("cancelTimeout");
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(timeout_callback) object:NULL];
    timeout_active = false;
}

- (void) timeout_callback {
    TRACE("timeout_callback");
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

- (void) setTimeout3: (int) delay {
    TRACE("setTimeout3");
    [self cancelTimeout3];
    [self performSelector:@selector(timeout3_callback) withObject:NULL afterDelay:(delay / 1000.0)];
    timeout3_active = true;
}

- (void) cancelTimeout3 {
    TRACE("cancelTimeout3");
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(timeout3_callback) object:NULL];
    timeout3_active = false;
}
    
- (void) timeout3_callback {
    TRACE("timeout3_callback");
    timeout3_active = false;
    keep_running = core_timeout3(1);
    if (keep_running)
        [self startRunner];
}

- (void) setRepeater: (int) delay {
    TRACE("setRepeater");
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(repeater_callback) object:NULL];
    [self performSelector:@selector(repeater_callback) withObject:NULL afterDelay:(delay / 1000.0)];
    repeater_active = true;
}

- (void) cancelRepeater {
    TRACE("cancelRepeater");
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(repeater_callback) object:NULL];
    repeater_active = false;
}

- (void) repeater_callback {
    TRACE("repeater_callback");
    int repeat = core_repeat();
    if (repeat != 0)
        [self setRepeater:(repeat == 1 ? 200 : 100)];
    else
        [self setTimeout:1];
}

+ (void) stopTextPrinting {
    if (print_txt != NULL) {
        fclose(print_txt);
        print_txt = NULL;
    }
}

+ (void) stopGifPrinting {
    if (print_gif != NULL) {
        shell_finish_gif(gif_seeker, gif_writer);
        fclose(print_gif);
        print_gif = NULL;
        gif_seq = -1;
    }
}

static HardwareDelegate *hwDel = nil;
static CMMotionManager *motMgr = nil;
static CLLocationManager *locMgr = nil;

- (void) start_accelerometer {
    if (motMgr == nil) {
        motMgr = [[CMMotionManager alloc] init];
        motMgr.accelerometerUpdateInterval = 1;
        [motMgr startAccelerometerUpdates];
    }
}

- (void) start_location {
    if (locMgr == nil) {
        locMgr = [[CLLocationManager alloc] init];
        if (hwDel == nil)
            hwDel = [HardwareDelegate alloc];
        locMgr.delegate = hwDel;
    }
    if ([locMgr respondsToSelector:@selector(requestWhenInUseAuthorization)])
        [locMgr requestWhenInUseAuthorization];
    locMgr.distanceFilter = kCLDistanceFilterNone;
    locMgr.desiredAccuracy = kCLLocationAccuracyBest;
    [locMgr startUpdatingLocation];
}

- (void) start_heading {
    if (locMgr == NULL) {
        locMgr = [[CLLocationManager alloc] init];
        if (hwDel == NULL)
            hwDel = [HardwareDelegate alloc];
        locMgr.delegate = hwDel;
    }
    if (![CLLocationManager headingAvailable])
        return;
    locMgr.headingFilter = kCLHeadingFilterNone;
    [locMgr startUpdatingHeading];
}

- (void) turn_off_print_ann {
    ann_print = 0;
    skin_update_annunciator(3, 0, calcView);
    ann_print_timeout_active = FALSE;
}

- (void) print_ann_helper:(int)prt {
    if (ann_print_timeout_active) {
        [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(turn_off_print_ann) object:NULL];
        ann_print_timeout_active = FALSE;
    }
    if (ann_print != prt) {
        if (prt) {
            ann_print = 1;
            skin_update_annunciator(3, ann_print, calcView);
        } else {
            [self performSelector:@selector(turn_off_print_ann) withObject:NULL afterDelay:1];
            ann_print_timeout_active = TRUE;
        }
    }
}

@end

///////////////////////////////////////////////////////////////////////////////
/////                   Here beginneth thy olde C code                    /////
///////////////////////////////////////////////////////////////////////////////

extern int off_enable_flag;

static int read_shell_state(int *ver) {
    TRACE("read_shell_state");
    int magic;
    int version;
    int state_size;
    int state_version;
    
    if (fread(&magic, 1, sizeof(int), statefile) != sizeof(int))
        return 0;
    if (magic != FREE42_MAGIC)
        return 0;
    
    if (fread(&version, 1, sizeof(int), statefile) != sizeof(int))
        return 0;
    if (version == 0) {
        /* State file version 0 does not contain shell state,
         * only core state, so we just hard-init the shell.
         */
        init_shell_state(-1);
        *ver = version;
        return 1;
    }
    
    if (fread(&state_size, 1, sizeof(int), statefile) != sizeof(int))
        return 0;
    if (fread(&state_version, 1, sizeof(int), statefile) != sizeof(int))
        return 0;
    if (state_version < 0 || state_version > SHELL_VERSION)
        /* Unknown shell state version */
        return 0;
    if (fread(&state, 1, state_size, statefile) != state_size)
        return 0;
    if (state_version >= 8) {
        core_settings.matrix_singularmatrix = state.matrix_singularmatrix;
        core_settings.matrix_outofrange = state.matrix_outofrange;
        core_settings.auto_repeat = state.auto_repeat;
    }
    
    init_shell_state(state_version);
    *ver = version;
    return 1;
}

static void init_shell_state(int version) {
    TRACE("init_shell_state");
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
            /* fall through */
        case 1:
            state.alwaysOn = 0;
            /* fall through */
        case 2:
            state.keyClicks = 1;
            /* fall through */
        case 3:
            state.hapticFeedback = 0;
            /* fall through */
        case 4:
            strcpy(state.landscapeSkinName, "Landscape");
            state.orientationMode = 0;
            state.maintainSkinAspect[0] = 1;
            state.maintainSkinAspect[1] = 1;
            /* fall through */
        case 5:
            state.offEnabled = false;
            /* fall through */
        case 6:
            strcpy(state.coreName, "Untitled");
            /* fall through */
        case 7:
            core_settings.matrix_singularmatrix = false;
            core_settings.matrix_outofrange = false;
            core_settings.auto_repeat = true;
            /* fall through */
        case 8:
            /* current version (SHELL_VERSION = 8),
             * so nothing to do here since everything
             * was initialized from the state file.
             */
            ;
    }
    off_enable_flag = state.offEnabled ? 1 : 0;
}

static void quit2(bool really_quit) {
    TRACE("quit2");

    [PrintView dump];
    
    if (print_txt != NULL) {
        fclose(print_txt);
        print_txt = NULL;
    }
    
    if (print_gif != NULL) {
        shell_finish_gif(gif_seeker, gif_writer);
        fclose(print_gif);
        print_gif = NULL;
    }
    
    shell_spool_exit();

    mkdir("config", 0755);
    write_shell_state();

    char corefilename[FILENAMELEN];
    snprintf(corefilename, FILENAMELEN, "config/%s.f42", state.coreName);
    core_save_state(corefilename);
    if (really_quit) {
        //core_cleanup();
        exit(0);
    }
}

static void shell_keydown() {
    TRACE("shell_keydown");
    int repeat;
    if (skey == -1)
        skey = skin_find_skey(ckey);
    skin_set_pressed_key(skey, calcView);
    if (timeout3_active && (macro != NULL || ckey != 28 /* KEY_SHIFT */)) {
        [calcView cancelTimeout3];
        core_timeout3(0);
    }
    
    // We temporarily set we_want_cpu to 'true', to force the calls
    // to core_keydown() in this function to return quickly. This is
    // necessary since this function runs on the main thread, and we
    // can't peek ahead in the event queue while core_keydown() is
    // hogging the CPU on the main thread. (The lack of something like
    // EventAvail is an annoying omission of the iPhone API.)
        
    if (macro != NULL) {
        if (macro_is_name) {
            we_want_cpu = 1;
            keep_running = core_keydown_command((const char *) macro, &enqueued, &repeat);
            we_want_cpu = 0;
        } else {
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
                skin_repaint_display(calcView);
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
        }
    } else {
        we_want_cpu = 1;
        keep_running = core_keydown(ckey, &enqueued, &repeat);
        we_want_cpu = 0;
    }
    
    if (quit_flag)
        quit2(true);
    else if (keep_running)
        [calcView startRunner];
    else {
        [calcView cancelTimeout];
        [calcView cancelRepeater];
        if (repeat != 0)
            [calcView setRepeater:(repeat == 1 ? 1000 : 500)];
        else if (!enqueued)
            [calcView setTimeout:1];
    }
}

static void shell_keyup() {
    TRACE("shell_keyup");
    skin_set_pressed_key(-1, calcView);
    ckey = 0;
    skey = -1;
    [calcView cancelTimeout];
    [calcView cancelRepeater];
    if (!enqueued) {
        keep_running = core_keyup();
        if (quit_flag)
            quit2(true);
        else if (keep_running)
            [calcView startRunner];
    } else if (keep_running) {
        [calcView startRunner];
    }
}

static int write_shell_state() {
    TRACE("write_shell_state");
    int magic = FREE42_MAGIC;
    int version = 27;
    int state_size = sizeof(state);
    int state_version = SHELL_VERSION;

    state.offEnabled = off_enable_flag != 0;
    
    FILE *statefile = fopen("config/state", "w");
    if (statefile == NULL)
        return 0;
    if (fwrite(&magic, 1, sizeof(int), statefile) != sizeof(int))
        return 0;
    if (fwrite(&version, 1, sizeof(int), statefile) != sizeof(int))
        return 0;
    if (fwrite(&state_size, 1, sizeof(int), statefile) != sizeof(int))
        return 0;
    if (fwrite(&state_version, 1, sizeof(int), statefile) != sizeof(int))
        return 0;
    state.matrix_singularmatrix = core_settings.matrix_singularmatrix;
    state.matrix_outofrange = core_settings.matrix_outofrange;
    state.auto_repeat = core_settings.auto_repeat;
    if (fwrite(&state, 1, sizeof(state), statefile) != sizeof(state))
        return 0;
    
    fclose(statefile);
    return 1;
}

void shell_blitter(const char *bits, int bytesperline, int x, int y, int width, int height) {
    TRACE("shell_blitter");
    skin_display_blitter(bits, bytesperline, x, y, width, height, calcView);
}

const char *shell_platform() {
    static char p[16];
    strncpy(p, [Free42AppDelegate getVersion], 16);
    strncat(p, " iOS", 16);
    p[15] = 0;
    return p;
}

void shell_beeper(int frequency, int duration) {
    TRACE("shell_beeper");
    const int cutoff_freqs[] = { 164, 220, 243, 275, 293, 324, 366, 418, 438, 550 };
    for (int i = 0; i < 10; i++) {
        if (frequency <= cutoff_freqs[i]) {
            [RootViewController playSound:i];
            shell_delay(250);
            return;
        }
    }
    [RootViewController playSound:10];
    shell_delay(125);
}

void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad) {
    TRACE("shell_annunciators");
    if (updn != -1 && ann_updown != updn) {
        ann_updown = updn;
        skin_update_annunciator(1, ann_updown, calcView);
    }
    if (shf != -1 && ann_shift != shf) {
        ann_shift = shf;
        skin_update_annunciator(2, ann_shift, calcView);
    }
    if (prt != -1) {
        [calcView print_ann_helper:prt];
    }
    if (run != -1 && ann_run != run) {
        ann_run = run;
        skin_update_annunciator(4, ann_run, calcView);
    }
    if (g != -1 && ann_g != g) {
        ann_g = g;
        skin_update_annunciator(6, ann_g, calcView);
    }
    if (rad != -1 && ann_rad != rad) {
        ann_rad = rad;
        skin_update_annunciator(7, ann_rad, calcView);
    }
}

int shell_always_on(int ao) {
    int ret = state.alwaysOn;
    if (ao != -1) {
        state.alwaysOn = ao != 0;
        [UIApplication sharedApplication].idleTimerDisabled = state.alwaysOn ? YES : NO;
    }
    return ret;
}

void shell_log(const char *message) {
    NSLog(@"%s", message);
}

int shell_wants_cpu() {
    TRACE("shell_wants_cpu");
    if (we_want_cpu)
        return true;
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec > runner_end_time.tv_sec
        || now.tv_sec == runner_end_time.tv_sec && now.tv_usec >= runner_end_time.tv_usec;
}

void shell_delay(int duration) {
    TRACE("shell_delay");
    struct timespec ts;
    ts.tv_sec = duration / 1000;
    ts.tv_nsec = (duration % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void shell_request_timeout3(int delay) {
    TRACE("shell_request_timeout3");
    [calcView setTimeout3:delay];
}

unsigned int shell_get_mem() {
    TRACE("shell_get_mem");
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

void shell_powerdown() {
    TRACE("shell_powerdown");
    quit_flag = 1;
    we_want_cpu = 1;
}

int8 shell_random_seed() {
    TRACE("shell_random_seed");
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000LL + tv.tv_usec / 1000;
}

unsigned int shell_milliseconds() {
    TRACE("shell_milliseconds");
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned int) (tv.tv_sec * 1000L + tv.tv_usec / 1000);
}

int shell_decimal_point() {
    NSLocale *loc = [NSLocale currentLocale];
    NSString *dec = [loc objectForKey:NSLocaleDecimalSeparator];
    return [dec isEqualToString:@","] ? 0 : 1;
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

void shell_print(const char *text, int length,
                 const char *bits, int bytesperline,
                 int x, int y, int width, int height) {
    TRACE("shell_print");

    int xx, yy;
    int oldlength, newlength;
    
    for (yy = 0; yy < height; yy++) {
        int4 Y = (printout_bottom + yy) % PRINT_LINES;
        for (xx = 0; xx < 143; xx++) {
            int bit;
            if (xx < width) {
                char c = bits[(y + yy) * bytesperline + ((x + xx) >> 3)];
                bit = (c & (1 << ((x + xx) & 7))) != 0;
            } else
                bit = 0;
            if (bit)
                print_bitmap[Y * PRINT_BYTESPERLINE + (xx >> 3)] |= 1 << (xx & 7);
            else
                print_bitmap[Y * PRINT_BYTESPERLINE + (xx >> 3)] &= ~(1 << (xx & 7));
        }
    }
    
    oldlength = printout_bottom - printout_top;
    if (oldlength < 0)
        oldlength += PRINT_LINES;
    printout_bottom = (printout_bottom + height) % PRINT_LINES;
    newlength = oldlength + height;
    
    update_params params;
    params.oldlength = oldlength;
    params.newlength = newlength;
    params.height = height;
    [[PrintView instance] updatePrintout:&params];
    
    if (state.printerToTxtFile) {
        int err;
        char buf[1000];
        
        if (print_txt == NULL) {
            print_txt = fopen(state.printerTxtFileName, "a");
            if (print_txt == NULL) {
                err = errno;
                state.printerToTxtFile = 0;
                snprintf(buf, 1000, "Can't open \"%s\" for output:\n%s (%d)\nPrinting to text file disabled.", state.printerTxtFileName, strerror(err), err);
                show_message("Message", buf);
                goto done_print_txt;
            }
        }
        
        if (text != NULL)
            shell_spool_txt(text, length, txt_writer, txt_newliner);
        else
            shell_spool_bitmap_to_txt(bits, bytesperline, x, y, width, height, txt_writer, txt_newliner);
    done_print_txt:;
    }
    
    if (state.printerToGifFile) {
        int err;
        char buf[1000];
        
        if (print_gif != NULL
                && gif_lines + height > state.printerGifMaxLength) {
            shell_finish_gif(gif_seeker, gif_writer);
            fclose(print_gif);
            print_gif = NULL;
        }
        
        if (print_gif == NULL) {
            while (1) {
                ssize_t len, p;
                
                gif_seq = (gif_seq + 1) % 10000;
                
                strcpy(print_gif_name, state.printerGifFileName);
                len = strlen(print_gif_name);
                
                /* Strip ".gif" extension, if present */
                if (len >= 4 &&
                    strcasecmp(print_gif_name + len - 4, ".gif") == 0) {
                    len -= 4;
                    print_gif_name[len] = 0;
                }
                
                /* Strip ".[0-9]+", if present */
                p = len;
                while (p > 0 && print_gif_name[p] >= '0'
                       && print_gif_name[p] <= '9')
                    p--;
                if (p < len && p >= 0 && print_gif_name[p] == '.')
                    print_gif_name[p] = 0;
                
                /* Make sure we have enough space for the ".nnnn.gif" */
                p = FILENAMELEN - 10;
                print_gif_name[p] = 0;
                p = strlen(print_gif_name);
                snprintf(print_gif_name + p, 6, ".%04d", gif_seq);
                strcat(print_gif_name, ".gif");
                
                if (!is_file(print_gif_name))
                    break;
            }
            print_gif = fopen(print_gif_name, "w+");
            if (print_gif == NULL) {
                err = errno;
                state.printerToGifFile = 0;
                snprintf(buf, 1000, "Can't open \"%s\" for output:\n%s (%d)\nPrinting to GIF file disabled.", print_gif_name, strerror(err), err);
                show_message("Message", buf);
                goto done_print_gif;
            }
            if (!shell_start_gif(gif_writer, 143, state.printerGifMaxLength)) {
                state.printerToGifFile = 0;
                show_message("Message", "Not enough memory for the GIF encoder.\nPrinting to GIF file disabled.");
                goto done_print_gif;
            }
            gif_lines = 0;
        }
        
        shell_spool_gif(bits, bytesperline, x, y, width, height, gif_writer);
        gif_lines += height;

        if (print_gif != NULL && gif_lines + 9 > state.printerGifMaxLength) {
            shell_finish_gif(gif_seeker, gif_writer);
            fclose(print_gif);
            print_gif = NULL;
        }
        done_print_gif:;
    }

    print_text[print_text_bottom++] = (char) (text == NULL ? 255 : length);
    if (print_text_bottom == PRINT_TEXT_SIZE)
        print_text_bottom = 0;
    if (text != NULL) {
        if (print_text_bottom + length < PRINT_TEXT_SIZE) {
            memcpy(print_text + print_text_bottom, text, length);
            print_text_bottom += length;
        } else {
            int part = PRINT_TEXT_SIZE - print_text_bottom;
            memcpy(print_text + print_text_bottom, text, part);
            memcpy(print_text, text + part, length - part);
            print_text_bottom = length - part;
        }
    }
    print_text_pixel_height += text == NULL ? 16 : 9;
    while (print_text_pixel_height > PRINT_LINES - 1) {
        int tll = print_text[print_text_top] == 255 ? 16 : 9;
        print_text_pixel_height -= tll;
        print_text_top += tll == 16 ? 1 : (print_text[print_text_top] + 1);
        if (print_text_top >= PRINT_TEXT_SIZE)
            print_text_top -= PRINT_TEXT_SIZE;
    }
}

//////////////////////////////////////////////////////////////////////
/////   Accelerometer, Location Services, and Compass support    /////
//////////////////////////////////////////////////////////////////////

int shell_get_acceleration(double *x, double *y, double *z) {
    static bool accelerometer_active = false;
    if (!accelerometer_active) {
        accelerometer_active = true;
        [calcView start_accelerometer];
    }
    CMAccelerometerData *cmd = [motMgr accelerometerData];
    if (cmd == nil) {
        *x = *y = *z = 0;
    } else {
        *x = cmd.acceleration.x;
        *y = cmd.acceleration.y;
        *z = cmd.acceleration.z;
    }
    return 1;
}

int shell_get_location(double *lat, double *lon, double *lat_lon_acc, double *elev, double *elev_acc) {
    static bool location_active = false;
    if (!location_active) {
        location_active = true;
        [calcView start_location];
    }
    *lat = loc_lat;
    *lon = loc_lon;
    *lat_lon_acc = loc_lat_lon_acc;
    *elev = loc_elev;
    *elev_acc = loc_elev_acc;
    return 1;
}

int shell_get_heading(double *mag_heading, double *true_heading, double *acc, double *x, double *y, double *z) {
    static bool heading_active = false;
    if (!heading_active) {
        heading_active = true;
        [calcView start_heading];
    }
    *mag_heading = hdg_mag;
    *true_heading = hdg_true;
    *acc = hdg_acc;
    *x = hdg_x;
    *y = hdg_y;
    *z = hdg_z;
    return 1;
}

//////////////////////////////////////////////////////////////////////
/////          Here endeth ye iPhone hardware support.           /////
//////////////////////////////////////////////////////////////////////

/* Callbacks used by shell_print() and shell_spool_txt() / shell_spool_gif() */

static void show_message(const char *title, const char *message) {
    // TODO!
    fprintf(stderr, "%s\n", message);
}

static void txt_writer(const char *text, int length) {
    size_t n;
    if (print_txt == NULL)
        return;
    n = fwrite(text, 1, length, print_txt);
    if (n != length) {
        char buf[1000];
        state.printerToTxtFile = 0;
        fclose(print_txt);
        print_txt = NULL;
        snprintf(buf, 1000, "Error while writing to \"%s\".\nPrinting to text file disabled", state.printerTxtFileName);
        show_message("Message", buf);
    }
}   

static void txt_newliner() {
    if (print_txt == NULL)
        return;
    fputc('\r', print_txt);
    fputc('\n', print_txt);
    fflush(print_txt);
}   

static void gif_seeker(int4 pos) {
    if (print_gif == NULL)
        return;
    if (fseek(print_gif, pos, SEEK_SET) == -1) {
        char buf[1000];
        state.printerToGifFile = 0;
        fclose(print_gif);
        print_gif = NULL;
        snprintf(buf, 1000, "Error while seeking \"%s\".\nPrinting to GIF file disabled", print_gif_name);
        show_message("Message", buf);
    }
}

static void gif_writer(const char *text, int length) {
    size_t n;
    if (print_gif == NULL)
        return;
    n = fwrite(text, 1, length, print_gif);
    if (n != length) {
        char buf[1000];
        state.printerToGifFile = 0;
        fclose(print_gif);
        print_gif = NULL;
        snprintf(buf, 1000, "Error while writing to \"%s\".\nPrinting to GIF file disabled", print_gif_name);
        show_message("Message", buf);
    }
}

static bool is_file(const char *name) {
    struct stat st;
    if (stat(name, &st) == -1)
        return false;
    return S_ISREG(st.st_mode);
}
