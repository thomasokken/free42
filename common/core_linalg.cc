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

#include <stdlib.h>

#include "core_linalg.h"
#include "core_decimal.h"
#include "core_main.h"
#include "core_variables.h"


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
    int4 i, imax, j, k;
    phloat max, tmp, sum, *scale;
    int state;
    int (*completion)(int, vartype_realmatrix *, int4 *, phloat);
} lu_r_data_struct;

lu_r_data_struct *lu_r_data;

static int lu_decomp_r(vartype_realmatrix *a, int4 *perm,
		       int (*completion)(int, vartype_realmatrix *,
					  int4 *, phloat)) LINALG_SECT;
static int lu_decomp_r_worker(int interrupted) LINALG_SECT;

static int lu_decomp_r(vartype_realmatrix *a, int4 *perm,
		       int (*completion)(int, vartype_realmatrix *,
					  int4 *, phloat)) {
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
    mode_stoppable = 0;
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
    int4 k = dat->k;
    phloat max = dat->max;
    phloat tmp = dat->tmp;
    phloat sum = dat->sum;

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
	    sum = a[i * n + j];
	    for (k = 0; k < i; k++) {
		sum -= a[i * n + k] * a[k * n + j];
		STATE(2);
	    }
	    a[i * n + j] = sum;
	}

	max = 0;
	imax = j;
	for (i = j; i < n; i++) {
	    sum = a[i * n + j];
	    for (k = 0; k < j; k++) {
		sum -= a[i * n + k] * a[k * n + j];
		STATE(3);
	    }
	    a[i * n  + j] = sum;
	    if (scale[i] == 0) {
		imax = i;
		break;
	    }
	    tmp = (sum < 0 ? -sum : sum) / scale[i];
	    if (tmp > max) {
		imax = i;
		max = tmp;
	    }
	}

	if (j != imax) {
	    for (k = 0; k < n; k++) {
		tmp = a[imax * n + k];
		a[imax * n + k] = a[j * n + k];
		a[j * n + k] = tmp;
		STATE(4);
	    }
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
    dat->k = k;
    dat->max = max;
    dat->tmp = tmp;
    dat->sum = sum;
    return ERR_INTERRUPTIBLE;
}


typedef struct {
    vartype_complexmatrix *a;
    int4 *perm;
    phloat det_re, det_im;
    int4 i, imax, j, k;
    phloat max, tmp, tmp_re, tmp_im, sum_re, sum_im, *scale;
    int state;
    int (*completion)(int, vartype_complexmatrix *, int4 *, phloat, phloat);
} lu_c_data_struct;

lu_c_data_struct *lu_c_data;

static int lu_decomp_c(vartype_complexmatrix *a, int4 *perm,
		       int (*completion)(int, vartype_complexmatrix *,
					  int4 *, phloat, phloat)) LINALG_SECT;
static int lu_decomp_c_worker(int interrupted) LINALG_SECT;

static int lu_decomp_c(vartype_complexmatrix *a, int4 *perm,
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
    mode_stoppable = 0;
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
    int4 k = dat->k;
    phloat max = dat->max;
    phloat tmp = dat->tmp;
    phloat tmp_re = dat->tmp_re;
    phloat tmp_im = dat->tmp_im;
    phloat sum_re = dat->sum_re;
    phloat sum_im = dat->sum_im;

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
	    sum_re = a[2 * (i * n + j)];
	    sum_im = a[2 * (i * n + j) + 1];
	    for (k = 0; k < i; k++) {
		xre = a[2 * (i * n + k)];
		xim = a[2 * (i * n + k) + 1];
		yre = a[2 * (k * n + j)];
		yim = a[2 * (k * n + j) + 1];
		sum_re -= xre * yre - xim * yim;
		sum_im -= xim * yre + xre * yim;
		STATE(2);
	    }
	    a[2 * (i * n + j)] = sum_re;
	    a[2 * (i * n + j) + 1] = sum_im;
	}

	max = 0;
	for (i = j; i < n; i++) {
	    sum_re = a[2 * (i * n + j)];
	    sum_im = a[2 * (i * n + j) + 1];
	    for (k = 0; k < j; k++) {
		xre = a[2 * (i * n + k)];
		xim = a[2 * (i * n + k) + 1];
		yre = a[2 * (k * n + j)];
		yim = a[2 * (k * n + j) + 1];
		sum_re -= xre * yre - xim * yim;
		sum_im -= xim * yre + xre * yim;
		STATE(3);
	    }
	    a[2 * (i * n + j)] = sum_re;
	    a[2 * (i * n + j) + 1] = sum_im;
	    if (scale[i] == 0) {
		imax = i;
		break;
	    }
	    tmp = hypot(sum_re, sum_im) / scale[i];
	    if (tmp > max) {
		imax = i;
		max = tmp;
	    }
	}

	if (j != imax) {
	    for (k = 0; k < n; k++) {
		tmp = a[2 * (imax * n + k)];
		a[2 * (imax * n + k)] = a[2 * (j * n + k)];
		a[2 * (j * n + k)] = tmp;
		tmp = a[2 * (imax * n + k) + 1];
		a[2 * (imax * n + k) + 1] = a[2 * (j * n + k) + 1];
		a[2 * (j * n + k) + 1] = tmp;
		STATE(4);
	    }
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
    dat->k = k;
    dat->max = max;
    dat->tmp = tmp;
    dat->tmp_re = tmp_re;
    dat->tmp_im = tmp_im;
    dat->sum_re = sum_re;
    dat->sum_im = sum_im;
    return ERR_INTERRUPTIBLE;
}


/*****************************/
/***** Back-substitution *****/
/*****************************/

typedef struct {
    vartype_realmatrix *a;
    int4 *perm;
    vartype_realmatrix *b;
    int4 i, ii, j, ll, k;
    phloat sum;
    int state;
    void (*completion)(int, vartype_realmatrix *, int4 *, vartype_realmatrix *);
} backsub_rr_data_struct;

static backsub_rr_data_struct *backsub_rr_data;

static int lu_backsubst_rr(vartype_realmatrix *a,
			    int4 *perm,
			    vartype_realmatrix *b,
			    void (*completion)(int, vartype_realmatrix *,
				    int4 *, vartype_realmatrix *)) LINALG_SECT;
static int lu_backsubst_rr_worker(int interrupted) LINALG_SECT;

