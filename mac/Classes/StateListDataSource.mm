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
#import "StateListDataSource.h"
#import "Free42AppDelegate.h"


@implementation StateListDataSource

- (void) awakeFromNib {
    count = 0;
    names = NULL;
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView {
    return count;
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex {
    return names[rowIndex];
}

- (void)tableView:(NSTableView *)aTableView setObjectValue:(id)anObject forTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex {
    // TODO
}

static int case_insens_comparator(const void *a, const void *b) {
    return strcasecmp(*(const char **) a, *(const char **) b);
}

- (void) loadStateNames {
    if (names != NULL) {
        for (int i = 0; i < count; i++)
            [names[i] release];
        free(names);
    }
    count = 0;
    names = NULL;
    
    DIR *dir = opendir(free42dirname);
    if (dir == NULL)
        return;
    
    struct dirent *dent;
    char *statename[100];
    int nstates = 0;
    
    while ((dent = readdir(dir)) != NULL && nstates < 100) {
        int namelen = strlen(dent->d_name);
        if (namelen < 4)
            continue;
        namelen -= 4;
        if (strcasecmp(dent->d_name + namelen, ".f42") != 0)
            continue;
        char *st = (char *) malloc(namelen + 1);
        // TODO - handle memory allocation failure
        memcpy(st, dent->d_name, namelen);
        st[namelen] = 0;
        statename[nstates++] = st;
    }
    closedir(dir);

    qsort(statename, nstates, sizeof(char *), case_insens_comparator);
    
    count = nstates;
    names = (NSString **) malloc(nstates * sizeof(NSString *));
    for (int i = 0; i < count; i++) {
        names[i] = [[NSString stringWithCString:statename[i] encoding:NSUTF8StringEncoding] retain];
    }
}

- (NSString **) getNames {
    return names;
}

@end
