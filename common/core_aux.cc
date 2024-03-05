/* 

  Aux util functions for library interface

*/

#include <string.h>


#include "core_main.h"
#include "core_helpers.h"
#include "core_display.h"
#include "core_variables.h"
#include "core_aux.h"

#include "shell.h"

/*
int prgmline2buf(char *buf, int len, int4 line, int highlight,
                        int cmd, arg_struct *arg,
                        bool shift_left = false,
                        bool highlight_final_end = true);

int prgmline2buf(char *buf, int len, int4 line, int highlight,
                        int cmd, arg_struct *arg, const char *orig_num,
                        bool shift_left = false,
                        bool highlight_final_end = true);

... and changed again:
*/

int prgmline2buf(char *buf, int len, int4 line, int highlight,
                        int cmd, arg_struct *arg, const char *orig_num,
                        bool shift_left = false,
                        bool highlight_final_end = true,
                        char **xstr = NULL);



/*
 ▄   ▄  ▄   ▄   ▄▄▄    ▄ ▄▄   ▄▄▄  
 ▀▄ ▄▀  ▀▄ ▄▀  ▀   █   █▀  ▀ █   ▀ 
  █▄█    █▄█   ▄▀▀▀█   █      ▀▀▀▄ 
   █      █    ▀▄▄▀█   █     ▀▄▄▄▀ 
*/


typedef struct {
  const char * name;
  vartype * var;
  double val;
  char namelen;
  bool rw;
} vvar_t;


#define VV_DevID  0
#define VV_GrMod  1
#define VV_RefLCD 2
#define VV_ResX   3
#define VV_ResY   4
#define VV_Vbat   5
#define VVAR_CNT  6


static vvar_t vvars[] = {
  {"DevID",  0, 0, 5, 0},
  {"GrMod",  0, 0, 5, 1},
  {"RefLCD", 0, 7, 6, 1},
  {"ResX",   0, 0, 4, 0},
  {"ResY",   0, 0, 4, 0},
  {"Vbat",   0, 0, 4, 0}
};


int find_vvar(const char *name, int namelength) {
  int ix = 0;
  for ( ; ix < VVAR_CNT; ix++) {
    if ( vvars[ix].namelen != namelength ) continue;
    if ( !strncmp(vvars[ix].name, name, namelength) )
      return ix;
  }
  return -1;
}

#include "stdio.h"

void update_vvar(vvar_t *vv, double val) {
  //printf("Update var '%s' from %i to %i\n", vv->name, (int)(1000*vv->val), (int)(1000*val));

  // No change and we have the var
  if ( val == vv->val && vv->var != NULL )
    return;

  free_vartype(vv->var);
  vv->val = val;
  vv->var = new_real(val);
}


vartype *recall_vvar(const char *name, int namelength) {

#ifndef ARM
  return NULL;

#else
  int ix = find_vvar(name, namelength);
  if (ix < 0)
    return NULL;

  vvar_t * vv = vvars+ix;
  double val;
  
  switch(ix) {
    case VV_DevID:
      val = shell_dev_id();
      break;
    case VV_ResX :
      val = gr_MAXX();
      break;
    case VV_ResY :
      val = gr_MAXY();
      break;
    case VV_Vbat :
      val = shell_vbat();
      break;
    //case VV_GrMod:
    default:
      val = vvars[ix].val;
      break;
  }

  update_vvar(vv, val);

  return vv->var;
#endif
}



int rval_to_int(vartype *value) {
  int val = 0;

  if (value->type == TYPE_REAL) {
    phloat x = ((vartype_real *) value)->x;
#ifdef BCD_MATH
    val = to_int4(x);
#else
    val = (int)floor(x+0.5);
#endif
  }
  return val;
}



// Returns: 1=variable consumed, 0=unknown variable
int store_vvar(const char *name, int namelength, vartype *value) {

#ifndef ARM
  return 0;

#else
  int ix = find_vvar(name, namelength);
  if (ix < 0)
    return 0;

  vvar_t * vv = vvars+ix;

  if (vv->rw) {
    double val = 0;

    switch (ix) {
      case VV_GrMod:
        {
          int gm = rval_to_int(value) & 3;
          if (gm == 1) gm = 0;
          val = (double)gm;
        }
        break;

      case VV_RefLCD: {
          int rf = rval_to_int(value);
          if (rf >= 0) {
            val = (double)(rf & 7);
          } else {
            val = vv->val;
            shell_force_lcd_refresh((-rf) & 7);
          }
        }
        break;

      default:
        val = 0;
        break;
    }

    free_vartype(value); // We aren't using passed variable
    update_vvar(vv, val);
  }

  return 1;
#endif
}



/*
 ▄▄▄▄▄▄ ▄▄▄▄▄▄ ▄▄▄▄▄▄
 ▄▄▄▄▄▄ ▄▄▄▄▄▄ ▄▄▄▄▄▄
*/



