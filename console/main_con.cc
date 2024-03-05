
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_CURSES
#include "con_curses.h"
#endif

#include "core_main.h"
#include "core_display.h"
#include "core_helpers.h"



static int ann_updown = 0;
static int ann_shift = 0;
static int ann_print = 0;
static int ann_run = 0;
static int ann_battery = 0;
static int ann_g = 0;
static int ann_rad = 0;



int hp2ascii(char *dst, const char *src, int srclen) {
    const char *esc;
    unsigned char c;
    int s, d = 0;
    for (s = 0; s < srclen; s++) {
        c = src[s];
        if (c >= 130 && c != 138)
            c &= 127;
        switch (c) {
            /* NOTE: this code performs the following 12 translations
             * that are not ASCII, but seem to be widely accepted --
             * that is, they looked OK when I tried them in several
             * fonts in Windows and Linux, and in Memo Pad on the Palm:
             *
             *   0: 247 (0367) divide
             *   1: 215 (0327) multiply
             *   8: 191 (0277) upside-down question mark
             *  17: 181 (0265) lowercase mu
             *  18: 163 (0243) sterling
             *  19: 176 (0260) degree
             *  20: 197 (0305) Aring
             *  21: 209 (0321) Ntilde
             *  22: 196 (0304) Aumlaut
             *  25: 198 (0306) AE
             *  28: 214 (0326) Oumlaut
             *  29: 220 (0334) Uumlaut
             *
             * Two additional candidates are these:
             *
             *  26: 133 (0205) ellipsis
             *  31: 149 (0225) bullet
             *
             * I'm not using those last two because support for them is not
             * as good: they exist in Times New Roman and Adobe Courier
             * (tested on Windows and Linux, respectively) and on the Palm,
             * but are missing from Windows Fixedsys (the default Notepad
             * font, so that is a concern!) and X11 lucidatypewriter and
             * fixed.
             * Note that 133 and 149 are both in the 128-159 range, that
             * is, the Ctrl+Meta range, which is unused in many fonts.
             * Eventually, I should probably support several translation
             * modes: raw, pure ASCII (only emit codes 32-126 and 10),
             * non-pure as below, and more aggressive non-pure (using the
             * ellipsis and fatdot codes, and maybe others). Then again,
             * maybe not. :-)
             */
            case  0:   esc = "\367"; break;
            case  1:   esc = "\327"; break;
            case  2:   esc = "\\sqrt"; break;
            case  3:   esc = "\\int"; break;
            case  4:   esc = "\\gray1"; break;
            case  5:   esc = "\\Sigma"; break;
            case  6:   esc = ">"; break;
            case  7:   esc = "\\pi"; break;
            case  8:   esc = "\277"; break;
            case  9:   esc = "<="; break;
            case 11:   esc = ">="; break;
            case 12:   esc = "!="; break;
            case 13:   esc = "\\r"; break;
            case 14:   esc = "v"; break;
            case 15:   esc = "->"; break;
            case 16:   esc = "<-"; break;
            case 17:   esc = "\265"; break;
            case 18:   esc = "\243"; break;
            case 19:   esc = "\260"; break;
            case 20:   esc = "\305"; break;
            case 21:   esc = "\321"; break;
            case 22:   esc = "\304"; break;
            case 23:   esc = "\\angle"; break;
            case 24:   esc = "E"; break;
            case 25:   esc = "\306"; break;
            case 26:   esc = "..."; break;
            case 27:   esc = "\\esc"; break;
            case 28:   esc = "\326"; break;
            case 29:   esc = "\334"; break;
            case 30:   esc = "\\gray2"; break;
            case 31:   esc = "\\bullet"; break;
            case '\\': esc = "\\\\"; break;
            case 127:  esc = "|-"; break;
            case 128:  esc = ":"; break;
            case 129:  esc = "y"; break;
            case 138:  esc = "\\LF"; break;
            default:   dst[d++] = c; continue;
        }
        while (*esc != 0)
            dst[d++] = *esc++;
    }
    return d;
}


/* === Undefined functions ... just stubs === */



double shell_random_seed() {
  return 0.78237947239847;
}


void shell_print(const char *text, int length,
  const char *bits, int bytesperline,
  int x, int y, int width, int height)
{
  printf("Shell print: %s\n", text);
}


void shell_beeper(int frequency, int duration) {
}

int shell_wants_cpu() {
  return 0;
}

void shell_get_time_date(uint4 *tim, uint4 *date, int *weekday) {
  *tim = 12345600;
  *date = 20150825;
  *weekday = 2;
}


uint4 shell_get_mem() {
  return 16*1024; // Returns free mem
}

uint4 shell_milliseconds() {
  return 1;
}

void shell_request_timeout3(int delay) {
}

void shell_delay(int duration) {
  // usleep(duration * 1000);
}

void shell_powerdown() {
}

