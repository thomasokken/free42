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

#ifndef SHELL_MAIN_H
#define SHELL_MAIN_H 1

#include "stdafx.h"
#include "resource.h"

#define FILENAMELEN 256

typedef struct state {
    BOOL extras;
    WINDOWPLACEMENT mainPlacement;
    int mainPlacementValid;
    WINDOWPLACEMENT printOutPlacement;
    int printOutPlacementValid;
    int printOutOpen;
    int printerToTxtFile;
    int printerToGifFile;
    char printerTxtFileName[FILENAMELEN];
    char printerGifFileName[FILENAMELEN];
    int printerGifMaxLength;
    char skinName[FILENAMELEN];
    BOOL alwaysOnTop;
    BOOL singleInstance;
    BOOL calculatorKey;
    char coreFileName[FILENAMELEN];
} state_type;

extern state_type state;

extern HINSTANCE hInst;                                    // current instance
extern char free42dirname[FILENAMELEN];

int browse_file(HWND owner, char *title, int save, char *filter, char *defExt, char *buf, int buflen);

#endif
