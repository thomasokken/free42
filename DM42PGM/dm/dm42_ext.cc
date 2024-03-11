#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "core_main.h"
#include "core_aux.h"

extern "C"
{

#include <main.h>
#include <dmcp.h>
#include <ini.h>

#include <dm42_menu.h>
#include <dm42_fns.h>


char *strsep (char **, const char *);

static const char *dm42_keys[] = {
    "SIGMA", "INV", "SQRT", "LOG", "LN", "XEQ",
    "STO", "RCL", "RDN", "SIN", "COS", "TAN",
    "ENTER", "SWAP", "CHS", "E", "BSP", 
    "UP", "7", "8", "9", "DIV", 
    "DOWN", "4", "5", "6", "MUL",
    "SHIFT", "1", "2", "3", "SUB",
    "EXIT", "0", "DOT", "RUN", "ADD",
    "F1", "F2", "F3", "F4", "F5", "F6",
};

#define MAX_KEYS_LEN 64

static const char *keycode2keyname(int keycode) {
    if (keycode < 0 || keycode >= MAX_FNKEY_NR) {
        return NULL;
    }
    return dm42_keys[keycode-1];
}

static int keyname2keycode(const char *keyname) {
    for (int i = 0; i < MAX_FNKEY_NR; i++) {
        if (strcasecmp(dm42_keys[i], keyname) == 0) {
            return i+1;
        }
    }
    return -1;
}

struct dm42_macro {
    char keyname[12];
    char keys[MAX_KEYS_LEN];
    struct dm42_macro *next;
};

struct dm42_keymap {
    char name[16];
    struct dm42_macro *macros;    
    struct dm42_keymap *next;
};

static struct dm42_keymap *keymaps = NULL;
static struct dm42_keymap *current_keymap = NULL;

static void macro_free_keymaps() {
    struct dm42_keymap *km = keymaps;
    while (km != NULL) {
        struct dm42_keymap *next = km->next;
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
        struct dm42_keymap *km = (struct dm42_keymap *)malloc(sizeof(struct dm42_keymap));
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
    strncpy(macro->keyname, name, 11);
    strncpy(macro->keys, value, MAX_KEYS_LEN-1);
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
        return 0;
    }
    uint size = f_size(&file);
    char *buffer = (char *)malloc(size + 1);
    if (buffer == NULL) {
        lcd_puts(t24, "Fail to alloc.");
        lcd_refresh();
        wait_for_key_press();
        f_close(&file);
        return 0;
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
    return MRET_EXIT;
}

void macro_set_keymap(const char *keymap) {
    struct dm42_keymap *km = keymaps;
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
    struct dm42_keymap *km = keymaps;
    while (km != NULL) {
        if (num-- == 0) {
            strncpy(keymap, km->name, size);
            return true;
        }
        km = km->next;
    }
    return false;
}

bool macro_exec(int key, bool shift) {
    if (current_keymap == NULL) {
        return false;
    }
    char keyname[12];
    if (shift) {
        sprintf(keyname, "SHIFT %s", keycode2keyname(key));
    } else {
        strcpy(keyname, keycode2keyname(key));
    }
    bool enqueued;
    int repeat;
    struct dm42_macro *macro = current_keymap->macros;
    while (macro != NULL) {
        if (strcasecmp(macro->keyname, keyname) == 0) {
            char keys[MAX_KEYS_LEN];
            strncpy(keys, macro->keys, sizeof(keys)-1);
            char *p = keys;
            char *key;
            if (shift) core_keydown(KEY_SHIFT, &enqueued, &repeat); // release shift key
            while ((key = strsep(&p, " ")) != NULL) {
                if (strlen(key) > 2 && key[0] == '"') {
                    core_keydown(KEY_ENTER, &enqueued, &repeat);
                    for (char *ch = key+1; *ch != '"'; ch++) {
                        const char text[2] = {*ch, '\0'};
                        core_keydown_command(text, true,  &enqueued, &repeat);
                    }
                    core_keydown(KEY_ENTER, &enqueued, &repeat);
                } else {
                    int keycode = keyname2keycode(key);
                    if (keycode > 0) {
                        core_keydown(keycode, &enqueued, &repeat);
                    } else {
                        core_keydown_command(key, false, &enqueued, &repeat);
                    }
                    if (!enqueued) core_keyup();
                }
            }
            return true;
        }
        macro = macro->next;
    }
    return false;
}


static int print_to_screen_count = 0;

void print_to_screen(const char *text, int length) {
    if (t24->y >= LCD_Y - t24->f->height*2) {
        t24->y = LCD_Y; lcd_prevLn(t24);
        lcd_putsR(t24, "    Press any key to continue");
        lcd_refresh();
        wait_for_key_press();        
        print_to_screen_count = 0;
    }
    if (print_to_screen_count == 0) {
        lcd_clear_buf();
        lcd_writeClr(t24);
        lcd_writeNl(t24);
    }
    lcd_puts(t24, text);
    lcd_refresh();
    print_to_screen_count++;
}

} // extern "C"