static int lu_backsubst_rr(vartype_realmatrix *a,
			    int4 *perm,
			    vartype_realmatrix *b,
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
    mode_stoppable = 0;
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
    int4 j = dat->j;
    int4 ll = dat->ll;
    int4 k = dat->k;
    phloat sum = dat->sum;

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
		for (j = ii; j < i; j++) {
		    sum -= a[i * n + j] * b[j * q + k];
		    STATE(1);
		}
	    } else if (sum != 0)
		ii = i;
	    b[i * q + k] = sum;
	}
	for (i = n - 1; i >= 0; i--) {
	    sum = b[i * q + k];
	    for (j = i + 1; j < n; j++) {
		sum -= a[i * n + j] * b[j * q + k];
		STATE(2);
	    }
	    t = sum / a[i * n + i];
	    if (p_isinf(t) || p_isnan(t)) {
		if (core_settings.matrix_outofrange
					&& !flags.f.range_error_ignore)
		    return ERR_OUT_OF_RANGE;
		else
		    t = p_isinf(t) < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
	    }
	    b[i * q + k] = t;
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
    dat->sum = sum;
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

static int lu_backsubst_rc(vartype_realmatrix *a,
			    int4 *perm,
			    vartype_complexmatrix *b,
			    void (*completion)(int, vartype_realmatrix *,
				int4 *, vartype_complexmatrix *)) LINALG_SECT;
static int lu_backsubst_rc_worker(int interrupted) LINALG_SECT;

static int lu_backsubst_rc(vartype_realmatrix *a,
			    int4 *perm,
			    vartype_complexmatrix *b,
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
    mode_stoppable = 0;
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
    phloat tmp;

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
		for (j = ii; j < i; j++) {
		    tmp = a[i * n + j];
		    sum_re -= tmp * b[2 * (j * q + k)];
		    sum_im -= tmp * b[2 * (j * q + k) + 1];
		    STATE(1);
		}
	    } else if (sum_re != 0 || sum_im != 0)
		ii = i;
	    b[2 * (i * q + k)] = sum_re;
	    b[2 * (i * q + k) + 1] = sum_im;
	}
	for (i = n - 1; i >= 0; i--) {
	    sum_re = b[2 * (i * q + k)];
	    sum_im = b[2 * (i * q + k) + 1];
	    for (j = i + 1; j < n; j++) {
		tmp = a[i * n + j];
		sum_re -= tmp * b[2 * (j * q + k)];
		sum_im -= tmp * b[2 * (j * q + k) + 1];
		STATE(2);
	    }
	    tmp = a[i * n + i];
	    t_re = sum_re / tmp;
	    t_im = sum_im / tmp;
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

static int lu_backsubst_cc(vartype_complexmatrix *a,
			    int4 *perm,
			    vartype_complexmatrix *b,
			    void (*completion)(int, vartype_complexmatrix *,
				int4 *, vartype_complexmatrix *)) LINALG_SECT;
static int lu_backsubst_cc_worker(int interrupted) LINALG_SECT;

static int lu_backsubst_cc(vartype_complexmatrix *a,
			    int4 *perm,
			    vartype_complexmatrix *b,
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
    mode_stoppable = 0;
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
    phloat tmp, tmp_re, tmp_im;

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
		for (j = ii; j < i; j++) {
		    bre = b[2 * (j * q + k)];
		    bim = b[2 * (j * q + k) + 1];
		    tmp_re = a[2 * (i * n + j)];
		    tmp_im = a[2 * (i * n + j) + 1];
		    sum_re -= bre * tmp_re - bim * tmp_im;
		    sum_im -= bim * tmp_re + bre * tmp_im;
		    STATE(1);
		}
	    } else if (sum_re != 0 || sum_im != 0)
		ii = i;
	    b[2 * (i * q + k)] = sum_re;
	    b[2 * (i * q + k) + 1] = sum_im;
	}
	for (i = n - 1; i >= 0; i--) {
	    sum_re = b[2 * (i * q + k)];
	    sum_im = b[2 * (i * q + k) + 1];
	    for (j = i + 1; j < n; j++) {
		bre = b[2 * (j * q + k)];
		bim = b[2 * (j * q + k) + 1];
		tmp_re = a[2 * (i * n + j)];
		tmp_im = a[2 * (i * n + j) + 1];
		sum_re -= bre * tmp_re - bim * tmp_im;
		sum_im -= bim * tmp_re + bre * tmp_im;
		STATE(2);
	    }
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


/**********************************/
/***** Matrix-matrix division *****/
/**********************************/

static void (*linalg_div_completion)(int, vartype *);
static const vartype *linalg_div_left;
static vartype *linalg_div_result;

static int div_rr_completion1(int error, vartype_realmatrix *a, int4 *perm,
				    phloat det) LINALG_SECT;
static void div_rr_completion2(int error, vartype_realmatrix *a, int4 *perm,
				    vartype_realmatrix *b) LINALG_SECT;
static int div_rc_completion1(int error, vartype_complexmatrix *a, int4 *perm,
				    phloat det_re, phloat det_im) LINALG_SECT;
static void div_rc_completion2(int error, vartype_complexmatrix *a, int4 *perm,
				    vartype_complexmatrix *b) LINALG_SECT;
static int div_cr_completion1(int error, vartype_realmatrix *a, int4 *perm,
				    phloat det) LINALG_SECT;
static void div_cr_completion2(int error, vartype_realmatrix *a, int4 *perm,
				    vartype_complexmatrix *b) LINALG_SECT;
static int div_cc_completion1(int error, vartype_complexmatrix *a, int4 *perm,
				    phloat det_re, phloat det_im) LINALG_SECT;
static void div_cc_completion2(int error, vartype_complexmatrix *a, int4 *perm,
				    vartype_complexmatrix *b) LINALG_SECT;

int linalg_div(const vartype *left, const vartype *right,
				    void (*completion)(int, vartype *)) {
    if (left->type == TYPE_REALMATRIX) {
	if (right->type == TYPE_REALMATRIX) {
	    vartype_realmatrix *num = (vartype_realmatrix *) left;
	    vartype_realmatrix *denom = (vartype_realmatrix *) right;
	    vartype *lu, *res;
	    int4 rows = num->rows;
	    int4 columns = num->columns;
	    int4 *perm;
	    if (denom->rows != rows || denom->columns != rows) {
		completion(ERR_DIMENSION_ERROR, NULL);
		return ERR_DIMENSION_ERROR;
	    }
	    perm = (int4 *) malloc(rows * sizeof(int4));
	    if (perm == NULL) {
		completion(ERR_INSUFFICIENT_MEMORY, NULL);
		return ERR_INSUFFICIENT_MEMORY;
	    }
	    lu = new_realmatrix(rows, rows);
	    if (lu == NULL) {
		free(perm);
		completion(ERR_INSUFFICIENT_MEMORY, NULL);
		return ERR_INSUFFICIENT_MEMORY;
	    }
	    res = new_realmatrix(rows, columns);
	    if (res == NULL) {
		free(perm);
		free_vartype(lu);
		completion(ERR_INSUFFICIENT_MEMORY, NULL);
		return ERR_INSUFFICIENT_MEMORY;
	    }
	    matrix_copy(lu, right);
	    linalg_div_completion = completion;
	    linalg_div_left = left;
	    linalg_div_result = res;
	    return lu_decomp_r((vartype_realmatrix *) lu, perm,
						div_rr_completion1); 
	} else {
	    vartype_realmatrix *num = (vartype_realmatrix *) left;
	    vartype_complexmatrix *denom = (vartype_complexmatrix *) right;
	    vartype *lu, *res;
	    int4 rows = num->rows;
	    int4 columns = num->columns;
	    int4 *perm;
	    if (denom->rows != rows || denom->columns != rows) {
		completion(ERR_DIMENSION_ERROR, NULL);
		return ERR_DIMENSION_ERROR;
	    }
	    perm = (int4 *) malloc(rows * sizeof(int4));
	    if (perm == NULL) {
		completion(ERR_INSUFFICIENT_MEMORY, NULL);
		return ERR_INSUFFICIENT_MEMORY;
	    }
	    lu = new_complexmatrix(rows, rows);
	    if (lu == NULL) {
		free(perm);
		completion(ERR_INSUFFICIENT_MEMORY, NULL);
		return ERR_INSUFFICIENT_MEMORY;
	    }
	    res = new_complexmatrix(rows, columns);
	    if (res == NULL) {
		free(perm);
		free_vartype(lu);
		completion(ERR_INSUFFICIENT_MEMORY, NULL);
		return ERR_INSUFFICIENT_MEMORY;
	    }
	    matrix_copy(lu, right);
	    linalg_div_completion = completion;
	    linalg_div_left = left;
	    linalg_div_result = res;
	    return lu_decomp_c((vartype_complexmatrix *) lu, perm,
						div_rc_completion1);
	}
    } else {
	if (right->type == TYPE_REALMATRIX) {
	    vartype_complexmatrix *num = (vartype_complexmatrix *) left;
	    vartype_realmatrix *denom = (vartype_realmatrix *) right;
	    vartype *lu, *res;
	    int4 rows = num->rows;
	    int4 columns = num->columns;
	    int4 *perm;
	    if (denom->rows != rows || denom->columns != rows) {
		completion(ERR_DIMENSION_ERROR, 0);
		return ERR_DIMENSION_ERROR;
	    }
	    perm = (int4 *) malloc(rows * sizeof(int4));
	    if (perm == NULL) {
		completion(ERR_INSUFFICIENT_MEMORY, NULL);
		return ERR_INSUFFICIENT_MEMORY;
	    }
	    lu = new_realmatrix(rows, rows);
	    if (lu == NULL) {
		free(perm);
		completion(ERR_INSUFFICIENT_MEMORY, NULL);
		return ERR_INSUFFICIENT_MEMORY;
	    }
	    res = new_complexmatrix(rows, columns);
	    if (res == NULL) {
		free(perm);
		free_vartype(lu);
		completion(ERR_INSUFFICIENT_MEMORY, NULL);
		return ERR_INSUFFICIENT_MEMORY;
	    }
	    matrix_copy(lu, right);
	    linalg_div_completion = completion;
	    linalg_div_left = left;
	    linalg_div_result = res;
	    return lu_decomp_r((vartype_realmatrix *) lu, perm,
						    div_cr_completion1);
	} else {
	    vartype_complexmatrix *num = (vartype_complexmatrix *) left;
	    vartype_complexmatrix *denom = (vartype_complexmatrix *) right;
	    vartype *lu, *res;
	    int4 rows = num->rows;
	    int4 columns = num->columns;
	    int4 *perm;
	    if (denom->rows != rows || denom->columns != rows) {
		completion(ERR_DIMENSION_ERROR, NULL);
		return ERR_DIMENSION_ERROR;
	    }
	    perm = (int4 *) malloc(rows * sizeof(int4));
	    if (perm == NULL) {
		completion(ERR_INSUFFICIENT_MEMORY, NULL);
		return ERR_INSUFFICIENT_MEMORY;
	    }
	    lu = new_complexmatrix(rows, rows);
	    if (lu == NULL) {
		free(perm);
		completion(ERR_INSUFFICIENT_MEMORY, NULL);
		return ERR_INSUFFICIENT_MEMORY;
	    }
	    res = new_complexmatrix(rows, columns);
	    if (res == NULL) {
		free(perm);
		free_vartype(lu);
		completion(ERR_INSUFFICIENT_MEMORY, NULL);
		return ERR_INSUFFICIENT_MEMORY;
	    }
	    matrix_copy(lu, right);
	    linalg_div_completion = completion;
	    linalg_div_left = left;
	    linalg_div_result = res;
	    return lu_decomp_c((vartype_complexmatrix *) lu, perm,
						    div_cc_completion1);
	}
    }
}

static int div_rr_completion1(int error, vartype_realmatrix *a, int4 *perm,
					 phloat det) {
    if (error != ERR_NONE) {
	free_vartype((vartype *) a);
	free(perm);
	free_vartype(linalg_div_result);
	return error;
    } else {
	matrix_copy(linalg_div_result, linalg_div_left);
	return lu_backsubst_rr(a, perm,
				(vartype_realmatrix *) linalg_div_result,
				div_rr_completion2);
    }
}

static void div_rr_completion2(int error, vartype_realmatrix *a, int4 *perm,
					  vartype_realmatrix *b) {
    if (error != ERR_NONE)
	free_vartype(linalg_div_result); /* Note: linalg_div_result == b */
    free_vartype((vartype *) a);
    free(perm);
    linalg_div_completion(error, linalg_div_result);
}

static int div_rc_completion1(int error, vartype_complexmatrix *a, int4 *perm,
					 phloat det_re, phloat det_im) {
    if (error != ERR_NONE) {
	free_vartype((vartype *) a);
	free(perm);
	free_vartype(linalg_div_result);
	return error;
    } else {
	matrix_copy(linalg_div_result, linalg_div_left);
	return lu_backsubst_cc(a, perm,
				(vartype_complexmatrix *) linalg_div_result,
				div_rc_completion2);
    }
}

static void div_rc_completion2(int error, vartype_complexmatrix *a, int4 *perm,
					  vartype_complexmatrix *b) {
    if (error != ERR_NONE)
	free_vartype(linalg_div_result); /* Note: linalg_div_result == b */
    free_vartype((vartype *) a);
    free(perm);
    linalg_div_completion(error, linalg_div_result);
}

static int div_cr_completion1(int error, vartype_realmatrix *a, int4 *perm,
				    phloat det) {
    if (error != ERR_NONE) {
	free_vartype((vartype *) a);
	free(perm);
	free_vartype(linalg_div_result);
	return error;
    } else {
	matrix_copy(linalg_div_result, linalg_div_left);
	return lu_backsubst_rc(a, perm,
				(vartype_complexmatrix *) linalg_div_result,
				div_cr_completion2);
    }
}

static void div_cr_completion2(int error, vartype_realmatrix *a, int4 *perm,
				    vartype_complexmatrix *b) {
    if (error != ERR_NONE)
	free_vartype(linalg_div_result); /* Note: linalg_div_result == b */
    free_vartype((vartype *) a);
    free(perm);
    linalg_div_completion(error, linalg_div_result);
}

static int div_cc_completion1(int error, vartype_complexmatrix *a, int4 *perm,
				    phloat det_re, phloat det_im) {
    if (error != ERR_NONE) {
	free_vartype((vartype *) a);
	free(perm);
	free_vartype(linalg_div_result);
	return error;
    } else {
	matrix_copy(linalg_div_result, linalg_div_left);
	return lu_backsubst_cc(a, perm,
				(vartype_complexmatrix *) linalg_div_result,
				div_cc_completion2);
    }
}

static void div_cc_completion2(int error, vartype_complexmatrix *a, int4 *perm,
				    vartype_complexmatrix *b) {
    if (error != ERR_NONE)
	free_vartype(linalg_div_result); /* Note: linalg_div_result == b */
    free_vartype((vartype *) a);
    free(perm);
    linalg_div_completion(error, linalg_div_result);
}


/****************************************/
/***** Matrix-matrix multiplication *****/
/****************************************/

typedef struct {
    vartype_realmatrix *left;
    vartype_realmatrix *right;
    vartype *result;
    int4 i, j, k;
    phloat sum;
    void (*completion)(int error, vartype *result);
} mul_rr_data_struct;

static mul_rr_data_struct *mul_rr_data;

static int matrix_mul_rr(vartype_realmatrix *left, vartype_realmatrix *right,
			 void (*completion)(int, vartype *)) LINALG_SECT;
static int matrix_mul_rr_worker(int interrupted) LINALG_SECT;

static int matrix_mul_rr(vartype_realmatrix *left, vartype_realmatrix *right,
			 void (*completion)(int, vartype *)) {

    mul_rr_data_struct *dat;
    int error;

    if (left->columns != right->rows) {
	error = ERR_DIMENSION_ERROR;
	goto finished;
    }

    if (!contains_no_strings(left) || !contains_no_strings(right)) {
	error = ERR_ALPHA_DATA_IS_INVALID;
	goto finished;
    }

    dat = (mul_rr_data_struct *) malloc(sizeof(mul_rr_data_struct));
    if (dat == NULL) {
	error = ERR_INSUFFICIENT_MEMORY;
	goto finished;
    }

    dat->result = new_realmatrix(left->rows, right->columns);
    if (dat->result == NULL) {
	free(dat);
	error = ERR_INSUFFICIENT_MEMORY;
	goto finished;
    }

    dat->left = left;
    dat->right = right;
    dat->i = 0;
    dat->j = 0;
    dat->k = 0;
    dat->sum = 0;
    dat->completion = completion;

    mul_rr_data = dat;
    mode_interruptible = matrix_mul_rr_worker;
    mode_stoppable = 0;
    return ERR_INTERRUPTIBLE;

    finished:
    completion(error, NULL);
    return error;
}

static int matrix_mul_rr_worker(int interrupted) {
    mul_rr_data_struct *dat = mul_rr_data;
    int count = 0;
    int inf;
    phloat *l = dat->left->array->data;
    phloat *r = dat->right->array->data;
    phloat *p = ((vartype_realmatrix *) dat->result)->array->data;
    int4 i = dat->i;
    int4 j = dat->j;
    int4 k = dat->k;
    int4 m = dat->left->rows;
    int4 n = dat->right->columns;
    int4 q = dat->left->columns;
    phloat sum = dat->sum;

    if (interrupted) {
	dat->completion(ERR_INTERRUPTED, NULL);
	free_vartype(dat->result);
	free(dat);
	return ERR_INTERRUPTED;
    }

    while (count++ < 1000) {
	sum += l[i * q + k] * r[k * n + j];
	if (++k < q)
	    continue;
	k = 0;
	if ((inf = p_isinf(sum)) != 0) {
	    if (core_settings.matrix_outofrange && !flags.f.range_error_ignore){
		dat->completion(ERR_OUT_OF_RANGE, NULL);
		free_vartype(dat->result);
		free(dat);
		return ERR_OUT_OF_RANGE;
	    } else
		sum = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
	}
	p[i * n + j] = sum;
	sum = 0;
	if (++j < n)
	    continue;
	j = 0;
	if (++i < m)
	    continue;
	else {
	    dat->completion(ERR_NONE, dat->result);
	    free(dat);
	    return ERR_NONE;
	}
    }

    dat->i = i;
    dat->j = j;
    dat->k = k;
    dat->sum = sum;
    return ERR_INTERRUPTIBLE;
}

#if 0
/* TODO: Blocked matrix multiplication
 * This commented-out function implements a working blocked matrix
 * multiplication algorithm. It works by breaking up the multiplication into
 * multiplications of submatrices of the multiplicands. If the submatrices are
 * sufficiently small, they can be made to fit entirely in the CPU's L1 cache.
 * With block sizes of around 45, I have observed a speed-up factor of circa
 * 1.7 on a Pentium MMX, and with block sizes of about 90, I have observed a
 * speed-up factor of circa 3 on a Duron. (Those were the optimum block sizes
 * observed for each processor.) These numbers suggest the Pentium MMX has a 32
 * kilobyte L1 cache, and the Duron has a 128 kilobyte L1 cache.
 * For production use, the block size should be a user-configurable parameter,
 * with a built-in tool to automatically determine the optimum value. Since the
 * UI for this kind of advanced configuration does not exist yet, and sice the
 * determination of optimum block size is a very slow process, and since I
 * expect most HP-42S users will not be very interested in using a calculator
 * to work with order-100+ matrices, and since even in the optimum case the
 * speed-up is not staggering, I'm putting full implementation of this feature
 * off until some later date.
 */
static int matrix_mul_rr(vartype_realmatrix *left, vartype_realmatrix *right,
			 vartype **result) {
    phloat *cache = NULL;
    phloat *leftcache, *rightcache;

    phloat *l = left->array->data;
    phloat *r = right->array->data;
    phloat *p;
    int inf;
    int4 m, n, q;
    int4 i, j, k;

    q = left->columns;
    if (q != right->rows)
	return ERR_DIMENSION_ERROR;
    m = left->rows;
    n = right->columns;

    if (!contains_no_strings(left) || !contains_no_strings(right))
	return ERR_ALPHA_DATA_IS_INVALID;

    *result = new_realmatrix(m, n);
    if (*result == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    p = ((vartype_realmatrix *) *result)->array->data;

    if (BLOCK_SIZE > 0) {
	int4 cachesize = 2 * BLOCK_SIZE * BLOCK_SIZE;
	int4 alignment = CACHE_ALIGNMENT;
	int4 misalgn;
	if (alignment == 0)
	    alignment = 1;
	misalgn = alignment % sizeof(phloat);
	if (misalgn != 0)
	    alignment += sizeof(phloat) - misalgn;

	cache = (phloat *) malloc(cachesize * sizeof(phloat) + alignment);
	if (cache != NULL) {
	    misalgn = ((unsigned long) cache) % alignment;
	    if (misalgn == 0)
		leftcache = cache;
	    else
		leftcache = cache + (alignment - misalgn) / sizeof(phloat);
	    rightcache = leftcache + BLOCK_SIZE * BLOCK_SIZE;
	}
    }

    if (cache == NULL) {

	/* Falling back on basic i,j,k algorithm */
	for (i = 0; i < m; i++)
	    for (j = 0; j < n; j++) {
		phloat sum = 0;
		for (k = 0; k < q; k++)
		    sum += l[i * q + k] * r[k * n + j];
		if ((inf = p_isinf(sum)) != 0) {
		    if (core_settings.matrix_outofrange
					    && !flags.f.range_error_ignore)
			return ERR_OUT_OF_RANGE;
		    else
			sum = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
		}
		p[i * n + j] = sum;
	    }
	return ERR_NONE;

    } else {

	/* Blocked i,j,k algorithm for optimal cache utilization */
	int4 i, j, k, ii, jj, kk;
	for (i = 0; i < m; i += BLOCK_SIZE) {
	    int4 iimax = m - i;
	    if (iimax > BLOCK_SIZE)
		iimax = BLOCK_SIZE;
	    for (j = 0; j < n; j += BLOCK_SIZE) {
		int4 jjmax = n - j;
		if (jjmax > BLOCK_SIZE)
		    jjmax = BLOCK_SIZE;
		for (k = 0; k < q; k += BLOCK_SIZE) {
		    int4 kkmax = q - k;
		    if (kkmax > BLOCK_SIZE)
			kkmax = BLOCK_SIZE;
		    for (ii = 0; ii < iimax; ii++)
			for (kk = 0; kk < kkmax; kk++)
			    leftcache[ii * BLOCK_SIZE + kk]
				= l[(ii + i) * q + (kk + k)];
		    for (kk = 0; kk < kkmax; kk++)
			for (jj = 0; jj < jjmax; jj++)
			    rightcache[kk * BLOCK_SIZE + jj]
				= r[(kk + k) * n + (jj + j)];
		    for (ii = 0; ii < iimax; ii++)
			for (jj = 0; jj < jjmax; jj++) {
			    phloat sum = p[(ii + i) * n + (jj + j)];
			    for (kk = 0; kk < kkmax; kk++)
				sum += leftcache[ii * BLOCK_SIZE + kk]
					* rightcache[kk * BLOCK_SIZE + jj];
			    if ((inf = p_isinf(sum)) != 0) {
				if (core_settings.matrix_outofrange
					    && !flags.f.range_error_ignore) {
				    free(cache);
				    return ERR_OUT_OF_RANGE;
				} else
				    sum = inf < 0 ? NEG_HUGE_PHLOAT
						  : POS_HUGE_PHLOAT;
			    }
			    p[(ii + i) * n + (jj + j)] = sum;
			}
		}
	    }
	}
	free(cache);
	return ERR_NONE;

    }
}
#endif

typedef struct {
    vartype_realmatrix *left;
    vartype_complexmatrix *right;
    vartype *result;
    int4 i, j, k;
    phloat sum_re, sum_im;
    void (*completion)(int error, vartype *result);
} mul_rc_data_struct;

static mul_rc_data_struct *mul_rc_data;

static int matrix_mul_rc(vartype_realmatrix *left, vartype_complexmatrix *right,
			 void (*completion)(int, vartype *)) LINALG_SECT;
static int matrix_mul_rc_worker(int interrupted) LINALG_SECT;

static int matrix_mul_rc(vartype_realmatrix *left, vartype_complexmatrix *right,
			 void (*completion)(int, vartype *)) {

    mul_rc_data_struct *dat;
    int error;

    if (left->columns != right->rows) {
	error = ERR_DIMENSION_ERROR;
	goto finished;
    }

    if (!contains_no_strings(left)) {
	error = ERR_ALPHA_DATA_IS_INVALID;
	goto finished;
    }

    dat = (mul_rc_data_struct *) malloc(sizeof(mul_rc_data_struct));
    if (dat == NULL) {
	error = ERR_INSUFFICIENT_MEMORY;
	goto finished;
    }

    dat->result = new_complexmatrix(left->rows, right->columns);
    if (dat->result == NULL) {
	free(dat);
	error = ERR_INSUFFICIENT_MEMORY;
	goto finished;
    }

    dat->left = left;
    dat->right = right;
    dat->i = 0;
    dat->j = 0;
    dat->k = 0;
    dat->sum_re = 0;
    dat->sum_im = 0;
    dat->completion = completion;

    mul_rc_data = dat;
    mode_interruptible = matrix_mul_rc_worker;
    mode_stoppable = 0;
    return ERR_INTERRUPTIBLE;

    finished:
    completion(error, NULL);
    return error;
}

static int matrix_mul_rc_worker(int interrupted) {
    mul_rc_data_struct *dat = mul_rc_data;
    int count = 0;
    int inf;
    phloat *l = dat->left->array->data;
    phloat *r = dat->right->array->data;
    phloat *p = ((vartype_complexmatrix *) dat->result)->array->data;
    int4 i = dat->i;
    int4 j = dat->j;
    int4 k = dat->k;
    int4 m = dat->left->rows;
    int4 n = dat->right->columns;
    int4 q = dat->left->columns;
    phloat sum_re = dat->sum_re;
    phloat sum_im = dat->sum_im;

    if (interrupted) {
	dat->completion(ERR_INTERRUPTED, NULL);
	free_vartype(dat->result);
	free(dat);
	return ERR_INTERRUPTED;
    }

    while (count++ < 1000) {
	phloat tmp = l[i * q + k];
	sum_re += tmp * r[2 * (k * n + j)];
	sum_im += tmp * r[2 * (k * n + j) + 1];
	if (++k < q)
	    continue;
	k = 0;
	if ((inf = p_isinf(sum_re)) != 0) {
	    if (core_settings.matrix_outofrange && !flags.f.range_error_ignore){
		dat->completion(ERR_OUT_OF_RANGE, NULL);
		free_vartype(dat->result);
		free(dat);
		return ERR_OUT_OF_RANGE;
	    } else
		sum_re = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
	}
	if ((inf = p_isinf(sum_im)) != 0) {
	    if (core_settings.matrix_outofrange && !flags.f.range_error_ignore){
		dat->completion(ERR_OUT_OF_RANGE, NULL);
		free_vartype(dat->result);
		free(dat);
		return ERR_OUT_OF_RANGE;
	    } else
		sum_im = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
	}
	p[2 * (i * n + j)] = sum_re;
	p[2 * (i * n + j) + 1] = sum_im;
	sum_re = 0;
	sum_im = 0;
	if (++j < n)
	    continue;
	j = 0;
	if (++i < m)
	    continue;
	else {
	    dat->completion(ERR_NONE, dat->result);
	    free(dat);
	    return ERR_NONE;
	}
    }

    dat->i = i;
    dat->j = j;
    dat->k = k;
    dat->sum_re = sum_re;
    dat->sum_im = sum_im;
    return ERR_INTERRUPTIBLE;
}

typedef struct {
    vartype_complexmatrix *left;
    vartype_realmatrix *right;
    vartype *result;
    int4 i, j, k;
    phloat sum_re, sum_im;
    void (*completion)(int error, vartype *result);
} mul_cr_data_struct;

static mul_cr_data_struct *mul_cr_data;

static int matrix_mul_cr(vartype_complexmatrix *left, vartype_realmatrix *right,
			 void (*completion)(int, vartype *)) LINALG_SECT;
static int matrix_mul_cr_worker(int interrupted) LINALG_SECT;

static int matrix_mul_cr(vartype_complexmatrix *left, vartype_realmatrix *right,
			 void (*completion)(int, vartype *)) {

    mul_cr_data_struct *dat;
    int error;

    if (left->columns != right->rows) {
	error = ERR_DIMENSION_ERROR;
	goto finished;
    }

    if (!contains_no_strings(right)) {
	error = ERR_ALPHA_DATA_IS_INVALID;
	goto finished;
    }

    dat = (mul_cr_data_struct *) malloc(sizeof(mul_cr_data_struct));
    if (dat == NULL) {
	error = ERR_INSUFFICIENT_MEMORY;
	goto finished;
    }

    dat->result = new_complexmatrix(left->rows, right->columns);
    if (dat->result == NULL) {
	free(dat);
	error = ERR_INSUFFICIENT_MEMORY;
	goto finished;
    }

    dat->left = left;
    dat->right = right;
    dat->i = 0;
    dat->j = 0;
    dat->k = 0;
    dat->sum_re = 0;
    dat->sum_im = 0;
    dat->completion = completion;

    mul_cr_data = dat;
    mode_interruptible = matrix_mul_cr_worker;
    mode_stoppable = 0;
    return ERR_INTERRUPTIBLE;

    finished:
    completion(error, NULL);
    return error;
}

static int matrix_mul_cr_worker(int interrupted) {
    mul_cr_data_struct *dat = mul_cr_data;
    int count = 0;
    int inf;
    phloat *l = dat->left->array->data;
    phloat *r = dat->right->array->data;
    phloat *p = ((vartype_complexmatrix *) dat->result)->array->data;
    int4 i = dat->i;
    int4 j = dat->j;
    int4 k = dat->k;
    int4 m = dat->left->rows;
    int4 n = dat->right->columns;
    int4 q = dat->left->columns;
    phloat sum_re = dat->sum_re;
    phloat sum_im = dat->sum_im;

    if (interrupted) {
	dat->completion(ERR_INTERRUPTED, NULL);
	free_vartype(dat->result);
	free(dat);
	return ERR_INTERRUPTED;
    }

    while (count++ < 1000) {
	phloat tmp = r[k * n + j];
	sum_re += tmp * l[2 * (i * q + k)];
	sum_im += tmp * l[2 * (i * q + k) + 1];
	if (++k < q)
	    continue;
	k = 0;
	if ((inf = p_isinf(sum_re)) != 0) {
	    if (core_settings.matrix_outofrange && !flags.f.range_error_ignore){
		dat->completion(ERR_OUT_OF_RANGE, NULL);
		free_vartype(dat->result);
		free(dat);
		return ERR_OUT_OF_RANGE;
	    } else
		sum_re = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
	}
	if ((inf = p_isinf(sum_im)) != 0) {
	    if (core_settings.matrix_outofrange && !flags.f.range_error_ignore){
		dat->completion(ERR_OUT_OF_RANGE, NULL);
		free_vartype(dat->result);
		free(dat);
		return ERR_OUT_OF_RANGE;
	    } else
		sum_im = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
	}
	p[2 * (i * n + j)] = sum_re;
	p[2 * (i * n + j) + 1] = sum_im;
	sum_re = 0;
	sum_im = 0;
	if (++j < n)
	    continue;
	j = 0;
	if (++i < m)
	    continue;
	else {
	    dat->completion(ERR_NONE, dat->result);
	    free(dat);
	    return ERR_NONE;
	}
    }

    dat->i = i;
    dat->j = j;
    dat->k = k;
    dat->sum_re = sum_re;
    dat->sum_im = sum_im;
    return ERR_INTERRUPTIBLE;
}

typedef struct {
    vartype_complexmatrix *left;
    vartype_complexmatrix *right;
    vartype *result;
    int4 i, j, k;
    phloat sum_re, sum_im;
    void (*completion)(int error, vartype *result);
} mul_cc_data_struct;

static mul_cc_data_struct *mul_cc_data;

static int matrix_mul_cc(vartype_complexmatrix *left, vartype_complexmatrix *right,
			 void (*completion)(int, vartype *)) LINALG_SECT;
static int matrix_mul_cc_worker(int interrupted) LINALG_SECT;

static int matrix_mul_cc(vartype_complexmatrix *left, vartype_complexmatrix *right,
			 void (*completion)(int, vartype *)) {

    mul_cc_data_struct *dat;
    int error;

    if (left->columns != right->rows) {
	error = ERR_DIMENSION_ERROR;
	goto finished;
    }

    dat = (mul_cc_data_struct *) malloc(sizeof(mul_cc_data_struct));
    if (dat == NULL) {
	error = ERR_INSUFFICIENT_MEMORY;
	goto finished;
    }

    dat->result = new_complexmatrix(left->rows, right->columns);
    if (dat->result == NULL) {
	free(dat);
	error = ERR_INSUFFICIENT_MEMORY;
	goto finished;
    }

    dat->left = left;
    dat->right = right;
    dat->i = 0;
    dat->j = 0;
    dat->k = 0;
    dat->sum_re = 0;
    dat->sum_im = 0;
    dat->completion = completion;

    mul_cc_data = dat;
    mode_interruptible = matrix_mul_cc_worker;
    mode_stoppable = 0;
    return ERR_INTERRUPTIBLE;

    finished:
    completion(error, NULL);
    return error;
}

static int matrix_mul_cc_worker(int interrupted) {
    mul_cc_data_struct *dat = mul_cc_data;
    int count = 0;
    int inf;
    phloat *l = dat->left->array->data;
    phloat *r = dat->right->array->data;
    phloat *p = ((vartype_complexmatrix *) dat->result)->array->data;
    int4 i = dat->i;
    int4 j = dat->j;
    int4 k = dat->k;
    int4 m = dat->left->rows;
    int4 n = dat->right->columns;
    int4 q = dat->left->columns;
    phloat sum_re = dat->sum_re;
    phloat sum_im = dat->sum_im;

    if (interrupted) {
	dat->completion(ERR_INTERRUPTED, NULL);
	free_vartype(dat->result);
	free(dat);
	return ERR_INTERRUPTED;
    }

    while (count++ < 1000) {
	phloat l_re = l[2 * (i * q + k)];
	phloat l_im = l[2 * (i * q + k) + 1];
	phloat r_re = r[2 * (k * n + j)];
	phloat r_im = r[2 * (k * n + j) + 1];
	sum_re += l_re * r_re - l_im * r_im;
	sum_im += l_im * r_re + l_re * r_im;
	if (++k < q)
	    continue;
	k = 0;
	if ((inf = p_isinf(sum_re)) != 0) {
	    if (core_settings.matrix_outofrange && !flags.f.range_error_ignore){
		dat->completion(ERR_OUT_OF_RANGE, NULL);
		free_vartype(dat->result);
		free(dat);
		return ERR_OUT_OF_RANGE;
	    } else
		sum_re = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
	}
	if ((inf = p_isinf(sum_im)) != 0) {
	    if (core_settings.matrix_outofrange && !flags.f.range_error_ignore){
		dat->completion(ERR_OUT_OF_RANGE, NULL);
		free_vartype(dat->result);
		free(dat);
		return ERR_OUT_OF_RANGE;
	    } else
		sum_im = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
	}
	p[2 * (i * n + j)] = sum_re;
	p[2 * (i * n + j) + 1] = sum_im;
	sum_re = 0;
	sum_im = 0;
	if (++j < n)
	    continue;
	j = 0;
	if (++i < m)
	    continue;
	else {
	    dat->completion(ERR_NONE, dat->result);
	    free(dat);
	    return ERR_NONE;
	}
    }

    dat->i = i;
    dat->j = j;
    dat->k = k;
    dat->sum_re = sum_re;
    dat->sum_im = sum_im;
    return ERR_INTERRUPTIBLE;
}

int linalg_mul(const vartype *left, const vartype *right,
				    void (*completion)(int, vartype *)) {
    if (left->type == TYPE_REALMATRIX) {
	if (right->type == TYPE_REALMATRIX)
	    return matrix_mul_rr((vartype_realmatrix *) left,
				 (vartype_realmatrix *) right,
				 completion);
	else
	    return matrix_mul_rc((vartype_realmatrix *) left,
				 (vartype_complexmatrix *) right,
				 completion);
    } else {
	if (right->type == TYPE_REALMATRIX)
	    return matrix_mul_cr((vartype_complexmatrix *) left,
				 (vartype_realmatrix *) right,
				 completion);
	else
	    return matrix_mul_cc((vartype_complexmatrix *) left,
				 (vartype_complexmatrix *) right,
				 completion);
    }
}


/**************************/
/***** Matrix inverse *****/
/**************************/

static void (*linalg_inv_completion)(int error, vartype *det);
static vartype *linalg_inv_result;

static int inv_r_completion1(int error, vartype_realmatrix *a, int4 *perm,
				phloat det) LINALG_SECT;
static void inv_r_completion2(int error, vartype_realmatrix *a, int4 *perm,
				vartype_realmatrix *b) LINALG_SECT;
static int inv_c_completion1(int error, vartype_complexmatrix *a, int4 *perm,
				phloat det_re, phloat det_im) LINALG_SECT;
static void inv_c_completion2(int error, vartype_complexmatrix *a, int4 *perm,
				vartype_complexmatrix *b) LINALG_SECT;

int linalg_inv(const vartype *src, void (*completion)(int, vartype *)) {
    int4 n;
    int4 *perm;
    if (src->type == TYPE_REALMATRIX) {
	vartype_realmatrix *ma = (vartype_realmatrix *) src;
	vartype *lu, *inv;
	n = ma->rows;
	if (n != ma->columns)
	    return ERR_DIMENSION_ERROR;
	if (!contains_no_strings(ma))
	    return ERR_ALPHA_DATA_IS_INVALID;
	lu = new_realmatrix(n, n);
	if (lu == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	inv = new_realmatrix(n, n);
	if (inv == NULL) {
	    free_vartype(lu);
	    return ERR_INSUFFICIENT_MEMORY;
	}
	perm = (int4 *) malloc(n * sizeof(int4));
	if (perm == NULL) {
	    free_vartype(lu);
	    free_vartype(inv);
	    return ERR_INSUFFICIENT_MEMORY;
	}
	matrix_copy(lu, src);
	linalg_inv_completion = completion;
	linalg_inv_result = inv;
	return lu_decomp_r((vartype_realmatrix *) lu, perm, inv_r_completion1);
    } else {
	vartype_complexmatrix *ma = (vartype_complexmatrix *) src;
	vartype *lu, *inv;
	n = ma->rows;
	if (n != ma->columns)
	    return ERR_DIMENSION_ERROR;
	lu = new_complexmatrix(n, n);
	if (lu == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	inv = new_complexmatrix(n, n);
	if (inv == NULL) {
	    free_vartype(lu);
	    return ERR_INSUFFICIENT_MEMORY;
	}
	perm = (int4 *) malloc(n * sizeof(int4));
	if (perm == NULL) {
	    free_vartype(lu);
	    free_vartype(inv);
	    return ERR_INSUFFICIENT_MEMORY;
	}
	matrix_copy(lu, src);
	linalg_inv_completion = completion;
	linalg_inv_result = inv;
	return lu_decomp_c((vartype_complexmatrix *) lu, perm,
						    inv_c_completion1);
    }
}

static int inv_r_completion1(int error, vartype_realmatrix *a, int4 *perm,
				phloat det) {
    if (error != ERR_NONE) {
	free_vartype(linalg_inv_result);
	free_vartype((vartype *) a);
	free(perm);
	linalg_inv_completion(error, NULL);
	return error;
    } else {
	int4 i, n = a->rows;
	vartype_realmatrix *inv = (vartype_realmatrix *) linalg_inv_result;
	for (i = 0; i < n; i++)
	    inv->array->data[i * (n + 1)] = 1;
	return lu_backsubst_rr(a, perm, inv, inv_r_completion2);
    }
}

static void inv_r_completion2(int error, vartype_realmatrix *a, int4 *perm,
				vartype_realmatrix *b) {
    if (error != ERR_NONE)
	free_vartype(linalg_inv_result); /* Note: linalg_inv_result == b */
    free_vartype((vartype *) a);
    free(perm);
    linalg_inv_completion(error, linalg_inv_result);
}

static int inv_c_completion1(int error, vartype_complexmatrix *a, int4 *perm,
				phloat det_re, phloat det_im) {
    if (error != ERR_NONE) {
	free_vartype(linalg_inv_result);
	free_vartype((vartype *) a);
	free(perm);
	linalg_inv_completion(error, NULL);
	return error;
    } else {
	int4 i, n = a->rows;
	vartype_complexmatrix *inv =
			    (vartype_complexmatrix *) linalg_inv_result;
	for (i = 0; i < n; i++)
	    inv->array->data[2 * (i * (n + 1))] = 1;
	return lu_backsubst_cc(a, perm, inv, inv_c_completion2);
    }
}

static void inv_c_completion2(int error, vartype_complexmatrix *a, int4 *perm,
				vartype_complexmatrix *b) {
    if (error != ERR_NONE)
	free_vartype(linalg_inv_result); /* Note: linalg_inv_result == b */
    free_vartype((vartype *) a);
    free(perm);
    linalg_inv_completion(error, linalg_inv_result);
}


/******************************/
/***** Matrix determinant *****/
/******************************/

static void (*linalg_det_completion)(int error, vartype *det);
static int linalg_det_prev_sm_err;

static int det_r_completion(int error, vartype_realmatrix *a, int4 *perm,
				    phloat det) LINALG_SECT;
static int det_c_completion(int error, vartype_complexmatrix *a, int4 *perm,
				    phloat det_re, phloat det_im) LINALG_SECT;

int linalg_det(const vartype *src, void (*completion)(int, vartype *)) {
    int4 n;
    int4 *perm;
    if (src->type == TYPE_REALMATRIX) {
	vartype_realmatrix *ma = (vartype_realmatrix *) src;
	n = ma->rows;
	if (n != ma->columns) {
	    completion(ERR_DIMENSION_ERROR, 0);
	    return ERR_DIMENSION_ERROR;
	}
	if (!contains_no_strings(ma)) {
	    completion(ERR_ALPHA_DATA_IS_INVALID, 0);
	    return ERR_ALPHA_DATA_IS_INVALID;
	}
	ma = (vartype_realmatrix *) dup_vartype(src);
	if (ma == NULL) {
	    completion(ERR_INSUFFICIENT_MEMORY, 0);
	    return ERR_INSUFFICIENT_MEMORY;
	}
	if (!disentangle((vartype *) ma)) {
	    free_vartype((vartype *) ma);
	    completion(ERR_INSUFFICIENT_MEMORY, 0);
	    return ERR_INSUFFICIENT_MEMORY;
	}
	perm = (int4 *) malloc(n * sizeof(int4));
	if (perm == NULL) {
	    free_vartype((vartype *) ma);
	    completion(ERR_INSUFFICIENT_MEMORY, 0);
	    return ERR_INSUFFICIENT_MEMORY;
	}

	/* Before calling lu_decomp_r, make sure the 'singular matrix'
	 * error reporting mode is on; we don't want the HP-42S compatible
	 * zero-pivot-fudging to take place when all we're doing is computing
	 * the determinant.
	 * The completion routine will restore the 'singular matrix' error
	 * mode to its original value.
	 */
	linalg_det_prev_sm_err = core_settings.matrix_singularmatrix;
	core_settings.matrix_singularmatrix = 1;

	linalg_det_completion = completion;
	return lu_decomp_r(ma, perm, det_r_completion); 
    } else /* src->type == TYPE_COMPLEXMATRIX */ {
	vartype_complexmatrix *ma = (vartype_complexmatrix *) src;
	n = ma->rows;
	if (n != ma->columns)
	    return ERR_DIMENSION_ERROR;
	ma = (vartype_complexmatrix *) dup_vartype(src);
	if (ma == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	if (!disentangle((vartype *) ma)) {
	    free_vartype((vartype *) ma);
	    return ERR_INSUFFICIENT_MEMORY;
	}
	n = ma->rows;
	perm = (int4 *) malloc(n * sizeof(int4));
	if (perm == NULL) {
	    free_vartype((vartype *) ma);
	    return ERR_INSUFFICIENT_MEMORY;
	}

	/* Before calling lu_decomp_c, make sure the 'singular matrix'
	 * error reporting mode is on; we don't want the HP-42S compatible
	 * zero-pivot-fudging to take place when all we're doing is computing
	 * the determinant.
	 * The completion routine will restore the 'singular matrix' error
	 * mode to its original value.
	 */
	linalg_det_prev_sm_err = core_settings.matrix_singularmatrix;
	core_settings.matrix_singularmatrix = 1;

	linalg_det_completion = completion;
	return lu_decomp_c(ma, perm, det_c_completion); 
    }
}

static int det_r_completion(int error, vartype_realmatrix *a, int4 *perm,
					 phloat det) {
    vartype *det_v;

    core_settings.matrix_singularmatrix = linalg_det_prev_sm_err;

    free_vartype((vartype *) a);
    free(perm);
    if (error == ERR_SINGULAR_MATRIX) {
	det = 0;
	error = ERR_NONE;
    }
    if (error == ERR_NONE) {
	int inf = p_isinf(det);
	if (inf != 0) {
	    if (flags.f.range_error_ignore)
		det = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
	    else
		error = ERR_OUT_OF_RANGE;
	}
    }
    if (error == ERR_NONE) {
	det_v = new_real(det);
	if (det_v == NULL)
	    error = ERR_INSUFFICIENT_MEMORY;
    }

    linalg_det_completion(error, det_v);
    return error;
}

static int det_c_completion(int error, vartype_complexmatrix *a, int4 *perm,
				    phloat det_re, phloat det_im) {
    vartype *det_v;

    core_settings.matrix_singularmatrix = linalg_det_prev_sm_err;

    free_vartype((vartype *) a);
    free(perm);
    if (error == ERR_SINGULAR_MATRIX) {
	det_re = 0;
	det_im = 0;
	error = ERR_NONE;
    }
    if (error == ERR_NONE) {
	int inf;
	if ((inf = p_isinf(det_re)) != 0) {
	    if (flags.f.range_error_ignore)
		det_re = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
	    else
		error = ERR_OUT_OF_RANGE;
	}
	if ((inf = p_isinf(det_im)) != 0) {
	    if (flags.f.range_error_ignore)
		det_im = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
	    else
		error = ERR_OUT_OF_RANGE;
	}
    }
    if (error == ERR_NONE) {
	det_v = new_complex(det_re, det_im);
	if (det_v == NULL)
	    error = ERR_INSUFFICIENT_MEMORY;
    }

    linalg_det_completion(error, det_v);
    return error;
}
