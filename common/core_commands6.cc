/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2010  Thomas Okken
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

#include "core_commands2.h"
#include "core_commands6.h"
#include "core_display.h"
#include "core_helpers.h"
#include "core_main.h"
#include "core_math2.h"
#include "core_sto_rcl.h"
#include "core_variables.h"
#include "shell.h"

/********************************************************/
/* Implementations of HP-42S built-in functions, part 6 */
/********************************************************/

static int mappable_sin_r(phloat x, phloat *y) COMMANDS6_SECT;
static int mappable_sin_r(phloat x, phloat *y) {
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

static int mappable_sin_c(phloat xre, phloat xim,
			     phloat *yre, phloat *yim) COMMANDS6_SECT;
static int mappable_sin_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    /* NOTE: DEG/RAD/GRAD mode does not apply here. */
    phloat sinxre, cosxre;
    phloat sinhxim, coshxim;
    int inf;
    sincos(xre, &sinxre, &cosxre);
    sinhxim = sinh(xim);
    coshxim = cosh(xim);
    *yre = sinxre * coshxim;
    if ((inf = p_isinf(*yre)) != 0) {
	if (flags.f.range_error_ignore)
	    *yre = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *yim = cosxre * sinhxim;
    if ((inf = p_isinf(*yim)) != 0) {
	if (flags.f.range_error_ignore)
	    *yim = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
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

static int mappable_cos_r(phloat x, phloat *y) COMMANDS6_SECT;
static int mappable_cos_r(phloat x, phloat *y) {
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

static int mappable_cos_c(phloat xre, phloat xim,
			     phloat *yre, phloat *yim) COMMANDS6_SECT;
static int mappable_cos_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    /* NOTE: DEG/RAD/GRAD mode does not apply here. */
    phloat sinxre, cosxre;
    phloat sinhxim, coshxim;
    int inf;
    sincos(xre, &sinxre, &cosxre);
    sinhxim = sinh(xim);
    coshxim = cosh(xim);
    *yre = cosxre * coshxim;
    if ((inf = p_isinf(*yre)) != 0) {
	if (flags.f.range_error_ignore)
	    *yre = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *yim = -sinxre * sinhxim;
    if ((inf = p_isinf(*yim)) != 0) {
	if (flags.f.range_error_ignore)
	    *yim = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
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

static int mappable_tan_r(phloat x, phloat *y) COMMANDS6_SECT;
static int mappable_tan_r(phloat x, phloat *y) {
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
    if (p_isnan(*y))
	goto infinite;
    if ((inf = p_isinf(*y)) != 0) {
	infinite:
	if (flags.f.range_error_ignore)
	    *y = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
	else
	    return ERR_OUT_OF_RANGE;
    }
    return ERR_NONE;
}

static int mappable_tan_c(phloat xre, phloat xim,
			     phloat *yre, phloat *yim) COMMANDS6_SECT;
static int mappable_tan_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    /* NOTE: DEG/RAD/GRAD mode does not apply here. */
    phloat sinxre, cosxre;
    phloat sinhxim, coshxim;
    phloat re_sin, im_sin, re_cos, im_cos, abs_cos;
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
	    *yre = re_sin * re_cos + im_sin * im_cos > 0 ? POS_HUGE_PHLOAT
							 : NEG_HUGE_PHLOAT;
	    *yim = im_sin * re_cos - re_sin * im_cos > 0 ? POS_HUGE_PHLOAT
							 : NEG_HUGE_PHLOAT;
	    return ERR_NONE;
	} else
	    return ERR_OUT_OF_RANGE;
    }

    *yre = (re_sin * re_cos + im_sin * im_cos) / abs_cos / abs_cos;
    if ((inf = p_isinf(*yre)) != 0) {
	if (flags.f.range_error_ignore)
	    *yre = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *yim = (im_sin * re_cos - re_sin * im_cos) / abs_cos / abs_cos;
    if ((inf = p_isinf(*yim)) != 0) {
	if (flags.f.range_error_ignore)
	    *yim = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
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

static int mappable_asin_r(phloat x, phloat *y) COMMANDS6_SECT;
static int mappable_asin_r(phloat x, phloat *y) {
    if (x < -1 || x > 1)
	return ERR_INVALID_DATA;
    *y = rad_to_angle(asin(x));
    return ERR_NONE;
}

static int mappable_asin_c(phloat xre, phloat xim, phloat *yre, phloat *yim)
								COMMANDS6_SECT;
static int mappable_asin_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    phloat tre, tim;
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
	phloat x = ((vartype_real *) reg_x)->x;
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
	if (v == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
    } else {
	int err = map_unary(reg_x, &v, mappable_asin_r, mappable_asin_c);
	if (err != ERR_NONE)
	    return err;
    }
    unary_result(v);
    return ERR_NONE;
}

static int mappable_acos_r(phloat x, phloat *y) COMMANDS6_SECT;
static int mappable_acos_r(phloat x, phloat *y) {
    if (x < -1 || x > 1)
	return ERR_INVALID_DATA;
    *y = rad_to_angle(acos(x));
    return ERR_NONE;
}

static int mappable_acos_c(phloat xre, phloat xim, phloat *yre, phloat *yim)
								COMMANDS6_SECT;
static int mappable_acos_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    phloat tre, tim;
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
	phloat x = ((vartype_real *) reg_x)->x;
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
	if (v == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
    } else {
	int err = map_unary(reg_x, &v, mappable_acos_r, mappable_acos_c);
	if (err != ERR_NONE)
	    return err;
    }
    unary_result(v);
    return ERR_NONE;
}

static int mappable_atan_r(phloat x, phloat *y) COMMANDS6_SECT;
static int mappable_atan_r(phloat x, phloat *y) {
    *y = rad_to_angle(atan(x));
    return ERR_NONE;
}

static int mappable_atan_c(phloat xre, phloat xim, phloat *yre, phloat *yim)
								COMMANDS6_SECT;
static int mappable_atan_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    phloat tre, tim;
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

static int mappable_log_r(phloat x, phloat *y) COMMANDS6_SECT;
static int mappable_log_r(phloat x, phloat *y) {
    if (x <= 0)
	return ERR_INVALID_DATA;
    else {
	*y = log10(x);
	return ERR_NONE;
    }
}

static int mappable_log_c(phloat xre, phloat xim,
			     phloat *yre, phloat *yim) COMMANDS6_SECT;
static int mappable_log_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    phloat h = hypot(xre, xim);
    if (h == 0)
	return ERR_INVALID_DATA;
    else {
	if (p_isinf(h)) {
	    const phloat s = 10000;
	    h = hypot(xre / s, xim / s);
	    *yre = log10(h) + 4;
	} else
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

static int mappable_10_pow_x_r(phloat x, phloat *y) COMMANDS6_SECT;
static int mappable_10_pow_x_r(phloat x, phloat *y) {
    *y = pow(10, x);
    if (p_isinf(*y) != 0) {
	if (!flags.f.range_error_ignore)
	    return ERR_OUT_OF_RANGE;
	*y = POS_HUGE_PHLOAT;
    }
    return ERR_NONE;
}

static int mappable_10_pow_x_c(phloat xre, phloat xim,
			     phloat *yre, phloat *yim) COMMANDS6_SECT;
static int mappable_10_pow_x_c(phloat xre, phloat xim, phloat *yre, phloat *yim){
    int inf;
    phloat h;
    xim *= log(10);
    if ((inf = p_isinf(xim)) != 0)
	xim = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
    h = pow(10, xre);
    if ((inf = p_isinf(h)) == 0) {
	*yre = cos(xim) * h;
	*yim = sin(xim) * h;
	return ERR_NONE;
    } else if (flags.f.range_error_ignore) {
	phloat t = cos(xim);
	if (t == 0)
	    *yre = 0;
	else if (t < 0)
	    *yre = inf < 0 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
	else
	    *yre = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
	t = sin(xim);
	if (t == 0)
	    *yim = 0;
	else if (t < 0)
	    *yim = inf < 0 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
	else
	    *yim = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
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

static int mappable_ln_r(phloat x, phloat *y) COMMANDS6_SECT;
static int mappable_ln_r(phloat x, phloat *y) {
    if (x <= 0)
	return ERR_INVALID_DATA;
    else {
	*y = log(x);
	return ERR_NONE;
    }
}

static int mappable_ln_c(phloat xre, phloat xim,
			     phloat *yre, phloat *yim) COMMANDS6_SECT;
static int mappable_ln_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    phloat h = hypot(xre, xim);
    if (h == 0)
	return ERR_INVALID_DATA;
    else {
	if (p_isinf(h)) {
	    const phloat s = 10000;
	    h = hypot(xre / s, xim / s);
	    *yre = log(h) + log(s);
	} else
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

static int mappable_e_pow_x_r(phloat x, phloat *y) COMMANDS6_SECT;
static int mappable_e_pow_x_r(phloat x, phloat *y) {
    *y = exp(x);
    if (p_isinf(*y) != 0) {
	if (!flags.f.range_error_ignore)
	    return ERR_OUT_OF_RANGE;
	*y = POS_HUGE_PHLOAT;
    }
    return ERR_NONE;
}

static int mappable_e_pow_x_c(phloat xre, phloat xim,
			     phloat *yre, phloat *yim) COMMANDS6_SECT;
static int mappable_e_pow_x_c(phloat xre, phloat xim, phloat *yre, phloat *yim){
    phloat h = exp(xre);
    int inf = p_isinf(h);
    if (inf == 0) {
	*yre = cos(xim) * h;
	*yim = sin(xim) * h;
	return ERR_NONE;
    } else if (flags.f.range_error_ignore) {
	phloat t = cos(xim);
	if (t == 0)
	    *yre = 0;
	else if (t < 0)
	    *yre = inf < 0 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
	else
	    *yre = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
	t = sin(xim);
	if (t == 0)
	    *yim = 0;
	else if (t < 0)
	    *yim = inf < 0 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
	else
	    *yim = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
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

static int mappable_sqrt_r(phloat x, phloat *y) COMMANDS6_SECT;
static int mappable_sqrt_r(phloat x, phloat *y) {
    if (x < 0)
	return ERR_INVALID_DATA;
    else {
	*y = sqrt(x);
	return ERR_NONE;
    }
}

static int mappable_sqrt_c(phloat xre, phloat xim, phloat *yre, phloat *yim)
								COMMANDS6_SECT;
static int mappable_sqrt_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    /* TODO: review -- is there a better way, without all the trig? */
    phloat r = sqrt(hypot(xre, xim));
    phloat phi = atan2(xim, xre) / 2;
    phloat s, c;
    sincos(phi, &s, &c);
    *yre = r * c;
    *yim = r * s;
    return ERR_NONE;
}

int docmd_sqrt(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL) {
	phloat x = ((vartype_real *) reg_x)->x;
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

static int mappable_square_r(phloat x, phloat *y) COMMANDS6_SECT;
static int mappable_square_r(phloat x, phloat *y) {
    phloat r = x * x;
    int inf;
    if ((inf = p_isinf(r)) != 0) {
	if (flags.f.range_error_ignore)
	    r = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *y = r;
    return ERR_NONE;
}

static int mappable_square_c(phloat xre, phloat xim,
			     phloat *yre, phloat *yim) COMMANDS6_SECT;
static int mappable_square_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    phloat rre = xre * xre - xim * xim;
    phloat rim = 2 * xre * xim;
    int inf;
    if ((inf = p_isinf(rre)) != 0) {
	if (flags.f.range_error_ignore)
	    rre = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
	else
	    return ERR_OUT_OF_RANGE;
    }
    if ((inf = p_isinf(rim)) != 0) {
	if (flags.f.range_error_ignore)
	    rim = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
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

static int mappable_inv_r(phloat x, phloat *y) COMMANDS6_SECT;
static int mappable_inv_r(phloat x, phloat *y) {
    int inf;
    if (x == 0)
	return ERR_DIVIDE_BY_0;
    *y = 1 / x;
    if ((inf = p_isinf(*y)) != 0) {
	if (!flags.f.range_error_ignore)
	    return ERR_OUT_OF_RANGE;
	*y = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
    }
    return ERR_NONE;
}

static int mappable_inv_c(phloat xre, phloat xim,
			     phloat *yre, phloat *yim) COMMANDS6_SECT;
static int mappable_inv_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    int inf;
    phloat h = hypot(xre, xim);
    if (h == 0)
	return ERR_DIVIDE_BY_0;
    *yre = xre / h / h;
    if ((inf = p_isinf(*yre)) != 0) {
	if (!flags.f.range_error_ignore)
	    return ERR_OUT_OF_RANGE;
	*yre = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
    }
    *yim = (-xim) / h / h;
    if ((inf = p_isinf(*yim)) != 0) {
	if (!flags.f.range_error_ignore)
	    return ERR_OUT_OF_RANGE;
	*yim = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
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
    phloat yr, yphi;
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
	phloat x = ((vartype_real *) reg_x)->x;
	if (x == floor(x)) {
	    /* Integer exponent */
	    if (reg_y->type == TYPE_REAL) {
		/* Real number to integer power */
		phloat y = ((vartype_real *) reg_y)->x;
		phloat r = pow(y, x);
		if (p_isnan(r))
		    /* Should not happen; pow() is supposed to be able
		     * to raise negative numbers to integer exponents
		     */
		    return ERR_INVALID_DATA;
		if ((inf = p_isinf(r)) != 0) {
		    if (!flags.f.range_error_ignore)
			return ERR_OUT_OF_RANGE;
		    r = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
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
		phloat rre, rim, yre, yim;
		int4 ex;
		if (x < -2147483647.0 || x > 2147483647.0)
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
		ex = to_int4(x);
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
		    phloat h = hypot(yre, yim);
		    yre = yre / h / h;
		    yim = (-yim) / h / h;
		    ex = -ex;
		}
		while (1) {
		    phloat tmp;
		    if ((ex & 1) != 0) {
			tmp = rre * yre - rim * yim;
			rim = rre * yim + rim * yre;
			rre = tmp;
			/* TODO: can one component be infinite while
			 * the other is zero? If yes, how do we handle
			 * that?
			 */
			if (p_isinf(rre) && p_isinf(rim))
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
		if ((inf = p_isinf(rre)) != 0) {
		    if (!flags.f.range_error_ignore)
			return ERR_OUT_OF_RANGE;
		    rre = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
		}
		if ((inf = p_isinf(rim)) != 0) {
		    if (!flags.f.range_error_ignore)
			return ERR_OUT_OF_RANGE;
		    rim = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
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
	    phloat y = ((vartype_real *) reg_y)->x;
	    phloat r;
	    if (y < 0) {
		if (flags.f.real_result_only)
		    return ERR_INVALID_DATA;
		yr = -y;
		yphi = PI;
		goto complex_pow_real_2;
	    }
	    r = pow(y, x);
	    inf = p_isinf(r);
	    if (inf != 0) {
		if (!flags.f.range_error_ignore)
		    return ERR_OUT_OF_RANGE;
		r = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
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
	    complex_pow_real_1:
	    {
		phloat yre = ((vartype_complex *) reg_y)->re;
		phloat yim = ((vartype_complex *) reg_y)->im;
		yr = hypot(yre, yim);
		yphi = atan2(yim, yre);
	    }
	    complex_pow_real_2:
	    yr = pow(yr, x);
	    yphi *= x;
	    phloat rre, rim;
	    if ((inf = p_isinf(yr)) != 0) {
		if (!flags.f.range_error_ignore)
		    return ERR_OUT_OF_RANGE;
		else {
		    phloat re, im;
		    sincos(yphi, &im, &re);
		    rre = re == 0 ? 0 :
			    re < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
		    rim = im == 0 ? 0 :
			    im < 0 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
		}
	    } else {
		phloat re, im;
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
	phloat xre = ((vartype_complex *) reg_x)->re;
	phloat xim = ((vartype_complex *) reg_x)->im;
	phloat yre, yim;
	phloat lre, lim;
	phloat tmp;
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

//////////////////////////////////////////////////////////////////////////
/////     Accelerometer, Location Services, and Compass support      /////
///// iPhone only, for now. In order to compile this, the shell must /////
/////   provide shell_get_acceleration() etc., and those are only    /////
/////             implemented in the iPhone shell so far.            /////
//////////////////////////////////////////////////////////////////////////

#ifdef IPHONE
int docmd_accel(arg_struct *arg) {
    if (!core_settings.enable_ext_accel)
	return ERR_NONEXISTENT;
    double x, y, z;
    int err = shell_get_acceleration(&x, &y, &z);
    if (err == 0)
	return ERR_NONEXISTENT;
    vartype *new_x = new_real(x);
    vartype *new_y = new_real(y);
    vartype *new_z = new_real(z);
    if (new_x == NULL || new_y == NULL || new_z == NULL) {
	free_vartype(new_x);
	free_vartype(new_y);
	free_vartype(new_z);
	return ERR_INSUFFICIENT_MEMORY;
    }
    free_vartype(reg_t);
    free_vartype(reg_z);
    if (flags.f.stack_lift_disable) {
	free_vartype(reg_x);
	reg_t = reg_y;
    } else {
	free_vartype(reg_y);
	reg_t = reg_x;
    }
    reg_z = new_z;
    reg_y = new_y;
    reg_x = new_x;
    if (flags.f.trace_print && flags.f.printer_exists)
	docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_locat(arg_struct *arg) {
    if (!core_settings.enable_ext_locat)
	return ERR_NONEXISTENT;
    double lat, lon, lat_lon_acc, elev, elev_acc;
    int err = shell_get_location(&lat, &lon, &lat_lon_acc, &elev, &elev_acc);
    if (err == 0)
	return ERR_NONEXISTENT;
    vartype *new_x = new_real(lat);
    vartype *new_y = new_real(lon);
    vartype *new_z = new_real(elev);
    vartype *new_t = new_realmatrix(1, 2);
    if (new_x == NULL || new_y == NULL || new_z == NULL || new_t == NULL) {
	free_vartype(new_x);
	free_vartype(new_y);
	free_vartype(new_z);
	free_vartype(new_t);
	return ERR_INSUFFICIENT_MEMORY;
    }
    vartype_realmatrix *rm = (vartype_realmatrix *) new_t;
    rm->array->data[0] = lat_lon_acc;
    rm->array->data[1] = elev_acc;
    free_vartype(reg_t);
    free_vartype(reg_z);
    free_vartype(reg_y);
    free_vartype(reg_x);
    reg_t = new_t;
    reg_z = new_z;
    reg_y = new_y;
    reg_x = new_x;
    if (flags.f.trace_print && flags.f.printer_exists)
	docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_heading(arg_struct *arg) {
    if (!core_settings.enable_ext_heading)
	return ERR_NONEXISTENT;
    double mag_heading, true_heading, acc, x, y, z;
    int err = shell_get_heading(&mag_heading, &true_heading, &acc, &x, &y, &z);
    if (err == 0)
	return ERR_NONEXISTENT;
    vartype *new_x = new_real(mag_heading);
    vartype *new_y = new_real(true_heading);
    vartype *new_z = new_real(acc);
    vartype *new_t = new_realmatrix(1, 3);
    if (new_x == NULL || new_y == NULL || new_z == NULL || new_t == NULL) {
	free_vartype(new_x);
	free_vartype(new_y);
	free_vartype(new_z);
	free_vartype(new_t);
	return ERR_INSUFFICIENT_MEMORY;
    }
    vartype_realmatrix *rm = (vartype_realmatrix *) new_t;
    rm->array->data[0] = x;
    rm->array->data[1] = y;
    rm->array->data[2] = z;
    free_vartype(reg_t);
    free_vartype(reg_z);
    free_vartype(reg_y);
    free_vartype(reg_x);
    reg_t = new_t;
    reg_z = new_z;
    reg_y = new_y;
    reg_x = new_x;
    if (flags.f.trace_print && flags.f.printer_exists)
	docmd_prx(NULL);
    return ERR_NONE;
}
#endif

/////////////////////////////////////////////////
///// HP-41 Time Module & CX Time emulation /////
/////////////////////////////////////////////////

static int date2comps(phloat x, int4 *yy, int4 *mm, int4 *dd) COMMANDS6_SECT;
static int date2comps(phloat x, int4 *yy, int4 *mm, int4 *dd) {
    int4 m = to_int4(floor(x));
#ifdef BCD_MATH
    int4 d = to_int4(floor((x - m) * 100));
    int4 y = to_int4(x * 1000000) % 10000;
#else
    int4 r = (int4) floor((x - m) * 100000000 + 0.5);
    r /= 100;
    int4 d = r / 10000;
    int4 y = r % 10000;
#endif

    if (mode_time_dmy) {
	int4 t = m;
	m = d;
	d = t;
    }

    if (y < 1582 || y > 4320 || m < 1 || m > 12 || d < 1 || d > 31)
	return ERR_INVALID_DATA;
    if ((m == 4 || m == 6 || m == 9 || m == 11) && d == 31)
	return ERR_INVALID_DATA;
    if (m == 2 && d > ((y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 29 : 28))
	return ERR_INVALID_DATA;
    if (y == 1582 && (m < 10 || m == 10 && d < 15)
	    || y == 4320 && (m > 9 || m == 9 && d > 10))
	return ERR_INVALID_DATA;

    *yy = y;
    *mm = m;
    *dd = d;
    return ERR_NONE;
}

static phloat comps2date(int4 y, int4 m, int4 d) COMMANDS6_SECT;
static phloat comps2date(int4 y, int4 m, int4 d) {
    if (mode_time_dmy) {
	int4 t = m;
	m = d;
	d = t;
    }
    return phloat(m * 1000000 + d * 10000 + y) / 1000000;
}

/* Gregorian Date <-> Day Number conversion functions
 * Algorithm due to Henry F. Fliegel and Thomas C. Van Flandern,
 * Communications of the ACM, Vol. 11, No. 10 (October, 1968).
 */
static int greg2jd(int4 y, int4 m, int4 d, int4 *jd) COMMANDS6_SECT;
static int greg2jd(int4 y, int4 m, int4 d, int4 *jd) {
    *jd = ( 1461 * ( y + 4800 + ( m - 14 ) / 12 ) ) / 4 +
	  ( 367 * ( m - 2 - 12 * ( ( m - 14 ) / 12 ) ) ) / 12 -
	  ( 3 * ( ( y + 4900 + ( m - 14 ) / 12 ) / 100 ) ) / 4 +
	  d - 32075;
    return ERR_NONE;
}

static int jd2greg(int4 jd, int4 *y, int4 *m, int4 *d) COMMANDS6_SECT;
static int jd2greg(int4 jd, int4 *y, int4 *m, int4 *d) {
    if (jd < 2299161 || jd > 3299160)
	return ERR_OUT_OF_RANGE;
    int4 l = jd + 68569;
    int4 n = ( 4 * l ) / 146097;
    l = l - ( 146097 * n + 3 ) / 4;
    int4 i = ( 4000 * ( l + 1 ) ) / 1461001;
    l = l - ( 1461 * i ) / 4 + 31;
    int4 j = ( 80 * l ) / 2447;
    *d = l - ( 2447 * j ) / 80;
    l = j / 11;
    *m = j + 2 - ( 12 * l );
    *y = 100 * ( n - 49 ) + i + l;
    return ERR_NONE;
}


int docmd_adate(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

int docmd_almcat(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

int docmd_almnow(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

int docmd_atime(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

int docmd_atime24(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

int docmd_clk12(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    mode_time_clk24 = false;
    return ERR_NONE;
}

int docmd_clk24(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    mode_time_clk24 = true;
    return ERR_NONE;
}

int docmd_clkt(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    mode_time_clktd = false;
    return ERR_NONE;
}

int docmd_clktd(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    mode_time_clktd = true;
    return ERR_NONE;
}

int docmd_clock(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

int docmd_correct(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

static char weekdaynames[] = "SUNMONTUEWEDTHUFRISAT";

int docmd_date(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    uint4 date;
    int weekday;
    shell_get_time_date(NULL, &date, &weekday);
    int y = date / 10000;
    int m = date / 100 % 100;
    int d = date % 100;
    if (mode_time_dmy)
	date = y + m * 10000 + d * 1000000;
    else
	date = y + m * 1000000 + d * 10000;
    vartype *new_x = new_real((int4) date);
    if (new_x == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    ((vartype_real *) new_x)->x /= 1000000;
    recall_result(new_x);
    if (!program_running()) {
	/* Note: I'm not completely faithful to the HP-41 here. It formats the
	 * date as "14.03.2010 SUN" in DMY mode, and as "03/14/2010:SU" in MDY
	 * mode. I mimic the former, but the latter I changed to
	 * "03/14/2010 SUN"; the MDY display format used on the HP-41 is the
	 * way it is because that was all they could fit in its 12-character
	 * display. (Note that the periods in the DMY format and the colon in
	 * the MDY format don't take up a character position on the HP-41.)
	 */
	char buf[22];
	int bufptr = 0;
	int n = mode_time_dmy ? d : m;
	if (n < 10)
	    char2buf(buf, 22, &bufptr, '0');
	bufptr += int2string(n, buf + bufptr, 22 - bufptr);
	char2buf(buf, 22, &bufptr, mode_time_dmy ? ':' : '/');
	n = mode_time_dmy ? m : d;
	if (n < 10)
	    char2buf(buf, 22, &bufptr, '0');
	bufptr += int2string(n, buf + bufptr, 22 - bufptr);
	char2buf(buf, 22, &bufptr, mode_time_dmy ? ':' : '/');
	bufptr += int2string(y, buf + bufptr, 22 - bufptr);
	char2buf(buf, 22, &bufptr, ' ');
	string2buf(buf, 22, &bufptr, weekdaynames + weekday * 3, 3);
	clear_row(0);
	draw_string(0, 0, buf, bufptr);
	flush_display();
	flags.f.message = 1;
	flags.f.two_line_message = 0;
    }
    /* TODO: Trace-mode printing. What should I print, the contents of X,
     * or, when not in a running program, the nicely formatted date?
     */
    return ERR_NONE;
}

int docmd_date_plus(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    // TODO: Accept real matrices as well?
    if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_x->type != TYPE_REAL)
	return ERR_INVALID_TYPE;
    if (reg_y->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_y->type != TYPE_REAL)
	return ERR_INVALID_TYPE;

    phloat date = ((vartype_real *) reg_y)->x;
    if (date < 0 || date > 100)
	return ERR_INVALID_DATA;
    phloat days = ((vartype_real *) reg_x)->x;
    if (days < -1000000 || days > 1000000)
	return ERR_OUT_OF_RANGE;

    int4 y, m, d, jd;
    int err = date2comps(date, &y, &m, &d);
    if (err != ERR_NONE)
	return err;
    err = greg2jd(y, m, d, &jd);
    if (err != ERR_NONE)
	return err;
    jd += to_int4(floor(days));
    err = jd2greg(jd, &y, &m, &d);
    if (err != ERR_NONE)
	return err;
    date = comps2date(y, m, d);

    vartype *new_x = new_real(date);
    if (new_x == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    binary_result(new_x);
    return ERR_NONE;
}

int docmd_ddays(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    // TODO: Accept real matrices as well?
    if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_x->type != TYPE_REAL)
	return ERR_INVALID_TYPE;
    if (reg_y->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_y->type != TYPE_REAL)
	return ERR_INVALID_TYPE;

    phloat date1 = ((vartype_real *) reg_y)->x;
    if (date1 < 0 || date1 > 100)
	return ERR_INVALID_DATA;
    phloat date2 = ((vartype_real *) reg_x)->x;
    if (date2 < 0 || date2 > 100)
	return ERR_INVALID_DATA;
    int4 y, m, d, jd1, jd2;
    int err = date2comps(date1, &y, &m, &d);
    if (err != ERR_NONE)
	return err;
    err = greg2jd(y, m, d, &jd1);
    if (err != ERR_NONE)
	return err;
    err = date2comps(date2, &y, &m, &d);
    if (err != ERR_NONE)
	return err;
    err = greg2jd(y, m, d, &jd2);
    if (err != ERR_NONE)
	return err;

    vartype *new_x = new_real(jd2 - jd1);
    if (new_x == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    binary_result(new_x);
    return ERR_NONE;
}

int docmd_dmy(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    mode_time_dmy = true;
    return ERR_NONE;
}

int docmd_dow(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    // TODO: Accept real matrices as well?
    if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_x->type != TYPE_REAL)
	return ERR_INVALID_TYPE;

    phloat x = ((vartype_real *) reg_x)->x;
    if (x < 0 || x > 100)
	return ERR_INVALID_DATA;

    int4 y, m, d, jd;
    int err = date2comps(x, &y, &m, &d);
    if (err != ERR_NONE)
	return err;
    err = greg2jd(y, m, d, &jd);
    if (err != ERR_NONE)
	return err;
    jd = (jd + 1) % 7;

    vartype *new_x = new_real(jd);
    if (new_x == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    unary_result(new_x);

    if (!program_running()) {
	clear_row(0);
	draw_string(0, 0, weekdaynames + jd * 3, 3);
	flush_display();
	flags.f.message = 1;
	flags.f.two_line_message = 0;
    }

    return ERR_NONE;
}

int docmd_mdy(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    mode_time_dmy = false;
    return ERR_NONE;
}

int docmd_rclaf(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

int docmd_rclsw(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

int docmd_runsw(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

int docmd_setaf(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

int docmd_setdate(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

int docmd_setime(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

int docmd_setsw(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

int docmd_stopsw(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

int docmd_sw(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

int docmd_t_plus_x(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

int docmd_time(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    uint4 time;
    shell_get_time_date(&time, NULL, NULL);
    vartype *new_x = new_real((int4) time);
    if (new_x == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    ((vartype_real *) new_x)->x /= 1000000;
    recall_result(new_x);
    if (!program_running()) {
	int h = time / 1000000;
	bool am;
	if (!mode_time_clk24) {
	    am = h < 12;
	    h = h % 12;
	    if (h == 0)
		h = 12;
	}
	int m = time / 10000 % 100;
	int s = time / 100 % 100;
	char buf[22];
	int bufptr = 0;
	if (h < 10)
	    char2buf(buf, 22, &bufptr, ' ');
	bufptr += int2string(h, buf + bufptr, 22 - bufptr);
	char2buf(buf, 22, &bufptr, ':');
	if (m < 10)
	    char2buf(buf, 22, &bufptr, '0');
	bufptr += int2string(m, buf + bufptr, 22 - bufptr);
	char2buf(buf, 22, &bufptr, ':');
	if (s < 10)
	    char2buf(buf, 22, &bufptr, '0');
	bufptr += int2string(s, buf + bufptr, 22 - bufptr);
	if (!mode_time_clk24) {
	    char2buf(buf, 22, &bufptr, ' ');
	    char2buf(buf, 22, &bufptr, am ? 'A' : 'P');
	    char2buf(buf, 22, &bufptr, 'M');
	}
	clear_row(0);
	draw_string(0, 0, buf, bufptr);
	flush_display();
	flags.f.message = 1;
	flags.f.two_line_message = 0;
    }
    /* TODO: Trace-mode printing. What should I print, the contents of X,
     * or, when not in a running program, the nicely formatted time?
     */
    return ERR_NONE;
}

int docmd_xyzalm(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

int docmd_clalma(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

int docmd_clalmx(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

int docmd_clralms(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

int docmd_rclalm(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}

int docmd_swpt(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
	return ERR_NONEXISTENT;
    return ERR_NOT_YET_IMPLEMENTED;
}
