/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2017  Thomas Okken
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
#include "core_display.h"
#include "core_helpers.h"
#include "core_main.h"
#include "core_math1.h"
#include "core_math2.h"
#include "core_sto_rcl.h"
#include "core_variables.h"
#include "shell.h"


/********************************************************/
/* Implementations of HP-42S built-in functions, part 2 */
/********************************************************/

static const char *virtual_flags =
    /* 00-49 */ "00000000000000000000000000010000000000000000011101"
    /* 50-99 */ "00010000000000010000000001000000000000000000000000";

int docmd_sf(arg_struct *arg) {
    int err;
    int4 num;
    err = arg_to_num(arg, &num);
    if (err != ERR_NONE)
        return err;
    if (num >= 100)
        return ERR_NONEXISTENT;
    if (num >= 36 && num <= 80)
        return ERR_RESTRICTED_OPERATION;
    if (virtual_flags[num] == '1')
        return virtual_flag_handler(FLAGOP_SF, num);
    else {
        flags.farray[num] = 1;
        if (num == 30)
            /* This is the stack_lift_disable flag.
             * Since we automatically enable stack lift after every command,
             * unless mode_disable_stack_lift is set, we must intervene for
             * this to actually have the intended effect.
             */
            mode_disable_stack_lift = true;
        return ERR_NONE;
    }
}

int docmd_cf(arg_struct *arg) {
    int err;
    int4 num;
    err = arg_to_num(arg, &num);
    if (err != ERR_NONE)
        return err;
    if (num >= 100)
        return ERR_NONEXISTENT;
    if (num >= 36 && num <= 80)
        return ERR_RESTRICTED_OPERATION;
    if (virtual_flags[num] == '1')
        return virtual_flag_handler(FLAGOP_CF, num);
    else {
        flags.farray[num] = 0;
        return ERR_NONE;
    }
}

int docmd_fs_t(arg_struct *arg) {
    int err;
    int4 num;
    err = arg_to_num(arg, &num);
    if (err != ERR_NONE)
        return err;
    if (num >= 100)
        return ERR_NONEXISTENT;
    if (virtual_flags[num] == '1')
        return virtual_flag_handler(FLAGOP_FS_T, num);
    else
        return flags.farray[num] ? ERR_YES : ERR_NO;
}

int docmd_fc_t(arg_struct *arg) {
    int err;
    int4 num;
    err = arg_to_num(arg, &num);
    if (err != ERR_NONE)
        return err;
    if (num >= 100)
        return ERR_NONEXISTENT;
    if (virtual_flags[num] == '1')
        return virtual_flag_handler(FLAGOP_FC_T, num);
    else
        return flags.farray[num] ? ERR_NO : ERR_YES;
}

int docmd_fsc_t(arg_struct *arg) {
    int err;
    int4 num;
    err = arg_to_num(arg, &num);
    if (err != ERR_NONE)
        return err;
    if (num >= 100)
        return ERR_NONEXISTENT;
    if (num >= 36 && num <= 80)
        return ERR_RESTRICTED_OPERATION;
    if (virtual_flags[num] == '1')
        return virtual_flag_handler(FLAGOP_FSC_T, num);
    else {
        err = flags.farray[num] ? ERR_YES : ERR_NO;
        flags.farray[num] = 0;
        return err;
    }
}

int docmd_fcc_t(arg_struct *arg) {
    int err;
    int4 num;
    err = arg_to_num(arg, &num);
    if (err != ERR_NONE)
        return err;
    if (num >= 100)
        return ERR_NONEXISTENT;
    if (num >= 36 && num <= 80)
        return ERR_RESTRICTED_OPERATION;
    if (virtual_flags[num] == '1')
        return virtual_flag_handler(FLAGOP_FCC_T, num);
    else {
        err = flags.farray[num] ? ERR_NO : ERR_YES;
        flags.farray[num] = 0;
        return err;
    }
}

int docmd_comb(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL && reg_y->type == TYPE_REAL) {
        phloat y = ((vartype_real *) reg_y)->x;
        phloat x = ((vartype_real *) reg_x)->x;
        phloat r = 1, q = 1;
        vartype *v;
        if (x < 0 || x != floor(x) || x == x - 1 || y < 0 || y != floor(y))
            return ERR_INVALID_DATA;
        if (y < x)
            return ERR_INVALID_DATA;
        if (x > y / 2)
            x = y - x;
        while (q <= x) {
            r *= y--;
            if (p_isinf(r)) {
                if (flags.f.range_error_ignore) {
                    r = POS_HUGE_PHLOAT;
                    break;
                } else
                    return ERR_OUT_OF_RANGE;
            }
            r /= q++;
        }
        v = new_real(r);
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

int docmd_perm(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL && reg_y->type == TYPE_REAL) {
        phloat y = ((vartype_real *) reg_y)->x;
        phloat x = ((vartype_real *) reg_x)->x;
        phloat r = 1;
        vartype *v;
        if (x < 0 || x != floor(x) || x == x - 1 || y < 0 || y != floor(y))
            return ERR_INVALID_DATA;
        if (y < x)
            return ERR_INVALID_DATA;
        while (x > 0) {
            r *= y--;
            if (p_isinf(r)) {
                if (flags.f.range_error_ignore) {
                    r = POS_HUGE_PHLOAT;
                    break;
                } else
                    return ERR_OUT_OF_RANGE;
            }
            x--;
        }
        v = new_real(r);
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

static int mappable_fact(phloat x, phloat *y) {
    phloat f = 1;
    if (x < 0 || x != floor(x))
        return ERR_INVALID_DATA;
    while (x > 1) {
        f *= x--;
        if (p_isinf(f)) {
            if (flags.f.range_error_ignore) {
                *y = POS_HUGE_PHLOAT;
                return ERR_NONE;
            } else
                return ERR_OUT_OF_RANGE;
        }
    }
    *y = f;
    return ERR_NONE;
}

int docmd_fact(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL || reg_x->type == TYPE_REALMATRIX) {
        vartype *v;
        int err = map_unary(reg_x, &v, mappable_fact, NULL);
        if (err == ERR_NONE)
            unary_result(v);
        return err;
    } else if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;
}

int docmd_gamma(arg_struct *arg) {
    if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else if (reg_x->type == TYPE_COMPLEX || reg_x->type == TYPE_COMPLEXMATRIX)
        return ERR_INVALID_TYPE;
    else {
        vartype *v;
        int err = map_unary(reg_x, &v, math_gamma, NULL);
        if (err == ERR_NONE)
            unary_result(v);
        return err;
    }
}

int docmd_ran(arg_struct *arg) {
    vartype *v = new_real(math_random());
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    recall_result(v);
    return ERR_NONE;
}

int docmd_seed(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL) {
        phloat x = ((vartype_real *) reg_x)->x;
        if (x == 0) {
            int8 s = shell_random_seed();
            if (s < 0)
                s = -s;
            s %= 100000000000000LL;
            s = s * 10 + 1;
            random_number_high = s / 10000000LL;
            random_number_low = s % 100000000LL;
            return ERR_NONE;
        }
        if (x < 0)
            x = -x;
        #ifdef BCD_MATH
            const phloat e12(1000000000000LL);
            if (x >= 1) {
                int exp = to_int(floor(log10(x)));
                Phloat mant = floor(x * pow(Phloat(10), 11 - exp) + 0.5);
                if (mant >= Phloat(1000000000000LL)) {
                    mant /= 10;
                    exp++;
                }
                x = (mant + ((exp + 1) % 100) / Phloat(100) + Phloat(1, 1000)) / e12;
            } else if (x >= Phloat(1LL, 1000000000000LL)) {
                x = floor(x * e12) / e12 + Phloat(1LL, 1000000000000000LL);
            } else {
                int exp = to_int(floor(log10(x) + 10000));
                if (exp > 9984)
                    exp = 9984;
                x = ((exp + 16) % 100) / Phloat(100000000000000LL) + Phloat(1LL, 1000000000000000LL);
            }
            random_number_high = to_int8(x * Phloat(10000000));
            random_number_low = to_int8(fmod(x * Phloat(1000000000000000LL), Phloat(100000000)));
        #else
            if (x >= 1) {
                int exp = (int) floor(log10(x));
                int8 mant = (int8) floor(x * pow(10.0, 11 - exp) + 0.5);
                if (mant >= 1000000000000LL) {
                    mant /= 10;
                    exp++;
                }
                random_number_high = mant / 100000;
                random_number_low = (mant % 100000) * 1000L + (exp + 1) % 100 * 10 + 1;
            } else if (x >= 1e-12) {
                int8 t = (int8) floor(x * 1e12);
                random_number_high = t / 100000;
                random_number_low = (t % 100000) * 1000L + 1;
            } else {
                int exp = (int) floor(log10(x) + 1000);
                if (exp > 984)
                    exp = 984;
                random_number_high = 0;
                random_number_low = (exp + 16) % 100 * 10 + 1;
            }
        #endif
        return ERR_NONE;
    } else if (arg->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;
}

int docmd_lbl(arg_struct *arg) {
    if (!flags.f.message)
        fly_goose();
    return ERR_NONE;
}

int docmd_rtn(arg_struct *arg) {
    if (program_running()) {
        int newprgm;
        int4 newpc;
        pop_rtn_addr(&newprgm, &newpc);
        if (newprgm == -3)
            return return_to_integ(0);
        else if (newprgm == -2)
            return return_to_solve(0);
        else if (newprgm == -1) {
            if (pc >= prgms[current_prgm].size)
                /* It's an END; go to line 0 */
                pc = -1;
            return ERR_STOP;
        } else {
            current_prgm = newprgm;
            pc = newpc;
            return ERR_NONE;
        }
    } else {
        clear_all_rtns();
        pc = -1;
        return ERR_NONE;
    }
}

int docmd_input(arg_struct *arg) {
    vartype *v;
    int err;
    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }

    err = generic_rcl(arg, &v);
    if (err == ERR_NONEXISTENT) {
        v = new_real(0);
        if (v == NULL)
            return ERR_INSUFFICIENT_MEMORY;
    } else if (err != ERR_NONE)
        return err;

    input_length = 0;
    input_arg = *arg;
    if (arg->type == ARGTYPE_NUM) {
        char2buf(input_name, 11, &input_length, 'R');
        if (arg->val.num < 10)
            char2buf(input_name, 11, &input_length, '0');
        input_length += int2string(arg->val.num, input_name + input_length,
                                            11 - input_length);
    } else if (arg->type == ARGTYPE_STK) {
        string2buf(input_name, 11, &input_length, "ST ", 3);
        char2buf(input_name, 11, &input_length, arg->val.stk);
    } else if (arg->type == ARGTYPE_STR) {
        string2buf(input_name, 11, &input_length, arg->val.text, arg->length);
    } else {
        free_vartype(v);
        return ERR_INVALID_TYPE;
    }

    recall_result(v);
    return ERR_STOP;
}

