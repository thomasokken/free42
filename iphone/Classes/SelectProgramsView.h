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


@interface SelectProgramsView : UIView <UITableViewDataSource, UIAlertViewDelegate> {
    NSMutableArray *programNames;
    UIBarButtonItem *doneButton;
    UIBarButtonItem *backButton;
    UITableView *programTable;
    BOOL share;
}

@property (nonatomic, retain) IBOutlet UIBarButtonItem *doneButton;
@property (nonatomic, retain) IBOutlet UIBarButtonItem *backButton;
@property (nonatomic, retain) IBOutlet UITableView *programTable;

- (void) raised:(BOOL)share;
- (IBAction) done;
- (IBAction) back;
- (UITableViewCell *) tableView:(UITableView *)table cellForRowAtIndexPath:(NSIndexPath*) indexPath;
- (NSInteger) tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section;
- (void) alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex;

@end
