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

#import "StatesWindow.h"
#import "StateListDataSource.h"
#import "Free42AppDelegate.h"

@implementation StatesWindow

@synthesize current;
@synthesize stateListView;
@synthesize switchToButton;
@synthesize actionMenu;
@synthesize stateListDataSource;

- (void) awakeFromNib {
    [actionMenu addItemWithTitle:@"Duplicate"];
    [actionMenu addItemWithTitle:@"Rename"];
    [actionMenu addItemWithTitle:@"Delete"];
    [actionMenu addItemWithTitle:@"Import"];
    [actionMenu addItemWithTitle:@"Export"];
}

- (const char *) selectedStateName {
    int row = [stateListView selectedRow];
    if (row == -1)
        return NULL;
    else
        return [[stateListDataSource getNames][row] cStringUsingEncoding:NSUTF8StringEncoding];
}

- (void) update {
    const char *name = [self selectedStateName];
    BOOL stateSelected;
    if (name == NULL) {
        [switchToButton setTitle:@"Switch To"];
        stateSelected = NO;
    } else {
        if (strcmp(name, state.coreName) == 0)
            [switchToButton setTitle:@"Reload"];
        else
            [switchToButton setTitle:@"Switch To"];
        stateSelected = YES;
    }
    [switchToButton setEnabled:stateSelected];
    [[actionMenu itemAtIndex:1] setEnabled:stateSelected];
    [[actionMenu itemAtIndex:2] setEnabled:stateSelected];
    [[actionMenu itemAtIndex:3] setEnabled:stateSelected];
    [[actionMenu itemAtIndex:5] setEnabled:stateSelected];
}

- (void) becomeKeyWindow {
    [current setStringValue:[NSString stringWithCString:state.coreName encoding:NSUTF8StringEncoding]];
    [stateListDataSource loadStateNames];
    [stateListView reloadData];
    [self update];
    [super becomeKeyWindow];
}

- (IBAction) stateListAction:(id)sender {
    [self update];
}

- (IBAction) stateListDoubleAction:(id)sender {
    [self switchTo:sender];
}

- (IBAction) switchTo:(id)sender {
    const char *name = [self selectedStateName];
    if (name == NULL)
        return;
    [Free42AppDelegate loadState:name];
    [self close];
}

- (void) doDuplicate {
    // Copy named state to 'state copy' or 'state copy 2', etc.,
    // keep trying until you find a name that doesn't clash.
    // If active, don't copy the file but export the current
    // state to the new name.
}

- (void) doRename {
    // Prompt for new name using a simple text box dialog,
    // not a file selection dialog.
    // If active: update state.coreName as well
}

- (void) doDelete {
    // If active: do hard reset first
    // all cases: delete the named file
}

- (void) doImport {
    // Get file using file selector, then copy it into the Free42
    // directory. Confirmation in case of name clash. Confirmation
    // whether to switch to the newly imported state.
    // Or, keeping it simple: forbid name clash; do not load.
}

- (void) doExport {
    // Get filename using file selector, then either copy
    // the state file given by 'selectedStateName' (if it's not
    // the active state) or export the active state (if it is).
}

- (IBAction) menuSelected:(id)sender {
    int sel = [actionMenu indexOfSelectedItem];
    switch (sel) {
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
            [self doImport];
            break;
        case 5:
            [self doExport];
            break;
    }
}

- (IBAction) done:(id)sender {
    [self close];
}

@end
