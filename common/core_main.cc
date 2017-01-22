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

#include "core_main.h"
#include "core_commands2.h"
#include "core_commands4.h"
#include "core_display.h"
#include "core_helpers.h"
#include "core_keydown.h"
#include "core_math1.h"
#include "core_sto_rcl.h"
#include "core_tables.h"
#include "core_variables.h"
#include "shell.h"
#include "shell_spool.h"


static void set_shift(bool state) {
    if (mode_shift != state) {
        mode_shift = state;
        shell_annunciators(-1, state, -1, -1, -1, -1);
    }
}

static void continue_running();
static void stop_interruptible();
static int handle_error(int error);

int repeating = 0;
int repeating_shift;
int repeating_key;

static int4 oldpc;

core_settings_struct core_settings;

void core_init(int read_saved_state, int4 version) {

    /* Possible values for read_saved_state:
     * 0: state file not present (Memory Clear)
     * 1: state file present and looks OK so far
     * 2: state file present but not OK (State File Corrupt)
     */

    phloat_init();
    if (read_saved_state != 1 || !load_state(version))
        hard_reset(read_saved_state != 0);

    repaint_display();
    shell_annunciators(mode_updown,
                       mode_shift,
                       0 /*print*/,
                       mode_running,
                       flags.f.grad,
                       flags.f.rad || flags.f.grad);
}

#if defined(IPHONE) || defined(ANDROID)
void core_enter_background() {
    if (mode_interruptible != NULL)
        stop_interruptible();
    set_running(false);
    save_state();
}
#endif

void core_quit() {
#ifndef ANDROID
    // In Android, core_enter_background() is always called
    // before core_quit().
    // TODO: Does that apply to the iPhone verson as well?
    if (mode_interruptible != NULL)
        stop_interruptible();
    save_state();
#endif
    free_vartype(reg_x);
    free_vartype(reg_y);
    free_vartype(reg_z);
    free_vartype(reg_t);
    free_vartype(reg_lastx);
    purge_all_vars();
    clear_all_prgms();
    if (vars != NULL)
        free(vars);
    clean_vartype_pools();

#ifdef ANDROID
    reinitialize_globals();
#endif
}

void core_repaint_display() {
    repaint_display();
}

int core_menu() {
    return mode_clall || get_front_menu() != NULL;
}

int core_alpha_menu() {
    int *menu = get_front_menu();
    return menu != NULL && *menu >= MENU_ALPHA1 && *menu <= MENU_ALPHA_MISC2;
}

int core_hex_menu() {
    int *menu = get_front_menu();
    return menu != NULL && *menu == MENU_BASE_A_THRU_F;
}

int core_keydown(int key, int *enqueued, int *repeat) {

    *enqueued = 0;
    *repeat = 0;

    if (key != 0)
        no_keystrokes_yet = false;

    if (key == KEY_SHIFT) {
        set_shift(!mode_shift);
        return (mode_running && !mode_getkey && !mode_pause) || keybuf_head != keybuf_tail;
    }

    if (mode_pause) {
        mode_pause = false;
        set_running(false);
        if (!mode_shift && (key == KEY_RUN || key == KEY_EXIT)) {
            redisplay();
            return 0;
        }
    }

    if (mode_interruptible != NULL) {
        /* We're in the middle of an interruptible function
         * (e.g., INVRT, PRP); queue up any keystrokes and invoke
         * the appropriate callback to keep the funtion moving along
         */
        int error, keep_running;
        if (key != 0) {
            /* Enqueue... */
            *enqueued = 1;
            if (key == KEY_EXIT ||
                    (mode_stoppable && !mode_shift && key == KEY_RUN)) {
                keybuf_tail = keybuf_head;
                stop_interruptible();
                return 0;
            } else {
                if (((keybuf_head + 1) & 15) != keybuf_tail) {
                    if (mode_shift)
                        key = -key;
                    keybuf[keybuf_head] = key;
                    keybuf_head = (keybuf_head + 1) & 15;
                }
            }
            set_shift(false);
        }
        error = mode_interruptible(0);
        if (error == ERR_INTERRUPTIBLE)
            /* Still not done */
            return 1;
        mode_interruptible = NULL;
        keep_running = handle_error(error);
        if (mode_running) {
            if (!keep_running)
                set_running(false);
        } else {
            shell_annunciators(-1, -1, -1, 0, -1, -1);
            pending_command = CMD_NONE;
        }
        if (mode_running || keybuf_tail != keybuf_head)
            return 1;
        else {
            redisplay();
            return 0;
        }
    }

    if (mode_running && !mode_getkey) {
        /* We're running; queue up any keystrokes and invoke
         * continue_running() to keep the program moving along
         */
        if (key != 0) {
            if (key == KEY_EXIT) {
                keybuf_tail = keybuf_head;
                set_shift(false);
                set_running(false);
                pending_command = CMD_CANCELLED;
                return 0;
            }
            /* Enqueue... */
            *enqueued = 1;
            if (!mode_shift && key == KEY_RUN) {
                keybuf_tail = keybuf_head;
                set_running(false);
                redisplay();
                return 0;
            }
            if (((keybuf_head + 1) & 15) != keybuf_tail) {
                if (mode_shift)
                    key = -key;
                keybuf[keybuf_head] = key;
                keybuf_head = (keybuf_head + 1) & 15;
            }
            set_shift(false);
        }
        continue_running();
        if ((mode_running && !mode_getkey && !mode_pause) || keybuf_tail != keybuf_head)
            return 1;
        else {
            if (mode_getkey)
                /* Technically, the program is still running, but we turn
                 * off the 'running' annunciator so that the user has some
                 * cue that they may now type. (Actually, I'm doing it
                 * purely because the HP-42S does it, too!)
                 */
                shell_annunciators(-1, -1, -1, 0, -1, -1);
            else if (!mode_pause)
                redisplay();
            return 0;
        }
    }

    /* If we get here, mode_running must be false.
     * or a program is running but hanging in GETKEY;
     */

    if (keybuf_tail != keybuf_head) {
        /* We're not running, or a program is waiting in GETKEY;
         * feed queued-up keystroke to keydown()
         */
        int oldshift = 0;
        int oldkey = keybuf[keybuf_tail];
        if (oldkey < 0) {
            oldkey = -oldkey;
            oldshift = 1;
        }
        keybuf_tail = (keybuf_tail + 1) & 15;
        /* If we're in GETKEY mode, the 'running' annunciator is off;
         * see the code circa 30 lines back.
         * We now turn it back on since program execution resumes.
         */
        if (mode_getkey && mode_running)
            shell_annunciators(-1, -1, -1, 1, -1, -1);
        /* Feed the dequeued key to the usual suspects */
        keydown(oldshift, oldkey);
        core_keyup();
        /* We've just de-queued a key; may have to enqueue
         * one as well, if the user is actually managing to
         * type while we're unwinding the keyboard buffer
         */
        if (key != 0) {
            if (((keybuf_head + 1) & 15) != keybuf_tail) {
                if (mode_shift)
                    key = -key;
                keybuf[keybuf_head] = key;
                keybuf_head = (keybuf_head + 1) & 15;
            }
            set_shift(false);
        }
        return (mode_running && !mode_getkey) || keybuf_head != keybuf_tail;
    }

    /* No program is running, or it is running but waiting for a
     * keystroke (GETKEY); handle any new keystroke that has just come in
     */
    if (key != 0) {
        int shift = mode_shift;
        set_shift(false);
        if (mode_getkey && mode_running)
            shell_annunciators(-1, -1, -1, 1, -1, -1);
        keydown(shift, key);
        if (repeating != 0) {
            *repeat = repeating;
            repeating = 0;
        }
        return mode_running && !mode_getkey;
    }

    /* Nothing going on at all! */
    return 0;
}

int core_repeat() {
    keydown(repeating_shift, repeating_key);
    int rpt = repeating;
    repeating = 0;
    return rpt;
}

void core_keytimeout1() {
    if (pending_command == CMD_LINGER1 || pending_command == CMD_LINGER2)
        return;
    if (pending_command == CMD_RUN || pending_command == CMD_SST) {
        int saved_pending_command = pending_command;
        if (pc == -1)
            pc = 0;
        prgm_highlight_row = 1;
        flags.f.prgm_mode = 2; /* HACK - magic value to tell redisplay() */
                               /* not to suppress option menu highlights */
        pending_command = CMD_NONE;
        redisplay();
        flags.f.prgm_mode = 0;
        pending_command = saved_pending_command;
    } else if (pending_command != CMD_NONE && pending_command != CMD_CANCELLED
            && (cmdlist(pending_command)->flags & FLAG_NO_SHOW) == 0) {
        display_command(0);
        /* If the program catalog was left up by GTO or XEQ,
         * don't paint over it */
        if (mode_transientmenu == MENU_NONE || pending_command == CMD_NULL)
            display_x(1);
        flush_display();
    }
}

void core_keytimeout2() {
    if (pending_command == CMD_LINGER1 || pending_command == CMD_LINGER2)
        return;
    remove_program_catalog = 0;
    if (pending_command != CMD_NONE && pending_command != CMD_CANCELLED
            && (cmdlist(pending_command)->flags & FLAG_NO_SHOW) == 0) {
        clear_row(0);
        draw_string(0, 0, "NULL", 4);
        display_x(1);
        flush_display();
        pending_command = CMD_CANCELLED;
    }
}

bool core_timeout3(int repaint) {
    if (mode_pause) {
        if (repaint) {
            /* The PSE ended normally */
            mode_pause = false;
            if (mode_goose >= 0)
                mode_goose = -1 - mode_goose;
        }
        return true;
    }
    /* Remove the output of SHOW, MEM, or shift-VARMENU from the display */
    if (pending_command == CMD_LINGER1)
        pending_command = CMD_CANCELLED;
    else if (pending_command == CMD_LINGER2) {
        flags.f.message = 0;
        flags.f.two_line_message = 0;
        pending_command = CMD_NONE;
        if (repaint)
            redisplay();
    }
    return false;
}

