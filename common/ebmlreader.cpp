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

#include <sstream>
#include <stdio.h>
#include <string.h>
#include "ebmlreader.h"

using namespace std;

/* private */
bool ebmlreader::read_vint(uint32_t *n) {
    char sc;
    is->read(&sc, 1);
    if (!is->good() || sc == 0)
        return false;
    unsigned char c = sc;
    unsigned char mask = 0x80;
    int size = 0;
    while ((c & mask) == 0) {
        size += 8;
        mask >>= 1;
    }
    c -= mask;
    bool unknown = c == mask - 1;
    uint32_t nn = ((uint32_t) c) << size;
    while ((size -= 8) >= 0) {
        is->read(&sc, 1);
        if (!is->good())
            return false;
        c = sc;
        nn |= ((uint32_t) c) << size;
        if (c != 0xff)
            unknown = false;
    }
    *n = unknown ? (uint32_t) -1 : nn;
    return true;
}

/* public */
bool ebmlreader::get_element(uint32_t *id, uint32_t *size) {
    return read_vint(id) && read_vint(size);
}

/* public */
bool ebmlreader::skip_body(uint32_t size) {
    is->ignore(size);
    return is->good();
}

/* public */
bool ebmlreader::get_int_body(uint32_t size, uint64_t *n) {
    if (size > 8)
        return false;
    uint64_t nn = 0;
    for (uint32_t i = 0; i < size; i++) {
        char sc;
        is->read(&sc, 1);
        if (!is->good())
            return false;
        unsigned char c = sc;
        nn = (nn << 8) | c;
    }
    *n = nn;
    return true;
}

/* public */
bool ebmlreader::get_float_body(uint32_t size, double *f) {
    if (size == 4) {
        uint32_t b = 0;
        for (int i = 0; i < 4; i++) {
            char sc;
            is->read(&sc, 1);
            if (!is->good())
                return false;
            unsigned char c = sc;
            b = (b << 8) | c;
        }
        *f = *(float *) &b;
        return true;
    } else if (size == 8) {
        uint64_t b = 0;
        for (int i = 0; i < 8; i++) {
            char sc;
            is->read(&sc, 1);
            if (!is->good())
                return false;
            unsigned char c = sc;
            b = (b << 8) | c;
        }
        *f = *(double *) &b;
        return true;
    } else {
        return false;
    }
}

/* public */
bool ebmlreader::get_string_body(uint32_t size, char *buf) {
    is->read(buf, size);
    buf[size] = 0;
    return is->good();
}

/* public */
bool ebmlreader::get_data_body(uint32_t size, char *buf) {
    is->read(buf, size);
    return is->good();
}
