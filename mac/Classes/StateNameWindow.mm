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

#import "StateNameWindow.h"

@implementation StateNameWindow

@synthesize label;
@synthesize stateName;

- (void) showMessage:(NSString *)message {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert addButtonWithTitle:@"OK"];
    [alert setMessageText:@"Error"];
    [alert setInformativeText:message];
    [alert setAlertStyle:NSCriticalAlertStyle];
    [alert runModal];
}

- (void) setupWithLabel:(NSString *)labelText existingNames:(NSString **)names count:(int)n {
    [label setStringValue:labelText];
    [stateName setStringValue:@""];
    existingNames = names;
    count = n;
    confirmed = NO;
}

- (NSString *) selectedName {
    return confirmed ? [stateName stringValue] : nil;
}

- (IBAction) ok:(id)sender {
    NSString *name = [stateName stringValue];
    if ([name length] == 0 || [name containsString:@"/"]) {
        [self showMessage:@"That name is not valid."];
        return;
    }
    for (int i = 0; i < count; i++)
        if ([name caseInsensitiveCompare:existingNames[i]] == NSOrderedSame) {
            [self showMessage:@"That name is already in use."];
            return;
        }
    confirmed = YES;
    [NSApp stopModal];
    [self close];
}

- (IBAction) cancel:(id)sender {
    [NSApp stopModal];
    [self close];
}

@end