int4 shell_read_saved_state(void *buf, int4 bufsize) {
  return -1;
}




/* ======================= */
/* === LOCAL FUNCTIONS === */
/* ======================= */

#ifdef USE_CURSES
void curses_init() {
  setlocale(LC_ALL, "");
  initscr();  noecho();  halfdelay(1);
  nonl();
  intrflush(stdscr, FALSE);
  keypad(stdscr, TRUE);
}
void curses_end() {
  echo(); nocbreak();
  endwin();
}
int curses_map_key(int key) {
  int calc_key = -1;
  switch(key) {
    case 'q': calc_key =  1; break;
    case 'w': calc_key =  2; break;
    case 'e': calc_key =  3; break;
    case 'r': calc_key =  4; break;
    case 't': calc_key =  5; break;
    case 'y': calc_key =  6; break;
    case 'a': calc_key =  7; break;
    case 's': calc_key =  8; break;
    case 'd': calc_key =  9; break;
    case 'f': calc_key = 10; break;
    case 'g': calc_key = 11; break;
    case 'h': calc_key = 12; break;
    case KEY_ENTER:
    case 'z': calc_key = 13; break;
    case 'x': calc_key = 13; break;
    case 'c': calc_key = 14; break;
    case 'v': calc_key = 15; break;
    case 'E':
    case 'b': calc_key = 16; break;
    case KEY_BACKSPACE:
    case 'n': calc_key = 17; break;
    case KEY_UP_CURS:
              calc_key = 18; break;
    case KEY_DOWN_CURS:
              calc_key = 23; break;
    case KEY_TAB:
              calc_key = 28; break;
    case 'o': calc_key = 33; break;

    case '0': calc_key = 34; break;
    case '1': calc_key = 29; break;
    case '2': calc_key = 30; break;
    case '3': calc_key = 31; break;
    case '4': calc_key = 24; break;
    case '5': calc_key = 25; break;
    case '6': calc_key = 26; break;
    case '7': calc_key = 19; break;
    case '8': calc_key = 20; break;
    case '9': calc_key = 21; break;

    case '/': calc_key = 22; break;
    case '*': calc_key = 27; break;
    case '-': calc_key = 32; break;
    case '+': calc_key = 37; break;
    case '.': calc_key = 35; break;
    
    case 'p': calc_key = 36; break;
    default:
      break;
  }
  return calc_key;
}
#endif




#define LCDW 131
#define LCDH  16

char disp[LCDH][LCDW+1];


void disp_clear() {
  int i;
  memset(disp, ' ', (LCDW+1)*LCDH);
  for(i=0; i<LCDH; i++)
    disp[i][LCDW]=0;
}


#if USE_CURSES
#define NL "\r\n"
#else
#define NL "\n"
#endif


#ifdef UTF8_DISP

char const* disp_char(int i) {
  switch (i) {
    case  0: return " ";
    case  1: return "\xe2\x96\x80"; // 2580 - top 
    case  2: return "\xe2\x96\x84"; // 2584 - down
    case  3: return "\xe2\x96\x88"; // 2588 - both
    
    case  4: return "\xe2\x95\x90"; // 2550 - ==
    case  5: return "\xe2\x95\x92"; // 2552 - ,=
    case  6: return "\xe2\x95\x95"; // 2555 - =,
    case  7: return "\xe2\x95\x98"; // 2558 - '=
    case  8: return "\xe2\x95\x9b"; // 255b - ='
    case  9: return "\xe2\x94\x82"; // 2502 - |

    case 11: return "\xe2\x96\xbc"; // 25bc - triangle down
    case 12: return "\xe2\x96\xb2"; // 25b2 - triangle up
    default: break;
  }
  return " ";
}
void disp_char_print(int i) {
  printf("%s", disp_char(i));
}

#define P  disp_char_print
#define PC disp_char_print_cnt

#endif

int disp_annun() {
  if (ann_updown) {
#ifdef UTF8_DISP
    P(11); P(12);
#else
    printf("v^");
#endif  
  } else
    printf("  ");

  printf("  ");
  printf("%s", ann_shift?"SHIFT":"     ");
  printf("  ");
  return 11; // number of chars displayed
}

#ifdef UTF8_DISP

void disp_char_print_cnt(int i, int cnt) {
  const char * s = disp_char(i);
  int a;
  for(a=0; a<cnt; a++)
    printf("%s", s);
}


void disp_print() {
  int i,j;
  P(5); PC(4,LCDW+2);  P(6); printf(NL);
  P(9); printf(" "); i=disp_annun(); PC(0,LCDW+1-i); P(9); printf(NL);
  P(9); PC(0,LCDW+2); P(9); printf(NL);
  for(i=0; i<LCDH; i+=2) {
    P(9); printf(" ");
    for(j=0; j<LCDW; j++) {
      int k = 0;
      if (disp[i][j]   != ' ') k+=1;
      if (disp[i+1][j] != ' ') k+=2;
      P(k);
    }
    printf(" "); P(9); printf(NL);
  }
  P(7); PC(4,LCDW+2); P(8); printf(NL);
}

