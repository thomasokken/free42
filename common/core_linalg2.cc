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

#include "core_linalg2.h"
#include "core_globals.h"
#include "core_main.h"


#define STATE(s)             \
        if (--count <= 0) {  \
            dat->state = s;  \
            goto suspend;    \
        }                    \
        state##s:            \
        ;


/****************************/
/***** LU decomposition *****/
/****************************/

typedef struct {
    vartype_realmatrix *a;
    int4 *perm;
    phloat det;
    int4 i, imax, j;
    phloat max, tmp, *scale;
    int state;
    int (*completion)(int, vartype_realmatrix *, int4 *, phloat);
} lu_r_data_struct;

lu_r_data_struct *lu_r_data;

static int lu_decomp_r_worker(int interrupted);

int lu_decomp_r(vartype_realmatrix *a, int4 *perm,
                int (*completion)(int, vartype_realmatrix *, int4 *, phloat)) {
    lu_r_data_struct *dat =
                (lu_r_data_struct *) malloc(sizeof(lu_r_data_struct));

    if (dat == NULL)
        return completion(ERR_INSUFFICIENT_MEMORY, a, perm, 0);

    dat->scale = (phloat *) malloc(a->rows * sizeof(phloat));
    if (dat->scale == NULL) {
        free(dat);
        return completion(ERR_INSUFFICIENT_MEMORY, a, perm, 0);
    }

    dat->a = a;
    dat->perm = perm;
    dat->completion = completion;

    dat->state = 0;

    lu_r_data = dat;
    mode_interruptible = lu_decomp_r_worker;
    mode_stoppable = false;
    return ERR_INTERRUPTIBLE;
}

static int lu_decomp_r_worker(int interrupted) {
    
    lu_r_data_struct *dat = lu_r_data;

    phloat *a = dat->a->array->data;
    int4 n = dat->a->rows;
    phloat *scale = dat->scale;
    int4 *perm = dat->perm;
    int count = 1000;
    int err;

    int4 i = dat->i;
    int4 imax = dat->imax;
    int4 j = dat->j;
    phloat max = dat->max;
    phloat tmp = dat->tmp;
    phloat dot;

    if (interrupted) {
        free(scale);
        err = dat->completion(ERR_INTERRUPTED, dat->a, perm, 0);
        free(dat);
        return err;
    }

    switch (dat->state) {
        case 0: break;
        case 1: goto state1;
        case 2: goto state2;
        case 3: goto state3;
        case 4: goto state4;
        case 5: goto state5;
    }

    dat->det = 1;

    for (i = 0; i < n; i++) {
        max = 0;
        for (j = 0; j < n; j++) {
            tmp = a[i * n + j];
            if (tmp < 0)
                tmp = -tmp;
            if (tmp > max)
                max = tmp;
            STATE(1);
        }
        scale[i] = max;
    }

    for (j = 0; j < n; j++) {
        for (i = 0; i < j; i++) {
            compensated_dot_rr(flags.f.f19, i, a + i * n, 1, a + j, n, &dot);
            a[i * n + j] -= dot;
            count -= i;
            STATE(2);
        }

        max = 0;
        imax = j;
        for (i = j; i < n; i++) {
            compensated_dot_rr(flags.f.f19, j, a + i * n, 1, a + j, n, &dot);
            dot = a[i * n + j] - dot;
            a[i * n + j] = dot;
            if (scale[i] == 0) {
                imax = i;
                break;
            }
            tmp = (dot < 0 ? -dot : dot) / scale[i];
            if (tmp > max) {
                imax = i;
                max = tmp;
            }
            count -= j;
            STATE(3);
        }

        if (j != imax) {
            for (int4 k = 0; k < n; k++) {
                tmp = a[imax * n + k];
                a[imax * n + k] = a[j * n + k];
                a[j * n + k] = tmp;
            }
            STATE(4);
            dat->det = -dat->det;
            scale[imax] = scale[j];
        }

        perm[j] = imax;
        if (a[j * n + j] == 0) {
            if (core_settings.matrix_singularmatrix) {
                free(scale);
                err = dat->completion(ERR_SINGULAR_MATRIX, dat->a, perm, 0);
                free(dat);
                return err;
            } else {
                /* For a zero pivot, substitute a small positive number.
                 * I use a number that's about 10^-20 times the size of
                 * the maximum of the original column, with a minimum of
                 * 10^20 / POS_HUGE_PHLOAT.
                 */
                phloat tiniest = 1e20 / POS_HUGE_PHLOAT;
                phloat tiny;
                if (scale[j] == 0)
                    tiny = tiniest;
                else {
                    tiny = pow(10, floor(log10(scale[j])) - 20);
                    if (tiny < tiniest)
                        tiny = tiniest;
                }
                a[j * n + j] = tiny;
            }
        }
        dat->det *= a[j * n + j];
        if (j != n - 1) {
            tmp = 1 / a[j * n + j];
            for (i = j + 1; i < n; i++) {
                a[i * n + j] *= tmp;
                STATE(5);
            }
        }
    }

    free(scale);
    err = dat->completion(ERR_NONE, dat->a, perm, dat->det);
    free(dat);
    return err;

    suspend:
    dat->i = i;
    dat->imax = imax;
    dat->j = j;
    dat->max = max;
    dat->tmp = tmp;
    return ERR_INTERRUPTIBLE;
}


