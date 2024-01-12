#include <fstream>
#include <sstream>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "core_main.h"
#include "core_globals.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <raw-file>\nBuild date: %s\n", argv[0], __DATE__);
        return 1;
    }

    core_init(0, 0, NULL, 0);

    core_import_programs(0, argv[1]);

    int len = strlen(argv[1]);
    if (len >= 4 && strcasecmp(argv[1] + (len - 4), ".raw") == 0)
        len -= 4;
    std::string outname = std::string(argv[1], len) + ".txt";
    FILE *out = fopen(outname.c_str(), "wb");

    flags.f.prgm_mode = 1;
    for (int i = 0; i < prgms_count; i++) {
        current_prgm = i;
        char *txt = core_copy();
        if (i > 0)
            fputs("\r\n", out);
        fputs(txt, out);
        free(txt);
    }
    fclose(out);

    return 0;
}

const char *shell_platform() {
    return NULL;
}

void shell_blitter(const char *bits, int bytesperline, int x, int y,
                             int width, int height) {
    //
}

void shell_beeper(int tone) {
    //
}

void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad) {
    //
}

bool shell_wants_cpu() {
    return false;
}

void shell_delay(int duration) {
    //
}

void shell_request_timeout3(int delay) {
    //
}

uint8 shell_get_mem() {
    return 0;
}

bool shell_low_battery() {
    return false;
}

void shell_powerdown() {
    //
}

int8 shell_random_seed() {
    return 0;
}

uint4 shell_milliseconds() {
    return 0;
}

const char *shell_number_format() {
    return localeconv()->decimal_point;
}

int shell_date_format() {
    return 0;
}

bool shell_clk24() {
    return false;
}

void shell_print(const char *text, int length,
                 const char *bits, int bytesperline,
                 int x, int y, int width, int height) {
    //
}

void shell_get_time_date(uint4 *time, uint4 *date, int *weekday) {
    *time = 0;
    *date = 15821015;
    *weekday = 5;
}

void shell_message(const char *message) {
    //
}

void shell_log(const char *message) {
    //
}
