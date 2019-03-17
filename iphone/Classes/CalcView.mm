/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2019  Thomas Okken
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
#import <pthread.h>

#import <AudioToolbox/AudioServices.h>
#import <CoreLocation/CoreLocation.h>

#import "CalcView.h"
#import "PrintView.h"
#import "RootViewController.h"
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

static int level = 0;

class Tracer {
private:
    const char *name;
public:
    Tracer(const char *name) {
        this->name = name;
        for (int i = 0; i < level; i++)
            fprintf(stderr, " ");
        fprintf(stderr, "ENTERING %s\n", name);
        level++;
    }
    ~Tracer() {
        level--;
        for (int i = 0; i < level; i++)
            fprintf(stderr, " ");
        fprintf(stderr, "EXITING %s\n", name);
    }
};

#if 0
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
static FILE* statefile;

static int quit_flag = 0;
static int enqueued;
static int keep_running = 0;
static int we_want_cpu = 0;
static bool is_running = false;
static pthread_mutex_t is_running_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t is_running_cond = PTHREAD_COND_INITIALIZER;

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
static pthread_mutex_t ann_print_timeout_mutex = PTHREAD_MUTEX_INITIALIZER;
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

static double accel_x = 0, accel_y = 0, accel_z = 0;
static double loc_lat = 0, loc_lon = 0, loc_lat_lon_acc = -1, loc_elev = 0, loc_elev_acc = -1;
static double hdg_mag = 0, hdg_true = 0, hdg_acc = -1, hdg_x = 0, hdg_y = 0, hdg_z = 0;

@interface HardwareDelegate : NSObject <UIAccelerometerDelegate, CLLocationManagerDelegate> {}
- (void)accelerometer:(UIAccelerometer *)accelerometer didAccelerate:(UIAcceleration *)acceleration;
- (void)locationManager:(CLLocationManager *)manager didUpdateToLocation:(CLLocation *)newLocation fromLocation:(CLLocation *)oldLocation;
- (void)locationManager:(CLLocationManager *)manager didFailWithError:(NSError *)error;
- (void)locationManager:(CLLocationManager *)manager didUpdateHeading:(CLHeading *)newHeading;
- (BOOL)locationManagerShouldDisplayHeadingCalibration:(CLLocationManager *)manager;
@end

@implementation HardwareDelegate

- (void)accelerometer:(UIAccelerometer *)accelerometer didAccelerate:(UIAcceleration *)acceleration {
    accel_x = acceleration.x;
    accel_y = acceleration.y;
    accel_z = acceleration.z;
    NSLog(@"Acceleration received: %g %g %g", accel_x, accel_y, accel_z);
}

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

- (void) setNeedsDisplayInRectSafely2:(id) myrect {
    TRACE("setNeedsDisplayInRectSafely2");
    CGRect *r = (CGRect *) [((NSValue *) myrect) pointerValue];
    [self setNeedsDisplayInRect:*r];
    delete r;
}

- (void) setNeedsDisplayInRectSafely:(CGRect) rect {
    TRACE("setNeedsDisplayInRectSafely");
    if ([NSThread isMainThread])
        [self setNeedsDisplayInRect:rect];
    else {
        CGRect *r = new CGRect;
        r->origin.x = rect.origin.x;
        r->origin.y = rect.origin.y;
        r->size.width = rect.size.width;
        r->size.height = rect.size.height;
        [self performSelectorOnMainThread:@selector(setNeedsDisplayInRectSafely2:) withObject:[NSValue valueWithPointer:r] waitUntilDone:NO];
    }
}

- (void) showMainMenu {
    UIActionSheet *menu =
    [[UIActionSheet alloc] initWithTitle:@"Main Menu"
                                delegate:self cancelButtonTitle:@"Cancel" destructiveButtonTitle:nil
                       otherButtonTitles:@"Show Print-Out", @"Program Import & Export", @"Preferences", @"Select Skin", @"Copy", @"Paste", @"About Free42", nil];
    
    [menu showInView:self];
    [menu release];
}

- (void) showImportExportMenu {
    UIActionSheet *menu =
    [[UIActionSheet alloc] initWithTitle:@"Import & Export Menu"
                                delegate:self cancelButtonTitle:@"Cancel" destructiveButtonTitle:nil
                       otherButtonTitles:@"HTTP Server", @"Import Programs", @"Export Programs", @"Back", nil];
    
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
                // Preferences
                [RootViewController showPreferences];
                break;
            case 3:
                // Select Skin
                [RootViewController showSelectSkin];
                break;
            case 4:
                // Copy
                [self doCopy];
                break;
            case 5:
                // Paste
                [self doPaste];
                break;
            case 6:
                // About Free42
                [RootViewController showAbout];
                break;
            case 7:
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
                [RootViewController doExport];
                break;
            case 3:
                // Back
                [self showMainMenu];
                break;
            case 4:
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

- (void) touchesBegan3 {
    TRACE("touchesBegan3");
    if (state.keyClicks)
        AudioServicesPlaySystemSound(1105);
    if (state.hapticFeedback) {
        UIImpactFeedbackGenerator *fbgen = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleLight];
        [fbgen impactOccurred];
    }
    macro = skin_find_macro(ckey);
    shell_keydown();
    mouse_key = 1;
}

