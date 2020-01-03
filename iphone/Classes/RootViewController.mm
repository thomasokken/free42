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

#import <AudioToolbox/AudioServices.h>

#import "CalcView.h"
#import "PrintView.h"
#import "HTTPServerView.h"
#import "SelectSkinView.h"
#import "SelectProgramsView.h"
#import "PreferencesView.h"
#import "AboutView.h"
#import "SelectFileView.h"
#import "DeleteSkinView.h"
#import "LoadSkinView.h"
#import "StatesView.h"
#import "RootViewController.h"
#import "shell.h"
#import "shell_skin_iphone.h"
#import "core_main.h"

static SystemSoundID soundIDs[20];

static RootViewController *instance;

@implementation RootViewController

@synthesize window;

@synthesize calcView;
@synthesize printView;
@synthesize httpServerView;
@synthesize selectSkinView;
@synthesize selectProgramsView;
@synthesize preferencesView;
@synthesize aboutView;
@synthesize selectFileView;
@synthesize deleteSkinView;
@synthesize loadSkinView;
@synthesize statesView;


- (void) awakeFromNib {
    [super awakeFromNib];
    instance = self;

    const char *sound_names[] = {
            "tone0", "tone1", "tone2", "tone3", "tone4",
            "tone5", "tone6", "tone7", "tone8", "tone9",
            "squeak",
            "click1", "click2", "click3", "click4", "click5",
            "click6", "click7", "click8", "click9"
        };
    for (int i = 0; i < 20; i++) {
        NSString *name = [NSString stringWithUTF8String:sound_names[i]];
        NSString *path = [[NSBundle mainBundle] pathForResource:name ofType:@"wav"];
        OSStatus status = AudioServicesCreateSystemSoundID((CFURLRef)[NSURL fileURLWithPath:path], &soundIDs[i]);
        if (status)
            NSLog(@"error loading sound: %@", name);
    }
    
    [self.view addSubview:printView];
    [self.view addSubview:httpServerView];
    [self.view addSubview:selectSkinView];
    [self.view addSubview:deleteSkinView];
    [self.view addSubview:loadSkinView];
    [self.view addSubview:selectProgramsView];
    [self.view addSubview:preferencesView];
    [self.view addSubview:aboutView];
    [self.view addSubview:selectFileView];
    [self.view addSubview:statesView];
    [self.view addSubview:calcView];
    [self layoutSubViews];
    
    // Make the strip above the content area black. On iPhone
    // and iPod touch, this turns the status bar black; on iPad,
    // the strip above the content area is not the status bar,
    // but you still want it to be black. The difference is
    // relevant; see the preferredStatusBarStyle method, below.
    [self.view setBackgroundColor:UIColor.blackColor];
    
    [window makeKeyAndVisible];
    window.rootViewController = self;
}

- (UIStatusBarStyle) preferredStatusBarStyle {
    // On iPhone and iPod touch, use LightContent, to get light
    // text that is readable on the black status bar. On iPad,
    // leave the default alone, because we're not actually
    // changing the status bar color there, so we shouldn't be
    // messing with the status bar text either.
    NSString *model = [UIDevice currentDevice].model;
    if ([model hasPrefix:@"iPad"])
        return [super preferredStatusBarStyle];
    else
        return UIStatusBarStyleLightContent;
}

- (UIInterfaceOrientationMask) supportedInterfaceOrientations {
    if (state.orientationMode == 0)
        return UIInterfaceOrientationMaskAll;
    else if (state.orientationMode == 1)
        return UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskPortraitUpsideDown;
    else // state.orientationMode == 2
        return UIInterfaceOrientationMaskLandscape;
}

- (void) layoutSubViews {
    CGRect r;
    if ([UIApplication sharedApplication].isStatusBarHidden)
        r = self.view.bounds;
    else {
        int sbh1 = [UIApplication sharedApplication].statusBarFrame.size.height;
        int sbh2 = window.bounds.size.height - self.view.bounds.size.height;
        int sbh = sbh1 - sbh2;
        r = CGRectMake(self.view.bounds.origin.x, self.view.bounds.origin.y + sbh, self.view.bounds.size.width, self.view.bounds.size.height - sbh);
    }
    if (@available(iOS 11.0, *)) {
        if (window.bounds.size.width > window.bounds.size.height) {
            UIEdgeInsets ei = window.safeAreaInsets;
            r.origin.x += ei.left;
            r.size.width -= ei.right + ei.left;
        }
    }
    printView.frame = r;
    httpServerView.frame = r;
    selectSkinView.frame = r;
    deleteSkinView.frame = r;
    loadSkinView.frame = r;
    selectProgramsView.frame = r;
    preferencesView.frame = r;
    aboutView.frame = r;
    selectFileView.frame = r;
    statesView.frame = r;
    calcView.frame = r;
}