int core_keyup() {
    if (mode_pause) {
        /* The only way this can happen is if they key in question was Shift */
        return 0;
    }

    int error = ERR_NONE;

    if (pending_command == CMD_LINGER1 || pending_command == CMD_LINGER2) {
        pending_command = CMD_LINGER2;
        return mode_running || keybuf_head != keybuf_tail;
    }

    if (pending_command == CMD_SILENT_OFF) {
#ifdef IPHONE
        if (off_enabled())
            shell_powerdown();
        else {
            set_running(false);
            squeak();
        }
#else
        shell_powerdown();
#endif
        pending_command = CMD_NONE;
        return 0;
    }

    if (pending_command == CMD_NONE)
        return mode_running || keybuf_head != keybuf_tail;

    if (remove_program_catalog) {
        if (mode_transientmenu == MENU_CATALOG)
            set_menu(MENULEVEL_TRANSIENT, MENU_NONE);
        else if (mode_plainmenu == MENU_CATALOG)
            set_menu(MENULEVEL_PLAIN, MENU_NONE);
        remove_program_catalog = 0;
    }

    if (pending_command == CMD_CANCELLED || pending_command == CMD_NULL) {
        pending_command = CMD_NONE;
        redisplay();
        return mode_running || keybuf_head != keybuf_tail;
    }

    mode_varmenu = pending_command == CMD_VMSTO
                    || pending_command == CMD_VMSTO2
                    || pending_command == CMD_VMSOLVE
                    || pending_command == CMD_VMEXEC;

    if (input_length > 0) {
        /* INPUT active */
        if (pending_command == CMD_RUN || pending_command == CMD_SST) {
            int err = generic_sto(&input_arg, 0);
            if ((flags.f.trace_print || flags.f.normal_print)
                    && flags.f.printer_exists) {
                char lbuf[12], rbuf[100];
                int llen, rlen;
                string_copy(lbuf, &llen, input_name, input_length);
                lbuf[llen++] = '=';
                rlen = vartype2string(reg_x, rbuf, 100);
                print_wide(lbuf, llen, rbuf, rlen);
            }
            input_length = 0;
            if (err != ERR_NONE) {
                pending_command = CMD_NONE;
                display_error(err, 1);
                redisplay();
                return mode_running || keybuf_head != keybuf_tail;
            }
        } else if (pending_command == CMD_GTO
                || pending_command == CMD_GTODOT
                || pending_command == CMD_GTODOTDOT
                || pending_command == CMD_RTN)
            /* NOTE: set_running(true) also ends INPUT mode, so commands that
             * cause program execution to start do not have to be handled here.
             */
            input_length = 0;
    }

    if (pending_command == CMD_VMEXEC) {
        string_copy(reg_alpha, &reg_alpha_length,
                    pending_command_arg.val.text, pending_command_arg.length);
        goto do_run;
    }
    if (pending_command == CMD_RUN) {
        do_run:
        if ((flags.f.trace_print || flags.f.normal_print)
                && flags.f.printer_exists)
            print_command(pending_command, &pending_command_arg);
        pending_command = CMD_NONE;
        if (pc == -1)
            pc = 0;
        set_running(true);
        return 1;
    }

    if (pending_command == CMD_SST) {
        int cmd;
        arg_struct arg;
        oldpc = pc;
        if (pc == -1)
            pc = 0;
        get_next_command(&pc, &cmd, &arg, 1);
        if ((flags.f.trace_print || flags.f.normal_print)
                && flags.f.printer_exists)
            print_program_line(current_prgm, oldpc);
        mode_disable_stack_lift = false;
        set_running(true);
        error = cmdlist(cmd)->handler(&arg);
        set_running(false);
        mode_pause = false;
    } else {
        if ((flags.f.trace_print || flags.f.normal_print)
                && flags.f.printer_exists)
            print_command(pending_command, &pending_command_arg);
        mode_disable_stack_lift = false;
        error = cmdlist(pending_command)->handler(&pending_command_arg);
        mode_pause = false;
    }

    if (error == ERR_INTERRUPTIBLE) {
        shell_annunciators(-1, -1, -1, 1, -1, -1);
        return 1;
    }

    handle_error(error);
    pending_command = CMD_NONE;
    if (!mode_getkey && !mode_pause)
        redisplay();
    return (mode_running && !mode_getkey && !mode_pause) || keybuf_head != keybuf_tail;
}

int core_allows_powerdown(int *want_cpu) {
    int allow = shell_low_battery() || !(mode_running || mode_getkey
                    || flags.f.continuous_on || mode_interruptible != NULL);
    *want_cpu = 0;
    if (!allow && mode_getkey) {
        /* We're being asked to power down but we're refusing.
         * If this happens in the middle of a GETKEY, it should return 70,
         * the code for OFF, but without stopping program execution.
         */
        vartype *seventy = new_real(70);
        if (seventy != NULL) {
            recall_result(seventy);
            flags.f.stack_lift_disable = 0;
            if (mode_running) {
                shell_annunciators(-1, -1, -1, 1, -1, -1);
                *want_cpu = 1;
            } else
                redisplay();
        } else {
            /* Memory allocation failure... The program will stop now,
             * anyway, so we change our mind and allow the powerdown.
             */
            display_error(ERR_INSUFFICIENT_MEMORY, 1);
            set_running(false);
            redisplay();
            allow = 1;
        }
        mode_getkey = 0;
    }
    return allow;
}

int core_powercycle() {
    bool need_redisplay = false;

    if (mode_interruptible != NULL)
        stop_interruptible();

    no_keystrokes_yet = true;

    keybuf_tail = keybuf_head;
    set_shift(false);
    flags.f.continuous_on = 0;
    pending_command = CMD_NONE;

    if (mode_getkey) {
        /* A real HP-42S can't be switched off while GETKEY is active: pressing
         * OFF on the keyboard returns code 70 and stops program execution; and
         * when the auto-poweroff timeout expires, code 70 is returned but
         * program execution continues.
         * Since Free42 can be shut down in ways the HP-42S can't (exiting the
         * application, or turning off power on a Palm), I have to fake it a
         * bit; I put 70 in X as if the user had done OFF twice on a real 42S.
         * Note that, as on a real 42S, we don't allow auto-powerdown while
         * GETKEY is waiting; if an auto-powerdown request happens during
         * GETKEY, it returns 70 but program execution is not stopped, and the
         * power stays on (see core_allows_powerdown(), above).
         */
        vartype *seventy = new_real(70);
        if (seventy != NULL) {
            recall_result(seventy);
            flags.f.stack_lift_disable = 0;
        } else {
            display_error(ERR_INSUFFICIENT_MEMORY, 1);
            flags.f.auto_exec = 0;
        }
        if (!flags.f.auto_exec)
            need_redisplay = true;
        mode_getkey = false;
    }

    if (flags.f.auto_exec) {
        if (mode_command_entry)
            finish_command_entry(false);
        if (flags.f.prgm_mode) {
            if (mode_alpha_entry) {
                pc = incomplete_saved_pc;
                prgm_highlight_row = incomplete_saved_highlight_row;
            } else if (mode_number_entry) {
                arg_struct arg;
                arg.type = ARGTYPE_DOUBLE;
                arg.val_d = entered_number;
                store_command(pc, CMD_NUMBER, &arg);
                prgm_highlight_row = 1;
            }
            flags.f.prgm_mode = false;
        }
        mode_alpha_entry = false;
        mode_number_entry = false;
        set_menu(MENULEVEL_ALPHA, MENU_NONE);
        flags.f.alpha_mode = 0;
        set_running(true);
        flags.f.auto_exec = 0;
        need_redisplay = false;
    } else {
        if (mode_running) {
            set_running(false);
            need_redisplay = true;
        }
    }

#ifdef BCD_MATH
    if (need_redisplay || state_file_number_format != NUMBER_FORMAT_BID128)
        redisplay();
#else
    if (need_redisplay || state_file_number_format != NUMBER_FORMAT_BINARY)
        redisplay();
#endif

    return mode_running;
}

int core_list_programs(char *buf, int bufsize) {
    int lastidx = -1;
    int bufptr = 0;
    int label;
    int count = 0;
    for (label = 0; label < labels_count; label++) {
        int len = labels[label].length;
        char name[51];
        int namelen = 0;
        int end = 0;
        int i;

        if (len == 0) {
            if (labels[label].prgm == lastidx)
                continue;
            if (label == labels_count - 1) {
                string2buf(name, 21, &namelen, ".END.", 5);
                namelen = 5;
            } else {
                string2buf(name, 21, &namelen, "END", 3);
                namelen = 3;
            }
            end = 1;
        } else {
            name[namelen++] = '"';
            namelen += hp2ascii(name + namelen, labels[label].name, len);
            name[namelen++] = '"';
            end = labels[label + 1].length == 0;
        }

        lastidx = labels[label].prgm;

        if (bufptr + namelen + 1 >= bufsize) {
            if (bufptr > 0 && buf[bufptr - 1] != 0) {
                buf[bufptr - 1] = 0;
                count++;
            }
            return count;
        }
        for (i = 0; i < namelen; i++)
            buf[bufptr++] = name[i];
        if (end) {
            buf[bufptr++] = 0;
            count++;
        } else {
            buf[bufptr++] = ' ';
        }
    }
    return count;
}

