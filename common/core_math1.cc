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

// to enable debug out
//#define WINDOWS_DEBUG

#ifdef WINDOWS_DEBUG
#include <windows.h> 
#endif 

#include <stdlib.h>

#include "core_math1.h"
#include "core_commands2.h"
#include "core_display.h"
#include "core_globals.h"
#include "core_helpers.h"
#include "core_main.h"
#include "core_variables.h"
#include "shell.h"

#define SOLVE_VERSION 4
#define INTEG_VERSION 3
#define NUM_SHADOWS 10

/* Solver */
typedef struct {
    int version;
    char prgm_name[7];
    int prgm_length;
    char active_prgm_name[7];
    int active_prgm_length;
    char var_name[7];
    int var_length;
    int keep_running;
    int prev_prgm;
    int4 prev_pc;
    int state;
    int which;
    int toggle;
    int retry_counter;
    phloat retry_value;
    phloat x1, x2, x3;
    phloat fx1, fx2;
    phloat prev_x, curr_x, curr_f;
    phloat xm, fxm;
    char shadow_name[NUM_SHADOWS][7];
    int shadow_length[NUM_SHADOWS];
    phloat shadow_value[NUM_SHADOWS];
    uint4 last_disp_time;
} solve_state;

static solve_state solve;

#define ROMB_K 5
// 1/2 million evals max!
#define ROMB_MAX 20

/* Integrator */
typedef struct {
    int version;
    char prgm_name[7];
    int prgm_length;
    char active_prgm_name[7];
    int active_prgm_length;
    char var_name[7];
    int var_length;
    int keep_running;
    int prev_prgm;
    int4 prev_pc;
    int state;
    phloat llim, ulim, acc;
    phloat a, b, eps;
    int n, m, i, k;
    phloat h, sum;
    phloat c[ROMB_K];
    phloat s[ROMB_K+1];
    int nsteps;
    phloat p;
    phloat t, u;
    phloat prev_int;
    phloat prev_res;
} integ_state;

static integ_state integ;


static void reset_solve();
static void reset_integ();


bool persist_math() {
    int size = sizeof(solve_state);
    solve.version = SOLVE_VERSION;
    if (!shell_write_saved_state(&size, sizeof(int))) return false;
    if (!shell_write_saved_state(&solve, sizeof(solve_state))) return false;
    size = sizeof(integ_state);
    integ.version = INTEG_VERSION;
    if (!shell_write_saved_state(&size, sizeof(int))) return false;
    if (!shell_write_saved_state(&integ, sizeof(integ_state))) return false;
    return true;
}

bool unpersist_math(bool discard) {
    int size;
    bool success;
    void *dummy;

    if (shell_read_saved_state(&size, sizeof(int)) != sizeof(int))
        return false;
    if (!discard && size == sizeof(solve_state)) {
        if (shell_read_saved_state(&solve, size) != size)
            return false;
        if (solve.version != SOLVE_VERSION)
            reset_solve();
    } else {
        dummy = malloc(size);
        if (dummy == NULL)
            return false;
        success = shell_read_saved_state(dummy, size) == size;
        free(dummy);
        if (!success)
            return false;
        reset_solve();
    }

    if (shell_read_saved_state(&size, sizeof(int)) != sizeof(int))
        return false;
    if (!discard && size == sizeof(integ_state)) {
        if (shell_read_saved_state(&integ, size) != size)
            return false;
        if (integ.version != INTEG_VERSION)
            reset_integ();
    } else {
        dummy = malloc(size);
        if (dummy == NULL)
            return false;
        success = shell_read_saved_state(dummy, size) == size;
        free(dummy);
        if (!success)
            return false;
        reset_integ();
    }

    return true;
}

void reset_math() {
    reset_solve();
    reset_integ();
}

static void reset_solve() {
    int i;
    for (i = 0; i < NUM_SHADOWS; i++)
        solve.shadow_length[i] = 0;
    solve.prgm_length = 0;
    solve.active_prgm_length = 0;
    solve.state = 0;
    if (mode_appmenu == MENU_SOLVE)
        set_menu_return_err(MENULEVEL_APP, MENU_NONE, true);
}

static int find_shadow(const char *name, int length) {
    int i;
    for (i = 0; i < NUM_SHADOWS; i++)
        if (string_equals(solve.shadow_name[i], solve.shadow_length[i],
                          name, length))
            return i;
    return -1;
}

