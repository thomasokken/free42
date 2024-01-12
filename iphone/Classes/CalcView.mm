/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2024  Thomas Okken
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
#import "shell_skin.h"
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
static void calc_keydown(NSString *characters, long flags, int keycode);
static void calc_keyup(NSString *characters, long flags, int keycode);

static int read_shell_state(int *version);
static void init_shell_state(int version);
static int write_shell_state();

state_type state;
FILE* statefile;

static bool quit_flag = false;
static bool enqueued;
static bool keep_running = false;
static bool we_want_cpu = false;

static int ckey = 0;
static int skey;
static unsigned char *macro;
static bool macro_is_name;
static bool mouse_key;
static int active_keycode = -1;
static bool just_pressed_shift = false;
static UITouch *currentTouch = nil;
static int keymap_length = 0;
static keymap_entry *keymap = NULL;

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
    UIAlertController *ctrl = [UIAlertController
            alertControllerWithTitle:@"Main Menu"
            message:nil
            preferredStyle:UIAlertControllerStyleActionSheet];
    [ctrl addAction:[UIAlertAction actionWithTitle:@"Show Print-Out"
                    style:UIAlertActionStyleDefault
                    handler:^(UIAlertAction *action)
                        { [RootViewController showPrintOut]; }]];
    [ctrl addAction:[UIAlertAction actionWithTitle:@"Program Import & Export"
                    style:UIAlertActionStyleDefault
                    handler:^(UIAlertAction *action)
                        { [self showImportExportMenu]; }]];
    [ctrl addAction:[UIAlertAction actionWithTitle:@"States"
                    style:UIAlertActionStyleDefault
                    handler:^(UIAlertAction *action)
                        { [RootViewController showStates:nil]; }]];
    [ctrl addAction:[UIAlertAction actionWithTitle:@"Preferences"
                    style:UIAlertActionStyleDefault
                    handler:^(UIAlertAction *action)
                        { [RootViewController showPreferences]; }]];
    [ctrl addAction:[UIAlertAction actionWithTitle:@"Select Skin"
                    style:UIAlertActionStyleDefault
                    handler:^(UIAlertAction *action)
                        { [RootViewController showSelectSkin]; }]];
    [ctrl addAction:[UIAlertAction actionWithTitle:@"Copy"
                    style:UIAlertActionStyleDefault
                    handler:^(UIAlertAction *action)
                        { [self doCopy]; }]];
    [ctrl addAction:[UIAlertAction actionWithTitle:@"Paste"
                    style:UIAlertActionStyleDefault
                    handler:^(UIAlertAction *action)
                        { [self doPaste]; }]];
    [ctrl addAction:[UIAlertAction actionWithTitle:@"About Free42"
                    style:UIAlertActionStyleDefault
                    handler:^(UIAlertAction *action)
                        { [RootViewController showAbout]; }]];
    [ctrl addAction:[UIAlertAction actionWithTitle:@"Cancel"
                    style:UIAlertActionStyleCancel
                    handler:^(UIAlertAction *action)
                        { return; }]];
    [RootViewController presentViewController:ctrl animated:YES completion:nil];
}