typedef struct {
    vartype_complexmatrix *a;
    int4 *perm;
    phloat det_re, det_im;
    int4 i, imax, j;
    phloat max, tmp, tmp_re, tmp_im, *scale;
    int state;
    int (*completion)(int, vartype_complexmatrix *, int4 *, phloat, phloat);
} lu_c_data_struct;

lu_c_data_struct *lu_c_data;

static int lu_decomp_c_worker(int interrupted);

int lu_decomp_c(vartype_complexmatrix *a, int4 *perm,
                int (*completion)(int, vartype_complexmatrix *,
                                          int4 *, phloat, phloat)) {
    lu_c_data_struct *dat =
                (lu_c_data_struct *) malloc(sizeof(lu_c_data_struct));

    if (dat == NULL)
        return completion(ERR_INSUFFICIENT_MEMORY, a, perm, 0, 0);

    dat->scale = (phloat *) malloc(a->rows * sizeof(phloat));
    if (dat->scale == NULL) {
        free(dat);
        return completion(ERR_INSUFFICIENT_MEMORY, a, perm, 0, 0);
    }

    dat->a = a;
    dat->perm = perm;
    dat->completion = completion;

    dat->state = 0;

    lu_c_data = dat;
    mode_interruptible = lu_decomp_c_worker;
    mode_stoppable = false;
    return ERR_INTERRUPTIBLE;
}

