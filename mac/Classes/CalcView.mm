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

#import "CalcView.h"
#import "Free42AppDelegate.h"
#import "shell_skin.h"

@implementation CalcView

@synthesize keyboardShortcutsMenuItem;

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
        self.postsFrameChangedNotifications = YES;
        [[NSNotificationCenter defaultCenter]
             addObserver:self
             selector:@selector(frameDidChange:)
             name:NSViewFrameDidChangeNotification
             object:self];
        keyboardShortcutsShowing = false;
    }
    return self;
}

- (void)drawRect:(NSRect)rect {
    skin_repaint(&rect, keyboardShortcutsShowing);
}

- (BOOL) acceptsFirstResponder {
    return YES;
}

- (void) viewDidMoveToWindow {
    [[self.window standardWindowButton:NSWindowZoomButton] setEnabled:NO];
    [super viewDidMoveToWindow];
}

- (void)frameDidChange:(NSNotification*)notification {
    int sw, sh;
    skin_get_size(&sw, &sh);
    state.mainWindowWidth = self.frame.size.width;
    state.mainWindowHeight = self.frame.size.height;
    [self scaleUnitSquareToSize:NSMakeSize(self.bounds.size.width / sw, self.bounds.size.height / sh)];
    [self setNeedsDisplay:YES];
}

- (void)mouseDown:(NSEvent *)theEvent {
    NSPoint loc = [theEvent locationInWindow];
    loc = [self convertPoint:loc fromView:nil];
    calc_mousedown((int) loc.x, (int) loc.y);
}

- (void)mouseUp:(NSEvent *)theEvent {
    calc_mouseup();
}

- (void)keyDown:(NSEvent *)theEvent {
    if ([theEvent isARepeat])
        return;
    NSString *characters = [theEvent characters];
    if ([characters length] == 0) {
        if (@available(macOS 10.15, *)) {
            characters = [theEvent charactersByApplyingModifiers:[theEvent modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask ^ NSEventModifierFlagShift];
        }
    }
    calc_keydown(characters, [theEvent modifierFlags], [theEvent keyCode]);
}

- (void)keyUp:(NSEvent *)theEvent {
    if ([theEvent isARepeat])
        return;
    NSString *characters = [theEvent characters];
    if ([characters length] == 0) {
        if (@available(macOS 10.15, *)) {
            characters = [theEvent charactersByApplyingModifiers:[theEvent modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask ^ NSEventModifierFlagShift];
        }
    }
    calc_keyup(characters, [theEvent modifierFlags], [theEvent keyCode]);
}

- (void)flagsChanged:(NSEvent *)theEvent {
    calc_keymodifierschanged([theEvent modifierFlags]);
}

- (IBAction) toggleKeyboardShortcuts:(id)sender {
    keyboardShortcutsShowing = !keyboardShortcutsShowing;
    [keyboardShortcutsMenuItem setState:keyboardShortcutsShowing ? NSOnState : NSOffState];
    [self setNeedsDisplay:YES];
}

- (void) viewDidChangeEffectiveAppearance {
    [super viewDidChangeEffectiveAppearance];
    self.needsDisplay = YES;
}

@end
