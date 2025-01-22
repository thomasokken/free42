#include "shell_skin.h"
#include "core_main.h"

#include <iostream>

int main(int argc, char* argv[]) {
    core_init(0, 1, "", 0);

    while (true) {
        int input;
        std::cin >> input;
        bool* enqued = new bool;
        int* repeat = new int;
        core_keydown(input, enqued, repeat);
    }

    return 0;
}

void shell_get_time_date(unsigned int*, unsigned int*, int*) {

}

void shell_powerdown() {

}


void shell_message(char const*) {

}


void shell_request_timeout3(int) {

}

bool shell_wants_cpu() {
    return false;
}

void shell_delay(int) {

}

const char* shell_number_format() {
    return ",011";
}

int8 shell_random_seed() {
    return 0;
}

void shell_blitter(char const* bits, int bytesperline, int x, int y, int width, int height) {
    std::cout << "redrawing: (" << bytesperline << ", " << x << ", " << y << ", " << width << ", " << height << ")" << std::endl;

    for (int yi = y; yi < y + height; yi++) {
        for (int xi = x; xi < x + width; xi++) {
            std::cout << (((int) bits[xi / 8 + yi * bytesperline] & (1 << (xi % 8))) == 0 ? ' ' : '#');
        }

        std::cout << std::endl;
    }
}

uint4 shell_milliseconds() {
    return 0;
}

void shell_beeper(int) {

}

uint8 shell_get_mem() {
    return 0;
}

void shell_print(char const*, int, char const*, int, int, int, int, int) {

}

const char* shell_platform() {
    return nullptr;
}

int shell_date_format() {
    return 0;
}

bool shell_clk24() {
    return false;
}

bool shell_low_battery() {
    return false;
}

int skin_getchar() {
    return 0;
}

int skin_init_image(int, int, SkinColor const*, int, int) {
    return 0;
}

void skin_put_pixels(unsigned char const*) {

}

void skin_finish_image() {

}


void shell_annuciators(int updn, int shf, int prt, int run, int g, int rad) {

}

void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad) {
    std::cout << "updating annuncators" << std::endl;
    if (updn != -1)
        std::cout << "updn: " << (updn == 1 ? "on" : "off") << " ";

    if (shf != -1)
        std::cout << "shf: " << (shf == 1 ? "on" : "off") << " ";

    if (prt != -1)
        std::cout << "prt: " << (prt == 1 ? "on" : "off") << " ";

    if (run != -1)
        std::cout << "run: " << (run == 1 ? "on" : "off") << " ";    

    if (g != -1)
        std::cout << "g: " << (g == 1 ? "on" : "off") << " ";  

    if (rad != -1)
        std::cout << "rad: " << (rad == 1 ? "on" : "off") << " ";  
}