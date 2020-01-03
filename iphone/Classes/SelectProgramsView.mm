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

#import <dirent.h>
#import <sys/stat.h>
#import "SelectProgramsView.h"
#import "CalcView.h"
#import "RootViewController.h"
#import "SelectFileView.h"
#import "shell_skin_iphone.h"
#import "core_main.h"


@implementation SelectProgramsView

@synthesize doneButton;
@synthesize backButton;
@synthesize programTable;

- (id) initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Initialization code
    }
    return self;
}

- (id) initWithCoder:(NSCoder *)coder {
    [super initWithCoder:coder];
    programNames = [[NSMutableArray arrayWithCapacity:10] retain];
    return self;
}

- (void) drawRect:(CGRect)rect {
    // Drawing code
}

- (void) raised:(BOOL)share {
    // This gets called just before the view is raised, every time
    [programNames removeAllObjects];
    char *buf = core_list_programs();
    if (buf != NULL) {
        int count = ((buf[0] & 255) << 24) | ((buf[1] & 255) << 16) | ((buf[2] & 255) << 8) | (buf[3] & 255);
        char *p = buf + 4;
        for (int i = 0; i < count; i++) {
            [programNames addObject:[NSString stringWithUTF8String:p]];
            p += strlen(p) + 1;
        }
        free(buf);
    }
    [programTable reloadData];
    self->share = share;
}

- (IBAction) done {
    // OK
    [RootViewController showMain];
    NSArray *selection = [programTable indexPathsForSelectedRows];
    if (selection == nil)
        return;
    if (share)
        [self doExport2];
    else
        [SelectFileView raiseWithTitle:@"Select Program File Name" selectTitle:@"OK" types:@"raw,*" selectDir:NO callbackObject:self callbackSelector:@selector(doExport:)];
}

static NSString *export_path = nil;

- (void) doExport:(NSString *) path {
    if (export_path != nil)
        [export_path release];
    export_path = [path retain];
    
    const char *cpath = [path UTF8String];
    struct stat st;
    if (stat(cpath, &st) == 0) {
        UIAlertView *errorAlert = [[UIAlertView alloc] initWithTitle:@"File Exists"
                                                             message:@"File exists; overwrite?"
                                                            delegate:self
                                                   cancelButtonTitle:@"Cancel"
                                                   otherButtonTitles:@"OK", nil];
        [errorAlert show];
        [errorAlert release];
        return;
    } else
        [self doExport2];
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
    if (buttonIndex == 1)
        [self doExport2];
}

- (void) doExport2 {
    NSArray *selection = [programTable indexPathsForSelectedRows];
    NSUInteger count = [selection count];
    int *indexes = new int[count];
    for (int i = 0; i < count; i++) {
        NSIndexPath *index = (NSIndexPath *) [selection objectAtIndex:i];
        indexes[i] = (int) [index indexAtPosition:1];
    }
    if (share) {
        NSString *rawName = [programNames objectAtIndex:[[selection objectAtIndex:0] indexAtPosition:1]];
        if ([rawName characterAtIndex:0] == '"') {
            NSRange q = [rawName rangeOfString:@"\"" options:0 range:NSMakeRange(1, [rawName length] - 1)];
            rawName = [rawName substringWithRange:NSMakeRange(1, q.location - 1)];
        } else {
            rawName = @"Untitled";
        }
        NSString *tmp = NSTemporaryDirectory();
        NSString *rawPath = [NSString stringWithFormat:@"%@/%@.raw", tmp, rawName];
        const char *rawPathC = [rawPath UTF8String];
        core_export_programs((int) count, indexes, rawPathC);
        UIActivityViewController *activityViewController = [[UIActivityViewController alloc] initWithActivityItems:@[[NSURL fileURLWithPath:rawPath]] applicationActivities:nil];
        [self.window.rootViewController presentViewController:activityViewController animated:YES completion:nil];
    } else {
        core_export_programs((int) count, indexes, [export_path UTF8String]);
        [export_path release];
        export_path = nil;
    }
    delete[] indexes;
}

- (IBAction) back {
    // Cancel
    [RootViewController showMain];
}

- (UITableViewCell *) tableView:(UITableView *)table cellForRowAtIndexPath:(NSIndexPath *) indexPath {
    NSUInteger n = [indexPath indexAtPosition:1];
    NSString *s = [programNames objectAtIndex:n];
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
    cell.textLabel.text = s;
    return cell;
}

- (NSInteger) tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section {
    NSUInteger n = [programNames count];
    return n;
}

- (void) dealloc {
    [super dealloc];
}


@end
