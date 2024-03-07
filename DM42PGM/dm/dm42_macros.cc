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

static char macros_filenames[32] = "";
static char macros_keymap[32] = "";

int macros_load_callback(const char *fpath, const char *fname, void *data) {
    FRESULT res;
    FIL *fp = NULL;

    lcd_puts(t24, "Loading ...");
    lcd_puts(t24, fname);
    lcd_refresh();
    res = f_open(fp, fpath, FA_READ);
    if (res != FR_OK) {
        lcd_puts(t24, "Fail to open.");
        lcd_refresh();
        wait_for_key_press();
        return 0; // Continue in the file list
    }
    strncpy(macros_filenames, fname, sizeof(macros_filenames)-1);
    f_close(fp);
    lcd_puts(t24, "Success");
    lcd_refresh();
    sys_delay(1500);
    return 0;
}

void macros_set_keymap(const char *keymap) {
    if (strlen(keymap) > 0 && strlen(keymap) < 32) {
        strncpy(macros_keymap, keymap, 32);
    }
}

const char *macros_get_keymap() {
    return macros_keymap;
}

bool macros_read_keymap(int num, char *keymap, int size) {
    if (strlen(macros_filenames) == 0) {
        return false;
    }
    if (ini_getsection(num, keymap, size, macros_filenames) > 0) {
        return true;
    }
    return false;
}

int macros_get_keys(int keycode, uint8_t keys[], int len) {
    if (strlen(macros_filenames) == 0) {
        return 0;
    }
    char keyname[4];
    char macro[32];
    sprintf(keyname, "%d", keycode);
    if (ini_gets(macros_keymap, keyname, "", macro, sizeof macro, macros_filenames) > 0) {
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
