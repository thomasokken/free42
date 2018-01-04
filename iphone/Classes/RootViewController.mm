/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2018  Thomas Okken
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
#import "RootViewController.h"
#import "shell.h"
#import "shell_skin_iphone.h"
#import "core_main.h"

static SystemSoundID soundIDs[11];

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


- (void) awakeFromNib {
    instance = self;

    const char *sound_names[] = { "tone0", "tone1", "tone2", "tone3", "tone4", "tone5", "tone6", "tone7", "tone8", "tone9", "squeak" };
    for (int i = 0; i < 11; i++) {
        NSString *name = [NSString stringWithCString:sound_names[i] encoding:NSUTF8StringEncoding];
        NSString *path = [[NSBundle mainBundle] pathForResource:name ofType:@"wav"];
        OSStatus status = AudioServicesCreateSystemSoundID((CFURLRef)[NSURL fileURLWithPath:path], &soundIDs[i]);
        if (status)
            NSLog(@"error loading sound: %@", name);
    }
    
    CGRect bounds = CGRectMake(window.bounds.origin.x, window.bounds.origin.y + 20,
                               window.bounds.size.width, window.bounds.size.height - 20);
    printView.frame = bounds;
    [window addSubview:printView];
    httpServerView.frame = bounds;
    [window addSubview:httpServerView];
    selectSkinView.frame = bounds;
    [window addSubview:selectSkinView];
    selectProgramsView.frame = bounds;
    [window addSubview:selectProgramsView];
    preferencesView.frame = bounds;
    [window addSubview:preferencesView];
    aboutView.frame = bounds;
    [window addSubview:aboutView];
    selectFileView.frame = bounds;
    [window addSubview:selectFileView];
    calcView.frame = bounds;
    [window addSubview:calcView];
    [window makeKeyAndVisible];
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

+ (void) playSound: (int) which {
    AudioServicesPlaySystemSound(soundIDs[which]);
}

- (void) showMain2 {
    [window bringSubviewToFront:calcView];
}

+ (void) showMain {
    [instance showMain2];
}

- (void) showPrintOut2 {
    [window bringSubviewToFront:printView];
}

+ (void) showPrintOut {
    [instance showPrintOut2];
}

- (void) showHttpServer2 {
    [httpServerView raised];
    [window bringSubviewToFront:httpServerView];
}

+ (void) showHttpServer {
    [instance showHttpServer2];
}

- (void) showSelectSkin2 {
    [selectSkinView raised];
    [window bringSubviewToFront:selectSkinView];
}

+ (void) showSelectSkin {
    [instance showSelectSkin2];
}

- (void) showPreferences2 {
    [preferencesView raised];
    [window bringSubviewToFront:preferencesView];
}

+ (void) showPreferences {
    [instance showPreferences2];
}

- (void) showAbout2 {
    [aboutView raised];
    [window bringSubviewToFront:aboutView];
}

+ (void) showAbout {
    [instance showAbout2];
}

- (void) showSelectFile2 {
    [selectFileView raised];
    [window bringSubviewToFront:selectFileView];
}

+ (void) showSelectFile {
    [instance showSelectFile2];
}

+ (void) doImport {
    [SelectFileView raiseWithTitle:@"Import Programs" selectTitle:@"Import" types:@"raw,*" selectDir:NO callbackObject:instance callbackSelector:@selector(doImport2:)];
}

static FILE *import_file;

static int my_shell_read(char *buf, int buflen) {
    ssize_t nread;
    if (import_file == NULL)
        return -1;
    nread = fread(buf, 1, buflen, import_file);
    if (nread != buflen && ferror(import_file)) {
        fclose(import_file);
        import_file = NULL;
        [RootViewController showMessage:@"An error occurred while reading the file; import was terminated prematurely."];
        return -1;
    } else
        return (int) nread;
}

- (void) doImport2:(NSString *) path {
    import_file = fopen([path cStringUsingEncoding:NSUTF8StringEncoding], "r");
    if (import_file == NULL) {
        [RootViewController showMessage:@"The file could not be opened."];
    } else {
        import_programs(my_shell_read);
        if (import_file != NULL) {
            fclose(import_file);
            import_file = NULL;
        }
    }
}

+ (void) doExport {
    [instance.selectProgramsView raised];
    [instance.window bringSubviewToFront:instance.selectProgramsView];
}

@end

static int (*writer_callback)(const char *buf, int buflen);

int shell_write(const char *buf, int buflen) {
    return writer_callback(buf, buflen);
}

void export_programs(int count, const int *indexes, int (*writer)(const char *buf, int buflen)) {
    writer_callback = writer;
    core_export_programs(count, indexes, NULL);
}

static int (*reader_callback)(char *buf, int buflen);

int shell_read(char *buf, int buflen) {
    return reader_callback(buf, buflen);
}

void import_programs(int (*reader)(char *buf, int buflen)) {
    reader_callback = reader;
    core_import_programs(NULL);
}
