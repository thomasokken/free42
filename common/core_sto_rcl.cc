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

#include <stdlib.h>

#include "core_helpers.h"
#include "core_linalg1.h"
#include "core_sto_rcl.h"
#include "core_variables.h"


static int apply_sto_operation(char operation, vartype *oldval);
static void generic_sto_completion(int error, vartype *res);

static bool preserve_ij;


static int apply_sto_operation(char operation, vartype *oldval) {
    if (!ensure_var_space(1))
        return ERR_INSUFFICIENT_MEMORY;
    vartype *newval;
    int error;
    switch (operation) {
        case '/':
            preserve_ij = true;
            return generic_div(reg_x, oldval, generic_sto_completion);
        case '*':
            preserve_ij = false;
            return generic_mul(reg_x, oldval, generic_sto_completion);
        case '-':
            preserve_ij = true;
            error = generic_sub(reg_x, oldval, &newval);
            generic_sto_completion(error, newval);
            return error;
        case '+':
            preserve_ij = true;
            error = generic_add(reg_x, oldval, &newval);
            generic_sto_completion(error, newval);
            return error;
        default:
            return ERR_INTERNAL_ERROR;
    }
}

int generic_div(const vartype *px, const vartype *py, void (*completion)(int, vartype *)) {
    if (px->type == TYPE_STRING || py->type == TYPE_STRING) {
        completion(ERR_ALPHA_DATA_IS_INVALID, NULL);
        return ERR_ALPHA_DATA_IS_INVALID;
    } else if ((px->type == TYPE_REALMATRIX || px->type == TYPE_COMPLEXMATRIX)
            && (py->type == TYPE_REALMATRIX || py->type == TYPE_COMPLEXMATRIX)){
        return linalg_div(py, px, completion);
    } else {
        vartype *dst;
        int error = map_binary(px, py, &dst, div_rr, div_rc, div_cr, div_cc);
        completion(error, dst);
        return error;
    }
}

int generic_mul(const vartype *px, const vartype *py, void (*completion)(int, vartype *)) {
    if (px->type == TYPE_STRING || py->type == TYPE_STRING) {
        completion(ERR_ALPHA_DATA_IS_INVALID, NULL);
        return ERR_ALPHA_DATA_IS_INVALID;
    } else if ((px->type == TYPE_REALMATRIX || px->type == TYPE_COMPLEXMATRIX)
            && (py->type == TYPE_REALMATRIX || py->type == TYPE_COMPLEXMATRIX)){
        return linalg_mul(py, px, completion);
    } else {
        vartype *dst;
        int error = map_binary(px, py, &dst, mul_rr, mul_rc, mul_cr, mul_cc);
        completion(error, dst);
        return error;
    }
}

