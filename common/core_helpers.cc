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

#include "core_helpers.h"
#include "core_commands2.h"
#include "core_display.h"
#include "core_phloat.h"
#include "core_main.h"
#include "core_variables.h"
#include "shell.h"


int resolve_ind_arg(arg_struct *arg) {
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
                if (rm->array->is_string[num]) {
                    int i;
                    phloat *d = &rm->array->data[num];
                    arg->type = ARGTYPE_STR;
                    arg->length = phloat_length(*d);
                    for (i = 0; i < phloat_length(*d); i++)
                        arg->val.text[i] = phloat_text(*d)[i];
                } else {
                    phloat x = rm->array->data[num];
                    if (x < 0)
                        x = -x;
                    if (x >= 2147483648.0)
                        arg->val.num = 2147483647;
                    else
                        arg->val.num = to_int4(x);
                    arg->type = ARGTYPE_NUM;
                }
                return ERR_NONE;
            }
        }
        case ARGTYPE_IND_STK: {
            switch (arg->val.stk) {
                case 'X': v = reg_x; break;
                case 'Y': v = reg_y; break;
                case 'Z': v = reg_z; break;
                case 'T': v = reg_t; break;
                case 'L': v = reg_lastx; break;
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
                int i;
                arg->type = ARGTYPE_STR;
                arg->length = s->length;
                for (i = 0; i < s->length; i++)
                    arg->val.text[i] = s->text[i];
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
    } else
        return ERR_INVALID_TYPE;
}

int is_pure_real(const vartype *matrix) {
    vartype_realmatrix *rm;
    int4 size, i;
    if (matrix->type != TYPE_REALMATRIX)
        return 0;
    rm = (vartype_realmatrix *) matrix;
    size = rm->rows * rm->columns;
    for (i = 0; i < size; i++)
        if (rm->array->is_string[i])
            return 0;
    return 1;
}

void recall_result(vartype *v) {
    if (flags.f.stack_lift_disable)
        free_vartype(reg_x);
    else {
        free_vartype(reg_t);
        reg_t = reg_z;
        reg_z = reg_y;
        reg_y = reg_x;
    }
    reg_x = v;
    if (flags.f.trace_print && flags.f.printer_exists)
        docmd_prx(NULL);
}

void recall_two_results(vartype *x, vartype *y) {
    if (flags.f.stack_lift_disable) {
        free_vartype(reg_t);
        free_vartype(reg_x);
        reg_t = reg_z;
        reg_z = reg_y;
    } else {
        free_vartype(reg_t);
        free_vartype(reg_z);
        reg_t = reg_y;
        reg_z = reg_x;
    }
    reg_y = y;
    reg_x = x;
    if (flags.f.trace_print && flags.f.printer_exists)
        docmd_prx(NULL);
}

void unary_result(vartype *x) {
    free_vartype(reg_lastx);
    reg_lastx = reg_x;
    reg_x = x;
    if (flags.f.trace_print && flags.f.printer_exists)
        docmd_prx(NULL);
}

