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

#import <sys/stat.h>
#import <unistd.h>
#import <UIKit/UIKit.h>
#import "Classes/Free42AppDelegate.h"

int main(int argc, char *argv[]) {
    
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    
    NSLog(@"Free42 %s, built %s %s", [Free42AppDelegate getVersion], __DATE__, __TIME__);
    
    // This is so that the remainder of the Free42 code can assume that the current
    // directory is the home directory; this will also be the top-level directory that
    // users can navigate with the built-in HTTP server.
    NSString *homedir = [NSString stringWithFormat:@"%@/Documents", NSHomeDirectory()];
    NSLog(@"home = %@", homedir);
    chdir([homedir UTF8String]);
    mkdir("skins", 0755);

    int retVal = UIApplicationMain(argc, argv, nil, nil);
    [pool release];
    return retVal;
}
