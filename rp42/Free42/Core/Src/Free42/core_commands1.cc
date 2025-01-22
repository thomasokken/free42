/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2024  Thomas Okken
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

#include "core_commands1.h"
#include "core_commands2.h"
#include "core_display.h"
#include "core_helpers.h"
#include "core_main.h"
#include "core_math1.h"
#include "core_sto_rcl.h"
#include "core_variables.h"
#include "shell.h"


/********************************************************/
/* Implementations of HP-42S built-in functions, part 1 */
/********************************************************/

int docmd_clx(arg_struct *arg) {
    free_vartype(stack[sp]);
    stack[sp] = new_real(0);
    mode_disable_stack_lift = true;
    return ERR_NONE;
}

int docmd_enter(arg_struct *arg) {
    vartype *v = dup_vartype(stack[sp]);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    char prev_stack_lift = flags.f.stack_lift_disable;
    flags.f.stack_lift_disable = 0;
    if (recall_result_silently(v) != ERR_NONE) {
        flags.f.stack_lift_disable = prev_stack_lift;
        return ERR_INSUFFICIENT_MEMORY;
    }
    mode_disable_stack_lift = true;
    print_stack_trace();
    return ERR_NONE;
}

int docmd_swap(arg_struct *arg) {
    vartype *temp = stack[sp];
    stack[sp] = stack[sp - 1];
    stack[sp - 1] = temp;
    print_trace();
    return ERR_NONE;
}

int docmd_rdn(arg_struct *arg) {
    if (sp > 0) {
        vartype *temp = stack[sp];
        memmove(stack + 1, stack, sp * sizeof(vartype *));
        stack[0] = temp;
    }
    print_trace();
    return ERR_NONE;
}

int docmd_chs(arg_struct *arg) {
    switch (stack[sp]->type) {
        case TYPE_REAL: {
            vartype_real *r = (vartype_real *) stack[sp];
            r->x = -(r->x);
            break;
        }
        case TYPE_COMPLEX: {
            vartype_complex *c = (vartype_complex *) stack[sp];
            c->re = -(c->re);
            c->im = -(c->im);
            break;
        }
        case TYPE_REALMATRIX: {
            vartype_realmatrix *rm = (vartype_realmatrix *) stack[sp];
            if (contains_strings(rm))
                return ERR_ALPHA_DATA_IS_INVALID;
            if (!disentangle((vartype *) rm))
                return ERR_INSUFFICIENT_MEMORY;
            int4 sz = rm->rows * rm->columns;
            for (int4 i = 0; i < sz; i++)
                rm->array->data[i] = -(rm->array->data[i]);
            break;
        }
        case TYPE_COMPLEXMATRIX: {
            vartype_complexmatrix *cm = (vartype_complexmatrix *) stack[sp];
            if (!disentangle((vartype *) cm))
                return ERR_INSUFFICIENT_MEMORY;
            int4 sz = cm->rows * cm->columns * 2;
            for (int4 i = 0; i < sz; i++)
                cm->array->data[i] = -(cm->array->data[i]);
            break;
        }
        default:
            return ERR_INTERNAL_ERROR;
    }
    print_trace();
    return ERR_NONE;
}

static int docmd_div_completion(int error, vartype *res) {
    if (error != ERR_NONE)
        return error;
    return binary_result(res);
}

int docmd_div(arg_struct *arg) {
    return generic_div(stack[sp], stack[sp - 1], docmd_div_completion);
}

static int docmd_mul_completion(int error, vartype *res) {
    if (error != ERR_NONE)
        return error;
    return binary_result(res);
}

int docmd_mul(arg_struct *arg) {
    return generic_mul(stack[sp], stack[sp - 1], docmd_mul_completion);
}

int docmd_sub(arg_struct *arg) {
    vartype *res;
    int error = generic_sub(stack[sp], stack[sp - 1], &res);
    if (error != ERR_NONE)
        return error;
    return binary_result(res);
}

int docmd_add(arg_struct *arg) {
    vartype *res;
    int error = generic_add(stack[sp], stack[sp - 1], &res);
    if (error != ERR_NONE)
        return error;
    return binary_result(res);
}

int docmd_lastx(arg_struct *arg) {
    vartype *v = dup_vartype(lastx);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    return recall_result(v);
}

