/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2018  Thomas Okken
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

#include "core_commands1.h"
#include "core_commands2.h"
#include "core_commands3.h"
#include "core_commands4.h"
#include "core_display.h"
#include "core_helpers.h"
#include "core_linalg1.h"
#include "core_main.h"
#include "core_math2.h"
#include "core_sto_rcl.h"
#include "core_variables.h"

/********************************************************/
/* Implementations of HP-42S built-in functions, part 3 */
/********************************************************/

static int mappable_acosh_r(phloat x, phloat *y) {
    if (x >= 1) {
        *y = acosh(x);
        return ERR_NONE;
    } else
        return ERR_INVALID_DATA;
}

static int mappable_acosh_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    return math_acosh(xre, xim, yre, yim);
}

int docmd_acosh(arg_struct *arg) {
    vartype *v;
    if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else if (reg_x->type == TYPE_REAL) {
        phloat x = ((vartype_real *) reg_x)->x;
        if (x < 1) {
            if (flags.f.real_result_only)
                return ERR_INVALID_DATA;
            else {
                phloat re, im;
                int err = math_acosh(x, 0, &re, &im);
                if (err != ERR_NONE)
                    return err;
                v = new_complex(re, im);
            }
        } else
            v = new_real(acosh(x));
        if (v == NULL)
            return ERR_INSUFFICIENT_MEMORY;
    } else {
        int err = map_unary(reg_x, &v, mappable_acosh_r, mappable_acosh_c);
        if (err != ERR_NONE)
            return err;
    }
    unary_result(v);
    return ERR_NONE;
}

int docmd_aleng(arg_struct *arg) {
    vartype *v = new_real(reg_alpha_length);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    recall_result(v);
    return ERR_NONE;
}

int docmd_aoff(arg_struct *arg) {
    set_menu(MENULEVEL_ALPHA, MENU_NONE);
    return ERR_NONE;
}

int docmd_aon(arg_struct *arg) {
    mode_alpha_entry = false;
    set_menu(MENULEVEL_ALPHA, MENU_ALPHA1);
    return ERR_NONE;
}

int docmd_arot(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL) {
        phloat x;
        char buf[44];
        int i, j;
        if (reg_alpha_length == 0)
            goto done;
        x = ((vartype_real *) reg_x)->x;
        if (x < 0)
            x = -floor(-x);
        else
            x = floor(x);
        j = to_int(fmod(x, reg_alpha_length));
        if (j == 0)
            goto done;
        if (j < 0)
            j += reg_alpha_length;
        for (i = 0; i < reg_alpha_length; i++)
            buf[i] = reg_alpha[i];
        for (i = 0; i < reg_alpha_length; i++)
            reg_alpha[i] = buf[(i + j) % reg_alpha_length];
        done:
        if (flags.f.trace_print && flags.f.printer_exists)
            docmd_pra(NULL);
        return ERR_NONE;
    } else if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;
}

int docmd_ashf(arg_struct *arg) {
    int i;
    reg_alpha_length -= 6;
    if (reg_alpha_length < 0)
        reg_alpha_length = 0;
    for (i = 0; i < reg_alpha_length; i++)
        reg_alpha[i] = reg_alpha[i + 6];
    if (flags.f.trace_print && flags.f.printer_exists)
        docmd_pra(NULL);
    return ERR_NONE;
}

static int mappable_asinh_r(phloat x, phloat *y) {
    *y = asinh(x);
    return ERR_NONE;
}

static int mappable_asinh_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    return math_asinh(xre, xim, yre, yim);
}

int docmd_asinh(arg_struct *arg) {
    if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else {
        vartype *v;
        int err = map_unary(reg_x, &v, mappable_asinh_r, mappable_asinh_c);
        if (err == ERR_NONE)
            unary_result(v);
        return err;
    }
}

static int mappable_atanh_r(phloat x, phloat *y) {
    if (x == 1 || x == -1)
        return ERR_INVALID_DATA;
    *y = atanh(x);

    /* Theoretically, you could go out of range, but in practice,
     * you can't get close enough to the critical values to cause
     * trouble.
     */
    return ERR_NONE;
}

