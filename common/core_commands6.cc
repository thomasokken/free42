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

#include "core_commands6.h"
#include "core_helpers.h"
#include "core_math2.h"
#include "core_sto_rcl.h"
#include "core_variables.h"

/********************************************************/
/* Implementations of HP-42S built-in functions, part 6 */
/********************************************************/

static int mappable_sin_r(phloat x, phloat *y) {
    if (flags.f.rad)
        *y = sin(x);
    else if (flags.f.grad)
        *y = sin_grad(x);
    else
        *y = sin_deg(x);
    return ERR_NONE;
}

static int mappable_sin_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    /* NOTE: DEG/RAD/GRAD mode does not apply here. */
    if (xim == 0) {
        *yre = sin(xre);
        *yim = 0;
        return ERR_NONE;
    } else if (xre == 0) {
        *yre = 0;
        *yim = sinh(xim);
        return ERR_NONE;
    }
    phloat sinxre, cosxre;
    phloat sinhxim, coshxim;
    int inf;
    p_sincos(xre, &sinxre, &cosxre);
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

static int mappable_cos_r(phloat x, phloat *y) {
    if (flags.f.rad)
        *y = cos(x);
    else if (flags.f.grad)
        *y = cos_grad(x);
    else
        *y = cos_deg(x);
    return ERR_NONE;
}

static int mappable_cos_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    /* NOTE: DEG/RAD/GRAD mode does not apply here. */
    if (xim == 0) {
        *yre = cos(xre);
        *yim = 0;
        return ERR_NONE;
    } else if (xre == 0) {
        *yre = cosh(xim);
        int inf;
        if ((inf = p_isinf(*yre)) != 0)
            if (flags.f.range_error_ignore)
                *yre = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        *yim = 0;
        return ERR_NONE;
    }

    phloat sinxre, cosxre;
    phloat sinhxim, coshxim;
    int inf;
    p_sincos(xre, &sinxre, &cosxre);
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

static int mappable_tan_r(phloat x, phloat *y) {
    return math_tan(x, y, false);
}

static int mappable_tan_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    /* NOTE: DEG/RAD/GRAD mode does not apply here. */

    if (xim == 0) {
        *yim = 0;
        return math_tan(xre, yre, true);
    } else if (xre == 0) {
        *yre = 0;
        *yim = tanh(xim);
        return ERR_NONE;
    }

    phloat xre2 = xre * 2;
    if (p_isnan(xre) || p_isnan(xim) || p_isinf(xre2)) {
        *yre = NAN_PHLOAT;
        *yim = NAN_PHLOAT;
        return ERR_NONE;
    }
    phloat xim2 = xim * 2;
    phloat sinxre2, cosxre2;
    p_sincos(xre2, &sinxre2, &cosxre2);
    phloat sinhxim2 = sinh(xim2);
    phloat coshxim2 = cosh(xim2);
    phloat d = cosxre2 + coshxim2;
    if (d == 0) {
        if (flags.f.range_error_ignore) {
            *yre = POS_HUGE_PHLOAT;
            *yim = POS_HUGE_PHLOAT;
            return ERR_NONE;
        } else
            return ERR_OUT_OF_RANGE;
    }
    if (p_isinf(d) != 0) {
        *yre = 0;
        *yim = xim < 0 ? -1 : 1;
        return ERR_NONE;
    }
    *yre = sinxre2 / d;
    *yim = sinhxim2 / d;
    int inf;
    if ((inf = p_isinf(*yre)) != 0) {
        if (flags.f.range_error_ignore)
            *yre = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
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

static int mappable_asin_r(phloat x, phloat *y) {
    if (x < -1 || x > 1)
        return ERR_INVALID_DATA;
    if (!flags.f.rad) {
        if (x == 1) {
            *y = flags.f.grad ? 100 : 90;
            return ERR_NONE;
        } else if (x == -1) {
            *y = flags.f.grad ? -100 : -90;
            return ERR_NONE;
        }
    }
    *y = rad_to_angle(asin(x));
    return ERR_NONE;
}

static int mappable_asin_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    phloat tre, tim;
    int err = math_asinh(-xim, xre, &tre, &tim);
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
                if (x > 0)
                    v = new_complex(PI / 2, -acosh(x));
                else
                    v = new_complex(-PI / 2, acosh(-x));
            }
        } else {
            v = new_real(0);
            if (v != NULL)
                mappable_asin_r(x, &((vartype_real *) v)->x);
        }
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

static int mappable_acos_r(phloat x, phloat *y) {
    if (x < -1 || x > 1)
        return ERR_INVALID_DATA;
    if (!flags.f.rad)
        if (x == 0) {
            *y = flags.f.grad ? 100 : 90;
            return ERR_NONE;
        } else if (x == -1) {
            *y = flags.f.grad ? 200 : 180;
            return ERR_NONE;
        }
    *y = rad_to_angle(acos(x));
    return ERR_NONE;
}

