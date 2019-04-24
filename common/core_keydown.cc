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

#include <stdlib.h>

#include "core_keydown.h"
#include "core_commands2.h"
#include "core_commands3.h"
#include "core_display.h"
#include "core_helpers.h"
#include "core_main.h"
#include "core_math1.h"
#include "core_tables.h"
#include "core_variables.h"
#include "shell.h"


static int is_number_key(int shift, int key) {
    int *menu = get_front_menu();
    if (menu != NULL && *menu == MENU_BASE_A_THRU_F
            && (key == KEY_SIGMA || key == KEY_INV || key == KEY_SQRT
                || key == KEY_LOG || key == KEY_LN || key == KEY_XEQ))
        return 1;
    return !shift && (key == KEY_0 || key == KEY_1 || key == KEY_2
                || key == KEY_3 || key == KEY_4 || key == KEY_5
                || key == KEY_6 || key == KEY_7 || key == KEY_8
                || key == KEY_9 || key == KEY_DOT || key == KEY_E);
}

static int basekeys() {
    if (!baseapp)
        return 0;
    int *menu = get_front_menu();
    return menu != NULL && (*menu == MENU_BASE || *menu == MENU_BASE_A_THRU_F || *menu == MENU_BASE_LOGIC);
}

static void set_solve_integ(int solve) {
    if (flags.f.prgm_mode || !mvar_prgms_exist()) {
        set_menu(MENULEVEL_APP, solve ? MENU_SOLVE : MENU_INTEG);
        if (!flags.f.prgm_mode)
            display_error(ERR_NO_MENU_VARIABLES, 0);
    } else {
        int err = set_menu_return_err(MENULEVEL_APP, MENU_CATALOG, false);
        if (err == ERR_NONE) {
            set_cat_section(solve ? CATSECT_PGM_SOLVE : CATSECT_PGM_INTEG);
            move_cat_row(0);
            clear_row(0);
            if (solve)
                draw_string(0, 0, "Select Solve Program", 20);
            else
                draw_string(0, 0, "Select \003f(x) Program", 20);
            flags.f.message = 1;
            flags.f.two_line_message = 0;
        } else
            display_error(err, 1);
    }
    redisplay();
}

static void view(const char *varname, int varlength) {
    arg_struct arg;
    int i, err;
    arg.type = ARGTYPE_STR;
    arg.length = varlength;
    for (i = 0; i < varlength; i++)
        arg.val.text[i] = varname[i];
    err = view_helper(&arg, 0);
    if (err != ERR_NONE) {
        display_error(err, 1);
        flush_display();
        pending_command = CMD_NONE;
    } else {
        pending_command = CMD_LINGER1;
        shell_request_timeout3(2000);
    }
}

void keydown(int shift, int key) {
    int *menu;

    // Preserve state of Shift, to allow MENU handlers to implement
    // different behaviors for unshifted and shifted menu keys.
    flags.f.shift_state = shift;

    pending_command = CMD_NONE;

    if (key >= 1024 && key < 2048) {
        /* TODO: is this actually needed?
         * Filtering out ASCII key events if the alpha menu is not actually
         * active, just in case the subsequent key handling code doesn't
         * handle them properly in all cases. Using a code coverage tool on
         * the keydown handler might be an idea...
         */
        menu = get_front_menu();
        if (menu == NULL || *menu < MENU_ALPHA1 || *menu > MENU_ALPHA_MISC2)
            return;
    } else if (key < 1 || key > 37 && key < 2048) {
        /* Bad key code */
        squeak();
        return;
    }

    if (mode_clall) {
        if (!shift && key == KEY_SIGMA)
            pending_command = CMD_CLALLb;
        else if (key == KEY_EXIT)
            pending_command = CMD_CANCELLED;
        else
            pending_command = CMD_NULL;
        mode_clall = false;
        return;
    }

    if (mode_getkey) {
        vartype *result = new_real(shift ? key + 37 : key);
        if (result != NULL) {
            recall_result(result);
            flags.f.stack_lift_disable = 0;
        } else {
            display_error(ERR_INSUFFICIENT_MEMORY, 1);
            set_running(false);
        }
        if (key == KEY_EXIT || (!shift && key == KEY_RUN))
            set_running(false);
        mode_getkey = false;
        if (!mode_running)
            redisplay();
        return;
    }

    if (shift && key == KEY_EXIT) {
        pending_command = CMD_SILENT_OFF;
        return;
    }

    if (shift && key == KEY_RUN) {
        if (mode_command_entry) {
            squeak();
            return;
        }
        if (flags.f.prgm_mode) {
            if (mode_alpha_entry)
                finish_alpha_prgm_line();
            else if (mode_number_entry) {
                arg_struct arg;
                arg.type = ARGTYPE_DOUBLE;
                arg.val_d = entered_number;
                store_command(pc, CMD_NUMBER, &arg);
                prgm_highlight_row = 1;
            }
        } else if (mode_alpha_entry) {
            if ((flags.f.trace_print || flags.f.normal_print)
                    && flags.f.printer_exists)
                docmd_pra(NULL);
        } else if (mode_number_entry) {
            if ((flags.f.trace_print || flags.f.normal_print)
                    && flags.f.printer_exists) {
                deferred_print = 1;
                print_command(CMD_NULL, NULL);
            }
        }

        mode_alpha_entry = false;
        mode_number_entry = false;
        flags.f.prgm_mode = !flags.f.prgm_mode;

        mode_varmenu = false;
        if (flags.f.prgm_mode) {
            if (mode_appmenu == MENU_BASE_A_THRU_F)
                set_menu(MENULEVEL_APP, MENU_BASE);
            else if (mode_plainmenu == MENU_PROGRAMMABLE)
                set_menu(MENULEVEL_PLAIN, MENU_NONE);
            input_length = 0;
        }
        flags.f.message = 0;
        flags.f.two_line_message = 0;
        redisplay();
        return;
    }

    if (flags.f.message && !shift && key == KEY_BSP) {
        flags.f.message = 0;
        flags.f.two_line_message = 0;
        redisplay();
        return;
    }

    flags.f.message = 0;
    flags.f.two_line_message = 0;

    if (mode_number_entry && get_base() == 16 && key == KEY_SIGMA
            && (menu = get_front_menu()) != NULL && *menu == MENU_BASE) {
        /* Special case -- entering the A...F menu while in base 16
         * does *not* cancel number entry mode (unlike all other menu
         * keys)... So we intercept and handle it before all the other
         * logic can mess things up.
         */
        keydown_number_entry(-1, -1);
        return;
    }

    if (mode_number_entry
            && !is_number_key(shift, key)
            && (key != KEY_CHS || shift || basekeys() || get_base() != 10)
            && (key != KEY_BSP || shift)) {
        /* Leaving number entry mode */
        mode_number_entry = false;
        if (flags.f.prgm_mode) {
            arg_struct arg;
            arg.type = ARGTYPE_DOUBLE;
            arg.val_d = entered_number;
            store_command(pc, CMD_NUMBER, &arg);
            prgm_highlight_row = 1;
        } else if ((flags.f.trace_print || flags.f.normal_print)
                && flags.f.printer_exists)
            deferred_print = 1;
    }

    if (mode_command_entry
            && (shift || get_front_menu() == NULL)
            && (key == KEY_UP || key == KEY_DOWN)) {
        /* Trying to do SST or BST while in command entry mode */
        squeak();
        return;
    }

    if (key == KEY_UP || (key == KEY_DOWN &&
                (flags.f.prgm_mode || (!shift && get_front_menu() != NULL)))) {
        /* UP, DOWN, BST, or prgm-mode SST */
        repeating = 1;
        repeating_shift = shift;
        repeating_key = key;
    }

    if (flags.f.prgm_mode && (key == KEY_UP || key == KEY_DOWN)
            && (shift || get_front_menu() == NULL)) {
        /* Stepping through the program in prgm mode */
        if (flags.f.prgm_mode && mode_alpha_entry)
            finish_alpha_prgm_line();
        clear_all_rtns();
        if (key == KEY_UP)
            bst();
        else
            sst();
        redisplay();
        return;
    }    
    
    if (!flags.f.prgm_mode && key == KEY_UP
            && (shift || get_front_menu() == NULL)) {
        /* BST in normal or alpha mode */
        if (mode_alpha_entry
                && (flags.f.trace_print || flags.f.normal_print)
                && flags.f.printer_exists)
            docmd_pra(NULL);
        mode_alpha_entry = false;
        clear_all_rtns();
        bst();
        flags.f.prgm_mode = 1;
        redisplay();
        flags.f.prgm_mode = 0;
        pending_command = CMD_CANCELLED;
        return;
    }

    if (mode_number_entry) 
        keydown_number_entry(shift, key);
    else if (mode_command_entry)
        keydown_command_entry(shift, key);
    else if (core_alpha_menu())
        keydown_alpha_mode(shift, key);
    else
        keydown_normal_mode(shift, key);
}

