/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2019  Thomas Okken
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

#include <stdlib.h>

#include "core_globals.h"
#include "core_helpers.h"
#include "core_display.h"
#include "core_variables.h"


// We cache vartype_real, vartype_complex, and vartype_string instances, to
// cut down on the malloc/free overhead. This overhead is particularly painful
// in the PalmOS ARM version, because it has to do an ARM-to-68K call for each
// malloc or free, and that's a performance killer when running programs.
// TODO: Pools may cause memory fragmentation. To fix, override malloc() and
// realloc() with versions that empty the pools and retry before returning
// NULL.

typedef struct pool_real {
    vartype_real r;
    struct pool_real *next;
} pool_real;

static pool_real *realpool = NULL;

typedef struct pool_complex {
    vartype_complex c;
    struct pool_complex *next;
} pool_complex;

static pool_complex *complexpool = NULL;

typedef struct pool_string {
    vartype_string s;
    struct pool_string *next;
} pool_string;

static pool_string *stringpool = NULL;

vartype *new_real(phloat value) {
    pool_real *r;
    if (realpool == NULL) {
        r = (pool_real *) malloc(sizeof(pool_real));
        if (r == NULL)
            return NULL;
        r->r.type = TYPE_REAL;
    } else {
        r = realpool;
        realpool = realpool->next;
    }
    r->r.x = value;
    return (vartype *) r;
}

vartype *new_complex(phloat re, phloat im) {
    pool_complex *c;
    if (complexpool == NULL) {
        c = (pool_complex *) malloc(sizeof(pool_complex));
        if (c == NULL)
            return NULL;
        c->c.type = TYPE_COMPLEX;
    } else {
        c = complexpool;
        complexpool = complexpool->next;
    }
    c->c.re = re;
    c->c.im = im;
    return (vartype *) c;
}

vartype *new_string(const char *text, int length) {
    pool_string *s;
    if (stringpool == NULL) {
        s = (pool_string *) malloc(sizeof(pool_string));
        if (s == NULL)
            return NULL;
        s->s.type = TYPE_STRING;
    } else {
        s = stringpool;
        stringpool = stringpool->next;
    }
    int i;
    s->s.type = TYPE_STRING;
    s->s.length = length > 6 ? 6 : length;
    for (i = 0; i < s->s.length; i++)
        s->s.text[i] = text[i];
    return (vartype *) s;
}

vartype *new_realmatrix(int4 rows, int4 columns) {
    double d_bytes = ((double) rows) * ((double) columns) * sizeof(phloat);
    if (((double) (int4) d_bytes) != d_bytes)
        return NULL;

    vartype_realmatrix *rm = (vartype_realmatrix *)
                                        malloc(sizeof(vartype_realmatrix));
    if (rm == NULL)
        return NULL;
    int4 i, sz;
    rm->type = TYPE_REALMATRIX;
    rm->rows = rows;
    rm->columns = columns;
    sz = rows * columns;
    rm->array = (realmatrix_data *) malloc(sizeof(realmatrix_data));
    if (rm->array == NULL) {
        free(rm);
        return NULL;
    }
    rm->array->data = (phloat *) malloc(sz * sizeof(phloat));
    if (rm->array->data == NULL) {
        /* Oops */
        free(rm->array);
        free(rm);
        return NULL;
    }
    rm->array->is_string = (char *) malloc(sz);
    if (rm->array->is_string == NULL) {
        /* Oops */
        free(rm->array->data);
        free(rm->array);
        free(rm);
        return NULL;
    }
    for (i = 0; i < sz; i++)
        rm->array->data[i] = 0;
    for (i = 0; i < sz; i++)
        rm->array->is_string[i] = 0;
    rm->array->refcount = 1;
    return (vartype *) rm;
}

