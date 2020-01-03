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

#import "StateNameWindow.h"
#import "Free42AppDelegate.h"

@implementation StateNameWindow

@synthesize label;
@synthesize stateName;

- (void) setupWithLabel:(NSString *)labelText existingNames:(NSMutableArray *)names {
    [label setStringValue:labelText];
    [stateName setStringValue:@""];
    existingNames = [NSMutableArray arrayWithArray:names];
    confirmed = NO;
}

- (NSString *) selectedName {
    return confirmed ? [stateName stringValue] : nil;
}

- (IBAction) ok:(id)sender {
    NSString *name = [stateName stringValue];
    if ([name length] == 0 || [name containsString:@"/"]) {
        [Free42AppDelegate showMessage:@"That name is not valid." withTitle:@"Error"];
        return;
    }
    for (int i = 0; i < [existingNames count]; i++)
        if ([name caseInsensitiveCompare:[existingNames objectAtIndex:i]] == NSOrderedSame) {
            [Free42AppDelegate showMessage:@"That name is already in use." withTitle:@"Error"];
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
