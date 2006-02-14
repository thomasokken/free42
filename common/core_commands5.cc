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

#include "core_commands2.h"
#include "core_commands5.h"
#include "core_decimal.h"
#include "core_display.h"
#include "core_helpers.h"
#include "core_main.h"
#include "core_variables.h"

int appmenu_exitcallback_2(int menuid) {
    if (menuid == MENU_BASE
	    || menuid == MENU_BASE_A_THRU_F
	    || menuid == MENU_BASE_LOGIC) {
	mode_appmenu = menuid;
	set_appmenu_exitcallback(2);
    } else {
	set_base(10);
	mode_appmenu = menuid;
	baseapp = 0;
    }
    return ERR_NONE;
}

static int base_helper(int base) COMMANDS5_SECT;
static int base_helper(int base) {
    if (program_running()) {
	int err = set_menu_return_err(MENULEVEL_APP, MENU_BASE);
	if (err != ERR_NONE)
	    return err;
	set_appmenu_exitcallback(2);
	baseapp = 1;
    }
    set_base(base);
    return ERR_NONE;
}

int docmd_binm(arg_struct *arg) {
    return base_helper(2);
}

int docmd_octm(arg_struct *arg) {
    return base_helper(8);
}

int docmd_decm(arg_struct *arg) {
    return base_helper(10);
}

int docmd_hexm(arg_struct *arg) {
    return base_helper(16);
}

int docmd_linf(arg_struct *arg) {
    flags.f.lin_fit = 1;
    flags.f.log_fit = 0;
    flags.f.exp_fit = 0;
    flags.f.pwr_fit = 0;
    return ERR_NONE;
}

int docmd_logf(arg_struct *arg) {
    flags.f.lin_fit = 0;
    flags.f.log_fit = 1;
    flags.f.exp_fit = 0;
    flags.f.pwr_fit = 0;
    return ERR_NONE;
}

int docmd_expf(arg_struct *arg) {
    flags.f.lin_fit = 0;
    flags.f.log_fit = 0;
    flags.f.exp_fit = 1;
    flags.f.pwr_fit = 0;
    return ERR_NONE;
}

int docmd_pwrf(arg_struct *arg) {
    flags.f.lin_fit = 0;
    flags.f.log_fit = 0;
    flags.f.exp_fit = 0;
    flags.f.pwr_fit = 1;
    return ERR_NONE;
}

int docmd_allsigma(arg_struct *arg) {
    flags.f.all_sigma = 1;
    return ERR_NONE;
}

