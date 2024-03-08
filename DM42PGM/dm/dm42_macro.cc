#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core_main.h"
#include "core_aux.h"

#include <minIni.h>

extern "C"
{

#include <main.h>
#include <dmcp.h>

#include <dm42_menu.h>
#include <dm42_fns.h>

static char macro_filenames[32] = "";
static char macro_keymap[16] = "";


int keymaps_load_callback(const char *fpath, const char *fname, void *data) {
    lcd_puts(t24, "Loading ...");
    lcd_puts(t24, fname);
    lcd_refresh();
    if (ini_getsection(0, macro_keymap, sizeof(macro_keymap), fname) <= 0) {
        lcd_puts(t24, "Fail to read.");
        lcd_refresh();
        wait_for_key_press();
        return 0; // Continue in the file list
    }
    strncpy(macro_filenames, fname, sizeof(macro_filenames)-1);
    lcd_puts(t24, "Success");
    lcd_refresh();
    sys_delay(1500);
    return 0;
}

void macro_set_keymap(const char *keymap) {
    if (strlen(keymap) > 0 && strlen(keymap) < 16) {
        strncpy(macro_keymap, keymap, 16);
    }
}

const char *macro_get_keymap() {
    return macro_keymap;
}

bool macro_find_keymap(int num, char *keymap, int size) {
    if (strlen(macro_filenames) == 0) {
        return false;
    }
    if (ini_getsection(num, keymap, size, macro_filenames) > 0) {
        return true;
    }
    return false;
}

int macro_get_keys(int keycode, uint8_t keys[], int len) {
    if (strlen(macro_filenames) == 0) {
        return 0;
    }
    char keyname[4];
    char macro[32];
    sprintf(keyname, "%d", keycode);
    if (ini_gets(macro_keymap, keyname, "", macro, sizeof macro, macro_filenames) > 0) {
        char *tok = strtok(macro, " ");
        int i;
        for (i = 0; tok != NULL && i < len; i++) {
            keys[i] = atoi(tok);
            tok = strtok(NULL, " ");
        }
        return i;
    }
    return 0;
}

} // extern "C"
