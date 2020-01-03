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
#import <sys/stat.h>
#import "RootViewController.h"
#import "SelectFileView.h"

@implementation SelectFileView

@synthesize navigationItem;
@synthesize directoryLabel;
@synthesize directoryListingView;
@synthesize nameField;
@synthesize typeSelector;
@synthesize selectFileButton;
@synthesize scrollView;
@synthesize contentView;

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
        activeField = nil;
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
    dirName = @".";
    [RootViewController showSelectFile];
}

- (void) raised {
    // watch the keyboard so we can adjust the user interface if necessary.
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardDidShow:) name:UIKeyboardDidShowNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillHide:) name:UIKeyboardWillHideNotification object:nil];
    if ([[self gestureRecognizers] count] == 0) {
        UITapGestureRecognizer* tapBackground = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(hideKeyboard)];
        [tapBackground setNumberOfTapsRequired:1];
        tapBackground.cancelsTouchesInView = NO;
        [self addGestureRecognizer:tapBackground];
    }
    scrollView.contentSize = contentView.frame.size;
    
    [navigationItem setTitle:windowTitle];
    [windowTitle release];
    [selectFileButton setTitle:selectTitle forState:UIControlStateNormal];
    [selectFileButton setTitle:selectTitle forState:UIControlStateHighlighted];
    [selectFileButton setTitle:selectTitle forState:UIControlStateDisabled];
    [selectFileButton setTitle:selectTitle forState:UIControlStateSelected];
    [selectTitle release];
    
    [typeSelector removeAllSegments];
    NSArray *ta = [types componentsSeparatedByString:@","];
    NSUInteger tc = [ta count];
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

- (void) keyboardDidShow:(NSNotification *)notif {
    NSDictionary *userInfo = [notif userInfo];
    CGSize kbSize = [[userInfo objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue].size;
    
    UIEdgeInsets contentInsets = UIEdgeInsetsMake(0.0, 0.0, kbSize.height, 0.0);
    scrollView.contentInset = contentInsets;
    scrollView.scrollIndicatorInsets = contentInsets;
    
    // Make sure the active field is visible
    if (activeField != nil)
        [self.scrollView scrollRectToVisible:activeField.frame animated:YES];
}

- (void) keyboardWillHide:(NSNotification *)notif {
    UIEdgeInsets contentInsets = UIEdgeInsetsZero;
    scrollView.contentInset = contentInsets;
    scrollView.scrollIndicatorInsets = contentInsets;
}

- (void) textFieldDidBeginEditing:(UITextField *)textField {
    activeField = textField;
}

- (void) textFieldDidEndEditing:(UITextField *)textField {
    if (activeField == textField)
        activeField = nil;
}

- (BOOL) textFieldShouldReturn:(UITextField *)textField {
    [textField resignFirstResponder];
    return NO;
}

- (void)hideKeyboard {
    [self endEditing:YES];
}

- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    NSUInteger n = [indexPath indexAtPosition:1];
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
    NSUInteger n = [indexPath indexAtPosition:1];
    if (dirList == NULL || n >= [dirList count])
        return nil;
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
    cell.textLabel.text = [dirList objectAtIndex:n];
    return cell;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (section == 0 && dirList != NULL)
        return [dirList count];
    else
        return 0;
}

- (IBAction) selectFile {
    [self endEditing:YES];
    // unregister for keyboard notifications while not visible.
    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIKeyboardDidShowNotification object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIKeyboardWillHideNotification object:nil];

    [[self superview] sendSubviewToBack:self];
    NSString *fullName = [NSString stringWithFormat:@"%@/%@", dirName, [nameField text]];
    // Get rid of the leading "./"
    fullName = [fullName substringFromIndex:2];
    // If the selected type is not All Files (*), append file type, if necessary
    if ([fullName length] > 0) {
        NSString *suffix = [typeSelector titleForSegmentAtIndex:[typeSelector selectedSegmentIndex]];
        if ([suffix compare:@"*"] != NSOrderedSame) {
            suffix = [NSString stringWithFormat:@".%@", suffix];
            if (![[fullName lowercaseString] hasSuffix:[suffix lowercaseString]])
                fullName = [fullName stringByAppendingString:suffix];
        }
    }
    [callbackObject performSelector:callbackSelector withObject:fullName];
    [callbackObject release];
    [dirName release];
}

- (IBAction) cancel {
    [self endEditing:YES];
    // unregister for keyboard notifications while not visible.
    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIKeyboardDidShowNotification object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIKeyboardWillHideNotification object:nil];

    [[self superview] sendSubviewToBack:self];
    [callbackObject release];
    [dirName release];
}

- (IBAction) mkDir {
    NSString *fullName = [NSString stringWithFormat:@"%@/%@", dirName, [nameField text]];
    const char *cpath = [fullName UTF8String];
    mkdir(cpath, 0755);
    [self typeChanged];
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
    
    const char *typeStr = [[typeSelector titleForSegmentAtIndex:[typeSelector selectedSegmentIndex]] UTF8String];

    const char *cDirName = [dirName UTF8String];
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
                size_t flen = strlen(d->d_name);
                size_t slen = strlen(typeStr);
                if (flen <= slen)
                    continue;
                if (d->d_name[flen - slen - 1] != '.' || strcasecmp(d->d_name + (flen - slen), typeStr) != 0)
                    continue;
            }
            [dirList addObject:[NSString stringWithUTF8String:d->d_name]];
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
