/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2023  Thomas Okken
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
#include <string.h>

#include "core_globals.h"
#include "core_helpers.h"
#include "core_display.h"
#include "core_variables.h"


// We cache vartype_real, vartype_complex, and vartype_string instances, to
// cut down on the malloc/free overhead.

#define POOLSIZE 10
static vartype_real *realpool[POOLSIZE];
static vartype_complex *complexpool[POOLSIZE];
static vartype_string *stringpool[POOLSIZE];
static int realpool_size = 0;
static int complexpool_size = 0;
static int stringpool_size = 0;

vartype *new_real(phloat value) {
    vartype_real *r;
    if (realpool_size > 0) {
        r = realpool[--realpool_size];
    } else {
        r = (vartype_real *) malloc(sizeof(vartype_real));
        if (r == NULL)
            return NULL;
        r->type = TYPE_REAL;
    }
    r->x = value;
    return (vartype *) r;
}

vartype *new_complex(phloat re, phloat im) {
    vartype_complex *c;
    if (complexpool_size > 0) {
        c = complexpool[--complexpool_size];
    } else {
        c = (vartype_complex *) malloc(sizeof(vartype_complex));
        if (c == NULL)
            return NULL;
        c->type = TYPE_COMPLEX;
    }
    c->re = re;
    c->im = im;
    return (vartype *) c;
}

vartype *new_string(const char *text, int length) {
    char *dbuf;
    if (length > SSLENV) {
        dbuf = (char *) malloc(length);
        if (dbuf == NULL)
            return NULL;
    }
    vartype_string *s;
    if (stringpool_size > 0) {
        s = stringpool[--stringpool_size];
    } else {
        s = (vartype_string *) malloc(sizeof(vartype_string));
        if (s == NULL) {
            if (length > SSLENV)
                free(dbuf);
            return NULL;
        }
        s->type = TYPE_STRING;
    }
    s->length = length;
    if (length > SSLENV)
        s->t.ptr = dbuf;
    if (text != NULL)
        memcpy(length > SSLENV ? s->t.ptr : s->t.buf, text, length);
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
        free(rm->array);
        free(rm);
        return NULL;
    }
    rm->array->is_string = (char *) malloc(sz);
    if (rm->array->is_string == NULL) {
        free(rm->array->data);
        free(rm->array);
        free(rm);
        return NULL;
    }
    for (i = 0; i < sz; i++)
        rm->array->data[i] = 0;
    memset(rm->array->is_string, 0, sz);
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
        free(cm->array);
        free(cm);
        return NULL;
    }
    for (i = 0; i < sz; i++)
        cm->array->data[i] = 0;
    cm->array->refcount = 1;
    return (vartype *) cm;
}

vartype *new_list(int4 size) {
    vartype_list *list = (vartype_list *) malloc(sizeof(vartype_list));
    if (list == NULL)
        return NULL;
    list->type = TYPE_LIST;
    list->size = size;
    list->array = (list_data *) malloc(sizeof(list_data));
    if (list->array == NULL) {
        free(list);
        return NULL;
    }
    list->array->data = (vartype **) malloc(size * sizeof(vartype *));
    if (list->array->data == NULL && size != 0) {
        free(list->array);
        free(list);
        return NULL;
    }
    memset(list->array->data, 0, size * sizeof(vartype *));
    list->array->refcount = 1;
    return (vartype *) list;
}

