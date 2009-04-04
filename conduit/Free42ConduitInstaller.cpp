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

#include <windows.h>
#include <stdio.h>
#include <CondMgr.h>
#include <HSAPI.h>

#include "resource_installer.h"


static void install(HWND hDlg) {
	char msg[1000];

	char srcpath[MAX_PATH];
	GetModuleFileName(0, srcpath, MAX_PATH - 1);
	char *lastbackslash = strrchr(srcpath, '\\');
	if (lastbackslash != 0)
		*(lastbackslash + 1) = 0;
	else
		srcpath[0] = 0;
	strcat(srcpath, "Free42Conduit.dll");

	char dstpath[MAX_PATH];
	int dstpathlen = MAX_PATH;
	int err = CmGetCorePath(dstpath, &dstpathlen);
	if (err != 0) {
		sprintf(msg, "Could not determine Palm directory (%d).", err);
		MessageBox(hDlg, msg, "Error", MB_ICONERROR);
		return;
	}
	if (dstpath[dstpathlen] != '\\')
		strncat(dstpath, "\\", MAX_PATH - 1);
	strncat(dstpath, "Free42Conduit.dll", MAX_PATH - 1);
	dstpath[MAX_PATH - 1] = 0;
	if (!CopyFile(srcpath, dstpath, TRUE)) {
		DWORD res = GetLastError();
		if (res == ERROR_FILE_EXISTS)
			MessageBox(hDlg, "Free42 Conduit was already installed.", "Message", MB_ICONASTERISK);
		else {
			sprintf(msg, "Could not copy Free42Conduit.dll to %s (%d).", dstpath, res);
			MessageBox(hDlg, msg, "Error", MB_ICONERROR);
		}
		return;
	}
	err = CmInstallCreator("Fk42", CONDUIT_APPLICATION);
	if (err != 0) {
		DeleteFile(dstpath);
		if (err == ERR_CREATORID_ALREADY_IN_USE)
			MessageBox(hDlg, "Free42 Conduit was already installed.", "Message", MB_ICONASTERISK);
		else {
			sprintf(msg, "Could not register conduit (%d).", err);
			MessageBox(hDlg, msg, "Error", MB_ICONERROR);
		}
		return;
	}
	err = CmSetCreatorName("Fk42", "Free42Conduit.dll");
	if (err != 0) {
		CmRemoveConduitByCreatorID("Fk42");
		DeleteFile(dstpath);
		sprintf(msg, "Could not register conduit (%d).", err);
		MessageBox(hDlg, msg, "Error", MB_ICONERROR);
		return;
	}
	CmSetCreatorPriority("Fk42", 2);
		
	HsRefreshConduitInfo();
	MessageBox(hDlg, "Free42 Conduit installed successfully.", "Message", MB_ICONASTERISK);
}

static void uninstall(HWND hDlg) {
	char path[MAX_PATH];
	char msg[1000];
	int pathlen = MAX_PATH;
	int err = CmGetCorePath(path, &pathlen);
	if (err != 0) {
		sprintf(msg, "Could not determine Palm directory (%d).", err);
		MessageBox(hDlg, msg, "Error", MB_ICONERROR);
		return;
	}
	if (path[pathlen] != '\\')
		strncat(path, "\\", MAX_PATH - 1);
	strncat(path, "Free42Conduit.dll", MAX_PATH - 1);
	path[MAX_PATH - 1] = 0;
	err = CmRemoveConduitByCreatorID("Fk42");
	if (err == ERR_NO_CONDUIT) {
		MessageBox(hDlg, "Free42 Conduit was not installed.", "Message", MB_ICONASTERISK);
		DeleteFile(path);
		return;
	}
	if (err < 0) {
		sprintf(msg, "Could not unregister conduit (%d).", err);
		MessageBox(hDlg, msg, "Error", MB_ICONERROR);
		return;
	}
	DeleteFile(path);

	// TODO: remove HKEY_CURRENT_USER\Software\Thomas Okken Software\Free42\LocalDir
	// from the registry; also remove Free42 and Thomas Okken Software if they're
	// now empty.

	HsRefreshConduitInfo();
	MessageBox(hDlg, "Free42 Conduit uninstalled successfully.", "Message", MB_ICONASTERISK);
}

static LRESULT CALLBACK MainDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {

		case WM_INITDIALOG: {
			HWND ctl = GetDlgItem(hDlg, IDC_INSTALL);
			SendMessage(ctl, BM_SETCHECK, 1, 0);
			return TRUE;
		}

		case WM_COMMAND: {
			int cmd = LOWORD(wParam);
			switch (cmd) {
				case IDOK: {
					HWND ok = GetDlgItem(hDlg, IDOK);
					HWND cancel = GetDlgItem(hDlg, IDCANCEL);
					EnableWindow(ok, FALSE);
					EnableWindow(cancel, FALSE);
					HCURSOR hourglass = LoadCursor(NULL, (LPCTSTR) IDC_WAIT);
					HCURSOR prevcursor = SetCursor(hourglass);
					HWND ctl = GetDlgItem(hDlg, IDC_INSTALL);
					if (SendMessage(ctl, BM_GETCHECK, 0, 0))
						install(hDlg);
					else
						uninstall(hDlg);
					SetCursor(prevcursor);
					EnableWindow(ok, TRUE);
					EnableWindow(cancel, TRUE);
					return TRUE;
				}
				case IDCANCEL: {
					EndDialog(hDlg, cmd);
					return TRUE;
				}
				default:
					return FALSE;
			}
		}

		default:
			return FALSE;
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	DialogBox(hInstance, (LPCTSTR)IDD_MAIN_DIALOG, NULL, (DLGPROC)MainDialog);
	return 0;
}