static int lu_decomp_c_worker(int interrupted) {
    
    lu_c_data_struct *dat = lu_c_data;

    phloat *a = dat->a->array->data;
    int4 n = dat->a->rows;
    phloat *scale = dat->scale;
    int4 *perm = dat->perm;
    int count = 1000;
    int err;

    int4 i = dat->i;
    int4 imax = dat->imax;
    int4 j = dat->j;
    phloat max = dat->max;
    phloat tmp = dat->tmp;
    phloat tmp_re = dat->tmp_re;
    phloat tmp_im = dat->tmp_im;
    phloat dot_re, dot_im;

    phloat xre, xim, yre, yim;
    phloat tiniest = 1e20 / POS_HUGE_PHLOAT;
    phloat tiny;
    phloat s_re, s_im;

    if (interrupted) {
        free(scale);
        err = dat->completion(ERR_INTERRUPTED, dat->a, perm, 0, 0);
        free(dat);
        return err;
    }

    switch (dat->state) {
        case 0: break;
        case 1: goto state1;
        case 2: goto state2;
        case 3: goto state3;
        case 4: goto state4;
        case 5: goto state5;
    }

    dat->det_re = 1;
    dat->det_im = 0;

    for (i = 0; i < n; i++) {
        max = 0;
        for (j = 0; j < n; j++) {
            tmp = hypot(a[2 * (i * n + j)], a[2 * (i * n + j) + 1]);
            if (tmp > max)
                max = tmp;
            STATE(1);
        }
        scale[i] = max;
    }

    for (j = 0; j < n; j++) {
        for (i = 0; i < j; i++) {
            compensated_dot_cc(flags.f.f19, i, a + 2 * i * n, 2, a + 2 * j, 2 * n, &dot_re, &dot_im);
            a[2 * (i * n + j)] -= dot_re;
            a[2 * (i * n + j) + 1] -= dot_im;
            count -= i;
            STATE(2);
        }

        max = 0;
        for (i = j; i < n; i++) {
            compensated_dot_cc(flags.f.f19, j, a + 2 * i * n, 2, a + 2 * j, 2 * n, &dot_re, &dot_im);
            dot_re = a[2 * (i * n + j)] - dot_re;
            dot_im = a[2 * (i * n + j) + 1] - dot_im;
            a[2 * (i * n + j)] = dot_re;
            a[2 * (i * n + j) + 1] = dot_im;
            if (scale[i] == 0) {
                imax = i;
                break;
            }
            tmp = hypot(dot_re, dot_im) / scale[i];
            if (tmp > max) {
                imax = i;
                max = tmp;
            }
            count -= j;
            STATE(3);
        }

        if (j != imax) {
            for (int4 k = 0; k < n; k++) {
                tmp = a[2 * (imax * n + k)];
                a[2 * (imax * n + k)] = a[2 * (j * n + k)];
                a[2 * (j * n + k)] = tmp;
                tmp = a[2 * (imax * n + k) + 1];
                a[2 * (imax * n + k) + 1] = a[2 * (j * n + k) + 1];
                a[2 * (j * n + k) + 1] = tmp;
            }
            STATE(4);
            dat->det_re = -dat->det_re;
            dat->det_im = -dat->det_im;
            scale[imax] = scale[j];
        }

        perm[j] = imax;
        tmp_re = a[2 * (j * n + j)];
        tmp_im = a[2 * (j * n + j) + 1];
        if (tmp_re == 0 && tmp_im == 0) {
            if (core_settings.matrix_singularmatrix) {
                free(scale);
                err = dat->completion(ERR_NONE, dat->a, perm, 0, 0);
                free(dat);
                return err;
            } else {
                /* For a zero pivot, substitute a small positive number.
                 * I use a number that's about 10^-20 times the size of
                 * the maximum of the original column, with a minimum of
                 * 10^20 / POS_HUGE_PHLOAT.
                 */
                if (scale[j] == 0)
                    tiny = tiniest;
                else {
                    tiny = pow(10, floor(log10(scale[j])) - 20);
                    if (tiny < tiniest)
                        tiny = tiniest;
                }
                a[2 * (j * n + j)] = tmp_re = tiny;
                a[2 * (j * n + j) + 1] = tmp_im = 0;
            }
        }
        tmp = dat->det_re * tmp_re - dat->det_im * tmp_im;
        dat->det_im = dat->det_im * tmp_re + dat->det_re * tmp_im;
        dat->det_re = tmp;
        if (j != n - 1) {
            tmp = hypot(tmp_re, tmp_im);
            s_re = tmp_re / tmp / tmp;
            s_im = -tmp_im / tmp / tmp;
            for (i = j + 1; i < n; i++) {
                tmp_re = a[2 * (i * n + j)];
                tmp_im = a[2 * (i * n + j) + 1];
                a[2 * (i * n + j)] = tmp_re * s_re - tmp_im * s_im;
                a[2 * (i * n + j) + 1] = tmp_im * s_re + tmp_re * s_im;
                STATE(5);
            }
        }
    }

    free(scale);
    err = dat->completion(ERR_NONE, dat->a, perm, dat->det_re, dat->det_im);
    free(dat);
    return err;

    suspend:
    dat->i = i;
    dat->imax = imax;
    dat->j = j;
    dat->max = max;
    dat->tmp = tmp;
    dat->tmp_re = tmp_re;
    dat->tmp_im = tmp_im;
    return ERR_INTERRUPTIBLE;
}


/*****************************/
/***** Back-substitution *****/
/*****************************/

typedef struct {
    vartype_realmatrix *a;
    int4 *perm;
    vartype_realmatrix *b;
    int4 i, ii, ll, k;
    int state;
    void (*completion)(int, vartype_realmatrix *, int4 *, vartype_realmatrix *);
} backsub_rr_data_struct;

static backsub_rr_data_struct *backsub_rr_data;

static int lu_backsubst_rr_worker(int interrupted);

