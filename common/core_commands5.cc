/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2025  Thomas Okken
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

#include <string.h>
#include <stdlib.h>
#include "bid_conf.h"
#include "bid_functions.h"

#include "core_commands2.h"
#include "core_commands5.h"
#include "core_display.h"
#include "core_helpers.h"
#include "core_main.h"
#include "core_math1.h"
#include "core_sto_rcl.h"
#include "core_variables.h"

/********************************************************/
/* Implementations of HP-42S built-in functions, part 5 */
/********************************************************/

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

static struct sum_struct {
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
        if (r->array->is_string[i] != 0)
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

static struct model_struct {
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
    return recall_result(rv);
}

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
    if (stack[sp]->type == TYPE_REAL || stack[sp]->type == TYPE_REALMATRIX) {
        err = map_unary(stack[sp], &v, mappable_fcstx, NULL);
        if (err == ERR_NONE)
            unary_result(v);
        return err;
    } else if (stack[sp]->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;
}

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
    if (stack[sp]->type == TYPE_REAL || stack[sp]->type == TYPE_REALMATRIX) {
        err = map_unary(stack[sp], &v, mappable_fcsty, NULL);
        if (err == ERR_NONE)
            unary_result(v);
        return err;
    } else if (stack[sp]->type == TYPE_STRING)
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
    if (flags.f.big_stack) {
        return recall_two_results(mx, my);
    } else {
        binary_two_results(mx, my);
        return ERR_NONE;
    }
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
    if (flags.f.big_stack) {
        return recall_two_results(sx, sy);
    } else {
        binary_two_results(sx, sy);
        return ERR_NONE;
    }
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
    return recall_result(v);
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
    if (flags.f.big_stack) {
        return recall_two_results(sx, sy);
    } else {
        binary_two_results(sx, sy);
        return ERR_NONE;
    }
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
    return recall_result(v);
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
    return recall_result(v);
}

int docmd_integ(arg_struct *arg) {
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
    if (!program_running())
        clear_all_rtns();
    string_copy(reg_alpha, &reg_alpha_length, arg->val.text, arg->length);
    return start_integ(arg->val.text, arg->length);
}

int docmd_linsigma(arg_struct *arg) {
    flags.f.all_sigma = 0;
    return ERR_NONE;
}

int docmd_pgmslv(arg_struct *arg) {
    int err;
    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    if (arg->type == ARGTYPE_STR) {
        int prgm;
        int4 pc;
        if (!find_global_label(arg, &prgm, &pc))
            return ERR_LABEL_NOT_FOUND;
        set_solve_prgm(arg->val.text, arg->length);
        return ERR_NONE;
    } else
        return ERR_INVALID_TYPE;
}

int docmd_pgmint(arg_struct *arg) {
    int err;
    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    if (arg->type == ARGTYPE_STR) {
        int prgm;
        int4 pc;
        if (!find_global_label(arg, &prgm, &pc))
            return ERR_LABEL_NOT_FOUND;
        set_integ_prgm(arg->val.text, arg->length);
        return ERR_NONE;
    } else
        return ERR_INVALID_TYPE;
}

int appmenu_exitcallback_3(int menuid, bool exitall) {
    if (menuid == MENU_NONE && !exitall) {
        set_menu(MENULEVEL_APP, MENU_CATALOG);
        set_cat_section(CATSECT_PGM_SOLVE);
    } else
        mode_appmenu = menuid;
    return ERR_NONE;
}

int docmd_pgmslvi(arg_struct *arg) {
    if (arg->type == ARGTYPE_STR) {
        int idx;
        if (!find_global_label_index(arg, &idx))
            return ERR_LABEL_NOT_FOUND;
        if (!label_has_mvar(idx))
            return ERR_NO_MENU_VARIABLES;
        set_solve_prgm(arg->val.text, arg->length);
        string_copy(varmenu, &varmenu_length, arg->val.text, arg->length);
        varmenu_row = 0;
        varmenu_role = 1;
        set_menu(MENULEVEL_APP, MENU_VARMENU);
        set_appmenu_exitcallback(3);
        return ERR_NONE;
    } else
        return ERR_INVALID_TYPE;
}

int appmenu_exitcallback_4(int menuid, bool exitall) {
    if (menuid == MENU_NONE && !exitall) {
        set_menu(MENULEVEL_APP, MENU_CATALOG);
        set_cat_section(CATSECT_PGM_INTEG);
    } else
        mode_appmenu = menuid;
    return ERR_NONE;
}

int appmenu_exitcallback_5(int menuid, bool exitall) {
    if (menuid == MENU_NONE && !exitall) {
        get_integ_prgm(varmenu, &varmenu_length);
        varmenu_row = 0;
        varmenu_role = 2;
        set_menu(MENULEVEL_APP, MENU_VARMENU);
        set_appmenu_exitcallback(4);
    } else
        mode_appmenu = menuid;
    return ERR_NONE;
}

