/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2019  Thomas Okken
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

#ifndef CORE_GLOBALS_H
#define CORE_GLOBALS_H 1


#include "free42.h"
#include "core_phloat.h"
#include "core_tables.h"


/**********/
/* Errors */
/**********/

#define ERR_NONE                    0
#define ERR_ALPHA_DATA_IS_INVALID   1
#define ERR_INSUFFICIENT_MEMORY     2
#define ERR_NOT_YET_IMPLEMENTED     3
#define ERR_OUT_OF_RANGE            4
#define ERR_DIVIDE_BY_0             5
#define ERR_INVALID_TYPE            6
#define ERR_INVALID_DATA            7
#define ERR_DIMENSION_ERROR         8
#define ERR_SIZE_ERROR              9
#define ERR_INTERNAL_ERROR         10
#define ERR_NONEXISTENT            11
#define ERR_RESTRICTED_OPERATION   12
#define ERR_YES                    13
#define ERR_NO                     14
#define ERR_STOP                   15
#define ERR_LABEL_NOT_FOUND        16
#define ERR_NO_REAL_VARIABLES      17
#define ERR_NO_COMPLEX_VARIABLES   18
#define ERR_NO_MATRIX_VARIABLES    19
#define ERR_NO_MENU_VARIABLES      20
#define ERR_STAT_MATH_ERROR        21
#define ERR_INVALID_FORECAST_MODEL 22
#define ERR_SOLVE_INTEG_RTN_LOST   23
#define ERR_SINGULAR_MATRIX        24
#define ERR_SOLVE_SOLVE            25
#define ERR_INTEG_INTEG            26
#define ERR_RUN                    27
#define ERR_INTERRUPTED            28
#define ERR_PRINTING_IS_DISABLED   29
#define ERR_INTERRUPTIBLE          30
#define ERR_NO_VARIABLES           31
#define ERR_SUSPICIOUS_OFF         32

typedef struct {
    const char *text;
    int length;
} error_spec;

extern error_spec errors[];


/*************/
/* Key codes */
/*************/

#define KEY_SIGMA  1
#define KEY_INV    2
#define KEY_SQRT   3
#define KEY_LOG    4
#define KEY_LN     5
#define KEY_XEQ    6
#define KEY_STO    7
#define KEY_RCL    8
#define KEY_RDN    9
#define KEY_SIN   10
#define KEY_COS   11
#define KEY_TAN   12
#define KEY_ENTER 13
#define KEY_SWAP  14
#define KEY_CHS   15
#define KEY_E     16
#define KEY_BSP   17
#define KEY_UP    18
#define KEY_7     19
#define KEY_8     20
#define KEY_9     21
#define KEY_DIV   22
#define KEY_DOWN  23
#define KEY_4     24
#define KEY_5     25
#define KEY_6     26
#define KEY_MUL   27
#define KEY_SHIFT 28
#define KEY_1     29
#define KEY_2     30
#define KEY_3     31
#define KEY_SUB   32
#define KEY_EXIT  33
#define KEY_0     34
#define KEY_DOT   35
#define KEY_RUN   36
#define KEY_ADD   37


/*********/
/* Menus */
/*********/

