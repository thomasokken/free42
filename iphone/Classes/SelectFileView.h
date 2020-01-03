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


@interface SelectFileView : UIView <UITextFieldDelegate, UITableViewDelegate, UITableViewDataSource> {
    UINavigationItem *navigationItem;
    UILabel *directoryLabel;
    UITableView *directoryListingView;
    UITextField *nameField;
    UISegmentedControl *typeSelector;
    UIButton *selectFileButton;
    UIScrollView *scrollView;
    UIView *contentView;
    UITextField *activeField;
}

@property (nonatomic, retain) IBOutlet UINavigationItem *navigationItem;
@property (nonatomic, retain) IBOutlet UILabel *directoryLabel;
@property (nonatomic, retain) IBOutlet UITableView *directoryListingView;
@property (nonatomic, retain) IBOutlet UITextField *nameField;
@property (nonatomic, retain) IBOutlet UISegmentedControl *typeSelector;
@property (nonatomic, retain) IBOutlet UIButton *selectFileButton;
@property (nonatomic, retain) IBOutlet UIScrollView *scrollView;
@property (nonatomic, retain) IBOutlet UIView *contentView;

+ (void) raiseWithTitle:(NSString *)wt selectTitle:(NSString *)st types:(NSString *)t selectDir:(BOOL)sd callbackObject:(id)cb_id callbackSelector:(SEL)cb_sel;
- (void) raised;
- (void) textFieldDidBeginEditing:(UITextField *)textField;
- (void) textFieldDidEndEditing:(UITextField *)textField;
- (BOOL) textFieldShouldReturn:(UITextField *)textField;
- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath;
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath;
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section;
- (IBAction) selectFile;
- (IBAction) cancel;
- (IBAction) mkDir;
- (IBAction) typeChanged;

@end
