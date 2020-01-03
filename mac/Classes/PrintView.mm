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

#import "PrintView.h"
#import "Free42AppDelegate.h"


@implementation PrintView

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
    }
    return self;
}

- (void) initialUpdate {
    int height = printout_bottom - printout_top;
    if (height < 0)
        height += PRINT_LINES;
    NSSize s;
    s.width = 358;
    s.height = height;
    [self setFrameSize:s];
    [self setNeedsDisplay:YES];
}

- (void) scrollToBottom {
    NSScrollView *scrollView = (NSScrollView *) [[self superview] superview];
    scrollView.verticalScroller.floatValue = 1;
    NSPoint p;
    p.x = 0;
    p.y = 0;
    [self scrollPoint:p];
}

- (void) updatePrintout:(update_params *) ppar {
    int newlength = ppar->newlength;
    int oldlength = ppar->oldlength;
    int height = ppar->height;
    if (newlength >= PRINT_LINES) {
        printout_top = (printout_bottom + 2) % PRINT_LINES;
        newlength = PRINT_LINES - 2;
        if (newlength != oldlength) {
            NSSize s;
            s.width = 358;
            s.height = newlength;
            [self setFrameSize:s];
        }
        NSPoint p;
        p.x = 0;
        p.y = 0;
        [self scrollPoint:p];
        // TODO: Instead of repainting everything, copying the existing
        // print-out that merely has to move, may be more efficient
        [self setNeedsDisplay:YES];
    } else {
        NSSize s;
        s.width = 358;
        s.height = newlength;
        [self setFrameSize:s];
        NSPoint p;
        p.x = 0;
        p.y = 0;
        [self scrollPoint:p];
        NSRect r;
        r.origin.x = 0;
        r.origin.y = oldlength;
        r.size.width = 358;
        r.size.height = 2 * height;
        [self setNeedsDisplayInRect:r];
    }
}

- (void)drawRect:(NSRect)rect {
    CGContextRef myContext = (CGContextRef) [[NSGraphicsContext currentContext] graphicsPort];
    CGContextSetRGBFillColor(myContext, 1.0, 1.0, 1.0, 1.0);
    CGContextFillRect(myContext, NSRectToCGRect(rect));
    CGContextSetRGBFillColor(myContext, 0.0, 0.0, 0.0, 1.0);
    int xmin = (int) rect.origin.x;
    int xmax = (int) (rect.origin.x + rect.size.width);
    int ymin = (int) rect.origin.y;
    int ymax = (int) (rect.origin.y + rect.size.height);
    int wh = [self bounds].size.height;
    int length = printout_bottom - printout_top;
    if (length < 0)
        length += PRINT_LINES;
    for (int v = ymin; v < ymax; v++) {
        int v2 = wh - 1 - v; // because of OS X upside-down coordinate system
        v2 = printout_top + v2;
        if (v2 >= PRINT_LINES)
            v2 -= PRINT_LINES;
        for (int h = xmin; h < xmax; h++) {
            int h2 = h - 36;
            if (h2 >= 0 && h2 < 286) {
                int pixel = (print_bitmap[v2 * PRINT_BYTESPERLINE + (h2 >> 3)] & (1 << (h2 & 7))) != 0;
                if (pixel)
                    CGContextFillRect(myContext, CGRectMake(h, v, 1, 1));
            }
        }
    }
}

@end