- (void) enterBackground {
    [CalcView enterBackground];
}

- (void) leaveBackground {
    [CalcView leaveBackground];
}

- (void) quit {
    [CalcView quit];
}

static BOOL battery_is_low = NO;
static BOOL battery_is_low_ann = NO;

- (void) batteryLevelChanged {
    BOOL low = [[UIDevice currentDevice] batteryLevel] < 0.205;
    if (low != battery_is_low) {
        battery_is_low = low;
        shell_low_battery();
    }
}

int shell_low_battery() {
    if (battery_is_low_ann != battery_is_low) {
        battery_is_low_ann = battery_is_low;
        skin_update_annunciator(5, battery_is_low, instance.calcView);
    }
    return battery_is_low;
}

+ (void) showMessage:(NSString *) message {
    UIAlertView *errorAlert = [[UIAlertView alloc] initWithTitle:@"Message"
                                                         message:message
                                                        delegate:nil
                                               cancelButtonTitle:@"OK"
                                               otherButtonTitles:nil];
    [errorAlert show];
    [errorAlert release];
}

void shell_message(const char *message) {
    [RootViewController showMessage:[NSString stringWithUTF8String:message]];
}

+ (void) playSound: (int) which {
    AudioServicesPlaySystemSound(soundIDs[which]);
}

- (void) showMain2 {
    [self.view bringSubviewToFront:calcView];
}

+ (void) showMain {
    [instance showMain2];
}

- (void) showPrintOut2 {
    [self.view bringSubviewToFront:printView];
}

+ (void) showPrintOut {
    [instance showPrintOut2];
}

- (void) showHttpServer2 {
    [httpServerView raised];
    [self.view bringSubviewToFront:httpServerView];
}

+ (void) showHttpServer {
    [instance showHttpServer2];
}

- (void) showSelectSkin2 {
    [selectSkinView raised];
    [self.view bringSubviewToFront:selectSkinView];
}

+ (void) showSelectSkin {
    [instance showSelectSkin2];
}

- (void) showPreferences2 {
    [preferencesView raised];
    [self.view bringSubviewToFront:preferencesView];
}

+ (void) showPreferences {
    [instance showPreferences2];
}

- (void) showAbout2 {
    [aboutView raised];
    [self.view bringSubviewToFront:aboutView];
}

+ (void) showAbout {
    [instance showAbout2];
}

- (void) showSelectFile2 {
    [selectFileView raised];
    [self.view bringSubviewToFront:selectFileView];
}

+ (void) showSelectFile {
    [instance showSelectFile2];
}

+ (void) doImport {
    [SelectFileView raiseWithTitle:@"Import Programs" selectTitle:@"Import" types:@"raw,*" selectDir:NO callbackObject:instance callbackSelector:@selector(doImport2:)];
}

- (void) doImport2:(NSString *) path {
    core_import_programs(0, [path UTF8String]);
}

+ (void) doExport:(BOOL)share {
    [instance.selectProgramsView raised:share];
    [instance.self.view bringSubviewToFront:instance.selectProgramsView];
}

- (void) showLoadSkin2 {
    [loadSkinView raised];
    [self.view bringSubviewToFront:loadSkinView];
}

+ (void) showLoadSkin {
    [instance showLoadSkin2];
}

- (void) showStates2:(NSString *)stateName {
    [statesView raised];
    if (stateName != nil) {
        [statesView selectState:stateName];
        [stateName release];
    }
    [self.view bringSubviewToFront:statesView];
}

+ (void) showStates:(NSString *)stateName {
    [instance showStates2:stateName];
}

- (void) showDeleteSkin2 {
    [deleteSkinView raised];
    [self.view bringSubviewToFront:deleteSkinView];
}

+ (void) showDeleteSkin {
    [instance showDeleteSkin2];
}

@end