int view_helper(arg_struct *arg, int print) {
    int err;
    char buf[22];
    int bufptr = 0, part2;
    vartype *v;
    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    switch (arg->type) {
        case ARGTYPE_NUM: {
            int num = arg->val.num;
            char2buf(buf, 22, &bufptr, 'R');
            if (num < 10)
                char2buf(buf, 22, &bufptr, '0');
            bufptr += int2string(num, buf + bufptr, 22 - bufptr);
            break;
        }
        case ARGTYPE_STK:
            string2buf(buf, 22, &bufptr, "ST ", 3);
            char2buf(buf, 22, &bufptr, arg->val.stk);
            break;
        case ARGTYPE_STR:
            string2buf(buf, 22, &bufptr, arg->val.text, arg->length);
            break;
    }
    char2buf(buf, 22, &bufptr, '=');
    part2 = bufptr;
    err = generic_rcl(arg, &v);
    if (err != ERR_NONE)
        return err;
    bufptr += vartype2string(v, buf + bufptr, 22 - bufptr);
    free_vartype(v);
    clear_row(0);
    draw_string(0, 0, buf, bufptr);
    flush_display();
    flags.f.message = 1;
    flags.f.two_line_message = 0;

    if (print && (flags.f.printer_enable || !program_running())) {
        if (flags.f.printer_exists)
            print_wide(buf, part2, buf + part2, bufptr - part2);
        else
            return ERR_STOP;
    }
    return ERR_NONE;
}

int docmd_view(arg_struct *arg) {
    return view_helper(arg, 1);
}

static void aview_helper() {
#define DISP_ROWS 2
#define DISP_COLUMNS 22
    int line_start[DISP_ROWS];
    int line_length[DISP_ROWS];
    int line = 0;
    int i;
    line_start[0] = 0;
    for (i = 0; i < reg_alpha_length; i++) {
        if (reg_alpha[i] == 10) {
            if (line == DISP_ROWS - 1)
                break;
            line_length[line] = i - line_start[line];
            line_start[++line] = i + 1;
        } else if (i == line_start[line] + DISP_COLUMNS) {
            if (line == DISP_ROWS - 1)
                break;
            line_length[line] = i - line_start[line];
            line_start[++line] = i;
        }
    }
    line_length[line] = i - line_start[line];
    flags.f.message = 1;
    flags.f.two_line_message = line == 1;
    clear_row(0);
    if (flags.f.two_line_message || program_running())
        clear_row(1);
    for (i = 0; i <= line; i++)
        draw_string(0, i, reg_alpha + line_start[i], line_length[i]);
    flush_display();
}

int docmd_aview(arg_struct *arg) {
    aview_helper();
    if (flags.f.printer_enable || !program_running()) {
        if (flags.f.printer_exists)
            docmd_pra(arg);
        else
            return ERR_STOP;
    }
    return ERR_NONE;
}

int docmd_xeq(arg_struct *arg) {
    if (program_running()) {
        int oldprgm = current_prgm;
        int4 oldpc = pc;
        int error = docmd_gto(arg);
        if (error != ERR_NONE)
            return error;
        return push_rtn_addr(oldprgm, oldpc);
    } else {
        int err = docmd_gto(arg);
        if (err != ERR_NONE)
            return err;
        clear_all_rtns();
        return ERR_RUN;
    }
}

int docmd_prompt(arg_struct *arg) {
    aview_helper();
    if (flags.f.printer_enable && flags.f.printer_exists
            && (flags.f.trace_print || flags.f.normal_print))
        docmd_pra(arg);
    return ERR_STOP;
}

int docmd_pse(arg_struct *arg) {
    if (program_running()) {
        int saved_command = pending_command;
        pending_command = CMD_NONE;
        redisplay();
        pending_command = saved_command;
#ifdef OLD_PSE
        shell_delay(1000);
        if (mode_goose >= 0)
            mode_goose = -1 - mode_goose;
#else
        mode_pause = true;
#endif
    }
    return ERR_NONE;
}

