/*****************************************************************************
 * rom2raw -- extracts user code from HP-41 ROM images
 * Copyright (C) 2004-2020  Thomas Okken
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see http://www.gnu.org/licenses/.
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

/* The following is a list of all XROM numbers used by the HP-42S. Some of
 * these match functions that may occur in HP-41 code. When such XROM numbers
 * are found, rom2raw will *not* warn about them; all others are flagged as
 * being potentially problematic XROM calls.
 * NOTE: XROM 01,33 - 01,38 are hyperbolics and their inverses. They match the
 * corresponding XROM numbers in the Math module, but in that module, they are
 * implemented in user code. When rom2raw is used to convert the Math module,
 * these XROMs will be converted to XEQ commands, which is probably just as
 * well; if they are encountered in any other ROM, they will be left alone,
 * which means the HP-42S/Free42 machine code implementations will execute.
 * NOTE: XROM 29,27 - 29,32 are functions specific to the unidirectional
 * infrared printer interface. The IR Printer module also has these functions,
 * but the XROM numbers are different (see table below). I could handle this by
 * remapping those XROMs, but then again, why bother: I doubt that any ROM
 * exists that actually calls these functions. This *is* something to consider
 * when writing a generic HP-41-to-42S user code translator, though.
 *         +------------+------------+
 *         | IR Printer |   HP-42S   |
 * +-------+------------+------------+
 * | MAN   | XROM 29,28 | XROM 29,27 |
 * | NORM  | XROM 29,31 | XROM 29,28 |
 * | TRACE | XROM 29,38 | XROM 29,29 |
 * | PRON  | XROM 29,33 | XROM 29,30 |
 * | PROFF | XROM 29,32 | XROM 29,31 |
 * | DELAY | XROM 29,27 | XROM 29,32 |
 * +-------+------------+------------+
 */

typedef struct {
    int number;
    int allow;
    char *name;
} xrom_spec;