void put_shadow(const char *name, int length, phloat value) {
    int i = find_shadow(name, length);
    if (i == -1) {
        for (i = 0; i < NUM_SHADOWS; i++)
            if (solve.shadow_length[i] == 0)
                goto do_insert;
        /* No empty slots available. Remove slot 0 (the oldest) and
         * move all subsequent ones down, freeing up slot NUM_SHADOWS - 1
         */
        for (i = 0; i < NUM_SHADOWS - 1; i++) {
            string_copy(solve.shadow_name[i], &solve.shadow_length[i],
                        solve.shadow_name[i + 1], solve.shadow_length[i + 1]);
            solve.shadow_value[i] = solve.shadow_value[i + 1];
        }
    }
    do_insert:
    string_copy(solve.shadow_name[i], &solve.shadow_length[i], name, length);
    solve.shadow_value[i] = value;
}

int get_shadow(const char *name, int length, phloat *value) {
    int i = find_shadow(name, length);
    if (i == -1)
        return 0;
    *value = solve.shadow_value[i];
    return 1;
}

void remove_shadow(const char *name, int length) {
    int i = find_shadow(name, length);
    int j;
    if (i == -1)
        return;
    for (j = i; j < NUM_SHADOWS - 1; j++) {
        string_copy(solve.shadow_name[i], &solve.shadow_length[i],
                    solve.shadow_name[i + 1], solve.shadow_length[i + 1]);
        solve.shadow_value[i] = solve.shadow_value[i + 1];
    }
    solve.shadow_length[NUM_SHADOWS - 1] = 0;
}

void set_solve_prgm(const char *name, int length) {
    string_copy(solve.prgm_name, &solve.prgm_length, name, length);
}

