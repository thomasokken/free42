#ifndef DM42_MACROS_H
#define DM42_MACROS_H

#include <stdbool.h>
#include <stdint.h>

// Keymaps files dir
#define KEYMAP_DIR     "/KEYS"
#define KEYMAP_EXT     ".keymap"

#define MI_PRINT_TO_SCREEN     70


// keymap functions
int keymaps_load_callback(const char *fpath, const char *fname, void *data);
void macro_set_keymap(const char *keymap);
const char *macro_get_keymap();
bool macro_find_keymap(int num, char *keymap, int size);
bool macro_exec(int key, bool shift);

// print to graphic screen
bool is_print_to_screen();
bool is_print_to_screen_active();
void set_print_to_screen(bool value);
void print_to_screen(const char *text, int length);

#endif // DM42_MACROS_H