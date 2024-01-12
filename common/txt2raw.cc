#include <fstream>
#include <sstream>
#include <string>

#include <stdio.h>
#include <string.h>

#include "core_main.h"
#include "core_globals.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <text-file>\nBuild date: %s\n", argv[0], __DATE__);
        return 1;
    }

    core_init(0, 0, NULL, 0);

    std::ifstream in(argv[1]);
    std::stringstream txtbuf;
    txtbuf << in.rdbuf();
    flags.f.prgm_mode = 1;
    core_paste(txtbuf.str().c_str());

    int len = strlen(argv[1]);
    if (len >= 4 && strcasecmp(argv[1] + (len - 4), ".txt") == 0)
        len -= 4;
    std::string outname = std::string(argv[1], len) + ".raw";

    int *indexes = new int[prgms_count];
    for (int i = 0; i < prgms_count; i++)
        indexes[i] = i;
    core_export_programs(prgms_count, indexes, outname.c_str());

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
    return ".";
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
