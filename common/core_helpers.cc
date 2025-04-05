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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "core_helpers.h"
#include "core_commands2.h"
#include "core_display.h"
#include "core_phloat.h"
#include "core_main.h"
#include "core_variables.h"
#include "shell.h"


int resolve_ind_arg(arg_struct *arg, char *buf, int *buflen) {
    vartype *v;
    switch (arg->type) {
        case ARGTYPE_IND_NUM: {
            vartype *regs = recall_var("REGS", 4);
            if (regs == NULL)
                return ERR_SIZE_ERROR;
            if (regs->type != TYPE_REALMATRIX)
                return ERR_INVALID_TYPE;
            else {
                vartype_realmatrix *rm = (vartype_realmatrix *) regs;
                int4 size = rm->rows * rm->columns;
                int4 num = arg->val.num;
                if (num >= size)
                    return ERR_SIZE_ERROR;
                if (rm->array->is_string[num] == 0) {
                    phloat x = rm->array->data[num];
                    if (x < 0)
                        x = -x;
                    if (x >= 2147483648.0)
                        arg->val.num = 2147483647;
                    else
                        arg->val.num = to_int4(x);
                    arg->type = ARGTYPE_NUM;
                } else {
                    char *text;
                    int4 len;
                    get_matrix_string(rm, num, &text, &len);
                    if (len == 0)
                        return ERR_RESTRICTED_OPERATION;
                    if (buf == NULL) {
                        if (len > 7)
                            return ERR_NAME_TOO_LONG;
                        arg->length = len;
                        memcpy(arg->val.text, text, len);
                    } else {
                        if (len < *buflen)
                            *buflen = len;
                        memcpy(buf, text, *buflen);
                    }
                    arg->type = ARGTYPE_STR;
                }
                return ERR_NONE;
            }
        }
        case ARGTYPE_IND_STK: {
            int idx;
            switch (arg->val.stk) {
                case 'X': idx = 0; break;
                case 'Y': idx = 1; break;
                case 'Z': idx = 2; break;
                case 'T': idx = 3; break;
                case 'L': idx = -1; break;
            }
            if (idx == -1) {
                v = lastx;
            } else {
                if (idx > sp)
                    return ERR_STACK_DEPTH_ERROR;
                v = stack[sp - idx];
            }
            goto finish_resolve;
        }
        case ARGTYPE_IND_STR: {
            v = recall_var(arg->val.text, arg->length);
            if (v == NULL)
                return ERR_NONEXISTENT;
            finish_resolve:
            if (v->type == TYPE_REAL) {
                phloat x = ((vartype_real *) v)->x;
                if (x < 0)
                    x = -x;
                if (x >= 2147483648.0)
                    arg->val.num = 2147483647;
                else
                    arg->val.num = to_int4(x);
                arg->type = ARGTYPE_NUM;
                return ERR_NONE;
            } else if (v->type == TYPE_STRING) {
                vartype_string *s = (vartype_string *) v;
                if (s->length == 0)
                    return ERR_RESTRICTED_OPERATION;
                if (buf == NULL) {
                    if (s->length > 7)
                        return ERR_NAME_TOO_LONG;
                    arg->length = s->length;
                    memcpy(arg->val.text, s->txt(), s->length);
                } else {
                    if (s->length < *buflen)
                        *buflen = s->length;
                    memcpy(buf, s->txt(), *buflen);
                }
                arg->type = ARGTYPE_STR;
                return ERR_NONE;
            } else
                return ERR_INVALID_TYPE;
        }
        default:
            /* Should not happen; this function should only
             * be called to resolve indirect arguments.
             */
            return ERR_INTERNAL_ERROR;
    }
}

int arg_to_num(arg_struct *arg, int4 *num) {
    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        int err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    if (arg->type == ARGTYPE_NUM) {
        *num = arg->val.num;
        return ERR_NONE;
    } else if (arg->type == ARGTYPE_STR)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;
}

bool dim_to_int4(vartype *dim, int4 *res) {
    phloat d = ((vartype_real *) dim)->x;
    if (d <= -2147483648.0 || d >= 2147483648.0)
        return false;
    int4 dd = to_int4(d);
    if (dd == 0)
        return false;
    if (dd < 0)
        dd = -dd;
    *res = dd - 1;
    return true;
}

int recall_result_silently(vartype *v) {
    if (flags.f.stack_lift_disable) {
        if (sp == -1)
            sp = 0;
        else
            free_vartype(stack[sp]);
    } else if (flags.f.big_stack) {
        if (!ensure_stack_capacity(1)) {
            free_vartype(v);
            return ERR_INSUFFICIENT_MEMORY;
        }
        sp++;
    } else {
        free_vartype(stack[REG_T]);
        stack[REG_T] = stack[REG_Z];
        stack[REG_Z] = stack[REG_Y];
        stack[REG_Y] = stack[REG_X];
    }
    stack[sp] = v;
    return ERR_NONE;
}

int recall_result(vartype *v) {
    int res = recall_result_silently(v);
    if (res == ERR_NONE)
        print_trace();
    return res;
}

int recall_two_results(vartype *x, vartype *y) {
    if (flags.f.big_stack) {
        bool sld = flags.f.stack_lift_disable && sp != -1;
        int off = sld ? 1 : 2;
        if (!ensure_stack_capacity(off)) {
            free_vartype(x);
            free_vartype(y);
            return ERR_INSUFFICIENT_MEMORY;
        }
        if (sld)
            free_vartype(stack[sp]);
        sp += off;
    } else {
        if (flags.f.stack_lift_disable) {
            free_vartype(stack[REG_T]);
            free_vartype(stack[REG_X]);
            stack[REG_T] = stack[REG_Z];
            stack[REG_Z] = stack[REG_Y];
        } else {
            free_vartype(stack[REG_T]);
            free_vartype(stack[REG_Z]);
            stack[REG_T] = stack[REG_Y];
            stack[REG_Z] = stack[REG_X];
        }
    }
    stack[sp - 1] = y;
    stack[sp] = x;
    print_trace();
    return ERR_NONE;
}

void unary_result(vartype *x) {
    free_vartype(lastx);
    lastx = stack[sp];
    stack[sp] = x;
    print_trace();
}

int unary_two_results(vartype *x, vartype *y) {
    if (flags.f.big_stack) {
        if (!ensure_stack_capacity(1)) {
            free_vartype(x);
            free_vartype(y);
            return ERR_INSUFFICIENT_MEMORY;
        }
        free_vartype(lastx);
        lastx = stack[sp];
        sp++;
    } else {
        free_vartype(stack[REG_T]);
        stack[REG_T] = stack[REG_Z];
        stack[REG_Z] = stack[REG_Y];
        free_vartype(lastx);
        lastx = stack[REG_X];
    }
    stack[sp - 1] = y;
    stack[sp] = x;
    print_trace();
    return ERR_NONE;
}

int binary_result(vartype *x) {
    vartype *t;
    if (!flags.f.big_stack) {
        t = dup_vartype(stack[REG_T]);
        if (t == NULL) {
            free_vartype(x);
            return ERR_INSUFFICIENT_MEMORY;
        }
    }
    free_vartype(lastx);
    lastx = stack[sp];
    free_vartype(stack[sp - 1]);
    if (flags.f.big_stack) {
        sp--;
    } else {
        stack[REG_Y] = stack[REG_Z];
        stack[REG_Z] = t;
    }
    stack[sp] = x;
    print_trace();
    return ERR_NONE;
}

void binary_two_results(vartype *x, vartype *y) {
    if (flags.f.big_stack) {
        while (sp < 1)
            stack[++sp] = NULL;
    }
    if (stack[sp] != NULL) {
        free_vartype(lastx);
        lastx = stack[sp];
    }
    free_vartype(stack[sp - 1]);
    stack[sp - 1] = y;
    stack[sp] = x;
    print_trace();
}

