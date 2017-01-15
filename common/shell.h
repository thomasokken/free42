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

#ifndef SHELL_H
#define SHELL_H 1

#include "free42.h"

/* shell_blitter()
 *
 * Callback invoked by the emulator core to cause the display, or some portion
 * of it, to be repainted.
 *
 * 'bits' is a pointer to a 1 bpp (monochrome) bitmap. The bits within a byte
 * are laid out with left corresponding to least significant, right
 * corresponding to most significant; this corresponds to the convention for
 * X11 images, but it is the reverse of the convention for MacOS and its
 * derivatives (Microsoft Windows and PalmOS).
 * The bytes are laid out sequentially, that is, bits[0] is at the top
 * left corner, bits[1] is to the right of bits[0], bits[2] is to the right of
 * bits[1], and so on; this corresponds to X11, MacOS, Windows, and PalmOS
 * usage.
 * 'bytesperline' is the number of bytes per line of the bitmap; this means
 * that the bits just below bits[0] are at bits[bytesperline].
 * 'x', 'y', 'width', and 'height' define the part of the bitmap that needs to
 * be repainted. 'x' and 'y' are 0-based coordinates, with (0, 0) being the top
 * left corner of the bitmap, and x coordinates increasing to the right, and y
 * coordinates increasing downwards. 'width' and 'height' are the width and
 * height of the area to be repainted.
 */
void shell_blitter(const char *bits, int bytesperline, int x, int y,
                             int width, int height);

/* shell_beeper()
 * Callback invoked by the emulator core to play a sound.
 * The first parameter is the frequency in Hz; the second is the
 * duration in ms. The sound volume is up to the GUI to control.
 * Sound playback should be synchronous (the beeper function should
 * not return until the sound has finished), if possible.
 */
void shell_beeper(int frequency, int duration);

/* shell_annunciators()
 * Callback invoked by the emulator core to change the state of the display
 * annunciators (up/down, shift, print, run, battery, (g)rad).
 * Every parameter can have values 0 (turn off), 1 (turn on), or -1 (leave
 * unchanged).
 * The battery annunciator is missing from the list; this is the only one of
 * the lot that the emulator core does not actually have any control over, and
 * so the shell is expected to handle that one by itself.
 */
void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad);

/* shell_wants_cpu()
 *
 * Callback used by the emulator core to check for pending events.
 * It calls this periodically during long operations, such as running a
 * user program, or the solver, etc. The shell should not handle any events
 * in this call! If there are pending events, it should return 1; the currently
 * active invocation of core_keydown() or core_keyup() will then return
 * immediately (with a return value of 1, to indicate that it would like to get
 * the CPU back as soon as possible).
 */
int shell_wants_cpu();

/* Callback to suspend execution for the given number of milliseconds. No event
 * processing will take place during the wait, so the core can call this
 * without having to worry about core_keydown() etc. being re-entered.
 */
void shell_delay(int duration);

/* Callback to ask the shell to call core_timeout3() after the given number of
 * milliseconds. If there are keystroke events during that time, the timeout is
 * cancelled. (Pressing 'shift' does not cancel the timeout.)
 * This function supports the delay after SHOW, MEM, and shift-VARMENU.
 */
void shell_request_timeout3(int delay);

/* shell_read_saved_state()
 *
 * Callback to read from the saved state. The function will read up to n
 * bytes into the buffer pointed to by buf, and return the number of bytes
 * actually read. The function returns -1 if an error was encountered; a return
 * value of 0 signifies the end of input.
 * The emulator core should only call this function from core_init(), and only
 * if core_init() was called with an argument of 1. (Nothing horrible will
 * happen if you try to call this function during other contexts, but you will
 * always get an error then.)
 */
int4 shell_read_saved_state(void *buf, int4 bufsize);

/* shell_write_saved_state()
 * Callback to dump the saved state to persistent storage.
 * Returns 1 on success, 0 on error.
 * The emulator core should only call this function from core_quit(). (Nothing
 * horrible will happen if you try to call this function during other contexts,
 * but you will always get an error then.)
 */
bool shell_write_saved_state(const void *buf, int4 nbytes);

/* shell_get_mem()
 * Callback to get the amount of free memory in bytes.
 */
uint4 shell_get_mem();

/* shell_low_battery()
 * Callback to find out if the battery is low. Used to emulate flag 49 and the
 * battery annunciator, and also taken into account when deciding whether or
 * not to allow a power-down -- so as long as the shell provides a functional
 * implementation of shell_low_battery(), it can leave the decision on how to
 * respond to sysNotifySleepRequestEvent to core_allows_powerdown().
 */
int shell_low_battery();