vartype *new_complexmatrix(int4 rows, int4 columns) {
    double d_bytes = ((double) rows) * ((double) columns) * sizeof(phloat) * 2;
    if (((double) (int4) d_bytes) != d_bytes)
        return NULL;

    vartype_complexmatrix *cm = (vartype_complexmatrix *)
                                        malloc(sizeof(vartype_complexmatrix));
    if (cm == NULL)
        return NULL;
    int4 i, sz;
    cm->type = TYPE_COMPLEXMATRIX;
    cm->rows = rows;
    cm->columns = columns;
    sz = rows * columns * 2;
    cm->array = (complexmatrix_data *) malloc(sizeof(complexmatrix_data));
    if (cm->array == NULL) {
        free(cm);
        return NULL;
    }
    cm->array->data = (phloat *) malloc(sz * sizeof(phloat));
    if (cm->array->data == NULL) {
        /* Oops */
        free(cm->array);
        free(cm);
        return NULL;
    }
    for (i = 0; i < sz; i++)
        cm->array->data[i] = 0;
    cm->array->refcount = 1;
    return (vartype *) cm;
}

vartype *new_matrix_alias(vartype *m) {
    if (m->type == TYPE_REALMATRIX) {
        vartype_realmatrix *rm1 = (vartype_realmatrix *) m;
        vartype_realmatrix *rm2 = (vartype_realmatrix *)
                                        malloc(sizeof(vartype_realmatrix));
        if (rm2 == NULL)
            return NULL;
        *rm2 = *rm1;
        rm2->array->refcount++;
        return (vartype *) rm2;
    } else if (m->type == TYPE_COMPLEXMATRIX) {
        vartype_complexmatrix *cm1 = (vartype_complexmatrix *) m;
        vartype_complexmatrix *cm2 = (vartype_complexmatrix *)
                                        malloc(sizeof(vartype_complexmatrix));
        if (cm2 == NULL)
            return NULL;
        *cm2 = *cm1;
        cm2->array->refcount++;
        return (vartype *) cm2;
    } else
        return NULL;
}

void free_vartype(vartype *v) {
    if (v == NULL)
        return;
    switch (v->type) {
        case TYPE_REAL: {
            pool_real *r = (pool_real *) v;
            r->next = realpool;
            realpool = r;
            break;
        }
        case TYPE_COMPLEX: {
            pool_complex *c = (pool_complex *) v;
            c->next = complexpool;
            complexpool = c;
            break;
        }
        case TYPE_STRING: {
            pool_string *s = (pool_string *) v;
            s->next = stringpool;
            stringpool = s;
            break;
        }
        case TYPE_REALMATRIX: {
            vartype_realmatrix *rm = (vartype_realmatrix *) v;
            if (--(rm->array->refcount) == 0) {
                free(rm->array->data);
                free(rm->array->is_string);
                free(rm->array);
            }
            free(rm);
            break;
        }
        case TYPE_COMPLEXMATRIX: {
            vartype_complexmatrix *cm = (vartype_complexmatrix *) v;
            if (--(cm->array->refcount) == 0) {
                free(cm->array->data);
                free(cm->array);
            }
            free(cm);
            break;
        }
    }
}

void clean_vartype_pools() {
    while (realpool != NULL) {
        pool_real *r = realpool;
        realpool = r->next;
        free(r);
    }
    while (complexpool != NULL) {
        pool_complex *c = complexpool;
        complexpool = c->next;
        free(c);
    }
    while (stringpool != NULL) {
        pool_string *s = stringpool;
        stringpool = s->next;
        free(s);
    }
}

vartype *dup_vartype(const vartype *v) {
    if (v == NULL)
        return NULL;
    switch (v->type) {
        case TYPE_REAL: {
            vartype_real *r = (vartype_real *) v;
            return new_real(r->x);
        }
        case TYPE_COMPLEX: {
            vartype_complex *c = (vartype_complex *) v;
            return new_complex(c->re, c->im);
        }
        case TYPE_REALMATRIX: {
            vartype_realmatrix *rm = (vartype_realmatrix *) v;
            vartype_realmatrix *rm2 = (vartype_realmatrix *)
                                        malloc(sizeof(vartype_realmatrix));
            if (rm2 == NULL)
                return NULL;
            rm2->type = TYPE_REALMATRIX;
            rm2->rows = rm->rows;
            rm2->columns = rm->columns;
            rm2->array = rm->array;
            rm->array->refcount++;
            return (vartype *) rm2;
        }
        case TYPE_COMPLEXMATRIX: {
            vartype_complexmatrix *cm = (vartype_complexmatrix *) v;
            vartype_complexmatrix *cm2 = (vartype_complexmatrix *)
                                        malloc(sizeof(vartype_complexmatrix));
            if (cm2 == NULL)
                return NULL;
            cm2->type = TYPE_COMPLEXMATRIX;
            cm2->rows = cm->rows;
            cm2->columns = cm->columns;
            cm2->array = cm->array;
            cm->array->refcount++;
            return (vartype *) cm2;
        }
        case TYPE_STRING: {
            vartype_string *s = (vartype_string *) v;
            return new_string(s->text, s->length);
        }
        default:
            return NULL;
    }
}

