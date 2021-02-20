/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2021  Thomas Okken
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

static ci_string stateLabel;
static ci_string stateName;
static vector<ci_string> stateNames;
static ci_string selectedStateName;

HMENU moreMenu = NULL;

static void loadStateNames() {
    stateNames.clear();

    WIN32_FIND_DATAW wfd;
    ci_string path = free42dirname;
    path += L"\\*.f42";

    HANDLE search = FindFirstFileW(path.c_str(), &wfd);
    if (search != INVALID_HANDLE_VALUE) {
        do {
            if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                ci_string s;
                s.assign(wfd.cFileName, wcslen(wfd.cFileName) - 4);
                stateNames.push_back(s);
            }
        } while (FindNextFileW(search, &wfd));
        FindClose(search);
    }

    std::sort(stateNames.begin(), stateNames.end());
}

static bool verifyStateName(const ci_string name) {
    const wchar_t *str = name.c_str();
    wchar_t c;
    while ((c = *str++) != 0) {
        if (c >= 1 && c <= 31 || wcschr(L"<>:\"/\\|?*", c) != NULL)
            return false;
    }
    return true;
}

static bool stateNameInUse(const ci_string name) {
    for(vector<ci_string>::iterator iter = stateNames.begin(); iter != stateNames.end(); ++iter)
        if (name == *iter)
            return true;
    return false;
}

