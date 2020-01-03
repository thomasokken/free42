/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2020  Thomas Okken
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
#include <stdarg.h>

#include "core_commands2.h"
#include "core_commands7.h"
#include "core_display.h"
#include "core_helpers.h"
#include "core_main.h"
#include "core_variables.h"
#include "shell.h"

/////////////////////////////////////////////////////////////////
///// Accelerometer, Location Services, and Compass support /////
/////////////////////////////////////////////////////////////////

#if defined(ANDROID) || defined(IPHONE)
int docmd_accel(arg_struct *arg) {
    if (!core_settings.enable_ext_accel)
        return ERR_NONEXISTENT;
    double x, y, z;
    int err = shell_get_acceleration(&x, &y, &z);
    if (err == 0)
        return ERR_NONEXISTENT;
    vartype *new_x = new_real(x);
    vartype *new_y = new_real(y);
    vartype *new_z = new_real(z);
    if (new_x == NULL || new_y == NULL || new_z == NULL) {
        free_vartype(new_x);
        free_vartype(new_y);
        free_vartype(new_z);
        return ERR_INSUFFICIENT_MEMORY;
    }
    free_vartype(reg_t);
    free_vartype(reg_z);
    if (flags.f.stack_lift_disable) {
        free_vartype(reg_x);
        reg_t = reg_y;
    } else {
        free_vartype(reg_y);
        reg_t = reg_x;
    }
    reg_z = new_z;
    reg_y = new_y;
    reg_x = new_x;
    if (flags.f.trace_print && flags.f.printer_exists)
        docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_locat(arg_struct *arg) {
    if (!core_settings.enable_ext_locat)
        return ERR_NONEXISTENT;
    double lat, lon, lat_lon_acc, elev, elev_acc;
    int err = shell_get_location(&lat, &lon, &lat_lon_acc, &elev, &elev_acc);
    if (err == 0)
        return ERR_NONEXISTENT;
    vartype *new_x = new_real(lat);
    vartype *new_y = new_real(lon);
    vartype *new_z = new_real(elev);
    vartype *new_t = new_realmatrix(1, 2);
    if (new_x == NULL || new_y == NULL || new_z == NULL || new_t == NULL) {
        free_vartype(new_x);
        free_vartype(new_y);
        free_vartype(new_z);
        free_vartype(new_t);
        return ERR_INSUFFICIENT_MEMORY;
    }
    vartype_realmatrix *rm = (vartype_realmatrix *) new_t;
    rm->array->data[0] = lat_lon_acc;
    rm->array->data[1] = elev_acc;
    free_vartype(reg_t);
    free_vartype(reg_z);
    free_vartype(reg_y);
    free_vartype(reg_x);
    reg_t = new_t;
    reg_z = new_z;
    reg_y = new_y;
    reg_x = new_x;
    if (flags.f.trace_print && flags.f.printer_exists)
        docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_heading(arg_struct *arg) {
    if (!core_settings.enable_ext_heading)
        return ERR_NONEXISTENT;
    double mag_heading, true_heading, acc, x, y, z;
    int err = shell_get_heading(&mag_heading, &true_heading, &acc, &x, &y, &z);
    if (err == 0)
        return ERR_NONEXISTENT;
    vartype *new_x = new_real(mag_heading);
    vartype *new_y = new_real(true_heading);
    vartype *new_z = new_real(acc);
    vartype *new_t = new_realmatrix(1, 3);
    if (new_x == NULL || new_y == NULL || new_z == NULL || new_t == NULL) {
        free_vartype(new_x);
        free_vartype(new_y);
        free_vartype(new_z);
        free_vartype(new_t);
        return ERR_INSUFFICIENT_MEMORY;
    }
    vartype_realmatrix *rm = (vartype_realmatrix *) new_t;
    rm->array->data[0] = x;
    rm->array->data[1] = y;
    rm->array->data[2] = z;
    free_vartype(reg_t);
    free_vartype(reg_z);
    free_vartype(reg_y);
    free_vartype(reg_x);
    reg_t = new_t;
    reg_z = new_z;
    reg_y = new_y;
    reg_x = new_x;
    if (flags.f.trace_print && flags.f.printer_exists)
        docmd_prx(NULL);
    return ERR_NONE;
}
#endif

/////////////////////////////////////////////////
///// HP-41 Time Module & CX Time emulation /////
/////////////////////////////////////////////////

static int date2comps(phloat x, int4 *yy, int4 *mm, int4 *dd) {
    int4 y, m, d;
#ifdef BCD_MATH
    if (flags.f.ymd) {
        y = to_int4(floor(x));
        m = to_int4(floor((x - y) * 100));
        d = to_int4(x * 10000) % 100;
    } else {
        m = to_int4(floor(x));
        d = to_int4(floor((x - m) * 100));
        y = to_int4(x * 1000000) % 10000;
    }
#else
    if (flags.f.ymd) {
        y = to_int4(floor(x));
        int4 r = (int4) floor((x - y) * 100000000 + 0.5);
        r /= 10000;
        m = r / 100;
        d = r % 100;
    } else {
        m = to_int4(floor(x));
        int4 r = (int4) floor((x - m) * 100000000 + 0.5);
        r /= 100;
        d = r / 10000;
        y = r % 10000;
    }
#endif

    if (flags.f.dmy) {
        int4 t = m;
        m = d;
        d = t;
    }

    if (y < 1582 || y > 4320 || m < 1 || m > 12 || d < 1 || d > 31)
        return ERR_INVALID_DATA;
    if ((m == 4 || m == 6 || m == 9 || m == 11) && d == 31)
        return ERR_INVALID_DATA;
    if (m == 2 && d > ((y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 29 : 28))
        return ERR_INVALID_DATA;
    if (y == 1582 && (m < 10 || m == 10 && d < 15)
            || y == 4320 && (m > 9 || m == 9 && d > 10))
        return ERR_INVALID_DATA;

    *yy = y;
    *mm = m;
    *dd = d;
    return ERR_NONE;
}

static phloat comps2date(int4 y, int4 m, int4 d) {
    if (flags.f.ymd)
        return phloat(y * 10000 + m * 100 + d) / 10000;
    if (flags.f.dmy) {
        int4 t = m;
        m = d;
        d = t;
    }
    return phloat(m * 1000000 + d * 10000 + y) / 1000000;
}

/* Gregorian Date <-> Day Number conversion functions
 * Algorithm due to Henry F. Fliegel and Thomas C. Van Flandern,
 * Communications of the ACM, Vol. 11, No. 10 (October, 1968).
 */
static int greg2jd(int4 y, int4 m, int4 d, int4 *jd) {
    *jd = ( 1461 * ( y + 4800 + ( m - 14 ) / 12 ) ) / 4 +
          ( 367 * ( m - 2 - 12 * ( ( m - 14 ) / 12 ) ) ) / 12 -
          ( 3 * ( ( y + 4900 + ( m - 14 ) / 12 ) / 100 ) ) / 4 +
          d - 32075;
    return ERR_NONE;
}

static int jd2greg(int4 jd, int4 *y, int4 *m, int4 *d) {
    if (jd < 2299161 || jd > 3299160)
        return ERR_OUT_OF_RANGE;
    int4 l = jd + 68569;
    int4 n = ( 4 * l ) / 146097;
    l = l - ( 146097 * n + 3 ) / 4;
    int4 i = ( 4000 * ( l + 1 ) ) / 1461001;
    l = l - ( 1461 * i ) / 4 + 31;
    int4 j = ( 80 * l ) / 2447;
    *d = l - ( 2447 * j ) / 80;
    l = j / 11;
    *m = j + 2 - ( 12 * l );
    *y = 100 * ( n - 49 ) + i + l;
    return ERR_NONE;
}


int docmd_adate(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
        return ERR_NONEXISTENT;
    if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_x->type != TYPE_REAL)
        return ERR_INVALID_TYPE;

    phloat x = ((vartype_real *) reg_x)->x;
    if (x < 0)
        x = -x;

    int digits;
    if (flags.f.fix_or_all && flags.f.eng_or_all)
        digits = 11;
    else {
        digits = 0;
        if (flags.f.digits_bit3)
            digits += 8;
        if (flags.f.digits_bit2)
            digits += 4;
        if (flags.f.digits_bit1)
            digits += 2;
        if (flags.f.digits_bit0)
            digits += 1;
    }

    char buf[10];
    int bufptr = 0;

    if (flags.f.ymd) {
        if (x >= 10000)
            return ERR_INVALID_DATA;

        int4 y = to_int4(floor(x));
#ifdef BCD_MATH
        int4 m = to_int4(floor((x - y) * 100));
        int4 d = to_int4(x * 10000) % 100;
#else
        int4 r = (int4) floor((x - y) * 100000000 + 0.5);
        r /= 10000;
        int4 m = r / 100;
        int4 d = r % 100;
#endif
        bufptr += int2string(y, buf + bufptr, 10 - bufptr);
        if (digits > 0) {
            char2buf(buf, 10, &bufptr, '-');
            if (m < 10)
                char2buf(buf, 10, &bufptr, '0');
            bufptr += int2string(m, buf + bufptr, 10 - bufptr);
            if (digits > 2) {
                char2buf(buf, 10, &bufptr, '-');
                if (d < 10)
                    char2buf(buf, 10, &bufptr, '0');
                bufptr += int2string(d, buf + bufptr, 10 - bufptr);
            }
        }
    } else {
        if (x >= 100)
            return ERR_INVALID_DATA;

        int4 m = to_int4(floor(x));
#ifdef BCD_MATH
        int4 d = to_int4(floor((x - m) * 100));
        int4 y = to_int4(x * 1000000) % 10000;
#else
        int4 r = (int4) floor((x - m) * 100000000 + 0.5);
        r /= 100;
        int4 d = r / 10000;
        int4 y = r % 10000;
#endif
        int c = y / 100;
        y %= 100;

        if (m < 10)
            char2buf(buf, 10, &bufptr, '0');
        bufptr += int2string(m, buf + bufptr, 10 - bufptr);
        if (digits > 0) {
            char2buf(buf, 10, &bufptr, flags.f.dmy ? '.' : '/');
            if (d < 10)
                char2buf(buf, 10, &bufptr, '0');
            bufptr += int2string(d, buf + bufptr, 10 - bufptr);
            if (digits > 2) {
                char2buf(buf, 10, &bufptr, flags.f.dmy ? '.' : '/');
                if (digits > 4) {
                    if (c < 10)
                        char2buf(buf, 10, &bufptr, '0');
                    bufptr += int2string(c, buf + bufptr, 10 - bufptr);
                }
                if (y < 10)
                    char2buf(buf, 10, &bufptr, '0');
                bufptr += int2string(y, buf + bufptr, 10 - bufptr);
            }
        }
    }

    append_alpha_string(buf, bufptr, 0);
    return ERR_NONE;
}

int docmd_atime(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
        return ERR_NONEXISTENT;
    if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_x->type != TYPE_REAL)
        return ERR_INVALID_TYPE;

    phloat x = ((vartype_real *) reg_x)->x;
    bool neg = x < 0;
    if (neg)
        x = -x;
    if (x >= 100)
        return ERR_INVALID_DATA;
    int h = to_int(floor(x));
    if (h == 0)
        neg = false;
    int4 ms = to_int4(floor((x - floor(x)) * 1000000));
    int m = (int) (ms / 10000);
    int s = (int) (ms / 100 % 100);
    int cs = (int) (ms % 100);
    bool am = false;
    bool pm = false;

    if (mode_time_clk24) {
        if (neg && h >= 1 && h <= 11)
            h += 12;
    } else if (h < 24) {
        if (!neg && h < 12)
            am = true;
        else
            pm = true;
        if (h == 0)
            h = 12;
        else if (h > 12)
            h -= 12;
    }

    int digits;
    if (flags.f.fix_or_all && flags.f.eng_or_all)
        digits = 11;
    else {
        digits = 0;
        if (flags.f.digits_bit3)
            digits += 8;
        if (flags.f.digits_bit2)
            digits += 4;
        if (flags.f.digits_bit1)
            digits += 2;
        if (flags.f.digits_bit0)
            digits += 1;
    }

    char buf[14];
    int bufptr = 0;
    if (h < 10)
        char2buf(buf, 14, &bufptr, mode_time_clk24 ? '0' : ' ');
    bufptr += int2string(h, buf + bufptr, 14 - bufptr);
    if (digits > 0) {
        char2buf(buf, 14, &bufptr, ':');
        if (m < 10)
            char2buf(buf, 14, &bufptr, '0');
        bufptr += int2string(m, buf + bufptr, 14 - bufptr);
        if (digits > 2) {
            char2buf(buf, 14, &bufptr, ':');
            if (s < 10)
                char2buf(buf, 14, &bufptr, '0');
            bufptr += int2string(s, buf + bufptr, 14 - bufptr);
            if (digits > 4) {
                char2buf(buf, 14, &bufptr, '.');
                if (cs < 10)
                    char2buf(buf, 14, &bufptr, '0');
                bufptr += int2string(cs, buf + bufptr, 14 - bufptr);
            }
        }
    }
    if (am)
        string2buf(buf, 14, &bufptr, " AM", 3);
    else if (pm)
        string2buf(buf, 14, &bufptr, " PM", 3);
    append_alpha_string(buf, bufptr, 0);

    return ERR_NONE;
}

int docmd_atime24(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
        return ERR_NONEXISTENT;
    bool saved_clk24 = mode_time_clk24;
    mode_time_clk24 = true;
    int res = docmd_atime(arg);
    mode_time_clk24 = saved_clk24;
    return res;
}

int docmd_clk12(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
        return ERR_NONEXISTENT;
    mode_time_clk24 = false;
    return ERR_NONE;
}

int docmd_clk24(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
        return ERR_NONEXISTENT;
    mode_time_clk24 = true;
    return ERR_NONE;
}

static char weekdaynames[] = "SUNMONTUEWEDTHUFRISAT";

int docmd_date(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
        return ERR_NONEXISTENT;
    uint4 date;
    int weekday;
    shell_get_time_date(NULL, &date, &weekday);
    int y = date / 10000;
    int m = date / 100 % 100;
    int d = date % 100;
    if (flags.f.ymd)
        date = y * 10000L + m * 100 + d;
    else if (flags.f.dmy)
        date = y + m * 10000L + d * 1000000;
    else
        date = y + m * 1000000 + d * 10000L;
    vartype *new_x = new_real((int4) date);
    if (new_x == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    ((vartype_real *) new_x)->x /= flags.f.ymd ? 10000 : 1000000;
    if (!program_running()) {
        /* Note: I'm not completely faithful to the HP-41 here. It formats the
         * date as "14.03.2010 SUN" in DMY mode, and as "03/14/2010:SU" in MDY
         * mode. I mimic the former, but the latter I changed to
         * "03/14/2010 SUN"; the MDY display format used on the HP-41 is the
         * way it is because that was all they could fit in its 12-character
         * display. (Note that the periods in the DMY format and the colon in
         * the MDY format don't take up a character position on the HP-41.)
         */
        char buf[22];
        int bufptr = 0;
        if (flags.f.ymd) {
            bufptr += int2string(y, buf + bufptr, 22 - bufptr);
            char2buf(buf, 22, &bufptr, '-');
            if (m < 10)
                char2buf(buf, 22, &bufptr, '0');
            bufptr += int2string(m, buf + bufptr, 22 - bufptr);
            char2buf(buf, 22, &bufptr, '-');
            if (d < 10)
                char2buf(buf, 22, &bufptr, '0');
            bufptr += int2string(d, buf + bufptr, 22 - bufptr);
        } else {
            int n = flags.f.dmy ? d : m;
            if (n < 10)
                char2buf(buf, 22, &bufptr, '0');
            bufptr += int2string(n, buf + bufptr, 22 - bufptr);
            char2buf(buf, 22, &bufptr, flags.f.dmy ? '.' : '/');
            n = flags.f.dmy ? m : d;
            if (n < 10)
                char2buf(buf, 22, &bufptr, '0');
            bufptr += int2string(n, buf + bufptr, 22 - bufptr);
            char2buf(buf, 22, &bufptr, flags.f.dmy ? '.' : '/');
            bufptr += int2string(y, buf + bufptr, 22 - bufptr);
        }
        char2buf(buf, 22, &bufptr, ' ');
        string2buf(buf, 22, &bufptr, weekdaynames + weekday * 3, 3);
        clear_row(0);
        draw_string(0, 0, buf, bufptr);
        flush_display();
        flags.f.message = 1;
        flags.f.two_line_message = 0;
        if (flags.f.trace_print && flags.f.printer_exists)
            print_text(buf, bufptr, 1);
    }
    recall_result(new_x);
    return ERR_NONE;
}

int docmd_date_plus(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
        return ERR_NONEXISTENT;
    // TODO: Accept real matrices as well?
    if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_x->type != TYPE_REAL)
        return ERR_INVALID_TYPE;
    if (reg_y->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_y->type != TYPE_REAL)
        return ERR_INVALID_TYPE;

    phloat date = ((vartype_real *) reg_y)->x;
    if (date < 0 || date > (flags.f.ymd ? 10000 : 100))
        return ERR_INVALID_DATA;
    phloat days = ((vartype_real *) reg_x)->x;
    if (days < -1000000 || days > 1000000)
        return ERR_OUT_OF_RANGE;

    int4 y, m, d, jd;
    int err = date2comps(date, &y, &m, &d);
    if (err != ERR_NONE)
        return err;
    err = greg2jd(y, m, d, &jd);
    if (err != ERR_NONE)
        return err;
    jd += to_int4(floor(days));
    err = jd2greg(jd, &y, &m, &d);
    if (err != ERR_NONE)
        return err;
    date = comps2date(y, m, d);

    vartype *new_x = new_real(date);
    if (new_x == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    binary_result(new_x);
    return ERR_NONE;
}

int docmd_ddays(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
        return ERR_NONEXISTENT;
    // TODO: Accept real matrices as well?
    if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_x->type != TYPE_REAL)
        return ERR_INVALID_TYPE;
    if (reg_y->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_y->type != TYPE_REAL)
        return ERR_INVALID_TYPE;

    phloat date1 = ((vartype_real *) reg_y)->x;
    if (date1 < 0 || date1 > (flags.f.ymd ? 10000 : 100))
        return ERR_INVALID_DATA;
    phloat date2 = ((vartype_real *) reg_x)->x;
    if (date2 < 0 || date2 > (flags.f.ymd ? 10000 : 100))
        return ERR_INVALID_DATA;
    int4 y, m, d, jd1, jd2;
    int err = date2comps(date1, &y, &m, &d);
    if (err != ERR_NONE)
        return err;
    err = greg2jd(y, m, d, &jd1);
    if (err != ERR_NONE)
        return err;
    err = date2comps(date2, &y, &m, &d);
    if (err != ERR_NONE)
        return err;
    err = greg2jd(y, m, d, &jd2);
    if (err != ERR_NONE)
        return err;

    vartype *new_x = new_real(jd2 - jd1);
    if (new_x == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    binary_result(new_x);
    return ERR_NONE;
}

int docmd_dmy(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
        return ERR_NONEXISTENT;
    flags.f.dmy = true;
    flags.f.ymd = false;
    return ERR_NONE;
}

int docmd_dow(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
        return ERR_NONEXISTENT;
    // TODO: Accept real matrices as well?
    if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_x->type != TYPE_REAL)
        return ERR_INVALID_TYPE;

    phloat x = ((vartype_real *) reg_x)->x;
    if (x < 0 || x > (flags.f.ymd ? 10000 : 100))
        return ERR_INVALID_DATA;

    int4 y, m, d, jd;
    int err = date2comps(x, &y, &m, &d);
    if (err != ERR_NONE)
        return err;
    err = greg2jd(y, m, d, &jd);
    if (err != ERR_NONE)
        return err;
    jd = (jd + 1) % 7;

    vartype *new_x = new_real(jd);
    if (new_x == NULL)
        return ERR_INSUFFICIENT_MEMORY;

    if (!program_running()) {
        clear_row(0);
        draw_string(0, 0, weekdaynames + jd * 3, 3);
        flush_display();
        flags.f.message = 1;
        flags.f.two_line_message = 0;
        if (flags.f.trace_print && flags.f.printer_exists)
            print_text(weekdaynames + jd * 3, 3, 1);
    }

    unary_result(new_x);
    return ERR_NONE;
}

int docmd_mdy(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
        return ERR_NONEXISTENT;
    flags.f.dmy = false;
    flags.f.ymd = false;
    return ERR_NONE;
}

int docmd_time(arg_struct *arg) {
    if (!core_settings.enable_ext_time)
        return ERR_NONEXISTENT;
    uint4 time;
    shell_get_time_date(&time, NULL, NULL);
    vartype *new_x = new_real((int4) time);
    if (new_x == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    ((vartype_real *) new_x)->x /= 1000000;
    if (!program_running()) {
        int h = time / 1000000;
        bool am;
        if (!mode_time_clk24) {
            am = h < 12;
            h = h % 12;
            if (h == 0)
                h = 12;
        }
        int m = time / 10000 % 100;
        int s = time / 100 % 100;
        char buf[22];
        int bufptr = 0;
        if (h < 10)
            char2buf(buf, 22, &bufptr, ' ');
        bufptr += int2string(h, buf + bufptr, 22 - bufptr);
        char2buf(buf, 22, &bufptr, ':');
        if (m < 10)
            char2buf(buf, 22, &bufptr, '0');
        bufptr += int2string(m, buf + bufptr, 22 - bufptr);
        char2buf(buf, 22, &bufptr, ':');
        if (s < 10)
            char2buf(buf, 22, &bufptr, '0');
        bufptr += int2string(s, buf + bufptr, 22 - bufptr);
        if (!mode_time_clk24) {
            char2buf(buf, 22, &bufptr, ' ');
            char2buf(buf, 22, &bufptr, am ? 'A' : 'P');
            char2buf(buf, 22, &bufptr, 'M');
        }
        clear_row(0);
        draw_string(0, 0, buf, bufptr);
        flush_display();
        flags.f.message = 1;
        flags.f.two_line_message = 0;
        if (flags.f.trace_print && flags.f.printer_exists)
            print_text(buf, bufptr, 1);
    }
    recall_result(new_x);
    return ERR_NONE;
}

// The YMD function is not an original Time Module function, and in Free42,
// it is grouped with the "Programming" extension, but logically, of course,
// it belongs here. Also, most of the YMD implementation consists of
// modifications to Time Module functions, so in that sense, most of it is
// here anyway.

int docmd_ymd(arg_struct *arg) {
    if (!core_settings.enable_ext_prog)
        return ERR_NONEXISTENT;
    flags.f.dmy = false;
    flags.f.ymd = true;
    return ERR_NONE;
}

////////////////////////////////////////////////////////////////
///// Intel Decimal Floating-Point Math Library: self-test /////
////////////////////////////////////////////////////////////////

#ifdef FREE42_FPTEST

static int tests_lineno;
extern const char *readtest_lines[];

extern "C" {
    int readtest_main(int argc, char *argv[]);

    int tests_eof() {
        return readtest_lines[tests_lineno] == NULL;
    }

    void tests_readline(char *buf, int bufsize) {
        const char *line = readtest_lines[tests_lineno++];
        strncpy(buf, line, bufsize - 1);
        buf[bufsize - 1] = 0;
    }

    int testlogprintf(const char *fmt, ...) {
        int c;
        va_list ap;
        char text[1024];
        va_start(ap, fmt);
        c = vsprintf(text, fmt, ap);
        shell_log(text);
        va_end(ap);
        return c;
    }
}

int docmd_fptest(arg_struct *arg) {
    if (!core_settings.enable_ext_fptest)
        return ERR_NONEXISTENT;
    tests_lineno = 0;
    char *argv[] = { (char *) "readtest", NULL };
    int result = readtest_main(1, argv);
    vartype *v = new_real(result);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    recall_result(v);
    return ERR_NONE;
}

#else

int docmd_fptest(arg_struct *arg) {
    return ERR_NONEXISTENT;
}

#endif

/////////////////////////////////
///// Programming Extension /////
/////////////////////////////////

int docmd_lsto(arg_struct *arg) {
    if (!core_settings.enable_ext_prog)
        return ERR_NONEXISTENT;
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
    /* Only allow matrices to be stored in "REGS" */
    if (string_equals(arg->val.text, arg->length, "REGS", 4)
            && reg_x->type != TYPE_REALMATRIX
            && reg_x->type != TYPE_COMPLEXMATRIX)
        return ERR_RESTRICTED_OPERATION;
    /* When EDITN is active, don't allow the matrix being
     * edited to be overwritten. */
    if (matedit_mode == 3 && string_equals(arg->val.text,
                arg->length, matedit_name, matedit_length))
        return ERR_RESTRICTED_OPERATION;
    vartype *newval = dup_vartype(reg_x);
    if (newval == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    return store_var(arg->val.text, arg->length, newval, true);
}

int docmd_wsize(arg_struct *arg) {
    if (!core_settings.enable_ext_prog)
        return ERR_NONEXISTENT;
    if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    if (reg_x->type != TYPE_REAL)
        return ERR_INVALID_TYPE;
    phloat x = ((vartype_real *) reg_x)->x;
#ifdef BCD_MATH
    if (x >= 65 || x < 1)
#else
    if (x >= 53 || x < 1)
#endif
        return ERR_INVALID_DATA;
    mode_wsize = to_int(x);
    if (flags.f.trace_print && flags.f.printer_exists)
        docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_wsize_t(arg_struct *arg) {
    if (!core_settings.enable_ext_prog)
        return ERR_NONEXISTENT;
    vartype *new_x = new_real(effective_wsize());
    if (new_x == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    recall_result(new_x);
    return ERR_NONE;
}

int docmd_bsigned(arg_struct *arg) {
    if (!core_settings.enable_ext_prog)
        return ERR_NONEXISTENT;
    flags.f.base_signed = !flags.f.base_signed;
    return ERR_NONE;
}

int docmd_bwrap(arg_struct *arg) {
    if (!core_settings.enable_ext_prog)
        return ERR_NONEXISTENT;
    flags.f.base_wrap = !flags.f.base_wrap;
    return ERR_NONE;
}

int docmd_breset(arg_struct *arg) {
    if (!core_settings.enable_ext_prog)
        return ERR_NONEXISTENT;
    mode_wsize = 36;
    flags.f.base_signed = 1;
    flags.f.base_wrap = 0;
    return ERR_NONE;
}
