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

#import "PrintTileView.h"
#import "PrintView.h"

@implementation PrintTileView

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code
    }
    return self;
}

- (void) awakeFromNib {
    [super awakeFromNib];
    // Automatically redraw whenever our frame changes
    self.contentMode = UIViewContentModeRedraw;
}

- (void)drawRect:(CGRect)rect
{
    //NSLog(@"tile.drawRect: %f %f %f %f", rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
    CGFloat scale = [PrintView scale];
    CGContextRef myContext = UIGraphicsGetCurrentContext();
    CGContextSetRGBFillColor(myContext, 1.0, 1.0, 1.0, 1.0);
    //CGContextFillRect(myContext, rect);
    CGContextSetRGBFillColor(myContext, 0.0, 0.0, 0.0, 1.0);
    int xmin = floor(rect.origin.x / scale);
    if (xmin < 18)
        xmin = 18;
    int xmax = ceil((rect.origin.x + rect.size.width) / scale);
    if (xmax > 161)
        xmax = 161;
    int ymin = floor(rect.origin.y / scale);
    if (ymin < 0)
        ymin = 0;
    int ymax = ceil((rect.origin.y + rect.size.height) / scale);
    int length = printout_bottom - printout_top;
    if (length < 0)
        length += PRINT_LINES;
    if (ymax > length)
        ymax = length;
    for (int v = ymin; v < ymax; v++) {
        int v2 = printout_top + v;
        if (v2 >= PRINT_LINES)
            v2 -= PRINT_LINES;
        for (int h = xmin; h < xmax; h++) {
            int h2 = h - 18;
            int pixel = (print_bitmap[v2 * PRINT_BYTESPERLINE + (h2 >> 3)] & (1 << (h2 & 7))) != 0;
            if (pixel)
                CGContextFillRect(myContext, CGRectMake(h * scale, v * scale, scale, scale));
        }
    }
}

@end