int disentangle(vartype *v) {
    switch (v->type) {
        case TYPE_REALMATRIX: {
            vartype_realmatrix *rm = (vartype_realmatrix *) v;
            if (rm->array->refcount == 1)
                return 1;
            else {
                realmatrix_data *md = (realmatrix_data *)
                                        malloc(sizeof(realmatrix_data));
                if (md == NULL)
                    return 0;
                int4 sz = rm->rows * rm->columns;
                int4 i;
                md->data = (phloat *) malloc(sz * sizeof(phloat));
                if (md->data == NULL) {
                    free(md);
                    return 0;
                }
                md->is_string = (char *) malloc(sz);
                if (md->is_string == NULL) {
                    free(md->data);
                    free(md);
                    return 0;
                }
                for (i = 0; i < sz; i++)
                    md->data[i] = rm->array->data[i];
                for (i = 0; i < sz; i++)
                    md->is_string[i] = rm->array->is_string[i];
                md->refcount = 1;
                rm->array->refcount--;
                rm->array = md;
                return 1;
            }
        }
        case TYPE_COMPLEXMATRIX: {
            vartype_complexmatrix *cm = (vartype_complexmatrix *) v;
            if (cm->array->refcount == 1)
                return 1;
            else {
                complexmatrix_data *md = (complexmatrix_data *)
                                            malloc(sizeof(complexmatrix_data));
                if (md == NULL)
                    return 0;
                int4 sz = cm->rows * cm->columns * 2;
                int4 i;
                md->data = (phloat *) malloc(sz * sizeof(phloat));
                if (md->data == NULL) {
                    free(md);
                    return 0;
                }
                for (i = 0; i < sz; i++)
                    md->data[i] = cm->array->data[i];
                md->refcount = 1;
                cm->array->refcount--;
                cm->array = md;
                return 1;
            }
        }
        case TYPE_REAL:
        case TYPE_COMPLEX:
        case TYPE_STRING:
        default:
            return 1;
    }
}

int lookup_var(const char *name, int namelength) {
    int i, j;
    for (i = vars_count - 1; i >= 0; i--) {
        if (vars[i].hidden)
            continue;
        if (vars[i].length == namelength) {
            for (j = 0; j < namelength; j++)
                if (vars[i].name[j] != name[j])
                    goto nomatch;
            return i;
        }
        nomatch:;
    }
    return -1;
}

vartype *recall_var(const char *name, int namelength) {
    int varindex = lookup_var(name, namelength);
    if (varindex == -1)
        return NULL;
    else
        return vars[varindex].value;
}

bool ensure_var_space(int n) {
    int nc = vars_count + n;
    if (nc > vars_capacity) {
        var_struct *nv = (var_struct *) realloc(vars, nc * sizeof(var_struct));
        if (nv == NULL)
            return false;
        vars_capacity = nc;
        vars = nv;
    }
    return true;
}

