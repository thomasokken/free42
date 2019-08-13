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

#include <string>
#include <vector>
#include <algorithm>
#include <string.h>

#include "shell_main.h"
#include "StatesWindow.h"
#include "core_main.h"

using std::string;
using std::vector;
using std::sort;

static string stateLabel;
static string stateName;
static vector<string> stateNames;

HMENU moreMenu = NULL;

static void loadStateNames() {
	stateNames.clear();

	WIN32_FIND_DATA wfd;
	string path = free42dirname;
	path += "\\*.f42";

	HANDLE search = FindFirstFile(path.c_str(), &wfd);
    if (search != INVALID_HANDLE_VALUE) {
        do {
            if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
				string s;
				s.assign(wfd.cFileName, strlen(wfd.cFileName) - 4);
				stateNames.push_back(s);
			}
		} while (FindNextFile(search, &wfd));
		FindClose(search);
	}

	// TODO: Case insensitive sorting!
	std::sort(stateNames.begin(), stateNames.end());
}

static LRESULT CALLBACK StateNameDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
        case WM_INITDIALOG: {
            SetDlgItemText(hDlg, IDC_STATE_PROMPT, stateLabel.c_str());
            SetDlgItemText(hDlg, IDC_STATE_NAME, "");
			return TRUE;
		}
        case WM_COMMAND: {
            int cmd = LOWORD(wParam);
            switch (cmd) {
				case IDOK: {
					char buf[FILENAMELEN];
					GetDlgItemText(hDlg, IDC_STATE_NAME, buf, FILENAMELEN);
					// TODO: Check for illegal characters, and check
					// for names that are already in use
					stateName = buf;
					EndDialog(hDlg, 0);
					return TRUE;
				}
				case IDCANCEL: {
					stateName = "";
					EndDialog(hDlg, 0);
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

static string getStateName(HWND hDlg, const string label) {
	stateLabel = label;
    DialogBox(hInst, (LPCTSTR)IDD_STATE_NAME, hDlg, (DLGPROC)StateNameDlgProc);
	return stateName;
}

static string getSelectedStateName(HWND hDlg) {
	HWND list = GetDlgItem(hDlg, IDC_STATES);
	int index = SendMessage(list, LB_GETCURSEL, 0, 0);
	if (index == LB_ERR)
		return "";
	else
		return stateNames[index];
}

static void updateUI(HWND hDlg, bool rescan) {
	string selName = getSelectedStateName(hDlg);
	if (rescan) {
		loadStateNames();
		HWND list = GetDlgItem(hDlg, IDC_STATES);
		SendMessage(list, LB_RESETCONTENT, 0, 0);
		int index = -1;
		int i = 0;
		for(vector<string>::iterator iter = stateNames.begin(); iter != stateNames.end(); ++iter) {
			SendMessage(list, LB_ADDSTRING, 0, (LPARAM) iter->c_str());
			if (selName == *iter)
				index = i;
			i++;
		}
		if (index != -1)
			SendMessage(list, LB_SETCURSEL, index, 0);
	}

	HWND switchToButton = GetDlgItem(hDlg, IDOK);
	bool stateSelected;
    bool activeStateSelected;

    if (selName == "") {
		SendMessage(switchToButton, WM_SETTEXT, 0, (LPARAM) "Switch To");
        stateSelected = false;
    } else {
        activeStateSelected = selName == state.coreName; // TODO: Case insensitive comparison
        if (activeStateSelected)
			SendMessage(switchToButton, WM_SETTEXT, 0, (LPARAM) "Reload");
        else
			SendMessage(switchToButton, WM_SETTEXT, 0, (LPARAM) "Switch To");
        stateSelected = true;
    }

	EnableWindow(switchToButton, stateSelected);
	HMENU hMenu = GetSubMenu(moreMenu, 0);
	EnableMenuItem(hMenu, IDM_MORE_DUPLICATE, stateSelected ? MF_ENABLED : MF_DISABLED);
	EnableMenuItem(hMenu, IDM_MORE_RENAME, stateSelected ? MF_ENABLED : MF_DISABLED);
	EnableMenuItem(hMenu, IDM_MORE_DELETE, stateSelected && !activeStateSelected ? MF_ENABLED : MF_DISABLED);
	EnableMenuItem(hMenu, IDM_MORE_EXPORT, stateSelected ? MF_ENABLED : MF_DISABLED);
}

static void switchTo(HWND hDlg) {
	string selName = getSelectedStateName(hDlg);
	if (selName == "")
        return;
    if (selName != state.coreName) { // TODO: Case insensitivity
		string path = free42dirname;
		path += "\\";
		path += state.coreName;
		path += ".f42";
		core_save_state(path.c_str());
    }
    core_cleanup();
	strcpy(state.coreName, selName.c_str()); // TODO: Length limit
	string path = free42dirname;
	path += "\\";
	path += state.coreName;
	path += ".f42";
	core_init(1, 26, path.c_str(), 0);
    EndDialog(hDlg, 0);
}

static void doNew(HWND hDlg) {
	string name = getStateName(hDlg, "New state name:");
	if (name == "")
		return;
	string path = free42dirname;
	path += "\\";
	path += name;
	path += ".f42";
    FILE *f = fopen(path.c_str(), "wb");
    fprintf(f, "24kF");
    fclose(f);
	updateUI(hDlg, true);
}

static void doDuplicate() {

}

static void doRename() {

}

static void doDelete() {

}

static void doImport(HWND hDlg) {
	char buf[FILENAMELEN];
	buf[0] = 0;
    if (browse_file(hDlg,
                    "Select State File",
                    0,
                    "Free42 State (*.f42)\0*.f42\0All Files (*.*)\0*.*\0\0",
                    "f42",
                    buf,
                    FILENAMELEN))
		fprintf(stderr, "Huzzah!\n");
}

static void doExport() {

}

// Mesage handler for States dialog.
LRESULT CALLBACK StatesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
        case WM_INITDIALOG: {
			if (moreMenu == NULL)
				moreMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_STATES_MORE));
			string txt("Current: ");
			txt += state.coreName;
            SetDlgItemText(hDlg, IDC_CURRENT, txt.c_str());
			updateUI(hDlg, true);
			return TRUE;
		}
        case WM_COMMAND: {
            int cmd = LOWORD(wParam);
            switch (cmd) {
				case IDOK: {
					switchTo(hDlg);
					return TRUE;
				}
				case IDC_MORE: {
		            HWND moreButton = GetDlgItem(hDlg, IDC_MORE);
					RECT rect;
					GetWindowRect(moreButton, &rect);
					POINT pt;
					pt.x = rect.left;
					pt.y = rect.bottom;
					ClientToScreen(hDlg, &pt);
					HMENU hMenu = GetSubMenu(moreMenu, 0);
					TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, rect.left, rect.bottom, 0, hDlg, NULL);
					return TRUE;
				}
				case IDCANCEL: {
					EndDialog(hDlg, 0);
					return TRUE;
				}
				case IDM_MORE_NEW: {
					doNew(hDlg);
					return TRUE;
				}
				case IDM_MORE_DUPLICATE: {
					doDuplicate();
					return TRUE;
				}
				case IDM_MORE_RENAME: {
					doRename();
					return TRUE;
				}
				case IDM_MORE_DELETE: {
					doDelete();
					return TRUE;
				}
				case IDM_MORE_IMPORT: {
					doImport(hDlg);
					return TRUE;
				}
				case IDM_MORE_EXPORT: {
					doExport();
					return TRUE;
				}
				case IDC_STATES: {
					int wmEvent = HIWORD(wParam);
					switch (wmEvent) {
						// TODO: Selection behavior: you can't deselect
						// in a single-select box, apparently; that appears
						// to require using a multi-select box, and then
						// deselecting all items but the most recently
						// selected one in the LBN_SELCHANGE handler.
						// (Or something like that. Details TBD.)
						case LBN_SELCHANGE:
						case LBN_SELCANCEL:
							updateUI(hDlg, false);
							return TRUE;
						case LBN_DBLCLK:
							switchTo(hDlg);
							return TRUE;
					}
					return FALSE;
				}
			}
		}
	}
	return FALSE;
}