xrom_spec hp42s_xroms[] = {
    /* XROM 01,33 */ 0x061, 1, "SINH",      /* Math (see notes, above) */
    /* XROM 01,34 */ 0x062, 1, "COSH",      /* Math (see notes, above) */
    /* XROM 01,35 */ 0x063, 1, "TANH",      /* Math (see notes, above) */
    /* XROM 01,36 */ 0x064, 1, "ASINH",     /* Math (see notes, above) */
    /* XROM 01,37 */ 0x065, 1, "ATANH",     /* Math (see notes, above) */
    /* XROM 01,38 */ 0x066, 1, "ACOSH",     /* Math (see notes, above) */
    /* XROM 01,47 */ 0x06F, 0, "COMB",
    /* XROM 01,48 */ 0x070, 0, "PERM",
    /* XROM 01,49 */ 0x071, 0, "RAN",
    /* XROM 01,50 */ 0x072, 0, "COMPLEX",
    /* XROM 01,51 */ 0x073, 0, "SEED",
    /* XROM 01,52 */ 0x074, 0, "GAMMA",
    /* XROM 02,31 */ 0x09F, 0, "BEST",
    /* XROM 02,32 */ 0x0A0, 0, "EXPF",
    /* XROM 02,33 */ 0x0A1, 0, "LINF",
    /* XROM 02,34 */ 0x0A2, 0, "LOGF",
    /* XROM 02,35 */ 0x0A3, 0, "PWRF",
    /* XROM 02,36 */ 0x0A4, 0, "SLOPE",
    /* XROM 02,37 */ 0x0A5, 0, "SUM",
    /* XROM 02,38 */ 0x0A6, 0, "YINT",
    /* XROM 02,39 */ 0x0A7, 0, "CORR",
    /* XROM 02,40 */ 0x0A8, 0, "FCSTX",
    /* XROM 02,41 */ 0x0A9, 0, "FCSTY",
    /* XROM 02,42 */ 0x0AA, 0, "INSR",
    /* XROM 02,43 */ 0x0AB, 0, "DELR",
    /* XROM 02,44 */ 0x0AC, 0, "WMEAN",
    /* XROM 02,45 */ 0x0AD, 0, "LIN\316\243",
    /* XROM 02,46 */ 0x0AE, 0, "ALL\316\243",
    /* XROM 03,34 */ 0x0E2, 0, "HEXM",
    /* XROM 03,35 */ 0x0E3, 0, "DECM",
    /* XROM 03,36 */ 0x0E4, 0, "OCTM",
    /* XROM 03,37 */ 0x0E5, 0, "BINM",
    /* XROM 03,38 */ 0x0E6, 0, "BASE+",
    /* XROM 03,39 */ 0x0E7, 0, "BASE-",
    /* XROM 03,40 */ 0x0E8, 0, "BASE*",
    /* XROM 03,41 */ 0x0E9, 0, "BASE/",
    /* XROM 03,42 */ 0x0EA, 0, "BASE+/-",
    /* XROM 09,25 */ 0x259, 0, "POLAR",
    /* XROM 09,26 */ 0x25A, 0, "RECT",
    /* XROM 09,27 */ 0x25B, 0, "RDX.",
    /* XROM 09,28 */ 0x25C, 0, "RDX,",
    /* XROM 09,29 */ 0x25D, 0, "ALL",
    /* XROM 09,30 */ 0x25E, 0, "MENU",
    /* XROM 09,31 */ 0x25F, 0, "X\342\211\2450?", /* >= */
    /* XROM 09,32 */ 0x260, 0, "X\342\211\245Y?", /* >= */
    /* XROM 09,34 */ 0x262, 0, "CLKEYS",
    /* XROM 09,35 */ 0x263, 0, "KEYASN",
    /* XROM 09,36 */ 0x264, 0, "LCLBL",
    /* XROM 09,37 */ 0x265, 0, "REAL?",
    /* XROM 09,38 */ 0x266, 0, "MAT?",
    /* XROM 09,39 */ 0x267, 0, "CPX?",
    /* XROM 09,40 */ 0x268, 0, "STR?",
    /* XROM 09,42 */ 0x26A, 0, "CPXRES",
    /* XROM 09,43 */ 0x26B, 0, "REALRES",
    /* XROM 09,44 */ 0x26C, 0, "EXITALL",
    /* XROM 09,45 */ 0x26D, 0, "CLMENU",
    /* XROM 09,46 */ 0x26E, 0, "GETKEY",
    /* XROM 09,47 */ 0x26F, 0, "CUSTOM",
    /* XROM 09,48 */ 0x270, 0, "ON",
    /* XROM 22,07 */ 0x587, 1, "NOT",       /* Advantage */
    /* XROM 22,08 */ 0x588, 1, "AND",       /* Advantage */
    /* XROM 22,09 */ 0x589, 1, "OR",        /* Advantage */
    /* XROM 22,10 */ 0x58A, 1, "XOR",       /* Advantage */
    /* XROM 22,11 */ 0x58B, 1, "ROTXY",     /* Advantage */
    /* XROM 22,12 */ 0x58C, 1, "BIT?",      /* Advantage */
    /* XROM 24,49 */ 0x631, 1, "AIP",       /* Advantage */
    /* XROM 25,01 */ 0x641, 1, "ALENG",     /* Extended Functions */
    /* XROM 25,06 */ 0x646, 1, "AROT",      /* Extended Functions */
    /* XROM 25,07 */ 0x647, 1, "ATOX",      /* Extended Functions */
    /* XROM 25,28 */ 0x65C, 1, "POSA",      /* Extended Functions */
    /* XROM 25,47 */ 0x66F, 1, "XTOA",      /* Extended Functions */
    /* XROM 25,56 */ 0x678, 1, "\316\243REG?", /* CX Extended Functions */
    /* XROM 26,01 */ 0x681, 1, "ADATE",     /* Time */
    /* XROM 26,04 */ 0x684, 1, "ATIME",     /* Time */
    /* XROM 26,05 */ 0x685, 1, "ATIME24",   /* Time */
    /* XROM 26,06 */ 0x686, 1, "CLK12",     /* Time */
    /* XROM 26,07 */ 0x687, 1, "CLK24",     /* Time */
    /* XROM 26,12 */ 0x68C, 1, "DATE",      /* Time */
    /* XROM 26,13 */ 0x68D, 1, "DATE+",     /* Time */
    /* XROM 26,14 */ 0x68E, 1, "DDAYS",     /* Time */
    /* XROM 26,15 */ 0x68F, 1, "DMY",       /* Time */
    /* XROM 26,16 */ 0x690, 1, "DOW",       /* Time */
    /* XROM 26,17 */ 0x691, 1, "MDY",       /* Time */
    /* XROM 26,28 */ 0x69C, 1, "TIME",      /* Time */
    /* XROM 27,09 */ 0x6C9, 0, "TRANS",
    /* XROM 27,10 */ 0x6CA, 0, "CROSS",
    /* XROM 27,11 */ 0x6CB, 0, "DOT",
    /* XROM 27,12 */ 0x6CC, 0, "DET",
    /* XROM 27,13 */ 0x6CD, 0, "UVEC",
    /* XROM 27,14 */ 0x6CE, 0, "INVRT",
    /* XROM 27,15 */ 0x6CF, 0, "FNRM",
    /* XROM 27,16 */ 0x6D0, 0, "RSUM",
    /* XROM 27,17 */ 0x6D1, 0, "R<>R",
    /* XROM 27,18 */ 0x6D2, 0, "I+",
    /* XROM 27,19 */ 0x6D3, 0, "I-",
    /* XROM 27,20 */ 0x6D4, 0, "J+",
    /* XROM 27,21 */ 0x6D5, 0, "J-",
    /* XROM 27,22 */ 0x6D6, 0, "STOEL",
    /* XROM 27,23 */ 0x6D7, 0, "RCLEL",
    /* XROM 27,24 */ 0x6D8, 0, "STOIJ",
    /* XROM 27,25 */ 0x6D9, 0, "RCLIJ",
    /* XROM 27,26 */ 0x6DA, 0, "NEWMAT",
    /* XROM 27,27 */ 0x6DB, 0, "OLD",
    /* XROM 27,28 */ 0x6DC, 0, "\342\206\220", /* left */
    /* XROM 27,29 */ 0x6DD, 0, "\342\206\222", /* right */
    /* XROM 27,30 */ 0x6DE, 0, "\342\206\221", /* up */
    /* XROM 27,31 */ 0x6DF, 0, "\342\206\223", /* down */
    /* XROM 27,33 */ 0x6E1, 0, "EDIT",
    /* XROM 27,34 */ 0x6E2, 0, "WRAP",
    /* XROM 27,35 */ 0x6E3, 0, "GROW",
    /* XROM 27,39 */ 0x6E7, 0, "DIM?",
    /* XROM 27,40 */ 0x6E8, 0, "GETM",
    /* XROM 27,41 */ 0x6E9, 0, "PUTM",
    /* XROM 27,42 */ 0x6EA, 0, "[MIN]",
    /* XROM 27,43 */ 0x6EB, 0, "[MAX]",
    /* XROM 27,44 */ 0x6EC, 0, "[FIND]",
    /* XROM 27,45 */ 0x6ED, 0, "RNRM",
    /* XROM 29,08 */ 0x748, 1, "PRA",       /* Printer */
    /* XROM 29,18 */ 0x752, 1, "PR\316\243", /* Printer */
    /* XROM 29,19 */ 0x753, 1, "PRSTK",     /* Printer */
    /* XROM 29,20 */ 0x754, 1, "PRX",       /* Printer */
    /* XROM 29,27 */ 0x75B, 0, "MAN",       /* see notes, above */
    /* XROM 29,28 */ 0x75C, 0, "NORM",      /* see notes, above */
    /* XROM 29,29 */ 0x75D, 0, "TRACE",     /* see notes, above */
    /* XROM 29,30 */ 0x75E, 0, "PON",       /* see notes, above */
    /* XROM 29,31 */ 0x75F, 0, "POFF",      /* see notes, above */
    /* XROM 29,32 */ 0x760, 0, "DELAY",     /* see notes, above */
    /* XROM 29,33 */ 0x761, 0, "PRUSR",
    /* XROM 29,34 */ 0x762, 0, "PRLCD",
    /* XROM 29,35 */ 0x763, 0, "CLLCD",
    /* XROM 29,36 */ 0x764, 0, "AGRAPH",
    /* XROM 29,37 */ 0x765, 0, "PIXEL",
    /* XROM 31,15 */ 0x7CF, 0, "ACCEL",     /* Free42 Android/iOS extension */
    /* XROM 31,16 */ 0x7D0, 0, "LOCAT",     /* Free42 Android/iOS extension */
    /* XROM 31,17 */ 0x7D1, 0, "HEADING",   /* Free42 Android/iOS extension */
    /* XROM 31,18 */ 0x7D2, 0, "FPTEST",    /* Free42 Intel FP test suite */
    /* sentinel */      -1, 0, NULL
};