static int export_hp42s(int index, int (*progress_report)(const char *)) {
    int4 pc = 0;
    int cmd;
    arg_struct arg;
    int saved_prgm = current_prgm;
    uint4 hp42s_code;
    unsigned char code_flags, code_name, code_std_1, code_std_2;
    char cmdbuf[25];
    int cmdlen;
    char buf[1000];
    int buflen = 0;
    int i;
    int cancel = 0;

    current_prgm = index;
    do {
        get_next_command(&pc, &cmd, &arg, 0);
        hp42s_code = cmdlist(cmd)->hp42s_code;
        code_flags = hp42s_code >> 24;
        code_name = hp42s_code >> 16;
        code_std_1 = hp42s_code >> 8;
        code_std_2 = hp42s_code;
        cmdlen = 0;
        switch (code_flags) {
            case 1:
                /* A command that requires some special attention */
                if (cmd == CMD_STO) {
                    if (arg.type == ARGTYPE_NUM && arg.val.num <= 15)
                        cmdbuf[cmdlen++] = 0x30 + arg.val.num;
                    else
                        goto normal;
                } else if (cmd == CMD_RCL) {
                    if (arg.type == ARGTYPE_NUM && arg.val.num <= 15)
                        cmdbuf[cmdlen++] = 0x20 + arg.val.num;
                    else
                        goto normal;
                } else if (cmd == CMD_FIX || cmd == CMD_SCI || cmd == CMD_ENG) {
                    char byte2;
                    if (arg.type != ARGTYPE_NUM || arg.val.num <= 9)
                        goto normal;
                    cmdbuf[cmdlen++] = (char) 0xF1;
                    if (arg.val.num == 10) {
                        switch (cmd) {
                            case CMD_FIX: byte2 = (char) 0xD5; break;
                            case CMD_SCI: byte2 = (char) 0xD6; break;
                            case CMD_ENG: byte2 = (char) 0xD7; break;
                        }
                    } else {
                        switch (cmd) {
                            case CMD_FIX: byte2 = (char) 0xE5; break;
                            case CMD_SCI: byte2 = (char) 0xE6; break;
                            case CMD_ENG: byte2 = (char) 0xE7; break;
                        }
                    }
                    cmdbuf[cmdlen++] = byte2;
                } else if (cmd == CMD_SIZE) {
                    cmdbuf[cmdlen++] = (char) 0xF3;
                    cmdbuf[cmdlen++] = (char) 0xF7;
                    cmdbuf[cmdlen++] = arg.val.num >> 8;
                    cmdbuf[cmdlen++] = arg.val.num;
                } else if (cmd == CMD_LBL) {
                    if (arg.type == ARGTYPE_NUM) {
                        if (arg.val.num <= 14)
                            cmdbuf[cmdlen++] = 0x01 + arg.val.num;
                        else
                            goto normal;
                    } else if (arg.type == ARGTYPE_STR) {
                        if (progress_report != NULL) {
                            char s[52];
                            int sl = hp2ascii(s + 1, arg.val.text, arg.length);
                            s[0] = s[sl + 1] = '"';
                            s[sl + 2] = 0;
                            if (progress_report(s)) {
                                cancel = 1;
                                goto done;
                            }
                        }
                        cmdbuf[cmdlen++] = (char) 0xC0;
                        cmdbuf[cmdlen++] = 0x00;
                        cmdbuf[cmdlen++] = 0xF1 + arg.length;
                        cmdbuf[cmdlen++] = 0x00;
                        for (i = 0; i < arg.length; i++)
                            cmdbuf[cmdlen++] = arg.val.text[i];
                    } else
                        goto normal;
                } else if (cmd == CMD_INPUT) {
                    if (arg.type == ARGTYPE_IND_NUM
                            || arg.type == ARGTYPE_IND_STK)
                        code_std_2 = 0xEE;
                    goto normal;
                } else if (cmd == CMD_XEQ) {
                    if (arg.type == ARGTYPE_NUM || arg.type == ARGTYPE_LCLBL) {
                        code_std_1 = 0xE0;
                        code_std_2 = 0x00;
                        goto normal;
                    } else if (arg.type == ARGTYPE_STR) {
                        cmdbuf[cmdlen++] = 0x1E;
                        cmdbuf[cmdlen++] = 0xF0 + arg.length;
                        for (i = 0; i < arg.length; i++)
                            cmdbuf[cmdlen++] = arg.val.text[i];
                    } else
                        goto normal;
                } else if (cmd == CMD_GTO) {
                    if (arg.type == ARGTYPE_NUM && arg.val.num <= 14) {
                        cmdbuf[cmdlen++] = 0xB1 + arg.val.num;
                        cmdbuf[cmdlen++] = 0x00;
                    } else if (arg.type == ARGTYPE_NUM
                                        || arg.type == ARGTYPE_LCLBL) {
                        code_std_1 = 0xD0;
                        code_std_2 = 0x00;
                        goto normal;
                    } else if (arg.type == ARGTYPE_IND_NUM
                                        || arg.type == ARGTYPE_IND_STK) {
                        cmdbuf[cmdlen++] = (char) 0xAE;
                        if (arg.type == ARGTYPE_IND_NUM)
                            arg.type = ARGTYPE_NUM;
                        else
                            arg.type = ARGTYPE_STK;
                        goto non_string_suffix;
                    } else if (arg.type == ARGTYPE_STR) {
                        cmdbuf[cmdlen++] = 0x1D;
                        cmdbuf[cmdlen++] = 0xF0 + arg.length;
                        for (i = 0; i < arg.length; i++)
                            cmdbuf[cmdlen++] = arg.val.text[i];
                    } else
                        goto normal;
                } else if (cmd == CMD_END) {
                    if (progress_report != NULL && progress_report("END")) {
                        cancel = 1;
                        goto done;
                    }
                    cmdbuf[cmdlen++] = (char) 0xC0;
                    cmdbuf[cmdlen++] = 0x00;
                    cmdbuf[cmdlen++] = 0x0D;
                } else if (cmd == CMD_NUMBER) {
                    char *p = phloat2program(arg.val_d);
                    char dot = flags.f.decimal_point ? '.' : ',';
                    char c;
                    while ((c = *p++) != 0) {
                        if (c >= '0' && c <= '9')
                            cmdbuf[cmdlen++] = 0x10 + c - '0';
                        else if (c == dot)
                            cmdbuf[cmdlen++] = 0x1A;
                        else if (c == 24)
                            cmdbuf[cmdlen++] = 0x1B;
                        else if (c == '-')
                            cmdbuf[cmdlen++] = 0x1C;
                        else
                            /* Should not happen */
                            continue;
                    }
                    cmdbuf[cmdlen++] = 0x00;
                } else if (cmd == CMD_STRING) {
                    cmdbuf[cmdlen++] = 0xF0 + arg.length;
                    for (i = 0; i < arg.length; i++)
                        cmdbuf[cmdlen++] = arg.val.text[i];
                } else if (cmd >= CMD_ASGN01 && cmd <= CMD_ASGN18) {
                    if (arg.type == ARGTYPE_STR) {
                        cmdbuf[cmdlen++] = 0xF2 + arg.length;
                        cmdbuf[cmdlen++] = (char) 0xC0;
                        for (i = 0; i < arg.length; i++)
                            cmdbuf[cmdlen++] = arg.val.text[i];
                    } else {
                        /* arg.type == ARGTYPE_COMMAND; we don't use that
                         * any more, but just to be safe (in case anyone ever
                         * actually used this in a program), we handle it
                         * anyway.
                         */
                        const command_spec *cs = cmdlist(arg.val.cmd);
                        cmdbuf[cmdlen++] = 0xF2 + cs->name_length;
                        cmdbuf[cmdlen++] = (char) 0xC0;
                        for (i = 0; i < cs->name_length; i++)
                            cmdbuf[cmdlen++] = cs->name[i];
                    }
                    cmdbuf[cmdlen++] = cmd - CMD_ASGN01;
                } else if ((cmd >= CMD_KEY1G && cmd <= CMD_KEY9G) 
                            || (cmd >= CMD_KEY1X && cmd <= CMD_KEY9X)) {
                    int keyg = cmd <= CMD_KEY9G;
                    int keynum = cmd - (keyg ? CMD_KEY1G : CMD_KEY1X) + 1;
                    if (arg.type == ARGTYPE_STR || arg.type == ARGTYPE_IND_STR){
                        cmdbuf[cmdlen++] = 0xF2 + arg.length;
                        cmdbuf[cmdlen++] = keyg ? 0xC3 : 0xC2;
                        if (arg.type == ARGTYPE_IND_STR)
                            cmdbuf[cmdlen - 1] += 8;
                        cmdbuf[cmdlen++] = keynum;
                        for (i = 0; i < arg.length; i++)
                            cmdbuf[cmdlen++] = arg.val.text[i];
                    } else {
                        cmdbuf[cmdlen++] = (char) 0xF3;
                        cmdbuf[cmdlen++] = keyg ? 0xE3 : 0xE2;
                        cmdbuf[cmdlen++] = keynum;
                        goto non_string_suffix;
                    }
                } else if (cmd == CMD_XROM) {
                    cmdbuf[cmdlen++] = (char) (0xA0 + ((arg.val.num >> 8) & 7));
                    cmdbuf[cmdlen++] = (char) arg.val.num;
                } else {
                    /* Shouldn't happen */
                    continue;
                }
                break;
            case 0:
            normal:
                if (arg.type == ARGTYPE_STR || arg.type == ARGTYPE_IND_STR) {
                    int i;
                    cmdbuf[cmdlen++] = 0xF0 + arg.length + 1;
                    cmdbuf[cmdlen++] = arg.type == ARGTYPE_STR ? code_name
                                                            : code_name + 8;
                    for (i = 0; i < arg.length; i++)
                        cmdbuf[cmdlen++] = arg.val.text[i];
                } else {
                    unsigned char suffix;
                    if (code_std_1 != 0)
                        cmdbuf[cmdlen++] = code_std_1;
                    cmdbuf[cmdlen++] = code_std_2;

                    non_string_suffix:
                    suffix = 0;
                    switch (arg.type) {
                        case ARGTYPE_NONE:
                            goto no_suffix;
                        case ARGTYPE_IND_NUM:
                            suffix = 0x80;
                        case ARGTYPE_NUM:
                            suffix += arg.val.num;
                            break;
                        case ARGTYPE_IND_STK:
                            suffix = 0x80;
                        case ARGTYPE_STK:
                            switch (arg.val.stk) {
                                case 'X': suffix += 0x73; break;
                                case 'Y': suffix += 0x72; break;
                                case 'Z': suffix += 0x71; break;
                                case 'T': suffix += 0x70; break;
                                case 'L': suffix += 0x74; break;
                                default:
                                    /* Shouldn't happen */
                                    continue;
                            }
                            break;
                        case ARGTYPE_LCLBL:
                            if (arg.val.lclbl >= 'A' && arg.val.lclbl <= 'J')
                                suffix = arg.val.lclbl - 'A' + 0x66;
                            else if (arg.val.lclbl >= 'a' &&
                                                        arg.val.lclbl <= 'e')
                                suffix = arg.val.lclbl - 'a' + 0x7B;
                            else
                                /* Shouldn't happen */
                                continue;
                            break;
                        default:
                            /* Shouldn't happen */
                            /* Values not handled above are ARGTYPE_NEG_NUM,
                             * which is converted to ARGTYPE_NUM by
                             * get_next_command(); ARGTYPE_DOUBLE, which only
                             * occurs with CMD_NUMBER, which is handled in the
                             * special-case section, above; ARGTYPE_COMMAND,
                             * which is handled in the special-case section;
                             * and ARGTYPE_LBLINDEX, which is converted to
                             * ARGTYPE_STR before being stored in a program.
                             */
                            continue;
                    }
                    cmdbuf[cmdlen++] = suffix;
                    no_suffix:
                    ;
                }
                break;
            case 2:
            default:
                /* Illegal command */
                continue;
        }
        if (buflen + cmdlen > 1000) {
            if (!shell_write(buf, buflen))
                goto done;
            buflen = 0;
        }
        for (i = 0; i < cmdlen; i++)
            buf[buflen++] = cmdbuf[i];
    } while (cmd != CMD_END && pc < prgms[index].size);
    if (buflen > 0)
        shell_write(buf, buflen);
    done:
    current_prgm = saved_prgm;
    return cancel;
}

int4 core_program_size(int prgm_index) {
    int4 pc = 0;
    int cmd;
    arg_struct arg;
    int saved_prgm = current_prgm;
    uint4 hp42s_code;
    unsigned char code_flags, code_std_1;
    //unsigned char code_name, code_std_2;
    int4 size = 0;

    current_prgm = prgm_index;
    do {
        get_next_command(&pc, &cmd, &arg, 0);
        hp42s_code = cmdlist(cmd)->hp42s_code;
        code_flags = hp42s_code >> 24;
        //code_name = hp42s_code >> 16;
        code_std_1 = hp42s_code >> 8;
        //code_std_2 = hp42s_code;
        switch (code_flags) {
            case 1:
                /* A command that requires some special attention */
                if (cmd == CMD_STO) {
                    if (arg.type == ARGTYPE_NUM && arg.val.num <= 15)
                        size += 1;
                    else
                        goto normal;
                } else if (cmd == CMD_RCL) {
                    if (arg.type == ARGTYPE_NUM && arg.val.num <= 15)
                        size += 1;
                    else
                        goto normal;
                } else if (cmd == CMD_FIX || cmd == CMD_SCI || cmd == CMD_ENG) {
                    goto normal;
                } else if (cmd == CMD_SIZE) {
                    size += 4;
                } else if (cmd == CMD_LBL) {
                    if (arg.type == ARGTYPE_NUM) {
                        if (arg.val.num <= 14)
                            size += 1;
                        else
                            goto normal;
                    } else if (arg.type == ARGTYPE_STR) {
                        size += arg.length + 4;
                    } else
                        goto normal;
                } else if (cmd == CMD_INPUT) {
                    goto normal;
                } else if (cmd == CMD_XEQ) {
                    if (arg.type == ARGTYPE_NUM || arg.type == ARGTYPE_LCLBL) {
                        size += 3;
                    } else if (arg.type == ARGTYPE_STR) {
                        size += arg.length + 2;
                    } else
                        goto normal;
                } else if (cmd == CMD_GTO) {
                    if (arg.type == ARGTYPE_NUM && arg.val.num <= 14) {
                        size += 2;
                    } else if (arg.type == ARGTYPE_NUM
                                        || arg.type == ARGTYPE_LCLBL) {
                        size += 3;
                    } else if (arg.type == ARGTYPE_STR) {
                        size += arg.length + 2;
                    } else
                        goto normal;
                } else if (cmd == CMD_END) {
                    /* Not counted for the line 00 total */
                } else if (cmd == CMD_NUMBER) {
                    char *p = phloat2program(arg.val_d);
                    while (*p++ != 0)
                        size += 1;
                    size += 1;
                } else if (cmd == CMD_STRING) {
                    size += arg.length + 1;
                } else if (cmd >= CMD_ASGN01 && cmd <= CMD_ASGN18) {
                    if (arg.type == ARGTYPE_STR)
                        size += arg.length + 3;
                    else
                        /* arg.type == ARGTYPE_COMMAND; we don't use that
                         * any more, but just to be safe (in case anyone ever
                         * actually used this in a program), we handle it
                         * anyway.
                         */
                        size += cmdlist(arg.val.cmd)->name_length + 3;
                } else if ((cmd >= CMD_KEY1G && cmd <= CMD_KEY9G) 
                            || (cmd >= CMD_KEY1X && cmd <= CMD_KEY9X)) {
                    if (arg.type == ARGTYPE_STR || arg.type == ARGTYPE_IND_STR)
                        size += arg.length + 3;
                    else
                        size += 4;
                } else if (cmd == CMD_XROM) {
                    size += 2;
                } else {
                    /* Shouldn't happen */
                    continue;
                }
                break;
            case 0:
            normal:
                if (arg.type == ARGTYPE_STR || arg.type == ARGTYPE_IND_STR) {
                    size += arg.length + 2;
                } else {
                    size += code_std_1 == 0 ? 1 : 2;
                    if (arg.type != ARGTYPE_NONE)
                        size += 1;
                }
                break;
            case 2:
            default:
                /* Illegal command */
                continue;
        }
    } while (cmd != CMD_END && pc < prgms[prgm_index].size);
    current_prgm = saved_prgm;
    return size;
}

