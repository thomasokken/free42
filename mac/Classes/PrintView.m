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

#import "PrintView.h"
#import "MyRect.h"


@implementation PrintView

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
    }
    return self;
}

- (void)drawRect:(NSRect)rect {
	CGContextRef myContext = (CGContextRef) [[NSGraphicsContext currentContext] graphicsPort];
	CGContextSetRGBFillColor(myContext, 1.0, 0.0, 0.0, 1.0);
	CGContextFillRect(myContext, CGRectMake(10, 10, 30, 30));
	CGContextFillRect(myContext, CGRectMake(0, 50, 286, 30));
	CGContextFillRect(myContext, CGRectMake(1, 90, 284, 30));
}

- (void) setNeedsDisplayInRectSafely2:(id) myrect {
	MyRect *mr = (MyRect *) myrect;
	CGRect cr = [mr rect];
	NSRect invalRect;
	invalRect.origin.x = cr.origin.x;
	invalRect.origin.y = cr.origin.y;
	invalRect.size.width = cr.size.width;
	invalRect.size.height = cr.size.height;
	[self setNeedsDisplayInRect:invalRect];
}

- (void) setNeedsDisplayInRectSafely:(CGRect) rect {
	if ([NSThread isMainThread]) {
		NSRect invalRect;
		invalRect.origin.x = rect.origin.x;
		invalRect.origin.y = rect.origin.y;
		invalRect.size.width = rect.size.width;
		invalRect.size.height = rect.size.height;
		[self setNeedsDisplayInRect:invalRect];
	} else
		[self performSelectorOnMainThread:@selector(setNeedsDisplayInRectSafely2:) withObject:[MyRect rectWithCGRect:rect] waitUntilDone:NO];
}

@end
