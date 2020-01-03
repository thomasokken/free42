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

#import <UIKit/UIKit.h>

@interface PreferencesView : UIView <UITextFieldDelegate> {
    UIBarButtonItem *doneButton;
    UISwitch *singularMatrixSwitch;
    UISwitch *matrixOutOfRangeSwitch;
    UISwitch *autoRepeatSwitch;
    UISwitch *alwaysOnSwitch;
    UISlider *keyClicksSlider;
    UISlider *hapticFeedbackSlider;
    UISegmentedControl *orientationSelector;
    UISwitch *maintainSkinAspectSwitch;
    UISwitch *printToTextSwitch;
    UITextField *printToTextField;
    UISwitch *printToGifSwitch;
    UITextField *printToGifField;
    UITextField *maxGifLengthField;
    UIScrollView *scrollView;
    UIView *contentView;
    UITextField *activeField;
}

@property (nonatomic, retain) IBOutlet UIBarButtonItem *doneButton;
@property (nonatomic, retain) IBOutlet UISwitch *singularMatrixSwitch;
@property (nonatomic, retain) IBOutlet UISwitch *matrixOutOfRangeSwitch;
@property (nonatomic, retain) IBOutlet UISwitch *autoRepeatSwitch;
@property (nonatomic, retain) IBOutlet UISwitch *alwaysOnSwitch;
@property (nonatomic, retain) IBOutlet UISlider *keyClicksSlider;
@property (nonatomic, retain) IBOutlet UISlider *hapticFeedbackSlider;
@property (nonatomic, retain) IBOutlet UISegmentedControl *orientationSelector;
@property (nonatomic, retain) IBOutlet UISwitch *maintainSkinAspectSwitch;
@property (nonatomic, retain) IBOutlet UISwitch *printToTextSwitch;
@property (nonatomic, retain) IBOutlet UITextField *printToTextField;
@property (nonatomic, retain) IBOutlet UISwitch *printToGifSwitch;
@property (nonatomic, retain) IBOutlet UITextField *printToGifField;
@property (nonatomic, retain) IBOutlet UITextField *maxGifLengthField;
@property (nonatomic, retain) IBOutlet UIScrollView *scrollView;
@property (nonatomic, retain) IBOutlet UIView *contentView;

- (void) raised;
- (void) textFieldDidBeginEditing:(UITextField *)textField;
- (void) textFieldDidEndEditing:(UITextField *)textField;
- (BOOL) textFieldShouldReturn:(UITextField *)textField;
- (IBAction) keyClicksSliderUpdated;
- (IBAction) hapticFeedbackSliderUpdated;
- (IBAction) done;
- (IBAction) browseTextFile;
- (IBAction) browseGifFile;

@end