int ternary_result(vartype *x) {
    if (flags.f.big_stack) {
        free_vartype(lastx);
        lastx = stack[sp];
        free_vartype(stack[sp - 1]);
        free_vartype(stack[sp - 2]);
        sp -= 2;
    } else {
        vartype *tt = dup_vartype(stack[REG_T]);
        if (tt == NULL) {
            free_vartype(x);
            return ERR_INSUFFICIENT_MEMORY;
        }
        vartype *ttt = dup_vartype(stack[REG_T]);
        if (ttt == NULL) {
            free_vartype(x);
            free_vartype(tt);
            return ERR_INSUFFICIENT_MEMORY;
        }
        free_vartype(lastx);
        lastx = stack[REG_X];
        free_vartype(stack[REG_Y]);
        free_vartype(stack[REG_Z]);
        stack[REG_Y] = tt;
        stack[REG_Z] = ttt;
    }
    stack[sp] = x;
    print_trace();
    return ERR_NONE;
}

bool ensure_stack_capacity(int n) {
    if (!flags.f.big_stack)
        return true;
    if (stack_capacity > sp + n)
        return true;
    int new_capacity = stack_capacity + n + 16;
    vartype **new_stack = (vartype **) realloc(stack, new_capacity * sizeof(vartype *));
    if (new_stack == NULL)
        return false;
    stack = new_stack;
    stack_capacity = new_capacity;
    return true;
}

void shrink_stack() {
    int new_capacity = sp + 1;
    if (new_capacity < 4)
        new_capacity = 4;
    vartype **new_stack = (vartype **) realloc(stack, new_capacity * sizeof(vartype *));
    if (new_stack != NULL) {
        stack = new_stack;
        stack_capacity = new_capacity;
    }
}

phloat rad_to_angle(phloat x) {
    if (flags.f.rad)
        return x;
    else if (flags.f.grad)
        return x * (200 / PI);
    else
        return x * (180 / PI);
}

phloat rad_to_deg(phloat x) {
    return x * (180 / PI);
}

phloat deg_to_rad(phloat x) {
    return x / (180 / PI);
}

void append_alpha_char(char c) {
    if (reg_alpha_length == 44) {
        int i;
        for (i = 0; i < 43; i++)
            reg_alpha[i] = reg_alpha[i + 1];
        reg_alpha[43] = c;
    } else
        reg_alpha[reg_alpha_length++] = c;
}

void append_alpha_string(const char *buf, int buflen, int reverse) {
    int needed, i;
    if (buflen > 44) {
        if (!reverse)
            buf += buflen - 44;
        buflen = 44;
    }
    needed = reg_alpha_length + buflen - 44;
    if (needed > 0) {
        for (i = 0; i < reg_alpha_length - needed; i++)
            reg_alpha[i] = reg_alpha[i + needed];
        reg_alpha_length -= needed;
    }
    if (reverse)
        for (i = buflen - 1; i >= 0; i--)
            reg_alpha[reg_alpha_length++] = buf[i];
    else
        for (i = 0; i < buflen; i++)
            reg_alpha[reg_alpha_length++] = buf[i];
}

void string_copy(char *dst, int *dstlen, const char *src, int srclen) {
    int i;
    *dstlen = srclen;
    for (i = 0; i < srclen; i++)
        dst[i] = src[i];
}

bool string_equals(const char *s1, int s1len, const char *s2, int s2len) {
    int i;
    if (s1len != s2len)
        return false;
    for (i = 0; i < s1len; i++)
        if (s1[i] != s2[i])
            return false;
    return true;
}

int string_pos(const char *ntext, int nlen, const vartype *hs, int startpos) {
    int pos = -1;
    if (hs->type == TYPE_REAL) {
        phloat x = ((const vartype_real *) hs)->x;
        char c;
        int i;
        if (x < 0)
            x = -x;
        if (x >= 256)
            return -2;
        c = to_char(x);
        for (i = startpos; i < nlen; i++)
            if (ntext[i] == c) {
                pos = i;
                break;
            }
    } else {
        const vartype_string *s = (const vartype_string *) hs;
        if (s->length != 0) {
            int i, j;
            const char *text = s->txt();
            for (i = startpos; i < nlen - s->length + 1; i++) {
                for (j = 0; j < s->length; j++)
                    if (ntext[i + j] != text[j])
                        goto notfound;
                pos = i;
                break;
                notfound:;
            }
        }
    }
    return pos;
}

bool vartype_equals(const vartype *v1, const vartype *v2) {
    if (v1->type != v2->type)
        return false;
    switch (v1->type) {
        case TYPE_REAL: {
            const vartype_real *x = (const vartype_real *) v1;
            const vartype_real *y = (const vartype_real *) v2;
            return x->x == y->x;
        }
        case TYPE_COMPLEX: {
            const vartype_complex *x = (const vartype_complex *) v1;
            const vartype_complex *y = (const vartype_complex *) v2;
            return x->re == y->re && x->im == y->im;
        }
        case TYPE_REALMATRIX: {
            const vartype_realmatrix *x = (const vartype_realmatrix *) v1;
            const vartype_realmatrix *y = (const vartype_realmatrix *) v2;
            if (x->array == y->array)
                return true;
            int4 sz, i;
            if (x->rows != y->rows || x->columns != y->columns)
                return false;
            sz = x->rows * x->columns;
            for (i = 0; i < sz; i++) {
                int xstr = x->array->is_string[i];
                int ystr = y->array->is_string[i];
                if (xstr != ystr)
                    return false;
                if (xstr == 0) {
                    if (x->array->data[i] != y->array->data[i])
                        return false;
                } else {
                    int len1, len2;
                    const char *text1, *text2;
                    get_matrix_string(x, i, &text1, &len1);
                    get_matrix_string(y, i, &text2, &len2);
                    if (!string_equals(text1, len1, text2, len2))
                        return false;
                }
            }
            return true;
        }
        case TYPE_COMPLEXMATRIX: {
            const vartype_complexmatrix *x = (const vartype_complexmatrix *) v1;
            const vartype_complexmatrix *y = (const vartype_complexmatrix *) v2;
            if (x->array == y->array)
                return true;
            int4 sz, i;
            if (x->rows != y->rows || x->columns != y->columns)
                return false;
            sz = 2 * x->rows * x->columns;
            for (i = 0; i < sz; i++)
                if (x->array->data[i] != y->array->data[i])
                    return false;
            return true;
        }
        case TYPE_STRING: {
            const vartype_string *x = (const vartype_string *) v1;
            const vartype_string *y = (const vartype_string *) v2;
            return string_equals(x->txt(), x->length, y->txt(), y->length);
        }
        case TYPE_LIST: {
            const vartype_list *x = (const vartype_list *) v1;
            const vartype_list *y = (const vartype_list *) v2;
            if (x->array == y->array)
                return true;
            if (x->size != y->size)
                return false;
            int4 sz = x->size;
            const vartype **data1 = (const vartype **) x->array->data;
            const vartype **data2 = (const vartype **) y->array->data;
            for (int4 i = 0; i < sz; i++)
                if (!vartype_equals(data1[i], data2[i]))
                    return false;
            return true;
        }
        default:
            /* Looks like someone added a type that we're not handling yet! */
            return false;
    }
}

