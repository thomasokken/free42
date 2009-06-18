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

#import <dirent.h>
#import "SelectSkinView.h"
#import "MainView.h"
#import "shell_iphone.h"
#import "shell_skin_iphone.h"
#import "core_main.h"

// From skins.cc
extern int skin_count;
extern const char *skin_name[];


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
	return self;
}

- (void) drawRect:(CGRect)rect {
    // Drawing code
}

- (void) raised {
	// This gets called just before the view is raised, every time
	// TODO: highlight the currently selected skin
	// TODO: separator between built-in and external skins
	[skinNames removeAllObjects];
	for (int i = 0; i < skin_count; i++)
		[skinNames addObject:[NSString stringWithCString:skin_name[i]]];
	DIR *dir = opendir("skins");
	struct dirent *d;
	while ((d = readdir(dir)) != NULL) {
		int len = strlen(d->d_name);
		if (len < 8 || strcmp(d->d_name + len - 7, ".layout") != 0)
			continue;
		d->d_name[len - 7] = 0;
		[skinNames addObject:[NSString stringWithCString:d->d_name]];
	}
	closedir(dir);
	[skinTable reloadData];
}

- (IBAction) done {
	[shell_iphone showMain];
}

- (void) tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	int n = [indexPath indexAtPosition:1];
	NSString *name = [skinNames objectAtIndex:n];
	[name getCString:state.skinName maxLength:FILENAMELEN encoding:NSUTF8StringEncoding];
	long width, height;
	skin_load(&width, &height);
	core_repaint_display();
	[MainView repaint];
	[self done];
}

- (UITableViewCell *) tableView:(UITableView *)table cellForRowAtIndexPath:(NSIndexPath *) indexPath {
	int n = [indexPath indexAtPosition:1];
	NSString *s = [skinNames objectAtIndex:n];
	UITableViewCell *cell = [[UITableViewCell alloc] initWithFrame:CGRectZero reuseIdentifier:nil];
	cell.textLabel.text = s;
	return cell;
}

- (NSInteger) tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section {
	int n = [skinNames count];
	return n;
}

- (void) dealloc {
    [super dealloc];
}


@end
