#ifndef __CORE_AUX_H__
#define __CORE_AUX_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*
typedef enum {
  AUX_REG_X = 1,
  AUX_REG_Y,
  AUX_REG_Z,
  AUX_REG_T,
  AUX_REG_LX,
  AUX_REG_A
} reg_id_t;
*/

#define AUX_REG_X   1
#define AUX_REG_Y   2
#define AUX_REG_Z   3
#define AUX_REG_T   4
#define AUX_REG_LX  5
#define AUX_REG_A   6

typedef int reg_id_t;


void set_graphics_mode(int gm);
int graphics_mode();
int gr_MAXX();
int gr_MAXY();


// Convert register to string
int reg2str(char *buf, int buflen, reg_id_t reg_id);


// Avoid Free42 functionality disablig auto-started program
// after ON to turn the calc back to OFF
void clear_no_keystrokes_yet();

// Returns mode pause state
int is_mode_pause();

// Returns mode number entry state
int is_number_entry();

// Returns wide print flag
int is_wide_print();

// Returns clk24 flag
int is_clk24();

// Returns dmy flag
int is_dmy();

// Is program mode
int is_pgm_mode();

// Set clk24 flag
void set_clk24(int val);

// Set DMY flag
void set_dmy(int val);


// Is goose displayed
int is_goose();

// Returns RefLCD mask for LCD update during program RUN
int get_reflcd_mask();
void set_reflcd_mask(int reflcd);

typedef struct {
  int pgm_line;
  int8_t y_row;
} pgm_info_t;

void get_pgm_info(pgm_info_t * pi);

typedef struct {
  int pc;
  int line;
  int8_t is_end;
  char * buf;
  int buflen;
} pgm_line_t;


void pgm_line_init(pgm_line_t * p, char * buf, int buflen);
int get_pgm_line(pgm_line_t * p, int line);


// Returns free42 version string
const char * free42_version_str();

// Get/Set calc flag
int get_calc_flag(int flag_nr);
void set_calc_flag(int flag_nr, int val);

// DynStack
int is_dynstack();
int get_dynstack_size();


// Core keydown hack to support separate menu key line
//int core_keydown_ex(int key, int *enqueued, int *repeat, int no_menu_key);
bool core_keydown_ex(int key, bool *enqueued, int *repeat, int no_menu_key);

// Returns printer delay set by DELAY command
unsigned int core_printer_delay();
void core_set_printer_delay(unsigned int val);

void core_redisplay();

#ifdef __cplusplus
} // extern "C"
#endif

char* core_display_buffer();

#endif
