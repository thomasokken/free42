/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2015  Thomas Okken
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

#ifndef EBML_DTD_H
#define EBML_DTD_H

// Element IDs used in the Free42 state file.

#define STATE_ID_EBML                   0x0a45dfa3
 #define STATE_ID_EBMLVersion           0x0286
 #define STATE_ID_EBMLReadVersion       0x02f7
 #define STATE_ID_EBMLMaxIDLength       0x02f2
 #define STATE_ID_EBMLMaxSizeLength     0x02f3
 #define STATE_ID_DocType               0x0282
 #define STATE_ID_DocTypeVersion        0x0287
 #define STATE_ID_DocTypeReadVersion    0x0285

#define STATE_ID_PLATFORM               0x01
#define STATE_ID_SHELL_STATE_VERSION    0x02
#define STATE_ID_SHELL_STATE            0x03

#define STATE_ID_LAYOUT                 0x04
 #define STATE_ID_LAYOUT_BIGENDIAN      0x05
 #define STATE_ID_LAYOUT_DECIMAL        0x06
 #define STATE_ID_LAYOUT_VARTYPE        0x07
 #define STATE_ID_LAYOUT_ARG            0x08
 #define STATE_ID_LAYOUT_VAR_STRUCT     0x09
 #define STATE_ID_LAYOUT_PRGM_STRUCT    0x10

#endif
