#include "core_main.h"
#include "main.h"

#include <algorithm>

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
    return 0;
}

void shell_beeper(int) {

}

uint8 shell_get_mem() {
    return 1000;
}

void shell_print(char const*, int, char const*, int, int, int, int, int) {

}

const char* shell_platform() {
    return 0;
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


void shell_annuciators(int updn, int shf, int prt, int run, int g, int rad) {

}

void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad) {
	if (shf) frame[131] |= 0xf0;
	else     frame[131] &= 0x0f;
	if (updn) frame[131] |= 0x0f;
	else      frame[131] &= 0xf0;

	if (prt) frame[131+132] |= 0xf0;
	else     frame[131+132] &= 0x0f;
	if (run) frame[131+132] |= 0x0f;
	else     frame[131+132] &= 0xf0;

	if (g) frame[131+132*2] |= 0xf0;
	else     frame[131+132*2] &= 0x0f;
	if (rad) frame[131+132*2] |= 0x0f;
	else     frame[131+132*2] &= 0xf0;

	//frame[131 * 0] = updn ? 0xff : 0x00;
	//frame[132 * 1 + 131] = shf ? 0xff : 0x00;
	//frame[132 * 2 + 131] = prt ? 0xff : 0x00;
	//frame[132 * 3 + 131] = run ? 0xff : 0x00;

    /*std::cout << "updating annuncators" << std::endl;
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
        std::cout << "rad: " << (rad == 1 ? "on" : "off") << " ";  */
}
