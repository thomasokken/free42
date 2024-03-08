#ifndef DM42_MACROS_H
#define DM42_MACROS_H

#include <stdbool.h>
#include <stdint.h>

// Keymaps files dir
#define KEYMAP_DIR     "/KEYS"
#define KEYMAP_EXT     ".ini"

int keymaps_load_callback(const char *fpath, const char *fname, void *data);

void macro_set_keymap(const char *keymap);
const char *macro_get_keymap();
bool macro_find_keymap(int num, char *keymap, int size);
int macro_get_keys(int keycode, uint8_t keys[], int len);

#endif // DM42_MACROS_H