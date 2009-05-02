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
#import <sys/stat.h>
#import "shell_iphone.h"
#import "SelectFileView.h"

@implementation SelectFileView

@synthesize navigationItem;
@synthesize directoryLabel;
@synthesize directoryListingView;
@synthesize nameField;
@synthesize typeSelector;
@synthesize selectFileButton;

static NSString *windowTitle;
static NSString *selectTitle;
static NSString *types;
static BOOL selectDir;
static id callbackObject;
static SEL callbackSelector;

static NSString *dirName = NULL;
static NSMutableArray *dirList = NULL;
static bool *dirType = NULL;
static int dirTypeLength = 0;
static int dirTypeCapacity = 0;

- (id) initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Initialization code
    }
    return self;
}


- (void) drawRect:(CGRect)rect {
    // Drawing code
}

+ (void) raiseWithTitle:(NSString *)wt selectTitle:(NSString *)st types:(NSString *)t selectDir:(BOOL)sd callbackObject:(id)cb_id callbackSelector:(SEL)cb_sel {
	windowTitle = [wt retain];
	selectTitle = [st retain];
	types = [t retain];
	selectDir = sd;
	callbackObject = [cb_id retain];
	callbackSelector = cb_sel;
	dirName = [[NSString stringWithString:@"."] retain];
	[shell_iphone showSelectFile];
}

- (void) raised {
	// watch the keyboard so we can adjust the user interface if necessary.
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillShow:) name:UIKeyboardWillShowNotification object:nil];
	
	[navigationItem setTitle:windowTitle];
	[windowTitle release];
	[selectFileButton setTitle:selectTitle forState:UIControlStateNormal];
	[selectFileButton setTitle:selectTitle forState:UIControlStateHighlighted];
	[selectFileButton setTitle:selectTitle forState:UIControlStateDisabled];
	[selectFileButton setTitle:selectTitle forState:UIControlStateSelected];
	[selectTitle release];
	
	[typeSelector removeAllSegments];
	NSArray *ta = [types componentsSeparatedByString:@","];
	int tc = [ta count];
	if (tc == 0)
		typeSelector.enabled = NO;
	else {
		typeSelector.enabled = YES;
		for (int i = 0; i < tc; i++) {
			[typeSelector insertSegmentWithTitle:[ta objectAtIndex:i] atIndex:i animated:NO];
			[typeSelector setWidth:0 forSegmentAtIndex:i];
		}
		typeSelector.selectedSegmentIndex = 0;
		// It appears that UISegmentedControl.removeAllSegments does
		// not actually remove all segments, so now we make sure:
		while ([typeSelector numberOfSegments] > tc)
			[typeSelector removeSegmentAtIndex:tc animated:NO];
	}
	
	// Force initial directory read
	[self typeChanged];
}

static int keyboard_height = 0;
static int view_offset = 0;

- (void) keyboardWillShow:(NSNotification *)notif {
	NSDictionary *userInfo = [notif userInfo];
	NSValue *v = [userInfo objectForKey:UIKeyboardBoundsUserInfoKey];
	CGRect r;
	[v getValue:&r];
	keyboard_height = r.size.height;
}

- (void) textFieldDidBeginEditing:(UITextField *)textField {
	CGRect v_r = [self frame];
	CGRect tf_r = [textField frame];
	view_offset = v_r.origin.y + v_r.size.height - keyboard_height - tf_r.origin.y - tf_r.size.height - 5;
	if (view_offset > 0)
		view_offset = 0;
	[UIView beginAnimations:@"foo" context:NULL];
	[self setCenter:CGPointMake([self center].x, [self center].y + view_offset)];
	[UIView commitAnimations];
}

- (void) textFieldDidEndEditing:(UITextField *)textField {
	[UIView beginAnimations:@"foo" context:NULL];
	[self setCenter:CGPointMake([self center].x, [self center].y - view_offset)];
	[UIView commitAnimations];
}

- (BOOL) textFieldShouldReturn:(UITextField *)textField {
	[textField resignFirstResponder];
	return NO;
}

- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	int n = [indexPath indexAtPosition:1];
	if (dirList == NULL || n >= [dirList count])
		return nil;
	NSString *item = [dirList objectAtIndex:n];
	if (dirType[n]) {
		// Selected item is a directory
		NSString *newDirName;
		if ([item isEqualToString:@".."]) {
			NSRange r = [dirName rangeOfString:@"/" options:NSBackwardsSearch];
			if (r.location == NSNotFound)
				return nil;
			newDirName = [[dirName substringToIndex:r.location] retain];
		} else {
			newDirName = [[NSString stringWithFormat:@"%@/%@", dirName, item] retain];
		}
		[dirName release];
		dirName = newDirName;
		[self performSelectorOnMainThread:@selector(typeChanged) withObject:nil waitUntilDone:NO];
	} else {
		// Selected item is a file
		[nameField setText:item];
	}
	return nil;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	int n = [indexPath indexAtPosition:1];
	if (dirList == NULL || n >= [dirList count])
		return nil;
	UITableViewCell *cell = [[UITableViewCell alloc] initWithFrame:CGRectZero reuseIdentifier:nil];
	cell.text = [dirList objectAtIndex:n];
	return cell;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	if (section == 0 && dirList != NULL)
		return [dirList count];
	else
		return 0;
}

- (IBAction) selectFile {
	// unregister for keyboard notifications while not visible.
	[[NSNotificationCenter defaultCenter] removeObserver:self name:UIKeyboardWillShowNotification object:nil];
	[[self superview] sendSubviewToBack:self];
	NSString *fullName = [NSString stringWithFormat:@"%@/%@", dirName, [nameField text]];
	// Get rid of the leading "./"
	fullName = [fullName substringFromIndex:2];
	[callbackObject performSelector:callbackSelector withObject:fullName];
	[callbackObject release];
	[dirName release];
}

- (IBAction) cancel {
	// unregister for keyboard notifications while not visible.
	[[NSNotificationCenter defaultCenter] removeObserver:self name:UIKeyboardWillShowNotification object:nil];
	[[self superview] sendSubviewToBack:self];
	[callbackObject release];
	[dirName release];
}

- (IBAction) mkDir {
	//
}

- (IBAction) typeChanged {
	if (dirList == NULL)
		dirList = [[NSMutableArray arrayWithCapacity:10] retain];
	else
		[dirList removeAllObjects];
	dirTypeLength = 0;
	
	// Display directory name
	if ([dirName isEqualToString:@"."])
		[directoryLabel setText:@"Documents"];
	else {
		NSRange r = [dirName rangeOfString:@"/" options:NSBackwardsSearch];
		[directoryLabel setText:[dirName substringFromIndex:(r.location + 1)]];
	}
	
	const char *typeStr = [[typeSelector titleForSegmentAtIndex:[typeSelector selectedSegmentIndex]] cStringUsingEncoding:NSUTF8StringEncoding];

	const char *cDirName = [dirName cStringUsingEncoding:NSUTF8StringEncoding];
	DIR *dir = opendir(cDirName);
	struct dirent *d;
	
	while ((d = readdir(dir)) != NULL) {
		if (strcmp(d->d_name, ".") == 0)
			continue;
		if (strcmp(d->d_name, "..") == 0 && strcmp(cDirName, ".") == 0)
			// Don't show ".." in the top-level (Documents) directory
			continue;
		char *p = (char *) malloc(strlen(cDirName) + strlen(d->d_name) + 2);
		strcpy(p, cDirName);
		strcat(p, "/");
		strcat(p, d->d_name);
		struct stat s;
		int err = stat(p, &s);
		free(p);
		if (err == 0 && (S_ISREG(s.st_mode) || S_ISDIR(s.st_mode))) {
			if (S_ISREG(s.st_mode) && strcmp(typeStr, "*") != 0) {
				// Check file suffix
				int flen = strlen(d->d_name);
				int slen = strlen(typeStr);
				if (flen - slen <= 0)
					continue;
				if (d->d_name[flen - slen - 1] != '.' || strcasecmp(d->d_name + (flen - slen), typeStr) != 0)
					continue;
			}
			[dirList addObject:[NSString stringWithCString:d->d_name encoding:NSUTF8StringEncoding]];
			dirTypeLength++;
			if (dirTypeLength > dirTypeCapacity) {
				dirTypeCapacity += 10;
				dirType = (bool *) realloc(dirType, dirTypeCapacity * sizeof(bool));
			}
			dirType[[dirList count] - 1] = S_ISDIR(s.st_mode);
		}
	}
	closedir(dir);
	[directoryListingView reloadData];
}

- (void) dealloc {
    [super dealloc];
}


@end
