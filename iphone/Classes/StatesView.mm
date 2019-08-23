/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2019  Thomas Okken
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
#import "CalcView.h"
#import "StatesView.h"
#import "RootViewController.h"


@implementation StatesView

@synthesize switchToButton;
@synthesize stateTable;

- (id) initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Initialization code
    }
    return self;
}

- (id) initWithCoder:(NSCoder *)coder {
    [super initWithCoder:coder];
    stateNames = [[NSMutableArray arrayWithCapacity:10] retain];
    selectedIndex = -1;
    return self;
}

- (void) drawRect:(CGRect)rect {
    // Drawing code
}

- (void) raised {
    // This gets called just before the view is raised, every time
    [stateNames removeAllObjects];
    int index = 0;
    selectedIndex = -1;
    DIR *dir = opendir("config");
    struct dirent *d;
    while ((d = readdir(dir)) != NULL) {
        size_t len = strlen(d->d_name);
        if (len < 5 || strcmp(d->d_name + len - 4, ".f42") != 0)
            continue;
        d->d_name[len - 4] = 0;
        NSString *s = [NSString stringWithUTF8String:d->d_name];
        [stateNames addObject:s];
        if (strcasecmp(d->d_name, state.coreName) == 0)
            selectedIndex = index;
        index++;
    }
    closedir(dir);
    [switchToButton setEnabled:NO];
    [switchToButton setTitle:@"Switch To"];
    mode = 0;
    [stateTable reloadData];
}

- (IBAction) switchTo {
    
}

- (IBAction) more {
    UIActionSheet *menu;
    switch (mode) {
        case 0:
            menu = [[UIActionSheet alloc] initWithTitle:@"States Menu" delegate:self cancelButtonTitle:@"Cancel" destructiveButtonTitle:nil otherButtonTitles:@"New", nil];
            break;
        case 1:
            menu = [[UIActionSheet alloc] initWithTitle:@"States Menu" delegate:self cancelButtonTitle:@"Cancel" destructiveButtonTitle:nil otherButtonTitles:@"New", @"Duplicate", @"Rename", @"Share", nil];
            break;
        case 2:
            menu = [[UIActionSheet alloc] initWithTitle:@"States Menu" delegate:self cancelButtonTitle:@"Cancel" destructiveButtonTitle:nil otherButtonTitles:@"New", @"Duplicate", @"Rename", @"Delete", @"Share", nil];
            break;
    }
    [menu showInView:self];
    [menu release];
}

- (IBAction) done {
    [RootViewController showMain];
}

- (void) doNew {

}

- (void) doDuplicate {

}

- (void) doRename {

}

- (void) doDelete {
    NSIndexPath *index = [stateTable indexPathForSelectedRow];
    if (index == nil)
        return;
    int idx = (int) [index indexAtPosition:1];
    NSString *stateName = [stateNames objectAtIndex:idx];
    remove([[NSString stringWithFormat:@"%@.f42", stateName] UTF8String]);
    [self raised];
}

- (void) doShare {

}

- (void) actionSheet:(UIActionSheet *) actionSheet clickedButtonAtIndex:(NSInteger) buttonIndex {
    switch (mode) {
        case 0:
            switch (buttonIndex) {
                case 0:
                    [self doNew];
                    break;
                case 1:
                    // Cancel
                    break;
            }
        case 1:
            switch (buttonIndex) {
                case 0:
                    [self doNew];
                    break;
                case 1:
                    [self doDuplicate];
                    break;
                case 2:
                    [self doRename];
                    break;
                case 3:
                    [self doShare];
                    break;
                case 4:
                    // Cancel
                    break;
            }
        case 2:
            switch (buttonIndex) {
                case 0:
                    [self doNew];
                    break;
                case 1:
                    [self doDuplicate];
                    break;
                case 2:
                    [self doRename];
                    break;
                case 3:
                    [self doDelete];
                    break;
                case 4:
                    [self doShare];
                    break;
                case 5:
                    // Cancel
                    break;
            }
    }
}

- (void) tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    NSUInteger n = [indexPath indexAtPosition:1];
    NSString *s = [stateNames objectAtIndex:n];
    [switchToButton setEnabled:YES];
    if (strcmp([s UTF8String], state.coreName) == 0) {
        [switchToButton setTitle:@"Reload"];
        mode = 1;
    } else {
        [switchToButton setTitle:@"Switch To"];
        mode = 2;
    }
}

- (UITableViewCell *) tableView:(UITableView *)table cellForRowAtIndexPath:(NSIndexPath *) indexPath {
    NSUInteger n = [indexPath indexAtPosition:1];
    NSString *s = [stateNames objectAtIndex:n];
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
    cell.textLabel.text = s;
    cell.accessoryType = selectedIndex == (int) n ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
    return cell;
}

- (NSInteger) tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section {
    NSUInteger n = [stateNames count];
    return n;
}

- (void) dealloc {
    [super dealloc];
}


@end
