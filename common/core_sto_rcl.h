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

#ifndef CORE_STO_RCL_H
#define CORE_STO_RCL_H 1


#include "free42.h"
#include "core_phloat.h"
#include "core_globals.h"


/************************************************/
/* Signatures for functions mapped by map_unary */
/************************************************/

typedef int (*mappable_r)(phloat x, phloat *z);
typedef int (*mappable_c)(phloat xre, phloat xim, phloat *zre, phloat *zim);


/*************************************************/
/* Signatures for functions mapped by map_binary */
/*************************************************/

typedef int (*mappable_rr)(phloat x, phloat y, phloat *z);
typedef int (*mappable_rc)(phloat x, phloat yre, phloat yim,
						phloat *zre, phloat *zim);
typedef int (*mappable_cr)(phloat xre, phloat xim, phloat y,
						phloat *zre, phloat *zim);
typedef int (*mappable_cc)(phloat xre, phloat xim, phloat yre, phloat yim,
						phloat *zre, phloat *zim);


/*********************/
/* Utility functions */
/*********************/

int apply_sto_operation(char operation, vartype *x, vartype *oldval,
			void (*completion)(int, vartype *)) STO_RCL_SECT;


/****************************************************************/
/* Generic arithmetic operators, for use in the implementations */
/* of +, -, *, /, STO+, STO-, etc...                            */
/****************************************************************/

int generic_div(const vartype *x, const vartype *y,
			    void (*completion)(int, vartype *)) STO_RCL_SECT;
int generic_mul(const vartype *x, const vartype *y,
			    void (*completion)(int, vartype *)) STO_RCL_SECT;
int generic_sub(const vartype *x, const vartype *y, vartype **res) STO_RCL_SECT;
int generic_add(const vartype *x, const vartype *y, vartype **res) STO_RCL_SECT;
int generic_rcl(arg_struct *arg, vartype **dst) STO_RCL_SECT;
int generic_sto(arg_struct *arg, char operation) STO_RCL_SECT;


/**********************************************/
/* Mappers to apply unary or binary operators */
/* to arbitrary parameter types               */
/**********************************************/

int map_unary(const vartype *src, vartype **dst, mappable_r, mappable_c mc)
							    STO_RCL_SECT;
int map_binary(const vartype *src1, const vartype *src2, vartype **dst,
	    mappable_rr mrr, mappable_rc mrc, mappable_cr mcr, mappable_cc mcc)
							    STO_RCL_SECT;

/**************************************************************/
/* Operators that can be used by the mapping functions, above */
/**************************************************************/

int div_rr(phloat x, phloat y, phloat *z) STO_RCL_SECT;
int div_rc(phloat x, phloat yre, phloat yim, phloat *zre, phloat *zim)
								STO_RCL_SECT;
int div_cr(phloat xre, phloat xim, phloat y, phloat *zre, phloat *zim)
								STO_RCL_SECT;
int div_cc(phloat xre, phloat xim, phloat yre, phloat yim,
				    phloat *zre, phloat *zim) STO_RCL_SECT;

int mul_rr(phloat x, phloat y, phloat *z) STO_RCL_SECT;
int mul_rc(phloat x, phloat yre, phloat yim, phloat *zre, phloat *zim)
								STO_RCL_SECT;
int mul_cr(phloat xre, phloat xim, phloat y, phloat *zre, phloat *zim)
								STO_RCL_SECT;
int mul_cc(phloat xre, phloat xim, phloat yre, phloat yim,
				    phloat *zre, phloat *zim) STO_RCL_SECT;

int sub_rr(phloat x, phloat y, phloat *z) STO_RCL_SECT;
int sub_rc(phloat x, phloat yre, phloat yim, phloat *zre, phloat *zim)
								STO_RCL_SECT;
int sub_cr(phloat xre, phloat xim, phloat y, phloat *zre, phloat *zim)
								STO_RCL_SECT;
int sub_cc(phloat xre, phloat xim, phloat yre, phloat yim,
				    phloat *zre, phloat *zim) STO_RCL_SECT;

int add_rr(phloat x, phloat y, phloat *z) STO_RCL_SECT;
int add_rc(phloat x, phloat yre, phloat yim, phloat *zre, phloat *zim)
								STO_RCL_SECT;
int add_cr(phloat xre, phloat xim, phloat y, phloat *zre, phloat *zim)
								STO_RCL_SECT;
int add_cc(phloat xre, phloat xim, phloat yre, phloat yim,
				    phloat *zre, phloat *zim) STO_RCL_SECT;

#endif
