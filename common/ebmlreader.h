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
#include <vector>
using namespace std;

class ifstream;

class ebmlreader {
    private:
        ifstream *is;
        vector<uint4> size_stack;
        uint4 current_size;
        bool is_good;
        bool read_vint(uint4 *n, uint4 *nbytes);
        bool ebmlreader::bytes_read(uint4 size);

    public:
        ebmlreader(const char *filename);
        ~ebmlreader();
        bool good();
        void set_bad();
        int depth();
        bool get_element(uint4 *id, uint4 *size);
        bool skip_body(uint4 size);
        bool get_int_body(uint4 size, uint8 *n);
        bool get_float_body(uint4 size, double *f);
        bool get_string_body(uint4 size, char *buf);
        bool get_data_body(uint4 size, char *buf);
};

#endif