int core_export_programs(int count, const int *indexes,
                          int (*progress_report)(const char *)) {
    int i;
    for (i = 0; i < count; i++) {
        int p = indexes[i];
        if (export_hp42s(p, progress_report))
            return 1;
    }
    return 0;
}

static int hp42tofree42[] = {
    /* Flag values: 0 = simple 1-byte command; 1 = 1-byte command with
     * embedded argument; 2 = 2-byte command, argument follows;
     * 3 = everything else, special case handling required.
     */

    /* 00-0F */
    /* NULL, LBL 00-14 */
    CMD_NULL  | 0x3000,
    CMD_LBL   | 0x1000,
    CMD_LBL   | 0x1000,
    CMD_LBL   | 0x1000,
    CMD_LBL   | 0x1000,
    CMD_LBL   | 0x1000,
    CMD_LBL   | 0x1000,
    CMD_LBL   | 0x1000,
    CMD_LBL   | 0x1000,
    CMD_LBL   | 0x1000,
    CMD_LBL   | 0x1000,
    CMD_LBL   | 0x1000,
    CMD_LBL   | 0x1000,
    CMD_LBL   | 0x1000,
    CMD_LBL   | 0x1000,
    CMD_LBL   | 0x1000,

    /* 10-1F */
    /* 0-9, ., E, -, GTO "", XEQ "", W "" */
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,

    /* 20-2F */
    /* RCL 00-15 */
    CMD_RCL   | 0x1000,
    CMD_RCL   | 0x1000,
    CMD_RCL   | 0x1000,
    CMD_RCL   | 0x1000,
    CMD_RCL   | 0x1000,
    CMD_RCL   | 0x1000,
    CMD_RCL   | 0x1000,
    CMD_RCL   | 0x1000,
    CMD_RCL   | 0x1000,
    CMD_RCL   | 0x1000,
    CMD_RCL   | 0x1000,
    CMD_RCL   | 0x1000,
    CMD_RCL   | 0x1000,
    CMD_RCL   | 0x1000,
    CMD_RCL   | 0x1000,
    CMD_RCL   | 0x1000,

    /* 30-3F */
    /* STO 00-15 */
    CMD_STO   | 0x1000,
    CMD_STO   | 0x1000,
    CMD_STO   | 0x1000,
    CMD_STO   | 0x1000,
    CMD_STO   | 0x1000,
    CMD_STO   | 0x1000,
    CMD_STO   | 0x1000,
    CMD_STO   | 0x1000,
    CMD_STO   | 0x1000,
    CMD_STO   | 0x1000,
    CMD_STO   | 0x1000,
    CMD_STO   | 0x1000,
    CMD_STO   | 0x1000,
    CMD_STO   | 0x1000,
    CMD_STO   | 0x1000,
    CMD_STO   | 0x1000,

    /* 40-4F */
    CMD_ADD         | 0x0000,
    CMD_SUB         | 0x0000,
    CMD_MUL         | 0x0000,
    CMD_DIV         | 0x0000,
    CMD_X_LT_Y      | 0x0000,
    CMD_X_GT_Y      | 0x0000,
    CMD_X_LE_Y      | 0x0000,
    CMD_SIGMAADD    | 0x0000,
    CMD_SIGMASUB    | 0x0000,
    CMD_HMSADD      | 0x0000,
    CMD_HMSSUB      | 0x0000,
    CMD_MOD         | 0x0000,
    CMD_PERCENT     | 0x0000,
    CMD_PERCENT_CH  | 0x0000,
    CMD_TO_REC      | 0x0000,
    CMD_TO_POL      | 0x0000,

    /* 50-5F */
    CMD_LN         | 0x0000,
    CMD_SQUARE     | 0x0000,
    CMD_SQRT       | 0x0000,
    CMD_Y_POW_X    | 0x0000,
    CMD_CHS        | 0x0000,
    CMD_E_POW_X    | 0x0000,
    CMD_LOG        | 0x0000,
    CMD_10_POW_X   | 0x0000,
    CMD_E_POW_X_1  | 0x0000,
    CMD_SIN        | 0x0000,
    CMD_COS        | 0x0000,
    CMD_TAN        | 0x0000,
    CMD_ASIN       | 0x0000,
    CMD_ACOS       | 0x0000,
    CMD_ATAN       | 0x0000,
    CMD_TO_DEC     | 0x0000,

    /* 60-6F */
    CMD_INV     | 0x0000,
    CMD_ABS     | 0x0000,
    CMD_FACT    | 0x0000,
    CMD_X_NE_0  | 0x0000,
    CMD_X_GT_0  | 0x0000,
    CMD_LN_1_X  | 0x0000,
    CMD_X_LT_0  | 0x0000,
    CMD_X_EQ_0  | 0x0000,
    CMD_IP      | 0x0000,
    CMD_FP      | 0x0000,
    CMD_TO_RAD  | 0x0000,
    CMD_TO_DEG  | 0x0000,
    CMD_TO_HMS  | 0x0000,
    CMD_TO_HR   | 0x0000,
    CMD_RND     | 0x0000,
    CMD_TO_OCT  | 0x0000,
    
    /* 70-7F */
    CMD_CLSIGMA  | 0x0000,
    CMD_SWAP     | 0x0000,
    CMD_PI       | 0x0000,
    CMD_CLST     | 0x0000,
    CMD_RUP      | 0x0000,
    CMD_RDN      | 0x0000,
    CMD_LASTX    | 0x0000,
    CMD_CLX      | 0x0000,
    CMD_X_EQ_Y   | 0x0000,
    CMD_X_NE_Y   | 0x0000,
    CMD_SIGN     | 0x0000,
    CMD_X_LE_0   | 0x0000,
    CMD_MEAN     | 0x0000,
    CMD_SDEV     | 0x0000,
    CMD_AVIEW    | 0x0000,
    CMD_CLD      | 0x0000,

    /* 80-8F */
    CMD_DEG     | 0x0000,
    CMD_RAD     | 0x0000,
    CMD_GRAD    | 0x0000,
    CMD_ENTER   | 0x0000,
    CMD_STOP    | 0x0000,
    CMD_RTN     | 0x0000,
    CMD_BEEP    | 0x0000,
    CMD_CLA     | 0x0000,
    CMD_ASHF    | 0x0000,
    CMD_PSE     | 0x0000,
    CMD_CLRG    | 0x0000,
    CMD_AOFF    | 0x0000,
    CMD_AON     | 0x0000,
    CMD_OFF     | 0x0000,
    CMD_PROMPT  | 0x0000,
    CMD_ADV     | 0x0000,

    /* 90-9F */
    CMD_RCL       | 0x2000,
    CMD_STO       | 0x2000,
    CMD_STO_ADD   | 0x2000,
    CMD_STO_SUB   | 0x2000,
    CMD_STO_MUL   | 0x2000,
    CMD_STO_DIV   | 0x2000,
    CMD_ISG       | 0x2000,
    CMD_DSE       | 0x2000,
    CMD_VIEW      | 0x2000,
    CMD_SIGMAREG  | 0x2000,
    CMD_ASTO      | 0x2000,
    CMD_ARCL      | 0x2000,
    CMD_FIX       | 0x2000,
    CMD_SCI       | 0x2000,
    CMD_ENG       | 0x2000,
    CMD_TONE      | 0x2000,

    /* A0-AF */
    /* XROM (+ 42S ext), GTO/XEQ IND, SPARE1 */
    CMD_NULL   | 0x3000,
    CMD_NULL   | 0x3000,
    CMD_NULL   | 0x3000,
    CMD_NULL   | 0x3000,
    CMD_NULL   | 0x3000,
    CMD_NULL   | 0x3000,
    CMD_NULL   | 0x3000,
    CMD_NULL   | 0x3000,
    CMD_SF     | 0x2000,
    CMD_CF     | 0x2000,
    CMD_FSC_T  | 0x2000,
    CMD_FCC_T  | 0x2000,
    CMD_FS_T   | 0x2000,
    CMD_FC_T   | 0x2000,
    CMD_NULL   | 0x3000,
    CMD_NULL   | 0x3000,

    /* B0-BF */
    /* SPARE2, GTO 00-14 */
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,

    /* C0-CF */
    /* GLOBAL */
    CMD_NULL    | 0x3000,
    CMD_NULL    | 0x3000,
    CMD_NULL    | 0x3000,
    CMD_NULL    | 0x3000,
    CMD_NULL    | 0x3000,
    CMD_NULL    | 0x3000,
    CMD_NULL    | 0x3000,
    CMD_NULL    | 0x3000,
    CMD_NULL    | 0x3000,
    CMD_NULL    | 0x3000,
    CMD_NULL    | 0x3000,
    CMD_NULL    | 0x3000,
    CMD_NULL    | 0x3000,
    CMD_NULL    | 0x3000,
    CMD_X_SWAP  | 0x2000,
    CMD_LBL     | 0x2000,

    /* D0-DF */
    /* 3-byte GTO */
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,

    /* E0-EF */
    /* 3-byte XEQ */
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,

    /* F0-FF */
    /* Strings + 42S ext */
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000,
    CMD_NULL  | 0x3000
};

