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
#import <stdlib.h>
#import "CalcView.h"
#import "StatesView.h"
#import "RootViewController.h"
#import "core_main.h"


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

    // make sure "config" exists
    mkdir("config", 0755);
    
    // In case we're called right after an upgrade from < 2.5, or right
    // after a fresh install, make sure that at least a dummy file exists
    // for the current state, so that the initial states list won't be
    // empty.
    NSString *currentStateFileName = [NSString stringWithFormat:@"config/%@.f42", [NSString stringWithUTF8String:state.coreName]];
    const char *currentStateFileNameC = [currentStateFileName UTF8String];
    struct stat st;
    if (stat(currentStateFileNameC, &st) != 0) {
        FILE *f = fopen(currentStateFileNameC, "w");
        fwrite(FREE42_MAGIC_STR, 1, 4, f);
        fclose(f);
    }

    [stateNames removeAllObjects];
    int index = 0;
    DIR *dir = opendir("config");
    struct dirent *d;
    while ((d = readdir(dir)) != NULL) {
        size_t len = strlen(d->d_name);
        if (len < 5 || strcmp(d->d_name + len - 4, ".f42") != 0)
            continue;
        d->d_name[len - 4] = 0;
        NSString *s = [NSString stringWithUTF8String:d->d_name];
        [stateNames addObject:s];
        index++;
    }
    closedir(dir);
    [stateNames sortUsingSelector:@selector(compare:)];
    selectedIndex = -1;
    NSString *core = [NSString stringWithUTF8String:state.coreName];
    for (int i = 0; i < [stateNames count]; i++)
        if ([[stateNames objectAtIndex:i] compare:core] == NSOrderedSame) {
            selectedIndex = i;
            break;
        }

    [switchToButton setEnabled:NO];
    [switchToButton setTitle:@"Switch To"];
    mode = 0;
    [stateTable reloadData];
}

- (void) selectState:(NSString *)stateName {
    int selIndex = -1;
    for (int i = 0; i < [stateNames count]; i++) {
        NSString *name = [stateNames objectAtIndex:i];
        if ([name compare:stateName] == NSOrderedSame) {
            selIndex = i;
            break;
        }
    }
    if (selIndex != -1) {
        NSIndexPath *path = [NSIndexPath indexPathForItem:selIndex inSection:0];
        [stateTable selectRowAtIndexPath:path animated:YES scrollPosition:UITableViewScrollPositionMiddle];
        [self tableView:stateTable didSelectRowAtIndexPath:path];
    }
}

- (IBAction) switchTo {
    NSString *name = [self selectedStateName];
    if (name == nil)
        return;
    if (strcmp([name UTF8String], state.coreName) != 0) {
        [self switchTo2];
        return;
    }
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Confirm Revert" message:[NSString stringWithFormat:@"Are you sure you want to revert the state \"%@\" to the last version saved?", name] preferredStyle:UIAlertControllerStyleAlert];
    UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil];
    UIAlertAction *okAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
        [self switchTo2];
    }];
    [alert addAction:cancelAction];
    [alert addAction:okAction];
    [self.window.rootViewController presentViewController:alert animated:YES completion:nil];
}

- (void) switchTo2 {
    NSString *name = [self selectedStateName];
    if (name != nil)
        [CalcView loadState:[name UTF8String]];
    [RootViewController showMain];
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

- (NSString *) selectedStateName {
    NSIndexPath *index = [stateTable indexPathForSelectedRow];
    if (index == nil)
        return nil;
    int idx = (int) [index indexAtPosition:1];
    return [stateNames objectAtIndex:idx];
}

- (void) getNewStateNameWithPrompt:(NSString *)prompt completion:(SEL)sel {
    UIAlertController* alert = [UIAlertController alertControllerWithTitle:@"State Name"
               message:prompt
               preferredStyle:UIAlertControllerStyleAlert];
    UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil];
    UIAlertAction *okAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
        NSString *s = alert.textFields.firstObject.text;
        [self performSelector:sel withObject:s];
    }];
    [alert addTextFieldWithConfigurationHandler:^(UITextField *tf) {}];
    [alert addAction:cancelAction];
    [alert addAction:okAction];
    [self.window.rootViewController presentViewController:alert animated:YES completion:nil];
}

- (void) doNew {
    [self getNewStateNameWithPrompt:@"Enter new state name:" completion:@selector(doNew2:)];
}

- (void) doNew2:(NSString *)name {
    if ([name length] == 0 || [name containsString:@"/"]) {
        [RootViewController showMessage:@"That name is not valid."];
        return;
    }
    NSString *fname = [NSString stringWithFormat:@"config/%@.f42", name];
    const char *cname = [fname UTF8String];
    struct stat st;
    if (stat(cname, &st) == 0) {
        [RootViewController showMessage:@"That name is already in use."];
        return;
    }
    FILE *f = fopen(cname, "w");
    fwrite(FREE42_MAGIC_STR, 1, 4, f);
    fclose(f);
    [self raised];
}

