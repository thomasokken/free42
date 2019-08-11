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
#import "StateNameWindow.h"
#import "Free42AppDelegate.h"

@implementation StatesWindow

@synthesize current;
@synthesize stateListView;
@synthesize switchToButton;
@synthesize actionMenu;
@synthesize stateListDataSource;
@synthesize stateNameWindow;

- (void) awakeFromNib {
    [actionMenu addItemWithTitle:@"New"];
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

- (void) update:(BOOL)rescan {
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
    [[actionMenu itemAtIndex:2] setEnabled:stateSelected];
    [[actionMenu itemAtIndex:3] setEnabled:stateSelected];
    [[actionMenu itemAtIndex:4] setEnabled:stateSelected && strcmp(name, state.coreName) != 0];
    [[actionMenu itemAtIndex:6] setEnabled:stateSelected];
    if (rescan) {
        [stateListDataSource loadStateNames];
        [stateListView reloadData];
    }
}

- (void) becomeKeyWindow {
    [current setStringValue:[NSString stringWithCString:state.coreName encoding:NSUTF8StringEncoding]];
    [self update:YES];
    [super becomeKeyWindow];
}

- (IBAction) stateListAction:(id)sender {
    [self update:NO];
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

- (void) doNew {
    [stateNameWindow setupWithLabel:@"New State Name:" existingNames:[stateListDataSource getNames] count:[stateListDataSource getNameCount]];
    [NSApp runModalForWindow:stateNameWindow];
    NSString *name = [stateNameWindow selectedName];
    if (name == nil)
        return;
    char fname[FILENAMELEN];
    snprintf(fname, FILENAMELEN, "%s/%s.f42", free42dirname, [name cStringUsingEncoding:NSUTF8StringEncoding]);
    FILE *f = fopen(fname, "w");
    fprintf(f, "24kF");
    fclose(f);
    [self update:YES];
}

- (void) doDuplicate {
    // Copy named state to 'state copy' or 'state copy 2', etc.,
    // keep trying until you find a name that doesn't clash.
    // If active, don't copy the file but export the current
    // state to the new name.
}

- (void) doRename {
    const char *oldname = [self selectedStateName];
    if (oldname == NULL)
        return;
    [stateNameWindow setupWithLabel:[NSString stringWithFormat:@"Rename \"%s\" to:", state.coreName] existingNames:[stateListDataSource getNames] count:[stateListDataSource getNameCount]];
    [NSApp runModalForWindow:stateNameWindow];
    NSString *newname = [stateNameWindow selectedName];
    if (newname == nil)
        return;
    char oldpath[FILENAMELEN], newpath[FILENAMELEN];
    snprintf(oldpath, FILENAMELEN, "%s/%s.f42", free42dirname, oldname);
    snprintf(newpath, FILENAMELEN, "%s/%s.f42", free42dirname, [newname UTF8String]);
    rename(oldpath, newpath);
    if (strcasecmp(oldname, state.coreName) == 0) {
        strncpy(state.coreName, [newname UTF8String], FILENAMELEN);
        [current setStringValue:newname];
    }
    [self update:YES];
}

- (void) doDelete {
    // If active: do nothing. The dialog should prevent this anyway.
    // all other cases: delete the named file
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
            [self doNew];
            break;
        case 2:
            [self doDuplicate];
            break;
        case 3:
            [self doRename];
            break;
        case 4:
            [self doDelete];
            break;
        case 5:
            [self doImport];
            break;
        case 6:
            [self doExport];
            break;
    }
}

- (IBAction) done:(id)sender {
    [self close];
}

@end
