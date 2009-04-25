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

#import "shell_iphone.h"
#import "SelectFileView.h"

@implementation SelectFileView

@synthesize navigationItem;
@synthesize directoryLabel;
@synthesize directoryListingView;
@synthesize nameField;
@synthesize typeSelector;
@synthesize selectFileButton;

static NSString *windowTitle;
static NSString *selectTitle;
static NSString *types;
static BOOL selectDir;
static id callbackObject;
static SEL callbackSelector;

static NSString *dirName;

- (id) initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Initialization code
    }
    return self;
}


- (void) drawRect:(CGRect)rect {
    // Drawing code
}

+ (void) raiseWithTitle:(NSString *)wt selectTitle:(NSString *)st types:(NSString *)t selectDir:(BOOL)sd callbackObject:(id)cb_id callbackSelector:(SEL)cb_sel {
	windowTitle = [wt retain];
	selectTitle = [st retain];
	types = [t retain];
	selectDir = sd;
	callbackObject = [cb_id retain];
	callbackSelector = cb_sel;
	dirName = [[NSString stringWithString:@"."] retain];
	[shell_iphone showSelectFile];
}

- (void) raised {
	// watch the keyboard so we can adjust the user interface if necessary.
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillShow:) name:UIKeyboardWillShowNotification object:nil];
	
	[navigationItem setTitle:windowTitle];
	[windowTitle release];
	[selectFileButton setTitle:selectTitle forState:(UIControlStateNormal | UIControlStateHighlighted | UIControlStateDisabled | UIControlStateSelected)];
	[selectTitle release];
}

static int keyboard_height = 0;
static int view_offset = 0;

- (void) keyboardWillShow:(NSNotification *)notif {
	NSDictionary *userInfo = [notif userInfo];
	NSValue *v = [userInfo objectForKey:UIKeyboardBoundsUserInfoKey];
	CGRect r;
	[v getValue:&r];
	keyboard_height = r.size.height;
}

- (void) textFieldDidBeginEditing:(UITextField *)textField {
	CGRect v_r = [self frame];
	CGRect tf_r = [textField frame];
	view_offset = v_r.origin.y + v_r.size.height - keyboard_height - tf_r.origin.y - tf_r.size.height - 5;
	if (view_offset > 0)
		view_offset = 0;
	[UIView beginAnimations:@"foo" context:NULL];
	[self setCenter:CGPointMake([self center].x, [self center].y + view_offset)];
	[UIView commitAnimations];
}

- (void) textFieldDidEndEditing:(UITextField *)textField {
	[UIView beginAnimations:@"foo" context:NULL];
	[self setCenter:CGPointMake([self center].x, [self center].y - view_offset)];
	[UIView commitAnimations];
}

- (BOOL) textFieldShouldReturn:(UITextField *)textField {
	[textField resignFirstResponder];
	return NO;
}

- (IBAction) selectFile {
	// unregister for keyboard notifications while not visible.
	[[NSNotificationCenter defaultCenter] removeObserver:self name:UIKeyboardWillShowNotification object:nil];
	[[self superview] sendSubviewToBack:self];
	NSString *fullName = [NSString stringWithFormat:@"%@/%@", dirName, [nameField text]];
	[callbackObject performSelector:callbackSelector withObject:fullName];
	[callbackObject release];
	[dirName release];
}

- (IBAction) cancel {
	// unregister for keyboard notifications while not visible.
	[[NSNotificationCenter defaultCenter] removeObserver:self name:UIKeyboardWillShowNotification object:nil];
	[[self superview] sendSubviewToBack:self];
	[callbackObject release];
	[dirName release];
}

- (IBAction) mkDir {
	//
}

- (IBAction) up {
	//
}

- (IBAction) down {
	//
}

- (IBAction) typeChanged {
	//
}

- (void) dealloc {
    [super dealloc];
}


@end
