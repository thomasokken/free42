/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2009  Thomas Okken
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

@class MainView;
@class PrintOutView;
@class HTTPServerView;
@class SelectSkinView;
@class PreferencesView;
@class AboutView;
@class SelectFileView;

@interface shell_iphone : NSObject <UIApplicationDelegate> {
    UIWindow *window;
	UIView *containerView;
	MainView *mainView;
	PrintOutView *printOutView;
	HTTPServerView *httpServerView;
	SelectSkinView *selectSkinView;
	PreferencesView *preferencesView;
	AboutView *aboutView;
	SelectFileView *selectFileView;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet UIView *containerView;
@property (nonatomic, retain) IBOutlet MainView *mainView;
@property (nonatomic, retain) IBOutlet PrintOutView *printOutView;
@property (nonatomic, retain) IBOutlet HTTPServerView *httpServerView;
@property (nonatomic, retain) IBOutlet SelectSkinView *selectSkinView;
@property (nonatomic, retain) IBOutlet PreferencesView *preferencesView;
@property (nonatomic, retain) IBOutlet AboutView *aboutView;
@property (nonatomic, retain) IBOutlet SelectFileView *selectFileView;

+ (void) playSound: (int) which;
+ (void) showMain;
+ (void) showPrintOut;
+ (void) showHttpServer;
+ (void) showSelectSkin;
+ (void) showPreferences;
+ (void) showAbout;
+ (void) showSelectFile;
+ (const char *) getVersion;

@end

