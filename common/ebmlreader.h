/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2014  Thomas Okken
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
#include <istream>
#include <stack>
using namespace std;

class ebmlreader {
    private:
        istream *is;
        stack<uint32_t> size_stack;
        bool read_vint(uint32_t *n);

    public:
        ebmlreader(istream *is) {
            this->is = is;
        }
        bool get_element(uint32_t *id, uint32_t *size);
        bool skip_body(uint32_t size);
        bool get_int_body(uint32_t size, uint64_t *n);
        bool get_float_body(uint32_t size, double *f);
        bool get_string_body(uint32_t size, char *buf);
        bool get_data_body(uint32_t size, char *buf);
};

#endif
