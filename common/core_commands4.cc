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

#include "core_commands2.h"
#include "core_commands3.h"
#include "core_commands4.h"
#include "core_decimal.h"
#include "core_display.h"
#include "core_helpers.h"
#include "core_linalg.h"
#include "core_main.h"
#include "core_math.h"
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

int docmd_insr(arg_struct *arg) {
    vartype *m, *newx;
    vartype_realmatrix *rm;
    vartype_complexmatrix *cm;
    int4 rows, columns, i;
    int err, refcount;
    int interactive;

    switch (matedit_mode) {
	case 0:
	    return ERR_NONEXISTENT;
	case 1:
	case 3:
	    m = recall_var(matedit_name, matedit_length);
	    break;
	case 2:
	    m = matedit_x;
	    break;
	default:
	    return ERR_INTERNAL_ERROR;
    }
    if (m == NULL)
	return ERR_NONEXISTENT;
    if (m->type != TYPE_REALMATRIX && m->type != TYPE_COMPLEXMATRIX)
	return ERR_INVALID_TYPE;

    interactive = matedit_mode == 2 || matedit_mode == 3;
    if (interactive) {
	err = docmd_stoel(NULL);
	if (err != ERR_NONE)
	    return err;
    }

    if (m->type == TYPE_REALMATRIX) {
	rm = (vartype_realmatrix *) m;
	rows = rm->rows;
	columns = rm->columns;
	refcount = rm->array->refcount;
	if (interactive) {
	    newx = new_real(0);
	    if (newx == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	}
    } else {
	cm = (vartype_complexmatrix *) m;
	rows = cm->rows;
	columns = cm->columns;
	refcount = cm->array->refcount;
	if (interactive) {
	    newx = new_complex(0, 0);
	    if (newx == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	}
    }

    if (matedit_i >= rows)
	matedit_i = rows - 1;
    if (matedit_j >= columns)
	matedit_j = columns - 1;

    if (refcount == 1) {
	/* We have this array to ourselves so we can modify it in place */
	err = dimension_array_ref(m, rows + 1, columns);
	if (err != ERR_NONE) {
	    if (interactive)
		free_vartype(newx);
	    return err;
	}
	rows++;
	if (m->type == TYPE_REALMATRIX) {
	    for (i = rows * columns - 1; i >= (matedit_i + 1) * columns; i--) {
		rm->array->is_string[i] = rm->array->is_string[i - columns];
		rm->array->data[i] = rm->array->data[i - columns];
	    }
	    for (i = matedit_i * columns; i < (matedit_i + 1) * columns; i++) {
		rm->array->is_string[i] = 0;
		rm->array->data[i].d = 0;
	    }
	} else {
	    for (i = 2 * rows * columns - 1;
			    i >= 2 * (matedit_i + 1) * columns; i--)
		cm->array->data[i] = cm->array->data[i - 2 * columns];
	    for (i = 2 * matedit_i * columns;
			    i < 2 * (matedit_i + 1) * columns; i++)
		cm->array->data[i] = 0;
	}
    } else {
	/* We're sharing this array. I don't use disentangle() because it
	 * does not deal with resizing. */
	int4 newsize = (rows + 1) * columns;
	if (m->type == TYPE_REALMATRIX) {
	    realmatrix_data *array = (realmatrix_data *)
				malloc(sizeof(realmatrix_data));
	    if (array == NULL) {
		if (interactive)
		    free_vartype(newx);
		return ERR_INSUFFICIENT_MEMORY;
	    }
	    array->data = (double_or_string *)
				malloc(newsize * sizeof(double_or_string));
	    if (array->data == NULL) {
		if (interactive)
		    free_vartype(newx);
		free(array);
		return ERR_INSUFFICIENT_MEMORY;
	    }
	    array->is_string = (char *) malloc(newsize);
	    if (array->is_string == NULL) {
		if (interactive)
		    free_vartype(newx);
		free(array->data);
		free(array);
		return ERR_INSUFFICIENT_MEMORY;
	    }
	    for (i = 0; i < matedit_i * columns; i++) {
		array->is_string[i] = rm->array->is_string[i];
		array->data[i] = rm->array->data[i];
	    }
	    for (i = matedit_i * columns; i < (matedit_i + 1) * columns; i++) {
		array->is_string[i] = 0;
		array->data[i].d = 0;
	    }
	    for (i = (matedit_i + 1) * columns; i < newsize; i++) {
		array->is_string[i] = rm->array->is_string[i - columns];
		array->data[i] = rm->array->data[i - columns];
	    }
	    array->refcount = 1;
	    rm->array->refcount--;
	    rm->array = array;
	    rm->rows++;
	} else {
	    complexmatrix_data *array = (complexmatrix_data *)
				malloc(sizeof(complexmatrix_data));
	    if (array == NULL) {
		if (interactive)
		    free_vartype(newx);
		return ERR_INSUFFICIENT_MEMORY;
	    }
	    array->data = (double *) malloc(2 * newsize * sizeof(double));
	    if (array->data == NULL) {
		if (interactive)
		    free_vartype(newx);
		free(array);
		return ERR_INSUFFICIENT_MEMORY;
	    }
	    for (i = 0; i < 2 * matedit_i * columns; i++)
		array->data[i] = cm->array->data[i];
	    for (i = 2 * matedit_i * columns;
				i < 2 * (matedit_i + 1) * columns; i++)
		array->data[i] = 0;
	    for (i = 2 * (matedit_i + 1) * columns; i < 2 * newsize; i++)
		array->data[i] = cm->array->data[i - 2 * columns];
	    array->refcount = 1;
	    cm->array->refcount--;
	    cm->array = array;
	    cm->rows++;
	}
    }
    if (interactive) {
	free_vartype(reg_x);
	reg_x = newx;
    }
    return ERR_NONE;
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

static void invrt_completion(int error, vartype *res) COMMANDS4_SECT;
static void invrt_completion(int error, vartype *res) {
    if (error == ERR_NONE)
	unary_result(res);
}

int docmd_invrt(arg_struct *arg) {
    return linalg_inv(reg_x, invrt_completion);
}

int docmd_j_add(arg_struct *arg) {
    int4 rows, columns;
    int4 oldi = matedit_i;
    int4 oldj = matedit_j;
    int err = matedit_get_dim(&rows, &columns);
    if (err != ERR_NONE)
	return err;
    if (++matedit_j >= columns) {
	flags.f.matrix_edge_wrap = 1;
	matedit_j = 0;
	if (++matedit_i >= rows) {
	    flags.f.matrix_end_wrap = 1;
	    if (flags.f.grow) {
		if (matedit_mode == 2)
		    err = dimension_array_ref(matedit_x, rows + 1, columns);
		else
		    err = dimension_array(matedit_name, matedit_length,
					    rows + 1, columns);
		if (err != ERR_NONE) {
		    matedit_i = oldi;
		    matedit_j = oldj;
		    return err;
		}
		matedit_i = rows;
	    } else
		matedit_i = 0;
	} else
	    flags.f.matrix_end_wrap = 0;
    } else {
	flags.f.matrix_edge_wrap = 0;
	flags.f.matrix_end_wrap = 0;
    }
    return ERR_NONE;
}

int docmd_j_sub(arg_struct *arg) {
    int4 rows, columns;
    int err = matedit_get_dim(&rows, &columns);
    if (err != ERR_NONE)
	return err;
    if (--matedit_j < 0) {
	flags.f.matrix_edge_wrap = 1;
	matedit_j = columns - 1;
	if (--matedit_i < 0) {
	    flags.f.matrix_end_wrap = 1;
	    matedit_i = rows - 1;
	} else
	    flags.f.matrix_end_wrap = 0;
    } else {
	flags.f.matrix_edge_wrap = 0;
	flags.f.matrix_end_wrap = 0;
    }
    return ERR_NONE;
}

int docmd_linf(arg_struct *arg) {
    flags.f.lin_fit = 1;
    flags.f.log_fit = 0;
    flags.f.exp_fit = 0;
    flags.f.pwr_fit = 0;
    return ERR_NONE;
}

int docmd_linsigma(arg_struct *arg) {
    flags.f.all_sigma = 0;
    return ERR_NONE;
}

static int mappable_ln_1_x(double x, double *y) COMMANDS4_SECT;
static int mappable_ln_1_x(double x, double *y) {
    if (x <= -1)
	return ERR_INVALID_DATA;
    *y = log1p(x);
    return ERR_NONE;
}   

int docmd_ln_1_x(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL || reg_x->type == TYPE_REALMATRIX) {
	vartype *v;
	int err = map_unary(reg_x, &v, mappable_ln_1_x, NULL);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    } else if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else
	return ERR_INVALID_TYPE;
}

int docmd_logf(arg_struct *arg) {
    flags.f.lin_fit = 0;
    flags.f.log_fit = 1;
    flags.f.exp_fit = 0;
    flags.f.pwr_fit = 0;
    return ERR_NONE;
}

int docmd_not(arg_struct *arg) {
    int8 x;
    int err;
    vartype *v;
    if ((err = get_base_param(reg_x, &x)) != ERR_NONE)
	return err;
    v = new_real((double) ~x);
    if (v == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

int docmd_old(arg_struct *arg) {
    return docmd_rclel(NULL);
}

int docmd_or(arg_struct *arg) {
    int8 x, y; 
    int err; 
    vartype *v;
    if ((err = get_base_param(reg_x, &x)) != ERR_NONE) 
	return err;
    if ((err = get_base_param(reg_y, &y)) != ERR_NONE)
	return err;
    v = new_real((double) (x | y));
    if (v == NULL) 
	return ERR_INSUFFICIENT_MEMORY;
    binary_result(v);
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

int appmenu_exitcallback_3(int menuid) {
    if (menuid == MENU_NONE) {
	set_menu(MENULEVEL_APP, MENU_CATALOG);
	set_cat_section(CATSECT_PGM_SOLVE);
    } else
	mode_appmenu = menuid;
    return ERR_NONE;
}

int docmd_pgmslvi(arg_struct *arg) {
    /* This command can only be invoked from a menu; we assume that
     * the menu handler only gives us valid arguments. We do check
     * the argument type, but the existence of the named label, and
     * whether it actually has MVAR instructions, we just assume.
     */
    if (arg->type == ARGTYPE_STR) {
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

int appmenu_exitcallback_4(int menuid) {
    if (menuid == MENU_NONE) {
	set_menu(MENULEVEL_APP, MENU_CATALOG);
	set_cat_section(CATSECT_PGM_INTEG);
    } else
	mode_appmenu = menuid;
    return ERR_NONE;
}

int appmenu_exitcallback_5(int menuid) {
    if (menuid == MENU_NONE) {
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
    /* This command can only be invoked from a menu; we assume that
     * the menu handler only gives us valid arguments. We do check
     * the argument type, but the existence of the named label, and
     * whether it actually has MVAR instructions, we just assume.
     */
    if (arg->type == ARGTYPE_STR) {
	set_integ_prgm(arg->val.text, arg->length);
	string_copy(varmenu, &varmenu_length, arg->val.text, arg->length);
	varmenu_row = 0;
	varmenu_role = 2;
	set_menu(MENULEVEL_APP, MENU_VARMENU);
	set_appmenu_exitcallback(4);
	clear_row(0);
	draw_string(0, 0, "Set Vars; Select \003var", 21);
	flags.f.message = 1;
	flags.f.two_line_message = 0;
	mode_varmenu = 1;
	return ERR_NONE;
    } else
	return ERR_INVALID_TYPE;
}

int docmd_posa(arg_struct *arg) {
    int pos = -1;
    vartype *v;
    if (reg_x->type == TYPE_REAL) {
	double x = ((vartype_real *) reg_x)->x;
	char c;
	int i;
	if (x < 0)
	    x = -x;
	if (core_settings.ip_hack)
	    x += 5e-9;
	if (x >= 256)
	    return ERR_INVALID_DATA;
	c = (char) x;
	for (i = 0; i < reg_alpha_length; i++)
	    if (reg_alpha[i] == c) {
		pos = i;
		break;
	    }
    } else if (reg_x->type == TYPE_STRING) {
	vartype_string *s = (vartype_string *) reg_x;
	if (s->length != 0) {
	    int i, j;
	    for (i = 0; i < reg_alpha_length - s->length; i++) {
		for (j = 0; j < s->length; j++)
		    if (reg_alpha[i + j] != s->text[j])
			goto notfound;
		pos = i;
		break;
		notfound:;
	    }
	}
    } else
	return ERR_INVALID_TYPE;
    v = new_real(pos);
    if (v == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

int docmd_putm(arg_struct *arg) {
    vartype *m;
    int4 i, j;

    switch (matedit_mode) {
	case 0:
	    return ERR_NONEXISTENT;
	case 1:
	case 3:
	    m = recall_var(matedit_name, matedit_length);
	    break;
	case 2:
	    m = matedit_x;
	    break;
	default:
	    return ERR_INTERNAL_ERROR;
    }

    if (m == NULL)
	return ERR_NONEXISTENT;

    if (m->type != TYPE_REALMATRIX && m->type != TYPE_COMPLEXMATRIX)
	/* Shouldn't happen, but could, as long as I don't
	 * implement matrix locking
	 */
	return ERR_INVALID_TYPE;

    if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else if (reg_x->type == TYPE_REAL || reg_x->type == TYPE_COMPLEX)
	return ERR_INVALID_TYPE;

    if (m->type == TYPE_REALMATRIX) {
	vartype_realmatrix *src, *dst;
	if (reg_x->type == TYPE_COMPLEXMATRIX)
	    return ERR_INVALID_TYPE;
	src = (vartype_realmatrix *) reg_x;
	dst = (vartype_realmatrix *) m;
	if (src->rows + matedit_i > dst->rows
		|| src->columns + matedit_j > dst->columns)
	    return ERR_DIMENSION_ERROR;
	if (!disentangle(m))
	    return ERR_INSUFFICIENT_MEMORY;
	for (i = 0; i < src->rows; i++)
	    for (j = 0; j < src->columns; j++) {
		int4 n1 = i * src->columns + j;
		int4 n2 = (i + matedit_i) * dst->columns + j + matedit_j;
		dst->array->is_string[n2] = src->array->is_string[n1];
		dst->array->data[n2] = src->array->data[n1];
	    }
	return ERR_NONE;
    } else if (reg_x->type == TYPE_REALMATRIX) {
	vartype_realmatrix *src = (vartype_realmatrix *) reg_x;
	vartype_complexmatrix *dst = (vartype_complexmatrix *) m;
	if (src->rows + matedit_i > dst->rows
		|| src->columns + matedit_j > dst->columns)
	    return ERR_DIMENSION_ERROR;
	for (i = 0; i < src->rows * src->columns; i++)
	    if (src->array->is_string[i])
		return ERR_ALPHA_DATA_IS_INVALID;
	if (!disentangle(m))
	    return ERR_INSUFFICIENT_MEMORY;
	for (i = 0; i < src->rows; i++)
	    for (j = 0; j < src->columns; j++) {
		int4 n1 = i * src->columns + j;
		int4 n2 = (i + matedit_i) * dst->columns + j + matedit_j;
		dst->array->data[n2 * 2] = src->array->data[n1].d;
		dst->array->data[n2 * 2 + 1] = 0;
	    }
	return ERR_NONE;
    } else {
	vartype_complexmatrix *src = (vartype_complexmatrix *) reg_x;
	vartype_complexmatrix *dst = (vartype_complexmatrix *) m;
	if (src->rows + matedit_i > dst->rows
		|| src->columns + matedit_j > dst->columns)
	    return ERR_DIMENSION_ERROR;
	if (!disentangle(m))
	    return ERR_INSUFFICIENT_MEMORY;
	for (i = 0; i < src->rows; i++)
	    for (j = 0; j < src->columns; j++) {
		int4 n1 = i * src->columns + j;
		int4 n2 = (i + matedit_i) * dst->columns + j + matedit_j;
		dst->array->data[n2 * 2] = src->array->data[n1 * 2];
		dst->array->data[n2 * 2 + 1] = src->array->data[n1 * 2 + 1];
	    }
	return ERR_NONE;
    }
}

int docmd_pwrf(arg_struct *arg) {
    flags.f.lin_fit = 0;
    flags.f.log_fit = 0;
    flags.f.exp_fit = 0;
    flags.f.pwr_fit = 1;
    return ERR_NONE;
}

int docmd_rclel(arg_struct *arg) {
    vartype *m, *v;
    switch (matedit_mode) {
	case 0:
	    return ERR_NONEXISTENT;
	case 1:
	case 3:
	    m = recall_var(matedit_name, matedit_length);
	    break;
	case 2:
	    m = matedit_x;
	    break;
	default:
	    return ERR_INTERNAL_ERROR;
    }

    if (m == NULL)
	return ERR_NONEXISTENT;

    if (m->type == TYPE_REALMATRIX) {
	vartype_realmatrix *rm = (vartype_realmatrix *) m;
	int4 n = matedit_i * rm->columns + matedit_j;
	if (rm->array->is_string[n])
	    v = new_string(rm->array->data[n].s.text,
			   rm->array->data[n].s.length);
	else
	    v = new_real(rm->array->data[n].d);
    } else if (m->type == TYPE_COMPLEXMATRIX) {
	vartype_complexmatrix *cm = (vartype_complexmatrix *) m;
	int4 n = matedit_i * cm->columns + matedit_j;
	v = new_complex(cm->array->data[2 * n],
			cm->array->data[2 * n + 1]);
    } else
	return ERR_INVALID_TYPE;
    if (v == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    recall_result(v);
    return ERR_NONE;
}

int docmd_rclij(arg_struct *arg) {
    vartype *i, *j;
    if (matedit_mode == 0)
	return ERR_NONEXISTENT;
    i = new_real(matedit_i + 1);
    j = new_real(matedit_j + 1);
    if (i == NULL || j == NULL) {
	free_vartype(i);
	free_vartype(j);
	return ERR_INSUFFICIENT_MEMORY;
    }
    recall_two_results(j, i);
    return ERR_NONE;
}

int docmd_rnrm(arg_struct *arg) {
    if (reg_x->type == TYPE_REALMATRIX) {
	vartype *v;
	vartype_realmatrix *rm = (vartype_realmatrix *) reg_x;
	int4 size = rm->rows * rm->columns;
	int4 i, j;
	double max = 0;
	for (i = 0; i < size; i++)
	    if (rm->array->is_string[i])
		return ERR_ALPHA_DATA_IS_INVALID;
	for (i = 0; i < rm->rows; i++) {
	    double nrm = 0;
	    for (j = 0; j < rm->columns; j++) {
		double x = rm->array->data[i * rm->columns + j].d;
		if (x >= 0)
		    nrm += x;
		else
		    nrm -= x;
	    }
	    if (isinf(nrm)) {
		if (flags.f.range_error_ignore)
		    max = POS_HUGE_DOUBLE;
		else
		    return ERR_OUT_OF_RANGE;
		break;
	    }
	    if (nrm > max)
		max = nrm;
	}
	v = new_real(max);
	if (v == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	unary_result(v);
	return ERR_NONE;
    } else if (reg_x->type == TYPE_COMPLEXMATRIX) {
	vartype *v;
	vartype_complexmatrix *cm = (vartype_complexmatrix *) reg_x;
	int4 i, j;
	double max = 0;
	for (i = 0; i < cm->rows; i++) {
	    double nrm = 0;
	    for (j = 0; j < cm->columns; j++) {
		double re = cm->array->data[2 * (i * cm->columns + j)];
		double im = cm->array->data[2 * (i * cm->columns + j) + 1];
		nrm += hypot(re, im);
	    }
	    if (isinf(nrm)) {
		if (flags.f.range_error_ignore)
		    max = POS_HUGE_DOUBLE;
		else
		    return ERR_OUT_OF_RANGE;
		break;
	    }
	    if (nrm > max)
		max = nrm;
	}
	v = new_real(max);
	if (v == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	unary_result(v);
	return ERR_NONE;
    } else if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else
	return ERR_INVALID_TYPE;
}

int docmd_rotxy(arg_struct *arg) {
    int8 x, y, res;
    int err; 
    vartype *v;
    if ((err = get_base_param(reg_x, &x)) != ERR_NONE) 
	return err;
    if ((err = get_base_param(reg_y, &y)) != ERR_NONE)
	return err;
    if (x < -35 || x > 35)
	return ERR_INVALID_DATA;
    if (x == 0)
	res = y;
    else {
	y &= 0xfffffffffLL;
	if (x > 0)
	    res = (y >> x) | (y << (36 - x));
	else {
	    x = -x;
	    res = (y << x) | (y >> (36 - x));
	}
	if ((res & 0x800000000LL) == 0)
	    res &= 0x7ffffffffLL;
	else
	    res |= 0xfffffff000000000LL;
    }
    v = new_real((double) res);
    if (v == NULL) 
	return ERR_INSUFFICIENT_MEMORY;
    binary_result(v);
    return ERR_NONE;
}

int docmd_rsum(arg_struct *arg) {
    if (reg_x->type == TYPE_REALMATRIX) {
	vartype_realmatrix *rm = (vartype_realmatrix *) reg_x;
	vartype_realmatrix *res;
	int4 size = rm->rows * rm->columns;
	int4 i, j;
	for (i = 0; i < size; i++)
	    if (rm->array->is_string[i])
		return ERR_ALPHA_DATA_IS_INVALID;
	res = (vartype_realmatrix *) new_realmatrix(rm->rows, 1);
	if (res == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	for (i = 0; i < rm->rows; i++) {
	    double sum = 0;
	    int inf;
	    for (j = 0; j < rm->columns; j++)
		sum += rm->array->data[i * rm->columns + j].d;
	    if ((inf = isinf(sum)) != 0) {
		if (flags.f.range_error_ignore)
		    sum = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
		else {
		    free_vartype((vartype *) res);
		    return ERR_OUT_OF_RANGE;
		}
	    }
	    res->array->data[i].d = sum;
	}
	unary_result((vartype *) res);
	return ERR_NONE;
    } else if (reg_x->type == TYPE_COMPLEXMATRIX) {
	vartype_complexmatrix *cm = (vartype_complexmatrix *) reg_x;
	vartype_complexmatrix *res;
	int4 i, j;
	res = (vartype_complexmatrix *) new_complexmatrix(cm->rows, 1);
	if (res == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	for (i = 0; i < cm->rows; i++) {
	    double sum_re = 0, sum_im = 0;
	    int inf;
	    for (j = 0; j < cm->columns; j++) {
		sum_re += cm->array->data[2 * (i * cm->columns + j)];
		sum_im += cm->array->data[2 * (i * cm->columns + j) + 1];
	    }
	    if ((inf = isinf(sum_re)) != 0) {
		if (flags.f.range_error_ignore)
		    sum_re = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
		else {
		    free_vartype((vartype *) res);
		    return ERR_OUT_OF_RANGE;
		}
	    }
	    if ((inf = isinf(sum_im)) != 0) {
		if (flags.f.range_error_ignore)
		    sum_im = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
		else {
		    free_vartype((vartype *) res);
		    return ERR_OUT_OF_RANGE;
		}
	    }
	    res->array->data[2 * i] = sum_re;
	    res->array->data[2 * i + 1] = sum_im;
	}
	unary_result((vartype *) res);
	return ERR_NONE;
    } else if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else
	return ERR_INVALID_TYPE;
}

int docmd_swap_r(arg_struct *arg) {
    vartype *m;
    double xx, yy;
    int4 x, y, i;

    switch (matedit_mode) {
	case 0:
	    return ERR_NONEXISTENT;
	case 1:
	case 3:
	    m = recall_var(matedit_name, matedit_length);
	    break;
	case 2:
	    m = matedit_x;
	    break;
	default:
	    return ERR_INTERNAL_ERROR;
    }

    if (m == NULL)
	return ERR_NONEXISTENT;

    if (m->type != TYPE_REALMATRIX && m->type != TYPE_COMPLEXMATRIX)
	/* Should not happen, but could, as long as I don't implement
	 * matrix locking. */
	return ERR_INVALID_TYPE;
    
    if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_x->type != TYPE_REAL)
	return ERR_INVALID_TYPE;
    if (reg_y->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_y->type != TYPE_REAL)
	return ERR_INVALID_TYPE;

    xx = ((vartype_real *) reg_x)->x;
    if (xx < 0)
	xx = -xx;
    if (xx < 1 || xx >= 2147483648.0)
	return ERR_DIMENSION_ERROR;
    if (core_settings.ip_hack)
	xx += 5e-9;
    x = ((int4) xx) - 1;

    yy = ((vartype_real *) reg_y)->x;
    if (yy < 0)
	yy = -yy;
    if (yy < 1 || yy >= 2147483648.0)
	return ERR_DIMENSION_ERROR;
    if (core_settings.ip_hack)
	yy += 5e-9;
    y = ((int4) yy) - 1;

    if (m->type == TYPE_REALMATRIX) {
	vartype_realmatrix *rm = (vartype_realmatrix *) m;
	if (x > rm->rows || y > rm->rows)
	    return ERR_DIMENSION_ERROR;
	else if (x == y)
	    return ERR_NONE;
	if (!disentangle(m))
	    return ERR_INSUFFICIENT_MEMORY;
	for (i = 0; i < rm->columns; i++) {
	    int4 n1 = x * rm->columns + i;
	    int4 n2 = y * rm->columns + i;
	    char tempc = rm->array->is_string[n1];
	    double_or_string tempds = rm->array->data[n1];
	    rm->array->is_string[n1] = rm->array->is_string[n2];
	    rm->array->data[n1] = rm->array->data[n2];
	    rm->array->is_string[n2] = tempc;
	    rm->array->data[n2] = tempds;
	}
	return ERR_NONE;
    } else /* m->type == TYPE_COMPLEXMATRIX */ {
	vartype_complexmatrix *cm = (vartype_complexmatrix *) m;
	if (x > cm->rows || y > cm->rows)
	    return ERR_DIMENSION_ERROR;
	else if (x == y)
	    return ERR_NONE;
	if (!disentangle(m))
	    return ERR_INSUFFICIENT_MEMORY;
	for (i = 0; i < 2 * cm->columns; i++) {
	    int4 n1 = x * 2 * cm->columns + i;
	    int4 n2 = y * 2 * cm->columns + i;
	    double tempd = cm->array->data[n1];
	    cm->array->data[n1] = cm->array->data[n2];
	    cm->array->data[n2] = tempd;
	}
	return ERR_NONE;
    }
}

static int mappable_sinh_r(double x, double *y) COMMANDS4_SECT;
static int mappable_sinh_r(double x, double *y) {
    int inf;
    *y = sinh(x);
    if ((inf = isinf(*y)) != 0) {
	if (flags.f.range_error_ignore)
	    *y = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    return ERR_NONE;
}   

static int mappable_sinh_c(double xre, double xim,
	                             double *yre, double *yim) COMMANDS4_SECT;
static int mappable_sinh_c(double xre, double xim, double *yre, double *yim) {
    double sinhxre, coshxre;
    double sinxim, cosxim;
    int inf;
    sinhxre = sinh(xre);
    coshxre = cosh(xre);
    sincos(xim, &sinxim, &cosxim);
    *yre = sinhxre * cosxim;
    if ((inf = isinf(*yre)) != 0) {
	if (flags.f.range_error_ignore)
	    *yre = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *yim = coshxre * sinxim;
    if ((inf = isinf(*yim)) != 0) {
	if (flags.f.range_error_ignore)
	    *yim = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    return ERR_NONE;
}

int docmd_sinh(arg_struct *arg) {
    if (reg_x->type != TYPE_STRING) {
	vartype *v;
	int err = map_unary(reg_x, &v, mappable_sinh_r, mappable_sinh_c);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    } else
	return ERR_ALPHA_DATA_IS_INVALID;
}

int docmd_solve(arg_struct *arg) {
    int err;
    vartype *v;
    double x1, x2;
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

    if (reg_x->type == TYPE_REAL)
	x2 = ((vartype_real *) reg_x)->x;
    else if (reg_x->type == TYPE_STRING)
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
    double x1, x2;
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

int docmd_stoel(arg_struct *arg) {
    vartype *m;
    switch (matedit_mode) {
	case 0:
	    return ERR_NONEXISTENT;
	case 1:
	case 3:
	    m = recall_var(matedit_name, matedit_length);
	    break;
	case 2:
	    m = matedit_x;
	    break;
	default:
	    return ERR_INTERNAL_ERROR;
    }

    if (m == NULL)
	return ERR_NONEXISTENT;

    if (m->type != TYPE_REALMATRIX && m->type != TYPE_COMPLEXMATRIX)
	/* Should not happen, but could, as long as I don't implement
	 * matrix locking.
	 */
	return ERR_INVALID_TYPE;

    if (!disentangle(m))
	return ERR_INSUFFICIENT_MEMORY;

    if (m->type == TYPE_REALMATRIX) {
	vartype_realmatrix *rm = (vartype_realmatrix *) m;
	int4 n = matedit_i * rm->columns + matedit_j;
	if (reg_x->type == TYPE_REAL) {
	    rm->array->is_string[n] = 0;
	    rm->array->data[n].d = ((vartype_real *) reg_x)->x;
	    return ERR_NONE;
	} else if (reg_x->type == TYPE_STRING) {
	    vartype_string *s = (vartype_string *) reg_x;
	    int i;
	    rm->array->is_string[n] = 1;
	    rm->array->data[n].s.length = s->length;
	    for (i = 0; i < s->length; i++)
		rm->array->data[n].s.text[i] = s->text[i];
	    return ERR_NONE;
	} else
	    return ERR_INVALID_TYPE;
    } else /* m->type == TYPE_COMPLEXMATRIX */ {
	vartype_complexmatrix *cm = (vartype_complexmatrix *) m;
	int4 n = matedit_i * cm->columns + matedit_j;
	if (reg_x->type == TYPE_REAL) {
	    cm->array->data[2 * n] = ((vartype_real *) reg_x)->x;
	    cm->array->data[2 * n + 1] = 0;
	    return ERR_NONE;
	} else if (reg_x->type == TYPE_COMPLEX) {
	    vartype_complex *c = (vartype_complex *) reg_x;
	    cm->array->data[2 * n] = c->re;
	    cm->array->data[2 * n + 1] = c->im;
	    return ERR_NONE;
	} else
	    return ERR_INVALID_TYPE;
    }
}

int docmd_stoij(arg_struct *arg) {
    vartype *m;
    double x, y;
    int4 i, j;

    switch (matedit_mode) {
	case 0:
	    return ERR_NONEXISTENT;
	case 1:
	case 3:
	    m = recall_var(matedit_name, matedit_length);
	    break;
	case 2:
	    m = matedit_x;
	    break;
	default:
	    return ERR_INTERNAL_ERROR;
    }

    if (m == NULL)
	return ERR_NONEXISTENT;

    if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_x->type != TYPE_REAL)
	return ERR_INVALID_TYPE;
    if (reg_y->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_y->type != TYPE_REAL)
	return ERR_INVALID_TYPE;

    x = ((vartype_real *) reg_x)->x;
    if (x < 0)
	x = -x;
    if (core_settings.ip_hack)
	x += 5e-9;
    j = (int4) x;
    y = ((vartype_real *) reg_y)->x;
    if (y < 0)
	y = -y;
    if (core_settings.ip_hack)
	y += 5e-9;
    i = (int4) y;

    if (m->type == TYPE_REALMATRIX) {
	vartype_realmatrix *rm = (vartype_realmatrix *) m;
	if (i == 0 || i > rm->rows || j == 0 || j > rm->columns)
	    return ERR_DIMENSION_ERROR;
    } else if (m->type == TYPE_COMPLEXMATRIX) {
	vartype_complexmatrix *cm = (vartype_complexmatrix *) m;
	if (i == 0 || i > cm->rows || j == 0 || j > cm->columns)
	    return ERR_DIMENSION_ERROR;
    } else
	/* Should not happen, but could, as long as I don't implement
	 * matrix locking. */
	return ERR_INVALID_TYPE;

    matedit_i = i - 1;
    matedit_j = j - 1;
    return ERR_NONE;
}

static int mappable_tanh_r(double x, double *y) COMMANDS4_SECT;
static int mappable_tanh_r(double x, double *y) {
    *y = tanh(x);
    return ERR_NONE;
}   

static int mappable_tanh_c(double xre, double xim,
	                             double *yre, double *yim) COMMANDS4_SECT;
static int mappable_tanh_c(double xre, double xim, double *yre, double *yim) {
    double sinhxre, coshxre;
    double sinxim, cosxim;
    double re_sinh, re_cosh, im_sinh, im_cosh, abs_cosh;
    int inf;

    sinhxre = sinh(xre);
    coshxre = cosh(xre);
    sincos(xim, &sinxim, &cosxim);

    re_sinh = sinhxre * cosxim;
    im_sinh = coshxre * sinxim;
    re_cosh = coshxre * cosxim;
    im_cosh = sinhxre * sinxim;
    abs_cosh = hypot(re_cosh, im_cosh);

    if (abs_cosh == 0) {
	if (flags.f.range_error_ignore) {
	    *yre = re_sinh * im_sinh + re_cosh * im_cosh > 0 ? POS_HUGE_DOUBLE
							     : NEG_HUGE_DOUBLE;
	    *yim = im_sinh * re_cosh - re_sinh * im_cosh > 0 ? POS_HUGE_DOUBLE
							     : NEG_HUGE_DOUBLE;
	} else
	    return ERR_OUT_OF_RANGE;
    }

    *yre = (re_sinh * re_cosh + im_sinh * im_cosh) / abs_cosh / abs_cosh;
    if ((inf = isinf(*yre)) != 0) {
	if (flags.f.range_error_ignore)
	    *yre = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    *yim = (im_sinh * re_cosh - re_sinh * im_cosh) / abs_cosh / abs_cosh;
    if ((inf = isinf(*yim)) != 0) {
	if (flags.f.range_error_ignore)
	    *yim = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    return ERR_NONE;
}

int docmd_tanh(arg_struct *arg) {
    if (reg_x->type != TYPE_STRING) {
	vartype *v;
	int err = map_unary(reg_x, &v, mappable_tanh_r, mappable_tanh_c);
	if (err == ERR_NONE)
	    unary_result(v);
	return err;
    } else
	return ERR_ALPHA_DATA_IS_INVALID;
}

int docmd_trans(arg_struct *arg) {
    if (reg_x->type == TYPE_REALMATRIX) {
	vartype_realmatrix *src = (vartype_realmatrix *) reg_x;
	vartype_realmatrix *dst;
	int4 rows = src->rows;
	int4 columns = src->columns;
	int4 i, j;
	dst = (vartype_realmatrix *) new_realmatrix(columns, rows);
	if (dst == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	for (i = 0; i < rows; i++)
	    for (j = 0; j < columns; j++) {
		int4 n1 = i * columns + j;
		int4 n2 = j * rows + i;
		dst->array->is_string[n2] = src->array->is_string[n1];
		dst->array->data[n2] = src->array->data[n1];
	    }
	unary_result((vartype *) dst);
	return ERR_NONE;
    } else if (reg_x->type == TYPE_COMPLEXMATRIX) {
	vartype_complexmatrix *src = (vartype_complexmatrix *) reg_x;
	vartype_complexmatrix *dst;
	int4 rows = src->rows;
	int4 columns = src->columns;
	int4 i, j;
	dst = (vartype_complexmatrix *) new_complexmatrix(columns, rows);
	if (dst == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	for (i = 0; i < rows; i++)
	    for (j = 0; j < columns; j++) {
		int4 n1 = 2 * (i * columns + j);
		int4 n2 = 2 * (j * rows + i);
		dst->array->data[n2] = src->array->data[n1];
		dst->array->data[n2 + 1] = src->array->data[n1 + 1];
	    }
	unary_result((vartype *) dst);
	return ERR_NONE;
    } else if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else
	return ERR_INVALID_TYPE;
}

int docmd_wrap(arg_struct *arg) {
    flags.f.grow = 0;
    return ERR_NONE;
}

int docmd_x_swap(arg_struct *arg) {
    vartype *v;
    int err = generic_rcl(arg, &v);
    if (err != ERR_NONE)
	return err;
    err = generic_sto(arg, 0);
    if (err != ERR_NONE)
	free_vartype(v);
    else {
	free_vartype(reg_x);
	reg_x = v;
	if (flags.f.trace_print && flags.f.printer_exists)
	    docmd_prx(NULL);
    }
    return err;
}

int docmd_xor(arg_struct *arg) {
    int8 x, y;
    int err; 
    vartype *v;
    if ((err = get_base_param(reg_x, &x)) != ERR_NONE) 
	return err;
    if ((err = get_base_param(reg_y, &y)) != ERR_NONE)
	return err;
    v = new_real((double) (x ^ y));
    if (v == NULL) 
	return ERR_INSUFFICIENT_MEMORY;
    binary_result(v);
    return ERR_NONE;
}

int docmd_to_dec(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL) {
	double x = ((vartype_real *) reg_x)->x;
	int neg = x < 0;
	int8 oct, mul = 1, dec = 0;
	vartype *v;
	if (neg)
	    x = -x;
	oct = (int8) x;
	if (oct != x || oct > 777777777777LL)
	    return ERR_INVALID_DATA;
	while (oct != 0) {
	    int digit = (int) (oct % 10);
	    oct /= 10;
	    if (digit > 7)
		return ERR_INVALID_DATA;
	    dec += digit * mul;
	    mul <<= 3;
	}
	if (neg)
	    dec = -dec;
	v = new_real((double) dec);
	if (v == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	unary_result(v);
	return ERR_NONE;
    } else if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else
	return ERR_INVALID_TYPE;
}

int docmd_to_oct(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL) {
	double x = ((vartype_real *) reg_x)->x;
	int neg = x < 0;
	int8 dec, mul = 1, oct = 0;
	vartype *v;
	if (neg)
	    x = -x;
	dec = (int8) x;
	if (dec != x || dec > 68719476735LL)
	    return ERR_INVALID_DATA;
	while (dec != 0) {
	    int digit = (int) (dec & 7);
	    dec >>= 3;
	    oct += digit * mul;
	    mul *= 10;
	}
	if (neg)
	    oct = -oct;
	v = new_real((double) oct);
	if (v == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	unary_result(v);
	return ERR_NONE;
    } else if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    else
	return ERR_INVALID_TYPE;
}

#define DIR_LEFT  0
#define DIR_RIGHT 1
#define DIR_UP    2
#define DIR_DOWN  3

static int matedit_move(int direction) COMMANDS4_SECT;
static int matedit_move(int direction) {
    vartype *m, *v;
    vartype_realmatrix *rm;
    vartype_complexmatrix *cm;
    int4 rows, columns, new_i, new_j, old_n, new_n;
    int edge_flag = 0;
    int end_flag = 0;

    switch (matedit_mode) {
	case 0:
	    return ERR_NONEXISTENT;
	case 1:
	case 3:
	    m = recall_var(matedit_name, matedit_length);
	    break;
	case 2:
	    m = matedit_x;
	    break;
	default:
	    return ERR_INTERNAL_ERROR;
    }

    if (m == NULL)
	return ERR_NONEXISTENT;

    if (m->type == TYPE_REALMATRIX) {
	rm = (vartype_realmatrix *) m;
	rows = rm->rows;
	columns = rm->columns;
    } else if (m->type == TYPE_COMPLEXMATRIX) {
	cm = (vartype_complexmatrix *) m;
	rows = cm->rows;
	columns = cm->columns;
    } else
	return ERR_INVALID_TYPE;

    if (!disentangle(m))
	return ERR_INSUFFICIENT_MEMORY;

    new_i = matedit_i;
    new_j = matedit_j;
    switch (direction) {
	case DIR_LEFT:
	    if (--new_j < 0) {
		edge_flag = 1;
		new_j = columns - 1;
		if (--new_i < 0) {
		    end_flag = 1;
		    new_i = rows - 1;
		}
	    }
	    break;
	case DIR_RIGHT:
	    if (++new_j >= columns) {
		edge_flag = 1;
		new_j = 0;
		if (++new_i >= rows) {
		    end_flag = 1;
		    if (flags.f.grow) {
			int err;
			if (matedit_mode == 2)
			    err = dimension_array_ref(matedit_x,
						      rows + 1, columns);
			else
			    err = dimension_array(matedit_name, matedit_length,
						  rows + 1, columns);
			if (err != ERR_NONE)
			    return err;
			new_i = rows++;
		    } else
			new_i = 0;
		}
	    }
	    break;
	case DIR_UP:
	    if (--new_i < 0) {
		edge_flag = 1;
		new_i = rows - 1;
		if (--new_j < 0) {
		    end_flag = 1;
		    new_j = columns - 1;
		}
	    }
	    break;
	case DIR_DOWN:
	    if (++new_i >= rows) {
		edge_flag = 1;
		new_i = 0;
		if (++new_j >= columns) {
		    end_flag = 1;
		    new_j = 0;
		}
	    }
	    break;
    }

    old_n = matedit_i * columns + matedit_j;
    new_n = new_i * columns + new_j;

    if (m->type == TYPE_REALMATRIX) {
	if (old_n != new_n) {
	    if (rm->array->is_string[new_n])
		v = new_string(rm->array->data[new_n].s.text,
			    rm->array->data[new_n].s.length);
	    else
		v = new_real(rm->array->data[new_n].d);
	    if (v == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	}
	if (reg_x->type == TYPE_REAL) {
	    rm->array->is_string[old_n] = 0;
	    rm->array->data[old_n].d = ((vartype_real *) reg_x)->x;
	} else if (reg_x->type == TYPE_STRING) {
	    vartype_string *s = (vartype_string *) reg_x;
	    int i;
	    rm->array->is_string[old_n] = 1;
	    rm->array->data[old_n].s.length = s->length;
	    for (i = 0; i < s->length; i++)
		rm->array->data[old_n].s.text[i] = s->text[i];
	} else {
	    free_vartype(v);
	    return ERR_INVALID_TYPE;
	}
    } else /* m->type == TYPE_COMPLEXMATRIX */ {
	if (old_n != new_n) {
	    v = new_complex(cm->array->data[2 * new_n],
			    cm->array->data[2 * new_n + 1]);
	    if (v == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	}
	if (reg_x->type == TYPE_REAL) {
	    cm->array->data[2 * old_n] = ((vartype_real *) reg_x)->x;
	    cm->array->data[2 * old_n + 1] = 0;
	} else if (reg_x->type == TYPE_COMPLEX) {
	    vartype_complex *c = (vartype_complex *) reg_x;
	    cm->array->data[2 * old_n] = c->re;
	    cm->array->data[2 * old_n + 1] = c->im;
	} else {
	    free_vartype(v);
	    return ERR_INVALID_TYPE;
	}
    }

    matedit_i = new_i;
    matedit_j = new_j;
    flags.f.matrix_edge_wrap = edge_flag;
    flags.f.matrix_end_wrap = end_flag;
    if (old_n != new_n) {
	free_vartype(reg_x);
	reg_x = v;
    }
    mode_disable_stack_lift = 1;
    if (flags.f.trace_print && flags.f.printer_enable)
	docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_left(arg_struct *arg) {
    return matedit_move(DIR_LEFT);
}

int docmd_up(arg_struct *arg) {
    return matedit_move(DIR_UP);
}

int docmd_down(arg_struct *arg) {
    return matedit_move(DIR_DOWN);
}

int docmd_right(arg_struct *arg) {
    return matedit_move(DIR_RIGHT);
}

int docmd_percent_ch(arg_struct *arg) {
    double x, y, r;
    int inf;
    vartype *v;
    if (reg_x->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_x->type != TYPE_REAL)
	return ERR_INVALID_TYPE;
    if (reg_y->type == TYPE_STRING)
	return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_y->type != TYPE_REAL)
	return ERR_INVALID_TYPE;
    x = ((vartype_real *) reg_x)->x;
    y = ((vartype_real *) reg_y)->x;
    if (y == 0)
	return ERR_DIVIDE_BY_0;
    r = (x - y) / y * 100;
    if ((inf = isinf(r)) != 0) {
	if (flags.f.range_error_ignore)
	    r = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
	else
	    return ERR_OUT_OF_RANGE;
    }
    v = new_real(r);
    if (v == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    /* Binary function, but unary result, like % */
    unary_result(v);
    return ERR_NONE;
}

static vartype *matx_v;

static void matx_completion(int error, vartype *res) COMMANDS4_SECT;
static void matx_completion(int error, vartype *res) {
    if (error != ERR_NONE) {
	free_vartype(matx_v);
	return;
    }
    store_var("MATX", 4, res);
    matedit_prev_appmenu = MENU_MATRIX_SIMQ;
    set_menu(MENULEVEL_APP, MENU_MATRIX_EDIT1);
    /* NOTE: no need to use set_menu_return_err() here, since the MAT[ABX]
     * commands can only be invoked from the SIMQ menu; the SIMQ menu
     * has no exit callback, so leaving it never fails.
     */
    set_appmenu_exitcallback(1);
    if (res->type == TYPE_REALMATRIX) {
	vartype_realmatrix *m = (vartype_realmatrix *) res;
	vartype_real *v = (vartype_real *) matx_v;
	v->x = m->array->data[0].d;
    } else {
	vartype_complexmatrix *m = (vartype_complexmatrix *) res;
	vartype_complex *v = (vartype_complex *) matx_v;
	v->re = m->array->data[0];
	v->im = m->array->data[1];
    }
    free_vartype(reg_x);
    reg_x = matx_v;
    matedit_mode = 3;
    matedit_length = 4;
    matedit_name[0] = 'M';
    matedit_name[1] = 'A';
    matedit_name[2] = 'T';
    matedit_name[3] = 'X';
    matedit_i = 0;
    matedit_j = 0;
}

static int matabx(int which) COMMANDS4_SECT;
static int matabx(int which) {
    vartype *mat, *v;

    switch (which) {
	case 0:
	    mat = recall_var("MATA", 4);
	    break;

	case 1:
	    mat = recall_var("MATB", 4);
	    break;

	case 2: {
	    vartype *mata, *matb;

	    mata = recall_var("MATA", 4);
	    if (mata == NULL)
		return ERR_NONEXISTENT;
	    if (mata->type != TYPE_REALMATRIX
		    && mata->type != TYPE_COMPLEXMATRIX)
		return ERR_INVALID_TYPE;

	    matb = recall_var("MATB", 4);
	    if (matb == NULL)
		return ERR_NONEXISTENT;
	    if (matb->type != TYPE_REALMATRIX
		    && matb->type != TYPE_COMPLEXMATRIX)
		return ERR_INVALID_TYPE;

	    if (mata->type == TYPE_REALMATRIX && matb->type == TYPE_REALMATRIX)
		matx_v = new_real(0);
	    else
		matx_v = new_complex(0, 0);
	    if (matx_v == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	    return linalg_div(matb, mata, matx_completion);
	}
    }

    if (mat->type == TYPE_REALMATRIX) {
	vartype_realmatrix *rm = (vartype_realmatrix *) mat;
	if (rm->array->is_string[0])
	    v = new_string(rm->array->data[0].s.text,
			    rm->array->data[0].s.length);
	else
	    v = new_real(rm->array->data[0].d);
    } else {
	vartype_complexmatrix *cm = (vartype_complexmatrix *) mat;
	v = new_complex(cm->array->data[0], cm->array->data[1]);
    }
    if (v == NULL)
	return ERR_INSUFFICIENT_MEMORY;

    matedit_prev_appmenu = MENU_MATRIX_SIMQ;
    set_menu(MENULEVEL_APP, MENU_MATRIX_EDIT1);
    /* NOTE: no need to use set_menu_return_err() here, since the MAT[ABX]
     * commands can only be invoked from the SIMQ menu; the SIMQ menu
     * has no exit callback, so leaving it never fails.
     */
    set_appmenu_exitcallback(1);
    free_vartype(reg_x);
    reg_x = v;
    matedit_mode = 3;
    matedit_length = 4;
    matedit_name[0] = 'M';
    matedit_name[1] = 'A';
    matedit_name[2] = 'T';
    matedit_name[3] = which == 0 ? 'A' : 'B';
    matedit_i = 0;
    matedit_j = 0;
    return ERR_NONE;
}

int docmd_mata(arg_struct *arg) {
    return matabx(0);
}

int docmd_matb(arg_struct *arg) {
    return matabx(1);
}

int docmd_matx(arg_struct *arg) {
    return matabx(2);
}

int docmd_simq(arg_struct *arg) {
    vartype *m, *mata, *matb, *matx;
    int4 dim;
    int err;

    if (arg->type != ARGTYPE_NUM)
	return ERR_INVALID_TYPE;
    dim = arg->val.num;
    if (dim <= 0)
	return ERR_DIMENSION_ERROR;

    m = recall_var("MATA", 4);
    if (m == NULL) {
	mata = new_realmatrix(dim, dim);
	if (mata == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
    } else {
	mata = dup_vartype(m);
	if (mata == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	err = dimension_array_ref(mata, dim, dim);
	if (err != ERR_NONE)
	    return err;
    }

    m = recall_var("MATB", 4);
    if (m == NULL) {
	matb = new_realmatrix(dim, 1);
	if (matb == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
    } else {
	matb = dup_vartype(m);
	if (matb == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	err = dimension_array_ref(matb, dim, 1);
	if (err != ERR_NONE)
	    return err;
    }

    m = recall_var("MATX", 4);
    if (m == NULL) {
	matx = new_realmatrix(dim, 1);
	if (matx == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
    } else {
	matx = dup_vartype(m);
	if (matx == NULL)
	    return ERR_INSUFFICIENT_MEMORY;
	err = dimension_array_ref(matx, dim, 1);
	if (err != ERR_NONE)
	    return err;
    }

    err = set_menu_return_err(MENULEVEL_APP, MENU_MATRIX_SIMQ);
    if (err != ERR_NONE) {
	/* Didn't work; we're stuck in the matrix editor
	 * waiting for the user to put something valid into X.
	 * (Then again, how can anyone issue the SIMQ command if
	 * they're in the matrix editor? SIMQ has the 'hidden'
	 * command property. Oh, well, better safe than sorry...)
	 */
	free_vartype(mata);
	free_vartype(matb);
	free_vartype(matx);
	return err;
    }

    store_var("MATX", 4, matx);
    store_var("MATB", 4, matb);
    store_var("MATA", 4, mata);
    return ERR_NONE;
}

static int max_min_helper(int do_max) COMMANDS4_SECT;
static int max_min_helper(int do_max) {
    vartype *m;
    vartype_realmatrix *rm;
    double max_or_min_value = do_max ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
    int4 i, max_or_min_index = 0;
    vartype *new_x, *new_y;

    switch (matedit_mode) {
	case 0:
	    return ERR_NONEXISTENT;
	case 1:
	case 3:
	    m = recall_var(matedit_name, matedit_length);
	    break;
	case 2:
	    m = matedit_x;
	    break;
	default:
	    return ERR_INTERNAL_ERROR;
    }
    if (m == NULL)
	return ERR_NONEXISTENT;
    if (m->type != TYPE_REALMATRIX)
	return ERR_INVALID_TYPE;
    rm = (vartype_realmatrix *) m;

    for (i = matedit_i; i < rm->rows; i++) {
	int4 index = i * rm->columns + matedit_j;
	double e;
	if (rm->array->is_string[index])
	    return ERR_ALPHA_DATA_IS_INVALID;
	e = rm->array->data[index].d;
	if (do_max ? e >= max_or_min_value : e <= max_or_min_value) {
	    max_or_min_value = e;
	    max_or_min_index = i;
	}
    }
    new_x = new_real(max_or_min_value);
    if (new_x == NULL)
	return ERR_INSUFFICIENT_MEMORY;
    new_y = new_real(max_or_min_index + 1);
    if (new_y == NULL) {
	free_vartype(new_x);
	return ERR_INSUFFICIENT_MEMORY;
    }
    recall_two_results(new_x, new_y);
    return ERR_NONE;
}

int docmd_max(arg_struct *arg) {
    return max_min_helper(1);
}

int docmd_min(arg_struct *arg) {
    return max_min_helper(0);
}

int docmd_find(arg_struct *arg) {
    vartype *m;
    if (reg_x->type == TYPE_REALMATRIX || reg_x->type == TYPE_COMPLEXMATRIX)
	return ERR_INVALID_TYPE;
    switch (matedit_mode) {
	case 0:
	    return ERR_NONEXISTENT;
	case 1:
	case 3:
	    m = recall_var(matedit_name, matedit_length);
	    break;
	case 2:
	    m = matedit_x;
	    break;
	default:
	    return ERR_INTERNAL_ERROR;
    }
    if (m == NULL)
	return ERR_NONEXISTENT;
    if (m->type == TYPE_REALMATRIX) {
	vartype_realmatrix *rm;
	int4 i, j, p = 0;
	if (reg_x->type == TYPE_COMPLEX)
	    return ERR_NO;
	rm = (vartype_realmatrix *) m;
	if (reg_x->type == TYPE_REAL) {
	    double d = ((vartype_real *) reg_x)->x;
	    for (i = 0; i < rm->rows; i++)
		for (j = 0; j < rm->columns; j++)
		    if (!rm->array->is_string[p] && rm->array->data[p].d == d) {
			matedit_i = i;
			matedit_j = j;
			return ERR_YES;
		    } else
			p++;
	} else /* reg_x->type == TYPE_STRING */ {
	    vartype_string *s = (vartype_string *) reg_x;
	    for (i = 0; i < rm->rows; i++)
		for (j = 0; j < rm->columns; j++)
		    if (rm->array->is_string[p]
			    && string_equals(s->text, s->length, 
					     rm->array->data[p].s.text,
					     rm->array->data[p].s.length)) {
			matedit_i = i;
			matedit_j = j;
			return ERR_YES;
		    } else
			p++;
	}
    } else /* m->type == TYPE_COMPLEXMATRIX */ {
	vartype_complexmatrix *cm;
	int4 i, j, p = 0;
	double re, im;
	if (reg_x->type != TYPE_COMPLEX)
	    return ERR_NO;
	cm = (vartype_complexmatrix *) m;
	re = ((vartype_complex *) reg_x)->re;
	im = ((vartype_complex *) reg_x)->im;
	for (i = 0; i < cm->rows; i++)
	    for (j = 0; j < cm->columns; j++)
		if (cm->array->data[p] == re && cm->array->data[p + 1] == im) {
		    matedit_i = i;
		    matedit_j = j;
		    return ERR_YES;
		} else
		    p += 2;
    }
    return ERR_NO;
}

static void accum(double *sum, double term, int weight) COMMANDS4_SECT;
static void accum(double *sum, double term, int weight) {
    int inf;
    double s;
    if (weight == 1)
	s = *sum + term;
    else
	s = *sum - term;
    if ((inf = isinf(s)) != 0)
	s = inf < 0 ? NEG_HUGE_DOUBLE : POS_HUGE_DOUBLE;
    *sum = s;
}

static double sigma_helper_2(double_or_string *sigmaregs,
			     double x, double y, int weight) COMMANDS4_SECT;
static double sigma_helper_2(double_or_string *sigmaregs,
			     double x, double y, int weight) {

    accum(&sigmaregs[0].d, x, weight);
    accum(&sigmaregs[1].d, x * x, weight);
    accum(&sigmaregs[2].d, y, weight);
    accum(&sigmaregs[3].d, y * y, weight);
    accum(&sigmaregs[4].d, x * y, weight);
    accum(&sigmaregs[5].d, 1, weight);

    if (flags.f.all_sigma) {
	if (x > 0) {
	    double lnx = log(x);
	    if (y > 0) {
		double lny = log(y);
		accum(&sigmaregs[8].d, lny, weight);
		accum(&sigmaregs[9].d, lny * lny, weight);
		accum(&sigmaregs[10].d, lnx * lny, weight);
		accum(&sigmaregs[11].d, x * lny, weight);
	    } else {
		flags.f.exp_fit_invalid = 1;
		flags.f.pwr_fit_invalid = 1;
	    }
	    accum(&sigmaregs[6].d, lnx, weight);
	    accum(&sigmaregs[7].d, lnx * lnx, weight);
	    accum(&sigmaregs[12].d, lnx * y, weight);
	} else {
	    if (y > 0) {
		double lny = log(y);
		accum(&sigmaregs[8].d, lny, weight);
		accum(&sigmaregs[9].d, lny * lny, weight);
		accum(&sigmaregs[11].d, x * lny, weight);
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

    return sigmaregs[5].d;
}

static int sigma_helper_1(int weight) COMMANDS4_SECT;
static int sigma_helper_1(int weight) {
    /* Check if summation registers are OK */
    int4 first = mode_sigma_reg;
    int4 last = first + (flags.f.all_sigma ? 13 : 6);
    int4 size, i;
    vartype *regs = recall_var("REGS", 4);
    vartype_realmatrix *r;
    double_or_string *sigmaregs;
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

    /* All summation registers present, real-valued, non-string. */
    switch (reg_x->type) {
	case TYPE_REAL: {
	    if (reg_y->type == TYPE_REAL) {
		vartype_real *x = (vartype_real *) new_real(0);
		if (x == NULL)
		    return ERR_INSUFFICIENT_MEMORY;
		x->x = sigma_helper_2(sigmaregs,
				      ((vartype_real *) reg_x)->x,
				      ((vartype_real *) reg_y)->x,
				      weight);
		free_vartype(reg_lastx);
		reg_lastx = reg_x;
		reg_x = (vartype *) x;
		mode_disable_stack_lift = 1;
		return ERR_NONE;
	    } else if (reg_y->type == TYPE_STRING)
		return ERR_ALPHA_DATA_IS_INVALID;
	    else
		return ERR_INVALID_TYPE;
	}
	case TYPE_REALMATRIX: {
	    vartype_realmatrix *rm = (vartype_realmatrix *) reg_x;
	    vartype_real *x;
	    int4 i;
	    if (rm->columns != 2)
		return ERR_DIMENSION_ERROR;
	    for (i = 0; i < rm->rows * 2; i++)
		if (rm->array->is_string[i])
		    return ERR_ALPHA_DATA_IS_INVALID;
	    x = (vartype_real *) new_real(0);
	    if (x == NULL)
		return ERR_INSUFFICIENT_MEMORY;
	    for (i = 0; i < rm->rows; i++)
		x->x = sigma_helper_2(sigmaregs,
				      rm->array->data[i * 2].d,
				      rm->array->data[i * 2 + 1].d,
				      weight);
	    free_vartype(reg_lastx);
	    reg_lastx = reg_x;
	    reg_x = (vartype *) x;
	    mode_disable_stack_lift = 1;
	    return ERR_NONE;
	}
	case TYPE_STRING:
	    return ERR_ALPHA_DATA_IS_INVALID;
	default:
	    return ERR_INVALID_TYPE;
    }
}

int docmd_sigmaadd(arg_struct *arg) {
    int err = sigma_helper_1(1);
    if (err == ERR_NONE && flags.f.trace_print && flags.f.printer_exists)
	docmd_prx(NULL);
    return err;
}

int docmd_sigmasub(arg_struct *arg) {
    int err = sigma_helper_1(-1);
    if (err == ERR_NONE && flags.f.trace_print && flags.f.printer_exists)
	docmd_prx(NULL);
    return err;
}
