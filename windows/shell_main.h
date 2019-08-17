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
#include "free42.h"
#include "util.h"

struct state_type {
    bool extras;
    WINDOWPLACEMENT mainPlacement;
    bool mainPlacementValid;
    WINDOWPLACEMENT printOutPlacement;
    bool printOutPlacementValid;
    bool printOutOpen;
    bool printerToTxtFile;
    bool printerToGifFile;
    ci_string printerTxtFileName;
    ci_string printerGifFileName;
    int4 printerGifMaxLength;
    ci_string skinName;
    bool alwaysOnTop;
    bool singleInstance;
    bool calculatorKey;
    ci_string coreName;
};

extern state_type state;

extern HINSTANCE hInst;                                    // current instance
extern ci_string free42dirname;

#endif
