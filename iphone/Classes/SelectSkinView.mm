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

#import <dirent.h>
#import "SelectSkinView.h"
#import "CalcView.h"
#import "Free42AppDelegate.h"
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
	char buf[1024];
	NSString *path = [[NSBundle mainBundle] pathForResource:@"builtin_skins" ofType:@"txt"];
	[path getCString:buf maxLength:1024 encoding:NSUTF8StringEncoding];
	FILE *builtins = fopen(buf, "r");
	while (fgets(buf, 1024, builtins) != NULL) {
		char *context;
		char *name = strtok_r(buf, " \t\r\n", &context);
		[skinNames addObject:[NSString stringWithCString:name encoding:NSUTF8StringEncoding]];
	}
	fclose(builtins);
	DIR *dir = opendir("skins");
	struct dirent *d;
	int num_builtin_skins = [skinNames count];
	while ((d = readdir(dir)) != NULL) {
		int len = strlen(d->d_name);
		if (len < 8 || strcmp(d->d_name + len - 7, ".layout") != 0)
			continue;
		d->d_name[len - 7] = 0;
		NSString *s = [NSString stringWithCString:d->d_name encoding:NSUTF8StringEncoding];
		for (int i = 0; i < num_builtin_skins; i++)
			if ([s caseInsensitiveCompare:[skinNames objectAtIndex:i]] == 0)
				goto skip;
		[skinNames addObject:s];
		skip:;
	}
	closedir(dir);
	[skinTable reloadData];
}

- (IBAction) done {
	[Free42AppDelegate showMain];
}

- (void) tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	int n = [indexPath indexAtPosition:1];
	NSString *name = [skinNames objectAtIndex:n];
	[name getCString:state.skinName maxLength:FILENAMELEN encoding:NSUTF8StringEncoding];
	long width, height;
	skin_load(&width, &height);
	core_repaint_display();
	[CalcView repaint];
	[self done];
}

- (UITableViewCell *) tableView:(UITableView *)table cellForRowAtIndexPath:(NSIndexPath *) indexPath {
	int n = [indexPath indexAtPosition:1];
	NSString *s = [skinNames objectAtIndex:n];
	UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
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
