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
#import <sys/stat.h>
#import <dirent.h>

#import "Free42AppDelegate.h"
#import "RootViewController.h"
#import "StatesView.h"
#import "core_main.h"

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
        NSString *appVersion = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"];
        strcpy(version, [appVersion UTF8String]);
        // Version string consists of up to four dot-separated numbers.
        // If there are four, change the last ".nn" to a letter.
        int pos, num;
        if (sscanf(version, "%*d.%*d.%*d.%n%d", &pos, &num) == 1) {
            version[pos - 1] = 'a' + num - 1;
            version[pos] = 0;
        }
    }   
    return version;
}

static BOOL urlInInbox(NSURL *url) {
    if (![url isFileURL])
        return NO;
    NSString *path = [url path];
    if (path == nil)
        return NO;
    const char *cpath = [path UTF8String];
    struct stat st;
    if (stat(cpath, &st) != 0 || !S_ISREG(st.st_mode))
        return NO;
    NSString *inbox = [NSString stringWithFormat:@"%@/Documents/Inbox/", NSHomeDirectory()];
    return [path hasPrefix:inbox];
}

- (BOOL) application:(UIApplication *)app
            openURL:(NSURL *)url
            options:(NSDictionary<UIApplicationOpenURLOptionsKey, id> *)options; {
    NSMutableArray *fromNames = [NSMutableArray array];
    // If the URL is not a file:/// URL pointing to a file in our Inbox, copy
    // the URL contents into a temporary location. Otherwise, just proceed to Inbox
    // processing.
    if (!urlInInbox(url)) {
        NSString *ext = [url pathExtension];
        if (ext != nil
            && ([ext caseInsensitiveCompare:@"f42"] == NSOrderedSame
                || [ext caseInsensitiveCompare:@"raw"] == NSOrderedSame)) {
            NSString *name = [url lastPathComponent];
            NSString *destPath = [NSString stringWithFormat:@"%@/Documents/_TEMP_/%@", NSHomeDirectory(), name];

            // Create temp directory, or clear it out if it existed already
            if (mkdir("_TEMP_", 0755) != 0 && errno == EEXIST) {
                DIR *dir = opendir("_TEMP_");
                if (dir != NULL) {
                    struct dirent *d;
                    char path[1024];
                    while ((d = readdir(dir)) != NULL) {
                        if (strcmp(d->d_name, ".") != 0 && strcmp(d->d_name, "..") != 0) {
                            snprintf(path, 1024, "_TEMP_/%s", d->d_name);
                            remove(path);
                        }
                    }
                    closedir(dir);
                }
            }
            
            NSError *err = nil;
            NSFileManager *fm = [NSFileManager defaultManager];
            BOOL secure = [url isFileURL] && ![fm isReadableFileAtPath:[url path]];
            if (secure)
                secure = [url startAccessingSecurityScopedResource];
            [fm copyItemAtURL:url toURL:[NSURL fileURLWithPath:destPath] error:&err];
            if (secure)
                [url stopAccessingSecurityScopedResource];
            if (err != nil)
                NSLog(@"File copy failed: %@", [err localizedDescription]);
            else
                [fromNames addObject:[NSString stringWithFormat:@"_TEMP_/%@", name]];
        }
    }
    // Handle all files with names ending in .f42 or .raw that happen to be in our Inbox.
    DIR *dir = opendir("Inbox");
    if (dir != NULL) {
        struct dirent *d;
        while ((d = readdir(dir)) != NULL) {
            size_t len = strlen(d->d_name);
            if (len < 5 || (strcasecmp(d->d_name + len - 4, ".f42") != 0
                         && strcasecmp(d->d_name + len - 4, ".raw") != 0))
                continue;
            [fromNames addObject:[NSString stringWithFormat:@"Inbox/%@", [NSString stringWithUTF8String:d->d_name]]];
        }
        closedir(dir);
    }
    if ([fromNames count] == 0) {
        [RootViewController showMessage:@"Import failed."];
        return NO;
    }
    
    NSString *firstState = nil;
    int nProgs = 0;
    for (int i = 0; i < [fromNames count]; i++) {
        NSString *fromPath = [fromNames objectAtIndex:i];
        NSString *fromName = [fromPath lastPathComponent];
        const char *fromPathC = [fromPath UTF8String];
        size_t clen = strlen(fromPathC);
        if (strcasecmp(fromPathC + clen - 4, ".f42") == 0) {
            FILE *f = fopen(fromPathC, "r");
            if (f == NULL) {
                remove(fromPathC);
                continue;
            }
            char sig[5];
            size_t n = fread(sig, 1, 4, f);
            fclose(f);
            sig[4] = 0;
            if (n != 4 || strcmp(sig, FREE42_MAGIC_STR) != 0) {
                remove(fromPathC);
                continue;
            }
            fromName = [fromName substringToIndex:[fromName length] - 4];
            NSString *toName = fromName;
            NSString *toPath = [NSString stringWithFormat:@"config/%@.f42", toName];
            struct stat st;
            if (stat([toPath UTF8String], &st) == 0) {
                toName = [StatesView makeCopyName:toName];
                toPath = [NSString stringWithFormat:@"config/%@.f42", toName];
            }
            mkdir("config", 0755);
            rename(fromPathC, [toPath UTF8String]);
            if (firstState == nil)
                firstState = toName;
        } else {
            // Must be .raw, because in the first loop, we only collect
            // files with extensions .f42 or .raw
            // Make sure the file ends in Cx xx 0D before importing it.
            FILE *f = fopen(fromPathC, "r");
            if (f != NULL) {
                fseek(f, -3, SEEK_END);
                char sig[3];
                size_t n = fread(sig, 1, 3, f);
                fclose(f);
                if (n == 3 && (sig[0] & 0x0f0) == 0x0c0 && sig[2] == 0x0d) {
                    core_import_programs(0, fromPathC);
                    nProgs++;
                }
            }
            remove(fromPathC);
        }
    }
    
    // Remove the temp directory we created for handling URL imports
    rmdir("_TEMP_");
    if (firstState == nil) {
        if (nProgs == 0) {
            [RootViewController showMessage:@"Import failed."];
            return NO;
        } else {
            NSString *message = [NSString stringWithFormat:@"%d raw file%s imported.", nProgs, nProgs == 1 ? "" : "s"];
            [RootViewController showMessage:message];
            return YES;
        }
    } else {
        [RootViewController performSelectorOnMainThread:@selector(showStates:) withObject:[firstState retain] waitUntilDone:NO];
        return YES;
    }
}

@end