#define MENU_NONE          -1
#define MENU_SHORTCUT      -2
#define MENU_ALPHA1         0
#define MENU_ALPHA2         1
#define MENU_ALPHA_ABCDE1   2
#define MENU_ALPHA_ABCDE2   3
#define MENU_ALPHA_FGHI     4
#define MENU_ALPHA_JKLM     5
#define MENU_ALPHA_NOPQ1    6
#define MENU_ALPHA_NOPQ2    7
#define MENU_ALPHA_RSTUV1   8
#define MENU_ALPHA_RSTUV2   9
#define MENU_ALPHA_WXYZ    10
#define MENU_ALPHA_PAREN   11
#define MENU_ALPHA_ARROW   12
#define MENU_ALPHA_COMP    13
#define MENU_ALPHA_MATH    14
#define MENU_ALPHA_PUNC1   15
#define MENU_ALPHA_PUNC2   16
#define MENU_ALPHA_MISC1   17
#define MENU_ALPHA_MISC2   18
#define MENU_ST            19
#define MENU_IND_ST        20
#define MENU_IND           21
#define MENU_MODES1        22
#define MENU_MODES2        23
#define MENU_DISP          24
#define MENU_CLEAR1        25
#define MENU_CLEAR2        26
#define MENU_CONVERT1      27
#define MENU_CONVERT2      28
#define MENU_FLAGS         29
#define MENU_PROB          30
#define MENU_CUSTOM1       31
#define MENU_CUSTOM2       32
#define MENU_CUSTOM3       33
#define MENU_PGM_FCN1      34
#define MENU_PGM_FCN2      35
#define MENU_PGM_FCN3      36
#define MENU_PGM_FCN4      37
#define MENU_PGM_XCOMP0    38
#define MENU_PGM_XCOMPY    39
#define MENU_PRINT1        40
#define MENU_PRINT2        41
#define MENU_PRINT3        42
#define MENU_TOP_FCN       43
#define MENU_CATALOG       44
#define MENU_BLANK         45
#define MENU_PROGRAMMABLE  46
#define MENU_VARMENU       47
#define MENU_STAT1         48
#define MENU_STAT2         49
#define MENU_STAT_CFIT     50
#define MENU_STAT_MODL     51
#define MENU_MATRIX1       52
#define MENU_MATRIX2       53
#define MENU_MATRIX3       54
#define MENU_MATRIX_SIMQ   55
#define MENU_MATRIX_EDIT1  56
#define MENU_MATRIX_EDIT2  57
#define MENU_BASE          58
#define MENU_BASE_A_THRU_F 59
#define MENU_BASE_LOGIC    60
#define MENU_SOLVE         61
#define MENU_INTEG         62
#define MENU_INTEG_PARAMS  63


typedef struct {
    int menuid;
    unsigned char title_length;
    char title[7];
} menu_item_spec;

typedef struct {
    int parent;
    int next;
    int prev;
    menu_item_spec child[6];
} menu_spec;

extern menu_spec menus[];


/***********************/
/* Variable data types */
/***********************/

#define TYPE_NULL 0
#define TYPE_REAL 1
#define TYPE_COMPLEX 2
#define TYPE_REALMATRIX 3
#define TYPE_COMPLEXMATRIX 4
#define TYPE_STRING 5

typedef struct {
    int type;
} vartype;


typedef struct {
    int type;
    phloat x;
} vartype_real;


typedef struct {
    int type;
    phloat re, im;
} vartype_complex;


typedef struct {
    int refcount;
    phloat *data;
    char *is_string;
} realmatrix_data;

typedef struct {
    int type;
    int4 rows;
    int4 columns;
    realmatrix_data *array;
} vartype_realmatrix;


typedef struct {
    int refcount;
    phloat *data;
} complexmatrix_data;

typedef struct {
    int type;
    int4 rows;
    int4 columns;
    complexmatrix_data *array;
} vartype_complexmatrix;


typedef struct {
    int type;
    int length;
    char text[6];
} vartype_string;

/******************/
/* Emulator state */
/******************/

/* Registers */
extern vartype *reg_x;
extern vartype *reg_y;
extern vartype *reg_z;
extern vartype *reg_t;
extern vartype *reg_lastx;
extern int reg_alpha_length;
extern char reg_alpha[44];

/* FLAGS
 * Note: flags whose names start with VIRTUAL_ are named here for reference
 * only; they are actually handled by virtual_flag_handler(). Setting or
 * clearing them in 'flags' has no effect.
 * Flags whose names are the letter 'f' followed by two digits have no
 * specified meaning according to the HP-42S manual; they are either user flags
 * or reserved.
 */
