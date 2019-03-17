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

#import <UIKit/UIKit.h>

#define SHELL_VERSION 5
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
};

extern state_type state;


@interface CalcView : UIView <UIActionSheetDelegate> {
    //
}

- (void) awakeFromNib;
- (void) layoutSubviews;
+ (BOOL) isPortrait;
+ (CGFloat) width;
+ (CGFloat) height;
- (void) actionSheet:(UIActionSheet *) actionSheet clickedButtonAtIndex:(NSInteger) buttonIndex;
- (void) touchesBegan: (NSSet *) touches withEvent: (UIEvent *) event;
- (void) setNeedsDisplayInRectSafely:(CGRect) rect;
+ (void) repaint;
+ (void) quit;
+ (void) enterBackground;
+ (void) leaveBackground;
- (void) doCopy;
- (void) doPaste;
- (void) setTimeout:(int) which;
- (void) cancelTimeout3;
- (void) setRepeater:(int) delay;
- (void) cancelRepeater;
+ (void) stopTextPrinting;
+ (void) stopGifPrinting;

@end