int anum(const char *text, int len, phloat *res) {
    char buf[50];
    int src_pos = 0;
    retry:
    bool have_mant = false;
    bool have_mant_digits = false;
    bool neg_mant = false;
    bool have_radix = false;
    bool have_exp = false;
    bool neg_exp = false;
    int exp_pos = 0;
    int buf_pos = 0;
    buf[buf_pos++] = '+';
    for (; src_pos < len; src_pos++) {
        char c = text[src_pos];
        if (!flags.f.decimal_point)
            if (c == '.')
                c = ',';
            else if (c == ',')
                c = '.';
        if (c == '+' || flags.f.thousands_separators && c == ',')
            continue;
        if (!have_mant) {
            if (c == '-') {
                neg_mant = !neg_mant;
            } else if (c >= '0' && c <= '9') {
                buf[buf_pos++] = c;
                have_mant = true;
                have_mant_digits = true;
            } else if (c == '.') {
                buf[buf_pos++] = '0';
                buf[buf_pos++] = '.';
                have_mant = true;
                have_radix = true;
            } else {
                neg_mant = false;
            }
        } else if (!have_exp) {
            if (c == '-') {
                neg_mant = !neg_mant;
            } else if (c >= '0' && c <= '9') {
                buf[buf_pos++] = c;
                have_mant_digits = true;
            } else if (c == '.') {
                if (!have_radix) {
                    buf[buf_pos++] = c;
                    have_radix = true;
                }
            } else if (c == 'E' || c == 'e' || c == 24) {
                if (!have_mant_digits)
                    goto retry;
                buf[buf_pos++] = 'e';
                exp_pos = buf_pos;
                buf[buf_pos++] = '+';
                have_exp = true;
            } else if (c == '.') {
                /* ignore */
            } else {
                if (!have_mant_digits)
                    goto retry;
                break;
            }
        } else {
            if (c == '-') {
                neg_exp = !neg_exp;
            } else if (c >= '0' && c <= '9') {
                buf[buf_pos++] = c;
            } else if (c == '.' || c == 'E' || c == 'e' || c == 24) {
                /* ignore */
            } else {
                break;
            }
        }
    }
    if (!have_mant_digits)
        return false;
    if (neg_mant)
        buf[0] = '-';
    if (have_exp && buf_pos == exp_pos + 1) {
        buf_pos -= 2;
        have_exp = false;
    }
    if (have_exp && neg_exp)
        buf[exp_pos] = '-';
    buf[buf_pos++] = 0;
    phloat p;
#ifdef BCD_MATH
    BID_UINT128 b;
    bid128_from_string(&b, buf);
    p = b;
#else
    sscanf(buf, "%le", &p);
#endif
    if (p_isnan(p))
        return false;
    if (p_isinf(p))
        p = p > 0 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
    *res = p;
    return true;
}

#if (!defined(ANDROID) && !defined(IPHONE))
static bool always_on = false;
bool shell_always_on(int ao) {
    bool ret = always_on;
    if (ao != -1)
        always_on = ao == 1;
    return ret;
}
#endif

int virtual_flag_handler(int flagop, int flagnum) {
    /* NOTE: the determination which flag numbers are handled by this
     * function is made by docmd_sf() etc.; they do this based on a constant
     * string 'virtual_flags' defined in core_globals.cc.
     */
    switch (flagnum) {
        case 27: /* custom_menu */ {
            int its_on = mode_plainmenu == MENU_CUSTOM1
                            || mode_plainmenu == MENU_CUSTOM2
                            || mode_plainmenu == MENU_CUSTOM3;
            switch (flagop) {
                case FLAGOP_SF:
                    if (!its_on)
                        set_menu(MENULEVEL_PLAIN, MENU_CUSTOM1);
                    return ERR_NONE;
                case FLAGOP_CF:
                    if (its_on)
                        set_menu(MENULEVEL_PLAIN, MENU_NONE);
                    return ERR_NONE;
                case FLAGOP_FS_T:
                    return its_on ? ERR_YES : ERR_NO;
                case FLAGOP_FC_T:
                    return its_on ? ERR_NO : ERR_YES;
                case FLAGOP_FSC_T:
                    if (its_on)
                        set_menu(MENULEVEL_PLAIN, MENU_NONE);
                    return its_on ? ERR_YES : ERR_NO;
                case FLAGOP_FCC_T:
                    if (its_on)
                        set_menu(MENULEVEL_PLAIN, MENU_NONE);
                    return its_on ? ERR_NO : ERR_YES;
                default:
                    return ERR_INTERNAL_ERROR;
            }
        }
        case 44: /* continuous on */ {
            switch (flagop) {
                case FLAGOP_FS_T:
                    return shell_always_on(-1) ? ERR_YES : ERR_NO;
                case FLAGOP_FC_T:
                    return shell_always_on(-1) ? ERR_NO : ERR_YES;
                default:
                    return ERR_INTERNAL_ERROR;
            }
        }
        case 45: /* solving */ {
            switch (flagop) {
                case FLAGOP_FS_T:
                    return solve_active() ? ERR_YES : ERR_NO;
                case FLAGOP_FC_T:
                    return solve_active() ? ERR_NO : ERR_YES;
                default:
                    return ERR_INTERNAL_ERROR;
            }
        }
        case 46: /* integrating */ {
            switch (flagop) {
                case FLAGOP_FS_T:
                    return integ_active() ? ERR_YES : ERR_NO;
                case FLAGOP_FC_T:
                    return integ_active() ? ERR_NO : ERR_YES;
                default:
                    return ERR_INTERNAL_ERROR;
            }
        }
        case 47: /* variable_menu */ {
            int its_on = mode_appmenu == MENU_VARMENU
                && (varmenu_role == 0 || varmenu_role == 3);
            switch (flagop) {
                case FLAGOP_FS_T:
                    return its_on ? ERR_YES : ERR_NO;
                case FLAGOP_FC_T:
                    return its_on ? ERR_NO : ERR_YES;
                default:
                    return ERR_INTERNAL_ERROR;
            }
        }
        case 48: /* alpha_mode */ {
            bool alpha = alpha_active();
            switch (flagop) {
                case FLAGOP_FS_T:
                    return alpha ? ERR_YES : ERR_NO;
                case FLAGOP_FC_T:
                    return alpha ? ERR_NO : ERR_YES;
                default:
                    return ERR_INTERNAL_ERROR;
            }
        }
        case 49: /* low_battery */ {
            bool lowbat = shell_low_battery();
            switch (flagop) {
                case FLAGOP_FS_T:
                    return lowbat ? ERR_YES : ERR_NO;
                case FLAGOP_FC_T:
                    return lowbat ? ERR_NO : ERR_YES;
                default:
                    return ERR_INTERNAL_ERROR;
            }
        }
        case 53: /* input */ {
            switch (flagop) {
                case FLAGOP_FS_T:
                    return input_length > 0 ? ERR_YES : ERR_NO;
                case FLAGOP_FC_T:
                    return input_length > 0 ? ERR_NO : ERR_YES;
                default:
                    return ERR_INTERNAL_ERROR;
            }
        }
        case 65: /* matrix_editor */ {
            int its_on = matedit_mode == 2 || matedit_mode == 3;
            switch (flagop) {
                case FLAGOP_FS_T:
                    return its_on ? ERR_YES : ERR_NO;
                case FLAGOP_FC_T:
                    return its_on ? ERR_NO : ERR_YES;
                default:
                    return ERR_INTERNAL_ERROR;
            }
        }
        case 75: /* programmable_menu */ {
            int its_on = mode_plainmenu == MENU_PROGRAMMABLE;
            switch (flagop) {
                case FLAGOP_FS_T:
                    return its_on ? ERR_YES : ERR_NO;
                case FLAGOP_FC_T:
                    return its_on ? ERR_NO : ERR_YES;
                default:
                    return ERR_INTERNAL_ERROR;
            }
        }
        default:
            return ERR_INTERNAL_ERROR;
    }
}

int get_base() {
    if (flags.f.prgm_mode)
        return 10;
    int base = 0;
    if (flags.f.base_bit0) base += 1;
    if (flags.f.base_bit1) base += 2;
    if (flags.f.base_bit2) base += 4;
    if (flags.f.base_bit3) base += 8;
    if (base == 1 || base == 7 || base == 15)
        return base + 1;
    else
        return 10;
}

void set_base(int base, bool a_thru_f) {
    int oldbase = 0;
    if (flags.f.base_bit0) oldbase += 1;
    if (flags.f.base_bit1) oldbase += 2;
    if (flags.f.base_bit2) oldbase += 4;
    if (flags.f.base_bit3) oldbase += 8;

    if (base == 2 || base == 8 || base == 16)
        base--;
    else
        base = 0;
    flags.f.base_bit0 = (base & 1) != 0;
    flags.f.base_bit1 = (base & 2) != 0;
    flags.f.base_bit2 = (base & 4) != 0;
    flags.f.base_bit3 = (base & 8) != 0;
    if (!a_thru_f && mode_appmenu == MENU_BASE_A_THRU_F)
        set_menu(MENULEVEL_APP, MENU_BASE);

    if (base != oldbase)
        print_trace();
}