typedef union {
    char farray[100];
    struct {
        char f00; char f01; char f02; char f03; char f04;
        char f05; char f06; char f07; char f08; char f09;
        char f10;
        char auto_exec;
        char double_wide_print;
        char lowercase_print;
        char f14;
        char trace_print; /* 'normal_print' ignored if this is set */
        char normal_print;
        char f17; char f18; char f19; char f20;
        char printer_enable;
        char numeric_data_input;
        char alpha_data_input;
        char range_error_ignore;
        char error_ignore;
        char audio_enable;
        char VIRTUAL_custom_menu;
        char decimal_point;
        char thousands_separators;
        char stack_lift_disable;
        char dmy; /* Time Module DMY mode */
        char f32; char f33;
        char agraph_control1; /* 0 (default): dst = dst | src, 1: dst = src, */
        char agraph_control0; /* 2: dst = dst & ~src, 3: dst = dst ^ src */
        char digits_bit3;
        char digits_bit2;
        char digits_bit1;
        char digits_bit0;
        char fix_or_all;
        char eng_or_all;
        char grad;
        char rad;
        char continuous_on;
        char VIRTUAL_solving;
        char VIRTUAL_integrating;
        char VIRTUAL_variable_menu;
        char VIRTUAL_alpha_mode;
        char VIRTUAL_low_battery;
        char message;
        char two_line_message;
        char prgm_mode;
        char VIRTUAL_input;
        char f54;
        char printer_exists;
        char lin_fit;
        char log_fit;
        char exp_fit;
        char pwr_fit;
        char all_sigma;
        char log_fit_invalid;
        char exp_fit_invalid;
        char pwr_fit_invalid;
        char f64;
        char VIRTUAL_matrix_editor;
        char grow;
        char f67;
        char base_bit0; /* Note: dec=0, bin=1, oct=7, hex=15 */
        char base_bit1;
        char base_bit2;
        char base_bit3;
        char local_label;
        char polar;
        char real_result_only;
        char VIRTUAL_programmable_menu;
        char matrix_edge_wrap;
        char matrix_end_wrap;
        char f78; char f79; char f80; char f81; char f82;
        char f83; char f84; char f85; char f86; char f87;
        char f88; char f89; char f90; char f91; char f92;
        char f93; char f94; char f95; char f96; char f97;
        char f98; char f99;
    } f;
} flags_struct;
extern flags_struct flags;

/* Variables */
typedef struct {
    unsigned char length;
    char name[7];
    vartype *value;
} var_struct;
typedef struct {
    unsigned char length;
    char name[7];
    int4 value;
} var_struct_32bit;
extern int vars_capacity;
extern int vars_count;
extern var_struct *vars;

/* Programs */
typedef struct {
    int4 capacity;
    int4 size;
    int lclbl_invalid;
    unsigned char *text;
} prgm_struct;
typedef struct {
    int4 capacity;
    int4 size;
    int lclbl_invalid;
    int4 text;
} prgm_struct_32bit;
extern int prgms_capacity;
extern int prgms_count;
extern prgm_struct *prgms;
typedef struct {
    unsigned char length;
    char name[7];
    int prgm;
    int4 pc;
} label_struct;
extern int labels_capacity;
extern int labels_count;
extern label_struct *labels;

extern int current_prgm;
extern int4 pc;
extern int prgm_highlight_row;

extern int varmenu_length;
extern char varmenu[7];
extern int varmenu_rows;
extern int varmenu_row;
extern int varmenu_labellength[6];
extern char varmenu_labeltext[6][7];
extern int varmenu_role;


/****************/
/* More globals */
/****************/

extern bool mode_clall;
extern int (*mode_interruptible)(int);
extern bool mode_stoppable;
extern bool mode_command_entry;
extern bool mode_number_entry;
extern bool mode_alpha_entry;
extern bool mode_shift;
extern int mode_appmenu;
extern int mode_plainmenu;
extern bool mode_plainmenu_sticky;
extern int mode_transientmenu;
extern int mode_alphamenu;
extern int mode_commandmenu;
extern bool mode_running;
extern bool mode_getkey;
extern bool mode_pause;
extern bool mode_disable_stack_lift;
extern bool mode_varmenu;
extern bool mode_updown;
extern int4 mode_sigma_reg;
extern int mode_goose;
extern bool mode_time_clktd;
extern bool mode_time_clk24;