/*
- (BOOL) copyStateFrom:(const char *)orig_name to:(const char *)copy_name {
    FILE *fin = fopen(orig_name, "r");
    FILE *fout = fopen(copy_name, "w");
    if (fin != NULL && fout != NULL) {
        char buf[1024];
        int n;
        while ((n = fread(buf, 1, 1024, fin)) > 0)
            fwrite(buf, 1, n, fout);
        if (ferror(fin) || ferror(fout))
            goto duplication_failed;
        fclose(fin);
        fclose(fout);
        return YES;
    } else {
        duplication_failed:
        if (fin != NULL)
            fclose(fin);
        if (fout != NULL)
            fclose(fout);
        remove(copy_name);
        return NO;
    }
}

- (void) doDuplicate {
    NSString *name = [self selectedStateName];
    if (name == nil)
        return;
    NSString *copyName = [NSString stringWithFormat:@"%s/%@", free42dirname, name];
    int n = 0;

    // We're naming duplicates by appending " copy" or " copy NNN" to the name
    // of the original, but if the name of the original already ends with " copy"
    // or " copy NNN", it seems more elegant to continue the sequence rather than
    // add another " copy" suffix.
    int len = [copyName length];
    if (len > 5 && [[copyName substringFromIndex:len - 5] caseInsensitiveCompare:@" copy"] == NSOrderedSame) {
        copyName = [copyName substringToIndex:len - 5];
        n = 1;
    } else if (len > 7) {
        int pos = len - 7;
        int m = 0;
        int p = 1;
        while (pos > 0) {
            unichar c = [copyName characterAtIndex:pos + 6];
            if (c < '0' || c > '9')
                goto not_a_copy;
            m += p * (c - '0');
            p *= 10;
            if ([[copyName substringWithRange:NSMakeRange(pos, 6)] caseInsensitiveCompare:@" copy "] == NSOrderedSame) {
                n = m;
                copyName = [copyName substringToIndex:pos];
                break;
            } else
                pos--;
        }
        not_a_copy:;
    }

    NSString *finalName;
    const char *finalNameC;
    while (true) {
        n++;
        if (n == 1)
            finalName = [NSString stringWithFormat:@"%@ copy.f42", copyName];
        else
            finalName = [NSString stringWithFormat:@"%@ copy %d.f42", copyName, n];
        finalNameC = [finalName UTF8String];
        struct stat st;
        if (stat(finalNameC, &st) != 0)
            // File does not exist; that means we have a usable name
            break;
    }

    // Once we get here, copy_name contains a valid name for creating the duplicate.
    // What we do next depends on whether the selected state is the currently active
    // one. If it is, we'll call core_save_state(), to make sure the duplicate
    // actually matches the most up-to-date state; otherwise, we can simply copy
    // the existing state file.
    if ([name caseInsensitiveCompare:[NSString stringWithUTF8String:state.coreName]] == NSOrderedSame)
        core_save_state(finalNameC);
    else {
        NSString *origName = [NSString stringWithFormat:@"%s/%@.f42", free42dirname, name];
        if (![self copyStateFrom:[origName UTF8String] to:finalNameC])
            [Free42AppDelegate showMessage:@"State duplication failed." withTitle:@"Error"];
    }
    [self updateUI:YES];
}

- (void) doRename {
    NSString *oldname = [[self selectedStateName] retain];
    if (oldname == nil)
        return;
    [stateNameWindow setupWithLabel:[NSString stringWithFormat:@"Rename \"%@\" to:", oldname] existingNames:[stateListDataSource getNames]];
    [NSApp runModalForWindow:stateNameWindow];
    NSString *newname = [stateNameWindow selectedName];
    if (newname == nil) {
        [oldname release];
        return;
    }
    NSString *oldpath = [NSString stringWithFormat:@"%s/%@.f42", free42dirname, oldname];
    NSString *newpath = [NSString stringWithFormat:@"%s/%@.f42", free42dirname, newname];
    rename([oldpath UTF8String], [newpath UTF8String]);
    if ([oldname caseInsensitiveCompare:[NSString stringWithUTF8String:state.coreName]] == NSOrderedSame) {
        strncpy(state.coreName, [newname UTF8String], FILENAMELEN);
        [current setStringValue:newname];
    }
    [oldname release];
    [self updateUI:YES];
}

- (void) doDelete {
    NSString *name = [self selectedStateName];
    if (name == nil)
        return;
    if ([name caseInsensitiveCompare:[NSString stringWithUTF8String:state.coreName]] == NSOrderedSame) {
        return;
    }
    NSAlert *alert = [[[NSAlert alloc] init] autorelease];
    [alert addButtonWithTitle:@"Delete"];
    [alert addButtonWithTitle:@"Cancel"];
    [alert setMessageText:@"Delete state?"];
    [alert setInformativeText:[NSString stringWithFormat:@"Are you sure you want to delete the state \"%@\"?", name]];
    [alert setAlertStyle:NSWarningAlertStyle];
    // This 'retain' appears to be necessary because of the runModal call.
    // I added the other 'retain' calls in this class as a precaution against
    // the same scenario.
    [name retain];
    if ([alert runModal] != NSAlertFirstButtonReturn) {
        [name release];
        return;
    }
    NSString *statePath = [NSString stringWithFormat:@"%s/%@.f42", free42dirname, name];
    remove([statePath UTF8String]);
    [name release];
    [self updateUI:YES];
}

- (void) doImport {
    FileOpenPanel *openDlg = [FileOpenPanel panelWithTitle:@"Import State" types:@"Free42 State;f42;All Files;*"];
    if ([openDlg runModal] != NSOKButton)
        return;
    NSArray *paths = [openDlg paths];
    for (int i = 0; i < [paths count]; i++) {
        NSString *path = [paths objectAtIndex:i];
        NSString *name = [path lastPathComponent];
        int len = [name length];
        if (len > 4 && [[name substringFromIndex:len - 4] caseInsensitiveCompare:@".f42"] == NSOrderedSame)
            name = [name substringToIndex:len - 4];
        NSString *destPath = [NSString stringWithFormat:@"%s/%@.f42", free42dirname, name];
        const char *destPathC = [destPath UTF8String];
        struct stat st;
        if (stat(destPathC, &st) == 0)
            [Free42AppDelegate showMessage:[NSString stringWithFormat:@"A state named \"%@\" already exists.", name] withTitle:@"Error"];
        else if (![self copyStateFrom:[path UTF8String] to:destPathC])
            [Free42AppDelegate showMessage:@"State import failed." withTitle:@"Error"];
    }
    [self updateUI:YES];
}

- (void) doExport {
    NSString *name = [[self selectedStateName] retain];
    if (name == nil)
        return;
    FileSavePanel *saveDlg = [FileSavePanel panelWithTitle:@"Export State" types:@"Free42 State;f42;All Files;*"];
    if ([saveDlg runModal] != NSOKButton) {
        [name release];
        return;
    }
    NSString *copyPath = [saveDlg path];
    const char *copyPathC = [copyPath UTF8String];
    if ([name caseInsensitiveCompare:[NSString stringWithUTF8String:state.coreName]] == NSOrderedSame)
        core_save_state(copyPathC);
    else {
        NSString *origPath = [NSString stringWithFormat:@"%s/%@.f42", free42dirname, name];
        if (![self copyStateFrom:[origPath UTF8String] to:copyPathC])
            [Free42AppDelegate showMessage:@"State export failed." withTitle:@"Error"];
    }
    [name release];
}
*/
