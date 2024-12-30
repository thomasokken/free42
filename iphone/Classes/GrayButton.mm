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

#import "GrayButton.h"

@implementation GrayButton

- (void)encodeWithCoder:(NSCoder *)encoder{
    [super encodeWithCoder:encoder];
}

- (id)initWithCoder:(NSCoder *)decoder {
    [super initWithCoder:decoder];
    /*
     * Once we start targeting iOS 13 or later, this check can be removed;
     * and once we start targeting iOS 15 or later, we can get rid of this
     * class altogether, and use a plain ol' UIButton with Style: Gray.
     * Note: the only iPhones that I'm still supporting that require iOS 12
     * are the 5s and 6; all later models run at least iOS 15.
     */
    if (@available(iOS 13.0, *)) {
        self.backgroundColor = UIColor.systemGray5Color;
    } else {
        self.backgroundColor = UIColor.lightGrayColor;
    }
    self.layer.cornerRadius = 10;
    self.clipsToBounds = YES;
    return self;
}

@end
