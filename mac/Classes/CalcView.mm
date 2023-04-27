/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2023  Thomas Okken
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

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
    }
    return self;
}

- (void)drawRect:(NSRect)rect {
    skin_repaint(&rect);
}

- (BOOL) acceptsFirstResponder {
    return YES;
}

- (void)mouseDown:(NSEvent *)theEvent {
    NSPoint loc = [theEvent locationInWindow];
    calc_mousedown((int) loc.x, (int) loc.y);
}

- (void)mouseUp:(NSEvent *)theEvent {
    calc_mouseup();
}

- (void)keyDown:(NSEvent *)theEvent {
    if ([theEvent isARepeat])
        return;
    calc_keydown([theEvent characters], [theEvent modifierFlags], [theEvent keyCode]);
}

- (void)keyUp:(NSEvent *)theEvent {
    if ([theEvent isARepeat])
        return;
    calc_keyup([theEvent characters], [theEvent modifierFlags], [theEvent keyCode]);
}

- (void)flagsChanged:(NSEvent *)theEvent {
    calc_keymodifierschanged([theEvent modifierFlags]);
}

@end
