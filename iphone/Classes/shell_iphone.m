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

#import <AudioToolbox/AudioServices.h>

#import "MainView.h"
#import "PrintOutView.h"
#import "HTTPServerView.h"
#import "SelectSkinView.h"
#import "PreferencesView.h"
#import "AboutView.h"
#import "SelectFileView.h"
#import "shell_iphone.h"

static SystemSoundID soundIDs[12];

static shell_iphone *instance;

@implementation shell_iphone

@synthesize window;

@synthesize containerView;
@synthesize mainView;
@synthesize printOutView;
@synthesize httpServerView;
@synthesize selectSkinView;
@synthesize preferencesView;
@synthesize aboutView;
@synthesize selectFileView;


- (void) applicationDidFinishLaunching:(UIApplication *)application {
    // Override point for customization after application launch
	instance = self;

	const char *sound_names[] = { "tone0", "tone1", "tone2", "tone3", "tone4", "tone5", "tone6", "tone7", "tone8", "tone9", "squeak", "click" };
	for (int i = 0; i < 12; i++) {
		NSString *name = [NSString stringWithCString:sound_names[i]];
		NSString *path = [[NSBundle mainBundle] pathForResource:name ofType:@"wav"];
		OSStatus status = AudioServicesCreateSystemSoundID((CFURLRef)[NSURL fileURLWithPath:path], &soundIDs[i]);
		if (status)
			NSLog(@"error loading sound:  %d", name);
	}

	[containerView addSubview:printOutView];
	[containerView addSubview:httpServerView];
	[containerView addSubview:selectSkinView];
	[containerView addSubview:preferencesView];
	[containerView addSubview:aboutView];
	[containerView addSubview:selectFileView];
	[containerView addSubview:mainView];
    [window makeKeyAndVisible];
}

- (void)applicationWillTerminate:(UIApplication *)application {
	[MainView quit];
}

- (void) dealloc {
    [window release];
    [mainView release];
    [printOutView release];
    [httpServerView release];
    [selectSkinView release];
    [preferencesView release];
    [aboutView release];
    [super dealloc];
}

+ (void) playSound: (int) which {
	AudioServicesPlaySystemSound(soundIDs[which]);
}

- (void) showMain2 {
	[containerView bringSubviewToFront:mainView];
}

+ (void) showMain {
	[instance showMain2];
}

- (void) showPrintOut2 {
	[printOutView raised];
	[containerView bringSubviewToFront:printOutView];
}

+ (void) showPrintOut {
	[instance showPrintOut2];
}

- (void) showHttpServer2 {
	[httpServerView raised];
	[containerView bringSubviewToFront:httpServerView];
}

+ (void) showHttpServer {
	[instance showHttpServer2];
}

- (void) showSelectSkin2 {
	[selectSkinView raised];
	[containerView bringSubviewToFront:selectSkinView];
}

+ (void) showSelectSkin {
	[instance showSelectSkin2];
}

- (void) showPreferences2 {
	[preferencesView raised];
	[containerView bringSubviewToFront:preferencesView];
}

+ (void) showPreferences {
	[instance showPreferences2];
}

- (void) showAbout2 {
	[aboutView raised];
	[containerView bringSubviewToFront:aboutView];
}

+ (void) showAbout {
	[instance showAbout2];
}

- (void) showSelectFile2 {
	[selectFileView raised];
	[containerView bringSubviewToFront:selectFileView];
}

+ (void) showSelectFile {
	[instance showSelectFile2];
}

@end
