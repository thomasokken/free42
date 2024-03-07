#ifndef DM42_MACROS_H
#define DM42_MACROS_H

#include <stdbool.h>
#include <stdint.h>

// Keymaps files dir
#define KEYMAP_DIR     "/KEYS"
#define KEYMAP_EXT     ".ini"

int keymaps_load_callback(const char *fpath, const char *fname, void *data);

void macros_set_keymap(const char *keymap);
const char *macros_get_keymap();
bool macros_read_keymap(int num, char *keymap, int size);
int macros_get_keys(int keycode, uint8_t keys[], int len);

#endif // DM42_MACROS_H