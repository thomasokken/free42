/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
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

#ifndef CORE_HELPERS_H
#define CORE_HELPERS_H 1


#include "free42.h"
#include "core_phloat.h"
#include "core_globals.h"


/*********************/
/* Utility functions */
/*********************/

int resolve_ind_arg(arg_struct *arg) HELPERS_SECT;
int arg_to_num(arg_struct *arg, int4 *num) HELPERS_SECT;
int is_pure_real(const vartype *matrix) HELPERS_SECT;
void recall_result(vartype *v) HELPERS_SECT;
void recall_two_results(vartype *x, vartype *y) HELPERS_SECT;
void unary_result(vartype *x) HELPERS_SECT;
void binary_result(vartype *x) HELPERS_SECT;
phloat rad_to_angle(phloat x) HELPERS_SECT;
phloat rad_to_deg(phloat x) HELPERS_SECT;
phloat deg_to_rad(phloat x) HELPERS_SECT;
void append_alpha_char(char c) HELPERS_SECT;
void append_alpha_string(const char *buf, int buflen, int reverse) HELPERS_SECT;

void string_copy(char *dst, int *dstlen, const char *src, int srclen)
								HELPERS_SECT;
int string_equals(const char *s1, int s1len, const char *s2, int s2len)
								HELPERS_SECT;

#define FLAGOP_SF 0
#define FLAGOP_CF 1
#define FLAGOP_FS_T 2
#define FLAGOP_FC_T 3
#define FLAGOP_FSC_T 4
#define FLAGOP_FCC_T 5

int virtual_flag_handler(int flagop, int flagnum) HELPERS_SECT;

int get_base() HELPERS_SECT;
void set_base(int base) HELPERS_SECT;
int get_base_param(const vartype *v, int8 *n) HELPERS_SECT;
int base_range_check(int8 *n) HELPERS_SECT;

void print_text(const char *text, int length, int left_justified) HELPERS_SECT;
void print_lines(const char *text, int length, int left_justified) HELPERS_SECT;
void print_right(const char *left, int leftlen,
		 const char *right, int rightlen) HELPERS_SECT;
void print_wide(const char *left, int leftlen,
		const char *right, int rightlen) HELPERS_SECT;
void print_command(int cmd, const arg_struct *arg) HELPERS_SECT;

void generic_r2p(phloat re, phloat im, phloat *r, phloat *phi) HELPERS_SECT;
void generic_p2r(phloat r, phloat phi, phloat *re, phloat *im) HELPERS_SECT;

/***********************/
/* Miscellaneous stuff */
/***********************/

int dimension_array(const char *name, int namelen, int4 rows, int4 columns)
								  HELPERS_SECT;
int dimension_array_ref(vartype *matrix, int4 rows, int4 columns) HELPERS_SECT;

phloat fix_hms(phloat x) HELPERS_SECT;

void char2buf(char *buf, int buflen, int *bufptr, char c) HELPERS_SECT;
void string2buf(char *buf, int buflen, int *bufptr, const char *s, int slen)
							    HELPERS_SECT;
int int2string(int4 n, char *buf, int buflen) HELPERS_SECT;
int vartype2string(const vartype *v, char *buf, int buflen) HELPERS_SECT;
char *phloat2program(phloat d) HELPERS_SECT;
int easy_phloat2string(phloat d, char *buf, int buflen, int base_mode)
							    HELPERS_SECT;
int ip2revstring(phloat d, char *buf, int buflen) HELPERS_SECT;


#endif
