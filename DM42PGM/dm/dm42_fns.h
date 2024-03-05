/*

BSD 3-Clause License

Copyright (c) 2015-2021, SwissMicros
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


  The software and related material is released as “NOMAS”  (NOt MAnufacturer Supported). 

  1. Info is released to assist customers using, exploring and extending the product
  2. Do NOT contact the manufacturer with questions, seeking support, etc. regarding
     NOMAS material as no support is implied or committed-to by the Manufacturer
  3. The Manufacturer may reply and/or update materials if and when needed solely
     at their discretion

*/
#ifndef __DM42_MAIN_H__
#define __DM42_MAIN_H__



// Stack display

#define LINE_REG_X    0
#define LINE_REG_Y    1
#define LINE_REG_Z    2
#define LINE_REG_T    3
#define LINE_REG_L    4
#define LINE_REG_A    5

#define STACK_REG_L0  BIT(0)
#define STACK_REG_Y   BIT(1)
#define STACK_REG_Z   BIT(2)
#define STACK_REG_T   BIT(3)
#define STACK_REG_L   BIT(4)
#define STACK_REG_A   BIT(5)


#define STACK_XYZTL    (STACK_REG_Y|STACK_REG_Z|STACK_REG_T|STACK_REG_L)
#define STACK_XYZTA    (STACK_REG_Y|STACK_REG_Z|STACK_REG_T|STACK_REG_A)
#define STACK_XYZT     (STACK_REG_Y|STACK_REG_Z|STACK_REG_T)
#define STACK_XYL      (STACK_REG_Y|STACK_REG_L)
#define STACK_XYA      (STACK_REG_Y|STACK_REG_A)
#define STACK_XY       (STACK_REG_Y)
#define STACK_LXYZT    (STACK_REG_L0|STACK_REG_Y|STACK_REG_Z|STACK_REG_T)


// LCD update constants
#define LCD_UPD_MAIN    1
#define LCD_UPD_GOOSE   2
#define LCD_UPD_ANN     4
#define LCD_UPD_ALL     7
#define LCD_UPD_DFLT   -1


// Program load/save
#define PGM_DIR      "/PROGRAMS"
#define PGM_EXT      ".raw"
#define MAX_PGM_SEL    64

extern FRESULT pgm_res;


// State files dir
#define STATE_DIR      "/STATE"
#define STATE_EXT_MASK ".?42"
#define STATE_EXT      ".f42"


// Print to file dir
#define PRINT_DIR      "/PRINTS"
#define PRINT_GR_EXT   ".bmp"
#define PRINT_TXT_EXT  ".txt"


// Graphics backup during off
//#define GR_OFF_IMG    SCR_DIR "/GROFFIMG.BMP"



// Top-Bar aka header 
#define DISP_STATFN        1
#define DISP_DOW           2
#define DISP_DATE          3
#define DISP_DATE_SEP      4
#define DISP_SHORTMON      5
#define DISP_TIME          6
#define DISP_VOLTAGE       7

//void toggle_disp_voltage();
//int is_disp_voltage();

void toggle_disp(int what);
int is_disp(int what);

char get_disp_date_sep();

int get_reg_font_offset(int line_reg_nr);
int inc_reg_font_offset(int line_reg_nr);
int set_reg_font_offset(int line_reg_nr, int offs);


#define PRTOF_TEXT       0
#define PRTOF_GRAPHICS   1
#define PRTOF_BUF_COUNT  2

#define PRTOF_GR_IN_TXT  6
#define PRTOF_NOIR       7
#define PRINT_DBLNL      8



int is_print_to_file(int what);
void set_print_to_file(int what, int val, int gui);

#define PRTOF_LIMIT_FLUSH  0
#define PRTOF_FULL_FLUSH   1
void prtof_buf_flush(int what, int full);

// --


#define ALPHA_NO_TRANSL  '_'
extern const char alpha_upper_transl[];


// State functions

void set_stack_layout(int layout);
int get_stack_layout();
char * get_stack_layout_str(char *s, int layout);


int get_dynstackext();
void set_dynstackext(int val);


void copy_reset_state_filename(char *s, int maxlen);

// res >= 0 -> OK
int savestat_check_read(const char * state_file_name);

#define LCD_HEADER_LINES 24
#define LCD_ANN_LINES    16

// Force to redraw calc LCD
void calc_lcd_redraw();


#define AUX_BUF_PGM_LIST_SIZE (3*512)
#define AUX_BUF_SELS_SIZE       (512)


#ifndef max
#define max(x,y) ({ \
  __typeof__ (x) _x = (x); \
  __typeof__ (y) _y = (y); \
  _x > _y ? _x : _y; })
#endif

#ifndef min
#define min(x,y) ({ \
  __typeof__ (x) _x = (x); \
  __typeof__ (y) _y = (y); \
  _x < _y ? _x : _y; })
#endif

#endif
