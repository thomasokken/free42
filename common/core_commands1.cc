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

#include "core_commands1.h"
#include "core_commands2.h"
#include "core_decimal.h"
#include "core_display.h"
#include "core_globals.h"
#include "core_helpers.h"
#include "core_main.h"
#include "core_math.h"
#include "core_variables.h"
#include "shell.h"


/********************************************************/
/* Implementations of HP-42S built-in functions, part 1 */
/********************************************************/

int docmd_clx(arg_struct *arg) {
    free_vartype(reg_x);
    reg_x = new_real(0);
    mode_disable_stack_lift = 1;
    return ERR_NONE;
}

int docmd_enter(arg_struct *arg) {
    vartype *v = dup_vartype(reg_x);
    if (v == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    free_vartype(reg_t);
    reg_t = reg_z;
    reg_z = reg_y;
    reg_y = v;
    mode_disable_stack_lift = 1;
    return ERR_NONE;
}

int docmd_swap(arg_struct *arg) {
    vartype *temp = reg_x;
    reg_x = reg_y;
    reg_y = temp;
    if (flags.f.trace_print && flags.f.printer_exists)
	docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_rdn(arg_struct *arg) {
    vartype *temp = reg_x;
    reg_x = reg_y;
    reg_y = reg_z;
    reg_z = reg_t;
    reg_t = temp;
    if (flags.f.trace_print && flags.f.printer_exists)
	docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_chs(arg_struct *arg) {
    switch (reg_x->type) {
	case TYPE_REAL: {
	    vartype_real *r = (vartype_real *) reg_x;
	    r->x = -(r->x);
	    break;
	}
	case TYPE_COMPLEX: {
	    vartype_complex *c = (vartype_complex *) reg_x;
	    c->re = -(c->re);
	    c->im = -(c->im);
	    break;
	}
	case TYPE_REALMATRIX: {
	    vartype_realmatrix *rm = (vartype_realmatrix *) reg_x;
	    int4 sz = rm->rows * rm->columns;
	    int4 i;
	    for (i = 0; i < sz; i++)
		if (rm->array->is_string[i])
		    return ERR_ALPHA_DATA_IS_INVALID;
	    if (!disentangle((vartype *) rm))
		return ERR_INSUFFICIENT_MEMORY;
	    for (i = 0; i < sz; i++)
		rm->array->data[i].d = -(rm->array->data[i].d);
	    break;
	}
	case TYPE_COMPLEXMATRIX: {
	    vartype_complexmatrix *cm = (vartype_complexmatrix *) reg_x;
	    int4 sz = cm->rows * cm->columns * 2;
	    int4 i;
	    if (!disentangle((vartype *) cm))
		return ERR_INSUFFICIENT_MEMORY;
	    for (i = 0; i < sz; i++)
		cm->array->data[i] = -(cm->array->data[i]);
	    break;
	}
	case TYPE_STRING:
	    return ERR_ALPHA_DATA_IS_INVALID;
    }
    if (flags.f.trace_print && flags.f.printer_exists)
	docmd_prx(NULL);
    return ERR_NONE;
}

static void docmd_div_completion(int error, vartype *res) COMMANDS1_SECT;
static void docmd_div_completion(int error, vartype *res) {
    if (error == ERR_NONE)
	binary_result(res);
}

int docmd_div(arg_struct *arg) {
    return generic_div(reg_x, reg_y, docmd_div_completion);
}

static void docmd_mul_completion(int error, vartype *res) COMMANDS1_SECT;
static void docmd_mul_completion(int error, vartype *res) {
    if (error == ERR_NONE)
	binary_result(res);
}

int docmd_mul(arg_struct *arg) {
    return generic_mul(reg_x, reg_y, docmd_mul_completion);
}

int docmd_sub(arg_struct *arg) {
    vartype *res;
    int error = generic_sub(reg_x, reg_y, &res);
    if (error == ERR_NONE)
	binary_result(res);
    return error;
}

int docmd_add(arg_struct *arg) {
    vartype *res;
    int error = generic_add(reg_x, reg_y, &res);
    if (error == ERR_NONE)
	binary_result(res);
    return error;
}

int docmd_lastx(arg_struct *arg) {
    vartype *v = dup_vartype(reg_lastx);
    if (v == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    recall_result(v);
    return ERR_NONE;
}

static int mappable_sin_r(double x, double *y) COMMANDS1_SECT;
static int mappable_sin_r(double x, double *y) {
    if (flags.f.rad) {
	*y = sin(x);
    } else if (flags.f.grad) {
	x = fmod(x, 400);
	if (x < 0)
	    x += 400;
	if (x == 0 || x == 200)
	    *y = 0;
	else if (x == 100)
	    *y = 1;
	else if (x == 300)
	    *y = -1;
	else
	    *y = sin(x / (200 / PI));
    } else {
	x = fmod(x, 360);
	if (x < 0)
	    x += 360;
	if (x == 0 || x == 180)
	    *y = 0;
	else if (x == 90)
	    *y = 1;
	else if (x == 270)
	    *y = -1;
	else
	    *y = sin(x / (180 / PI));
    }
    return ERR_NONE;
}

static int mappable_sin_c(double xre, double xim,
			     double *yre, double *yim) COMMANDS1_SECT;
static int mappable_sin_c(double xre, double xim, double *yre, double *yim) {
    /* NOTE: DEG/RAD/GRAD mode does not apply here. */
    double sinxre, cosxre;
    double sinhxim, coshxim;
    int inf;
    sincos(xre, &sinxre, &cosxre);
    sinhxim = sinh(xim);
    coshxim = cosh(xim);
    *yre = sinxre * coshxim;
    if ((inf = isinf(*yre)) != 0) {
	if (flags.f.range_error_ignore)
	    *yre = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *yim = cosxre * sinhxim;
    if ((inf = isinf(*yim)) != 0) {
	if (flags.f.range_error_ignore)
	    *yim = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    return ERR_NONE;
}

int docmd_sin(arg_struct *arg) {
    if (reg_x->type != TYPE_STRING) {
	vartype *v;
	int err = map_unary(reg_x, &v, mappable_sin_r, mappable_sin_c);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    } else
	return ERR_ALPHA_DATA_IS_INVALID;
}

static int mappable_cos_r(double x, double *y) COMMANDS1_SECT;
static int mappable_cos_r(double x, double *y) {
    if (flags.f.rad) {
	*y = cos(x);
    } else if (flags.f.grad) {
	x = fmod(x, 400);
	if (x < 0)
	    x += 400;
	if (x == 0)
	    *y = 1;
	else if (x == 100 || x == 300)
	    *y = 0;
	else if (x == 200)
	    *y = -1;
	else
	    *y = cos(x / (200 / PI));
    } else {
	x = fmod(x, 360);
	if (x < 0)
	    x += 360;
	if (x == 0)
	    *y = 1;
	else if (x == 90 || x == 270)
	    *y = 0;
	else if (x == 180)
	    *y = -1;
	else
	    *y = cos(x / (180 / PI));
    }
    return ERR_NONE;
}

static int mappable_cos_c(double xre, double xim,
			     double *yre, double *yim) COMMANDS1_SECT;
static int mappable_cos_c(double xre, double xim, double *yre, double *yim) {
    /* NOTE: DEG/RAD/GRAD mode does not apply here. */
    double sinxre, cosxre;
    double sinhxim, coshxim;
    int inf;
    sincos(xre, &sinxre, &cosxre);
    sinhxim = sinh(xim);
    coshxim = cosh(xim);
    *yre = cosxre * coshxim;
    if ((inf = isinf(*yre)) != 0) {
	if (flags.f.range_error_ignore)
	    *yre = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *yim = -sinxre * sinhxim;
    if ((inf = isinf(*yim)) != 0) {
	if (flags.f.range_error_ignore)
	    *yim = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    return ERR_NONE;
}

int docmd_cos(arg_struct *arg) {
    if (reg_x->type != TYPE_STRING) {
	vartype *v;
	int err = map_unary(reg_x, &v, mappable_cos_r, mappable_cos_c);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    } else
	return ERR_ALPHA_DATA_IS_INVALID;
}

static int mappable_tan_r(double x, double *y) COMMANDS1_SECT;
static int mappable_tan_r(double x, double *y) {
    int inf = 1;
    if (flags.f.rad) {
	*y = tan(x);
    } else if (flags.f.grad) {
	x = fmod(x, 200);
	if (x < 0)
	    x += 200;
	if (x == 0)
	    *y = 0;
	else if (x == 50)
	    *y = 1;
	else if (x == 100)
	    goto infinite;
	else if (x == 150)
	    *y = -1;
	else
	    *y = tan(x / (200 / PI));
    } else {
	x = fmod(x, 180);
	if (x < 0)
	    x += 180;
	if (x == 0)
	    *y = 0;
	else if (x == 45)
	    *y = 1;
	else if (x == 90)
	    goto infinite;
	else if (x == 135)
	    *y = -1;
	else
	    *y = tan(x / (180 / PI));
    }
    if (isnan(*y))
	goto infinite;
    if ((inf = isinf(*y)) != 0) {
	infinite:
	if (flags.f.range_error_ignore)
	    *y = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    return ERR_NONE;
}

static int mappable_tan_c(double xre, double xim,
			     double *yre, double *yim) COMMANDS1_SECT;
static int mappable_tan_c(double xre, double xim, double *yre, double *yim) {
    /* NOTE: DEG/RAD/GRAD mode does not apply here. */
    double sinxre, cosxre;
    double sinhxim, coshxim;
    double re_sin, im_sin, re_cos, im_cos, abs_cos;
    int inf;

    sincos(xre, &sinxre, &cosxre);
    sinhxim = sinh(xim);
    coshxim = cosh(xim);

    re_sin = sinxre * coshxim;
    im_sin = cosxre * sinhxim;
    re_cos = cosxre * coshxim;
    im_cos = -sinxre * sinhxim;
    abs_cos = hypot(re_cos, im_cos);

    if (abs_cos == 0) {
	if (flags.f.range_error_ignore) {
	    *yre = re_sin * re_cos + im_sin * im_cos > 0 ? POS_HUGE_DOUBLE
							 : NEG_HUGE_DOUBLE;
	    *yim = im_sin * re_cos - re_sin * im_cos > 0 ? POS_HUGE_DOUBLE
							 : NEG_HUGE_DOUBLE;
	    return ERR_NONE;
	} else
	    return ERR_OUT_OF_RANGE;
    }

    *yre = (re_sin * re_cos + im_sin * im_cos) / abs_cos / abs_cos;
    if ((inf = isinf(*yre)) != 0) {
	if (flags.f.range_error_ignore)
	    *yre = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *yim = (im_sin * re_cos - re_sin * im_cos) / abs_cos / abs_cos;
    if ((inf = isinf(*yim)) != 0) {
	if (flags.f.range_error_ignore)
	    *yim = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }

    return ERR_NONE;
}

int docmd_tan(arg_struct *arg) {
    if (reg_x->type != TYPE_STRING) {
	vartype *v;
	int err = map_unary(reg_x, &v, mappable_tan_r, mappable_tan_c);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    } else
	return ERR_ALPHA_DATA_IS_INVALID;
}

static int mappable_asin_r(double x, double *y) COMMANDS1_SECT;
static int mappable_asin_r(double x, double *y) {
    if (x < -1 || x > 1)
	return ERR_INVALID_DATA;
    *y = rad_to_angle(asin(x));
    return ERR_NONE;
}

static int mappable_asin_c(double xre, double xim, double *yre, double *yim)
								COMMANDS1_SECT;
static int mappable_asin_c(double xre, double xim, double *yre, double *yim) {
    double tre, tim;
    int err = math_asinh_acosh(-xim, xre, &tre, &tim, 1);
    *yre = tim;
    *yim = -tre;
    return err;
}

int docmd_asin(arg_struct *arg) {
    vartype *v;
    if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_x->type == TYPE_REAL) {
	double x = ((vartype_real *) reg_x)->x;
	if (x < -1 || x > 1) {
	    if (flags.f.real_result_only)
		return ERR_INVALID_DATA;
	    else {
		/* TODO: review */
		if (x > 0)
		    v = new_complex(PI / 2, -acosh(x));
		else
		    v = new_complex(-PI / 2, acosh(-x));
	    }
	} else
	    v = new_real(rad_to_angle(asin(x)));
    } else {
	int err = map_unary(reg_x, &v, mappable_asin_r, mappable_asin_c);
	if (err != ERR_NONE)
	    return err;
    }
    unary_result(v);
    return ERR_NONE;
}

static int mappable_acos_r(double x, double *y) COMMANDS1_SECT;
static int mappable_acos_r(double x, double *y) {
    if (x < -1 || x > 1)
	return ERR_INVALID_DATA;
    *y = rad_to_angle(acos(x));
    return ERR_NONE;
}

static int mappable_acos_c(double xre, double xim, double *yre, double *yim)
								COMMANDS1_SECT;
static int mappable_acos_c(double xre, double xim, double *yre, double *yim) {
    double tre, tim;
    int err = math_asinh_acosh(xre, xim, &tre, &tim, 0);
    *yre = tim;
    *yim = -tre;
    return err;
}

int docmd_acos(arg_struct *arg) {
    vartype *v;
    if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_x->type == TYPE_REAL) {
	double x = ((vartype_real *) reg_x)->x;
	if (x < -1 || x > 1) {
	    if (flags.f.real_result_only)
		return ERR_INVALID_DATA;
	    else {
		/* TODO: review */
		if (x > 0)
		    v = new_complex(0, acosh(x));
		else
		    v = new_complex(PI, -acosh(-x));
	    }
	} else
	    v = new_real(rad_to_angle(acos(x)));
    } else {
	int err = map_unary(reg_x, &v, mappable_acos_r, mappable_acos_c);
	if (err != ERR_NONE)
	    return err;
    }
    unary_result(v);
    return ERR_NONE;
}

static int mappable_atan_r(double x, double *y) COMMANDS1_SECT;
static int mappable_atan_r(double x, double *y) {
    *y = rad_to_angle(atan(x));
    return ERR_NONE;
}

static int mappable_atan_c(double xre, double xim, double *yre, double *yim)
								COMMANDS1_SECT;
static int mappable_atan_c(double xre, double xim, double *yre, double *yim) {
    double tre, tim;
    int err = math_atanh(xim, -xre, &tre, &tim);
    *yre = -tim;
    *yim = tre;
    return err;
}

int docmd_atan(arg_struct *arg) {
    if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else {
	vartype *v;
	int err = map_unary(reg_x, &v, mappable_atan_r, mappable_atan_c);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    }
}

static int mappable_log_r(double x, double *y) COMMANDS1_SECT;
static int mappable_log_r(double x, double *y) {
    if (x <= 0)
	return ERR_INVALID_DATA;
    else {
	*y = log10(x);
	return ERR_NONE;
    }
}

static int mappable_log_c(double xre, double xim,
			     double *yre, double *yim) COMMANDS1_SECT;
static int mappable_log_c(double xre, double xim, double *yre, double *yim) {
    double h = hypot(xre, xim);
    if (h == 0)
	return ERR_INVALID_DATA;
    else {
	*yre = log10(h);
	*yim = atan2(xim, xre) / log(10);
	return ERR_NONE;
    }
}

int docmd_log(arg_struct *arg) {
    if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else if (reg_x->type == TYPE_REAL) {
	vartype_real *x = (vartype_real *) reg_x;
	if (x->x == 0)
	    return ERR_INVALID_DATA;
	else if (x->x < 0) {
	    if (flags.f.real_result_only)
		return ERR_INVALID_DATA;
	    else {
		vartype *r = new_complex(log10(-x->x), PI / log(10));
		if (r == NULL)
		    return ERR_INSUFFICIENT_MEMORY;
		else {
		    unary_result(r);
		    return ERR_NONE;
		}
	    }
	} else {
	    vartype *r = new_real(log10(x->x));
	    if (r == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	    else {
		unary_result(r);
		return ERR_NONE;
	    }
	}
    } else {
	vartype *v;
	int err = map_unary(reg_x, &v, mappable_log_r, mappable_log_c);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    }
}

static int mappable_10_pow_x_r(double x, double *y) COMMANDS1_SECT;
static int mappable_10_pow_x_r(double x, double *y) {
    *y = pow(10, x);
    if (isinf(*y) != 0) {
	if (!flags.f.range_error_ignore)
	    return ERR_OUT_OF_RANGE;
	*y = POS_HUGE_DOUBLE;
    }
    return ERR_NONE;
}

static int mappable_10_pow_x_c(double xre, double xim,
			     double *yre, double *yim) COMMANDS1_SECT;
static int mappable_10_pow_x_c(double xre, double xim, double *yre, double *yim){
    int inf;
    double h;
    xim *= log(10);
    if ((inf = isinf(xim)) != 0)
	xim = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
    h = pow(10, xre);
    if ((inf = isinf(h)) == 0) {
	*yre = cos(xim) * h;
	*yim = sin(xim) * h;
	return ERR_NONE;
    } else if (flags.f.range_error_ignore) {
	double t = cos(xim);
	if (t == 0)
	    *yre = 0;
	else if (t < 0)
	    *yre = inf < 0 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    *yre = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
	t = sin(xim);
	if (t == 0)
	    *yim = 0;
	else if (t < 0)
	    *yim = inf < 0 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    *yim = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
	return ERR_NONE;
    } else
	return ERR_OUT_OF_RANGE;
}

int docmd_10_pow_x(arg_struct *arg) {
    if (reg_x->type != TYPE_STRING) {
	vartype *v;
	int err = map_unary(reg_x, &v, mappable_10_pow_x_r,
				       mappable_10_pow_x_c);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    } else
	return ERR_ALPHA_DATA_IS_INVALID;
}

static int mappable_ln_r(double x, double *y) COMMANDS1_SECT;
static int mappable_ln_r(double x, double *y) {
    if (x <= 0)
	return ERR_INVALID_DATA;
    else {
	*y = log(x);
	return ERR_NONE;
    }
}

static int mappable_ln_c(double xre, double xim,
			     double *yre, double *yim) COMMANDS1_SECT;
static int mappable_ln_c(double xre, double xim, double *yre, double *yim) {
    double h = hypot(xre, xim);
    if (h == 0)
	return ERR_INVALID_DATA;
    else {
	*yre = log(h);
	*yim = atan2(xim, xre);
	return ERR_NONE;
    }
}

int docmd_ln(arg_struct *arg) {
    if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else if (reg_x->type == TYPE_REAL) {
	vartype_real *x = (vartype_real *) reg_x;
	if (x->x == 0)
	    return ERR_INVALID_DATA;
	else if (x->x < 0) {
	    if (flags.f.real_result_only)
		return ERR_INVALID_DATA;
	    else {
		vartype *r = new_complex(log(-x->x), PI);
		if (r == NULL)
		    return ERR_INSUFFICIENT_MEMORY;
		else {
		    unary_result(r);
		    return ERR_NONE;
		}
	    }
	} else {
	    vartype *r = new_real(log(x->x));
	    if (r == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	    else {
		unary_result(r);
		return ERR_NONE;
	    }
	}
    } else {
	vartype *v;
	int err = map_unary(reg_x, &v, mappable_ln_r, mappable_ln_c);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    }
}

static int mappable_e_pow_x_r(double x, double *y) COMMANDS1_SECT;
static int mappable_e_pow_x_r(double x, double *y) {
    *y = exp(x);
    if (isinf(*y) != 0) {
	if (!flags.f.range_error_ignore)
	    return ERR_OUT_OF_RANGE;
	*y = POS_HUGE_DOUBLE;
    }
    return ERR_NONE;
}

static int mappable_e_pow_x_c(double xre, double xim,
			     double *yre, double *yim) COMMANDS1_SECT;
static int mappable_e_pow_x_c(double xre, double xim, double *yre, double *yim){
    double h = exp(xre);
    int inf = isinf(h);
    if (inf == 0) {
	*yre = cos(xim) * h;
	*yim = sin(xim) * h;
	return ERR_NONE;
    } else if (flags.f.range_error_ignore) {
	double t = cos(xim);
	if (t == 0)
	    *yre = 0;
	else if (t < 0)
	    *yre = inf < 0 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    *yre = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
	t = sin(xim);
	if (t == 0)
	    *yim = 0;
	else if (t < 0)
	    *yim = inf < 0 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    *yim = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
	return ERR_NONE;
    } else
	return ERR_OUT_OF_RANGE;
}

int docmd_e_pow_x(arg_struct *arg) {
    if (reg_x->type != TYPE_STRING) {
	vartype *v;
	int err = map_unary(reg_x, &v, mappable_e_pow_x_r, mappable_e_pow_x_c);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    } else
	return ERR_ALPHA_DATA_IS_INVALID;
}

static int mappable_sqrt_r(double x, double *y) COMMANDS1_SECT;
static int mappable_sqrt_r(double x, double *y) {
    if (x < 0)
	return ERR_INVALID_DATA;
    else {
	*y = sqrt(x);
	return ERR_NONE;
    }
}

static int mappable_sqrt_c(double xre, double xim, double *yre, double *yim)
								COMMANDS1_SECT;
static int mappable_sqrt_c(double xre, double xim, double *yre, double *yim) {
    /* TODO: review -- is there a better way, without all the trig? */
    double r = sqrt(hypot(xre, xim));
    double phi = atan2(xim, xre) / 2;
    double s, c;
    sincos(phi, &s, &c);
    *yre = r * c;
    *yim = r * s;
    return ERR_NONE;
}

int docmd_sqrt(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL) {
	double x = ((vartype_real *) reg_x)->x;
	vartype *v;
	if (x < 0) {
	    if (flags.f.real_result_only)
		return ERR_INVALID_DATA;
	    v = new_complex(0, sqrt(-x));
	} else
	    v = new_real(sqrt(x));
	if (v == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	unary_result(v);
	return ERR_NONE;
    } else if (reg_x->type == TYPE_STRING) {
	return ERR_ALPHA_DATA_IS_INVALID;
    } else {
	vartype *v;
	int err = map_unary(reg_x, &v, mappable_sqrt_r, mappable_sqrt_c);
	if (err != ERR_NONE)
	    return err;
	unary_result(v);
	return ERR_NONE;
    }
}

static int mappable_square_r(double x, double *y) COMMANDS1_SECT;
static int mappable_square_r(double x, double *y) {
    double r = x * x;
    int inf;
    if ((inf = isinf(r)) != 0) {
	if (flags.f.range_error_ignore)
	    r = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *y = r;
    return ERR_NONE;
}

static int mappable_square_c(double xre, double xim,
			     double *yre, double *yim) COMMANDS1_SECT;
static int mappable_square_c(double xre, double xim, double *yre, double *yim) {
    double rre = xre * xre - xim * xim;
    double rim = 2 * xre * xim;
    int inf;
    if ((inf = isinf(rre)) != 0) {
	if (flags.f.range_error_ignore)
	    rre = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    if ((inf = isinf(rim)) != 0) {
	if (flags.f.range_error_ignore)
	    rim = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *yre = rre;
    *yim = rim;
    return ERR_NONE;
}

int docmd_square(arg_struct *arg) {
    if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else {
	vartype *v;
	int err = map_unary(reg_x, &v, mappable_square_r, mappable_square_c);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    }
}

static int mappable_inv_r(double x, double *y) COMMANDS1_SECT;
static int mappable_inv_r(double x, double *y) {
    int inf;
    if (x == 0)
	return ERR_DIVIDE_BY_0;
    *y = 1 / x;
    if ((inf = isinf(*y)) != 0) {
	if (!flags.f.range_error_ignore)
	    return ERR_OUT_OF_RANGE;
	*y = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
    }
    return ERR_NONE;
}

static int mappable_inv_c(double xre, double xim,
			     double *yre, double *yim) COMMANDS1_SECT;
static int mappable_inv_c(double xre, double xim, double *yre, double *yim) {
    int inf;
    double h = hypot(xre, xim);
    if (h == 0)
	return ERR_DIVIDE_BY_0;
    *yre = xre / h / h;
    if ((inf = isinf(*yre)) != 0) {
	if (!flags.f.range_error_ignore)
	    return ERR_OUT_OF_RANGE;
	*yre = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
    }
    *yim = (-xim) / h / h;
    if ((inf = isinf(*yim)) != 0) {
	if (!flags.f.range_error_ignore)
	    return ERR_OUT_OF_RANGE;
	*yim = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
    }
    return ERR_NONE;
}

int docmd_inv(arg_struct *arg) {
    if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else {
	vartype *v;
	int err = map_unary(reg_x, &v, mappable_inv_r, mappable_inv_c);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    }
}

int docmd_y_pow_x(arg_struct *arg) {
    double yr, yphi;
    int inf;
    vartype *res;

    if (reg_x->type == TYPE_STRING || reg_y->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else if (reg_x->type == TYPE_REALMATRIX
	    || reg_x->type == TYPE_COMPLEXMATRIX
	    || reg_y->type == TYPE_REALMATRIX
	    || reg_y->type == TYPE_COMPLEXMATRIX)
	return ERR_INVALID_TYPE;
    else if (reg_x->type == TYPE_REAL) {
	double x = ((vartype_real *) reg_x)->x;
	if (x == floor(x)) {
	    /* Integer exponent */
	    if (reg_y->type == TYPE_REAL) {
		/* Real number to integer power */
		double y = ((vartype_real *) reg_y)->x;
		double r = pow(y, x);
		if (isnan(r))
		    /* Should not happen; pow() is supposed to be able
		     * to raise negative numbers to integer exponents
		     */
		    return ERR_INVALID_DATA;
		if ((inf = isinf(r)) != 0) {
		    if (!flags.f.range_error_ignore)
			return ERR_OUT_OF_RANGE;
		    r = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
		}
		res = new_real(r);
		if (res == NULL)
		    return ERR_INSUFFICIENT_MEMORY;
		else {
		    binary_result(res);
		    return ERR_NONE;
		}
	    } else {
		/* Complex number to integer power */
		double rre, rim, yre, yim;
		int4 ex;
		if (x < -2147483647 || x > 2147483647)
		    /* For really huge exponents, the repeated-squaring
		     * algorithm for integer exponents loses its accuracy
		     * and speed advantage, and we switch to the general
		     * complex-to-real-power code instead.
		     */
		    goto complex_pow_real_1;
		rre = 1;
		rim = 0;
		yre = ((vartype_complex *) reg_y)->re;
		yim = ((vartype_complex *) reg_y)->im;
		ex = (int4) x;
		if (yre == 0 && yim == 0) {
		    if (ex < 0)
			return ERR_INVALID_DATA;
		    else if (ex == 0) {
			res = new_complex(1, 0);
			if (res == NULL)
			    return ERR_INSUFFICIENT_MEMORY;
			else {
			    binary_result(res);
			    return ERR_NONE;
			}
		    }
		}
		if (ex < 0) {
		    double h = hypot(yre, yim);
		    yre = yre / h / h;
		    yim = (-yim) / h / h;
		    ex = -ex;
		}
		while (1) {
		    double tmp;
		    if ((ex & 1) != 0) {
			tmp = rre * yre - rim * yim;
			rim = rre * yim + rim * yre;
			rre = tmp;
			/* TODO: can one component be infinite while
			 * the other is zero? If yes, how do we handle
			 * that?
			 */
			if (isinf(rre) && isinf(rim))
			    break;
			if (rre == 0 && rim == 0)
			    break;
		    }
		    ex >>= 1;
		    if (ex == 0)
			break;
		    tmp = yre * yre - yim * yim;
		    yim = 2 * yre * yim;
		    yre = tmp;
		}
		if ((inf = isinf(rre)) != 0) {
		    if (!flags.f.range_error_ignore)
			return ERR_OUT_OF_RANGE;
		    rre = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
		}
		if ((inf = isinf(rim)) != 0) {
		    if (!flags.f.range_error_ignore)
			return ERR_OUT_OF_RANGE;
		    rim = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
		}
		res = new_complex(rre, rim);
		if (res == NULL)
		    return ERR_INSUFFICIENT_MEMORY;
		else {
		    binary_result(res);
		    return ERR_NONE;
		}
	    }
	} else if (reg_y->type == TYPE_REAL) {
	    /* Real number to noninteger real power */
	    double y = ((vartype_real *) reg_y)->x;
	    double r;
	    if (y < 0) {
		if (flags.f.real_result_only)
		    return ERR_INVALID_DATA;
		yr = -y;
		yphi = PI;
		goto complex_pow_real_2;
	    }
	    r = pow(y, x);
	    inf = isinf(r);
	    if (inf != 0) {
		if (!flags.f.range_error_ignore)
		    return ERR_OUT_OF_RANGE;
		r = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
	    }
	    res = new_real(r);
	    if (res == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	    else {
		binary_result(res);
		return ERR_NONE;
	    }
	} else {
	    /* Complex (or negative real) number to noninteger real power */
	    double yre, yim, rre, rim;
	    complex_pow_real_1:
	    yre = ((vartype_complex *) reg_y)->re;
	    yim = ((vartype_complex *) reg_y)->im;
	    yr = hypot(yre, yim);
	    yphi = atan2(yim, yre);
	    complex_pow_real_2:
	    yr = pow(yr, x);
	    yphi *= x;
	    if ((inf = isinf(yr)) != 0) {
		if (!flags.f.range_error_ignore)
		    return ERR_OUT_OF_RANGE;
		else {
		    double re, im;
		    sincos(yphi, &im, &re);
		    rre = re == 0 ? 0 :
			    re < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
		    rim = im == 0 ? 0 :
			    im < 0 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
		}
	    } else {
		double re, im;
		sincos(yphi, &im, &re);
		rre = yr * re;
		rim = yr * im;
	    }
	    res = new_complex(rre, rim);
	    if (res == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	    else {
		binary_result(res);
		return ERR_NONE;
	    }
	}
    } else {
	/* Real or complex number to complex power */
	double xre = ((vartype_complex *) reg_x)->re;
	double xim = ((vartype_complex *) reg_x)->im;
	double yre, yim;
	double lre, lim;
	double tmp;
	int err;
	if (reg_y->type == TYPE_REAL) {
	    yre = ((vartype_real *) reg_y)->x;
	    yim = 0;
	} else {
	    yre = ((vartype_complex *) reg_y)->re;
	    yim = ((vartype_complex *) reg_y)->im;
	}
	if (yre == 0 && yim == 0) {
	    if (xre < 0 || (xre == 0 && xim != 0))
		return ERR_INVALID_DATA;
	    else if (xre == 0)
		res = new_complex(1, 0);
	    else
		res = new_complex(0, 0);
	    if (res == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	    else {
		binary_result(res);
		return ERR_NONE;
	    }
	}
	err = mappable_ln_c(yre, yim, &lre, &lim);
	if (err != ERR_NONE)
	    return err;
	tmp = lre * xre - lim * xim;
	lim = lre * xim + lim * xre;
	lre = tmp;
	err = mappable_e_pow_x_c(lre, lim, &xre, &xim);
	if (err != ERR_NONE)
	    return err;
	res = new_complex(xre, xim);
	if (res == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	else {
	    binary_result(res);
	    return ERR_NONE;
	}
    }
}

int docmd_percent(arg_struct *arg) {
    if (reg_x->type == TYPE_STRING || reg_y->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_x->type != TYPE_REAL || reg_y->type != TYPE_REAL)
	return ERR_INVALID_TYPE;
    else {
	vartype_real *x = (vartype_real *) reg_x;
	vartype_real *y = (vartype_real *) reg_y;
	double res = x->x * y->x;
	if (isinf(res)) {
	    /* Try different evaluation order */
	    res = (x->x / 100.0) * y->x;
	    if (isinf(res))
		return ERR_OUT_OF_RANGE;
	} else
	    res /= 100.0;
	free_vartype(reg_lastx);
	reg_lastx = reg_x;
	reg_x = new_real(res);
	if (flags.f.trace_print && flags.f.printer_exists)
	    docmd_prx(NULL);
	return ERR_NONE;
    }
}

int docmd_pi(arg_struct *arg) {
    vartype *v = new_real(PI);
    if (v == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    recall_result(v);
    return ERR_NONE;
}

int docmd_complex(arg_struct *arg) {
    switch (reg_x->type) {
	case TYPE_REAL: {
	    if (reg_y->type == TYPE_STRING)
		return ERR_ALPHA_DATA_IS_INVALID;
	    else if (reg_y->type != TYPE_REAL)
		return ERR_INVALID_TYPE;
	    free_vartype(reg_lastx);
	    reg_lastx = reg_x;
	    if (flags.f.polar) {
		double re, im;
		generic_p2r(((vartype_real *) reg_y)->x,
			    ((vartype_real *) reg_x)->x, &re, &im);
		reg_x = new_complex(re, im);
	    } else {
		reg_x = new_complex(((vartype_real *) reg_y)->x,
				    ((vartype_real *) reg_x)->x);
	    }
	    free_vartype(reg_y);
	    reg_y = reg_z;
	    reg_z = dup_vartype(reg_t);
	    break;
	}
	case TYPE_COMPLEX: {
	    free_vartype(reg_lastx);
	    reg_lastx = reg_x;
	    free_vartype(reg_t);
	    reg_t = reg_z;
	    reg_z = reg_y;
	    if (flags.f.polar) {
		double r, phi;
		int inf;
		generic_r2p(((vartype_complex *) reg_x)->re,
			    ((vartype_complex *) reg_x)->im, &r, &phi);
		if ((inf = isinf(r)) != 0)
		    r = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
		reg_y = new_real(r);
		reg_x = new_real(phi);
	    } else {
		reg_y = new_real(((vartype_complex *) reg_x)->re);
		reg_x = new_real(((vartype_complex *) reg_x)->im);
	    }
	    break;
	}
	case TYPE_REALMATRIX: {
	    if (reg_y->type == TYPE_STRING)
		return ERR_ALPHA_DATA_IS_INVALID;
	    else if (reg_y->type != TYPE_REALMATRIX)
		return ERR_INVALID_TYPE;
	    else {
		vartype_realmatrix *re_m = (vartype_realmatrix *) reg_y;
		vartype_realmatrix *im_m = (vartype_realmatrix *) reg_x;
		vartype_complexmatrix *cm;
		int4 sz, i;
		if (re_m->rows != im_m->rows
			|| re_m->columns != im_m->columns)
		    return ERR_DIMENSION_ERROR;

		sz = re_m->rows * re_m->columns;
		for (i = 0; i < sz; i++)
		    if (re_m->array->is_string[i])
			return ERR_ALPHA_DATA_IS_INVALID;
		for (i = 0; i < sz; i++)
		    if (im_m->array->is_string[i])
			return ERR_ALPHA_DATA_IS_INVALID;

		cm = (vartype_complexmatrix *)
				new_complexmatrix(re_m->rows, re_m->columns);
		if (cm == NULL)
		    return ERR_INSUFFICIENT_MEMORY;
		if (flags.f.polar) {
		    for (i = 0; i < sz; i++) {
			generic_p2r(re_m->array->data[i].d,
				    im_m->array->data[i].d,
				    &cm->array->data[2 * i],
				    &cm->array->data[2 * i + 1]);
		    }
		} else {
		    for (i = 0; i < sz; i++) {
			cm->array->data[2 * i] = re_m->array->data[i].d;
			cm->array->data[2 * i + 1] = im_m->array->data[i].d;
		    }
		}
		free_vartype(reg_lastx);
		reg_lastx = reg_x;
		free_vartype(reg_y);
		reg_y = reg_z;
		reg_z = dup_vartype(reg_t);
		reg_x = (vartype *) cm;
		break;
	    }
	}
	case TYPE_COMPLEXMATRIX: {
	    vartype_complexmatrix *cm = (vartype_complexmatrix *) reg_x;
	    int4 rows = cm->rows;
	    int4 columns = cm->columns;
	    int4 sz = rows * columns;
	    int4 i;
	    vartype_realmatrix *re_m = (vartype_realmatrix *)
					    new_realmatrix(rows, columns);
	    vartype_realmatrix *im_m;
	    if (re_m == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	    im_m = (vartype_realmatrix *) new_realmatrix(rows, columns);
	    if (im_m == NULL) {
		free_vartype((vartype *) re_m);
		return ERR_INSUFFICIENT_MEMORY;
	    }
	    if (flags.f.polar) {
		for (i = 0; i < sz; i++) {
		    double r, phi;
		    int inf;
		    generic_r2p(cm->array->data[2 * i],
				cm->array->data[2 * i + 1], &r, &phi);
		    if ((inf = isinf(r)) != 0)
			r = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
		    re_m->array->data[i].d = r;
		    im_m->array->data[i].d = phi;
		}
	    } else {
		for (i = 0; i < sz; i++) {
		    re_m->array->data[i].d = cm->array->data[2 * i];
		    im_m->array->data[i].d = cm->array->data[2 * i + 1];
		}
	    }
	    free_vartype(reg_lastx);
	    reg_lastx = reg_x;
	    free_vartype(reg_t);
	    reg_t = reg_z;
	    reg_z = reg_y;
	    reg_y = (vartype *) re_m;
	    reg_x = (vartype *) im_m;
	    break;
	}
	case TYPE_STRING:
	    return ERR_ALPHA_DATA_IS_INVALID;
    }
    if (flags.f.trace_print && flags.f.printer_exists)
	docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_sto(arg_struct *arg) {
    return generic_sto(arg, 0);
}

int docmd_sto_div(arg_struct *arg) {
    return generic_sto(arg, '/');
}

int docmd_sto_mul(arg_struct *arg) {
    return generic_sto(arg, '*');
}

int docmd_sto_sub(arg_struct *arg) {
    return generic_sto(arg, '-');
}

int docmd_sto_add(arg_struct *arg) {
    return generic_sto(arg, '+');
}

int docmd_rcl(arg_struct *arg) {
    vartype *v;
    int err = generic_rcl(arg, &v);
    if (err == ERR_NONE)
	recall_result(v);
    return err;
}

/* Temporary for use by docmd_rcl_div() & docmd_rcl_mul() */
static vartype *temp_v;

static void docmd_rcl_div_completion(int error, vartype *res) COMMANDS1_SECT;
static void docmd_rcl_div_completion(int error, vartype *res) {
    free_vartype(temp_v);
    if (error == ERR_NONE)
	unary_result(res);
}

int docmd_rcl_div(arg_struct *arg) {
    int err = generic_rcl(arg, &temp_v);
    if (err != ERR_NONE)
	return err;
    return generic_div(temp_v, reg_x, docmd_rcl_div_completion);
}

static void docmd_rcl_mul_completion(int error, vartype *res) COMMANDS1_SECT;
static void docmd_rcl_mul_completion(int error, vartype *res) {
    free_vartype(temp_v);
    if (error == ERR_NONE)
	unary_result(res);
}

int docmd_rcl_mul(arg_struct *arg) {
    int err = generic_rcl(arg, &temp_v);
    if (err != ERR_NONE)
	return err;
    return generic_mul(temp_v, reg_x, docmd_rcl_mul_completion);
}

int docmd_rcl_sub(arg_struct *arg) {
    vartype *v, *w;
    int err = generic_rcl(arg, &v);
    if (err != ERR_NONE)
	return err;
    err = generic_sub(v, reg_x, &w);
    free_vartype(v);
    if (err == ERR_NONE)
	unary_result(w);
    return err;
}

int docmd_rcl_add(arg_struct *arg) {
    vartype *v, *w;
    int err = generic_rcl(arg, &v);
    if (err != ERR_NONE)
	return err;
    err = generic_add(v, reg_x, &w);
    free_vartype(v);
    if (err == ERR_NONE)
	unary_result(w);
    return err;
}

int docmd_fix(arg_struct *arg) {
    int err;
    int4 num;
    err = arg_to_num(arg, &num);
    if (err != ERR_NONE)
	return err;
    if (num > 11)
	return ERR_INVALID_DATA;
    flags.f.digits_bit3 = (num & 8) != 0;
    flags.f.digits_bit2 = (num & 4) != 0;
    flags.f.digits_bit1 = (num & 2) != 0;
    flags.f.digits_bit0 = (num & 1) != 0;
    flags.f.fix_or_all = 1;
    flags.f.eng_or_all = 0;
    if (flags.f.trace_print && flags.f.printer_exists)
	docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_sci(arg_struct *arg) {
    int err;
    int4 num;
    err = arg_to_num(arg, &num);
    if (err != ERR_NONE)
	return err;
    if (num > 11)
	return ERR_INVALID_DATA;
    flags.f.digits_bit3 = (num & 8) != 0;
    flags.f.digits_bit2 = (num & 4) != 0;
    flags.f.digits_bit1 = (num & 2) != 0;
    flags.f.digits_bit0 = (num & 1) != 0;
    flags.f.fix_or_all = 0;
    flags.f.eng_or_all = 0;
    if (flags.f.trace_print && flags.f.printer_exists)
	docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_eng(arg_struct *arg) {
    int err;
    int4 num;
    err = arg_to_num(arg, &num);
    if (err != ERR_NONE)
	return err;
    if (num > 11)
	return ERR_INVALID_DATA;
    flags.f.digits_bit3 = (num & 8) != 0;
    flags.f.digits_bit2 = (num & 4) != 0;
    flags.f.digits_bit1 = (num & 2) != 0;
    flags.f.digits_bit0 = (num & 1) != 0;
    flags.f.fix_or_all = 0;
    flags.f.eng_or_all = 1;
    if (flags.f.trace_print && flags.f.printer_exists)
	docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_all(arg_struct *arg) {
    flags.f.digits_bit3 = 0;
    flags.f.digits_bit2 = 0;
    flags.f.digits_bit1 = 0;
    flags.f.digits_bit0 = 0;
    flags.f.fix_or_all = 1;
    flags.f.eng_or_all = 1;
    if (flags.f.trace_print && flags.f.printer_exists)
	docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_null(arg_struct *arg) {
    return ERR_NONE;
}

int docmd_asto(arg_struct *arg) {
    /* I'm lazy enough to spot that ASTO has exactly the same side effects as
     * STO with the first 6 characters of ALPHA in ST X. If you're also aware
     * that new_string() constructs a string of no more than 6 characters,
     * you'll see that this code does the job quite nicely.
     */
    if (arg->type == ARGTYPE_STK && arg->val.stk == 'X') {
	free_vartype(reg_x);
	reg_x = new_string(reg_alpha, reg_alpha_length);
	return ERR_NONE;
    } else {
	vartype *saved_x = reg_x;
	int err;
	reg_x = new_string(reg_alpha, reg_alpha_length);
	err = docmd_sto(arg);
	free_vartype(reg_x);
	reg_x = saved_x;
	return err;
    }
}

int docmd_arcl(arg_struct *arg) {
    vartype *saved_x = dup_vartype(reg_x);
    vartype *v;
    int saved_nostacklift = flags.f.stack_lift_disable;
    int saved_trace = flags.f.trace_print;
    int err;
    char buf[100];
    int bufptr;

    /* Do some contortions to use docmd_rcl() to get the variable,
     * and do it without affecting the stack.
     */
    flags.f.stack_lift_disable = 1;
    flags.f.trace_print = 0;
    err = docmd_rcl(arg);
    flags.f.stack_lift_disable = saved_nostacklift;
    flags.f.trace_print = saved_trace;
    if (err != ERR_NONE) {
	free_vartype(saved_x);
	return err;
    } else {
	v = reg_x;
	reg_x = saved_x;
    }
    /* Convert the variable to a string, using the same conversion
     * used when displaying the variable in 'normal' mode -- except
     * for strings, which we append with no quotes.
     */
    if (v->type == TYPE_STRING) {
	vartype_string *s = (vartype_string *) v;
	append_alpha_string(s->text, s->length, 0);
    } else {
	bufptr = vartype2string(v, buf, 100);
	append_alpha_string(buf, bufptr, 0);
    }
    free_vartype(v);

    if (flags.f.alpha_mode && !program_running())
	set_alpha_entry(1);
    if (flags.f.trace_print && flags.f.printer_exists)
	docmd_pra(NULL);
    return ERR_NONE;
}

int docmd_cla(arg_struct *arg) {
    reg_alpha_length = 0;
    set_alpha_entry(0);
    return ERR_NONE;
}

int docmd_deg(arg_struct *arg) {
    flags.f.grad = 0;
    flags.f.rad = 0;
    shell_annunciators(-1, -1, -1, -1, 0, 0);
    return ERR_NONE;
}

int docmd_rad(arg_struct *arg) {
    flags.f.grad = 0;
    flags.f.rad = 1;
    shell_annunciators(-1, -1, -1, -1, 0, 1);
    return ERR_NONE;
}

int docmd_grad(arg_struct *arg) {
    flags.f.grad = 1;
    flags.f.rad = 0;
    shell_annunciators(-1, -1, -1, -1, 1, 1);
    return ERR_NONE;
}

int docmd_rect(arg_struct *arg) {
    flags.f.polar = 0;
    if (flags.f.trace_print && flags.f.printer_exists)
	docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_polar(arg_struct *arg) {
    flags.f.polar = 1;
    if (flags.f.trace_print && flags.f.printer_exists)
	docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_size(arg_struct *arg) {
    if (arg->type != ARGTYPE_NUM)
	return ERR_INVALID_TYPE;
    return dimension_array("REGS", 4, arg->val.num, 1);
}

int docmd_quiet(arg_struct *arg) {
    flags.f.audio_enable = !flags.f.audio_enable;
    return ERR_NONE;
}

int docmd_cpxres(arg_struct *arg) {
    flags.f.real_result_only = 0;
    return ERR_NONE;
}

int docmd_realres(arg_struct *arg) {
    flags.f.real_result_only = 1;
    return ERR_NONE;
}

int docmd_keyasn(arg_struct *arg) {
    flags.f.local_label = 0;
    return ERR_NONE;
}

int docmd_lclbl(arg_struct *arg) {
    flags.f.local_label = 1;
    return ERR_NONE;
}

int docmd_rdxdot(arg_struct *arg) {
    flags.f.decimal_point = 1;
    if (flags.f.trace_print && flags.f.printer_exists)
	docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_rdxcomma(arg_struct *arg) {
    flags.f.decimal_point = 0;
    if (flags.f.trace_print && flags.f.printer_exists)
	docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_clsigma(arg_struct *arg) {
    vartype *regs = recall_var("REGS", 4);
    vartype_realmatrix *r;
    int4 first = mode_sigma_reg;
    int4 last = first + (flags.f.all_sigma ? 13 : 6);
    int4 size, i;
    if (regs == NULL)
	return ERR_SIZE_ERROR;
    if (regs->type != TYPE_REALMATRIX)
	return ERR_INVALID_TYPE;
    r = (vartype_realmatrix *) regs;
    size = r->rows * r->columns;
    if (last > size)
	return ERR_SIZE_ERROR;
    for (i = first; i < last; i++) {
	r->array->is_string[i] = 0;
	r->array->data[i].d = 0;
    }
    flags.f.log_fit_invalid = 0;
    flags.f.exp_fit_invalid = 0;
    flags.f.pwr_fit_invalid = 0;
    return ERR_NONE;
}

int docmd_clp(arg_struct *arg) {
    return clear_prgm(arg);
}

int docmd_clv(arg_struct *arg) {
    int err;
    if (arg->type == ARGTYPE_IND_NUM
	    || arg->type == ARGTYPE_IND_STK
	    || arg->type == ARGTYPE_IND_STR) {
	err = resolve_ind_arg(arg);
	if (err != ERR_NONE)
	    return err;
    }
    if (arg->type == ARGTYPE_STR) {
	purge_var(arg->val.text, arg->length);
	remove_shadow(arg->val.text, arg->length);
	return ERR_NONE;
    } else
	return ERR_INVALID_TYPE;
}

int docmd_clst(arg_struct *arg) {
    free_vartype(reg_x);
    reg_x = new_real(0);
    free_vartype(reg_y);
    reg_y = new_real(0);
    free_vartype(reg_z);
    reg_z = new_real(0);
    free_vartype(reg_t);
    reg_t = new_real(0);
    return ERR_NONE;
}

int docmd_clrg(arg_struct *arg) {
    vartype *regs = recall_var("REGS", 4);
    if (regs == NULL)
	return ERR_NONEXISTENT;
    if (regs->type == TYPE_REALMATRIX) {
	vartype_realmatrix *rm;
	int4 sz, i;
	if (!disentangle(regs))
	    return ERR_INSUFFICIENT_MEMORY;
	rm = (vartype_realmatrix *) regs;
	sz = rm->rows * rm->columns;
	for (i = 0; i < sz; i++)
	    rm->array->data[i].d = 0;
	for (i = 0; i < sz; i++)
	    rm->array->is_string[i] = 0;
	return ERR_NONE;
    } else if (regs->type == TYPE_COMPLEXMATRIX) {
	vartype_complexmatrix *cm;
	int4 sz, i;
	if (!disentangle(regs))
	    return ERR_INSUFFICIENT_MEMORY;
	cm = (vartype_complexmatrix *) regs;
	sz = 2 * cm->rows * cm->columns;
	for (i = 0; i < sz; i++)
	    cm->array->data[i] = 0;
	return ERR_NONE;
    } else {
	/* Should not happen; STO does not allow anything other
	 * than a matrix to be stored in 'REGS'.
	 */
	return ERR_INTERNAL_ERROR;
    }
}

int docmd_del(arg_struct *arg) {
    if (arg->type != ARGTYPE_NUM)
	return ERR_INVALID_TYPE;
    clear_prgm_lines(arg->val.num);
    return ERR_NONE;
}

int docmd_clkeys(arg_struct *arg) {
    clear_custom_menu();
    return ERR_NONE;
}

int docmd_cllcd(arg_struct *arg) {
    clear_display();
    flush_display();
    flags.f.message = 1;
    flags.f.two_line_message = 1;
    return ERR_NONE;
}

int docmd_clmenu(arg_struct *arg) {
    clear_prgm_menu();
    return ERR_NONE;
}

int docmd_clall(arg_struct *arg) {
    vartype *regs;

    /* Clear all registers */
    free_vartype(reg_x);
    reg_x = new_real(0);
    free_vartype(reg_y);
    reg_y = new_real(0);
    free_vartype(reg_z);
    reg_z = new_real(0);
    free_vartype(reg_t);
    reg_t = new_real(0);
    free_vartype(reg_lastx);
    reg_lastx = new_real(0);
    reg_alpha_length = 0;

    /* Exit all menus (even leaving the matrix editor
     * is guaranteed not to fail because there's 0 in X,
     * and that is always valid).
     */
    set_menu(MENULEVEL_APP, MENU_NONE);
    flags.f.prgm_mode = 0;

    /* Clear all programs and variables */
    clear_all_prgms();
    goto_dot_dot();
    purge_all_vars();
    regs = new_realmatrix(25, 1);
    store_var("REGS", 4, regs);

    return ERR_NONE;
}

static int mappable_to_deg(double x, double *y) COMMANDS1_SECT;
static int mappable_to_deg(double x, double *y) {
    double r;
    int inf;
    r = rad_to_deg(x);
    if ((inf = isinf(r)) != 0) {
	if (flags.f.range_error_ignore)
	    r = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *y = r;
    return ERR_NONE;
}

int docmd_to_deg(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL || reg_x->type == TYPE_REALMATRIX) {
	vartype *v;
	int err = map_unary(reg_x, &v, mappable_to_deg, NULL);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    } else if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else
	return ERR_INVALID_TYPE;
}

static int mappable_to_rad(double x, double *y) COMMANDS1_SECT;
static int mappable_to_rad(double x, double *y) {
    *y = deg_to_rad(x);
    return ERR_NONE;
}

int docmd_to_rad(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL || reg_x->type == TYPE_REALMATRIX) {
	vartype *v;
	int err = map_unary(reg_x, &v, mappable_to_rad, NULL);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    } else if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else
	return ERR_INVALID_TYPE;
}

static int mappable_to_hr(double x, double *y) COMMANDS1_SECT;
static int mappable_to_hr(double x, double *y) {
    int neg = x < 0;
    int8 ix, ixhr;
    double h, res;
    if (neg)
	x = -x;
    
    if (x == x + 1)
	res = x;
    else if (x < 0.01)
	res = x / 0.36;
    else {
	h = floor(x);
	x -= h;
	ix = (int8) (x * 1000000000000.0 + 0.5);
	ixhr = ix % LL(10000000000);
	ix /= LL(10000000000);
	ixhr += (ix % 100) * LL(6000000000);
	res = h + ixhr / 360000000000.0;
    }
    *y = neg ? -res : res;
    return ERR_NONE;
}

int docmd_to_hr(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL || reg_x->type == TYPE_REALMATRIX) {
	vartype *v;
	int err = map_unary(reg_x, &v, mappable_to_hr, NULL);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    } else if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else
	return ERR_INVALID_TYPE;
}

static int mappable_to_hms(double x, double *y) COMMANDS1_SECT;
static int mappable_to_hms(double x, double *y) {
    int neg = x < 0;
    double r, t;
    if (neg)
	x = -x;
    r = floor(x);
    x = (x - r) * 60;
    t = floor(x);
    r += t / 100 + (x - t) * 3 / 500;

    /* Round-off may have caused the minutes or seconds to reach 60;
     * detect this and fix...
     */
    r = fix_hms(r);

    *y = neg ? -r : r;
    return ERR_NONE;
}

int docmd_to_hms(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL || reg_x->type == TYPE_REALMATRIX) {
	vartype *v;
	int err = map_unary(reg_x, &v, mappable_to_hms, NULL);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    } else if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else
	return ERR_INVALID_TYPE;
}

int docmd_to_rec(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL) {
	if (reg_y->type == TYPE_REAL || reg_y->type == TYPE_COMPLEX) {
	    /* Note: the strange behavior re: real number in X, and
	     * complex number in Y, is for bug-compatibility with the
	     * real HP-42S. It's not very useful, but it doesn't really
	     * hurt either, I suppose. If I ever implement an "enhanced"
	     * Free42 mode, I'll probably make that combination of
	     * arguments return ERR_INVALID_TYPE, or at least offer the
	     * option of selecting that behavior.
	     */
	    double r = ((vartype_real *) reg_x)->x;
	    double phi = reg_y->type == TYPE_REAL
			    ? ((vartype_real *) reg_y)->x
			    : ((vartype_complex *) reg_y)->re;
	    double x, y;
	    vartype *vx, *vy;
	    generic_p2r(r, phi, &x, &y);
	    vx = new_real(x);
	    if (vx == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	    vy = new_real(y);
	    if (vy == NULL) {
		free_vartype(vx);
		return ERR_INSUFFICIENT_MEMORY;
	    }
	    free_vartype(reg_y);
	    reg_y = vy;
	    free_vartype(reg_lastx);
	    reg_lastx = reg_x;
	    reg_x = vx;
	    if (flags.f.trace_print && flags.f.printer_exists)
		docmd_prx(NULL);
	    return ERR_NONE;
	} else if (reg_y->type == TYPE_STRING)
	    return ERR_ALPHA_DATA_IS_INVALID;
	else
	    /* The original HP-42S has a bug here: it accepts real and complex
	     * matrices, and also complex numbers, in Y, when X is real.
	     * I allow the Y-is-complex while X-is-real behavior (see above),
	     * but the Y-is-matrix case does not yield anything useful or even
	     * recognizable on the real HP-42S, so I feel I'm not going to
	     * break anything (that wasn't broken to begin with) by returning
	     * an error message here.
	     */
	    return ERR_INVALID_TYPE;
    } else if (reg_x->type == TYPE_COMPLEX) {
	vartype_complex *c = (vartype_complex *) reg_x;
	double x, y;
	vartype *v;
	generic_p2r(c->re, c->im, &x, &y);
	v = new_complex(x, y);
	if (v == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	unary_result(v);
	return ERR_NONE;
    } else if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else
	return ERR_INVALID_TYPE;
}

int docmd_to_pol(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL) {
	if (reg_y->type == TYPE_REAL || reg_y->type == TYPE_COMPLEX) {
	    /* Note: the strange behavior re: real number in X, and
	     * complex number in Y, is for bug-compatibility with the
	     * real HP-42S. It's not very useful, but it doesn't really
	     * hurt either, I suppose. If I ever implement an "enhanced"
	     * Free42 mode, I'll probably make that combination of
	     * arguments return ERR_INVALID_TYPE, or at least offer the
	     * option of selecting that behavior.
	     */
	    double x = ((vartype_real *) reg_x)->x;
	    double y = reg_y->type == TYPE_REAL
			    ? ((vartype_real *) reg_y)->x
			    : ((vartype_complex *) reg_y)->re;
	    double r, phi;
	    vartype *vx, *vy;
	    generic_r2p(x, y, &r, &phi);
	    if (isinf(r)) {
		if (flags.f.range_error_ignore)
		    r = POS_HUGE_DOUBLE;
		else
		    return ERR_OUT_OF_RANGE;
	    }
	    vx = new_real(r);
	    if (vx == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	    vy = new_real(phi);
	    if (vy == NULL) {
		free_vartype(vx);
		return ERR_INSUFFICIENT_MEMORY;
	    }
	    free_vartype(reg_y);
	    reg_y = vy;
	    free_vartype(reg_lastx);
	    reg_lastx = reg_x;
	    reg_x = vx;
	    if (flags.f.trace_print && flags.f.printer_exists)
		docmd_prx(NULL);
	    return ERR_NONE;
	} else if (reg_y->type == TYPE_STRING)
	    return ERR_ALPHA_DATA_IS_INVALID;
	else
	    /* The original HP-42S has a bug here: it accepts real and complex
	     * matrices, and also complex numbers, in Y, when X is real.
	     * I allow the Y-is-complex while X-is-real behavior (see above),
	     * but the Y-is-matrix case does not yield anything useful or even
	     * recognizable on the real HP-42S, so I feel I'm not going to
	     * break anything (that wasn't broken to begin with) by returning
	     * an error message here.
	     */
	    return ERR_INVALID_TYPE;
    } else if (reg_x->type == TYPE_COMPLEX) {
	vartype_complex *c = (vartype_complex *) reg_x;
	double r, phi;
	vartype *v;
	generic_r2p(c->re, c->im, &r, &phi);
	if (isinf(r)) {
	    if (flags.f.range_error_ignore)
		r = POS_HUGE_DOUBLE;
	    else
		return ERR_OUT_OF_RANGE;
	}
	v = new_complex(r, phi);
	if (v == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	unary_result(v);
	return ERR_NONE;
    } else if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else
	return ERR_INVALID_TYPE;
}

static int mappable_ip(double x, double *y) COMMANDS1_SECT;
static int mappable_ip(double x, double *y) {
    if (core_settings.ip_hack) {
	if (x < 0)
	    *y = -floor(5e-9 - x);
	else
	    *y = floor(5e-9 + x);
    } else {
	if (x < 0)
	    *y = -floor(-x);
	else
	    *y = floor(x);
    }
    return ERR_NONE;
}

int docmd_ip(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL || reg_x->type == TYPE_REALMATRIX) {
	vartype *v;
	int err = map_unary(reg_x, &v, mappable_ip, NULL);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    } else if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else
	return ERR_INVALID_TYPE;
}

static int mappable_fp(double x, double *y) COMMANDS1_SECT;
static int mappable_fp(double x, double *y) {
    if (core_settings.ip_hack) {
	if (x < 0)
	    *y = x + floor(5e-9 - x);
	else
	    *y = x - floor(5e-9 + x);
    } else {
	if (x < 0)
	    *y = x + floor(-x);
	else
	    *y = x - floor(x);
    }
    return ERR_NONE;
}

int docmd_fp(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL || reg_x->type == TYPE_REALMATRIX) {
	vartype *v;
	int err = map_unary(reg_x, &v, mappable_fp, NULL);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    } else if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else
	return ERR_INVALID_TYPE;
}

static double rnd_multiplier;

static int mappable_rnd_r(double x, double *y) COMMANDS1_SECT;
static int mappable_rnd_r(double x, double *y) {
    if (flags.f.fix_or_all) {
	if (flags.f.eng_or_all)
	    *y = x;
	else {
	    double t = x;
	    int neg = t < 0;
	    if (neg)
		t = -t;
	    if (t > 1e20)
		*y = x;
	    else {
		t = floor(t * rnd_multiplier + 0.5) / rnd_multiplier;
		*y = neg ? -t : t;
	    }
	}
	return ERR_NONE;
    } else {
	double t = x;
	int neg;
	double scale;
	if (t == 0)
	    *y = 0;
	else {
	    if (t < 0) {
		t = -t;
		neg = 1;
	    } else
		neg = 0;
	    scale = pow(10, floor(log10(t)));
	    if (scale > t) {
		/* Theoretically, this can't happen, but due to limited
		 * precision, the log of something like 9.999999999+
		 * might be computed as 1 instead of 0.99999999+,
		 * hence the extra check.
		 */
		scale /= 10;
	    }
	    t = floor(t / scale * rnd_multiplier + 0.5)
					    / rnd_multiplier * scale;
	    if (isinf(t)) {
		if (flags.f.range_error_ignore)
		    *y = neg ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
		else
		    return ERR_OUT_OF_RANGE;
	    } else
		*y = neg ? -t : t;
	}
	return ERR_NONE;
    }
}

static int mappable_rnd_c(double xre, double xim, double *yre, double *yim)
								COMMANDS1_SECT;
static int mappable_rnd_c(double xre, double xim, double *yre, double *yim) {
    int err = mappable_rnd_r(xre, yre);
    if (err != ERR_NONE)
	return err;
    return mappable_rnd_r(xim, yim);
}

int docmd_rnd(arg_struct *arg) {
    if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else {
	vartype *v;
	int err;
	int digits = 0;
	if (flags.f.digits_bit3) digits += 8;
	if (flags.f.digits_bit2) digits += 4;
	if (flags.f.digits_bit1) digits += 2;
	if (flags.f.digits_bit0) digits += 1;
	rnd_multiplier = pow(10, digits);
	err = map_unary(reg_x, &v, mappable_rnd_r, mappable_rnd_c);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    }
}

int docmd_abs(arg_struct *arg) {
    /* Can't use map_unary here because it does not support
     * complex-to-real mappers. Oh, well, this only affects
     * ABS, to what the heck.
     */
    switch (reg_x->type) {
	case TYPE_REAL: {
	    vartype *r;
	    double x = ((vartype_real *) reg_x)->x;
	    if (x < 0)
		x = -x;
	    r = new_real(x);
	    if (r == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	    unary_result(r);
	    return ERR_NONE;
	}
	case TYPE_COMPLEX: {
	    vartype *r;
	    double re = ((vartype_complex *) reg_x)->re;
	    double im = ((vartype_complex *) reg_x)->im;
	    r = new_real(hypot(re, im));
	    if (r == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	    unary_result(r);
	    return ERR_NONE;
	}
	case TYPE_STRING: {
	    return ERR_ALPHA_DATA_IS_INVALID;
	}
	case TYPE_REALMATRIX: {
	    vartype_realmatrix *src;
	    vartype_realmatrix *dst;
	    int4 size, i;
	    src = (vartype_realmatrix *) reg_x;
	    dst = (vartype_realmatrix *)
				new_realmatrix(src->rows, src->columns);
	    if (dst == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	    if (!is_pure_real(reg_x)) {
		free_vartype((vartype *) dst);
		return ERR_ALPHA_DATA_IS_INVALID;
	    }
	    size = src->rows * src->columns;
	    for (i = 0; i < size; i++) {
		double x = src->array->data[i].d;
		if (x < 0)
		    x = -x;
		dst->array->data[i].d = x;
	    }
	    unary_result((vartype *) dst);
	    return ERR_NONE;
	}
	case TYPE_COMPLEXMATRIX: {
	    vartype_complexmatrix *src;
	    vartype_realmatrix *dst;
	    int4 size, i;
	    src = (vartype_complexmatrix *) reg_x;
	    dst = (vartype_realmatrix *)
				new_realmatrix(src->rows, src->columns);
	    if (dst == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	    size = src->rows * src->columns;
	    for (i = 0; i < size; i++) {
		dst->array->data[i].d = hypot(src->array->data[i * 2],
					      src->array->data[i * 2 + 1]);
	    }
	    unary_result((vartype *) dst);
	    return ERR_NONE;
	}
	default:
	    return ERR_INTERNAL_ERROR;
    }
}

static int mappable_sign(double xre, double xim, double *yre, double *yim)
								COMMANDS1_SECT;
static int mappable_sign(double xre, double xim, double *yre, double *yim) {
    double h = hypot(xre, xim);
    if (h == 0) {
	*yre = 0;
	*yim = 0;
    } else {
	*yre = xre / h;
	*yim = xim / h;
    }
    return ERR_NONE;
}

int docmd_sign(arg_struct *arg) {
    switch (reg_x->type) {
	case TYPE_REAL: {
	    /* Note that this implementation has sign(0) = 1, which is not
	     * how most programming languages handle zero. This is a holdover
	     * from the HP-41C, where SIGN did double duty for recognizing
	     * strings: they would return zero and could be recognized in this
	     * way.
	     * The HP-42S has the STR? function for recognizing strings, but
	     * it still supports the HP-41C's SIGN behavior for compatibility.
	     * And so does Free42, of course.
	     */
	    vartype *r = new_real(((vartype_real *) reg_x)->x < 0 ? -1 : 1);
	    if (r == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	    unary_result(r);
	    return ERR_NONE;
	}
	case TYPE_STRING: {
	    vartype *r = new_real(0);
	    if (r == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	    unary_result(r);
	    return ERR_NONE;
	}
	case TYPE_REALMATRIX: {
	    /* Can't use the mapper here because it won't handle strings. */
	    vartype_realmatrix *src;
	    vartype_realmatrix *dst;
	    int4 size, i;
	    src = (vartype_realmatrix *) reg_x;
	    dst = (vartype_realmatrix *)
				new_realmatrix(src->rows, src->columns);
	    if (dst == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	    size = src->rows * src->columns;
	    for (i = 0; i < size; i++) {
		if (src->array->is_string[i])
		    dst->array->data[i].d = 0;
		else
		    dst->array->data[i].d = src->array->data[i].d < 0 ? -1 : 1;
	    }
	    unary_result((vartype *) dst);
	    return ERR_NONE;
	}
	case TYPE_COMPLEX:
	case TYPE_COMPLEXMATRIX: {
	    vartype *v;
	    map_unary(reg_x, &v, NULL, mappable_sign);
	    unary_result((vartype *) v);
	    return ERR_NONE;
	}
	default:
	    return ERR_INTERNAL_ERROR;
    }
}

int docmd_mod(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL && reg_y->type == TYPE_REAL) {
	double x = ((vartype_real *) reg_x)->x;
	double y = ((vartype_real *) reg_y)->x;
	double res;
	vartype *v;
	if (x == 0)
	    res = y;
	else if (y == 0)
	    res = 0;
	else {
	    res = fmod(y, x);
	    if (res != 0 && ((x > 0 && y < 0) || (x < 0 && y > 0)))
		res += x;
	}
	v = new_real(res);
	if (v == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	binary_result(v);
	return ERR_NONE;
    } else if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else if (reg_x->type != TYPE_REAL)
	return ERR_INVALID_TYPE;
    else if (reg_y->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else
	return ERR_INVALID_TYPE;
}
