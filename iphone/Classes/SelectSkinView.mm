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

#import <dirent.h>
#import "SelectSkinView.h"
#import "CalcView.h"
#import "RootViewController.h"
#import "shell_skin_iphone.h"
#import "core_main.h"


@implementation SelectSkinView

@synthesize doneButton;
@synthesize skinTable;

- (id) initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Initialization code
    }
    return self;
}

- (id) initWithCoder:(NSCoder *)coder {
    [super initWithCoder:coder];
    skinNames = [[NSMutableArray arrayWithCapacity:10] retain];
    selectedIndex = -1;
    return self;
}

- (void) drawRect:(CGRect)rect {
    // Drawing code
}

- (void) raised {
    // This gets called just before the view is raised, every time
    [skinNames removeAllObjects];
    DIR *dir = opendir("skins");
    struct dirent *d;
    while ((d = readdir(dir)) != NULL) {
        size_t len = strlen(d->d_name);
        if (len < 8 || strcmp(d->d_name + len - 7, ".layout") != 0)
            continue;
        d->d_name[len - 7] = 0;
        NSString *s = [NSString stringWithUTF8String:d->d_name];
        [skinNames addObject:s];
    }
    closedir(dir);
    [skinNames sortUsingSelector:@selector(caseInsensitiveCompare:)];
    char buf[1024];
    NSString *path = [[NSBundle mainBundle] pathForResource:@"builtin_skins" ofType:@"txt"];
    [path getCString:buf maxLength:1024 encoding:NSUTF8StringEncoding];
    FILE *builtins = fopen(buf, "r");
    int builtins_count = 0;
    while (fgets(buf, 1024, builtins) != NULL) {
        char *context;
        char *name = strtok_r(buf, " \t\r\n", &context);
        [skinNames insertObject:[NSString stringWithUTF8String:name] atIndex:builtins_count++];
    }
    fclose(builtins);
    if (enabled != NULL)
        delete[] enabled;
    int total_count = [skinNames count];
    enabled = new bool[total_count];
    for (int i = 0; i < builtins_count; i++) {
        enabled[i] = true;
        NSString *n1 = [skinNames objectAtIndex:i];
        for (int j = builtins_count; j < total_count; j++) {
            NSString *n2 = [skinNames objectAtIndex:j];
            if ([n1 compare:n2] == NSOrderedSame) {
                enabled[i] = false;
                break;
            }
        }
    }
    for (int i = builtins_count; i < total_count; i++)
        enabled[i] = true;
    char *skinName = [CalcView isPortrait] ? state.skinName : state.landscapeSkinName;
    NSString *name = [NSString stringWithUTF8String:skinName];
    selectedIndex = (int) ([skinNames count] - 1);
    while (selectedIndex >= 0 && [[skinNames objectAtIndex:selectedIndex] compare:name] != NSOrderedSame)
        selectedIndex--;
    [skinTable reloadData];
}

- (IBAction) done {
    [RootViewController showMain];
}

- (IBAction) loadSkin {
    [RootViewController showLoadSkin];
}

- (IBAction) deleteSkin {
    [RootViewController showDeleteSkin];
}

- (void) tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    NSUInteger n = [indexPath indexAtPosition:1];
    NSString *name = [skinNames objectAtIndex:n];
    char *skinName = [CalcView isPortrait] ? state.skinName : state.landscapeSkinName;
    [name getCString:skinName maxLength:FILENAMELEN encoding:NSUTF8StringEncoding];
    long width, height;
    skin_load(&width, &height);
    core_repaint_display();
    [CalcView repaint];
    [self done];
}

- (UITableViewCell *) tableView:(UITableView *)table cellForRowAtIndexPath:(NSIndexPath *) indexPath {
    NSUInteger n = [indexPath indexAtPosition:1];
    NSString *s = [skinNames objectAtIndex:n];
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
    cell.textLabel.text = s;
    cell.accessoryType = selectedIndex == (int) n ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
    cell.userInteractionEnabled = enabled[n];
    [cell.textLabel setEnabled:enabled[n]];
    return cell;
}

- (NSInteger) tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section {
    NSUInteger n = [skinNames count];
    return n;
}

- (void) dealloc {
    [super dealloc];
}


@end
