/*
 * RP.c
 *
 *  Created on: Apr 28, 2025
 *      Author: Jerem
 */
#include <rp/RP.hh>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


SystemCallData __attribute__((section(".SYS_CALL_DATA"))) systemCallData;

#define NOP             0x0000
#define GET_KEY         0x0001
#define WA_KEY          0x0002
#define PUSH_KEY        0x0003
#define RELEASE_KEY     0x0004
#define CLEAR_KEY_QUEUE 0x0005
#define LCD_ON          0x0010
#define LCD_OFF         0x0011
#define DRAW_LCD        0x0012
#define DRAW_PAGE0      0x0018
#define DRAW_PAGE1      0x0019
#define DRAW_PAGE2      0x001A
#define DRAW_PAGE3      0x001B
#define CLEAR_LCD       0x0013
#define POWER_DOWN      0x0020
#define GET_ERROR       0x0030
#define CLEAR_ERROR     0x0031
#define DELAY           0x0040
#define DELAY_UNTIL     0x0041
#define MILLIS          0x0042
#define PASTE           0x0050

#define FOPEN           0x0100
#define FCLOSE          0x0101
#define FREAD           0x0110
#define FWRITE          0x0111
#define FSEEK           0x0112
#define FSTAT           0x0113
#define STAT            0x0114
#define FLIST           0x0120
#define FUNLINK         0x0121
#define FRENAME         0x0122
#define MKDIR           0x0123
#define RMDIR           0x0124

#define PRINT_SUCCESS 0
#define PRINT_FAIL 1
#define PRINT_NOT_AVAILABLE 2

#define RP_FILE_ERROR -1