int generic_sub(const vartype *px, const vartype *py, vartype **dst) {
    if (px->type == TYPE_REAL && py->type == TYPE_REAL) {
        vartype_real *x = (vartype_real *) px;
        vartype_real *y = (vartype_real *) py;
        phloat res = y->x - x->x;
        int inf = p_isinf(res);
        if (inf != 0) {
            if (flags.f.range_error_ignore)
                res = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        }
        *dst = new_real(res);
        if (*dst == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        else
            return ERR_NONE;
    } else if (px->type == TYPE_COMPLEX && py->type == TYPE_COMPLEX) {
        vartype_complex *x = (vartype_complex *) px;
        vartype_complex *y = (vartype_complex *) py;
        int inf;
        phloat re, im;
        re = y->re - x->re;
        inf = p_isinf(re);
        if (inf != 0) {
            if (flags.f.range_error_ignore)
                re = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        }
        im = y->im - x->im;
        inf = p_isinf(im);
        if (inf != 0) {
            if (flags.f.range_error_ignore)
                im = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        }
        *dst = new_complex(re, im);
        if (*dst == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        else
            return ERR_NONE;
    } else if (px->type == TYPE_REAL && py->type == TYPE_COMPLEX) {
        vartype_real *x = (vartype_real *) px;
        vartype_complex *y = (vartype_complex *) py;
        int inf;
        phloat re = y->re - x->x;
        inf = p_isinf(re);
        if (inf != 0) {
            if (flags.f.range_error_ignore)
                re = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        }
        *dst = new_complex(re, y->im);
        if (*dst == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        else
            return ERR_NONE;
    } else if (px->type == TYPE_COMPLEX && py->type == TYPE_REAL) {
        vartype_complex *x = (vartype_complex *) px;
        vartype_real *y = (vartype_real *) py;
        phloat re = y->x - x->re;
        int inf = p_isinf(re);
        if (inf != 0) {
            if (flags.f.range_error_ignore)
                re = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        }
        *dst = new_complex(re, -x->im);
        if (*dst == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        else
            return ERR_NONE;
    } else if (px->type == TYPE_STRING || py->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return map_binary(px, py, dst, sub_rr, sub_rc, sub_cr, sub_cc);
}

int generic_add(const vartype *px, const vartype *py, vartype **dst) {
    if (px->type == TYPE_REAL && py->type == TYPE_REAL) {
        vartype_real *x = (vartype_real *) px;
        vartype_real *y = (vartype_real *) py;
        phloat res = y->x + x->x;
        int inf = p_isinf(res);
        if (inf != 0) {
            if (flags.f.range_error_ignore)
                res = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        }
        *dst = new_real(res);
        if (*dst == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        else
            return ERR_NONE;
    } else if (px->type == TYPE_COMPLEX && py->type == TYPE_COMPLEX) {
        vartype_complex *x = (vartype_complex *) px;
        vartype_complex *y = (vartype_complex *) py;
        int inf;
        phloat re, im;
        re = x->re + y->re;
        inf = p_isinf(re);
        if (inf != 0) {
            if (flags.f.range_error_ignore)
                re = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        }
        im = x->im + y->im;
        inf = p_isinf(im);
        if (inf != 0) {
            if (flags.f.range_error_ignore)
                im = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        }
        *dst = new_complex(re, im);
        if (*dst == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        else
            return ERR_NONE;
    } else if (px->type == TYPE_REAL && py->type == TYPE_COMPLEX) {
        vartype_real *x = (vartype_real *) px;
        vartype_complex *y = (vartype_complex *) py;
        phloat re = y->re + x->x;
        int inf = p_isinf(re);
        if (inf != 0) {
            if (flags.f.range_error_ignore)
                re = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        }
        *dst = new_complex(re, y->im);
        if (*dst == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        else
            return ERR_NONE;
    } else if (px->type == TYPE_COMPLEX && py->type == TYPE_REAL) {
        vartype_complex *x = (vartype_complex *) px;
        vartype_real *y = (vartype_real *) py;
        phloat re = x->re + y->x;
        int inf = p_isinf(re);
        if (inf != 0) {
            if (flags.f.range_error_ignore)
                re = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        }
        *dst = new_complex(re, x->im);
        if (*dst == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        else
            return ERR_NONE;
    } else if (px->type == TYPE_STRING || py->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return map_binary(px, py, dst, add_rr, add_rc, add_cr, add_cc);
}

int generic_rcl(arg_struct *arg, vartype **dst) {
    int err;
    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    switch (arg->type) {
        case ARGTYPE_NUM: {
            vartype *regs = recall_var("REGS", 4);
            if (regs == NULL)
                return ERR_SIZE_ERROR;
            else if (regs->type == TYPE_REALMATRIX) {
                vartype_realmatrix *rm = (vartype_realmatrix *) regs;
                int4 size = rm->rows * rm->columns;
                int4 index = arg->val.num;
                phloat ds;
                if (index >= size)
                    return ERR_SIZE_ERROR;
                ds = rm->array->data[index];
                if (rm->array->is_string[index])
                    *dst = new_string(phloat_text(ds), phloat_length(ds));
                else
                    *dst = new_real(ds);
                if (*dst == NULL)
                    return ERR_INSUFFICIENT_MEMORY;
                return ERR_NONE;
            } else if (regs->type == TYPE_COMPLEXMATRIX) {
                vartype_complexmatrix *cm = (vartype_complexmatrix *) regs;
                int4 size = cm->rows * cm->columns;
                if (arg->val.num >= size)
                    return ERR_SIZE_ERROR;
                *dst = new_complex(cm->array->data[arg->val.num * 2],
                                cm->array->data[arg->val.num * 2 + 1]);
                if (*dst == NULL)
                    return ERR_INSUFFICIENT_MEMORY;
                return ERR_NONE;
            } else {
                /* This should never happen; STO should prevent
                 * "REGS" from being any other type than a real or
                 * complex matrix.
                 */
                return ERR_INTERNAL_ERROR;
            }
        }
        case ARGTYPE_STK: {
            switch (arg->val.stk) {
                case 'X': *dst = reg_x; break;
                case 'Y': *dst = reg_y; break;
                case 'Z': *dst = reg_z; break;
                case 'T': *dst = reg_t; break;
                case 'L': *dst = reg_lastx; break;
            }
            *dst = dup_vartype(*dst);
            if (*dst == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            return ERR_NONE;
        }
        case ARGTYPE_STR: {
            *dst = recall_var(arg->val.text, arg->length);
            if (*dst == NULL)
                return ERR_NONEXISTENT;
            *dst = dup_vartype(*dst);
            if (*dst == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            return ERR_NONE;
        }
        default:
            return ERR_INTERNAL_ERROR;
    }
}

static arg_struct temp_arg;

static void generic_sto_completion(int error, vartype *res) {
    if (error != ERR_NONE)
        return;
    if (temp_arg.type == ARGTYPE_STK) {
        switch (temp_arg.val.stk) {
            case 'X':
                free_vartype(reg_x);
                reg_x = res;
                break;
            case 'Y':
                free_vartype(reg_y);
                reg_y = res;
                break;
            case 'Z':
                free_vartype(reg_z);
                reg_z = res;
                break;
            case 'T':
                free_vartype(reg_t);
                reg_t = res;
                break;
            case 'L':
                free_vartype(reg_lastx);
                reg_lastx = res;
                break;
        }
    } else /* temp_arg.type == ARGTYPE_STR */ {
        // If the destination of store_var() is the indexed matrix, it sets I
        // and J to 1. This is *not* the desired behavior for STO+, STO-, and
        // STO/. (It is correct for STO and STO*, since those can cause the
        // destination's dimensions to change.)
        int4 i = matedit_i;
        int4 j = matedit_j;
        store_var(temp_arg.val.text, temp_arg.length, res);
        if (preserve_ij) {
            matedit_i = i;
            matedit_j = j;
        }
    }
}

int generic_sto(arg_struct *arg, char operation) {
    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        int err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }

    if (operation != 0 && reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;

    switch (arg->type) {
        case ARGTYPE_NUM: {
            vartype *regs = recall_var("REGS", 4);
            if (regs == NULL)
                return ERR_SIZE_ERROR;
            if (regs->type == TYPE_REALMATRIX) {
                vartype_realmatrix *rm = (vartype_realmatrix *) regs;
                int4 size = rm->rows * rm->columns;
                int4 num = arg->val.num;
                if (num >= size)
                    return ERR_SIZE_ERROR;
                if (reg_x->type == TYPE_STRING) {
                    if (!disentangle((vartype *) rm))
                        return ERR_INSUFFICIENT_MEMORY;
                    vartype_string *vs = (vartype_string *) reg_x;
                    phloat *ds = rm->array->data + num;
                    int len, i;
                    len = vs->length;
                    phloat_length(*ds) = len;
                    for (i = 0; i < len; i++)
                        phloat_text(*ds)[i] = vs->text[i];
                    rm->array->is_string[num] = 1;
                    return ERR_NONE;
                } else if (reg_x->type == TYPE_REAL) {
                    if (!disentangle((vartype *) rm))
                        return ERR_INSUFFICIENT_MEMORY;
                    if (operation == 0) {
                        rm->array->data[num] = ((vartype_real *) reg_x)->x;
                        rm->array->is_string[num] = 0;
                    } else {
                        phloat x, n;
                        int inf;
                        if (rm->array->is_string[num])
                            return ERR_ALPHA_DATA_IS_INVALID;
                        x = ((vartype_real *) reg_x)->x;
                        n = rm->array->data[num];
                        switch (operation) {
                            case '/': if (x == 0) return ERR_DIVIDE_BY_0;
                                      n /= x; break;
                            case '*': n *= x; break;
                            case '-': n -= x; break;
                            case '+': n += x; break;
                        }
                        inf = p_isinf(n);
                        if (inf != 0) {
                            if (flags.f.range_error_ignore)
                                n = inf == 1 ? POS_HUGE_PHLOAT
                                             : NEG_HUGE_PHLOAT;
                            else
                                return ERR_OUT_OF_RANGE;
                        }
                        rm->array->data[num] = n;
                    }
                    return ERR_NONE;
                } else
                    return ERR_INVALID_TYPE;
            } else if (regs->type == TYPE_COMPLEXMATRIX) {
                vartype_complexmatrix *cm = (vartype_complexmatrix *) regs;
                int4 size = cm->rows * cm->columns;
                int4 num = arg->val.num;
                phloat re, im;
                if (num >= size)
                    return ERR_SIZE_ERROR;
                if (reg_x->type == TYPE_STRING)
                    return ERR_ALPHA_DATA_IS_INVALID;
                else if (reg_x->type != TYPE_REAL
                        && reg_x->type != TYPE_COMPLEX)
                    return ERR_INVALID_TYPE;
                if (!disentangle((vartype *) cm))
                    return ERR_INSUFFICIENT_MEMORY;
                if (operation == 0) {
                    if (reg_x->type == TYPE_REAL) {
                        re = ((vartype_real *) reg_x)->x;
                        im = 0;
                    } else {
                        re = ((vartype_complex *) reg_x)->re;
                        im = ((vartype_complex *) reg_x)->im;
                    }
                    cm->array->data[num * 2] = re;
                    cm->array->data[num * 2 + 1] = im;
                } else {
                    phloat nre = cm->array->data[num * 2];
                    phloat nim = cm->array->data[num * 2 + 1];
                    int inf;
                    if (reg_x->type == TYPE_REAL) {
                        phloat x;
                        x = ((vartype_real *) reg_x)->x;
                        switch (operation) {
                            case '/': if (x == 0) return ERR_DIVIDE_BY_0;
                                      nre /= x; nim /= x; break;
                            case '*': nre *= x; nim *= x; break;
                            case '-': nre -= x; break;
                            case '+': nre += x; break;
                        }
                    } else {
                        phloat xre, xim, h, tmp;
                        xre = ((vartype_complex *) reg_x)->re;
                        xim = ((vartype_complex *) reg_x)->im;
                        h = xre * xre + xim * xim;
                        /* TODO: handle overflows in intermediate results */
                        switch (operation) {
                            case '/':
                                if (h == 0)
                                    return ERR_DIVIDE_BY_0;
                                tmp = (nre * xre + nim * xim) / h;
                                nim = (xre * nim - xim * nre) / h;
                                nre = tmp;
                                break;
                            case '*':
                                tmp = nre * xre - nim * xim;
                                nim = nre * xim + nim * xre;
                                nre = tmp;
                                break;
                            case '-':
                                nre -= xre;
                                nim -= xim;
                                break;
                            case '+':
                                nre += xre;
                                nim += xim;
                                break;
                        }
                    }
                    inf = p_isinf(nre);
                    if (inf != 0) {
                        if (flags.f.range_error_ignore)
                            nre = inf == 1 ? POS_HUGE_PHLOAT
                                           : NEG_HUGE_PHLOAT;
                        else
                            return ERR_OUT_OF_RANGE;
                    }
                    inf = p_isinf(nim);
                    if (inf != 0) {
                        if (flags.f.range_error_ignore)
                            nim = inf == 1 ? POS_HUGE_PHLOAT
                                           : NEG_HUGE_PHLOAT;
                        else
                            return ERR_OUT_OF_RANGE;
                    }
                    cm->array->data[num * 2] = nre;
                    cm->array->data[num * 2 + 1] = nim;
                }
                return ERR_NONE;
            } else {
                /* This should never happen; STO should prevent
                 * "REGS" from being any other type than a real or
                 * complex matrix.
                 */
                return ERR_INTERNAL_ERROR;
            }
        }
        case ARGTYPE_STK: {
            vartype *newval;
            if (operation == 0) {
                if (arg->val.stk == 'X') {
                    /* STO ST X : no-op! */
                    return ERR_NONE;
                }
                newval = dup_vartype(reg_x);
                if (newval == NULL)
                    return ERR_INSUFFICIENT_MEMORY;
                switch (arg->val.stk) {
                    case 'Y':
                        free_vartype(reg_y);
                        reg_y = newval;
                        break;
                    case 'Z':
                        free_vartype(reg_z);
                        reg_z = newval;
                        break;
                    case 'T':
                        free_vartype(reg_t);
                        reg_t = newval;
                        break;
                    case 'L':
                        free_vartype(reg_lastx);
                        reg_lastx = newval;
                        break;
                }
                return ERR_NONE;
            } else {
                vartype *oldval;
                switch (arg->val.stk) {
                    case 'X': oldval = reg_x; break;
                    case 'Y': oldval = reg_y; break;
                    case 'Z': oldval = reg_z; break;
                    case 'T': oldval = reg_t; break;
                    case 'L': oldval = reg_lastx; break;
                }
                temp_arg = *arg;
                return apply_sto_operation(operation, oldval);
            }
        }
        case ARGTYPE_STR: {
            if (operation == 0) {
                vartype *newval;
                /* Only allow matrices to be stored in "REGS" */
                if (string_equals(arg->val.text, arg->length, "REGS", 4)
                        && reg_x->type != TYPE_REALMATRIX
                        && reg_x->type != TYPE_COMPLEXMATRIX)
                    return ERR_RESTRICTED_OPERATION;
                /* When EDITN is active, don't allow the matrix being
                 * edited to be overwritten. */
                if (matedit_mode == 3 && string_equals(arg->val.text,
                            arg->length, matedit_name, matedit_length))
                        return ERR_RESTRICTED_OPERATION;
                newval = dup_vartype(reg_x);
                if (newval == NULL)
                    return ERR_INSUFFICIENT_MEMORY;
                int err = store_var(arg->val.text, arg->length, newval);
                if (err != ERR_NONE)
                    free_vartype(newval);
                return err;
            } else {
                /* When EDITN is active, don't allow the matrix being edited to
                 * be multiplied by another matrix, since this could cause the
                 * target's dimensions to change. Note that this restriction is
                 * applied even if the target's dimensions would not, in fact,
                 * change. */
                if (operation == '*'
                        && (reg_x->type == TYPE_REALMATRIX
                            || reg_x->type == TYPE_COMPLEXMATRIX)
                        && matedit_mode == 3
                        && string_equals(arg->val.text,
                                arg->length, matedit_name, matedit_length))
                    return ERR_RESTRICTED_OPERATION;
                vartype *oldval = recall_var(arg->val.text, arg->length);
                if (oldval == NULL)
                    return ERR_NONEXISTENT;
                temp_arg = *arg;
                return apply_sto_operation(operation, oldval);
            }
        }
        default:
            return ERR_INVALID_TYPE;
    }
}

int map_unary(const vartype *src, vartype **dst, mappable_r mr, mappable_c mc) {
    int error;
    switch (src->type) {
        case TYPE_REAL: {
            phloat r;
            error = mr(((vartype_real *) src)->x, &r);
            if (error == ERR_NONE) {
                *dst = new_real(r);
                if (*dst == NULL)
                    return ERR_INSUFFICIENT_MEMORY;
            }
            return error;
        }
        case TYPE_COMPLEX: {
            phloat rre, rim;
            error = mc(((vartype_complex *) src)->re,
                       ((vartype_complex *) src)->im, &rre, &rim);
            if (error == ERR_NONE) {
                *dst = new_complex(rre, rim);
                if (*dst == NULL)
                    return ERR_INSUFFICIENT_MEMORY;
            }
            return error;
        }
        case TYPE_REALMATRIX: {
            vartype_realmatrix *sm = (vartype_realmatrix *) src;
            vartype_realmatrix *dm;
            int4 size, i;
            int error;
            dm = (vartype_realmatrix *) new_realmatrix(sm->rows, sm->columns);
            if (dm == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            size = sm->rows * sm->columns;
            for (i = 0; i < size; i++) {
                if (sm->array->is_string[i]) {
                    free_vartype((vartype *) dm);
                    return ERR_ALPHA_DATA_IS_INVALID;
                }
            }
            for (i = 0; i < size; i++) {
                error = mr(sm->array->data[i], &dm->array->data[i]);
                if (error != ERR_NONE) {
                    free_vartype((vartype *) dm);
                    return error;
                }
            }
            *dst = (vartype *) dm;
            return ERR_NONE;
        }
        case TYPE_COMPLEXMATRIX: {
            vartype_complexmatrix *sm = (vartype_complexmatrix *) src;
            vartype_complexmatrix *dm;
            int4 rows = sm->rows;
            int4 columns = sm->columns;
            int4 size = 2 * rows * columns;
            int4 i;
            int error;
            dm = (vartype_complexmatrix *) new_complexmatrix(rows, columns);
            if (dm == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            for (i = 0; i < size; i += 2) {
                error = mc(sm->array->data[i], sm->array->data[i + 1],
                           &dm->array->data[i], &dm->array->data[i + 1]);
                if (error != ERR_NONE) {
                    free_vartype((vartype *) dm);
                    return error;
                }
            }
            *dst = (vartype *) dm;
            return ERR_NONE;
        }
        default:
            return ERR_INTERNAL_ERROR;
    }
}

int map_binary(const vartype *src1, const vartype *src2, vartype **dst,
        mappable_rr mrr, mappable_rc mrc, mappable_cr mcr, mappable_cc mcc) {
    int error;
    switch (src1->type) {
        case TYPE_REAL:
            switch (src2->type) {
                case TYPE_REAL: {
                    phloat r;
                    error = mrr(((vartype_real *) src1)->x,
                                ((vartype_real *) src2)->x, &r);
                    if (error == ERR_NONE) {
                        *dst = new_real(r);
                        if (*dst == NULL)
                            return ERR_INSUFFICIENT_MEMORY;
                    }
                    return error;
                }
                case TYPE_COMPLEX: {
                    phloat rre, rim;
                    error = mrc(((vartype_real *) src1)->x,
                                ((vartype_complex *) src2)->re,
                                ((vartype_complex *) src2)->im,
                                &rre, &rim);
                    if (error == ERR_NONE) {
                        *dst = new_complex(rre, rim);
                        if (*dst == NULL)
                            return ERR_INSUFFICIENT_MEMORY;
                    }
                    return error;
                }
                case TYPE_REALMATRIX: {
                    vartype_realmatrix *sm = (vartype_realmatrix *) src2;
                    vartype_realmatrix *dm;
                    int4 size, i;
                    int error;
                    dm = (vartype_realmatrix *)
                                        new_realmatrix(sm->rows, sm->columns);
                    if (dm == NULL)
                        return ERR_INSUFFICIENT_MEMORY;
                    size = sm->rows * sm->columns;
                    for (i = 0; i < size; i++)
                        if (sm->array->is_string[i]) {
                            free_vartype((vartype *) dm);
                            return ERR_ALPHA_DATA_IS_INVALID;
                        }
                    for (i = 0; i < size; i++) {
                        error = mrr(((vartype_real *) src1)->x,
                                    sm->array->data[i],
                                    &dm->array->data[i]);
                        if (error != ERR_NONE) {
                            free_vartype((vartype *) dm);
                            return error;
                        }
                    }
                    *dst = (vartype *) dm;
                    return ERR_NONE;
                }
                case TYPE_COMPLEXMATRIX: {
                    vartype_complexmatrix *sm = (vartype_complexmatrix *) src2;
                    vartype_complexmatrix *dm;
                    int4 size, i;
                    int error;
                    dm = (vartype_complexmatrix *)
                                    new_complexmatrix(sm->rows, sm->columns);
                    if (dm == NULL)
                        return ERR_INSUFFICIENT_MEMORY;
                    size = 2 * sm->rows * sm->columns;
                    for (i = 0; i < size; i += 2) {
                        error = mrc(((vartype_real *) src1)->x,
                                    sm->array->data[i],
                                    sm->array->data[i + 1],
                                    &dm->array->data[i],
                                    &dm->array->data[i + 1]);
                        if (error != ERR_NONE) {
                            free_vartype((vartype *) dm);
                            return error;
                        }
                    }
                    *dst = (vartype *) dm;
                    return ERR_NONE;
                }
                default:
                    return ERR_INTERNAL_ERROR;
            }
        case TYPE_COMPLEX:
            switch (src2->type) {
                case TYPE_REAL: {
                    phloat rre, rim;
                    error = mcr(((vartype_complex *) src1)->re,
                                ((vartype_complex *) src1)->im,
                                ((vartype_real *) src2)->x,
                                &rre, &rim);
                    if (error == ERR_NONE) {
                        *dst = new_complex(rre, rim);
                        if (*dst == NULL)
                            return ERR_INSUFFICIENT_MEMORY;
                    }
                    return error;
                }
                case TYPE_COMPLEX: {
                    phloat rre, rim;
                    error = mcc(((vartype_complex *) src1)->re,
                                ((vartype_complex *) src1)->im,
                                ((vartype_complex *) src2)->re,
                                ((vartype_complex *) src2)->im,
                                &rre, &rim);
                    if (error == ERR_NONE) {
                        *dst = new_complex(rre, rim);
                        if (*dst == NULL)
                            return ERR_INSUFFICIENT_MEMORY;
                    }
                    return error;
                }
                case TYPE_REALMATRIX: {
                    vartype_realmatrix *sm = (vartype_realmatrix *) src2;
                    vartype_complexmatrix *dm;
                    int4 size, i;
                    int error;
                    dm = (vartype_complexmatrix *)
                                    new_complexmatrix(sm->rows, sm->columns);
                    if (dm == NULL)
                        return ERR_INSUFFICIENT_MEMORY;
                    size = sm->rows * sm->columns;
                    for (i = 0; i < size; i++)
                        if (sm->array->is_string[i]) {
                            free_vartype((vartype *) dm);
                            return ERR_ALPHA_DATA_IS_INVALID;
                        }
                    for (i = 0; i < size; i++) {
                        error = mcr(((vartype_complex *) src1)->re,
                                    ((vartype_complex *) src1)->im,
                                    sm->array->data[i],
                                    &dm->array->data[i * 2],
                                    &dm->array->data[i * 2 + 1]);
                        if (error != ERR_NONE) {
                            free_vartype((vartype *) dm);
                            return error;
                        }
                    }
                    *dst = (vartype *) dm;
                    return ERR_NONE;
                }
                case TYPE_COMPLEXMATRIX: {
                    vartype_complexmatrix *sm = (vartype_complexmatrix *) src2;
                    vartype_complexmatrix *dm;
                    int4 size, i;
                    int error;
                    dm = (vartype_complexmatrix *)
                                    new_complexmatrix(sm->rows, sm->columns);
                    if (dm == NULL)
                        return ERR_INSUFFICIENT_MEMORY;
                    size = 2 * sm->rows * sm->columns;
                    for (i = 0; i < size; i += 2) {
                        error = mcc(((vartype_complex *) src1)->re,
                                    ((vartype_complex *) src1)->im,
                                    sm->array->data[i],
                                    sm->array->data[i + 1],
                                    &dm->array->data[i],
                                    &dm->array->data[i + 1]);
                        if (error != ERR_NONE) {
                            free_vartype((vartype *) dm);
                            return error;
                        }
                    }
                    *dst = (vartype *) dm;
                    return ERR_NONE;
                }
                default:
                    return ERR_INTERNAL_ERROR;
            }
        case TYPE_REALMATRIX:
            switch (src2->type) {
                case TYPE_REAL: {
                    vartype_realmatrix *sm = (vartype_realmatrix *) src1;
                    vartype_realmatrix *dm;
                    int4 size, i;
                    int error;
                    dm = (vartype_realmatrix *)
                                        new_realmatrix(sm->rows, sm->columns);
                    if (dm == NULL)
                        return ERR_INSUFFICIENT_MEMORY;
                    size = sm->rows * sm->columns;
                    for (i = 0; i < size; i++)
                        if (sm->array->is_string[i]) {
                            free_vartype((vartype *) dm);
                            return ERR_ALPHA_DATA_IS_INVALID;
                        }
                    for (i = 0; i < size; i++) {
                        error = mrr(sm->array->data[i],
                                    ((vartype_real *) src2)->x,
                                    &dm->array->data[i]);
                        if (error != ERR_NONE) {
                            free_vartype((vartype *) dm);
                            return error;
                        }
                    }
                    *dst = (vartype *) dm;
                    return ERR_NONE;
                }
                case TYPE_COMPLEX: {
                    vartype_realmatrix *sm = (vartype_realmatrix *) src1;
                    vartype_complexmatrix *dm;
                    int4 size, i;
                    int error;
                    dm = (vartype_complexmatrix *)
                                    new_complexmatrix(sm->rows, sm->columns);
                    if (dm == NULL)
                        return ERR_INSUFFICIENT_MEMORY;
                    size = sm->rows * sm->columns;
                    for (i = 0; i < size; i++)
                        if (sm->array->is_string[i]) {
                            free_vartype((vartype *) dm);
                            return ERR_ALPHA_DATA_IS_INVALID;
                        }
                    for (i = 0; i < size; i++) {
                        error = mrc(sm->array->data[i],
                                    ((vartype_complex *) src2)->re,
                                    ((vartype_complex *) src2)->im,
                                    &dm->array->data[i * 2],
                                    &dm->array->data[i * 2 + 1]);
                        if (error != ERR_NONE) {
                            free_vartype((vartype *) dm);
                            return error;
                        }
                    }
                    *dst = (vartype *) dm;
                    return ERR_NONE;
                }
                case TYPE_REALMATRIX: {
                    vartype_realmatrix *sm1 = (vartype_realmatrix *) src1;
                    vartype_realmatrix *sm2 = (vartype_realmatrix *) src2;
                    vartype_realmatrix *dm;
                    int4 size, i;
                    int error;
                    if (sm1->rows != sm2->rows || sm1->columns != sm2->columns)
                        return ERR_DIMENSION_ERROR;
                    dm = (vartype_realmatrix *)
                                    new_realmatrix(sm1->rows, sm1->columns);
                    if (dm == NULL)
                        return ERR_INSUFFICIENT_MEMORY;
                    size = sm1->rows * sm1->columns;
                    for (i = 0; i < size; i++)
                        if (sm1->array->is_string[i]
                                    || sm2->array->is_string[i]) {
                            free_vartype((vartype *) dm);
                            return ERR_ALPHA_DATA_IS_INVALID;
                        }
                    for (i = 0; i < size; i++) {
                        error = mrr(sm1->array->data[i],
                                    sm2->array->data[i],
                                    &dm->array->data[i]);
                        if (error != ERR_NONE) {
                            free_vartype((vartype *) dm);
                            return error;
                        }
                    }
                    *dst = (vartype *) dm;
                    return ERR_NONE;
                }
                case TYPE_COMPLEXMATRIX: {
                    vartype_realmatrix *sm1 = (vartype_realmatrix *) src1;
                    vartype_complexmatrix *sm2 = (vartype_complexmatrix *) src2;
                    vartype_complexmatrix *dm;
                    int4 size, i;
                    int error;
                    if (sm1->rows != sm2->rows || sm1->columns != sm2->columns)
                        return ERR_DIMENSION_ERROR;
                    dm = (vartype_complexmatrix *)
                                    new_complexmatrix(sm1->rows, sm1->columns);
                    if (dm == NULL)
                        return ERR_INSUFFICIENT_MEMORY;
                    size = sm1->rows * sm1->columns;
                    for (i = 0; i < size; i++)
                        if (sm1->array->is_string[i]) {
                            free_vartype((vartype *) dm);
                            return ERR_ALPHA_DATA_IS_INVALID;
                        }
                    for (i = 0; i < size; i++) {
                        error = mrc(sm1->array->data[i],
                                    sm2->array->data[i * 2],
                                    sm2->array->data[i * 2 + 1],
                                    &dm->array->data[i * 2],
                                    &dm->array->data[i * 2 + 1]);
                        if (error != ERR_NONE) {
                            free_vartype((vartype *) dm);
                            return error;
                        }
                    }
                    *dst = (vartype *) dm;
                    return ERR_NONE;
                }
                default:
                    return ERR_INTERNAL_ERROR;
            }
        case TYPE_COMPLEXMATRIX:
            switch (src2->type) {
                case TYPE_REAL: {
                    vartype_complexmatrix *sm = (vartype_complexmatrix *) src1;
                    vartype_complexmatrix *dm;
                    int4 size, i;
                    int error;
                    dm = (vartype_complexmatrix *)
                                    new_complexmatrix(sm->rows, sm->columns);
                    if (dm == NULL)
                        return ERR_INSUFFICIENT_MEMORY;
                    size = 2 * sm->rows * sm->columns;
                    for (i = 0; i < size; i += 2) {
                        error = mcr(sm->array->data[i],
                                    sm->array->data[i + 1],
                                    ((vartype_real *) src2)->x,
                                    &dm->array->data[i],
                                    &dm->array->data[i + 1]);
                        if (error != ERR_NONE) {
                            free_vartype((vartype *) dm);
                            return error;
                        }
                    }
                    *dst = (vartype *) dm;
                    return ERR_NONE;
                }
                case TYPE_COMPLEX: {
                    vartype_complexmatrix *sm = (vartype_complexmatrix *) src1;
                    vartype_complexmatrix *dm;
                    int4 size, i;
                    int error;
                    dm = (vartype_complexmatrix *)
                                    new_complexmatrix(sm->rows, sm->columns);
                    if (dm == NULL)
                        return ERR_INSUFFICIENT_MEMORY;
                    size = 2 * sm->rows * sm->columns;
                    for (i = 0; i < size; i += 2) {
                        error = mcc(sm->array->data[i],
                                    sm->array->data[i + 1],
                                    ((vartype_complex *) src2)->re,
                                    ((vartype_complex *) src2)->im,
                                    &dm->array->data[i],
                                    &dm->array->data[i + 1]);
                        if (error != ERR_NONE) {
                            free_vartype((vartype *) dm);
                            return error;
                        }
                    }
                    *dst = (vartype *) dm;
                    return ERR_NONE;
                }
                case TYPE_REALMATRIX: {
                    vartype_complexmatrix *sm1 = (vartype_complexmatrix *) src1;
                    vartype_realmatrix *sm2 = (vartype_realmatrix *) src2;
                    vartype_complexmatrix *dm;
                    int4 size, i;
                    int error;
                    if (sm1->rows != sm2->rows || sm1->columns != sm2->columns)
                        return ERR_DIMENSION_ERROR;
                    dm = (vartype_complexmatrix *)
                                    new_complexmatrix(sm1->rows, sm1->columns);
                    if (dm == NULL)
                        return ERR_INSUFFICIENT_MEMORY;
                    size = sm1->rows * sm1->columns;
                    for (i = 0; i < size; i++)
                        if (sm2->array->is_string[i]) {
                            free_vartype((vartype *) dm);
                            return ERR_ALPHA_DATA_IS_INVALID;
                        }
                    for (i = 0; i < size; i++) {
                        error = mcr(sm1->array->data[i * 2],
                                    sm1->array->data[i * 2 + 1],
                                    sm2->array->data[i],
                                    &dm->array->data[i * 2],
                                    &dm->array->data[i * 2 + 1]);
                        if (error != ERR_NONE) {
                            free_vartype((vartype *) dm);
                            return error;
                        }
                    }
                    *dst = (vartype *) dm;
                    return ERR_NONE;
                }
                case TYPE_COMPLEXMATRIX: {
                    vartype_complexmatrix *sm1 = (vartype_complexmatrix *) src1;
                    vartype_complexmatrix *sm2 = (vartype_complexmatrix *) src2;
                    vartype_complexmatrix *dm;
                    int4 size, i;
                    int error;
                    if (sm1->rows != sm2->rows || sm1->columns != sm2->columns)
                        return ERR_DIMENSION_ERROR;
                    dm = (vartype_complexmatrix *)
                                    new_complexmatrix(sm1->rows, sm1->columns);
                    if (dm == NULL)
                        return ERR_INSUFFICIENT_MEMORY;
                    size = 2 * sm1->rows * sm1->columns;
                    for (i = 0; i < size; i += 2) {
                        error = mcc(sm1->array->data[i],
                                    sm1->array->data[i + 1],
                                    sm2->array->data[i],
                                    sm2->array->data[i + 1],
                                    &dm->array->data[i],
                                    &dm->array->data[i + 1]);
                        if (error != ERR_NONE) {
                            free_vartype((vartype *) dm);
                            return error;
                        }
                    }
                    *dst = (vartype *) dm;
                    return ERR_NONE;
                }
                default:
                    return ERR_INTERNAL_ERROR;
            }
        default:
            return ERR_INTERNAL_ERROR;
    }
}

int div_rr(phloat x, phloat y, phloat *z) {
    phloat r;
    int inf;
    if (x == 0)
        return ERR_DIVIDE_BY_0;
    r = y / x;
    inf = p_isinf(r);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            r = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    *z = r;
    return ERR_NONE;
}
    
int div_rc(phloat x, phloat yre, phloat yim, phloat *zre, phloat *zim) {
    phloat rre, rim;
    int inf;
    if (x == 0)
        return ERR_DIVIDE_BY_0;
    rre = yre / x;
    inf = p_isinf(rre);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rre = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    rim = yim / x;
    inf = p_isinf(rim);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rim = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = rim;
    return ERR_NONE;
}

int div_cr(phloat xre, phloat xim, phloat y, phloat *zre, phloat *zim) {
    phloat rre, rim, h;
    int inf;
    /* TODO: overflows in intermediate results */
    h = xre * xre + xim * xim;
    if (h == 0)
        return ERR_DIVIDE_BY_0;
    rre = y * xre / h;
    inf = p_isinf(rre);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rre = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    rim = -y * xim / h;
    inf = p_isinf(rim);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rim = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = rim;
    return ERR_NONE;
}

int div_cc(phloat xre, phloat xim, phloat yre, phloat yim,
                                                    phloat *zre, phloat *zim) {
    phloat rre, rim, h;
    int inf;
    /* TODO: overflows in intermediate results */
    h = xre * xre + xim * xim;
    if (h == 0)
        return ERR_DIVIDE_BY_0;
    rre = (xre * yre + xim * yim) / h;
    inf = p_isinf(rre);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rre = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    rim = (xre * yim - yre * xim) / h;
    inf = p_isinf(rim);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rim = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = rim;
    return ERR_NONE;
}

int mul_rr(phloat x, phloat y, phloat *z) {
    phloat r;
    int inf;
    r = y * x;
    inf = p_isinf(r);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            r = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    *z = r;
    return ERR_NONE;
}
    
int mul_rc(phloat x, phloat yre, phloat yim, phloat *zre, phloat *zim) {
    phloat rre, rim;
    int inf;
    rre = yre * x;
    inf = p_isinf(rre);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rre = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    rim = yim * x;
    inf = p_isinf(rim);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rim = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = rim;
    return ERR_NONE;
}

int mul_cr(phloat xre, phloat xim, phloat y, phloat *zre, phloat *zim) {
    phloat rre, rim;
    int inf;
    rre = y * xre;
    inf = p_isinf(rre);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rre = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    rim = y * xim;
    inf = p_isinf(rim);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rim = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = rim;
    return ERR_NONE;
}

int mul_cc(phloat xre, phloat xim, phloat yre, phloat yim,
                                                    phloat *zre, phloat *zim) {
    phloat rre, rim;
    int inf;
    rre = xre * yre - xim * yim;
    inf = p_isinf(rre);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rre = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    rim = xre * yim + yre * xim;
    inf = p_isinf(rim);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rim = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = rim;
    return ERR_NONE;
}

int sub_rr(phloat x, phloat y, phloat *z) {
    phloat r;
    int inf;
    r = y - x;
    inf = p_isinf(r);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            r = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    *z = r;
    return ERR_NONE;
}
    
int sub_rc(phloat x, phloat yre, phloat yim, phloat *zre, phloat *zim) {
    phloat rre;
    int inf;
    rre = yre - x;
    inf = p_isinf(rre);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rre = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = yim;
    return ERR_NONE;
}

int sub_cr(phloat xre, phloat xim, phloat y, phloat *zre, phloat *zim) {
    phloat rre;
    int inf;
    rre = y - xre;
    inf = p_isinf(rre);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rre = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = -xim;
    return ERR_NONE;
}

int sub_cc(phloat xre, phloat xim, phloat yre, phloat yim,
                                                    phloat *zre, phloat *zim) {
    phloat rre, rim;
    int inf;
    rre = yre - xre;
    inf = p_isinf(rre);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rre = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    rim = yim - xim;
    inf = p_isinf(rim);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rim = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = rim;
    return ERR_NONE;
}

int add_rr(phloat x, phloat y, phloat *z) {
    phloat r;
    int inf;
    r = y + x;
    inf = p_isinf(r);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            r = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    *z = r;
    return ERR_NONE;
}
    
int add_rc(phloat x, phloat yre, phloat yim, phloat *zre, phloat *zim) {
    phloat rre;
    int inf;
    rre = yre + x;
    inf = p_isinf(rre);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rre = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = yim;
    return ERR_NONE;
}

int add_cr(phloat xre, phloat xim, phloat y, phloat *zre, phloat *zim) {
    phloat rre;
    int inf;
    rre = y + xre;
    inf = p_isinf(rre);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rre = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = xim;
    return ERR_NONE;
}

int add_cc(phloat xre, phloat xim, phloat yre, phloat yim,
                                                    phloat *zre, phloat *zim) {
    phloat rre, rim;
    int inf;
    rre = yre + xre;
    inf = p_isinf(rre);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rre = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    rim = yim + xim;
    inf = p_isinf(rim);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rim = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = rim;
    return ERR_NONE;
}