static int call_solve_fn(int which, int state) {
    if (solve.active_prgm_length == 0)
        return ERR_NONEXISTENT;
    int err, i;
    arg_struct arg;
    vartype *v = recall_var(solve.var_name, solve.var_length);
    phloat x = which == 1 ? solve.x1 : which == 2 ? solve.x2 : solve.x3;
    solve.prev_x = solve.curr_x;
    solve.curr_x = x;
    if (v == NULL || v->type != TYPE_REAL) {
        v = new_real(x);
        if (v == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        store_var(solve.var_name, solve.var_length, v);
    } else
        ((vartype_real *) v)->x = x;
    solve.which = which;
    solve.state = state;
    arg.type = ARGTYPE_STR;
    arg.length = solve.active_prgm_length;
    for (i = 0; i < arg.length; i++)
        arg.val.text[i] = solve.active_prgm_name[i];
    err = docmd_gto(&arg);
    if (err != ERR_NONE) {
        free_vartype(v);
        return err;
    }
    push_rtn_addr(-2, 0);
    return ERR_RUN;
}

int start_solve(const char *name, int length, phloat x1, phloat x2) {
    if (solve_active())
        return ERR_SOLVE_SOLVE;
    string_copy(solve.var_name, &solve.var_length, name, length);
    string_copy(solve.active_prgm_name, &solve.active_prgm_length,
                solve.prgm_name, solve.prgm_length);
    solve.prev_prgm = current_prgm;
    solve.prev_pc = pc;
    if (x1 == x2) {
        if (x1 == 0) {
            x2 = 1;
            solve.retry_counter = 0;
        } else {
            x2 = x1 * 1.000001;
            if (p_isinf(x2))
                x2 = x1 * 0.999999;
            solve.retry_counter = -10;
        }
    } else {
        solve.retry_counter = 10;
        solve.retry_value = fabs(x1) < fabs(x2) ? x1 : x2;
    }
    if (x1 < x2) {
        solve.x1 = x1;
        solve.x2 = x2;
    } else {
        solve.x1 = x2;
        solve.x2 = x1;
    }
    solve.last_disp_time = 0;
    solve.toggle = 1;
    solve.keep_running = !should_i_stop_at_this_level() && program_running();
    return call_solve_fn(1, 1);
}

typedef struct {
    const char *text;
    int length;
} message_spec;

#define SOLVE_ROOT          0
#define SOLVE_SIGN_REVERSAL 1
#define SOLVE_EXTREMUM      2
#define SOLVE_BAD_GUESSES   3
#define SOLVE_CONSTANT      4

static const message_spec solve_message[] = {
    { NULL,             0 },
    { "Sign Reversal", 13 },
    { "Extremum",       8 },
    { "Bad Guess(es)", 13 },
    { "Constant?",      9 }
};

static int finish_solve(int message) {
    vartype *v, *new_x, *new_y, *new_z, *new_t;
    arg_struct arg;
    int dummy, print;

    solve.state = 0;

    if (solve.which == -1) {
        /* Ridders was terminated because it wasn't making progress; this does
         * not necessarily mean that x3 is the best guess so far. So, to be
         * sure, select the value with the lowest absulute function value.
         */
        phloat t1 = fabs(solve.fx1);
        phloat t2 = fabs(solve.fx2);
        phloat t3 = fabs(solve.curr_f);
        phloat t;
        if (t1 < t2) {
            solve.which = 1;
            t = t1;
        } else {
            solve.which = 2;
            t = t2;
        }
        if (t3 < t)
            solve.which = 3;
    }

    v = recall_var(solve.var_name, solve.var_length);
    ((vartype_real *) v)->x = solve.which == 1 ? solve.x1 :
                                solve.which == 2 ? solve.x2 : solve.x3;
    new_x = dup_vartype(v);
    new_y = new_real(solve.prev_x);
    new_z = new_real(solve.curr_f);
    new_t = new_real(message);
    if (new_x == NULL || new_y == NULL || new_z == NULL || new_t == NULL) {
        free_vartype(new_x);
        free_vartype(new_y);
        free_vartype(new_z);
        free_vartype(new_t);
        return ERR_INSUFFICIENT_MEMORY;
    }
    free_vartype(reg_x);
    free_vartype(reg_y);
    free_vartype(reg_z);
    free_vartype(reg_t);
    reg_x = new_x;
    reg_y = new_y;
    reg_z = new_z;
    reg_t = new_t;

    current_prgm = solve.prev_prgm;
    pc = solve.prev_pc;

    arg.type = ARGTYPE_STR;
    string_copy(arg.val.text, &dummy, solve.var_name, solve.var_length);
    arg.length = solve.var_length;

    print = flags.f.trace_print && flags.f.printer_exists;

    if (!solve.keep_running) {
        view_helper(&arg, print);
        if (message != 0) {
            clear_row(1);
            draw_string(0, 1, solve_message[message].text,
                              solve_message[message].length);
            flags.f.message = 1;
            flags.f.two_line_message = 1;
            flush_display();
        }
    } else {
        if (print) {
            char namebuf[8], valbuf[22];
            int namelen = 0, vallen;
            string2buf(namebuf, 8, &namelen, solve.var_name, solve.var_length);
            char2buf(namebuf, 8, &namelen, '=');
            vallen = vartype2string(v, valbuf, 22);
            print_wide(namebuf, namelen, valbuf, vallen);
        }
    }

    if (print && message != 0)
        print_lines(solve_message[message].text,
                    solve_message[message].length, 1);

    return solve.keep_running ? ERR_NONE : ERR_STOP;
}

#ifdef WINDOWS_DEBUG
static void printout(const phloat& p, const char* s) {
    char buf[256];
    buf[0] = 0;
    if (s) {
        strcpy(buf, s);
        strcat(buf, " ");
    }
    p.bcd.asString(buf + strlen(buf));
    strcat(buf, "\n");
    OutputDebugString(buf);
}
#endif

int return_to_solve(int failure, bool stop) {
    phloat f, slope, s, xnew, prev_f = solve.curr_f;
    uint4 now_time;

    if (stop)
        solve.keep_running = 0;

    if (solve.state == 0)
        return ERR_INTERNAL_ERROR;
    if (!failure) {
        if (reg_x->type == TYPE_REAL) {
            f = ((vartype_real *) reg_x)->x;
            solve.curr_f = f;
            if (f == 0)
                return finish_solve(SOLVE_ROOT);
        } else {
            solve.curr_f = POS_HUGE_PHLOAT;
            failure = 1;
        }
    } else
        solve.curr_f = POS_HUGE_PHLOAT;

    if (!failure && solve.retry_counter != 0) {
        if (solve.retry_counter > 0)
            solve.retry_counter--;
        else
            solve.retry_counter++;
    }

    now_time = shell_milliseconds();
    if (now_time < solve.last_disp_time)
        // shell_milliseconds() wrapped around
        solve.last_disp_time = 0;
    if (!solve.keep_running && solve.state > 1
                                && now_time >= solve.last_disp_time + 250) {
        /* Put on a show so the user won't think we're just drinking beer
         * while they're waiting anxiously for the solver to converge...
         */
        char buf[22];
        int bufptr = 0, i;
        solve.last_disp_time = now_time;
        clear_display();
        bufptr = phloat2string(solve.curr_x, buf, 22, 0, 0, 3,
                                    flags.f.thousands_separators);
        for (i = bufptr; i < 21; i++)
            buf[i] = ' ';
        buf[21] = failure ? '?' : solve.curr_f > 0 ? '+' : '-';
        draw_string(0, 0, buf, 22);
        bufptr = phloat2string(solve.prev_x, buf, 22, 0, 0, 3,
                                    flags.f.thousands_separators);
        for (i = bufptr; i < 21; i++)
            buf[i] = ' ';
        buf[21] = prev_f == POS_HUGE_PHLOAT ? '?' : prev_f > 0 ? '+' : '-';
        draw_string(0, 1, buf, 22);
        flush_display();
        flags.f.message = 1;
        flags.f.two_line_message = 1;
    }

    switch (solve.state) {

        case 1:
            /* first evaluation of x1 */
            if (failure) {
                if (solve.retry_counter > 0)
                    solve.retry_counter = -solve.retry_counter;
                return call_solve_fn(2, 2);
            } else {
                solve.fx1 = f;
                return call_solve_fn(2, 3);
            }

        case 2:
            /* first evaluation of x2 after x1 was unsuccessful */
            if (failure)
                return finish_solve(SOLVE_BAD_GUESSES);
            solve.fx2 = f;
            solve.x1 = (solve.x1 + solve.x2) / 2;
            if (solve.x1 == solve.x2)
                return finish_solve(SOLVE_BAD_GUESSES);
            return call_solve_fn(1, 3);

        case 3:
            /* make sure f(x1) != f(x2) */
            if (failure) {
                if (solve.which == 1)
                    solve.x1 = (solve.x1 + solve.x2) / 2;
                else
                    solve.x2 = (solve.x1 + solve.x2) / 2;
                if (solve.x1 == solve.x2)
                    return finish_solve(SOLVE_BAD_GUESSES);
                return call_solve_fn(solve.which, 3);
            }
            if (solve.which == 1)
                solve.fx1 = f;
            else
                solve.fx2 = f;
            if (solve.fx1 == solve.fx2) {
                /* If f(x1) == f(x2), we assume we're in a local flat spot.
                 * We extend the interval exponentially until we have two
                 * values of x, both of which are evaluated successfully,
                 * and yielding different values; from that moment on, we can
                 * apply the secant method.
                 */
                int which;
                phloat x;
                if (solve.toggle) {
                    x = solve.x2 + 100 * (solve.x2 - solve.x1);
                    if (p_isinf(x)) {
                        if (solve.retry_counter != 0)
                            goto retry_solve;
                        return finish_solve(SOLVE_CONSTANT);
                    }
                    which = 2;
                    solve.x2 = x;
                } else {
                    x = solve.x1 - 100 * (solve.x2 - solve.x1);
                    if (p_isinf(x)) {
                        if (solve.retry_counter != 0)
                            goto retry_solve;
                        return finish_solve(SOLVE_CONSTANT);
                    }
                    which = 1;
                    solve.x1 = x;
                }
                solve.toggle = !solve.toggle;
                return call_solve_fn(which, 3);
            }
            /* When we get here, f(x1) != f(x2), and we can start
             * applying the secant method.
             */
            goto do_secant;

        case 4:
            /* secant method, evaluated x3 */
        case 5:
            /* just performed bisection */
            if (failure) {
                if (solve.x3 > solve.x2) {
                    /* Failure outside [x1, x2]; approach x2 */
                    solve.x3 = (solve.x2 + solve.x3) / 2;
                    if (solve.x3 == solve.x2)
                        return finish_solve(SOLVE_EXTREMUM);
                } else if (solve.x3 < solve.x1) {
                    /* Failure outside [x1, x2]; approach x1 */
                    solve.x3 = (solve.x1 + solve.x3) / 2;
                    if (solve.x3 == solve.x1)
                        return finish_solve(SOLVE_EXTREMUM);
                } else {
                    /* Failure inside [x1, x2];
                     * alternately approach x1 and x2 */
                    if (solve.toggle) {
                        phloat old_x3 = solve.x3;
                        if (solve.x3 <= (solve.x1 + solve.x2) / 2)
                            solve.x3 = (solve.x1 + solve.x3) / 2;
                        else
                            solve.x3 = (solve.x2 + solve.x3) / 2;
                        if (solve.x3 == old_x3)
                            return finish_solve(SOLVE_SIGN_REVERSAL);
                    } else
                        solve.x3 = solve.x1 + solve.x2 - solve.x3;
                    solve.toggle = !solve.toggle;
                    if (solve.x3 == solve.x1 || solve.x3 == solve.x2)
                        return finish_solve(SOLVE_SIGN_REVERSAL);
                }
                return call_solve_fn(3, 4);
            } else if (solve.fx1 > 0 && solve.fx2 > 0) {
                if (solve.fx1 > solve.fx2) {
                    if (f >= solve.fx1 && solve.state != 5)
                        goto do_bisection;
                    solve.x1 = solve.x3;
                    solve.fx1 = f;
                } else {
                    if (f >= solve.fx2 && solve.state != 5)
                        goto do_bisection;
                    solve.x2 = solve.x3;
                    solve.fx2 = f;
                }
            } else if (solve.fx1 < 0 && solve.fx2 < 0) {
                if (solve.fx1 < solve.fx2) {
                    if (f <= solve.fx1 && solve.state != 5)
                        goto do_bisection;
                    solve.x1 = solve.x3;
                    solve.fx1 = f;
                } else {
                    if (f <= solve.fx2 && solve.state != 5)
                        goto do_bisection;
                    solve.x2 = solve.x3;
                    solve.fx2 = f;
                }
            } else {
                /* f(x1) and f(x2) have opposite signs; assuming f is
                 * continuous on the interval [x1, x2], there is at least
                 * one root. We use x3 to replace x1 or x2 and narrow the
                 * interval, even if f(x3) is actually worse than f(x1) and
                 * f(x2). This way we're guaranteed to home in on the root
                 * (but of course we'll get stuck if we encounter a
                 * discontinuous sign reversal instead, e.g. 1/x at x = 0).
                 * Such is life.
                 */
                if ((solve.fx1 > 0 && f > 0) || (solve.fx1 < 0 && f < 0)) {
                    solve.x1 = solve.x3;
                    solve.fx1 = f;
                } else {
                    solve.x2 = solve.x3;
                    solve.fx2 = f;
                }
            }
            if (solve.x2 < solve.x1) {
                /* Make sure x1 is always less than x2 */
                phloat tmp = solve.x1;
                solve.x1 = solve.x2;
                solve.x2 = tmp;
                tmp = solve.fx1;
                solve.fx1 = solve.fx2;
                solve.fx2 = tmp;
            }
            do_secant:
            if (solve.fx1 == solve.fx2)
                return finish_solve(SOLVE_EXTREMUM);
            if ((solve.fx1 > 0 && solve.fx2 < 0)
                    || (solve.fx1 < 0 && solve.fx2 > 0))
                goto do_ridders;
            slope = (solve.fx2 - solve.fx1) / (solve.x2 - solve.x1);
            if (p_isinf(slope)) {
                solve.x3 = (solve.x1 + solve.x2) / 2;
                if (solve.x3 == solve.x1 || solve.x3 == solve.x2)
                    return finish_solve(SOLVE_ROOT);
                else
                    return call_solve_fn(3, 4);
            } else if (slope == 0) {
                /* Underflow caused by x2 - x1 being too big.
                 * We're changing the calculation sequence to steer
                 * clear of trouble.
                 */
                solve.x3 = solve.x1 - solve.fx1 * (solve.x2 - solve.x1)
                                                / (solve.fx2 - solve.fx1);

                goto finish_secant;
            } else {
                int inf;
                solve.x3 = solve.x1 - solve.fx1 / slope;
                finish_secant:
                if ((inf = p_isinf(solve.x3)) != 0) {
                    if (solve.retry_counter != 0)
                        goto retry_solve;
                    return finish_solve(SOLVE_EXTREMUM);
                }
                /* The next two 'if' statements deal with the case that the
                 * secant extrapolation returns one of the points we already
                 * had. We assume this means no improvement is possible.
                 * We fudge the 'solve' struct a bit to make sure we don't
                 * return the 'bad' value as the root.
                 */
                if (solve.x3 == solve.x1) {
                    if (fabs(slope) > 1e50) {
                        // Not improving because slope too steep
                        solve.x3 = solve.x1 - (solve.x2 - solve.x1) / 100;
                        return call_solve_fn(3, 4);
                    }
                    solve.which = 1;
                    solve.curr_f = solve.fx1;
                    solve.prev_x = solve.x2;
                    return finish_solve(SOLVE_ROOT);
                }
                if (solve.x3 == solve.x2) {
                    if (fabs(slope) > 1e50) {
                        // Not improving because slope too steep
                        solve.x3 = solve.x2 + (solve.x2 - solve.x1) / 100;
                        return call_solve_fn(3, 4);
                    }
                    solve.which = 2;
                    solve.curr_f = solve.fx2;
                    solve.prev_x = solve.x1;
                    return finish_solve(SOLVE_ROOT);
                }
                /* If we're extrapolating, make sure we don't race away from
                 * the current interval too quickly */
                if (solve.x3 < solve.x1) {
                    phloat min = solve.x1 - 100 * (solve.x2 - solve.x1);
                    if (solve.x3 < min)
                        solve.x3 = min;
                } else if (solve.x3 > solve.x2) {
                    phloat max = solve.x2 + 100 * (solve.x2 - solve.x1);
                    if (solve.x3 > max)
                        solve.x3 = max;
                } else {
                    /* If we're interpolating, make sure we actually make
                     * some progress. Enforce a minimum distance between x3
                     * and the edges of the interval.
                     */
                    phloat eps = (solve.x2 - solve.x1) / 10;
                    if (solve.x3 < solve.x1 + eps)
                        solve.x3 = solve.x1 + eps;
                    else if (solve.x3 > solve.x2 - eps)
                        solve.x3 = solve.x2 - eps;
                }
                return call_solve_fn(3, 4);
            }

            retry_solve:
            /* We hit infinity without finding two values of x where f(x) has
             * opposite signs, but we got to infinity suspiciously quickly.
             * If we started with two guesses, we now retry with only the lower
             * of the two; if we started with one guess, we now retry with
             * starting guesses of 0 and 1.
             */
            if (solve.retry_counter > 0) {
                solve.x1 = solve.retry_value;
                solve.x2 = solve.x1 * 1.000001;
                if (p_isinf(solve.x2))
                    solve.x2 = solve.x1 * 0.999999;
                if (solve.x1 > solve.x2) {
                    phloat tmp = solve.x1;
                    solve.x1 = solve.x2;
                    solve.x2 = tmp;
                }
                solve.retry_counter = -10;
            } else {
                solve.x1 = 0;
                solve.x2 = 1;
                solve.retry_counter = 0;
            }
            return call_solve_fn(1, 1);

            do_bisection:
            solve.x3 = (solve.x1 + solve.x2) / 2;
            return call_solve_fn(3, 5);

        case 6:
            /* Ridders' method, evaluated midpoint */
            if (failure)
                goto do_bisection;
            s = sqrt(f * f - solve.fx1 * solve.fx2);
            if (s == 0) {
                /* Mathematically impossible, but numerically possible if
                 * the function is so close to zero that f^2 underflows.
                 * We could handle this better but this seems adequate.
                 */
                solve.which = -1;
                return finish_solve(SOLVE_ROOT);
            }
            solve.xm = solve.x3;
            solve.fxm = f;
            if (solve.fx1 < solve.fx2)
                s = -s;
            xnew = solve.xm + (solve.xm - solve.x1) * (solve.fxm / s);
            if (xnew == solve.x1 || xnew == solve.x2) {
                solve.which = -1;
                return finish_solve(SOLVE_ROOT);
            }
            solve.x3 = xnew;
            return call_solve_fn(3, 7);

        case 7:
            /* Ridders' method, evaluated xnew */
            if (failure)
                goto do_bisection;
            if ((f > 0 && solve.fxm < 0) || (f < 0 && solve.fxm > 0)) {
                if (solve.xm < solve.x3) {
                    solve.x1 = solve.xm;
                    solve.fx1 = solve.fxm;
                    solve.x2 = solve.x3;
                    solve.fx2 = f;
                } else {
                    solve.x1 = solve.x3;
                    solve.fx1 = f;
                    solve.x2 = solve.xm;
                    solve.fx2 = solve.fxm;
                }
            } else if ((f > 0 && solve.fx1 < 0) || (f < 0 && solve.fx1 > 0)) {
                solve.x2 = solve.x3;
                solve.fx2 = f;
            } else /* f > 0 && solve.fx2 < 0 || f < 0 && solve.fx2 > 0 */ {
                solve.x1 = solve.x3;
                solve.fx1 = f;
            }
            do_ridders:
            solve.x3 = (solve.x1 + solve.x2) / 2;
            // TODO: The following termination condition should really be
            //
            //  if (solve.x3 == solve.x1 || solve.x3 == solve.x2)
            //
            // since it is mathematically impossible for x3 to lie outside the
            // interval [x1, x2], but due to decimal round-off, it is, in fact,
            // possible, e.g. consider x1 = 7...2016 and x2 = 7...2018; this
            // results in x3 = 7...2015. Fixing this will require either
            // extended-precision math for solver internals, or at least a
            // slightly smarter implementation of the midpoint calculation,
            // e.g. fall back on x3 = x1 + (x2 - x1) / 2 if x3 = (x1 + x2) / 2
            // returns an incorrect result. 
            if (solve.x3 <= solve.x1 || solve.x3 >= solve.x2) {
                solve.which = -1;
                return finish_solve(SOLVE_ROOT);
            } else
                return call_solve_fn(3, 6);

        default:
            return ERR_INTERNAL_ERROR;
    }
}

static void reset_integ() {
    integ.prgm_length = 0;
    integ.active_prgm_length = 0;
    integ.state = 0;
    if (mode_appmenu == MENU_INTEG || mode_appmenu == MENU_INTEG_PARAMS)
        set_menu_return_err(MENULEVEL_APP, MENU_NONE, true);
}

void set_integ_prgm(const char *name, int length) {
    string_copy(integ.prgm_name, &integ.prgm_length, name, length);
}

void get_integ_prgm(char *name, int *length) {
    string_copy(name, length, integ.prgm_name, integ.prgm_length);
}

void set_integ_var(const char *name, int length) {
    string_copy(integ.var_name, &integ.var_length, name, length);
}

void get_integ_var(char *name, int *length) {
    string_copy(name, length, integ.var_name, integ.var_length);
}

static int call_integ_fn() {
    if (integ.active_prgm_length == 0)
        return ERR_NONEXISTENT;
    int err, i;
    arg_struct arg;
    phloat x = integ.u;
    vartype *v = recall_var(integ.var_name, integ.var_length);
    if (v == NULL || v->type != TYPE_REAL) {
        v = new_real(x);
        if (v == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        store_var(integ.var_name, integ.var_length, v);
    } else
        ((vartype_real *) v)->x = x;
    arg.type = ARGTYPE_STR;
    arg.length = integ.active_prgm_length;
    for (i = 0; i < arg.length; i++)
        arg.val.text[i] = integ.active_prgm_name[i];
    err = docmd_gto(&arg);
    if (err != ERR_NONE) {
        free_vartype(v);
        return err;
    }
    push_rtn_addr(-3, 0);
    return ERR_RUN;
}

int start_integ(const char *name, int length) {
    vartype *v;
    if (integ_active())
        return ERR_INTEG_INTEG;
    v = recall_var("LLIM", 4);
    if (v == NULL)
        return ERR_NONEXISTENT;
    else if (v->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else if (v->type != TYPE_REAL)
        return ERR_INVALID_TYPE;
    integ.llim = ((vartype_real *) v)->x;
    v = recall_var("ULIM", 4);
    if (v == NULL)
        return ERR_NONEXISTENT;
    else if (v->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else if (v->type != TYPE_REAL)
        return ERR_INVALID_TYPE;
    integ.ulim = ((vartype_real *) v)->x;
    v = recall_var("ACC", 3);
    if (v == NULL)
        integ.acc = 0;
    else if (v->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else if (v->type != TYPE_REAL)
        return ERR_INVALID_TYPE;
    else
        integ.acc = ((vartype_real *) v)->x;
    if (integ.acc < 0)
        integ.acc = 0;
    string_copy(integ.var_name, &integ.var_length, name, length);
    string_copy(integ.active_prgm_name, &integ.active_prgm_length,
                integ.prgm_name, integ.prgm_length);
    integ.prev_prgm = current_prgm;
    integ.prev_pc = pc;

    integ.a = integ.llim;
    integ.b = integ.ulim - integ.llim;
    integ.h = 2;
    integ.prev_int = 0;
    integ.nsteps = 1;
    integ.n = 1;
    integ.state = 1;
    integ.s[0] = 0;
    integ.k = 1;
    integ.prev_res = 0;

    integ.keep_running = !should_i_stop_at_this_level() && program_running();
    if (!integ.keep_running) {
        clear_row(0);
        draw_string(0, 0, "Integrating", 11);
        flush_display();
        flags.f.message = 1;
        flags.f.two_line_message = 0;
    }
    return return_to_integ(0, false);
}

static int finish_integ() {
    vartype *x, *y;
    int saved_trace = flags.f.trace_print;
    integ.state = 0;

    x = new_real(integ.sum * integ.b * 0.75);
    y = new_real(integ.eps);
    if (x == NULL || y == NULL) {
        free_vartype(x);
        free_vartype(y);
        return ERR_INSUFFICIENT_MEMORY;
    }
    flags.f.trace_print = 0;
    recall_two_results(x, y);
    flags.f.trace_print = saved_trace;

    current_prgm = integ.prev_prgm;
    pc = integ.prev_pc;

    if (!integ.keep_running) {
        char buf[22];
        int bufptr = 0;
        string2buf(buf, 22, &bufptr, "\003=", 2);
        bufptr += vartype2string(x, buf + bufptr, 22 - bufptr);
        clear_row(0);
        draw_string(0, 0, buf, bufptr);
        flush_display();
        flags.f.message = 1;
        flags.f.two_line_message = 0;
        if (flags.f.trace_print && flags.f.printer_exists)
            print_wide(buf, 2, buf + 2, bufptr - 2);
        return ERR_STOP;
    } else
        return ERR_NONE;
}


/* approximate integral of `f' between `a' and `b' subject to a given
 * error. Use Romberg method with refinement substitution, x = (3u-u^3)/2
 * which prevents endpoint evaluation and causes non-uniform sampling.
 */

int return_to_integ(int failure, bool stop) {
    if (stop)
        integ.keep_running = 0;
    
    switch (integ.state) {
    case 0:
        return ERR_INTERNAL_ERROR;

    case 1:
        integ.state = 2;

    loop1:

        integ.p = integ.h / 2 - 1;
        integ.sum = 0.0;
        integ.i = 0;

    loop2:

        integ.t = 1 - integ.p * integ.p;
        integ.u = integ.p + integ.t * integ.p / 2;
        integ.u = (integ.u * integ.b + integ.b) / 2 + integ.a;
        return call_integ_fn();

    case 2:
        if (!failure && reg_x->type == TYPE_REAL)
            integ.sum += integ.t * ((vartype_real *) reg_x)->x;
        integ.p += integ.h;
        if (++integ.i < integ.nsteps)
            goto loop2;

        // update integral moving resuslt
        integ.prev_int = (integ.prev_int + integ.sum*integ.h)/2;
        integ.s[integ.k++] = integ.prev_int;

        if (integ.n >= ROMB_K-1) {
            int i, m;
            int ns = ROMB_K-1;
            phloat dm = 1;
            for (i = 0; i < ROMB_K; ++i) integ.c[i] = integ.s[i];
            integ.sum = integ.s[ns];
            for (m = 1; m < ROMB_K; ++m) {
                dm /= 4;
                for (i = 0; i < ROMB_K-m; ++i)
                    integ.c[i] = (integ.c[i+1]-integ.c[i]*dm*4)/(1-dm);
                integ.sum += integ.c[--ns]*dm;
            }

            phloat res = integ.sum * integ.b * 0.75;
            integ.eps = fabs(integ.prev_res - res);
            integ.prev_res = res;
            if (integ.eps <= integ.acc * fabs(res))
                // done!
                return finish_integ();

            for (i = 0; i < ROMB_K-1; ++i) integ.s[i] = integ.s[i+1];
            integ.k = ROMB_K-1;
        }

        integ.nsteps <<= 1;
        integ.h /= 2.0;

        if (++integ.n >= ROMB_MAX)
            return finish_integ(); // too many
        
        goto loop1;
    default:
        return ERR_INTERNAL_ERROR;
    }
}
