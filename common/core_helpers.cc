/*****************************************************************************
 * Free42 -- a free HP-42S calculator clone
 * Copyright (C) 2004-2005  Thomas Okken
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

#include "core_helpers.h"
#include "core_commands2.h"
#include "core_decimal.h"
#include "core_display.h"
#include "core_linalg.h"
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
		    double_or_string *ds = &rm->array->data[num];
		    arg->type = ARGTYPE_STR;
		    arg->length = ds->s.length;
		    for (i = 0; i < ds->s.length; i++)
			arg->val.text[i] = ds->s.text[i];
		} else {
		    double x = rm->array->data[num].d;
		    if (x < 0)
			x = -x;
		    if (core_settings.ip_hack)
			x += 5e-9;
		    arg->type = ARGTYPE_NUM;
		    arg->val.num = (int4) x;
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
		double x = ((vartype_real *) v)->x;
		if (x < 0)
		    x = -x;
		if (core_settings.ip_hack)
		    x += 5e-9;
		arg->type = ARGTYPE_NUM;
		arg->val.num = (int4) x;
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

int apply_sto_operation(char operation, vartype *x, vartype *oldval,
				    void (*completion)(int, vartype *)) {
    vartype *newval;
    int error;
    switch (operation) {
	case '/':
	    return generic_div(x, oldval, completion);
	case '*':
	    return generic_mul(x, oldval, completion);
	case '-':
	    error = generic_sub(x, oldval, &newval);
	    completion(error, newval);
	    return error;
	case '+':
	    error = generic_add(x, oldval, &newval);
	    completion(error, newval);
	    return error;
	default:
	    return ERR_INTERNAL_ERROR;
    }
}

double rad_to_angle(double x) {
    if (flags.f.rad)
	return x;
    else if (flags.f.grad)
	return x * (200 / PI);
    else
	return x * (180 / PI);
}

double angle_to_rad(double x) {
    if (flags.f.rad)
	return x;
    else if (flags.f.grad)
	return fmod(x, 400) / (200 / PI);
    else
	return fmod(x, 360) / (180 / PI);
}

double rad_to_deg(double x) {
    return x * (180 / PI);
}

double deg_to_rad(double x) {
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

int string_equals(const char *s1, int s1len, const char *s2, int s2len) {
    int i;
    if (s1len != s2len)
	return 0;
    for (i = 0; i < s1len; i++)
	if (s1[i] != s2[i])
	    return 0;
    return 1;
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
	    int its_on = mode_plainmenu = MENU_PROGRAMMABLE;
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
    double x;
    int8 t;
    if (v->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else if (v->type != TYPE_REAL)
	return ERR_INVALID_TYPE;
    x = ((vartype_real *) v)->x;
    if (x > 34359738367.0 || x < -34359738368.0)
	return ERR_INVALID_DATA;
    if (core_settings.ip_hack) {
	if (x < 0)
	    x -= 5e-9;
	else
	    x += 5e-9;
    }
    t = (int8) x;
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
	if (flags.f.lowercase_print && c >= 'A' && c <= 'Z')
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
	if (len - line_start + 1 + rightlen <= width) {
	    pad = width - (len - line_start) - rightlen;
	    for (i = 0; i < pad; i++)
		buf[len++] = ' ';
	    for (i = 0; i < rightlen; i++)
		buf[len++] = right[i];
	    print_text(buf + line_start, len - line_start, 1);
	} else {
	    print_text(buf + line_start, len - line_start, 1);
	    print_text(right, rightlen, 0);
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
    } else
	print_lines(buf, bufptr, 0);

    deferred_print = 0;
    shell_annunciators(-1, -1, 0, -1, -1, -1);
}

int generic_div(const vartype *px, const vartype *py, void (*completion)(int, vartype *)) {
    if (px->type == TYPE_STRING || py->type == TYPE_STRING) {
	completion(ERR_ALPHA_DATA_IS_INVALID, NULL);
	return ERR_ALPHA_DATA_IS_INVALID;
    } else if ((px->type == TYPE_REALMATRIX || px->type == TYPE_COMPLEXMATRIX)
	    && (py->type == TYPE_REALMATRIX || py->type == TYPE_COMPLEXMATRIX)){
	return linalg_div(py, px, completion);
    } else {
	vartype *dst;
	int error = map_binary(px, py, &dst, div_rr, div_rc, div_cr, div_cc);
	completion(error, dst);
	return error;
    }
}

int generic_mul(const vartype *px, const vartype *py, void (*completion)(int, vartype *)) {
    if (px->type == TYPE_STRING || py->type == TYPE_STRING) {
	completion(ERR_ALPHA_DATA_IS_INVALID, NULL);
	return ERR_ALPHA_DATA_IS_INVALID;
    } else if ((px->type == TYPE_REALMATRIX || px->type == TYPE_COMPLEXMATRIX)
	    && (py->type == TYPE_REALMATRIX || py->type == TYPE_COMPLEXMATRIX)){
	return linalg_mul(py, px, completion);
    } else {
	vartype *dst;
	int error = map_binary(px, py, &dst, mul_rr, mul_rc, mul_cr, mul_cc);
	completion(error, dst);
	return error;
    }
}

int generic_sub(const vartype *px, const vartype *py, vartype **dst) {
    if (px->type == TYPE_REAL && py->type == TYPE_REAL) {
	vartype_real *x = (vartype_real *) px;
	vartype_real *y = (vartype_real *) py;
	double res = y->x - x->x;
	int inf = isinf(res);
	if (inf != 0) {
	    if (flags.f.range_error_ignore)
		res = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	    else
		return ERR_OUT_OF_RANGE;
	}
	*dst = new_real(res);
	if (*dst == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	else
	    return ERR_NONE;
    } else if (px->type == TYPE_COMPLEX && py->type == TYPE_COMPLEX) {
	vartype_complex *x = (vartype_complex *) px;
	vartype_complex *y = (vartype_complex *) py;
	int inf;
	double re, im;
	re = y->re - x->re;
	inf = isinf(re);
	if (inf != 0) {
	    if (flags.f.range_error_ignore)
		re = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	    else
		return ERR_OUT_OF_RANGE;
	}
	im = y->im - x->im;
	inf = isinf(im);
	if (inf != 0) {
	    if (flags.f.range_error_ignore)
		im = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	    else
		return ERR_OUT_OF_RANGE;
	}
	*dst = new_complex(re, im);
	if (*dst == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	else
	    return ERR_NONE;
    } else if (px->type == TYPE_REAL && py->type == TYPE_COMPLEX) {
	vartype_real *x = (vartype_real *) px;
	vartype_complex *y = (vartype_complex *) py;
	int inf;
	double re = y->re - x->x;
	inf = isinf(re);
	if (inf != 0) {
	    if (flags.f.range_error_ignore)
		re = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	    else
		return ERR_OUT_OF_RANGE;
	}
	*dst = new_complex(re, y->im);
	if (*dst == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	else
	    return ERR_NONE;
    } else if (px->type == TYPE_COMPLEX && py->type == TYPE_REAL) {
	vartype_complex *x = (vartype_complex *) px;
	vartype_real *y = (vartype_real *) py;
	double re = y->x - x->re;
	int inf = isinf(re);
	if (inf != 0) {
	    if (flags.f.range_error_ignore)
		re = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	    else
		return ERR_OUT_OF_RANGE;
	}
	*dst = new_complex(re, -x->im);
	if (*dst == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	else
	    return ERR_NONE;
    } else if (px->type == TYPE_STRING || py->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else
	return map_binary(px, py, dst, sub_rr, sub_rc, sub_cr, sub_cc);
}

int generic_add(const vartype *px, const vartype *py, vartype **dst) {
    if (px->type == TYPE_REAL && py->type == TYPE_REAL) {
	vartype_real *x = (vartype_real *) px;
	vartype_real *y = (vartype_real *) py;
	double res = y->x + x->x;
	int inf = isinf(res);
	if (inf != 0) {
	    if (flags.f.range_error_ignore)
		res = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	    else
		return ERR_OUT_OF_RANGE;
	}
	*dst = new_real(res);
	if (*dst == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	else
	    return ERR_NONE;
    } else if (px->type == TYPE_COMPLEX && py->type == TYPE_COMPLEX) {
	vartype_complex *x = (vartype_complex *) px;
	vartype_complex *y = (vartype_complex *) py;
	int inf;
	double re, im;
	re = x->re + y->re;
	inf = isinf(re);
	if (inf != 0) {
	    if (flags.f.range_error_ignore)
		re = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	    else
		return ERR_OUT_OF_RANGE;
	}
	im = x->im + y->im;
	inf = isinf(im);
	if (inf != 0) {
	    if (flags.f.range_error_ignore)
		im = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	    else
		return ERR_OUT_OF_RANGE;
	}
	*dst = new_complex(re, im);
	if (*dst == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	else
	    return ERR_NONE;
    } else if (px->type == TYPE_REAL && py->type == TYPE_COMPLEX) {
	vartype_real *x = (vartype_real *) px;
	vartype_complex *y = (vartype_complex *) py;
	double re = y->re + x->x;
	int inf = isinf(re);
	if (inf != 0) {
	    if (flags.f.range_error_ignore)
		re = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	    else
		return ERR_OUT_OF_RANGE;
	}
	*dst = new_complex(re, y->im);
	if (*dst == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	else
	    return ERR_NONE;
    } else if (px->type == TYPE_COMPLEX && py->type == TYPE_REAL) {
	vartype_complex *x = (vartype_complex *) px;
	vartype_real *y = (vartype_real *) py;
	double re = x->re + y->x;
	int inf = isinf(re);
	if (inf != 0) {
	    if (flags.f.range_error_ignore)
		re = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	    else
		return ERR_OUT_OF_RANGE;
	}
	*dst = new_complex(re, x->im);
	if (*dst == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	else
	    return ERR_NONE;
    } else if (px->type == TYPE_STRING || py->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else
	return map_binary(px, py, dst, add_rr, add_rc, add_cr, add_cc);
}

int generic_rcl(arg_struct *arg, vartype **dst) {
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
		double_or_string ds;
		if (index >= size)
		    return ERR_SIZE_ERROR;
		ds = rm->array->data[index];
		if (rm->array->is_string[index])
		    *dst = new_string(ds.s.text, ds.s.length);
		else
		    *dst = new_real(ds.d);
		if (*dst == NULL)
		    return ERR_INSUFFICIENT_MEMORY;
		return ERR_NONE;
	    } else if (regs->type == TYPE_COMPLEXMATRIX) {
		vartype_complexmatrix *cm = (vartype_complexmatrix *) regs;
		int4 size = cm->rows * cm->columns;
		if (arg->val.num >= size)
		    return ERR_SIZE_ERROR;
		*dst = new_complex(cm->array->data[arg->val.num * 2],
				cm->array->data[arg->val.num * 2 + 1]);
		if (*dst == NULL)
		    return ERR_INSUFFICIENT_MEMORY;
		return ERR_NONE;
	    } else {
		/* This should never happen; STO should prevent
		 * "REGS" from being any other type than a real or
		 * complex matrix.
		 */
		return ERR_INTERNAL_ERROR;
	    }
	}
	case ARGTYPE_STK: {
	    switch (arg->val.stk) {
		case 'X': *dst = reg_x; break;
		case 'Y': *dst = reg_y; break;
		case 'Z': *dst = reg_z; break;
		case 'T': *dst = reg_t; break;
		case 'L': *dst = reg_lastx; break;
	    }
	    *dst = dup_vartype(*dst);
	    if (*dst == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	    return ERR_NONE;
	}
	case ARGTYPE_STR: {
	    *dst = recall_var(arg->val.text, arg->length);
	    if (*dst == NULL)
		return ERR_NONEXISTENT;
	    *dst = dup_vartype(*dst);
	    if (*dst == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	    return ERR_NONE;
	}
	default:
	    return ERR_INTERNAL_ERROR;
    }
}