int docmd_atanh(arg_struct *arg) {
    vartype *v;
    int err;
    if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else if (reg_x->type == TYPE_REAL) {
        phloat x = ((vartype_real *) reg_x)->x;
        if (x == 1 || x == -1)
            return ERR_INVALID_DATA;
        if (x < -1 || x > 1) {
            if (flags.f.real_result_only)
                return ERR_INVALID_DATA;
            else {
                phloat tre, tim;
                err = math_atanh(x, 0, &tre, &tim);
                if (err != ERR_NONE)
                    return err;
                v = new_complex(tre, tim);
            }
        } else
            v = new_real(atanh(x));
        if (v == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        unary_result(v);
        return ERR_NONE;
    } else {
        err = map_unary(reg_x, &v, mappable_atanh_r, math_atanh);
        if (err == ERR_NONE)
            unary_result(v);
        return err;
    }
}

int docmd_atox(arg_struct *arg) {
    vartype *v = new_real(reg_alpha_length == 0 ? 0 :
                                    (unsigned char) reg_alpha[0]);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    if (reg_alpha_length > 0) {
        int i;
        reg_alpha_length--;
        for (i = 0; i < reg_alpha_length; i++)
            reg_alpha[i] = reg_alpha[i + 1];
    }
    if (flags.f.trace_print && flags.f.printer_exists)
        docmd_pra(NULL);
    recall_result(v);
    return ERR_NONE;
}

static int mappable_cosh_r(phloat x, phloat *y) {
    int inf;
    *y = cosh(x);
    if ((inf = p_isinf(*y)) != 0) {
        if (flags.f.range_error_ignore)
            *y = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    return ERR_NONE;
}   

static int mappable_cosh_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    if (xim == 0) {
        *yim = 0;
        return mappable_cosh_r(xre, yre);
    } else if (xre == 0) {
        *yre = cos(xim);
        *yim = 0;
        return ERR_NONE;
    }

    phloat sinhxre, coshxre;
    phloat sinxim, cosxim;
    int inf;
    sinhxre = sinh(xre);
    coshxre = cosh(xre);
    sincos(xim, &sinxim, &cosxim);
    *yre = coshxre * cosxim;
    if ((inf = p_isinf(*yre)) != 0) {
        if (flags.f.range_error_ignore)
            *yre = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    *yim = sinhxre * sinxim;
    if ((inf = p_isinf(*yim)) != 0) {
        if (flags.f.range_error_ignore)
            *yim = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    return ERR_NONE;
}

int docmd_cosh(arg_struct *arg) {
    if (reg_x->type != TYPE_STRING) {
        vartype *v;
        int err = map_unary(reg_x, &v, mappable_cosh_r, mappable_cosh_c);
        if (err == ERR_NONE)
            unary_result(v);
        return err;
    } else
        return ERR_ALPHA_DATA_IS_INVALID;
}

/* NOTE: it is possible to generalize the cross product to more than
 * 3 dimensions... Something for Free42++ perhaps?
 */
int docmd_cross(arg_struct *arg) {
    if (reg_x->type == TYPE_STRING || reg_y->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else if (reg_x->type == TYPE_COMPLEX && reg_y->type == TYPE_COMPLEX) {
        vartype_complex *left = (vartype_complex *) reg_y;
        vartype_complex *right = (vartype_complex *) reg_x;
        vartype *v;
        phloat d = left->re * right->im - left->im * right->re;
        int inf;
        if ((inf = p_isinf(d)) != 0) {
            if (flags.f.range_error_ignore)
                d = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        }
        v = new_real(d);
        if (v == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        binary_result(v);
        return ERR_NONE;
    } else if (reg_x->type == TYPE_REALMATRIX
                        && reg_y->type == TYPE_REALMATRIX) {
        vartype_realmatrix *left = (vartype_realmatrix *) reg_y;
        vartype_realmatrix *right = (vartype_realmatrix *) reg_x;
        int4 ls = left->rows * left->columns;
        int4 rs = right->rows * right->columns;
        int4 i;
        int inf;
        phloat xl, yl = 0, zl = 0, xr, yr = 0, zr = 0;
        phloat xres, yres, zres;
        vartype_realmatrix *res;
        if (ls > 3 || rs > 3)
            return ERR_DIMENSION_ERROR;
        for (i = 0; i < ls; i++)
            if (left->array->is_string[i])
                return ERR_ALPHA_DATA_IS_INVALID;
        for (i = 0; i < rs; i++)
            if (right->array->is_string[i])
                return ERR_ALPHA_DATA_IS_INVALID;
        switch (ls) {
            case 3: zl = left->array->data[2];
            case 2: yl = left->array->data[1];
            case 1: xl = left->array->data[0];
        }
        switch (rs) {
            case 3: zr = right->array->data[2];
            case 2: yr = right->array->data[1];
            case 1: xr = right->array->data[0];
        }
        xres = yl * zr - zl * yr;
        if ((inf = p_isinf(xres)) != 0) {
            if (flags.f.range_error_ignore)
                xres = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        }
        yres = zl * xr - xl * zr;
        if ((inf = p_isinf(yres)) != 0) {
            if (flags.f.range_error_ignore)
                yres = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        }
        zres = xl * yr - yl * xr;
        if ((inf = p_isinf(zres)) != 0) {
            if (flags.f.range_error_ignore)
                zres = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        }
        res = (vartype_realmatrix *) new_realmatrix(1, 3);
        if (res == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        res->array->data[0] = xres;
        res->array->data[1] = yres;
        res->array->data[2] = zres;
        binary_result((vartype *) res);
        return ERR_NONE;
    } else
        return ERR_INVALID_TYPE;
}

int docmd_custom(arg_struct *arg) {
    if (mode_plainmenu != MENU_CUSTOM1
            && mode_plainmenu != MENU_CUSTOM2
            && mode_plainmenu != MENU_CUSTOM3)
        set_menu(MENULEVEL_PLAIN, MENU_CUSTOM1);
    return ERR_NONE;
}

int docmd_delr(arg_struct *arg) {
    vartype *m, *newx;
    vartype_realmatrix *rm;
    vartype_complexmatrix *cm;
    int4 rows, columns, i, j, n, newi;
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
    if (m->type != TYPE_REALMATRIX && m->type != TYPE_COMPLEXMATRIX)
        return ERR_INVALID_TYPE;

    if (m->type == TYPE_REALMATRIX) {
        rm = (vartype_realmatrix *) m;
        rows = rm->rows;
        columns = rm->columns;
        refcount = rm->array->refcount;
    } else {
        cm = (vartype_complexmatrix *) m;
        rows = cm->rows;
        columns = cm->columns;
        refcount = cm->array->refcount;
    }

    if (rows == 1)
        return ERR_DIMENSION_ERROR;

    if (matedit_i >= rows)
        matedit_i = rows - 1;
    if (matedit_j >= columns)
        matedit_j = columns - 1;
    n = (matedit_i + 1) * columns + matedit_j;

    if (matedit_i == rows - 1) {
        /* Deleting the bottom row; we need to move up one row, and
         * the next IJ value we show in X is going to be from *two* rows
         * higher: if you delete row 4 and it's not the bottom row, then
         * we'll show the value that's now in row 5; if it is the bottom
         * row, we move up one, and show the value that's now in row 3.
         */
        newi = matedit_i - 1;
        n -= 2 * columns;
    } else
        newi = matedit_i;

    interactive = matedit_mode == 2 || matedit_mode == 3;
    if (interactive) {
        if (m->type == TYPE_REALMATRIX) {
            if (rm->array->is_string[n])
                newx = new_string(phloat_text(rm->array->data[n]),
                                  phloat_length(rm->array->data[n]));
            else
                newx = new_real(rm->array->data[n]);
        } else
            newx = new_complex(cm->array->data[2 * n],
                               cm->array->data[2 * n + 1]);

        if (newx == NULL)
            return ERR_INSUFFICIENT_MEMORY;
    }

    if (refcount == 1) {
        /* We have this array to ourselves so we can modify it in place
         * We shuffle the array elements around so that if the reallocation
         * succeeds, everything is in the right place, but if it fails, we
         * still have all the data so we can roll everything back. And best
         * of all, no temporary memory allocations needed!
         */
        if (m->type == TYPE_REALMATRIX) {
            for (j = 0; j < columns; j++) {
                phloat tempd = rm->array->data[matedit_i * columns + j];
                char tempc = rm->array->is_string[matedit_i * columns + j];
                for (i = matedit_i; i < rows - 1; i++) {
                    rm->array->data[i * columns + j] =
                                rm->array->data[(i + 1) * columns + j];
                    rm->array->is_string[i * columns + j] =
                                rm->array->is_string[(i + 1) * columns + j];
                }
                rm->array->data[(rows - 1) * columns + j] = tempd;
                rm->array->is_string[(rows - 1) * columns + j] = tempc;
            }
            err = dimension_array_ref(m, rows - 1, columns);
            if (err != ERR_NONE) {
                /* Dang! Now we have to rotate everything back to where
                 * it was before. */
                for (j = 0; j < columns; j++) {
                    phloat tempd = rm->array->data[(rows - 1) * columns + j];
                    char tempc = rm->array->is_string[(rows - 1) * columns + j];
                    for (i = rows - 1; i > matedit_i; i--) {
                        rm->array->data[i * columns + j] =
                                    rm->array->data[(i - 1) * columns + j];
                        rm->array->is_string[i * columns + j] =
                                    rm->array->is_string[(i - 1) * columns + j];
                    }
                    rm->array->data[matedit_i * columns + j] = tempd;
                    rm->array->is_string[matedit_i * columns + j] = tempc;
                }
                if (interactive)
                    free_vartype(newx);
                return err;
            }
        } else {
            for (j = 0; j < 2 * columns; j++) {
                phloat tempd = cm->array->data[matedit_i * 2 * columns + j];
                for (i = matedit_i; i < rows - 1; i++)
                    cm->array->data[i * 2 * columns + j] =
                                cm->array->data[(i + 1) * 2 * columns + j];
                cm->array->data[(rows - 1) * 2 * columns + j] = tempd;
            }
            err = dimension_array_ref(m, rows - 1, columns);
            if (err != ERR_NONE) {
                /* Dang! Now we have to rotate everything back to where
                 * it was before. */
                for (j = 0; j < 2 * columns; j++) {
                    phloat tempd = cm->array->data[(rows - 1) * 2 * columns + j];
                    for (i = rows - 1; i > matedit_i; i--)
                        cm->array->data[i * 2 * columns + j] =
                                    cm->array->data[(i - 1) * 2 * columns + j];
                    cm->array->data[matedit_i * 2 * columns + j] = tempd;
                }
                if (interactive)
                    free_vartype(newx);
                return err;
            }
        }
    } else {
        /* We're sharing this array. I don't use disentangle() because it
         * does not deal with resizing. */
        int4 newsize = (rows - 1) * columns;
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
            for (i = matedit_i * columns; i < newsize; i++) {
                array->is_string[i] = rm->array->is_string[i + columns];
                array->data[i] = rm->array->data[i + columns];
            }
            array->refcount = 1;
            rm->array->refcount--;
            rm->array = array;
            rm->rows--;
        } else {
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
            for (i = 2 * matedit_i * columns; i < 2 * newsize; i++)
                array->data[i] = cm->array->data[i + 2 * columns];
            array->refcount = 1;
            cm->array->refcount--;
            cm->array = array;
            cm->rows--;
        }
    }
    if (interactive) {
        free_vartype(reg_x);
        reg_x = newx;
    }
    matedit_i = newi;
    return ERR_NONE;
}

static void det_completion(int error, vartype *det) {
    if (error == ERR_NONE)
        unary_result(det);
}

int docmd_det(arg_struct *arg) {
    if (reg_x->type == TYPE_REALMATRIX || reg_x->type == TYPE_COMPLEXMATRIX)
        return linalg_det(reg_x, det_completion);
    else if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;
}

int docmd_dim(arg_struct *arg) {
    phloat x, y;
    int err;
    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    if (arg->type != ARGTYPE_STR)
        return ERR_INVALID_TYPE;

    if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_x->type != TYPE_REAL)
        return ERR_INVALID_TYPE;
    if (reg_y->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_y->type != TYPE_REAL)
        return ERR_INVALID_TYPE;
    x = ((vartype_real *) reg_x)->x;
    if (x == 0)
        return ERR_DIMENSION_ERROR;
    if (x < 0)
        x = -x;
    if (x >= 2147483648.0)
        return ERR_INSUFFICIENT_MEMORY;
    y = ((vartype_real *) reg_y)->x;
    if (y == 0)
        return ERR_DIMENSION_ERROR;
    if (y < 0)
        y = -y;
    if (y >= 2147483648.0)
        return ERR_INSUFFICIENT_MEMORY;
    return dimension_array(arg->val.text, arg->length, to_int(y), to_int(x), true);
}

int docmd_dot(arg_struct *arg) {
    /* TODO: look for range errors in intermediate results.
     * Right now, 1e300+1e300i DOT 1e300-1e300i returns NaN
     * on the Palm, because two infinities of opposite signs
     * are added. (Why no NaN on the PC? Weird stuff: doing
     * "d = xre * yre + xim * yim" yields a different result
     * than "d = xre * yre ; d += xim * yim". Go figure.)
     */
    vartype *v;
    if (reg_x->type == TYPE_STRING || reg_y->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else if (reg_x->type == TYPE_REALMATRIX && reg_y->type == TYPE_REALMATRIX) {
        vartype_realmatrix *rm1 = (vartype_realmatrix *) reg_x;
        vartype_realmatrix *rm2 = (vartype_realmatrix *) reg_y;
        int4 size = rm1->rows * rm1->columns;
        int4 i;
        phloat dot = 0;
        int inf;
        if (size != rm2->rows * rm2->columns)
            return ERR_DIMENSION_ERROR;
        for (i = 0; i < size; i++)
            if (rm1->array->is_string[i] || rm2->array->is_string[i])
                return ERR_ALPHA_DATA_IS_INVALID;
        for (i = 0; i < size; i++)
            dot += rm1->array->data[i] * rm2->array->data[i];
        if ((inf = p_isinf(dot)) != 0) {
            if (flags.f.range_error_ignore)
                dot = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        }
        v = new_real(dot);
    } else if ((reg_x->type == TYPE_REALMATRIX
                    && reg_y->type == TYPE_COMPLEXMATRIX)
                ||
               (reg_x->type == TYPE_COMPLEXMATRIX
                    && reg_y->type == TYPE_REALMATRIX)) {
        vartype_realmatrix *rm;
        vartype_complexmatrix *cm;
        int4 size, i;
        phloat dot_re = 0, dot_im = 0;
        int inf;
        if (reg_x->type == TYPE_REALMATRIX) {
            rm = (vartype_realmatrix *) reg_x;
            cm = (vartype_complexmatrix *) reg_y;
        } else {
            rm = (vartype_realmatrix *) reg_y;
            cm = (vartype_complexmatrix *) reg_x;
        }
        size = rm->rows * rm->columns;
        if (size != cm->rows * cm->columns)
            return ERR_DIMENSION_ERROR;
        for (i = 0; i < size; i++)
            if (rm->array->is_string[i])
                return ERR_ALPHA_DATA_IS_INVALID;
        for (i = 0; i < size; i++) {
            dot_re += rm->array->data[i] * cm->array->data[2 * i];
            dot_im += rm->array->data[i] * cm->array->data[2 * i + 1];
        }
        if ((inf = p_isinf(dot_re)) != 0) {
            if (flags.f.range_error_ignore)
                dot_re = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        }
        if ((inf = p_isinf(dot_im)) != 0) {
            if (flags.f.range_error_ignore)
                dot_im = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        }
        v = new_complex(dot_re, dot_im);
    } else if (reg_x->type == TYPE_COMPLEXMATRIX
                    && reg_y->type == TYPE_COMPLEXMATRIX) {
        vartype_complexmatrix *cm1 = (vartype_complexmatrix *) reg_x;
        vartype_complexmatrix *cm2 = (vartype_complexmatrix *) reg_y;
        int4 size, i;
        phloat dot_re = 0, dot_im = 0;
        int inf;
        size = cm1->rows * cm1->columns;
        if (size != cm2->rows * cm2->columns)
            return ERR_DIMENSION_ERROR;
        size *= 2;
        for (i = 0; i < size; i += 2) {
            phloat re1 = cm1->array->data[i];
            phloat im1 = cm1->array->data[i + 1];
            phloat re2 = cm2->array->data[i];
            phloat im2 = cm2->array->data[i + 1];
            dot_re += re1 * re2 - im1 * im2;
            dot_im += re1 * im2 + re2 * im1;
        }
        if ((inf = p_isinf(dot_re)) != 0) {
            if (flags.f.range_error_ignore)
                dot_re = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        }
        if ((inf = p_isinf(dot_im)) != 0) {
            if (flags.f.range_error_ignore)
                dot_im = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        }
        v = new_complex(dot_re, dot_im);
    } else if (reg_x->type == TYPE_COMPLEX && reg_y->type == TYPE_COMPLEX) {
        vartype_complex *x = (vartype_complex *) reg_x;
        vartype_complex *y = (vartype_complex *) reg_y;
        phloat d = x->re * y->re + x->im * y->im;
        int inf;
        if ((inf = p_isinf(d)) != 0) {
            if (flags.f.range_error_ignore)
                d = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        }
        v = new_real(d);
    } else
        return ERR_INVALID_TYPE;
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    binary_result(v);
    return ERR_NONE;
}

int matedit_get_dim(int4 *rows, int4 *columns) {
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

    if (m->type == TYPE_REALMATRIX) {
        vartype_realmatrix *rm = (vartype_realmatrix *) m;
        *rows = rm->rows;
        *columns = rm->columns;
        return ERR_NONE;
    } else if (m->type == TYPE_COMPLEXMATRIX) {
        vartype_complexmatrix *cm = (vartype_complexmatrix *) m;
        *rows = cm->rows;
        *columns = cm->columns;
        return ERR_NONE;
    } else
        return ERR_INVALID_TYPE;
}

/* NOTE: this is a callback for set_menu(); it is declared in
 * core_display.h but defined here.
 */
int appmenu_exitcallback_1(int menuid, bool exitall) {
    if (menuid == MENU_MATRIX_EDIT1 || menuid == MENU_MATRIX_EDIT2) {
        /* Just switching menus within the editor */
        mode_appmenu = menuid;
        set_appmenu_exitcallback(1);
        return ERR_NONE;
    } else {
        /* The user is trying to leave the editor; we only allow that
         * if storing X is successful. */
        int err = docmd_stoel(NULL);
        if (err == ERR_INSUFFICIENT_MEMORY)
            /* There's no graceful way to handle this, at least not without
             * some code restructuring (TODO); this is the one error that
             * should *not* prevent the user from getting out of the matrix
             * editor! Unfortunately, setting the error to ERR_NONE means we
             * aren't reporting the fact that changing the matrix element
             * was unsuccessful.
             */
            err = ERR_NONE;
        if (err == ERR_NONEXISTENT)
            /* This can happen when the user overwrites the indexed
             * matrix with a scalar. The fact that that is even possible
             * is a bug; attempting to do that, or to CLV it, should
             * cause a Restricted Operation message.
             * This code is a workaround to allow users to escape from
             * the Matrix Editor once its state has become hosed in the
             * above manner. We could remove this hack eventually. (TODO)
             */
            err = ERR_NONE;
        if (err != ERR_NONE) {
            /* Reinstate the callback so we'll get called again when
             * the user tries to leave again. */
            set_appmenu_exitcallback(1);
            return err;
        }
        if (matedit_mode == 2) {
            free_vartype(reg_x);
            reg_x = matedit_x;
            matedit_x = NULL;
        }
        matedit_mode = 0;
        flags.f.grow = 0;
        flags.f.stack_lift_disable = 0;
        /* Note: no need to check the value returned by set_menu() here:
         * since we're in a set_menu() callback, we know that there is no
         * callback registered at the moment (set_menu() unregisters
         * callbacks just before invoking them), so these set_menu() calls
         * won't fail.
         */
        if (menuid == MENU_NONE && !exitall)
            set_menu(MENULEVEL_APP, matedit_prev_appmenu);
        else
            set_menu(MENULEVEL_APP, menuid);
        return ERR_NONE;
    }
}

static int finish_edit() {
    if (matedit_mode == 2 || matedit_mode == 3)
        /* Try to finish current interactive editing session using
         * set_menu(). The callback (appmenu_exitcallback) will
         * return its error status to set_menu(), which passes it
         * on to us.
         */
        return set_menu_return_err(MENULEVEL_APP, MENU_NONE, false);
    else
        return ERR_NONE;
}

int docmd_edit(arg_struct *arg) {
    int err = finish_edit();
    if (err != ERR_NONE)
        return err;
    if (reg_x->type == TYPE_REALMATRIX || reg_x->type == TYPE_COMPLEXMATRIX) {
        vartype *v;
        if (reg_x->type == TYPE_REALMATRIX) {
            vartype_realmatrix *rm = (vartype_realmatrix *) reg_x;
            if (rm->array->is_string[0])
                v = new_string(phloat_text(rm->array->data[0]),
                               phloat_length(rm->array->data[0]));
            else
                v = new_real(rm->array->data[0]);
        } else {
            vartype_complexmatrix *cm = (vartype_complexmatrix *) reg_x;
            v = new_complex(cm->array->data[0], cm->array->data[1]);
        }
        if (v == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        matedit_mode = 2;
        flags.f.grow = 0;
        matedit_x = reg_x;
        reg_x = v;
        matedit_i = 0;
        matedit_j = 0;
        if (mode_appmenu >= MENU_MATRIX1 && mode_appmenu <= MENU_MATRIX_SIMQ)
            matedit_prev_appmenu = mode_appmenu;
        else
            matedit_prev_appmenu = MENU_NONE;
        set_menu(MENULEVEL_APP, MENU_MATRIX_EDIT1);
        set_appmenu_exitcallback(1);
        if (flags.f.trace_print && flags.f.printer_exists)
            docmd_prx(NULL);
        return ERR_NONE;
    } else
        return ERR_INVALID_TYPE;
}

int docmd_editn(arg_struct *arg) {
    vartype *m;
    int err;

    err = finish_edit();
    if (err != ERR_NONE)
        return err;

    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    if (arg->type != ARGTYPE_STR)
        return ERR_INVALID_TYPE;
    m = recall_var(arg->val.text, arg->length);
    if (m == NULL)
        return ERR_NONEXISTENT;
    else if (m->type != TYPE_REALMATRIX && m->type != TYPE_COMPLEXMATRIX)
        return ERR_INVALID_TYPE;
    else {
        vartype *v;
        int i;
        if (m->type == TYPE_REALMATRIX) {
            vartype_realmatrix *rm = (vartype_realmatrix *) m;
            if (rm->array->is_string[0])
                v = new_string(phloat_text(rm->array->data[0]),
                               phloat_length(rm->array->data[0]));
            else
                v = new_real(rm->array->data[0]);
        } else {
            vartype_complexmatrix *cm = (vartype_complexmatrix *) m;
            v = new_complex(cm->array->data[0], cm->array->data[1]);
        }
        if (v == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        /* TODO: implement a mechanism that locks a matrix while it is
         * under edit. While locked, operations such as CLV, DIM, or
         * assignment should fail with ERR_RESTRICTED_OPERATION.
         */
        matedit_mode = 3;
        flags.f.grow = 0;
        matedit_length = arg->length;
        for (i = 0; i < matedit_length; i++)
            matedit_name[i] = arg->val.text[i];
        free_vartype(reg_x);
        reg_x = v;
        matedit_i = 0;
        matedit_j = 0;
        if (mode_appmenu >= MENU_MATRIX1 && mode_appmenu <= MENU_MATRIX_SIMQ)
            matedit_prev_appmenu = mode_appmenu;
        else
            matedit_prev_appmenu = MENU_NONE;
        set_menu(MENULEVEL_APP, MENU_MATRIX_EDIT1);
        set_appmenu_exitcallback(1);
        if (flags.f.trace_print && flags.f.printer_exists)
            docmd_prx(NULL);
        return ERR_NONE;
    }
}

int docmd_exitall(arg_struct *arg) {
    return set_menu_return_err(MENULEVEL_APP, MENU_NONE, true);
}

static int mappable_e_pow_x_1(phloat x, phloat *y) {
    *y = expm1(x);
    if (p_isinf(*y) != 0) {
        if (flags.f.range_error_ignore)
            *y = POS_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    return ERR_NONE;
}   

int docmd_e_pow_x_1(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL || reg_x->type == TYPE_REALMATRIX) {
        vartype *v;
        int err = map_unary(reg_x, &v, mappable_e_pow_x_1, NULL);
        if (err == ERR_NONE)
            unary_result(v);
        return err;
    } else if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;
}

static int fnrm(vartype *m, phloat *norm) {
    if (m->type == TYPE_REALMATRIX) {
        vartype_realmatrix *rm = (vartype_realmatrix *) m;
        int4 size = rm->rows * rm->columns;
        int4 i;
        phloat nrm = 0;
        for (i = 0; i < size; i++)
            if (rm->array->is_string[i])
                return ERR_ALPHA_DATA_IS_INVALID;
        for (i = 0; i < size; i++) {
            /* TODO -- overflows in intermediaries */
            phloat x = rm->array->data[i];
            nrm += x * x;
        }
        if (p_isinf(nrm)) {
            if (flags.f.range_error_ignore)
                nrm = POS_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        } else
            nrm = sqrt(nrm);
        *norm = nrm;
        return ERR_NONE;
    } else if (m->type == TYPE_COMPLEXMATRIX) {
        vartype_complexmatrix *cm = (vartype_complexmatrix *) m;
        int4 size = 2 * cm->rows * cm->columns;
        int4 i;
        phloat nrm = 0;
        for (i = 0; i < size; i++) {
            /* TODO -- overflows in intermediaries */
            phloat x = cm->array->data[i];
            nrm += x * x;
        }
        if (p_isinf(nrm)) {
            if (flags.f.range_error_ignore)
                nrm = POS_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        } else
            nrm = sqrt(nrm);
        *norm = nrm;
        return ERR_NONE;
    } else if (m->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;
}

int docmd_fnrm(arg_struct *arg) {
    phloat norm;
    vartype *v;
    int err = fnrm(reg_x, &norm);
    if (err != ERR_NONE)
        return err;
    v = new_real(norm);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

int docmd_getm(arg_struct *arg) {
    vartype *m;
    phloat xx, yy;
    int4 x, y;

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
    
    if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_x->type != TYPE_REAL)
        return ERR_INVALID_TYPE;
    if (reg_y->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_y->type != TYPE_REAL)
        return ERR_INVALID_TYPE;

    xx = ((vartype_real *) reg_x)->x;
    if (xx <= -2147483648.0 || xx >= 2147483648.0)
        return ERR_DIMENSION_ERROR;
    x = to_int4(xx);
    if (x == 0)
        return ERR_DIMENSION_ERROR;
    if (x < 0)
        x = -x;

    yy = ((vartype_real *) reg_y)->x;
    if (yy <= -2147483648.0 || yy >= 2147483648.0)
        return ERR_DIMENSION_ERROR;
    y = to_int4(yy);
    if (y == 0)
        return ERR_DIMENSION_ERROR;
    if (y < 0)
        y = -y;

    if (m->type == TYPE_REALMATRIX) {
        vartype_realmatrix *src, *dst;
        int4 i, j;
        src = (vartype_realmatrix *) m;
        if (src->rows < matedit_i + y || src->columns < matedit_j + x)
            return ERR_DIMENSION_ERROR;
        dst = (vartype_realmatrix *) new_realmatrix(y, x);
        if (dst == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        for (i = 0; i < y; i++)
            for (j = 0; j < x; j++) {
                int4 n1 = (i + matedit_i) * src->columns + j + matedit_j;
                int4 n2 = i * dst->columns + j;
                dst->array->is_string[n2] = src->array->is_string[n1];
                dst->array->data[n2] = src->array->data[n1];
            }
        binary_result((vartype *) dst);
        return ERR_NONE;
    } else /* m->type == TYPE_COMPLEXMATRIX */ {
        vartype_complexmatrix *src, *dst;
        int4 i, j;
        src = (vartype_complexmatrix *) m;
        if (src->rows < matedit_i + y || src->columns < matedit_j + x)
            return ERR_DIMENSION_ERROR;
        dst = (vartype_complexmatrix *) new_complexmatrix(y, x);
        if (dst == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        for (i = 0; i < y; i++)
            for (j = 0; j < x; j++) {
                int4 n1 = (i + matedit_i) * src->columns + j + matedit_j;
                int4 n2 = i * dst->columns + j;
                dst->array->data[n2 * 2] = src->array->data[n1 * 2];
                dst->array->data[n2 * 2 + 1] = src->array->data[n1 * 2 + 1];
            }
        binary_result((vartype *) dst);
        return ERR_NONE;
    }
}

int docmd_grow(arg_struct *arg) {
    flags.f.grow = 1;
    return ERR_NONE;
}

static int hms_add_or_sub(bool add) {
    if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_x->type != TYPE_REAL)
        return ERR_INVALID_TYPE;
    if (reg_y->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_y->type != TYPE_REAL)
        return ERR_INVALID_TYPE;

    phloat x = ((vartype_real *) reg_x)->x;
    phloat y = ((vartype_real *) reg_y)->x;
    phloat r;
    bool neg;

    if (x < 0) {
        x = -x;
        add = !add;
    }
    if (y < 0) {
        y = -y;
        add = !add;
        neg = 1;
    } else
        neg = 0;

    #ifdef BCD_MATH
        phloat xh, xm, xs, yh, ym, ys, t;
        xh = floor(x);
        t = (x - xh) * 100;
        xm = floor(t);
        xs = (t - xm) * 100;
        yh = floor(y);
        t = (y - yh) * 100;
        ym = floor(t);
        ys = (t - ym) * 100;

        if (add) {
            ys += xs;
            if (ys >= 60) {
                ys -= 60;
                ym++;
            }
            ym += xm;
            if (ym >= 60) {
                ym -= 60;
                yh++;
            }
            yh += xh;
        } else {
            ys -= xs;
            if (ys < 0) {
                ys += 60;
                ym--;
            } else if (ys >= 60) {
                ys -= 60;
                ym++;
            }
            ym -= xm;
            if (ym < 0) {
                ym += 60;
                yh--;
            } else if (ym >= 60) {
                ym -= 60;
                yh++;
            }
            yh -= xh;
            if (yh < 0) {
                if (ys != 0) {
                    ys -= 60;
                    ym++;
                }
                if (ym != 0) {
                    ym -= 60;
                    yh++;
                }
            }
        }

        r = yh + ym / 100 + ys / 10000;

    #else

        /* I'm doing the computation in integer math. The nastiness is necessary,
        * because this is one of those cases (along with ->HR, ISG, and DSE)
        * where binary representation errors can mess things up pretty badly...
        * For example, you enter 0.45, thinking 45 minutes; the closest binary
        * representation is 0.449999999..., which could easily be mistaken for 44
        * minutes 99.99999... seconds -- 40 seconds off!
        */
        double rx, ry, res;
        int8 ix, iy, ixhr, iyhr, ires, ireshr;

        rx = floor(x);
        ry = floor(y);
        res = add ? ry + rx : ry - rx;
        ix = (int8) (((x - rx) * 1000000000000.0) + 0.5);
        iy = (int8) (((y - ry) * 1000000000000.0) + 0.5);
        ixhr = ix % LL(10000000000);
        iyhr = iy % LL(10000000000);
        ix /= LL(10000000000);
        iy /= LL(10000000000);
        ixhr += (ix % 100) * LL(6000000000);
        iyhr += (iy % 100) * LL(6000000000);
        ixhr += (ix / 100) * LL(360000000000);
        iyhr += (iy / 100) * LL(360000000000);
        ireshr = add ? iyhr + ixhr : iyhr - ixhr;
        while (ireshr < 0 && res > 0) {
            ireshr += LL(360000000000);
            res -= 1;
        }
        while (ireshr > 0 && res < 0) {
            ireshr -= LL(360000000000);
            res += 1;
        }
        ires = ireshr % LL(6000000000);
        ireshr /= LL(6000000000);
        ires += (ireshr % 60) * LL(10000000000);
        ires += (ireshr / 60) * LL(1000000000000);
        res += ires / 1000000000000.0;
        r = res;
    #endif

    /* Round-off may have caused the minutes or seconds to reach 60;
     * detect this and fix...
     */
    r = fix_hms(r);

    if (neg)
        r = -r;

    int inf;
    if ((inf = p_isinf(r)) != 0) {
        if (flags.f.range_error_ignore)
            r = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }

    vartype *v = new_real(r);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    binary_result(v);
    return ERR_NONE;
}

int docmd_hmsadd(arg_struct *arg) {
    return hms_add_or_sub(true);
}

int docmd_hmssub(arg_struct *arg) {
    return hms_add_or_sub(false);
}

int docmd_i_add(arg_struct *arg) {
    int4 rows, columns;
    int err = matedit_get_dim(&rows, &columns);
    if (err != ERR_NONE)
        return err;
    if (++matedit_i >= rows) {
        flags.f.matrix_edge_wrap = 1;
        matedit_i = 0;
        if (++matedit_j >= columns) {
            flags.f.matrix_end_wrap = 1;
            matedit_j = 0;
        } else
            flags.f.matrix_end_wrap = 0;
    } else {
        flags.f.matrix_edge_wrap = 0;
        flags.f.matrix_end_wrap = 0;
    }
    return ERR_NONE;
}

int docmd_i_sub(arg_struct *arg) {
    int4 rows, columns;
    int err = matedit_get_dim(&rows, &columns);
    if (err != ERR_NONE)
        return err;
    if (--matedit_i < 0) {
        flags.f.matrix_edge_wrap = 1;
        matedit_i = rows - 1;
        if (--matedit_j < 0) {
            flags.f.matrix_end_wrap = 1;
            matedit_j = columns - 1;
        } else
            flags.f.matrix_end_wrap = 0;
    } else {
        flags.f.matrix_edge_wrap = 0;
        flags.f.matrix_end_wrap = 0;
    }
    return ERR_NONE;
}

void matedit_goto(int4 row, int4 column) {
    int4 rows, columns;
    int err = matedit_get_dim(&rows, &columns);
    if (err == ERR_NONE) {
        if (row == 0 || row > rows || column == 0 || column > columns)
            err = ERR_DIMENSION_ERROR;
        else {
            int prev_stack_lift_disable;
            matedit_i = row - 1;
            matedit_j = column - 1;
            prev_stack_lift_disable = flags.f.stack_lift_disable;
            flags.f.stack_lift_disable = 1;
            err = docmd_rclel(NULL);
            flags.f.stack_lift_disable = prev_stack_lift_disable;
        }
    }
    if (err != ERR_NONE) {
        display_error(err, 0);
        flush_display();
    }
}

int docmd_index(arg_struct *arg) {
    int err;
    vartype *m;
    int i;

    if (matedit_mode == 2 || matedit_mode == 3)
        /* TODO: on the real HP-42S, you get this error message the
         * moment you say INDEX, that is, before even having to
         * complete the command. Doing that in free42 would require
         * a check in start_incomplete_command() or do_interactive().
         * I'm putting that off until I have a better idea of whether
         * this is an isolated special case (in which case I'll go
         * for the quick-and-dirty hack) or something more general
         * (in which case I'll redesign stuff if necessary).
         */
        return ERR_RESTRICTED_OPERATION;

    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    if (arg->type != ARGTYPE_STR)
        return ERR_INVALID_TYPE;

    m = recall_var(arg->val.text, arg->length);
    if (m == NULL)
        return ERR_NONEXISTENT;
    if (m->type != TYPE_REALMATRIX && m->type != TYPE_COMPLEXMATRIX)
        return ERR_INVALID_TYPE;

    /* TODO: keep a 'weak' lock on the matrix while it is indexed.
     * If it is deleted or redimensioned, I and J should be reset to 1.
     * Note that the current code uses a lazy, keep-it-safe approach
     * (it does not assume I and J are necessarily in bounds when I+, J-,
     * LEFT, RIGHT, etc., are called, but simply forces I and J within
     * bounds just before using them), but this does not always yield the
     * same result (try deleting the indexed matrix and then recreating it
     * immediately; on the real HP-42S, I and J are now both 1; on Free42,
     * I and J are unchanged).
     */
    matedit_mode = 1;
    matedit_i = 0;
    matedit_j = 0;
    matedit_length = arg->length;
    for (i = 0; i < matedit_length; i++)
        matedit_name[i] = arg->val.text[i];
    return ERR_NONE;
}

int docmd_uvec(arg_struct *arg) {
    phloat norm;
    int err;
    vartype *v;
    if (reg_x->type == TYPE_COMPLEXMATRIX)
        return ERR_INVALID_TYPE;
    if (reg_x->type == TYPE_COMPLEX) {
        vartype_complex *z = (vartype_complex *) reg_x;
        if (z->re == 0 && z->im == 0)
            return ERR_INVALID_DATA;
        else
            return docmd_sign(arg);
    }
    err = fnrm(reg_x, &norm);
    if (err != ERR_NONE)
        return err;
    if (norm == 0) {
        return ERR_INVALID_DATA;
    } else {
        vartype_realmatrix *src = (vartype_realmatrix *) reg_x;
        vartype_realmatrix *dst;
        int4 rows = src->rows;
        int4 columns = src->columns;
        int4 size = rows * columns;
        int4 i;
        dst = (vartype_realmatrix *) new_realmatrix(rows, columns);
        if (dst == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        for (i = 0; i < size; i++)
            dst->array->data[i] = src->array->data[i] / norm;
        v = (vartype *) dst;
    }
    unary_result(v);
    return ERR_NONE;
}
