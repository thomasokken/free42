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

#include "core_commands2.h"
#include "core_commands3.h"
#include "core_commands4.h"
#include "core_display.h"
#include "core_helpers.h"
#include "core_linalg1.h"
#include "core_math2.h"
#include "core_sto_rcl.h"
#include "core_variables.h"

/********************************************************/
/* Implementations of HP-42S built-in functions, part 4 */
/********************************************************/

int docmd_insr(arg_struct *arg) {
    vartype *m, *newx;
    vartype_realmatrix *rm;
    vartype_complexmatrix *cm;
    vartype_list *list;
    int4 rows, columns, i;
    int err, refcount;
    int interactive;

    switch (matedit_mode) {
        case 0:
            return ERR_NONEXISTENT;
        case 1:
        case 3:
            m = recall_var(matedit_name, matedit_length);
            break;
        case 2:
            m = matedit_x;
            break;
        default:
            return ERR_INTERNAL_ERROR;
    }
    if (m == NULL)
        return ERR_NONEXISTENT;
    if (m->type != TYPE_REALMATRIX
            && m->type != TYPE_COMPLEXMATRIX
            && m->type != TYPE_LIST)
        return ERR_INVALID_TYPE;

    interactive = matedit_mode == 2 || matedit_mode == 3;
    if (interactive && sp != -1) {
        err = docmd_stoel(NULL);
        if (err != ERR_NONE)
            return err;
    }

    if (m->type == TYPE_REALMATRIX) {
        rm = (vartype_realmatrix *) m;
        rows = rm->rows;
        columns = rm->columns;
        refcount = rm->array->refcount;
        if (interactive) {
            newx = new_real(0);
            if (newx == NULL)
                return ERR_INSUFFICIENT_MEMORY;
        }
    } else if (m->type == TYPE_COMPLEXMATRIX) {
        cm = (vartype_complexmatrix *) m;
        rows = cm->rows;
        columns = cm->columns;
        refcount = cm->array->refcount;
        if (interactive) {
            newx = new_complex(0, 0);
            if (newx == NULL)
                return ERR_INSUFFICIENT_MEMORY;
        }
    } else {
        list = (vartype_list *) m;
        rows = list->size;
        columns = 1;
        refcount = list->array->refcount;
        if (interactive) {
            newx = new_real(0);
            if (newx == NULL)
                return ERR_INSUFFICIENT_MEMORY;
        }
    }

    if (matedit_i >= rows)
        matedit_i = rows - 1;
    if (matedit_j >= columns)
        matedit_j = columns - 1;

    if (refcount == 1) {
        /* We have this array to ourselves so we can modify it in place */
        err = dimension_array_ref(m, rows + 1, columns);
        if (err != ERR_NONE) {
            if (interactive)
                free_vartype(newx);
            return err;
        }
        rows++;
        if (m->type == TYPE_REALMATRIX) {
            for (i = rows * columns - 1; i >= (matedit_i + 1) * columns; i--) {
                rm->array->is_string[i] = rm->array->is_string[i - columns];
                rm->array->data[i] = rm->array->data[i - columns];
            }
            for (i = matedit_i * columns; i < (matedit_i + 1) * columns; i++) {
                rm->array->is_string[i] = 0;
                rm->array->data[i] = 0;
            }
        } else if (m->type == TYPE_COMPLEXMATRIX) {
            for (i = 2 * rows * columns - 1;
                            i >= 2 * (matedit_i + 1) * columns; i--)
                cm->array->data[i] = cm->array->data[i - 2 * columns];
            for (i = 2 * matedit_i * columns;
                            i < 2 * (matedit_i + 1) * columns; i++)
                cm->array->data[i] = 0;
        } else {
            vartype *v = new_real(0);
            if (v == NULL) {
                free_vartype(newx);
                return ERR_INSUFFICIENT_MEMORY;
            }
            memmove(list->array->data + matedit_i + 1, list->array->data + matedit_i, (rows - matedit_i - 1) * sizeof(vartype *));
            list->array->data[matedit_i] = v;
        }
    } else {
        /* Make sure the new array is less than 2 GB,
         * so it's addressable with a signed 32-bit index */
        int esize;
        switch (m->type) {
            case TYPE_REALMATRIX: esize = sizeof(phloat); break;
            case TYPE_COMPLEXMATRIX: esize = 2 * sizeof(phloat); break;
            case TYPE_LIST: esize = sizeof(vartype *); break;
        }
        double d_bytes = ((double) (rows + 1)) * ((double) columns) * esize;
        if (((double) (int4) d_bytes) != d_bytes)
            return ERR_INSUFFICIENT_MEMORY;

        /* We're sharing this array. I don't use disentangle() because it
         * does not deal with resizing. */
        int4 newsize = (rows + 1) * columns;
        if (m->type == TYPE_REALMATRIX) {
            realmatrix_data *array = (realmatrix_data *)
                                malloc(sizeof(realmatrix_data));
            if (array == NULL) {
                if (interactive)
                    free_vartype(newx);
                return ERR_INSUFFICIENT_MEMORY;
            }
            array->data = (phloat *) malloc(newsize * sizeof(phloat));
            if (array->data == NULL) {
                if (interactive)
                    free_vartype(newx);
                free(array);
                return ERR_INSUFFICIENT_MEMORY;
            }
            array->is_string = (char *) malloc(newsize);
            if (array->is_string == NULL) {
                if (interactive)
                    free_vartype(newx);
                free(array->data);
                free(array);
                return ERR_INSUFFICIENT_MEMORY;
            }
            for (i = 0; i < matedit_i * columns; i++) {
                array->is_string[i] = rm->array->is_string[i];
                array->data[i] = rm->array->data[i];
            }
            for (i = matedit_i * columns; i < (matedit_i + 1) * columns; i++) {
                array->is_string[i] = 0;
                array->data[i] = 0;
            }
            for (i = (matedit_i + 1) * columns; i < newsize; i++) {
                array->is_string[i] = rm->array->is_string[i - columns];
                array->data[i] = rm->array->data[i - columns];
            }
            array->refcount = 1;
            rm->array->refcount--;
            rm->array = array;
            rm->rows++;
        } else if (m->type == TYPE_COMPLEXMATRIX) {
            complexmatrix_data *array = (complexmatrix_data *)
                                malloc(sizeof(complexmatrix_data));
            if (array == NULL) {
                if (interactive)
                    free_vartype(newx);
                return ERR_INSUFFICIENT_MEMORY;
            }
            array->data = (phloat *) malloc(2 * newsize * sizeof(phloat));
            if (array->data == NULL) {
                if (interactive)
                    free_vartype(newx);
                free(array);
                return ERR_INSUFFICIENT_MEMORY;
            }
            for (i = 0; i < 2 * matedit_i * columns; i++)
                array->data[i] = cm->array->data[i];
            for (i = 2 * matedit_i * columns;
                                i < 2 * (matedit_i + 1) * columns; i++)
                array->data[i] = 0;
            for (i = 2 * (matedit_i + 1) * columns; i < 2 * newsize; i++)
                array->data[i] = cm->array->data[i - 2 * columns];
            array->refcount = 1;
            cm->array->refcount--;
            cm->array = array;
            cm->rows++;
        } else {
            list_data *array = (list_data *) malloc(sizeof(list_data));
            if (array == NULL) {
                if (interactive)
                    free_vartype(newx);
                return ERR_INSUFFICIENT_MEMORY;
            }
            array->data = (vartype **) malloc(newsize * sizeof(vartype *));
            if (array->data == NULL) {
                if (interactive)
                    free_vartype(newx);
                free(array);
                return ERR_INSUFFICIENT_MEMORY;
            }
            for (int4 i = 0; i < newsize; i++) {
                array->data[i] = i < matedit_i ? dup_vartype(list->array->data[i])
                               : i > matedit_i ? dup_vartype(list->array->data[i - 1])
                               : new_real(0);
                if (array->data[i] == NULL) {
                    for (int4 j = 0; j < i; j++)
                        free_vartype(array->data[j]);
                    if (interactive)
                        free_vartype(newx);
                    free(array->data);
                    free(array);
                    return ERR_INSUFFICIENT_MEMORY;
                }
            }
            array->refcount = 1;
            list->array->refcount--;
            list->array = array;
            list->size++;
        }
    }
    if (interactive) {
        if (sp == -1)
            sp = 0;
        else
            free_vartype(stack[sp]);
        stack[sp] = newx;
    }
    mode_disable_stack_lift = true;
    return ERR_NONE;
}