int docmd_pgminti(arg_struct *arg) {
    if (arg->type == ARGTYPE_STR) {
        int idx;
        if (!find_global_label_index(arg, &idx))
            return ERR_LABEL_NOT_FOUND;
        if (!label_has_mvar(idx))
            return ERR_NO_MENU_VARIABLES;
        set_integ_prgm(arg->val.text, arg->length);
        string_copy(varmenu, &varmenu_length, arg->val.text, arg->length);
        varmenu_row = 0;
        varmenu_role = 2;
        set_menu(MENULEVEL_APP, MENU_VARMENU);
        set_appmenu_exitcallback(4);
        clear_row(0);
        draw_string(0, 0, "Set Vars; Select \3var", 21);
        flags.f.message = 1;
        flags.f.two_line_message = 0;
        mode_varmenu = true;
        return ERR_NONE;
    } else
        return ERR_INVALID_TYPE;
}

////////////////
///// BASE /////
////////////////

int appmenu_exitcallback_2(int menuid, bool exitall) {
    if (menuid >= MENU_BASE1
            && menuid <= MENU_BASE_DISP
            && !exitall) {
        mode_appmenu = menuid;
        set_appmenu_exitcallback(2);
    } else {
        set_base(10);
        mode_appmenu = menuid;
        baseapp = 0;
    }
    return ERR_NONE;
}

int docmd_a_thru_f(arg_struct *arg) {
    int err = set_menu_return_err(MENULEVEL_APP, MENU_BASE_A_THRU_F, false);
    if (err != ERR_NONE)
        return err;
    set_appmenu_exitcallback(2);
    baseapp = 1;
    set_base(16, true);
    return ERR_NONE;
}