void keydown_number_entry(int shift, int key) {
    phloat x;
    char buf[100];
    int bufptr;
    int base = get_base();

    if (key == -1) {
        /* Hack... The user is switching to the A...F menu */
        set_menu(MENULEVEL_APP, MENU_BASE_A_THRU_F);
        redisplay();
        goto draw_number;
    }

    if (base != 10 && (key == KEY_E || key == KEY_DOT))
        return;

    /* NOTE: 'key' can only be KEY_CHS at this point in the code
     * if 'baseapp' is false and get_base() returns 10; in all other
     * cases, the +/- key will end number entry mode and we won't
     * get here, and the CMD_CHS or CMD_BASECHS function will be
     * invoked instead.
     */

    if (key == KEY_BSP && cmdline_length == 1) {
        mode_number_entry = false;
        if (flags.f.prgm_mode) {
            pc = line2pc(pc2line(pc) - 1);
            prgm_highlight_row = 0;
            redisplay();
            return;
        } else {
            pending_command = CMD_CLX;
            return;
        }
    }

    if (key == KEY_BSP) {
        cmdline_length--;
        if (!flags.f.prgm_mode && base == 10)
            fix_thousands_separators(cmdline, &cmdline_length);
        if (core_settings.auto_repeat) {
            repeating = 2;
            repeating_key = key;
            repeating_shift = shift;
        }
    } else if (key == KEY_CHS) {
        /* Check if mantissa or exponent gets the sign change */
        int i;
        int exp_pos = -1;
        for (i = 0; i < cmdline_length; i++)
            if (cmdline[i] == 24) {
                exp_pos = i;
                break;
            }
        if (exp_pos != -1) {
            /* Change exponent sign */
            int failed = 0;
            phloat d;
            undo_chs_exp:
            if (cmdline_length > exp_pos + 1
                    && cmdline[exp_pos + 1] == '-') {
                for (i = exp_pos + 1; i < cmdline_length - 1; i++)
                    cmdline[i] = cmdline[i + 1];
                cmdline_length--;
            } else {
                for (i = cmdline_length; i > exp_pos + 1; i--)
                    cmdline[i] = cmdline[i - 1];
                cmdline[exp_pos + 1] = '-';
                cmdline_length++;
            }
            if (!failed) {
                if (string2phloat(cmdline, cmdline_length, &d) != 0) {
                    failed = 1;
                    goto undo_chs_exp;
                }
            } else
                return;
        } else {
            /* Change mantissa sign */
            if (cmdline[0] == '-') {
                if (cmdline_length == 1) {
                    if (flags.f.prgm_mode) {
                        mode_number_entry = false;
                        pc = line2pc(pc2line(pc) - 1);
                            prgm_highlight_row = 0;
                        redisplay();
                        return;
                    } else {
                        /* This is a bit odd, but it's how the HP-42S
                         * does it, so there.
                         */
                        mode_number_entry = false;
                        free_vartype(reg_x);
                        reg_x = new_real(0);
                        pending_command = CMD_CLX;
                        return;
                    }
                }
                for (i = 0; i < cmdline_length - 1; i++)
                    cmdline[i] = cmdline[i + 1];
                cmdline_length--;
            } else {
                for (i = cmdline_length; i > 0; i--)
                    cmdline[i] = cmdline[i - 1];
                cmdline[0] = '-';
                cmdline_length++;
            }
        }
    } else if (key == KEY_E) {
        int exp_pos = -1;
        int only_zeroes = 1;
        int seen_dot = 0;
        char dot = flags.f.decimal_point ? '.' : ',';
        int i;
        for (i = 0; i < cmdline_length; i++) {
            char c = cmdline[i];
            if (c >= '1' && c <= '9')
                only_zeroes = 0;
            else if (c == dot)
                seen_dot = 1;
            else if (c == 24) {
                exp_pos = i;
                break;
            }
        }
        if (exp_pos == -1) {
            if (only_zeroes) {
                if (cmdline_length > 0 && cmdline[0] == '-')
                    cmdline_length = 1;
                else
                    cmdline_length = 0;
                cmdline[cmdline_length++] = '1';
                if (seen_dot)
                    cmdline[cmdline_length++] = dot;
            }
            cmdline[cmdline_length++] = 24;
        } else
            return;
    } else if (key == KEY_DOT) {
        if (cmdline_length == 0 ||
                (cmdline_length == 1 && cmdline[0] == '-')) {
            cmdline[cmdline_length++] = '0';
            cmdline[cmdline_length++] = flags.f.decimal_point ? '.' : ',';
        } else {
            /* Only allow dot if there isn't one already, and
             * there is no exponent either
             */
            int dot_or_exp_pos = -1;
            int i;
            char dot = flags.f.decimal_point ? '.' : ',';
            for (i = 0; i < cmdline_length; i++)
                if (cmdline[i] == dot || cmdline[i] == 24) {
                    dot_or_exp_pos = i;
                    break;
                }
            if (dot_or_exp_pos == -1)
                cmdline[cmdline_length++] = dot;
            else
                return;
        }
    } else /* KEY_0 .. KEY_9 or hex A-F */ {
        int digit;
        char c;
        switch (key) {
            case KEY_0: digit = 0; break;
            case KEY_1: digit = 1; break;
            case KEY_2: digit = 2; break;
            case KEY_3: digit = 3; break;
            case KEY_4: digit = 4; break;
            case KEY_5: digit = 5; break;
            case KEY_6: digit = 6; break;
            case KEY_7: digit = 7; break;
            case KEY_8: digit = 8; break;
            case KEY_9: digit = 9; break;
            case KEY_SIGMA: digit = 10; break;
            case KEY_INV:   digit = 11; break;
            case KEY_SQRT:  digit = 12; break;
            case KEY_LOG:   digit = 13; break;
            case KEY_LN:    digit = 14; break;
            case KEY_XEQ:   digit = 15; break;
        }
        if (digit >= base)
            return;
        if (core_settings.auto_repeat) {
            repeating = 2;
            repeating_key = key;
            repeating_shift = shift;
        }
        c = digit < 10 ? '0' + digit : 'A' + digit - 10;
        cmdline[cmdline_length++] = c;
        if (base == 10) {
            if (string2phloat(cmdline, cmdline_length, &x) == 0) {
                if (!flags.f.prgm_mode)
                    fix_thousands_separators(cmdline, &cmdline_length);
            } else {
                cmdline_length--;
                return;
            }
        } else {
            int bits = base == 2 ? 1 : base == 8 ? 3 : 4;
            bits *= cmdline_length;
            if (bits > 36) {
                cmdline_length--;
                return;
            }
        }
    }

    if (base == 10) {
        if (string2phloat(cmdline, cmdline_length, &x) != 0)
            /* Should never happen */
            x = 0;
    } else {
        int8 n = 0;
        int i;
        for (i = 0; i < cmdline_length; i++) {
            char c = cmdline[i];
            int digit = c <= '9' ? c - '0' : c - 'A' + 10;
            n = n * base + digit;
        }
        if (n & LL(0x800000000))
            n |= LL(0xfffffff000000000);
        x = (phloat) n;
    }

    if (flags.f.prgm_mode)
        entered_number = x;
    else {
        free_vartype(reg_x);
        reg_x = new_real(x);
    }

    draw_number:

    bufptr = 0;
    if (flags.f.prgm_mode) {
        int line = pc2line(pc);
        if (line < 10)
            char2buf(buf, 100, &bufptr, '0');
        bufptr += int2string(line, buf + bufptr, 100 - bufptr);
        char2buf(buf, 100, &bufptr, 6);
    } else if (matedit_mode == 2 || matedit_mode == 3) {
        bufptr += int2string(matedit_i + 1, buf + bufptr, 100 - bufptr);
        char2buf(buf, 100, &bufptr, ':');
        bufptr += int2string(matedit_j + 1, buf + bufptr, 100 - bufptr);
        char2buf(buf, 100, &bufptr, '=');
    } else if (input_length > 0) {
        string2buf(buf, 100, &bufptr, input_name, input_length);
        char2buf(buf, 100, &bufptr, '?');
    } else
        string2buf(buf, 100, &bufptr, "x\200", 2);
    string2buf(buf, 100, &bufptr, cmdline, cmdline_length);
    char2buf(buf, 100, &bufptr, '_');

    clear_row(cmdline_row);
    if (bufptr <= 22)
        draw_string(0, cmdline_row, buf, bufptr);
    else {
        draw_char(0, cmdline_row, 26);
        draw_string(1, cmdline_row, buf + bufptr - 21, 21);
    }
    flush_display();
    return;
}