int entry[1024], entry_index[1024], mach_entry[1024];
int rom[65536], rom_size;
int pages, rom_number[16], num_func[16];
int convert_strings = 1;

int entry_index_compar(const void *ap, const void *bp) {
    int a = *((int *) ap);
    int b = *((int *) bp);
    return entry[a] - entry[b];
}

unsigned char chartrans[] = {
    31, 'x', 31, 16, 31, 31, 31, 14, 31, 31, 31, 31, 17, 23, 31, 31,
    31, 31, 38, 20, 20, 22, 22, 28, 28, 29, 29, 25, 25, 12, 18, 30,
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
    96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 7, 124, 15, 5, 127
};

void string_convert(int len, unsigned char *s) {
    int i;
    if (!convert_strings)
        return;
    for (i = 0; i < len; i++)
        s[i] = chartrans[s[i] & 127];
}

void getname(char *dest, int src) {
    int c, len = 0;
    char k;
    char *d = dest;
    do {
        c = rom[--src];
        k = c & 127;
        if (k >= 0 && k <= 31)
            k += 64;
        *d++ = k;
    } while ((c & 128) == 0 && src > 0 && ++len < 16);
    *d = 0;
}

int xrom2index(int modnum, int funcnum) {
    int res = 0;
    int p;
    for (p = 0; p < pages; p++) {
        if (rom_number[p] == modnum)
            break;
        res += num_func[p];
    }
    return res + funcnum;
}

char *hp2ascii(char *src, int len) {
    char *esc;
    unsigned char c;
    static char dst[256];
    int s, d = 0;
    for (s = 0; s < len; s++) {
        c = src[s] & 127;
        switch (c) {
            case   0: esc = "\342\227\206"; break; /* diamond */
            case   1: esc = "\313\243"; break;     /* small high x */
            case   2: esc = "x\314\204"; break;    /* x with macron (mean) */
            case   3: esc = "\342\206\220"; break; /* left arrow */
            case   4: esc = "\316\261"; break;     /* lowercase alpha */
            case   5: esc = "\316\262"; break;     /* lowercase beta */
            case   6: esc = "\316\223"; break;     /* uppercase gamma */
            case   7: esc = "\342\206\223"; break; /* down arrow */
            case   8: esc = "\316\224"; break;     /* uppercase delta */
            case   9: esc = "\317\203"; break;     /* lowercase sigma */
            case  10: esc = "\342\227\206"; break; /* diamond */
            case  11: esc = "\316\273"; break;     /* lowercase lambda */
            case  12: esc = "\316\274"; break;     /* lowercase mu */
            case  13: esc = "\342\210\241"; break; /* measured angle */
            case  14: esc = "\317\204"; break;     /* lowercase tau */
            case  15: esc = "\316\246"; break;     /* uppercase phi */
            case  16: esc = "\316\230"; break;     /* uppercase theta */
            case  17: esc = "\316\251"; break;     /* uppercase omega */
            case  18: esc = "\316\264"; break;     /* lowercase delta */
            case  19: esc = "\303\205"; break;     /* uppercase a with ring */
            case  20: esc = "\303\245"; break;     /* lowercase a with ring */
            case  21: esc = "\303\204"; break;     /* uppercase a with umlaut */
            case  22: esc = "\303\244"; break;     /* lowercase a with umlaut */
            case  23: esc = "\303\226"; break;     /* uppercase o with umlaut */
            case  24: esc = "\303\266"; break;     /* lowercase o with umlaut */
            case  25: esc = "\303\234"; break;     /* uppercase u with umlaut */
            case  26: esc = "\303\274"; break;     /* lowercase u with umlaut */
            case  27: esc = "\303\206"; break;     /* uppercase ae ligature */
            case  28: esc = "\303\246"; break;     /* lowercase ae ligature */
            case  29: esc = "\342\211\240"; break; /* not-equals sign */
            case  30: esc = "\302\243"; break;     /* pound sterling sign */
            case  31: esc = "\342\226\222"; break; /* gray rectangle */
            case  94: esc = "\342\206\221"; break; /* up arrow */
            case 123: esc = "\317\200"; break;     /* lowercase pi */
            case 125: esc = "\342\206\222"; break; /* right arrow */
            case 126: esc = "\316\243"; break;     /* uppercase sigma */
            case 127: esc = "\342\224\234"; break; /* append sign */
            default: dst[d++] = c; continue;
        }
        while (*esc != 0)
            dst[d++] = *esc++;
    }
    dst[d] = 0;
    return dst;
}

