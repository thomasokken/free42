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

#import "MainView.h"
#import "shell_skin_iphone.h"

@implementation MainView


- (id)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Note: this does not get called when instantiated from a nib file,
		// so don't bother doing anything here!
    }
    return self;
}


- (void)drawRect:(CGRect)rect {
	if (skin_name == nil) {
		skin_name = @"Realistic";
		long w, h;
		skin_load(skin_name, &w, &h);
		skin_width = w;
		skin_height = h;
	}
	skin_repaint();
	/*
	CGContextRef myContext = UIGraphicsGetCurrentContext();
	CGFloat color[4];
	color[0] = (random() & 255) / 255.0;
	color[1] = (random() & 255) / 255.0;
	color[2] = (random() & 255) / 255.0;
	color[3] = 1.0;
	CGContextSetFillColor(myContext, color);
	CGContextFillEllipseInRect(myContext, CGRectMake(10, 10, 300, 300));
	*/
}


- (void)dealloc {
    [super dealloc];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
	[super touchesBegan:touches withEvent:event];
	[self setNeedsDisplay];
}

@end
