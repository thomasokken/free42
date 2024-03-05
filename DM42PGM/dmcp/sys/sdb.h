/*

BSD 3-Clause License

Copyright (c) 2018, SwissMicros
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
#ifndef __SYS_SDB_H__
#define __SYS_SDB_H__

// === IFC START ===
// System data block

typedef int get_flag_fn_t();
typedef void set_flag_fn_t(int val);

typedef int run_menu_item_fn_t(uint8_t line_id);
typedef const char * menu_line_str_fn_t(uint8_t line_id, char * s, const int slen);

typedef void void_fn_t();


typedef struct {
  volatile uint32_t calc_state;
  FIL * ppgm_fp;
  const char * key_to_alpha_table;

  run_menu_item_fn_t * run_menu_item_app;
  menu_line_str_fn_t * menu_line_str_app;

  void_fn_t * after_fat_format;

  get_flag_fn_t * is_flag_dmy;
  set_flag_fn_t * set_flag_dmy;
  get_flag_fn_t * is_flag_clk24;
  set_flag_fn_t * set_flag_clk24;
  get_flag_fn_t * is_beep_mute;
  set_flag_fn_t * set_beep_mute;

  disp_stat_t * pds_t20;
  disp_stat_t * pds_t24;
  disp_stat_t * pds_fReg;

} sys_sdb_t;


#define calc_state      (sdb.calc_state)
#define ppgm_fp         (sdb.ppgm_fp)

#define key_to_alpha_table (sdb.key_to_alpha_table)

#define run_menu_item_app  (sdb.run_menu_item_app)
#define menu_line_str_app  (sdb.menu_line_str_app)

#define after_fat_format  (sdb.after_fat_format)

#define is_flag_dmy     (sdb.is_flag_dmy)
#define set_flag_dmy    (sdb.set_flag_dmy)
#define is_flag_clk24   (sdb.is_flag_clk24)
#define set_flag_clk24  (sdb.set_flag_clk24)
#define is_beep_mute    (sdb.is_beep_mute)
#define set_beep_mute   (sdb.set_beep_mute)

// === IFC END ===


extern sys_sdb_t sdb;

#if 0
// === IFC START ===

#define t20             (sdb.pds_t20)
#define t24             (sdb.pds_t24)
#define fReg            (sdb.pds_fReg)

#define sdb (*((sys_sdb_t*)0x10002000))

// === IFC END ===
#endif

#endif