void binary_result(vartype *x) {
    free_vartype(reg_lastx);
    reg_lastx = reg_x;
    reg_x = x;
    free_vartype(reg_y);
    reg_y = reg_z;
    reg_z = dup_vartype(reg_t);
    if (flags.f.trace_print && flags.f.printer_exists)
        docmd_prx(NULL);
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

int virtual_flag_handler(int flagop, int flagnum) {
    /* NOTE: the determination which flag numbers are handled by this
     * function is made by docmd_sf() etc.; they do this based on a constant
     * string 'virtual_flags' defined locally in core_commands.c.
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
            int its_on = mode_appmenu == MENU_VARMENU && varmenu_role == 0;
            switch (flagop) {
                case FLAGOP_FS_T:
                    return its_on ? ERR_YES : ERR_NO;
                case FLAGOP_FC_T:
                    return its_on ? ERR_NO : ERR_YES;
                default:
                    return ERR_INTERNAL_ERROR;
            }
        }
        case 49: /* low_battery */ {
            int lowbat = shell_low_battery();
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

void set_base(int base) {
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
    if (mode_appmenu == MENU_BASE_A_THRU_F)
        set_menu(MENULEVEL_APP, MENU_BASE);

    if (base != oldbase && flags.f.trace_print && flags.f.printer_exists)
        docmd_prx(NULL);
}

int get_base_param(const vartype *v, int8 *n) {
    if (v->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else if (v->type != TYPE_REAL)
        return ERR_INVALID_TYPE;
    phloat x = ((vartype_real *) v)->x;
    if (x > 34359738367.0 || x < -34359738368.0)
        return ERR_INVALID_DATA;
    int8 t = to_int8(x);
    if ((t & LL(0x800000000)) != 0)
        *n = t | LL(0xfffffff000000000);
    else
        *n = t;
    return ERR_NONE;
}

int base_range_check(int8 *n) {
    if (*n < LL(-34359738368)) {
        if (flags.f.range_error_ignore)
            *n = LL(-34359738368);
        else
            return ERR_OUT_OF_RANGE;
    } else if (*n > LL(34359738367)) {
        if (flags.f.range_error_ignore)
            *n = LL(34359738367);
        else
            return ERR_OUT_OF_RANGE;
    }
    return ERR_NONE;
}

void print_text(const char *text, int length, int left_justified) {
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
        char charbits[5];
        int j;
        get_char(charbits, buf[i]);
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

void print_lines(const char *text, int length, int left_justified) {
    int line_start = 0;
    int width = flags.f.double_wide_print ? 12 : 24;
    while (line_start + width < length) {
        print_text(text + line_start, width, left_justified);
        line_start += width;
    }
    print_text(text + line_start, length - line_start, left_justified);
}

void print_right(const char *left, int leftlen, const char *right, int rightlen) {
    char buf[100];
    int len;
    int width = flags.f.double_wide_print ? 12 : 24;
    int i, pad;

    string_copy(buf, &len, left, leftlen);
    if (len + rightlen + 1 <= width) {
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
        print_text(buf, len, 0);
    } else {
        int line_start = 0;
        while (line_start + width < len) {
            print_text(buf + line_start, width, 1);
            line_start += width;
        }
        pad = width - (len - line_start) - rightlen;
        if (pad < 1)
            pad = 1;
        for (i = 0; i < pad; i++)
            buf[len++] = ' ';
        for (i = 0; i < rightlen; i++)
            buf[len++] = right[i];
        print_lines(buf + line_start, len - line_start, 1);
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
        print_text(buf, bufptr, 1);
    } else {
        for (i = 0; i < leftlen; i++) {
            buf[bufptr++] = left[i];
            if (bufptr == width) {
                print_text(buf, width, 1);
                bufptr = 0;
            }
        }
        for (i = 0; i < rightlen; i++) {
            buf[bufptr++] = right[i];
            if (bufptr == width) {
                print_text(buf, width, 1);
                bufptr = 0;
            }
        }
        if (bufptr > 0)
            print_text(buf, bufptr, 1);
    }
}

void print_command(int cmd, const arg_struct *arg) {
    char buf[100];
    int bufptr = 0;
    
    if (cmd == CMD_NULL && !deferred_print)
        return;

    shell_annunciators(-1, -1, 1, -1, -1, -1);

    if (cmd != CMD_NULL)
        bufptr += command2buf(buf, 100, cmd, arg);

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
        print_right(cmdline, cmdline_length, buf, bufptr);
    } else {
        /* Normally we print commands right-justified, but if they don't fit
         * on one line, we print them left-justified, because having the excess
         * go near the right margin looks weird and confusing.
         */
        int left = bufptr > (flags.f.double_wide_print ? 12 : 24);
        print_lines(buf, bufptr, left);
    }

    deferred_print = 0;
    shell_annunciators(-1, -1, 0, -1, -1, -1);
}

void generic_r2p(phloat re, phloat im, phloat *r, phloat *phi) {
    if (im == 0) {
        if (re >= 0) {
            *r = re;
            *phi = 0;
        } else {
            *r = -re;
            *phi = flags.f.grad ? 200 : flags.f.rad ? PI : 180;
        }
    } else if (re == 0) {
        if (im > 0) {
            *r = im;
            *phi = flags.f.grad ? 100 : flags.f.rad ? PI / 2 : 90;
        } else {
            *r = -im;
            *phi = flags.f.grad ? -100 : flags.f.rad ? -PI / 2: -90;
        }
    } else {
        *r = hypot(re, im);
        *phi = rad_to_angle(atan2(im, re));
    }
}

void generic_p2r(phloat r, phloat phi, phloat *re, phloat *im) {
    phloat tre, tim;
    if (flags.f.rad) {
        sincos(phi, &tim, &tre);
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
        r = sqrt(0.5);
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
        r = sqrt(0.5);
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
    if (check_matedit
            && (matedit_mode == 1 || matedit_mode == 3)
            && string_equals(name, namelen, matedit_name, matedit_length)) {
        if (matedit_mode == 1)
            matedit_i = matedit_j = 0;
        else
            return ERR_RESTRICTED_OPERATION;
    }

    vartype *matrix = recall_var(name, namelen);
    /* NOTE: 'size' will only ever be 0 when we're called from
     * docmd_size(); docmd_dim() does not allow 0-size matrices.
     */
    int4 size = rows * columns;
    if (matrix == NULL || (matrix->type != TYPE_REALMATRIX
                        && matrix->type != TYPE_COMPLEXMATRIX)) {
        vartype *newmatrix;
        if (size == 0)
            return ERR_NONE;
        newmatrix = new_realmatrix(rows, columns);
        if (newmatrix == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        store_var(name, namelen, newmatrix);
        return ERR_NONE;
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
            int4 i, s, oldsize;
            phloat *new_data = (phloat *)
                                    realloc(oldmatrix->array->data,
                                            size * sizeof(phloat));
            if (new_data == NULL) {
                free(new_is_string);
                return ERR_INSUFFICIENT_MEMORY;
            }
            oldsize = oldmatrix->rows * oldmatrix->columns;
            s = oldsize < size ? oldsize : size;
            for (i = 0; i < s; i++)
                new_is_string[i] = oldmatrix->array->is_string[i];
            for (i = s; i < size; i++) {
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
                free(new_array->data);
                free(new_array);
                return ERR_INSUFFICIENT_MEMORY;
            }
            oldsize = oldmatrix->rows * oldmatrix->columns;
            s = oldsize < size ? oldsize : size;
            for (i = 0; i < s; i++) {
                new_array->is_string[i] = oldmatrix->array->is_string[i];
                new_array->data[i] = oldmatrix->array->data[i];
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
    } else /* matrix->type == TYPE_COMPLEXMATRIX */ {
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

#if defined(NO_SINCOS) && !defined(BCD_MATH)

void sincos(double x, double *sinx, double *cosx) {
    *sinx = sin(x);
    *cosx = cos(x);
}

#endif

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
            for (i = 0; i < s->length; i++)
                char2buf(buf, buflen, &chars_so_far, s->text[i]);
            char2buf(buf, buflen, &chars_so_far, '"');
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

char *phloat2program(phloat d) {
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