int get_base_param(const vartype *v, int8 *n) {
    phloat x = ((vartype_real *) v)->x;
    return phloat2base(x, n) ? ERR_NONE : ERR_INVALID_DATA;
}

int base_range_check(int8 *n, bool force_wrap) {
    int wsize = effective_wsize();
    if (force_wrap || flags.f.base_wrap) {
        if (flags.f.base_signed) {
            if ((*n & (1LL << (wsize - 1))) != 0)
                *n |= -1LL << (wsize - 1);
            else
                *n &= (1LL << (wsize - 1)) - 1;
        } else {
            if (wsize < 64)
                *n &= (1ULL << wsize) - 1;
        }
    } else if (flags.f.base_signed) {
        if (wsize == 64)
            return ERR_NONE;
        int8 high = 1LL << (wsize - 1);
        int8 low = -high;
        high--;
        if (*n < low) {
            if (flags.f.range_error_ignore)
                *n = low;
            else
                return ERR_OUT_OF_RANGE;
        } else if (*n > high) {
            if (flags.f.range_error_ignore)
                *n = high;
            else
                return ERR_OUT_OF_RANGE;
        }
    } else {
        uint8 *un = (uint8 *) n;
        uint8 high = wsize == 64 ? ~0ULL : (1ULL << wsize) - 1;
        if (*un > high) {
            if (flags.f.range_error_ignore)
                *un = high;
            else
                return ERR_OUT_OF_RANGE;
        }
    }
    return ERR_NONE;
}

int effective_wsize() {
#ifdef BCD_MATH
    return mode_wsize;
#else
    return mode_wsize > 53 ? 53 : mode_wsize;
#endif
}

phloat base2phloat(int8 n) {
    if (flags.f.base_signed)
        return phloat(n);
    else
        return phloat((uint8) n);
}

bool phloat2base(phloat p, int8 *res) {
    int wsize = effective_wsize();
    if (flags.f.base_wrap) {
        phloat ip = p < 0 ? -floor(-p) : floor(p);
        phloat d = pow(phloat(2), wsize);
        phloat r = fmod(ip, d);
        if (r < 0)
            r += d;
        int8 n = (int8) to_uint8(r);
        if (flags.f.base_signed) {
            int8 m = 1LL << (wsize - 1);
            if ((n & m) != 0)
                n |= -1LL << (wsize - 1);
            else
                n &= (1LL << (wsize - 1)) - 1;
        } else {
            if (wsize < 64)
                n &= (1ULL << wsize) - 1;
        }
        *res = n;
    } else if (flags.f.base_signed) {
        phloat high = pow(phloat(2), wsize - 1);
        phloat low = -high;
        high--;
        if (p > high || p < low)
            return false;
        int8 t = to_int8(p);
        if ((t & (1LL << (wsize - 1))) != 0)
            t |= -1LL << (wsize - 1);
        else
            t &= (1LL << (wsize - 1)) - 1;
        *res = t;
    } else {
        if (p < 0)
            return false;
        phloat high = pow(phloat(2), wsize) - 1;
        if (p > high)
            return false;
        *res = (int8) to_uint8(p);
    }
    return true;
}

void print_text(const char *text, int length, bool left_justified) {
    /* TODO: check preferences so we don't waste any time generating
     * print-outs that aren't going to be accepted by the shell anyway
     */
    char buf[24];
    int width = flags.f.double_wide_print ? 12 : 24;
    int bufptr = 0, i;
    char bitmap[162];

    /* Handle left/right justification and lowercase printing */
    if (length > width)
        length = width;
    if (!left_justified)
        for (i = length; i < width; i++)
            buf[bufptr++] = ' ';
    for (i = 0; i < length; i++) {
        char c = text[i];
        if (flags.f.lowercase_print && (c & 127) >= 'A' && (c & 127) <= 'Z')
            c += 32;
        else if (c == 10)
            c = (char) 138;
        buf[bufptr++] = c;
    }

    /* Do bit-mapped printing */
    for (i = 0; i < 162; i++)
        bitmap[i] = 0;
    for (i = 0; i < bufptr; i++) {
        int j;
        const unsigned char *charbits = get_char(buf[i]);
        for (j = 0; j < 5; j++) {
            int x1 = i * 6 + j;
            int x2 = x1 + 1;
            int x, y;
            if (flags.f.double_wide_print) {
                x1 <<= 1;
                x2 <<= 1;
            }
            for (x = x1; x < x2; x++)
                for (y = 0; y < 8; y++)
                    if ((charbits[j] & (1 << y)) != 0)
                        bitmap[y * 18 + (x >> 3)] |= 1 << (x & 7);
        }
    }

    /* Handle text-mode double-width printing by inserting spaces and
     * underscores; do text-mode printing */
    if (flags.f.double_wide_print) {
        for (i = bufptr - 1; i >= 0; i--) {
            char c = buf[i];
            buf[2 * i] = c;
            buf[2 * i + 1] = c == ' ' ? ' ' : '_';
        }
        bufptr *= 2;
    }

    shell_print(buf, bufptr, bitmap, 18, 0, 0, 143, 9);
}

void print_lines(const char *text, int length, bool left_justified) {
    int line_start = 0;
    int width = flags.f.double_wide_print ? 12 : 24;
    while (line_start + width < length) {
        print_text(text + line_start, width, left_justified);
        line_start += width;
    }
    print_text(text + line_start, length - line_start, length > width ? true : left_justified);
}

void print_right(const char *left, int leftlen, const char *right, int rightlen) {
    char buf[24];
    int len;
    int width = flags.f.double_wide_print ? 12 : 24;
    int i, pad;

    if (leftlen + rightlen + 1 <= width) {
        string_copy(buf, &len, left, leftlen);
        buf[len++] = ' ';
        pad = width - len - rightlen;
        if (pad > 6 - rightlen)
            pad = 6 - rightlen;
        if (pad < 0)
            pad = 0;
        for (i = 0; i < pad; i++)
            buf[len++] = ' ';
        for (i = 0; i < rightlen; i++)
            buf[len++] = right[i];
        print_text(buf, len, false);
    } else {
        int line_start = 0;
        while (leftlen - line_start >= width) {
            print_text(left + line_start, width, true);
            line_start += width;
        }
        if (leftlen - line_start + rightlen + 1 > width) {
            print_text(left + line_start, leftlen - line_start, true);
            print_text(right, rightlen, false);
        } else {
            string_copy(buf, &len, left + line_start, leftlen - line_start);
            pad = width - len - rightlen;
            for (i = 0; i < pad; i++)
                buf[len++] = ' ';
            for (i = 0; i < rightlen; i++)
                buf[len++] = right[i];
            print_text(buf, len, true);
        }
    }
}

void print_wide(const char *left, int leftlen, const char *right, int rightlen) {
    int width = flags.f.double_wide_print ? 12 : 24;
    char buf[24];
    int bufptr = 0, i;

    if (leftlen + rightlen <= width) {
        for (i = 0; i < leftlen; i++)
            buf[bufptr++] = left[i];
        for (i = width - leftlen - rightlen; i > 0; i--)
            buf[bufptr++] = ' ';
        for (i = 0; i < rightlen; i++)
            buf[bufptr++] = right[i];
        print_text(buf, bufptr, true);
    } else {
        for (i = 0; i < leftlen; i++) {
            buf[bufptr++] = left[i];
            if (bufptr == width) {
                print_text(buf, width, true);
                bufptr = 0;
            }
        }
        for (i = 0; i < rightlen; i++) {
            buf[bufptr++] = right[i];
            if (bufptr == width) {
                print_text(buf, width, true);
                bufptr = 0;
            }
        }
        if (bufptr > 0)
            print_text(buf, bufptr, true);
    }
}

