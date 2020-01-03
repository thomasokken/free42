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

#import "AboutView.h"
#import "Free42AppDelegate.h"
#import "RootViewController.h"


@implementation AboutView

@synthesize doneButton;
@synthesize versionLabel;
@synthesize copyrightLabel;

- (id)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Initialization code
    }
    return self;
}

- (void) awakeFromNib {
    [super awakeFromNib];
    [versionLabel setText:[NSString stringWithFormat:@"Free42 %s", [Free42AppDelegate getVersion]]];
    [copyrightLabel setText:@"© 2004-2020 Thomas Okken"];
}

- (void)drawRect:(CGRect)rect {
    // Drawing code
}

- (void) raised {
    // start-up code
}

- (IBAction) done {
    [RootViewController showMain];
}

- (void)dealloc {
    [super dealloc];
}


@end