int lu_backsubst_rr(vartype_realmatrix *a, int4 *perm, vartype_realmatrix *b,
                    void (*completion)(int, vartype_realmatrix *,
                                    int4 *, vartype_realmatrix *)) {
    backsub_rr_data_struct *dat =
            (backsub_rr_data_struct *) malloc(sizeof(backsub_rr_data_struct));

    if (dat == NULL) {
        completion(ERR_INSUFFICIENT_MEMORY, a, perm, b);
        return ERR_INSUFFICIENT_MEMORY;
    }

    dat->a = a;
    dat->perm = perm;
    dat->b = b;
    dat->completion = completion;

    dat->state = 0;

    backsub_rr_data = dat;
    mode_interruptible = lu_backsubst_rr_worker;
    mode_stoppable = false;
    return ERR_INTERRUPTIBLE;
}

static int lu_backsubst_rr_worker(int interrupted) {
    backsub_rr_data_struct *dat = backsub_rr_data;
    phloat *a = dat->a->array->data;
    int4 n = dat->a->rows;
    phloat *b = dat->b->array->data;
    int4 q = dat->b->columns;
    int4 *perm = dat->perm;
    int count = 1000;

    int4 i = dat->i;
    int4 ii = dat->ii;
    int4 ll = dat->ll;
    int4 k = dat->k;
    phloat sum, dot;

    phloat t;

    if (interrupted) {
        dat->completion(ERR_INTERRUPTED, dat->a, perm, dat->b);
        free(dat);
        return ERR_INTERRUPTED;
    }

    switch (dat->state) {
        case 0: break;
        case 1: goto state1;
        case 2: goto state2;
    }

    for (k = 0; k < q; k++) {
        ii = -1;
        for (i = 0; i < n; i++) {
            ll = perm[i];
            sum = b[ll * q + k];
            b[ll * q + k] = b[i * q + k];
            if (ii != -1) {
                compensated_dot_rr(flags.f.f18, i - ii, a + i * n + ii, 1, b + ii * q + k, q, &dot);
                sum -= dot;
                count -= i - ii;
            } else if (sum != 0)
                ii = i;
            b[i * q + k] = sum;
            STATE(1);
        }
        for (i = n - 1; i >= 0; i--) {
            compensated_dot_rr(flags.f.f18, n - i - 1, a + i * n + i + 1, 1, b + (i + 1) * q + k, q, &dot);
            dot = b[i * q + k] - dot;
            t = dot / a[i * n + i];
            if (p_isinf(t) || p_isnan(t)) {
                if (core_settings.matrix_outofrange
                                        && !flags.f.range_error_ignore)
                    return ERR_OUT_OF_RANGE;
                else
                    t = p_isinf(t) < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
            }
            b[i * q + k] = t;
            count -= n - i;
            STATE(2);
        }
    }

    dat->completion(ERR_NONE, dat->a, perm, dat->b);
    free(dat);
    return ERR_NONE;

    suspend:
    dat->i = i;
    dat->ii = ii;
    dat->ll = ll;
    dat->k = k;
    return ERR_INTERRUPTIBLE;
}

typedef struct {
    vartype_realmatrix *a;
    int4 *perm;
    vartype_complexmatrix *b;
    int4 i, ii, j, ll, k;
    phloat sum_re, sum_im;
    int state;
    void (*completion)(int, vartype_realmatrix *, int4 *,
                                            vartype_complexmatrix *);
} backsub_rc_data_struct;

static backsub_rc_data_struct *backsub_rc_data;

static int lu_backsubst_rc_worker(int interrupted);

int lu_backsubst_rc(vartype_realmatrix *a, int4 *perm, vartype_complexmatrix *b,
                    void (*completion)(int, vartype_realmatrix *,
                                int4 *, vartype_complexmatrix *)) {
    backsub_rc_data_struct *dat =
            (backsub_rc_data_struct *) malloc(sizeof(backsub_rc_data_struct));

    if (dat == NULL) {
        completion(ERR_INSUFFICIENT_MEMORY, a, perm, b);
        return ERR_INSUFFICIENT_MEMORY;
    }

    dat->a = a;
    dat->perm = perm;
    dat->b = b;
    dat->completion = completion;

    dat->state = 0;

    backsub_rc_data = dat;
    mode_interruptible = lu_backsubst_rc_worker;
    mode_stoppable = false;
    return ERR_INTERRUPTIBLE;
}