void keydown_command_entry(int shift, int key) {
    if (mode_commandmenu == MENU_ST || mode_commandmenu == MENU_IND_ST) {
        int menukey;
        if (!shift && key == KEY_BSP) {
            pending_command = CMD_NULL;
            finish_command_entry(false);
            return;
        }
        if (key == KEY_EXIT) {
            pending_command = CMD_CANCELLED;
            finish_command_entry(false);
            return;
        }
        menukey = find_menu_key(key);
        if (!shift && ((menukey >= 0 && menukey <= 4)
                || (menukey == 5 && mode_commandmenu == MENU_IND_ST))) {
            if (mode_commandmenu == MENU_IND_ST && menukey == 0) {
                incomplete_ind = 1;
                incomplete_maxdigits = 2;
                set_catalog_menu(CATSECT_REAL_ONLY);
                redisplay();
                return;
            }
            if (mode_commandmenu == MENU_IND_ST)
                menukey--;
            pending_command = incomplete_command;
            pending_command_arg.type =
                        incomplete_ind ? ARGTYPE_IND_STK : ARGTYPE_STK;
            pending_command_arg.val.stk = "LXYZT"[menukey];
            finish_command_entry(true);
            return;
        }
        squeak();
        return;
    }

    if (incomplete_command == CMD_LBL && incomplete_length == 0
            && mode_commandmenu != MENU_CATALOG) {
        /* LBL is weird. It's sort of like you have alpha and numeric
         * at the same time. When we're at length 0, we have to handle
         * both possibilities and pick the right one.
         */

        /* On the Palm, we accept 0-9 as alphanumeric in this case;
         * if the user wants numeric, it's easy enough to just tap
         * the appropriate keys on the virtual keyboard.
         * On PCs, on the other hand, we want the number keys to
         * behave like they always do, so you can enter LBL 00 without
         * having to use the virtual keyboard. If an alpha LBL with
         * a name starting with '0'-'9' is desired, the user will have
         * to use the usual trick: activate a submenu of ALPHA by
         * pressing any menu key (F1-F6), and then typing the number.
         */
        if ((mode_commandmenu == MENU_ALPHA1 || mode_commandmenu == MENU_ALPHA2)
                && key >= 1024 + '0' && key <= 1024 + '9')
            switch (key - 1024) {
                case '0': key = KEY_0; break;
                case '1': key = KEY_1; break;
                case '2': key = KEY_2; break;
                case '3': key = KEY_3; break;
                case '4': key = KEY_4; break;
                case '5': key = KEY_5; break;
                case '6': key = KEY_6; break;
                case '7': key = KEY_7; break;
                case '8': key = KEY_8; break;
                case '9': key = KEY_9; break;
            }

        if (key >= 1024 && key < 2048
            || (key == KEY_SIGMA || key == KEY_INV || key == KEY_SQRT
                    || key == KEY_LOG || key == KEY_LN || key == KEY_XEQ)
            || (!shift &&
                (key == KEY_E || key == KEY_UP || key == KEY_DOWN
                    || key == KEY_DIV || key == KEY_MUL || key == KEY_SUB
                    || key == KEY_ADD || key == KEY_DOT))
            || (shift &&
                (key == KEY_RCL || key == KEY_RDN))
            || (mode_commandmenu >= MENU_ALPHA_ABCDE1 &&
                mode_commandmenu <= MENU_ALPHA_MISC2 && (
                    key == KEY_0 || key == KEY_1 || key == KEY_2
                    || key == KEY_3 || key == KEY_4 || key == KEY_5
                    || key == KEY_6 || key == KEY_7 || key == KEY_8
                    || key == KEY_9)))
            goto do_incomplete_alpha;
        if (shift && key == KEY_ADD) {
            if (mode_commandmenu == MENU_CATALOG)
                squeak();
            else {
                incomplete_alpha = 1;
                set_catalog_menu(CATSECT_TOP);
                redisplay();
            }
            return;
        }
        if (key == KEY_EXIT && mode_commandmenu >= MENU_ALPHA_ABCDE1
                            && mode_commandmenu <= MENU_ALPHA_MISC2) {
            if (mode_commandmenu <= MENU_ALPHA_WXYZ)
                mode_commandmenu = MENU_ALPHA1;
            else
                mode_commandmenu = MENU_ALPHA2;
            redisplay();
            return;
        }
    }
    
    if (incomplete_command == CMD_LBL && !incomplete_alpha && incomplete_length == 1
            && shift && key == KEY_ENTER) {
        /* More LBL weirdness: you can switch to ALPHA mode while entering
         * a numeric LBL
         */
        incomplete_alpha = 1;
        incomplete_str[0] = '0' + incomplete_num;
        incomplete_num = 0;
        mode_commandmenu = MENU_ALPHA1;
        redisplay();
        return;
    }

    if ((incomplete_command == CMD_ASTO || incomplete_command == CMD_ARCL)
            && incomplete_length == 1 && mode_commandmenu == MENU_NONE
            && mode_alphamenu >= MENU_ALPHA1 && mode_alphamenu <= MENU_ALPHA_MISC2) {
        /* A similar oddity are ASTO and ARCL, when entered using the
         * STO and RCL keys while ALPHA mode is active. Initially,
         * you'll get a menu of all variables, but when you type a
         * digit, the variables menu disappears, and the ALPHA menu
         * returns... but we still want 0-9 to be treated as numeric.
         */
        switch (key - 1024) {
            case '0': key = KEY_0; break;
            case '1': key = KEY_1; break;
            case '2': key = KEY_2; break;
            case '3': key = KEY_3; break;
            case '4': key = KEY_4; break;
            case '5': key = KEY_5; break;
            case '6': key = KEY_6; break;
            case '7': key = KEY_7; break;
            case '8': key = KEY_8; break;
            case '9': key = KEY_9; break;
        }
    }

    
    /* Another oddity: ASSIGN... */
    if (incomplete_argtype == ARG_CKEY) {
        int menukey = find_menu_key(key);
        if (menukey != -1) {
            pending_command = CMD_ASGN01 + menukey
                            + 6 * (mode_commandmenu - MENU_CUSTOM1);
            finish_command_entry(true);
        } else if (!shift && key == KEY_BSP) {
            pending_command = CMD_NULL;
            finish_command_entry(false);
        } else if (key == KEY_EXIT) {
            pending_command = CMD_CANCELLED;
            finish_command_entry(false);
        } else if (!shift && (key == KEY_UP || key == KEY_DOWN)) {
            mode_commandmenu += key == KEY_UP ? -1 : 1;
            if (mode_commandmenu < MENU_CUSTOM1)
                set_menu(MENULEVEL_COMMAND, MENU_CUSTOM3);
            else if (mode_commandmenu > MENU_CUSTOM3)
                set_menu(MENULEVEL_COMMAND, MENU_CUSTOM1);
            redisplay();
        } else
            squeak();
        return;
    }

    /* And yet another oddity: KEYG and KEYX */
    if (incomplete_argtype == ARG_MKEY) {
        int cmd = incomplete_command == CMD_KEYG ? CMD_KEY1G : CMD_KEY1X;
        if (shift) {
            squeak();
            return;
        }
        switch (key) {
            case KEY_SIGMA: case KEY_1: break;
            case KEY_INV: case KEY_2: cmd++; break;
            case KEY_SQRT: case KEY_3: cmd += 2; break;
            case KEY_LOG: case KEY_4: cmd += 3; break;
            case KEY_LN: case KEY_5: cmd += 4; break;
            case KEY_XEQ: case KEY_6: cmd += 5; break;
            case KEY_UP: case KEY_7: cmd += 6; break;
            case KEY_DOWN: case KEY_8: cmd += 7; break;
            case KEY_EXIT: case KEY_9: cmd += 8; break;
            case KEY_BSP:
                pending_command = CMD_NULL;
                finish_command_entry(false);
                return;
            default:
                squeak();
                return;
        }
        start_incomplete_command(cmd);
        return;
    }

    if (mode_commandmenu == MENU_CATALOG) {
        int menukey = find_menu_key(key);
        int catsect = get_cat_section();
        if (menukey != -1) {
            if (catsect == CATSECT_TOP) {
                switch (menukey) {
                    case 0:
                        set_cat_section(CATSECT_FCN);
                        move_cat_row(0);
                        break;
                    case 1:
                        set_cat_section(CATSECT_PGM);
                        move_cat_row(0);
                        break;
                    case 2:
                        if (!vars_exist(1, 0, 0)) {
                            squeak();
                            return;
                        } else {
                            set_cat_section(CATSECT_REAL);
                            move_cat_row(0);
                            break;
                        }
                    case 3:
                        if (!vars_exist(0, 1, 0)) {
                            squeak();
                            return;
                        } else {
                            set_cat_section(CATSECT_CPX);
                            move_cat_row(0);
                            break;
                        }
                    case 4:
                        if (!vars_exist(0, 0, 1)) {
                            squeak();
                            return;
                        } else {
                            set_cat_section(CATSECT_MAT);
                            move_cat_row(0);
                            break;
                        }
                    case 5: display_mem();
                            pending_command = CMD_LINGER1;
                            shell_request_timeout3(2000);
                            return;
                }
                redisplay();
                return;
            } else {
                int i, itemindex;
                itemindex = get_cat_item(menukey);
                if (itemindex == -1) {
                    squeak();
                    return;
                }
                if (catsect == CATSECT_PGM || catsect == CATSECT_PGM_ONLY) {
                    if (labels[itemindex].length == 0) {
                        /* END or .END. */
                        if (incomplete_command != CMD_GTODOT
                                && incomplete_command != CMD_PRP
                                && (flags.f.prgm_mode
                                    || (incomplete_command != CMD_GTO
                                        && incomplete_command != CMD_XEQ
                                        && incomplete_command != CMD_CLP))) {
                            squeak();
                            return;
                        }
                    }
                    pending_command = incomplete_command;
                    if (incomplete_command == CMD_GTO
                            || incomplete_command == CMD_GTODOT
                            || incomplete_command == CMD_XEQ
                            || incomplete_command == CMD_CLP
                            || incomplete_command == CMD_PRP) {
                        pending_command_arg.type = ARGTYPE_LBLINDEX;
                        pending_command_arg.val.num = itemindex;
                        xeq_invisible = 0;
                    } else {
                        pending_command_arg.type =
                            incomplete_ind ? ARGTYPE_IND_STR : ARGTYPE_STR;
                        pending_command_arg.length =
                                                labels[itemindex].length;
                        for (i = 0; i < pending_command_arg.length; i++)
                            pending_command_arg.val.text[i] =
                                                labels[itemindex].name[i];
                    }
                    finish_command_entry(true);
                    return;
                }
                pending_command = incomplete_command;
                pending_command_arg.type =
                            incomplete_ind ? ARGTYPE_IND_STR : ARGTYPE_STR;
                if (catsect == CATSECT_FCN) {
                    const command_spec *cs = cmdlist(itemindex);
                    pending_command_arg.length = cs->name_length;
                    for (i = 0; i < pending_command_arg.length; i++)
                        pending_command_arg.val.text[i] = cs->name[i];
                } else {
                    pending_command_arg.length = vars[itemindex].length;
                    for (i = 0; i < pending_command_arg.length; i++)
                        pending_command_arg.val.text[i] =
                                            vars[itemindex].name[i];
                }
                if (!incomplete_ind && incomplete_command == CMD_XEQ)
                    finish_xeq();
                else
                    finish_command_entry(true);
                return;
            }
        }
        if (!shift && (key == KEY_UP || key == KEY_DOWN)) {
            move_cat_row(key == KEY_UP ? -1 : 1);
            redisplay();
            return;
        }
        if (key == KEY_EXIT) {
            if (catsect == CATSECT_FCN
                    || catsect == CATSECT_PGM
                    || catsect == CATSECT_REAL
                    || catsect == CATSECT_CPX
                    || catsect == CATSECT_MAT) {
                set_cat_section(CATSECT_TOP);
                redisplay();
            } else {
                pending_command = CMD_CANCELLED;
                finish_command_entry(false);
            }
            return;
        }
    }

    if (mode_commandmenu == MENU_VARMENU) {
        int menukey = find_menu_key(key);
        if (menukey != -1) {
            int i;
            if (varmenu_labellength[menukey] == 0) {
                squeak();
                return;
            }
            pending_command = incomplete_command;
            pending_command_arg.type = ARGTYPE_STR;
            pending_command_arg.length = varmenu_labellength[menukey];
            for (i = 0; i < pending_command_arg.length; i++)
                pending_command_arg.val.text[i] =
                                        varmenu_labeltext[menukey][i];
            finish_command_entry(false);
            return;
        }
        if (!shift && (key == KEY_UP || key == KEY_DOWN)) {
            if (varmenu_rows > 1) {
                if (key == KEY_UP) {
                    varmenu_row--;
                    if (varmenu_row < 0)
                        varmenu_row = varmenu_rows - 1;
                } else {
                    varmenu_row++;
                    if (varmenu_row >= varmenu_rows)
                        varmenu_row = 0;
                }
                redisplay();
            }
            return;
        }
        if (key == KEY_EXIT) {
            pending_command = CMD_CANCELLED;
            finish_command_entry(false);
            return;
        }
    } else if (mode_commandmenu == MENU_INTEG_PARAMS) {
        int menukey = find_menu_key(key);
        if (menukey != -1) {
            const char *name;
            int length, i;
            switch (menukey) {
                case 0: name = "LLIM"; length = 4; break;
                case 1: name = "ULIM"; length = 4; break;
                case 2: name = "ACC";  length = 3; break;
                default: squeak(); return;
            }
            pending_command = incomplete_command;
            pending_command_arg.type = ARGTYPE_STR;
            pending_command_arg.length = length;
            for (i = 0; i < length; i++)
                pending_command_arg.val.text[i] = name[i];
            finish_command_entry(false);
            return;
        }
        if (key == KEY_EXIT) {
            pending_command = CMD_CANCELLED;
            finish_command_entry(false);
            return;
        }
    }

    if (!incomplete_alpha) {
        if (key == KEY_EXIT) {
            pending_command = CMD_CANCELLED;
            finish_command_entry(false);
            return;
        }

        if (mode_commandmenu == MENU_IND) {
            if (!shift && key == KEY_SIGMA) {
                incomplete_command = CMD_GTO;
                incomplete_argtype = ARG_LBL;
                incomplete_ind = 1;
                incomplete_maxdigits = 2;
                set_catalog_menu(CATSECT_REAL_ONLY);
                redisplay();
                return;
            } else if (key == KEY_ENTER) {
                incomplete_argtype = ARG_LBL;
                incomplete_alpha = 1;
                set_menu(MENULEVEL_COMMAND, MENU_ALPHA1);
                redisplay();
                return;
            }
        }

        if (incomplete_command == CMD_STO
                && !shift && incomplete_length == 0
                && (key == KEY_DIV || key == KEY_MUL
                    || key == KEY_SUB || key == KEY_ADD)) {
            switch (key) {
                case KEY_DIV: incomplete_command = CMD_STO_DIV; break;
                case KEY_MUL: incomplete_command = CMD_STO_MUL; break;
                case KEY_SUB: incomplete_command = CMD_STO_SUB; break;
                case KEY_ADD: incomplete_command = CMD_STO_ADD; break;
            }
            redisplay();
            return;
        }
        if (incomplete_command == CMD_RCL
                && !shift && incomplete_length == 0
                && (key == KEY_DIV || key == KEY_MUL
                    || key == KEY_SUB || key == KEY_ADD)) {
            switch (key) {
                case KEY_DIV: incomplete_command = CMD_RCL_DIV; break;
                case KEY_MUL: incomplete_command = CMD_RCL_MUL; break;
                case KEY_SUB: incomplete_command = CMD_RCL_SUB; break;
                case KEY_ADD: incomplete_command = CMD_RCL_ADD; break;
            }
            redisplay();
            return;
        }
        if (incomplete_command == CMD_GTO
                && !incomplete_ind
                && !shift && incomplete_length == 0
                && key == KEY_DOT) {
            incomplete_command = CMD_GTODOT;
            incomplete_argtype = ARG_OTHER;
            incomplete_maxdigits = 4;
            set_menu(MENULEVEL_COMMAND, MENU_IND);
            redisplay();
            return;
        }
        if (incomplete_command == CMD_GTODOT
                && !shift && incomplete_length == 0
                && key == KEY_DOT) {
            pending_command = CMD_GTODOTDOT;
            pending_command_arg.type = ARGTYPE_NONE;
            finish_command_entry(false);
            return;
        }
            
        
        if (key == KEY_ENTER) {
            if (incomplete_length == 0) {
                if (incomplete_ind
                        && !flags.f.prgm_mode
                        && !vars_exist(1, 0, 0))
                    squeak();
                else if (incomplete_ind
                        || incomplete_argtype == ARG_VAR
                        || incomplete_argtype == ARG_REAL
                        || incomplete_argtype == ARG_NAMED
                        || incomplete_argtype == ARG_LBL
                        || incomplete_argtype == ARG_PRGM) {
                    incomplete_alpha = 1;
                    set_menu(MENULEVEL_COMMAND, MENU_ALPHA1);
                    redisplay();
                } else
                    squeak();
                return;
            } else if (!shift) {
                if (incomplete_command == CMD_GOTOROW) {
                    pending_command_arg.val.num = incomplete_num;
                    start_incomplete_command(CMD_GOTOCOLUMN);
                    return;
                } else if (incomplete_command == CMD_GOTOCOLUMN) {
                    matedit_goto(pending_command_arg.val.num, incomplete_num);
                    pending_command = CMD_NONE;
                    finish_command_entry(true);
                    return;
                }
                pending_command = incomplete_command;
                pending_command_arg.type =
                            incomplete_ind ? ARGTYPE_IND_NUM : ARGTYPE_NUM;
                pending_command_arg.length = incomplete_maxdigits;
                pending_command_arg.val.num = incomplete_num;
                finish_command_entry(false);
                return;
            }
        }

        if (incomplete_length == 0 && !shift && key == KEY_DOT) {
            if (incomplete_ind) {
                if (incomplete_argtype == ARG_VAR
                        || incomplete_argtype == ARG_REAL
                        || incomplete_argtype == ARG_MAT
                        || incomplete_argtype == ARG_RVAR
                        || incomplete_argtype == ARG_NAMED
                        || incomplete_argtype == ARG_LBL
                        || incomplete_argtype == ARG_PRGM
                        || incomplete_argtype == ARG_NUM9
                        || incomplete_argtype == ARG_NUM11
                        || incomplete_argtype == ARG_NUM99) {
                    set_menu(MENULEVEL_COMMAND, MENU_ST);
                    redisplay();
                }
            } else {
                if (incomplete_argtype == ARG_VAR
                        || incomplete_argtype == ARG_REAL) {
                    set_menu(MENULEVEL_COMMAND, MENU_IND_ST);
                    redisplay();
                } else if (incomplete_argtype == ARG_NUM9
                        || incomplete_argtype == ARG_NUM11
                        || incomplete_argtype == ARG_NUM99
                        || incomplete_argtype == ARG_LBL) {
                    incomplete_ind = 1;
                    incomplete_maxdigits = 2;
                    set_catalog_menu(CATSECT_REAL_ONLY);
                    redisplay();
                } else {
                    squeak();
                }
            }
            return;
        }

        if (incomplete_length < incomplete_maxdigits && !shift &&
                (key == KEY_0 || key == KEY_1 || key == KEY_2
                    || key == KEY_3 || key == KEY_4 || key == KEY_5
                    || key == KEY_6 || key == KEY_7 || key == KEY_8
                    || key == KEY_9)) {
            int digit;
            switch (key) {
                case KEY_0: digit = 0; break;
                case KEY_1: digit = 1; break;
                case KEY_2: digit = 2; break;
                case KEY_3: digit = 3; break;
                case KEY_4: digit = 4; break;
                case KEY_5: digit = 5; break;
                case KEY_6: digit = 6; break;
                case KEY_7: digit = 7; break;
                case KEY_8: digit = 8; break;
                case KEY_9: digit = 9; break;
            }
            incomplete_num = incomplete_num * 10 + digit;
            incomplete_length++;
            if (incomplete_length == incomplete_maxdigits) {
                if (incomplete_command == CMD_GOTOROW) {
                    pending_command_arg.val.num = incomplete_num;
                    start_incomplete_command(CMD_GOTOCOLUMN);
                    return;
                } else if (incomplete_command == CMD_GOTOCOLUMN) {
                    matedit_goto(pending_command_arg.val.num, incomplete_num);
                    pending_command = CMD_NONE;
                    finish_command_entry(true);
                    return;
                }
                pending_command = incomplete_command;
                pending_command_arg.type =
                            incomplete_ind ? ARGTYPE_IND_NUM : ARGTYPE_NUM;
                pending_command_arg.length = incomplete_maxdigits;
                if (!incomplete_ind &&
                        incomplete_argtype == ARG_NUM11 && incomplete_num > 11)
                    incomplete_num = 11;
                pending_command_arg.val.num = incomplete_num;
                finish_command_entry(true);
                return;
            } else
                set_menu(MENULEVEL_COMMAND, MENU_NONE);
            redisplay();
            return;
        }

        if (incomplete_length < incomplete_maxdigits
                    && !shift && key == KEY_BSP) {
            if (incomplete_length == 0) {
                pending_command = CMD_NULL;
                finish_command_entry(false);
                return;
            } else {
                incomplete_length--;
                incomplete_num /= 10;
                if (incomplete_length == 0) {
                    if (incomplete_command >= CMD_KEY1G
                            && incomplete_command <= CMD_KEY9X)
                        start_incomplete_command(
                                incomplete_command <= CMD_KEY9G ? CMD_KEYG
                                                                : CMD_KEYX);
                    else if (incomplete_argtype == ARG_VAR)
                        if (mode_appmenu == MENU_VARMENU)
                            set_menu(MENULEVEL_COMMAND, MENU_VARMENU);
                        else if (mode_appmenu == MENU_INTEG_PARAMS)
                            set_menu(MENULEVEL_COMMAND, MENU_INTEG_PARAMS);
                        else
                            set_catalog_menu(CATSECT_VARS_ONLY);
                    else if (incomplete_ind)
                        set_catalog_menu(CATSECT_REAL_ONLY);
                    else if (incomplete_argtype == ARG_REAL)
                        if (mode_appmenu == MENU_VARMENU)
                            set_menu(MENULEVEL_COMMAND, MENU_VARMENU);
                        else if (mode_appmenu == MENU_INTEG_PARAMS)
                            set_menu(MENULEVEL_COMMAND, MENU_INTEG_PARAMS);
                        else
                            set_catalog_menu(CATSECT_REAL_ONLY);
                    else if (incomplete_argtype == ARG_LBL)
                        set_catalog_menu(CATSECT_PGM_ONLY);
                    else if (incomplete_command == CMD_GTODOT)
                        set_menu(MENULEVEL_COMMAND, MENU_IND);
                    else if (incomplete_command == CMD_LBL)
                        set_menu(MENULEVEL_COMMAND, MENU_ALPHA1);
                }
                redisplay();
                return;
            }
        }

        /* Some bad key... */
        squeak();
        return;

    } else {
        /* incomplete_alpha */

        int menukey;
        const menu_spec *m;
        char c;

        do_incomplete_alpha:
        m = NULL;

        if (key >= 1024 && key < 2048) {
            c = key - 1024;
            goto handle_char;
        } else if (mode_commandmenu != MENU_NONE
                && (menukey = find_menu_key(key)) != -1) {
            const menu_item_spec *mi;
            m = menus + mode_commandmenu;
            mi = m->child + menukey;
            if (mi->menuid != MENU_NONE) {
                set_menu(MENULEVEL_COMMAND, mi->menuid);
                redisplay();
                return;
            }
            c = mi->title[0];
            if (shift && c >= 'A' && c <= 'Z')
                c += 'a' - 'A';
            handle_char:
            if (incomplete_length < 7)
                incomplete_str[incomplete_length++] = c;
            if (m != NULL)
                set_menu(MENULEVEL_COMMAND, m->parent);
            /* incomplete_alpha can be 0 at this point if
             * the command is CMD_LBL. */
            incomplete_alpha = 1;
            redisplay();
            return;
        }

        if ((incomplete_argtype == ARG_NAMED
                    || incomplete_argtype == ARG_PRGM
                    || incomplete_argtype == ARG_RVAR
                    || incomplete_argtype == ARG_MAT)
                && incomplete_command != CMD_ASSIGNa
                && incomplete_command != CMD_CLP
                && incomplete_command != CMD_PRP
                && incomplete_command != CMD_MVAR
                && !incomplete_ind
                && incomplete_length == 0
                && (mode_commandmenu < MENU_ALPHA1
                    || mode_commandmenu > MENU_ALPHA_MISC2)
                && !shift && key == KEY_DOT) {
            incomplete_ind = 1;
            incomplete_alpha = 0;
            set_catalog_menu(CATSECT_REAL_ONLY);
            redisplay();
            return;
        }

        if (incomplete_length == 0 && shift && key == KEY_ADD) {
            if (mode_commandmenu == MENU_CATALOG)
                squeak();
            else {
                set_catalog_menu(CATSECT_TOP);
                redisplay();
            }
            return;
        }

        /* Handle keys that represent characters */
        if (mode_commandmenu == MENU_CATALOG)
            goto nocharkey1;
        if (!shift) {
            switch (key) {
                case KEY_0:   c = '0'; break;
                case KEY_1:   c = '1'; break;
                case KEY_2:   c = '2'; break;
                case KEY_3:   c = '3'; break;
                case KEY_4:   c = '4'; break;
                case KEY_5:   c = '5'; break;
                case KEY_6:   c = '6'; break;
                case KEY_7:   c = '7'; break;
                case KEY_8:   c = '8'; break;
                case KEY_9:   c = '9'; break;
                case KEY_DOT: c = '.'; break;
                case KEY_E:   c = 24;  break;
                case KEY_DIV: c = 0;   break;
                case KEY_MUL: c = 1;   break;
                case KEY_SUB: c = '-'; break;
                case KEY_ADD: c = '+'; break;
                default: goto nocharkey1;
            }
        } else {
            switch (key) {
                case KEY_RCL: c = '%'; break;
                case KEY_RDN: c = 7;   break;
                default: goto nocharkey1;
            }
        }
        if (incomplete_length < 7)
            incomplete_str[incomplete_length++] = c;
        /* incomplete_alpha can be 0 at this point if
         * the command is CMD_LBL. */
        incomplete_alpha = 1;
        redisplay();
        return;
        nocharkey1:
        /* End of handling keys that represent characters */

        if (!shift && (key == KEY_UP || key == KEY_DOWN)) {
            const menu_spec *m = menus + mode_commandmenu;
            int nextmenu = key == KEY_UP ? m->prev : m->next;
            if (nextmenu != MENU_NONE) {
                set_menu(MENULEVEL_COMMAND, nextmenu);
                redisplay();
            }
            return;
        }
        
        if (key == KEY_EXIT) {
            const menu_spec *m;
            if (mode_commandmenu == MENU_NONE) {
                pending_command = CMD_CANCELLED;
                finish_command_entry(false);
                return;
            }
            if (mode_commandmenu == MENU_CATALOG) {
                int catsect = get_cat_section();
                if (catsect == CATSECT_PGM
                        || catsect == CATSECT_REAL
                        || catsect == CATSECT_CPX
                        || catsect == CATSECT_MAT) {
                    set_cat_section(CATSECT_TOP);
                    redisplay();
                    return;
                }
            }
            m = menus + mode_commandmenu;
            set_menu(MENULEVEL_COMMAND, m->parent);
            if (mode_commandmenu == MENU_NONE) {
                pending_command = CMD_CANCELLED;
                finish_command_entry(false);
            } else
                redisplay();
            return;
        }

        if (!shift && key == KEY_BSP) {
            if (incomplete_command >= CMD_KEY1G
                    && incomplete_command <= CMD_KEY9X) {
                if (incomplete_length == 0) {
                    if (mode_commandmenu >= MENU_ALPHA1
                            && mode_commandmenu <= MENU_ALPHA_MISC2)
                        start_incomplete_command(
                                incomplete_command <= CMD_KEY9G
                                        ? CMD_KEYG : CMD_KEYX);
                    else {
                        pending_command = CMD_NULL;
                        finish_command_entry(false);
                    }
                } else {
                    incomplete_length--;
                    redisplay();
                }
                return;
            }
            if (incomplete_length == 0) {
                int catsect;
                if (mode_commandmenu >= MENU_ALPHA1
                        && mode_commandmenu <= MENU_ALPHA_MISC2) {
                    if (incomplete_command == CMD_GTODOT) {
                        incomplete_argtype = ARG_OTHER;
                        incomplete_maxdigits = 4;
                        incomplete_alpha = 0;
                        set_menu(MENULEVEL_COMMAND, MENU_IND);
                        redisplay();
                        return;
                    } else if (incomplete_argtype == ARG_NAMED) {
                        if (incomplete_command == CMD_ASSIGNa)
                            set_catalog_menu(CATSECT_TOP);
                        else
                            set_catalog_menu(CATSECT_VARS_ONLY);
                        if (incomplete_ind)
                            goto out_of_alpha;
                    } else if (incomplete_argtype == ARG_RVAR) {
                        if (incomplete_command == CMD_MVAR) {
                            pending_command = CMD_NULL;
                            finish_command_entry(false);
                        } else
                            goto out_of_alpha;
                    } else if (incomplete_argtype == ARG_MAT) {
                        if (vars_exist(0, 0, 1))
                            set_catalog_menu(CATSECT_MAT_ONLY);
                        else
                            set_menu(MENULEVEL_COMMAND, MENU_NONE);
                    } else if (incomplete_argtype == ARG_PRGM)
                        set_catalog_menu(CATSECT_PGM_ONLY);
                    else
                        goto out_of_alpha;
                    redisplay();
                } else if (mode_commandmenu == MENU_CATALOG
                        && ((catsect = get_cat_section()) == CATSECT_FCN
                                || catsect == CATSECT_PGM
                                || catsect == CATSECT_REAL
                                || catsect == CATSECT_CPX
                                || catsect == CATSECT_MAT)) {
                    set_catalog_menu(CATSECT_TOP);
                    redisplay();
                } else {
                    pending_command = CMD_NULL;
                    finish_command_entry(false);
                }
                return;
            }
            incomplete_length--;
            if (incomplete_length == 0) {
                if (incomplete_command == CMD_GTODOT) {
                    incomplete_argtype = ARG_OTHER;
                    incomplete_maxdigits = 4;
                    incomplete_alpha = 0;
                    set_menu(MENULEVEL_COMMAND, MENU_IND);
                    redisplay();
                    return;
                } else if (incomplete_argtype == ARG_NAMED) {
                    if (incomplete_command == CMD_ASSIGNa)
                        set_catalog_menu(CATSECT_TOP);
                    else
                        set_catalog_menu(CATSECT_VARS_ONLY);
                    if (incomplete_ind)
                        goto out_of_alpha;
                } else if (incomplete_argtype == ARG_RVAR) {
                    if (vars_exist(1, 0, 0))
                        set_catalog_menu(CATSECT_REAL_ONLY);
                } else if (incomplete_argtype == ARG_MAT) {
                    if (vars_exist(0, 0, 1))
                        set_catalog_menu(CATSECT_MAT_ONLY);
                } else if (incomplete_argtype == ARG_PRGM)
                    set_catalog_menu(CATSECT_PGM_ONLY);
                else {
                    out_of_alpha:
                    if (incomplete_ind
                            || incomplete_argtype != ARG_RVAR)
                        incomplete_alpha = 0;
                    if (incomplete_ind)
                        set_catalog_menu(CATSECT_REAL_ONLY);
                    else if (incomplete_argtype == ARG_VAR)
                        if (mode_appmenu == MENU_VARMENU)
                            set_menu(MENULEVEL_COMMAND, MENU_VARMENU);
                        else if (mode_appmenu == MENU_INTEG_PARAMS)
                            set_menu(MENULEVEL_COMMAND, MENU_INTEG_PARAMS);
                        else
                            set_catalog_menu(CATSECT_VARS_ONLY);
                    else if (incomplete_argtype == ARG_REAL)
                        if (mode_appmenu == MENU_VARMENU)
                            set_menu(MENULEVEL_COMMAND, MENU_VARMENU);
                        else if (mode_appmenu == MENU_INTEG_PARAMS)
                            set_menu(MENULEVEL_COMMAND, MENU_INTEG_PARAMS);
                        else
                            set_catalog_menu(CATSECT_REAL_ONLY);
                    else if (incomplete_argtype == ARG_LBL)
                        set_catalog_menu(CATSECT_PGM_ONLY);
                    else if (incomplete_command == CMD_GTODOT)
                        set_menu(MENULEVEL_COMMAND, MENU_IND);
                    else if (incomplete_command == CMD_LBL)
                        set_menu(MENULEVEL_COMMAND, MENU_ALPHA1);
                    else
                        set_menu(MENULEVEL_COMMAND, MENU_NONE);
                }
            }
            redisplay();
            return;
        }

        if (key == KEY_ENTER) {
            int i;
            if (incomplete_length == 0) {
                int catsect;
                if (mode_commandmenu == MENU_NONE
                        || (mode_commandmenu == MENU_CATALOG
                            && (catsect = get_cat_section()) != CATSECT_FCN
                            && catsect != CATSECT_PGM
                            && catsect != CATSECT_REAL
                            && catsect != CATSECT_CPX
                            && catsect != CATSECT_MAT)) {
                    set_menu(MENULEVEL_COMMAND, MENU_ALPHA1);
                    redisplay();
                    return;
                } else if ((incomplete_command != CMD_ASSIGNa
                        && incomplete_command != CMD_CLP
                        && incomplete_command != CMD_PRP)
                        || mode_commandmenu < MENU_ALPHA1
                        || mode_commandmenu > MENU_ALPHA_MISC2) {
                    squeak();
                    return;
                }
                /* ASSIGN, CLP, or PRP, alpha menu active, zero-length string:
                 * these are the only cases where an empty string is allowed
                 * as an argument. We fall through to the command completion
                 * code below.
                 */
            }

            pending_command = incomplete_command;

            if (!incomplete_ind
                    && (incomplete_command == CMD_GTO
                        || incomplete_command == CMD_XEQ
                        || incomplete_command == CMD_LBL
                        || (incomplete_command >= CMD_KEY1G
                            && incomplete_command <= CMD_KEY9X))
                    && incomplete_length == 1
                    && ((incomplete_str[0] >= 'A'
                            && incomplete_str[0] <= 'J')
                        || (incomplete_str[0] >= 'a'
                            && incomplete_str[0] <= 'e'))) {
                /* Display XEQ "A" briefly before changing to XEQ A */
                mode_command_entry = false;
                pending_command_arg.type = ARGTYPE_STR;
                pending_command_arg.length = 1;
                pending_command_arg.val.text[0] = incomplete_str[0];
                if (flags.f.prgm_mode) {
                    flags.f.prgm_mode = 0;
                    redisplay();
                    flags.f.prgm_mode = 1;
                    shell_delay(125);
                } else
                    redisplay();
                pending_command_arg.type = ARGTYPE_LCLBL;
                pending_command_arg.val.lclbl = incomplete_str[0];
                set_menu(MENULEVEL_COMMAND, MENU_NONE);
                finish_command_entry(false);
                return;
            } else {
                pending_command_arg.type =
                        incomplete_ind ? ARGTYPE_IND_STR : ARGTYPE_STR;
                pending_command_arg.length = incomplete_length;
                for (i = 0; i < incomplete_length; i++)
                    pending_command_arg.val.text[i] = incomplete_str[i];

                if (!incomplete_ind && incomplete_command == CMD_XEQ)
                    finish_xeq();
                else
                    finish_command_entry(true);
                return;
            }
        }

        squeak();
        return;
    }
}

