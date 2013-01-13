/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2013  Thomas Okken
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
#import "PreferencesView.h"
#import "AboutView.h"
#import "SelectFileView.h"
#import "Free42AppDelegate.h"

static SystemSoundID soundIDs[11];

static Free42AppDelegate *instance;
static char version[32] = "";

@implementation Free42AppDelegate

@synthesize window;

@synthesize containerView;
@synthesize calcView;
@synthesize printView;
@synthesize httpServerView;
@synthesize selectSkinView;
@synthesize preferencesView;
@synthesize aboutView;
@synthesize selectFileView;


- (void) applicationDidFinishLaunching:(UIApplication *)application {
    // Override point for customization after application launch
	instance = self;

	const char *sound_names[] = { "tone0", "tone1", "tone2", "tone3", "tone4", "tone5", "tone6", "tone7", "tone8", "tone9", "squeak" };
	for (int i = 0; i < 11; i++) {
		NSString *name = [NSString stringWithCString:sound_names[i] encoding:NSUTF8StringEncoding];
		NSString *path = [[NSBundle mainBundle] pathForResource:name ofType:@"wav"];
		OSStatus status = AudioServicesCreateSystemSoundID((CFURLRef)[NSURL fileURLWithPath:path], &soundIDs[i]);
		if (status)
			NSLog(@"error loading sound: %@", name);
	}
	
	[containerView addSubview:printView];
	[containerView addSubview:httpServerView];
	[containerView addSubview:selectSkinView];
	[containerView addSubview:preferencesView];
	[containerView addSubview:aboutView];
	[containerView addSubview:selectFileView];
	[containerView addSubview:calcView];
    [window makeKeyAndVisible];
}

- (void) applicationDidEnterBackground:(UIApplication *)application {
    [CalcView enterBackground];
}

- (void) applicationWillEnterForeground:(UIApplication *)application {
    [CalcView leaveBackground];
}

- (void) applicationWillTerminate:(UIApplication *)application {
	[CalcView quit];
}

- (void) dealloc {
    [window release];
    [calcView release];
    [printView release];
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
	[containerView bringSubviewToFront:calcView];
}

+ (void) showMain {
	[instance showMain2];
}

- (void) showPrintOut2 {
	[containerView bringSubviewToFront:printView];
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

// C version of getVersion, needed by the HTTP Server

const char *get_version() {
	return [Free42AppDelegate getVersion];
}

@end