static void invrt_completion(int error, vartype *res) {
    if (error == ERR_NONE)
        unary_result(res);
}

int docmd_invrt(arg_struct *arg) {
    return linalg_inv(stack[sp], invrt_completion);
}

int docmd_j_add(arg_struct *arg) {
    int4 rows, columns;
    int4 oldi = matedit_i;
    int4 oldj = matedit_j;
    int err = matedit_get_dim(&rows, &columns);
    if (err != ERR_NONE)
        return err;
    if (++matedit_j >= columns) {
        flags.f.matrix_edge_wrap = 1;
        matedit_j = 0;
        if (++matedit_i >= rows) {
            flags.f.matrix_end_wrap = 1;
            if (flags.f.grow) {
                if (matedit_mode == 2)
                    err = dimension_array_ref(matedit_x, rows + 1, columns);
                else
                    err = dimension_array(matedit_name, matedit_length,
                                            rows + 1, columns, false);
                if (err != ERR_NONE) {
                    matedit_i = oldi;
                    matedit_j = oldj;
                    return err;
                }
                matedit_i = rows;
            } else
                matedit_i = 0;
        } else
            flags.f.matrix_end_wrap = 0;
    } else {
        flags.f.matrix_edge_wrap = 0;
        flags.f.matrix_end_wrap = 0;
    }
    return ERR_NONE;
}

int docmd_j_sub(arg_struct *arg) {
    int4 rows, columns;
    int err = matedit_get_dim(&rows, &columns);
    if (err != ERR_NONE)
        return err;
    if (--matedit_j < 0) {
        flags.f.matrix_edge_wrap = 1;
        matedit_j = columns - 1;
        if (--matedit_i < 0) {
            flags.f.matrix_end_wrap = 1;
            matedit_i = rows - 1;
        } else
            flags.f.matrix_end_wrap = 0;
    } else {
        flags.f.matrix_edge_wrap = 0;
        flags.f.matrix_end_wrap = 0;
    }
    return ERR_NONE;
}

static int mappable_ln_1_x(phloat x, phloat *y) {
    if (x <= -1)
        return ERR_INVALID_DATA;
    *y = log1p(x);
    return ERR_NONE;
}

int docmd_ln_1_x(arg_struct *arg) {
    vartype *v;
    int err = map_unary(stack[sp], &v, mappable_ln_1_x, NULL);
    if (err == ERR_NONE)
        unary_result(v);
    return err;
}

