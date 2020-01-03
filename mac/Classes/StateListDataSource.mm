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
#import "StateListDataSource.h"
#import "Free42AppDelegate.h"


@implementation StateListDataSource

- (void) awakeFromNib {
    names = [[NSMutableArray array] retain];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView {
    return [names count];
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex {
    return names[rowIndex];
}

- (void)tableView:(NSTableView *)aTableView setObjectValue:(id)anObject forTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex {
    // TODO
}

- (void) loadStateNames {
    [names removeAllObjects];
    
    DIR *dir = opendir(free42dirname);
    if (dir == NULL)
        return;
    
    struct dirent *dent;
    
    while ((dent = readdir(dir)) != NULL) {
        int namelen = strlen(dent->d_name);
        if (namelen < 4)
            continue;
        namelen -= 4;
        if (strcasecmp(dent->d_name + namelen, ".f42") != 0)
            continue;
        char *tempname = (char *) malloc(namelen + 1);
        strncpy(tempname, dent->d_name, namelen);
        tempname[namelen] = 0;
        NSString *st = [NSString stringWithUTF8String:tempname];
        free(tempname);
        [names addObject:st];
    }
    closedir(dir);

    [names sortUsingSelector:@selector(localizedCaseInsensitiveCompare:)];
}

- (NSArray *) getNames {
    return names;
}

@end