static int generic_loop_helper(phloat *x, bool isg) {
    phloat t;
    #ifdef BCD_MATH
        phloat i;
    #else
        int8 i;
    #endif
    int4 j, k;
    int s;

    if (*x == (isg ? *x + 1 : *x - 1)) {
        /* Too big to do anything useful with; this is what the real
         * HP-42S does in this case:
         */
        return isg == (*x < 0) ? ERR_YES : ERR_NO;
    }
    /* Break number up as follows: II.JJJKKRRRRR
     * The sign goes with I; everything else is considered positive.
     */
    t = *x;
    if (t < 0) {
        t = -t;
        s = -1;
    } else
        s = 1;

    #ifdef BCD_MATH
        i = floor(t);
        t = (t - i) * 100000;
    #else
        i = to_int8(t);
        t = (t - i) * 100000;
        /* The 0.0000005 is a precaution to prevent the loop increment
         * value from being taken to be 1 lower than what the user intended;
         * this can happen because the decimal fractions used here cannot,
         * in general, be represented exactly in binary, so that what should
         * be 10.00902 may actually end up being approximated as something
         * fractionally lower -- and 10.0090199999999+ would be interpreted
         * as having a loop increment of 1, not the 2 that was intended.
         * By adding 0.0000005 before truncating, we effectively round to
         * 7 decimals, which is all that a real HP-42S would have left after
         * the multiplication by 100000. So, we sacrifice some of the range
         * of an IEEE-754 double, but maintain HP-42S compatibility.
         */
        t = t + 0.0000005;
    #endif
    k = to_int4(t);
    j = k / 100;
    k -= j * 100;
    if (k == 0)
        k = 1;

    /* Update the 'real' loop control value separately from the components
     * we have just separated out. I'm very paranoid about cumulative errors,
     * so I don't rebuild the loop control value from i, j, k, etc.
     * This way is computationally cheaper, anyway.
     */
    if (isg) {
        if (*x < 0 && *x > -k)
            *x = -(*x) + k - 2 * i;
        else
            *x += k;
    } else {
        if (*x > 0 && *x < k)
            *x = -(*x) - k + 2 * i;
        else
            *x -= k;
    }

    /* Now we do what you would expect ISG/DSE to do... */
    if (isg) {
        if (s == -1)
            i = k - i;
        else
            i = k + i;
        return i > j ? ERR_NO : ERR_YES;
    } else {
        if (s == -1)
            i = -i - k;
        else
            i = i - k;
        return i <= j ? ERR_NO : ERR_YES;
    }
}

static int generic_loop(arg_struct *arg, bool isg) {
    int err;
    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    switch (arg->type) {
        case ARGTYPE_NUM: {
            vartype *regs = recall_var("REGS", 4);
            if (regs == NULL)
                return ERR_SIZE_ERROR;
            else if (regs->type == TYPE_REALMATRIX) {
                vartype_realmatrix *rm = (vartype_realmatrix *) regs;
                int4 size = rm->rows * rm->columns;
                int4 index = arg->val.num;
                if (index >= size)
                    return ERR_SIZE_ERROR;
                if (rm->array->is_string[index])
                    return ERR_ALPHA_DATA_IS_INVALID;
                else {
                    if (!disentangle(regs))
                        return ERR_INSUFFICIENT_MEMORY;
                    return generic_loop_helper(&rm->array->data[index], isg);
                }
            } else if (regs->type == TYPE_COMPLEXMATRIX) {
                return ERR_INVALID_TYPE;
            } else {
                /* This should never happen; STO should prevent
                 * "REGS" from being any other type than a real or
                 * complex matrix.
                 */
                return ERR_INTERNAL_ERROR;
            }
        }
        case ARGTYPE_STK: {
            vartype *v;
            switch (arg->val.stk) {
                case 'X': v = reg_x; break;
                case 'Y': v = reg_y; break;
                case 'Z': v = reg_z; break;
                case 'T': v = reg_t; break;
                case 'L': v = reg_lastx; break;
            }
            if (v->type == TYPE_REAL)
                return generic_loop_helper(&((vartype_real *) v)->x, isg);
            else if (v->type == TYPE_STRING)
                return ERR_ALPHA_DATA_IS_INVALID;
            else
                return ERR_INVALID_TYPE;
        }
        case ARGTYPE_STR: {
            vartype *v = recall_var(arg->val.text, arg->length);
            if (v->type == TYPE_REAL)
                return generic_loop_helper(&((vartype_real *) v)->x, isg);
            else if (v->type == TYPE_STRING)
                return ERR_ALPHA_DATA_IS_INVALID;
            else
                return ERR_INVALID_TYPE;
        }
        default:
            return ERR_INTERNAL_ERROR;
    }
}

int docmd_isg(arg_struct *arg) {
    return generic_loop(arg, true);
}

int docmd_dse(arg_struct *arg) {
    return generic_loop(arg, false);
}

int docmd_aip(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL) {
        char buf[44];
        int size = ip2revstring(((vartype_real *) reg_x)->x, buf, 44);
        append_alpha_string(buf, size, 1);
        if (flags.f.trace_print && flags.f.printer_exists)
            docmd_pra(NULL);
        return ERR_NONE;
    } else if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;
}

int docmd_xtoa(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL) {
        phloat x = ((vartype_real *) reg_x)->x;
        if (x < 0)
            x = -x;
        if (x >= 256)
            return ERR_INVALID_DATA;
        append_alpha_char(to_char(x));
    } else if (reg_x->type == TYPE_STRING) {
        vartype_string *s = (vartype_string *) reg_x;
        append_alpha_string(s->text, s->length, 0);
    } else if (reg_x->type == TYPE_REALMATRIX) {
        vartype_realmatrix *m = (vartype_realmatrix *) reg_x;
        int4 size = m->rows * m->columns;
        int4 i;
        char buf[44];
        int buflen = 0;
        for (i = size - 1; i >= 0; i--) {
            if (m->array->is_string[i]) {
                int j;
                for (j = phloat_length(m->array->data[i]) - 1; j >= 0; j--) {
                    buf[buflen++] = phloat_text(m->array->data[i])[j];
                    if (buflen == 44)
                        goto done;
                }
            } else {
                phloat d = m->array->data[i];
                if (d < 0)
                    d = -d;
                if (d >= 256)
                    buf[buflen++] = (char) 255;
                else
                    buf[buflen++] = to_char(d);
                if (buflen == 44)
                    goto done;
            }
        }
        done:
        append_alpha_string(buf, buflen, 1);
    } else
        return ERR_INVALID_TYPE;
    if (flags.f.trace_print && flags.f.printer_exists)
        docmd_pra(NULL);
    return ERR_NONE;
}

int docmd_agraph(arg_struct *arg) {
    switch (reg_x->type) {
        case TYPE_REAL: {
            if (reg_y->type == TYPE_REAL) {
                phloat x = ((vartype_real *) reg_x)->x;
                phloat y = ((vartype_real *) reg_y)->x;
                draw_pattern(x, y, reg_alpha, reg_alpha_length);
                flush_display();
                flags.f.message = flags.f.two_line_message = 1;
                return ERR_NONE;
            } else if (reg_y->type == TYPE_STRING)
                return ERR_ALPHA_DATA_IS_INVALID;
            else
                return ERR_INVALID_TYPE;
        }
        case TYPE_COMPLEX: {
            phloat x = ((vartype_complex *) reg_x)->re;
            phloat y = ((vartype_complex *) reg_x)->im;
            draw_pattern(x, y, reg_alpha, reg_alpha_length);
            flush_display();
            flags.f.message = flags.f.two_line_message = 1;
            return ERR_NONE;
        }
        case TYPE_COMPLEXMATRIX: {
            vartype_complexmatrix *cm = (vartype_complexmatrix *) reg_x;
            int4 size = 2 * cm->rows * cm->columns;
            int4 i;
            for (i = 0; i < size; i += 2)
                draw_pattern(cm->array->data[i], cm->array->data[i + 1],
                             reg_alpha, reg_alpha_length);
            flush_display();
            flags.f.message = flags.f.two_line_message = 1;
            return ERR_NONE;
        }
        case TYPE_STRING:
            return ERR_ALPHA_DATA_IS_INVALID;
        default:
            return ERR_INVALID_TYPE;
    }
}