char *instr_map[] = {
    "+", "-", "*", "/", "X<Y?", "X>Y?", "X<=Y?", "\316\243+",
    "\316\243-", "HMS+", "HMS-", "MOD", "%", "%CH", "P-R", "R-P",
    "LN", "X\342\206\2212", "SQRT", "Y\342\206\221X", "CHS", "E\342\206\221X", "LOG", "10\342\206\221X",
    "E\342\206\221X-1", "SIN", "COS", "TAN", "ASIN", "ACOS", "ATAN", "DEC",
    "1/X", "ABS", "FACT", "X\342\211\2400?", "X>0?", "LN1+X", "X<0?", "X=0?",
    "INT", "FRC", "D-R", "R-D", "HMS", "HR", "RND", "OCT",
    "CL\316\243", "X<>Y", "PI", "CLST", "R\342\206\221", "RDN", "LASTX", "CLX",
    "X=Y?", "X\342\211\240Y?", "SIGN", "X<=0?", "MEAN", "SDEV", "AVIEW", "CLD",
    "DEG", "RAD", "GRAD", "ENTER\342\206\221", "STOP", "RTN", "BEEP", "CLA",
    "ASHF", "PSE", "CLRG", "AOFF", "AON", "OFF", "PROMPT", "ADV",
    "RCL", "STO", "ST+", "ST-", "ST*", "ST/", "ISG", "DSE",
    "VIEW", "\316\243REG", "ASTO", "ARCL", "FIX", "SCI", "ENG", "TONE",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "SF", "CF", "FS?C", "FC?C", "FS?", "FC?"
};

void bufprintf(char *buf, const char *fmt, ...) {
    int n;
    static char buf2[1024];
    va_list ap;
    va_start(ap, fmt);
    n = vsprintf(buf2, fmt, ap);
    va_end(ap);
    strcat(buf, buf2);
}

char *arg2str(unsigned char x, int one_digit) {
    static char buf[20];
    if (x <= 101)
        sprintf(buf, one_digit ? "%d" : "%02d", x);
    else if (x == 122)
        sprintf(buf, "\342\224\234");
    else if (x <= 127)
        sprintf(buf, "%c", "ABCDEFGHIJTZYXLMNOPQ.abcde"[x - 102]);
    else if (x <= 239)
        sprintf(buf, "IND %02d", x - 128);
    else if (x == 250)
        sprintf(buf, "IND \342\224\234");
    else
        sprintf(buf, "IND %c", "ABCDEFGHIJTZYXLMNOPQ.abcde"[x - 230]);
    return buf;
}

int print_line(int lineno, unsigned char *instr, int len) {
    int k = instr[0];
    static char buf[1024];
    buf[0] = 0;
    if (k == 0x00)
        return 0;
    bufprintf(buf, "%02d", lineno);
    if (k <= 0x0F) {
        bufprintf(buf, "\342\226\270LBL %02d", (k & 15) - 1);
    } else if (k <= 0x1C) {
        int i;
        bufprintf(buf, " ");
        for (i = 0; i < len; i++) {
            k = instr[i];
            if (k >= 0x10 && k <= 0x19)
                bufprintf(buf, "%c", '0' + k - 0x10);
            else if (k == 0x1A)
                bufprintf(buf, ".");
            else if (k == 0x1B)
                bufprintf(buf, " E");
            else if (k == 0x1C)
                bufprintf(buf, "-");
        }
    } else if (k <= 0x1F) {
        if (k == 0x1D)
            bufprintf(buf, " GTO \"");
        else if (k == 0x1E)
            bufprintf(buf, " XEQ \"");
        else
            bufprintf(buf, " W \"");
        bufprintf(buf, "%s\"", hp2ascii(instr + 2, instr[1] & 15));
    } else if (k <= 0x2F) {
        bufprintf(buf, " RCL %02d", k & 15);
    } else if (k <= 0x3F) {
        bufprintf(buf, " STO %02d", k & 15);
    } else if (k <=0x8F) {
        bufprintf(buf, " %s", instr_map[k - 0x40]);
    } else if (k <= 0x9F || k >= 0xA8 && k <= 0xAE) {
        if (k == 0xAE) {
            bufprintf(buf, " %s %s", (instr[1] & 0x80) != 0 ? "XEQ" : "GTO",
                        arg2str(instr[1] | 0x80, 0));
        } else {
            bufprintf(buf, " %s %s", instr_map[k - 0x40],
                        arg2str(instr[1], k >= 0x9C && k <= 0x9F));
        }
    } else if (k <= 0xA7) {
        bufprintf(buf, " XROM %02d,%02d",
                        ((k & 7) << 2) | (instr[1] >> 6), instr[1] & 63);
    } else if (k == 0xAF) {
        bufprintf(buf, " SPARE1");
    } else if (k == 0xB0) {
        bufprintf(buf, " SPARE2");
    } else if (k <= 0xBF) {
        bufprintf(buf, " GTO %02d", (k & 15) - 1);
    } else if (k <= 0xCD) {
        if (instr[2] < 0xF1)
            bufprintf(buf, " END");
        else
            bufprintf(buf, "\342\226\270LBL \"%s\"",
                                hp2ascii(instr + 4, (instr[2] & 15) - 1));
    } else if (k == 0xCE) {
        bufprintf(buf, " X<> %s", arg2str(instr[1], 0));
    } else if (k == 0xCF) {
        /* TODO: how about IND arguments? And how does the
           synthetics-detecting code deal with that? */
        bufprintf(buf, "\342\226\270LBL %s", arg2str(instr[1], 0));
    } else if (k <= 0xDF) {
        bufprintf(buf, " GTO %s", arg2str(instr[2] & 127, 0));
    } else if (k <= 0xEF) {
        bufprintf(buf, " XEQ %s", arg2str(instr[2] & 127, 0));
    } else {
        int len = k & 15;
        if (len > 0 && instr[1] == 127)
            bufprintf(buf, " \342\224\234\"%s\"", hp2ascii(instr + 2, len - 1));
        else
            bufprintf(buf, " \"%s\"", hp2ascii(instr + 1, len));
    }
    printf("%s", buf);
    return strlen(buf);
}

