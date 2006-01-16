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

#ifndef CORE_HELPERS_H
#define CORE_HELPERS_H 1


#include "free42.h"
#include "core_globals.h"


/************************************************/
/* Signatures for functions mapped by map_unary */
/************************************************/

typedef int (*mappable_r)(double x, double *z);
typedef int (*mappable_c)(double xre, double xim, double *zre, double *zim);


/*************************************************/
/* Signatures for functions mapped by map_binary */
/*************************************************/

typedef int (*mappable_rr)(double x, double y, double *z);
typedef int (*mappable_rc)(double x, double yre, double yim,
						double *zre, double *zim);
typedef int (*mappable_cr)(double xre, double xim, double y,
						double *zre, double *zim);
typedef int (*mappable_cc)(double xre, double xim, double yre, double yim,
						double *zre, double *zim);


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
int apply_sto_operation(char operation, vartype *x, vartype *oldval,
			void (*completion)(int, vartype *)) HELPERS_SECT;
double rad_to_angle(double x) HELPERS_SECT;
double angle_to_rad(double x) HELPERS_SECT;
double rad_to_deg(double x) HELPERS_SECT;
double deg_to_rad(double x) HELPERS_SECT;
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


/****************************************************************/
/* Generic arithmetic operators, for use in the implementations */
/* of +, -, *, /, STO+, STO-, etc...                            */
/****************************************************************/

int generic_div(const vartype *x, const vartype *y,
			    void (*completion)(int, vartype *)) HELPERS_SECT;
int generic_mul(const vartype *x, const vartype *y,
			    void (*completion)(int, vartype *)) HELPERS_SECT;
int generic_sub(const vartype *x, const vartype *y, vartype **res) HELPERS_SECT;
int generic_add(const vartype *x, const vartype *y, vartype **res) HELPERS_SECT;
int generic_rcl(arg_struct *arg, vartype **dst) HELPERS_SECT;
int generic_sto(arg_struct *arg, char operation) HELPERS_SECT;
void generic_r2p(double re, double im, double *r, double *phi) HELPERS_SECT;
void generic_p2r(double r, double phi, double *re, double *im) HELPERS_SECT;


/**********************************************/
/* Mappers to apply unary or binary operators */
/* to arbitrary parameter types               */
/**********************************************/

int map_unary(const vartype *src, vartype **dst, mappable_r, mappable_c mc)
							    HELPERS_SECT;
int map_binary(const vartype *src1, const vartype *src2, vartype **dst,
	    mappable_rr mrr, mappable_rc mrc, mappable_cr mcr, mappable_cc mcc)
							    HELPERS_SECT;

/**************************************************************/
/* Operators that can be used by the mapping functions, above */
/**************************************************************/

int div_rr(double x, double y, double *z) HELPERS_SECT;
int div_rc(double x, double yre, double yim, double *zre, double *zim)
								HELPERS_SECT;
int div_cr(double xre, double xim, double y, double *zre, double *zim)
								HELPERS_SECT;
int div_cc(double xre, double xim, double yre, double yim,
				    double *zre, double *zim) HELPERS_SECT;

int mul_rr(double x, double y, double *z) HELPERS_SECT;
int mul_rc(double x, double yre, double yim, double *zre, double *zim)
								HELPERS_SECT;
int mul_cr(double xre, double xim, double y, double *zre, double *zim)
								HELPERS_SECT;
int mul_cc(double xre, double xim, double yre, double yim,
				    double *zre, double *zim) HELPERS_SECT;

int sub_rr(double x, double y, double *z) HELPERS_SECT;
int sub_rc(double x, double yre, double yim, double *zre, double *zim)
								HELPERS_SECT;
int sub_cr(double xre, double xim, double y, double *zre, double *zim)
								HELPERS_SECT;
int sub_cc(double xre, double xim, double yre, double yim,
				    double *zre, double *zim) HELPERS_SECT;

int add_rr(double x, double y, double *z) HELPERS_SECT;
int add_rc(double x, double yre, double yim, double *zre, double *zim)
								HELPERS_SECT;
int add_cr(double xre, double xim, double y, double *zre, double *zim)
								HELPERS_SECT;
int add_cc(double xre, double xim, double yre, double yim,
				    double *zre, double *zim) HELPERS_SECT;

/***********************/
/* Miscellaneous stuff */
/***********************/

int dimension_array(const char *name, int namelen, int4 rows, int4 columns)
								  HELPERS_SECT;
int dimension_array_ref(vartype *matrix, int4 rows, int4 columns) HELPERS_SECT;

double fix_hms(double x) HELPERS_SECT;


#endif