static void pixel_helper(phloat dx, phloat dy) {
    dx = dx < 0 ? -floor(-dx + 0.5) : floor(dx + 0.5);
    dy = dy < 0 ? -floor(-dy + 0.5) : floor(dy + 0.5);
    int x = dx < -132 ? -132 : dx > 132 ? 132 : to_int(dx);
    int y = dy < -132 ? -132 : dy > 132 ? 132 : to_int(dy);
    int i;
    int dot = 1;
    if (x < 0) {
        x = -x;
        if (x >= 1 && x <= 131)
            for (i = 0; i < 16; i++)
                draw_pixel(x - 1, i);
        dot = 0;
    }
    if (y < 0) {
        y = -y;
        if (y >= 1 && y <= 16)
            for (i = 0; i < 131; i++)
                draw_pixel(i, y - 1);
        dot = 0;
    }
    if (dot && x >= 1 && x <= 131 && y >= 1 && y <= 16)
        draw_pixel(x - 1, y - 1);
}

int docmd_pixel(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL) {
        if (reg_y->type == TYPE_REAL) {
            pixel_helper(((vartype_real *) reg_x)->x,
                         ((vartype_real *) reg_y)->x);
            flush_display();
            flags.f.message = flags.f.two_line_message = 1;
            return ERR_NONE;
        } else if (reg_y->type == TYPE_STRING)
            return ERR_ALPHA_DATA_IS_INVALID;
        else
            return ERR_INVALID_TYPE;
    } else if (reg_x->type == TYPE_COMPLEX) {
        pixel_helper(((vartype_complex *) reg_x)->re,
                     ((vartype_complex *) reg_x)->im);
        flush_display();
        flags.f.message = flags.f.two_line_message = 1;
        return ERR_NONE;
    } else if (reg_x->type == TYPE_COMPLEXMATRIX) {
        vartype_complexmatrix *m = (vartype_complexmatrix *) reg_x;
        int4 size = 2 * m->rows * m->columns;
        int4 i;
        for (i = 0; i < size; i += 2)
            pixel_helper(m->array->data[i], m->array->data[i + 1]);
        flush_display();
        flags.f.message = flags.f.two_line_message = 1;
        return ERR_NONE;
    } else if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;
}

int docmd_beep(arg_struct *arg) {
    tone(8);
    tone(5);
    tone(9);
    tone(8);
    return ERR_NONE;
}

int docmd_tone(arg_struct *arg) {
    int err;
    int4 num;
    err = arg_to_num(arg, &num);
    if (err != ERR_NONE)
        return err;
    if (num >= 10)
        return ERR_INVALID_DATA;
    tone(num);
    return ERR_NONE;
}

int docmd_mvar(arg_struct *arg) {
    return ERR_NONE;
}

int docmd_varmenu(arg_struct *arg) {
    int saved_prgm = current_prgm;
    int prgm;
    int4 pc;
    int command;
    arg_struct arg2;
    int i;

    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        int err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }

    if (!find_global_label(arg, &prgm, &pc))
        return ERR_LABEL_NOT_FOUND;
    pc += get_command_length(prgm, pc);
    current_prgm = prgm;
    get_next_command(&pc, &command, &arg2, 0);
    current_prgm = saved_prgm;
    if (command != CMD_MVAR)
        return ERR_NO_MENU_VARIABLES;
    varmenu_length = arg->length;
    for (i = 0; i < varmenu_length; i++)
        varmenu[i] = arg->val.text[i];
    varmenu_row = 0;
    varmenu_role = 0;
    return set_menu_return_err(MENULEVEL_APP, MENU_VARMENU, false);
}

int docmd_getkey(arg_struct *arg) {
    mode_getkey = true;
    return ERR_NONE;
}

int docmd_menu(arg_struct *arg) {
    set_menu(MENULEVEL_PLAIN, MENU_PROGRAMMABLE);
    mode_plainmenu_sticky = true;
    return ERR_NONE;
}

int docmd_x_eq_0(arg_struct *arg) {
    if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else if (reg_x->type != TYPE_REAL)
        return ERR_INVALID_TYPE;
    return ((vartype_real *) reg_x)->x == 0 ? ERR_YES : ERR_NO;
}

int docmd_x_ne_0(arg_struct *arg) {
    if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else if (reg_x->type != TYPE_REAL)
        return ERR_INVALID_TYPE;
    return ((vartype_real *) reg_x)->x != 0 ? ERR_YES : ERR_NO;
}

int docmd_x_lt_0(arg_struct *arg) {
    if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else if (reg_x->type != TYPE_REAL)
        return ERR_INVALID_TYPE;
    return ((vartype_real *) reg_x)->x < 0 ? ERR_YES : ERR_NO;
}

int docmd_x_gt_0(arg_struct *arg) {
    if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else if (reg_x->type != TYPE_REAL)
        return ERR_INVALID_TYPE;
    return ((vartype_real *) reg_x)->x > 0 ? ERR_YES : ERR_NO;
}

int docmd_x_le_0(arg_struct *arg) {
    if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else if (reg_x->type != TYPE_REAL)
        return ERR_INVALID_TYPE;
    return ((vartype_real *) reg_x)->x <= 0 ? ERR_YES : ERR_NO;
}

int docmd_x_ge_0(arg_struct *arg) {
    if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else if (reg_x->type != TYPE_REAL)
        return ERR_INVALID_TYPE;
    return ((vartype_real *) reg_x)->x >= 0 ? ERR_YES : ERR_NO;
}

int docmd_x_eq_y(arg_struct *arg) {
    if (reg_x->type != reg_y->type)
        return ERR_NO;
    switch (reg_x->type) {
        case TYPE_REAL: {
            vartype_real *x = (vartype_real *) reg_x;
            vartype_real *y = (vartype_real *) reg_y;
            return x->x == y->x ? ERR_YES : ERR_NO;
        }
        case TYPE_COMPLEX: {
            vartype_complex *x = (vartype_complex *) reg_x;
            vartype_complex *y = (vartype_complex *) reg_y;
            return x->re == y->re && x->im == y->im ? ERR_YES : ERR_NO;
        }
        case TYPE_REALMATRIX: {
            vartype_realmatrix *x = (vartype_realmatrix *) reg_x;
            vartype_realmatrix *y = (vartype_realmatrix *) reg_y;
            int4 sz, i;
            if (x->rows != y->rows || x->columns != y->columns)
                return ERR_NO;
            sz = x->rows * x->columns;
            for (i = 0; i < sz; i++) {
                int xstr = x->array->is_string[i];
                int ystr = y->array->is_string[i];
                if (xstr != ystr)
                    return ERR_NO;
                if (xstr) {
                    if (!string_equals(phloat_text(x->array->data[i]),
                                       phloat_length(x->array->data[i]),
                                       phloat_text(y->array->data[i]),
                                       phloat_length(y->array->data[i])))
                        return ERR_NO;
                } else {
                    if (x->array->data[i] != y->array->data[i])
                        return ERR_NO;
                }
            }
            return ERR_YES;
        }
        case TYPE_COMPLEXMATRIX: {
            vartype_complexmatrix *x = (vartype_complexmatrix *) reg_x;
            vartype_complexmatrix *y = (vartype_complexmatrix *) reg_y;
            int4 sz, i;
            if (x->rows != y->rows || x->columns != y->columns)
                return ERR_NO;
            sz = 2 * x->rows * x->columns;
            for (i = 0; i < sz; i++)
                if (x->array->data[i] != y->array->data[i])
                    return ERR_NO;
            return ERR_YES;
        }
        case TYPE_STRING: {
            vartype_string *x = (vartype_string *) reg_x;
            vartype_string *y = (vartype_string *) reg_y;
            if (string_equals(x->text, x->length, y->text, y->length))
                return ERR_YES;
            else
                return ERR_NO;
        }
        default:
            /* Can't happen */
            return ERR_INTERNAL_ERROR;
    }
}