- (void) touchesBegan2 {
    TRACE("touchesBegan2");
    we_want_cpu = 1;
    pthread_mutex_lock(&is_running_mutex);
    while (is_running)
        pthread_cond_wait(&is_running_cond, &is_running_mutex);
    pthread_mutex_unlock(&is_running_mutex);
    we_want_cpu = 0;
    [self performSelectorOnMainThread:@selector(touchesBegan3) withObject:NULL waitUntilDone:NO];
}

- (void) touchesBegan: (NSSet *) touches withEvent: (UIEvent *) event {
    TRACE("touchesBegan");
    [super touchesBegan:touches withEvent:event];
    UITouch *touch = (UITouch *) [touches anyObject];
    CGPoint p = [touch locationInView:self];
    int x = (int) p.x;
    int y = (int) p.y;
    if (skin_in_menu_area(x, y)) {
        [self showMainMenu];
    } else if (ckey == 0) {
        skin_find_key(x, y, ann_shift != 0, &skey, &ckey);
        if (ckey != 0) {
            if (is_running)
                [self performSelectorInBackground:@selector(touchesBegan2) withObject:NULL];
            else
                [self touchesBegan3];
        }
    }
}

- (void) touchesEnded3 {
    TRACE("touchesEnded3");
    shell_keyup();
}

- (void) touchesEnded2 {
    TRACE("touchesEnded2");
    we_want_cpu = 1;
    pthread_mutex_lock(&is_running_mutex);
    while (is_running)
        pthread_cond_wait(&is_running_cond, &is_running_mutex);
    pthread_mutex_unlock(&is_running_mutex);
    we_want_cpu = 0;
    [self performSelectorOnMainThread:@selector(touchesEnded3) withObject:NULL waitUntilDone:NO];
}

- (void) touchesEnded: (NSSet *) touches withEvent: (UIEvent *) event {
    TRACE("touchesEnded");
    [super touchesEnded:touches withEvent:event];
    if (ckey != 0 && mouse_key) {
        if (is_running)
            [self performSelectorInBackground:@selector(touchesEnded2) withObject:NULL];
        else
            [self touchesEnded3];
    }
}

- (void) touchesCancelled: (NSSet *) touches withEvent: (UIEvent *) event {
    TRACE("touchesCancelled");
    [super touchesCancelled:touches withEvent:event];
    if (ckey != 0 && mouse_key) {
        if (is_running)
            [self performSelectorInBackground:@selector(touchesEnded2) withObject:NULL];
        else
            [self touchesEnded3];
    }
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
    const char *buf = [txt UTF8String];
    core_paste(buf);
}

- (void) startRunner {
    TRACE("startRunner");
    [self performSelectorInBackground:@selector(runner) withObject:NULL];
}

- (void) awakeFromNib {
    TRACE("awakeFromNib");
    [super awakeFromNib];
    calcView = self;
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

    long w, h;
    skin_load(&w, &h);
    
    core_init(init_mode, version);
    if (statefile != NULL) {
        fclose(statefile);
        statefile = NULL;
    }
    keep_running = core_powercycle();
    if (keep_running)
        [self startRunner];
    if (shell_always_on(-1))
        [UIApplication sharedApplication].idleTimerDisabled = YES;
}