static int mappable_acos_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    phloat tre, tim;
    int err = math_acosh(xre, xim, &tre, &tim);
    if (xim < 0 || xim == 0 && xre > 1) {
        *yre = -tim;
        *yim = tre;
    } else {
        *yre = tim;
        *yim = -tre;
    }
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
        } else {
            v = new_real(0);
            if (v != NULL)
                mappable_acos_r(x, &((vartype_real *) v)->x);
        }
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

static int mappable_atan_r(phloat x, phloat *y) {
    if (!flags.f.rad) {
        if (p_isinf(x)) {
            *y = flags.f.grad ? 100 : 90;
            return ERR_NONE;
        } else if (x == 1) {
            *y = flags.f.grad ? 50 : 45;
            return ERR_NONE;
        } else if (x == -1) {
            *y = flags.f.grad ? -50 : -45;
            return ERR_NONE;
        }
    }
    *y = rad_to_angle(atan(x));
    return ERR_NONE;
}

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

static int mappable_log_r(phloat x, phloat *y) {
    if (x <= 0)
        return ERR_INVALID_DATA;
    else {
        *y = log10(x);
        return ERR_NONE;
    }
}

static int mappable_log_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    if (xim == 0) {
        if (xre == 0)
            return ERR_INVALID_DATA;
        if (xre > 0) {
            *yre = log10(xre);
            *yim = 0;
        } else {
            *yre = log10(-xre);
            *yim = PI / log(10.0);
        }
        return ERR_NONE;
    } else if (xre == 0) {
        if (xim > 0) {
            *yre = log10(xim);
            *yim = PI / log(100.0);
        } else {
            *yre = log10(-xim);
            *yim = -PI / log(100.0);
        }
        return ERR_NONE;
    } else {
        phloat h = hypot(xre, xim);
        if (p_isinf(h)) {
            const phloat s = 10000;
            h = hypot(xre / s, xim / s);
            *yre = log10(h) + 4;
        } else
            *yre = log10(h);
        *yim = atan2(xim, xre) / log(10.0);
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
                vartype *r = new_complex(log10(-x->x), PI / log(10.0));
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

static int mappable_10_pow_x_r(phloat x, phloat *y) {
    *y = pow(10, x);
    if (p_isinf(*y) != 0) {
        if (!flags.f.range_error_ignore)
            return ERR_OUT_OF_RANGE;
        *y = POS_HUGE_PHLOAT;
    }
    return ERR_NONE;
}

static int mappable_10_pow_x_c(phloat xre, phloat xim, phloat *yre, phloat *yim){
    int inf;
    phloat h;
    xim *= log(10.0);
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

static int mappable_ln_r(phloat x, phloat *y) {
    if (x <= 0)
        return ERR_INVALID_DATA;
    else {
        *y = log(x);
        return ERR_NONE;
    }
}

static int mappable_ln_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    if (xim == 0) {
        if (xre == 0)
            return ERR_INVALID_DATA;
        if (xre > 0) {
            *yre = log(xre);
            *yim = 0;
        } else {
            *yre = log(-xre);
            *yim = PI;
        }
        return ERR_NONE;
    } else if (xre == 0) {
        if (xim > 0) {
            *yre = log(xim);
            *yim = PI / 2;
        } else {
            *yre = log(-xim);
            *yim = -PI / 2;
        }
        return ERR_NONE;
    } else {
        phloat h = hypot(xre, xim);
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

static int mappable_e_pow_x_r(phloat x, phloat *y) {
    *y = exp(x);
    if (p_isinf(*y) != 0) {
        if (!flags.f.range_error_ignore)
            return ERR_OUT_OF_RANGE;
        *y = POS_HUGE_PHLOAT;
    }
    return ERR_NONE;
}

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

static int mappable_sqrt_r(phloat x, phloat *y) {
    if (x < 0)
        return ERR_INVALID_DATA;
    else {
        *y = sqrt(x);
        return ERR_NONE;
    }
}

static int mappable_sqrt_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    if (xre == 0 && xim == 0) {
        *yre = 0;
        *yim = 0;
        return ERR_NONE;
    }

    phloat r = hypot(xre, xim);
    phloat a = sqrt((r + fabs(xre)) / 2);
    phloat b = xim / (a * 2);

    if (p_isinf(a)) {
        xre /= 100;
        xim /= 100;
        r = hypot(xre, xim);
        a = sqrt((r + fabs(xre)) / 2);
        b = xim / (a * 2);
        a *= 10;
        b *= 10;
    }

    if (xre >= 0) {
        *yre = a;
        *yim = b;
    } else if (b >= 0) {
        *yre = b;
        *yim = a;
    } else {
        *yre = -b;
        *yim = -a;
    }
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
                if (x == 0 && y == 0)
                    return ERR_INVALID_DATA;
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
                if (ex <= 0 && yre == 0 && yim == 0)
                    return ERR_INVALID_DATA;
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
                    p_sincos(yphi, &im, &re);
                    rre = re == 0 ? 0 :
                            re < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
                    rim = im == 0 ? 0 :
                            im < 0 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
                }
            } else {
                phloat re, im;
                p_sincos(yphi, &im, &re);
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
            if (xre <= 0)
                return ERR_INVALID_DATA;
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