static int hp42ext[] = {
    /* Flag values: 0 = string, 1 = IND string, 2 = suffix, 3 = special,
     * 4 = illegal */
    /* 80-8F */
    CMD_VIEW    | 0x0000,
    CMD_STO     | 0x0000,
    CMD_STO_ADD | 0x0000,
    CMD_STO_SUB | 0x0000,
    CMD_STO_MUL | 0x0000,
    CMD_STO_DIV | 0x0000,
    CMD_X_SWAP  | 0x0000,
    CMD_INDEX   | 0x0000,
    CMD_VIEW    | 0x1000,
    CMD_STO     | 0x1000,
    CMD_STO_ADD | 0x1000,
    CMD_STO_SUB | 0x1000,
    CMD_STO_MUL | 0x1000,
    CMD_STO_DIV | 0x1000,
    CMD_X_SWAP  | 0x1000,
    CMD_INDEX   | 0x1000,

    /* 90-9F */
    CMD_MVAR    | 0x0000,
    CMD_RCL     | 0x0000,
    CMD_RCL_ADD | 0x0000,
    CMD_RCL_SUB | 0x0000,
    CMD_RCL_MUL | 0x0000,
    CMD_RCL_DIV | 0x0000,
    CMD_ISG     | 0x0000,
    CMD_DSE     | 0x0000,
    CMD_MVAR    | 0x1000,
    CMD_RCL     | 0x1000,
    CMD_RCL_ADD | 0x1000,
    CMD_RCL_SUB | 0x1000,
    CMD_RCL_MUL | 0x1000,
    CMD_RCL_DIV | 0x1000,
    CMD_ISG     | 0x1000,
    CMD_DSE     | 0x1000,

    /* A0-AF */
    CMD_NULL  | 0x4000,
    CMD_NULL  | 0x4000,
    CMD_NULL  | 0x4000,
    CMD_NULL  | 0x4000,
    CMD_NULL  | 0x4000,
    CMD_NULL  | 0x4000,
    CMD_NULL  | 0x4000,
    CMD_NULL  | 0x4000,
    CMD_SF    | 0x1000,
    CMD_CF    | 0x1000,
    CMD_FSC_T | 0x1000,
    CMD_FCC_T | 0x1000,
    CMD_FS_T  | 0x1000,
    CMD_FC_T  | 0x1000,
    CMD_GTO   | 0x1000,
    CMD_XEQ   | 0x1000,

    /* B0-BF */
    CMD_CLV    | 0x0000,
    CMD_PRV    | 0x0000,
    CMD_ASTO   | 0x0000,
    CMD_ARCL   | 0x0000,
    CMD_PGMINT | 0x0000,
    CMD_PGMSLV | 0x0000,
    CMD_INTEG  | 0x0000,
    CMD_SOLVE  | 0x0000,
    CMD_CLV    | 0x1000,
    CMD_PRV    | 0x1000,
    CMD_ASTO   | 0x1000,
    CMD_ARCL   | 0x1000,
    CMD_PGMINT | 0x1000,
    CMD_PGMSLV | 0x1000,
    CMD_INTEG  | 0x1000,
    CMD_SOLVE  | 0x1000,

    /* CO-CF */
    CMD_NULL    | 0x3000, /* ASSIGN */
    CMD_VARMENU | 0x0000,
    CMD_NULL    | 0x3000, /* KEYX name */
    CMD_NULL    | 0x3000, /* KEYG name */
    CMD_DIM     | 0x0000,
    CMD_INPUT   | 0x0000,
    CMD_EDITN   | 0x0000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_VARMENU | 0x1000,
    CMD_NULL    | 0x3000, /* KEYX IND name */
    CMD_NULL    | 0x3000, /* KEYG IND name */
    CMD_DIM     | 0x1000,
    CMD_INPUT   | 0x1000,
    CMD_EDITN   | 0x1000,
    CMD_NULL    | 0x4000,

    /* DO-DF */
    CMD_INPUT    | 0x2000,
    CMD_RCL_ADD  | 0x2000,
    CMD_RCL_SUB  | 0x2000,
    CMD_RCL_MUL  | 0x2000,
    CMD_RCL_DIV  | 0x2000,
    CMD_NULL     | 0x4000, /* FIX 10 */
    CMD_NULL     | 0x4000, /* SCI 10 */
    CMD_NULL     | 0x4000, /* ENG 10 */
    CMD_CLV      | 0x2000,
    CMD_PRV      | 0x2000,
    CMD_INDEX    | 0x2000,
    CMD_SIGMAREG | 0x1000,
    CMD_FIX      | 0x1000,
    CMD_SCI      | 0x1000,
    CMD_ENG      | 0x1000,
    CMD_TONE     | 0x1000,

    /* E0-EF */
    CMD_NULL   | 0x4000,
    CMD_NULL   | 0x4000,
    CMD_NULL   | 0x3000, /* KEYX suffix */
    CMD_NULL   | 0x3000, /* KEYG suffix */
    CMD_NULL   | 0x4000,
    CMD_NULL   | 0x4000, /* FIX 11 */
    CMD_NULL   | 0x4000, /* SCI 11 */
    CMD_NULL   | 0x4000, /* ENG 11 */
    CMD_PGMINT | 0x2000,
    CMD_PGMSLV | 0x2000,
    CMD_INTEG  | 0x2000,
    CMD_SOLVE  | 0x2000,
    CMD_DIM    | 0x2000,
    CMD_NULL   | 0x4000,
    CMD_INPUT  | 0x2000,
    CMD_EDITN  | 0x2000,

    /* F0-FF */
    CMD_CLP     | 0x0000,
    CMD_NULL    | 0x4000, /* XFCN */
    CMD_NULL    | 0x4000, /* GTO . nnnn */
    CMD_NULL    | 0x4000, /* GTO .. */
    CMD_NULL    | 0x4000, /* GTO . "name" */
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000, /* DEL */
    CMD_NULL    | 0x3000, /* SIZE */
    CMD_VARMENU | 0x2000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000
};


static int getbyte(char *buf, int *bufptr, int *buflen, int maxlen) {
    if (*bufptr == *buflen) {
        *buflen = shell_read(buf, maxlen);
        if (*buflen <= 0)
            return -1;
        *bufptr = 0;
    }
    return (unsigned char) buf[(*bufptr)++];
}

void core_import_programs(int (*progress_report)(const char *)) {
    char buf[1000];
    int i, nread = 0;
    int need_to_rebuild_label_table = 0;

    int pos = 0;
    int byte1, byte2, suffix;
    int cmd, flag, str_len;
    int done_flag = 0;
    arg_struct arg;
    int assign = 0;
    int at_end = 1;
    int instrcount = -1;
    int report_label = 0;

    set_running(false);

    /* Set print mode to MAN during the import, to prevent store_command()
     * from printing programs as they load
     */
    int saved_trace = flags.f.trace_print;
    int saved_normal = flags.f.normal_print;
    flags.f.trace_print = 0;
    flags.f.normal_print = 0;

    while (!done_flag) {
        skip:
        if (progress_report != NULL) {
            /* Poll the progress dialog every 100 instructions, in case
             * we're reading a bogus file: normally, we only update the
             * dialog when we encounter a GLOBAL, and that should be often
             * enough when reading legitimate HP-42S programs, but if we're
             * reading garbage, it may not happen at all, and that would
             * mean the user would get no chance to intervene.
             */
            if (++instrcount == 100) {
                if (progress_report(NULL))
                    goto done;
                instrcount = 0;
            }
        }
        byte1 = getbyte(buf, &pos, &nread, 1000);
        if (byte1 == -1)
            goto done;
        cmd = hp42tofree42[byte1];
        flag = cmd >> 12;
        cmd &= 0x0FFF;
        if (flag == 0) {
            arg.type = ARGTYPE_NONE;
            goto store;
        } else if (flag == 1) {
            arg.type = ARGTYPE_NUM;
            arg.val.num = byte1 & 15;
            if (cmd == CMD_LBL)
                arg.val.num--;
            goto store;
        } else if (flag == 2) {
            suffix = getbyte(buf, &pos, &nread, 1000);
            if (suffix == -1)
                goto done;
            goto do_suffix;
        } else /* flag == 3 */ {
            if (byte1 == 0x00)
                /* NULL */
                goto skip;
            else if (byte1 >= 0x10 && byte1 <= 0x1C) {
                /* Number */
                char numbuf[19];
                int numlen = 0;
                int s2d_err;
                do {
                    if (byte1 == 0x1A)
                        byte1 = flags.f.decimal_point ? '.' : ',';
                    else if (byte1 == 0x1B)
                        byte1 = 24;
                    else if (byte1 == 0x1C)
                        byte1 = '-';
                    else
                        byte1 += '0' - 0x10;
                    numbuf[numlen++] = byte1;
                    byte1 = getbyte(buf, &pos, &nread, 1000);
                } while (byte1 >= 0x10 && byte1 <= 0x1C);
                if (byte1 == -1)
                    done_flag = 1;
                else if (byte1 != 0x00)
                    pos--;
                s2d_err = string2phloat(numbuf, numlen, &arg.val_d);
                switch (s2d_err) {
                    case 0: /* OK */
                        break;
                    case 1: /* +overflow */
                        arg.val_d = POS_HUGE_PHLOAT;
                        break;
                    case 2: /* -overflow */
                        arg.val_d = NEG_HUGE_PHLOAT;
                        break;
                    case 3: /* +underflow */
                        arg.val_d = POS_TINY_PHLOAT;
                        break;
                    case 4: /* -underflow */
                        arg.val_d = NEG_TINY_PHLOAT;
                        break;
                    case 5: /* error */
                        arg.val_d = 0;
                        break;
                }
                cmd = CMD_NUMBER;
                arg.type = ARGTYPE_DOUBLE;
            } else if (byte1 == 0x1D || byte1 == 0x1E) {
                cmd = byte1 == 0x1D ? CMD_GTO : CMD_XEQ;
                str_len = getbyte(buf, &pos, &nread, 1000);
                if (str_len == -1)
                    goto done;
                else if (str_len < 0x0F1) {
                    pos--;
                    goto skip;
                } else
                    str_len -= 0x0F0;
                arg.type = ARGTYPE_STR;
                goto do_string;
            } else if (byte1 == 0x1F) {
                /* "W" function (see HP-41C instruction table */
                goto skip;
            } else if (byte1 >= 0x0A0 && byte1 <= 0x0A7) {
                /* XROM & parameterless HP-42S extensions.
                 * I don't want to build a reverse look-up table
                 * for these, so instead I just do a linear search
                 * on the cmdlist table.
                 */
                uint4 code;
                byte2 = getbyte(buf, &pos, &nread, 1000);
                if (byte2 == -1)
                    goto done;
                code = (((unsigned int) byte1) << 8) | byte2;
                for (i = 0; i < CMD_SENTINEL; i++)
                    if (cmdlist(i)->hp42s_code == code) {
                        if ((cmdlist(i)->flags & FLAG_HIDDEN) != 0)
                            break;
                        cmd = i;
                        arg.type = ARGTYPE_NONE;
                        goto store;
                    }
                /* Not found; insert XROM instruction */
                cmd = CMD_XROM;
                arg.type = ARGTYPE_NUM;
                arg.val.num = code & 0x07FF;
                goto store;
            } else if (byte1 == 0x0AE) {
                /* GTO/XEQ IND */
                suffix = getbyte(buf, &pos, &nread, 1000);
                if (suffix == -1)
                    goto done;
                if ((suffix & 0x80) != 0)
                    cmd = CMD_XEQ;
                else {
                    cmd = CMD_GTO;
                    suffix |= 0x080;
                }
                goto do_suffix;
            } else if (byte1 == 0x0AF || byte1 == 0x0B0) {
                /* SPARE functions (see HP-41C instruction table */
                goto skip;
            } else if (byte1 >= 0x0B1 && byte1 <= 0x0BF) {
                /* 2-byte GTO */
                byte2 = getbyte(buf, &pos, &nread, 1000);
                if (byte2 == -1)
                    goto done;
                cmd = CMD_GTO;
                arg.type = ARGTYPE_NUM;
                arg.val.num = (byte1 & 15) - 1;
                goto store;
            } else if (byte1 >= 0x0C0 && byte1 <= 0x0CD) {
                /* GLOBAL */
                byte2 = getbyte(buf, &pos, &nread, 1000);
                if (byte2 == -1)
                    goto done;
                str_len = getbyte(buf, &pos, &nread, 1000);
                if (str_len == -1)
                    goto done;
                if (str_len < 0x0F1) {
                    /* END */
                    if (progress_report != NULL && progress_report("END"))
                        goto done;
                    at_end = 1;
                    goto skip;
                } else {
                    /* LBL "" */
                    str_len -= 0x0F1;
                    byte2 = getbyte(buf, &pos, &nread, 1000);
                    if (byte2 == -1)
                        goto done;
                    cmd = CMD_LBL;
                    arg.type = ARGTYPE_STR;
                    if (progress_report != NULL)
                        report_label = 1;
                    goto do_string;
                }
            } else if (byte1 >= 0x0D0 && byte1 <= 0x0EF) {
                /* 3-byte GTO & XEQ */
                byte2 = getbyte(buf, &pos, &nread, 1000);
                if (byte2 == -1)
                    goto done;
                suffix = getbyte(buf, &pos, &nread, 1000);
                if (suffix == -1)
                    goto done;
                cmd = byte1 <= 0x0DF ? CMD_GTO : CMD_XEQ;
                suffix &= 0x7F;
                goto do_suffix;
            } else /* byte1 >= 0xF0 && byte1 <= 0xFF */ {
                /* Strings and parameterized HP-42S extensions */
                if (byte1 == 0x0F0) {
                    /* Zero-length strings can only be entered synthetically
                     * on the HP-41; they act as NOPs and are sometimes used
                     * right after ISG or DSE.
                     * I would be within my rights to drop these instructions
                     * on the floor -- the real 42S doesn't deal with them
                     * all that gracefully either -- but I'm just too nice,
                     * so I convert them to |-"", which is also a NOP.
                     */
                    cmd = CMD_STRING;
                    arg.type = ARGTYPE_STR;
                    arg.length = 1;
                    arg.val.text[0] = 127;
                    goto store;
                }
                byte2 = getbyte(buf, &pos, &nread, 1000);
                if (byte1 == 0x0F1) {
                    switch (byte2) {
                        case 0x0D5: cmd = CMD_FIX; arg.val.num = 10; break;
                        case 0x0D6: cmd = CMD_SCI; arg.val.num = 10; break;
                        case 0x0D7: cmd = CMD_ENG; arg.val.num = 10; break;
                        case 0x0E5: cmd = CMD_FIX; arg.val.num = 11; break;
                        case 0x0E6: cmd = CMD_SCI; arg.val.num = 11; break;
                        case 0x0E7: cmd = CMD_ENG; arg.val.num = 11; break;
                        default: goto plain_string;
                    }
                    arg.type = ARGTYPE_NUM;
                    goto store;
                }
                if ((byte2 & 0x080) == 0 || byte1 < 0x0F2) {
                    /* String */
                    int i;
                    plain_string:
                    str_len = byte1 - 0x0F0;
                    pos--;
                    cmd = CMD_STRING;
                    arg.type = ARGTYPE_STR;
                    do_string:
                    for (i = 0; i < str_len; i++) {
                        suffix = getbyte(buf, &pos, &nread, 1000);
                        if (suffix == -1)
                            goto done;
                        arg.val.text[i] = suffix;
                    }
                    arg.length = str_len;
                    if (assign) {
                        assign = 0;
                        suffix = getbyte(buf, &pos, &nread, 1000);
                        if (suffix == -1)
                            goto done;
                        if (suffix > 17) {
                            /* Bad assign... Fix the command to the string
                             * it would have been if I had known this
                             * earlier.
                             */
                            for (i = arg.length; i > 0; i--)
                                arg.val.text[i] = arg.val.text[i - 1];
                            arg.val.text[0] = (char) 0xC0;
                            arg.val.text[arg.length + 1] = suffix;
                            arg.length += 2;
                        } else {
                            cmd += suffix;
                        }
                    }
                    if (report_label) {
                        char s[52];
                        int sl = hp2ascii(s + 1, arg.val.text, arg.length);
                        s[0] = s[sl + 1] = '"';
                        s[sl + 2] = 0;
                        report_label = 0;
                        if (progress_report(s))
                            goto done;
                    }
                    goto store;
                } else {
                    /* Parameterized HP-42S extension */
                    cmd = hp42ext[byte2 - 0x080];
                    flag = cmd >> 12;
                    cmd &= 0x0FFF;
                    if (flag == 0 || flag == 1) {
                        arg.type = flag == 0 ? ARGTYPE_STR
                                                : ARGTYPE_IND_STR;
                        str_len = byte1 - 0x0F1;
                        goto do_string;
                    } else if (flag == 2) {
                        int ind;
                        if (byte1 != 0x0F2)
                            goto plain_string;
                        suffix = getbyte(buf, &pos, &nread, 1000);
                        if (suffix == -1)
                            goto done;
                        do_suffix:
                        ind = (suffix & 0x080) != 0;
                        suffix &= 0x7F;
                        if (!ind && suffix >= 102 && suffix <= 111) {
                            arg.type = ARGTYPE_LCLBL;
                            arg.val.lclbl = 'A' + (suffix - 102);
                        } else if (!ind && suffix >= 123) {
                            arg.type = ARGTYPE_LCLBL;
                            arg.val.lclbl = 'a' + (suffix - 123);
                        } else if (suffix >= 112 && suffix <= 116) {
                            arg.type = ind ? ARGTYPE_IND_STK : ARGTYPE_STK;
                            switch (suffix) {
                                case 112: arg.val.stk = 'T'; break;
                                case 113: arg.val.stk = 'Z'; break;
                                case 114: arg.val.stk = 'Y'; break;
                                case 115: arg.val.stk = 'X'; break;
                                case 116: arg.val.stk = 'L'; break;
                            }
                        } else {
                            arg.type = ind ? ARGTYPE_IND_NUM : ARGTYPE_NUM;
                            arg.val.num = suffix;
                        }
                        goto store;
                    } else if (flag == 3) {
                        if (byte2 == 0x0C0) {
                            /* ASSIGN */
                            str_len = byte1 - 0x0F2;
                            if (str_len == 0)
                                goto plain_string;
                            assign = 1;
                            cmd = CMD_ASGN01;
                            arg.type = ARGTYPE_STR;
                            goto do_string;
                        } else if (byte2 == 0x0C2 || byte2 == 0x0C3
                                || byte2 == 0x0CA || byte2 == 0x0CB) {
                            /* KEYG/KEYX name, KEYG/KEYX IND name */
                            str_len = byte1 - 0x0F2;
                            if (str_len == 0)
                                goto plain_string;
                            cmd = byte2 == 0x0C2 || byte2 == 0x0CA
                                    ? CMD_KEY1X : CMD_KEY1G;
                            suffix = getbyte(buf, &pos, &nread, 1000);
                            if (suffix == -1)
                                goto done;
                            if (suffix < 1 || suffix > 9) {
                                /* Treat as plain string. Alas, it is
                                 * not safe to back up 2 positions, so
                                 * I do the whole thing here.
                                 */
                                int i;
                                bad_keyg_keyx:
                                cmd = CMD_STRING;
                                arg.type = ARGTYPE_STR;
                                arg.length = str_len + 2;
                                arg.val.text[0] = byte2;
                                arg.val.text[1] = suffix;
                                for (i = 2; i < arg.length; i++) {
                                    int c = getbyte(buf, &pos, &nread,1000);
                                    if (c == -1)
                                        goto done;
                                    arg.val.text[i] = c;
                                }
                                goto store;
                            }
                            cmd += suffix - 1;
                            arg.type = byte2 == 0x0C2 || byte2 == 0x0C3
                                        ? ARGTYPE_STR : ARGTYPE_IND_STR;
                            goto do_string;
                        } else if (byte2 == 0x0E2 || byte2 == 0x0E3) {
                            /* KEYG/KEYX suffix */
                            if (byte1 != 0x0F3)
                                goto plain_string;
                            suffix = getbyte(buf, &pos, &nread, 1000);
                            if (suffix == -1)
                                goto done;
                            if (suffix < 1 || suffix > 9)
                                goto bad_keyg_keyx;
                            cmd = byte2 == 0x0E2 ? CMD_KEY1X : CMD_KEY1G;
                            cmd += suffix - 1;
                            suffix = getbyte(buf, &pos, &nread, 1000);
                            if (suffix == -1)
                                goto done;
                            goto do_suffix;
                        } else /* byte2 == 0x0F7 */ {
                            /* SIZE */
                            int sz;
                            if (byte1 != 0x0F3)
                                goto plain_string;
                            suffix = getbyte(buf, &pos, &nread, 1000);
                            if (suffix == -1)
                                goto done;
                            sz = suffix << 8;
                            suffix = getbyte(buf, &pos, &nread, 1000);
                            if (suffix == -1)
                                goto done;
                            sz += suffix;
                            cmd = CMD_SIZE;
                            arg.type = ARGTYPE_NUM;
                            arg.val.num = sz;
                            goto store;
                        }
                    } else /* flag == 4 */ {
                        /* Illegal value; treat as plain string */
                        goto plain_string;
                    }
                }
            }
        }
        store:
        if (at_end) {
            goto_dot_dot();
            at_end = 0;
        }
        store_command_after(&pc, cmd, &arg);
        need_to_rebuild_label_table = 1;
    }

    done:
    if (need_to_rebuild_label_table) {
        rebuild_label_table();
        update_catalog();
    }

    flags.f.trace_print = saved_trace;
    flags.f.normal_print = saved_normal;
}

