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


struct update_params {
    int oldlength;
    int newlength;
    int height;
};

#define PRINT_LINES 18000
#define PRINT_BYTESPERLINE 18
#define PRINT_SIZE 324000
// Room for PRINT_LINES / 9 lines, plus two, plus one byte
#define PRINT_TEXT_SIZE 50051

extern unsigned char *print_bitmap;
extern int printout_top;
extern int printout_bottom;
extern unsigned char *print_text;
extern int print_text_top;
extern int print_text_bottom;
extern int print_text_pixel_height;

@class PrintTileView;

@interface PrintView : UIView <UIActionSheetDelegate, UIScrollViewDelegate> {
    UIScrollView *scrollView;
    PrintTileView *tile1;
    PrintTileView *tile2;
}

@property (nonatomic, retain) IBOutlet UIScrollView *scrollView;
@property (nonatomic, retain) IBOutlet PrintTileView *tile1;
@property (nonatomic, retain) IBOutlet PrintTileView *tile2;

+ (PrintView *) instance;
+ (CGFloat) scale;
- (void) awakeFromNib;
- (IBAction) advance;
- (IBAction) edit;
- (void) actionSheet:(UIActionSheet *) actionSheet clickedButtonAtIndex:(NSInteger) buttonIndex;
- (IBAction) share;
- (IBAction) done;
- (void) updatePrintout:(update_params *) params;
- (void) scrollToBottom;
+ (void) dump;
- (void) scrollViewDidScroll:(UIScrollView *)scrollView;
- (void) repositionTiles:(bool)force;

@end