void spaces(int x) {
    do {
        printf(" ");
    } while (--x > 0);
}

int main(int argc, char *argv[]) {
    int argnum;
    FILE *in, *out;
    int pos;
    unsigned char rom_name[256], buf[256], outfile_name[256] = "";
    int f, e, i, j, p, total_func;
    int used_xrom[2048];
    int machine_code_warning = 0;
    int synthetic_code_warning = 0;
    int show_help = 0;
    int list = 0;

    /**********************/
    /* Parse command line */
    /**********************/

    char **argv2 = (char **) malloc(argc * sizeof(char *));
    int argc2 = 0;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0)
            convert_strings = 0;
        else if (strcmp(argv[i], "-l") == 0)
            list = 1;
        else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc)
                strcpy(outfile_name, argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0)
            show_help = 1;
        else
            argv2[argc2++] = argv[i];
    }

    if (show_help || argc2 == 0) {
        printf(
"Usage: rom2raw [-o outputfile] [-s] [-l] [-h] inputfiles...\n"
"    All input files are concatenated and treated as a single ROM image,\n"
"     to facilitate working with multi-page ROMs. To process multiple ROMs,\n"
"     you must run rom2raw once for each ROM.\n"
"    If the -o option is omitted, the ROM name from the image will be used.\n"
"    The -s option disables HP-41 => HP-42S character code translation.\n"
"    The -l option turns on user code listings, to help you find problematic\n"
"     XROMs and synthetic instructions.\n"
"    The -h option shows this message.\n");
        exit(0);
    }

    /************************/
    /* Read ROM image files */
    /************************/

    rom_size = 0;

    printf("Input file%s:", argc2 == 1 ? "" : "s");
    for (argnum = 0; argnum < argc2; argnum++)
        printf(" %s", argv2[argnum]);
    printf("\n");

    for (argnum = 0; argnum < argc2; argnum++) {
        unsigned char rbuf[5];
        in = fopen(argv2[argnum], "rb");
        if (in == NULL) {
            int err = errno;
            printf("Can't open \"%s\" for reading: %s (%d)\n",
                                            argv2[argnum], strerror(err), err);
            exit(1);
        }
        if (fread(rbuf, 1, 5, in) == 5 && strcmp("MOD1", (char *) rbuf) == 0) {
            int num_pages, i;
            if (argc2 > 1) {
                printf("\"%s\" is a MOD file, and MOD files can only be "
                                  "processed one at a time.\n", argv2[argnum]);
                exit(1);
            }
            if (fseek(in, 691, SEEK_CUR) != 0) {
                int err;
                mod_fail:
                err = errno;
                printf("Failed to read MOD file \"%s\": %s (%d)\n",
                                            argv2[argnum], strerror(err), err);
                exit(1);
            }
            num_pages = fgetc(in);
            if (num_pages == EOF)
                goto mod_fail;
            num_pages &= 255;
            if (num_pages > 16) {
                printf("MOD file \"%s\" contains %d pages; rom2raw can only "
                               "handle up to 16.\n", argv2[argnum], num_pages);
                exit(1);
            }
            while (num_pages-- > 0) {
                if (fseek(in, 68, SEEK_CUR) != 0)
                    goto mod_fail;
                for (i = 0; i < 1024; i++) {
                    if (fread(rbuf, 1, 5, in) != 5)
                        goto mod_fail;
                    rom[rom_size++] = rbuf[0] | (rbuf[1] & 3) << 8;
                    rom[rom_size++] = rbuf[1] >> 2 | (rbuf[2] & 15) << 6;
                    rom[rom_size++] = rbuf[2] >> 4 | (rbuf[3] & 63) << 4;
                    rom[rom_size++] = rbuf[3] >> 6 | rbuf[4] << 2;
                }
            }
        } else {
            fseek(in, 0, SEEK_SET);
            while (rom_size < 65536) {
                int c1, c2;
                c1 = fgetc(in);
                if (c1 == EOF)
                    break;
                c2 = fgetc(in);
                if (c2 == EOF)
                    break;
                rom[rom_size++] = (c1 << 8) | c2;
            }
        }
        fclose(in);
    }

    /****************************************/
    /* Determine ROM and output file names, */
    /*   and do some other preparations.    */
    /****************************************/

    num_func[0] = rom[1];
    if (num_func[0] == 0 || num_func[0] > 64)
        num_func[0] = 64;

    pos = ((rom[2] & 255) << 8) | (rom[3] & 255);
    if (pos <= num_func[0] * 2 + 2 || pos >= rom_size) {
        printf("Bad offset to ROM name (%d, rom size = %d)\n", pos, rom_size);
        strcpy(rom_name, "Unnamed");
    } else
        getname(rom_name, pos);

    if (outfile_name[0] == 0) {
        unsigned char ch;
        p = 0;
        do {
            ch = rom_name[p];
            if (ch == ' ' || ch == '/' || ch == '\\')
                ch = '_';
            outfile_name[p] = ch;
            p++;
        } while (ch != 0);
        strcat(outfile_name, ".raw");
    }
    
    printf("Output file: %s\n", outfile_name);
    printf("ROM Name: %s\n", rom_name);
    pages = (rom_size + 4095) / 4096;
    printf("ROM Size: %d (0x%03X), %d page%s\n",
                            rom_size, rom_size, pages, pages == 1 ? "" : "s");

    /********************************************/
    /* Read page directories and compile a list */
    /*  of user and machine code entry points.  */
    /********************************************/

    total_func = 0;

    for (p = 0; p < pages; p++) {
        int page_base = 4096 * p;
        printf("--- Page %d ---\n", p);
        rom_number[p] = rom[page_base];
        if (rom_number[p] > 31) {
            printf("Bad ROM number (%d), using %d instead.\n",
                                        rom_number[p], rom_number[p] & 31);
            rom_number[p] &= 31;
        } else
            printf("ROM Number: %d\n", rom_number[p]);
        num_func[p] = rom[page_base + 1];
        if (num_func[p] <= 0 || num_func[p] > 64) {
            printf("Bad function count (%d), skipping this page.\n",
                                        num_func[p]);
            num_func[p] = 0;
            continue;
        }

        printf("%d functions (XROM %02d,00 - %02d,%02d)\n",
                num_func[p], rom_number[p], rom_number[p], num_func[p] - 1);

        for (f = 0; f < num_func[p]; f++) {
            int mcode;
            e = (rom[page_base + f * 2 + 2] << 8)
                    | (rom[page_base + f * 2 + 3] & 255);
            mcode = (e & 0x20000) == 0;
            e &= 0xffff;
            if (e >= 0x8000)
                e -= 0x10000;
            e += page_base;
            if (mcode) {
                if (e < 0 || e >= rom_size) {
                    printf("Bad machine code entry point for "
                            "XROM %02d,%02d: 0x%03X.\n",
                            rom_number[p], f, e);
                    mach_entry[total_func] = 0;
                } else {
                    getname(buf, e);
                    printf("XROM %02d,%02d: %s %s\n", rom_number[p], f,
                            rom[e] == 0x3E0 ? "dummy entry" : "machine code",
                            hp2ascii(buf, strlen(buf)));
                    mach_entry[total_func] = e;
                    if (rom[e] != 0x3E0)
                        machine_code_warning = 1;
                }
                entry[total_func] = 0;
            } else {
                e &= 0xFFFF;
                if (e >= rom_size - 5) {
                    printf("Bad user code entry point for "
                            "XROM %02d,%02d: 0x%03X, skipping.\n",
                            rom_number[p],
                            f, e);
                    entry[total_func] = 0;
                } else if ((rom[e] & 0xF0) != 0xC0
                            || rom[e + 2] < 0xF2 || rom[e + 2] > 0xF8) {
                    printf("User code entry point (0x%03X) from "
                            "XROM %02d,%02d does not point to a "
                            "global label; skipping.\n",
                            e, rom_number[p], f);
                    entry[total_func] = 0;
                } else {
                    entry[total_func] = e;
                    for (i = 0; i < (rom[e + 2] & 15) - 1; i++)
                        buf[i] = rom[e + 4 + i];
                    buf[i] = 0;
                    printf("XROM %02d,%02d: user code \"%s\"\n",
                                        rom_number[p], f, hp2ascii(buf, i));
                }
            }
            entry_index[total_func] = total_func;
            total_func++;
        }
    }

    if (machine_code_warning)
        printf("Warning: this ROM contains machine code; "
                "this code cannot be translated.\n");

    qsort(entry_index, total_func, sizeof(int), entry_index_compar);
    f = 0;
    while (entry[entry_index[f]] == 0 && f < total_func)
        f++;

    /****************************************/
    /* Open output file and write user code */
    /****************************************/

    out = fopen(outfile_name, "wb");
    if (out == NULL) {
        int err = errno;
        printf("Can't open \"%s\" for writing: %s (%d)\n",
                                            outfile_name, strerror(err), err);
        exit(2);
    }

    for (i = 0; i < 2048; i++)
        used_xrom[i] = 0;

    pos = 0;
    while (f < total_func) {
        unsigned char instr[16];
        int c, k, a;
        int lineno = 0;
        if (entry[entry_index[f]] < pos) {
            f++;
            continue;
        }
        pos = entry[entry_index[f]];
        if (list)
            printf("\n");
        do {
            int synth = 0;
            int conv_off, conv_len = 0;
            lineno++;
            i = 0;
            do {
                c = rom[pos++];
                instr[i++] = c & 255;
            } while ((c & 512) == 0 && (rom[pos] & 256) == 0);
            k = instr[0];
            a = instr[1];
            if (k == 0x00) {
                /* NULL */
                lineno--;
            } else if (k >= 0x1D && k <= 0x1F) {
                /* GTO/XEQ/W <alpha> */
                conv_off = 2;
                conv_len = instr[1] & 15;
                if (k == 0x1F || instr[1] < 0xF1 || instr[1] > 0xF7)
                    /* W instr, or bad label length */
                    synth = 1;
            } else if (k >= 0x90 && k <= 0x98 || k == 0x9A
                    || k == 0x9B || k == 0xAE || k == 0xCE) {
                /* RCL, STO, STO+, STO-, STO*, STO/, ISG, DSE, VIEW,
                 * ASTO, ARCL, GTO/XEQ IND, X<>
                 */
                a &= 127;
                if (a > 99 && a < 112 || a > 116)
                    /* Argument is not 00-99, stack, IND 00-99, or IND stack */
                    synth = 1;
            } else if (k == 0x99) {
                /* SigmaREG */
                if (a > 99 && a < 128 || a > 227 && a < 240 || a > 244)
                    /* Argument is not 00-99, IND 00-99, or IND stack */
                    synth = 1;
            } else if (k >= 0x9C && k <= 0x9F) {
                /* FIX, SCI, ENG, TONE */
                if (a > 9 && a < 128 || a > 227 && a < 240 || a > 244)
                    /* Argument is not 0-9, IND 00-99, or IND stack */
                    synth = 1;
            } else if (k >= 0xA8 && k <= 0xAB) {
                /* SF, CF, FS?C, FC?C */
                if (a > 29 && a < 128 || a > 227 && a < 240 || a > 244)
                    /* Argument is not 00-29, IND 00-99, or IND stack */
                    synth = 1;
            } else if (k == 0xAC || k == 0xAD) {
                /* FS?, FC? */
                if (a > 55 && a < 128 || a > 227 && a < 240 || a > 244)
                    /* Argument is not 00-55, IND 00-99, or IND stack */
                    synth = 1;
            } else if (k == 0xAF || k == 0xB0) {
                /* SPARE1, SPARE2 */
                synth = 1;
            } else if (k >= 0xB1 && k <= 0xBF) {
                /* Short-form GTO; wipe out offset (second byte) */
                instr[1] = 0;
            } else if (k >= 0xC0 && k <= 0xCD) {
                /* Global; wipe out offset
                 * (low nybble of 1st byte + all of 2nd byte)
                 */
                instr[0] &= 0xF0;
                instr[1] = 0;
                if (instr[2] < 0xF1)
                    /* END */
                    instr[2] = 0x0D;
                else {
                    conv_off = 4;
                    conv_len = (instr[2] & 15) - 1;
                    if (instr[2] > 0xF8)
                        /* bad label length */
                        synth = 1;
                }
            } else if (k == 0xCF) {
                /* LBL */
                if (a > 99 && a < 102 || a > 111 && a < 123 || a > 128)
                    /* Argument is not 00-99, A-J, or a-e */
                    synth = 1;
            } else if (k >= 0xD0 && k <= 0xEF) {
                /* Long-form GTO, and XEQ: wipe out offset
                 * (low nybble of 1st byte + all of 2nd byte)
                 * Also wipe out bit 7 of byte 2. I don't know why it
                 * should ever be set, but it happens.
                 */
                instr[0] &= 0xF0;
                instr[1] = 0;
                instr[2] &= 0x7F;
                a = instr[2];
                if (a > 99 && a < 102 || a > 111 && a < 123
                        || a > 227 && a < 240 || a > 244)
                    /* Argument is not 00-99, A-J, a-e,
                     * IND 00-99, or IND stack */
                    synth = 1;
            } else if (k >= 0xA0 && k <= 0xA7) {
                /* XROM */
                int num = ((k & 7) << 8) | instr[1];
                int modnum = num >> 6;
                int instnum = num & 63;
                int islocal = 0;
                for (p = 0; p < pages; p++)
                    if (num_func[p] != 0 && rom_number[p] == modnum) {
                        islocal = 1;
                        break;
                    }
                if (islocal) {
                    /* Local XROM */
                    int idx = xrom2index(modnum, instnum);
                    if (entry[idx] == 0) {
                        /* Mcode XROM, can't translate */
                        used_xrom[num] = 1;
                    } else {
                        /* User code XROM, translate to XEQ */
                        int len = (rom[entry[idx] + 2] & 15) - 1;
                        instr[0] = 0x1E;
                        instr[1] = 0xF0 + len;
                        for (i = 0; i < len; i++)
                            instr[i + 2] = rom[entry[idx] + 4 + i];
                        i = len + 2;
                        conv_off = 2;
                        conv_len = len;
                    }
                } else {
                    /* Nonlocal XROM;
                     * we'll separate the HP-42S XROMs out later
                     */
                    used_xrom[num] = 2;
                }
                if (list) {
                    if (used_xrom[num] == 0) {
                        /* Translated to XEQ; no special action */
                        print_line(lineno, instr, i);
                        printf("\n");
                    } else {
                        int x, mistaken_identity = 0;
                        char linebuf[1024];
                        for (x = 0; hp42s_xroms[x].number != -1; x++)
                            if (hp42s_xroms[x].number == num)
                                break;
                        if (hp42s_xroms[x].number != -1) {
                            if (hp42s_xroms[x].allow) {
                                used_xrom[num] = 0;
                                printf("%02d %s\n", lineno,
                                                    hp42s_xroms[x].name);
                            } else
                                mistaken_identity = 1;
                        }
                        if (used_xrom[num] == 1) {
                            /* Mcode XROM */
                            int y = mach_entry[xrom2index(modnum, instnum)];
                            if (y == 0)
                                sprintf(linebuf,
                                       "%02d XROM %02d,%02d (bad entry)",
                                       lineno, modnum, instnum);
                            else {
                                char namebuf[30];
                                getname(namebuf, y);
                                sprintf(linebuf, "%02d %s", lineno,
                                        hp2ascii(namebuf, strlen(namebuf)));
                            }
                            printf(linebuf);
                            spaces(30 - strlen(linebuf));
                            if (mistaken_identity)
                                printf("Will be mistaken for "
                                        "HP-42S function %s\n",
                                        hp42s_xroms[x].name);
                            else
                                printf("Machine code XROM %02d,%02d\n",
                                                        modnum, instnum);
                        } else if (used_xrom[num] == 2) {
                            sprintf(linebuf, "%02d XROM %02d,%02d",
                                                    lineno, modnum, instnum);
                            printf(linebuf);
                            spaces(30 - strlen(linebuf));
                            if (mistaken_identity)
                                printf("Will be mistaken for "
                                        "HP-42S function %s\n",
                                        hp42s_xroms[x].name);
                            else
                                printf("Non-local XROM\n");
                        }
                    }
                }
            } else if (k == 0xF0) {
                /* zero-length string */
                synth = 1;
            } else if (k > 0xF0) {
                conv_off = 1;
                conv_len = k & 15;
            }
            if (synth)
                synthetic_code_warning = 1;
            if (list && k != 0x00 && (k < 0xA0 || k > 0xA7)) {
                /* NULLs are not printed, and XROMs were handled earlier */
                int x = print_line(lineno, instr, i);
                if (synth) {
                    spaces(30 - x);
                    printf("Synthetic");
                }
                printf("\n");
            }
            if (conv_len != 0)
                string_convert(conv_len, instr + conv_off);
            for (j = 0; j < i; j++)
                fputc(instr[j], out);
        } while ((c & 512) == 0);
    }

    fclose(out);

    /*********************************************************************/
    /* Don't complain about XROMs that match HP-42S instructions         */
    /* if those are indeed the same instructions as in the corresponding */
    /* HP-41 ROMs; complain about all the others.                        */
    /*********************************************************************/

    for (i = 0; hp42s_xroms[i].number != -1; i++) {
        if (hp42s_xroms[i].allow)
            used_xrom[hp42s_xroms[i].number] = 0;
        else if (used_xrom[hp42s_xroms[i].number] != 0)
            used_xrom[hp42s_xroms[i].number] = 3;
    }

    /************************************************/
    /* Print warning about local machine code XROMs */
    /************************************************/

    j = 0;
    for (i = 0; i < 2048; i++) {
        if (used_xrom[i] == 1) {
            int p;
            if (j == 0) {
                j = 1;
                printf("\nThe following machine code XROMs were called "
                        "from user code:\n");
            }
            p = mach_entry[xrom2index(i >> 6, i & 63)];
            if (p == 0)
                strcpy(buf, "(bad entry point)");
            else
                getname(buf, p);
            printf("XROM %02d,%02d: %s\n", i >> 6, i & 63,
                                                hp2ascii(buf, strlen(buf)));
        }
    }

    /***************************************/
    /* Print warning about non-local XROMs */
    /***************************************/

    for (i = 0; i < 2048; i++) {
        if (used_xrom[i] == 2) {
            if (j < 2) {
                if (j == 0)
                    printf("\n");
                j = 2;
                printf("The following non-local XROMs were called "
                        "from user code:\n");
            }
            printf("XROM %02d,%02d\n", i >> 6, i & 63);
        }
    }

    /**************************************************************/
    /* Print warnings about XROMs that match HP-42S instructions, */
    /* but which have different meanings on the HP-41 than on the */
    /* HP-42S (these are the ones that have allow=0 in the        */
    /* hp42s_xroms table at the beginning of this file).          */
    /**************************************************************/

    for (i = 0; i < 2048; i++) {
        if (used_xrom[i] == 3) {
            int p;
            if (j < 3) {
                if (j == 0)
                    printf("\n");
                j = 3;
                printf("The following XROMs were called which are "
                        "going to be\nmistaken for HP-42S commands:\n");
            }
            for (f = 0; hp42s_xroms[f].number != i; f++);
            p = mach_entry[xrom2index(i >> 6, i & 63)];
            if (p == 0)
                strcpy(buf, "(bad entry point)");
            else
                getname(buf, p);
            printf("XROM %02d,%02d: %s => %s\n", i >> 6, i & 63,
                            hp2ascii(buf, strlen(buf)), hp42s_xroms[f].name);
        }
    }

    /******************************************************/
    /* Print a few final words of caution, if appropriate */
    /******************************************************/

    if (j != 0)
        printf("Because of these XROM calls, "
                "the converted user code may not work.\n");
    if (synthetic_code_warning)
        printf("\nWarning: this ROM contains synthetic code;\n"
                "this code will probably fail on a HP-42S.\n");
    printf("\n");

    return 0;
}
