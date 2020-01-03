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

#import "PrintView.h"
#import "RootViewController.h"
#import "shell.h"
#import "shell_spool.h"


@implementation PrintView

@synthesize scrollView;
@synthesize tile1;
@synthesize tile2;

static PrintView *instance;
static CGFloat scale;

unsigned char *print_bitmap;
int printout_top;
int printout_bottom;
unsigned char *print_text;
int print_text_top;
int print_text_bottom;
int print_text_pixel_height;

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
    print_text = (unsigned char *) malloc(PRINT_TEXT_SIZE);
    // TODO - handle memory allocation failure
    scale = (CGFloat) (self.bounds.size.width / 179.0);
    
    FILE *printfile = fopen("config/print", "r");
    if (printfile != NULL) {
        size_t n = fread(&printout_bottom, 1, sizeof(int), printfile);
        if (n == sizeof(int)) {
            int bytes = printout_bottom * PRINT_BYTESPERLINE;
            n = fread(print_bitmap, 1, bytes, printfile);
            if (n == bytes) {
                n = fread(&print_text_bottom, 1, sizeof(int), printfile);
                size_t n2 = fread(&print_text_pixel_height, 1, sizeof(int), printfile);
                if (n == sizeof(int) && n2 == sizeof(int)) {
                    n = fread(print_text, 1, print_text_bottom, printfile);
                    if (n != print_text_bottom) {
                        print_text_bottom = 0;
                        print_text_pixel_height = 0;
                    }
                } else {
                    print_text_bottom = 0;
                    print_text_pixel_height = 0;
                }
            } else {
                printout_bottom = 0;
                print_text_bottom = 0;
                print_text_pixel_height = 0;
            }
        } else {
            printout_bottom = 0;
            print_text_bottom = 0;
            print_text_pixel_height = 0;
        }
        fclose(printfile);
    } else {
        printout_bottom = 0;
        print_text_bottom = 0;
        print_text_pixel_height = 0;
    }
    printout_top = 0;
    print_text_top = 0;

    [self repositionTiles:true];
    [self scrollToBottom];
}

- (void)dealloc {
    [super dealloc];
}

+ (PrintView *) instance {
    return instance;
}

+ (CGFloat) scale {
    return scale;
}

static char *tb;
static int tblen, tbcap;

static void tbwriter(const char *text, int length) {
    if (tblen + length > tbcap) {
        tbcap += length + 8192;
        tb = (char *) realloc(tb, tbcap);
    }
    if (tb != NULL) {
        memcpy(tb + tblen, text, length);
        tblen += length;
    }
}

static void tbnewliner() {
    tbwriter("\n", 1);
}

static void tbnonewliner() {
    // No-op
}

- (NSString *) printOutAsText {
    tb = NULL;
    tblen = tbcap = 0;
    
    int len = print_text_bottom - print_text_top;
    if (len < 0)
        len += PRINT_TEXT_SIZE;
    // Calculate effective top, since printout_top can point
    // at a truncated line, and we want to skip those when
    // copying
    int top = printout_bottom - print_text_pixel_height;
    if (top < 0)
        top += PRINT_LINES;
    int p = print_text_top;
    int pixel_v = 0;
    while (len > 0) {
        int z = print_text[p++];
        if (z == 255) {
            char buf[34];
            for (int v = 0; v < 16; v += 2) {
                for (int vv = 0; vv < 2; vv++) {
                    int V = top + pixel_v + v + vv;
                    if (V >= PRINT_LINES)
                        V -= PRINT_LINES;
                    for (int h = 0; h < 17; h++) {
                        unsigned char a = print_bitmap[V * PRINT_BYTESPERLINE + h];
                        buf[vv * 17 + h] = a;
                    }
                }
                shell_spool_bitmap_to_txt(buf, 17, 0, 0, 131, 2, tbwriter, tbnewliner);
            }
            pixel_v += 16;
        } else {
            if (p + z < PRINT_TEXT_SIZE) {
                shell_spool_txt((const char *) (print_text + p), z, tbwriter, tbnewliner);
                p += z;
            } else {
                int d = PRINT_TEXT_SIZE - p;
                shell_spool_txt((const char *) (print_text + p), d, tbwriter, tbnonewliner);
                shell_spool_txt((const char *) print_text, z - d, tbwriter, tbnewliner);
                p = z - d;
            }
            len -= z;
            pixel_v += 9;
        }
        len--;
    }
    tbwriter("\0", 1);
    
    if (tb == NULL) {
        return @"";
    } else {
        NSString *txt = [NSString stringWithUTF8String:tb];
        free(tb);
        return txt;
    }
}

- (void) copyAsText {
    NSString *txt = [self printOutAsText];
    UIPasteboard *pb = [UIPasteboard generalPasteboard];
    [pb setString:txt];
}