int docmd_complex(arg_struct *arg) {
    switch (stack[sp]->type) {
        case TYPE_REAL: {
            if (stack[sp - 1]->type == TYPE_STRING)
                return ERR_ALPHA_DATA_IS_INVALID;
            else if (stack[sp - 1]->type != TYPE_REAL)
                return ERR_INVALID_TYPE;
            vartype *v;
            if (flags.f.polar) {
                phloat re, im;
                generic_p2r(((vartype_real *) stack[sp - 1])->x,
                            ((vartype_real *) stack[sp])->x, &re, &im);
                v = new_complex(re, im);
            } else {
                v = new_complex(((vartype_real *) stack[sp - 1])->x,
                                ((vartype_real *) stack[sp])->x);
            }
            if (v == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            return binary_result(v);
        }
        case TYPE_COMPLEX: {
            vartype *new_x = new_real(0);
            vartype *new_y = new_real(0);
            if (new_x == NULL || new_y == NULL) {
                free_vartype(new_x);
                free_vartype(new_y);
                return ERR_INSUFFICIENT_MEMORY;
            }
            if (flags.f.polar) {
                phloat r, phi;
                generic_r2p(((vartype_complex *) stack[sp])->re,
                            ((vartype_complex *) stack[sp])->im, &r, &phi);
                if (p_isinf(r) != 0) {
                    if (flags.f.range_error_ignore)
                        r = POS_HUGE_PHLOAT;
                    else {
                        free_vartype(new_x);
                        free_vartype(new_y);
                        return ERR_OUT_OF_RANGE;
                    }
                }
                ((vartype_real *) new_y)->x = r;
                ((vartype_real *) new_x)->x = phi;
            } else {
                ((vartype_real *) new_y)->x = ((vartype_complex *) stack[sp])->re;
                ((vartype_real *) new_x)->x = ((vartype_complex *) stack[sp])->im;
            }
            return unary_two_results(new_x, new_y);
        }
        case TYPE_REALMATRIX: {
            if (stack[sp - 1]->type == TYPE_STRING)
                return ERR_ALPHA_DATA_IS_INVALID;
            else if (stack[sp - 1]->type != TYPE_REALMATRIX)
                return ERR_INVALID_TYPE;
            else {
                vartype_realmatrix *re_m = (vartype_realmatrix *) stack[sp - 1];
                vartype_realmatrix *im_m = (vartype_realmatrix *) stack[sp];
                vartype_complexmatrix *cm;
                int4 sz, i;
                if (re_m->rows != im_m->rows
                        || re_m->columns != im_m->columns)
                    return ERR_DIMENSION_ERROR;

                if (contains_strings(re_m) || contains_strings(im_m))
                    return ERR_ALPHA_DATA_IS_INVALID;

                cm = (vartype_complexmatrix *)
                                new_complexmatrix(re_m->rows, re_m->columns);
                if (cm == NULL)
                    return ERR_INSUFFICIENT_MEMORY;
                sz = re_m->rows * re_m->columns;
                if (flags.f.polar) {
                    for (i = 0; i < sz; i++) {
                        generic_p2r(re_m->array->data[i],
                                    im_m->array->data[i],
                                    &cm->array->data[2 * i],
                                    &cm->array->data[2 * i + 1]);
                    }
                } else {
                    for (i = 0; i < sz; i++) {
                        cm->array->data[2 * i] = re_m->array->data[i];
                        cm->array->data[2 * i + 1] = im_m->array->data[i];
                    }
                }
                return binary_result((vartype *) cm);
            }
        }
        case TYPE_COMPLEXMATRIX: {
            vartype_complexmatrix *cm = (vartype_complexmatrix *) stack[sp];
            int4 rows = cm->rows;
            int4 columns = cm->columns;
            int4 sz = rows * columns;
            int4 i;
            vartype_realmatrix *re_m = (vartype_realmatrix *)
                                            new_realmatrix(rows, columns);
            vartype_realmatrix *im_m = (vartype_realmatrix *)
                                            new_realmatrix(rows, columns);
            if (re_m == NULL || im_m == NULL) {
                free_vartype((vartype *) re_m);
                free_vartype((vartype *) im_m);
                return ERR_INSUFFICIENT_MEMORY;
            }
            if (flags.f.polar) {
                for (i = 0; i < sz; i++) {
                    phloat r, phi;
                    generic_r2p(cm->array->data[2 * i],
                                cm->array->data[2 * i + 1], &r, &phi);
                    if (p_isinf(r) != 0)
                        if (flags.f.range_error_ignore)
                            r = POS_HUGE_PHLOAT;
                        else {
                            free_vartype((vartype *) re_m);
                            free_vartype((vartype *) im_m);
                            return ERR_OUT_OF_RANGE;
                        }
                    re_m->array->data[i] = r;
                    im_m->array->data[i] = phi;
                }
            } else {
                for (i = 0; i < sz; i++) {
                    re_m->array->data[i] = cm->array->data[2 * i];
                    im_m->array->data[i] = cm->array->data[2 * i + 1];
                }
            }
            return unary_two_results((vartype *) im_m, (vartype *) re_m);
        }
        case TYPE_STRING:
            return ERR_ALPHA_DATA_IS_INVALID;
        default:
            return ERR_INVALID_TYPE;
    }
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
    if (err != ERR_NONE)
        return err;
    return recall_result(v);
}

/* Temporary for use by docmd_rcl_div() & docmd_rcl_mul() */
static vartype *temp_v;

static int docmd_rcl_div_completion(int error, vartype *res) {
    free_vartype(temp_v);
    if (error == ERR_NONE)
        unary_result(res);
    return error;
}

int docmd_rcl_div(arg_struct *arg) {
    int err = generic_rcl(arg, &temp_v);
    if (err != ERR_NONE)
        return err;
    err = assert_numeric(temp_v);
    if (err != ERR_NONE)
        return err;
    return generic_div(temp_v, stack[sp], docmd_rcl_div_completion);
}

static int docmd_rcl_mul_completion(int error, vartype *res) {
    free_vartype(temp_v);
    if (error == ERR_NONE)
        unary_result(res);
    return error;
}

int docmd_rcl_mul(arg_struct *arg) {
    int err = generic_rcl(arg, &temp_v);
    if (err != ERR_NONE)
        return err;
    err = assert_numeric(temp_v);
    if (err != ERR_NONE)
        return err;
    return generic_mul(temp_v, stack[sp], docmd_rcl_mul_completion);
}

int docmd_rcl_sub(arg_struct *arg) {
    vartype *v, *w;
    int err = generic_rcl(arg, &v);
    if (err != ERR_NONE)
        return err;
    err = assert_numeric(v);
    if (err != ERR_NONE)
        return err;
    err = generic_sub(v, stack[sp], &w);
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
    err = assert_numeric(v);
    if (err != ERR_NONE)
        return err;
    err = generic_add(v, stack[sp], &w);
    free_vartype(v);
    if (err == ERR_NONE)
        unary_result(w);
    return err;
}

int docmd_fix(arg_struct *arg) {
    if (arg->type == ARGTYPE_STK)
        return ERR_INVALID_DATA;
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
    print_trace();
    return ERR_NONE;
}

int docmd_sci(arg_struct *arg) {
    if (arg->type == ARGTYPE_STK)
        return ERR_INVALID_DATA;
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
    print_trace();
    return ERR_NONE;
}

int docmd_eng(arg_struct *arg) {
    if (arg->type == ARGTYPE_STK)
        return ERR_INVALID_DATA;
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
    print_trace();
    return ERR_NONE;
}

int docmd_all(arg_struct *arg) {
    flags.f.digits_bit3 = 0;
    flags.f.digits_bit2 = 0;
    flags.f.digits_bit1 = 0;
    flags.f.digits_bit0 = 0;
    flags.f.fix_or_all = 1;
    flags.f.eng_or_all = 1;
    print_trace();
    return ERR_NONE;
}

int docmd_null(arg_struct *arg) {
    return ERR_NONE;
}

int docmd_asto(arg_struct *arg) {
    int temp_alpha_length = reg_alpha_length;
    if (reg_alpha_length > 6)
        reg_alpha_length = 6;
    int err = docmd_xasto(arg);
    reg_alpha_length = temp_alpha_length;
    return err;
}

int docmd_xasto(arg_struct *arg) {
    /* Using STO to do the dirty work. Just need a bit of special care
     * in case the destination is ST X or IND ST X.
     */
    vartype *s = new_string(reg_alpha, reg_alpha_length);
    if (s == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    if (arg->type == ARGTYPE_STK && arg->val.stk == 'X') {
        // Special case for ASTO ST X
        if (sp >= 0)
            free_vartype(stack[sp]);
        else
            sp++;
        stack[sp] = s;
        return ERR_NONE;
    } else {
        int err;
        if (arg->type == ARGTYPE_IND_STK && arg->val.stk == 'X') {
            // Special case for ASTO IND ST X
            err = resolve_ind_arg(arg);
            if (err != ERR_NONE) {
                free_vartype(s);
                return err;
            }
        }
        if (sp == -1) {
            sp = 0;
            stack[sp] = s;
            err = docmd_sto(arg);
            if (sp == 0)
                sp = -1;
            else
                stack[sp] = new_real(0);
        } else {
            vartype *saved_x = stack[sp];
            stack[sp] = s;
            err = docmd_sto(arg);
            stack[sp] = saved_x;
        }
        free_vartype(s);
        return err;
    }
}

int docmd_arcl(arg_struct *arg) {
    /* Do some contortions to use docmd_rcl() to get the variable,
     * and do it without affecting the stack.
     */
    char saved_nostacklift = flags.f.stack_lift_disable;
    char saved_trace = flags.f.trace_print;
    flags.f.trace_print = 0;
    vartype *saved_x;
    if (sp == -1) {
        saved_x = NULL;
    } else {
        saved_x = dup_vartype(stack[sp]);
        if (saved_x == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        flags.f.stack_lift_disable = 1;
    }
    int err = docmd_rcl(arg);
    flags.f.stack_lift_disable = saved_nostacklift;
    flags.f.trace_print = saved_trace;
    if (err != ERR_NONE) {
        free_vartype(saved_x);
        return err;
    }
    vartype *v = stack[sp];
    if (saved_x == NULL)
        sp = -1;
    else
        stack[sp] = saved_x;

    /* Convert the variable to a string, using the same conversion
     * used when displaying the variable in 'normal' mode -- except
     * for strings, which we append with no quotes.
     */
    if (v->type == TYPE_STRING) {
        vartype_string *s = (vartype_string *) v;
        append_alpha_string(s->txt(), s->length, 0);
    } else {
        char buf[100];
        int bufptr = vartype2string(v, buf, 100);
        append_alpha_string(buf, bufptr, 0);
    }
    free_vartype(v);

    if (alpha_active() && !program_running())
        set_alpha_entry(true);
    if (flags.f.trace_print && flags.f.printer_exists)
        docmd_pra(NULL);
    return ERR_NONE;
}

int docmd_cla(arg_struct *arg) {
    reg_alpha_length = 0;
    set_alpha_entry(false);
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
    print_trace();
    return ERR_NONE;
}

int docmd_polar(arg_struct *arg) {
    flags.f.polar = 1;
    print_trace();
    return ERR_NONE;
}

int docmd_size(arg_struct *arg) {
    if (arg->type != ARGTYPE_NUM)
        return ERR_INVALID_TYPE;
    return dimension_array("REGS", 4, arg->val.num, 1, true);
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
    print_trace();
    return ERR_NONE;
}

int docmd_rdxcomma(arg_struct *arg) {
    flags.f.decimal_point = 0;
    print_trace();
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
        if (r->array->is_string[i] == 2)
            free(*(void **) &r->array->data[i]);
        r->array->is_string[i] = 0;
        r->array->data[i] = 0;
    }
    flags.f.log_fit_invalid = 0;
    flags.f.exp_fit_invalid = 0;
    flags.f.pwr_fit_invalid = 0;
    return ERR_NONE;
}

int docmd_clp(arg_struct *arg) {
    return clear_prgm(arg);
}

static int clv_helper(arg_struct *arg, bool global) {
    int err;
    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    if (arg->type == ARGTYPE_STR) {
        /* When EDITN is active, don't allow the matrix being
         * edited to be deleted. */
        if (matedit_mode == 3
                && string_equals(arg->val.text, arg->length, matedit_name, matedit_length))
            return ERR_RESTRICTED_OPERATION;
        if (!purge_var(arg->val.text, arg->length, global, !global))
            return ERR_RESTRICTED_OPERATION;
        if (!global)
            maybe_pop_indexed_matrix(arg->val.text, arg->length);
        remove_shadow(arg->val.text, arg->length);
        return ERR_NONE;
    } else
        return ERR_INVALID_TYPE;
}

int docmd_clv(arg_struct *arg) {
    return clv_helper(arg, true);
}

int docmd_lclv(arg_struct *arg) {
    return clv_helper(arg, false);
}

int docmd_clst(arg_struct *arg) {
    for (int i = 0; i <= sp; i++)
        free_vartype(stack[i]);
    clean_vartype_pools();
    if (flags.f.big_stack) {
        sp = -1;
        shrink_stack();
    } else {
        for (int i = 0; i < 4; i++)
            stack[i] = new_real(0);
    }
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
        free_long_strings(rm->array->is_string, rm->array->data, sz);
        for (i = 0; i < sz; i++)
            rm->array->data[i] = 0;
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
    docmd_clst(NULL);
    free_vartype(lastx);
    lastx = new_real(0);
    reg_alpha_length = 0;

    /* Exit all menus (even leaving the matrix editor
     * is guaranteed not to fail because there's 0 in X,
     * and that is always valid).
     */
    set_menu_return_err(MENULEVEL_APP, MENU_NONE, true);
    // Clear X again, in case EDIT was active
    if (sp >= 0) {
        free_vartype(stack[sp]);
        if (flags.f.big_stack)
            sp = -1;
        else
            stack[sp] = new_real(0);
    }

    flags.f.prgm_mode = 0;

    /* Clear all programs and variables */
    clear_all_prgms();
    goto_dot_dot(false);
    purge_all_vars();
    regs = new_realmatrix(25, 1);
    store_var("REGS", 4, regs);

    /* Clear the CUSTOM and programmable menus */
    clear_custom_menu();
    clear_prgm_menu();

    return ERR_NONE;
}

int docmd_percent(arg_struct *arg) {
    vartype_real *x = (vartype_real *) stack[sp];
    vartype_real *y = (vartype_real *) stack[sp - 1];
    phloat res = x->x * y->x;
    if (p_isinf(res)) {
        /* Try different evaluation order */
        res = (x->x / 100.0) * y->x;
        if (p_isinf(res))
            return ERR_OUT_OF_RANGE;
    } else
        res /= 100.0;
    vartype *new_x = new_real(res);
    if (new_x == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(new_x);
    return ERR_NONE;
}

int docmd_pi(arg_struct *arg) {
    vartype *v = new_real(PI);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    return recall_result(v);
}

static int mappable_to_deg(phloat x, phloat *y) {
    phloat r;
    int inf;
    r = rad_to_deg(x);
    if ((inf = p_isinf(r)) != 0) {
        if (flags.f.range_error_ignore)
            r = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    *y = r;
    return ERR_NONE;
}

int docmd_to_deg(arg_struct *arg) {
    vartype *v;
    int err = map_unary(stack[sp], &v, mappable_to_deg, NULL);
    if (err == ERR_NONE)
        unary_result(v);
    return err;
}

static int mappable_to_rad(phloat x, phloat *y) {
    *y = deg_to_rad(x);
    return ERR_NONE;
}

int docmd_to_rad(arg_struct *arg) {
    vartype *v;
    int err = map_unary(stack[sp], &v, mappable_to_rad, NULL);
    if (err == ERR_NONE)
        unary_result(v);
    return err;
}

static int mappable_to_hr(phloat x, phloat *y) {
    int neg = x < 0;
    phloat res;
#ifdef BCD_MATH
    const phloat point01(1, 100);
    const phloat point36(36, 100);
#endif
    if (neg)
        x = -x;

    if (x == x + 1)
        res = x;
    else {
        #ifdef BCD_MATH
            if (x < point01)
                res = x / point36;
            else {
                phloat xh = floor(x);
                phloat t = (x - xh) * 100;
                phloat xm = floor(t);
                phloat xs = (t - xm) * 100;
                res = xh + xm / 60 + xs / 3600;
            }
        #else
            if (x < 0.01)
                res = x / 0.36;
            else {
                int8 ix, ixhr;
                phloat h = floor(x);
                x -= h;
                //ix = (int8) (x * 1000000000000.0 + 0.5);
                x = (x * 1000000000000.0 + 0.5); ix = to_int8(x);
                ixhr = ix % 10000000000LL;
                ix /= 10000000000LL;
                ixhr += (ix % 100) * 6000000000LL;
                res = h + ixhr / 360000000000.0;
            }
        #endif
    }
    *y = neg ? -res : res;
    return ERR_NONE;
}

int docmd_to_hr(arg_struct *arg) {
    vartype *v;
    int err = map_unary(stack[sp], &v, mappable_to_hr, NULL);
    if (err == ERR_NONE)
        unary_result(v);
    return err;
}

static int mappable_to_hms(phloat x, phloat *y) {
    int neg = x < 0;
    phloat r, t;
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
    vartype *v;
    int err = map_unary(stack[sp], &v, mappable_to_hms, NULL);
    if (err == ERR_NONE)
        unary_result(v);
    return err;
}

int docmd_to_rec(arg_struct *arg) {
    if (stack[sp]->type == TYPE_REAL) {
        if (stack[sp - 1]->type == TYPE_REAL) {
            phloat r = ((vartype_real *) stack[sp])->x;
            phloat phi = ((vartype_real *) stack[sp - 1])->x;
            phloat x, y;
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
            binary_two_results(vx, vy);
            return ERR_NONE;
        } else if (stack[sp - 1]->type == TYPE_STRING)
            return ERR_ALPHA_DATA_IS_INVALID;
        else
            return ERR_INVALID_TYPE;
    } else if (stack[sp]->type == TYPE_COMPLEX) {
        vartype_complex *c = (vartype_complex *) stack[sp];
        phloat x, y;
        vartype *v;
        generic_p2r(c->re, c->im, &x, &y);
        v = new_complex(x, y);
        if (v == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        unary_result(v);
        return ERR_NONE;
    } else if (stack[sp]->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;
}

int docmd_to_pol(arg_struct *arg) {
    if (stack[sp]->type == TYPE_REAL) {
        if (stack[sp - 1]->type == TYPE_REAL) {
            phloat x = ((vartype_real *) stack[sp])->x;
            phloat y = ((vartype_real *) stack[sp - 1])->x;
            phloat r, phi;
            vartype *vx, *vy;
            generic_r2p(x, y, &r, &phi);
            if (p_isinf(r)) {
                if (flags.f.range_error_ignore)
                    r = POS_HUGE_PHLOAT;
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
            binary_two_results(vx, vy);
            return ERR_NONE;
        } else if (stack[sp - 1]->type == TYPE_STRING)
            return ERR_ALPHA_DATA_IS_INVALID;
        else
            return ERR_INVALID_TYPE;
    } else if (stack[sp]->type == TYPE_COMPLEX) {
        vartype_complex *c = (vartype_complex *) stack[sp];
        phloat r, phi;
        vartype *v;
        generic_r2p(c->re, c->im, &r, &phi);
        if (p_isinf(r)) {
            if (flags.f.range_error_ignore)
                r = POS_HUGE_PHLOAT;
            else
                return ERR_OUT_OF_RANGE;
        }
        v = new_complex(r, phi);
        if (v == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        unary_result(v);
        return ERR_NONE;
    } else if (stack[sp]->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;
}

static int mappable_ip(phloat x, phloat *y) {
    if (x < 0)
        *y = -floor(-x);
    else
        *y = floor(x);
    return ERR_NONE;
}

int docmd_ip(arg_struct *arg) {
    vartype *v;
    int err = map_unary(stack[sp], &v, mappable_ip, NULL);
    if (err == ERR_NONE)
        unary_result(v);
    return err;
}

static int mappable_fp(phloat x, phloat *y) {
    if (x < 0)
        *y = x + floor(-x);
    else
        *y = x - floor(x);
    return ERR_NONE;
}

int docmd_fp(arg_struct *arg) {
    vartype *v;
    int err = map_unary(stack[sp], &v, mappable_fp, NULL);
    if (err == ERR_NONE)
        unary_result(v);
    return err;
}

static phloat rnd_multiplier;

static int mappable_rnd_r(phloat x, phloat *y) {
    if (flags.f.fix_or_all && !flags.f.eng_or_all) {
        phloat t = x;
        int neg = t < 0;
        if (neg)
            t = -t;
        if (t >= ALWAYS_INT_FROM)
            *y = x;
        else {
            t = floor(t * rnd_multiplier + 0.5) / rnd_multiplier;
            *y = neg ? -t : t;
        }
        return ERR_NONE;
    } else {
        phloat t = x;
        int neg;
        phloat scale;
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
            if (p_isinf(t)) {
                if (flags.f.range_error_ignore)
                    *y = neg ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
                else
                    return ERR_OUT_OF_RANGE;
            } else
                *y = neg ? -t : t;
        }
        return ERR_NONE;
    }
}

static int mappable_rnd_c(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    int err = mappable_rnd_r(xre, yre);
    if (err != ERR_NONE)
        return err;
    return mappable_rnd_r(xim, yim);
}

int docmd_rnd(arg_struct *arg) {
    vartype *v;
    int err;
    int digits;
    if (flags.f.fix_or_all && flags.f.eng_or_all) {
        digits = 11;
    } else {
        digits = 0;
        if (flags.f.digits_bit3) digits += 8;
        if (flags.f.digits_bit2) digits += 4;
        if (flags.f.digits_bit1) digits += 2;
        if (flags.f.digits_bit0) digits += 1;
    }
    rnd_multiplier = pow(10.0, digits);
    err = map_unary(stack[sp], &v, mappable_rnd_r, mappable_rnd_c);
    if (err == ERR_NONE)
        unary_result(v);
    return err;
}

int docmd_abs(arg_struct *arg) {
    switch (stack[sp]->type) {
        case TYPE_REAL: {
            vartype *r;
            phloat x = ((vartype_real *) stack[sp])->x;
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
            phloat a = hypot(((vartype_complex *) stack[sp])->re,
                             ((vartype_complex *) stack[sp])->im);
            if (p_isinf(a) == 0)
                r = new_real(a);
            else if (flags.f.range_error_ignore)
                r = new_real(POS_HUGE_PHLOAT);
            else
                return ERR_OUT_OF_RANGE;
            if (r == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            unary_result(r);
            return ERR_NONE;
        }
        case TYPE_REALMATRIX: {
            if (contains_strings((vartype_realmatrix *) stack[sp]))
                return ERR_ALPHA_DATA_IS_INVALID;
            vartype_realmatrix *src;
            vartype_realmatrix *dst;
            int4 size, i;
            src = (vartype_realmatrix *) stack[sp];
            dst = (vartype_realmatrix *)
                                new_realmatrix(src->rows, src->columns);
            if (dst == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            size = src->rows * src->columns;
            for (i = 0; i < size; i++) {
                phloat x = src->array->data[i];
                if (x < 0)
                    x = -x;
                dst->array->data[i] = x;
            }
            unary_result((vartype *) dst);
            return ERR_NONE;
        }
        default:
            return ERR_INTERNAL_ERROR;
    }
}

static int mappable_sign(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    phloat h = hypot(xre, xim);
    if (h == 0) {
        *yre = 0;
        *yim = 0;
    } else if (p_isinf(h) == 0) {
        *yre = xre / h;
        *yim = xim / h;
    } else {
        xre /= 10000;
        xim /= 10000;
        h = hypot(xre, xim);
        *yre = xre / h;
        *yim = xim / h;
    }
    return ERR_NONE;
}

int docmd_sign(arg_struct *arg) {
    switch (stack[sp]->type) {
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
            vartype *r = new_real(((vartype_real *) stack[sp])->x < 0 ? -1 : 1);
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
            src = (vartype_realmatrix *) stack[sp];
            dst = (vartype_realmatrix *)
                                new_realmatrix(src->rows, src->columns);
            if (dst == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            size = src->rows * src->columns;
            for (i = 0; i < size; i++) {
                if (src->array->is_string[i] != 0)
                    dst->array->data[i] = 0;
                else
                    dst->array->data[i] = src->array->data[i] < 0 ? -1 : 1;
            }
            unary_result((vartype *) dst);
            return ERR_NONE;
        }
        case TYPE_COMPLEX:
        case TYPE_COMPLEXMATRIX: {
            vartype *v;
            map_unary(stack[sp], &v, NULL, mappable_sign);
            unary_result((vartype *) v);
            return ERR_NONE;
        }
        default:
            return ERR_INTERNAL_ERROR;
    }
}

int docmd_mod(arg_struct *arg) {
    phloat x = ((vartype_real *) stack[sp])->x;
    phloat y = ((vartype_real *) stack[sp - 1])->x;
    phloat res;
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
    return binary_result(v);
}
