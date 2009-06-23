/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2009  Thomas Okken
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

#import <Cocoa/Cocoa.h>

#define FILENAMELEN 256
#define SHELL_VERSION 0

typedef struct state_type {
    int printerToTxtFile;
    int printerToGifFile;
    char printerTxtFileName[FILENAMELEN];
    char printerGifFileName[FILENAMELEN];
    int printerGifMaxLength;
    char mainWindowKnown, printWindowKnown, printWindowMapped;
    int mainWindowX, mainWindowY;
    int printWindowX, printWindowY, printWindowHeight;
    char skinName[FILENAMELEN];
};

extern state_type state;

extern char free42dirname[FILENAMELEN];

void calc_mousedown(int x, int y);
void calc_mouseup();

@class CalcView;

@interface Free42AppDelegate : NSObject {
	NSWindow *mainWindow;
	CalcView *calcView;
	NSWindow *printWindow;
	NSWindow *preferencesWindow;
	NSWindow *selectProgramsWindow;
	NSWindow *aboutWindow;
}

@property (nonatomic, retain) IBOutlet NSWindow *mainWindow;
@property (nonatomic, retain) IBOutlet CalcView *calcView;
@property (nonatomic, retain) IBOutlet NSWindow *printWindow;
@property (nonatomic, retain) IBOutlet NSWindow *preferencesWindow;
@property (nonatomic, retain) IBOutlet NSWindow *selectProgramsWindow;
@property (nonatomic, retain) IBOutlet NSWindow *aboutWindow;

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification;
- (void)applicationWillTerminate:(NSNotification *)aNotification;
- (IBAction) importPrograms:(id)sender;

@end
