/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2024  Thomas Okken
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

#ifndef CORE_VARIABLES_H
#define CORE_VARIABLES_H 1

#include "core_phloat.h"

/***********************/
/* Variable data types */
/***********************/

#define TYPE_NULL 0
#define TYPE_REAL 1
#define TYPE_COMPLEX 2
#define TYPE_REALMATRIX 3
#define TYPE_COMPLEXMATRIX 4
#define TYPE_STRING 5
#define TYPE_LIST 6

struct vartype {
    int type;
};


struct vartype_real {
    int type;
    phloat x;
};


struct vartype_complex {
    int type;
    phloat re, im;
};


struct realmatrix_data {
    int refcount;
    phloat *data;
    char *is_string;
};

struct vartype_realmatrix {
    int type;
    int4 rows;
    int4 columns;
    realmatrix_data *array;
};


struct complexmatrix_data {
    int refcount;
    phloat *data;
};

struct vartype_complexmatrix {
    int type;
    int4 rows;
    int4 columns;
    complexmatrix_data *array;
};


/* Maximum short string length in a stand-alone variable */
#define SSLENV ((int) (sizeof(char *) < 8 ? 8 : sizeof(char *)))
/* Maximum short string length in a matrix element */
#define SSLENM ((int) sizeof(phloat) - 1)

struct vartype_string {
    int type;
    int4 length;
    /* When length <= SSLENV, use buf; otherwise, use ptr */
    union {
        char buf[SSLENV];
        char *ptr;
    } t;
    char *txt() {
        return length > SSLENV ? t.ptr : t.buf;
    }
    const char *txt() const {
        return length > SSLENV ? t.ptr : t.buf;
    }
    void trim1();
};


struct list_data {
    int refcount;
    vartype **data;
};

struct vartype_list {
    int type;
    int4 size;
    list_data *array;
};


vartype *new_real(phloat value);
vartype *new_complex(phloat re, phloat im);
vartype *new_string(const char *s, int slen);
vartype *new_realmatrix(int4 rows, int4 columns);
vartype *new_complexmatrix(int4 rows, int4 columns);
vartype *new_list(int4 size);
void free_vartype(vartype *v);
void clean_vartype_pools();
void free_long_strings(char *is_string, phloat *data, int4 n);
void get_matrix_string(vartype_realmatrix *rm, int4 i, char **text, int4 *length);
void get_matrix_string(const vartype_realmatrix *rm, int4 i, const char **text, int4 *length);
bool put_matrix_string(vartype_realmatrix *rm, int4 i, const char *text, int4 length);
vartype *dup_vartype(const vartype *v);
int disentangle(vartype *v);
int lookup_var(const char *name, int namelength);
vartype *recall_var(const char *name, int namelength);
bool ensure_var_space(int n);
int store_var(const char *name, int namelength, vartype *value, bool local = false);
bool purge_var(const char *name, int namelength, bool global = true, bool local = true);
void purge_all_vars();
bool vars_exist(int section);
bool contains_strings(const vartype_realmatrix *rm);
int matrix_copy(vartype *dst, const vartype *src);
vartype *recall_private_var(const char *name, int namelength);
vartype *recall_and_purge_private_var(const char *name, int namelength);
int store_private_var(const char *name, int namelength, vartype *value);

#endif
