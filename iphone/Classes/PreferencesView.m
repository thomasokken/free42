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

#import "PreferencesView.h"
#import "shell_iphone.h"
#import "MainView.h"
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
}

- (IBAction) browseTextFile {
	[shell_iphone playSound:10];
}

- (IBAction) browseGifFile {
	[shell_iphone playSound:10];
}

- (IBAction) doImport {
	[shell_iphone playSound:10];
}

- (IBAction) doExport {
	[shell_iphone playSound:10];
}

- (IBAction) clearPrint {
	[shell_iphone playSound:10];
}

- (IBAction) done {
	core_settings.matrix_singularmatrix = singularMatrixSwitch.on;
	core_settings.matrix_outofrange = matrixOutOfRangeSwitch.on;
	core_settings.auto_repeat = autoRepeatSwitch.on;
	state.printerToTxtFile = printToTextSwitch.on;
	NSString *s = [printToTextField text];
	[s getCString:state.printerTxtFileName maxLength:FILENAMELEN encoding:NSUTF8StringEncoding];
	core_settings.raw_text = rawTextSwitch.on;
	state.printerToGifFile = printToGifSwitch.on;
	s = [printToGifField text];
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
	[shell_iphone showMain];
}

- (void) dealloc {
    [super dealloc];
}


@end