static void print_command_2(const char *text, int len) {
    shell_annunciators(-1, -1, 1, -1, -1, -1);

    if (deferred_print) {
        /* If the display mode is FIX n, and the user has not entered
         * an exponent, pad the fractional part to n, in order to get
         * decimal points to line up.
         */
        if (flags.f.fix_or_all && !flags.f.eng_or_all) {
            int i, n, dot_found = 0, ip_digits = 0, fp_digits = 0;
            char dot = flags.f.decimal_point ? '.' : ',';
            for (i = 0; i < cmdline_length; i++) {
                char c = cmdline[i];
                if (c == 24)
                    goto never_mind;
                if (c == dot)
                    dot_found = 1;
                if (c >= '0' && c <= '9') {
                    if (dot_found)
                        fp_digits++;
                    else
                        ip_digits++;
                }
            }
            n = 0;
            if (flags.f.digits_bit3) n += 8;
            if (flags.f.digits_bit2) n += 4;
            if (flags.f.digits_bit1) n += 2;
            if (flags.f.digits_bit0) n += 1;
            if (!dot_found)
                cmdline[cmdline_length++] = dot;
            while (fp_digits < n && ip_digits + fp_digits < 12) {
                cmdline[cmdline_length++] = '0';
                fp_digits++;
            }
            never_mind:;
        }
        print_right(cmdline, cmdline_length, text, len);
    } else {
        /* Normally we print commands right-justified, but if they don't fit
         * on one line, we print them left-justified, because having the excess
         * go near the right margin looks weird and confusing.
         */
        bool left = len > (flags.f.double_wide_print ? 12 : 24);
        print_lines(text, len, left);
    }

    deferred_print = 0;
    shell_annunciators(-1, -1, 0, -1, -1, -1);
}

void print_command(int cmd, const arg_struct *arg) {
    char buf[100];
    int bufptr = 0;

    if (cmd == CMD_NULL && !deferred_print)
        return;

    if (cmd != CMD_NULL)
        bufptr += command2buf(buf, 100, cmd, arg);

    print_command_2(buf, bufptr);
}

void print_menu_trace(const char *name, int len) {
    if (!flags.f.prgm_mode
            && (flags.f.trace_print || flags.f.normal_print) && flags.f.printer_exists)
        print_command_2(name, len);
}

void print_menu_trace_always(const char *name, int len) {
    if ((flags.f.trace_print || flags.f.normal_print) && flags.f.printer_exists)
        print_command_2(name, len);
}

void print_trace() {
    if (flags.f.trace_print && flags.f.printer_exists)
        if (flags.f.normal_print || sp == -1)
            docmd_prstk(NULL);
        else
            docmd_prx(NULL);
}

void print_stack_trace() {
    if (flags.f.trace_print && flags.f.normal_print && flags.f.printer_exists)
        docmd_prstk(NULL);
}

void generic_r2p(phloat re, phloat im, phloat *r, phloat *phi) {
    if (im == 0) {
        if (re >= 0) {
            *r = re;
            *phi = 0;
        } else {
            *r = -re;
            *phi = flags.f.rad ? PI : flags.f.grad ? 200 : 180;
        }
    } else if (re == 0) {
        if (im > 0) {
            *r = im;
            *phi = flags.f.rad ? PI / 2 : flags.f.grad ? 100 : 90;
        } else {
            *r = -im;
            *phi = flags.f.rad ? -PI / 2 : flags.f.grad ? -100 : -90;
        }
    } else {
        *r = hypot(re, im);
        *phi = rad_to_angle(atan2(im, re));
    }
}

void generic_p2r(phloat r, phloat phi, phloat *re, phloat *im) {
    phloat tre, tim;
    if (flags.f.rad) {
        p_sincos(phi, &tim, &tre);
    } else if (flags.f.grad) {
        phi = fmod(phi, 400);
        if (phi < 0)
            phi += 400;
        if (phi == 0) {
            tre = 1;
            tim = 0;
        } else if (phi == 100) {
            tre = 0;
            tim = 1;
        } else if (phi == 200) {
            tre = -1;
            tim = 0;
        } else if (phi == 300) {
            tre = 0;
            tim = -1;
        } else {
            tre = cos_grad(phi);
            tim = sin_grad(phi);
        }
    } else {
        phi = fmod(phi, 360);
        if (phi < 0)
            phi += 360;
        if (phi == 0) {
            tre = 1;
            tim = 0;
        } else if (phi == 90) {
            tre = 0;
            tim = 1;
        } else if (phi == 180) {
            tre = -1;
            tim = 0;
        } else if (phi == 270) {
            tre = 0;
            tim = -1;
        } else {
            tre = cos_deg(phi);
            tim = sin_deg(phi);
        }
    }
    *re = r * tre;
    *im = r * tim;
}

static phloat sin_or_cos_deg(phloat x, bool do_sin) {
    bool neg = false;
    if (x < 0) {
        x = -x;
        if (do_sin)
            neg = true;
    }
    x = fmod(x, 360);
    if (x >= 180) {
        x -= 180;
        neg = !neg;
    }
    if (x >= 90) {
        x -= 90;
        do_sin = !do_sin;
        if (do_sin)
            neg = !neg;
    }
    phloat r;
    if (x == 45)
        r = sqrt(phloat(0.5));
    else {
        if (x > 45) {
            x = 90 - x;
            do_sin = !do_sin;
        }
        x /= 180 / PI;
        r = do_sin ? sin(x) : cos(x);
    }
    return neg ? -r : r;
}

phloat sin_deg(phloat x) {
    return sin_or_cos_deg(x, true);
}

phloat cos_deg(phloat x) {
    return sin_or_cos_deg(x, false);
}

static phloat sin_or_cos_grad(phloat x, bool do_sin) {
    bool neg = false;
    if (x < 0) {
        x = -x;
        if (do_sin)
            neg = true;
    }
    x = fmod(x, 400);
    if (x >= 200) {
        x -= 200;
        neg = !neg;
    }
    if (x >= 100) {
        x -= 100;
        do_sin = !do_sin;
        if (do_sin)
            neg = !neg;
    }
    phloat r;
    if (x == 50)
        r = sqrt(phloat(0.5));
    else {
        if (x > 50) {
            x = 100 - x;
            do_sin = !do_sin;
        }
        x /= 200 / PI;
        r = do_sin ? sin(x) : cos(x);
    }
    return neg ? -r : r;
}

phloat sin_grad(phloat x) {
    return sin_or_cos_grad(x, true);
}

phloat cos_grad(phloat x) {
    return sin_or_cos_grad(x, false);
}