static LRESULT CALLBACK StateNameDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            SetDlgItemTextW(hDlg, IDC_STATE_PROMPT, stateLabel.c_str());
            SetDlgItemTextW(hDlg, IDC_STATE_NAME, L"");
            return TRUE;
        }
        case WM_COMMAND: {
            int cmd = LOWORD(wParam);
            switch (cmd) {
                case IDOK: {
                    stateName = GetDlgItemTextLong(hDlg, IDC_STATE_NAME);
                    if (!verifyStateName(stateName)) {
                        MessageBox(hDlg, "That name is not valid.", "Message", MB_ICONWARNING);
                        return TRUE;
                    }
                    if (stateNameInUse(stateName)) {
                        MessageBox(hDlg, "That name is already in use.", "Message", MB_ICONWARNING);
                        return TRUE;
                    }
                    EndDialog(hDlg, 0);
                    return TRUE;
                }
                case IDCANCEL: {
                    stateName = L"";
                    EndDialog(hDlg, 0);
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

static ci_string getStateName(HWND hDlg, const ci_string label) {
    stateLabel = label;
    DialogBoxW(hInst, (LPCWSTR)IDD_STATE_NAME, hDlg, (DLGPROC)StateNameDlgProc);
    return stateName;
}

static void updateUI(HWND hDlg, bool rescan) {
    HWND list = GetDlgItem(hDlg, IDC_STATES);
    if (rescan) {
        loadStateNames();
        SendMessage(list, LB_RESETCONTENT, 0, 0);
        int index = -1;
        int i = 0;
        for(vector<ci_string>::iterator iter = stateNames.begin(); iter != stateNames.end(); ++iter) {
            SendMessageW(list, LB_ADDSTRING, 0, (LPARAM) iter->c_str());
            if (selectedStateName == *iter)
                index = i;
            i++;
        }
        if (index != -1)
            SendMessage(list, LB_SETCURSEL, index, 0);
    }
    
    int index = SendMessage(list, LB_GETCURSEL, 0, 0);
    if (index == LB_ERR)
        selectedStateName = L"";
    else
        selectedStateName = stateNames[index];

    HWND switchToButton = GetDlgItem(hDlg, IDOK);
    bool stateSelected;
    bool activeStateSelected;

    if (selectedStateName == L"") {
        SendMessage(switchToButton, WM_SETTEXT, 0, (LPARAM) "Switch To");
        stateSelected = false;
    } else {
        activeStateSelected = selectedStateName == state.coreName;
        if (activeStateSelected)
            SendMessage(switchToButton, WM_SETTEXT, 0, (LPARAM) "Revert");
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
    if (selectedStateName == L"")
        return;
    if (selectedStateName == state.coreName) {
        ci_string prompt = ci_string(L"Are you sure you want to revert the state \"") + selectedStateName + L"\" to the last version saved?";
        if (MessageBoxW(hDlg, prompt.c_str(), L"Revert State?", MB_OKCANCEL) != IDOK)
            return;
    } else {
        ci_string path = free42dirname;
        path += L"\\";
        path += state.coreName;
        path += L".f42";
        char *cpath = wide2utf(path.c_str());
        core_save_state(cpath);
        free(cpath);
    }
    core_cleanup();
    wcscpy(state.coreName, selectedStateName.c_str()); // TODO: Length limit
    ci_string path = free42dirname;
    path += L"\\";
    path += state.coreName;
    path += L".f42";
    char *cpath = wide2utf(path.c_str());
    core_init(1, 26, cpath, 0);
    free(cpath);
    bool running = core_powercycle();
    EndDialog(hDlg, running ? 1 : 0);
}

static void doNew(HWND hDlg) {
    ci_string name = getStateName(hDlg, L"New state name:");
    if (name == L"")
        return;
    ci_string path = free42dirname;
    path += L"\\";
    path += name;
    path += L".f42";
    FILE *f = _wfopen(path.c_str(), L"wb");
    fprintf(f, FREE42_MAGIC_STR);
    fclose(f);
    updateUI(hDlg, true);
}

static bool copyState(const wchar_t *orig_name, const wchar_t *copy_name) {
    FILE *fin = _wfopen(orig_name, L"rb");
    FILE *fout = _wfopen(copy_name, L"wb");
    if (fin != NULL && fout != NULL) {
        char buf[1024];
        int n;
        while ((n = fread(buf, 1, 1024, fin)) > 0)
            fwrite(buf, 1, n, fout);
        if (ferror(fin) || ferror(fout))
            goto duplication_failed;
        fclose(fin);
        fclose(fout);
        return true;
    } else {
        duplication_failed:
        if (fin != NULL)
            fclose(fin);
        if (fout != NULL)
            fclose(fout);
        _wremove(copy_name);
        return false;
    }
}

static void doDuplicate(HWND hDlg) {
    if (selectedStateName == L"")
        return;
    ci_string copyName = free42dirname;
    copyName += L"\\";
    copyName += selectedStateName;
    int n = 0;

    // We're naming duplicates by appending " copy" or " copy NNN" to the name
    // of the original, but if the name of the original already ends with " copy"
    // or " copy NNN", it seems more elegant to continue the sequence rather than
    // add another " copy" suffix.
    int len = copyName.length();
    if (len > 5 && copyName.substr(len - 5) == L" copy") {
        copyName = copyName.substr(0, len - 5);
        n = 1;
    } else if (len > 7) {
        int pos = len - 7;
        int m = 0;
        int p = 1;
        while (pos > 0) {
            wchar_t c = copyName[pos + 6];
            if (c < '0' || c > '9')
                goto not_a_copy;
            m += p * (c - '0');
            p *= 10;
            if (copyName.substr(pos, 6) == L" copy ") {
                n = m;
                copyName = copyName.substr(0, pos);
                break;
            } else
                pos--;
        }
        not_a_copy:;
    }

    ci_string finalName;
    const wchar_t *finalNameC;
    while (true) {
        n++;
        if (n == 1)
            finalName = copyName + L" copy.f42";
        else
            finalName = copyName + L" copy " + to_ci_string(n) + L".f42";
        finalNameC = finalName.c_str();
        if (GetFileAttributesW(finalNameC) == INVALID_FILE_ATTRIBUTES)
            // File does not exist; that means we have a usable name
            break;
    }

    // Once we get here, copy_name contains a valid name for creating the duplicate.
    // What we do next depends on whether the selected state is the currently active
    // one. If it is, we'll call core_save_state(), to make sure the duplicate
    // actually matches the most up-to-date state; otherwise, we can simply copy
    // the existing state file.
    if (selectedStateName == state.coreName) {
        char *cfn = wide2utf(finalNameC);
        core_save_state(cfn);
        free(cfn);
    } else {
        ci_string origName = free42dirname;
        origName += L"\\";
        origName += selectedStateName;
        origName += L".f42";
        if (!copyState(origName.c_str(), finalNameC))
            MessageBox(hDlg, "State duplication failed.", "Message", MB_ICONWARNING);
    }
    updateUI(hDlg, true);
}

static void doRename(HWND hDlg) {
    if (selectedStateName == L"")
        return;
    ci_string prompt = L"Rename \"";
    prompt += selectedStateName;
    prompt += L"\" to:";
    ci_string newname = getStateName(hDlg, prompt);
    if (newname == L"")
        return;
    ci_string oldpath = ci_string(free42dirname) + L"\\" + selectedStateName + L".f42";
    ci_string newpath = ci_string(free42dirname) + L"\\" + newname + L".f42";
    _wrename(oldpath.c_str(), newpath.c_str());
    if (selectedStateName == state.coreName) {
        wcsncpy(state.coreName, newname.c_str(), FILENAMELEN);
        SetDlgItemTextW(hDlg, IDC_CURRENT, newname.c_str());
    }
    updateUI(hDlg, true);
}

static void doDelete(HWND hDlg) {
    if (selectedStateName == L"")
        return;
    if (selectedStateName == state.coreName)
        return;
    ci_string prompt = ci_string(L"Are you sure you want to delete the state \"") + selectedStateName + L"\"?";
    if (MessageBoxW(hDlg, prompt.c_str(), L"Delete State?", MB_OKCANCEL) != IDOK)
        return;
    ci_string statePath = ci_string(free42dirname) + L"\\" + selectedStateName + L".f42";
    _wremove(statePath.c_str());
    updateUI(hDlg, true);
}

static void doImport(HWND hDlg) {
    wchar_t buf[FILENAMELEN];
    buf[0] = 0;
    if (!browse_file_w(hDlg,
                    L"Import State",
                    0,
                    L"Free42 State (*.f42)\0*.f42\0All Files (*.*)\0*.*\0\0",
                    NULL,
                    buf,
                    FILENAMELEN))
        return;
    ci_string path = buf;
    size_t p = path.find_last_of('\\');
    ci_string name = p == std::string::npos ? path : path.substr(p + 1);
    int len = name.length();
    if (len > 4 && name.substr(len - 4) == L".f42")
        name = name.substr(0, len - 4);
    ci_string destPath = ci_string(free42dirname) + L"\\" + name + L".f42";
    const wchar_t *destPathC = destPath.c_str();
    if (GetFileAttributesW(destPathC) != INVALID_FILE_ATTRIBUTES) {
        ci_string message = ci_string(L"A state named \"") + name + L"\" already exists.";
        MessageBoxW(hDlg, message.c_str(), L"Message", MB_ICONWARNING);
    } else if (!copyState(path.c_str(), destPathC))
        MessageBox(hDlg, "State import failed.", "Message", MB_ICONWARNING);
    updateUI(hDlg, true);
}

static void doExport(HWND hDlg) {
    if (selectedStateName == L"")
        return;
    wchar_t buf[FILENAMELEN];
    wcsncpy(buf, selectedStateName.c_str(), FILENAMELEN);
    buf[FILENAMELEN - 1] = 0;
    if (!browse_file_w(hDlg,
                    L"Export State",
                    1,
                    L"Free42 State (*.f42)\0*.f42\0All Files (*.*)\0*.*\0\0",
                    L"f42",
                    buf,
                    FILENAMELEN))
        return;
    ci_string copyPath = buf;
    const wchar_t *copyPathC = copyPath.c_str();
    if (selectedStateName == state.coreName) {
        char *sfn = wide2utf(copyPathC);
        core_save_state(sfn);
        free(sfn);
    } else {
        ci_string origPath = ci_string(free42dirname) + L"\\" + selectedStateName + L".f42";
        if (!copyState(origPath.c_str(), copyPathC))
            MessageBox(hDlg, "State export failed.", "Message", MB_ICONWARNING);
    }
}

// Message handler for States dialog.
LRESULT CALLBACK StatesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            if (moreMenu == NULL)
                moreMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_STATES_MORE));

            // Make sure a file exists for the current state. This isn't necessarily
            // the case, specifically, right after starting up with a version <= 25
            // state file.
            ci_string currentStateName = ci_string(free42dirname) + L"/" + state.coreName + L".f42";
            const wchar_t *currentStateNameC = currentStateName.c_str();
            if (GetFileAttributesW(currentStateNameC) == INVALID_FILE_ATTRIBUTES) {
                FILE *f = _wfopen(currentStateNameC, L"wb");
                fwrite(FREE42_MAGIC_STR, 1, 4, f);
                fclose(f);
            }

            ci_string txt(L"Current: ");
            txt += state.coreName;
            SetDlgItemTextW(hDlg, IDC_CURRENT, txt.c_str());
            selectedStateName = L"";
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
                    doDuplicate(hDlg);
                    return TRUE;
                }
                case IDM_MORE_RENAME: {
                    doRename(hDlg);
                    return TRUE;
                }
                case IDM_MORE_DELETE: {
                    doDelete(hDlg);
                    return TRUE;
                }
                case IDM_MORE_IMPORT: {
                    doImport(hDlg);
                    return TRUE;
                }
                case IDM_MORE_EXPORT: {
                    doExport(hDlg);
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
                        //case LBN_SELCANCEL:
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
