/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2024  Thomas Okken
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
@synthesize loadButton;
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
    showMessages = YES;
}

- (void)hideKeyboard {
    [self endEditing:YES];
}

- (IBAction) done {
    showMessages = NO;
    [task[0] cancel];
    [task[1] cancel];
    [RootViewController showSelectSkin];
}

- (IBAction) reload {
    NSString *url = [urlField text];
    NSURLRequest *req = [NSURLRequest requestWithURL:[NSURL URLWithString:url]];
    [webView loadRequest:req];
}

- (IBAction) loadSkin {
    NSString *url = [urlField text];
    NSArray *urls = [LoadSkinView skinUrlPair:url];
    if (urls == nil) {
        [RootViewController showMessage:@"Invalid Skin URL"];
        return;
    }
    [loadButton setEnabled:NO];
    skinName = [urls[2] retain];
    if (session == nil) {
        NSURLSessionConfiguration *conf = [NSURLSessionConfiguration ephemeralSessionConfiguration];
        session = [[NSURLSession sessionWithConfiguration:conf] retain];
    }
    for (int t = 0; t < 2; t++) {
        task[t] = [session dataTaskWithURL:[NSURL URLWithString:urls[t]] completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
            taskSuccess[t] = error == nil;
            if (taskSuccess[t]) {
                NSString *name = t == 0 ? @"_temp_gif_" : @"_temp_layout_";
                NSString *fname = [NSString stringWithFormat:@"skins/%@", name];
                [data writeToFile:fname atomically:NO];
            }
            task[t] = nil;
            [self performSelectorOnMainThread:@selector(finishTask) withObject:nil waitUntilDone:NO];
        }];
    }
    [task[0] resume];
    [task[1] resume];
}

- (void) finishTask {
    if (task[0] != nil || task[1] != nil)
        return;
    if (taskSuccess[0] && taskSuccess[1]) {
        char buf[FILENAMELEN];
        snprintf(buf, FILENAMELEN, "skins/%s.gif", [skinName UTF8String]);
        rename("skins/_temp_gif_", buf);
        snprintf(buf, FILENAMELEN, "skins/%s.layout", [skinName UTF8String]);
        rename("skins/_temp_layout_", buf);
        if (showMessages)
            [RootViewController showMessage:@"Skin Loaded"];
    } else {
        remove("skins/_temp_gif_");
        remove("skins/_temp_layout_");
        if (showMessages)
            [RootViewController showMessage:@"Loading Skin Failed"];
    }
    [skinName release];
    [loadButton setEnabled:YES];
}

- (void)webViewDidStartLoad:(UIWebView *)webView {
    [loadButton setEnabled:NO];
    [loadButton setTitle:@"..."];
}

- (void)webViewDidFinishLoad:(UIWebView *)webView {
    NSString *url = [[[webView request] URL] absoluteString];
    [urlField setText:url];
    [loadButton setTitle:@"Load"];
    [loadButton setEnabled:[LoadSkinView skinUrlPair:url] != nil];
}

+ (NSArray *)skinUrlPair:(NSString *)url {
    NSURLComponents *u = [NSURLComponents componentsWithString:url];
    NSString *path = [u path];
    if (path == nil)
        return nil;
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
        return nil;
    }
    NSString *baseName = [gifUrl lastPathComponent];
    return [NSArray arrayWithObjects:gifUrl, layoutUrl, [[baseName substringToIndex:[baseName length] - 4] stringByRemovingPercentEncoding], nil];
}

@end
