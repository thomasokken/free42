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

    // Make sure a file exists for the current state. This isn't necessarily
    // the case, specifically, right after starting up with a version <= 25
    // state file.
    NSString *currentStateFileName = [NSString stringWithFormat:@"%s/%@.f42", free42dirname, [NSString stringWithUTF8String:state.coreName]];
    const char *currentStateFileNameC = [currentStateFileName UTF8String];
    struct stat st;
    if (stat(currentStateFileNameC, &st) != 0) {
        FILE *f = fopen(currentStateFileNameC, "w");
        fwrite(FREE42_MAGIC_STR, 1, 4, f);
        fclose(f);
    }
}

- (NSString *) selectedStateName {
    int row = [stateListView selectedRow];
    if (row == -1)
        return nil;
    else
        return [NSString stringWithString:[[stateListDataSource getNames] objectAtIndex:row]];
}

- (void) updateUI:(BOOL)rescan {
    NSString *selName = [self selectedStateName];
    if (rescan) {
        [stateListDataSource loadStateNames];
        [stateListView reloadData];
        bool found = false;
        if (selName != nil) {
            NSMutableArray *names = [stateListDataSource getNames];
            for (int i = [names count] - 1; i >= 0; i--) {
                if ([[names objectAtIndex:i] caseInsensitiveCompare:selName] == NSOrderedSame) {
                    [stateListView selectRowIndexes:[NSIndexSet indexSetWithIndex:i] byExtendingSelection:NO];
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            [stateListView deselectAll:self];
            selName = nil;
        }
    }

    bool stateSelected;
    bool activeStateSelected;

    if (selName == nil) {
        [switchToButton setTitle:@"Switch To"];
        stateSelected = false;
    } else {
        activeStateSelected = [selName caseInsensitiveCompare:[NSString stringWithUTF8String:state.coreName]] == NSOrderedSame;
        if (activeStateSelected)
            [switchToButton setTitle:@"Revert"];
        else
            [switchToButton setTitle:@"Switch To"];
        stateSelected = true;
    }

    [switchToButton setEnabled:stateSelected];
    [[actionMenu itemAtIndex:2] setEnabled:stateSelected];
    [[actionMenu itemAtIndex:3] setEnabled:stateSelected];
    [[actionMenu itemAtIndex:4] setEnabled:stateSelected && !activeStateSelected];
    [[actionMenu itemAtIndex:6] setEnabled:stateSelected];
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
    NSString *name = [self selectedStateName];
    if (name == nil)
        return;
    if (strcmp([name UTF8String], state.coreName) == 0) {
        NSAlert *alert = [[[NSAlert alloc] init] autorelease];
        [alert addButtonWithTitle:@"Revert"];
        [alert addButtonWithTitle:@"Cancel"];
        [alert setMessageText:@"Revert state?"];
        [alert setInformativeText:[NSString stringWithFormat:@"Are you sure you want to revert the state \"%@\" to the last version saved?", name]];
        [alert setAlertStyle:NSWarningAlertStyle];
        if ([alert runModal] != NSAlertFirstButtonReturn)
            return;
    }
    [Free42AppDelegate loadState:[name UTF8String]];
    [self close];
}

- (void) doNew {
    [stateNameWindow setupWithLabel:@"New State Name:" existingNames:[stateListDataSource getNames]];
    [NSApp runModalForWindow:stateNameWindow];
    NSString *name = [stateNameWindow selectedName];
    if (name == nil)
        return;
    NSString *fname = [NSString stringWithFormat:@"%s/%@.f42", free42dirname, name];
    FILE *f = fopen([fname UTF8String], "w");
    fprintf(f, FREE42_MAGIC_STR);
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
    NSString *name = [self selectedStateName];
    if (name == nil)
        return;
    NSString *copyName = [NSString stringWithFormat:@"%s/%@", free42dirname, name];
    int n = 0;

    // We're naming duplicates by appending " copy" or " copy NNN" to the name
    // of the original, but if the name of the original already ends with " copy"
    // or " copy NNN", it seems more elegant to continue the sequence rather than
    // add another " copy" suffix.
    int len = [copyName length];
    if (len > 5 && [[copyName substringFromIndex:len - 5] caseInsensitiveCompare:@" copy"] == NSOrderedSame) {
        copyName = [copyName substringToIndex:len - 5];
        n = 1;
    } else if (len > 7) {
        int pos = len - 7;
        int m = 0;
        int p = 1;
        while (pos > 0) {
            unichar c = [copyName characterAtIndex:pos + 6];
            if (c < '0' || c > '9')
                goto not_a_copy;
            m += p * (c - '0');
            p *= 10;
            if ([[copyName substringWithRange:NSMakeRange(pos, 6)] caseInsensitiveCompare:@" copy "] == NSOrderedSame) {
                n = m;
                copyName = [copyName substringToIndex:pos];
                break;
            } else
                pos--;
        }
        not_a_copy:;
    }

    NSString *finalName;
    const char *finalNameC;
    while (true) {
        n++;
        if (n == 1)
            finalName = [NSString stringWithFormat:@"%@ copy.f42", copyName];
        else
            finalName = [NSString stringWithFormat:@"%@ copy %d.f42", copyName, n];
        finalNameC = [finalName UTF8String];
        struct stat st;
        if (stat(finalNameC, &st) != 0)
            // File does not exist; that means we have a usable name
            break;
    }

    // Once we get here, copy_name contains a valid name for creating the duplicate.
    // What we do next depends on whether the selected state is the currently active
    // one. If it is, we'll call core_save_state(), to make sure the duplicate
    // actually matches the most up-to-date state; otherwise, we can simply copy
    // the existing state file.
    if ([name caseInsensitiveCompare:[NSString stringWithUTF8String:state.coreName]] == NSOrderedSame)
        core_save_state(finalNameC);
    else {
        NSString *origName = [NSString stringWithFormat:@"%s/%@.f42", free42dirname, name];
        if (![self copyStateFrom:[origName UTF8String] to:finalNameC])
            [Free42AppDelegate showMessage:@"State duplication failed." withTitle:@"Error"];
    }
    [self updateUI:YES];
}

- (void) doRename {
    NSString *oldname = [self selectedStateName];
    if (oldname == nil)
        return;
    [stateNameWindow setupWithLabel:[NSString stringWithFormat:@"Rename \"%@\" to:", oldname] existingNames:[stateListDataSource getNames]];
    [NSApp runModalForWindow:stateNameWindow];
    NSString *newname = [stateNameWindow selectedName];
    if (newname == nil)
        return;
    NSString *oldpath = [NSString stringWithFormat:@"%s/%@.f42", free42dirname, oldname];
    NSString *newpath = [NSString stringWithFormat:@"%s/%@.f42", free42dirname, newname];
    rename([oldpath UTF8String], [newpath UTF8String]);
    if ([oldname caseInsensitiveCompare:[NSString stringWithUTF8String:state.coreName]] == NSOrderedSame) {
        strncpy(state.coreName, [newname UTF8String], FILENAMELEN);
        [current setStringValue:newname];
    }
    [self updateUI:YES];
}

- (void) doDelete {
    NSString *name = [self selectedStateName];
    if (name == nil)
        return;
    if ([name caseInsensitiveCompare:[NSString stringWithUTF8String:state.coreName]] == NSOrderedSame) {
        return;
    }
    NSAlert *alert = [[[NSAlert alloc] init] autorelease];
    [alert addButtonWithTitle:@"Delete"];
    [alert addButtonWithTitle:@"Cancel"];
    [alert setMessageText:@"Delete state?"];
    [alert setInformativeText:[NSString stringWithFormat:@"Are you sure you want to delete the state \"%@\"?", name]];
    [alert setAlertStyle:NSWarningAlertStyle];
    if ([alert runModal] != NSAlertFirstButtonReturn)
        return;
    NSString *statePath = [NSString stringWithFormat:@"%s/%@.f42", free42dirname, name];
    remove([statePath UTF8String]);
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
        NSString *destPath = [NSString stringWithFormat:@"%s/%@.f42", free42dirname, name];
        const char *destPathC = [destPath UTF8String];
        struct stat st;
        if (stat(destPathC, &st) == 0)
            [Free42AppDelegate showMessage:[NSString stringWithFormat:@"A state named \"%@\" already exists.", name] withTitle:@"Error"];
        else if (![self copyStateFrom:[path UTF8String] to:destPathC])
            [Free42AppDelegate showMessage:@"State import failed." withTitle:@"Error"];
    }
    [self updateUI:YES];
}

- (void) doExport {
    NSString *name = [self selectedStateName];
    if (name == nil)
        return;
    FileSavePanel *saveDlg = [FileSavePanel panelWithTitle:@"Export State" types:@"Free42 State;f42;All Files;*" path:name];
    if ([saveDlg runModal] != NSOKButton)
        return;
    NSString *copyPath = [saveDlg path];
    const char *copyPathC = [copyPath UTF8String];
    if ([name caseInsensitiveCompare:[NSString stringWithUTF8String:state.coreName]] == NSOrderedSame)
        core_save_state(copyPathC);
    else {
        NSString *origPath = [NSString stringWithFormat:@"%s/%@.f42", free42dirname, name];
        if (![self copyStateFrom:[origPath UTF8String] to:copyPathC])
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
