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

#ifndef CORE_MAIN_H
#define CORE_MAIN_H 1

#include "free42.h"


/**********************************/
/* Shell/Core interface functions */
/**********************************/

/* core_init()
 *
 * This function initializes the emulator core. If the read_state parameter is
 * 1, the core should read saved state using shell_read_saved_state(); if
 * it is 0, or if there is a problem reading the saved state, it should perform
 * a hard reset.
 * If the read_state parameter is 1, the 'version' parameter should contain the
 * state file version number; otherwise its value is not used.
 * This is guaranteed to be the first function called on the emulator core.
 */
void core_init(int read_state, int4 version);

#if defined(IPHONE) || defined(ANDROID)

/* core_enter_background()
 *
 * This function is called when the iPhone app has been placed in background
 * mode. It writes the state, just like core_quit(), but it doesn't perform
 * any cleanup, so the app can resume instantly if it is brought back to
 * the foreground. If the app is killed while in the background, nothing is
 * lost.
 */
void core_enter_background();

#endif

/* core_quit()
 *
 * This function shuts down the emulator core. The core should save its state
 * using shell_write_saved_state().
 * This is guaranteed to be the last function called on the emulator core.
 */
void core_quit();

/* core_repaint_display()
 *
 * This function asks the emulator core to repaint the display. The core will
 * respond by immediately calling shell_blitter() to repaint the entire
 * display.
 * The shell uses this function to re-generate the display after switching
 * skins.
 */
void core_repaint_display();

/* core_menu()
 *
 * The shell uses this function to check if a menu is active. This affects
 * whether or not clicking in the display, to activate menu keys, is
 * enabled.
 */
int core_menu();

/* core_alpha_menu()
 *
 * The shell uses this function to check if the core is in "alpha" mode (i.e.
 * the ALPHA menu or any of its submenus is active). This affects how events
 * from the keyboard (the real PC keyboard, not the on-screen one emulated by
 * Free42) or PalmOS Graffiti device are handled: in alpha mode, printable
 * ASCII characters are sent straight to the core; outside alpha mode, all
 * key events are translated sequences of HP-42S key events according to the
 * keymap file.
 */
int core_alpha_menu();

/* core_hex_menu()
 *
 * The shell uses this function to check if the core is in "hex" mode (i.e.
 * the A..F submenu of the BASE application is active). This affects how events
 * from the keyboard (the real PC keyboard, not the on-screen one emulated by
 * Free42) are handled: in hex mode, 'A' through 'F' and 'a' through 'f' are
 * translated to keycodes 1 through 6, regardless of the keyboard map.
 */
int core_hex_menu();

/* core_keydown()
 *
 * This function informs the emulator core that an HP-42S key was pressed. Keys
 * are identified using a numeric key code, which corresponds to the key
 * numbers returned by the HP-42S 'GETKEY' function: 'Sigma+' is 1, '1/x' is 2,
 * and so on. The shift key is 28 -- 'shift' handling is performed by the
 * emulator core, so it needs to receive raw key codes, unlike the 'GETKEY'
 * function, which handles the shift key itself and then returns key codes in
 * the range 38..74 for shifted keys. The core_keydown() function should only
 * be called with key codes from 1 to 37, inclusive.
 * Keys that cause immediate action should be handled immediately when this
 * function is called, and calls to core_keytimeout1(), core_keytimeout2(),
 * and core_keyup() should be ignored (until the next core_keydown(), that
 * is!). Keys that are handled only when *released* should not be handled
 * immediately when this function is called; the emulator core should store the
 * key code, handle core_keytimeout1() and core_keytimeout2() as appropriate,
 * and not perform any action until core_keyup() is called.
 * RETURNS: a flag indicating whether or not the front end should call this
 * function again as soon as possible. This will be 1 if the calculator is
 * running a user program, and is only returning execution to the shell because
 * it has detected that there is a pending event.
 * The 'enqueued' pointer is a return parameter that the emulator core uses to
 * tell the shell if it has enqueued the keystroke. If this is 1, the shell
 * should not send call timeout1(), timeout2(), or keyup() for this keystroke.
 * NOTE: a key code of 0 (zero) signifies 'no key'; this is needed if the shell
 * is calling this function because it asked to be called (by returning 1 the
 * last time) but no keystrokes are available. It is not necessary to balance
 * the keydown call with a keyup in this case.
 * The 'repeat' pointer is a return parameter that the emulator core uses to
 * ask the shell to auto-repeat the current key. If this is set to 1 or 2, the
 * shell will not call timeout1() and timeout2(), but will repeatedly call
 * core_repeat() until the key is released. (1 requests a slow repeat rate, for
 * SST/BST; 2 requests a fast repeat rate, for number/alpha entry.)
 */
int core_keydown(int key, int *enqueued, int *repeat);

/* core_repeat()
 *
 * This function is called by the shell to signal auto-repeating key events.
 * It is the core's responsibility to keep track of *which* key is repeating.
 * The function can return 0, to request repeating to stop; 1, which requests
 * a slow repeat rate, for SST/BST; or 2, which requests a fast repeat rate,
 * for number/alpha entry.
 */
int core_repeat();

/* core_keytimeout1()
 *
 * This function informs the emulator core that the currently pressed key has
 * been held down for 1/4 of a second. (If the key is released less than 1/4
 * second after being pressed, this function is not called.)
 * For keys that do not execute immediately, this marks the moment when the
 * calculator displays the key's function name.
 */
void core_keytimeout1();

/* core_keytimeout2()
 *
 * This function informs the emulator core that 2 seconds have passed since
 * core_keytimeout1() was called. (If the key is released less than 2 seconds
 * after core_keytimeout1() is called, this function is not called.)
 * This marks the moment when the calculator switches from displaying the key's
 * function name to displaying 'NULL' (informing the user that the key has been
 * annulled and so no operation will be performed when it is released).
 */
