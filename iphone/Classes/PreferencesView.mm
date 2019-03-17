/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2019  Thomas Okken
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
#import "CalcView.h"
#import "SelectFileView.h"
#import "RootViewController.h"
#import "core_main.h"
#import "shell.h"
#import "shell_skin_iphone.h"

@implementation PreferencesView

@synthesize doneButton;
@synthesize singularMatrixSwitch;
@synthesize matrixOutOfRangeSwitch;
@synthesize autoRepeatSwitch;
@synthesize alwaysOnSwitch;
@synthesize keyClicksSwitch;
@synthesize hapticFeedbackSwitch;
@synthesize orientationSelector;
@synthesize maintainSkinAspectSwitch;
@synthesize printToTextSwitch;
@synthesize printToTextField;
@synthesize printToGifSwitch;
@synthesize printToGifField;
@synthesize maxGifLengthField;
@synthesize scrollView;
@synthesize contentView;

- (id) initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Initialization code
        activeField = nil;
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
    [alwaysOnSwitch setOn:shell_always_on(-1)];
    [keyClicksSwitch setOn:state.keyClicks != 0];
    [hapticFeedbackSwitch setOn:state.hapticFeedback != 0];
    [orientationSelector setSelectedSegmentIndex:state.orientationMode];
    [maintainSkinAspectSwitch setOn:state.maintainSkinAspect[[CalcView isPortrait] ? 0 : 1] != 0];
    [printToTextSwitch setOn:(state.printerToTxtFile != 0)];
    [printToTextField setText:[NSString stringWithCString:state.printerTxtFileName encoding:NSUTF8StringEncoding]];
    [printToGifSwitch setOn:(state.printerToGifFile != 0)];
    [printToGifField setText:[NSString stringWithCString:state.printerGifFileName encoding:NSUTF8StringEncoding]];
    [maxGifLengthField setText:[NSString stringWithFormat:@"%d", state.printerGifMaxLength]];
    
    // watch the keyboard so we can adjust the user interface if necessary.
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardDidShow:) name:UIKeyboardDidShowNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillHide:) name:UIKeyboardWillHideNotification object:nil];
    if ([[self gestureRecognizers] count] == 0) {
        UITapGestureRecognizer* tapBackground = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(hideKeyboard)];
        [tapBackground setNumberOfTapsRequired:1];
        tapBackground.cancelsTouchesInView = NO;
        [self addGestureRecognizer:tapBackground];
    }
    scrollView.contentSize = contentView.frame.size;
}

- (void) keyboardDidShow:(NSNotification *)notif {
    NSDictionary *userInfo = [notif userInfo];
    CGSize kbSize = [[userInfo objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue].size;

    UIEdgeInsets contentInsets = UIEdgeInsetsMake(0.0, 0.0, kbSize.height, 0.0);
    scrollView.contentInset = contentInsets;
    scrollView.scrollIndicatorInsets = contentInsets;

    // Make sure the active field is visible
    if (activeField != nil)
        [self.scrollView scrollRectToVisible:activeField.frame animated:YES];
}

- (void) keyboardWillHide:(NSNotification *)notif {
    UIEdgeInsets contentInsets = UIEdgeInsetsZero;
    scrollView.contentInset = contentInsets;
    scrollView.scrollIndicatorInsets = contentInsets;
}

- (void) textFieldDidBeginEditing:(UITextField *)textField {
    activeField = textField;
}

- (void) textFieldDidEndEditing:(UITextField *)textField {
    if (activeField == textField)
        activeField = nil;
}

- (BOOL) textFieldShouldReturn:(UITextField *)textField {
    [textField resignFirstResponder];
    return NO;
}

- (void)hideKeyboard {
    [self endEditing:YES];
}

- (IBAction) browseTextFile {
    if (activeField != nil)
        [activeField resignFirstResponder];
    [SelectFileView raiseWithTitle:@"Select Text File Name" selectTitle:@"OK" types:@"txt,*" selectDir:NO callbackObject:self callbackSelector:@selector(browseTextFileCB:)];
}

- (void) browseTextFileCB:(NSString *) path {
    if ([path length] > 0 && ![[path lowercaseString] hasSuffix:@".txt"])
        path = [path stringByAppendingString:@".txt"];
    [printToTextField setText:path];
}

- (IBAction) browseGifFile {
    if (activeField != nil)
        [activeField resignFirstResponder];
    [SelectFileView raiseWithTitle:@"Select GIF File Name" selectTitle:@"OK" types:@"gif,*" selectDir:NO callbackObject:self callbackSelector:@selector(browseGifFileCB:)];
}

- (void) browseGifFileCB:(NSString *) path {
    if ([path length] > 0 && ![[path lowercaseString] hasSuffix:@".gif"])
        path = [path stringByAppendingString:@".gif"];
    [printToGifField setText:path];
}

- (IBAction) done {
    [self endEditing:YES];
    
    // unregister for keyboard notifications while not visible.
    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIKeyboardDidShowNotification object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIKeyboardWillHideNotification object:nil];

    char buf[FILENAMELEN];
    core_settings.matrix_singularmatrix = singularMatrixSwitch.on;
    core_settings.matrix_outofrange = matrixOutOfRangeSwitch.on;
    core_settings.auto_repeat = autoRepeatSwitch.on;
    shell_always_on(alwaysOnSwitch.on);
    state.keyClicks = keyClicksSwitch.on;
    state.hapticFeedback = hapticFeedbackSwitch.on;
    state.orientationMode = (int) orientationSelector.selectedSegmentIndex;
    int isPortrait = [CalcView isPortrait] ? 0 : 1;
    int maintainSkinAspect = maintainSkinAspectSwitch.on ? 1 : 0;
    if (maintainSkinAspect != state.maintainSkinAspect[isPortrait]) {
        state.maintainSkinAspect[isPortrait] = maintainSkinAspect;
        long w, h;
        skin_load(&w, &h);
        core_repaint_display();
        [CalcView repaint];
    }
    state.printerToTxtFile = printToTextSwitch.on;
    NSString *s = [printToTextField text];
    if ([s length] > 0 && ![[s lowercaseString] hasSuffix:@".txt"])
        s = [s stringByAppendingString:@".txt"];
    strcpy(buf, state.printerTxtFileName);
    [s getCString:state.printerTxtFileName maxLength:FILENAMELEN encoding:NSUTF8StringEncoding];
    if (!state.printerToTxtFile || strcmp(buf, state.printerTxtFileName) != 0)
        [CalcView stopTextPrinting];
    state.printerToGifFile = printToGifSwitch.on;
    s = [printToGifField text];
    if ([s length] > 0 && ![[s lowercaseString] hasSuffix:@".gif"])
        s = [s stringByAppendingString:@".gif"];
    strcpy(buf, state.printerGifFileName);
    [s getCString:state.printerGifFileName maxLength:FILENAMELEN encoding:NSUTF8StringEncoding];
    s = [maxGifLengthField text];
    char numbuf[32];
    [s getCString:numbuf maxLength:32 encoding:NSUTF8StringEncoding];
    if (sscanf(numbuf, "%d", &state.printerGifMaxLength) == 1) {
        if (state.printerGifMaxLength < 16)
            state.printerGifMaxLength = 16;
        else if (state.printerGifMaxLength > 32767)
            state.printerGifMaxLength = 32767;
    } else
        state.printerGifMaxLength = 256;
    if (!state.printerToGifFile || strcmp(buf, state.printerGifFileName) != 0)
        [CalcView stopGifPrinting];
    [RootViewController showMain];
}

- (void) dealloc {
    [super dealloc];
}


@end
