/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2023  Thomas Okken
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "core_main.h"
#include "core_commands2.h"
#include "core_commands4.h"
#include "core_commands7.h"
#include "core_display.h"
#include "core_display.h"
#include "core_helpers.h"
#include "core_keydown.h"
#include "core_math1.h"
#include "core_sto_rcl.h"
#include "core_tables.h"
#include "core_variables.h"
#include "shell.h"
#include "shell_spool.h"

#ifndef BCD_MATH
// We need these locally for BID128->double conversion
#include "bid_conf.h"
#include "bid_functions.h"
#endif

#ifdef WINDOWS
FILE *my_fopen(const char *name, const char *mode);
int my_rename(const char *oldname, const char *newname);
int my_remove(const char *name);
#else
#define my_fopen fopen
#define my_rename rename
#define my_remove remove
#endif


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

void core_init(int read_saved_state, int4 version, const char *state_file_name, int offset) {

    /* Possible values for read_saved_state:
     * 0: state file not present (Memory Clear)
     * 1: state file present and looks OK so far
     * 2: state file present but not OK (State File Corrupt)
     */

    phloat_init();

    char *state_file_name_crash = NULL;
    if (read_saved_state == 1) {
        // Before loading state, rename the state file by appending .crash
        // to its name. We'll rename it back, right after loading is done.
        // This way, if we crash while loading state, we end up with a
        // renamed state file, so the next time the app is run, it will
        // launch with Memory Clear rather than just crashing on startup over
        // and over, and, on iOS and Android, needing to be deleted and
        // reinstalled.
        size_t bufsize = strlen(state_file_name) + 24;
        state_file_name_crash = (char *) malloc(bufsize);
        uint4 date, time;
        int weekday;
        shell_get_time_date(&time, &date, &weekday);
        snprintf(state_file_name_crash, bufsize, "%s.%08u%08u.crash", state_file_name, date, time);
        my_rename(state_file_name, state_file_name_crash);
        gfile = my_fopen(state_file_name_crash, "rb");
        if (gfile == NULL)
            read_saved_state = 0;
        else if (offset > 0)
            fseek(gfile, offset, SEEK_SET);
    } else
        gfile = NULL;

    bool clear, too_new = false;
    int reason = 0;
    if (read_saved_state != 1 || !load_state(version, &clear, &too_new)) {
        reason = too_new ? 2 : (read_saved_state != 0 && !clear) ? 1 : 0;
        hard_reset(reason);
    }
    if (gfile != NULL)
        fclose(gfile);
    if (state_file_name_crash != NULL) {
        if (reason == 0) {
            my_rename(state_file_name_crash, state_file_name);
        } else {
            char *tmp = (char *) malloc(strlen(state_file_name_crash) + 3);
            strcpy(tmp, state_file_name_crash);
            tmp[strlen(state_file_name_crash) - 6] = 0;
            strcat(tmp, reason == 1 ? ".corrupt" : ".too_new");
            my_rename(state_file_name_crash, tmp);
            free(tmp);
        }
        free(state_file_name_crash);
    }

    repaint_display();
    shell_annunciators(mode_updown,
                       mode_shift,
                       0 /*print*/,
                       mode_running,
                       !flags.f.rad && flags.f.grad,
                       flags.f.rad || flags.f.grad);
}

void core_save_state(const char *state_file_name) {
    if (mode_interruptible != NULL)
        stop_interruptible();
    set_running(false);

    size_t bufsize = strlen(state_file_name) + 24;
    char *state_file_name_crash = (char *) malloc(bufsize);
    uint4 date, time;
    int weekday;
    shell_get_time_date(&time, &date, &weekday);
    snprintf(state_file_name_crash, bufsize, "%s.%08u%08u.crash", state_file_name, date, time);

    gfile = my_fopen(state_file_name_crash, "wb");
    if (gfile != NULL) {
        bool success;
        save_state(&success);
        fclose(gfile);
        if (success) {
            my_remove(state_file_name);
            my_rename(state_file_name_crash, state_file_name);
        }
    }
}

void core_cleanup() {
    for (int i = 0; i <= sp; i++)
        free_vartype(stack[i]);
    sp = -1;
    free(stack);
    stack = NULL;
    stack_capacity = 0;
    free_vartype(lastx);
    lastx = NULL;
    purge_all_vars();
    clear_all_prgms();
    if (vars != NULL) {
        free(vars);
        vars = NULL;
        vars_capacity = 0;
    }
    clean_vartype_pools();
}

void core_repaint_display() {
    repaint_display();
}

bool core_menu() {
    return mode_clall || get_front_menu() != MENU_NONE;
}

bool alpha_active() {
    int menu = get_front_menu();
    return menu >= MENU_ALPHA1 && menu <= MENU_ALPHA_MISC2;
}

bool core_alpha_menu() {
    return !mode_getkey && alpha_active();
}

bool core_hex_menu() {
    return get_front_menu() == MENU_BASE_A_THRU_F;
}

static int ascii2hp(char *dst, int dstlen, const char *src, int srclen = -1);

int core_special_menu_key(int which) {
    return special_menu_key(which);
}

bool core_keydown_command(const char *name, bool *enqueued, int *repeat) {
    char hpname[70];
    int len = ascii2hp(hpname, 63, name);
    int cmd = find_builtin(hpname, len);
    if (cmd == CMD_NONE) {
        set_shift(false);
        squeak();
    }
    return core_keydown(cmd == CMD_NONE ? 0 : cmd + 2048, enqueued, repeat);
}

bool core_keydown(int key, bool *enqueued, int *repeat) {

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
         * the appropriate callback to keep the function moving along
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
        error = mode_interruptible(false);
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
    if (pending_command == CMD_RUN || pending_command == CMD_SST
            || pending_command == CMD_SST_UP || pending_command == CMD_SST_RT) {
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
            && (cmd_array[pending_command].flags & FLAG_NO_SHOW) == 0) {
        display_command(0);
        /* If the program catalog was left up by GTO or XEQ,
         * don't paint over it */
        if (mode_transientmenu == MENU_NONE || pending_command == CMD_NULL) {
            bool saved_prgm_mode = flags.f.prgm_mode;
            flags.f.prgm_mode = 0;
            display_x(1);
            flags.f.prgm_mode = saved_prgm_mode;
        }
        flush_display();
    }
}

void core_keytimeout2() {
    if (pending_command == CMD_LINGER1 || pending_command == CMD_LINGER2)
        return;
    remove_program_catalog = 0;
    if (pending_command != CMD_NONE && pending_command != CMD_CANCELLED
            && (cmd_array[pending_command].flags & FLAG_NO_SHOW) == 0) {
        clear_row(0);
        draw_string(0, 0, "NULL", 4);
        bool saved_prgm_mode = flags.f.prgm_mode;
        flags.f.prgm_mode = 0;
        display_x(1);
        flags.f.prgm_mode = saved_prgm_mode;
        flush_display();
        pending_command = CMD_CANCELLED;
    }
}

