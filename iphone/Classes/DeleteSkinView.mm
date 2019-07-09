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
#import "DeleteSkinView.h"
#import "CalcView.h"
#import "RootViewController.h"
#import "shell_skin_iphone.h"
#import "core_main.h"


@implementation DeleteSkinView

@synthesize doneButton;
@synthesize skinTable;

- (id) initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Initialization code
    }
    return self;
}

- (id) initWithCoder:(NSCoder *)coder {
    [super initWithCoder:coder];
    skinNames = [[NSMutableArray arrayWithCapacity:10] retain];
    selectedIndex[0] = selectedIndex[1] = -1;
    return self;
}

- (void) drawRect:(CGRect)rect {
    // Drawing code
}

- (void) raised {
    // This gets called just before the view is raised, every time
    // TODO: separator between built-in and external skins
    [skinNames removeAllObjects];
    int index = 0;
    selectedIndex[0] = selectedIndex[1] = -1;
    DIR *dir = opendir("skins");
    struct dirent *d;
    NSUInteger num_builtin_skins = [skinNames count];
    while ((d = readdir(dir)) != NULL) {
        size_t len = strlen(d->d_name);
        if (len < 8 || strcmp(d->d_name + len - 7, ".layout") != 0)
            continue;
        d->d_name[len - 7] = 0;
        NSString *s = [NSString stringWithCString:d->d_name encoding:NSUTF8StringEncoding];
        for (int i = 0; i < num_builtin_skins; i++)
            if ([s caseInsensitiveCompare:[skinNames objectAtIndex:i]] == 0)
                goto skip;
        [skinNames addObject:s];
        if (strcasecmp(d->d_name, state.skinName) == 0)
            selectedIndex[0] = index;
        else if (strcasecmp(d->d_name, state.landscapeSkinName) == 0)
            selectedIndex[1] = index;
        index++;
        skip:;
    }
    closedir(dir);
    [skinTable reloadData];
}

- (IBAction) done {
    [RootViewController showSelectSkin];
}

- (IBAction) deleteSkin {
    NSArray *selection = [skinTable indexPathsForSelectedRows];
    NSUInteger count = [selection count];
    for (int i = 0; i < count; i++) {
        NSIndexPath *index = (NSIndexPath *) [selection objectAtIndex:i];
        int idx = (int) [index indexAtPosition:1];
        NSString *skinName = [skinNames objectAtIndex:idx];
        remove([[NSString stringWithFormat:@"skins/%@.gif", skinName] cStringUsingEncoding:NSUTF8StringEncoding]);
        remove([[NSString stringWithFormat:@"skins/%@.layout", skinName] cStringUsingEncoding:NSUTF8StringEncoding]);
    }
    [self raised];
}

- (void) tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    // Nothing to do here; the actual deletion is triggered by the Delete button
}

- (UITableViewCell *) tableView:(UITableView *)table cellForRowAtIndexPath:(NSIndexPath *) indexPath {
    NSUInteger n = [indexPath indexAtPosition:1];
    NSString *s = [skinNames objectAtIndex:n];
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
    cell.textLabel.text = s;
    cell.accessoryType = selectedIndex[0] == (int) n || selectedIndex[1] == (int) n ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
    return cell;
}

- (NSInteger) tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section {
    NSUInteger n = [skinNames count];
    return n;
}

- (void) dealloc {
    [super dealloc];
}


@end