extern phloat entered_number;
extern int entered_string_length;
extern char entered_string[15];

extern int pending_command;
extern arg_struct pending_command_arg;
extern int xeq_invisible;

/* Multi-keystroke commands -- edit state */
/* Relevant when mode_command_entry != 0 */
extern int incomplete_command;
extern int incomplete_ind;
extern int incomplete_alpha;
extern int incomplete_length;
extern int incomplete_maxdigits;
extern int incomplete_argtype;
extern int incomplete_num;
extern char incomplete_str[7];
extern int4 incomplete_saved_pc;
extern int4 incomplete_saved_highlight_row;

#define CATSECT_TOP 0
#define CATSECT_FCN 1
#define CATSECT_PGM 2
#define CATSECT_REAL 3
#define CATSECT_CPX 4
#define CATSECT_MAT 5
#define CATSECT_PGM_ONLY 6
#define CATSECT_REAL_ONLY 7
#define CATSECT_MAT_ONLY 8
#define CATSECT_VARS_ONLY 9
#define CATSECT_PGM_SOLVE 10
#define CATSECT_PGM_INTEG 11

/* Command line handling temporaries */
extern char cmdline[100];
extern int cmdline_length;
extern int cmdline_row;

/* Matrix editor / matrix indexing */
extern int matedit_mode; /* 0=off, 1=index, 2=edit, 3=editn */
extern char matedit_name[7];
extern int matedit_length;
extern vartype *matedit_x;
extern int4 matedit_i;
extern int4 matedit_j;
extern int matedit_prev_appmenu;

/* INPUT */
extern char input_name[11];
extern int input_length;
extern arg_struct input_arg;

/* BASE application */
extern int baseapp;

/* Random number generator */
extern int8 random_number_low, random_number_high;

/* NORM & TRACE mode: number waiting to be printed */
extern int deferred_print;

/* Keystroke buffer - holds keystrokes received while
 * there is a program running.
 */
extern int keybuf_head;
extern int keybuf_tail;
extern int keybuf[16];

extern int remove_program_catalog;

#define NUMBER_FORMAT_BINARY 0
#define NUMBER_FORMAT_BCD20_OLD 1
#define NUMBER_FORMAT_BCD20_NEW 2
#define NUMBER_FORMAT_BID128 3
extern int state_file_number_format;

extern bool no_keystrokes_yet;


/*********************/
/* Utility functions */
/*********************/

void clear_all_prgms();
int clear_prgm(const arg_struct *arg);
int clear_prgm_by_index(int prgm_index);
void clear_prgm_lines(int4 count);
void goto_dot_dot();
int mvar_prgms_exist();
int label_has_mvar(int lblindex);
int get_command_length(int prgm, int4 pc);
void get_next_command(int4 *pc, int *command, arg_struct *arg, int find_target);
void rebuild_label_table();
void delete_command(int4 pc);
void store_command(int4 pc, int command, arg_struct *arg);
void store_command_after(int4 *pc, int command, arg_struct *arg);
int4 pc2line(int4 pc);
int4 line2pc(int4 line);
int4 find_local_label(const arg_struct *arg);
int find_global_label(const arg_struct *arg, int *prgm, int4 *pc);
int push_rtn_addr(int prgm, int4 pc);
void pop_rtn_addr(int *prgm, int4 *pc);
void clear_all_rtns();
bool solve_active();
bool integ_active();
void unwind_stack_until_solve();

bool load_state(int4 version);
void save_state();
void hard_reset(int bad_state_file);

bool read_arg(arg_struct *arg, bool old);
bool write_arg(const arg_struct *arg);
bool read_phloat(phloat *d);
bool write_phloat(phloat d);

#ifdef ANDROID
void reinitialize_globals();
#endif

#ifdef IPHONE
bool off_enabled();
#endif

#endif
