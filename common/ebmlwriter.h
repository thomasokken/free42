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

#ifndef EBMLREADER_H
#define EBMLREADER_H

#include <stdint.h>
#include <stack>
using namespace std;

class ostream;

class ebmlwriter {
    private:
        ostream *os;
        stack<ostream *> os_stack;
        stack<uint4> id_stack;

    public:
        ebmlwriter(const char *filename);
        ~ebmlwriter();
        void start_element(uint4 id);
        void write_vint(uint8 n);
        void write_vsint(int8 n);
        void write_unknown();
        void write_data(int length, const char *buf);
        void end_element();

        void write_int_element(uint4 id, uint8 n);
        void write_float_element(uint4 id, double f);
        void write_string_element(uint4 id, const char *s);
        void write_data_element(uint4 id, int length, const char *buf);

        void start_element_with_unknown_length(uint4 id);
};

#endif
