/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2012  Thomas Okken
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

#import "PreferencesView.h"
#import "shell_iphone.h"
#import "MainView.h"
#import "SelectFileView.h"
#import "core_main.h"

@implementation PreferencesView

@synthesize doneButton;
@synthesize singularMatrixSwitch;
@synthesize matrixOutOfRangeSwitch;
@synthesize autoRepeatSwitch;
@synthesize printToTextSwitch;
@synthesize printToTextField;
@synthesize rawTextSwitch;
@synthesize printToGifSwitch;
@synthesize printToGifField;
@synthesize maxGifLengthField;
@synthesize popupKeyboardSwitch;

- (id) initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Initialization code
    }
    return self;
}

- (void) drawRect:(CGRect)rect {
    // Drawing code
}

- (void) raised {
	[singularMatrixSwitch setOn:core_settings.matrix_singularmatrix];
	[matrixOutOfRangeSwitch setOn:core_settings.matrix_outofrange];
	[autoRepeatSwitch setOn:core_settings.auto_repeat];
	[printToTextSwitch setOn:(state.printerToTxtFile != 0)];
	[printToTextField setText:[NSString stringWithCString:state.printerTxtFileName encoding:NSUTF8StringEncoding]];
	[rawTextSwitch setOn:core_settings.raw_text];
	[printToGifSwitch setOn:(state.printerToGifFile != 0)];
	[printToGifField setText:[NSString stringWithCString:state.printerGifFileName encoding:NSUTF8StringEncoding]];
	[maxGifLengthField setText:[NSString stringWithFormat:@"%d", state.printerGifMaxLength]];
	[popupKeyboardSwitch setOn:(state.popupKeyboard != 0)];
	
	// watch the keyboard so we can adjust the user interface if necessary.
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillShow:) name:UIKeyboardWillShowNotification object:nil];
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

- (IBAction) browseTextFile {
	[SelectFileView raiseWithTitle:@"Select Text File Name" selectTitle:@"OK" types:@"txt,*" selectDir:NO callbackObject:self callbackSelector:@selector(browseTextFileCB:)];
}

- (void) browseTextFileCB:(NSString *) path {
	if ([path length] > 0 && ![[path lowercaseString] hasSuffix:@".txt"])
		path = [path stringByAppendingString:@".txt"];
	[printToTextField setText:path];
}

- (IBAction) browseGifFile {
	[SelectFileView raiseWithTitle:@"Select GIF File Name" selectTitle:@"OK" types:@"gif,*" selectDir:NO callbackObject:self callbackSelector:@selector(browseGifFileCB:)];
}

- (void) browseGifFileCB:(NSString *) path {
	if ([path length] > 0 && ![[path lowercaseString] hasSuffix:@".gif"])
		path = [path stringByAppendingString:@".gif"];
	[printToGifField setText:path];
}

- (IBAction) done {
	// unregister for keyboard notifications while not visible.
	[[NSNotificationCenter defaultCenter] removeObserver:self name:UIKeyboardWillShowNotification object:nil];

	core_settings.matrix_singularmatrix = singularMatrixSwitch.on;
	core_settings.matrix_outofrange = matrixOutOfRangeSwitch.on;
	core_settings.auto_repeat = autoRepeatSwitch.on;
	state.printerToTxtFile = printToTextSwitch.on;
	NSString *s = [printToTextField text];
	if ([s length] > 0 && ![[s lowercaseString] hasSuffix:@".txt"])
		s = [s stringByAppendingString:@".txt"];
	[s getCString:state.printerTxtFileName maxLength:FILENAMELEN encoding:NSUTF8StringEncoding];
	core_settings.raw_text = rawTextSwitch.on;
	state.printerToGifFile = printToGifSwitch.on;
	s = [printToGifField text];
	if ([s length] > 0 && ![[s lowercaseString] hasSuffix:@".gif"])
		s = [s stringByAppendingString:@".gif"];
	[s getCString:state.printerGifFileName maxLength:FILENAMELEN encoding:NSUTF8StringEncoding];
	s = [maxGifLengthField text];
	char buf[32];
	[s getCString:buf maxLength:32 encoding:NSUTF8StringEncoding];
	if (sscanf(buf, "%d", &state.printerGifMaxLength) == 1) {
		if (state.printerGifMaxLength < 32)
			state.printerGifMaxLength = 32;
		else if (state.printerGifMaxLength > 32767)
			state.printerGifMaxLength = 32767;
	} else
		state.printerGifMaxLength = 256;
	state.popupKeyboard = popupKeyboardSwitch.on;
	[shell_iphone showMain];
}

- (void) dealloc {
    [super dealloc];
}


@end
