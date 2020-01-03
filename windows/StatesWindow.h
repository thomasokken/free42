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

#ifndef STATES_WINDOW_H
#define STATES_WINDOW_H 1

#include "stdafx.h"
#include "resource.h"

LRESULT CALLBACK StatesDlgProc(HWND, UINT, WPARAM, LPARAM);

/*
#import <Cocoa/Cocoa.h>

@class StateListDataSource;
@class StateNameWindow;

@interface StatesWindow : NSWindow {
    NSTextField *current;
    NSTableView *stateListView;
    NSButton *switchToButton;
    NSPopUpButton *actionMenu;
    StateListDataSource *stateListDataSource;
    StateNameWindow *stateNameWindow;
}

@property (nonatomic, retain) IBOutlet NSTextField *current;
@property (nonatomic, retain) IBOutlet NSTableView *stateListView;
@property (nonatomic, retain) IBOutlet NSButton *switchToButton;
@property (nonatomic, retain) IBOutlet NSPopUpButton *actionMenu;
@property (nonatomic, retain) IBOutlet StateListDataSource *stateListDataSource;
@property (nonatomic, retain) IBOutlet StateNameWindow *stateNameWindow;

- (IBAction) stateListAction:(id)sender;
- (IBAction) stateListDoubleAction:(id)sender;
- (IBAction) switchTo:(id)sender;
- (IBAction) menuSelected:(id)sender;
- (IBAction) done:(id)sender;

- (void) reset;

@end
*/

#endif