void keydown_alpha_mode(int shift, int key) {
    int menukey;
    const menu_spec *m = NULL;
    char c;
    int command;

    if (key >= 1024 && key < 2048) {
        c = key - 1024;
        goto handle_char;
    }

    menukey = find_menu_key(key);
    if (menukey != -1) {
        const menu_item_spec *mi;
        m = menus + mode_alphamenu;
        mi = m->child + menukey;
        if (mi->menuid != MENU_NONE) {
            set_menu(MENULEVEL_ALPHA, mi->menuid);
            redisplay();
            return;
        }
        c = mi->title[0];
        if (shift && c >= 'A' && c <= 'Z')
            c += 'a' - 'A';

        handle_char:
        if (flags.f.prgm_mode) {
            if (!mode_alpha_entry)
                start_alpha_prgm_line();
            if (entered_string_length < 15)
                entered_string[entered_string_length++] = c;
        } else {
            if (!mode_alpha_entry) {
                reg_alpha_length = 0;
                flags.f.alpha_data_input = 1;
                mode_alpha_entry = true;
            }
            append_alpha_char(c);
            if (reg_alpha_length == 44)
                squeak();
        }
        if (core_settings.auto_repeat) {
            repeating = 2;
            repeating_key = c + 1024;
            repeating_shift = 0;
        }

        if (m != NULL)
            set_menu(MENULEVEL_ALPHA, m->parent);
        redisplay();
        return;
    }

    /* Handle keys that represent characters */
    if (!shift) {
        switch (key) {
            case KEY_0:   c = '0'; break;
            case KEY_1:   c = '1'; break;
            case KEY_2:   c = '2'; break;
            case KEY_3:   c = '3'; break;
            case KEY_4:   c = '4'; break;
            case KEY_5:   c = '5'; break;
            case KEY_6:   c = '6'; break;
            case KEY_7:   c = '7'; break;
            case KEY_8:   c = '8'; break;
            case KEY_9:   c = '9'; break;
            case KEY_DOT: c = '.'; break;
            case KEY_E:   c = 24;  break;
            case KEY_DIV: c = 0;   break;
            case KEY_MUL: c = 1;   break;
            case KEY_SUB: c = '-'; break;
            case KEY_ADD: c = '+'; break;
            default: goto nocharkey2;
        }
    } else {
        switch (key) {
            case KEY_RCL: c = '%'; break;
            case KEY_RDN: c = 7;   break;
            default: goto nocharkey2;
        }
    }
    if (flags.f.prgm_mode) {
        if (!mode_alpha_entry)
            start_alpha_prgm_line();
        if (entered_string_length < 15)
            entered_string[entered_string_length++] = c;
    } else {
        if (!mode_alpha_entry) {
            reg_alpha_length = 0;
            mode_alpha_entry = true;
            flags.f.alpha_data_input = 1;
        }
        append_alpha_char(c);
        if (reg_alpha_length == 44)
            squeak();
    }
    if (core_settings.auto_repeat) {
        repeating = 2;
        repeating_key = key;
        repeating_shift = shift;
    }
    redisplay();
    return;
    nocharkey2:
    /* End of handling keys that represent characters */

    if (!shift && (key == KEY_UP || key == KEY_DOWN)) {
        const menu_spec *m = menus + mode_alphamenu;
        int nextmenu = key == KEY_UP ? m->prev : m->next;
        if (nextmenu != MENU_NONE) {
            set_menu(MENULEVEL_ALPHA, nextmenu);
            redisplay();
        }
        return;
    }

    if (key == KEY_EXIT) {
        const menu_spec *m = menus + mode_alphamenu;
        set_menu(MENULEVEL_ALPHA, m->parent);
        if (mode_alphamenu == MENU_NONE) {
            if (mode_alpha_entry) {
                if (flags.f.prgm_mode)
                    finish_alpha_prgm_line();
                else if ((flags.f.trace_print || flags.f.normal_print)
                        && flags.f.printer_exists)
                    docmd_pra(NULL);
                mode_alpha_entry = false;
            }
            pending_command = CMD_CANCELLED;
        } else
            redisplay();
        return;
    }

    if (!shift && key == KEY_BSP) {
        if (flags.f.prgm_mode) {
            if (mode_alpha_entry) {
                if (entered_string_length > 0) {
                    if (core_settings.auto_repeat) {
                        repeating = 2;
                        repeating_key = key;
                        repeating_shift = shift;
                    }
                    entered_string_length--;
                } else
                    finish_alpha_prgm_line();
            } else {
                int4 line = pc2line(pc);
                if (line != 0
                        && (current_prgm != prgms_count - 1
                            || prgms[current_prgm].text[pc] != CMD_END)) {
                    delete_command(pc);
                    pc = line2pc(line - 1);
                }
                    prgm_highlight_row = 0;
                if (mode_alphamenu != MENU_ALPHA1
                        && mode_alphamenu != MENU_ALPHA2)
                    set_menu(MENULEVEL_ALPHA, menus[mode_alphamenu].parent);
            }
            redisplay();
            return;
        } else {
            if (mode_alpha_entry && reg_alpha_length > 0) {
                if (core_settings.auto_repeat) {
                    repeating = 2;
                    repeating_key = key;
                    repeating_shift = shift;
                }
                reg_alpha_length--;
                redisplay();
            } else
                pending_command = CMD_CLA;
            return;
        }
    }

    if (key == KEY_ENTER) {
        if (flags.f.prgm_mode) {
            if (mode_alpha_entry) {
                finish_alpha_prgm_line();
                set_menu(MENULEVEL_ALPHA, MENU_NONE);
            } else if (shift) {
                set_menu(MENULEVEL_ALPHA, MENU_NONE);
            } else {
                start_alpha_prgm_line();
                entered_string[0] = 127;
                entered_string_length = 1;
            }
        } else {
            if (shift || mode_alpha_entry) {
                if (mode_alpha_entry
                        && (flags.f.trace_print || flags.f.normal_print)
                        && flags.f.printer_exists)
                    docmd_pra(NULL);
                mode_alpha_entry = false;
                set_menu(MENULEVEL_ALPHA, MENU_NONE);
            } else
                mode_alpha_entry = true;
        }
        redisplay();
        return;
    }

    command = CMD_CANCELLED;
    if (!shift) {
        switch (key) {
            case KEY_STO: command = CMD_ASTO; break;
            case KEY_RCL: command = CMD_ARCL; break;
            case KEY_RUN: command = CMD_RUN; break;
            default: command = key >= 2048 ? key - 2048 : CMD_NONE; break;
        }
    } else {
        switch (key) {
            case KEY_CHS: set_plainmenu(MENU_MODES1); break;
            case KEY_E: set_plainmenu(MENU_DISP); break;
            case KEY_BSP: set_plainmenu(MENU_CLEAR1); break;
            case KEY_7: set_solve_integ(1); break;
            case KEY_8: set_solve_integ(0); break;
            case KEY_9: set_menu(MENULEVEL_APP, MENU_MATRIX1); break;
            case KEY_DIV: set_menu(MENULEVEL_APP, MENU_STAT1); break;
            case KEY_DOWN: command = CMD_SST; break;
            case KEY_4: set_menu(MENULEVEL_APP, MENU_BASE);
                        if (mode_appmenu == MENU_BASE) {
                            set_appmenu_exitcallback(2);
                            baseapp = 1;
                        }
                        break;
            case KEY_5: set_plainmenu(MENU_CONVERT1); break;
            case KEY_6: set_plainmenu(MENU_FLAGS); break;
            case KEY_MUL: set_plainmenu(MENU_PROB); break;
            case KEY_2: set_plainmenu(MENU_CUSTOM1); break;
            case KEY_3: set_plainmenu(MENU_PGM_FCN1); break;
            case KEY_SUB: set_plainmenu(MENU_PRINT1); break;
            case KEY_0: set_plainmenu(MENU_TOP_FCN); break;
            case KEY_DOT: show();
                            pending_command = CMD_LINGER1;
                            shell_request_timeout3(2000);
                            return;
            case KEY_ADD: set_plainmenu(MENU_CATALOG); break;
            default: command = key >= 2048 ? key - 2048 : CMD_NONE; break;
        }
    }

    if (command == CMD_NONE) {
        squeak();
        return;
    }
    if (mode_alpha_entry) {
        if (flags.f.prgm_mode)
            finish_alpha_prgm_line();
        else if ((flags.f.trace_print || flags.f.normal_print)
                && flags.f.printer_exists)
            docmd_pra(NULL);
        mode_alpha_entry = false;
    }

    if (command == CMD_CANCELLED) {
        /* plainmenu or appmenu switch */
        set_menu(MENULEVEL_ALPHA, MENU_NONE);
        redisplay();
        return;
    } else
        do_interactive(command);
}

