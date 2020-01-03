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
#import "SkinListDataSource.h"
#import "Free42AppDelegate.h"


@implementation SkinListDataSource

- (void) awakeFromNib {
    count = 0;
    selected = NULL;
    names = NULL;
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView {
    return count;
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex {
    if ([[aTableColumn identifier] isEqualToString:@"col1"])
        return [NSNumber numberWithBool:selected[rowIndex]];
    else
        return names[rowIndex];
}

- (void)tableView:(NSTableView *)aTableView setObjectValue:(id)anObject forTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex {
    if ([[aTableColumn identifier] isEqualToString:@"col1"])
        selected[rowIndex] = [anObject boolValue];
    else
        // Shouldn't update skin names, really
        ;
}

static int case_insens_comparator(const void *a, const void *b) {
    return strcasecmp(*(const char **) a, *(const char **) b);
}

- (void) loadSkinNames {
    if (names != NULL) {
        for (int i = 0; i < count; i++)
            [names[i] release];
        free(names);
        free(selected);
    }
    count = 0;
    names = NULL;
    selected = NULL;
    
    DIR *dir = opendir(free42dirname);
    if (dir == NULL)
        return;
    
    struct dirent *dent;
    char *skinname[100];
    int nskins = 0;
    
    while ((dent = readdir(dir)) != NULL && nskins < 100) {
        int namelen = strlen(dent->d_name);
        if (namelen < 7)
            continue;
        namelen -= 7;
        if (strcasecmp(dent->d_name + namelen, ".layout") != 0)
            continue;
        char *skn = (char *) malloc(namelen + 1);
        // TODO - handle memory allocation failure
        memcpy(skn, dent->d_name, namelen);
        skn[namelen] = 0;
        skinname[nskins++] = skn;
    }
    closedir(dir);

    qsort(skinname, nskins, sizeof(char *), case_insens_comparator);
    
    count = nskins;
    names = (NSString **) malloc(nskins * sizeof(NSString *));
    selected = (bool *) malloc(nskins * sizeof(bool));
    for (int i = 0; i < count; i++) {
        names[i] = [[NSString stringWithUTF8String:skinname[i]] retain];
        selected[i] = false;
    }
}

- (NSString **) getNames {
    return names;
}

- (bool *) getSelection {
    return selected;
}

@end