void free_vartype(vartype *v) {
    if (v == NULL)
        return;
    switch (v->type) {
        case TYPE_REAL: {
            if (realpool_size < POOLSIZE)
                realpool[realpool_size++] = (vartype_real *) v;
            else
                free(v);
            break;
        }
        case TYPE_COMPLEX: {
            if (complexpool_size < POOLSIZE)
                complexpool[complexpool_size++] = (vartype_complex *) v;
            else
                free(v);
            break;
        }
        case TYPE_STRING: {
            vartype_string *s = (vartype_string *) v;
            if (s->length > SSLENV)
                free(s->t.ptr);
            if (stringpool_size < POOLSIZE)
                stringpool[stringpool_size++] = s;
            else
                free(s);
            break;
        }
        case TYPE_REALMATRIX: {
            vartype_realmatrix *rm = (vartype_realmatrix *) v;
            if (--(rm->array->refcount) == 0) {
                int4 sz = rm->rows * rm->columns;
                free_long_strings(rm->array->is_string, rm->array->data, sz);
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
        case TYPE_LIST: {
            vartype_list *list = (vartype_list *) v;
            if (--(list->array->refcount) == 0) {
                for (int4 i = 0; i < list->size; i++)
                    free_vartype(list->array->data[i]);
                free(list->array->data);
                free(list->array);
            }
            free(list);
            break;
        }
    }
}

void clean_vartype_pools() {
    while (realpool_size > 0)
        free(realpool[--realpool_size]);
    while (complexpool_size > 0)
        free(complexpool[--complexpool_size]);
    while (stringpool_size > 0)
        free(stringpool[--stringpool_size]);
}

void free_long_strings(char *is_string, phloat *data, int4 n) {
    for (int4 i = 0; i < n; i++)
        if (is_string[i] == 2)
            free(*(void **) &data[i]);
}

void get_matrix_string(vartype_realmatrix *rm, int i, char **text, int4 *length) {
    if (rm->array->is_string[i] == 1) {
        char *t = (char *) &rm->array->data[i];
        *text = t + 1;
        *length = *t;
    } else {
        int4 *p = *(int4 **) &rm->array->data[i];
        *text = (char *) (p + 1);
        *length = *p;
    }
}

void get_matrix_string(const vartype_realmatrix *rm, int i, const char **text, int4 *length) {
    get_matrix_string((vartype_realmatrix *) rm, i, (char **) text, length);
}

bool put_matrix_string(vartype_realmatrix *rm, int i, const char *text, int4 length) {
    char *ptext;
    int4 plength;
    if (rm->array->is_string[i] != 0) {
        get_matrix_string(rm, i, &ptext, &plength);
        if (plength == length) {
            memcpy(ptext, text, length);
            return true;
        }
    }
    if (length > SSLENM) {
        int4 *p = (int4 *) malloc(length + 4);
        if (p == NULL)
            return false;
        *p = length;
        memcpy(p + 1, text, length);
        if (rm->array->is_string[i] == 2)
            free(*(void **) &rm->array->data[i]);
        *(int4 **) &rm->array->data[i] = p;
        rm->array->is_string[i] = 2;
    } else {
        void *oldptr = rm->array->is_string[i] == 2 ? *(void **) &rm->array->data[i] : NULL;
        char *t = (char *) &rm->array->data[i];
        t[0] = length;
        memmove(t + 1, text, length);
        rm->array->is_string[i] = 1;
        if (oldptr != NULL)
            free(oldptr);
    }
    return true;
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
            *rm2 = *rm;
            rm->array->refcount++;
            return (vartype *) rm2;
        }
        case TYPE_COMPLEXMATRIX: {
            vartype_complexmatrix *cm = (vartype_complexmatrix *) v;
            vartype_complexmatrix *cm2 = (vartype_complexmatrix *)
                                        malloc(sizeof(vartype_complexmatrix));
            if (cm2 == NULL)
                return NULL;
            *cm2 = *cm;
            cm->array->refcount++;
            return (vartype *) cm2;
        }
        case TYPE_STRING: {
            vartype_string *s = (vartype_string *) v;
            return new_string(s->txt(), s->length);
        }
        case TYPE_LIST: {
            vartype_list *list = (vartype_list *) v;
            vartype_list *list2 = (vartype_list *) malloc(sizeof(vartype_list));
            if (list2 == NULL)
                return NULL;
            *list2 = *list;
            list->array->refcount++;
            return (vartype *) list2;
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
                for (i = 0; i < sz; i++) {
                    md->is_string[i] = rm->array->is_string[i];
                    if (md->is_string[i] == 2) {
                        int4 *sp = *(int4 **) &rm->array->data[i];
                        int4 len = *sp + 4;
                        int4 *dp = (int4 *) malloc(len);
                        if (dp == NULL) {
                            free_long_strings(md->is_string, md->data, i);
                            free(md->is_string);
                            free(md->data);
                            free(md);
                            return 0;
                        }
                        memcpy(dp, sp, len);
                        *(int4 **) &md->data[i] = dp;
                    } else {
                        md->data[i] = rm->array->data[i];
                    }
                }
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
        case TYPE_LIST: {
            vartype_list *list = (vartype_list *) v;
            if (list->array->refcount == 1)
                return 1;
            else {
                list_data *ld = (list_data *) malloc(sizeof(list_data));
                if (ld == NULL)
                    return 0;
                ld->data = (vartype **) malloc(list->size * sizeof(vartype *));
                if (ld->data == NULL && list->size != 0) {
                    free(ld);
                    return 0;
                }
                for (int4 i = 0; i < list->size; i++) {
                    vartype *vv = list->array->data[i];
                    if (vv != NULL) {
                        vv = dup_vartype(vv);
                        if (vv == NULL) {
                            for (int4 j = 0; j < i; j++)
                                free_vartype(ld->data[j]);
                            free(ld->data);
                            free(ld);
                            return 0;
                        }
                    }
                    ld->data[i] = vv;
                }
                ld->refcount = 1;
                list->array->refcount--;
                list->array = ld;
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
        if ((vars[i].flags & (VAR_HIDDEN | VAR_PRIVATE)) != 0)
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
        vars[varindex].flags = 0;
    } else if (local && vars[varindex].level < get_rtn_level()) {
        bool must_push = false;
        if ((matedit_mode == 1 || matedit_mode == 3)
                && string_equals(name, namelength, matedit_name, matedit_length)) {
            if (matedit_mode == 3)
                return ERR_RESTRICTED_OPERATION;
            else
                must_push = true;
        }
        if (vars_count == vars_capacity) {
            int nc = vars_capacity + 25;
            var_struct *nv = (var_struct *) realloc(vars, nc * sizeof(var_struct));
            if (nv == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            vars_capacity = nc;
            vars = nv;
        }
        vars[varindex].flags |= VAR_HIDDEN;
        varindex = vars_count++;
        vars[varindex].length = namelength;
        for (i = 0; i < namelength; i++)
            vars[varindex].name[i] = name[i];
        vars[varindex].level = get_rtn_level();
        vars[varindex].flags = VAR_HIDING;
        if (must_push)
            push_indexed_matrix();
    } else {
        if (matedit_mode == 1 &&
                string_equals(name, namelength, matedit_name, matedit_length)) {
            if (value->type == TYPE_REALMATRIX
                    || value->type == TYPE_COMPLEXMATRIX
                    || value->type == TYPE_LIST)
                matedit_i = matedit_j = 0;
            else
                matedit_mode = 0;
        }
        free_vartype(vars[varindex].value);
    }
    vars[varindex].value = value;
    update_catalog();
    return ERR_NONE;
}

bool purge_var(const char *name, int namelength, bool global, bool local) {
    int varindex = lookup_var(name, namelength);
    if (varindex == -1)
        return true;
    if (vars[varindex].level == -1) {
        if (!global)
            // Asked to delete a local, but found a global;
            // not an error, but don't delete.
            return true;
    } else {
        if (!local)
            // Asked to delete a global, but found a local;
            // that's an error.
            return false;
        if (vars[varindex].level != get_rtn_level())
            // Found a local at a lower level;
            // not an error, but don't delete.
            return true;
    }
    if (matedit_mode == 1 && string_equals(matedit_name, matedit_length, name, namelength))
        matedit_mode = 0;
    free_vartype(vars[varindex].value);
    if ((vars[varindex].flags & VAR_HIDING) != 0) {
        for (int i = varindex - 1; i >= 0; i--)
            if ((vars[i].flags & VAR_HIDDEN) != 0 && string_equals(vars[i].name, vars[i].length, name, namelength)) {
                vars[i].flags &= ~VAR_HIDDEN;
                break;
            }
    }
    for (int i = varindex; i < vars_count - 1; i++)
        vars[i] = vars[i + 1];
    vars_count--;
    update_catalog();
    return true;
}

void purge_all_vars() {
    int i;
    for (i = 0; i < vars_count; i++)
        free_vartype(vars[i].value);
    vars_count = 0;
}

bool vars_exist(int section) {
    int i;
    for (i = 0; i < vars_count; i++) {
        if ((vars[i].flags & (VAR_HIDDEN | VAR_PRIVATE)) != 0)
            continue;
        if (section == -1)
            return true;
        switch (vars[i].value->type) {
            case TYPE_REAL:
                if (section == CATSECT_REAL)
                    return true;
                else
                    break;
            case TYPE_STRING:
                if (section == CATSECT_REAL || section == CATSECT_LIST_STR_ONLY)
                    return true;
                else
                    break;
            case TYPE_COMPLEX:
                if (section == CATSECT_CPX)
                    return true;
                else
                    break;
            case TYPE_REALMATRIX:
            case TYPE_COMPLEXMATRIX:
                if (section == CATSECT_MAT || section == CATSECT_MAT_LIST)
                    return true;
                else
                    break;
            case TYPE_LIST:
                if (section == CATSECT_LIST_STR_ONLY || section == CATSECT_MAT_LIST)
                    return true;
                else
                    break;
        }
    }
    return false;
}

bool contains_strings(const vartype_realmatrix *rm) {
    int4 size = rm->rows * rm->columns;
    for (int4 i = 0; i < size; i++)
        if (rm->array->is_string[i] != 0)
            return true;
    return false;
}

/* This is only used by core_linalg1, and does not deal with strings,
 * even when copying a real matrix to a real matrix. It returns an
 * error if any are encountered.
 */
int matrix_copy(vartype *dst, const vartype *src) {
    if (src->type == TYPE_REALMATRIX) {
        vartype_realmatrix *s = (vartype_realmatrix *) src;
        if (dst->type == TYPE_REALMATRIX) {
            vartype_realmatrix *d = (vartype_realmatrix *) dst;
            if (s->rows != d->rows || s->columns != d->columns)
                return ERR_DIMENSION_ERROR;
            if (contains_strings(s))
                return ERR_ALPHA_DATA_IS_INVALID;
            int4 size = s->rows * s->columns;
            free_long_strings(d->array->is_string, d->array->data, size);
            memset(d->array->is_string, 0, size);
            memcpy(d->array->data, s->array->data, size * sizeof(phloat));
            return ERR_NONE;
        } else if (dst->type == TYPE_COMPLEXMATRIX) {
            vartype_complexmatrix *d = (vartype_complexmatrix *) dst;
            if (s->rows != d->rows || s->columns != d->columns)
                return ERR_DIMENSION_ERROR;
            if (contains_strings(s))
                return ERR_ALPHA_DATA_IS_INVALID;
            int4 size = s->rows * s->columns;
            for (int4 i = 0; i < size; i++) {
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
        int4 size = s->rows * s->columns * 2;
        memcpy(d->array->data, s->array->data, size * sizeof(phloat));
        return ERR_NONE;
    } else
        return ERR_INVALID_TYPE;
}

static int lookup_private_var(const char *name, int namelength) {
    int level = get_rtn_level();
    int i, j;
    for (i = vars_count - 1; i >= 0; i--) {
        int vlevel = vars[i].level;
        if (vlevel == -1)
            continue;
        if (vlevel < level)
            break;
        if ((vars[i].flags & VAR_PRIVATE) == 0)
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

vartype *recall_private_var(const char *name, int namelength) {
    int varindex = lookup_private_var(name, namelength);
    if (varindex == -1)
        return NULL;
    else
        return vars[varindex].value;
}

vartype *recall_and_purge_private_var(const char *name, int namelength) {
    int varindex = lookup_private_var(name, namelength);
    if (varindex == -1)
        return NULL;
    vartype *ret = vars[varindex].value;
    for (int i = varindex; i < vars_count - 1; i++)
        vars[i] = vars[i + 1];
    vars_count--;
    return ret;
}

int store_private_var(const char *name, int namelength, vartype *value) {
    int varindex = lookup_private_var(name, namelength);
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
        vars[varindex].level = get_rtn_level();
        vars[varindex].flags = VAR_PRIVATE;
    } else {
        free_vartype(vars[varindex].value);
    }
    vars[varindex].value = value;
    return ERR_NONE;
}
