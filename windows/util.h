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

#ifndef UTIL_H
#define UTIL_H 1

#include <string>
#include "stdafx.h"

struct ci_char_traits : public std::char_traits<char> {
    static bool eq(char c1, char c2) { return toupper(c1) == toupper(c2); }
    static bool ne(char c1, char c2) { return toupper(c1) != toupper(c2); }
    static bool lt(char c1, char c2) { return toupper(c1) <  toupper(c2); }
    static int compare(const char* s1, const char* s2, size_t n) {
        while( n-- != 0 ) {
            if( toupper(*s1) < toupper(*s2) ) return -1;
            if( toupper(*s1) > toupper(*s2) ) return 1;
            ++s1; ++s2;
        }
        return 0;
    }
    static const char* find(const char* s, int n, char a) {
        while( n-- > 0 && toupper(*s) != toupper(a) ) {
            ++s;
        }
        return s;
    }
};

typedef std::basic_string<char, ci_char_traits> ci_string;

const char *msg2string(unsigned int msg);

ci_string GetDlgItemTextLong(HWND hWnd, int item);
ci_string browse_file(HWND owner, char *title, int save, char *filter, char *defExt, ci_string fname);
ci_string to_ci_string(int i);

#endif