- (BOOL) copyStateFrom:(const char *)orig_name to:(const char *)copy_name {
    FILE *fin = fopen(orig_name, "r");
    FILE *fout = fopen(copy_name, "w");
    if (fin != NULL && fout != NULL) {
        char buf[1024];
        size_t n;
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

+ (NSString *) makeCopyName:(NSString *)name {
    // We're naming duplicates by appending " copy" or " copy NNN" to the name
    // of the original, but if the name of the original already ends with " copy"
    // or " copy NNN", it seems more elegant to continue the sequence rather than
    // add another " copy" suffix.
    NSString *copyName = name;
    NSUInteger len = [copyName length];
    int n = 0;
    if (len > 5 && [[copyName substringFromIndex:len - 5] compare:@" copy"] == NSOrderedSame) {
        copyName = [copyName substringToIndex:len - 5];
        n = 1;
    } else {
        NSRange cpos = [copyName rangeOfString:@" copy " options:NSBackwardsSearch];
        if (cpos.location != NSNotFound) {
            // We don't just want to parse a number from the part following
            // " copy "; we want to make sure that there is nothing but a
            // number there, hence this somewhat elaborate logic.
            NSString *suffix = [copyName substringFromIndex:cpos.location + 6];
            int m = (int) [suffix integerValue];
            if (m > 1) {
                NSString *s2 = [NSString stringWithFormat:@"%d", m];
                if ([suffix isEqualToString:s2]) {
                    n = m;
                    copyName = [copyName substringToIndex:cpos.location];
                }
            }
        }
    }
    
    NSString *finalName, *finalPath;
    
    while (true) {
        n++;
        if (n == 1)
            finalName = [NSString stringWithFormat:@"%@ copy", copyName];
        else
            finalName = [NSString stringWithFormat:@"%@ copy %d", copyName, n];
        finalPath = [NSString stringWithFormat:@"config/%@.f42", finalName];
        struct stat st;
        if (stat([finalPath UTF8String], &st) != 0)
            // File does not exist; that means we have a usable name
            break;
    }
    
    return finalName;
}

- (void) doDuplicate {
    NSString *name = [self selectedStateName];
    if (name == nil)
        return;
    NSString *copyName = [StatesView makeCopyName:name];
    NSString *finalPath = [NSString stringWithFormat:@"config/%@.f42", copyName];
    const char *finalPathC = [finalPath UTF8String];
    
    // What we do next depends on whether the selected state is the currently active
    // one. If it is, we'll call core_save_state(), to make sure the duplicate
    // actually matches the most up-to-date state; otherwise, we can simply copy
    // the existing state file.
    if (strcmp([name UTF8String], state.coreName) == 0)
        core_save_state(finalPathC);
    else {
        NSString *origName = [NSString stringWithFormat:@"config/%@.f42", name];
        if (![self copyStateFrom:[origName UTF8String] to:finalPathC])
            [RootViewController showMessage:@"State duplication failed."];
    }
    [self raised];
}

- (void) doRename {
    NSString *oldName = [self selectedStateName];
    if (oldName == nil)
        return;
    NSString *prompt = [NSString stringWithFormat:@"Rename \"%@\" to:", oldName];
    [self getNewStateNameWithPrompt:prompt completion:@selector(doRename2:)];
}

- (void) doRename2:(NSString* )newName {
    NSString *oldName = [self selectedStateName];
    if (oldName == nil)
        return;
    if ([newName length] == 0 || [newName containsString:@"/"]) {
        [RootViewController showMessage:@"That name is not valid."];
        return;
    }
    NSString *newPath = [NSString stringWithFormat:@"config/%@.f42", newName];
    const char *newPathC = [newPath UTF8String];
    struct stat st;
    if (stat(newPathC, &st) == 0) {
        [RootViewController showMessage:@"That name is already in use."];
        return;
    }
    NSString *oldPath = [NSString stringWithFormat:@"config/%@.f42", oldName];
    rename([oldPath UTF8String], newPathC);
    if (strcmp([oldName UTF8String], state.coreName) == 0)
        strncpy(state.coreName, [newName UTF8String], FILENAMELEN);
    [self raised];
}

- (void) doDelete {
    NSString *name = [self selectedStateName];
    if (name == nil || strcmp([name UTF8String], state.coreName) == 0)
        return;
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Confirm Delete" message:[NSString stringWithFormat:@"Are you sure you want to delete the state \"%@\"?", name] preferredStyle:UIAlertControllerStyleAlert];
    UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil];
    UIAlertAction *okAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
        [self doDelete2];
    }];
    [alert addAction:cancelAction];
    [alert addAction:okAction];
    [self.window.rootViewController presentViewController:alert animated:YES completion:nil];
}

- (void) doDelete2 {
    NSString *name = [self selectedStateName];
    if (name == nil || strcmp([name UTF8String], state.coreName) == 0)
        return;
    NSString *path = [NSString stringWithFormat:@"config/%@.f42", name];
    remove([path UTF8String]);
    [self raised];
}

- (void) doShare {
    NSString *name = [self selectedStateName];
    if (name == nil)
        return;
    char *cwd = getcwd(NULL, 0);
    NSString *statePath = [NSString stringWithFormat:@"%s/config/%@.f42", cwd, name];
    free(cwd);
    if (strcmp([name UTF8String], state.coreName) == 0)
        core_save_state([statePath UTF8String]);
    UIActivityViewController *activityViewController = [[UIActivityViewController alloc] initWithActivityItems:@[[NSURL fileURLWithPath:statePath]] applicationActivities:nil];
    [self.window.rootViewController presentViewController:activityViewController animated:YES completion:nil];
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
            break;
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
            break;
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
            break;
    }
}

- (void) tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    NSUInteger n = [indexPath indexAtPosition:1];
    NSString *s = [stateNames objectAtIndex:n];
    [switchToButton setEnabled:YES];
    if (strcmp([s UTF8String], state.coreName) == 0) {
        [switchToButton setTitle:@"Revert"];
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
