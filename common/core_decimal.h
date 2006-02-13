/*****************************************************************************
 * Free42 -- a free HP-42S calculator clone
 * Copyright (C) 2004-2006  Thomas Okken
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *****************************************************************************/

#ifndef CORE_DECIMAL_H
#define CORE_DECIMAL_H 1


#include "free42.h"
#include "core_phloat.h"
#include "core_globals.h"

extern phloat POS_HUGE_PHLOAT;
extern phloat NEG_HUGE_PHLOAT;
extern phloat POS_TINY_PHLOAT;
extern phloat NEG_TINY_PHLOAT;

void make_bcd_table() DECIMAL_SECT;
void release_bcd_table() DECIMAL_SECT;
void char2buf(char *buf, int buflen, int *bufptr, char c) DECIMAL_SECT;
void string2buf(char *buf, int buflen, int *bufptr, const char *s, int slen)
							    DECIMAL_SECT;
int int2string(int4 n, char *buf, int buflen) DECIMAL_SECT;
int phloat2string(phloat d, char *buf, int buflen,
		  int base_mode, int digits, int dispmode,
		  int thousandssep) DECIMAL_SECT;
int easy_phloat2string(phloat d, char *buf, int buflen, int base_mode)
							    DECIMAL_SECT;
int vartype2string(const vartype *v, char *buf, int buflen) DECIMAL_SECT;
int string2phloat(const char *buf, int buflen, phloat *d) DECIMAL_SECT;
char *phloat2program(phloat d) DECIMAL_SECT;

void set_decimal_mode(int decimal) DECIMAL_SECT;


#endif