static int lu_backsubst_rc_worker(int interrupted) {
    backsub_rc_data_struct *dat = backsub_rc_data;
    phloat *a = dat->a->array->data;
    int4 n = dat->a->rows;
    phloat *b = dat->b->array->data;
    int4 q = dat->b->columns;
    int4 *perm = dat->perm;
    int count = 1000;

    int4 i = dat->i;
    int4 ii = dat->ii;
    int4 j = dat->j;
    int4 ll = dat->ll;
    int4 k = dat->k;
    phloat sum_re = dat->sum_re;
    phloat sum_im = dat->sum_im;
    phloat dot_re, dot_im, tmp;

    phloat t_re, t_im;

    if (interrupted) {
        dat->completion(ERR_INTERRUPTED, dat->a, perm, dat->b);
        free(dat);
        return ERR_INTERRUPTED;
    }

    switch (dat->state) {
        case 0: break;
        case 1: goto state1;
        case 2: goto state2;
    }

    for (k = 0; k < q; k++) {
        ii = -1;
        for (i = 0; i < n; i++) {
            ll = perm[i];
            sum_re = b[2 * (ll * q + k)];
            sum_im = b[2 * (ll * q + k) + 1];
            b[2 * (ll * q + k)] = b[2 * (i * q + k)];
            b[2 * (ll * q + k) + 1] = b[2 * (i * q + k) + 1];
            if (ii != -1) {
                compensated_dot_rc(flags.f.f18, i - ii, a + i * n + ii, 1, b + (2 * ii * q + k), 2 * q, &dot_re, &dot_im);
                sum_re -= dot_re;
                sum_im -= dot_im;
                count -= i - ii;
            } else if (sum_re != 0 || sum_im != 0)
                ii = i;
            b[2 * (i * q + k)] = sum_re;
            b[2 * (i * q + k) + 1] = sum_im;
            STATE(1);
        }
        for (i = n - 1; i >= 0; i--) {
            compensated_dot_rc(flags.f.f18, n - i - 1, a + i * n + i + 1, 1, b + 2 * ((i + 1) * q + k), 2 * q, &dot_re, &dot_im);
            dot_re = b[2 * (i * q + k)] - dot_re;
            dot_im = b[2 * (i * q + k) + 1] - dot_im;
            tmp = a[i * n + i];
            t_re = dot_re / tmp;
            t_im = dot_im / tmp;
            if (p_isinf(t_re) || p_isnan(t_re)) {
                if (core_settings.matrix_outofrange
                                        && !flags.f.range_error_ignore)
                    return ERR_OUT_OF_RANGE;
                else
                    t_re = p_isinf(t_re) < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
            }
            if (p_isinf(t_im) || p_isnan(t_im)) {
                if (core_settings.matrix_outofrange
                                        && !flags.f.range_error_ignore)
                    return ERR_OUT_OF_RANGE;
                else
                    t_im = p_isinf(t_im) < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
            }
            b[2 * (i * q + k)] = t_re;
            b[2 * (i * q + k) + 1] = t_im;
            count -= n - i;
            STATE(2);
        }
    }

    dat->completion(ERR_NONE, dat->a, perm, dat->b);
    free(dat);
    return ERR_NONE;

    suspend:
    dat->i = i;
    dat->ii = ii;
    dat->j = j;
    dat->ll = ll;
    dat->k = k;
    dat->sum_re = sum_re;
    dat->sum_im = sum_im;
    return ERR_INTERRUPTIBLE;
}

typedef struct {
    vartype_complexmatrix *a;
    int4 *perm;
    vartype_complexmatrix *b;
    int4 i, ii, j, ll, k;
    phloat sum_re, sum_im;
    int state;
    void (*completion)(int, vartype_complexmatrix *, int4 *,
                                            vartype_complexmatrix *);
} backsub_cc_data_struct;

static backsub_cc_data_struct *backsub_cc_data;

static int lu_backsubst_cc_worker(int interrupted);

int lu_backsubst_cc(vartype_complexmatrix *a, int4 *perm, vartype_complexmatrix *b,
                    void (*completion)(int, vartype_complexmatrix *,
                                int4 *, vartype_complexmatrix *)) {
    backsub_cc_data_struct *dat =
            (backsub_cc_data_struct *) malloc(sizeof(backsub_cc_data_struct));

    if (dat == NULL) {
        completion(ERR_INSUFFICIENT_MEMORY, a, perm, b);
        return ERR_INSUFFICIENT_MEMORY;
    }

    dat->a = a;
    dat->perm = perm;
    dat->b = b;
    dat->completion = completion;

    dat->state = 0;

    backsub_cc_data = dat;
    mode_interruptible = lu_backsubst_cc_worker;
    mode_stoppable = false;
    return ERR_INTERRUPTIBLE;
}