int docmd_and(arg_struct *arg) {
    int8 x, y;
    int err;
    vartype *v;
    if ((err = get_base_param(reg_x, &x)) != ERR_NONE)
	return err;
    if ((err = get_base_param(reg_y, &y)) != ERR_NONE)
	return err;
    v = new_real((phloat) (x & y));
    if (v == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    binary_result(v);
    return ERR_NONE;
}

int docmd_baseadd(arg_struct *arg) {
    int8 x, y, res;
    int err;
    vartype *v;
    if ((err = get_base_param(reg_x, &x)) != ERR_NONE)
	return err;
    if ((err = get_base_param(reg_y, &y)) != ERR_NONE)
	return err;
    res = x + y;
    if ((err = base_range_check(&res)) != ERR_NONE)
	return err;
    v = new_real((phloat) res);
    if (v == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    binary_result(v);
    return ERR_NONE;
}

int docmd_basesub(arg_struct *arg) {
    int8 x, y, res;
    int err;
    vartype *v;
    if ((err = get_base_param(reg_x, &x)) != ERR_NONE)
	return err;
    if ((err = get_base_param(reg_y, &y)) != ERR_NONE)
	return err;
    res = y - x;
    if ((err = base_range_check(&res)) != ERR_NONE)
	return err;
    v = new_real((phloat) res);
    if (v == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    binary_result(v);
    return ERR_NONE;
}

int docmd_basemul(arg_struct *arg) {
    int8 x, y;
    double res;
    int err;
    vartype *v;
    if ((err = get_base_param(reg_x, &x)) != ERR_NONE)
	return err;
    if ((err = get_base_param(reg_y, &y)) != ERR_NONE)
	return err;
    /* I compute the result in 'double' arithmetic, because doing it
     * in int8 arithmetic could cause me to overlook an out-of-range
     * condition (e.g. 2^32 * 2^32).
     */
    res = ((double) x) * ((double) y);
    if (res < -34359738368.0) {
	if (flags.f.range_error_ignore)
	    res = -34359738368.0;
	else
	    return ERR_OUT_OF_RANGE;
    } else if (res > 34359738367.0) {
	if (flags.f.range_error_ignore)
	    res = 34359738367.0;
	else
	    return ERR_OUT_OF_RANGE;
    }
    v = new_real(res);
    if (v == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    binary_result(v);
    return ERR_NONE;
}

int docmd_basediv(arg_struct *arg) {
    int8 x, y, res;
    int err;
    vartype *v;
    if ((err = get_base_param(reg_x, &x)) != ERR_NONE)
	return err;
    if ((err = get_base_param(reg_y, &y)) != ERR_NONE)
	return err;
    if (y == 0)
	return ERR_DIVIDE_BY_0;
    res = y / x;
    if ((err = base_range_check(&res)) != ERR_NONE)
	return err;
    v = new_real((phloat) res);
    if (v == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    binary_result(v);
    return ERR_NONE;
}

int docmd_basechs(arg_struct *arg) {
    int8 x;
    int err;
    if ((err = get_base_param(reg_x, &x)) != ERR_NONE)
	return err;
    if (x == LL(-34359738368)) {
	if (flags.f.range_error_ignore)
	    x = LL(34359738367);
	else
	    return ERR_OUT_OF_RANGE;
    } else
	x = -x;
    ((vartype_real *) reg_x)->x = (phloat) x;
    return ERR_NONE;
}

static struct {
    // PHLOAT_TODO: convert this on bin/dec mode switch
    phloat x;
    phloat x2;
    phloat y;
    phloat y2;
    phloat xy;
    phloat n;
    phloat lnx;
    phloat lnx2;
    phloat lny;
    phloat lny2;
    phloat lnxlny;
    phloat xlny;
    phloat ylnx;
} sum;

static int get_summation() COMMANDS5_SECT;
static int get_summation() {
    /* Check if summation registers are OK */
    int4 first = mode_sigma_reg;
    int4 last = first + (flags.f.all_sigma ? 13 : 6);
    int4 size, i;
    vartype *regs = recall_var("REGS", 4);
    vartype_realmatrix *r;
    phloat *sigmaregs;
    if (regs == NULL)
	return ERR_SIZE_ERROR;
    if (regs->type != TYPE_REALMATRIX)
	return ERR_INVALID_TYPE;
    r = (vartype_realmatrix *) regs;
    size = r->rows * r->columns;
    if (last > size)
	return ERR_SIZE_ERROR;
    for (i = first; i < last; i++)
	if (r->array->is_string[i])
	    return ERR_ALPHA_DATA_IS_INVALID;
    sigmaregs = r->array->data + first;
    sum.x = sigmaregs[0];
    sum.x2 = sigmaregs[1];
    sum.y = sigmaregs[2];
    sum.y2 = sigmaregs[3];
    sum.xy = sigmaregs[4];
    sum.n = sigmaregs[5];
    if (flags.f.all_sigma) {
	sum.lnx = sigmaregs[6];
	sum.lnx2 = sigmaregs[7];
	sum.lny = sigmaregs[8];
	sum.lny2 = sigmaregs[9];
	sum.lnxlny = sigmaregs[10];
	sum.xlny = sigmaregs[11];
	sum.ylnx = sigmaregs[12];
    }
    return ERR_NONE;
}
    
static struct {
    // PHLOAT_TODO: convert this on bin/dec mode switch
    // Update: really? Is this persistent?
    phloat x;
    phloat x2;
    phloat y;
    phloat y2;
    phloat xy;
    phloat n;
    int ln_before;
    int exp_after;
    int valid;
    phloat slope;
    phloat yint;
} model;

#define MODEL_NONE -1
#define MODEL_LIN 0
#define MODEL_LOG 1
#define MODEL_EXP 2
#define MODEL_PWR 3

static int get_model_summation(int modl) COMMANDS5_SECT;
static int get_model_summation(int modl) {
    int err = get_summation();
    if (err != ERR_NONE)
	return err;
    switch (modl) {
	case MODEL_LIN:
	    model.xy = sum.xy;
	    model.ln_before = 0;
	    model.exp_after = 0;
	    break;
	case MODEL_LOG:
	    if (flags.f.log_fit_invalid)
		return ERR_INVALID_FORECAST_MODEL;
	    model.xy = sum.ylnx;
	    model.ln_before = 1;
	    model.exp_after = 0;
	    break;
	case MODEL_EXP:
	    if (flags.f.exp_fit_invalid)
		return ERR_INVALID_FORECAST_MODEL;
	    model.xy = sum.xlny;
	    model.ln_before = 0;
	    model.exp_after = 1;
	    break;
	case MODEL_PWR:
	    if (flags.f.pwr_fit_invalid)
		return ERR_INVALID_FORECAST_MODEL;
	    model.xy = sum.lnxlny;
	    model.ln_before = 1;
	    model.exp_after = 1;
	    break;
	default:
	    return ERR_INVALID_FORECAST_MODEL;
    }
    if (model.ln_before) {
	model.x = sum.lnx;
	model.x2 = sum.lnx2;
    } else {
	model.x = sum.x;
	model.x2 = sum.x2;
    }
    if (model.exp_after) {
	model.y = sum.lny;
	model.y2 = sum.lny2;
    } else {
	model.y = sum.y;
	model.y2 = sum.y2;
    }
    model.n = sum.n;
    return ERR_NONE;
}

static int corr_helper(int modl, phloat *r) COMMANDS5_SECT;
static int corr_helper(int modl, phloat *r) {
    phloat cov, varx, vary, v, tr;
    int err = get_model_summation(modl);
    if (err != ERR_NONE)
	return err;
    if (model.n == 0 || model.n == 1)
	return ERR_STAT_MATH_ERROR;
    cov = model.xy - model.x * model.y / model.n;
    varx = model.x2 - model.x * model.x / model.n;
    vary = model.y2 - model.y * model.y / model.n;
    if (varx <= 0 || vary <= 0)
	return ERR_STAT_MATH_ERROR;
    v = varx * vary;
    if (v == 0)
	return ERR_STAT_MATH_ERROR;
    tr = cov / sqrt(v);
    if (tr < -1)
	tr = -1;
    else if (tr > 1)
	tr = 1;
    *r = tr;
    return ERR_NONE;
}

static int slope_yint_helper() COMMANDS5_SECT;
static int slope_yint_helper() {
    /* The caller should have made sure that 'model' is up to date
     * by calling get_model_summation() first.
     */
    phloat cov, varx, meanx, meany;
    int inf;
    if (model.n == 0 || model.n == 1)
	return ERR_STAT_MATH_ERROR;
    cov = model.xy - model.x * model.y / model.n;
    varx = model.x2 - model.x * model.x / model.n;
    if (varx == 0)
	return ERR_STAT_MATH_ERROR;
    model.slope = cov / varx;
    if ((inf = p_isinf(model.slope)) != 0)
	model.slope = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
    meanx = model.x / model.n;
    meany = model.y / model.n;
    model.yint = meany - model.slope * meanx;
    if ((inf = p_isinf(model.yint)) != 0)
	model.yint = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
    return ERR_NONE;
}

static int get_model() COMMANDS5_SECT;
static int get_model() {
    if (flags.f.lin_fit)
	return MODEL_LIN;
    else if (flags.f.log_fit)
	return MODEL_LOG;
    else if (flags.f.exp_fit)
	return MODEL_EXP;
    else if (flags.f.pwr_fit)
	return MODEL_PWR;
    else
	return MODEL_NONE;
}

int docmd_best(arg_struct *arg) {
    int best = MODEL_NONE;
    phloat bestr = 0;
    int firsterr = ERR_NONE;
    int i;
    for (i = MODEL_LIN; i <= MODEL_PWR; i++) {
	phloat r;
	int err = corr_helper(i, &r);
	if (err == ERR_NONE) {
	    if (r < 0)
		r = -r;
	    if (r > bestr) {
		best = i;
		bestr = r;
	    }
	} else {
	    if (firsterr == ERR_NONE)
		firsterr = err;
	}
    }
    if (best == MODEL_NONE)
	best = MODEL_LIN;
    else
	firsterr = ERR_NONE;
    flags.f.lin_fit = best == MODEL_LIN;
    flags.f.log_fit = best == MODEL_LOG;
    flags.f.exp_fit = best == MODEL_EXP;
    flags.f.pwr_fit = best == MODEL_PWR;
    return firsterr;
}

int docmd_bit_t(arg_struct *arg) {
    int8 x, y;
    int err;
    if ((err = get_base_param(reg_x, &x)) != ERR_NONE)
	return err;
    if ((err = get_base_param(reg_y, &y)) != ERR_NONE)
	return err;
    if (x < 0 || x > 35)
	return ERR_INVALID_DATA;
    return (y & (1L << x)) != 0 ? ERR_YES : ERR_NO;
}

int docmd_corr(arg_struct *arg) {
    phloat r;
    int err;
    vartype *rv;
    err = corr_helper(get_model(), &r);
    if (err != ERR_NONE)
	return err;
    rv = new_real(r);
    if (rv == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    recall_result(rv);
    return ERR_NONE;
}

static int mappable_fcstx(phloat x, phloat *y) COMMANDS5_SECT;
static int mappable_fcstx(phloat x, phloat *y) {
    int inf;
    if (model.exp_after) {
	if (x <= 0)
	    return ERR_INVALID_FORECAST_MODEL;
	x = log(x);
    }
    if (model.slope == 0)
	return ERR_STAT_MATH_ERROR;
    x = (x - model.yint) / model.slope;
    if (model.ln_before)
	x = exp(x);
    if ((inf = p_isinf(x)) != 0)
	x = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
    *y = x;
    return ERR_NONE;
}

int docmd_fcstx(arg_struct *arg) {
    int err = get_model_summation(get_model());
    vartype *v;
    if (err != ERR_NONE)
	return err;
    err = slope_yint_helper();
    if (err != ERR_NONE)
	return err;
    if (reg_x->type == TYPE_REAL || reg_x->type == TYPE_REALMATRIX) {
	err = map_unary(reg_x, &v, mappable_fcstx, NULL);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    } else if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else
	return ERR_INVALID_TYPE;
}

static int mappable_fcsty(phloat x, phloat *y) COMMANDS5_SECT;
static int mappable_fcsty(phloat x, phloat *y) {
    int inf;
    if (model.ln_before) {
	if (x <= 0)
	    return ERR_INVALID_FORECAST_MODEL;
	x = log(x);
    }
    x = x * model.slope + model.yint;
    if (model.exp_after)
	x = exp(x);
    if ((inf = p_isinf(x)) != 0)
	x = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
    *y = x;
    return ERR_NONE;
}

int docmd_fcsty(arg_struct *arg) {
    int err = get_model_summation(get_model());
    vartype *v;
    if (err != ERR_NONE)
	return err;
    err = slope_yint_helper();
    if (err != ERR_NONE)
	return err;
    if (reg_x->type == TYPE_REAL || reg_x->type == TYPE_REALMATRIX) {
	err = map_unary(reg_x, &v, mappable_fcsty, NULL);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    } else if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else
	return ERR_INVALID_TYPE;
}

int docmd_mean(arg_struct *arg) {
    phloat m;
    int inf;
    vartype *mx, *my;
    int err = get_summation();
    if (err != ERR_NONE)
	return err;
    if (sum.n == 0)
	return ERR_STAT_MATH_ERROR;
    m = sum.x / sum.n;
    if ((inf = p_isinf(m)) != 0)
	m = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
    mx = new_real(m);
    if (mx == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    m = sum.y / sum.n;
    if ((inf = p_isinf(m)) != 0)
	m = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
    my = new_real(m);
    if (my == NULL) {
	free_vartype(mx);
	return ERR_INSUFFICIENT_MEMORY;
    }
    free_vartype(reg_y);
    reg_y = my;
    free_vartype(reg_lastx);
    reg_lastx = reg_x;
    reg_x = mx;
    if (flags.f.trace_print && flags.f.printer_enable)
	docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_sdev(arg_struct *arg) {
    int err = get_summation();
    phloat var;
    vartype *sx, *sy;
    if (err != ERR_NONE)
	return err;
    if (sum.n == 0 || sum.n == 1)
	return ERR_STAT_MATH_ERROR;
    var = (sum.x2 - (sum.x * sum.x / sum.n)) / (sum.n - 1);
    if (var < 0)
	return ERR_STAT_MATH_ERROR;
    if (p_isinf(var))
	sx = new_real(POS_HUGE_PHLOAT);
    else
	sx = new_real(sqrt(var));
    if (sx == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    var = (sum.y2 - (sum.y * sum.y / sum.n)) / (sum.n - 1);
    if (var < 0)
	return ERR_STAT_MATH_ERROR;
    if (p_isinf(var))
	sy = new_real(POS_HUGE_PHLOAT);
    else
	sy = new_real(sqrt(var));
    if (sy == NULL) {
	free_vartype(sx);
	return ERR_INSUFFICIENT_MEMORY;
    }
    free_vartype(reg_y);
    reg_y = sy;
    free_vartype(reg_lastx);
    reg_lastx = reg_x;
    reg_x = sx;
    if (flags.f.trace_print && flags.f.printer_enable)
	docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_slope(arg_struct *arg) {
    int err = get_model_summation(get_model());
    vartype *v;
    if (err != ERR_NONE)
	return err;
    err = slope_yint_helper();
    if (err != ERR_NONE)
	return err;
    v = new_real(model.slope);
    if (v == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    recall_result(v);
    return ERR_NONE;
}

int docmd_sum(arg_struct *arg) {
    int err = get_summation();
    vartype *sx, *sy;
    if (err != ERR_NONE)
	return err;
    sx = new_real(sum.x);
    if (sx == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    sy = new_real(sum.y);
    if (sy == NULL) {
	free_vartype(sx);
	return ERR_INSUFFICIENT_MEMORY;
    }
    free_vartype(reg_lastx);
    free_vartype(reg_y);
    reg_y = sy;
    reg_lastx = reg_x;
    reg_x = sx;
    if (flags.f.trace_print && flags.f.printer_enable)
	docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_wmean(arg_struct *arg) {
    phloat wm;
    int inf;
    vartype *v;
    int err = get_summation();
    if (err != ERR_NONE)
	return err;
    if (sum.y == 0)
	return ERR_STAT_MATH_ERROR;
    wm = sum.xy / sum.y;
    if ((inf = p_isinf(wm)) != 0)
	wm = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
    v = new_real(wm);
    if (v == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    recall_result(v);
    return ERR_NONE;
}

int docmd_yint(arg_struct *arg) {
    int err = get_model_summation(get_model());
    phloat yint;
    vartype *v;
    if (err != ERR_NONE)
	return err;
    err = slope_yint_helper();
    if (err != ERR_NONE)
	return err;
    if (model.exp_after) {
	yint = exp(model.yint);
	if (p_isinf(yint) != 0)
	    yint = POS_HUGE_PHLOAT;
    } else
	yint = model.yint;
    v = new_real(yint);
    if (v == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    recall_result(v);
    return ERR_NONE;
}