/* shell_powerdown()
 * Callback to tell the shell that the emulator wants to power down.
 * Only called in response to OFF (shift-EXIT or the OFF command); automatic
 * power-off is left to the OS and/or shell.
 */
void shell_powerdown();

/* shell_random_seed()
 * When SEED is invoked with X = 0, the random number generator should be
 * seeded to a random value; the emulator core calls this function to obtain
 * it. The shell should construct a double in the range [0, 1) in a random
 * manner, using the real-time clock or some other source of randomness.
 * Note that distribution is not very important; the value will only be used to
 * seed the RNG. What's important that using shell_random_seed() guarantees
 * that the RNG will be initialized to a different sequence. This matters for
 * applications like games where you don't want the same sequence of cards
 * dealt each time.
 */
double shell_random_seed();

/* shell_milliseconds()
 * Returns an elapsed-time value in milliseconds. The caller should make no
 * assumptions as to what this value is relative to; it is only intended to
 * allow the emulator core make short-term elapsed-time measurements.
 */
uint4 shell_milliseconds();

/* shell_decimal_point()
 * Returns 0 if the host's locale uses comma as the decimal separator;
 * returns 1 if it uses dot or anything else.
 * Used to initialize flag 28 on hard reset.
 */
int shell_decimal_point();

/* shell_print()
 * Printer emulation. The first 2 parameters are the plain text version of the
 * data to be printed; the remaining 6 parameters are the bitmap version. The
 * former is used for text-mode copying and for spooling to text files; the
 * latter is used for graphics-mode coopying, spooling to image files, and
 * on-screen display.
 */
void shell_print(const char *text, int length,
                 const char *bits, int bytesperline,
                 int x, int y, int width, int height);

/* shell_write()
 *
 * Callback for core_export_programs(). Returns 0 if a problem occurred;
 * core_export_programs() should abort in that case.
 */
int shell_write(const char *buf, int4 buflen);

/* shell_read()
 *
 * Callback for core_import_programs(). Returns the number of bytes actually
 * read. Returns -1 if an error occurred; a return value of 0 signifies end of
 * input.
 */
int4 shell_read(char *buf, int4 buflen);

#if defined(ANDROID) || defined(IPHONE)
/* shell_get_acceleration()
 * shell_get_location()
 * shell_get_heading()
 *
 * These functions were added to support the iPhone's accelerometer, GPS, and
 * compass. Shells on platforms that do not provide this functionality should
 * return 0; a return value of 1 indicates success (though not necessarily
 * accuracy!).
 *
 * The units used here match those on the iPhone; implementations on other
 * platforms should transform their units to match iPhone conventions. This
 * probably won't be an issue in practice, since the iPhone's units match
 * established international standards, the one exception being acceleration,
 * which it expresses in units of Earth gravities rather than the standard
 * m/s^2. TODO: what is the exact conversion factor used by the iPhone?
 * 
 * shell_get_acceleration: x, y, z in g's (see above). Looking at the device
 * in portrait orientation, positive x points to the right, positive y points
 * up, and positive z points toward the user.
 * shell_get_location: lat and lon are in decimal degrees, with N and E
 * positive; lat_lon_acc, elev, and elev_acc are in meters. TODO: what is
 * elev relative to?
 * shell_get_heading: mag_heading and true_heading are in decimal degrees,
 * along the y axis (using the same coordinate system as described above),
 * with 0 being N, 90 being E, etc., acc is in decimal degrees, and x, y, and z
 * are magnetic deviation (again using the same coordinate system as above),
 * in microteslas, but normalized to the range -128..128. TODO: What's this
 * "normalization"? I hope they mean "clipped", because otherwise you wouldn't
 * have a unit, yet they claim the unit is microteslas.
 */
int shell_get_acceleration(double *x, double *y, double *z);
int shell_get_location(double *lat, double *lon, double *lat_lon_acc,
                                double *elev, double *elev_acc);
int shell_get_heading(double *mag_heading, double *true_heading, double *acc,
                                double *x, double *y, double *z);
#endif

/* shell_get_time_date()
 *
 * Get the current time and date. The date should be provided formatted as
 * YYYYMMDD, and the time should be provided formatted as HHMMSSss (24-hour).
 * The weekday is a number from 0 to 6, with 0 being Sunday.
 */
void shell_get_time_date(uint4 *time, uint4 *date, int *weekday);

/* shell_log()
 *
 * Writes text to some place where log output should go, typically Standard
 * Output on systems where this is easy to access, and the system's usual
 * logging facility otherwise.
 * Note: non-debug versions of Free42 should never use this.
 */
void shell_log(const char *message);

#endif