static arg_struct temp_arg;

static void generic_sto_completion(int error, vartype *res) HELPERS_SECT;
static void generic_sto_completion(int error, vartype *res) {
    if (error != ERR_NONE)
	return;
    if (temp_arg.type == ARGTYPE_STK) {
	switch (temp_arg.val.stk) {
	    case 'X':
		free_vartype(reg_x);
		reg_x = res;
		break;
	    case 'Y':
		free_vartype(reg_y);
		reg_y = res;
		break;
	    case 'Z':
		free_vartype(reg_z);
		reg_z = res;
		break;
	    case 'T':
		free_vartype(reg_t);
		reg_t = res;
		break;
	    case 'L':
		free_vartype(reg_lastx);
		reg_lastx = res;
		break;
	}
    } else /* temp_arg.type == ARGTYPE_STR */ {
	store_var(temp_arg.val.text, temp_arg.length, res);
    }
}

int generic_sto(arg_struct *arg, char operation) {
    if (arg->type == ARGTYPE_IND_NUM
	    || arg->type == ARGTYPE_IND_STK
	    || arg->type == ARGTYPE_IND_STR) {
	int err = resolve_ind_arg(arg);
	if (err != ERR_NONE)
	    return err;
    }

    if (operation != 0 && reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;

    switch (arg->type) {
	case ARGTYPE_NUM: {
	    vartype *regs = recall_var("REGS", 4);
	    if (regs == NULL)
		return ERR_SIZE_ERROR;
	    if (regs->type == TYPE_REALMATRIX) {
		vartype_realmatrix *rm = (vartype_realmatrix *) regs;
		int4 size = rm->rows * rm->columns;
		int4 num = arg->val.num;
		if (num >= size)
		    return ERR_SIZE_ERROR;
		if (reg_x->type == TYPE_STRING) {
		    vartype_string *vs = (vartype_string *) reg_x;
		    double_or_string *ds = rm->array->data + num;
		    int len, i;
		    if (!disentangle((vartype *) rm))
			return ERR_INSUFFICIENT_MEMORY;
		    len = vs->length;
		    ds->s.length = len;
		    for (i = 0; i < len; i++)
			ds->s.text[i] = vs->text[i];
		    rm->array->is_string[num] = 1;
		    return ERR_NONE;
		} else if (reg_x->type == TYPE_REAL) {
		    if (!disentangle((vartype *) rm))
			return ERR_INSUFFICIENT_MEMORY;
		    if (operation == 0) {
			rm->array->data[num].d = ((vartype_real *) reg_x)->x;
			rm->array->is_string[num] = 0;
		    } else {
			double x, n;
			int inf;
			if (rm->array->is_string[num])
			    return ERR_ALPHA_DATA_IS_INVALID;
			x = ((vartype_real *) reg_x)->x;
			n = rm->array->data[num].d;
			switch (operation) {
			    case '/': if (x == 0) return ERR_DIVIDE_BY_0;
				      n /= x; break;
			    case '*': n *= x; break;
			    case '-': n -= x; break;
			    case '+': n += x; break;
			}
			inf = isinf(n);
			if (inf != 0) {
			    if (flags.f.range_error_ignore)
				n = inf == 1 ? POS_HUGE_DOUBLE
					     : NEG_HUGE_DOUBLE;
			    else
				return ERR_OUT_OF_RANGE;
			}
			rm->array->data[num].d = n;
		    }
		    return ERR_NONE;
		} else
		    return ERR_INVALID_TYPE;
	    } else if (regs->type == TYPE_COMPLEXMATRIX) {
		vartype_complexmatrix *cm = (vartype_complexmatrix *) regs;
		int4 size = cm->rows * cm->columns;
		int4 num = arg->val.num;
		double re, im;
		if (num >= size)
		    return ERR_SIZE_ERROR;
		if (reg_x->type == TYPE_STRING)
		    return ERR_ALPHA_DATA_IS_INVALID;
		else if (reg_x->type != TYPE_REAL
			&& reg_x->type != TYPE_COMPLEX)
		    return ERR_INVALID_TYPE;
		if (!disentangle((vartype *) cm))
		    return ERR_INSUFFICIENT_MEMORY;
		if (operation == 0) {
		    if (reg_x->type == TYPE_REAL) {
			re = ((vartype_real *) reg_x)->x;
			im = 0;
		    } else {
			re = ((vartype_complex *) reg_x)->re;
			im = ((vartype_complex *) reg_x)->im;
		    }
		    cm->array->data[num * 2] = re;
		    cm->array->data[num * 2 + 1] = im;
		} else {
		    double nre = cm->array->data[num * 2];
		    double nim = cm->array->data[num * 2 + 1];
		    int inf;
		    if (reg_x->type == TYPE_REAL) {
			double x;
			x = ((vartype_real *) reg_x)->x;
			switch (operation) {
			    case '/': if (x == 0) return ERR_DIVIDE_BY_0;
				      nre /= x; nim /= x; break;
			    case '*': nre *= x; nim *= x; break;
			    case '-': nre -= x; break;
			    case '+': nre += x; break;
			}
		    } else {
			double xre, xim, h, tmp;
			xre = ((vartype_complex *) reg_x)->re;
			xim = ((vartype_complex *) reg_x)->im;
			h = xre * xre + xim * xim;
			/* TODO: handle overflows in intermediate results */
			switch (operation) {
			    case '/':
				if (h == 0)
				    return ERR_DIVIDE_BY_0;
				tmp = (nre * xre + nim * xim) / h;
				nim = (xre * nim - xim * nre) / h;
				nre = tmp;
				break;
			    case '*':
				tmp = nre * xre - nim * xim;
				nim = nre * xim + nim * xre;
				nre = tmp;
				break;
			    case '-':
				nre -= xre;
				nim -= xim;
				break;
			    case '+':
				nre += xre;
				nim += xim;
				break;
			}
		    }
		    inf = isinf(nre);
		    if (inf != 0) {
			if (flags.f.range_error_ignore)
			    nre = inf == 1 ? POS_HUGE_DOUBLE
					   : NEG_HUGE_DOUBLE;
			else
			    return ERR_OUT_OF_RANGE;
		    }
		    inf = isinf(nim);
		    if (inf != 0) {
			if (flags.f.range_error_ignore)
			    nim = inf == 1 ? POS_HUGE_DOUBLE
					   : NEG_HUGE_DOUBLE;
			else
			    return ERR_OUT_OF_RANGE;
		    }
		    cm->array->data[num * 2] = nre;
		    cm->array->data[num * 2 + 1] = nim;
		}
		return ERR_NONE;
	    } else {
		/* This should never happen; STO should prevent
		 * "REGS" from being any other type than a real or
		 * complex matrix.
		 */
		return ERR_INTERNAL_ERROR;
	    }
	}
	case ARGTYPE_STK: {
	    vartype *newval;
	    if (operation == 0) {
		if (arg->val.stk == 'X') {
		    /* STO ST X : no-op! */
		    return ERR_NONE;
		}
		newval = dup_vartype(reg_x);
		if (newval == NULL)
		    return ERR_INSUFFICIENT_MEMORY;
		switch (arg->val.stk) {
		    case 'Y':
			free_vartype(reg_y);
			reg_y = newval;
			break;
		    case 'Z':
			free_vartype(reg_z);
			reg_z = newval;
			break;
		    case 'T':
			free_vartype(reg_t);
			reg_t = newval;
			break;
		    case 'L':
			free_vartype(reg_lastx);
			reg_lastx = newval;
			break;
		}
		return ERR_NONE;
	    } else {
		vartype *oldval;
		switch (arg->val.stk) {
		    case 'X': oldval = reg_x; break;
		    case 'Y': oldval = reg_y; break;
		    case 'Z': oldval = reg_z; break;
		    case 'T': oldval = reg_t; break;
		    case 'L': oldval = reg_lastx; break;
		}
		temp_arg = *arg;
		return apply_sto_operation(operation, reg_x, oldval,
						generic_sto_completion);
	    }
	}
	case ARGTYPE_STR: {
	    if (operation == 0) {
		vartype *newval;
		/* Only allow matrices to be stored in "REGS" */
		if (arg->length == 4
			&& arg->val.text[0] == 'R'
			&& arg->val.text[1] == 'E'
			&& arg->val.text[2] == 'G'
			&& arg->val.text[3] == 'S'
			&& reg_x->type != TYPE_REALMATRIX
			&& reg_x->type != TYPE_COMPLEXMATRIX)
		    return ERR_RESTRICTED_OPERATION;
		newval = dup_vartype(reg_x);
		if (newval == NULL)
		    return ERR_INSUFFICIENT_MEMORY;
		store_var(arg->val.text, arg->length, newval);
		return ERR_NONE;
	    } else {
		vartype *oldval = recall_var(arg->val.text, arg->length);
		if (oldval == NULL)
		    return ERR_NONEXISTENT;
		temp_arg = *arg;
		return apply_sto_operation(operation, reg_x, oldval,
						generic_sto_completion);
	    }
	}
	default:
	    return ERR_INVALID_TYPE;
    }
}

void generic_r2p(double re, double im, double *r, double *phi) {
    *r = hypot(re, im);
    *phi = rad_to_angle(atan2(im, re));
}

void generic_p2r(double r, double phi, double *re, double *im) {
    double tre, tim;
    sincos(angle_to_rad(phi), &tim, &tre);
    *re = r * tre;
    *im = r * tim;
}

int map_unary(const vartype *src, vartype **dst, mappable_r mr, mappable_c mc) {
    int error;
    switch (src->type) {
	case TYPE_REAL: {
	    double r;
	    error = mr(((vartype_real *) src)->x, &r);
	    if (error == ERR_NONE) {
		*dst = new_real(r);
		if (*dst == NULL)
		    return ERR_INSUFFICIENT_MEMORY;
	    }
	    return error;
	}
	case TYPE_COMPLEX: {
	    double rre, rim;
	    error = mc(((vartype_complex *) src)->re,
		       ((vartype_complex *) src)->im, &rre, &rim);
	    if (error == ERR_NONE) {
		*dst = new_complex(rre, rim);
		if (*dst == NULL)
		    return ERR_INSUFFICIENT_MEMORY;
	    }
	    return error;
	}
	case TYPE_REALMATRIX: {
	    vartype_realmatrix *sm = (vartype_realmatrix *) src;
	    vartype_realmatrix *dm;
	    int4 size, i;
	    int error;
	    dm = (vartype_realmatrix *) new_realmatrix(sm->rows, sm->columns);
	    if (dm == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	    size = sm->rows * sm->columns;
	    for (i = 0; i < size; i++) {
		if (sm->array->is_string[i]) {
		    free_vartype((vartype *) dm);
		    return ERR_ALPHA_DATA_IS_INVALID;
		}
	    }
	    for (i = 0; i < size; i++) {
		error = mr(sm->array->data[i].d, &dm->array->data[i].d);
		if (error != ERR_NONE) {
		    free_vartype((vartype *) dm);
		    return error;
		}
	    }
	    *dst = (vartype *) dm;
	    return ERR_NONE;
	}
	case TYPE_COMPLEXMATRIX: {
	    vartype_complexmatrix *sm = (vartype_complexmatrix *) src;
	    vartype_complexmatrix *dm;
	    int4 rows = sm->rows;
	    int4 columns = sm->columns;
	    int4 size = 2 * rows * columns;
	    int4 i;
	    int error;
	    dm = (vartype_complexmatrix *) new_complexmatrix(rows, columns);
	    if (dm == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	    for (i = 0; i < size; i += 2) {
		error = mc(sm->array->data[i], sm->array->data[i + 1],
			   &dm->array->data[i], &dm->array->data[i + 1]);
		if (error != ERR_NONE) {
		    free_vartype((vartype *) dm);
		    return error;
		}
	    }
	    *dst = (vartype *) dm;
	    return ERR_NONE;
	}
	default:
	    return ERR_INTERNAL_ERROR;
    }
}

int map_binary(const vartype *src1, const vartype *src2, vartype **dst,
	mappable_rr mrr, mappable_rc mrc, mappable_cr mcr, mappable_cc mcc) {
    int error;
    switch (src1->type) {
	case TYPE_REAL:
	    switch (src2->type) {
		case TYPE_REAL: {
		    double r;
		    error = mrr(((vartype_real *) src1)->x,
				((vartype_real *) src2)->x, &r);
		    if (error == ERR_NONE) {
			*dst = new_real(r);
			if (*dst == NULL)
			    return ERR_INSUFFICIENT_MEMORY;
		    }
		    return error;
		}
		case TYPE_COMPLEX: {
		    double rre, rim;
		    error = mrc(((vartype_real *) src1)->x,
				((vartype_complex *) src2)->re,
				((vartype_complex *) src2)->im,
				&rre, &rim);
		    if (error == ERR_NONE) {
			*dst = new_complex(rre, rim);
			if (*dst == NULL)
			    return ERR_INSUFFICIENT_MEMORY;
		    }
		    return error;
		}
		case TYPE_REALMATRIX: {
		    vartype_realmatrix *sm = (vartype_realmatrix *) src2;
		    vartype_realmatrix *dm;
		    int4 size, i;
		    int error;
		    dm = (vartype_realmatrix *)
					new_realmatrix(sm->rows, sm->columns);
		    if (dm == NULL)
			return ERR_INSUFFICIENT_MEMORY;
		    size = sm->rows * sm->columns;
		    for (i = 0; i < size; i++)
			if (sm->array->is_string[i]) {
			    free_vartype((vartype *) dm);
			    return ERR_ALPHA_DATA_IS_INVALID;
			}
		    for (i = 0; i < size; i++) {
			error = mrr(((vartype_real *) src1)->x,
				    sm->array->data[i].d,
				    &dm->array->data[i].d);
			if (error != ERR_NONE) {
			    free_vartype((vartype *) dm);
			    return error;
			}
		    }
		    *dst = (vartype *) dm;
		    return ERR_NONE;
		}
		case TYPE_COMPLEXMATRIX: {
		    vartype_complexmatrix *sm = (vartype_complexmatrix *) src2;
		    vartype_complexmatrix *dm;
		    int4 size, i;
		    int error;
		    dm = (vartype_complexmatrix *)
				    new_complexmatrix(sm->rows, sm->columns);
		    if (dm == NULL)
			return ERR_INSUFFICIENT_MEMORY;
		    size = 2 * sm->rows * sm->columns;
		    for (i = 0; i < size; i += 2) {
			error = mrc(((vartype_real *) src1)->x,
				    sm->array->data[i],
				    sm->array->data[i + 1],
				    &dm->array->data[i],
				    &dm->array->data[i + 1]);
			if (error != ERR_NONE) {
			    free_vartype((vartype *) dm);
			    return error;
			}
		    }
		    *dst = (vartype *) dm;
		    return ERR_NONE;
		}
		default:
		    return ERR_INTERNAL_ERROR;
	    }
	case TYPE_COMPLEX:
	    switch (src2->type) {
		case TYPE_REAL: {
		    double rre, rim;
		    error = mcr(((vartype_complex *) src1)->re,
				((vartype_complex *) src1)->im,
				((vartype_real *) src2)->x,
				&rre, &rim);
		    if (error == ERR_NONE) {
			*dst = new_complex(rre, rim);
			if (*dst == NULL)
			    return ERR_INSUFFICIENT_MEMORY;
		    }
		    return error;
		}
		case TYPE_COMPLEX: {
		    double rre, rim;
		    error = mcc(((vartype_complex *) src1)->re,
				((vartype_complex *) src1)->im,
				((vartype_complex *) src2)->re,
				((vartype_complex *) src2)->im,
				&rre, &rim);
		    if (error == ERR_NONE) {
			*dst = new_complex(rre, rim);
			if (*dst == NULL)
			    return ERR_INSUFFICIENT_MEMORY;
		    }
		    return error;
		}
		case TYPE_REALMATRIX: {
		    vartype_realmatrix *sm = (vartype_realmatrix *) src2;
		    vartype_complexmatrix *dm;
		    int4 size, i;
		    int error;
		    dm = (vartype_complexmatrix *)
				    new_complexmatrix(sm->rows, sm->columns);
		    if (dm == NULL)
			return ERR_INSUFFICIENT_MEMORY;
		    size = sm->rows * sm->columns;
		    for (i = 0; i < size; i++)
			if (sm->array->is_string[i]) {
			    free_vartype((vartype *) dm);
			    return ERR_ALPHA_DATA_IS_INVALID;
			}
		    for (i = 0; i < size; i++) {
			error = mcr(((vartype_complex *) src1)->re,
				    ((vartype_complex *) src1)->im,
				    sm->array->data[i].d,
				    &dm->array->data[i * 2],
				    &dm->array->data[i * 2 + 1]);
			if (error != ERR_NONE) {
			    free_vartype((vartype *) dm);
			    return error;
			}
		    }
		    *dst = (vartype *) dm;
		    return ERR_NONE;
		}
		case TYPE_COMPLEXMATRIX: {
		    vartype_complexmatrix *sm = (vartype_complexmatrix *) src2;
		    vartype_complexmatrix *dm;
		    int4 size, i;
		    int error;
		    dm = (vartype_complexmatrix *)
				    new_complexmatrix(sm->rows, sm->columns);
		    if (dm == NULL)
			return ERR_INSUFFICIENT_MEMORY;
		    size = 2 * sm->rows * sm->columns;
		    for (i = 0; i < size; i += 2) {
			error = mcc(((vartype_complex *) src1)->re,
				    ((vartype_complex *) src1)->im,
				    sm->array->data[i],
				    sm->array->data[i + 1],
				    &dm->array->data[i],
				    &dm->array->data[i + 1]);
			if (error != ERR_NONE) {
			    free_vartype((vartype *) dm);
			    return error;
			}
		    }
		    *dst = (vartype *) dm;
		    return ERR_NONE;
		}
		default:
		    return ERR_INTERNAL_ERROR;
	    }
	case TYPE_REALMATRIX:
	    switch (src2->type) {
		case TYPE_REAL: {
		    vartype_realmatrix *sm = (vartype_realmatrix *) src1;
		    vartype_realmatrix *dm;
		    int4 size, i;
		    int error;
		    dm = (vartype_realmatrix *)
					new_realmatrix(sm->rows, sm->columns);
		    if (dm == NULL)
			return ERR_INSUFFICIENT_MEMORY;
		    size = sm->rows * sm->columns;
		    for (i = 0; i < size; i++)
			if (sm->array->is_string[i]) {
			    free_vartype((vartype *) dm);
			    return ERR_ALPHA_DATA_IS_INVALID;
			}
		    for (i = 0; i < size; i++) {
			error = mrr(sm->array->data[i].d,
				    ((vartype_real *) src2)->x,
				    &dm->array->data[i].d);
			if (error != ERR_NONE) {
			    free_vartype((vartype *) dm);
			    return error;
			}
		    }
		    *dst = (vartype *) dm;
		    return ERR_NONE;
		}
		case TYPE_COMPLEX: {
		    vartype_realmatrix *sm = (vartype_realmatrix *) src1;
		    vartype_complexmatrix *dm;
		    int4 size, i;
		    int error;
		    dm = (vartype_complexmatrix *)
				    new_complexmatrix(sm->rows, sm->columns);
		    if (dm == NULL)
			return ERR_INSUFFICIENT_MEMORY;
		    size = sm->rows * sm->columns;
		    for (i = 0; i < size; i++)
			if (sm->array->is_string[i]) {
			    free_vartype((vartype *) dm);
			    return ERR_ALPHA_DATA_IS_INVALID;
			}
		    for (i = 0; i < size; i++) {
			error = mrc(sm->array->data[i].d,
				    ((vartype_complex *) src2)->re,
				    ((vartype_complex *) src2)->im,
				    &dm->array->data[i * 2],
				    &dm->array->data[i * 2 + 1]);
			if (error != ERR_NONE) {
			    free_vartype((vartype *) dm);
			    return error;
			}
		    }
		    *dst = (vartype *) dm;
		    return ERR_NONE;
		}
		case TYPE_REALMATRIX: {
		    vartype_realmatrix *sm1 = (vartype_realmatrix *) src1;
		    vartype_realmatrix *sm2 = (vartype_realmatrix *) src2;
		    vartype_realmatrix *dm;
		    int4 size, i;
		    int error;
		    if (sm1->rows != sm2->rows || sm1->columns != sm2->columns)
			return ERR_DIMENSION_ERROR;
		    dm = (vartype_realmatrix *)
				    new_realmatrix(sm1->rows, sm1->columns);
		    if (dm == NULL)
			return ERR_INSUFFICIENT_MEMORY;
		    size = sm1->rows * sm1->columns;
		    for (i = 0; i < size; i++)
			if (sm1->array->is_string[i]
				    || sm2->array->is_string[i]) {
			    free_vartype((vartype *) dm);
			    return ERR_ALPHA_DATA_IS_INVALID;
			}
		    for (i = 0; i < size; i++) {
			error = mrr(sm1->array->data[i].d,
				    sm2->array->data[i].d,
				    &dm->array->data[i].d);
			if (error != ERR_NONE) {
			    free_vartype((vartype *) dm);
			    return error;
			}
		    }
		    *dst = (vartype *) dm;
		    return ERR_NONE;
		}
		case TYPE_COMPLEXMATRIX: {
		    vartype_realmatrix *sm1 = (vartype_realmatrix *) src1;
		    vartype_complexmatrix *sm2 = (vartype_complexmatrix *) src2;
		    vartype_complexmatrix *dm;
		    int4 size, i;
		    int error;
		    if (sm1->rows != sm2->rows || sm1->columns != sm2->columns)
			return ERR_DIMENSION_ERROR;
		    dm = (vartype_complexmatrix *)
				    new_complexmatrix(sm1->rows, sm1->columns);
		    if (dm == NULL)
			return ERR_INSUFFICIENT_MEMORY;
		    size = sm1->rows * sm1->columns;
		    for (i = 0; i < size; i++)
			if (sm1->array->is_string[i]) {
			    free_vartype((vartype *) dm);
			    return ERR_ALPHA_DATA_IS_INVALID;
			}
		    for (i = 0; i < size; i++) {
			error = mrc(sm1->array->data[i].d,
				    sm2->array->data[i * 2],
				    sm2->array->data[i * 2 + 1],
				    &dm->array->data[i * 2],
				    &dm->array->data[i * 2 + 1]);
			if (error != ERR_NONE) {
			    free_vartype((vartype *) dm);
			    return error;
			}
		    }
		    *dst = (vartype *) dm;
		    return ERR_NONE;
		}
		default:
		    return ERR_INTERNAL_ERROR;
	    }
	case TYPE_COMPLEXMATRIX:
	    switch (src2->type) {
		case TYPE_REAL: {
		    vartype_complexmatrix *sm = (vartype_complexmatrix *) src1;
		    vartype_complexmatrix *dm;
		    int4 size, i;
		    int error;
		    dm = (vartype_complexmatrix *)
				    new_complexmatrix(sm->rows, sm->columns);
		    if (dm == NULL)
			return ERR_INSUFFICIENT_MEMORY;
		    size = 2 * sm->rows * sm->columns;
		    for (i = 0; i < size; i += 2) {
			error = mcr(sm->array->data[i],
				    sm->array->data[i + 1],
				    ((vartype_real *) src2)->x,
				    &dm->array->data[i],
				    &dm->array->data[i + 1]);
			if (error != ERR_NONE) {
			    free_vartype((vartype *) dm);
			    return error;
			}
		    }
		    *dst = (vartype *) dm;
		    return ERR_NONE;
		}
		case TYPE_COMPLEX: {
		    vartype_complexmatrix *sm = (vartype_complexmatrix *) src1;
		    vartype_complexmatrix *dm;
		    int4 size, i;
		    int error;
		    dm = (vartype_complexmatrix *)
				    new_complexmatrix(sm->rows, sm->columns);
		    if (dm == NULL)
			return ERR_INSUFFICIENT_MEMORY;
		    size = 2 * sm->rows * sm->columns;
		    for (i = 0; i < size; i++) {
			error = mcc(sm->array->data[i],
				    sm->array->data[i + 1],
				    ((vartype_complex *) src2)->re,
				    ((vartype_complex *) src2)->im,
				    &dm->array->data[i * 2],
				    &dm->array->data[i * 2 + 1]);
			if (error != ERR_NONE) {
			    free_vartype((vartype *) dm);
			    return error;
			}
		    }
		    *dst = (vartype *) dm;
		    return ERR_NONE;
		}
		case TYPE_REALMATRIX: {
		    vartype_complexmatrix *sm1 = (vartype_complexmatrix *) src1;
		    vartype_realmatrix *sm2 = (vartype_realmatrix *) src2;
		    vartype_complexmatrix *dm;
		    int4 size, i;
		    int error;
		    if (sm1->rows != sm2->rows || sm1->columns != sm2->columns)
			return ERR_DIMENSION_ERROR;
		    dm = (vartype_complexmatrix *)
				    new_complexmatrix(sm1->rows, sm1->columns);
		    if (dm == NULL)
			return ERR_INSUFFICIENT_MEMORY;
		    size = sm1->rows * sm1->columns;
		    for (i = 0; i < size; i++)
			if (sm2->array->is_string[i]) {
			    free_vartype((vartype *) dm);
			    return ERR_ALPHA_DATA_IS_INVALID;
			}
		    for (i = 0; i < size; i++) {
			error = mcr(sm1->array->data[i * 2],
				    sm1->array->data[i * 2 + 1],
				    sm2->array->data[i].d,
				    &dm->array->data[i * 2],
				    &dm->array->data[i * 2 + 1]);
			if (error != ERR_NONE) {
			    free_vartype((vartype *) dm);
			    return error;
			}
		    }
		    *dst = (vartype *) dm;
		    return ERR_NONE;
		}
		case TYPE_COMPLEXMATRIX: {
		    vartype_complexmatrix *sm1 = (vartype_complexmatrix *) src1;
		    vartype_complexmatrix *sm2 = (vartype_complexmatrix *) src2;
		    vartype_complexmatrix *dm;
		    int4 size, i;
		    int error;
		    if (sm1->rows != sm2->rows || sm1->columns != sm2->columns)
			return ERR_DIMENSION_ERROR;
		    dm = (vartype_complexmatrix *)
				    new_complexmatrix(sm1->rows, sm1->columns);
		    if (dm == NULL)
			return ERR_INSUFFICIENT_MEMORY;
		    size = 2 * sm1->rows * sm1->columns;
		    for (i = 0; i < size; i++) {
			error = mcc(sm1->array->data[i * 2],
				    sm1->array->data[i * 2 + 1],
				    sm2->array->data[i * 2],
				    sm2->array->data[i * 2 + 1],
				    &dm->array->data[i * 2],
				    &dm->array->data[i * 2 + 1]);
			if (error != ERR_NONE) {
			    free_vartype((vartype *) dm);
			    return error;
			}
		    }
		    *dst = (vartype *) dm;
		    return ERR_NONE;
		}
		default:
		    return ERR_INTERNAL_ERROR;
	    }
	default:
	    return ERR_INTERNAL_ERROR;
    }
}

int div_rr(double x, double y, double *z) {
    double r;
    int inf;
    if (x == 0)
	return ERR_DIVIDE_BY_0;
    r = y / x;
    inf = isinf(r);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    r = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *z = r;
    return ERR_NONE;
}
    
int div_rc(double x, double yre, double yim, double *zre, double *zim) {
    double rre, rim;
    int inf;
    if (x == 0)
	return ERR_DIVIDE_BY_0;
    rre = yre / x;
    inf = isinf(rre);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    rre = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    rim = yim / x;
    inf = isinf(rim);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    rim = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = rim;
    return ERR_NONE;
}

int div_cr(double xre, double xim, double y, double *zre, double *zim) {
    double rre, rim, h;
    int inf;
    /* TODO: overflows in intermediate results */
    h = xre * xre + xim * xim;
    if (h == 0)
	return ERR_DIVIDE_BY_0;
    rre = y * xre / h;
    inf = isinf(rre);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    rre = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    rim = -y * xim / h;
    inf = isinf(rim);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    rim = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = rim;
    return ERR_NONE;
}

int div_cc(double xre, double xim, double yre, double yim,
						    double *zre, double *zim) {
    double rre, rim, h;
    int inf;
    /* TODO: overflows in intermediate results */
    h = xre * xre + xim * xim;
    if (h == 0)
	return ERR_DIVIDE_BY_0;
    rre = (xre * yre + xim * yim) / h;
    inf = isinf(rre);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    rre = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    rim = (xre * yim - yre * xim) / h;
    inf = isinf(rim);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    rim = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = rim;
    return ERR_NONE;
}

int mul_rr(double x, double y, double *z) {
    double r;
    int inf;
    r = y * x;
    inf = isinf(r);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    r = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *z = r;
    return ERR_NONE;
}
    
int mul_rc(double x, double yre, double yim, double *zre, double *zim) {
    double rre, rim;
    int inf;
    rre = yre * x;
    inf = isinf(rre);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    rre = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    rim = yim * x;
    inf = isinf(rim);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    rim = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = rim;
    return ERR_NONE;
}

int mul_cr(double xre, double xim, double y, double *zre, double *zim) {
    double rre, rim;
    int inf;
    rre = y * xre;
    inf = isinf(rre);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    rre = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    rim = y * xim;
    inf = isinf(rim);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    rim = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = rim;
    return ERR_NONE;
}

int mul_cc(double xre, double xim, double yre, double yim,
						    double *zre, double *zim) {
    double rre, rim;
    int inf;
    rre = xre * yre - xim * yim;
    inf = isinf(rre);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    rre = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    rim = xre * yim + yre * xim;
    inf = isinf(rim);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    rim = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = rim;
    return ERR_NONE;
}

int sub_rr(double x, double y, double *z) {
    double r;
    int inf;
    r = y - x;
    inf = isinf(r);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    r = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *z = r;
    return ERR_NONE;
}
    
int sub_rc(double x, double yre, double yim, double *zre, double *zim) {
    double rre;
    int inf;
    rre = yre - x;
    inf = isinf(rre);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    rre = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = yim;
    return ERR_NONE;
}

int sub_cr(double xre, double xim, double y, double *zre, double *zim) {
    double rre;
    int inf;
    rre = y - xre;
    inf = isinf(rre);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    rre = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = -xim;
    return ERR_NONE;
}

int sub_cc(double xre, double xim, double yre, double yim,
						    double *zre, double *zim) {
    double rre, rim;
    int inf;
    rre = yre - xre;
    inf = isinf(rre);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    rre = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    rim = yim - xim;
    inf = isinf(rim);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    rim = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = rim;
    return ERR_NONE;
}

int add_rr(double x, double y, double *z) {
    double r;
    int inf;
    r = y + x;
    inf = isinf(r);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    r = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *z = r;
    return ERR_NONE;
}
    
int add_rc(double x, double yre, double yim, double *zre, double *zim) {
    double rre;
    int inf;
    rre = yre + x;
    inf = isinf(rre);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    rre = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = yim;
    return ERR_NONE;
}

int add_cr(double xre, double xim, double y, double *zre, double *zim) {
    double rre;
    int inf;
    rre = y + xre;
    inf = isinf(rre);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    rre = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = xim;
    return ERR_NONE;
}

int add_cc(double xre, double xim, double yre, double yim,
						    double *zre, double *zim) {
    double rre, rim;
    int inf;
    rre = yre + xre;
    inf = isinf(rre);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    rre = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    rim = yim + xim;
    inf = isinf(rim);
    if (inf != 0) {
	if (flags.f.range_error_ignore)
	    rim = inf == 1 ? POS_HUGE_DOUBLE : NEG_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *zre = rre;
    *zim = rim;
    return ERR_NONE;
}

int dimension_array(const char *name, int namelen, int4 rows, int4 columns) {
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
	    int4 i, s, oldsize;
	    double_or_string *new_data = (double_or_string *)
				    realloc(oldmatrix->array->data,
					    size * sizeof(double_or_string));
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
		new_data[i].d = 0;
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
	    new_array->data = (double_or_string *)
				malloc(size * sizeof(double_or_string));
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
		new_array->data[i].d = 0;
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
	    double *new_data = (double *)
		    realloc(oldmatrix->array->data, 2 * size * sizeof(double));
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
	    new_array->data = (double *) malloc(2 * size * sizeof(double));
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

double fix_hms(double x) {
    /* NOTE: this function assumes x is non-negative */
    if (x == x + 1)
	return x;
    if (x < 0.0059)
	return x;
    if (((int8) (x * 10000)) % 100 == 60)
	x += 0.004;
    if (((int8) (x * 100)) % 100 == 60) {
	x += 0.4;
	if (((int8) (x * 10000)) % 100 == 60)
	    x += 0.004;
    }
    return x;
}

#ifdef NO_SINCOS

void sincos(double x, double *sinx, double *cosx) {
    *sinx = sin(x);
    *cosx = cos(x);
}

#endif