void core_copy(char *buf, int buflen) {
    int len = vartype2string(reg_x, buf, buflen - 1);
    buf[len] = 0;
    if (reg_x->type == TYPE_REAL || reg_x->type == TYPE_COMPLEX) {
        /* Convert small-caps 'E' to regular 'e' */
        while (--len >= 0)
            if (buf[len] == 24)
                buf[len] = 'e';
    }
}

static bool is_number_char(char c) {
    return (c >= '0' && c <= '9')
        || c == '.' || c == ','
        || c == '+' || c == '-'
        || c == 'e' || c == 'E' || c == 24;
}

static bool parse_phloat(const char *p, int len, phloat *res) {
    // We can't pass the string on to string2phloat() unchanged, because
    // that function is picky: it does not allow '+' signs, and it does
    // not allow the mantissa to be more than 12 digits long (including
    // leading zeroes). So, we massage the string a bit to make it
    // comply with those restrictions.
    char buf[100];
    bool in_mant = true;
    int mant_digits = 0;
    int i = 0, j = 0;
    while (i < 100 && j < len) {
        char c = p[j++];
        if (c == 0)
            break;
        if (c == '+')
            continue;
        else if (c == 'e' || c == 'E' || c == 24) {
            in_mant = false;
            buf[i++] = 24;
        } else if (c >= '0' && c <= '9') {
            if (!in_mant || mant_digits++ < 12)
                buf[i++] = c;
        } else
            buf[i++] = c;
    }
    int err = string2phloat(buf, i, res);
    if (err == 0)
        return true;
    else if (err == 1) {
        *res = POS_HUGE_PHLOAT;
        return true;
    } else if (err == 2) {
        *res = NEG_HUGE_PHLOAT;
        return true;
    } else if (err == 3 || err == 4) {
        *res = 0;
        return true;
    } else
        return false;
}

