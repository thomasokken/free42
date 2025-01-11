/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2025  Thomas Okken
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

#import <UIKit/UIKit.h>
#import "shell_skin.h"

#define SHELL_VERSION 14
#define FILENAMELEN 1024

struct state_type {
    int printerToTxtFile;
    int printerToGifFile;
    char printerTxtFileName[FILENAMELEN];
    char printerGifFileName[FILENAMELEN];
    int printerGifMaxLength;
    char skinName[FILENAMELEN];
    char landscapeSkinName[FILENAMELEN];
    int alwaysOn;
    int keyClicks;
    int hapticFeedback;
    int orientationMode; // 0=auto 1=portrait 2=landscape
    int maintainSkinAspect[2];
    bool offEnabled;
    char coreName[FILENAMELEN];
    bool matrix_singularmatrix;
    bool matrix_outofrange;
    bool auto_repeat;
    bool allow_big_stack;
    bool localized_copy_paste;
    int swipeDirectionMode; // 0=left 1=off 2=right
    bool popupAlphaKeyboard;
};

extern state_type state;
extern FILE *statefile;

void get_keymap(keymap_entry **map, int *length);


@interface CalcView : UIView {
    UIButton *keyboardShortcutsButton;
    bool keyboardShortcutsShowing;
}

@property (nonatomic, retain) IBOutlet UIButton *keyboardShortcutsButton;

- (void) awakeFromNib;
- (void) layoutSubviews;
+ (BOOL) isPortrait;
+ (CGFloat) width;
+ (CGFloat) height;
- (void) touchesBegan: (NSSet *) touches withEvent: (UIEvent *) event;
+ (void) repaint;
+ (void) quit;
+ (void) loadState:(const char *)name;
+ (void) enterBackground;
+ (void) leaveBackground;
- (void) doCopy;
- (void) doPaste;
- (void) setTimeout:(int) which;
- (void) cancelTimeout3;
- (void) setRepeater:(int) delay;
- (void) cancelRepeater;
+ (void) alphaKeyboardAlpha:(unsigned short) code;
+ (void) alphaKeyboardDown:(int) key;
+ (void) alphaKeyboardUp;
+ (void) keyFeedback;
+ (void) stopTextPrinting;
+ (void) stopGifPrinting;
- (void) setActive:(bool) active;
+ (void) readKeyMap;
- (IBAction) toggleKeyboardShortcuts:(id)sender;

@end