int docmd_x_ne_y(arg_struct *arg) {
    int err = docmd_x_eq_y(arg);
    switch (err) {
        case ERR_YES:
            return ERR_NO;
        case ERR_NO:
            return ERR_YES;
        default:
            return err;
    }
}

int docmd_x_lt_y(arg_struct *arg) {
    switch (reg_x->type) {
        case TYPE_REAL: {
            if (reg_y->type == TYPE_STRING)
                return ERR_ALPHA_DATA_IS_INVALID;
            else if (reg_y->type != TYPE_REAL)
                return ERR_INVALID_TYPE;
            if (((vartype_real *) reg_x)->x < ((vartype_real *) reg_y)->x)
                return ERR_YES;
            else
                return ERR_NO;
        }
        case TYPE_STRING:
            return ERR_ALPHA_DATA_IS_INVALID;
        default:
            return ERR_INVALID_TYPE;
    }
}

int docmd_x_gt_y(arg_struct *arg) {
    switch (reg_x->type) {
        case TYPE_REAL: {
            if (reg_y->type == TYPE_STRING)
                return ERR_ALPHA_DATA_IS_INVALID;
            else if (reg_y->type != TYPE_REAL)
                return ERR_INVALID_TYPE;
            if (((vartype_real *) reg_x)->x > ((vartype_real *) reg_y)->x)
                return ERR_YES;
            else
                return ERR_NO;
        }
        case TYPE_STRING:
            return ERR_ALPHA_DATA_IS_INVALID;
        default:
            return ERR_INVALID_TYPE;
    }
}

int docmd_x_le_y(arg_struct *arg) {
    switch (reg_x->type) {
        case TYPE_REAL: {
            if (reg_y->type == TYPE_STRING)
                return ERR_ALPHA_DATA_IS_INVALID;
            else if (reg_y->type != TYPE_REAL)
                return ERR_INVALID_TYPE;
            if (((vartype_real *) reg_x)->x <= ((vartype_real *) reg_y)->x)
                return ERR_YES;
            else
                return ERR_NO;
        }
        case TYPE_STRING:
            return ERR_ALPHA_DATA_IS_INVALID;
        default:
            return ERR_INVALID_TYPE;
    }
}

int docmd_x_ge_y(arg_struct *arg) {
    switch (reg_x->type) {
        case TYPE_REAL: {
            if (reg_y->type == TYPE_STRING)
                return ERR_ALPHA_DATA_IS_INVALID;
            else if (reg_y->type != TYPE_REAL)
                return ERR_INVALID_TYPE;
            if (((vartype_real *) reg_x)->x >= ((vartype_real *) reg_y)->x)
                return ERR_YES;
            else
                return ERR_NO;
        }
        case TYPE_STRING:
            return ERR_ALPHA_DATA_IS_INVALID;
        default:
            return ERR_INVALID_TYPE;
    }
}

typedef struct {
    const char *text;
    int length;
} sigma_label_spec;

static const sigma_label_spec sigma_labels[] = {
    { "\005X=",           3 },
    { "\005X^2=",         5 },
    { "\005Y=",           3 },
    { "\005Y^2=",         5 },
    { "\005XY=",          4 },
    { "N=",               2 },
    { "\005LN(X)=",       7 },
    { "\005LN(X)^2=",     9 },
    { "\005LN(Y)=",       7 },
    { "\005LN(Y)^2=",     9 },
    { "\005LN(X)LN(Y)=", 12 },
    { "\005XLN(Y)=",      8 },
    { "\005YLN(X)=",      8 }
};

int docmd_prsigma(arg_struct *arg) {
    vartype *regs = recall_var("REGS", 4);
    vartype_realmatrix *rm;
    int nr;
    int4 size, max, i;
    char buf[100];
    int bufptr;

    if (regs == NULL)
        return ERR_NONEXISTENT;
    if (regs->type != TYPE_REALMATRIX)
        return ERR_INVALID_TYPE;
    rm = (vartype_realmatrix *) regs;
    nr = flags.f.all_sigma ? 13 : 6;
    size = rm->rows * rm->columns;
    max = mode_sigma_reg + nr;
    if (max > size)
        return ERR_SIZE_ERROR;
    if (!flags.f.printer_enable && program_running())
        return ERR_NONE;
    if (!flags.f.printer_exists)
        return ERR_PRINTING_IS_DISABLED;

    shell_annunciators(-1, -1, 1, -1, -1, -1);
    print_text(NULL, 0, 1);
    for (i = 0; i < nr; i++) {
        int4 j = i + mode_sigma_reg;
        if (rm->array->is_string[j]) {
            bufptr = 0;
            char2buf(buf, 100, &bufptr, '"');
            string2buf(buf, 100, &bufptr, phloat_text(rm->array->data[j]),
                                          phloat_length(rm->array->data[j]));
            char2buf(buf, 100, &bufptr, '"');
        } else
            bufptr = easy_phloat2string(rm->array->data[j], buf, 100, 0);
        print_wide(sigma_labels[i].text, sigma_labels[i].length, buf, bufptr);
    }
    shell_annunciators(-1, -1, 0, -1, -1, -1);
    return ERR_NONE;
}

int docmd_prp(arg_struct *arg) {
    int prgm_index;
    if (arg->type == ARGTYPE_LBLINDEX)
        prgm_index = labels[arg->val.num].prgm;
    else if (arg->type == ARGTYPE_STR) {
        if (arg->length == 0)
            prgm_index = current_prgm;
        else {
            int4 pc;
            if (!find_global_label(arg, &prgm_index, &pc))
                return ERR_LABEL_NOT_FOUND;
        }
    } else
        return ERR_INVALID_TYPE;
    if (!flags.f.printer_exists)
        return ERR_PRINTING_IS_DISABLED;
    return print_program(prgm_index, -1, -1, 0);
}

static vartype *prv_var;
static int4 prv_index;
static int prv_worker(int interrupted);

