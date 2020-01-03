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

#ifndef CORE_KEYDOWN_H
#define CORE_KEYDOWN_H 1

#include "free42.h"

void keydown(int shift, int key);
void keydown_number_entry(int shift, int key);
void keydown_command_entry(int shift, int key);
void keydown_alpha_mode(int shift, int key);
void keydown_normal_mode(int shift, int key);

#endif
