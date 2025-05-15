/*
 * RP.h
 *
 *  Created on: Apr 28, 2025
 *      Author: Jerem
 */

#ifndef INC_RP_RP_HH_
#define INC_RP_RP_HH_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int RP_FILE;

typedef struct  __attribute__((packed)) {
	void* args;
	uint32_t result;
	uint16_t command;
} SystemCallData;

extern SystemCallData __attribute__((section(".SYS_CALL_DATA"))) systemCallData;


// miscelaneous
void RP_NOP();

// Key controls
char RP_GET_KEY();
char RP_WA_KEY();
void RP_PUSH_KEY(uint8_t key_code);
void RP_RELEASE_KEY();
void RP_CLEAR_KEY_QUEUE();

// Display controls
void RP_DISPLAY_ON();
void RP_DISPLAY_OFF();
void RP_DISPLAY_DRAW(char* buf);
void RP_DRAW_PAGE0(uint8_t* buf);
void RP_DRAW_PAGE1(uint8_t* buf);
void RP_DRAW_PAGE2(uint8_t* buf);
void RP_DRAW_PAGE3(uint8_t* buf);

// power controls
void RP_POWER_OFF();

// error controls
uint32_t RP_GET_ERROR();
uint32_t RP_CLEAR_ERROR(uint32_t flag);

// timing controls
void RP_DELAY(uint32_t millis);
void RP_DELAY_UNTIL(uint32_t millis);
uint32_t RP_MILLIS();

// clipboard controls
uint8_t RP_PASTE(char* buf);
uint8_t RP_PASTE_IMM(char* buf);

// process controls
void RP_EXIT(uint32_t code);

// file controls
uint32_t RP_FOPEN(RP_FILE* handle, const char* filename, unsigned char flags);
uint32_t RP_FCLOSE(RP_FILE handle);
uint32_t RP_FREAD(RP_FILE handle, char* buff, uint32_t len, uint32_t* bytesRead);
uint32_t RP_FWRITE(RP_FILE handle, const char* buff, uint32_t len, uint32_t* bytesWritten);
uint32_t RP_FSEEK(RP_FILE handle, uint32_t offset, uint32_t dir);
uint32_t RP_FLIST(const char* folder, char** file_list);
uint32_t RP_MKDIR(const char* folder);
uint32_t RP_UNLINK(const char* path);
uint32_t RP_RENAME(const char* old_path, const char* new_path);
uint32_t RP_RMDIR(const char* folder);
uint32_t RP_STAT(const char* path, struct stat* stat);
uint32_t RP_FSTAT(RP_FILE handle, struct stat* stat);

#ifdef __cplusplus
}
#endif

#endif /* INC_RP_RP_HH_ */