int dimension_array(const char *name, int namelen, int4 rows, int4 columns, bool check_matedit) {
    int idx = lookup_var(name, namelen);
    if (check_matedit
            && (matedit_mode == 1 || matedit_mode == 3)
            && idx != -1 && vars[idx].level == matedit_level
            && string_equals(name, namelen, matedit_name, matedit_length)) {
        if (matedit_mode == 1)
            matedit_i = matedit_j = 0;
        else
            return ERR_RESTRICTED_OPERATION;
    }

    vartype *matrix = idx == -1 ? NULL : vars[idx].value;
    /* NOTE: 'size' will only ever be 0 when we're called from
     * docmd_size(); docmd_dim() does not allow 0-size matrices.
     */
    int4 size = rows * columns;
    if (matrix == NULL || (matrix->type != TYPE_REALMATRIX
                        && matrix->type != TYPE_COMPLEXMATRIX
                        && matrix->type != TYPE_LIST)) {
        vartype *newmatrix;
        if (size == 0)
            return ERR_NONE;
        newmatrix = new_realmatrix(rows, columns);
        if (newmatrix == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        int err = store_var(name, namelen, newmatrix);
        if (err != ERR_NONE)
            free_vartype(newmatrix);
        return err;
    } else if (size == 0) {
        purge_var(name, namelen);
        return ERR_NONE;
    } else
        return dimension_array_ref(matrix, rows, columns);
}

int dimension_array_ref(vartype *matrix, int4 rows, int4 columns) {
    int4 size = rows * columns;
    if (matrix->type == TYPE_REALMATRIX) {
        vartype_realmatrix *oldmatrix = (vartype_realmatrix *) matrix;
        if (oldmatrix->rows == rows && oldmatrix->columns == columns)
            return ERR_NONE;
        if (oldmatrix->array->refcount == 1) {
            int4 oldsize = oldmatrix->rows * oldmatrix->columns;
            if (size == oldsize) {
                /* Easy case! */
                oldmatrix->rows = rows;
                oldmatrix->columns = columns;
                return ERR_NONE;
            } else if (size < oldsize) {
                /* Also pretty easy, shrinking means we don't have to worry
                 * about allocation failures. We do deal with realloc()
                 * failures, because technically, realloc() can fail even when
                 * shrinking, but that is easy to handle by simply hanging onto
                 * the existing block.
                 */
                free_long_strings(oldmatrix->array->is_string + size, oldmatrix->array->data + size, oldsize - size);
                char *new_is_string = (char *) realloc(oldmatrix->array->is_string, size);
                if (new_is_string != NULL)
                    oldmatrix->array->is_string = new_is_string;
                phloat *new_data = (phloat *) realloc(oldmatrix->array->data, size * sizeof(phloat));
                if (new_data != NULL)
                    oldmatrix->array->data = new_data;
                oldmatrix->rows = rows;
                oldmatrix->columns = columns;
                return ERR_NONE;
            }
            /* Since there are no shared references to this array,
             * I can modify it in place using a realloc(). However, I
             * only use realloc() on the 'data' array, not on the
             * 'is_string' array -- if I used it on both, and the second
             * call fails, I might be unable to roll back the first.
             * So, playing safe -- shouldn't be too big a handicap since
             * 'is_string' is a lot smaller than 'data', so the transient
             * memory overhead is only about 12.5%.
             */
            char *new_is_string = (char *) malloc(size);
            if (new_is_string == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            phloat *new_data = (phloat *) realloc(oldmatrix->array->data, size * sizeof(phloat));
            if (new_data == NULL) {
                free(new_is_string);
                return ERR_INSUFFICIENT_MEMORY;
            }
            memcpy(new_is_string, oldmatrix->array->is_string, oldsize);
            for (int4 i = oldsize; i < size; i++) {
                new_is_string[i] = 0;
                new_data[i] = 0;
            }
            free(oldmatrix->array->is_string);
            oldmatrix->array->is_string = new_is_string;
            oldmatrix->array->data = new_data;
            oldmatrix->rows = rows;
            oldmatrix->columns = columns;
            return ERR_NONE;
        } else {
            /* There are shared references to the matrix. This means I
             * can't realloc() it; I'm going to allocate a brand-new instance,
             * and copy the contents from the old instance. I don't use
             * disentangle(); that's only useful if you want to eliminate
             * shared references without resizing.
             */
            realmatrix_data *new_array;
            int4 i, s, oldsize;
            new_array = (realmatrix_data *) malloc(sizeof(realmatrix_data));
            if (new_array == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            new_array->data = (phloat *) malloc(size * sizeof(phloat));
            if (new_array->data == NULL) {
                free(new_array);
                return ERR_INSUFFICIENT_MEMORY;
            }
            new_array->is_string = (char *) malloc(size);
            if (new_array->is_string == NULL) {
                nomem:
                free(new_array->data);
                free(new_array);
                return ERR_INSUFFICIENT_MEMORY;
            }
            oldsize = oldmatrix->rows * oldmatrix->columns;
            s = oldsize < size ? oldsize : size;
            for (i = 0; i < s; i++) {
                new_array->is_string[i] = oldmatrix->array->is_string[i];
                if (oldmatrix->array->is_string[i] == 2) {
                    int4 *sp = *(int4 **) &oldmatrix->array->data[i];
                    int4 *dp = (int4 *) malloc(*sp + 4);
                    if (dp == NULL) {
                        free_long_strings(new_array->is_string, new_array->data, i);
                        free(new_array->is_string);
                        goto nomem;
                    }
                    memcpy(dp, sp, *sp + 4);
                    *(int4 **) &new_array->data[i] = dp;
                } else {
                    new_array->data[i] = oldmatrix->array->data[i];
                }
            }
            for (i = s; i < size; i++) {
                new_array->is_string[i] = 0;
                new_array->data[i] = 0;
            }
            new_array->refcount = 1;
            oldmatrix->array->refcount--;
            oldmatrix->array = new_array;
            oldmatrix->rows = rows;
            oldmatrix->columns = columns;
            return ERR_NONE;
        }
    } else if (matrix->type == TYPE_COMPLEXMATRIX) {
        vartype_complexmatrix *oldmatrix = (vartype_complexmatrix *) matrix;
        if (oldmatrix->rows == rows && oldmatrix->columns == columns)
            return ERR_NONE;
        if (oldmatrix->array->refcount == 1) {
            /* Since there are no shared references to this array,
             * I can modify it in place using a realloc().
             */
            int4 i, oldsize;
            phloat *new_data = (phloat *)
                    realloc(oldmatrix->array->data, 2 * size * sizeof(phloat));
            if (new_data == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            oldsize = oldmatrix->rows * oldmatrix->columns;
            for (i = 2 * oldsize; i < 2 * size; i++)
                new_data[i] = 0;
            oldmatrix->array->data = new_data;
            oldmatrix->rows = rows;
            oldmatrix->columns = columns;
            return ERR_NONE;
        } else {
            /* There are shared references to the matrix. This means I
             * can't realloc() it; I'm going to allocate a brand-new instance,
             * and copy the contents from the old instance. I don't use
             * disentangle(); that's only useful if you want to eliminate
             * shared references without resizing.
             */
            complexmatrix_data *new_array;
            int4 i, s, oldsize;
            new_array = (complexmatrix_data *)
                                        malloc(sizeof(complexmatrix_data));
            if (new_array == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            new_array->data = (phloat *) malloc(2 * size * sizeof(phloat));
            if (new_array->data == NULL) {
                free(new_array);
                return ERR_INSUFFICIENT_MEMORY;
            }
            oldsize = oldmatrix->rows * oldmatrix->columns;
            s = oldsize < size ? oldsize : size;
            for (i = 0; i < 2 * s; i++)
                new_array->data[i] = oldmatrix->array->data[i];
            for (i = 2 * s; i < 2 * size; i++)
                new_array->data[i] = 0;
            new_array->refcount = 1;
            oldmatrix->array->refcount--;
            oldmatrix->array = new_array;
            oldmatrix->rows = rows;
            oldmatrix->columns = columns;
            return ERR_NONE;
        }
    } else /* matrix->type == TYPE_LIST */ {
        if (columns != 1)
            return ERR_DIMENSION_ERROR;
        vartype_list *oldlist = (vartype_list *) matrix;
        if (oldlist->size == size)
            return ERR_NONE;
        if (oldlist->array->refcount == 1) {
            /* Since there are no shared references to this array,
             * I can modify it in place using a realloc().
             */
            if (oldlist->size > size) {
                for (int4 i = size; i < oldlist->size; i++) {
                    free_vartype(oldlist->array->data[i]);
                    oldlist->array->data[i] = NULL;
                }
                vartype **new_data = (vartype **) realloc(oldlist->array->data, size * sizeof(vartype *));
                /* Note: If the realloc() fails to shrink the array, we just keep
                 * using the existing one, basically pretending that it succeeded.
                 */
                if (new_data != NULL)
                    oldlist->array->data = new_data;
                oldlist->size = size;
                return ERR_NONE;
            } else {
                vartype **new_data = (vartype **) realloc(oldlist->array->data, size * sizeof(vartype *));
                if (new_data == NULL)
                    return ERR_INSUFFICIENT_MEMORY;
                for (int4 i = oldlist->size; i < size; i++) {
                    new_data[i] = new_real(0);
                    if (new_data[i] == NULL) {
                        /* Argh. Roll back everything and give up. */
                        for (int4 j = oldlist->size; j < i; j++) {
                            free_vartype(new_data[j]);
                            new_data[j] = NULL;
                        }
                        vartype **reverted_data = (vartype **) realloc(new_data, oldlist->size * sizeof(vartype *));
                        oldlist->array->data = reverted_data == NULL ? new_data : reverted_data;
                        return ERR_INSUFFICIENT_MEMORY;
                    }
                }
                oldlist->array->data = new_data;
                oldlist->size = size;
                return ERR_NONE;
            }
        } else {
            /* There are shared references to the list. This means I
             * can't realloc() it; I'm going to allocate a brand-new instance,
             * and copy the contents from the old instance. I don't use
             * disentangle(); that's only useful if you want to eliminate
             * shared references without resizing.
             */
            list_data *new_array = (list_data *) malloc(sizeof(list_data));
            if (new_array == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            new_array->data = (vartype **) malloc(size * sizeof(vartype *));
            if (new_array->data == NULL) {
                free(new_array);
                return ERR_INSUFFICIENT_MEMORY;
            }
            for (int4 i = 0; i < size; i++) {
                new_array->data[i] = i < oldlist->size ? dup_vartype(oldlist->array->data[i]) : new_real(0);
                if (new_array->data[i] == NULL) {
                    for (int4 j = 0; j < i; j++)
                        free_vartype(new_array->data[j]);
                    free(new_array->data);
                    free(new_array);
                    return ERR_INSUFFICIENT_MEMORY;
                }
            }
            new_array->refcount = 1;
            oldlist->array->refcount--;
            oldlist->array = new_array;
            oldlist->size = size;
            return ERR_NONE;
        }
    }
}

phloat fix_hms(phloat x) {
#ifdef BCD_MATH
    const phloat sec_corr(4, 1000);
    const phloat min_corr(4, 10);
#endif

    bool neg = x < 0;
    if (neg)
        x = -x;
    if (x == x + 1)
        return neg ? -x : x;
    if (x < 0.0059)
        return neg ? -x : x;
    #ifdef BCD_MATH
        if (floor(fmod(x * 10000, 100)) == 60)
            x += sec_corr;
        if (floor(fmod(x * 100, 100)) == 60) {
            x += min_corr;
            if (floor(fmod(x * 10000, 100)) == 60)
                x += sec_corr;
        }
    #else
        if (to_int8(x * 10000) % 100 == 60)
            x += 0.004;
        if (to_int8(x * 100) % 100 == 60) {
            x += 0.4;
            if (to_int8(x * 10000) % 100 == 60)
                x += 0.004;
        }
    #endif
    return neg ? -x : x;
}

void char2buf(char *buf, int buflen, int *bufptr, char c) {
    if (*bufptr < buflen)
        buf[(*bufptr)++] = c;
    else
        buf[buflen - 1] = 26;
}

void string2buf(char *buf, int buflen, int *bufptr, const char *s, int slen) {
    int i;
    for (i = 0; i < slen; i++)
        char2buf(buf, buflen, bufptr, s[i]);
}

int uint2string(uint4 n, char *buf, int buflen) {
    uint4 pt = 1;
    int count = 0;
    while (n / pt >= 10)
        pt *= 10;
    while (pt != 0) {
        char2buf(buf, buflen, &count, (char) ('0' + (n / pt) % 10));
        pt /= 10;
    }
    return count;
}

int int2string(int4 n, char *buf, int buflen) {
    uint4 u;
    int count = 0;
    if (n < 0) {
        char2buf(buf, buflen, &count, '-');
        u = -n;
    } else
        u = n;
    return count + uint2string(u, buf + count, buflen - count);
}

int ulong2string(uint8 n, char *buf, int buflen) {
    uint8 pt = 1;
    int count = 0;
    while (n / pt >= 10)
        pt *= 10;
    while (pt != 0) {
        char2buf(buf, buflen, &count, (char) ('0' + (n / pt) % 10));
        pt /= 10;
    }
    return count;
}

int vartype2string(const vartype *v, char *buf, int buflen, int max_mant_digits) {
    int dispmode;
    int digits = 0;

    if (flags.f.fix_or_all)
        dispmode = flags.f.eng_or_all ? 3 : 0;
    else
        dispmode = flags.f.eng_or_all ? 2 : 1;
    if (flags.f.digits_bit3)
        digits += 8;
    if (flags.f.digits_bit2)
        digits += 4;
    if (flags.f.digits_bit1)
        digits += 2;
    if (flags.f.digits_bit0)
        digits += 1;

    switch (v->type) {

        case TYPE_REAL:
            return phloat2string(((vartype_real *) v)->x, buf, buflen,
                                 1, digits, dispmode,
                                 flags.f.thousands_separators,
                                 max_mant_digits);

        case TYPE_COMPLEX: {
            phloat x, y;
            char x_buf[22];
            int x_len;
            char y_buf[22];
            int y_len;
            int i, ret_len;

            if (flags.f.polar) {
                generic_r2p(((vartype_complex *) v)->re,
                            ((vartype_complex *) v)->im, &x, &y);
                if (p_isinf(x))
                    x = POS_HUGE_PHLOAT;
            } else {
                x = ((vartype_complex *) v)->re;
                y = ((vartype_complex *) v)->im;
            }

            x_len = phloat2string(x, x_buf, 22,
                                  0, digits, dispmode,
                                  flags.f.thousands_separators,
                                  max_mant_digits);
            y_len = phloat2string(y, y_buf, 22,
                                  0, digits, dispmode,
                                  flags.f.thousands_separators,
                                  max_mant_digits);

            if (x_len + y_len + 2 > buflen) {
                /* Too long? Fall back on ENG 2 */
                x_len = phloat2string(x, x_buf, 22,
                                      0, 2, 2,
                                      flags.f.thousands_separators,
                                      max_mant_digits);
                y_len = phloat2string(y, y_buf, 22,
                                      0, 2, 2,
                                      flags.f.thousands_separators,
                                      max_mant_digits);
            }

            for (i = 0; i < buflen; i++) {
                if (i < x_len)
                    buf[i] = x_buf[i];
                else if (i < x_len + 1)
                    buf[i] = ' ';
                else if (i < x_len + 2) {
                    if (y_buf[0] == '-' && !flags.f.polar)
                        buf[i] = '-';
                    else
                        buf[i] = flags.f.polar ? 23 : 'i';
                } else if (i < x_len + 3) {
                    if (y_buf[0] == '-' && !flags.f.polar)
                        buf[i] = flags.f.polar ? 23 : 'i';
                    else
                        buf[i] = y_buf[0];
                } else
                    buf[i] = y_buf[i - x_len - 2];
            }

            ret_len = x_len + y_len + 2;
            if (ret_len > buflen) {
                buf[buflen - 1] = 26;
                ret_len = buflen;
            }

            return ret_len;
        }

        case TYPE_REALMATRIX: {
            vartype_realmatrix *m = (vartype_realmatrix *) v;
            int i;
            int chars_so_far = 0;
            string2buf(buf, buflen, &chars_so_far, "[ ", 2);
            i = int2string(m->rows, buf + chars_so_far, buflen - chars_so_far);
            chars_so_far += i;
            char2buf(buf, buflen, &chars_so_far, 'x');
            i = int2string(m->columns, buf + chars_so_far, buflen - chars_so_far);
            chars_so_far += i;
            string2buf(buf, buflen, &chars_so_far, " Matrix ]", 9);
            return chars_so_far;
        }

        case TYPE_COMPLEXMATRIX: {
            vartype_complexmatrix *m = (vartype_complexmatrix *) v;
            int i;
            int chars_so_far = 0;
            string2buf(buf, buflen, &chars_so_far, "[ ", 2);
            i = int2string(m->rows, buf + chars_so_far, buflen - chars_so_far);
            chars_so_far += i;
            char2buf(buf, buflen, &chars_so_far, 'x');
            i = int2string(m->columns, buf + chars_so_far, buflen - chars_so_far);
            chars_so_far += i;
            string2buf(buf, buflen, &chars_so_far, " Cpx Matrix ]", 13);
            return chars_so_far;
        }

        case TYPE_STRING: {
            vartype_string *s = (vartype_string *) v;
            int i;
            int chars_so_far = 0;
            char2buf(buf, buflen, &chars_so_far, '"');
            char *txt = s->txt();
            for (i = 0; i < s->length; i++)
                char2buf(buf, buflen, &chars_so_far, txt[i]);
            char2buf(buf, buflen, &chars_so_far, '"');
            return chars_so_far;
        }

        case TYPE_LIST: {
            vartype_list *list = (vartype_list *) v;
            int i;
            int chars_so_far = 0;
            string2buf(buf, buflen, &chars_so_far, "{ ", 2);
            i = int2string(list->size, buf + chars_so_far, buflen - chars_so_far);
            chars_so_far += i;
            string2buf(buf, buflen, &chars_so_far, "-Elem List }", 12);
            return chars_so_far;
        }

        default: {
            const char *msg = "UnsuppVarType";
            int msglen = 13;
            int i;
            for (i = 0; i < msglen; i++)
                buf[i] = msg[i];
            return msglen;
        }
    }
}

const char *phloat2program(phloat d) {
    /* Converts a phloat to its most compact representation;
     * used for generating HP-42S style number literals in programs.
     */
    static char allbuf[50];
    static char scibuf[50];
    int alllen;
    int scilen;
    char dot = flags.f.decimal_point ? '.' : ',';
    int decimal, zeroes, last_nonzero, exponent;
    int i;
    alllen = phloat2string(d, allbuf, 49, 0, 0, 3, 0, MAX_MANT_DIGITS);
    scilen = phloat2string(d, scibuf, 49, 0, MAX_MANT_DIGITS - 1, 1, 0, MAX_MANT_DIGITS);
    /* Shorten SCI representation by removing trailing zeroes,
     * and decreasing the exponent until the decimal point
     * shifts out of the mantissa.
     */
    decimal = -1;
    exponent = -1;
    for (i = 0; i < scilen; i++) {
        char c = scibuf[i];
        if (c == dot) {
            decimal = i;
            last_nonzero = i;
            zeroes = 0;
        } else if (c == 24) {
            exponent = i;
            break;
        } else if (c != '0') {
            last_nonzero = i;
            zeroes = 0;
        } else
            zeroes++;
    }
    if (decimal != -1) {
        if (zeroes > 0) {
            for (i = last_nonzero + 1; i < scilen - zeroes; i++)
                scibuf[i] = scibuf[i + zeroes];
            scilen -= zeroes;
        }
        if ((exponent == -1 && decimal == scilen - 1)
                || (exponent != -1 && exponent - decimal == 1)){
            for (i = decimal; i < scilen - 1; i++)
                scibuf[i] = scibuf[i + 1];
            scilen--;
        } else if (exponent != -1) {
            int offset, ex, neg, newexplen, t;
            exponent -= zeroes;
            offset = exponent - decimal - 1;
            ex = 0;
            neg = 0;
            for (i = exponent + 1; i < scilen; i++) {
                char c = scibuf[i];
                if (c == '-')
                    neg = 1;
                else
                    ex = ex * 10 + c - '0';
            }
            if (neg)
                ex = -ex;
            ex -= offset;
            if (ex < 0) {
                ex = -ex;
                neg = 1;
            } else
                neg = 0;
            newexplen = neg ? 2 : 1;
            t = 10;
            while (ex >= t) {
                newexplen++;
                t *= 10;
            }
            if (newexplen <= scilen - exponent - 1) {
                for (i = decimal; i < exponent; i++)
                    scibuf[i] = scibuf[i + 1];
                scilen = exponent;
                if (neg)
                    ex = -ex;
                scilen += int2string(ex, scibuf + exponent,
                                        49 - exponent);
            }
        }
    }
    if (scilen < alllen) {
        scibuf[scilen] = 0;
        return scibuf;
    } else {
        allbuf[alllen] = 0;
        return allbuf;
    }
}

int easy_phloat2string(phloat d, char *buf, int buflen, int base_mode) {
    int dispmode;
    int digits = 0;

    if (flags.f.fix_or_all)
        dispmode = flags.f.eng_or_all ? 3 : 0;
    else
        dispmode = flags.f.eng_or_all ? 2 : 1;
    if (flags.f.digits_bit3)
        digits += 8;
    if (flags.f.digits_bit2)
        digits += 4;
    if (flags.f.digits_bit1)
        digits += 2;
    if (flags.f.digits_bit0)
        digits += 1;

    return phloat2string(d, buf, buflen, base_mode,
                         digits, dispmode, flags.f.thousands_separators);
}

int ip2revstring(phloat d, char *buf, int buflen) {
    int s = 1;
    int bufpos = 0;

    if (d < 0) {
        d = -d;
        s = -1;
    }
    d = floor(d);
    while (d != 0 && bufpos < buflen) {
        char c = '0' + to_digit(d);
        buf[bufpos++] = c;
        d = floor(d / 10);
    }
    if (bufpos == 0)
        buf[bufpos++] = '0';
    if (s == -1 && bufpos < buflen)
        buf[bufpos++] = '-';
    return bufpos;
}

int matedit_get(vartype **res) {
    if (matedit_mode == 0)
        return ERR_NONEXISTENT;

    vartype *m = NULL;
    if (matedit_mode == 2)
        m = matedit_x;
    else
        for (int i = vars_count - 1; i >= 0; i--) {
            var_struct *lv = vars + i;
            if ((lv->flags & VAR_PRIVATE) != 0)
                continue;
            if (matedit_level != -1 && lv->level < matedit_level)
                return ERR_NONEXISTENT;
            if (lv->level == matedit_level && string_equals(matedit_name, matedit_length, lv->name, lv->length)) {
                m = lv->value;
                break;
            }
        }
    int err = ERR_NONEXISTENT;
    if (m == NULL) {
        bad_matrix:
        if (matedit_mode == 2 || matedit_mode == 3)
            leave_matrix_editor();
        return err;
    }

    for (int i = 0; i < matedit_stack_depth; i++) {
        if (m->type != TYPE_LIST) {
            err = i == 0 ? ERR_INVALID_TYPE : ERR_INVALID_DATA;
            goto bad_matrix;
        }
        vartype_list *list = (vartype_list *) m;
        if (matedit_stack[i] >= list->size) {
            err = ERR_INVALID_DATA;
            goto bad_matrix;
        }
        m = list->array->data[matedit_stack[i]];
    }

    if (m->type != TYPE_REALMATRIX && m->type != TYPE_COMPLEXMATRIX && m->type != TYPE_LIST) {
        err = matedit_stack_depth == 0 ? ERR_INVALID_TYPE : ERR_INVALID_DATA;
        goto bad_matrix;
    }

    // The following checks *should* be unnecessary, we're already preventing
    // those scenarios. Or that's what we're trying, anyway...
    if (m->type == TYPE_REALMATRIX) {
        vartype_realmatrix *rm = (vartype_realmatrix *) m;
        if (matedit_i >= rm->rows || matedit_j >= rm->columns)
            matedit_i = matedit_j = 0;
    } else if (m->type == TYPE_COMPLEXMATRIX) {
        vartype_complexmatrix *cm = (vartype_complexmatrix *) m;
        if (matedit_i >= cm->rows || matedit_j >= cm->columns)
            matedit_i = matedit_j = 0;
    } else { // m->type == TYPE_LIST
        vartype_list *list = (vartype_list *) m;
        if (matedit_i >= list->size)
            matedit_i = 0;
        matedit_j = 0;
    }

    *res = m;
    return ERR_NONE;
}

void leave_matrix_editor() {
    set_appmenu_exitcallback(0);
    set_menu(MENULEVEL_APP, MENU_NONE);
    matedit_mode = 0;
    free(matedit_stack);
    matedit_stack = NULL;
    matedit_stack_depth = 0;
}