static int lu_backsubst_cc_worker(int interrupted) {
    backsub_cc_data_struct *dat = backsub_cc_data;
    phloat *a = dat->a->array->data;
    int4 n = dat->a->rows;
    phloat *b = dat->b->array->data;
    int4 q = dat->b->columns;
    int4 *perm = dat->perm;
    int count = 1000;

    int4 i = dat->i;
    int4 ii = dat->ii;
    int4 j = dat->j;
    int4 ll = dat->ll;
    int4 k = dat->k;
    phloat sum_re = dat->sum_re;
    phloat sum_im = dat->sum_im;
    phloat dot_re, dot_im, tmp, tmp_re, tmp_im;

    phloat bre, bim;
    phloat t_re, t_im;

    if (interrupted) {
        dat->completion(ERR_INTERRUPTED, dat->a, perm, dat->b);
        free(dat);
        return ERR_INTERRUPTED;
    }

    switch (dat->state) {
        case 0: break;
        case 1: goto state1;
        case 2: goto state2;
    }

    for (k = 0; k < q; k++) {
        ii = -1;
        for (i = 0; i < n; i++) {
            ll = perm[i];
            sum_re = b[2 * (ll * q + k)];
            sum_im = b[2 * (ll * q + k) + 1];
            b[2 * (ll * q + k)] = b[2 * (i * q + k)];
            b[2 * (ll * q + k) + 1] = b[2 * (i * q + k) + 1];
            if (ii != -1) {
                compensated_dot_cc(flags.f.f18, i - ii, a + 2 * (i * n + ii), 2, b + (2 * ii * q + k), 2 * q, &dot_re, &dot_im);
                sum_re -= dot_re;
                sum_im -= dot_im;
                count -= i - ii;
            } else if (sum_re != 0 || sum_im != 0)
                ii = i;
            b[2 * (i * q + k)] = sum_re;
            b[2 * (i * q + k) + 1] = sum_im;
            STATE(1);
        }
        for (i = n - 1; i >= 0; i--) {
            compensated_dot_cc(flags.f.f18, n - i - 1, a + 2 * (i * n + i + 1), 2, b + 2 * ((i + 1) * q + k), 2 * q, &dot_re, &dot_im);
            dot_re = b[2 * (i * q + k)] - dot_re;
            dot_im = b[2 * (i * q + k) + 1] - dot_im;
            tmp_re = a[2 * (i * n + i)];
            tmp_im = a[2 * (i * n + i) + 1];
            tmp = hypot(tmp_re, tmp_im);
            tmp_re = tmp_re / tmp / tmp;
            tmp_im = -tmp_im / tmp / tmp;
            t_re = sum_re * tmp_re - sum_im * tmp_im;
            t_im = sum_im * tmp_re + sum_re * tmp_im;
            if (p_isinf(t_re) || p_isnan(t_re)) {
                if (core_settings.matrix_outofrange
                                        && !flags.f.range_error_ignore)
                    return ERR_OUT_OF_RANGE;
                else
                    t_re = p_isinf(t_re) < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
            }
            if (p_isinf(t_im) || p_isnan(t_im)) {
                if (core_settings.matrix_outofrange
                                        && !flags.f.range_error_ignore)
                    return ERR_OUT_OF_RANGE;
                else
                    t_im = p_isinf(t_im) < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
            }
            b[2 * (i * q + k)] = t_re;
            b[2 * (i * q + k) + 1] = t_im;
            count -= n - i;
            STATE(2);
        }
    }

    dat->completion(ERR_NONE, dat->a, perm, dat->b);
    free(dat);
    return ERR_NONE;

    suspend:
    dat->i = i;
    dat->ii = ii;
    dat->j = j;
    dat->ll = ll;
    dat->k = k;
    dat->sum_re = sum_re;
    dat->sum_im = sum_im;
    return ERR_INTERRUPTIBLE;
}

