/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2013  Thomas Okken
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
#import "Free42AppDelegate.h"


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
    instance = self;
    print_bitmap = (unsigned char *) malloc(PRINT_SIZE);
    // TODO - handle memory allocation failure
    
    FILE *printfile = fopen("config/print", "r");
    if (printfile != NULL) {
        int n = fread(&printout_bottom, 1, sizeof(int), printfile);
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
}

- (void)dealloc {
    [super dealloc];
}

+ (PrintView *) instance {
    return instance;
}

- (void)drawRect:(CGRect)rect {
    // Drawing code
}

- (void) raised {
	// start-up code
}

- (IBAction) clear {
	// TODO
}

- (IBAction) done {
	[Free42AppDelegate showMain];
}

- (void) initialUpdate {
    
}

- (void) updatePrintout:(id) params {
    
}

- (void) scrollToBottom {
    
}

+ (void) dump {
    FILE *printfile;
    int n, length;
    
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

@end
