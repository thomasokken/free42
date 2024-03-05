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
#ifndef __DM42_MENU_H__
#define __DM42_MENU_H__


extern const smenu_t         MID_MENU; // Main setup menu
extern const smenu_t         MID_FILE; // File menu
extern const smenu_t        MID_PRINT; // Print to file menu
extern const smenu_t MID_STACK_CONFIG; // Stack config menu
extern const smenu_t    MID_STATEFILE; // State file load/save
extern const smenu_t       MID_TOPBAR; // Top bar config
extern const smenu_t   MID_STACK_AREA; // Stack area config


// --------------------------------
//  Menu items
//    app range   0-127
//    sys range 128-255
// --------------------------------

// DM42 app MI's

#define MI_LOAD_PGM        28
#define MI_SAVE_PGM        29
#define MI_SAVE_STAT       30
#define MI_FILE            31
#define MI_CLEAN_RESET     32

#define MI_STACK_CONFIG    33
#define MI_STACK_XYZTL     34
#define MI_STACK_XYZTA     35
#define MI_STACK_XYZT      36
#define MI_STACK_XYL       37
#define MI_STACK_XYA       38
#define MI_STACK_XY        39
#define MI_STACK_LXYZT     40

#define MI_STATEFILE       41
#define MI_LOAD_STATE      42
#define MI_SAVE_STATE      43


#define MI_DISP_STATFN     44
#define MI_DISP_DOW        45
#define MI_DISP_DATE       46
#define MI_DISP_DATE_SEP   47
#define MI_DISP_SHORTMON   48
#define MI_DISP_TIME       49
#define MI_DISP_VOLTAGE    50

#define MI_SETTINGS        51
#define MI_ABOUT_PGM       52

#define MI_STACK_AREA      53
#define MI_SA_REG_X        54
#define MI_SA_REG_Y        55
#define MI_SA_REG_Z        56
#define MI_SA_REG_T        57
#define MI_SA_REG_L        58
#define MI_SA_REG_A        59

#define MI_TOPBAR_MENU     60

#define MI_PRINT_MENU      61
#define MI_PRTOF_GRAPH     62
#define MI_PRTOF_TEXT      63
#define MI_PRTOF_GR_IN_TXT 64
#define MI_PRTOF_NOIR      65
#define MI_PRINT_DBLNL     66

#define MI_DYNSTACKEXT     67

// --------------------------------

#define MRET_SAVESTATE   777

// --------------------------------


#define DISP_LOADING_STATE     100
#define DISP_SAVING_STATE      101
#define DISP_SAVE_FAILED       102
#define DISP_ALLOC_FAIL        103
#define DISP_NO_PGM_SEL        104
#define DISP_PRBUF_ALLOC_FAIL  105
#define DISP_NEW_HELP          106

int lcd_for_dm42(int what);

// Menu functions
const char * menu_line_str(uint8_t line_id, char * s, const int slen);
int run_menu_item(uint8_t line_id);

// After FAT format callback
void after_fat_format_dm42();

// Start help
void start_help();

// Filesystem function wrapper
void init_sf_buf();
void deinit_sf_buf();


#endif
