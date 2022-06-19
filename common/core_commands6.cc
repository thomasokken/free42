/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2022  Thomas Okken
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
#include "shell.h"

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
    vartype *v;
    int err = map_unary(stack[sp], &v, mappable_sin_r, mappable_sin_c);
    if (err == ERR_NONE)
        unary_result(v);
    return err;
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
    vartype *v;
    int err = map_unary(stack[sp], &v, mappable_cos_r, mappable_cos_c);
    if (err == ERR_NONE)
        unary_result(v);
    return err;
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
    if (p_isnan(xre) || p_isnan(xim)) {
        *yre = NAN_PHLOAT;
        *yim = NAN_PHLOAT;
        return ERR_NONE;
    }

    phloat sinxre, cosxre;
    p_sincos(xre, &sinxre, &cosxre);
    phloat sinhxim = sinh(xim);
    phloat coshxim = cosh(xim);
    phloat d = cosxre * cosxre + sinhxim * sinhxim;
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
    *yre = sinxre * cosxre / d;
    *yim = sinhxim * coshxim / d;
    return ERR_NONE;
}

int docmd_tan(arg_struct *arg) {
    vartype *v;
    int err = map_unary(stack[sp], &v, mappable_tan_r, mappable_tan_c);
    if (err == ERR_NONE)
        unary_result(v);
    return err;
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
    if (stack[sp]->type == TYPE_REAL) {
        phloat x = ((vartype_real *) stack[sp])->x;
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
        int err = map_unary(stack[sp], &v, mappable_asin_r, mappable_asin_c);
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
    if (stack[sp]->type == TYPE_REAL) {
        phloat x = ((vartype_real *) stack[sp])->x;
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
        int err = map_unary(stack[sp], &v, mappable_acos_r, mappable_acos_c);
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
    vartype *v;
    int err = map_unary(stack[sp], &v, mappable_atan_r, mappable_atan_c);
    if (err == ERR_NONE)
        unary_result(v);
    return err;
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
    if (stack[sp]->type == TYPE_REAL) {
        vartype_real *x = (vartype_real *) stack[sp];
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
        int err = map_unary(stack[sp], &v, mappable_log_r, mappable_log_c);
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
    vartype *v;
    int err = map_unary(stack[sp], &v, mappable_10_pow_x_r,
                                    mappable_10_pow_x_c);
    if (err == ERR_NONE)
        unary_result(v);
    return err;
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
    if (stack[sp]->type == TYPE_REAL) {
        vartype_real *x = (vartype_real *) stack[sp];
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
        int err = map_unary(stack[sp], &v, mappable_ln_r, mappable_ln_c);
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
    vartype *v;
    int err = map_unary(stack[sp], &v, mappable_e_pow_x_r, mappable_e_pow_x_c);
    if (err == ERR_NONE)
        unary_result(v);
    return err;
}

static int mappable_sqrt_r(phloat x, phloat *y) {
    if (x < 0)
        return ERR_INVALID_DATA;
    else {
        *y = sqrt(x);
        return ERR_NONE;
    }
}

int docmd_sqrt(arg_struct *arg) {
    if (stack[sp]->type == TYPE_REAL) {
        phloat x = ((vartype_real *) stack[sp])->x;
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
    } else {
        vartype *v;
        int err = map_unary(stack[sp], &v, mappable_sqrt_r, math_sqrt);
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
    vartype *v;
    int err = map_unary(stack[sp], &v, mappable_square_r, mappable_square_c);
    if (err == ERR_NONE)
        unary_result(v);
    return err;
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
    phloat r, t, rre, rim;
    int inf;
    if (xre == 0 && xim == 0)
        return ERR_DIVIDE_BY_0;
    if (fabs(xim) <= fabs(xre)) {
        r = xim / xre;
        t = 1 / (xre + xim * r);
        if (r == 0) {
            rre = t;
            rim = -xim * (1 / xre) * t;
        } else {
            rre = t;
            rim = -r * t;
        }
    } else {
        r = xre / xim;
        t = 1 / (xre * r + xim);
        if (r == 0) {
            rre = xre * (1 / xim) * t;
            rim = -t;
        } else {
            rre = r * t;
            rim = -t;
        }
    }
    inf = p_isinf(rre);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rre = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    inf = p_isinf(rim);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rim = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    *yre = rre;
    *yim = rim;
    return ERR_NONE;
}

int docmd_inv(arg_struct *arg) {
    vartype *v;
    int err = map_unary(stack[sp], &v, mappable_inv_r, mappable_inv_c);
    if (err == ERR_NONE)
        unary_result(v);
    return err;
}

int docmd_y_pow_x(arg_struct *arg) {
    phloat yr, yphi;
    int inf;
    vartype *res;

    if (false) {
        done:
        if (res == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        else
            return binary_result(res);
    }

    if (stack[sp]->type == TYPE_STRING || stack[sp - 1]->type == TYPE_STRING) {
        return ERR_ALPHA_DATA_IS_INVALID;
    } else if (stack[sp]->type != TYPE_REAL
            && stack[sp]->type != TYPE_COMPLEX
            || stack[sp - 1]->type != TYPE_REAL
            && stack[sp - 1]->type != TYPE_COMPLEX) {
        return ERR_INVALID_TYPE;
    } else if (stack[sp]->type == TYPE_REAL) {
        phloat x = ((vartype_real *) stack[sp])->x;
        if (x == floor(x)) {
            /* Integer exponent */
            if (stack[sp - 1]->type == TYPE_REAL) {
                /* Real number to integer power */
                phloat y = ((vartype_real *) stack[sp - 1])->x;
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
                goto done;
            } else {
                /* Complex number to integer power */
                phloat rre, rim, yre, yim;
                int4 ex;
                if (x < -256.0 || x > 256.0)
                    /* At some point, the cumulative error in repeated squaring
                     * becomes larger than the error in the polar power
                     * formula, so we only use repeated squaring for smallish
                     * exponents. The limit of 256 is somewhat arbitrary, but
                     * high enough that whenever possible, the function will
                     * return exact results for short bases, i.e. bases where
                     * first and last nonzero digits are close together. The
                     * smallest non-trivial base is 1+i, which can be raised to
                     * the power 225 in the decimal version, and to the power
                     * 107 in the binary version, with exact results.
                     */
                    goto complex_pow_real_1;
                rre = 1;
                rim = 0;
                yre = ((vartype_complex *) stack[sp - 1])->re;
                yim = ((vartype_complex *) stack[sp - 1])->im;
                ex = to_int4(x);
                if (ex <= 0 && yre == 0 && yim == 0)
                    return ERR_INVALID_DATA;
                bool invert = ex < 0;
                if (invert)
                    ex = -ex;
                while (1) {
                    phloat tmp;
                    if ((ex & 1) != 0) {
                        tmp = rre * yre - rim * yim;
                        rim = rre * yim + rim * yre;
                        rre = tmp;
                        if (p_isinf(rre) || p_isnan(rre) || p_isinf(rim) || p_isnan(rim))
                            goto complex_pow_real_1;
                        if (rre == 0 && rim == 0)
                            if (invert)
                                goto complex_pow_real_1;
                            else
                                break;
                    }
                    ex >>= 1;
                    if (ex == 0)
                        break;
                    tmp = yre * yre - yim * yim;
                    yim = 2 * yre * yim;
                    yre = tmp;
                    if (p_isinf(yre) || p_isnan(yre) || p_isinf(yim) || p_isnan(yim))
                        goto complex_pow_real_1;
                }
                if (invert) {
                    phloat h = hypot(rre, rim);
                    rre = rre / h / h;
                    rim = (-rim) / h / h;
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
                goto done;
            }
        } else if (stack[sp - 1]->type == TYPE_REAL) {
            /* Real number to noninteger real power */
            phloat y = ((vartype_real *) stack[sp - 1])->x;
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
            goto done;
        } else {
            /* Complex (or negative real) number to noninteger real power */
            complex_pow_real_1:
            {
                phloat yre = ((vartype_complex *) stack[sp - 1])->re;
                phloat yim = ((vartype_complex *) stack[sp - 1])->im;
                yr = hypot(yre, yim);
                if (yim == 0)
                    yphi = yre >= 0 ? 0 : PI;
                else
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
            goto done;
        }
    } else {
        /* Real or complex number to complex power */
        phloat xre = ((vartype_complex *) stack[sp])->re;
        phloat xim = ((vartype_complex *) stack[sp])->im;
        phloat yre, yim;
        phloat lre, lim;
        phloat tmp;
        int err;
        if (stack[sp - 1]->type == TYPE_REAL) {
            yre = ((vartype_real *) stack[sp - 1])->x;
            yim = 0;
        } else {
            yre = ((vartype_complex *) stack[sp - 1])->re;
            yim = ((vartype_complex *) stack[sp - 1])->im;
        }
        if (yre == 0 && yim == 0) {
            if (xre <= 0)
                return ERR_INVALID_DATA;
            else
                res = new_complex(0, 0);
            goto done;
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
        goto done;
    }
}

int docmd_anum(arg_struct *arg) {
    phloat res;
    if (!anum(reg_alpha, reg_alpha_length, &res))
        return ERR_NONE;
    vartype *v = new_real(res);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    flags.f.numeric_data_input = 1;
    return recall_result(v);
}

int docmd_x_swap_f(arg_struct *arg) {
    phloat x = ((vartype_real *) stack[sp])->x;
    if (x < 0)
        x = -x;
    if (x >= 256)
        return ERR_INVALID_DATA;
    int f = 0;
    for (int i = 7; i >= 0; i--)
        f = (f << 1) | flags.farray[i];
    int nf = to_int(x);
    for (int i = 0; i < 8; i++) {
        flags.farray[i] = nf & 1;
        nf >>= 1;
    }
    ((vartype_real *) stack[sp])->x = f;
    return ERR_NONE;
}

int docmd_rclflag(arg_struct *arg) {
    uint8 lfs = 0, hfs = 0;
    uint8 p = 1;
    for (int i = 0; i < 50; i++) {
        int j = i + 50;
        char lf = virtual_flags[i] == '1' ? virtual_flag_handler(FLAGOP_FS_T, i) == ERR_YES : flags.farray[i] != 0;
        char hf = virtual_flags[j] == '1' ? virtual_flag_handler(FLAGOP_FS_T, j) == ERR_YES : flags.farray[j] != 0;
        if (lf)
            lfs += p;
        if (hf)
            hfs += p;
        p <<= 1;
    }
#ifdef BCD_MATH
    vartype *v = new_complex(lfs, hfs);
#else
    /* Silence warning about possible loss of precision in VS2019.
     * The warning is correct, in that we're assigning 64-bit integers
     * to floating-point types with only 53-bit mantissas; but it just
     * happens to be the case that we know we're only using 50 bits
     * so this really is okay.
     */
    vartype *v = new_complex((double) lfs, (double) hfs);
#endif
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    return recall_result(v);
}

int docmd_stoflag(arg_struct *arg) {
    if (stack[sp]->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    if (stack[sp]->type != TYPE_REAL && stack[sp]->type != TYPE_COMPLEX)
        return ERR_INVALID_DATA;
    vartype_complex *c;
    int b, e;
    if (stack[sp]->type == TYPE_COMPLEX) {
        c = (vartype_complex *) stack[sp];
        b = 0;
        e = 99;
    } else {
        if (stack[sp - 1]->type == TYPE_STRING)
            return ERR_ALPHA_DATA_IS_INVALID;
        if (stack[sp - 1]->type != TYPE_COMPLEX)
            return ERR_INVALID_DATA;
        c = (vartype_complex *) stack[sp - 1];
        phloat x = ((vartype_real *) stack[sp])->x;
        if (x < 0)
            x = -x;
        if (x >= 100)
            return ERR_INVALID_DATA;
        b = to_int(x);
        x = (x - b) * 100;
        #ifndef BCD_MATH
            x = x + 0.0000000005;
        #endif
        e = to_int(x);
        if (e > 99)
            e = 99;
        if (e < b)
            e = b;
    }

    char old_g = !flags.f.rad && flags.f.grad;
    char old_rad = flags.f.rad || flags.f.grad;

    phloat lo = c->re;
    phloat hi = c->im;
    if (lo < 0)
        lo = -lo;
    if (hi < 0)
        hi = -hi;
    if (lo >= 1LL << 50 || hi >= 1LL << 50)
        return ERR_INVALID_DATA;
    uint8 lfs = to_int8(lo);
    uint8 hfs = to_int8(hi);
    uint8 p = 1;
    for (int i = 0; i < 50; i++) {
        int j = i + 50;
        char lf = virtual_flags[i] == '1' ? 0 : (lfs & p) != 0;
        char hf = virtual_flags[j] == '1' ? 0 : (hfs & p) != 0;
        if (i >= b && i <= e)
            flags.farray[i] = lf;
        if (j >= b && j <= e && j != 80)
            flags.farray[j] = hf;
        p <<= 1;
    }

    char new_g = !flags.f.rad && flags.f.grad;
    char new_rad = flags.f.rad || flags.f.grad;
    if (new_g == old_g)
        new_g = -1;
    if (new_rad == old_rad)
        new_rad = -1;
    if (new_g != -1 || new_rad != -1)
        shell_annunciators(-1, -1, -1, -1, new_g, new_rad);

    return ERR_NONE;
}
