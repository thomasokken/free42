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

#include <sstream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include "ebmlwriter.h"
#include "free42.h"

/* public */
ebmlwriter::ebmlwriter(const char *filename) {
    os = new ofstream(filename);
}

/* public */
ebmlwriter::~ebmlwriter() {
    while (!os_stack.empty()) {
        ostream *os2 = os_stack.top();
        delete os2;
        os_stack.pop();
    }
    delete os;
}

/* public */
void ebmlwriter::start_element(uint4 id) {
    id_stack.push(id);
    os_stack.push(os);
    os = new ostringstream;
}

/* public */
void ebmlwriter::write_vint(uint8 n) {
    uint8 lim = 0x80;
    int len = 1;
    while (true) {
        if (n < lim - 1)
            break;
        lim <<= 7;
        len++;
        if (len == 9) {
            fprintf(stderr, "ebmlwriter::write_vint(): arg %lu too large\n", (unsigned long) n);
            write_unknown();
            return;
        }
    }
    n |= lim;
    len <<= 3;
    while ((len -= 8) >= 0)
        *os << (char) (n >> len);
}

/* public */
void ebmlwriter::write_vsint(int8 n) {
    int8 lim = 0x40;
    int len = 1;
    while (true) {
        if (n < lim && n > -lim)
            break;
        lim <<= 7;
        len++;
        if (len == 9) {
            fprintf(stderr, "ebmlwriter::write_vsint(): arg %ld too large\n", (long) n);
            write_unknown();
            return;
        }
    }
    n = (n + lim - 1) | (lim << 1);
    len <<= 3;
    while ((len -= 8) >= 0)
        *os << (char) (n >> len);
}

/* public */
void ebmlwriter::write_unknown() {
    *os << (char) 0xff;
}

/* public */
void ebmlwriter::write_data(int length, const char *buf) {
    os->write(buf, length);
}

static int vint_size(uint8 n) {
    uint8 lim = 0x80;
    int len = 1;
    while (len < 9) {
        if (n < lim - 1)
            return len;
        lim <<= 7;
        len++;
    }
    return 9;
}

/* public */
void ebmlwriter::end_element() {
    uint4 id = id_stack.top();
    id_stack.pop();
    ostringstream *oss = static_cast<ostringstream *>(os);
    os = os_stack.top();
    os_stack.pop();
    string s = oss->str();
    int size = s.size();
    write_vint(id);
    write_vint(size);
    write_data(size, s.data());
    os->flush();
    delete oss;
}

/* public */
void ebmlwriter::write_int_element(uint4 id, uint8 n) {
    char buf[8];
    int len = 0;
    bool nz = false;
    for (int i = 56; i >= 0; i -= 8) {
        char c = (char) (n >> i);
        if (c != 0)
            nz = true;
        if (nz)
            buf[len++] = c;
    }
    if (len == 0)
        buf[len++] = 0;
    write_vint(id);
    write_vint(len);
    os->write(buf, len);
}

static bool is_little_endian() {
    union {
        uint4 i;
        char c[4];
    } u;
    u.i = 0x01020304;
    return u.c[0] == 4; 
}

/* public */
void ebmlwriter::write_float_element(uint4 id, double f) {
    write_vint(id);
    float ff = f;
    double fff = ff;
    if (f == fff) {
        union {
            float f;
            char buf[4];
        } u;
        u.f = ff;
        write_vint(4);
        if (is_little_endian())
            for (int i = 0; i < 2; i++) {
                char t = u.buf[i];
                u.buf[i] = u.buf[3 - i];
                u.buf[3 - i] = t;
            }
        os->write(u.buf, 4);
    } else {
        union {
            double d;
            char buf[8];
        } u;
        u.d = f;
        write_vint(8);
        if (is_little_endian())
            for (int i = 0; i < 4; i++) {
                char t = u.buf[i];
                u.buf[i] = u.buf[7 - i];
                u.buf[7 - i] = t;
            }
        os->write(u.buf, 8);
    }
}

/* public */
void ebmlwriter::write_string_element(uint4 id, const char *s) {
    int len = strlen(s);
    write_vint(id);
    write_vint(len);
    os->write(s, len);
}

/* public */
void ebmlwriter::write_data_element(uint4 id, int length, const char *buf) {
    write_vint(id);
    write_vint(length);
    os->write(buf, length);
}

/* public */
void ebmlwriter::start_element_with_unknown_length(uint4 id) {
    if (!os_stack.empty()) {
        fprintf(stderr, "ebmlwriter::start_element_with_unknown_length(): not at top level: stacksize = %u\n", (unsigned int) os_stack.size());
        return;
    }
    write_vint(id);
    write_unknown();
}