- (void) showImportExportMenu {
    UIAlertController *ctrl = [UIAlertController
            alertControllerWithTitle:@"Import & Export Menu"
            message:nil
            preferredStyle:UIAlertControllerStyleActionSheet];
    [ctrl addAction:[UIAlertAction actionWithTitle:@"HTTP Server"
                    style:UIAlertActionStyleDefault
                    handler:^(UIAlertAction *action)
                        { [RootViewController showHttpServer]; }]];
    [ctrl addAction:[UIAlertAction actionWithTitle:@"Import Programs"
                    style:UIAlertActionStyleDefault
                    handler:^(UIAlertAction *action)
                        { [RootViewController doImport]; }]];
    [ctrl addAction:[UIAlertAction actionWithTitle:@"Export Programs"
                    style:UIAlertActionStyleDefault
                    handler:^(UIAlertAction *action)
                        { [RootViewController doExport:NO]; }]];
    [ctrl addAction:[UIAlertAction actionWithTitle:@"Share Programs"
                    style:UIAlertActionStyleDefault
                    handler:^(UIAlertAction *action)
                        { [RootViewController doExport:YES]; }]];
    [ctrl addAction:[UIAlertAction actionWithTitle:@"Back"
                    style:UIAlertActionStyleDefault
                    handler:^(UIAlertAction *action)
                        { [self showMainMenu]; }]];
    [ctrl addAction:[UIAlertAction actionWithTitle:@"Cancel"
                    style:UIAlertActionStyleCancel
                    handler:^(UIAlertAction *action)
                        { return; }]];
    [RootViewController presentViewController:ctrl animated:YES completion:nil];
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

static char touchDelayed = 0;
static CGPoint touchPoint;

- (void) touchesBegan: (NSSet *) touches withEvent: (UIEvent *) event {
    TRACE("touchesBegan");
    [super touchesBegan:touches withEvent:event];
    if (touchDelayed != 0) {
        [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(touchesBegan2) object:nil];
        [self touchesBegan2];
    }
    if (ckey != 0)
        shell_keyup();
    [currentTouch release];
    currentTouch = (UITouch *) [touches anyObject];
    [currentTouch retain];
    touchPoint = [currentTouch locationInView:self];
    touchDelayed = 1;
    [self performSelector:@selector(touchesBegan2) withObject:nil afterDelay:0.05];
}

- (void) touchesBegan2 {
    CGPoint p = touchPoint;
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
            mouse_key = true;
        }
    }
    if (touchDelayed == 2) {
        if (ckey != 0)
            shell_keyup();
    }
    touchDelayed = 0;
}

- (void) myTouchesEnded:(NSSet *) touches {
    if (touchDelayed == 1) {
        touchDelayed = 2;
    } else {
        if (ckey != 0 && [touches containsObject:currentTouch]) {
            shell_keyup();
            [currentTouch release];
            currentTouch = nil;
        }
        touchDelayed = 0;
    }
}

- (void) touchesEnded: (NSSet *) touches withEvent: (UIEvent *) event {
    TRACE("touchesEnded");
    [super touchesEnded:touches withEvent:event];
    [self myTouchesEnded:touches];
}

- (void) touchesCancelled: (NSSet *) touches withEvent: (UIEvent *) event {
    TRACE("touchesCancelled");
    [super touchesCancelled:touches withEvent:event];
    [self myTouchesEnded:touches];
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
    bool dummy1;
    int dummy2;
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
    
    UIPanGestureRecognizer *panrec = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePan:)];
    panrec.cancelsTouchesInView = NO;
    panrec.delaysTouchesEnded = NO;
    [self addGestureRecognizer:panrec];
}

- (void) handlePan:(UIPanGestureRecognizer *)panrec {
    static CGFloat prevX;
    CGFloat dir;
    switch (state.swipeDirectionMode) {
        case 0: dir = 1; break;
        case 2: dir = -1; break;
        default: return;
    }
    UIGestureRecognizerState gstate = [panrec state];
    CGPoint p = [panrec translationInView:[self superview]];
    PrintView *print = ((Free42AppDelegate *) UIApplication.sharedApplication.delegate).rootViewController.printView;
    CGRect cf = self.frame;
    CGRect pf = print.frame;
    if (gstate == UIGestureRecognizerStateBegan) {
        // Make sure the Print-Out view isn't hidden
        [RootViewController showPrintOut];
        [RootViewController showMain];
        [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(touchesBegan2) object:nil];
        touchDelayed = 0;
        prevX = self.frame.origin.x;
    }
    if (gstate == UIGestureRecognizerStateEnded) {
        cf.origin.x = prevX;
        self.frame = cf;
        pf.origin.x = prevX;
        print.frame = pf;
        CGPoint v = [panrec velocityInView:[self superview]];
        CGFloat scale = self.bounds.size.width / self.bounds.size.height;
        if (scale < 1)
            scale = 1;
        if (scale * (p.x + v.x / 16) * dir < -self.bounds.size.width / 3)
            [RootViewController showPrintOut];
    } else {
        if (dir * p.x > 0)
            p.x = 0;
        cf.origin.x = self.superview.bounds.origin.x + p.x;
        self.frame = cf;
        pf.origin.x = self.superview.bounds.origin.x + p.x + dir * self.frame.size.width;
        print.frame = pf;
    }
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
    keep_running = core_timeout3(true);
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

- (void) setActive:(bool) active {
    if (active)
        [self becomeFirstResponder];
    else
        [self resignFirstResponder];
}

- (BOOL) canBecomeFirstResponder {
    return YES;
}

static void read_key_map(const char *keymapfilename);

+ (void) readKeyMap {
    read_key_map("config/keymap.txt");
}

// Keyboard handling (UIResponder methods)

// Tells this object when a physical button is first pressed.
- (void)pressesBegan:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    NSArray *p = [presses allObjects];
    bool handled = false;
    for (int i = 0; i < [p count]; i++) {
        UIPress *pr = [p objectAtIndex:i];
        NSObject *k = [pr key];
        if (k != nil) {
            @try {
                NSString *characters = [k valueForKey:@"characters"];
                NSString *nonModCharacters = [k valueForKey:@"charactersIgnoringModifiers"];
                if ([characters length] == 0 || [nonModCharacters hasPrefix:@"UIKeyInput"])
                    characters = nonModCharacters;
                long flags = [[k valueForKey:@"modifierFlags"] longValue];
                int keycode = [[k valueForKey:@"keyCode"] intValue];
                calc_keydown(characters, flags, keycode);
                handled = true;
            }
            @catch (id ex) {
                // [pr key] not an NSKey?
                // The situation is a bit weird; the docs say UIPress.key is defined in
                // iOS 9.0+, and is of type UIKey*, but the UIKey type is only defined
                // in iOS 13.4+. I'm assuming that in iOS 9, UIKey is the same as in
                // 13.4+, and that it just wasn't public, so that I should be able to
                // use valueForKey to get at its fields in iOS [9.0, 13.4). If we get
                // into this catch block, then that assumption would appear to be wrong.
                // I don't have anything with sufficiently old iOS versions to find out,
                // so at least for now, if there is an exception here, then the device
                // will simply not respond to the keyboard in CalcView, which is a
                // graceful failure mode at least.
            }
        }
    }
    if (!handled)
        [super pressesBegan:presses withEvent:event];
}

