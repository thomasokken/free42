/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2017  Thomas Okken
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

- (void) raised {
    // This gets called just before the view is raised, every time
    [programNames removeAllObjects];
    char buf[1024];
    int count = core_list_programs(buf, 1024);
    char *p = buf;
    for (int i = 0; i < count; i++) {
        // TODO: I'm using ISO-8859-1 encoding, but of course that's wrong.
        // I should write an HP-to-UTF8 translator to do this properly.
        [programNames addObject:[NSString stringWithCString:p encoding:NSISOLatin1StringEncoding]];
        p += strlen(p) + 1;
    }
    [programTable reloadData];
}

- (IBAction) done {
    // OK
    // Need to raise the main window now, in case the SelectFileView is cancelled
    [RootViewController showMain];
    NSArray *selection = [programTable indexPathsForSelectedRows];
    if (selection == nil)
        return;
    [SelectFileView raiseWithTitle:@"Select Program File Name" selectTitle:@"OK" types:@"raw,*" selectDir:NO callbackObject:self callbackSelector:@selector(doExport:)];
}

static FILE *export_file = NULL;
static NSString *export_path = nil;

static int my_shell_write(const char *buf, int buflen) {
    size_t written;
    if (export_file == NULL)
        return 0;
    written = fwrite(buf, 1, buflen, export_file);
    if (written != buflen) {
        [RootViewController showMessage:@"Export failed; there was an error writing to the file."];
        fclose(export_file);
        export_file = NULL;
        return 0;
    } else
        return 1;
}

- (void) doExport:(NSString *) path {
    if (export_path != nil)
        [export_path release];
    export_path = [path retain];
    
    const char *cpath = [path cStringUsingEncoding:NSUTF8StringEncoding];
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
    export_file = fopen([export_path cStringUsingEncoding:NSUTF8StringEncoding], "w");
    if (export_file == NULL) {
        [RootViewController showMessage:@"Export failed; could not create the file."];
        return;
    }
    NSArray *selection = [programTable indexPathsForSelectedRows];
    NSUInteger count = [selection count];
    int *indexes = new int[count];
    for (int i = 0; i < count; i++) {
        NSIndexPath *index = (NSIndexPath *) [selection objectAtIndex:i];
        indexes[i] = (int) [index indexAtPosition:1];
    }
    export_programs((int) count, indexes, my_shell_write);
    delete[] indexes;
    if (export_file != NULL) {
        fclose(export_file);
        export_file = NULL;
    }
    [export_path release];
    export_path = nil;
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
