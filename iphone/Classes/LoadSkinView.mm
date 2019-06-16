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

#import "LoadSkinView.h"
#import "CalcView.h"
#import "RootViewController.h"
#import "shell_skin_iphone.h"
#import "core_main.h"


@implementation LoadSkinView

@synthesize urlField;
@synthesize webView;

- (id) initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Initialization code
    }
    return self;
}

- (id) initWithCoder:(NSCoder *)coder {
    [super initWithCoder:coder];
    return self;
}

- (void) drawRect:(CGRect)rect {
    // Drawing code
}

- (void) raised {
    if ([[self gestureRecognizers] count] == 0) {
        UITapGestureRecognizer* tapBackground = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(hideKeyboard)];
        [tapBackground setNumberOfTapsRequired:1];
        tapBackground.cancelsTouchesInView = NO;
        [self addGestureRecognizer:tapBackground];
    }
    NSString *url = [urlField text];
    if (url == nil || [url length] == 0)
        [urlField setText:@"https://thomasokken.com/free42/skins/"];
    [self reload];
}

- (void)hideKeyboard {
    [self endEditing:YES];
}

- (IBAction) done {
    [RootViewController showSelectSkin];
}

- (IBAction) reload {
    NSString *url = [urlField text];
    NSURLRequest *req = [NSURLRequest requestWithURL:[NSURL URLWithString:url]];
    [webView loadRequest:req];
}

- (BOOL) tryLoad:(NSString *)url asFile:(NSString *)name {
    NSURLRequest *req = [NSURLRequest requestWithURL:[NSURL URLWithString:url]];
    NSURLResponse *resp;
    NSError *err;
    NSData *data = [NSURLConnection sendSynchronousRequest:req returningResponse:&resp error:&err];
    if (data == nil)
        return NO;
    NSString *fname = [NSString stringWithFormat:@"skins/%@", name];
    return [data writeToFile:fname atomically:NO];
}

- (IBAction) loadSkin {
    NSString *url = [urlField text];
    NSURLComponents *u = [NSURLComponents componentsWithString:url];
    NSString *path = [u path];
    if (path == nil) {
        [RootViewController showMessage:@"Invalid Skin URL"];
        return;
    }
    NSString *gifUrl = nil, *layoutUrl = nil;
    if ([path hasSuffix:@".gif"]) {
        gifUrl = url;
        NSRange r = NSMakeRange(0, [path length] - 3);
        NSString *layoutPath = [[path substringWithRange:r] stringByAppendingString:@"layout"];
        [u setPath:layoutPath];
        layoutUrl = [u string];
    } else if ([path hasSuffix:@".layout"]) {
        layoutUrl = url;
        NSRange r = NSMakeRange(0, [path length] - 6);
        NSString *gifPath = [[path substringWithRange:r] stringByAppendingString:@"gif"];
        [u setPath:gifPath];
        gifUrl = [u string];
    } else {
        [RootViewController showMessage:@"Invalid Skin URL"];
        return;
    }
    if ([self tryLoad:gifUrl asFile:@"_temp_gif_"]
            && [self tryLoad:layoutUrl asFile:@"_temp_layout_"]) {
        char buf[FILENAMELEN];
        NSURL *u = [NSURL URLWithString:gifUrl];
        snprintf(buf, FILENAMELEN, "skins/%s", [[u lastPathComponent] UTF8String]);
        rename("skins/_temp_gif_", buf);
        u = [NSURL URLWithString:layoutUrl];
        snprintf(buf, FILENAMELEN, "skins/%s", [[u lastPathComponent] UTF8String]);
        rename("skins/_temp_layout_", buf);
    } else {
        char buf[FILENAMELEN];
        snprintf(buf, FILENAMELEN, "skins/_temp_gif_");
        remove(buf);
        snprintf(buf, FILENAMELEN, "skins/_temp_layout_");
        remove(buf);
        [RootViewController showMessage:@"Loading Skin Failed"];
    }
}
- (void)webViewDidFinishLoad:(UIWebView *)webView {
    NSString *url = [[[webView request] URL] absoluteString];
    [urlField setText:url];
}

@end
