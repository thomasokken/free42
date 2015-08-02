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

#include <fstream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include "ebmlreader.h"
#include "free42.h"

using namespace std;

/* public */
ebmlreader::ebmlreader(const char *filename) {
    is = new ifstream(filename);
    is_good = is->good();
}

/* public */
ebmlreader::~ebmlreader() {
    delete is;
}

/* public */
bool ebmlreader::good() {
    is_good = is_good && is->good();
    return is_good;
}

/* public */
void set_bad() {
    is_good = false;
}

/* public */
bool ebmlreader::depth() {
    return size_stack.size();
}

/* private */
bool ebmlreader::read_vint(uint4 *n, uint4 *nbytes) {
    char sc;
    is->read(&sc, 1);
    if (!good() || sc == 0)
        return false;
    uint4 len = 1;
    unsigned char c = sc;
    unsigned char mask = 0x80;
    int size = 0;
    while ((c & mask) == 0) {
        size += 8;
        mask >>= 1;
    }
    c -= mask;
    bool unknown = c == mask - 1;
    uint4 nn = ((uint4) c) << size;
    while ((size -= 8) >= 0) {
        is->read(&sc, 1);
        if (!good())
            return false;
        len++;
        c = sc;
        nn |= ((uint4) c) << size;
        if (c != 0xff)
            unknown = false;
    }
    *n = unknown ? (uint4) -1 : nn;
    *nbytes = len;
    return true;
}

/* public */
bool ebmlreader::get_element(uint4 *id, uint4 *size) {
    uint4 id_size, size_size;
    if (!read_vint(id, &id_size) || !read_vint(size, &size_size))
        return false;
    if (size_size == (uint4) -1) {
        if (size_stack.empty())
            return true;
        else {
            // Unknown-length element not at top level -- not allowed
            is_good = false;
            return false;
        }
    } else {
        uint4 hdr_size = id_size + size_size;
        int n = size_stack.size();
        for (int i = 0; i < n; i++)
            size_stack[i] -= hdr_size;
        size_stack.push(size);
    }
}

/* private */
bool ebmlreader::bytes_read(uint4 size) {
    int n = size_stack.size() - 1;
    bool first = true;
    while (n >= 0) {
        uint4 remaining = size_stack[n] -= size;
        if (remaining < 0) {
            is_good = false;
            return false;
        }
        if (remaining == 0)
            if (!first) {
                is_good = false;
                return false;
            } else
                size_stack.pop_back();
        n--;
        first = false;
    }
    return true;
}

/* public */
bool ebmlreader::skip_body(uint4 size) {
    is->ignore(size);
    return good() && bytes_read(size);
}

/* public */
bool ebmlreader::get_int_body(uint4 size, uint8 *n) {
    if (size > 8) {
        is_good = false;
        return false;
    }
    uint8 nn = 0;
    for (uint4 i = 0; i < size; i++) {
        char sc;
        is->read(&sc, 1);
        if (!good())
            return false;
        unsigned char c = sc;
        nn = (nn << 8) | c;
    }
    *n = nn;
    return bytes_read(size);
}

/* public */
bool ebmlreader::get_float_body(uint4 size, double *f) {
    if (size == 4) {
        uint4 b = 0;
        for (int i = 0; i < 4; i++) {
            char sc;
            is->read(&sc, 1);
            if (!good())
                return false;
            unsigned char c = sc;
            b = (b << 8) | c;
        }
        *f = *(float *) &b;
        return bytes_read(size);
    } else if (size == 8) {
        uint8 b = 0;
        for (int i = 0; i < 8; i++) {
            char sc;
            is->read(&sc, 1);
            if (!good())
                return false;
            unsigned char c = sc;
            b = (b << 8) | c;
        }
        *f = *(double *) &b;
        return bytes_read(size);
    } else {
        is_good = false;
        return false;
    }
}

/* public */
bool ebmlreader::get_string_body(uint4 size, char *buf) {
    is->read(buf, size);
    buf[size] = 0;
    return good() && bytes_read(size);
}

/* public */
bool ebmlreader::get_data_body(uint4 size, char *buf) {
    is->read(buf, size);
    return good() && bytes_read(size);
}