int docmd_prv(arg_struct *arg) {
    if (arg->type == ARGTYPE_IND_NUM || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        int err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    if (arg->type != ARGTYPE_STR)
        return ERR_INVALID_TYPE;
    else {
        vartype *v = recall_var(arg->val.text, arg->length);
        char lbuf[32], rbuf[100];
        int llen = 0, rlen = 0;
        if (v == NULL)
            return ERR_NONEXISTENT;
        if (!flags.f.printer_enable && program_running())
            return ERR_NONE;
        if (!flags.f.printer_exists)
            return ERR_PRINTING_IS_DISABLED;

        shell_annunciators(-1, -1, 1, -1, -1, -1);
        string2buf(lbuf, 8, &llen, arg->val.text, arg->length);
        char2buf(lbuf, 8, &llen, '=');
        rlen = vartype2string(v, rbuf, 100);
        print_wide(lbuf, llen, rbuf, rlen);

        if (v->type == TYPE_REALMATRIX || v->type == TYPE_COMPLEXMATRIX) {
            prv_var = v;
            prv_index = 0;
            mode_interruptible = prv_worker;
            mode_stoppable = true;
            return ERR_INTERRUPTIBLE;
        } else {
            shell_annunciators(-1, -1, 0, -1, -1, -1);
            return ERR_NONE;
        }
    }
}

static int prv_worker(int interrupted) {
    char lbuf[32], rbuf[100];
    int llen = 0, rlen = 0;
    int4 i, j, sz;

    if (interrupted) {
        shell_annunciators(-1, -1, 0, -1, -1, -1);
        return ERR_STOP;
    }

    if (prv_var->type == TYPE_REALMATRIX) {
        vartype_realmatrix *rm = (vartype_realmatrix *) prv_var;
        i = prv_index / rm->columns;
        j = prv_index % rm->columns;
        sz = rm->rows * rm->columns;
        llen = int2string(i + 1, lbuf, 32);
        char2buf(lbuf, 32, &llen, ':');
        llen += int2string(j + 1, lbuf + llen, 32 - llen);
        char2buf(lbuf, 32, &llen, '=');
        if (rm->array->is_string[prv_index]) {
            rlen = 0;
            char2buf(rbuf, 100, &rlen, '"');
            string2buf(rbuf, 100, &rlen, phloat_text(rm->array->data[prv_index]),
                                    phloat_length(rm->array->data[prv_index]));
            char2buf(rbuf, 100, &rlen, '"');
        } else
            rlen = easy_phloat2string(rm->array->data[prv_index],
                                        rbuf, 100, 0);
        print_wide(lbuf, llen, rbuf, rlen);
    } else /* prv_var->type == TYPE_COMPLEXMATRIX) */ {
        vartype_complexmatrix *cm = (vartype_complexmatrix *) prv_var;
        vartype_complex cpx;
        cpx.type = TYPE_COMPLEX;
        i = prv_index / cm->columns;
        j = prv_index % cm->columns;
        sz = cm->rows * cm->columns;
        llen = int2string(i + 1, lbuf, 32);
        char2buf(lbuf, 32, &llen, ':');
        llen += int2string(j + 1, lbuf + llen, 32 - llen);
        char2buf(lbuf, 32, &llen, '=');
        cpx.re = cm->array->data[2 * prv_index];
        cpx.im = cm->array->data[2 * prv_index + 1];
        rlen = vartype2string((vartype *) &cpx, rbuf, 100);
        print_wide(lbuf, llen, rbuf, rlen);
    }

    if (++prv_index < sz)
        return ERR_INTERRUPTIBLE;
    else {
        shell_annunciators(-1, -1, 0, -1, -1, -1);
        return ERR_NONE;
    }
}

int docmd_prstk(arg_struct *arg) {
    char buf[100];
    int len;
    if (!flags.f.printer_enable && program_running())
        return ERR_NONE;
    if (!flags.f.printer_exists)
        return ERR_PRINTING_IS_DISABLED;
    shell_annunciators(-1, -1, 1, -1, -1, -1);
    print_text(NULL, 0, 1);
    len = vartype2string(reg_t, buf, 100);
    print_wide("T=", 2, buf, len);
    len = vartype2string(reg_z, buf, 100);
    print_wide("Z=", 2, buf, len);
    len = vartype2string(reg_y, buf, 100);
    print_wide("Y=", 2, buf, len);
    len = vartype2string(reg_x, buf, 100);
    print_wide("X=", 2, buf, len);
    shell_annunciators(-1, -1, 0, -1, -1, -1);
    return ERR_NONE;
}

int docmd_pra(arg_struct *arg) {
    // arg == NULL if we're called to do TRACE mode auto-print
    if (arg != NULL && !flags.f.printer_enable && program_running())
        return ERR_NONE;
    if (!flags.f.printer_exists)
        return ERR_PRINTING_IS_DISABLED;
    shell_annunciators(-1, -1, 1, -1, -1, -1);
    if (reg_alpha_length == 0)
        print_text(NULL, 0, 1);
    else {
        int line_start = 0;
        int width = flags.f.double_wide_print ? 12 : 24;
        int i;
        for (i = 0; i < reg_alpha_length; i++) {
            if (reg_alpha[i] == 10) {
                print_text(reg_alpha + line_start, i - line_start, 1);
                line_start = i + 1;
            } else if (i == line_start + width) {
                print_text(reg_alpha + line_start, i - line_start, 1);
                line_start = i;
            }
        }
        if (line_start < reg_alpha_length
                || (line_start > 0 && reg_alpha[line_start - 1] == 10))
            print_text(reg_alpha + line_start,
                       reg_alpha_length - line_start, 1);
    }
    shell_annunciators(-1, -1, 0, -1, -1, -1);
    return ERR_NONE;
}

int docmd_prx(arg_struct *arg) {
    // arg == NULL if we're called to do TRACE mode auto-print
    if (arg != NULL && !flags.f.printer_enable && program_running())
        return ERR_NONE;
    if (!flags.f.printer_exists)
        return ERR_PRINTING_IS_DISABLED;
    else {
        char buf[100];
        int len;
        shell_annunciators(-1, -1, 0, -1, -1, -1);
        len = vartype2string(reg_x, buf, 100);
        if (reg_x->type == TYPE_REAL || reg_x->type == TYPE_STRING)
            print_right(buf, len, "***", 3);
        else {
            /* Normally we print X right-justified, but if it doesn't fit on
             * one line, we print it left-justified, because having the excess
             * go near the right margin looks weird and confusing.
             */
            int left = len > (flags.f.double_wide_print ? 12 : 24);
            print_lines(buf, len, left);
        }

        if (arg != NULL && (reg_x->type == TYPE_REALMATRIX
                            || reg_x->type == TYPE_COMPLEXMATRIX)) {
            prv_var = reg_x;
            prv_index = 0;
            mode_interruptible = prv_worker;
            mode_stoppable = true;
            return ERR_INTERRUPTIBLE;
        } else {
            shell_annunciators(-1, -1, 0, -1, -1, -1);
            return ERR_NONE;
        }
    }
}

static int prusr_state;
static int prusr_index;
static int prusr_worker(int interrupted);

int docmd_prusr(arg_struct *arg) {
    if (!flags.f.printer_enable && program_running())
        return ERR_NONE;
    if (!flags.f.printer_exists)
        return ERR_PRINTING_IS_DISABLED;
    else {
        shell_annunciators(-1, -1, 1, -1, -1, -1);
        print_text(NULL, 0, 1);
        prusr_state = 0;
        prusr_index = vars_count - 1;
        mode_interruptible = prusr_worker;
        mode_stoppable = true;
        return ERR_INTERRUPTIBLE;
    }
}

static int prusr_worker(int interrupted) {
    if (interrupted) {
        shell_annunciators(-1, -1, 0, -1, -1, -1);
        return ERR_STOP;
    }

    if (prusr_state == 0) {
        char lbuf[8];
        char rbuf[100];
        int llen, rlen;
        if (prusr_index < 0) {
            if (vars_count > 0)
                print_text(NULL, 0, 1);
            prusr_state = 1;
            prusr_index = 0;
            goto state1;
        }
        llen = 0;
        string2buf(lbuf, 8, &llen, vars[prusr_index].name,
                                   vars[prusr_index].length);
        char2buf(lbuf, 8, &llen, '=');
        rlen = vartype2string(vars[prusr_index].value, rbuf, 100);
        print_wide(lbuf, llen, rbuf, rlen);
        prusr_index--;
    } else {
        char buf[13];
        int len;
        state1:
        len = 0;
        if (prusr_index >= labels_count) {
            shell_annunciators(-1, -1, 0, -1, -1, -1);
            return ERR_NONE;
        }
        if (labels[prusr_index].length == 0) {
            if (prusr_index == labels_count - 1)
                string2buf(buf, 13, &len, ".END.", 5);
            else
                string2buf(buf, 13, &len, "END", 3);
        } else {
            string2buf(buf, 13, &len, "LBL \"", 5);
            string2buf(buf, 13, &len, labels[prusr_index].name,
                                      labels[prusr_index].length);
            char2buf(buf, 13, &len, '"');
        }
        print_text(buf, len, 1);
        prusr_index++;
    }
    return ERR_INTERRUPTIBLE;
}

int docmd_list(arg_struct *arg) {
    if (arg->type != ARGTYPE_NUM)
        return ERR_INVALID_TYPE;
    if (arg->val.num == 0)
        return ERR_NONE;
    if (!flags.f.printer_exists)
        return ERR_PRINTING_IS_DISABLED;
    return print_program(current_prgm, pc, arg->val.num, 0);
}

int docmd_adv(arg_struct *arg) {
    if (flags.f.printer_exists
            && (flags.f.printer_enable || !program_running())) {
        shell_annunciators(-1, -1, 1, -1, -1, -1);
        print_text(NULL, 0, 1);
        shell_annunciators(-1, -1, 0, -1, -1, -1);
    }
    return ERR_NONE;
}

int docmd_prlcd(arg_struct *arg) {
    if (!flags.f.printer_enable && program_running())
        return ERR_NONE;
    if (!flags.f.printer_exists)
        return ERR_PRINTING_IS_DISABLED;
    else {
        shell_annunciators(-1, -1, 1, -1, -1, -1);
        print_display();
        shell_annunciators(-1, -1, 0, -1, -1, -1);
        return ERR_NONE;
    }
}

int docmd_delay(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL) {
        phloat x = ((vartype_real *) reg_x)->x;
        if (x < 0)
            x = -x;
        if (x >= 1.95)
            return ERR_INVALID_DATA;
        else
            /* We don't actually use the delay value... */
            return ERR_NONE;
    } else if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;
}

