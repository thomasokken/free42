#ifndef DM42_MACROS_H
#define DM42_MACROS_H

#include <stdbool.h>
#include <stdint.h>

// Keymaps files dir
#define KEYMAP_DIR     "/KEYS"
#define KEYMAP_EXT     ".keymap"

int keymaps_load_callback(const char *fpath, const char *fname, void *data);
void macro_set_keymap(const char *keymap);
const char *macro_get_keymap();
bool macro_find_keymap(int num, char *keymap, int size);
bool macro_exec(int key, bool shift);

#endif // DM42_MACROS_H