int store_var(const char *name, int namelength, vartype *value, bool local) {
    int varindex = lookup_var(name, namelength);
    int i;
    if (varindex == -1) {
        if (vars_count == vars_capacity) {
            int nc = vars_capacity + 25;
            var_struct *nv = (var_struct *) realloc(vars, nc * sizeof(var_struct));
            if (nv == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            vars_capacity = nc;
            vars = nv;
        }
        varindex = vars_count++;
        vars[varindex].length = namelength;
        for (i = 0; i < namelength; i++)
            vars[varindex].name[i] = name[i];
        vars[varindex].level = local ? get_rtn_level() : -1;
        vars[varindex].hidden = false;
        vars[varindex].hiding = false;
    } else if (local && vars[varindex].level < get_rtn_level()) {
        if (vars_count == vars_capacity) {
            int nc = vars_capacity + 25;
            var_struct *nv = (var_struct *) realloc(vars, nc * sizeof(var_struct));
            if (nv == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            vars_capacity = nc;
            vars = nv;
        }
        vars[varindex].hidden = true;
        varindex = vars_count++;
        vars[varindex].length = namelength;
        for (i = 0; i < namelength; i++)
            vars[varindex].name[i] = name[i];
        vars[varindex].level = get_rtn_level();
        vars[varindex].hidden = false;
        vars[varindex].hiding = true;
        push_indexed_matrix(name, namelength);
    } else {
        if (matedit_mode == 1 &&
                string_equals(name, namelength, matedit_name, matedit_length))
            matedit_i = matedit_j = 0;
        free_vartype(vars[varindex].value);
    }
    vars[varindex].value = value;
    update_catalog();
    return ERR_NONE;
}

void purge_var(const char *name, int namelength) {
    int varindex = lookup_var(name, namelength);
    if (varindex == -1)
        return;
    if (vars[varindex].level != -1 && vars[varindex].level != get_rtn_level())
        // Won't delete local var not created at this level
        return;
    free_vartype(vars[varindex].value);
    if (vars[varindex].hiding) {
        for (int i = varindex - 1; i >= 0; i--)
            if (vars[i].hidden && string_equals(vars[i].name, vars[i].length, name, namelength)) {
                vars[i].hidden = false;
                break;
            }
        pop_indexed_matrix(name, namelength);
    }
    for (int i = varindex; i < vars_count - 1; i++)
        vars[i] = vars[i + 1];
    vars_count--;
    update_catalog();
}

void purge_all_vars() {
    int i;
    for (i = 0; i < vars_count; i++)
        free_vartype(vars[i].value);
    vars_count = 0;
}

int vars_exist(int real, int cpx, int matrix) {
    int i;
    for (i = 0; i < vars_count; i++) {
        if (vars[i].hidden)
            continue;
        switch (vars[i].value->type) {
            case TYPE_REAL:
            case TYPE_STRING:
                if (real)
                    return 1;
                else
                    break;
            case TYPE_COMPLEX:
                if (cpx)
                    return 1;
                else
                    break;
            case TYPE_REALMATRIX:
            case TYPE_COMPLEXMATRIX:
                if (matrix)
                    return 1;
                else
                    break;
        }
    }
    return 0;
}

int contains_no_strings(const vartype_realmatrix *rm) {
    int4 size = rm->rows * rm->columns;
    int4 i;
    for (i = 0; i < size; i++)
        if (rm->array->is_string[i])
            return 0;
    return 1;
}

int matrix_copy(vartype *dst, const vartype *src) {
    int4 size, i;
    if (src->type == TYPE_REALMATRIX) {
        vartype_realmatrix *s = (vartype_realmatrix *) src;
        if (dst->type == TYPE_REALMATRIX) {
            vartype_realmatrix *d = (vartype_realmatrix *) dst;
            if (s->rows != d->rows || s->columns != d->columns)
                return ERR_DIMENSION_ERROR;
            size = s->rows * s->columns;
            for (i = 0; i < size; i++) {
                d->array->is_string[i] = s->array->is_string[i];
                d->array->data[i] = s->array->data[i];
            }
            return ERR_NONE;
        } else if (dst->type == TYPE_COMPLEXMATRIX) {
            vartype_complexmatrix *d = (vartype_complexmatrix *) dst;
            if (s->rows != d->rows || s->columns != d->columns)
                return ERR_DIMENSION_ERROR;
            if (!contains_no_strings(s))
                return ERR_ALPHA_DATA_IS_INVALID;
            size = s->rows * s->columns;
            for (i = 0; i < size; i++) {
                d->array->data[2 * i] = s->array->data[i];
                d->array->data[2 * i + 1] = 0;
            }
            return ERR_NONE;
        } else
            return ERR_INVALID_TYPE;
    } else if (src->type == TYPE_COMPLEXMATRIX
                    && dst->type == TYPE_COMPLEXMATRIX) {
        vartype_complexmatrix *s = (vartype_complexmatrix *) src;
        vartype_complexmatrix *d = (vartype_complexmatrix *) dst;
        if (s->rows != d->rows || s->columns != d->columns)
            return ERR_DIMENSION_ERROR;
        size = s->rows * s->columns * 2;
        for (i = 0; i < size; i++)
            d->array->data[i] = s->array->data[i];
        return ERR_NONE;
    } else
        return ERR_INVALID_TYPE;
}