// Tells the object when a button is released.
- (void)pressesEnded:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    NSArray *p = [presses allObjects];
    bool handled = false;
    for (int i = 0; i < [p count]; i++) {
        UIPress *pr = [p objectAtIndex:i];
        NSObject *k = [pr key];
        if (k != nil) {
            @try {
                NSString *characters = [k valueForKey:@"characters"];
                if ([characters length] == 0)
                    characters = [k valueForKey:@"charactersIgnoringModifiers"];
                long flags = [[k valueForKey:@"modifierFlags"] longValue];
                int keycode = [[k valueForKey:@"keyCode"] intValue];
                calc_keyup(characters, flags, keycode);
                handled = true;
            }
            @catch (id ex) {
                // [pr key] not an NSKey?
                // The situation is a bit weird; the docs say UIPress.key is defined in
                // iOS 9.0+, and is of type UIKey*, but the UIKey type is only defined
                // in iOS 13.4+. I'm assuming that in iOS 9, UIKey is the same as in
                // 13.4+, and that it just wasn't public, so that I should be able to
                // use valueForKey to get at its fields in iOS [9.0, 13.4). If we get
                // into this catch block, then that assumption would appear to be wrong.
                // I don't have anything with sufficiently old iOS versions to find out,
                // so at least for now, if there is an exception here, then the device
                // will simply not respond to the keyboard in CalcView, which is a
                // graceful failure mode at least.
            }
        }
    }
    if (!handled)
        [super pressesEnded:presses withEvent:event];
}

// Tells this object when a value associated with a press has changed.
- (void)pressesChanged:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    // No need to handle this
    [super pressesChanged:presses withEvent:event];
}

// Tells this object when a system event (such as a low-memory warning) cancels a press event.
- (void)pressesCancelled:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    // No need to handle this
    [super pressesCancelled:presses withEvent:event];
}

@end