void compensated_dot_rr(bool uncomp, int n,
                        const phloat *x, size_t xoff,
                        const phloat *y, size_t yoff,
                        phloat *res) {
    if (uncomp) {
        phloat r = 0;
        for (int i = 0; i < n; i++)
            r += x[i * xoff] * y[i * yoff];
        *res = r;
        return;
    }
    phloat s = *x * *y;
    phloat c = fma(*x, *y, -s);
    for (int i = 1; i < n; i++) {
        x += xoff;
        y += yoff;
        phloat p = *x * *y;
        phloat pp = fma(*x, *y, -p);
        phloat xx = s + p;
        phloat zz = xx - s;
        phloat ss = (s - (xx - zz)) + (p - zz);
        s = xx;
        c = c + (pp + ss);
    }
    *res = s + c;
}

void compensated_dot_rc(bool uncomp, int n,
                        const phloat *x, size_t xoff,
                        const phloat *ry, size_t yoff,
                        phloat *rres, phloat *cres) {
    if (uncomp) {
        phloat rre = 0, rim = 0;
        for (int i = 0; i < n; i++) {
            rre += x[i * xoff] * ry[i * yoff];
            rim += x[i * xoff] * ry[i * yoff + 1];
        }
        *rres = rre;
        *cres = rim;
        return;
    }
    phloat rs = *x * *ry;
    phloat cs = *x * *(ry + 1);
    phloat rc = fma(*x, *ry, -rs);
    phloat cc = fma(*x, *(ry + 1), -cs);
    for (int i = 1; i < n; i++) {
        x += xoff;
        ry += yoff;
        phloat p = *x * *ry;
        phloat pp = fma(*x, *ry, -p);
        phloat xx = rs + p;
        phloat zz = xx - rs;
        phloat ss = (rs - (xx - zz)) + (p - zz);
        rs = xx;
        rc = rc + (pp + ss);
        p = *x * *(ry + 1);
        pp = fma(*x, *(ry + 1), -p);
        xx = cs + p;
        zz = xx - cs;
        ss = (cs - (xx - zz)) + (p - zz);
        cs = xx;
        cc = cc + (pp + ss);
    }
    *rres = rs + rc;
    *cres = cs + cc;
}

void compensated_dot_cc(bool uncomp, int n,
                        const phloat *rx, size_t xoff,
                        const phloat *ry, size_t yoff,
                        phloat *rres, phloat *cres) {
    if (uncomp) {
        phloat rre = 0, rim = 0;
        for (int i = 0; i < n; i++) {
            rre += rx[i * xoff] * ry[i * yoff] - rx[i * xoff + 1] * ry[i * yoff + 1];
            rim += rx[i * xoff] * ry[i * yoff + 1] + rx[i * xoff + 1] * ry[i * yoff];
        }
        *rres = rre;
        *cres = rim;
        return;
    }
    phloat rs = *rx * *ry;
    phloat cs = *rx * *(ry + 1);
    phloat rc = fma(*rx, *ry, -rs);
    phloat cc = fma(*rx, *(ry + 1), -cs);
    int i = 0;
    phloat p, pp, xx, zz, ss;
    goto skip;
    for (; i < n; i++) {
        rx += xoff;
        ry += yoff;
        p = *rx * *ry;
        pp = fma(*rx, *ry, -p);
        xx = rs + p;
        zz = xx - rs;
        ss = (rs - (xx - zz)) + (p - zz);
        rs = xx;
        rc = rc + (pp + ss);
        p = *rx * *(ry + 1);
        pp = fma(*rx, *(ry + 1), -p);
        xx = cs + p;
        zz = xx - cs;
        ss = (cs - (xx - zz)) + (p - zz);
        cs = xx;
        cc = cc + (pp + ss);
        skip:
        p = -(*(rx + 1) * *(ry + 1));
        pp = -fma(*(rx + 1), *(ry + 1), p);
        xx = rs + p;
        zz = xx - rs;
        ss = (rs - (xx - zz)) + (p - zz);
        rs = xx;
        rc = rc + (pp + ss);
        p = *(rx + 1) * *ry;
        pp = fma(*(rx + 1), *ry, -p);
        xx = cs + p;
        zz = xx - cs;
        ss = (cs - (xx - zz)) + (p - zz);
        cs = xx;
        cc = cc + (pp + ss);
    }
    *rres = rs + rc;
    *cres = cs + cc;
}
