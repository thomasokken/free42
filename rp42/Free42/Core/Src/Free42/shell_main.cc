#include "core_main.h"
#include "main.h"

#include <algorithm>
#include <string.h>

//#include <iostream>

/*
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
*/

const int frame_size = 132 * 4;
char frame[132 * 4];
bool frame_ready = false;


void shell_get_time_date(unsigned int*, unsigned int*, int*) {

}

void shell_powerdown() {
    systemCallData.command = 0x0020; // 0x0012 = POWER_DOWN
    __asm__("SVC #0");
}


void shell_message(char const*) {

}


void shell_request_timeout3(int delay) {
    systemCallData.command = 0x0041; // 0x0041 = delay until key press
    systemCallData.args = (void*) delay; // delay in milliseconds
    __asm__("SVC #0");
}

bool shell_wants_cpu() {
    return false;
}

void shell_delay(int delay) {
    systemCallData.command = 0x0040; // 0x0040 = delay
    systemCallData.args = (void*) delay; // delay in milliseconds
    __asm__("SVC #0");
}

const char* shell_number_format() {
    return ".,33";
}

int8 shell_random_seed() {
    return 0;
}
//0x20017ee0 0x900485fa

int out_of_bounds_counter = 0;
void shell_blitter(char const* bits, int bytesperline, int x, int y, int width, int height) {
	for (int yi = y; yi < y + height; yi++) {
		for (int xi = x; xi < x + width; xi++) {
			unsigned int index1 = xi+(yi*2)/8*132;
			unsigned int index2 = xi+(yi*2+1)/8*132;

			if (index1 >= frame_size || index2 >= frame_size) {
				out_of_bounds_counter++;
				continue;
			}

			if (index1 == 131 || index2 == 131) continue;

            if (((int) bits[xi / 8 + yi * bytesperline] & (1 << (xi % 8))) == 0) {
                frame[index1] &= 0xff ^ (1 << ((yi*2) % 8));
                frame[index2] &= 0xff ^ (1 << ((yi*2+1) % 8));
            } else {

                frame[index1] |= (1 << ((yi*2+1) % 8));
                frame[index2] |= (1 << (yi*2 % 8));
            }
            //std::cout << (((int) bits[xi / 8 + yi * bytesperline] & (1 << (xi % 8))) == 0 ? ' ' : '#');

			//frame[xi + yi / 8 * 132] |= (bits[xi / 8 + yi * bytesperline] & (1 << (xi % 8)) == 0) ? 0: 1;//(1 << (yi%8));
		}
        //std::cout << std::endl;
	}

	systemCallData.args = frame; // the frame array contains the bytes to draw to the LCD
	systemCallData.command = 0x0012; // 0x0012 = DRAW_LCD
	__asm__("SVC #0");
}

uint4 shell_milliseconds() {
    systemCallData.command = 0x0042;
    __asm("SVC #0");

    return (uint4) systemCallData.result;
}

void shell_beeper(int) {

}

uint8 shell_get_mem() {
    return 1000;
}

void shell_print(char const* content, int length, char const*, int, int, int, int, int) {
	char* copy = new char[length + 1];
	if (copy == nullptr) return;

	memcpy(copy, content, length);
	copy[length] = '\0';

    systemCallData.command = 0x0050;
    systemCallData.args = (void*) copy;

    __asm("SVC #0");

    delete[] copy;
}

const char* PLATFORM = "RP42 0.0.5b";
const char* shell_platform() {
    return PLATFORM;
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


void skin_put_pixels(unsigned char const*) {

}

void skin_finish_image() {

}



void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad) {
	if (shf) frame[131] = 0xff;
	else frame[131] = 0x00;
}