int docmd_pon(arg_struct *arg) {
    flags.f.printer_exists = 1;
    flags.f.printer_enable = 1;
    return ERR_NONE;
}

int docmd_poff(arg_struct *arg) {
    flags.f.printer_exists = 0;
    flags.f.printer_enable = 0;
    return ERR_NONE;
}

int docmd_man(arg_struct *arg) {
    flags.f.trace_print = 0;
    flags.f.normal_print = 0;
    return ERR_NONE;
}

int docmd_norm(arg_struct *arg) {
    flags.f.trace_print = 0;
    flags.f.normal_print = 1;
    return ERR_NONE;
}

int docmd_trace(arg_struct *arg) {
    flags.f.trace_print = 1;
    flags.f.normal_print = 0;
    return ERR_NONE;
}

int docmd_gto(arg_struct *arg) {
    int running = program_running();
    if (!running)
        clear_all_rtns();

    if (arg->type == ARGTYPE_NUM || arg->type == ARGTYPE_LCLBL) {
        if (!running || arg->target == -1)
            arg->target = find_local_label(arg);
        if (arg->target == -2)
            return ERR_LABEL_NOT_FOUND;
        else {
            pc = arg->target;
            prgm_highlight_row = 1;
            return ERR_NONE;
        }
    }

    if (arg->type == ARGTYPE_STR) {
        int new_prgm;
        int4 new_pc;
        if (find_global_label(arg, &new_prgm, &new_pc)) {
            current_prgm = new_prgm;
            pc = new_pc;
            prgm_highlight_row = 1;
            return ERR_NONE;
        } else
            return ERR_LABEL_NOT_FOUND;
    }

    if (arg->type == ARGTYPE_IND_NUM || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        int err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
        if (arg->type == ARGTYPE_NUM) {
            int4 target_pc = find_local_label(arg);
            if (target_pc == -2)
                return ERR_LABEL_NOT_FOUND;
            else {
                pc = target_pc;
                prgm_highlight_row = 1;
                return ERR_NONE;
            }
        } else {
            int newprgm;
            int4 newpc;
            if (find_global_label(arg, &newprgm, &newpc)) {
                current_prgm = newprgm;
                pc = newpc;
                prgm_highlight_row = 1;
                return ERR_NONE;
            } else
                return ERR_LABEL_NOT_FOUND;
        }
    }

    if (arg->type == ARGTYPE_LBLINDEX) {
        int labelindex = arg->val.num;
        current_prgm = labels[labelindex].prgm;
        pc = labels[labelindex].pc;
        prgm_highlight_row = 1;
        return ERR_NONE;
    }

    return ERR_INTERNAL_ERROR;
}

int docmd_end(arg_struct *arg) {
    return docmd_rtn(arg);
}

