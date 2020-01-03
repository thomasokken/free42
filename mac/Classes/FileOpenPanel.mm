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

#import "FileOpenPanel.h"

@implementation FileOpenPanel

+ (FileOpenPanel *) panelWithTitle:(NSString *)title types:(NSString *)types {
    return [[FileOpenPanel alloc] initWithTitle:title types:types];
}

- (FileOpenPanel *) initWithTitle:(NSString *)title types:(NSString *)types {
    // The string 'types' should have the form (Label;suffix;)+
    panel = [NSOpenPanel openPanel];
    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:NO];

    NSArray *typeItems = [types componentsSeparatedByString:@";"];
    NSMutableArray *labels = [[NSMutableArray alloc] init];
    NSMutableArray *suffixesM = [[NSMutableArray alloc] init];
    for (int i = 0; i < [typeItems count] - 1; i += 2) {
        NSString *label = [typeItems objectAtIndex:i];
        NSString *suffix = [typeItems objectAtIndex:i + 1];
        [labels addObject:[NSString stringWithFormat:@"%@ (*.%@)", label, suffix]];
        [suffixesM addObject:suffix];
    }
    suffixes = suffixesM;

    NSString *theSuffix = [suffixes objectAtIndex:0];
    if ([theSuffix isEqualToString:@"*"])
        [panel setAllowedFileTypes:nil];
    else
        [panel setAllowedFileTypes:[NSArray arrayWithObject:theSuffix]];
    [panel setTitle:title];

    NSView *accessoryView = [[NSView alloc] initWithFrame:NSMakeRect(0.0, 0.0, 260, 32.0)];
    NSTextField *label = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 60, 22)];
    [label setEditable:NO];
    [label setStringValue:@"Format:"];
    [label setBordered:NO];
    [label setBezeled:NO];
    [label setDrawsBackground:NO];

    NSPopUpButton *popupButton = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(50.0, 2, 170, 22.0) pullsDown:NO];
    [popupButton addItemsWithTitles:labels];
    [popupButton setAction:@selector(selectFormat:)];
    [popupButton setTarget:self];

    [accessoryView addSubview:label];
    [accessoryView addSubview:popupButton];

    [panel setAccessoryView:accessoryView];
    if ([panel respondsToSelector:@selector(isAccessoryViewDisclosed)]) {
        // show accessory view when dialog opens
        panel.accessoryViewDisclosed = YES;
    }
    
    return self;
}

- (NSModalResponse) runModal {
    return [panel runModal];
}

- (void) selectFormat:(id)sender {
    NSPopUpButton *button = (NSPopUpButton *)sender;
    NSInteger selectedItemIndex = [button indexOfSelectedItem];
    selectedSuffix = selectedItemIndex;
    NSString *theSuffix = [suffixes objectAtIndex:selectedSuffix];
    if ([theSuffix isEqualToString:@"*"])
        [panel setAllowedFileTypes:nil];
    else
        [panel setAllowedFileTypes:[NSArray arrayWithObject:theSuffix]];
}

- (NSArray *) paths {
    NSArray *urls = [panel URLs];
    NSMutableArray *paths = [[NSMutableArray alloc] init];
    for (int i = 0; i < [urls count]; i++)
        [paths addObject:[[urls objectAtIndex:i] path]];
    return paths;
}

@end