- (UIImage *) printOutAsImage {
    int height = printout_bottom - printout_top;
    if (height < 0)
        height += PRINT_LINES;
    
    unsigned char *data;
    if (height == 0) {
        height = 1;
        data = (unsigned char *) malloc(2 * 358);
        memset(data, 255, 2 * 358);
    } else {
        data = (unsigned char *) malloc(height * 2 * 358);
        unsigned char *dst = data;
        for (int v = 0; v < height; v++) {
            int vv = printout_top + v;
            if (vv >= PRINT_LINES)
                vv -= PRINT_LINES;
            unsigned char *src = print_bitmap + vv * PRINT_BYTESPERLINE;
            for (int i = 0; i < 36; i++)
                *dst++ = 255;
            for (int h = 0; h < 143; h++) {
                unsigned char c = src[h >> 3];
                unsigned char k = (c & (1 << (h & 7))) == 0 ? 255 : 0;
                *dst++ = k;
                *dst++ = k;
            }
            for (int i = 0; i < 36; i++)
                *dst++ = 255;
            memcpy(dst, dst - 358, 358);
            dst += 358;
        }
    }
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceGray();
    CFDataRef d = CFDataCreateWithBytesNoCopy(NULL, data, height * 2 * 358, kCFAllocatorMalloc);
    CGDataProviderRef provider = CGDataProviderCreateWithCFData(d);
    CGImageRef cgImg = CGImageCreate(358, height * 2, 8, 8, 358, colorSpace, kCGImageAlphaNone, provider, NULL, false, kCGRenderingIntentDefault);
    UIImage *img = [UIImage imageWithCGImage:cgImg];
    // In the previous version of this logic, the following lines were
    // executed *after* putting img on the clipboard. Was that necessary?
    CGImageRelease(cgImg);
    CGDataProviderRelease(provider);
    CFRelease(d);
    CGColorSpaceRelease(colorSpace);
    return img;
}

- (void) copyAsImage {
    UIImage *img = [self printOutAsImage];
    UIPasteboard *pb = [UIPasteboard generalPasteboard];
    [pb setImage:img];
}

- (void) clear {
    printout_top = printout_bottom = 0;
    print_text_top = print_text_bottom = 0;
    print_text_pixel_height = 0;
    [self repositionTiles:false];
}

- (IBAction) advance {
    static const char *bits = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
    shell_print("", 0, bits, 18, 0, 0, 143, 9);
}

- (IBAction) edit {
    UIActionSheet *menu =
    [[UIActionSheet alloc] initWithTitle:@"Edit Menu"
                                delegate:self cancelButtonTitle:@"Cancel" destructiveButtonTitle:nil
                       otherButtonTitles:@"Copy as Text", @"Copy as Image", @"Clear", nil];
    [menu showInView:self];
    [menu release];
}

- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex {
    switch (buttonIndex) {
        case 0:
            [self copyAsText];
            break;
        case 1:
            [self copyAsImage];
            break;
        case 2:
            [self clear];
            break;
        case 3:
            // Cancel
            break;
    }
}

- (IBAction) share {
    NSString *txt = [self printOutAsText];
    UIImage *img = [self printOutAsImage];
    UIActivityViewController *activityViewController = [[UIActivityViewController alloc] initWithActivityItems:@[txt, img] applicationActivities:nil];
    [self.window.rootViewController presentViewController:activityViewController animated:YES completion:nil];
}

- (IBAction) done {
    [RootViewController showMain];
}

- (void) updatePrintout:(update_params *) ppar {
    int newlength = ppar->newlength;
    //int oldlength = ppar->oldlength;
    //int height = ppar->height;
    if (newlength >= PRINT_LINES) {
        printout_top = (printout_bottom + 1) % PRINT_LINES;
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
    size_t n;
    int length;
    
    printfile = fopen("config/print", "w");
    if (printfile != NULL) {
        // Write bitmap
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
        // Write text
        length = print_text_bottom - print_text_top;
        if (length < 0)
            length += PRINT_TEXT_SIZE;
        n = fwrite(&length, 1, sizeof(int), printfile);
        if (n != sizeof(int))
            goto failed;
        n = fwrite(&print_text_pixel_height, 1, sizeof(int), printfile);
        if (n != sizeof(int))
            goto failed;
        if (print_text_bottom >= print_text_top) {
            n = fwrite(print_text + print_text_top, 1, length, printfile);
            if (n != length)
                goto failed;
        } else {
            n = fwrite(print_text + print_text_top, 1, PRINT_TEXT_SIZE - print_text_top, printfile);
            if (n != PRINT_TEXT_SIZE - print_text_top)
                goto failed;
            n = fwrite(print_text, 1, print_text_bottom, printfile);
            if (n != print_text_bottom)
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
    [scrollView setContentSize:CGSizeMake(self.bounds.size.width, printHeight * scale)];
    int tilePos = ((int) offset.y) / ((int) size.height);
    CGRect tile1rect = CGRectMake(0, size.height * tilePos, size.width, size.height);
    if (tile1rect.origin.y < 0) {
        tile1rect.size.height += tile1rect.origin.y;
        tile1rect.origin.y = 0;
    }
    CGRect tile2rect = CGRectMake(0, size.height * (tilePos + 1), size.width, size.height);
    int excessHeight = tile2rect.origin.y + tile2rect.size.height - printHeight * scale;
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