static int base_helper(int base) {
    if (program_running()) {
        int err = set_menu_return_err(MENULEVEL_APP, MENU_BASE1, false);
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

int docmd_and(arg_struct *arg) {
    int8 x, y;
    int err;
    vartype *v;
    if ((err = get_base_param(stack[sp], &x)) != ERR_NONE)
        return err;
    if ((err = get_base_param(stack[sp - 1], &y)) != ERR_NONE)
        return err;
    int8 res = x & y;
    base_range_check(&res, true);
    v = new_real(base2phloat(res));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    return binary_result(v);
}

int docmd_or(arg_struct *arg) {
    int8 x, y;
    int err;
    vartype *v;
    if ((err = get_base_param(stack[sp], &x)) != ERR_NONE)
        return err;
    if ((err = get_base_param(stack[sp - 1], &y)) != ERR_NONE)
        return err;
    int8 res = x | y;
    base_range_check(&res, true);
    v = new_real(base2phloat(res));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    return binary_result(v);
}

int docmd_xor(arg_struct *arg) {
    int8 x, y;
    int err;
    vartype *v;
    if ((err = get_base_param(stack[sp], &x)) != ERR_NONE)
        return err;
    if ((err = get_base_param(stack[sp - 1], &y)) != ERR_NONE)
        return err;
    int8 res = x ^ y;
    base_range_check(&res, true);
    v = new_real(base2phloat(res));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    return binary_result(v);
}

int docmd_not(arg_struct *arg) {
    int8 x;
    int err;
    vartype *v;
    if ((err = get_base_param(stack[sp], &x)) != ERR_NONE)
        return err;
    int8 res = ~x;
    base_range_check(&res, true);
    v = new_real(base2phloat(res));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

int docmd_bit_t(arg_struct *arg) {
    int8 x, y;
    int err;
    if ((err = get_base_param(stack[sp], &x)) != ERR_NONE)
        return err;
    if ((err = get_base_param(stack[sp - 1], &y)) != ERR_NONE)
        return err;
    if (x < 0 || x >= effective_wsize())
        return ERR_INVALID_DATA;
    return (y & (1ULL << x)) != 0 ? ERR_YES : ERR_NO;
}

int docmd_rotxy(arg_struct *arg) {
    int x;
    uint8 y, res;
    int err;
    vartype *v;

    // Not using get_base_param() to fetch x, because that
    // would make it impossible to specify a negative shift
    // count in unsigned mode.
    phloat px = ((vartype_real *) stack[sp])->x;
    px = px < 0 ? -floor(-px) : floor(px);
    int wsize = effective_wsize();
    if (px >= wsize || px <= -wsize)
        return ERR_INVALID_DATA;
    x = to_int(px);

    if ((err = get_base_param(stack[sp - 1], (int8 *) &y)) != ERR_NONE)
        return err;
    if (x == 0)
        res = y;
    else {
        if (wsize < 64)
            y &= (1ULL << wsize) - 1;
        if (x > 0)
            res = (y >> x) | (y << (wsize - x));
        else {
            x = -x;
            res = (y << x) | (y >> (wsize - x));
        }
        base_range_check((int8 *) &res, true);
    }
    v = new_real(base2phloat((int8) res));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    return binary_result(v);
}

int docmd_baseadd(arg_struct *arg) {
    int8 x, y, res;
    int err;
    vartype *v;
    if ((err = get_base_param(stack[sp], &x)) != ERR_NONE)
        return err;
    if ((err = get_base_param(stack[sp - 1], &y)) != ERR_NONE)
        return err;
    res = x + y;
    int wsize = effective_wsize();
    if (!flags.f.base_wrap && wsize == 64) {
        if (flags.f.base_signed) {
            if (x > 0 && y > 0) {
                if (res < x || res < y)
                    if (flags.f.range_error_ignore)
                        res = (1LL << (effective_wsize() - 1)) - 1;
                    else
                        return ERR_OUT_OF_RANGE;
            } else if (x < 0 && y < 0) {
                if (res > x || res > y)
                    if (flags.f.range_error_ignore)
                        res = -1LL << (effective_wsize() - 1);
                    else
                        return ERR_OUT_OF_RANGE;
            }
        } else {
            if ((uint8) res < (uint8) x || (uint8) res < (uint8) y)
                if (flags.f.range_error_ignore)
                    res = ~0LL;
                else
                    return ERR_OUT_OF_RANGE;
        }
    } else {
        err = base_range_check(&res, false);
        if (err != ERR_NONE)
            return err;
    }
    v = new_real(base2phloat(res));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    err = binary_result(v);
    if (err == ERR_NONE) {
        if (wsize == 64)
            mode_carry = (uint8) x > (uint8) ~y;
        else {
            uint8 mask;
            mask = (1ULL << wsize) - 1;
            mode_carry = (uint8) (x & mask) > (uint8) (~y & mask);
        }
    }
    return err;
}

int docmd_basesub(arg_struct *arg) {
    int8 x, y, res;
    int err;
    vartype *v;
    if ((err = get_base_param(stack[sp], &x)) != ERR_NONE)
        return err;
    if ((err = get_base_param(stack[sp - 1], &y)) != ERR_NONE)
        return err;
    bool carry = (uint8) x > (uint8) y;
    if (!flags.f.base_signed && !flags.f.base_wrap && carry) {
        if (flags.f.range_error_ignore)
            res = 0;
        else
            return ERR_OUT_OF_RANGE;
    } else {
        res = y - x;
        if (!flags.f.base_wrap && effective_wsize() == 64) {
            if (flags.f.base_signed) {
                if (x < 0 && y > 0) {
                    if (x == (int8) 0x8000000000000000LL || res < -x || res < y)
                        if (flags.f.range_error_ignore)
                            res = (1LL << (effective_wsize() - 1)) - 1;
                        else
                            return ERR_OUT_OF_RANGE;
                } else if (x > 0 && y < 0) {
                    if (y == (int8) 0x8000000000000000LL || res > -x || res > y)
                        if (flags.f.range_error_ignore)
                            res = -1LL << (effective_wsize() - 1);
                        else
                            return ERR_OUT_OF_RANGE;
                }
            }
        } else {
            err = base_range_check(&res, false);
            if (err != ERR_NONE)
                return err;
        }
    }
    v = new_real(base2phloat(res));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    err = binary_result(v);
    if (err == ERR_NONE)
        mode_carry = carry;
    return err;
}

int docmd_basemul(arg_struct *arg) {
    int8 x, y;
    int8 res;
    int err;
    vartype *v;
    if ((err = get_base_param(stack[sp], &x)) != ERR_NONE)
        return err;
    if ((err = get_base_param(stack[sp - 1], &y)) != ERR_NONE)
        return err;
    if (x == 0 || y == 0) {
        res = 0;
    } else {
        /* Performing a 64-bit x 64-bit => 128-bit multiplication.
         * Doing it unsigned to keep things somewhat simple.
         */
        int wsize = effective_wsize();
        bool neg = false;
        uint8 op1, op2;
        if (flags.f.base_signed) {
            if (wsize == 64) {
                if (x == (int8) 0x8000000000000000LL && y != 1) {
                    if (flags.f.base_wrap)
                        res = (y & 1) == 0 ? 0 : x;
                    else if (flags.f.range_error_ignore)
                        res = y > 0 ? 0x8000000000000000LL : 0x7fffffffffffffffLL;
                    else
                        return ERR_OUT_OF_RANGE;
                } else if (y == (int8) 0x8000000000000000LL && x != 1) {
                    if (flags.f.base_wrap)
                        res = (x & 1) == 0 ? 0 : y;
                    else if (flags.f.range_error_ignore)
                        res = x > 0 ? 0x8000000000000000LL : 0x7fffffffffffffffLL;
                    else
                        return ERR_OUT_OF_RANGE;
                }
            }
            if (x < 0) {
                neg = true;
                op1 = (uint8) -x;
            } else {
                op1 = (uint8) x;
            }
            if (y < 0) {
                neg = !neg;
                op2 = (uint8) -y;
            } else {
                op2 = (uint8) y;
            }
        } else {
            op1 = (uint8) x;
            op2 = (uint8) y;
        }

        uint8 u1 = op1 & 0xffffffff;
        uint8 v1 = op2 & 0xffffffff;
        uint8 t = u1 * v1;
        uint8 w3 = t & 0xffffffff;
        uint8 k = t >> 32;

        op1 >>= 32;
        t = op1 * v1 + k;
        k = t & 0xffffffff;
        uint8 w1 = t >> 32;

        op2 >>= 32;
        t = u1 * op2 + k;
        k = t >> 32;

        uint8 hi = op1 * op2 + w1 + k;
        uint8 lo = (t << 32) + w3;

        if (flags.f.base_wrap) {
            if (neg)
                res = -(int8) lo;
            else
                res = (int8) lo;
            base_range_check(&res, true);
        } else if (flags.f.base_signed) {
            if (neg) {
                if (hi != 0 || lo > (1ULL << (wsize - 1)))
                    if (flags.f.range_error_ignore)
                        lo = 1ULL << (wsize - 1);
                    else
                        return ERR_OUT_OF_RANGE;
                res = - (int8) lo;
            } else {
                if (hi != 0 || lo > (1ULL << (wsize - 1)) - 1)
                    if (flags.f.range_error_ignore)
                        lo = (1ULL << (wsize - 1)) - 1;
                    else
                        return ERR_OUT_OF_RANGE;
                res = (int8) lo;
            }
        } else {
            if (hi != 0 || wsize < 64 && lo > (1ULL << wsize) - 1)
                if (flags.f.range_error_ignore)
                    lo = wsize == 64 ? ~0ULL : (1ULL << wsize) - 1;
                else
                    return ERR_OUT_OF_RANGE;
            res = (int8) lo;
        }
    }

    v = new_real(base2phloat(res));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    return binary_result(v);
}

int docmd_basediv(arg_struct *arg) {
    int8 x, y, res;
    int err;
    vartype *v;
    if ((err = get_base_param(stack[sp], &x)) != ERR_NONE)
        return err;
    if ((err = get_base_param(stack[sp - 1], &y)) != ERR_NONE)
        return err;
    if (x == 0)
        return ERR_DIVIDE_BY_0;
    if (flags.f.base_signed)
        res = y / x;
    else
        res = (int8) (((uint8) y) / ((uint8) x));
    if ((err = base_range_check(&res, false)) != ERR_NONE)
        return err;
    v = new_real(base2phloat(res));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    return binary_result(v);
}

int docmd_basechs(arg_struct *arg) {
    int8 x;
    int err;
    if ((err = get_base_param(stack[sp], &x)) != ERR_NONE)
        return err;
    x = -x;
    base_range_check(&x, true);
    ((vartype_real *) stack[sp])->x = base2phloat(x);
    return ERR_NONE;
}

/////////////////////////////
///// BASE enhancements /////
/////////////////////////////

int docmd_sl(arg_struct *arg) {
    int8 x;
    int err = get_base_param(stack[sp], &x);
    if (err != ERR_NONE) {
        flags.f.base_wrap = 0;
        return err;
    }
    int8 hibit = x & (1LL << (effective_wsize() - 1));
    mode_carry = hibit != 0;
    x = (x ^ hibit) << 1;
    base_range_check(&x, true);
    vartype *v = new_real(base2phloat(x));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

static int sr(bool arith) {
    int8 x;
    int err = get_base_param(stack[sp], &x);
    if (err != ERR_NONE)
        return err;
    mode_carry = x & 1;
    int8 hibit = x & (1LL << (effective_wsize() - 1));
    x >>= 1;
    if (hibit != 0)
        if (arith && flags.f.base_signed)
            x |= hibit;
        else
            x &= ~hibit;
    base_range_check(&x, true);
    vartype *v = new_real(base2phloat(x));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

int docmd_sr(arg_struct *arg) {
    return sr(false);
}

int docmd_asr(arg_struct *arg) {
    return sr(true);
}

static int rl(bool c) {
    int8 x;
    int err = get_base_param(stack[sp], &x);
    if (err != ERR_NONE)
        return err;
    int8 hibit = x & (1LL << (effective_wsize() - 1));
    x -= hibit;
    x <<= 1;
    bool carry = hibit != 0;
    if (c ? mode_carry : carry)
        x++;
    mode_carry = carry;
    base_range_check(&x, true);
    vartype *v = new_real(base2phloat(x));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

int docmd_rl(arg_struct *arg) {
    return rl(false);
}

int docmd_rlc(arg_struct *arg) {
    return rl(true);
}

static int rr(bool c) {
    int8 x;
    int err = get_base_param(stack[sp], &x);
    if (err != ERR_NONE)
        return err;
    bool carry = x & 1;
    x >>= 1;
    int wsize = effective_wsize();
    if (wsize < 64)
        x &= (1LL << wsize - 1) - 1;
    if (c ? mode_carry : carry)
        x |= 1LL << (wsize - 1);
    mode_carry = carry;
    base_range_check(&x, true);
    vartype *v = new_real(base2phloat(x));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

int docmd_rr(arg_struct *arg) {
    return rr(false);
}

int docmd_rrc(arg_struct *arg) {
    return rr(true);
}

int docmd_lj(arg_struct *arg) {
    int8 x;
    int err = get_base_param(stack[sp], &x);
    if (err != ERR_NONE)
        return err;
    int count = 0;
    if (x != 0) {
        int8 mask = 1LL << (effective_wsize() - 1);
        while ((x & mask) == 0) {
            x <<= 1;
            count++;
        }
    }
    base_range_check(&x, true);
    vartype *vx = new_real(count);
    vartype *vy = new_real(base2phloat(x));
    if (vx == NULL || vy == NULL) {
        free_vartype(vx);
        free_vartype(vy);
        return ERR_INSUFFICIENT_MEMORY;
    }
    return unary_two_results(vx, vy);
}

static int rn(bool left, bool c) {
    phloat nn = ((vartype_real *) stack[sp])->x;
    if (nn < 0)
        nn = -nn;
    int wsize = effective_wsize();
    if (nn >= wsize + 1)
        return ERR_INVALID_DATA;
    int n = to_int(nn);
    int8 xx;
    int err = get_base_param(stack[sp - 1], &xx);
    if (err != ERR_NONE)
        return err;
    uint8 x = xx;
    if (n != 0) {
        if (c) {
            if (left) {
                bool carry = (x >> (wsize - n)) & 1;
                int s = wsize - n + 1;
                uint8 t = s == 64 ? 0 : x >> s;
                x = (x << n) | t | (((uint8) mode_carry) << (n - 1));
                mode_carry = carry;
            } else {
                bool carry = (x >> (n - 1)) & 1;
                int s = wsize - n + 1;
                uint8 t = s == 64 ? 0 : x << s;
                x = (x >> n) | t | (((uint8) mode_carry) << (wsize - n));
                mode_carry = carry;
            }
        } else {
            if (left) {
                x = (x << n) | (x >> (wsize - n));
                mode_carry = x & 1;
            } else {
                x = (x >> n) | (x << (wsize - n));
                mode_carry = (x >> (wsize - 1)) & 1;
            }
        }
    }
    xx = x;
    base_range_check(&xx, true);
    vartype *v = new_real(base2phloat(xx));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

int docmd_rln(arg_struct *arg) {
    return rn(true, false);
}

int docmd_rrn(arg_struct *arg) {
    return rn(false, false);
}

int docmd_rlcn(arg_struct *arg) {
    return rn(true, true);
}

int docmd_rrcn(arg_struct *arg) {
    return rn(false, true);
}

static int bit(int op) {
    phloat x = ((vartype_real *) stack[sp])->x;
    if (x < 0)
        x = -x;
    int wsize = effective_wsize();
    if (x >= wsize)
        return ERR_INVALID_DATA;
    int n = to_int(x);
    int8 mask = 1LL << n;
    int8 f;
    int err = get_base_param(stack[sp - 1], &f);
    if (err != ERR_NONE)
        return err;
    if (op == 2) {
        err = unary_no_result();
        if (err == ERR_NONE)
            err = (f & mask) == 0 ? ERR_NO : ERR_YES;
        return err;
    }
    if (op == 0)
        f |= mask;
    else
        f &= ~mask;
    base_range_check(&f, true);
    vartype *v = new_real(base2phloat(f));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    return binary_result(v);
}

int docmd_sb(arg_struct *arg) {
    return bit(0);
}

int docmd_cb(arg_struct *arg) {
    return bit(1);
}

int docmd_b_t(arg_struct *arg) {
    return bit(2);
}

int docmd_num_b(arg_struct *arg) {
    int8 x;
    int err = get_base_param(stack[sp], &x);
    if (err != ERR_NONE)
        return err;
    int wsize = effective_wsize();
    if (wsize < 64)
        x &= (1LL << wsize) - 1;
    uint8 ux = (uint8) x;
    int count = 0;
    while (ux != 0) {
        count += ux & 1;
        ux >>= 1;
    }
    vartype *v = new_real(count);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

static int make_mask(bool left) {
    phloat x = ((vartype_real *) stack[sp])->x;
    if (x < 0)
        x = -x;
    int wsize = effective_wsize();
    if (x >= wsize + 1)
        return ERR_INVALID_DATA;
    int n = to_int(x);
    int8 res;
    if (n == 64)
        res = -1;
    else {
        res = (1LL << n) - 1;
        if (left)
            res <<= wsize - n;
    }
    base_range_check(&res, true);
    vartype *v = new_real(base2phloat(res));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

int docmd_maskl(arg_struct *arg) {
    return make_mask(true);
}

int docmd_maskr(arg_struct *arg) {
    return make_mask(false);
}

int docmd_sc(arg_struct *arg) {
    mode_carry = true;
    return ERR_NONE;
}

int docmd_cc(arg_struct *arg) {
    mode_carry = false;
    return ERR_NONE;
}

int docmd_c_t(arg_struct *arg) {
    return mode_carry ? ERR_YES : ERR_NO;
}

static int bits2result(void *data, int size) {
    int wsize = effective_wsize();
    vartype *v;
    if (size * 8 <= wsize) {
        int8 d;
        if (size == 4)
            d = *(uint4 *) data;
        else
            d = *(uint8 *) data;
        v = new_real((phloat) d);
    } else {
        v = new_string(NULL, size * 2);
        if (v != NULL) {
            unsigned char *d = (unsigned char *) data;
            char *dst = ((vartype_string *) v)->txt();
            unsigned short t = 0x0102;
            int dir = *((unsigned char *) &t) == 0x01 ? 1 : -1;
            if (dir == -1)
                d += size - 1;
            for (int i = 0; i < size; i++) {
                unsigned char c = *d;
                d += dir;
                *dst++ = "0123456789abcdef"[c >> 4];
                *dst++ = "0123456789abcdef"[c & 15];
            }
        }
    }
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

static bool result2bits(void *data, int size) {
    vartype *v = stack[sp];
    if (v->type == TYPE_REAL) {
        phloat x = ((vartype_real *) v)->x;
        if (x < 0 || x >= pow(phloat(2), 64))
            return false;
        switch (size) {
            case 4: *((uint4 *) data) = to_int8(x); break;
            case 8: *((uint8 *) data) = to_int8(x); break;
        }
        return true;
    } else {
        vartype_string *s = (vartype_string *) v;
        unsigned short t = 0x0102;
        int dir = *((unsigned char *) &t) == 0x01 ? 1 : -1;
        unsigned char *dst = (unsigned char *) data;
        if (dir == -1)
            dst += size - 1;
        if (s->length < 2 || (s->length & 1) != 0)
            return false;
        char *src = s->txt();
        for (int i = 0; i < s->length; i++) {
            char c = tolower(*src++);
            if (c < '0' || c > '9' && c < 'a' || c > 'f')
                return false;
        }
        int slen = s->length / 2;
        if (slen < size) {
            for (int i = slen; i < size; i++) {
                *dst = 0;
                dst += dir;
            }
        } else if (slen > size) {
            src += (size - slen) * 2;
            slen = size;
        }
        src = s->txt();
        for (int i = 0; i < slen; i++) {
            char c1 = tolower(*src++);
            char c2 = tolower(*src++);
            unsigned char b = (c1 < 'a' ? c1 - '0' : c1 - 'a' + 10) << 4
                            | (c2 < 'a' ? c2 - '0' : c2 - 'a' + 10);
            *dst = b;
            dst += dir;
        }
        return true;
    }
}

#ifdef BCD_MATH

int docmd_n_to_bs(arg_struct *arg) {
    BID_UINT128 x = ((vartype_real *) stack[sp])->x.val;
    float r;
    bid128_to_binary32(&r, &x);
    return bits2result(&r, 4);
}

int docmd_n_to_bd(arg_struct *arg) {
    BID_UINT128 x = ((vartype_real *) stack[sp])->x.val;
    double r;
    bid128_to_binary64(&r, &x);
    return bits2result(&r, 8);
}

int docmd_n_to_ds(arg_struct *arg) {
    BID_UINT128 x = ((vartype_real *) stack[sp])->x.val;
    BID_UINT32 r;
    bid128_to_bid32(&r, &x);
    return bits2result(&r, 4);
}

int docmd_n_to_dd(arg_struct *arg) {
    BID_UINT128 x = ((vartype_real *) stack[sp])->x.val;
    BID_UINT64 r;
    bid128_to_bid64(&r, &x);
    return bits2result(&r, 8);
}

int docmd_n_to_dq(arg_struct *arg) {
    BID_UINT128 x = ((vartype_real *) stack[sp])->x.val;
    return bits2result(&x, 16);
}

int docmd_bs_to_n(arg_struct *arg) {
    float x;
    if (!result2bits(&x, 4))
        return ERR_INVALID_DATA;
    vartype *v = new_real(x);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

int docmd_bd_to_n(arg_struct *arg) {
    double x;
    if (!result2bits(&x, 8))
        return ERR_INVALID_DATA;
    vartype *v = new_real(x);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

int docmd_ds_to_n(arg_struct *arg) {
    BID_UINT32 x;
    if (!result2bits(&x, 4))
        return ERR_INVALID_DATA;
    BID_UINT128 xx;
    bid32_to_bid128(&xx, &x);
    vartype *v = new_real(xx);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

int docmd_dd_to_n(arg_struct *arg) {
    BID_UINT64 x;
    if (!result2bits(&x, 8))
        return ERR_INVALID_DATA;
    BID_UINT128 xx;
    bid64_to_bid128(&xx, &x);
    vartype *v = new_real(xx);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

int docmd_dq_to_n(arg_struct *arg) {
    BID_UINT128 x;
    if (!result2bits(&x, 16))
        return ERR_INVALID_DATA;
    vartype *v = new_real(x);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

#else

int docmd_n_to_bs(arg_struct *arg) {
    double x = ((vartype_real *) stack[sp])->x;
    float r = (float) x;
    return bits2result(&r, 4);
}

int docmd_n_to_bd(arg_struct *arg) {
    double x = ((vartype_real *) stack[sp])->x;
    return bits2result(&x, 8);
}

int docmd_n_to_ds(arg_struct *arg) {
    double x = ((vartype_real *) stack[sp])->x;
    BID_UINT32 r;
    binary64_to_bid32(&r, &x);
    return bits2result(&r, 4);
}

int docmd_n_to_dd(arg_struct *arg) {
    double x = ((vartype_real *) stack[sp])->x;
    BID_UINT64 r;
    binary64_to_bid64(&r, &x);
    return bits2result(&r, 8);
}

int docmd_n_to_dq(arg_struct *arg) {
    double x = ((vartype_real *) stack[sp])->x;
    BID_UINT128 r;
    binary64_to_bid128(&r, &x);
    return bits2result(&r, 16);
}

int docmd_bs_to_n(arg_struct *arg) {
    float x;
    if (!result2bits(&x, 4))
        return ERR_INVALID_DATA;
    vartype *v = new_real(x);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

int docmd_bd_to_n(arg_struct *arg) {
    double x;
    if (!result2bits(&x, 8))
        return ERR_INVALID_DATA;
    vartype *v = new_real(x);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

int docmd_ds_to_n(arg_struct *arg) {
    BID_UINT32 x;
    if (!result2bits(&x, 4))
        return ERR_INVALID_DATA;
    double r;
    bid32_to_binary64(&r, &x);
    vartype *v = new_real(r);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

int docmd_dd_to_n(arg_struct *arg) {
    BID_UINT64 x;
    if (!result2bits(&x, 8))
        return ERR_INVALID_DATA;
    double r;
    bid64_to_binary64(&r, &x);
    vartype *v = new_real(r);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

int docmd_dq_to_n(arg_struct *arg) {
    BID_UINT128 x;
    if (!result2bits(&x, 16))
        return ERR_INVALID_DATA;
    double r;
    bid128_to_binary64(&r, &x);
    vartype *v = new_real(r);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

#endif

int docmd_wsize(arg_struct *arg) {
    phloat x = ((vartype_real *) stack[sp])->x;
#ifdef BCD_MATH
    if (x >= 65 || x < 1)
#else
    if (x >= 54 || x < 1)
#endif
        return ERR_INVALID_DATA;
    mode_wsize = to_int(x);
    print_trace();
    return ERR_NONE;
}

int docmd_wsize_t(arg_struct *arg) {
    vartype *new_x = new_real(effective_wsize());
    if (new_x == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    return recall_result(new_x);
}

int docmd_bsigned(arg_struct *arg) {
    flags.f.base_signed = !flags.f.base_signed;
    return ERR_NONE;
}

int docmd_bwrap(arg_struct *arg) {
    flags.f.base_wrap = !flags.f.base_wrap;
    return ERR_NONE;
}

int docmd_breset(arg_struct *arg) {
    mode_wsize = 36;
    flags.f.base_signed = 1;
    flags.f.base_wrap = 0;
    return ERR_NONE;
}

int docmd_decint(arg_struct *arg) {
    mode_dec_int = !mode_dec_int;
    return ERR_NONE;
}

int docmd_binsep(arg_struct *arg) {
    mode_bin_sep = !mode_bin_sep;
    return ERR_NONE;
}

int docmd_octsep(arg_struct *arg) {
    mode_oct_sep = !mode_oct_sep;
    return ERR_NONE;
}

int docmd_decsep(arg_struct *arg) {
    mode_dec_sep = !mode_dec_sep;
    return ERR_NONE;
}

int docmd_hexsep(arg_struct *arg) {
    mode_hex_sep = !mode_hex_sep;
    return ERR_NONE;
}

////////////////////////////////////
///// End of BASE enhancements /////
////////////////////////////////////

int docmd_solve(arg_struct *arg) {
    int err;
    vartype *v;
    phloat x1, x2;
    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    if (arg->type != ARGTYPE_STR)
        return ERR_INVALID_TYPE;

    v = recall_var(arg->val.text, arg->length);
    if (v == 0)
        x1 = 0;
    else if (v->type == TYPE_REAL)
        x1 = ((vartype_real *) v)->x;
    else if (v->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;

    if (stack[sp]->type == TYPE_REAL)
        x2 = ((vartype_real *) stack[sp])->x;
    else if (stack[sp]->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;

    if (!program_running())
        clear_all_rtns();
    string_copy(reg_alpha, &reg_alpha_length, arg->val.text, arg->length);
    return start_solve(arg->val.text, arg->length, x1, x2);
}

int docmd_vmsolve(arg_struct *arg) {
    vartype *v;
    phloat x1, x2;
    if (arg->type != ARGTYPE_STR)
        return ERR_INVALID_TYPE;

    v = recall_var(arg->val.text, arg->length);
    if (v == NULL) {
        x1 = 0;
        x2 = 1;
    } else if (v->type == TYPE_REAL) {
        x1 = ((vartype_real *) v)->x;
        if (!get_shadow(arg->val.text, arg->length, &x2))
            x2 = x1;
    } else if (v->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;

    clear_all_rtns();
    string_copy(reg_alpha, &reg_alpha_length, arg->val.text, arg->length);
    return start_solve(arg->val.text, arg->length, x1, x2);
}

int docmd_to_dec(arg_struct *arg) {
    phloat oct = ((vartype_real *) stack[sp])->x;
    phloat res;
    int neg = oct < 0;
    if (neg)
        oct = -oct;
    if (oct > 777777777777.0 || oct != floor(oct))
        return ERR_INVALID_DATA;
    vartype *v;
    #ifdef BCD_MATH
        phloat dec = 0, mul = 1;
        while (oct != 0) {
            int digit = to_digit(oct);
            if (digit > 7)
                return ERR_INVALID_DATA;
            oct = floor(oct / 10);
            dec += digit * mul;
            mul *= 8;
        }
        res = neg ? -dec : dec;
    #else
        int8 ioct = to_int8(oct);
        int8 dec = 0, mul = 1;
        while (ioct != 0) {
            int digit = (int) (ioct % 10);
            if (digit > 7)
                return ERR_INVALID_DATA;
            ioct /= 10;
            dec += digit * mul;
            mul <<= 3;
        }
        res = (double) (neg ? -dec : dec);
    #endif
    v = new_real(res);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

int docmd_to_oct(arg_struct *arg) {
    phloat dec = ((vartype_real *) stack[sp])->x;
    phloat res;
    int neg = dec < 0;
    if (neg)
        dec = -dec;
    if (dec > 68719476735.0 || dec != floor(dec))
        return ERR_INVALID_DATA;
    vartype *v;
    #ifdef BCD_MATH
        phloat oct = 0, mul = 1;
        while (dec != 0) {
            int digit = to_int(fmod(dec, 8));
            dec = floor(dec / 8);
            oct += digit * mul;
            mul *= 10;
        }
        res = neg ? -oct : oct;
    #else
        int8 idec = to_int8(dec);
        int8 oct = 0, mul = 1;
        while (idec != 0) {
            int digit = (int) (idec & 7);
            idec >>= 3;
            oct += digit * mul;
            mul *= 10;
        }
        res = (double) (neg ? -oct : oct);
    #endif
    v = new_real(res);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

static void accum(phloat *sum, phloat term, int weight) {
    int inf;
    phloat s;
    if (weight == 1)
        s = *sum + term;
    else
        s = *sum - term;
    if ((inf = p_isinf(s)) != 0)
        s = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
    *sum = s;
}

static phloat sigma_helper_2(phloat *sigmaregs,
                             phloat x, phloat y, int weight) {

    accum(&sigmaregs[0], x, weight);
    accum(&sigmaregs[1], x * x, weight);
    accum(&sigmaregs[2], y, weight);
    accum(&sigmaregs[3], y * y, weight);
    accum(&sigmaregs[4], x * y, weight);
    accum(&sigmaregs[5], 1, weight);

    if (flags.f.all_sigma) {
        if (x > 0) {
            phloat lnx = log(x);
            if (y > 0) {
                phloat lny = log(y);
                accum(&sigmaregs[8], lny, weight);
                accum(&sigmaregs[9], lny * lny, weight);
                accum(&sigmaregs[10], lnx * lny, weight);
                accum(&sigmaregs[11], x * lny, weight);
            } else {
                flags.f.exp_fit_invalid = 1;
                flags.f.pwr_fit_invalid = 1;
            }
            accum(&sigmaregs[6], lnx, weight);
            accum(&sigmaregs[7], lnx * lnx, weight);
            accum(&sigmaregs[12], lnx * y, weight);
        } else {
            if (y > 0) {
                phloat lny = log(y);
                accum(&sigmaregs[8], lny, weight);
                accum(&sigmaregs[9], lny * lny, weight);
                accum(&sigmaregs[11], x * lny, weight);
            } else
                flags.f.exp_fit_invalid = 1;
            flags.f.log_fit_invalid = 1;
            flags.f.pwr_fit_invalid = 1;
        }
    } else {
        flags.f.log_fit_invalid = 1;
        flags.f.exp_fit_invalid = 1;
        flags.f.pwr_fit_invalid = 1;
    }

    return sigmaregs[5];
}

static int sigma_helper_1(int weight) {
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
        if (r->array->is_string[i] != 0)
            return ERR_ALPHA_DATA_IS_INVALID;
    sigmaregs = r->array->data + first;

    /* All summation registers present, real-valued, non-string. */
    if (stack[sp]->type == TYPE_REALMATRIX) {
        vartype_realmatrix *rm = (vartype_realmatrix *) stack[sp];
        vartype_real *x;
        int4 i;
        if (rm->columns != 2)
            return ERR_DIMENSION_ERROR;
        for (i = 0; i < rm->rows * 2; i++)
            if (rm->array->is_string[i] != 0)
                return ERR_ALPHA_DATA_IS_INVALID;
        x = (vartype_real *) new_real(0);
        if (x == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        for (i = 0; i < rm->rows; i++)
            x->x = sigma_helper_2(sigmaregs,
                                    rm->array->data[i * 2],
                                    rm->array->data[i * 2 + 1],
                                    weight);
        free_vartype(lastx);
        lastx = stack[sp];
        stack[sp] = (vartype *) x;
        mode_disable_stack_lift = true;
        return ERR_NONE;
    } else {
        // stack[sp]->type == TYPE_REAL
        if (sp == 0 || stack[sp - 1]->type == TYPE_REAL) {
            vartype_real *x = (vartype_real *) new_real(0);
            if (x == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            phloat y = sp == 0 ? 0 : ((vartype_real *) stack[sp - 1])->x;
            x->x = sigma_helper_2(sigmaregs,
                                    ((vartype_real *) stack[sp])->x,
                                    y,
                                    weight);
            free_vartype(lastx);
            lastx = stack[sp];
            stack[sp] = (vartype *) x;
            mode_disable_stack_lift = true;
            return ERR_NONE;
        } else if (stack[sp - 1]->type == TYPE_STRING) {
            return ERR_ALPHA_DATA_IS_INVALID;
        } else {
            return ERR_INVALID_TYPE;
        }
    }
}

int docmd_sigmaadd(arg_struct *arg) {
    int err = sigma_helper_1(1);
    if (err == ERR_NONE)
        print_trace();
    return err;
}

int docmd_sigmasub(arg_struct *arg) {
    int err = sigma_helper_1(-1);
    if (err == ERR_NONE)
        print_trace();
    return err;
}