int docmd_number(arg_struct *arg) {
    vartype *new_x = new_real(arg->val_d);
    if (new_x == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    if (flags.f.stack_lift_disable)
        free_vartype(reg_x);
    else {
        free_vartype(reg_t);
        reg_t = reg_z;
        reg_z = reg_y;
        reg_y = reg_x;
    }
    reg_x = new_x;
    return ERR_NONE;
}

int docmd_string(arg_struct *arg) {
    int append = arg->length > 0 && arg->val.text[0] == 127;
    if (append) {
        append_alpha_string(arg->val.text + 1, arg->length - 1, 0);
        if (flags.f.trace_print && flags.f.printer_exists)
            docmd_pra(NULL);
    } else {
        reg_alpha_length = 0;
        append_alpha_string(arg->val.text, arg->length, 0);
    }
    return ERR_NONE;
}

int docmd_gtodot(arg_struct *arg) {
    if (arg->type == ARGTYPE_NUM) {
        pc = line2pc(arg->val.num);
        clear_all_rtns();
        prgm_highlight_row = 1;
        return ERR_NONE;
    } else if (arg->type == ARGTYPE_STR) {
        int new_prgm;
        int4 new_pc;
        if (find_global_label(arg, &new_prgm, &new_pc)) {
            current_prgm = new_prgm;
            pc = new_pc;
            clear_all_rtns();
            prgm_highlight_row = 1;
            return ERR_NONE;
        } else
            return ERR_LABEL_NOT_FOUND;
    } else if (arg->type == ARGTYPE_LBLINDEX) {
        int labelindex = arg->val.num;
        current_prgm = labels[labelindex].prgm;
        pc = labels[labelindex].pc;
        clear_all_rtns();
        prgm_highlight_row = 1;
        return ERR_NONE;
    } else
        return ERR_INVALID_TYPE;
}

int docmd_gtodotdot(arg_struct *arg) {
    goto_dot_dot();
    return ERR_NONE;
}

int docmd_stop(arg_struct *arg) {
    return ERR_STOP;
}

int docmd_newmat(arg_struct *arg) {
    vartype *m;

    if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else if (reg_x->type != TYPE_REAL)
        return ERR_INVALID_TYPE;
    if (reg_y->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else if (reg_y->type != TYPE_REAL)
        return ERR_INVALID_TYPE;

    phloat x = ((vartype_real *) reg_x)->x;
    if (x <= -2147483648.0 || x >= 2147483648.0)
        return ERR_DIMENSION_ERROR;
    int4 xx = to_int4(x);
    if (xx == 0)
        return ERR_DIMENSION_ERROR;
    if (xx < 0)
        xx = -xx;

    phloat y = ((vartype_real *) reg_y)->x;
    if (y <= -2147483648.0 || y >= 2147483648.0)
        return ERR_DIMENSION_ERROR;
    int4 yy = to_int4(y);
    if (yy == 0)
        return ERR_DIMENSION_ERROR;
    if (yy < 0)
        yy = -yy;

    m = new_realmatrix(yy, xx);
    if (m == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    else {
        binary_result(m);
        return ERR_NONE;
    }
}

int docmd_rup(arg_struct *arg) {
    vartype *temp = reg_x;    
    reg_x = reg_t;
    reg_t = reg_z;
    reg_z = reg_y;
    reg_y = temp;
    if (flags.f.trace_print && flags.f.printer_exists)
        docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_real_t(arg_struct *arg) {
    return reg_x->type == TYPE_REAL ? ERR_YES : ERR_NO;
}

int docmd_cpx_t(arg_struct *arg) {
    return reg_x->type == TYPE_COMPLEX ? ERR_YES : ERR_NO;
}

int docmd_str_t(arg_struct *arg) {
    return reg_x->type == TYPE_STRING ? ERR_YES : ERR_NO;
}

int docmd_mat_t(arg_struct *arg) {
    return reg_x->type == TYPE_REALMATRIX
            || reg_x->type == TYPE_COMPLEXMATRIX ? ERR_YES : ERR_NO;
}

int docmd_dim_t(arg_struct *arg) {
    int4 rows, columns;
    if (reg_x->type == TYPE_REALMATRIX) {
        rows = ((vartype_realmatrix *) reg_x)->rows;
        columns = ((vartype_realmatrix *) reg_x)->columns;
    } else if (reg_x->type == TYPE_COMPLEXMATRIX) {
        rows = ((vartype_complexmatrix *) reg_x)->rows;
        columns = ((vartype_complexmatrix *) reg_x)->columns;
    } else if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;
    vartype *new_y = new_real(rows);
    vartype *new_x = new_real(columns);
    if (new_x == NULL || new_y == NULL) {
        free_vartype(new_x);
        free_vartype(new_y);
        return ERR_INSUFFICIENT_MEMORY;
    }
    free_vartype(reg_lastx);
    reg_lastx = reg_x;
    free_vartype(reg_t);
    reg_t = reg_z;
    reg_z = reg_y;
    reg_y = new_y;
    reg_x = new_x;
    return ERR_NONE;
}

static int assign_helper(int num, arg_struct *arg) {
    if (arg->type == ARGTYPE_COMMAND) {
        /* For backward compatibility only; we don't allow this type
         * of assignment command to be created anymore, but we do want
         * programs that already contain such commands to continue
         * working.
         */
        const command_spec *cs = cmdlist(arg->val.cmd);
        assign_custom_key(num, cs->name, cs->name_length);
    } else
        assign_custom_key(num, arg->val.text, arg->length);
    flags.f.local_label = 0;
    return ERR_NONE;
}

int docmd_asgn01(arg_struct *arg) {
    return assign_helper(1, arg);
}

int docmd_asgn02(arg_struct *arg) {
    return assign_helper(2, arg);
}

int docmd_asgn03(arg_struct *arg) {
    return assign_helper(3, arg);
}

int docmd_asgn04(arg_struct *arg) {
    return assign_helper(4, arg);
}

int docmd_asgn05(arg_struct *arg) {
    return assign_helper(5, arg);
}

int docmd_asgn06(arg_struct *arg) {
    return assign_helper(6, arg);
}

int docmd_asgn07(arg_struct *arg) {
    return assign_helper(7, arg);
}

int docmd_asgn08(arg_struct *arg) {
    return assign_helper(8, arg);
}

int docmd_asgn09(arg_struct *arg) {
    return assign_helper(9, arg);
}

int docmd_asgn10(arg_struct *arg) {
    return assign_helper(10, arg);
}

int docmd_asgn11(arg_struct *arg) {
    return assign_helper(11, arg);
}

int docmd_asgn12(arg_struct *arg) {
    return assign_helper(12, arg);
}

int docmd_asgn13(arg_struct *arg) {
    return assign_helper(13, arg);
}

int docmd_asgn14(arg_struct *arg) {
    return assign_helper(14, arg);
}

int docmd_asgn15(arg_struct *arg) {
    return assign_helper(15, arg);
}

int docmd_asgn16(arg_struct *arg) {
    return assign_helper(16, arg);
}

int docmd_asgn17(arg_struct *arg) {
    return assign_helper(17, arg);
}

int docmd_asgn18(arg_struct *arg) {
    return assign_helper(18, arg);
}

int docmd_on(arg_struct *arg) {
    flags.f.continuous_on = 1;
    return ERR_NONE;
}

int docmd_off(arg_struct *arg) {
#ifdef IPHONE
    if (!off_enabled()) {
        squeak();
        return ERR_STOP;
    }
#endif
    if (program_running() && no_keystrokes_yet)
        return ERR_SUSPICIOUS_OFF;
    set_running(false);
    shell_powerdown();
    return ERR_NONE;
}

int docmd_key1g(arg_struct *arg) {
    assign_prgm_key(1, 1, arg);
    return ERR_NONE;
}

int docmd_key2g(arg_struct *arg) {
    assign_prgm_key(2, 1, arg);
    return ERR_NONE;
}

int docmd_key3g(arg_struct *arg) {
    assign_prgm_key(3, 1, arg);
    return ERR_NONE;
}

int docmd_key4g(arg_struct *arg) {
    assign_prgm_key(4, 1, arg);
    return ERR_NONE;
}

int docmd_key5g(arg_struct *arg) {
    assign_prgm_key(5, 1, arg);
    return ERR_NONE;
}

int docmd_key6g(arg_struct *arg) {
    assign_prgm_key(6, 1, arg);
    return ERR_NONE;
}

int docmd_key7g(arg_struct *arg) {
    assign_prgm_key(7, 1, arg);
    return ERR_NONE;
}

int docmd_key8g(arg_struct *arg) {
    assign_prgm_key(8, 1, arg);
    return ERR_NONE;
}

int docmd_key9g(arg_struct *arg) {
    assign_prgm_key(9, 1, arg);
    return ERR_NONE;
}

int docmd_key1x(arg_struct *arg) {
    assign_prgm_key(1, 0, arg);
    return ERR_NONE;
}

int docmd_key2x(arg_struct *arg) {
    assign_prgm_key(2, 0, arg);
    return ERR_NONE;
}

int docmd_key3x(arg_struct *arg) {
    assign_prgm_key(3, 0, arg);
    return ERR_NONE;
}

int docmd_key4x(arg_struct *arg) {
    assign_prgm_key(4, 0, arg);
    return ERR_NONE;
}

int docmd_key5x(arg_struct *arg) {
    assign_prgm_key(5, 0, arg);
    return ERR_NONE;
}

int docmd_key6x(arg_struct *arg) {
    assign_prgm_key(6, 0, arg);
    return ERR_NONE;
}

int docmd_key7x(arg_struct *arg) {
    assign_prgm_key(7, 0, arg);
    return ERR_NONE;
}

int docmd_key8x(arg_struct *arg) {
    assign_prgm_key(8, 0, arg);
    return ERR_NONE;
}

int docmd_key9x(arg_struct *arg) {
    assign_prgm_key(9, 0, arg);
    return ERR_NONE;
}

int docmd_vmsto(arg_struct *arg) {
    /* STO variant that is invoked from a VARMENU */
    int err = docmd_sto(arg);
    if (err == ERR_NONE)
        err = view_helper(arg, 0);
    if (err == ERR_NONE)
        mode_varmenu = true;
    return err;
}

int docmd_vmsto2(arg_struct *arg) {
    /* Special-purpose STO variant that is invoked from the Solver's VARMENU.
     * It saves the previous value of the target variable; this feature is used
     * by docmd_vmsolve() to provide the second initial guess (the first is
     * taken from the target variable, that is, the variable named as SOLVE's
     * parameter).
     */
    vartype *v;
    if (arg->type != ARGTYPE_STR)
        return ERR_INVALID_TYPE;
    v = recall_var(arg->val.text, arg->length);
    if (v == NULL || v->type != TYPE_REAL)
        remove_shadow(arg->val.text, arg->length);
    else
        put_shadow(arg->val.text, arg->length, ((vartype_real *) v)->x);

    return docmd_vmsto(arg);
}

int docmd_sigma_reg(arg_struct *arg) {
    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        int err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    if (arg->type != ARGTYPE_NUM)
        return ERR_INTERNAL_ERROR;
    mode_sigma_reg = (int4) arg->val.num;
    if (mode_sigma_reg < 0)
        mode_sigma_reg = -mode_sigma_reg;
    return ERR_NONE;
}

int docmd_sigma_reg_t(arg_struct *arg) {
    vartype *v = new_real(mode_sigma_reg);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    recall_result(v);
    return ERR_NONE;
}

int docmd_cld(arg_struct *arg) {
    if (flags.f.message && mode_goose >= 0)
        mode_goose = -1 - mode_goose;
    flags.f.message = 0;
    flags.f.two_line_message = 0;
    return ERR_NONE;
}
