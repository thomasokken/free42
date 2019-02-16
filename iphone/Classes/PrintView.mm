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

#import "PrintView.h"
#import "RootViewController.h"


@implementation PrintView

@synthesize clearButton;
@synthesize doneButton;
@synthesize scrollView;
@synthesize tile1;
@synthesize tile2;

static PrintView *instance;

unsigned char *print_bitmap;
int printout_top;
int printout_bottom;

- (id)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Initialization code
    }
    return self;
}

- (void) awakeFromNib {
    [super awakeFromNib];
    instance = self;
    print_bitmap = (unsigned char *) malloc(PRINT_SIZE);
    // TODO - handle memory allocation failure
    
    FILE *printfile = fopen("config/print", "r");
    if (printfile != NULL) {
        size_t n = fread(&printout_bottom, 1, sizeof(int), printfile);
        if (n == sizeof(int)) {
            int bytes = printout_bottom * PRINT_BYTESPERLINE;
            n = fread(print_bitmap, 1, bytes, printfile);
            if (n != bytes)
                printout_bottom = 0;
        } else
            printout_bottom = 0;
        fclose(printfile);
    } else
        printout_bottom = 0;
    printout_top = 0;
    for (int n = printout_bottom * PRINT_BYTESPERLINE; n < PRINT_SIZE; n++)
        print_bitmap[n] = 0;
    [self repositionTiles:true];
    [self scrollToBottom];
}

- (void)dealloc {
    [super dealloc];
}

+ (PrintView *) instance {
    return instance;
}

- (IBAction) clear {
    printout_top = printout_bottom = 0;
    [self repositionTiles:false];
}

- (IBAction) done {
    [RootViewController showMain];
}

- (void) updatePrintout:(id) params {
    update_params *ppar = (update_params *) [((NSValue *) params) pointerValue];
    int newlength = ppar->newlength;
    //int oldlength = ppar->oldlength;
    //int height = ppar->height;
    delete ppar;
    if (newlength >= PRINT_LINES) {
        printout_top = (printout_bottom + 2) % PRINT_LINES;
        [self repositionTiles:true];
    } else {
        [self repositionTiles:false];
    }
    [self scrollToBottom];
}

- (void) scrollToBottom {
    CGPoint bottomOffset = CGPointMake(0, self.scrollView.contentSize.height - self.scrollView.bounds.size.height);
    [self.scrollView setContentOffset:bottomOffset animated:NO];
}

- (void) scrollViewDidScroll:(UIScrollView *)scrollView {
    [self repositionTiles:false];
}

+ (void) dump {
    FILE *printfile;
    ssize_t n, length;
    
    printfile = fopen("config/print", "w");
    if (printfile != NULL) {
        length = printout_bottom - printout_top;
        if (length < 0)
            length += PRINT_LINES;
        n = fwrite(&length, 1, sizeof(int), printfile);
        if (n != sizeof(int))
            goto failed;
        if (printout_bottom >= printout_top) {
            n = fwrite(print_bitmap + PRINT_BYTESPERLINE * printout_top,
                       1, PRINT_BYTESPERLINE * length, printfile);
            if (n != PRINT_BYTESPERLINE * length)
                goto failed;
        } else {
            n = fwrite(print_bitmap + PRINT_BYTESPERLINE * printout_top,
                       1, PRINT_SIZE - PRINT_BYTESPERLINE * printout_top,
                       printfile);
            if (n != PRINT_SIZE - PRINT_BYTESPERLINE * printout_top)
                goto failed;
            n = fwrite(print_bitmap, 1,
                       PRINT_BYTESPERLINE * printout_bottom, printfile);
            if (n != PRINT_BYTESPERLINE * printout_bottom)
                goto failed;
        }
        
        fclose(printfile);
        goto done;
        
    failed:
        fclose(printfile);
        remove("config/print");
        
    done:
        ;
    }
}

- (void) repositionTiles:(bool)force {
    CGPoint offset = scrollView.contentOffset;
    CGSize size = scrollView.bounds.size;
    int printHeight = printout_bottom - printout_top;
    if (printHeight < 0)
        printHeight += PRINT_LINES;
    [scrollView setContentSize:CGSizeMake(self.bounds.size.width, printHeight)];
    int tilePos = ((int) offset.y) / ((int) size.height);
    CGRect tile1rect = CGRectMake(0, size.height * tilePos, size.width, size.height);
    if (tile1rect.origin.y < 0) {
        tile1rect.size.height += tile1rect.origin.y;
        tile1rect.origin.y = 0;
    }
    CGRect tile2rect = CGRectMake(0, size.height * (tilePos + 1), size.width, size.height);
    int excessHeight = tile2rect.origin.y + tile2rect.size.height - printHeight;
    if (excessHeight > 0) {
        int oldHeight = (int) tile2rect.size.height;
        int newHeight = oldHeight - excessHeight;
        if (newHeight < 0)
            newHeight = 0;
        tile2rect.size.height = newHeight;
        excessHeight -= oldHeight - newHeight;
        if (excessHeight > 0) {
            oldHeight = (int) tile1rect.size.height;
            newHeight = oldHeight - excessHeight;
            if (newHeight < 0)
                newHeight = 0;
            tile1rect.size.height = newHeight;
        }
    }
    if ((tilePos & 1) != 0) {
        CGRect temp = tile1rect;
        tile1rect = tile2rect;
        tile2rect = temp;
    }
    if (!CGRectEqualToRect([tile1 bounds], tile1rect)) {
        //NSLog(@"tile1 = (%d %d %d %d)", (int) tile1rect.origin.x, (int) tile1rect.origin.y, (int) tile1rect.size.width, (int) tile1rect.size.height);
        [tile1 setBounds:tile1rect];
        [tile1 setFrame:tile1rect];
    } else if (force)
        [tile1 setNeedsDisplay];
    if (!CGRectEqualToRect([tile2 bounds], tile2rect)) {
        //NSLog(@"tile2 = (%d %d %d %d)", (int) tile2rect.origin.x, (int) tile2rect.origin.y, (int) tile2rect.size.width, (int) tile2rect.size.height);
        [tile2 setBounds:tile2rect];
        [tile2 setFrame:tile2rect];
    } else if (force)
        [tile2 setNeedsDisplay];
}

@end