static void read_key_map(const char *keymapfilename) {
    FILE *keymapfile = NULL;//fopen(keymapfilename, "r");
    int kmcap = 0;
    char line[1024];
    int lineno = 0;

    if (keymapfile == NULL) {
        /* Try to create default keymap file */
        keymapfile = fopen(keymapfilename, "wb");
        if (keymapfile == NULL)
            return;
        NSString *path = [[NSBundle mainBundle] pathForResource:@"keymap" ofType:@"txt"];
        [path getCString:line maxLength:1024 encoding:NSUTF8StringEncoding];
        FILE *builtin_keymapfile = fopen(line, "r");
        int n;
        while ((n = fread(line, 1, 1024, builtin_keymapfile)) > 0)
            fwrite(line, 1, n, keymapfile);
        fclose(builtin_keymapfile);
        fclose(keymapfile);

        keymapfile = fopen(keymapfilename, "r");
        if (keymapfile == NULL)
            return;
    }

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

///////////////////////////////////////////////////////////////////////////////
/////                   Here beginneth thy olde C code                    /////
///////////////////////////////////////////////////////////////////////////////

extern bool off_enable_flag;

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
    if (state_version >= 9)
        core_settings.allow_big_stack = state.allow_big_stack;
    if (state_version >= 11)
        core_settings.localized_copy_paste = state.localized_copy_paste;
    
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
            core_settings.allow_big_stack = false;
            /* fall through */
        case 9:
            /* fall through */
        case 10:
            core_settings.localized_copy_paste = true;
            /* fall through */
        case 11:
            state.swipeDirectionMode = 0;
            /* fall through */
        case 12:
            /* fall through */
        case 13:
            /* current version (SHELL_VERSION = 13),
             * so nothing to do here since everything
             * was initialized from the state file.
             */
            ;
    }
    off_enable_flag = state.offEnabled;
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
        core_timeout3(false);
    }
    
    // We temporarily set we_want_cpu to 'true', to force the calls
    // to core_keydown() in this function to return quickly. This is
    // necessary since this function runs on the main thread, and we
    // can't peek ahead in the event queue while core_keydown() is
    // hogging the CPU on the main thread. (The lack of something like
    // EventAvail is an annoying omission of the iPhone API.)
        
    if (macro != NULL) {
        if (macro_is_name) {
            we_want_cpu = true;
            keep_running = core_keydown_command((const char *) macro, &enqueued, &repeat);
            we_want_cpu = false;
        } else {
            if (*macro == 0) {
                squeak();
                return;
            }
            bool one_key_macro = macro[1] == 0 || (macro[2] == 0 && macro[0] == 28);
            if (one_key_macro) {
                while (*macro != 0) {
                    we_want_cpu = true;
                    keep_running = core_keydown(*macro++, &enqueued, &repeat);
                    we_want_cpu = false;
                    if (*macro != 0 && !enqueued)
                        core_keyup();
                }
            } else {
                bool waitForProgram = !program_running();
                skin_display_set_enabled(false);
                while (*macro != 0) {
                    we_want_cpu = true;
                    keep_running = core_keydown(*macro++, &enqueued, &repeat);
                    we_want_cpu = false;
                    if (*macro != 0 && !enqueued)
                        keep_running = core_keyup();
                    while (waitForProgram && keep_running) {
                        we_want_cpu = true;
                        keep_running = core_keydown(0, &enqueued, &repeat);
                        we_want_cpu = false;
                    }
                }
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
        we_want_cpu = true;
        keep_running = core_keydown(ckey, &enqueued, &repeat);
        we_want_cpu = false;
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

static void calc_keydown(NSString *characters, long flags, int keycode) {
    if (ckey != 0 && mouse_key)
        return;

    int len = [characters length];
    just_pressed_shift = len == 0
        && (flags & (UIKeyModifierShift | UIKeyModifierControl | UIKeyModifierAlternate)) == UIKeyModifierShift;
    if (len == 0)
        return;

    NSString *c2 = nil;
    if ([characters hasPrefix:@"UIKeyInput"]) {
        NSString *s = [characters substringFromIndex:10];
        unsigned int fn;
        if ([s isEqualToString:@"UpArrow"])
            c2 = @"\uf700";
        else if ([s isEqualToString:@"DownArrow"])
            c2 = @"\uf701";
        else if ([s isEqualToString:@"LeftArrow"])
            c2 = @"\uf702";
        else if ([s isEqualToString:@"RightArrow"])
            c2 = @"\uf703";
        else if ([s isEqualToString:@"Insert"])
            c2 = @"\uf727";
        else if ([s isEqualToString:@"Delete"])
            c2 = @"\uf728";
        else if ([s isEqualToString:@"Home"])
            c2 = @"\uf729";
        else if ([s isEqualToString:@"End"])
            c2 = @"\uf72b";
        else if ([s isEqualToString:@"PageUp"])
            c2 = @"\uf72c";
        else if ([s isEqualToString:@"PageDown"])
            c2 = @"\uf72d";
        else if ([s isEqualToString:@"Escape"])
            c2 = @"\33";
        else if ([s hasPrefix:@"F"] && sscanf([s UTF8String] + 1, "%u", &fn) == 1)
            c2 = [NSString stringWithFormat:@"%C", (unsigned short) (0xf703 + fn)];
        else
            return;
    } else if ([characters isEqualToString:@"\10"])
        c2 = @"\177";
    else if ([characters isEqualToString:@"\5"])
        c2 = @"\uf727";
    else if ([characters isEqualToString:@"\177"])
        c2 = @"\uf728";
    if (c2 != nil) {
        characters = c2;
        len = [c2 length];
    }

    unsigned short c = [characters characterAtIndex:0];

    bool printable = len == 1 && c >= 33 && c <= 126;

    bool ctrl = (flags & UIKeyModifierControl) != 0;
    bool alt = (flags & UIKeyModifierAlternate) != 0;
    bool numpad = (flags & UIKeyModifierNumericPad) != 0;
    bool shift = (flags & UIKeyModifierShift) != 0;
    bool cshift = ann_shift != 0;

    if (ckey != 0) {
        shell_keyup();
        active_keycode = -1;
    }

    bool exact;
    unsigned char *key_macro = skin_keymap_lookup(c, printable, ctrl, alt, numpad, shift, cshift, &exact);
    if (key_macro == NULL || !exact) {
        for (int i = 0; i < keymap_length; i++) {
            keymap_entry *entry = keymap + i;
            if (ctrl == entry->ctrl
                    && alt == entry->alt
                    && (printable || shift == entry->shift)
                    && c == entry->keychar) {
                if ((!numpad || shift == entry->shift) && numpad == entry->numpad && cshift == entry->cshift) {
                    key_macro = entry->macro;
                    break;
                } else {
                    if ((numpad || !entry->numpad) && (cshift || !entry->cshift) && key_macro == NULL)
                        key_macro = entry->macro;
                }
            }
        }
    }

    if (key_macro == NULL || (key_macro[0] != 36 || key_macro[1] != 0)
            && (key_macro[0] != 28 || key_macro[1] != 36 || key_macro[2] != 0)) {
        // The test above is to make sure that whatever mapping is in
        // effect for R/S will never be overridden by the special cases
        // for the ALPHA and A..F menus.
        if (!ctrl && !alt) {
            if ((printable || c == ' ') && core_alpha_menu()) {
                if (c >= 'a' && c <= 'z')
                    c = c + 'A' - 'a';
                else if (c >= 'A' && c <= 'Z')
                    c = c + 'a' - 'A';
                ckey = 1024 + c;
                skey = -1;
                macro = NULL;
                shell_keydown();
                mouse_key = false;
                active_keycode = keycode;
                return;
            } else if (core_hex_menu() && ((c >= 'a' && c <= 'f')
                                        || (c >= 'A' && c <= 'F'))) {
                if (c >= 'a' && c <= 'f')
                    ckey = c - 'a' + 1;
                else
                    ckey = c - 'A' + 1;
                skey = -1;
                macro = NULL;
                shell_keydown();
                mouse_key = false;
                active_keycode = keycode;
                return;
            } else if (c == 0xf702 || c == 0xf703 || c == 0xf728) {
                int which;
               if (c == 0xf702)
                    which = shift ? 2 : 1;
                else if (c == 0xf703)
                    which = shift ? 4 : 3;
                else if (c == 0xf728)
                    which = 5;
                else
                    which = 0;
                if (which != 0) {
                    which = core_special_menu_key(which);
                    if (which != 0) {
                        ckey = which;
                        skey = -1;
                        macro = NULL;
                        shell_keydown();
                        mouse_key = false;
                        active_keycode = keycode;
                        return;
                    }
                }
            }
        }
    }

    if (key_macro != NULL) {
        // A keymap entry is a sequence of zero or more calculator
        // keystrokes (1..37) and/or macros (38..255). We expand
        // macros here before invoking shell_keydown().
        // If the keymap entry is one key, or two keys with the
        // first being 'shift', we highlight the key in question
        // by setting ckey; otherwise, we set ckey to -10, which
        // means no skin key will be highlighted.
        ckey = -10;
        skey = -1;
        if (key_macro[0] != 0)
            if (key_macro[1] == 0)
                ckey = key_macro[0];
            else if (key_macro[2] == 0 && key_macro[0] == 28)
                ckey = key_macro[1];
        bool needs_expansion = false;
        for (int j = 0; key_macro[j] != 0; j++)
            if (key_macro[j] > 37) {
                needs_expansion = true;
                break;
            }
        if (needs_expansion) {
            static unsigned char macrobuf[1024];
            int p = 0;
            for (int j = 0; key_macro[j] != 0 && p < 1023; j++) {
                int c = key_macro[j];
                if (c <= 37)
                    macrobuf[p++] = c;
                else {
                    unsigned char *m = skin_find_macro(c, &macro_is_name);
                    if (m != NULL)
                        while (*m != 0 && p < 1023)
                            macrobuf[p++] = *m++;
                }
            }
            macrobuf[p] = 0;
            macro = macrobuf;
        } else {
            macro = key_macro;
            macro_is_name = false;
        }
        shell_keydown();
        mouse_key = false;
        active_keycode = keycode;
    }
}

static void calc_keyup(NSString *characters, long flags, int keycode) {
    if (just_pressed_shift) {
        just_pressed_shift = false;
        ckey = 28;
        skey = -1;
        macro = NULL;
        shell_keydown();
        shell_keyup();
    } else if (!mouse_key && keycode == active_keycode) {
        shell_keyup();
        active_keycode = -1;
    }
}

static int write_shell_state() {
    TRACE("write_shell_state");
    int magic = FREE42_MAGIC;
    int version = 27;
    int state_size = sizeof(state);
    int state_version = SHELL_VERSION;

    state.offEnabled = off_enable_flag;
    
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
    state.allow_big_stack = core_settings.allow_big_stack;
    state.localized_copy_paste = core_settings.localized_copy_paste;
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

void shell_beeper(int tone) {
    TRACE("shell_beeper");
    [RootViewController playSound:tone];
    shell_delay(tone == 10 ? 125 : 250);
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

bool shell_always_on(int ao) {
    bool ret = state.alwaysOn;
    if (ao != -1) {
        state.alwaysOn = ao != 0;
        [UIApplication sharedApplication].idleTimerDisabled = state.alwaysOn ? YES : NO;
    }
    return ret;
}

void shell_log(const char *message) {
    NSLog(@"%s", message);
}

bool shell_wants_cpu() {
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

uint8 shell_get_mem() {
    TRACE("shell_get_mem");
    int mib[2];
    size_t memsize = 0;
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
    quit_flag = true;
    we_want_cpu = true;
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

const char *shell_number_format() {
    NSLocale *loc = [NSLocale currentLocale];
    static NSString *f = nil;
    [f release];
    f = [loc objectForKey:NSLocaleDecimalSeparator];
    NSNumberFormatter *fmt = [[NSNumberFormatter alloc] init];
    fmt.numberStyle = NSNumberFormatterDecimalStyle;
    if (fmt.usesGroupingSeparator) {
        NSString *sep = [loc objectForKey:NSLocaleGroupingSeparator];
        int ps = (int) fmt.groupingSize;
        int ss = (int) fmt.secondaryGroupingSize;
        if (ss == 0)
            ss = ps;
        f = [NSString stringWithFormat:@"%@%@%c%c", f, sep, '0' + ps, '0' + ss];
    }
    [f retain];
    return [f UTF8String];
}

int shell_date_format() {
    NSLocale *loc = [NSLocale currentLocale];
    NSString *dateFormat = [NSDateFormatter dateFormatFromTemplate:@"yyyy MM dd" options:0 locale:loc];
    NSUInteger y = [dateFormat rangeOfString:@"y"].location;
    NSUInteger m = [dateFormat rangeOfString:@"M"].location;
    NSUInteger d = [dateFormat rangeOfString:@"d"].location;
    if (d < m && m < y)
        return 1;
    else if (y < m && m < d)
        return 2;
    else
        return 0;
}

bool shell_clk24() {
    NSLocale *loc = [NSLocale currentLocale];
    NSString *timeFormat = [NSDateFormatter dateFormatFromTemplate:@"j" options:0 locale:loc];
    return [timeFormat rangeOfString:@"a"].location == NSNotFound;
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

bool shell_get_acceleration(double *x, double *y, double *z) {
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
    return true;
}

bool shell_get_location(double *lat, double *lon, double *lat_lon_acc, double *elev, double *elev_acc) {
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
    return true;
}

bool shell_get_heading(double *mag_heading, double *true_heading, double *acc, double *x, double *y, double *z) {
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
    return true;
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