void core_paste(const char *buf) {
    phloat re, im;
    int i, s1, e1, s2, e2;
    vartype *v;

    int base = get_base();
    if (base != 10) {
        int bpd = base == 2 ? 1 : base == 8 ? 3 : 4;
        int bits = 0;
        bool neg = false;
        int8 n = 0;
        i = 0;
        while (buf[i] == ' ')
            i++;
        if (buf[i] == '-') {
            neg = true;
            i++;
        }
        while (bits < 36) {
            char c = buf[i++];
            if (c == 0)
                break;
            int d;
            if (base == 16) {
                if (c >= '0' && c <= '9')
                    d = c - '0';
                else if (c >= 'A' && c <= 'F')
                    d = c - 'A' + 10;
                else if (c >= 'a' && c <= 'f')
                    d = c - 'a' + 10;
                else
                    break;
            } else {
                if (c >= 0 && c < '0' + base)
                    d = c - '0';
                else
                    break;
            }
            n = n << bpd | d;
            bits += bpd;
        }
        if (bits == 0)
            goto paste_string;
        if (neg)
            n = -n;
        if ((n & LL(0x800000000)) == 0)
            n &= LL(0x7ffffffff);
        else
            n |= LL(0xfffffff000000000);
        v = new_real((phloat) n);
        goto paste;
    }

    /* Try matching " %g i %g " */
    i = 0;
    while (buf[i] == ' ')
        i++;
    s1 = i;
    while (is_number_char(buf[i]))
        i++;
    e1 = i;
    if (e1 == s1)
        goto attempt_2;
    while (buf[i] == ' ')
        i++;
    if (buf[i] == 'i')
        i++;
    else
        goto attempt_2;
    while (buf[i] == ' ')
        i++;
    s2 = i;
    while (is_number_char(buf[i]))
        i++;
    e2 = i;
    if (e2 == s2)
        goto attempt_2;
    goto finish_complex;

    /* Try matching " %g + %g i " */
    attempt_2:
    i = 0;
    while (buf[i] == ' ')
        i++;
    s1 = i;
    while (is_number_char(buf[i]))
        i++;
    e1 = i;
    if (e1 == s1)
        goto attempt_3;
    while (buf[i] == ' ')
        i++;
    if (buf[i] == '+')
        i++;
    else
        goto attempt_3;
    while (buf[i] == ' ')
        i++;
    s2 = i;
    while (is_number_char(buf[i]))
        i++;
    e2 = i;
    if (e2 == s2)
        goto attempt_3;
    goto finish_complex;

    /* Try matching " ( %g , %g ) " */
    /* To avoid the ambiguity with the comma, a colon or semicolon is
     * also accepted; if those are used, you don't need to surround them
     * with spaces to distinguish them from 'number' chars
     */
    attempt_3:
    i = 0;
    while (buf[i] == ' ')
        i++;
    if (buf[i] == '(')
        i++;
    else
        goto attempt_4;
    while (buf[i] == ' ')
        i++;
    s1 = i;
    while (is_number_char(buf[i]))
        i++;
    e1 = i;
    if (e1 == s1)
        goto attempt_4;
    while (buf[i] == ' ')
        i++;
    if (buf[i] == ',' || buf[i] == ':' || buf[i] == ';')
        i++;
    else
        goto attempt_4;
    while (buf[i] == ' ')
        i++;
    s2 = i;
    while (is_number_char(buf[i]))
        i++;
    e2 = i;
    if (e2 == s2)
        goto attempt_4;
    finish_complex:
    if (!parse_phloat(buf + s1, e1 - s1, &re))
        goto attempt_4;
    if (!parse_phloat(buf + s2, e2 - s2, &im))
        goto attempt_4;
    v = new_complex(re, im);
    goto paste;

    /* Try matching " %g " */
    attempt_4:
    i = 0;
    while (buf[i] == ' ')
        i++;
    s1 = i;
    while (is_number_char(buf[i]))
        i++;
    e1 = i;
    if (e1 != s1 && parse_phloat(buf + s1, e1 - s1, &re))
        v = new_real(re);
    else {
        paste_string:
        int len = 0;
        while (len < 6 && buf[len] != 0)
            len++;
        v = new_string(buf, len);
    }

    paste:
    if (v == NULL) {
        squeak();
        return;
    } else {
        if (!flags.f.prgm_mode)
            mode_number_entry = false;
        recall_result(v);
        flags.f.stack_lift_disable = 0;
        flags.f.message = 0;
        flags.f.two_line_message = 0;
    }
    redisplay();
}

void set_alpha_entry(bool state) {
    mode_alpha_entry = state;
}

void set_running(bool state) {
    if (mode_running != state) {
        mode_running = state;
        shell_annunciators(-1, -1, -1, state, -1, -1);
    }
    if (state) {
        /* Cancel any pending INPUT command */
        input_length = 0;
        mode_goose = -2;
        prgm_highlight_row = 1;
    }
}

bool program_running() {
    return mode_running;
}

void do_interactive(int command) {
    int err;
    if ((cmdlist(command)->flags
                & (flags.f.prgm_mode ? FLAG_NO_PRGM : FLAG_PRGM_ONLY)) != 0) {
        display_error(ERR_RESTRICTED_OPERATION, 0);
        redisplay();
        return;
    }
    if (command == CMD_GOTOROW) {
        err = docmd_stoel(NULL);
        if (err != ERR_NONE) {
            display_error(err, 1);
            redisplay();
            return;
        }
    } else if (command == CMD_A_THRU_F) {
        set_base(16);
        set_menu(MENULEVEL_APP, MENU_BASE_A_THRU_F);
        redisplay();
        return;
    } else if (command == CMD_CLALLa) {
        mode_clall = true;
        set_menu(MENULEVEL_ALPHA, MENU_NONE);
        redisplay();
        return;
    } else if (command == CMD_CLV || command == CMD_PRV) {
        if (!flags.f.prgm_mode && vars_count == 0) {
            display_error(ERR_NO_VARIABLES, 0);
            redisplay();
            return;
        }
    } else if (command == CMD_SST && flags.f.prgm_mode) {
        sst();
        redisplay();
        repeating = 1;
        repeating_shift = 1;
        repeating_key = KEY_DOWN;
        return;
    } else if (command == CMD_BST) {
        bst();
        if (!flags.f.prgm_mode) {
            flags.f.prgm_mode = 1;
            redisplay();
            flags.f.prgm_mode = 0;
            pending_command = CMD_CANCELLED;
        } else
            redisplay();
        repeating = 1;
        repeating_shift = 1;
        repeating_key = KEY_UP;
        return;
    }

    if (flags.f.prgm_mode && (cmdlist(command)->flags & FLAG_IMMED) == 0) {
        if (command == CMD_RUN)
            command = CMD_STOP;
        if (cmdlist(command)->argtype == ARG_NONE) {
            arg_struct arg;
            arg.type = ARGTYPE_NONE;
            store_command_after(&pc, command, &arg);
            if (command == CMD_END)
                pc = 0;
            prgm_highlight_row = 1;
            redisplay();
        } else {
            incomplete_saved_pc = pc;
            incomplete_saved_highlight_row = prgm_highlight_row;
            if (pc == -1)
                pc = 0;
            else if (prgms[current_prgm].text[pc] != CMD_END)
                pc += get_command_length(current_prgm, pc);       
            prgm_highlight_row = 1;
            start_incomplete_command(command);
        }
    } else {
        if (cmdlist(command)->argtype == ARG_NONE)
            pending_command = command;
        else {
            if (flags.f.prgm_mode) {
                incomplete_saved_pc = pc;
                incomplete_saved_highlight_row = prgm_highlight_row;
            }
            start_incomplete_command(command);
        }
    }
}

static void continue_running() {
    int error;
    while (!shell_wants_cpu()) {
        int cmd;
        arg_struct arg;
        oldpc = pc;
        if (pc == -1)
            pc = 0;
        else if (pc >= prgms[current_prgm].size) {
            pc = -1;
            set_running(false);
            return;
        }
        get_next_command(&pc, &cmd, &arg, 1);
        if (flags.f.trace_print && flags.f.printer_exists)
            print_program_line(current_prgm, oldpc);
        mode_disable_stack_lift = false;
        error = cmdlist(cmd)->handler(&arg);
        if (mode_pause) {
            shell_request_timeout3(1000);
            return;
        }
        if (error == ERR_INTERRUPTIBLE)
            return;
        if (!handle_error(error))
            return;
        if (mode_getkey)
            return;
    }
}

typedef struct {
    char name[7];
    int namelen;
    int cmd_id;
} synonym_spec;

static synonym_spec hp41_synonyms[] =
{
    { "/",      1, CMD_DIV     },
    { "*",      1, CMD_MUL     },
    { "CHS",    3, CMD_CHS     },
    { "DEC",    3, CMD_TO_DEC  },
    { "D-R",    3, CMD_TO_RAD  },
    { "ENTER^", 6, CMD_ENTER   },
    { "FACT",   4, CMD_FACT    },
    { "FRC",    3, CMD_FP      },
    { "HMS",    3, CMD_TO_HMS  },
    { "HR",     2, CMD_TO_HR   },
    { "INT",    3, CMD_IP      },
    { "OCT",    3, CMD_TO_OCT  },
    { "P-R",    3, CMD_TO_REC  },
    { "R-D",    3, CMD_TO_DEG  },
    { "RDN",    3, CMD_RDN     },
    { "R-P",    3, CMD_TO_POL  },
    { "ST+",    3, CMD_STO_ADD },
    { "ST/",    3, CMD_STO_DIV },
    { "ST*",    3, CMD_STO_MUL },
    { "ST-",    3, CMD_STO_SUB },
    { "X<=0?",  5, CMD_X_LE_0  },
    { "X<=Y?",  5, CMD_X_LE_Y  },
    { "",       0, CMD_NONE    }
};

int find_builtin(const char *name, int namelen) {
    int i, j;

    for (i = 0; hp41_synonyms[i].cmd_id != CMD_NONE; i++) {
        if (namelen != hp41_synonyms[i].namelen)
            continue;
        for (j = 0; j < namelen; j++)
            if (name[j] != hp41_synonyms[i].name[j])
                goto nomatch1;
        return hp41_synonyms[i].cmd_id;
        nomatch1:;
    }

    for (i = 0; true; i++) {
        if (i == CMD_OPENF && !core_settings.enable_ext_copan) i += 14;
        if (i == CMD_DROP && !core_settings.enable_ext_bigstack) i++;
        if (i == CMD_ACCEL && !core_settings.enable_ext_accel) i++;
        if (i == CMD_LOCAT && !core_settings.enable_ext_locat) i++;
        if (i == CMD_HEADING && !core_settings.enable_ext_heading) i++;
        if (i == CMD_ADATE && !core_settings.enable_ext_time) i += 34;
        if (i == CMD_FPTEST && !core_settings.enable_ext_fptest) i++;
        if (i == CMD_SENTINEL)
            break;
        if ((cmdlist(i)->flags & FLAG_HIDDEN) != 0)
            continue;
        if (cmdlist(i)->name_length != namelen)
            continue;
        for (j = 0; j < namelen; j++) {
            unsigned char c1, c2;
            c1 = name[j];
            if (c1 >= 130 && c1 != 138)
                c1 &= 127;
            c2 = cmdlist(i)->name[j];
            if (c2 >= 130 && c2 != 138)
                c2 &= 127;
            if (c1 != c2)
                goto nomatch2;
        }
        return i;
        nomatch2:;
    }
    return CMD_NONE;
}

void sst() {
    if (pc >= prgms[current_prgm].size - 2) {
        pc = -1;
        prgm_highlight_row = 0;
    } else {
        if (pc == -1)
            pc = 0;
        else
            pc += get_command_length(current_prgm, pc);
        prgm_highlight_row = 1;
    }
}

void bst() {
    int4 line = pc2line(pc);
    if (line == 0) {
        pc = prgms[current_prgm].size - 2;
        prgm_highlight_row = 1;
    } else {
        pc = line2pc(line - 1);
        prgm_highlight_row = 0;
    }
}

void fix_thousands_separators(char *buf, int *bufptr) {
    /* First, remove the old separators... */
    int i, j = 0;
    char dot = flags.f.decimal_point ? '.' : ',';
    char sep = flags.f.decimal_point ? ',' : '.';
    int intdigits = 0;
    int counting_intdigits = 1;
    int nsep;
    for (i = 0; i < *bufptr; i++) {
        char c = buf[i];
        if (c != sep)
            buf[j++] = c;
        if (c == dot || c == 24)
            counting_intdigits = 0;
        else if (counting_intdigits && c >= '0' && c <= '9')
            intdigits++;
    }
    /* Now, put 'em back... */
    if (!flags.f.thousands_separators) {
        *bufptr = j;
        return;
    }
    nsep = (intdigits - 1) / 3;
    if (nsep == 0) {
        *bufptr = j;
        return;
    }
    for (i = j - 1; i >= 0; i--)
        buf[i + nsep] = buf[i];
    j += nsep;
    for (i = 0; i < j; i++) {
        char c = buf[i + nsep];
        buf[i] = c;
        if (nsep > 0 && c >= '0' && c <= '9') {
            if (intdigits % 3 == 1) {
                buf[++i] = sep;
                nsep--;
            }
            intdigits--;
        }
    }
    *bufptr = j;
}

int find_menu_key(int key) {
    switch (key) {
        case KEY_SIGMA: return 0;
        case KEY_INV:   return 1;
        case KEY_SQRT:  return 2;
        case KEY_LOG:   return 3;
        case KEY_LN:    return 4;
        case KEY_XEQ:   return 5;
        default:        return -1;
    }
}

