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

#import "ProgramListDataSource.h"


@implementation ProgramListDataSource

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
        // Shouldn't update program names, really
        ;
}

- (void) setProgramNames:(const char *)newNames {
    if (names != NULL) {
        for (int i = 0; i < count; i++)
            [names[i] release];
        free(names);
        free(selected);
    }
    count = newNames == NULL ? 0 : ((newNames[0] & 255) << 24) | ((newNames[1] & 255) << 16)
                                    | ((newNames[2] & 255) << 8) | (newNames[3] & 255);
    names = (NSString **) malloc(count * sizeof(NSString *));
    selected = (bool *) malloc(count * sizeof(bool));
    const char *p = newNames + 4;
    for (int i = 0; i < count; i++) {
        names[i] = [[NSString stringWithUTF8String:p] retain];
        selected[i] = false;
        p += strlen(p) + 1;
    }
}

- (bool *) getSelection {
    return selected;
}

- (NSString *) getItemAtIndex:(int)i {
    return names[i];
}

@end