void core_keytimeout2();

/* core_timeout3()
 *
 * A wakeup call that the core can request by calling shell_request_timeout3().
 * Used to implement PSE and the brief linger period after MEM, SHOW, and
 * shift-VARMENU. The 'repaint' parameter says whether the callback is invoked
 * because the timeout period expired (1), or because a key event is on its
 * way (0); if a key event is on its way, the emulator core should not repaint
 * the screen, in order to avoid flashing.
 * If the function returns 'true', this means that the timeout was used for
 * PSE, and the shell should resume program execution.
 */
bool core_timeout3(int repaint);

/* core_keyup()
 *
 * This function informs the emulator core that the currently pressed key (that
 * is, the one whose key code was given to it in the most recent call to
 * core_keydown()) has been released.
 * This function is always called when a key is released, regardless of how
 * long it was held down, and regardless of whether or not core_keytimeout1()
 * and core_keytimeout2() were called following the most recent
 * core_keydown().
 * RETURNS: a flag indicating whether or not the front end should call this
 * function again as soon as possible. This will be 1 if the calculator is
 * running a user program, and is only returning execution to the shell because
 * it has detected that there is a pending event.
 */
int core_keyup();

/* core_powercycle()
 *
 * This tells the core to pretend that a power cycle has just taken place.
 * Usually called right after core_init(), but on platforms where a power-down
 * can take place without shutting down applications, it can be called at any
 * time between core_init() and core_quit().
 * The core should respond by performing power-down activities, immediately
 * followed by power-up activities. (The emulator core is never expected to
 * actually emulate the state of *being* off -- the host environment can handle
 * being off just fine by itself, thank you very much -- it's just the side
 * effects of the *transitions* of a power cycle we want emulated (stopping
 * program execution, restarting it if flag 11 is set, clearing flags 11 and
 * 44, and maybe other stuff once I discover it).
 * This function returns a flag telling the shell whether or not the core wants
 * to run again -- this will be true if flag 11 was set, so there's program
 * execution to be done.
 */
int core_powercycle();

/* core_list_programs()
 *
 * This function is called by the shell when the user activates the Export
 * Program command. The core returns a list of program names, corresponding to
 * the programs as they appear in the PGM catalog, but in reverse order (the
 * .END. is last). Each item corresponds to one program, and consists of the
 * names of all the global labels in that program. Programs that contain no
 * global labels are given the name END (or .END.).
 * The indexes into the list of program names should be used to identify
 * programs to core_export_programs().
 * The function returns a dynamically allocated buffer. The first four bytes
 * are the number of programs returned (as a big-endian int) which is
 * guaranteed to always be at least 1. This is followed by a sequence of
 * 'program_count' null-terminated strings.
 * This function will return NULL if it fails to allocate the buffer.
 * The caller should free() the buffer once it is finished using it.
 */
char *core_list_programs();

/* core_program_size()
 * This function returns the size of a program, specified by its index.
 * The indexes correspond to those returned by core_list_programs(). The caller
 * should *only* use those indexes; any indexes outside of that range will
 * cause weirdness and mayhem.
 * The size returned by this function does not include the 3 bytes for the
 * END; this matches what the HP-42S displays on line 00, but the caller should
 * be aware that the actual byte stream produced by core_export_programs() will
 * be 3 bytes longer.
 */
int4 core_program_size(int prgm_index);

/* core_export_programs()
 *
 * This function is called by the shell after the user has selected a nonempty
 * set of programs (from the list returned by core_list_programs()) and
 * confirmed the operation (supplied a file name etc.). 
 * The 'count' parameter indicates how many programs are to be exported; the
 * 'indexes' parameter is an array of program indexes. The core will pass the
 * raw file data to the shell using the shell_write() function.
 */
void core_export_programs(int count, const int *indexes);

/* core_import_programs()
 *
 * This function is called by the shell after the user has selected a file to
 * import. The core will read the file data using the shell_read() function.
 */
void core_import_programs();

/* core_copy()
 *
 * Returns a string representation of the contents of the X register.
 * Used by the shell to implement the Copy command.
 * The caller should free the returned text using free(3).
 */
char *core_copy();

/* core_paste()
 *
 * Puts the given value on the stack, using RCL semantics. It tries to parse
 * the string as a complex or a real number; if that fails, it is pasted as
 * a plain string.
 * Used by the shell to implement the Paste command.
 */
void core_paste(const char *s);

/* core_settings
 *
 * This is a struct that stores user-configurable core settings. The shell
 * should provide the appropriate controls in a "Preferences" dialog box to
 * allow the user to view and change these settings.
 */
typedef struct {
    bool matrix_singularmatrix;
    bool matrix_outofrange;
    bool auto_repeat;
    bool enable_ext_accel;
    bool enable_ext_locat;
    bool enable_ext_heading;
    bool enable_ext_time;
    bool enable_ext_fptest;
} core_settings_struct;

extern core_settings_struct core_settings;


/*******************/
/* Keyboard repeat */
/*******************/

extern int repeating;
extern int repeating_shift;
extern int repeating_key;


/*******************/
/* Other functions */
/*******************/

void set_alpha_entry(bool state);
void set_running(bool state);
bool program_running();

int want_to_run_again();
void do_interactive(int command);
int find_builtin(const char *name, int namelen, bool strict);

void sst();
void bst();

void fix_thousands_separators(char *buf, int *bufptr);
int find_menu_key(int key);
void start_incomplete_command(int cmd_id);
void finish_command_entry(bool refresh);
void finish_xeq();
void start_alpha_prgm_line();
void finish_alpha_prgm_line();
int shiftcharacter(char c);


#endif