#else

void disp_print() {
  printf("-------------------------" NL);
  disp_annun();
  printf(NL);
  int i;
  for(i=0; i<LCDH; i++)
    printf("%s" NL, disp[i]);
}

#endif



/* --------------------------- */
/* --- LOCAL FUNCTIONS END --- */
/* --------------------------- */




/* ===================== */
/* === Ifc functions === */
/* ===================== */


void skin_repaint_annunciator(int which, bool state) {
}

void shell_blitter(const char *bits, int bytesperline, int x, int y, int width, int height) {
  printf("Shell blitter" NL);

  for (int v = y; v < y + height; v++) {
    for (int h = x; h < x + width; h++) {
      int b = bits[v * bytesperline + (h >> 3)] & (1 << (h & 7));
      disp[v][h] = b ? '#' : ' ';
    }
  }

  disp_print();
}

void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad) {
  printf("Shell annunciators" NL);
    if (updn != -1 && ann_updown != updn) {
        ann_updown = updn;
        skin_repaint_annunciator(1, ann_updown);
    }
    if (shf != -1 && ann_shift != shf) {
        ann_shift = shf;
        skin_repaint_annunciator(2, ann_shift);
    }
    if (prt != -1 && ann_print != prt) {
        ann_print = prt;
        skin_repaint_annunciator(3, ann_print);
    }
    if (run != -1 && ann_run != run) {
        ann_run = run;
        skin_repaint_annunciator(4, ann_run);
    }
    if (g != -1 && ann_g != g) {
        ann_g = g;
        skin_repaint_annunciator(6, ann_g);
    }
    if (rad != -1 && ann_rad != rad) {
        ann_rad = rad;
        skin_repaint_annunciator(7, ann_rad);
    }
  disp_print();
}

int shell_low_battery() {
  int lowbat = 0;
  if (lowbat != ann_battery) {
    ann_battery = lowbat;
    skin_repaint_annunciator(5, ann_battery);
  }
  return lowbat;
}

int decimal_point = 1;

int shell_decimal_point() {
  return decimal_point ? 1 : 0;
}



/* ------------------------- */
/* --- Ifc functions END --- */
/* ------------------------- */


void empty_keydown() {
  bool dummy1;
  int dummy2, keep_running;
  do {
    printf("empty keydown:" NL);
    keep_running = core_keydown(0, &dummy1, &dummy2);
    printf("end of empty keydown: keep_running=%i" NL, keep_running);
  } while (keep_running);
}

static bool enqueued;


#ifdef USE_CURSES



void main_loop_curses() {
  curses_init();
  (void)getch(); // Just make it print :)
  printf("%x %x" NL, KEY_UP_CURS, KEY_DOWN_CURS);

  core_init(0,0, NULL, 0);

  for(;;) {
    int e, key;
    empty_keydown();

    do {
      e = getch();
      if ( e == ERR ) {
        // timeout - do some timout stuf
        //printf("."); fflush(stdout);
      }
    } while ( e == ERR );

    if ( e == KEY_ESC ) break;

    printf("curs_key=%08x" NL,e); fflush(stdout);
    key = curses_map_key(e);
    if ( key > 0 ) {
      // Key pressed
      printf("key press %i" NL,key);
      int repeat, keep_running;
      keep_running = core_keydown(key, &enqueued, &repeat);
      printf("end of keydown: keep_running=%i  enqueued=%i  repeat=%i" NL, keep_running, enqueued, repeat);
      printf("keyup" NL);
      keep_running = core_keyup();
      printf("end of keyup: keep_running=%i" NL, keep_running);
    }
  }

  curses_end();
}

#else


#define LINELEN 256

void main_loop() {
  core_init(0,0, NULL, 0);
  for(;;) {
    char s[LINELEN];

    // Run until end
    empty_keydown();

    printf("> ");
    if ( !fgets(s, LINELEN, stdin) )
      break;

    int key = atoi(s);
    if ( key > 0 && key <= 37 ) {
      // Press key
      printf("key press %i\n",key);
      int repeat, keep_running;
      keep_running = core_keydown(key, &enqueued, &repeat);
      printf("end of keydown: keep_running=%i  enqueued=%i  repeat=%i\n", keep_running, enqueued, repeat);
      printf("keyup\n");
      keep_running = core_keyup();
      printf("end of keyup: keep_running=%i\n", keep_running);
    }
  }
}


#endif


int main(int argc, char *argv[]) {
  printf("Free42 console emulator\n");

#ifdef USE_CURSES
  main_loop_curses();
#else
  main_loop();
#endif  

  return 0;
}
