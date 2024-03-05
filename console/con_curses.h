#ifndef __CON_CURSES_H__
#define __CON_CURSES_H__

#include <locale.h>
#include <curses.h>



// Handle macro collisions
static const int KEY_ENTER_CURS = KEY_ENTER;
static const int KEY_UP_CURS    = KEY_UP;
static const int KEY_DOWN_CURS  = KEY_DOWN;
static const int KEY_EXIT_CURS  = KEY_EXIT;

#undef KEY_ENTER
#undef KEY_UP
#undef KEY_DOWN
#undef KEY_EXIT

#define KEY_TAB 9
#define KEY_ESC 0x1b

#endif