bool core_timeout3(bool repaint) {
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

bool core_keyup() {
    if (mode_pause) {
        /* The only way this can happen is if they key in question was Shift */
        return false;
    }

    int error = ERR_NONE;

    if (pending_command == CMD_LINGER1 || pending_command == CMD_LINGER2) {
        pending_command = CMD_LINGER2;
        return mode_running || keybuf_head != keybuf_tail;
    }

    if (pending_command == CMD_SILENT_OFF) {
#ifdef IPHONE
        if (off_enabled()) {
            shell_always_on(0);
            shell_powerdown();
        } else {
            set_running(false);
            squeak();
        }
#else
        shell_always_on(0);
        shell_powerdown();
#endif
        pending_command = CMD_NONE;
        return false;
    }

    if (pending_command == CMD_NONE)
        return mode_running || mode_interruptible != NULL || keybuf_head != keybuf_tail;

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
        if (pending_command == CMD_RUN || pending_command == CMD_SST
                || pending_command == CMD_SST_UP || pending_command == CMD_SST_RT) {
            int err = generic_sto(&input_arg, 0);
            input_length = 0;
            if (err != ERR_NONE) {
                pending_command = CMD_NONE;
                display_error(err, true);
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

    if (pending_command == CMD_VMEXEC || pending_command == CMD_PMEXEC) {
        string_copy(reg_alpha, &reg_alpha_length,
                    pending_command_arg.val.text, pending_command_arg.length);
        goto do_run;
    }
    if (pending_command == CMD_RUN || pending_command == CMD_SST_UP) {
        do_run:
        if ((flags.f.trace_print || flags.f.normal_print)
                && flags.f.printer_exists)
            print_command(pending_command, &pending_command_arg);
        if (pending_command == CMD_SST_UP)
            step_out();
        pending_command = CMD_NONE;
        if (pc == -1)
            pc = 0;
        set_running(true);
        return true;
    }

    if (pending_command == CMD_SST || pending_command == CMD_SST_RT) {
        int cmd;
        arg_struct arg;
        oldpc = pc;
        if (pc == -1)
            pc = 0;
        get_next_command(&pc, &cmd, &arg, 1, NULL);
        if (pending_command == CMD_SST_RT
                && (cmd == CMD_XEQ || cmd == CMD_SOLVE || cmd == CMD_INTEG)) {
            pc = oldpc;
            step_over();
            goto do_run;
        }
        if ((flags.f.trace_print || flags.f.normal_print)
                && flags.f.printer_exists) {
            if (cmd == CMD_LBL)
                print_text(NULL, 0, true);
            print_program_line(current_prgm, oldpc);
        }
        mode_disable_stack_lift = false;
        set_running(true);
        error = handle(cmd, &arg);
        set_running(false);
        mode_pause = false;
    } else {
        if ((flags.f.trace_print || flags.f.normal_print)
                && flags.f.printer_exists)
            print_command(pending_command, &pending_command_arg);
        mode_disable_stack_lift = false;
        error = handle(pending_command, &pending_command_arg);
        mode_pause = false;
    }

    if (error == ERR_INTERRUPTIBLE) {
        shell_annunciators(-1, -1, -1, 1, -1, -1);
        pending_command = CMD_NONE;
        return true;
    }

    handle_error(error);
    pending_command = CMD_NONE;
    if (!mode_getkey && !mode_pause)
        redisplay();
    return (mode_running && !mode_getkey && !mode_pause) || keybuf_head != keybuf_tail;
}

bool core_powercycle() {
    bool need_redisplay = false;

    if (mode_interruptible != NULL)
        stop_interruptible();

    no_keystrokes_yet = true;

    keybuf_tail = keybuf_head;
    set_shift(false);
    #if (!defined(ANDROID) && !defined(IPHONE))
    shell_always_on(0);
    #endif
    pending_command = CMD_NONE;

    if (mode_getkey) {
        /* A real HP-42S can't be switched off while GETKEY is active: pressing
         * OFF on the keyboard returns code 70 and stops program execution; and
         * when the auto-poweroff timeout expires, code 70 is returned but
         * program execution continues.
         * Since Free42 can be shut down in a way the HP-42S can't, by exiting
         * the application, I have to fake it a bit; I put 70 in X as if the
         * user had done OFF twice on a real 42S.
         */
        vartype *seventy = new_real(70);
        if (seventy != NULL) {
            if (recall_result(seventy) != ERR_NONE)
                goto nomem;
            flags.f.stack_lift_disable = 0;
        } else {
            nomem:
            display_error(ERR_INSUFFICIENT_MEMORY, true);
            flags.f.auto_exec = 0;
        }
        if (!flags.f.auto_exec)
            need_redisplay = true;
        mode_getkey = false;
    }

    if (flags.f.big_stack && !core_settings.allow_big_stack) {
        arg_struct dummy_arg;
        docmd_4stk(&dummy_arg);
        need_redisplay = true;
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
                cmdline[cmdline_length] = 0;
                store_command(pc, CMD_NUMBER, &arg, cmdline);
                prgm_highlight_row = 1;
            }
            flags.f.prgm_mode = false;
        }
        mode_alpha_entry = false;
        mode_number_entry = false;
        set_menu(MENULEVEL_ALPHA, MENU_NONE);
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

void core_update_allow_big_stack() {
    if (prgms == NULL)
        // Hack to deal with the Mac version calling this function
        // after already having called core_quit()
        return;
    if (flags.f.big_stack && !core_settings.allow_big_stack) {
        arg_struct dummy_arg;
        docmd_4stk(&dummy_arg);
    }
    redisplay();
}

char *core_list_programs() {
    int bufsize = 1024;
    char *buf = (char *) malloc(bufsize);
    if (buf == NULL)
        return NULL;
    int lastidx = -1;
    int bufptr = 4;
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
            bufsize += 1024;
            char *newbuf = (char *) realloc(buf, bufsize);
            if (newbuf == NULL) {
                if (bufptr > 0 && buf[bufptr - 1] != 0) {
                    buf[bufptr - 1] = 0;
                    count++;
                }
                goto done;
            }
            buf = newbuf;
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
    done:
    buf[0] = (char) (count >> 24);
    buf[1] = (char) (count >> 16);
    buf[2] = (char) (count >> 8);
    buf[3] = (char) count;
    return buf;
}

#ifdef IPHONE

// This would have been a lot cleaner using fmemopen(), but that's only supported
// in iOS 11 and later, and I'm not ready to give up on iOS 8 through 10 yet.

static char *raw_buf;
static size_t raw_size;
static size_t raw_pos;

static int raw_getc() {
    if (raw_buf == NULL)
        return fgetc(gfile);
    else {
        if (raw_pos < raw_size)
            return raw_buf[raw_pos++] & 255;
        else
            return EOF;
    }
}

static int raw_ungetc(int c) {
    if (raw_buf == NULL)
        return ungetc(c, gfile);
    else {
        raw_buf[--raw_pos] = (char) c;
        return c;
    }
}

static size_t raw_write(const char *buf, size_t size) {
    if (raw_buf == NULL)
        return fwrite(buf, 1, size, gfile);
    else {
        if (raw_pos + size > raw_size)
            size = raw_size - raw_pos;
        memcpy(raw_buf + raw_pos, buf, size);
        raw_pos += size;
        return size;
    }
}

static void raw_close(const char *mode) {
    if (raw_buf == NULL) {
        if (ferror(gfile)) {
            char msg[50];
            snprintf(msg, 50, "An error occurred during program %s.", mode);
            shell_message(msg);
        }
        fclose(gfile);
    }
}

#else

#define raw_getc() fgetc(gfile)
#define raw_ungetc(c) ungetc(c, gfile)
#define raw_write(buf, size) fwrite(buf, 1, size, gfile)
#define raw_close(dummy) fclose(gfile)

#endif

static void export_hp42s(int index) {
    int4 pc = 0;
    int cmd;
    arg_struct arg;
    int saved_prgm = current_prgm;
    unsigned char code_flags, code_name, code_std_1, code_std_2;
    char cmdbuf[50];
    int cmdlen;
    char buf[1000];
    int buflen = 0;
    int i;

    current_prgm = index;
    do {
        const char *orig_num;
        get_next_command(&pc, &cmd, &arg, 0, &orig_num);
        code_flags = (cmd_array[cmd].flags & (FLAG_SPECIAL | FLAG_ILLEGAL)) >> 5;
        code_name = cmd_array[cmd].scode;
        code_std_1 = cmd_array[cmd].code1;
        code_std_2 = cmd_array[cmd].code2;
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
                    if (arg.type == ARGTYPE_NUM || arg.type == ARGTYPE_STK
                                                || arg.type == ARGTYPE_LCLBL) {
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
                                        || arg.type == ARGTYPE_STK
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
                    cmdbuf[cmdlen++] = (char) 0xC0;
                    cmdbuf[cmdlen++] = 0x00;
                    cmdbuf[cmdlen++] = 0x0D;
                } else if (cmd == CMD_NUMBER) {
                    const char *p;
                    if (orig_num != NULL)
                        p = orig_num;
                    else
                        p = phloat2program(arg.val_d);
                    char c;
                    while ((c = *p++) != 0) {
                        if (c >= '0' && c <= '9')
                            cmdbuf[cmdlen++] = 0x10 + c - '0';
                        else if (c == '.' || c == ',')
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
                        const command_spec *cs = &cmd_array[arg.val.cmd];
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
                    if (arg.type == ARGTYPE_NUM) {
                        cmdbuf[cmdlen++] = (char) (0xA0 + ((arg.val.num >> 8) & 7));
                        cmdbuf[cmdlen++] = (char) arg.val.num;
                    } else {
                        cmdbuf[cmdlen++] = (char) (0xF0 + arg.length);
                        for (int i = 0; i < arg.length; i++)
                            cmdbuf[cmdlen++] = arg.val.text[i];
                    }
                } else if (cmd == CMD_XSTR) {
                    int len = arg.length;
                    if (len == 0) {
                        cmdbuf[cmdlen++] = (char) 0xF2;
                        cmdbuf[cmdlen++] = (char) 0xA7;
                        cmdbuf[cmdlen++] = (char) 0x41;
                    } else {
                        /* Writing directly to 'buf' here, not using 'cmdbuf', */
                        /* since XSTR can be very long and may not fit in one  */
                        /* piece.                                              */
                        const char *ptr = arg.val.xstr;
                        while (len > 0) {
                            if (buflen + 16 > 1000 - 50) {
                                if (raw_write(buf, buflen) != buflen)
                                    goto done;
                                buflen = 0;
                            }
                            int slen = len <= 13 ? len : 13;
                            buf[buflen++] = (char) (0xF2 + slen);
                            buf[buflen++] = (char) 0xA7;
                            buf[buflen++] = (char) (slen < len ? 0x49 : 0x41);
                            memcpy(buf + buflen, ptr, slen);
                            buflen += slen;
                            ptr += slen;
                            len -= slen;
                        }
                    }
                } else {
                    /* Shouldn't happen */
                    continue;
                }
                break;
            case 0:
            normal:
                if (arg.type == ARGTYPE_STR || arg.type == ARGTYPE_IND_STR) {
                    int i;
                    if ((code_name & 0x80) == 0) {
                        cmdbuf[cmdlen++] = 0xf0 + arg.length + 2;
                        cmdbuf[cmdlen++] = 0xa7;
                    } else {
                        cmdbuf[cmdlen++] = 0xf0 + arg.length + 1;
                    }
                    cmdbuf[cmdlen++] = arg.type == ARGTYPE_STR ? code_name
                                                            : code_name + 8;
                    for (i = 0; i < arg.length; i++)
                        cmdbuf[cmdlen++] = arg.val.text[i];
                } else {
                    unsigned char suffix;
                    if (code_std_1 != 0) {
                        if (code_std_1 == 0xf2 && (code_std_2 & 0x80) == 0) {
                            cmdbuf[cmdlen++] = 0xf3;
                            cmdbuf[cmdlen++] = 0xa7;
                        } else {
                            cmdbuf[cmdlen++] = code_std_1;
                        }
                    }
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
        if (buflen + cmdlen > 1000 - 50) {
            if (raw_write(buf, buflen) != buflen)
                goto done;
            buflen = 0;
        }
        for (i = 0; i < cmdlen; i++)
            buf[buflen++] = cmdbuf[i];
    } while (cmd != CMD_END && pc < prgms[index].size);
    if (buflen > 0)
        raw_write(buf, buflen);
    done:
    current_prgm = saved_prgm;
}

int4 core_program_size(int prgm_index) {
    int4 pc = 0;
    int cmd;
    arg_struct arg;
    int saved_prgm = current_prgm;
    unsigned char code_flags, code_std_1, code_std_2;
    //unsigned char code_name;
    int4 size = 0;

    current_prgm = prgm_index;
    do {
        const char *orig_num;
        get_next_command(&pc, &cmd, &arg, 0, &orig_num);
        code_flags = (cmd_array[cmd].flags & (FLAG_SPECIAL | FLAG_ILLEGAL)) >> 5;
        //code_name = cmd_array[cmd].scode;
        code_std_1 = cmd_array[cmd].code1;
        code_std_2 = cmd_array[cmd].code2;
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
                    if (arg.type == ARGTYPE_NUM || arg.type == ARGTYPE_STK
                                                || arg.type == ARGTYPE_LCLBL) {
                        size += 3;
                    } else if (arg.type == ARGTYPE_STR) {
                        size += arg.length + 2;
                    } else
                        goto normal;
                } else if (cmd == CMD_GTO) {
                    if (arg.type == ARGTYPE_NUM && arg.val.num <= 14) {
                        size += 2;
                    } else if (arg.type == ARGTYPE_NUM
                                        || arg.type == ARGTYPE_STK
                                        || arg.type == ARGTYPE_LCLBL) {
                        size += 3;
                    } else if (arg.type == ARGTYPE_STR) {
                        size += arg.length + 2;
                    } else
                        goto normal;
                } else if (cmd == CMD_END) {
                    /* Not counted for the line 00 total */
                } else if (cmd == CMD_NUMBER) {
                    const char *p;
                    if (orig_num != NULL)
                        p = orig_num;
                    else
                        p = phloat2program(arg.val_d);
                    size += strlen(p) + 1;
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
                        size += cmd_array[arg.val.cmd].name_length + 3;
                } else if ((cmd >= CMD_KEY1G && cmd <= CMD_KEY9G)
                            || (cmd >= CMD_KEY1X && cmd <= CMD_KEY9X)) {
                    if (arg.type == ARGTYPE_STR || arg.type == ARGTYPE_IND_STR)
                        size += arg.length + 3;
                    else
                        size += 4;
                } else if (cmd == CMD_XROM) {
                    if (arg.type == ARGTYPE_NUM)
                        size += 2;
                    else
                        size += arg.length + 1;
                } else if (cmd == CMD_XSTR) {
                    int n = (arg.length + 12) / 13;
                    if (n == 0)
                        n = 1;
                    size += arg.length + n * 3;
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
                if (code_std_1 == 0xf2 && (code_std_2 & 0x80) == 0)
                    /* Extra extension */
                    size += 1;
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

void core_export_programs(int count, const int *indexes, const char *raw_file_name) {
    if (raw_file_name != NULL) {
#ifdef IPHONE
        if (strncmp(raw_file_name, "mem:", 4) == 0) {
            char *buf;
            ssize_t size;
            memcpy(&buf, raw_file_name + 5, sizeof(char *));
            memcpy(&size, raw_file_name + 13, sizeof(ssize_t));
            raw_buf = buf;
            raw_size = size;
            raw_pos = 0;
        } else {
            raw_buf = NULL;
#endif
            gfile = my_fopen(raw_file_name, "wb");
            if (gfile == NULL) {
                char msg[1024];
                int err = errno;
                snprintf(msg, 1024, "Could not open \"%s\" for writing: %s (%d)", raw_file_name, strerror(err), err);
                shell_message(msg);
                return;
            }
#ifdef IPHONE
        }
    } else {
        raw_buf = NULL;
#endif
    }
    for (int i = 0; i < count; i++) {
        int p = indexes[i];
        export_hp42s(p);
    }
    if (raw_file_name != NULL)
        raw_close("export");
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
    CMD_NOP  | 0x0000,
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
     * 4 = illegal
     * Values 80-ff are the space used on the HP-42S, that is, strings whose
     * first character has its high bit set. Since the gaps left in that
     * scheme ran out when development of Plus42 started, additional
     * extensions had to be defined, and they are encoded using a7 as the
     * first character, and the opcode in the second character. By keeping
     * that second-character opcode in the range 00-7f, we can still
     * use a single byte lookup for all these extensions.
     */

    /* 00-0F */
    CMD_NULL    | 0x4000,
    CMD_XASTO   | 0x0000,
    CMD_LXASTO  | 0x0000,
    CMD_HEAD    | 0x0000,
    CMD_X_EQ_NN | 0x0000,
    CMD_X_NE_NN | 0x0000,
    CMD_X_LT_NN | 0x0000,
    CMD_X_GT_NN | 0x0000,
    CMD_NULL    | 0x4000,
    CMD_XASTO   | 0x1000,
    CMD_LXASTO  | 0x1000,
    CMD_HEAD    | 0x1000,
    CMD_X_EQ_NN | 0x1000,
    CMD_X_NE_NN | 0x1000,
    CMD_X_LT_NN | 0x1000,
    CMD_X_GT_NN | 0x1000,

    /* 10-1F */
    CMD_NULL    | 0x4000,
    CMD_XASTO   | 0x2000,
    CMD_LXASTO  | 0x2000,
    CMD_HEAD    | 0x2000,
    CMD_X_EQ_NN | 0x2000,
    CMD_X_NE_NN | 0x2000,
    CMD_X_LT_NN | 0x2000,
    CMD_X_GT_NN | 0x2000,
    CMD_X_LE_NN | 0x2000,
    CMD_X_GE_NN | 0x2000,
    CMD_0_EQ_NN | 0x2000,
    CMD_0_NE_NN | 0x2000,
    CMD_0_LT_NN | 0x2000,
    CMD_0_GT_NN | 0x2000,
    CMD_0_LE_NN | 0x2000,
    CMD_0_GE_NN | 0x2000,

    /* 20-2F */
    CMD_X_LE_NN | 0x0000,
    CMD_X_GE_NN | 0x0000,
    CMD_0_EQ_NN | 0x0000,
    CMD_0_NE_NN | 0x0000,
    CMD_0_LT_NN | 0x0000,
    CMD_0_GT_NN | 0x0000,
    CMD_0_LE_NN | 0x0000,
    CMD_0_GE_NN | 0x0000,
    CMD_X_LE_NN | 0x1000,
    CMD_X_GE_NN | 0x1000,
    CMD_0_EQ_NN | 0x1000,
    CMD_0_NE_NN | 0x1000,
    CMD_0_LT_NN | 0x1000,
    CMD_0_GT_NN | 0x1000,
    CMD_0_LE_NN | 0x1000,
    CMD_0_GE_NN | 0x1000,

    /* 30-3F */
    CMD_PGMVAR  | 0x2000,
    CMD_VARMNU1 | 0x2000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,

    /* 40-4F */
    CMD_PGMVAR  | 0x0000,
    CMD_XSTR    | 0x0000,
    CMD_VARMNU1 | 0x0000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_PGMVAR  | 0x1000,
    CMD_XSTR    | 0x1000,
    CMD_VARMNU1 | 0x1000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,
    CMD_NULL    | 0x4000,

    /* 50-5F */
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,

    /* 60-6F */
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_LCLV | 0x2000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,

    /* 70-7F */
    CMD_NULL | 0x4000,
    CMD_LCLV | 0x0000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_LCLV | 0x1000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,
    CMD_NULL | 0x4000,

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
    CMD_RTNERR | 0x2000,
    CMD_DROPN  | 0x2000,
    CMD_DUPN   | 0x2000,
    CMD_PICK   | 0x2000,
    CMD_UNPICK | 0x2000,
    CMD_RDNN   | 0x2000,
    CMD_RUPN   | 0x2000,
    CMD_NULL   | 0x3000, /* Extra extensions */
    CMD_SF     | 0x1000,
    CMD_CF     | 0x1000,
    CMD_FSC_T  | 0x1000,
    CMD_FCC_T  | 0x1000,
    CMD_FS_T   | 0x1000,
    CMD_FC_T   | 0x1000,
    CMD_GTO    | 0x1000,
    CMD_XEQ    | 0x1000,

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
    CMD_LSTO    | 0x0000,
    CMD_LASTO   | 0x2000,
    CMD_VARMENU | 0x1000,
    CMD_NULL    | 0x3000, /* KEYX IND name */
    CMD_NULL    | 0x3000, /* KEYG IND name */
    CMD_DIM     | 0x1000,
    CMD_INPUT   | 0x1000,
    CMD_EDITN   | 0x1000,
    CMD_LSTO    | 0x1000,

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
    CMD_FUNC   | 0x2000,
    CMD_NULL   | 0x4000,
    CMD_NULL   | 0x3000, /* KEYX suffix */
    CMD_NULL   | 0x3000, /* KEYG suffix */
    CMD_RTNERR | 0x1000,
    CMD_NULL   | 0x4000, /* FIX 11 */
    CMD_NULL   | 0x4000, /* SCI 11 */
    CMD_NULL   | 0x4000, /* ENG 11 */
    CMD_PGMINT | 0x2000,
    CMD_PGMSLV | 0x2000,
    CMD_INTEG  | 0x2000,
    CMD_SOLVE  | 0x2000,
    CMD_DIM    | 0x2000,
    CMD_LSTO   | 0x2000,
    CMD_INPUT  | 0x2000,
    CMD_EDITN  | 0x2000,

    /* F0-FF */
    CMD_CLP     | 0x0000,
    CMD_NULL    | 0x4000, /* XFCN */
    CMD_NULL    | 0x4000, /* GTO . nnnn */
    CMD_NULL    | 0x4000, /* GTO .. */
    CMD_NULL    | 0x4000, /* GTO . "name" */
    CMD_LASTO   | 0x0000,
    CMD_NULL    | 0x4000, /* DEL */
    CMD_NULL    | 0x3000, /* SIZE */
    CMD_VARMENU | 0x2000,
    CMD_DROPN   | 0x1000,
    CMD_DUPN    | 0x1000,
    CMD_PICK    | 0x1000,
    CMD_UNPICK  | 0x1000,
    CMD_LASTO   | 0x1000,
    CMD_RDNN    | 0x1000,
    CMD_RUPN    | 0x1000
};


static bool looks_like_zero(const char *buf) {
    char c;
    while ((c = *buf++) != 0) {
        if (c >= '1' && c <= '9')
            return false;
        if (c == 'E')
            return true;
    }
    return true;
}

static phloat parse_number_line(char *buf) {
    phloat res;
    if (buf[0] == 'E' || buf[0] == '-' && buf[1] == 'E') {
        char *buf2 = (char *) malloc(strlen(buf) + 2);
        strcpy(buf2 + 1, buf);
        if (buf[0] == 'E') {
            buf2[0] = '1';
        } else {
            buf2[0] = '-';
            buf2[1] = '1';
        }
        int len2 = (int) strlen(buf2);
        if (buf2[len2 - 1] == 'E')
            buf2[len2 - 1] = 0;
        res = parse_number_line(buf2);
        free(buf2);
        return res;
    }
#ifdef BCD_MATH
    res = Phloat(buf);
#else
    BID_UINT128 d;
    bid128_from_string(&d, buf);
    bid128_to_binary64(&res, &d);
#endif
    if (p_isinf(res) != 0)
        res = NAN_1_PHLOAT;
    else if (res == 0 && !looks_like_zero(buf))
        res = NAN_2_PHLOAT;
    return res;
}

void core_import_programs(int num_progs, const char *raw_file_name) {
    int i;

    int byte1, byte2, suffix;
    int cmd, flag, str_len;
    int done_flag = 0;
    arg_struct arg;
    int assign = 0;
    bool pending_end;

    if (raw_file_name != NULL) {
#ifdef IPHONE
        if (strncmp(raw_file_name, "mem:", 4) == 0) {
            char *buf;
            ssize_t size;
            memcpy(&buf, raw_file_name + 5, sizeof(char *));
            memcpy(&size, raw_file_name + 13, sizeof(ssize_t));
            raw_buf = buf;
            raw_size = size;
            raw_pos = 0;
        } else {
            raw_buf = NULL;
#endif
            gfile = my_fopen(raw_file_name, "rb");
            if (gfile == NULL) {
                char msg[1024];
                int err = errno;
                snprintf(msg, 1024, "Could not open \"%s\" for reading: %s (%d)", raw_file_name, strerror(err), err);
                shell_message(msg);
                return;
            }
#ifdef IPHONE
        }
    } else {
        raw_buf = NULL;
#endif
    }

    set_running(false);

    /* Set print mode to MAN during the import, to prevent store_command()
     * from printing programs as they load
     */
    int saved_trace = flags.f.trace_print;
    int saved_normal = flags.f.normal_print;
    flags.f.trace_print = 0;
    flags.f.normal_print = 0;

    if (num_progs > 0) {
        // Loading state file
        goto_dot_dot(true);
        pending_end = false;
    } else {
        current_prgm = prgms_count - 1;
        pc = prgms[current_prgm].size - 2;
        // No initial END needed if last program is empty
        pending_end = pc > 0;
    }

    char numbuf[50];
    char *xstr_buf = NULL;
    int xstr_len = 0;

    while (!done_flag) {
        skip:
        byte1 = raw_getc();
        if (byte1 == EOF)
            goto done;
        cmd = hp42tofree42[byte1];
        flag = cmd >> 12;
        cmd &= 0x0FFF;
        bool extra_extension = false;
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
            suffix = raw_getc();
            if (suffix == EOF)
                goto done;
            goto do_suffix;
        } else /* flag == 3 */ {
            if (byte1 == 0x00)
                /* NULL */
                goto skip;
            else if (byte1 >= 0x10 && byte1 <= 0x1C) {
                /* Number */
                int numlen = 0;
                do {
                    if (byte1 == 0x1A)
                        byte1 = '.';
                    else if (byte1 == 0x1B)
                        byte1 = 'E';
                    else if (byte1 == 0x1C)
                        byte1 = '-';
                    else
                        byte1 += '0' - 0x10;
                    numbuf[numlen++] = byte1;
                    byte1 = raw_getc();
                } while (byte1 >= 0x10 && byte1 <= 0x1C);
                if (byte1 == EOF)
                    done_flag = 1;
                else if (byte1 != 0x00)
                    raw_ungetc(byte1);
                numbuf[numlen++] = 0;
                arg.val_d = parse_number_line(numbuf);
                cmd = CMD_NUMBER;
                arg.type = ARGTYPE_DOUBLE;
            } else if (byte1 == 0x1D || byte1 == 0x1E) {
                cmd = byte1 == 0x1D ? CMD_GTO : CMD_XEQ;
                str_len = raw_getc();
                if (str_len == EOF)
                    goto done;
                else if (str_len < 0x0F1) {
                    raw_ungetc(str_len);
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
                 * on the cmd_array table.
                 */
                uint4 code;
                byte2 = raw_getc();
                if (byte2 == EOF)
                    goto done;
                code = (((unsigned int) byte1) << 8) | byte2;
                if (code >= 0x0A7DB && code <= 0x0A7DD) {
                    /* FUNC0, FUNC1, FUNC2: translate to FUNC nn */
                    cmd = CMD_FUNC;
                    arg.type = ARGTYPE_NUM;
                    if (code == 0x0A7DB)
                        arg.val.num = 0;
                    else if (code == 0x0A7DC)
                        arg.val.num = 11;
                    else // code == 0x0A7DD
                        arg.val.num = 21;
                    goto store;
                } else if (code == 0x0A7E0) {
                    /* Unparameterized RTNERR: translate to RTNERR IND ST X */
                    cmd = CMD_RTNERR;
                    arg.type = ARGTYPE_IND_STK;
                    arg.val.stk = 'X';
                    goto store;
                } else if (code >= 0x0a679 && code <= 0x0a67e) {
                    /* HP-41CX: X=NN? etc. */
                    switch (code & 0xf) {
                        case 0x9: cmd = CMD_X_EQ_NN; break;
                        case 0xa: cmd = CMD_X_NE_NN; break;
                        case 0xb: cmd = CMD_X_LT_NN; break;
                        case 0xc: cmd = CMD_X_LE_NN; break;
                        case 0xd: cmd = CMD_X_GT_NN; break;
                        case 0xe: cmd = CMD_X_GE_NN; break;
                    }
                    arg.type = ARGTYPE_IND_STK;
                    arg.val.stk = 'Y';
                    goto store;
                }
                for (i = 0; i < CMD_SENTINEL; i++)
                    if (cmd_array[i].code1 == byte1 && cmd_array[i].code2 == byte2) {
                        if ((cmd_array[i].flags & FLAG_HIDDEN) != 0)
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
                suffix = raw_getc();
                if (suffix == EOF)
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
                byte2 = raw_getc();
                if (byte2 == EOF)
                    goto done;
                cmd = CMD_GTO;
                arg.type = ARGTYPE_NUM;
                arg.val.num = (byte1 & 15) - 1;
                goto store;
            } else if (byte1 >= 0x0C0 && byte1 <= 0x0CD) {
                /* GLOBAL */
                byte2 = raw_getc();
                if (byte2 == EOF)
                    goto done;
                str_len = raw_getc();
                if (str_len == EOF)
                    goto done;
                if (str_len < 0x0F1) {
                    /* END */
                    cmd = CMD_END;
                    arg.type = ARGTYPE_NONE;
                    goto store;
                } else {
                    /* LBL "" */
                    str_len -= 0x0F1;
                    byte2 = raw_getc();
                    if (byte2 == EOF)
                        goto done;
                    cmd = CMD_LBL;
                    arg.type = ARGTYPE_STR;
                    goto do_string;
                }
            } else if (byte1 >= 0x0D0 && byte1 <= 0x0EF) {
                /* 3-byte GTO & XEQ */
                byte2 = raw_getc();
                if (byte2 == EOF)
                    goto done;
                suffix = raw_getc();
                if (suffix == EOF)
                    goto done;
                cmd = byte1 <= 0x0DF ? CMD_GTO : CMD_XEQ;
                suffix &= 0x7F;
                goto do_suffix;
            } else /* byte1 >= 0xF1 && byte1 <= 0xFF */ {
                /* Strings and parameterized HP-42S extensions */
                byte2 = raw_getc();
                if (byte2 == EOF)
                    goto done;
                if ((byte2 & 0x080) == 0) {
                    /* String */
                    int i;
                    cmd = CMD_STRING;
                    goto string_2;
                    xrom_string:
                    cmd = CMD_XROM;
                    string_2:
                    str_len = byte1 - 0x0F0;
                    raw_ungetc(byte2);
                    arg.type = ARGTYPE_STR;
                    do_string:
                    for (i = 0; i < str_len; i++) {
                        suffix = raw_getc();
                        if (suffix == EOF)
                            goto done;
                        arg.val.text[i] = suffix;
                    }
                    if (extra_extension) {
                        memmove(arg.val.text + 1, arg.val.text, str_len);
                        arg.val.text[0] = 0xa7;
                        str_len++;
                    }
                    arg.length = str_len;
                    if (assign) {
                        assign = 0;
                        suffix = raw_getc();
                        if (suffix == EOF)
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
                    goto store;
                } else {
                    /* Parameterized HP-42S extension */
                    if (byte1 == 0x0F1) {
                        switch (byte2) {
                            case 0x0D5: cmd = CMD_FIX; arg.val.num = 10; break;
                            case 0x0D6: cmd = CMD_SCI; arg.val.num = 10; break;
                            case 0x0D7: cmd = CMD_ENG; arg.val.num = 10; break;
                            case 0x0E5: cmd = CMD_FIX; arg.val.num = 11; break;
                            case 0x0E6: cmd = CMD_SCI; arg.val.num = 11; break;
                            case 0x0E7: cmd = CMD_ENG; arg.val.num = 11; break;
                            default: goto xrom_string;
                        }
                        arg.type = ARGTYPE_NUM;
                        goto store;
                    }
                    if (byte2 == 0xa7) {
                        byte2 = raw_getc();
                        if (byte2 == EOF)
                            goto done;
                        byte1--;
                        extra_extension = true;
                    }
                    cmd = hp42ext[byte2];
                    flag = cmd >> 12;
                    cmd &= 0x0FFF;
                    if (flag == 0 || flag == 1) {
                        arg.type = flag == 0 ? ARGTYPE_STR
                                                : ARGTYPE_IND_STR;
                        str_len = byte1 - 0x0F1;
                        extra_extension = false;
                        if (cmd != CMD_XSTR)
                            goto do_string;
                        // XSTR is stored as a sequence of instructions, since
                        // it may encode a string of up to 65535 characters, while
                        // instructions are limited to 16 bytes, giving a payload
                        // of up to 13 characters per instruction.
                        char *newbuf = (char *) realloc(xstr_buf, xstr_len + str_len);
                        if (newbuf == NULL)
                            goto done;
                        xstr_buf = newbuf;
                        while (str_len-- > 0) {
                            int b = raw_getc();
                            if (b == EOF)
                                goto done;
                            xstr_buf[xstr_len++] = b;
                        }
                        if (arg.type == ARGTYPE_IND_STR)
                            continue;
                        arg.type = ARGTYPE_XSTR;
                        arg.length = xstr_len;
                        arg.val.xstr = xstr_buf;
                        goto store;
                    } else if (flag == 2) {
                        int ind;
                        if (byte1 != 0x0F2)
                            goto xrom_string;
                        suffix = raw_getc();
                        if (suffix == EOF)
                            goto done;
                        do_suffix:
                        ind = (suffix & 0x080) != 0;
                        suffix &= 0x7F;
                        if (!ind && (cmd == CMD_LBL || cmd == CMD_GTO || cmd == CMD_XEQ
                                    || cmd >= CMD_KEY1G && cmd <= CMD_KEY9X)
                                && suffix >= 102 && suffix <= 111) {
                            arg.type = ARGTYPE_LCLBL;
                            arg.val.lclbl = 'A' + (suffix - 102);
                        } else if (!ind && (cmd == CMD_LBL || cmd == CMD_GTO || cmd == CMD_XEQ
                                    || cmd >= CMD_KEY1G && cmd <= CMD_KEY9X)
                                && suffix >= 123) {
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
                            assign = 1;
                            cmd = CMD_ASGN01;
                            arg.type = ARGTYPE_STR;
                            goto do_string;
                        } else if (byte2 == 0x0C2 || byte2 == 0x0C3
                                || byte2 == 0x0CA || byte2 == 0x0CB) {
                            /* KEYG/KEYX name, KEYG/KEYX IND name */
                            str_len = byte1 - 0x0F2;
                            if (str_len == 0)
                                goto xrom_string;
                            cmd = byte2 == 0x0C2 || byte2 == 0x0CA
                                    ? CMD_KEY1X : CMD_KEY1G;
                            suffix = raw_getc();
                            if (suffix == EOF)
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
                                    int c = raw_getc();
                                    if (c == EOF)
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
                                goto xrom_string;
                            suffix = raw_getc();
                            if (suffix == EOF)
                                goto done;
                            if (suffix < 1 || suffix > 9)
                                goto bad_keyg_keyx;
                            cmd = byte2 == 0x0E2 ? CMD_KEY1X : CMD_KEY1G;
                            cmd += suffix - 1;
                            suffix = raw_getc();
                            if (suffix == EOF)
                                goto done;
                            goto do_suffix;
                        } else /* byte2 == 0x0F7 */ {
                            /* SIZE */
                            int sz;
                            if (byte1 != 0x0F3)
                                goto xrom_string;
                            suffix = raw_getc();
                            if (suffix == EOF)
                                goto done;
                            sz = suffix << 8;
                            suffix = raw_getc();
                            if (suffix == EOF)
                                goto done;
                            sz += suffix;
                            cmd = CMD_SIZE;
                            arg.type = ARGTYPE_NUM;
                            arg.val.num = sz;
                            goto store;
                        }
                    } else /* flag == 4 */ {
                        /* Unknown value; store as string XROM */
                        goto xrom_string;
                    }
                }
            }
        }
        store:
        if (pending_end) {
            goto_dot_dot(true);
            pending_end = false;
        }
        if (cmd == CMD_END) {
            if (num_progs > 0 && --num_progs == 0)
                break;
            pending_end = true;
        } else {
            store_command_after(&pc, cmd, &arg, numbuf);
            free(xstr_buf);
            xstr_buf = NULL;
            xstr_len = 0;
        }
    }

    done:
    rebuild_label_table();
    if (!loading_state)
        update_catalog();

    flags.f.trace_print = saved_trace;
    flags.f.normal_print = saved_normal;

    if (raw_file_name != NULL)
        raw_close("import");
    free(xstr_buf);
}

static int real2buf(char *buf, phloat x, const char *format = NULL, bool force_decimal = true) {
    int bufptr = phloat2string(x, buf, 49, force_decimal ? 0 : 1, 0, 3, 0, MAX_MANT_DIGITS, format);
    /* Convert small-caps 'E' to regular 'e' */
    for (int i = 0; i < bufptr; i++)
        if (buf[i] == 24)
            buf[i] = 'e';
    return bufptr;
}

static int complex2buf(char *buf, phloat re, phloat im, bool always_rect, const char *format = NULL) {
    bool polar = !always_rect && flags.f.polar;
    phloat x, y;
    if (polar) {
        generic_r2p(re, im, &x, &y);
        if (p_isinf(x))
            x = POS_HUGE_PHLOAT;
    } else {
        x = re;
        y = im;
    }
    int bufptr = phloat2string(x, buf, 99, 0, 0, 3, 0, MAX_MANT_DIGITS, format);
    if (polar) {
        string2buf(buf, 99, &bufptr, " \342\210\240 ", 5);
    } else {
        if (y >= 0 || p_isinf(y) != 0 || p_isnan(y))
            buf[bufptr++] = '+';
    }
    bufptr += phloat2string(y, buf + bufptr, 99 - bufptr, 0, 0, 3, 0, MAX_MANT_DIGITS, format);
    if (!polar)
        buf[bufptr++] = 'i';
    /* Convert small-caps 'E' to regular 'e' */
    for (int i = 0; i < bufptr; i++)
        if (buf[i] == 24)
            buf[i] = 'e';
    return bufptr;
}

static void serialize_list(textbuf *tb, vartype_list *list, int indent) {
    char buf[50];
    tb_indent(tb, indent);
    tb_write(tb, "{\n", 2);
    indent += 2;
    tb_indent(tb, indent);
    int n = int2string(list->size, buf, 49);
    tb_write(tb, buf, n);
    tb_write(tb, "-Elem List\n", 11);
    for (int i = 0; i < list->size; i++) {
        vartype *elem = list->array->data[i];
        switch (elem->type) {
            case TYPE_NULL: {
                tb_indent(tb, indent);
                tb_write(tb, "null\n", 5);
                break;
            }
            case TYPE_REAL: {
                vartype_real *r = (vartype_real *) elem;
                tb_indent(tb, indent);
                n = real2buf(buf, r->x);
                tb_write(tb, buf, n);
                tb_write(tb, "\n", 1);
                break;
            }
            case TYPE_COMPLEX: {
                vartype_complex *c = (vartype_complex *) elem;
                tb_indent(tb, indent);
                n = complex2buf(buf, c->re, c->im, true);
                tb_write(tb, buf, n);
                tb_write(tb, "\n", 1);
                break;
            }
            case TYPE_STRING: {
                vartype_string *s = (vartype_string *) elem;
                tb_indent(tb, indent);
                tb_write(tb, "\"", 1);
                const char *txt = s->txt();
                char cbuf[5];
                for (int j = 0; j < s->length; j++) {
                    unsigned char c = txt[j];
                    if (c == 10)
                        c = 138;
                    else if (c >= 130 && c != 138)
                        c &= 127;
                    if (c == '"') {
                        tb_write(tb, "\\\"", 2);
                    } else if (c == '\\') {
                        tb_write(tb, "\\\\", 2);
                    } else {
                        n = hp2ascii(cbuf, (const char *) &c, 1);
                        tb_write(tb, cbuf, n);
                    }
                }
                tb_write(tb, "\"\n", 2);
                break;
            }
            case TYPE_REALMATRIX: {
                vartype_realmatrix *rm = (vartype_realmatrix *) elem;
                tb_indent(tb, indent);
                tb_write(tb, "[\n", 2);
                indent += 2;
                tb_indent(tb, indent);
                n = int2string(rm->rows, buf, 49);
                tb_write(tb, buf, n);
                tb_write(tb, "x", 1);
                n = int2string(rm->columns, buf, 49);
                tb_write(tb, buf, n);
                tb_write(tb, " Matrix\n", 8);
                for (int j = 0; j < rm->rows * rm->columns; j++) {
                    tb_indent(tb, indent);
                    if (rm->array->is_string[j]) {
                        tb_write(tb, "\"", 1);
                        char *text;
                        int4 len;
                        get_matrix_string(rm, j, &text, &len);
                        char cbuf[5];
                        for (int k = 0; k < len; k++) {
                            unsigned char c = text[k];
                            if (c == 10)
                                c = 138;
                            else if (c >= 130 && c != 138)
                                c &= 127;
                            if (c == '"') {
                                tb_write(tb, "\\\"", 2);
                            } else if (c == '\\') {
                                tb_write(tb, "\\\\", 2);
                            } else {
                                n = hp2ascii(cbuf, (const char *) &c, 1);
                                tb_write(tb, cbuf, n);
                            }
                        }
                        tb_write(tb, "\"\n", 2);
                    } else {
                        n = real2buf(buf, rm->array->data[j]);
                        tb_write(tb, buf, n);
                        tb_write(tb, "\n", 1);
                    }
                }
                indent -= 2;
                tb_indent(tb, indent);
                tb_write(tb, "]\n", 2);
                break;
            }
            case TYPE_COMPLEXMATRIX: {
                vartype_complexmatrix *cm = (vartype_complexmatrix *) elem;
                tb_indent(tb, indent);
                tb_write(tb, "[\n", 2);
                indent += 2;
                tb_indent(tb, indent);
                n = int2string(cm->rows, buf, 49);
                tb_write(tb, buf, n);
                tb_write(tb, "x", 1);
                n = int2string(cm->columns, buf, 49);
                tb_write(tb, buf, n);
                tb_write(tb, " Cpx Matrix\n", 12);
                for (int j = 0; j < cm->rows * cm->columns * 2; j += 2) {
                    tb_indent(tb, indent);
                    n = complex2buf(buf, cm->array->data[j], cm->array->data[j + 1], true);
                    tb_write(tb, buf, n);
                    tb_write(tb, "\n", 1);
                }
                indent -= 2;
                tb_indent(tb, indent);
                tb_write(tb, "]\n", 2);
                break;
            }
            case TYPE_LIST: {
                serialize_list(tb, (vartype_list *) elem, indent);
                break;
            }
        }
    }
    indent -= 2;
    tb_indent(tb, indent);
    tb_write(tb, "}\n", 2);
}

char *core_copy() {
    if (mode_interruptible != NULL)
        stop_interruptible();
    set_running(false);

    textbuf tb;
    tb.buf = NULL;
    tb.size = 0;
    tb.capacity = 0;
    tb.fail = false;

    if (flags.f.prgm_mode) {
        tb_print_current_program(&tb);
        textbuf_finish:
        tb_write_null(&tb);
        if (tb.fail) {
            free(tb.buf);
            display_error(ERR_INSUFFICIENT_MEMORY, false);
            redisplay();
            return NULL;
        } else
            return tb.buf;
    } else if (alpha_active()) {
        char *buf = (char *) malloc(5 * reg_alpha_length + 1);
        int bufptr = hp2ascii(buf, reg_alpha, reg_alpha_length);
        buf[bufptr] = 0;
        return buf;
    } else if (sp == -1) {
        char *buf = (char *) malloc(1);
        buf[0] = 0;
        return buf;
    } else if (stack[sp]->type == TYPE_REAL) {
        const char *format = core_settings.localized_copy_paste ? number_format() : NULL;
        char *buf = (char *) malloc(50);
        int bufptr = real2buf(buf, ((vartype_real *) stack[sp])->x, format, false);
        buf[bufptr] = 0;
        return buf;
    } else if (stack[sp]->type == TYPE_COMPLEX) {
        const char *format = core_settings.localized_copy_paste ? number_format() : NULL;
        char *buf = (char *) malloc(100);
        vartype_complex *c = (vartype_complex *) stack[sp];
        int bufptr = complex2buf(buf, c->re, c->im, false, format);
        buf[bufptr] = 0;
        return buf;
    } else if (stack[sp]->type == TYPE_STRING) {
        vartype_string *s = (vartype_string *) stack[sp];
        char *buf = (char *) malloc(5 * s->length + 1);
        int bufptr = hp2ascii(buf, s->txt(), s->length);
        buf[bufptr] = 0;
        return buf;
    } else if (stack[sp]->type == TYPE_REALMATRIX) {
        const char *format = core_settings.localized_copy_paste ? number_format() : NULL;
        vartype_realmatrix *rm = (vartype_realmatrix *) stack[sp];
        phloat *data = rm->array->data;
        char *is_string = rm->array->is_string;
        char buf[50];
        int n = 0;
        for (int r = 0; r < rm->rows; r++) {
            for (int c = 0; c < rm->columns; c++) {
                int bufptr;
                if (is_string[n] == 0) {
                    bufptr = real2buf(buf, data[n], format);
                    tb_write(&tb, buf, bufptr);
                } else {
                    char *text;
                    int4 len;
                    get_matrix_string(rm, n, &text, &len);
                    for (int4 i = 0; i < len; i += 10) {
                        int4 seg_len = len - i;
                        if (seg_len > 10)
                            seg_len = 10;
                        bufptr = hp2ascii(buf, text + i, seg_len);
                        tb_write(&tb, buf, bufptr);
                    }
                }
                if (c < rm->columns - 1)
                    tb_write(&tb, "\t", 1);
                n++;
            }
            if (r < rm->rows - 1)
                tb_write(&tb, "\n", 1);
        }
        goto textbuf_finish;
    } else if (stack[sp]->type == TYPE_COMPLEXMATRIX) {
        const char *format = core_settings.localized_copy_paste ? number_format() : NULL;
        vartype_complexmatrix *cm = (vartype_complexmatrix *) stack[sp];
        phloat *data = cm->array->data;
        char buf[100];
        int n = 0;
        for (int r = 0; r < cm->rows; r++) {
            for (int c = 0; c < cm->columns; c++) {
                int bufptr = complex2buf(buf, data[n], data[n + 1], true, format);
                if (c < cm->columns - 1)
                    buf[bufptr++] = '\t';
                tb_write(&tb, buf, bufptr);
                n += 2;
            }
            if (r < cm->rows - 1)
                tb_write(&tb, "\n", 1);
        }
        goto textbuf_finish;
    } else if (stack[sp]->type == TYPE_LIST) {
        serialize_list(&tb, (vartype_list *) stack[sp], 0);
        goto textbuf_finish;
    } else {
        // Shouldn't happen: unrecognized data type
        return NULL;
    }
}

const char *STR_INF = "<Infinity>";
const char *STR_NEG_INF = "<-Infinity>";
const char *STR_NAN = "<Not a Number>";

static int scan_number(const char *buf, int len, int pos, const char *format, bool no_sep = false) {
    if (buf[pos] == '<' || len > 1 && (buf[pos] == '-' || buf[pos] == '+') && buf[pos + 1] == '<') {
        int off = buf[pos] == '<' ? 0 : 1;
        if (len >= 10 + off && strncmp(buf + pos + off, STR_INF, 10) == 0)
            return pos + off + 10;
        else if (len >= 11 + off && strncmp(buf + pos + off, STR_NEG_INF, 11) == 0)
            return pos + off + 11;
        else if (len >= 14 + off && strncmp(buf + pos + off, STR_NAN, 14) == 0)
            return pos + off + 14;
        else
            return pos;
    }
    // 0: before number
    // 1: in mantissa, before decimal
    // 2: in mantissa, after decimal
    // 3: after E
    // 4: in exponent
    int state = 0;
    char dec, sep;
    if (format == NULL) {
        dec = flags.f.decimal_point ? '.' : ',';
        sep = flags.f.decimal_point ? ',' : '.';
    } else {
        dec = format[0];
        sep = format[1];
    }
    for (int p = pos; p < len; p++) {
        char c = buf[p];
        switch (state) {
            case 0:
                if (c >= '0' && c <= '9' || c == '+' || c == '-')
                    state = 1;
                else if (c == dec)
                    state = 2;
                else if (c == 'e' || c == 'E' || c == 24)
                    state = 3;
                else
                    return p;
                break;
            case 1:
                if (c >= '0' && c <= '9' || !no_sep && (c == sep || c == ' '))
                    /* state = 1 */;
                else if (c == dec)
                    state = 2;
                else if (c == 'e' || c == 'E' || c == 24)
                    state = 3;
                else
                    return p;
                break;
            case 2:
                if (c >= '0' && c <= '9')
                    /* state = 2 */;
                else if (c == 'e' || c == 'E' || c == 24)
                    state = 3;
                else
                    return p;
                break;
            case 3:
                if ((c >= '0' && c <= '9')
                        || c == '+' || c == '-')
                    state = 4;
                else
                    return p;
                break;
            case 4:
                if (c >= '0' && c <= '9')
                    /* state = 4 */;
                else
                    return p;
                break;
        }
    }
    return len;
}

static bool parse_phloat(const char *p, int len, phloat *res, const char *format) {
    if (p[0] == '<' || len > 1 && (p[0] == '-' || p[0] == '+') && p[1] == '<') {
        int off = p[0] == '<' ? 0 : 1;
        bool neg = p[0] == '-';
        if (len >= 10 + off && strncmp(p + off, STR_INF, 10) == 0) {
            goto return_inf;
        } else if (len >= 11 + off && strncmp(p + off, STR_NEG_INF, 11) == 0) {
            neg = !neg;
            return_inf:
            *res = (neg ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT) * 2;
            return true;
        } else if (len >= 14 + off && strncmp(p + off, STR_NAN, 14) == 0) {
            *res = NAN_PHLOAT;
            return true;
        } else
            return false;
    }
    // We can't pass the string on to string2phloat() unchanged, because
    // that function is picky: it does not allow '+' signs, and it does
    // not allow the mantissa to be more than 34 or 16 digits long (including
    // leading zeroes). So, we massage the string a bit to make it
    // comply with those restrictions.
    // Note that this function does not check well-formedness;
    // that part should be checked beforehand using scan_number().
    const size_t BSIZE = 100;
    char buf[BSIZE];
    bool in_mant = true;
    bool leading_zero = true;
    int exp_offset = 0;
    int exp = 0;
    bool neg_exp = false;
    int mant_digits = 0;
    bool in_int_mant = true;
    bool empty_mant = true;
    int i = 0, j = 0;
    char decimal, separator;
    if (format == NULL) {
        decimal = flags.f.decimal_point ? '.' : ',';
        separator = flags.f.decimal_point ? ',' : '.';
    } else {
        decimal = format[0];
        separator = format[1];
    }
    while (i < BSIZE - 1 && j < len) {
        char c = p[j++];
        if (c == 0)
            break;
        if (c == '+' || c == ' ' || c == separator) {
            continue;
        } else if (c == 'e' || c == 'E' || c == 24) {
            if (!empty_mant && mant_digits == 0)
                buf[i++] = '0';
            in_mant = false;
        } else if (c == decimal) {
            in_int_mant = false;
            buf[i++] = flags.f.decimal_point ? '.' : ',';
        } else if (c >= '0' && c <= '9') {
            if (in_mant) {
                empty_mant = false;
                if (in_int_mant) {
                    if (mant_digits < MAX_MANT_DIGITS) {
                        if (c != '0' || !leading_zero) {
                            buf[i++] = c;
                            leading_zero = false;
                            mant_digits++;
                        }
                    } else {
                        exp_offset++;
                    }
                } else {
                    if (mant_digits < MAX_MANT_DIGITS) {
                        if (c != '0' || !leading_zero) {
                            buf[i++] = c;
                            leading_zero = false;
                            mant_digits++;
                        } else {
                            exp_offset--;
                        }
                    }
                }
            } else {
                exp = exp * 10 + c - '0';
            }
        } else if (c == '-') {
            if (in_mant)
                buf[i++] = c;
            else
                neg_exp = true;
        }
    }
    if (in_mant && empty_mant)
        return false;
    if (in_mant && mant_digits == 0 && i < BSIZE - 1)
        buf[i++] = '0';
    if (neg_exp)
        exp = -exp;
    exp += exp_offset;
    if (exp != 0) {
        i += snprintf(buf + i, BSIZE - i, "\030%d", exp);
        if (i > BSIZE - 1)
            i = BSIZE - 1;
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

/* NOTE: The destination buffer should be able to store dstlen + 4
 * characters, because of how we parse [LF] and [ESC].
 */
static int ascii2hp(char *dst, int dstlen, const char *src, int srclen /* = -1 */) {
    int srcpos = 0, dstpos = 0;
    // state machine for detecting [LF] and [ESC]:
    // 0: ''
    // 1: '['
    // 2: '[L'
    // 3: '[LF'
    // 4: '[E'
    // 5: '[ES'
    // 6: '[ESC'
    int state = 0;
    bool afterCR = false;
    while (dstpos < dstlen + (state == 0 ? 1 : 4)) {
        char c = srclen == -1 || srcpos < srclen ? src[srcpos++] : 0;
        retry:
        if (c == 0)
            break;
        int code;
        if ((c & 0x80) == 0) {
            code = c;
        } else if ((c & 0xc0) == 0x80) {
            // Unexpected continuation byte
            continue;
        } else {
            int len;
            if ((c & 0xe0) == 0xc0) {
                len = 1;
                code = c & 0x1f;
            } else if ((c & 0xf0) == 0xe0) {
                len = 2;
                code = c & 0x0f;
            } else if ((c & 0xf8) == 0xf0) {
                len = 3;
                code = c & 0x07;
            } else {
                // Invalid UTF-8
                continue;
            }
            while (len-- > 0) {
                c = srclen == -1 || srcpos < srclen ? src[srcpos++] : 0;
                if ((c & 0xc0) != 0x80)
                    // Unexpected non-continuation byte
                    goto retry;
                code = code << 6 | c & 0x3f;
            }
        }
        // OK, we have a code.
        // Next, we translate CR to LF, but whenever that happens,
        // any immediately following LF should be dropped
        if (code == 13) {
            code = 10;
            afterCR = true;
        } else {
            bool prevAfterCR = afterCR;
            afterCR = false;
            if (prevAfterCR && code == 10)
                continue;
        }
        // Perform the inverse of the translation in hp2ascii()
        switch (code) {
            case 0x00f7: code =   0; break; // division sign
            case 0x00d7: code =   1; break; // multiplication sign
            case 0x221a: code =   2; break; // square root sign
            case 0x222b: code =   3; break; // integral sign
            case 0x2592: code =   4; break; // gray rectangle
            case 0x03a3:                    // Uppercase sigma
            case 0x2211: code =   5; break; // n-ary summation sign (i41CX)
            case 0x25b6:                    // right-pointing triangle
            case 0x25b8:                    // small right-pointing triangle
            case 0x25c6: code =   6; break; // black diamond (HP-41 LBL marker)
            case 0x03c0: code =   7; break; // lowercase pi
            case 0x00bf: code =   8; break; // upside-down question mark
            case 0x2264: code =   9; break; // less-than-or-equals sign
            case 0x2265: code =  11; break; // greater-than-or-equals sign
            case 0x2260: code =  12; break; // not-equals sign
            case 0x21b5: code =  13; break; // down-then-left arrow
            case 0x2193: code =  14; break; // downward-pointing arrow
            case 0x2192: code =  15; break; // right-pointing arrow
            case 0x2190: code =  16; break; // left-pointing arrow
            case 0x00b5:                    // micro sign
            case 0x03bc: code =  17; break; // lowercase mu
            case 0x00a3:                    // pound sterling sign
            case 0x20a4: code =  18; break; // lira sign (Emu42)
            case 0x00b0: code =  19; break; // degree symbol
            case 0x00c5:                    // uppercase a with ring
            case 0x00e5:                    // lowercase a with ring
            case 0x0226:                    // uppercase a with dot (i41CX)
            case 0x0227: code =  20; break; // lowercase a with dot (i41CX)
            case 0x00d1:                    // uppercase n with tilde
            case 0x00f1: code =  21; break; // lowercase n with tilde
            case 0x00c4:                    // uppercase a with umlaut
            case 0x00e4: code =  22; break; // lowercase a with umlaut
            case 0x2220:                    // angle symbol
            case 0x2221: code =  23; break; // measured angle symbol
            case 0x1d07: code =  24; break; // small-caps e
            case 0x00c6:                    // uppercase ae ligature
            case 0x00e6:                    // lowercase ae ligature
            case 0x1d01: code =  25; break; // small-caps ae ligature (i41CX)
            case 0x2025:                    // two-dot leader (Emu42)
            case 0x2026: code =  26; break; // ellipsis
            case 0x241b: code =  27; break; // ESC symbol
            case 0x00d6:                    // uppercase o with umlaut
            case 0x00f6: code =  28; break; // lowercase o with umlaut
            case 0x00dc:                    // uppercase u with umlaut
            case 0x00fc: code =  29; break; // lowercase u with umlaut
            case 0x00b7:                    // middle dot (Emu42)
            case 0x2022: code =  31; break; // bullet
            case 0x201c:                    // left curly double quote
            case 0x201d: code =  34; break; // right curly double quote
            case 0x2018:                    // left curly single quote
            case 0x2019: code =  39; break; // right curly single quote
            case 0x2191: code =  94; break; // upward-pointing arrow
            case 0x2212: code =  45; break; // minus sign
            case 0x22a2:                    // right tack sign (i41CX)
            case 0x22a6:                    // assertion sign (Emu42)
            case 0x251c: code = 127; break; // append sign
            case 0x028f: code = 129; break; // small-caps y
            case 0x240a: code = 138; break; // LF symbol
            // Combining accents: apply them if they fit,
            // otherwise ignore them
            case 0x0303:
                if (dstpos > 0 && dst[dstpos - 1] == 'N') {
                    code = 21;
                    dstpos--;
                } else {
                    state = 0;
                    continue;
                }
                break;
            case 0x0308:
                if (dstpos > 0) {
                    char k = dst[dstpos - 1];
                    if (k == 'A') {
                        code = 22;
                        dstpos--;
                    } else if (k == 'O') {
                        code = 28;
                        dstpos--;
                    } else if (k == 'U') {
                        code = 29;
                        dstpos--;
                    } else {
                        state = 0;
                        continue;
                    }
                } else {
                    state = 0;
                    continue;
                }
                break;
            case 0x030a:
                if (dstpos > 0 && dst[dstpos - 1] == 'A') {
                    code = 20;
                    dstpos--;
                } else {
                    state = 0;
                    continue;
                }
                break;
            case 0x20ac:
                // euro symbol; expect it to be followed by two hex digits,
                // encoding a character in the HP-42S character set. These
                // are used by the reverse translation, hp2ascii(), for HP-42S
                // characters that only have ambiguous translations.
                if (srclen == -1 ? src[srcpos] != 0 && src[srcpos + 1] != 0 : srcpos + 1 < srclen) {
                    char c1 = src[srcpos];
                    char c2 = src[srcpos + 1];
                    if (c1 >= '0' && c1 <= '9')
                        c1 = c1 - '0';
                    else if (c1 >= 'a' && c1 <= 'f')
                        c1 = c1 - 'a' + 10;
                    else if (c1 >= 'A' && c1 <= 'F')
                        c1 = c1 - 'A' + 10;
                    else
                        goto noescape;
                    if (c2 >= '0' && c2 <= '9')
                        c2 = c2 - '0';
                    else if (c2 >= 'a' && c2 <= 'f')
                        c2 = c2 - 'a' + 10;
                    else if (c2 >= 'A' && c2 <= 'F')
                        c2 = c2 - 'A' + 10;
                    else
                        goto noescape;
                    code = (c1 << 4) + c2;
                    srcpos += 2;
                } else {
                    noescape:
                    code = 31;
                }
                break;
            default:
                // Anything outside of the printable ASCII range or LF or
                // ESC is not representable, so we replace it with bullets,
                // except for combining diacritics and zero-width spaces,
                // which we skip, and tabs and various other whitespace
                // characters, which we treat as spaces.
                if (code >= 0x0300 && code <= 0x036f || code >= 0x200b && code <= 0x200d) {
                    state = 0;
                    continue;
                }
                if (code >= 9 && code <= 13 // ASCII whitespace chars
                        || code == 0x85 || code == 0xa0 // Latin-1 line break, non-break space
                        || code >= 0x2000 && code <= 0x200a // Unicode spaces
                        || code == 0x2028 // Unicode line separator
                        || code == 0x2029 // Unicode paragraph separator
                        || code == 0x202f // Narrow no-break space
                        )
                    code = 32;
                else if (code < 32 && code != 10 && code != 27 || code > 126)
                    code = 31;
                break;
        }
        switch (state) {
            case 0:
                if (code == '[')
                    state = 1;
                break;
            case 1:
                if (code == 'L')
                    state = 2;
                else if (code == 'E')
                    state = 4;
                else
                    state = 0;
                break;
            case 2:
                if (code == 'F')
                    state = 3;
                else
                    state = 0;
                break;
            case 3:
                if (code == ']') {
                    code = 10;
                    dstpos -= 3;
                }
                state = 0;
                break;
            case 4:
                if (code == 'S')
                    state = 5;
                else
                    state = 0;
                break;
            case 5:
                if (code == 'C')
                    state = 6;
                else
                    state = 0;
                break;
            case 6:
                if (code == ']') {
                    code = 27;
                    dstpos -= 4;
                }
                state = 0;
                break;
        }
        dst[dstpos++] = (char) code;
    }
    return dstpos > dstlen ? dstlen : dstpos;
}

struct text_alias {
    char len;
    char equiv;
    char text[8];
};

static text_alias aliases[] = {
    { 5,    2, "\\sqrt"   },
    { 4,    3, "\\int"    },
    { 6,    4, "\\gray1"  },
    { 6,    5, "\\Sigma"  },
    { 3,    7, "\\pi"     },
    { 2,    9, "<="       },
    { 3,   10, "\\LF"     },
    { 2,   11, ">="       },
    { 2,   12, "!="       },
    { 2,   15, "->"       },
    { 2,   16, "<-"       },
    { 6,   23, "\\angle"  },
    { 4,   26, "\\esc"    },
    { 6,   30, "\\gray2"  },
    { 7,   31, "\\bullet" },
    { 2, '\\', "\\\\"     },
    { 2,  127, "|-"       },
    { 1,   17, "\265"     },
    { 0,    0, ""         }
};

static int text2hp(char *buf, int len) {
    int srcpos = 0;
    int dstpos = 0;
    while (srcpos < len) {
        int al;
        for (int i = 0; (al = aliases[i].len) != 0; i++) {
            if (srcpos + al > len)
                continue;
            if (strncmp(buf + srcpos, aliases[i].text, al) == 0) {
                buf[dstpos++] = aliases[i].equiv;
                srcpos += al;
                break;
            }
        }
        if (al == 0)
            buf[dstpos++] = buf[srcpos++];
    }
    return dstpos;
}

static vartype *parse_base(const char *buf, int len) {
    int base = get_base();
    if (base == 10)
        return NULL;
    int bpd = base == 2 ? 1 : base == 8 ? 3 : 4;
    int bits = 0;
    bool neg = false;
    int8 n = 0;
    int i = 0;
    while (i < len && buf[i] == ' ')
        i++;
    if (i < len && buf[i] == '-') {
        neg = true;
        i++;
    }
    while (i < len && bits < 64) {
        char c = buf[i++];
        int d;
        if (base == 16) {
            if (c >= '0' && c <= '9')
                d = c - '0';
            else if (c >= 'A' && c <= 'F')
                d = c - 'A' + 10;
            else if (c >= 'a' && c <= 'f')
                d = c - 'a' + 10;
            else
                return NULL;
        } else {
            if (c >= 0 && c < '0' + base)
                d = c - '0';
            else
                return NULL;
        }
        n = n << bpd | d;
        bits += bpd;
    }
    while (i < len && buf[i] == ' ')
        i++;
    if (i < len)
        return NULL;
    if (bits == 0)
        return NULL;
    if (neg)
        n = -n;
    base_range_check(&n, true);
    return new_real((phloat) n);
}

static int parse_scalar(const char *buf, int len, bool strict, phloat *re, phloat *im, int *slen, const char *format = NULL) {
    int i, s1, e1, s2, e2;
    bool polar = false;
    bool empty_im = false;
    bool no_re = false;

    /* Try matching " %g <angle> %g " */
    i = 0;
    while (i < len && buf[i] == ' ')
        i++;
    s1 = i;
    i = scan_number(buf, len, i, format);
    e1 = i;
    if (e1 == s1)
        goto attempt_2;
    while (i < len && buf[i] == ' ')
        i++;
    if (i < len && buf[i] == 23)
        i++;
    else
        goto attempt_2;
    while (i < len && buf[i] == ' ')
        i++;
    s2 = i;
    i = scan_number(buf, len, i, format);
    e2 = i;
    if (e2 == s2)
        goto attempt_2;
    while (i < len && buf[i] == ' ')
        i++;
    if (i < len)
        goto attempt_2;
    polar = true;
    goto finish_complex;

    /* Try matching " %g[+-]%g[ij] " */
    attempt_2:
    i = 0;
    while (i < len && buf[i] == ' ')
        i++;
    s1 = i;
    i = scan_number(buf, len, i, format);
    e1 = i;
    s2 = i;
    i = scan_number(buf, len, i, format);
    e2 = i;
    if (i < len && (buf[i] == 'i' || buf[i] == 'j'))
        i++;
    else
        goto attempt_3;
    while (i < len && buf[i] == ' ')
        i++;
    if (i < len)
        goto attempt_3;
    if (e1 == s1) {
        *re = 0;
        *im = 1;
        return TYPE_COMPLEX;
    }
    if (e2 == s2) {
        no_re = true;
        e2 = e1;
        s2 = s1;
    }
    /* Handle x+i or x-i (imaginary part consisting of just a '+' or '-' */
    if (e2 == s2 + 1 && (buf[s2] == '+' || buf[s2] == '-'))
        empty_im = true;
    goto finish_complex;

    /* Try matching " ( %g , %g ) " */
    /* To avoid the ambiguity with the comma, a colon or semicolon is
     * also accepted; if those are used, you don't need to surround them
     * with spaces to distinguish them from 'number' chars
     */
    attempt_3:
    no_re = false;
    i = 0;
    while (i < len && buf[i] == ' ')
        i++;
    if (i < len && buf[i] == '(')
        i++;
    else
        goto attempt_4;
    while (i < len && buf[i] == ' ')
        i++;
    s1 = i;
    i = scan_number(buf, len, i, format, true);
    e1 = i;
    if (e1 == s1)
        goto attempt_4;
    while (i < len && buf[i] == ' ')
        i++;
    if (i < len || (buf[i] == ',' || buf[i] == ':' || buf[i] == ';'))
        i++;
    else
        goto attempt_4;
    while (i < len && buf[i] == ' ')
        i++;
    s2 = i;
    i = scan_number(buf, len, i, format, true);
    e2 = i;
    if (e2 == s2)
        goto attempt_4;
    while (i < len && buf[i] == ' ')
        i++;
    if (i < len && buf[i] == ')')
        i++;
    else
        goto attempt_4;
    while (i < len && buf[i] == ' ')
        i++;
    if (i < len)
        goto attempt_4;

    finish_complex:
    if (no_re)
        *re = 0;
    else if (!parse_phloat(buf + s1, e1 - s1, re, format))
        goto attempt_4;
    if (empty_im)
        *im = buf[s2] == '+' ? 1 : -1;
    else if (!parse_phloat(buf + s2, e2 - s2, im, format))
        goto attempt_4;
    if (polar)
        generic_p2r(*re, *im, re, im);
    return TYPE_COMPLEX;

    /* Try matching " %g " */
    attempt_4:
    i = 0;
    while (i < len && buf[i] == ' ')
        i++;
    s1 = i;
    i = scan_number(buf, len, i, format);
    e1 = i;
    if (e1 == s1)
        goto finish_string;
    if (strict) {
        while (i < len && buf[i] == ' ')
            i++;
        if (i < len)
            goto finish_string;
    }
    if (parse_phloat(buf + s1, e1 - s1, re, format))
        return TYPE_REAL;

    finish_string:
    *slen = len;
    return TYPE_STRING;
}

static bool nexttoken(const char *buf, int pos, int len, int *tok_start, int *tok_end) {
    bool have_token = false;
    while (pos < len) {
        char c = buf[pos];
        if (have_token) {
            if (c == ' ') {
                *tok_end = pos;
                return true;
            }
        } else {
            if (c != ' ') {
                *tok_start = pos;
                have_token = true;
            }
        }
        pos++;
    }
    *tok_end = pos;
    return have_token;
}

static void paste_programs(const char *buf) {
    bool after_end = true;
    bool done = false;
    int pos = 0;
    char hpbuf_s[259];
    char *hpbuf = NULL;
    int cmd;
    arg_struct arg;
    char numbuf[50];

    while (!done) {
        int end = pos;
        char c;
        while (c = buf[end], c != 0 && c != '\r' && c != '\n' && c != '\f')
            end++;
        if (c == 0)
            done = true;
        if (end == pos)
            goto line_done;
        // We now have a line between 'pos' and 'end', length 'end - pos'.
        // Convert to HP-42S encoding:
        int alen;
        alen = end - pos;
        if (alen > 255) {
            hpbuf = (char *) malloc(alen + 4);
            if (hpbuf == NULL) {
                display_error(ERR_INSUFFICIENT_MEMORY, false);
                redisplay();
                return;
            }
        } else {
            hpbuf = hpbuf_s;
        }
        int hpend;
        hpend = ascii2hp(hpbuf, alen, buf + pos, alen);
        // Perform additional translations, to support various 42S-to-text
        // and 41-to-text conversion schemes:
        hpend = text2hp(hpbuf, hpend);
        // Skip leading whitespace and line number.
        int hppos;
        hppos = 0;
        while (hpbuf[hppos] == ' ')
            hppos++;
        int prev_hppos, lineno_start, lineno_end;
        prev_hppos = hppos;
        lineno_start = -1;
        while (hppos < hpend && (c = hpbuf[hppos], c >= '0' && c <= '9'))
            hppos++;
        if (prev_hppos != hppos) {
            // Number found. If this is immediately followed by a period,
            // comma, or E, it's not a line number but an unnumbered number
            // line.
            if (hppos < hpend && (c = hpbuf[hppos], c == '.' || c == ','
                            || c == 'E' || c == 'e' || c == 24)) {
                int len = hpend - prev_hppos;
                if (len > 50)
                    len = 50;
                int i;
                for (i = 0; i < len; i++) {
                    c = hpbuf[prev_hppos + i];
                    if (c == ' ')
                        break;
                    if (c == 'e' || c == 24)
                        c = 'E';
                    else if (c == ',')
                        c = '.';
                    numbuf[i] = c;
                }
                if (i == 50)
                    // Too long
                    goto line_done;
                numbuf[i] = 0;
                cmd = CMD_NUMBER;
                arg.val_d = parse_number_line(numbuf);
                arg.type = ARGTYPE_DOUBLE;
                goto store;
            } else {
                // Check for 1/X, 10^X, 4STK, and generalized comparisons with 0
                int len = hpend - prev_hppos;
                if ((len == 3 || len > 3 && hpbuf[3] == ' ')
                        && strncmp(hpbuf + prev_hppos, "1/X", 3) == 0) {
                    cmd = CMD_INV;
                    arg.type = ARGTYPE_NONE;
                    goto store;
                } else if ((len == 4 || len > 4 && hpbuf[4] == ' ')
                        && strncmp(hpbuf + prev_hppos, "10^X", 4) == 0) {
                    cmd = CMD_10_POW_X;
                    arg.type = ARGTYPE_NONE;
                    goto store;
                } else if ((len == 4 || len > 4 && hpbuf[4] == ' ')
                        && strncmp(hpbuf + prev_hppos, "4STK", 4) == 0) {
                    cmd = CMD_4STK;
                    arg.type = ARGTYPE_NONE;
                    goto store;
                } else if (len >= 4 && hpbuf[prev_hppos] == '0'
                                    && hpbuf[prev_hppos + 2] == '?'
                                    && hpbuf[prev_hppos + 3] == ' ') {
                    switch (hpbuf[prev_hppos + 1]) {
                        case '=':    cmd = CMD_0_EQ_NN; goto parse_arg;
                        case '\014': cmd = CMD_0_NE_NN; goto parse_arg;
                        case '<':    cmd = CMD_0_LT_NN; goto parse_arg;
                        case '>':    cmd = CMD_0_GT_NN; goto parse_arg;
                        case '\011': cmd = CMD_0_LE_NN; goto parse_arg;
                        case '\013': cmd = CMD_0_GE_NN; goto parse_arg;
                        default: goto not_zero_comp;
                    }
                    parse_arg:
                    hppos = prev_hppos;
                    goto after_line_number;
                    not_zero_comp:;
                }
                // No decimal or exponent following the digits, and it's
                // not 1/X, 10^X, or 4STK; for now, assume it's a line number.
                lineno_start = prev_hppos;
                lineno_end = hppos;
            }
        }
        // Line number should be followed by a run of one or more characters,
        // which may be spaces, greater-than signs, or solid right-pointing
        // triangle (a.k.a. goose), but all but one of those characters must
        // be spaces
        bool goose;
        goose = false;
        prev_hppos = hppos;
        while (hppos < hpend) {
            c = hpbuf[hppos];
            if (c == '>' || c == 6) {
                if (goose)
                    break;
                else
                    goose = 1;
            } else if (c != ' ')
                break;
            hppos++;
        }
        // Now hppos should be pointing at the first character of the
        // command.
        after_line_number:
        if (hppos == hpend) {
            if (lineno_start == -1) {
                // empty line
                goto line_done;
            } else {
                // Nothing after the line number; treat this as a
                // number without a line number
                // Note that we could treat many more cases as unnumbered
                // numbers; basically, any number followed by something that
                // doesn't parse... but I'm not opening that can of worms until
                // I see a good reason to.
                hpbuf[lineno_end] = 0;
                cmd = CMD_NUMBER;
                strcpy(numbuf, hpbuf + lineno_start);
                arg.val_d = parse_number_line(numbuf);
                arg.type = ARGTYPE_DOUBLE;
                goto store;
            }
        }
        if (lineno_start != -1 && hppos == prev_hppos)
            // No space following line number? Not acceptable.
            goto line_done;
        if (hppos < hpend - 1 && (hpbuf[hppos] == 127 || hpbuf[hppos] == '+') && hpbuf[hppos + 1] == '"') {
            // Appended string
            hpbuf[hppos + 1] = 127;
            goto do_string;
        } else if (hppos < hpend && hpbuf[hppos] == '"') {
            // Non-appended string
            do_string:
            hppos++;
            // String literals can be up to 15 characters long, and they
            // can contain double quotes. We scan forward for up to 15
            // chars, and the final double quote we find is considered the
            // end of the string; any intervening double quotes are considered
            // to be part of the string.
            int last_quote = -1;
            int i;
            for (i = 0; i < 16; i++) {
                if (hppos + i == hpend)
                    break;
                c = hpbuf[hppos + i];
                if (c == '"')
                    last_quote = i;
            }
            if (last_quote == -1)
                // No closing quote? Fishy, but let's just grab 15
                // characters and hope for the best.
                last_quote = i < 15 ? i : 15;
            if (last_quote == 0) {
                cmd = CMD_NOP;
                arg.type = ARGTYPE_NONE;
            } else {
                cmd = CMD_STRING;
                arg.type = ARGTYPE_STR;
                arg.length = last_quote;
                memcpy(arg.val.text, hpbuf + hppos, arg.length);
                if (arg.length > 0)
                    arg.val.text[0] &= 127;
            }
        } else {
            // Not a string; try to find command
            int cmd_end = hppos;
            while (cmd_end < hpend && hpbuf[cmd_end] != ' ')
                cmd_end++;
            if (cmd_end == hppos)
                goto line_done;
            if (cmd_end - hppos == 5 && hpbuf[hppos] == 'X' && strncmp(hpbuf + 2, "NN?", 3) == 0) {
                // HP-41CX: X=NN? etc.
                switch (hpbuf[hppos + 1]) {
                    case '=': cmd = CMD_X_EQ_NN; goto cx_comp;
                    case  12: cmd = CMD_X_NE_NN; goto cx_comp;
                    case '<': cmd = CMD_X_LT_NN; goto cx_comp;
                    case '>': cmd = CMD_X_GT_NN; goto cx_comp;
                    case   9: cmd = CMD_X_LE_NN; goto cx_comp;
                    case  11: cmd = CMD_X_GE_NN; goto cx_comp;
                    default: goto not_cx_comp;
                }
                cx_comp:
                arg.type = ARGTYPE_IND_STK;
                arg.val.stk = 'Y';
                goto store;
                not_cx_comp:;
            }
            cmd = find_builtin(hpbuf + hppos, cmd_end - hppos);
            int tok_start, tok_end;
            int argtype;
            bool stk_allowed = true;
            bool string_required = false;
            if (cmd == CMD_SIZE) {
                if (!nexttoken(hpbuf, cmd_end, hpend, &tok_start, &tok_end))
                    goto line_done;
                if (tok_end - tok_start > 4)
                    goto line_done;
                int sz = 0;
                for (int i = tok_start; i < tok_end; i++) {
                    char c = hpbuf[i];
                    if (c < '0' || c > '9')
                        goto line_done;
                    sz = sz * 10 + c - '0';
                }
                arg.type = ARGTYPE_NUM;
                arg.val.num = sz;
                goto store;
            } else if (cmd == CMD_FUNC) {
                if (!nexttoken(hpbuf, cmd_end, hpend, &tok_start, &tok_end))
                    goto line_done;
                if (tok_end - tok_start != 2)
                    goto line_done;
                int io = 0;
                for (int i = tok_start; i < tok_end; i++) {
                    char c = hpbuf[i];
                    if (c < '0' || c > '4')
                        goto line_done;
                    io = io * 10 + c - '0';
                }
                arg.type = ARGTYPE_NUM;
                arg.val.num = io;
                goto store;
            } else if (cmd == CMD_ASSIGNa) {
                // What we're looking for is '".*"  *TO  *[0-9][0-9]'
                tok_end = hppos;
                bool after_to = false;
                int to_start;
                int keynum;
                while (true) {
                    if (!nexttoken(hpbuf, tok_end, hpend, &tok_start, &tok_end))
                        goto line_done;
                    int len = tok_end - tok_start;
                    if (after_to) {
                        if (len != 2 || !isdigit(hpbuf[tok_start])
                                     || !isdigit(hpbuf[tok_start + 1])) {
                            after_to = string_equals(hpbuf + tok_start, len, "TO", 2);
                            if (after_to)
                                to_start = tok_start;
                            continue;
                        }
                        after_to = false;
                        sscanf(hpbuf + tok_start, "%02d", &keynum);
                        if (keynum < 1 || keynum > 18)
                            continue;
                        else
                            break;
                    } else {
                        after_to = string_equals(hpbuf + tok_start, len, "TO", 2);
                        if (after_to)
                            to_start = tok_start;
                    }
                }
                // Between hppos (inclusive) and to_start (exclusive),
                // there should be a quote-delimited string...
                while (hppos < hpend && hpbuf[hppos] != '"')
                    hppos++;
                if (hppos == hpend)
                    goto line_done;
                to_start--;
                while (to_start > hppos && hpbuf[to_start] != '"')
                    to_start--;
                if (to_start == hppos)
                    // Only one quote sign found
                    goto line_done;
                int len = to_start - hppos - 1;
                if (len > 7)
                    len = 7;
                cmd = CMD_ASGN01 + keynum - 1;
                arg.type = ARGTYPE_STR;
                arg.length = len;
                memcpy(arg.val.text, hpbuf + hppos + 1, len);
                goto store;
            } else if (cmd == CMD_XSTR) {
                int q1 = -1, q2 = -1;
                for (int i = hppos; i < hpend; i++) {
                    if (hpbuf[i] == '"') {
                        if (q1 == -1)
                            q1 = i;
                        else
                            q2 = i;
                    }
                }
                if (q2 == -1)
                    goto line_done;
                arg.type = ARGTYPE_XSTR;
                arg.length = q2 - q1 - 1;
                arg.val.xstr = hpbuf + q1 + 1;
                goto store;
            } else if (cmd != CMD_NONE) {
                int flags;
                flags = cmd_array[cmd].flags;
                arg.type = ARGTYPE_NONE;
                if ((flags & (FLAG_IMMED | FLAG_HIDDEN | FLAG_NO_PRGM)) != 0)
                    goto line_done;
                argtype = cmd_array[cmd].argtype;
                bool ind;
                switch (argtype) {
                    case ARG_NONE: {
                        arg.type = ARGTYPE_NONE;
                        goto store;
                    }
                    case ARG_VAR:
                    case ARG_REAL:
                    case ARG_NUM9:
                    case ARG_NUM11:
                    case ARG_NUM99: {
                        string_only:
                        ind = false;
                        if (!nexttoken(hpbuf, cmd_end, hpend, &tok_start, &tok_end))
                            goto line_done;
                        if (string_equals(hpbuf + tok_start, tok_end - tok_start, "IND", 3)) {
                            ind = true;
                            if (cmd == CMD_CLP || cmd == CMD_MVAR)
                                goto line_done;
                            if (!nexttoken(hpbuf, tok_end, hpend, &tok_start, &tok_end))
                                goto line_done;
                        }
                        num_or_string:
                        if ((argtype == ARG_VAR || argtype == ARG_REAL || ind)
                                && string_equals(hpbuf + tok_start, tok_end - tok_start, "ST", 2)) {
                            if (!ind && (!stk_allowed || string_required))
                                goto line_done;
                            arg.type = ind ? ARGTYPE_IND_STK : ARGTYPE_STK;
                            if (!nexttoken(hpbuf, tok_end, hpend, &tok_start, &tok_end))
                                goto line_done;
                            if (tok_end - tok_start != 1)
                                goto line_done;
                            char c = hpbuf[tok_start];
                            if (c != 'X' && c != 'Y' && c != 'Z' && c != 'T'
                                    && c != 'L')
                                goto line_done;
                            arg.val.stk = c;
                            goto store;
                        }
                        if ((argtype == ARG_VAR || argtype == ARG_REAL || ind)
                                && tok_end - tok_start == 1) {
                            // Accept RCL Z etc., instead of RCL ST Z, for
                            // HP-41 compatibilitry.
                            char c = hpbuf[tok_start];
                            if (c == 'X' || c == 'Y' || c == 'Z' || c == 'T'
                                    || c == 'L') {
                                if (!ind && (!stk_allowed || string_required))
                                    goto line_done;
                                arg.type = ind ? ARGTYPE_IND_STK : ARGTYPE_STK;
                                arg.val.stk = c;
                                goto store;
                            }
                        }
                        if (!ind && argtype == ARG_NUM9) {
                            if (tok_end - tok_start == 1 && isdigit(hpbuf[tok_start])) {
                                arg.type = ARGTYPE_NUM;
                                arg.val.num = hpbuf[tok_start] - '0';
                                if (cmd == CMD_RTNERR && arg.val.num > 8)
                                    goto line_done;
                                goto store;
                            }
                            goto line_done;
                        }
                        if (!ind && argtype == ARG_NUM11 && tok_end - tok_start == 1
                                && isdigit(hpbuf[tok_start])) {
                            // Special case for FIX/SCI/ENG with 1-digit
                            // non-indirect argument; needed for parsing
                            // HP-41 code.
                            arg.type = ARGTYPE_NUM;
                            arg.val.num = hpbuf[tok_start] - '0';
                            goto store;
                        }
                        if (tok_end - tok_start == 2 && isdigit(hpbuf[tok_start])
                                                     && isdigit(hpbuf[tok_start + 1])) {
                            if (!ind && string_required)
                                goto line_done;
                            arg.type = ind ? ARGTYPE_IND_NUM : ARGTYPE_NUM;
                            sscanf(hpbuf + tok_start, "%02d", &arg.val.num);
                            if (!ind && argtype == ARG_NUM11 && arg.val.num > 11)
                                goto line_done;
                            goto store;
                        }
                        if ((argtype == ARG_VAR || argtype == ARG_REAL || ind)
                                && hpbuf[tok_start] == '"') {
                            arg.type = ind ? ARGTYPE_IND_STR : ARGTYPE_STR;
                            handle_string_arg:
                            hppos = tok_start + 1;
                            // String arguments can be up to 7 characters long, and they
                            // can contain double quotes. We scan forward for up to 7
                            // chars, and the final double quote we find is considered the
                            // end of the string; any intervening double quotes are considered
                            // to be part of the string.
                            int last_quote = -1;
                            int i;
                            for (i = 0; i < 8; i++) {
                                if (hppos + i == hpend)
                                    break;
                                c = hpbuf[hppos + i];
                                if (c == '"')
                                    last_quote = i;
                            }
                            if (last_quote == -1)
                                // No closing quote? Fishy, but let's just grab 7
                                // characters and hope for the best.
                                last_quote = i < 7 ? i : 7;
                            arg.length = last_quote;
                            memcpy(arg.val.text, hpbuf + hppos, arg.length);
                            goto store;
                        }
                        goto line_done;
                    }
                    case ARG_PRGM:
                    case ARG_NAMED:
                    case ARG_MAT:
                    case ARG_RVAR: {
                        string_required = true;
                        stk_allowed = false;
                        argtype = ARG_VAR;
                        goto string_only;
                    }
                    case ARG_LBL: {
                        tok_end = cmd_end;
                        gto_or_xeq:
                        if (!nexttoken(hpbuf, tok_end, hpend, &tok_start, &tok_end))
                            goto line_done;
                        ind = false;
                        if (string_equals(hpbuf + tok_start, tok_end - tok_start, "IND", 3)) {
                            ind = true;
                            if (!nexttoken(hpbuf, tok_end, hpend, &tok_start, &tok_end))
                                goto line_done;
                        }
                        if (cmd == CMD_LBL && ind)
                            goto line_done;
                        if (tok_end - tok_start == 1) {
                            char c = hpbuf[tok_start];
                            if (c >= 'A' && c <= 'J' || c >= 'a' && c <= 'e') {
                                arg.type = ARGTYPE_LCLBL;
                                arg.val.lclbl = c;
                                goto store;
                            } else
                                goto line_done;
                        }
                        argtype = ARG_VAR;
                        stk_allowed = false;
                        goto num_or_string;
                    }
                    case ARG_OTHER: {
                        if (cmd == CMD_LBL) {
                            tok_end = cmd_end;
                            goto gto_or_xeq;
                        }
                        goto line_done;
                    }
                    default:
                        goto line_done;
                }
            } else if (string_equals(hpbuf + hppos, cmd_end - hppos, "KEY", 3)) {
                // KEY GTO or KEY XEQ
                if (!nexttoken(hpbuf, cmd_end, hpend, &tok_start, &tok_end))
                    goto line_done;
                if (tok_end - tok_start != 1)
                    goto line_done;
                char c = hpbuf[tok_start];
                if (c < '1' || c > '9')
                    goto line_done;
                if (!nexttoken(hpbuf, tok_end, hpend, &tok_start, &tok_end))
                    goto line_done;
                if (string_equals(hpbuf + tok_start, tok_end - tok_start, "GTO", 3))
                    cmd = CMD_KEY1G + c - '1';
                else if (string_equals(hpbuf + tok_start, tok_end - tok_start, "XEQ", 3))
                    cmd = CMD_KEY1X + c - '1';
                else
                    goto line_done;
                goto gto_or_xeq;
            } else if (string_equals(hpbuf + hppos, cmd_end - hppos, ".END.", 5)) {
                cmd = CMD_END;
                arg.type = ARGTYPE_NONE;
                goto store;
            } else if (string_equals(hpbuf + hppos, cmd_end - hppos, "XROM", 4)) {
                // Should handle num,num and "lbl"
                if (!nexttoken(hpbuf, cmd_end, hpend, &tok_start, &tok_end))
                    goto line_done;
                if (hpbuf[tok_start] == '"') {
                    arg.type = ARGTYPE_STR;
                    cmd = CMD_XEQ;
                    goto handle_string_arg;
                }
                int len = tok_end - tok_start;
                if (len >= 4 && len <= 32 && len % 2 == 0 && hpbuf[tok_start] == '0' && hpbuf[tok_start + 1] == 'x') {
                    // XROM 0xdeadbeef: used for strings whose first character has its high
                    // bit set, putting it in the space of HP-42S extensions, but which do
                    // not correspond to any actual known extension.
                    char d = 0;
                    arg.length = 0;
                    for (int i = 2; i < len; i++) {
                        char c = hpbuf[tok_start + i];
                        if (c >= '0' && c <= '9')
                            d += c - '0';
                        else if (c >= 'A' && c <= 'F')
                            d += c - 'A' + 10;
                        else if (c >= 'a' && c <= 'f')
                            d += c - 'a' + 10;
                        else
                            goto line_done;
                        if ((i & 1) != 0) {
                            arg.val.text[arg.length++] = d;
                            d = 0;
                        } else {
                            d <<= 4;
                        }
                    }
                    cmd = CMD_XROM;
                    arg.type = ARGTYPE_STR;
                    goto store;
                }
                if (len > 5)
                    goto line_done;
                char xrombuf[6];
                memcpy(xrombuf, hpbuf + tok_start, len);
                xrombuf[len] = 0;
                int a, b;
                if (sscanf(xrombuf, "%d,%d", &a, &b) != 2)
                    goto line_done;
                if (a < 0 || a > 31 || b < 0 || b > 63)
                    goto line_done;
                cmd = CMD_XROM;
                arg.type = ARGTYPE_NUM;
                arg.val.num = (a << 6) | b;
                goto store;
            } else if ((string_equals(hpbuf + hppos, cmd_end - hppos - 1, "FNC", 3)
                    || string_equals(hpbuf + hppos, cmd_end - hppos - 1, "FUNC", 4))
                    && hpbuf[cmd_end - 1] >= '0'
                    && hpbuf[cmd_end - 1] <= '2') {
                cmd = CMD_FUNC;
                arg.type = ARGTYPE_NUM;
                switch (hpbuf[cmd_end - 1]) {
                    case '0': arg.val.num =  0; break;
                    case '1': arg.val.num = 11; break;
                    case '2': arg.val.num = 21; break;
                }
                goto store;
            } else {
                // Number or bust!
                if (nexttoken(hpbuf, hppos, hpend, &tok_start, &tok_end)) {
                    char c = hpbuf[tok_start];
                    bool have_exp = false;
                    if (c >= '0' && c <= '9' || c == '-' || c == '.' || c == ','
                            || c == 'E' || c == 'e' || c == 24) {
                        // The first character could plausibly be part of a number;
                        // let's run with it.
                        int len = tok_end - tok_start;
                        if (len > 49)
                            len = 49;
                        for (int i = 0; i < len; i++) {
                            c = hpbuf[tok_start + i];
                            if (c == 'e' || c == 24)
                                c = 'E';
                            else if (c == ',')
                                c = '.';
                            if (c == 'E')
                                have_exp = true;
                            numbuf[i] = c;
                        }
                        numbuf[len] = 0;
                        if (!have_exp) {
                            // In HP-41 program listings, there may be a space
                            // before the 'E' character, e.g. "1 E3". So, if we
                            // haven't seen an exponent yet, check if the next
                            // token looks like an exponent, and if so, add it.
                            if (nexttoken(hpbuf, tok_end, hpend, &tok_start, &tok_end)) {
                                c = hpbuf[tok_start];
                                if (c == 'E' || c == 'e' || c == 24) {
                                    int explen = tok_end - tok_start;
                                    bool is_exp = true;
                                    for (int i = 1; i < explen; i++) {
                                        c = hpbuf[tok_start + i];
                                        if (!(c == '-' && i == 1 || c >= '0' && c <= '9')) {
                                            is_exp = false;
                                            break;
                                        }
                                    }
                                    if (is_exp) {
                                        if (len + explen > 49)
                                            explen = 49 - len;
                                        char *p = numbuf + len;
                                        *p++ = 'E';
                                        for (int i = 1; i < explen; i++)
                                            *p++ = hpbuf[tok_start + i];
                                        *p = 0;
                                    }
                                }
                            }
                        }
                        cmd = CMD_NUMBER;
                        arg.val_d = parse_number_line(numbuf);
                        arg.type = ARGTYPE_DOUBLE;
                        goto store;
                    }
                }
                goto line_done;
            }
        }

        store:
        if (after_end)
            goto_dot_dot(false);
        after_end = cmd == CMD_END;
        if (!after_end)
            store_command_after(&pc, cmd, &arg, numbuf);

        line_done:
        if (cmd == CMD_RTNERR && arg.type == ARGTYPE_NONE) {
            // Hack to handle RTNERR with no argument as RTNERR IND ST X
            arg.type = ARGTYPE_IND_STK;
            arg.val.stk = 'X';
            goto store;
        }
        pos = end + 1;
        if (hpbuf != hpbuf_s) {
            free(hpbuf);
            hpbuf = NULL;
        }
    }
}

static int get_token(const char *buf, int *pos, int *start) {
    char c;
    while (true) {
        c = buf[*pos];
        if (c == 0)
            return 0;
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\f')
            break;
        (*pos)++;
    }
    *start = *pos;
    if (c == '"') {
        (*pos)++;
        while (true) {
            c = buf[*pos];
            if (c == 0)
                return *pos - *start;
            if (c == '"') {
                (*pos)++;
                return *pos - *start;
            }
            if (c == '\\') {
                (*pos)++;
                c = buf[*pos];
                if (c == 0)
                    return *pos - *start;
            }
            (*pos)++;
        }
    } else {
        while (true) {
            c = buf[*pos];
            if (c == 0 || c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f')
                return *pos - *start;
            if (c == '<') {
                if (strncmp(buf + *pos, STR_INF, 10) == 0) {
                    *pos += 10;
                } else if (strncmp(buf + *pos, STR_NEG_INF, 11) == 0) {
                    *pos += 11;
                } else if (strncmp(buf + *pos, STR_NAN, 14) == 0) {
                    *pos += 14;
                } else
                    (*pos)++;
            } else
                (*pos)++;
        }
    }
}

static int parse_int(const char *buf, int len) {
    int r = 0;
    for (int i = 0; i < len; i++) {
        char c = buf[i];
        if (c < '0' || c > '9')
            return -1;
        r = r * 10 + c - '0';
    }
    return r;
}

static char *parse_string(const char *buf, int len, int *slen) {
    char *s = (char *) malloc(len - 2);
    if (s == NULL)
        return NULL;
    int sl = 0;
    for (int i = 1; i < len - 1; i++) {
        char c = buf[i];
        if (c == '\\')
            c = buf[++i];
        s[sl++] = c;
    }
    char *s2 = (char *) malloc(sl + 4);
    if (s2 == NULL) {
        free(s);
        return NULL;
    }
    sl = ascii2hp(s2, sl, s, sl);
    free(s);
    *slen = sl;
    return s2;
}

static vartype *deserialize_list(const char *buf, int *pos) {
    int tstart;
    int tlen = get_token(buf, pos, &tstart);
    if (tlen != 1 || buf[tstart] != '{')
        return NULL;
    tlen = get_token(buf, pos, &tstart);
    if (tlen < 6 || strncmp(buf + tstart + tlen - 5, "-Elem", 5) != 0)
        return NULL;
    int len = parse_int(buf + tstart, tlen - 5);
    if (len == -1)
        return NULL;
    tlen = get_token(buf, pos, &tstart);
    if (tlen != 4 || strncmp(buf + tstart, "List", 4) != 0)
        return NULL;
    vartype_list *list = (vartype_list *) new_list(len);
    if (list == NULL)
        return NULL;
    for (int i = 0; i < len; i++) {
        tlen = get_token(buf, pos, &tstart);
        if (tlen == 0)
            goto failure;
        if (buf[tstart] == '{') {
            if (tlen != 1)
                goto failure;
            (*pos)--;
            vartype *e = deserialize_list(buf, pos);
            if (e == NULL)
                goto failure;
            list->array->data[i] = e;
        } else if (buf[tstart] == '[') {
            if (tlen != 1)
                goto failure;
            tlen = get_token(buf, pos, &tstart);
            if (tlen == 0)
                goto failure;
            int x = -1;
            for (int j = 0; j < tlen; j++) {
                if (buf[tstart + j] == 'x') {
                    x = j;
                    break;
                }
            }
            if (x == -1 || x == 0 || x == tlen - 1)
                goto failure;
            int rows = parse_int(buf + tstart, x);
            int cols = parse_int(buf + tstart + x + 1, tlen - x - 1);
            if (rows == -1 || cols == -1)
                goto failure;
            bool cpx;
            tlen = get_token(buf, pos, &tstart);
            vartype *m;
            if (tlen == 6 && strncmp(buf + tstart, "Matrix", 6) == 0) {
                m = new_realmatrix(rows, cols);
                if (m == NULL)
                    goto failure;
                cpx = false;
            } else if (tlen == 3 && strncmp(buf + tstart, "Cpx", 3) == 0) {
                tlen = get_token(buf, pos, &tstart);
                if (tlen != 6 || strncmp(buf + tstart, "Matrix", 6) != 0)
                    goto failure;
                m = new_complexmatrix(rows, cols);
                if (m == NULL)
                    goto failure;
                cpx = true;
            } else
                goto failure;
            list->array->data[i] = m;
            int cells = rows * cols;
            for (int j = 0; j < cells; j++) {
                tlen = get_token(buf, pos, &tstart);
                if (tlen == 0)
                    goto failure;
                if (buf[tstart] == '"') {
                    if (cpx)
                        goto failure;
                    int slen;
                    char *s = parse_string(buf + tstart, tlen, &slen);
                    if (s == NULL)
                        return NULL;
                    bool res = put_matrix_string((vartype_realmatrix *) m, j, s, slen);
                    free(s);
                    if (!res)
                        goto failure;
                } else {
                    phloat re, im;
                    int slen;
                    int type = parse_scalar(buf + tstart, tlen, true, &re, &im, &slen);
                    if (cpx) {
                        if (type == TYPE_REAL)
                            im = 0;
                        else if (type != TYPE_COMPLEX)
                            goto failure;
                        vartype_complexmatrix *cm = (vartype_complexmatrix *) m;
                        cm->array->data[j * 2] = re;
                        cm->array->data[j * 2 + 1] = im;
                    } else {
                        if (type != TYPE_REAL)
                            goto failure;
                        vartype_realmatrix *rm = (vartype_realmatrix *) m;
                        rm->array->data[j] = re;
                    }
                }
            }
            tlen = get_token(buf, pos, &tstart);
            if (tlen != 1 || buf[tstart] != ']')
                goto failure;
        } else if (buf[tstart] == '"') {
            int slen;
            char *s = parse_string(buf + tstart, tlen, &slen);
            if (s == NULL)
                return NULL;
            vartype *str = new_string(s, slen);
            free(s);
            if (str == NULL)
                goto failure;
            list->array->data[i] = str;
        } else {
            phloat re, im;
            int slen;
            int type = parse_scalar(buf + tstart, tlen, true, &re, &im, &slen);
            vartype *v;
            if (type == TYPE_REAL)
                v = new_real(re);
            else if (type == TYPE_COMPLEX)
                v = new_complex(re, im);
            else
                goto failure;
            if (v == NULL)
                goto failure;
            list->array->data[i] = v;
        }
    }
    tlen = get_token(buf, pos, &tstart);
    if (tlen != 1 || buf[tstart] != '}') {
        failure:
        free_vartype((vartype *) list);
        return NULL;
    } else
        return (vartype *) list;
}

void core_paste(const char *buf) {
    if (mode_interruptible != NULL)
        stop_interruptible();
    set_running(false);

    if (flags.f.prgm_mode) {
        paste_programs(buf);
    } else if (alpha_active()) {
        char hpbuf[48];
        int len = ascii2hp(hpbuf, 44, buf);
        int tlen = len + reg_alpha_length;
        if (tlen > 44) {
            int off = tlen - 44;
            memmove(reg_alpha, reg_alpha + off, off);
            reg_alpha_length -= off;
        }
        memcpy(reg_alpha + reg_alpha_length, hpbuf, len);
        reg_alpha_length += len;
        if (len > 0) {
            flags.f.alpha_data_input = 1;
            if (flags.f.trace_print && flags.f.printer_exists)
                docmd_pra(NULL);
        }
    } else {
        int rows = 0, cols = 0;
        int col = 1;
        int max_cell_size = 0;
        int cell_size = 0;
        int pos = 0;
        char lastchar, c = 0;
        vartype *v;
        if (buf[0] == '{') {
            // Try to parse a list; if unsuccessful, fall back on TSV parsing
            int tpos = 0;
            v = deserialize_list(buf, &tpos);
            if (v != NULL)
                goto parse_success;
        }
        while (true) {
            lastchar = c;
            c = buf[pos++];
            if (c == 0)
                break;
            if (c == '\r') {
                c = '\n';
                if (buf[pos] == '\n')
                    pos++;
            }
            if (c == '\n') {
                rows++;
                if (cols < col)
                    cols = col;
                col = 1;
                goto check_cell_size;
            } else if (c == '\t') {
                col++;
                check_cell_size:
                if (max_cell_size < cell_size)
                    max_cell_size = cell_size;
                cell_size = 0;
            } else {
                cell_size++;
            }
        }
        if (lastchar != 0 && lastchar != '\n') {
            rows++;
            if (cols < col)
                cols = col;
            if (max_cell_size < cell_size)
                max_cell_size = cell_size;
        }
        if (rows == 0) {
            return;
        } else if (rows == 1 && cols == 1) {
            // Scalar
            int len = (int) strlen(buf);
            if (len > 0 && buf[len - 1] == '\n') {
                len--;
                if (len > 0 && buf[len - 1] == '\r')
                    len--;
            }
            char *hpbuf = (char *) malloc(len + 4);
            if (hpbuf == NULL) {
                display_error(ERR_INSUFFICIENT_MEMORY, false);
                redisplay();
                return;
            }
            len = ascii2hp(hpbuf, len, buf, len);
            v = parse_base(hpbuf, len);
            if (v == NULL) {
                phloat re, im;
                int slen;
                const char *format = core_settings.localized_copy_paste ? number_format() : NULL;
                int type = parse_scalar(hpbuf, len, false, &re, &im, &slen, format);
                switch (type) {
                    case TYPE_REAL:
                        v = new_real(re);
                        break;
                    case TYPE_COMPLEX:
                        v = new_complex(re, im);
                        break;
                    case TYPE_STRING:
                        v = new_string(hpbuf, slen);
                        break;
                }
                if (v == NULL) {
                    display_error(ERR_INSUFFICIENT_MEMORY, false);
                    redisplay();
                    return;
                }
            }
            free(hpbuf);
        } else {
            // Matrix
            int n = rows * cols;
            phloat *data = (phloat *) malloc(n * sizeof(phloat));
            if (data == NULL) {
                display_error(ERR_INSUFFICIENT_MEMORY, false);
                redisplay();
                return;
            }
            char *is_string = (char *) malloc(n);
            if (is_string == NULL) {
                free(data);
                display_error(ERR_INSUFFICIENT_MEMORY, false);
                redisplay();
                return;
            }
            char *hpbuf = (char *) malloc(max_cell_size + 5);
            if (hpbuf == NULL) {
                free(data);
                free(is_string);
                display_error(ERR_INSUFFICIENT_MEMORY, false);
                redisplay();
                return;
            }
            int pos = 0;
            int spos = 0;
            int p = 0, row = 0, col = 0;
            const char *format = core_settings.localized_copy_paste ? number_format() : NULL;
            while (row < rows) {
                c = buf[pos++];
                if (c == 0 || c == '\t' || c == '\r' || c == '\n') {
                    int cellsize = pos - spos - 1;
                    if (c == '\r') {
                        c = '\n';
                        if (buf[pos] == '\n')
                            pos++;
                    }
                    int hplen = ascii2hp(hpbuf, cellsize, buf + spos, cellsize);
                    spos = pos;
                    phloat re, im;
                    int slen;
                    int type = parse_scalar(hpbuf, hplen, true, &re, &im, &slen, format);
                    if (is_string != NULL) {
                        switch (type) {
                            case TYPE_REAL:
                                data[p] = re;
                                is_string[p] = 0;
                                break;
                            case TYPE_COMPLEX:
                                free_long_strings(is_string, data, p);
                                for (int i = 0; i < p; i++)
                                    if (is_string[i] != 0)
                                        data[i] = 0;
                                free(is_string);
                                is_string = NULL;
                                phloat *newdata;
                                newdata = (phloat *) realloc(data, 2 * n * sizeof(phloat));
                                if (newdata == NULL) {
                                    nomem:
                                    free(data);
                                    free(hpbuf);
                                    display_error(ERR_INSUFFICIENT_MEMORY, false);
                                    redisplay();
                                    return;
                                }
                                data = newdata;
                                for (int i = p - 1; i >= 0; i--) {
                                    data[i * 2] = data[i];
                                    data[i * 2 + 1] = 0;
                                }
                                p *= 2;
                                data[p] = re;
                                data[p + 1] = im;
                                goto finish_complex_cell;
                            case TYPE_STRING:
                                if (slen == 0) {
                                    data[p] = 0;
                                    is_string[p] = 0;
                                } else if (slen <= SSLENM) {
                                    char *text = (char *) &data[p];
                                    *text = slen;
                                    memcpy(text + 1, hpbuf, slen);
                                    is_string[p] = 1;
                                } else {
                                    int4 *t = (int4 *) malloc(slen + 4);
                                    if (t == NULL) {
                                        free_long_strings(is_string, data, p);
                                        free(is_string);
                                        goto nomem;
                                    }
                                    *t = slen;
                                    memcpy(t + 1, hpbuf, slen);
                                    *(int4 **) &data[p] = t;
                                    is_string[p] = 2;
                                }
                                break;
                        }
                        p++;
                        col++;
                        if (c == 0 || c == '\n') {
                            while (col++ < cols) {
                                data[p] = 0;
                                is_string[p] = 0;
                                p++;
                            }
                            col = 0;
                            row++;
                        }
                    } else {
                        switch (type) {
                            case TYPE_REAL:
                                data[p] = re;
                                data[p + 1] = 0;
                                break;
                            case TYPE_COMPLEX:
                                data[p] = re;
                                data[p + 1] = im;
                                break;
                            case TYPE_STRING:
                                data[p] = 0;
                                data[p + 1] = 0;
                                break;
                        }
                        finish_complex_cell:
                        p += 2;
                        col++;
                        if (c == 0 || c == '\n') {
                            while (col++ < cols) {
                                data[p] = 0;
                                data[p + 1] = 0;
                                p += 2;
                            }
                            col = 0;
                            row++;
                        }
                    }
                }
                if (c == 0)
                    break;
            }

            free(hpbuf);
            if (is_string != NULL) {
                vartype_realmatrix *rm = (vartype_realmatrix *)
                                malloc(sizeof(vartype_realmatrix));
                if (rm == NULL) {
                    free_long_strings(is_string, data, p);
                    free(data);
                    free(is_string);
                    display_error(ERR_INSUFFICIENT_MEMORY, false);
                    redisplay();
                    return;
                }
                rm->array = (realmatrix_data *)
                                malloc(sizeof(realmatrix_data));
                if (rm->array == NULL) {
                    free(rm);
                    free_long_strings(is_string, data, p);
                    free(data);
                    free(is_string);
                    display_error(ERR_INSUFFICIENT_MEMORY, false);
                    redisplay();
                    return;
                }
                rm->type = TYPE_REALMATRIX;
                rm->rows = rows;
                rm->columns = cols;
                rm->array->data = data;
                rm->array->is_string = is_string;
                rm->array->refcount = 1;
                v = (vartype *) rm;
            } else {
                vartype_complexmatrix *cm = (vartype_complexmatrix *)
                                malloc(sizeof(vartype_complexmatrix));
                if (cm == NULL) {
                    free(data);
                    display_error(ERR_INSUFFICIENT_MEMORY, false);
                    redisplay();
                    return;
                }
                cm->array = (complexmatrix_data *)
                                malloc(sizeof(complexmatrix_data));
                if (cm->array == NULL) {
                    free(cm);
                    free(data);
                    display_error(ERR_INSUFFICIENT_MEMORY, false);
                    redisplay();
                    return;
                }
                cm->type = TYPE_COMPLEXMATRIX;
                cm->rows = rows;
                cm->columns = cols;
                cm->array->data = data;
                cm->array->refcount = 1;
                v = (vartype *) cm;
            }
        }
        parse_success:
        if (recall_result(v) != ERR_NONE) {
            display_error(ERR_INSUFFICIENT_MEMORY, false);
            redisplay();
            return;
        }
        mode_number_entry = false;
        mode_varmenu = false;
        flags.f.stack_lift_disable = 0;
        if (v->type == TYPE_REAL || v->type == TYPE_COMPLEX)
            flags.f.numeric_data_input = 1;
    }
    flags.f.message = 0;
    flags.f.two_line_message = 0;
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
    if ((cmd_array[command].flags
                & (flags.f.prgm_mode ? FLAG_NO_PRGM : FLAG_PRGM_ONLY)) != 0) {
        display_error(ERR_RESTRICTED_OPERATION, false);
        redisplay();
        return;
    }
    if (command == CMD_GOTOROW) {
        if (sp != -1) {
            err = docmd_stoel(NULL);
            if (err != ERR_NONE) {
                display_error(err, true);
                redisplay();
                return;
            }
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
    } else if (command == CMD_CLV || command == CMD_PRV || command == CMD_LCLV) {
        if (!flags.f.prgm_mode && vars_count == 0) {
            display_error(ERR_NO_VARIABLES, false);
            redisplay();
            return;
        }
    } else if ((command == CMD_SST || command == CMD_SST_UP
            || command == CMD_SST_RT) && flags.f.prgm_mode) {
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

    if (flags.f.prgm_mode && (cmd_array[command].flags & FLAG_IMMED) == 0) {
        if (command == CMD_RUN)
            command = CMD_STOP;
        if (cmd_array[command].argtype == ARG_NONE) {
            arg_struct arg;
            arg.type = ARGTYPE_NONE;
            store_command_after(&pc, command, &arg, NULL);
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
        if (cmd_array[command].argtype == ARG_NONE)
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
    do {
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
        get_next_command(&pc, &cmd, &arg, 1, NULL);
        if (flags.f.trace_print && flags.f.printer_exists) {
            if (cmd == CMD_LBL)
                print_text(NULL, 0, true);
            print_program_line(current_prgm, oldpc);
        }
        mode_disable_stack_lift = false;
        error = handle(cmd, &arg);
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
    } while (!shell_wants_cpu());
}

struct synonym_spec {
    char name[7];
    bool is_orig;
    int namelen;
    int cmd_id;
};

static synonym_spec hp41_synonyms[] =
{
    { "*",      true,  1, CMD_MUL     },
    { "x",      false, 1, CMD_MUL     },
    { "/",      true,  1, CMD_DIV     },
    { "CHS",    true,  3, CMD_CHS     },
    { "DEC",    true,  3, CMD_TO_DEC  },
    { "D-R",    true,  3, CMD_TO_RAD  },
    { "ENTER^", true,  6, CMD_ENTER   },
    { "FACT",   true,  4, CMD_FACT    },
    { "FRC",    true,  3, CMD_FP      },
    { "HMS",    true,  3, CMD_TO_HMS  },
    { "HR",     true,  2, CMD_TO_HR   },
    { "INT",    true,  3, CMD_IP      },
    { "OCT",    true,  3, CMD_TO_OCT  },
    { "P-R",    true,  3, CMD_TO_REC  },
    { "R-D",    true,  3, CMD_TO_DEG  },
    { "RCL*",   false, 4, CMD_RCL_MUL },
    { "RCLx",   false, 4, CMD_RCL_MUL },
    { "RCL/",   false, 4, CMD_RCL_DIV },
    { "RDN",    true,  3, CMD_RDN     },
    { "Rv",     false, 2, CMD_RDN     },
    { "R-P",    true,  3, CMD_TO_POL  },
    { "ST+",    true,  3, CMD_STO_ADD },
    { "ST-",    true,  3, CMD_STO_SUB },
    { "ST*",    true,  3, CMD_STO_MUL },
    { "ST/",    true,  3, CMD_STO_DIV },
    { "STO*",   false, 4, CMD_STO_MUL },
    { "STOx",   false, 4, CMD_STO_MUL },
    { "STO/",   false, 4, CMD_STO_DIV },
    { "X<=0?",  true,  5, CMD_X_LE_0  },
    { "X<=Y?",  true,  5, CMD_X_LE_Y  },
    { "X#0?",   false, 4, CMD_X_NE_0  },
    { "X#Y?",   false, 4, CMD_X_NE_Y  },
    { "X<>0?",  false, 5, CMD_X_NE_0  },
    { "X<>Y?",  false, 5, CMD_X_NE_Y  },
    { "v",      false, 1, CMD_DOWN    },
    { "SST\016",true,  4, CMD_SST     },
    { "X>=0?",  false, 5, CMD_X_GE_0  },
    { "X>=Y?",  false, 5, CMD_X_GE_Y  },
    { "S-N",    false, 3, CMD_S_TO_N  },
    { "N-S",    false, 3, CMD_N_TO_S  },
    { "NN-S",   false, 4, CMD_NN_TO_S },
    { "C-N",    false, 3, CMD_C_TO_N  },
    { "N-C",    false, 3, CMD_N_TO_C  },
    { "X<>?",   false, 4, CMD_X_NE_NN },
    { "X#?",    false, 3, CMD_X_NE_NN },
    { "X<=?",   false, 4, CMD_X_LE_NN },
    { "X>=?",   false, 4, CMD_X_GE_NN },
    { "0<>?",   false, 4, CMD_0_NE_NN },
    { "0#?",    false, 3, CMD_0_NE_NN },
    { "0<=?",   false, 4, CMD_0_LE_NN },
    { "0>=?",   false, 4, CMD_0_GE_NN },
    { "",       true,  0, CMD_NONE    }
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
        if (i == CMD_OPENF) i += 14; // Skip COPAN
        if (i == CMD_SENTINEL)
            break;
        if ((cmd_array[i].flags & FLAG_HIDDEN) != 0)
            continue;
        if (cmd_array[i].name_length != namelen)
            continue;
        for (j = 0; j < namelen; j++) {
            unsigned char c1, c2;
            c1 = name[j];
            if (c1 >= 130 && c1 != 138)
                c1 &= 127;
            c2 = cmd_array[i].name[j];
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
    int argtype = cmd_array[cmd_id].argtype;
    if (!flags.f.prgm_mode && (cmd_array[cmd_id].flags & FLAG_PRGM_ONLY) != 0) {
        display_error(ERR_RESTRICTED_OPERATION, false);
        redisplay();
        return;
    }
    incomplete_command = cmd_id;
    incomplete_ind = false;
    if (argtype == ARG_NAMED || argtype == ARG_PRGM
            || argtype == ARG_RVAR || argtype == ARG_MAT
            || argtype == ARG_XSTR)
        incomplete_alpha = true;
    else
        incomplete_alpha = false;
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
        if (mode_alphamenu != MENU_NONE || mode_plainmenu != MENU_NONE)
            set_catalog_menu(CATSECT_VARS_ONLY);
        else if (mode_appmenu == MENU_VARMENU)
            mode_commandmenu = MENU_VARMENU;
        else if (mode_appmenu == MENU_INTEG_PARAMS)
            mode_commandmenu = MENU_INTEG_PARAMS;
        else
            set_catalog_menu(CATSECT_VARS_ONLY);
    } else if (argtype == ARG_NAMED)
        set_catalog_menu(CATSECT_VARS_ONLY);
    else if (argtype == ARG_REAL) {
        if (mode_alphamenu != MENU_NONE || mode_plainmenu != MENU_NONE)
            set_catalog_menu(CATSECT_REAL_ONLY);
        else if (mode_appmenu == MENU_VARMENU)
            mode_commandmenu = MENU_VARMENU;
        else if (mode_appmenu == MENU_INTEG_PARAMS)
            mode_commandmenu = MENU_INTEG_PARAMS;
        else
            set_catalog_menu(CATSECT_REAL_ONLY);
    } else if (argtype == ARG_RVAR) {
        if (vars_exist(CATSECT_REAL))
            set_catalog_menu(CATSECT_REAL_ONLY);
        else if (flags.f.prgm_mode) {
            if (incomplete_command == CMD_MVAR)
                mode_commandmenu = MENU_ALPHA1;
        } else {
            mode_command_entry = false;
            display_error(ERR_NO_REAL_VARIABLES, false);
        }
    } else if (argtype == ARG_MAT) {
        if (flags.f.prgm_mode || vars_exist(CATSECT_MAT_LIST))
            set_catalog_menu(CATSECT_MAT_LIST_ONLY);
        else if (cmd_id != CMD_DIM) {
            mode_command_entry = false;
            display_error(ERR_NO_MATRIX_VARIABLES, false);
        }
    } else if (argtype == ARG_LBL || argtype == ARG_PRGM)
        set_catalog_menu(CATSECT_PGM_ONLY);
    else if (cmd_id == CMD_LBL || cmd_id == CMD_XSTR)
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
        } else if (pending_command == CMD_SST || pending_command == CMD_SST_UP
                || pending_command == CMD_SST_RT || pending_command == CMD_BST) {
            pc = incomplete_saved_pc;
            prgm_highlight_row = incomplete_saved_highlight_row;
            if (pending_command == CMD_BST)
                bst();
            else
                sst();
            repeating = 1;
            repeating_shift = 1;
            repeating_key = pending_command == CMD_BST ? KEY_UP : KEY_DOWN;
            pending_command = CMD_NONE;
            redisplay();
        } else {
            int inserting_an_end = pending_command == CMD_END;
            if ((cmd_array[pending_command].flags & FLAG_IMMED) != 0)
                goto do_it_now;
            store_command(pc, pending_command, &pending_command_arg, NULL);
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
        if (cmd_array[cmd].argtype == ARG_NONE) {
            pending_command = cmd;
            pending_command_arg.type = ARGTYPE_NONE;
            finish_command_entry(false);
            return;
        } else {
            shell_delay(250);
            pending_command = CMD_NONE;
            set_menu(MENULEVEL_COMMAND, MENU_NONE);
            if ((cmd == CMD_CLV || cmd == CMD_PRV || cmd == CMD_LCLV)
                    && !flags.f.prgm_mode && vars_count == 0) {
                display_error(ERR_NO_VARIABLES, false);
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
        store_command(pc, CMD_STRING, &arg, NULL);
        prgm_highlight_row = 1;
    }
    mode_alpha_entry = false;
}

static void stop_interruptible() {
    int error = mode_interruptible(true);
    handle_error(error);
    mode_interruptible = NULL;
    if (mode_running)
        set_running(false);
    else
        shell_annunciators(-1, -1, -1, false, -1, -1);
    pending_command = CMD_NONE;
    redisplay();
}

void set_old_pc(int4 pc) {
    oldpc = pc;
}

static void set_last_err(int error) {
    if (error <= RTNERR_MAX) {
        lasterr = error;
    } else {
        lasterr = -1;
        string_copy(lasterr_text, &lasterr_length, errors[error].text, errors[error].length);
    }
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
        } else if (error == ERR_NUMBER_TOO_LARGE
                || error == ERR_NUMBER_TOO_SMALL) {
            // Handling these separately because they shouldn't be
            // suppressed by flag 25, nor trapped by SOLVE
            goto handle_it;
        } else if (error != ERR_NONE && error != ERR_YES) {
            if (flags.f.error_ignore && error != ERR_SUSPICIOUS_OFF) {
                flags.f.error_ignore = 0;
                set_last_err(error);
                return 1;
            }
            if (solve_active() && (error == ERR_OUT_OF_RANGE
                                        || error == ERR_DIVIDE_BY_0
                                        || error == ERR_INVALID_DATA
                                        || error == ERR_STAT_MATH_ERROR)) {
                bool stop = unwind_stack_until_solve();
                error = return_to_solve(1, stop);
                if (error == ERR_STOP)
                    set_running(false);
                if (error == ERR_NONE || error == ERR_RUN || error == ERR_STOP)
                    return 0;
            }
            handle_it:
            pc = oldpc;
            display_error(error, true);
            set_running(false);
            return 0;
        }
        return 1;
    } else if (pending_command == CMD_SST || pending_command == CMD_SST_RT) {
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
        } else if (error == ERR_NUMBER_TOO_LARGE
                || error == ERR_NUMBER_TOO_SMALL) {
            // Handling these separately because they shouldn't be
            // suppressed by flag 25, nor trapped by SOLVE
            goto handle_it_2;
        } else {
            if (flags.f.error_ignore) {
                flags.f.error_ignore = 0;
                set_last_err(error);
                goto noerr;
            }
            if (solve_active() && (error == ERR_OUT_OF_RANGE
                                      || error == ERR_DIVIDE_BY_0
                                      || error == ERR_INVALID_DATA
                                      || error == ERR_STAT_MATH_ERROR)) {
                bool stop = unwind_stack_until_solve();
                error = return_to_solve(1, stop);
                if (error == ERR_NONE || error == ERR_RUN || error == ERR_STOP)
                    goto noerr;
            }
            handle_it_2:
            pc = oldpc;
            display_error(error, true);
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
            set_last_err(error);
            error = ERR_NONE;
        }
        if (error != ERR_NONE && error != ERR_STOP)
            display_error(error, true);
        return 0;
    }
}

const char *number_format() {
    const char *uf = shell_number_format();
    static char df[9];
    df[0] = 0;
    int len = ascii2hp(df, 4, uf);
    if (len >= 4)
        df[4] = 0;
    else
        df[1] = 0;
    // Sanity enforcement:
    // Decimal must be '.' or ','; default to '.'
    // Grouping char must be 0 (no grouping), '.', ',', '\'', or ' '; default to 0
    // Primary and secondary group sizes must be between 1 and 9; default to no grouping
    if (df[0] != ',')
        df[0] = '.';
    if (df[1] != 0 && df[1] != '.' && df[1] != ',' && df[1] != '\'' && df[1] != ' ')
        df[1] = 0;
    if (df[1] != 0 && !(df[2] >= '1' && df[2] <= '9' && df[3] >= '1' && df[3] <= '9'))
        df[1] = 0;
    return df;
}