- (void) runner {
    TRACE("runner");
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    int dummy1, dummy2;
    is_running = true;
    keep_running = core_keydown(0, &dummy1, &dummy2);
    pthread_mutex_lock(&is_running_mutex);
    is_running = false;
    pthread_cond_signal(&is_running_cond);
    pthread_mutex_unlock(&is_running_mutex);
    if (quit_flag)
        [self performSelectorOnMainThread:@selector(quitB) withObject:NULL waitUntilDone:NO];
    else if (keep_running && !we_want_cpu)
        [self performSelectorOnMainThread:@selector(startRunner) withObject:NULL waitUntilDone:NO];
    [pool release];
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

static pthread_mutex_t shell_helper_mutex = PTHREAD_MUTEX_INITIALIZER;
static int timeout3_delay;

- (void) shell_request_timeout3_helper {
    TRACE("shell_request_timeout3_helper");
    [calcView setTimeout3:timeout3_delay];
    pthread_mutex_unlock(&shell_helper_mutex);
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

static HardwareDelegate *hwDel = NULL;
static CLLocationManager *locMgr = NULL;

- (void) start_accelerometer {
    UIAccelerometer *am = [UIAccelerometer sharedAccelerometer];
    am.updateInterval = 1;
    if (hwDel == NULL)
        hwDel = [HardwareDelegate alloc];
    am.delegate = hwDel;
}

- (void) start_location {
    if (locMgr == NULL) {
        locMgr = [[CLLocationManager alloc] init];
        if (hwDel == NULL)
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
    pthread_mutex_lock(&ann_print_timeout_mutex);
    ann_print = 0;
    skin_update_annunciator(3, 0, calcView);
    ann_print_timeout_active = FALSE;
    pthread_mutex_unlock(&ann_print_timeout_mutex);
}

- (void) print_ann_helper:(NSNumber *)set {
    int prt = [set intValue];
    [set release];
    pthread_mutex_lock(&ann_print_timeout_mutex);
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
    pthread_mutex_unlock(&ann_print_timeout_mutex);
}

@end

///////////////////////////////////////////////////////////////////////////////
/////                   Here beginneth thy olde C code                    /////
///////////////////////////////////////////////////////////////////////////////

static int read_shell_state(int *ver) {
    TRACE("read_shell_state");
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
            /* current version (SHELL_VERSION = 5),
             * so nothing to do here since everything
             * was initialized from the state file.
             */
            ;
    }
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
    statefile = fopen("config/state", "w");
    if (statefile != NULL)
        write_shell_state();
    if (really_quit)
        core_quit();
    else
        core_enter_background();
    if (statefile != NULL)
        fclose(statefile);
    if (really_quit)
        exit(0);
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
    TRACE("shell_blitter");
    skin_display_blitter(bits, bytesperline, x, y, width, height, calcView);
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
        NSNumber *n = [[NSNumber numberWithInt:prt] retain];
        [calcView performSelectorOnMainThread:@selector(print_ann_helper:) withObject:n waitUntilDone:NO];
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
    return we_want_cpu;
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
    pthread_mutex_lock(&shell_helper_mutex);
    timeout3_delay = delay;
    [calcView performSelectorOnMainThread:@selector(shell_request_timeout3_helper) withObject:NULL waitUntilDone:NO];
}

int shell_read_saved_state(void *buf, int bufsize) {
    TRACE("shell_read_saved_state");
    if (statefile == NULL)
        return -1;
    else {
        size_t n = fread(buf, 1, bufsize, statefile);
        if (n != bufsize && ferror(statefile)) {
            fclose(statefile);
            statefile = NULL;
            return -1;
        } else
            return (int) n;
    }
}

bool shell_write_saved_state(const void *buf, int nbytes) {
    TRACE("shell_write_saved_state");
    if (statefile == NULL)
        return false;
    else {
        size_t n = fwrite(buf, 1, nbytes, statefile);
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
        int4 Y = (printout_bottom + 2 * yy) % PRINT_LINES;
        for (xx = 0; xx < 143; xx++) {
            int bit, px, py;
            if (xx < width) {
                char c = bits[(y + yy) * bytesperline + ((x + xx) >> 3)];
                bit = (c & (1 << ((x + xx) & 7))) != 0;
            } else
                bit = 0;
            for (px = xx * 2; px < (xx + 1) * 2; px++)
                for (py = Y; py < Y + 2; py++)
                    if (bit)
                        print_bitmap[py * PRINT_BYTESPERLINE + (px >> 3)]
                        |= 1 << (px & 7);
                    else
                        print_bitmap[py * PRINT_BYTESPERLINE + (px >> 3)]
                        &= ~(1 << (px & 7));
        }
    }
    
    oldlength = printout_bottom - printout_top;
    if (oldlength < 0)
        oldlength += PRINT_LINES;
    printout_bottom = (printout_bottom + 2 * height) % PRINT_LINES;
    newlength = oldlength + 2 * height;
    
    update_params *params = new update_params;
    params->oldlength = oldlength;
    params->newlength = newlength;
    params->height = height;
    [[PrintView instance] performSelectorOnMainThread:@selector(updatePrintout:) withObject:[NSValue valueWithPointer:params] waitUntilDone:YES];
    
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
            if (ftell(print_txt) == 0)
                fwrite("\357\273\277", 1, 3, print_txt);
        }
        
        shell_spool_txt(text, length, txt_writer, txt_newliner);
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
}

/*
int shell_write(const char *buf, int buflen) {
    return 0;
}

int shell_read(char *buf, int buflen) {
    return -1;
}
*/

//////////////////////////////////////////////////////////////////////
/////   Accelerometer, Location Services, and Compass support    /////
//////////////////////////////////////////////////////////////////////

int shell_get_acceleration(double *x, double *y, double *z) {
    static bool accelerometer_active = false;
    if (!accelerometer_active) {
        accelerometer_active = true;
        [calcView performSelectorOnMainThread:@selector(start_accelerometer) withObject:NULL waitUntilDone:NO];
    }
    *x = accel_x;
    *y = accel_y;
    *z = accel_z;
    return 1;
}

int shell_get_location(double *lat, double *lon, double *lat_lon_acc, double *elev, double *elev_acc) {
    static bool location_active = false;
    if (!location_active) {
        location_active = true;
        [calcView performSelectorOnMainThread:@selector(start_location) withObject:NULL waitUntilDone:NO];
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
        [calcView performSelectorOnMainThread:@selector(start_heading) withObject:NULL waitUntilDone:NO];
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