#ifdef __cplusplus
extern "C" {
#endif

// miscelaneous
void RP_NOP() {
	systemCallData.command = NOP;
	__asm volatile("SVC #0");
}

// Key controls
char RP_GET_KEY() {
	systemCallData.command = GET_KEY;
	__asm volatile("SVC #0");

	return (char) systemCallData.result;
}

char RP_WA_KEY() {
	systemCallData.command = WA_KEY;
	__asm volatile("SVC #0");

	return (char) systemCallData.result;
}

void RP_PUSH_KEY(uint8_t key_code) {
	systemCallData.command = PUSH_KEY;
	systemCallData.args = (void*) (uint32_t) key_code;

	__asm volatile("SVC #0");
}

void RP_RELEASE_KEY() {
	systemCallData.command = RELEASE_KEY;

	__asm volatile("SVC #0");
}

void RP_CLEAR_KEY_QUEUE() {
	systemCallData.command = CLEAR_KEY_QUEUE;

	__asm volatile("SVC #0");
}

// Display controls
void RP_DISPLAY_ON() {
	systemCallData.command = LCD_ON;

	__asm volatile("SVC #0");
}
void RP_DISPLAY_OFF() {
	systemCallData.command = LCD_OFF;

	__asm volatile("SVC #0");
}
void RP_DISPLAY_DRAW(char* buf) {
	systemCallData.command = DRAW_LCD;
	systemCallData.args = buf;

	__asm volatile("SVC #0");
}
void RP_DRAW_PAGE0(uint8_t* buf) {
	systemCallData.command = DRAW_PAGE0;
	systemCallData.args = buf;

	__asm volatile("SVC #0");
}
void RP_DRAW_PAGE1(uint8_t* buf) {
	systemCallData.command = DRAW_PAGE1;
	systemCallData.args = buf;

	__asm volatile("SVC #0");
}
void RP_DRAW_PAGE2(uint8_t* buf) {
	systemCallData.command = DRAW_PAGE2;
	systemCallData.args = buf;

	__asm volatile("SVC #0");
}
void RP_DRAW_PAGE3(uint8_t* buf) {
	systemCallData.command = DRAW_PAGE3;
	systemCallData.args = buf;

	__asm volatile("SVC #0");
}

// power controls
void RP_POWER_OFF() {
	systemCallData.command = POWER_DOWN;

	__asm volatile("SVC #0");
}

// error controls
uint32_t RP_GET_ERROR() {
	systemCallData.command = GET_ERROR;

	__asm volatile("SVC #0");

	return systemCallData.result;
}
uint32_t RP_CLEAR_ERROR(uint32_t flag) {
	systemCallData.command = CLEAR_ERROR;
	systemCallData.args = (void*) flag;

	__asm volatile("SVC #0");

	return systemCallData.result;
}

// timing controls
void RP_DELAY(uint32_t millis) {
	systemCallData.command = DELAY;
	systemCallData.args = (void*) millis;

	__asm volatile("SVC #0");
}
void RP_DELAY_UNTIL(uint32_t millis) {
	systemCallData.command = DELAY_UNTIL;
	systemCallData.args = (void*) millis;

	__asm volatile("SVC #0");
}
uint32_t RP_MILLIS() {
	systemCallData.command = MILLIS;

	__asm volatile("SVC #0");

	return systemCallData.result;
}

// clipboard controls
uint8_t RP_PASTE(char* buf) {
	return PRINT_NOT_AVAILABLE;
}
uint8_t RP_PASTE_IMM(char* buf) {
	systemCallData.command = PASTE;
	systemCallData.args = buf;

	__asm volatile("SVC #0");

	return PRINT_SUCCESS;
}

//process controls
void RP_EXIT(uint32_t code) {

}

// file controls
uint32_t RP_FOPEN(RP_FILE* handle, const char* filename, unsigned char flags) {
	systemCallData.command = FOPEN;

	struct {
		const char* filename;
		unsigned int flags;
		uint32_t handle;
	} args;

	args.filename = filename;
	args.flags = flags;
	systemCallData.args = &args;
	__asm volatile("SVC #0");

	*handle = args.handle;

	return systemCallData.result;
}

uint32_t RP_FCLOSE(RP_FILE handle) {
	systemCallData.command = FCLOSE;
	systemCallData.args = (void*) handle;

	__asm volatile ("SVC #0");

	return systemCallData.result;
}

uint32_t RP_FREAD(RP_FILE handle, char* buff, uint32_t len, uint32_t* bytesRead) {
	struct {
		uint32_t handle;
		uint32_t bytesRead;
		char* buf;
		uint32_t len;
	} buffer;

	buffer.handle = handle;
	buffer.buf = buff;
	buffer.len = len;

	systemCallData.command = FREAD;
	systemCallData.args = &buffer;
	__asm volatile("SVC #0");

	*bytesRead = buffer.bytesRead;

	return systemCallData.result;
}

uint32_t RP_FWRITE(RP_FILE handle, const char* buff, uint32_t len, uint32_t* bytesWritten) {
	struct {
		uint32_t handle;
		uint32_t bytesWritten;
		const char* buf;
		uint32_t len;
	} buffer;

	buffer.handle = handle;
	buffer.buf = buff;
	buffer.len = len;

	systemCallData.command = FWRITE;
	systemCallData.args = &buffer;
	__asm volatile("SVC #0");

	*bytesWritten = buffer.bytesWritten;

	return systemCallData.result;
}
uint32_t RP_FSEEK(RP_FILE handle, uint32_t offset, uint32_t dir) {
	struct {
		uint32_t handle;
		uint32_t offset;
		uint32_t dir;
	} args;

	args.handle = handle;
	args.offset = offset;
	args.dir = dir;

	systemCallData.command = FSEEK;
	systemCallData.args = &args;

	__asm volatile("SVC #0");

	return systemCallData.result;
}

uint32_t RP_STAT(const char* path, struct stat* stat) {
	struct {
		const char* path;
		struct stat* stat;
	} args;

	args.stat = stat;
	args.path = path;

	systemCallData.command = STAT;
	systemCallData.args = &args;

	__asm volatile("SVC #0");

	return systemCallData.result;
}

uint32_t RP_FSTAT(RP_FILE handle, struct stat* stat) {
	struct {
		int handle;
		struct stat* stat;
	} args;

	args.stat = stat;
	args.handle = handle;

	systemCallData.command = FSTAT;
	systemCallData.args = &args;

	__asm volatile("SVC #0");

	return systemCallData.result;
}

uint32_t RP_FLIST(const char* folder, char** file_list) {

}
uint32_t RP_MKDIR(const char* folder) {
	systemCallData.args = (void*) folder;
	systemCallData.command = MKDIR;

	__asm volatile("SVC #0");

	return systemCallData.result;
}
uint32_t RP_UNLINK(const char* path) {
	systemCallData.args = (void*) path;
	systemCallData.command = FUNLINK;

	__asm volatile("SVC #0");

	return systemCallData.result;
}
uint32_t RP_RENAME(const char* old_path, const char* new_path) {
	struct {
		const char* old_path;
		const char* new_path;
	} args;
	args.old_path = old_path;
	args.new_path = new_path;
	systemCallData.args = &args;
	systemCallData.command = FRENAME;
	__asm volatile("SVC #0");

	return systemCallData.result;
}
uint32_t RP_RMDIR(const char* folder) {
	systemCallData.args = (void*) folder;
	systemCallData.command = RMDIR;
	__asm volatile("SVC #0");

	return systemCallData.result;
}


#ifdef __cplusplus
}
#endif