extern "C" {


static const int graph_maxx[] = {131, 131, 200, 400};
static const int graph_maxy[] = { 16,  16, 120, 240};


int gr_MAXX() {
  int m = graphics_mode();
  return graph_maxx[m];
}

int gr_MAXY() {
  int m = graphics_mode();
  return graph_maxy[m];
}


/*
  0 => standard HP mode 131x16
  1 => for future use (mapped to 0 now)
  2 => 200x120
  3 => 400x240
*/

int graphics_mode() {
#ifdef ARM
  return vvars[VV_GrMod].val;
#else
  return 0;
#endif
}


void set_graphics_mode(int gm) {
#ifdef ARM
  vvars[VV_GrMod].val = gm;
#endif
}




/*
   ▄▄                          ▀      ▄▀▀                  ▄▀▀               
   ██   ▄   ▄  ▄   ▄         ▄▄▄    ▄▄█▄▄   ▄▄▄          ▄▄█▄▄  ▄ ▄▄    ▄▄▄  
  █  █  █   █   █▄█            █      █    █▀  ▀           █    █▀  █  █   ▀ 
  █▄▄█  █   █   ▄█▄            █      █    █               █    █   █   ▀▀▀▄ 
 █    █ ▀▄▄▀█  ▄▀ ▀▄         ▄▄█▄▄    █    ▀█▄▄▀           █    █   █  ▀▄▄▄▀ 

*/


void core_redisplay() {
  redisplay();
}


#define reg_x  stack[sp]
#define reg_y  stack[sp-1]
#define reg_z  stack[sp-2]
#define reg_t  stack[sp-3]
#define reg_lastx lastx


int reg2str(char *buf, int buflen, reg_id_t reg_id) {
  vartype *reg;
  int len;

  if (reg_id < 0) {
    reg = stack[sp + reg_id];
  } else {
    switch(reg_id) {
      default: // tame compiler
      case AUX_REG_X:
        if (get_dynstack_size() == 0) {
          buf[0] = 0;
          return 0;
        }
        reg = reg_x;
        break;
      case AUX_REG_Y:  reg = reg_y; break;
      case AUX_REG_Z:  reg = reg_z; break;
      case AUX_REG_T:  reg = reg_t; break;
      case AUX_REG_LX: reg = reg_lastx; break;
    }
  }

  if ( reg_id == AUX_REG_A ) {
    len = reg_alpha_length;
    if ( len > buflen-1 ) {
      len = buflen-2;
      buf[len] = 26; // ellipsis
    }
    memcpy(buf, reg_alpha, len);
  } else
    len = vartype2string(reg, buf, buflen-1);
  
  buf[len] = 0;
  return len;
}

void clear_no_keystrokes_yet() {
  no_keystrokes_yet = 0;
}


int is_pgm_mode() {
  return flags.f.prgm_mode;
}

int is_number_entry() {
  return mode_number_entry;
}

int is_goose() {
  return program_running() && !flags.f.message;
}

void get_pgm_info(pgm_info_t * pi) {
  pi->pgm_line = pc2line(pc);
  pi->y_row = core_menu() ? 0 : prgm_highlight_row;
}


static arg_struct pgm_arg;


void pgm_line_init(pgm_line_t * p, char * buf, int buflen) {
  memset(p, 0, sizeof(pgm_line_t));
  p->buf = buf;
  p->buflen = buflen;
  p->line = -1;
}


int get_pgm_line(pgm_line_t * p, int line) {
  int cmd = 0;
  int len = 0;
  while (!p->is_end && p->line < line) {
    if (p->line >= 0) {
      //get_next_command(&p->pc, &cmd, &pgm_arg, 0);
      get_next_command(&p->pc, &cmd, &pgm_arg, 0,
        NULL); // New arg const char **num_str, resolve
      p->is_end = cmd == CMD_END;
    }
    p->line++;
    len = prgmline2buf(p->buf, p->buflen, p->line, false, cmd, &pgm_arg,
      NULL, // TODO: New arg - orig_num - resolve
      false, true);
  }
  return len;
}



int is_mode_pause() {
  return mode_pause;
}

extern int no_menu_key_this_time;

bool core_keydown_ex(int key, bool *enqueued, int *repeat, int no_menu_key) {
  no_menu_key_this_time = no_menu_key;
  int ret = core_keydown(key,enqueued,repeat);
  no_menu_key_this_time = 0;
  return ret;
}


int is_clk24() {
  return mode_time_clk24;
}

int is_dmy() {
  return flags.f.dmy;
}

void set_clk24(int val) {
  mode_time_clk24 = val;
}

void set_dmy(int val) {
  flags.f.dmy = val;
}


int is_wide_print() {
  return flags.f.double_wide_print;
}


int get_reflcd_mask() {
  return vvars[VV_RefLCD].val;
}

void set_reflcd_mask(int reflcd) {
#ifdef ARM
  vvars[VV_RefLCD].val = reflcd;
#endif
}


int is_dynstack() {
  return flags.f.big_stack;
}

int get_dynstack_size() {
  return sp+1;
}


extern unsigned int printer_delay;

/* docmd_delay()...
#ifdef BCD_MATH
            int4 d = to_int4(x*1000);
#else
            int4 d = (int4)floor(x*1000+0.5);
#endif
            printer_delay = (unsigned int)d;
            if ( printer_delay > 1950 ) printer_delay = 1950;
*/

unsigned int core_printer_delay() {
  return printer_delay;
}

void core_set_printer_delay(unsigned int val) {
  if ( val > 1950 ) val = 1950;
  printer_delay = val;
}

// Temporary version hack - Version info isn't available in Free42 interface :(
#include "../windows/VERSION.h"

const char * free42_version_str() {
  return FREE42_VERSION_1;
}

int get_calc_flag(int flag_nr) {
  return flags.farray[flag_nr];
}

void set_calc_flag(int flag_nr, int val) {
  flags.farray[flag_nr] = val ? 1 : 0;
}


} // extern "C"