void keydown_normal_mode(int shift, int key) {
    int command;

    if (is_number_key(shift, key)) {
        /* Entering number entry mode */
        if (deferred_print)
            print_command(CMD_NULL, NULL);
        cmdline_length = 0;
        if (get_front_menu() != NULL)
            cmdline_row = 0;
        else
            cmdline_row = 1;
        mode_number_entry = true;
        if (flags.f.prgm_mode) {
            if (pc == -1)
                pc = 0;
            else if (prgms[current_prgm].text[pc] != CMD_END)
                pc += get_command_length(current_prgm, pc);
                prgm_highlight_row = 1;
                if (cmdline_row == 1)
                    display_prgm_line(0, -1);
        } else {
            if (!flags.f.stack_lift_disable) {
                free_vartype(reg_t);
                reg_t = reg_z;
                reg_z = reg_y;
                reg_y = dup_vartype(reg_x);
            } else
                flags.f.stack_lift_disable = 0;
            flags.f.numeric_data_input = 1;
            mode_varmenu = false;
            if (cmdline_row == 1)
                display_y(0);
            else
                /* Force repaint of menu; it could be hidden due to a recent
                 * two-line AVIEW command */
                redisplay();
        }
        keydown_number_entry(shift, key);
        return;
    }

    if (flags.f.prgm_mode && !shift && key == KEY_BSP) {
        int4 line = pc2line(pc);
        if (line == 0)
            return;
        if (current_prgm != prgms_count - 1
                || prgms[current_prgm].text[pc] != CMD_END)
            delete_command(pc);
        pc = line2pc(line - 1);
        prgm_highlight_row = 0;
        redisplay();
        return;
    }

    if ((mode_appmenu != MENU_NONE
                || mode_plainmenu != MENU_NONE
                || mode_transientmenu != MENU_NONE)
            && mode_alphamenu == MENU_NONE
            && mode_commandmenu == MENU_NONE) {
        int menukey = find_menu_key(key);
        int menu, level;
        if (mode_transientmenu != MENU_NONE) {
            menu = mode_transientmenu;
            level = MENULEVEL_TRANSIENT;
        } else if (mode_plainmenu != MENU_NONE) {
            menu = mode_plainmenu;
            level = MENULEVEL_PLAIN;
        } else {
            menu = mode_appmenu;
            level = MENULEVEL_APP;
        }

        if (menu == MENU_PROGRAMMABLE) {
            int keynum;
            if (menukey != -1)
                keynum = menukey + 1;
            else if (!shift) {
                switch (key) {
                    case KEY_UP: keynum = 7; break;
                    case KEY_DOWN: keynum = 8; break;
                    case KEY_EXIT: keynum = 9; break;
                    default: goto notprogmenu;
                }
            } else
                goto notprogmenu;
            do_prgm_menu_key(keynum);
            return;
            notprogmenu:;
        }

        if (menu == MENU_VARMENU) {
            if (menukey != -1) {
                if (varmenu_labellength[menukey] == 0) {
                    pending_command = CMD_NULL;
                } else if (shift && !flags.f.prgm_mode) {
                    view(varmenu_labeltext[menukey],
                         varmenu_labellength[menukey]);
                } else {
                    int i;
                    pending_command_arg.type = ARGTYPE_STR;
                    pending_command_arg.length =
                                        varmenu_labellength[menukey];
                    for (i = 0; i < pending_command_arg.length; i++)
                        pending_command_arg.val.text[i] =
                                            varmenu_labeltext[menukey][i];
                    if (flags.f.prgm_mode) {
                        pending_command = shift ? CMD_VIEW : CMD_STO;
                        store_command_after(&pc, pending_command,
                                                        &pending_command_arg);
                        prgm_highlight_row = 1;
                        pending_command = CMD_NONE;
                        redisplay();
                    } else {
                        switch (varmenu_role) {
                            case 0: /* Plain ol' VARMENU */
                                pending_command =
                                    mode_varmenu ? CMD_VMEXEC : CMD_VMSTO;
                                break;
                            case 1: /* Solver */
                                pending_command =
                                    mode_varmenu ? CMD_VMSOLVE : CMD_VMSTO2;
                                break;
                            case 2: /* Integrator */
                                if (mode_varmenu) {
                                    set_integ_var(varmenu_labeltext[menukey],
                                                  varmenu_labellength[menukey]);
                                    set_menu(MENULEVEL_APP, MENU_INTEG_PARAMS);
                                    set_appmenu_exitcallback(5);
                                    redisplay();
                                    return;
                                } else
                                    pending_command = CMD_VMSTO;
                        }
                    }
                }
                return;
            }
            if (!shift && key == KEY_UP) {
                if (varmenu_rows > 1) {
                    if (--varmenu_row < 0)
                        varmenu_row = varmenu_rows - 1;
                    pending_command = CMD_CANCELLED;
                }
                return;
            }
            if (!shift && key == KEY_DOWN) {
                if (varmenu_rows > 1) {
                    if (++varmenu_row >= varmenu_rows)
                        varmenu_row = 0;
                    pending_command = CMD_CANCELLED;
                }
                return;
            }
            if (key == KEY_EXIT) {
                set_menu(MENULEVEL_APP, MENU_NONE);
                pending_command = CMD_CANCELLED;
                return;
            }
        }

        if (menukey != -1) {
            if (menu == MENU_CUSTOM1
                    || menu == MENU_CUSTOM2
                    || menu == MENU_CUSTOM3) {
                if (flags.f.local_label) {
                    if (menukey == 5) {
                        int cmd = shift ? CMD_GTO : CMD_XEQ;
                        do_interactive(cmd);
                        return;
                    } else {
                        pending_command = CMD_XEQ;
                        pending_command_arg.type = ARGTYPE_LCLBL;
                        if (menu == MENU_CUSTOM1)
                            pending_command_arg.val.lclbl =
                                shift ? 'a' + menukey
                                            : 'A' + menukey;
                        else
                            pending_command_arg.val.lclbl = 'F' + menukey;
                        if (flags.f.prgm_mode) {
                            store_command_after(&pc, pending_command,
                                                        &pending_command_arg);
                            prgm_highlight_row = 1;
                            pending_command = CMD_NONE;
                            set_menu(MENULEVEL_COMMAND, MENU_NONE);
                            redisplay();
                        }
                        return;
                    }
                } else {
                    int keynum, length;
                    char name[7];
                    keynum = menukey + 6 * (menu - MENU_CUSTOM1) + 1;
                    get_custom_key(keynum, name, &length);
                    if (length == 0)
                        squeak();
                    else {
                        int dummyprgm;
                        int4 dummypc;
                        int i;
                        pending_command_arg.type = ARGTYPE_STR;
                        pending_command_arg.length = length;
                        for (i = 0; i < length; i++)
                            pending_command_arg.val.text[i] = name[i];
                        if (find_global_label(&pending_command_arg,
                                                &dummyprgm, &dummypc))
                            pending_command = CMD_XEQ;
                        else if (lookup_var(name, length) != -1)
                            pending_command = CMD_RCL;
                        else {
                            int cmd = find_builtin(name, length, true);
                            if (cmd == -1)
                                pending_command = CMD_XEQ;
                            else if (cmd == CMD_CLALLa) {
                                mode_clall = true;
                                set_menu(MENULEVEL_ALPHA, MENU_NONE);
                                pending_command = CMD_NONE;
                                redisplay();
                                return;
                            } else if (cmd == CMD_CLV || cmd == CMD_PRV) {
                                if (!flags.f.prgm_mode && vars_count == 0) {
                                    display_error(ERR_NO_VARIABLES, 0);
                                    pending_command = CMD_NONE;
                                    redisplay();
                                    return;
                                }
                                pending_command = CMD_NONE;
                                do_interactive(cmd);
                                return;
                            } else if ((cmd == CMD_SST || cmd == CMD_SST_UP || cmd == CMD_SST_RT)
                                    && flags.f.prgm_mode) {
                                sst();
                                pending_command = CMD_NONE;
                                redisplay();
                                repeating = 1;
                                repeating_shift = 1;
                                repeating_key = KEY_DOWN;
                                return;
                            } else if (cmd == CMD_BST) {
                                bst();
                                if (!flags.f.prgm_mode) {
                                    flags.f.prgm_mode = 1;
                                    redisplay();
                                    flags.f.prgm_mode = 0;
                                    pending_command = CMD_CANCELLED;
                                } else {
                                    redisplay();
                                    pending_command = CMD_NONE;
                                }
                                repeating = 1;
                                repeating_shift = 1;
                                repeating_key = KEY_UP;
                                return;
                            } else if (cmdlist(cmd)->argtype == ARG_NONE) {
                                pending_command = cmd;
                                pending_command_arg.type = ARGTYPE_NONE;
                            } else {
                                pending_command = CMD_NONE;
                                do_interactive(cmd);
                                return;
                            }
                        }
                        goto send_it_off;
                    }
                    return;
                }
            } else if (menu == MENU_CATALOG) {
                int catsect;
                catsect = get_cat_section();
                if (catsect == CATSECT_TOP) {
                    switch (menukey) {
                        case 0:
                            set_cat_section(CATSECT_FCN);
                            move_cat_row(0);
                            break;
                        case 1:
                            set_cat_section(CATSECT_PGM);
                            move_cat_row(0);
                            break;
                        case 2:
                            if (vars_exist(1, 0, 0)) {
                                set_cat_section(CATSECT_REAL);
                                move_cat_row(0);
                            } else {
                                display_error(ERR_NO_REAL_VARIABLES, 0);
                                flush_display();
                                return;
                            }
                            break;
                        case 3:
                            if (vars_exist(0, 1, 0)) {
                                set_cat_section(CATSECT_CPX);
                                move_cat_row(0);
                            } else {
                                display_error(ERR_NO_COMPLEX_VARIABLES, 0);
                                flush_display();
                                return;
                            }
                            break;
                        case 4:
                            if (vars_exist(0, 0, 1)) {
                                set_cat_section(CATSECT_MAT);
                                move_cat_row(0);
                            } else {
                                display_error(ERR_NO_MATRIX_VARIABLES, 0);
                                flush_display();
                                return;
                            }
                            break;
                        case 5:
                            display_mem();
                            pending_command = CMD_LINGER1;
                            shell_request_timeout3(2000);
                            return;
                    }
                    redisplay();
                    return;
                } else if (catsect == CATSECT_PGM
                            || catsect == CATSECT_PGM_ONLY) {
                    int labelindex = get_cat_item(menukey);
                    if (labelindex == -1) {
                        pending_command = CMD_NULL;
                        return;
                    }
                    if (flags.f.prgm_mode
                                    && labels[labelindex].length == 0) {
                        display_error(ERR_RESTRICTED_OPERATION, 0);
                        flush_display();
                        pending_command = CMD_NONE;
                        return;
                    }
                    pending_command = CMD_XEQ;
                    pending_command_arg.type = ARGTYPE_LBLINDEX;
                    pending_command_arg.val.num = labelindex;
                    xeq_invisible = 1;
                    if (!flags.f.prgm_mode
                            && (level == MENULEVEL_TRANSIENT
                                || !mode_plainmenu_sticky)) {
                        if (level == MENULEVEL_PLAIN) {
                            int row = get_cat_row();
                            set_menu(MENULEVEL_PLAIN, MENU_NONE);
                            set_menu(MENULEVEL_TRANSIENT, MENU_CATALOG);
                            set_cat_section(catsect);
                            set_cat_row(row);
                        }
                        remove_program_catalog = 1;
                    }
                } else if (catsect == CATSECT_PGM_SOLVE
                        || catsect == CATSECT_PGM_INTEG) {
                    int labelindex = get_cat_item(menukey);
                    int i;
                    if (labelindex == -1) {
                        pending_command = CMD_NULL;
                        return;
                    }
                    if (catsect == CATSECT_PGM_SOLVE)
                        pending_command = flags.f.prgm_mode ? CMD_PGMSLV
                                                            : CMD_PGMSLVi;
                    else
                        pending_command = flags.f.prgm_mode ? CMD_PGMINT
                                                            : CMD_PGMINTi;
                    pending_command_arg.type = ARGTYPE_STR;
                    pending_command_arg.length = labels[labelindex].length;
                    for (i = 0; i < pending_command_arg.length; i++)
                        pending_command_arg.val.text[i] =
                                                labels[labelindex].name[i];
                } else if (catsect == CATSECT_FCN) {
                    int cmd = get_cat_item(menukey);
                    if (cmd == -1)
                        if (flags.f.prgm_mode) {
                            pending_command = CMD_NULL;
                            return;
                        } else
                            cmd = CMD_NULL;
                    if (level == MENULEVEL_TRANSIENT
                            || !mode_plainmenu_sticky)
                        set_menu(level, MENU_NONE);
                    do_interactive(cmd);
                    return;
                } else {
                    int varindex = get_cat_item(menukey);
                    int i;
                    if (varindex == -1) {
                        pending_command = CMD_NULL;
                        return;
                    }
                    pending_command = CMD_RCL;
                    pending_command_arg.type = ARGTYPE_STR;
                    pending_command_arg.length = vars[varindex].length;
                    for (i = 0; i < pending_command_arg.length; i++)
                        pending_command_arg.val.text[i] =
                                                vars[varindex].name[i];
                    if (level == MENULEVEL_TRANSIENT
                            || !mode_plainmenu_sticky)
                        set_menu(level, MENU_NONE);
                }
                send_it_off:
                if (flags.f.prgm_mode &&
                        (cmdlist(pending_command)->flags & FLAG_IMMED) == 0) {
                    store_command_after(&pc, pending_command,
                                            &pending_command_arg);
                    if (pending_command == CMD_END)
                        /* current_prgm was already incremented by store_command() */
                        pc = 0;
                    prgm_highlight_row = 1;
                    pending_command = CMD_NONE;
                    if (level == MENULEVEL_TRANSIENT
                            || (level == MENULEVEL_PLAIN
                                && !mode_plainmenu_sticky))
                        set_menu(level, MENU_NONE);
                    redisplay();
                }
                return;
            } else if (menu == MENU_INTEG_PARAMS) {
                if (menukey <= 2) {
                    const char *name;
                    int length;
                    switch (menukey) {
                        case 0: name = "LLIM"; length = 4; break;
                        case 1: name = "ULIM"; length = 4; break;
                        case 2: name = "ACC";  length = 3; break;
                    }
                    if (shift && !flags.f.prgm_mode)
                        view(name, length);
                    else {
                        int i;
                        pending_command_arg.type = ARGTYPE_STR;
                        pending_command_arg.length = length;
                        for (i = 0; i < length; i++)
                            pending_command_arg.val.text[i] = name[i];
                        if (flags.f.prgm_mode) {
                            pending_command = shift ? CMD_VIEW : CMD_STO;
                            store_command_after(&pc, pending_command,
                                                    &pending_command_arg);
                            prgm_highlight_row = 1;
                            pending_command = CMD_NONE;
                            redisplay();
                        } else {
                            pending_command = CMD_VMSTO;
                        }
                    }
                } else if (menukey == 5) {
                    int tmp;
                    pending_command_arg.type = ARGTYPE_STR;
                    get_integ_var(pending_command_arg.val.text, &tmp);
                    pending_command_arg.length = tmp;
                    pending_command = CMD_INTEG;
                } else
                    pending_command = CMD_NULL;
                return;
            } else {
                const menu_item_spec *mi = menus[menu].child + menukey;
                int cmd_id = mi->menuid;
                if ((cmd_id & 0x3000) == 0) {
                    set_menu(level, cmd_id);
                    redisplay();
                    return;
                }
                if (menu == MENU_TOP_FCN && shift) {
                    switch (menukey) {
                        case 0: cmd_id = CMD_SIGMASUB; break;
                        case 1: cmd_id = CMD_Y_POW_X; break;
                        case 2: cmd_id = CMD_SQUARE; break;
                        case 3: cmd_id = CMD_10_POW_X; break;
                        case 4: cmd_id = CMD_E_POW_X; break;
                        case 5: cmd_id = CMD_GTO; break;
                    }
                } else if (menu == MENU_PGM_FCN1 && menukey == 5 && shift)
                    cmd_id = CMD_GTO;
                else if (menu == MENU_STAT1 && menukey == 0 && shift)
                    cmd_id = CMD_SIGMASUB;
                else
                    cmd_id &= 0xfff;
                if (level == MENULEVEL_TRANSIENT
                        || (level == MENULEVEL_PLAIN && !mode_plainmenu_sticky))
                    set_menu(level, MENU_NONE);
                if (cmd_id == CMD_NULL && flags.f.prgm_mode)
                    pending_command = CMD_NULL;
                else
                    do_interactive(cmd_id);
                return;
            }
        }

        if (!shift && (key == KEY_UP || key == KEY_DOWN)) {
            if (menu == MENU_CATALOG) {
                move_cat_row(key == KEY_UP ? -1 : 1);
                redisplay();
            } else if (flags.f.local_label
                    && (menu == MENU_CUSTOM1
                        || menu == MENU_CUSTOM2
                        || menu == MENU_CUSTOM3)) {
                set_menu(level, menu == MENU_CUSTOM1
                            ? MENU_CUSTOM2 : MENU_CUSTOM1);
                redisplay();
            } else {
                const menu_spec *m = menus + menu;
                int nextmenu = key == KEY_UP ? m->prev : m->next;
                if (nextmenu != MENU_NONE) {
                    set_menu(level, nextmenu);
                    redisplay();
                }
            }
            return;
        }
        
        if (key == KEY_EXIT) {
            if (menu == MENU_CATALOG) {
                int catsect = get_cat_section();
                if (catsect == CATSECT_FCN
                        || catsect == CATSECT_PGM
                        || catsect == CATSECT_REAL
                        || catsect == CATSECT_CPX
                        || catsect == CATSECT_MAT)
                    set_cat_section(CATSECT_TOP);
                else
                    set_menu(level, MENU_NONE);
            } else {
                const menu_spec *m = menus + menu;
                set_menu(level, m->parent);
            }
            pending_command = CMD_CANCELLED;
            return;
        }
    }

    if (shift && key == KEY_ENTER) {
        if (deferred_print)
            print_command(CMD_NULL, NULL);
        mode_alpha_entry = false;
        set_menu(MENULEVEL_ALPHA, MENU_ALPHA1);
        redisplay();
        return;
    }

    if (key == KEY_EXIT && flags.f.prgm_mode) {
        flags.f.prgm_mode = 0;
        pending_command = CMD_CANCELLED;
        return;
    }

    if (key == KEY_UP) {
        /* Either shift == 1, or there is no menu; this
         * means BST. This requires special care because it's
         * one of the rare cases of auto-repeat.
         * TODO.
         */
        clear_all_rtns();
        squeak();
        return;
    }

    if (!shift) {
        switch (key) {
            case KEY_SIGMA: command = CMD_SIGMAADD; break;
            case KEY_INV: command = CMD_INV; break;
            case KEY_SQRT: command = CMD_SQRT; break;
            case KEY_LOG: command = CMD_LOG; break;
            case KEY_LN: command = CMD_LN; break;
            case KEY_XEQ: command = CMD_XEQ; break;
            case KEY_STO: command = CMD_STO; break;
            case KEY_RCL: command = CMD_RCL; break;
            case KEY_RDN: command = CMD_RDN; break;
            case KEY_SIN: command = CMD_SIN; break;
            case KEY_COS: command = CMD_COS; break;
            case KEY_TAN: command = CMD_TAN; break;
            case KEY_ENTER: command = CMD_ENTER; break;
            case KEY_SWAP: command = CMD_SWAP; break;
            case KEY_CHS: command = basekeys() ? CMD_BASECHS : CMD_CHS; break;
            case KEY_BSP: command = CMD_CLX; break;
            case KEY_DIV: command = basekeys() ? CMD_BASEDIV : CMD_DIV; break;
            case KEY_DOWN: command = CMD_SST; break;
            case KEY_MUL: command = basekeys() ? CMD_BASEMUL : CMD_MUL; break;
            case KEY_SUB: command = basekeys() ? CMD_BASESUB : CMD_SUB; break;
            case KEY_EXIT:
                input_length = 0;
                pending_command = CMD_CANCELLED;
                return;
            case KEY_RUN: command = CMD_RUN; break;
            case KEY_ADD: command = basekeys() ? CMD_BASEADD : CMD_ADD; break;
            default: command = key >= 2048 ? key - 2048 : CMD_NONE; break;
        }
    } else {
        switch (key) {
            case KEY_SIGMA: command = CMD_SIGMASUB; break;
            case KEY_INV: command = CMD_Y_POW_X; break;
            case KEY_SQRT: command = CMD_SQUARE; break;
            case KEY_LOG: command = CMD_10_POW_X; break;
            case KEY_LN: command = CMD_E_POW_X; break;
            case KEY_XEQ: command = CMD_GTO; break;
            case KEY_STO: command = CMD_COMPLEX; break;
            case KEY_RCL: command = CMD_PERCENT; break;
            case KEY_RDN: command = CMD_PI; break;
            case KEY_SIN: command = CMD_ASIN; break;
            case KEY_COS: command = CMD_ACOS; break;
            case KEY_TAN: command = CMD_ATAN; break;
            case KEY_SWAP: command = CMD_LASTX; break;
            case KEY_CHS: set_plainmenu(MENU_MODES1); return;
            case KEY_E: set_plainmenu(MENU_DISP); return;
            case KEY_BSP: set_plainmenu(MENU_CLEAR1); return;
            case KEY_7: set_solve_integ(1); redisplay(); return;
            case KEY_8: set_solve_integ(0); redisplay(); return;
            case KEY_9: set_menu(MENULEVEL_APP, MENU_MATRIX1);
                        redisplay();
                        return;
            case KEY_DIV: set_menu(MENULEVEL_APP, MENU_STAT1);
                          redisplay();
                          return;
            case KEY_DOWN: command = CMD_SST; break;
            case KEY_4: set_menu(MENULEVEL_APP, MENU_BASE);
                        if (mode_appmenu == MENU_BASE) {
                            set_appmenu_exitcallback(2);
                            baseapp = 1;
                            redisplay();
                        }
                        return;
            case KEY_5: set_plainmenu(MENU_CONVERT1); return;
            case KEY_6: set_plainmenu(MENU_FLAGS); return;
            case KEY_MUL: set_plainmenu(MENU_PROB); return;
            case KEY_1: command = CMD_ASSIGNa; break;
            case KEY_2: set_plainmenu(MENU_CUSTOM1); return;
            case KEY_3: set_plainmenu(MENU_PGM_FCN1); return;
            case KEY_SUB: set_plainmenu(MENU_PRINT1); return;
            case KEY_DOT: show();
                            pending_command = CMD_LINGER1;
                            shell_request_timeout3(2000);
                            return;
            case KEY_0: set_plainmenu(MENU_TOP_FCN); return;
            case KEY_ADD: set_plainmenu(MENU_CATALOG); return;
            default: command = key >= 2048 ? key - 2048 : CMD_NONE; break;
        }
    }

    if (command == CMD_NONE)
        return;
    else
        do_interactive(command);
}
