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

#import "Free42AppDelegate.h"
#import "RootViewController.h"

static Free42AppDelegate *instance;
static char version[32] = "";

@implementation Free42AppDelegate

@synthesize rootViewController;

- (void) applicationDidFinishLaunching:(UIApplication *)application {
    // Override point for customization after application launch
    instance = self;

    [[UIDevice currentDevice] setBatteryMonitoringEnabled:YES];
    [[NSNotificationCenter defaultCenter] addObserver:rootViewController selector:@selector(batteryLevelChanged) name:UIDeviceBatteryLevelDidChangeNotification object:nil];
    [rootViewController batteryLevelChanged];
}

- (void) applicationDidEnterBackground:(UIApplication *)application {
    [rootViewController enterBackground];
}

- (void) applicationWillEnterForeground:(UIApplication *)application {
    [rootViewController batteryLevelChanged];
    [rootViewController leaveBackground];
}

- (void) applicationWillTerminate:(UIApplication *)application {
    [rootViewController quit];
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

@end
