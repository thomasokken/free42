#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core_main.h"
#include "core_aux.h"

extern "C"
{

#include <main.h>
#include <dmcp.h>
#include <ini.h>

#include <dm42_menu.h>
#include <dm42_fns.h>

struct dm42_macro {
    int keycode;
    uint8_t keys[10];
    uint len;
    struct dm42_macro *next;
};

struct keymap {
    char name[16];
    struct dm42_macro *macros;    
    struct keymap *next;
};

static struct keymap *keymaps = NULL;
static struct keymap *current_keymap = NULL;

static void macro_free_keymaps() {
    struct keymap *km = keymaps;
    while (km != NULL) {
        struct keymap *next = km->next;
        struct dm42_macro *macro = km->macros;
        while (macro != NULL) {
            struct dm42_macro *next = macro->next;
            free(macro);
            macro = next;
        }
        free(km);
        km = next;
    }
    keymaps = NULL;
    current_keymap = NULL;
}

static int keymaps_load_ini_handler(void* user, const char* section, const char* name,const char* value) {
    if (keymaps == NULL || strcmp(current_keymap->name, section) != 0) {
        struct keymap *km = (struct keymap *)malloc(sizeof(struct keymap));
        if (km == NULL) {
            return 0;
        }
        strncpy(km->name, section, sizeof(km->name)-1);
        km->next = NULL;
        km->macros = NULL;
        if (keymaps == NULL) {
            keymaps = km;
        } else {
            current_keymap->next = km;
        }
        current_keymap = km;
    }
    // split value into keys
    struct dm42_macro *macro = (struct dm42_macro *)malloc(sizeof(struct dm42_macro));
    if (macro == NULL) {
        return 0;
    }
    macro->keycode = atoi(name);
    macro->len = 0;
    macro->next = NULL;
    const char *p = value;
    while (*p != '\0') {
        if (macro->len >= sizeof(macro->keys)) {
            free(macro);
            return 0;
        }
        macro->keys[macro->len++] = atoi(p);
        while (*p != '\0' && *p != ' ') {
            p++;
        }
        if (*p == ' ') {
            p++;
        }
    }
    if (current_keymap->macros == NULL) {
        current_keymap->macros = macro;
    } else {
        struct dm42_macro *m = current_keymap->macros;
        while (m->next != NULL) {
            m = m->next;
        }
        m->next = macro;
    }

    return 1;
}

int keymaps_load_callback(const char *fpath, const char *fname, void *data) {
    lcd_puts(t24, "Loading ...");
    lcd_puts(t24, fname);
    lcd_refresh();
    macro_free_keymaps();
    FIL file;
    if (f_open(&file, fpath, FA_READ) != FR_OK) {
        lcd_puts(t24, "Fail to open.");
        lcd_refresh();
        wait_for_key_press();
        return 0; // Continue in the file list
    }
    uint size = f_size(&file);
    char *buffer = (char *)malloc(size + 1);
    if (buffer == NULL) {
        lcd_puts(t24, "Fail to alloc.");
        lcd_refresh();
        wait_for_key_press();
        f_close(&file);
        return 0; // Continue in the file list
    }
    uint read;
    f_read(&file, buffer, size, &read);
    f_close(&file);
    buffer[read] = '\0';
    if (ini_parse_string(buffer, keymaps_load_ini_handler, NULL) < 0) {
        lcd_puts(t24, "Fail to parse.");
        lcd_refresh();
        wait_for_key_press();
    } else {
        lcd_puts(t24, "Success");
        lcd_refresh();
        sys_delay(1500);
    }
    free(buffer);
    return 0;
}

void macro_set_keymap(const char *keymap) {
    if (keymap == NULL) {
        return;
    }
    struct keymap *km = keymaps;
    while (km != NULL) {
        if (strcmp(km->name, keymap) == 0) {
            current_keymap = km;
            return;
        }
        km = km->next;
    }
}

const char *macro_get_keymap() {
    if (current_keymap == NULL) {
        return NULL;
    }
    return current_keymap->name;
}

bool macro_find_keymap(int num, char *keymap, int size) {
    struct keymap *km = keymaps;
    while (km != NULL) {
        if (num-- == 0) {
            strncpy(keymap, km->name, size);
            return true;
        }
        km = km->next;
    }
    return false;
}

int macro_get_keys(int keycode, uint8_t keys[], uint len) {
    if (current_keymap == NULL) {
        return 0;
    }
    struct dm42_macro *macro = current_keymap->macros;
    while (macro != NULL) {
        if (macro->keycode == keycode) {
            if (macro->len > len) {
                return 0;
            }
            memcpy(keys, macro->keys, macro->len);
            return macro->len;
        }
        macro = macro->next;
    }
    return 0;
}

} // extern "C"
