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

@class CalcView;
@class PrintView;
@class HTTPServerView;
@class SelectSkinView;
@class SelectProgramsView;
@class PreferencesView;
@class AboutView;
@class SelectFileView;
@class DeleteSkinView;
@class LoadSkinView;
@class StatesView;

@interface RootViewController : UIViewController {
    UIWindow *window;
    CalcView *calcView;
    PrintView *printView;
    HTTPServerView *httpServerView;
    SelectSkinView *selectSkinView;
    SelectProgramsView *selectProgramsView;
    PreferencesView *preferencesView;
    AboutView *aboutView;
    SelectFileView *selectFileView;
    DeleteSkinView *deleteSkinView;
    LoadSkinView *loadSkinView;
    StatesView *statesView;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet CalcView *calcView;
@property (nonatomic, retain) IBOutlet PrintView *printView;
@property (nonatomic, retain) IBOutlet HTTPServerView *httpServerView;
@property (nonatomic, retain) IBOutlet SelectSkinView *selectSkinView;
@property (nonatomic, retain) IBOutlet SelectProgramsView *selectProgramsView;
@property (nonatomic, retain) IBOutlet PreferencesView *preferencesView;
@property (nonatomic, retain) IBOutlet AboutView *aboutView;
@property (nonatomic, retain) IBOutlet SelectFileView *selectFileView;
@property (nonatomic, retain) IBOutlet DeleteSkinView *deleteSkinView;
@property (nonatomic, retain) IBOutlet LoadSkinView *loadSkinView;
@property (nonatomic, retain) IBOutlet StatesView *statesView;

- (void) enterBackground;
- (void) leaveBackground;
- (void) quit;
- (void) batteryLevelChanged;
- (void) layoutSubViews;

+ (void) showMessage:(NSString *) message;
+ (void) playSound: (int) which;
+ (void) showMain;
+ (void) showPrintOut;
+ (void) showHttpServer;
+ (void) showSelectSkin;
+ (void) showPreferences;
+ (void) showAbout;
+ (void) showSelectFile;
+ (void) doImport;
+ (void) doExport:(BOOL)share;
+ (void) showLoadSkin;
+ (void) showDeleteSkin;
+ (void) showStates:(NSString *)stateName;

@end

void export_programs(int count, const int *indexes, int (*writer)(const char *buf, int buflen));
void import_programs(int (*reader)(char *buf, int buflen));
