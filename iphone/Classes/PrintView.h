/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2018  Thomas Okken
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


struct update_params {
    int oldlength;
    int newlength;
    int height;
};

#define PRINT_LINES 9000
#define PRINT_BYTESPERLINE 36
#define PRINT_SIZE 324000

extern unsigned char *print_bitmap;
extern int printout_top;
extern int printout_bottom;

@class PrintTileView;

@interface PrintView : UIView <UIScrollViewDelegate> {
    UIBarButtonItem *clearButton;
    UIBarButtonItem *doneButton;
    UIScrollView *scrollView;
    PrintTileView *tile1;
    PrintTileView *tile2;
}

@property (nonatomic, retain) IBOutlet UIBarButtonItem *clearButton;
@property (nonatomic, retain) IBOutlet UIBarButtonItem *doneButton;
@property (nonatomic, retain) IBOutlet UIScrollView *scrollView;
@property (nonatomic, retain) IBOutlet PrintTileView *tile1;
@property (nonatomic, retain) IBOutlet PrintTileView *tile2;

+ (PrintView *) instance;
- (void) awakeFromNib;
- (IBAction) clear;
- (IBAction) done;
- (void) updatePrintout:(id) params;
- (void) scrollToBottom;
+ (void) dump;
- (void) scrollViewDidScroll:(UIScrollView *)scrollView;
- (void) repositionTiles:(bool)force;

@end
