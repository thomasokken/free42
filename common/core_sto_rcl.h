/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2020  Thomas Okken
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


/****************************************************************/
/* Generic arithmetic operators, for use in the implementations */
/* of +, -, *, /, STO+, STO-, etc...                            */
/****************************************************************/

int generic_div(const vartype *x, const vartype *y,
                            void (*completion)(int, vartype *));
int generic_mul(const vartype *x, const vartype *y,
                            void (*completion)(int, vartype *));
int generic_sub(const vartype *x, const vartype *y, vartype **res);
int generic_add(const vartype *x, const vartype *y, vartype **res);
int generic_rcl(arg_struct *arg, vartype **dst);
int generic_sto(arg_struct *arg, char operation);


/**********************************************/
/* Mappers to apply unary or binary operators */
/* to arbitrary parameter types               */
/**********************************************/

int map_unary(const vartype *src, vartype **dst, mappable_r, mappable_c mc);
int map_binary(const vartype *src1, const vartype *src2, vartype **dst,
            mappable_rr mrr, mappable_rc mrc, mappable_cr mcr, mappable_cc mcc);

/**************************************************************/
/* Operators that can be used by the mapping functions, above */
/**************************************************************/

int div_rr(phloat x, phloat y, phloat *z);
int div_rc(phloat x, phloat yre, phloat yim, phloat *zre, phloat *zim);
int div_cr(phloat xre, phloat xim, phloat y, phloat *zre, phloat *zim);
int div_cc(phloat xre, phloat xim, phloat yre, phloat yim,
                                    phloat *zre, phloat *zim);

int mul_rr(phloat x, phloat y, phloat *z);
int mul_rc(phloat x, phloat yre, phloat yim, phloat *zre, phloat *zim);
int mul_cr(phloat xre, phloat xim, phloat y, phloat *zre, phloat *zim);
int mul_cc(phloat xre, phloat xim, phloat yre, phloat yim,
                                    phloat *zre, phloat *zim);

int sub_rr(phloat x, phloat y, phloat *z);
int sub_rc(phloat x, phloat yre, phloat yim, phloat *zre, phloat *zim);
int sub_cr(phloat xre, phloat xim, phloat y, phloat *zre, phloat *zim);
int sub_cc(phloat xre, phloat xim, phloat yre, phloat yim,
                                    phloat *zre, phloat *zim);

int add_rr(phloat x, phloat y, phloat *z);
int add_rc(phloat x, phloat yre, phloat yim, phloat *zre, phloat *zim);
int add_cr(phloat xre, phloat xim, phloat y, phloat *zre, phloat *zim);
int add_cc(phloat xre, phloat xim, phloat yre, phloat yim,
                                    phloat *zre, phloat *zim);

#endif
