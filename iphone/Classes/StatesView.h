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


@interface StatesView : UIView <UIActionSheetDelegate, UITableViewDelegate, UITableViewDataSource> {
    NSMutableArray *stateNames;
    UIBarButtonItem *switchToButton;
    UITableView *stateTable;
    int selectedIndex;
    int mode;
}

@property (nonatomic, retain) IBOutlet UIBarButtonItem *switchToButton;
@property (nonatomic, retain) IBOutlet UITableView *stateTable;

- (void) raised;
- (void) selectState:(NSString *)stateName;
- (IBAction) switchTo;
- (IBAction) more;
- (IBAction) done;
+ (NSString *) makeCopyName:(NSString *)name;
- (void) actionSheet:(UIActionSheet *) actionSheet clickedButtonAtIndex:(NSInteger) buttonIndex;
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath;
- (UITableViewCell *) tableView:(UITableView *)table cellForRowAtIndexPath:(NSIndexPath*) indexPath;
- (NSInteger) tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section;

@end
