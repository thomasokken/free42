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

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#import <AudioToolbox/AudioServices.h>

#import "shell_iphone.h"

static SystemSoundID soundIDs[12];

// From simpleserver.c
void handle_client(int csock);

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

	// TODO: This is highly preliminary. The server should only run in a special
	// mode, in which the normal Free42 UI is disabled, and where no programs are
	// running. One reason for this is that this server should not be running all
	// the time; the other is the possibility of things getting messed up if
	// imports and exports are done while programs are running etc.
	[self performSelectorInBackground:@selector(start_simple_server) withObject:NULL];
	
	[containerView addSubview:printOutView];
	[containerView addSubview:httpServerView];
	[containerView addSubview:selectSkinView];
	[containerView addSubview:preferencesView];
	[containerView addSubview:aboutView];
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

- (void) start_simple_server {
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    int port = 9090;
    int backlog = 32;
    int ssock, csock;
    struct sockaddr_in sa, ca;
    int err;
	
    ssock = socket(AF_INET, SOCK_STREAM, 0);
    if (ssock == -1) {
		err = errno;
		NSLog(@"Could not create socket: %s (%d)\n", strerror(err), err);
		return;
    }
	
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = INADDR_ANY;
    err = bind(ssock, (struct sockaddr *) &sa, sizeof(sa));
    if (err != 0) {
		err = errno;
		NSLog(@"Could not bind socket to port %d: %s (%d)\n", port, strerror(err), err);
		return;
    }
	
    err = listen(ssock, backlog);
    if (err != 0) {
		err = errno;
		NSLog(@"Could not listen (backlog = %d): %s (%d)\n", backlog, strerror(err), err);
		return;
    }
	
    while (1) {
		unsigned int n = sizeof(ca);
		char cname[256];
		csock = accept(ssock, (struct sockaddr *) &ca, &n);
		if (csock == -1) {
			err = errno;
			NSLog(@"Could not accept connection from client: %s (%d)\n", strerror(err), err);
			return;
		}
		inet_ntop(AF_INET, &ca.sin_addr, cname, sizeof(cname));
		NSLog(@"Accepted connection from %s\n", cname);
		handle_client(csock);
    }
	[pool release];
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
	[containerView bringSubviewToFront:printOutView];
}

+ (void) showPrintOut {
	[instance showPrintOut2];
}

- (void) showHttpServer2 {
	[containerView bringSubviewToFront:httpServerView];
}

+ (void) showHttpServer {
	[instance showHttpServer2];
}

- (void) showSelectSkin2 {
	[containerView bringSubviewToFront:selectSkinView];
}

+ (void) showSelectSkin {
	[instance showSelectSkin2];
}

- (void) showPreferences2 {
	[containerView bringSubviewToFront:preferencesView];
}

+ (void) showPreferences {
	[instance showPreferences2];
}

- (void) showAbout2 {
	[containerView bringSubviewToFront:aboutView];
}

+ (void) showAbout {
	[instance showAbout2];
}

@end