int docmd_posa(arg_struct *arg) {
    int pos = string_pos(reg_alpha, reg_alpha_length, stack[sp], 0);
    if (pos == -2)
        return ERR_INVALID_DATA;
    vartype *v = new_real(pos);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

int docmd_putm(arg_struct *arg) {
    vartype *m;
    int4 i, j;

    switch (matedit_mode) {
        case 0:
            return ERR_NONEXISTENT;
        case 1:
        case 3:
            m = recall_var(matedit_name, matedit_length);
            break;
        case 2:
            m = matedit_x;
            break;
        default:
            return ERR_INTERNAL_ERROR;
    }

    if (m == NULL)
        return ERR_NONEXISTENT;

    if (m->type != TYPE_REALMATRIX && m->type != TYPE_COMPLEXMATRIX)
        /* Shouldn't happen, but could, as long as I don't
         * implement matrix locking
         */
        return ERR_INVALID_TYPE;

    if (stack[sp]->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else if (stack[sp]->type == TYPE_REAL || stack[sp]->type == TYPE_COMPLEX)
        return ERR_INVALID_TYPE;

    if (m->type == TYPE_REALMATRIX) {
        vartype_realmatrix *src, *dst;
        if (stack[sp]->type == TYPE_COMPLEXMATRIX)
            return ERR_INVALID_TYPE;
        src = (vartype_realmatrix *) stack[sp];
        dst = (vartype_realmatrix *) m;
        if (src->rows + matedit_i > dst->rows
                || src->columns + matedit_j > dst->columns)
            return ERR_DIMENSION_ERROR;
        /* Duplicate and disentangle the source matrix, in order to get
         * a deep copy with all the long strings replicated; it
         * simplifies the logic by making rollbacks easier, and it
         * enables some nice sleight-of-hand for cleaning up the
         * overwritten entries in the destination as well.
         */
        vartype *v = dup_vartype((vartype *) src);
        if (v == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        if (!disentangle(v) || !disentangle(m)) {
            free_vartype(v);
            return ERR_INSUFFICIENT_MEMORY;
        }
        src = (vartype_realmatrix *) v;
        for (i = 0; i < src->rows; i++)
            for (j = 0; j < src->columns; j++) {
                int4 n1 = i * src->columns + j;
                int4 n2 = (i + matedit_i) * dst->columns + j + matedit_j;
                char tc = dst->array->is_string[n2];
                dst->array->is_string[n2] = src->array->is_string[n1];
                src->array->is_string[n1] = tc;
                phloat tp = dst->array->data[n2];
                dst->array->data[n2] = src->array->data[n1];
                src->array->data[n1] = tp;
            }
        free_vartype(v);
        return ERR_NONE;
    } else if (stack[sp]->type == TYPE_REALMATRIX) {
        vartype_realmatrix *src = (vartype_realmatrix *) stack[sp];
        vartype_complexmatrix *dst = (vartype_complexmatrix *) m;
        if (src->rows + matedit_i > dst->rows
                || src->columns + matedit_j > dst->columns)
            return ERR_DIMENSION_ERROR;
        if (contains_strings(src))
            return ERR_ALPHA_DATA_IS_INVALID;
        if (!disentangle(m))
            return ERR_INSUFFICIENT_MEMORY;
        for (i = 0; i < src->rows; i++)
            for (j = 0; j < src->columns; j++) {
                int4 n1 = i * src->columns + j;
                int4 n2 = (i + matedit_i) * dst->columns + j + matedit_j;
                dst->array->data[n2 * 2] = src->array->data[n1];
                dst->array->data[n2 * 2 + 1] = 0;
            }
        return ERR_NONE;
    } else {
        vartype_complexmatrix *src = (vartype_complexmatrix *) stack[sp];
        vartype_complexmatrix *dst = (vartype_complexmatrix *) m;
        if (src->rows + matedit_i > dst->rows
                || src->columns + matedit_j > dst->columns)
            return ERR_DIMENSION_ERROR;
        if (!disentangle(m))
            return ERR_INSUFFICIENT_MEMORY;
        for (i = 0; i < src->rows; i++)
            for (j = 0; j < src->columns; j++) {
                int4 n1 = i * src->columns + j;
                int4 n2 = (i + matedit_i) * dst->columns + j + matedit_j;
                dst->array->data[n2 * 2] = src->array->data[n1 * 2];
                dst->array->data[n2 * 2 + 1] = src->array->data[n1 * 2 + 1];
            }
        return ERR_NONE;
    }
}

int docmd_rclel(arg_struct *arg) {
    vartype *m, *v;
    switch (matedit_mode) {
        case 0:
            return ERR_NONEXISTENT;
        case 1:
        case 3:
            m = recall_var(matedit_name, matedit_length);
            break;
        case 2:
            m = matedit_x;
            break;
        default:
            return ERR_INTERNAL_ERROR;
    }

    if (m == NULL)
        return ERR_NONEXISTENT;

    if (m->type == TYPE_REALMATRIX) {
        vartype_realmatrix *rm = (vartype_realmatrix *) m;
        int4 n = matedit_i * rm->columns + matedit_j;
        if (rm->array->is_string[n] != 0) {
            char *text;
            int4 length;
            get_matrix_string(rm, n, &text, &length);
            v = new_string(text, length);
        } else
            v = new_real(rm->array->data[n]);
    } else if (m->type == TYPE_COMPLEXMATRIX) {
        vartype_complexmatrix *cm = (vartype_complexmatrix *) m;
        int4 n = matedit_i * cm->columns + matedit_j;
        v = new_complex(cm->array->data[2 * n],
                        cm->array->data[2 * n + 1]);
    } else if (m->type == TYPE_LIST) {
        vartype_list *list = (vartype_list *) m;
        v = dup_vartype(list->array->data[matedit_i]);
    } else
        return ERR_INVALID_TYPE;
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    return recall_result(v);
}

int docmd_rclij(arg_struct *arg) {
    vartype *i, *j;
    if (matedit_mode == 0)
        return ERR_NONEXISTENT;
    i = new_real(matedit_i + 1);
    j = new_real(matedit_j + 1);
    if (i == NULL || j == NULL) {
        free_vartype(i);
        free_vartype(j);
        return ERR_INSUFFICIENT_MEMORY;
    }
    return recall_two_results(j, i);
}

int docmd_rnrm(arg_struct *arg) {
    if (stack[sp]->type == TYPE_REALMATRIX) {
        vartype_realmatrix *rm = (vartype_realmatrix *) stack[sp];
        if (contains_strings(rm))
            return ERR_ALPHA_DATA_IS_INVALID;
        phloat max = 0;
        for (int4 i = 0; i < rm->rows; i++) {
            phloat nrm = 0;
            for (int4 j = 0; j < rm->columns; j++) {
                phloat x = rm->array->data[i * rm->columns + j];
                if (x >= 0)
                    nrm += x;
                else
                    nrm -= x;
            }
            if (p_isinf(nrm)) {
                if (flags.f.range_error_ignore)
                    max = POS_HUGE_PHLOAT;
                else
                    return ERR_OUT_OF_RANGE;
                break;
            }
            if (nrm > max)
                max = nrm;
        }
        vartype *v = new_real(max);
        if (v == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        unary_result(v);
        return ERR_NONE;
    } else {
        vartype *v;
        vartype_complexmatrix *cm = (vartype_complexmatrix *) stack[sp];
        int4 i, j;
        phloat max = 0;
        for (i = 0; i < cm->rows; i++) {
            phloat nrm = 0;
            for (j = 0; j < cm->columns; j++) {
                phloat re = cm->array->data[2 * (i * cm->columns + j)];
                phloat im = cm->array->data[2 * (i * cm->columns + j) + 1];
                nrm += hypot(re, im);
            }
            if (p_isinf(nrm)) {
                if (flags.f.range_error_ignore)
                    max = POS_HUGE_PHLOAT;
                else
                    return ERR_OUT_OF_RANGE;
                break;
            }
            if (nrm > max)
                max = nrm;
        }
        v = new_real(max);
        if (v == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        unary_result(v);
        return ERR_NONE;
    }
}

int docmd_rsum(arg_struct *arg) {
    if (stack[sp]->type == TYPE_REALMATRIX) {
        vartype_realmatrix *rm = (vartype_realmatrix *) stack[sp];
        if (contains_strings(rm))
            return ERR_ALPHA_DATA_IS_INVALID;
        vartype_realmatrix *res;
        res = (vartype_realmatrix *) new_realmatrix(rm->rows, 1);
        if (res == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        for (int4 i = 0; i < rm->rows; i++) {
            phloat sum = 0;
            int inf;
            for (int4 j = 0; j < rm->columns; j++)
                sum += rm->array->data[i * rm->columns + j];
            if ((inf = p_isinf(sum)) != 0) {
                if (flags.f.range_error_ignore)
                    sum = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
                else {
                    free_vartype((vartype *) res);
                    return ERR_OUT_OF_RANGE;
                }
            }
            res->array->data[i] = sum;
        }
        unary_result((vartype *) res);
        return ERR_NONE;
    } else if (stack[sp]->type == TYPE_COMPLEXMATRIX) {
        vartype_complexmatrix *cm = (vartype_complexmatrix *) stack[sp];
        vartype_complexmatrix *res;
        int4 i, j;
        res = (vartype_complexmatrix *) new_complexmatrix(cm->rows, 1);
        if (res == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        for (i = 0; i < cm->rows; i++) {
            phloat sum_re = 0, sum_im = 0;
            int inf;
            for (j = 0; j < cm->columns; j++) {
                sum_re += cm->array->data[2 * (i * cm->columns + j)];
                sum_im += cm->array->data[2 * (i * cm->columns + j) + 1];
            }
            if ((inf = p_isinf(sum_re)) != 0) {
                if (flags.f.range_error_ignore)
                    sum_re = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
                else {
                    free_vartype((vartype *) res);
                    return ERR_OUT_OF_RANGE;
                }
            }
            if ((inf = p_isinf(sum_im)) != 0) {
                if (flags.f.range_error_ignore)
                    sum_im = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
                else {
                    free_vartype((vartype *) res);
                    return ERR_OUT_OF_RANGE;
                }
            }
            res->array->data[2 * i] = sum_re;
            res->array->data[2 * i + 1] = sum_im;
        }
        unary_result((vartype *) res);
        return ERR_NONE;
    } else if (stack[sp]->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;
}

int docmd_swap_r(arg_struct *arg) {
    vartype *m;
    phloat xx, yy;
    int4 x, y, i;

    switch (matedit_mode) {
        case 0:
            return ERR_NONEXISTENT;
        case 1:
        case 3:
            m = recall_var(matedit_name, matedit_length);
            break;
        case 2:
            m = matedit_x;
            break;
        default:
            return ERR_INTERNAL_ERROR;
    }

    if (m == NULL)
        return ERR_NONEXISTENT;

    if (m->type != TYPE_REALMATRIX && m->type != TYPE_COMPLEXMATRIX)
        /* Should not happen, but could, as long as I don't implement
         * matrix locking. */
        return ERR_INVALID_TYPE;

    if (stack[sp]->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    if (stack[sp]->type != TYPE_REAL)
        return ERR_INVALID_TYPE;
    if (stack[sp - 1]->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    if (stack[sp - 1]->type != TYPE_REAL)
        return ERR_INVALID_TYPE;

    xx = ((vartype_real *) stack[sp])->x;
    if (xx <= -2147483648.0 || xx >= 2147483648.0)
        return ERR_DIMENSION_ERROR;
    x = to_int4(xx);
    if (x == 0)
        return ERR_DIMENSION_ERROR;
    if (x < 0)
        x = -x;
    x--;

    yy = ((vartype_real *) stack[sp - 1])->x;
    if (yy <= -2147483648.0 || yy >= 2147483648.0)
        return ERR_DIMENSION_ERROR;
    y = to_int4(yy);
    if (y == 0)
        return ERR_DIMENSION_ERROR;
    if (y < 0)
        y = -y;
    y--;

    if (m->type == TYPE_REALMATRIX) {
        vartype_realmatrix *rm = (vartype_realmatrix *) m;
        if (x >= rm->rows || y >= rm->rows)
            return ERR_DIMENSION_ERROR;
        else if (x == y)
            return ERR_NONE;
        if (!disentangle(m))
            return ERR_INSUFFICIENT_MEMORY;
        for (i = 0; i < rm->columns; i++) {
            int4 n1 = x * rm->columns + i;
            int4 n2 = y * rm->columns + i;
            char tempc = rm->array->is_string[n1];
            phloat tempds = rm->array->data[n1];
            rm->array->is_string[n1] = rm->array->is_string[n2];
            rm->array->data[n1] = rm->array->data[n2];
            rm->array->is_string[n2] = tempc;
            rm->array->data[n2] = tempds;
        }
        return ERR_NONE;
    } else /* m->type == TYPE_COMPLEXMATRIX */ {
        vartype_complexmatrix *cm = (vartype_complexmatrix *) m;
        if (x >= cm->rows || y >= cm->rows)
            return ERR_DIMENSION_ERROR;
        else if (x == y)
            return ERR_NONE;
        if (!disentangle(m))
            return ERR_INSUFFICIENT_MEMORY;
        for (i = 0; i < 2 * cm->columns; i++) {
            int4 n1 = x * 2 * cm->columns + i;
            int4 n2 = y * 2 * cm->columns + i;
            phloat tempd = cm->array->data[n1];
            cm->array->data[n1] = cm->array->data[n2];
            cm->array->data[n2] = tempd;
        }
        return ERR_NONE;
    }
}

static int mappable_sinh_r(phloat x, phloat *y) {
    int inf;
    *y = sinh(x);
    if ((inf = p_isinf(*y)) != 0) {
        if (flags.f.range_error_ignore)
            *y = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    return ERR_NONE;
}

static int mappable_sinh_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    if (xim == 0) {
        *yre = sinh(xre);
        *yim = 0;
        return ERR_NONE;
    } else if (xre == 0) {
        *yre = 0;
        *yim = sin(xim);
        return ERR_NONE;
    }
    phloat sinhxre, coshxre;
    phloat sinxim, cosxim;
    int inf;
    sinhxre = sinh(xre);
    coshxre = cosh(xre);
    p_sincos(xim, &sinxim, &cosxim);
    *yre = sinhxre * cosxim;
    if ((inf = p_isinf(*yre)) != 0) {
        if (flags.f.range_error_ignore)
            *yre = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    *yim = coshxre * sinxim;
    if ((inf = p_isinf(*yim)) != 0) {
        if (flags.f.range_error_ignore)
            *yim = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    return ERR_NONE;
}

int docmd_sinh(arg_struct *arg) {
    vartype *v;
    int err = map_unary(stack[sp], &v, mappable_sinh_r, mappable_sinh_c);
    if (err == ERR_NONE)
        unary_result(v);
    return err;
}

int docmd_stoel(arg_struct *arg) {
    vartype *m;
    switch (matedit_mode) {
        case 0:
            return ERR_NONEXISTENT;
        case 1:
        case 3:
            m = recall_var(matedit_name, matedit_length);
            break;
        case 2:
            m = matedit_x;
            break;
        default:
            return ERR_INTERNAL_ERROR;
    }

    if (m == NULL)
        return ERR_NONEXISTENT;

    if (m->type != TYPE_REALMATRIX
            && m->type != TYPE_COMPLEXMATRIX
            && m->type != TYPE_LIST)
        /* Should not happen, but could, as long as I don't implement
         * matrix locking.
         */
        return ERR_INVALID_TYPE;

    if (!disentangle(m))
        return ERR_INSUFFICIENT_MEMORY;

    if (m->type == TYPE_REALMATRIX) {
        vartype_realmatrix *rm = (vartype_realmatrix *) m;
        int4 n = matedit_i * rm->columns + matedit_j;
        if (stack[sp]->type == TYPE_REAL) {
            if (rm->array->is_string[n] == 2)
                free(*(void **) &rm->array->data[n]);
            rm->array->is_string[n] = 0;
            rm->array->data[n] = ((vartype_real *) stack[sp])->x;
            return ERR_NONE;
        } else if (stack[sp]->type == TYPE_STRING) {
            vartype_string *s = (vartype_string *) stack[sp];
            if (!put_matrix_string(rm, n, s->txt(), s->length))
                return ERR_INSUFFICIENT_MEMORY;
            return ERR_NONE;
        } else
            return ERR_INVALID_TYPE;
    } else if (m->type == TYPE_COMPLEXMATRIX) {
        vartype_complexmatrix *cm = (vartype_complexmatrix *) m;
        int4 n = matedit_i * cm->columns + matedit_j;
        if (stack[sp]->type == TYPE_REAL) {
            cm->array->data[2 * n] = ((vartype_real *) stack[sp])->x;
            cm->array->data[2 * n + 1] = 0;
            return ERR_NONE;
        } else if (stack[sp]->type == TYPE_COMPLEX) {
            vartype_complex *c = (vartype_complex *) stack[sp];
            cm->array->data[2 * n] = c->re;
            cm->array->data[2 * n + 1] = c->im;
            return ERR_NONE;
        } else if (stack[sp]->type == TYPE_STRING)
            return ERR_ALPHA_DATA_IS_INVALID;
        else
            return ERR_INVALID_TYPE;
    } else /* m->type == TYPE_LIST */ {
        vartype_list *list = (vartype_list *) m;
        vartype *v = dup_vartype(stack[sp]);
        if (v == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        free_vartype(list->array->data[matedit_i]);
        list->array->data[matedit_i] = v;
        return ERR_NONE;
    }
}

int docmd_stoij(arg_struct *arg) {
    vartype *m;
    phloat x, y;
    int4 i, j;

    switch (matedit_mode) {
        case 0:
            return ERR_NONEXISTENT;
        case 1:
        case 3:
            m = recall_var(matedit_name, matedit_length);
            break;
        case 2:
            m = matedit_x;
            break;
        default:
            return ERR_INTERNAL_ERROR;
    }

    if (m == NULL)
        return ERR_NONEXISTENT;

    if (stack[sp]->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    if (stack[sp]->type != TYPE_REAL)
        return ERR_INVALID_TYPE;
    if (stack[sp - 1]->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    if (stack[sp - 1]->type != TYPE_REAL)
        return ERR_INVALID_TYPE;

    x = ((vartype_real *) stack[sp])->x;
    if (x <= -2147483648.0 || x >= 2147483648.0)
        return ERR_DIMENSION_ERROR;
    j = to_int4(x);
    if (j < 0)
        j = -j;

    y = ((vartype_real *) stack[sp - 1])->x;
    if (y <= -2147483648.0 || y >= 2147483648.0)
        return ERR_DIMENSION_ERROR;
    i = to_int4(y);
    if (i < 0)
        i = -i;

    if (m->type == TYPE_REALMATRIX) {
        vartype_realmatrix *rm = (vartype_realmatrix *) m;
        if (i == 0 || i > rm->rows || j == 0 || j > rm->columns)
            return ERR_DIMENSION_ERROR;
    } else if (m->type == TYPE_COMPLEXMATRIX) {
        vartype_complexmatrix *cm = (vartype_complexmatrix *) m;
        if (i == 0 || i > cm->rows || j == 0 || j > cm->columns)
            return ERR_DIMENSION_ERROR;
    } else if (m->type == TYPE_LIST) {
        vartype_list *list = (vartype_list *) m;
        if (i == 0 || i > list->size || j != 1)
            return ERR_DIMENSION_ERROR;
    } else
        /* Should not happen, but could, as long as I don't implement
         * matrix locking. */
        return ERR_INVALID_TYPE;

    matedit_i = i - 1;
    matedit_j = j - 1;
    return ERR_NONE;
}

static int mappable_tanh_r(phloat x, phloat *y) {
    *y = tanh(x);
    return ERR_NONE;
}

static int mappable_tanh_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    /* NOTE: DEG/RAD/GRAD mode does not apply here. */

    if (xre == 0) {
        *yre = 0;
        return math_tan(xim, yim, true);
    } else if (xim == 0) {
        *yim = 0;
        *yre = tanh(xre);
        return ERR_NONE;
    }
    if (p_isnan(xim) || p_isnan(xre)) {
        *yim = NAN_PHLOAT;
        *yre = NAN_PHLOAT;
        return ERR_NONE;
    }

    phloat sinxim, cosxim;
    p_sincos(xim, &sinxim, &cosxim);
    phloat sinhxre = sinh(xre);
    phloat coshxre = cosh(xre);
    phloat d = cosxim * cosxim + sinhxre * sinhxre;
    if (d == 0) {
        if (flags.f.range_error_ignore) {
            *yim = POS_HUGE_PHLOAT;
            *yre = POS_HUGE_PHLOAT;
            return ERR_NONE;
        } else
            return ERR_OUT_OF_RANGE;
    }
    if (p_isinf(d) != 0) {
        *yim = 0;
        *yre = xre < 0 ? -1 : 1;
        return ERR_NONE;
    }
    *yim = sinxim * cosxim / d;
    *yre = sinhxre * coshxre / d;
    return ERR_NONE;
}

int docmd_tanh(arg_struct *arg) {
    vartype *v;
    int err = map_unary(stack[sp], &v, mappable_tanh_r, mappable_tanh_c);
    if (err == ERR_NONE)
        unary_result(v);
    return err;
}

int docmd_trans(arg_struct *arg) {
    if (stack[sp]->type == TYPE_REALMATRIX) {
        vartype_realmatrix *src = (vartype_realmatrix *) stack[sp];
        vartype_realmatrix *dst;
        int4 rows = src->rows;
        int4 columns = src->columns;
        int4 i, j;
        dst = (vartype_realmatrix *) new_realmatrix(columns, rows);
        if (dst == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        for (i = 0; i < rows; i++)
            for (j = 0; j < columns; j++) {
                int4 n1 = i * columns + j;
                int4 n2 = j * rows + i;
                dst->array->is_string[n2] = src->array->is_string[n1];
                if (dst->array->is_string[n2] == 2) {
                    int4 *sp = *(int4 **) &src->array->data[n1];
                    int4 *dp = (int4 *) malloc(*sp + 4);
                    if (dp == NULL) {
                        free_vartype((vartype *) dst);
                        return ERR_INSUFFICIENT_MEMORY;
                    }
                    memcpy(dp, sp, *sp + 4);
                    *(int4 **) &dst->array->data[n2] = dp;
                } else
                    dst->array->data[n2] = src->array->data[n1];
            }
        unary_result((vartype *) dst);
        return ERR_NONE;
    } else {
        vartype_complexmatrix *src = (vartype_complexmatrix *) stack[sp];
        vartype_complexmatrix *dst;
        int4 rows = src->rows;
        int4 columns = src->columns;
        int4 i, j;
        dst = (vartype_complexmatrix *) new_complexmatrix(columns, rows);
        if (dst == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        for (i = 0; i < rows; i++)
            for (j = 0; j < columns; j++) {
                int4 n1 = 2 * (i * columns + j);
                int4 n2 = 2 * (j * rows + i);
                dst->array->data[n2] = src->array->data[n1];
                dst->array->data[n2 + 1] = src->array->data[n1 + 1];
            }
        unary_result((vartype *) dst);
        return ERR_NONE;
    }
}

int docmd_wrap(arg_struct *arg) {
    flags.f.grow = 0;
    return ERR_NONE;
}

int docmd_x_swap(arg_struct *arg) {
    vartype *v;
    int err = generic_rcl(arg, &v);
    if (err != ERR_NONE)
        return err;
    err = generic_sto(arg, 0);
    if (err != ERR_NONE)
        free_vartype(v);
    else {
        free_vartype(stack[sp]);
        stack[sp] = v;
        print_trace();
    }
    return err;
}

#define DIR_LEFT  0
#define DIR_RIGHT 1
#define DIR_UP    2
#define DIR_DOWN  3

static int matedit_move(int direction) {
    vartype *m, *v;
    vartype_realmatrix *rm;
    vartype_complexmatrix *cm;
    vartype_list *list;
    int4 rows, columns, new_i, new_j, old_n, new_n;
    int edge_flag = 0;
    int end_flag = 0;

    switch (matedit_mode) {
        case 0:
            return ERR_NONEXISTENT;
        case 1:
        case 3:
            m = recall_var(matedit_name, matedit_length);
            break;
        case 2:
            m = matedit_x;
            break;
        default:
            return ERR_INTERNAL_ERROR;
    }

    if (m == NULL)
        return ERR_NONEXISTENT;

    if (m->type == TYPE_REALMATRIX) {
        rm = (vartype_realmatrix *) m;
        rows = rm->rows;
        columns = rm->columns;
    } else if (m->type == TYPE_COMPLEXMATRIX) {
        cm = (vartype_complexmatrix *) m;
        rows = cm->rows;
        columns = cm->columns;
    } else if (m->type == TYPE_LIST) {
        list = (vartype_list *) m;
        rows = list->size;
        columns = 1;
    } else
        return ERR_INVALID_TYPE;

    if (!disentangle(m))
        return ERR_INSUFFICIENT_MEMORY;

    new_i = matedit_i;
    new_j = matedit_j;
    switch (direction) {
        case DIR_LEFT:
            if (--new_j < 0) {
                edge_flag = 1;
                new_j = columns - 1;
                if (--new_i < 0) {
                    end_flag = 1;
                    new_i = rows - 1;
                }
            }
            break;
        case DIR_RIGHT:
            if (++new_j >= columns) {
                edge_flag = 1;
                new_j = 0;
                if (++new_i >= rows) {
                    end_flag = 1;
                    if (flags.f.grow) {
                        int err;
                        if (matedit_mode == 2)
                            err = dimension_array_ref(matedit_x,
                                                      rows + 1, columns);
                        else
                            err = dimension_array(matedit_name, matedit_length,
                                                  rows + 1, columns, false);
                        if (err != ERR_NONE)
                            return err;
                        new_i = rows++;
                    } else
                        new_i = 0;
                }
            }
            break;
        case DIR_UP:
            if (--new_i < 0) {
                edge_flag = 1;
                new_i = rows - 1;
                if (--new_j < 0) {
                    end_flag = 1;
                    new_j = columns - 1;
                }
            }
            break;
        case DIR_DOWN:
            if (++new_i >= rows) {
                edge_flag = 1;
                new_i = 0;
                if (++new_j >= columns) {
                    end_flag = 1;
                    new_j = 0;
                }
            }
            break;
    }

    old_n = matedit_i * columns + matedit_j;
    new_n = new_i * columns + new_j;

    if (m->type == TYPE_REALMATRIX) {
        if (old_n != new_n) {
            if (rm->array->is_string[new_n] != 0) {
                char *text;
                int4 len;
                get_matrix_string(rm, new_n, &text, &len);
                v = new_string(text, len);
            } else
                v = new_real(rm->array->data[new_n]);
            if (v == NULL)
                return ERR_INSUFFICIENT_MEMORY;
        }
        if (sp == -1) {
            /* There's nothing to store, so leave cell unchanged */
        } else if (stack[sp]->type == TYPE_REAL) {
            if (rm->array->is_string[old_n] == 2)
                free(*(void **) &rm->array->data[old_n]);
            rm->array->is_string[old_n] = 0;
            rm->array->data[old_n] = ((vartype_real *) stack[sp])->x;
        } else if (stack[sp]->type == TYPE_STRING) {
            vartype_string *s = (vartype_string *) stack[sp];
            if (!put_matrix_string(rm, old_n, s->txt(), s->length)) {
                free_vartype(v);
                return ERR_INSUFFICIENT_MEMORY;
            }
        } else {
            free_vartype(v);
            return ERR_INVALID_TYPE;
        }
    } else if (m->type == TYPE_COMPLEXMATRIX) {
        if (old_n != new_n) {
            v = new_complex(cm->array->data[2 * new_n],
                            cm->array->data[2 * new_n + 1]);
            if (v == NULL)
                return ERR_INSUFFICIENT_MEMORY;
        }
        if (sp == -1) {
            /* There's nothing to store, so leave cell unchanged */
        } else if (stack[sp]->type == TYPE_REAL) {
            cm->array->data[2 * old_n] = ((vartype_real *) stack[sp])->x;
            cm->array->data[2 * old_n + 1] = 0;
        } else if (stack[sp]->type == TYPE_COMPLEX) {
            vartype_complex *c = (vartype_complex *) stack[sp];
            cm->array->data[2 * old_n] = c->re;
            cm->array->data[2 * old_n + 1] = c->im;
        } else {
            free_vartype(v);
            return stack[sp]->type == TYPE_STRING ? ERR_ALPHA_DATA_IS_INVALID
                                              : ERR_INVALID_TYPE;
        }
    } else /* m->type == TYPE_LIST */ {
        if (old_n != new_n) {
            v = dup_vartype(list->array->data[new_n]);
            if (v == NULL)
                return ERR_INSUFFICIENT_MEMORY;
        }
        if (sp == -1) {
            /* There's nothing to store, so leave cell unchanged */
        } else {
            vartype *nv = dup_vartype(stack[sp]);
            if (nv == NULL) {
                free_vartype(v);
                return ERR_INSUFFICIENT_MEMORY;
            }
            free_vartype(list->array->data[old_n]);
            list->array->data[old_n] = nv;
        }
    }

    matedit_i = new_i;
    matedit_j = new_j;
    flags.f.matrix_edge_wrap = edge_flag;
    flags.f.matrix_end_wrap = end_flag;
    if (old_n != new_n) {
        if (sp == -1)
            sp = 0;
        else
            free_vartype(stack[sp]);
        stack[sp] = v;
    }
    mode_disable_stack_lift = true;
    print_trace();
    return ERR_NONE;
}

int docmd_left(arg_struct *arg) {
    return matedit_move(DIR_LEFT);
}

int docmd_up(arg_struct *arg) {
    return matedit_move(DIR_UP);
}

int docmd_down(arg_struct *arg) {
    return matedit_move(DIR_DOWN);
}

int docmd_right(arg_struct *arg) {
    return matedit_move(DIR_RIGHT);
}

int docmd_percent_ch(arg_struct *arg) {
    phloat x, y, r;
    int inf;
    vartype *v;
    x = ((vartype_real *) stack[sp])->x;
    y = ((vartype_real *) stack[sp - 1])->x;
    if (y == 0)
        return ERR_DIVIDE_BY_0;
    r = (x - y) / y * 100;
    if ((inf = p_isinf(r)) != 0) {
        if (flags.f.range_error_ignore)
            r = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    v = new_real(r);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    /* Binary function, but unary result, like % */
    unary_result(v);
    return ERR_NONE;
}

static vartype *matx_v;

static int matx_completion(int error, vartype *res) {
    if (error != ERR_NONE) {
        free_vartype(matx_v);
        return error;
    }
    store_var("MATX", 4, res);
    matedit_prev_appmenu = MENU_MATRIX_SIMQ;
    set_menu(MENULEVEL_APP, MENU_MATRIX_EDIT1);
    /* NOTE: no need to use set_menu_return_err() here, since the MAT[ABX]
     * commands can only be invoked from the SIMQ menu; the SIMQ menu
     * has no exit callback, so leaving it never fails.
     */
    set_appmenu_exitcallback(1);
    if (res->type == TYPE_REALMATRIX) {
        vartype_realmatrix *m = (vartype_realmatrix *) res;
        vartype_real *v = (vartype_real *) matx_v;
        v->x = m->array->data[0];
    } else {
        vartype_complexmatrix *m = (vartype_complexmatrix *) res;
        vartype_complex *v = (vartype_complex *) matx_v;
        v->re = m->array->data[0];
        v->im = m->array->data[1];
    }
    if (sp == -1)
        sp = 0;
    else
        free_vartype(stack[sp]);
    stack[sp] = matx_v;
    matedit_mode = 3;
    matedit_length = 4;
    matedit_name[0] = 'M';
    matedit_name[1] = 'A';
    matedit_name[2] = 'T';
    matedit_name[3] = 'X';
    matedit_i = 0;
    matedit_j = 0;
    if (flags.f.big_stack)
        mode_disable_stack_lift = true;
    else
        /* HP-42S bug compatibility */
        mode_disable_stack_lift = flags.f.stack_lift_disable;
    return ERR_NONE;
}

static int matabx(int which) {
    vartype *mat, *v;

    switch (which) {
        case 0:
            mat = recall_var("MATA", 4);
            goto mat_a_or_b_check;

        case 1:
            mat = recall_var("MATB", 4);
            mat_a_or_b_check:
            if (mat == NULL)
                return ERR_NONEXISTENT;
            if (mat->type == TYPE_STRING)
                return ERR_ALPHA_DATA_IS_INVALID;
            if (mat->type != TYPE_REALMATRIX && mat->type != TYPE_COMPLEXMATRIX)
                return ERR_INVALID_TYPE;
            break;

        case 2: {
            vartype *mata, *matb;

            matb = recall_var("MATB", 4);
            if (matb == NULL)
                return ERR_NONEXISTENT;
            if (matb->type == TYPE_STRING)
                return ERR_ALPHA_DATA_IS_INVALID;
            if (matb->type != TYPE_REALMATRIX && matb->type != TYPE_COMPLEXMATRIX)
                return ERR_INVALID_TYPE;

            mata = recall_var("MATA", 4);
            if (mata == NULL)
                return ERR_NONEXISTENT;
            if (mata->type == TYPE_STRING)
                return ERR_ALPHA_DATA_IS_INVALID;
            if (mata->type != TYPE_REALMATRIX && mata->type != TYPE_COMPLEXMATRIX)
                return ERR_INVALID_TYPE;

            if (!ensure_var_space(1))
                return ERR_INSUFFICIENT_MEMORY;
            if (mata->type == TYPE_REALMATRIX && matb->type == TYPE_REALMATRIX)
                matx_v = new_real(0);
            else
                matx_v = new_complex(0, 0);
            if (matx_v == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            return linalg_div(matb, mata, matx_completion);
        }
    }

    if (mat->type == TYPE_REALMATRIX) {
        vartype_realmatrix *rm = (vartype_realmatrix *) mat;
        if (rm->array->is_string[0] != 0) {
            char *text;
            int4 length;
            get_matrix_string(rm, 0, &text, &length);
            v = new_string(text, length);
        } else
            v = new_real(rm->array->data[0]);
    } else {
        vartype_complexmatrix *cm = (vartype_complexmatrix *) mat;
        v = new_complex(cm->array->data[0], cm->array->data[1]);
    }
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;

    matedit_prev_appmenu = MENU_MATRIX_SIMQ;
    set_menu(MENULEVEL_APP, MENU_MATRIX_EDIT1);
    /* NOTE: no need to use set_menu_return_err() here, since the MAT[ABX]
     * commands can only be invoked from the SIMQ menu; the SIMQ menu
     * has no exit callback, so leaving it never fails.
     */
    set_appmenu_exitcallback(1);
    if (sp == -1)
        sp = 0;
    else
        free_vartype(stack[sp]);
    stack[sp] = v;
    matedit_mode = 3;
    matedit_length = 4;
    matedit_name[0] = 'M';
    matedit_name[1] = 'A';
    matedit_name[2] = 'T';
    matedit_name[3] = which == 0 ? 'A' : 'B';
    matedit_i = 0;
    matedit_j = 0;
    if (flags.f.big_stack)
        mode_disable_stack_lift = true;
    else
        /* HP-42S bug compatibility */
        mode_disable_stack_lift = flags.f.stack_lift_disable;
    return ERR_NONE;
}

int docmd_mata(arg_struct *arg) {
    return matabx(0);
}

int docmd_matb(arg_struct *arg) {
    return matabx(1);
}

int docmd_matx(arg_struct *arg) {
    return matabx(2);
}

int docmd_simq(arg_struct *arg) {
    vartype *m, *mata, *matb, *matx;
    int4 dim;
    int err;

    if (arg->type != ARGTYPE_NUM)
        return ERR_INVALID_TYPE;
    dim = arg->val.num;
    if (dim <= 0)
        return ERR_DIMENSION_ERROR;

    if (!ensure_var_space(3))
        return ERR_INSUFFICIENT_MEMORY;

    m = recall_var("MATA", 4);
    if (m != NULL && (m->type == TYPE_REALMATRIX || m->type == TYPE_COMPLEXMATRIX)) {
        mata = dup_vartype(m);
        if (mata == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        err = dimension_array_ref(mata, dim, dim);
        if (err != ERR_NONE)
            goto abort_and_free_a;
    } else {
        mata = new_realmatrix(dim, dim);
        if (mata == NULL)
            return ERR_INSUFFICIENT_MEMORY;
    }

    m = recall_var("MATB", 4);
    if (m != NULL && (m->type == TYPE_REALMATRIX || m->type == TYPE_COMPLEXMATRIX)) {
        matb = dup_vartype(m);
        if (matb == NULL) {
            err = ERR_INSUFFICIENT_MEMORY;
            goto abort_and_free_a;
        }
        err = dimension_array_ref(matb, dim, 1);
        if (err != ERR_NONE)
            goto abort_and_free_a_b;
    } else {
        matb = new_realmatrix(dim, 1);
        if (matb == NULL) {
            err = ERR_INSUFFICIENT_MEMORY;
            goto abort_and_free_a;
        }
    }

    m = recall_var("MATX", 4);
    if (m != NULL && (m->type == TYPE_REALMATRIX || m->type == TYPE_COMPLEXMATRIX)) {
        matx = dup_vartype(m);
        if (matx == NULL) {
            err = ERR_INSUFFICIENT_MEMORY;
            goto abort_and_free_a_b;
        }
        err = dimension_array_ref(matx, dim, 1);
        if (err != ERR_NONE)
            goto abort_and_free_a_b_x;
    } else {
        matx = new_realmatrix(dim, 1);
        if (matx == NULL) {
            err = ERR_INSUFFICIENT_MEMORY;
            goto abort_and_free_a_b;
        }
    }

    err = set_menu_return_err(MENULEVEL_APP, MENU_MATRIX_SIMQ, false);
    if (err != ERR_NONE) {
        /* Didn't work; we're stuck in the matrix editor
         * waiting for the user to put something valid into X.
         * (Then again, how can anyone issue the SIMQ command if
         * they're in the matrix editor? SIMQ has the 'hidden'
         * command property. Oh, well, better safe than sorry...)
         */
        abort_and_free_a_b_x:
        free_vartype(matx);
        abort_and_free_a_b:
        free_vartype(matb);
        abort_and_free_a:
        free_vartype(mata);
        return err;
    }

    if (matedit_mode == 1 || matedit_mode == 3) {
        // Note: matedit_mode cannot be EDITN at this point,
        // because SIMQ is a non-programmable, non-assignable command
        // that isn't reachable while the matrix editor is active.
        // Still, we're following the 'better safe than sorry' dictum,
        // as per the previous comment.
        if (string_equals(matedit_name, matedit_length, "MATA", 4)
                || string_equals(matedit_name, matedit_length, "MATB", 4)
                || string_equals(matedit_name, matedit_length, "MATX", 4)) {
            matedit_i = matedit_j = 0;
        }
    }

    store_var("MATX", 4, matx);
    store_var("MATB", 4, matb);
    store_var("MATA", 4, mata);
    return ERR_NONE;
}

static int max_min_helper(int do_max) {
    vartype *m;
    vartype_realmatrix *rm;
    phloat max_or_min_value = do_max ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
    int4 i, max_or_min_index = 0;
    vartype *new_x, *new_y;

    switch (matedit_mode) {
        case 0:
            return ERR_NONEXISTENT;
        case 1:
        case 3:
            m = recall_var(matedit_name, matedit_length);
            break;
        case 2:
            m = matedit_x;
            break;
        default:
            return ERR_INTERNAL_ERROR;
    }
    if (m == NULL)
        return ERR_NONEXISTENT;
    if (m->type != TYPE_REALMATRIX)
        return ERR_INVALID_TYPE;
    rm = (vartype_realmatrix *) m;

    for (i = matedit_i; i < rm->rows; i++) {
        int4 index = i * rm->columns + matedit_j;
        phloat e;
        if (rm->array->is_string[index] != 0)
            return ERR_ALPHA_DATA_IS_INVALID;
        e = rm->array->data[index];
        if (do_max ? e >= max_or_min_value : e <= max_or_min_value) {
            max_or_min_value = e;
            max_or_min_index = i;
        }
    }
    new_x = new_real(max_or_min_value);
    if (new_x == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    new_y = new_real(max_or_min_index + 1);
    if (new_y == NULL) {
        free_vartype(new_x);
        return ERR_INSUFFICIENT_MEMORY;
    }
    return recall_two_results(new_x, new_y);
}

int docmd_max(arg_struct *arg) {
    return max_min_helper(1);
}

int docmd_min(arg_struct *arg) {
    return max_min_helper(0);
}

int docmd_find(arg_struct *arg) {
    vartype *m;
    if (stack[sp]->type == TYPE_REALMATRIX || stack[sp]->type == TYPE_COMPLEXMATRIX)
        return ERR_INVALID_TYPE;
    switch (matedit_mode) {
        case 0:
            return ERR_NONEXISTENT;
        case 1:
        case 3:
            m = recall_var(matedit_name, matedit_length);
            break;
        case 2:
            m = matedit_x;
            break;
        default:
            return ERR_INTERNAL_ERROR;
    }
    if (m == NULL)
        return ERR_NONEXISTENT;
    if (m->type == TYPE_REALMATRIX) {
        vartype_realmatrix *rm;
        int4 i, j, p = 0;
        if (stack[sp]->type == TYPE_COMPLEX)
            return ERR_NO;
        rm = (vartype_realmatrix *) m;
        if (stack[sp]->type == TYPE_REAL) {
            phloat d = ((vartype_real *) stack[sp])->x;
            for (i = 0; i < rm->rows; i++)
                for (j = 0; j < rm->columns; j++)
                    if (rm->array->is_string[p] == 0 && rm->array->data[p] == d) {
                        matedit_i = i;
                        matedit_j = j;
                        return ERR_YES;
                    } else
                        p++;
        } else /* stack[sp]->type == TYPE_STRING */ {
            vartype_string *s = (vartype_string *) stack[sp];
            char *text = s->txt();
            int4 len = s->length;
            for (i = 0; i < rm->rows; i++)
                for (j = 0; j < rm->columns; j++) {
                    if (rm->array->is_string[p] != 0) {
                        char *mtext;
                        int4 mlen;
                        get_matrix_string(rm, p, &mtext, &mlen);
                        if (string_equals(text, len, mtext, mlen)) {
                            matedit_i = i;
                            matedit_j = j;
                            return ERR_YES;
                        }
                    }
                    p++;
                }
        }
    } else /* m->type == TYPE_COMPLEXMATRIX */ {
        vartype_complexmatrix *cm;
        int4 i, j, p = 0;
        phloat re, im;
        if (stack[sp]->type != TYPE_COMPLEX)
            return ERR_NO;
        cm = (vartype_complexmatrix *) m;
        re = ((vartype_complex *) stack[sp])->re;
        im = ((vartype_complex *) stack[sp])->im;
        for (i = 0; i < cm->rows; i++)
            for (j = 0; j < cm->columns; j++)
                if (cm->array->data[p] == re && cm->array->data[p + 1] == im) {
                    matedit_i = i;
                    matedit_j = j;
                    return ERR_YES;
                } else
                    p += 2;
    }
    return ERR_NO;
}

int docmd_xrom(arg_struct *arg) {
    return ERR_NONEXISTENT;
}
