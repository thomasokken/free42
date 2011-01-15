/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2011  Thomas Okken
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

#ifndef CORE_ARM_H
#define CORE_ARM_H 1

#include "free42.h"

struct arg_core_init {
    int2 read_state;
    int2 dummy;
    int4 version;
};

struct arg_core_keydown {
    int2 key;
    int2 enqueued;
    int2 repeat;
};

struct arg_core_list_programs {
    char *buf;
    int2 bufsize;
};

struct arg_core_export_programs {
    int2 count;
    int2 dummy;
    const int4 *indexes;
};

struct arg_core_copy {
    char *buf;
    int2 buflen;
};

struct arg_shell_blitter {
    const char *bits;
    int2 bytesperline;
    int2 x;
    int2 y;
    int2 width;
    int2 height;
};

struct arg_shell_beeper {
    int2 frequency;
    int2 duration;
};

struct arg_shell_annunciators {
    int2 updn;
    int2 shf;
    int2 prt;
    int2 run;
    int2 g;
    int2 rad;
};

struct arg_shell_read_saved_state {
    void *buf;
    int4 bufsize;
};

struct arg_shell_write_saved_state {
    const void *buf;
    int4 nbytes;
};

struct arg_shell_get_time_date {
    uint4 time;
    uint4 date;
    int2 weekday;
};

struct arg_shell_print {
    const char *text;
    int2 length;
    int2 dummy;
    const char *bits;
    int2 bytesperline;
    int2 x;
    int2 y;
    int2 width;
    int2 height;
};

struct arg_shell_write {
    const char *buf;
    int4 buflen;
};

struct arg_shell_read {
    char *buf;
    int4 buflen;
};

struct arg_shell_put_bcd_table {
    void *bcdtab;
    uint4 size;
};

struct arg_realloc {
    void *ptr;
    size_t size;
};

#endif