void start_incomplete_command(int cmd_id) {
    int argtype = cmdlist(cmd_id)->argtype;
    if (!flags.f.prgm_mode && (cmdlist(cmd_id)->flags & FLAG_PRGM_ONLY) != 0) {
        display_error(ERR_RESTRICTED_OPERATION, 0);
        redisplay();
        return;
    }
    incomplete_command = cmd_id;
    incomplete_ind = 0;
    if (argtype == ARG_NAMED || argtype == ARG_PRGM
            || argtype == ARG_RVAR || argtype == ARG_MAT)
        incomplete_alpha = 1;
    else
        incomplete_alpha = 0;
    incomplete_length = 0;
    incomplete_num = 0;
    if (argtype == ARG_NUM9)
        incomplete_maxdigits = 1;
    else if (argtype == ARG_COUNT)
        incomplete_maxdigits = cmd_id == CMD_SIMQ ? 2 : 4;
    else
        incomplete_maxdigits = 2;
    incomplete_argtype = argtype;
    mode_command_entry = true;
    if (incomplete_command == CMD_ASSIGNa) {
        set_catalog_menu(CATSECT_TOP);
        flags.f.local_label = 0;
    } else if (argtype == ARG_CKEY)
        set_menu(MENULEVEL_COMMAND, MENU_CUSTOM1);
    else if (argtype == ARG_MKEY)
        set_menu(MENULEVEL_COMMAND, MENU_BLANK);
    else if (argtype == ARG_VAR) {
        if (mode_appmenu == MENU_VARMENU)
            mode_commandmenu = MENU_VARMENU;
        else if (mode_appmenu == MENU_INTEG_PARAMS)
            mode_commandmenu = MENU_INTEG_PARAMS;
        else
            set_catalog_menu(CATSECT_VARS_ONLY);
    } else if (argtype == ARG_NAMED)
        set_catalog_menu(CATSECT_VARS_ONLY);
    else if (argtype == ARG_REAL) {
        if (mode_appmenu == MENU_VARMENU)
            mode_commandmenu = MENU_VARMENU;
        else if (mode_appmenu == MENU_INTEG_PARAMS)
            mode_commandmenu = MENU_INTEG_PARAMS;
        else
            set_catalog_menu(CATSECT_REAL_ONLY);
    } else if (argtype == ARG_RVAR) {
        if (vars_exist(1, 0, 0))
            set_catalog_menu(CATSECT_REAL_ONLY);
        else if (flags.f.prgm_mode) {
            if (incomplete_command == CMD_MVAR)
                mode_commandmenu = MENU_ALPHA1;
        } else {
            mode_command_entry = false;
            display_error(ERR_NO_REAL_VARIABLES, 0);
        }
    } else if (argtype == ARG_MAT) {
        if (flags.f.prgm_mode || vars_exist(0, 0, 1))
            set_catalog_menu(CATSECT_MAT_ONLY);
        else if (cmd_id != CMD_DIM) {
            mode_command_entry = false;
            display_error(ERR_NO_MATRIX_VARIABLES, 0);
        }
    } else if (argtype == ARG_LBL || argtype == ARG_PRGM)
        set_catalog_menu(CATSECT_PGM_ONLY);
    else if (cmd_id == CMD_LBL)
        set_menu(MENULEVEL_COMMAND, MENU_ALPHA1);
    redisplay();
}

void finish_command_entry(bool refresh) {
    if (pending_command == CMD_ASSIGNa) {
        pending_command = CMD_NONE;
        start_incomplete_command(CMD_ASSIGNb);
        return;
    }
    mode_command_entry = false;
    if (flags.f.prgm_mode) {
        set_menu(MENULEVEL_COMMAND, MENU_NONE);
        if (pending_command == CMD_NULL || pending_command == CMD_CANCELLED) {
            pc = incomplete_saved_pc;
            prgm_highlight_row = incomplete_saved_highlight_row;
        } else if (pending_command == CMD_SST || pending_command == CMD_BST) {
            pc = incomplete_saved_pc;
            prgm_highlight_row = incomplete_saved_highlight_row;
            if (pending_command == CMD_SST)
                sst();
            else
                bst();
            repeating = 1;
            repeating_shift = 1;
            repeating_key = pending_command == CMD_SST ? KEY_DOWN : KEY_UP;
            pending_command = CMD_NONE;
            redisplay();
        } else {
            int inserting_an_end = pending_command == CMD_END;
            if ((cmdlist(pending_command)->flags & FLAG_IMMED) != 0)
                goto do_it_now;
            store_command(pc, pending_command, &pending_command_arg);
            if (inserting_an_end)
                /* current_prgm was already incremented by store_command() */
                pc = 0;
            prgm_highlight_row = 1;
            pending_command = CMD_NONE;
            redisplay();
        } 
    } else {
        do_it_now:
        if (refresh)
            redisplay();
        if (mode_commandmenu == MENU_CATALOG
                && (pending_command == CMD_GTO
                    || pending_command == CMD_XEQ)) {
            /* TODO: also applies to CLP, PRP, and maybe more.
             * Does *not* apply to the program menus displayed by VARMENU,
             * PGMINT, PGMSLV, and... (?)
             */
            int catsect = get_cat_section();
            if (catsect == CATSECT_PGM || catsect == CATSECT_PGM_ONLY) {
                int row = get_cat_row();
                set_menu(MENULEVEL_TRANSIENT, mode_commandmenu);
                set_cat_section(catsect);
                set_cat_row(row);
                remove_program_catalog = 1;
            }
        }
        if (pending_command == CMD_ASSIGNb
                || (pending_command >= CMD_ASGN01
                    && pending_command <= CMD_ASGN18)) {
            set_menu(MENULEVEL_PLAIN, mode_commandmenu);
            mode_plainmenu_sticky = true;
        }
        set_menu(MENULEVEL_COMMAND, MENU_NONE);
        if (pending_command == CMD_BST) {
            bst();
            flags.f.prgm_mode = 1;
            redisplay();
            flags.f.prgm_mode = 0;
            pending_command = CMD_CANCELLED;
            repeating = 1;
            repeating_shift = 1;
            repeating_key = KEY_UP;
        }
    }
}

void finish_xeq() {
    int prgm;
    int4 pc;
    int cmd;

    if (find_global_label(&pending_command_arg, &prgm, &pc))
        cmd = CMD_NONE;
    else
        cmd = find_builtin(pending_command_arg.val.text,
                           pending_command_arg.length);

    if (cmd == CMD_CLALLa) {
        mode_clall = true;
        set_menu(MENULEVEL_ALPHA, MENU_NONE);
        mode_command_entry = false;
        pending_command = CMD_NONE;
        redisplay();
        return;
    }

    if (cmd == CMD_NONE)
        finish_command_entry(true);
    else {
        /* Show the entered command in its XEQ "FOO"
         * form, briefly, just to confirm to the user
         * what they've entered; this will be replaced
         * by the matching built-in command after the
         * usual brief delay (timeout1() or the
         * explicit delay, below).
         */
        mode_command_entry = false;
        redisplay();
        if (cmdlist(cmd)->argtype == ARG_NONE) {
            pending_command = cmd;
            pending_command_arg.type = ARGTYPE_NONE;
            finish_command_entry(false);
            return;
        } else {
            shell_delay(250);
            pending_command = CMD_NONE;
            set_menu(MENULEVEL_COMMAND, MENU_NONE);
            if ((cmd == CMD_CLV || cmd == CMD_PRV)
                    && !flags.f.prgm_mode && vars_count == 0) {
                display_error(ERR_NO_VARIABLES, 0);
                pending_command = CMD_NONE;
                redisplay();
            } else
                start_incomplete_command(cmd);
            return;
        }
    }
}

void start_alpha_prgm_line() {
    incomplete_saved_pc = pc;
    incomplete_saved_highlight_row = prgm_highlight_row;
    if (pc == -1)
        pc = 0;
    else if (prgms[current_prgm].text[pc] != CMD_END)
        pc += get_command_length(current_prgm, pc);
    prgm_highlight_row = 1;
    if (cmdline_row == 1)
        display_prgm_line(0, -1);
    entered_string_length = 0;
    mode_alpha_entry = true;
}

void finish_alpha_prgm_line() {
    if (entered_string_length == 0) {
        pc = incomplete_saved_pc;
        prgm_highlight_row = incomplete_saved_highlight_row;
    } else {
        arg_struct arg;
        int i;
        arg.type = ARGTYPE_STR;
        arg.length = entered_string_length;
        for (i = 0; i < entered_string_length; i++)
            arg.val.text[i] = entered_string[i];
        store_command(pc, CMD_STRING, &arg);
        prgm_highlight_row = 1;
    }
    mode_alpha_entry = false;
}

static void stop_interruptible() {
    int error = mode_interruptible(1);
    handle_error(error);
    mode_interruptible = NULL;
    if (mode_running)
        set_running(false);
    else
        shell_annunciators(-1, -1, -1, false, -1, -1);
    pending_command = CMD_NONE;
    redisplay();
}

static int handle_error(int error) {
    if (mode_running) {
        if (error == ERR_RUN)
            error = ERR_NONE;
        if (error == ERR_NONE || error == ERR_NO || error == ERR_YES
                || error == ERR_STOP)
            flags.f.stack_lift_disable = mode_disable_stack_lift;
        if (error == ERR_NO) {
            if (prgms[current_prgm].text[pc] != CMD_END)
                pc += get_command_length(current_prgm, pc);
        } else if (error == ERR_STOP) {
            if (pc >= prgms[current_prgm].size)
                pc = -1;
            set_running(false);
            return 0;
        } else if (error != ERR_NONE && error != ERR_YES) {
            if (flags.f.error_ignore && error != ERR_SUSPICIOUS_OFF) {
                flags.f.error_ignore = 0;
                return 1;
            }
            if (solve_active() && (error == ERR_OUT_OF_RANGE
                                        || error == ERR_DIVIDE_BY_0
                                        || error == ERR_INVALID_DATA
                                        || error == ERR_STAT_MATH_ERROR)) {
                unwind_stack_until_solve();
                error = return_to_solve(1);
                if (error == ERR_STOP)
                    set_running(false);
                if (error == ERR_NONE || error == ERR_RUN || error == ERR_STOP)
                    return 0;
            }
            pc = oldpc;
            display_error(error, 1);
            set_running(false);
            return 0;
        }
        return 1;
    } else if (pending_command == CMD_SST) {
        if (error == ERR_RUN)
            error = ERR_NONE;
        if (error == ERR_NONE || error == ERR_NO || error == ERR_YES
                || error == ERR_STOP)
            flags.f.stack_lift_disable = mode_disable_stack_lift;
        if (error == ERR_NO) {
            if (prgms[current_prgm].text[pc] != CMD_END)
                pc += get_command_length(current_prgm, pc);
            goto noerr;
        } else if (error == ERR_NONE || error == ERR_YES || error == ERR_STOP) {
            noerr:
            error = ERR_NONE;
            if (pc > prgms[current_prgm].size)
                pc = -1;
        } else {
            if (flags.f.error_ignore) {
                flags.f.error_ignore = 0;
                goto noerr;
            }
            if (solve_active() && (error == ERR_OUT_OF_RANGE
                                      || error == ERR_DIVIDE_BY_0
                                      || error == ERR_INVALID_DATA
                                      || error == ERR_STAT_MATH_ERROR)) {
                unwind_stack_until_solve();
                error = return_to_solve(1);
                if (error == ERR_NONE || error == ERR_RUN || error == ERR_STOP)
                    goto noerr;
            }
            pc = oldpc;
            display_error(error, 1);
        }
        return 0;
    } else {
        if (error == ERR_RUN) {
            set_running(true);
            error = ERR_NONE;
        }
        if (error == ERR_NONE || error == ERR_NO || error == ERR_YES
                || error == ERR_STOP)
            flags.f.stack_lift_disable = mode_disable_stack_lift;
        else if (flags.f.error_ignore) {
            flags.f.error_ignore = 0;
            error = ERR_NONE;
        }
        if (error != ERR_NONE && error != ERR_STOP)
            display_error(error, 1);
        return 0;
    }
}
