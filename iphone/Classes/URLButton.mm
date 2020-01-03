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

#import "URLButton.h"

@implementation URLButton

- (void)encodeWithCoder:(NSCoder *)encoder{
    [super encodeWithCoder:encoder];
}

- (id)initWithCoder:(NSCoder *)decoder {
    [super initWithCoder:decoder];
    [self addTarget:self action:@selector(buttonClicked) forControlEvents:UIControlEventTouchUpInside];
    [self setTitleColor:[UIColor colorWithRed:0.933 green:0.0 blue:0.0 alpha:1.0] forState:UIControlStateHighlighted];
    return self;
}

- (void) buttonClicked {
    NSString *urlStr = self.titleLabel.text;
    NSURL *url = [NSURL URLWithString:urlStr];
    [[UIApplication sharedApplication] openURL:url];
}

@end
