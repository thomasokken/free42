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

#import <sys/stat.h>
#import "StatesWindow.h"
#import "StateListDataSource.h"
#import "StateNameWindow.h"
#import "Free42AppDelegate.h"
#import "FileOpenPanel.h"
#import "FileSavePanel.h"
#import "core_main.h"

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

- (void) reset {
    [stateListView deselectAll:self];
}

- (const char *) selectedStateName {
    int row = [stateListView selectedRow];
    if (row == -1)
        return NULL;
    else
        return [[stateListDataSource getNames][row] UTF8String];
}

- (void) updateUI:(BOOL)rescan {
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
    [current setStringValue:[NSString stringWithUTF8String:state.coreName]];
    [self updateUI:YES];
    [super becomeKeyWindow];
}

- (IBAction) stateListAction:(id)sender {
    [self updateUI:NO];
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
    snprintf(fname, FILENAMELEN, "%s/%s.f42", free42dirname, [name UTF8String]);
    FILE *f = fopen(fname, "w");
    fprintf(f, "24kF");
    fclose(f);
    [self updateUI:YES];
}

- (BOOL) copyStateFrom:(const char *)orig_name to:(const char *)copy_name {
    FILE *fin = fopen(orig_name, "r");
    FILE *fout = fopen(copy_name, "w");
    if (fin != NULL && fout != NULL) {
        char buf[1024];
        int n;
        while ((n = fread(buf, 1, 1024, fin)) > 0)
            fwrite(buf, 1, n, fout);
        if (ferror(fin) || ferror(fout))
            goto duplication_failed;
        fclose(fin);
        fclose(fout);
        return YES;
    } else {
        duplication_failed:
        if (fin != NULL)
            fclose(fin);
        if (fout != NULL)
            fclose(fout);
        remove(copy_name);
        return NO;
    }
}

- (void) doDuplicate {
    const char *name = [self selectedStateName];
    if (name == NULL)
        return;
    char copy_name[FILENAMELEN];
    snprintf(copy_name, FILENAMELEN - 4, "%s/%s", free42dirname, name);
    int n = 0;
    char suffix[10];
    while (true) {
        n++;
        if (n == 1000) {
            name_too_long:
            [Free42AppDelegate showMessage:@"State name is too long to duplicate; please rename it to something shorter first." withTitle:@"Error"];
            return;
        }
        if (n == 1)
            strcpy(suffix, " copy");
        else
            sprintf(suffix, " copy %d", n);
        int len = strlen(suffix);
        int pos = strlen(copy_name);
        if (pos + len >= FILENAMELEN - 4) {
            pos--;
            while (pos > 0 && (copy_name[pos] & 0xc0) == 0xc0)
                pos--;
            if (strchr(copy_name + pos, '/'))
                goto name_too_long;
        }
        copy_name[pos] = 0;
        strcat(copy_name, suffix);
        strcat(copy_name, ".f42");
        struct stat st;
        if (stat(copy_name, &st) == 0)
            // File exists; try next suffix
            snprintf(copy_name, FILENAMELEN - 4, "%s/%s", free42dirname, name);
        else
            break;
    }
    // Once we get here, copy_name contains a valid name for creating the duplicate.
    // What we do next depends on whether the selected state is the currently active
    // one. If it is, we'll call core_save_state(), to make sure the duplicate
    // actually matches the most up-to-date state; otherwise, we can simply copy
    // the existing state file.
    if (strcasecmp(name, state.coreName) == 0)
        core_save_state(copy_name);
    else {
        char orig_name[FILENAMELEN];
        snprintf(orig_name, FILENAMELEN, "%s/%s.f42", free42dirname, name);
        if (![self copyStateFrom:orig_name to:copy_name])
            [Free42AppDelegate showMessage:@"State duplication failed." withTitle:@"Error"];
    }
    [self updateUI:YES];
}

- (void) doRename {
    const char *oldname = [self selectedStateName];
    if (oldname == NULL)
        return;
    [stateNameWindow setupWithLabel:[NSString stringWithFormat:@"Rename \"%s\" to:", oldname] existingNames:[stateListDataSource getNames] count:[stateListDataSource getNameCount]];
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
    [self updateUI:YES];
}

- (void) doDelete {
    const char *name = [self selectedStateName];
    if (name == NULL)
        return;
    if (strcasecmp(name, state.coreName) == 0)
        return;
    NSAlert *alert = [[[NSAlert alloc] init] autorelease];
    [alert addButtonWithTitle:@"Delete"];
    [alert addButtonWithTitle:@"Cancel"];
    [alert setMessageText:@"Delete state?"];
    [alert setInformativeText:[NSString stringWithFormat:@"Are you sure you want to delete the state \"%s\"?", name]];
    [alert setAlertStyle:NSWarningAlertStyle];
    if ([alert runModal] != NSAlertFirstButtonReturn)
        return;
    char statePath[FILENAMELEN];
    snprintf(statePath, FILENAMELEN, "%s/%s.f42", free42dirname, name);
    remove(statePath);
    [self updateUI:YES];
}

- (void) doImport {
    FileOpenPanel *openDlg = [FileOpenPanel panelWithTitle:@"Import State" types:@"Free42 State;f42;All Files;*"];
    if ([openDlg runModal] != NSOKButton)
        return;
    NSArray *paths = [openDlg paths];
    for (int i = 0; i < [paths count]; i++) {
        NSString *path = [paths objectAtIndex:i];
        NSString *name = [path lastPathComponent];
        int len = [name length];
        if (len > 4 && [[name substringFromIndex:len - 4] caseInsensitiveCompare:@".f42"] == NSOrderedSame)
            name = [name substringToIndex:len - 4];
        const char *orig_path = [path UTF8String];
        char dest_path[FILENAMELEN];
        snprintf(dest_path, FILENAMELEN, "%s/%s.f42", free42dirname, [name UTF8String]);
        struct stat st;
        if (stat(dest_path, &st) == 0)
            [Free42AppDelegate showMessage:[NSString stringWithFormat:@"A state named \"%@\" already exists.", name] withTitle:@"Error"];
        else if (![self copyStateFrom:orig_path to:dest_path])
            [Free42AppDelegate showMessage:@"State import failed." withTitle:@"Error"];
    }
    [self updateUI:YES];
}

- (void) doExport {
    const char *name = [self selectedStateName];
    if (name == NULL)
        return;
    FileSavePanel *saveDlg = [FileSavePanel panelWithTitle:@"Export State" types:@"Free42 State;f42;All Files;*"];
    if ([saveDlg runModal] != NSOKButton)
        return;
    NSString *path = [saveDlg path];
    const char *copy_path = [path UTF8String];
    if (strcasecmp(name, state.coreName) == 0)
        core_save_state(copy_path);
    else {
        char orig_path[FILENAMELEN];
        snprintf(orig_path, FILENAMELEN, "%s/%s.f42", free42dirname, name);
        if (![self copyStateFrom:orig_path to:copy_path])
            [Free42AppDelegate showMessage:@"State export failed." withTitle:@"Error"];
    }
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
