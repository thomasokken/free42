/*****************************************************************************
 * rom2raw -- extracts user code from HP-41 ROM images
 * Copyright (C) 2006  Thomas Okken
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* TODO: I'm allowing all XROM numbers that are valid HP-42S instructions.
 * I should eliminate all those that the 42S did *not* inherit from the
 * 82143, Advantage Pac, etc. -- since we're importing HP-41 code, all other
 * matching XROM codes are just coincidental and are pretty much guaranteed
 * NOT to do what the original code expects.
 */
int hp42s_xroms[] = {
    /* XROM 01,33: SINH */      0x061,
    /* XROM 01,34: COSH */      0x062,
    /* XROM 01,35: TANH */      0x063,
    /* XROM 01,36: ASINH */     0x064,
    /* XROM 01,37: ATANH */     0x065,
    /* XROM 01,38: ACOSH */     0x066,
    /* XROM 01,47: COMB */      0x06F,
    /* XROM 01,48: PERM */      0x070,
    /* XROM 01,49: RAN */       0x071,
    /* XROM 01,50: COMPLEX */   0x072,
    /* XROM 01,51: SEED */      0x073,
    /* XROM 01,52: GAMMA */     0x074,
    /* XROM 02,31: BEST */      0x09F,
    /* XROM 02,32: EXPF */      0x0A0,
    /* XROM 02,33: LINF */      0x0A1,
    /* XROM 02,34: LOGF */      0x0A2,
    /* XROM 02,35: PWRF */      0x0A3,
    /* XROM 02,36: SLOPE */     0x0A4,
    /* XROM 02,37: SUM */       0x0A5,
    /* XROM 02,38: YINT */      0x0A6,
    /* XROM 02,39: CORR */      0x0A7,
    /* XROM 02,40: FCSTX */     0x0A8,
    /* XROM 02,41: FCSTY */     0x0A9,
    /* XROM 02,42: INSR */      0x0AA,
    /* XROM 02,43: DELR */      0x0AB,
    /* XROM 02,44: WMEAN */     0x0AC,
    /* XROM 02,45: LINSigma */  0x0AD,
    /* XROM 02,46: ALLSigma */  0x0AE,
    /* XROM 03,34: HEXM */      0x0E2,
    /* XROM 03,35: DECM */      0x0E3,
    /* XROM 03,36: OCTM */      0x0E4,
    /* XROM 03,37: BINM */      0x0E5,
    /* XROM 03,38: BASE+ */     0x0E6,
    /* XROM 03,39: BASE- */     0x0E7,
    /* XROM 03,40: BASE* */     0x0E8,
    /* XROM 03,41: BASE/ */     0x0E9,
    /* XROM 03,42: BASE+/- */   0x0EA,
    /* XROM 09,25: POLAR */     0x259,
    /* XROM 09,26: RECT */      0x25A,
    /* XROM 09,27: RDX. */      0x25B,
    /* XROM 09,28: RDX, */      0x25C,
    /* XROM 09,29: ALL */       0x25D,
    /* XROM 09,30: MENU */      0x25E,
    /* XROM 09,31: X>=0? */     0x25F,
    /* XROM 09,32: X>=Y? */     0x260,
    /* XROM 09,34: CLKEYS */    0x262,
    /* XROM 09,35: KEYASN */    0x263,
    /* XROM 09,36: LCLBL */     0x264,
    /* XROM 09,37: REAL? */     0x265,
    /* XROM 09,38: MAT? */      0x266,
    /* XROM 09,39: CPX? */      0x267,
    /* XROM 09,40: STR? */      0x268,
    /* XROM 09,42: CPXRES */    0x26A,
    /* XROM 09,43: REALRES */   0x26B,
    /* XROM 09,44: EXITALL */   0x26C,
    /* XROM 09,45: CLMENU */    0x26D,
    /* XROM 09,46: GETKEY */    0x26E,
    /* XROM 09,47: CUSTOM */    0x26F,
    /* XROM 09,48: ON */        0x270,
    /* XROM 22,07: NOT */       0x587,
    /* XROM 22,08: AND */       0x588,
    /* XROM 22,09: OR */        0x589,
    /* XROM 22,10: XOR */       0x58A,
    /* XROM 22,11: ROTXY */     0x58B,
    /* XROM 22,12: BIT? */      0x58C,
    /* XROM 24,49: AIP */       0x631,
    /* XROM 25,01: ALENG */     0x641,
    /* XROM 25,06: AROT */      0x646,
    /* XROM 25,07: ATOX */      0x647,
    /* XROM 25,28: POSA */      0x65C,
    /* XROM 25,47: XTOA */      0x66F,
    /* XROM 25,56: SigmaREG? */ 0x678,
    /* XROM 27,09: TRANS */     0x6C9,
    /* XROM 27,10: CROSS */     0x6CA,
    /* XROM 27,11: DOT */       0x6CB,
    /* XROM 27,12: DET */       0x6CC,
    /* XROM 27,13: UVEC */      0x6CD,
    /* XROM 27,14: INVRT */     0x6CE,
    /* XROM 27,15: FNRM */      0x6CF,
    /* XROM 27,16: RSUM */      0x6D0,
    /* XROM 27,17: R<>R */      0x6D1,
    /* XROM 27,18: I+ */        0x6D2,
    /* XROM 27,19: I- */        0x6D3,
    /* XROM 27,20: J+ */        0x6D4,
    /* XROM 27,21: J- */        0x6D5,
    /* XROM 27,22: STOEL */     0x6D6,
    /* XROM 27,23: RCLEL */     0x6D7,
    /* XROM 27,24: STOIJ */     0x6D8,
    /* XROM 27,25: RCLIJ */     0x6D9,
    /* XROM 27,26: NEWMAT */    0x6DA,
    /* XROM 27,27: OLD */       0x6DB,
    /* XROM 27,28: <- */        0x6DC,
    /* XROM 27,29: -> */        0x6DD,
    /* XROM 27,30: ^ */         0x6DE,
    /* XROM 27,31: v */         0x6DF,
    /* XROM 27,33: EDIT */      0x6E1,
    /* XROM 27,34: WRAP */      0x6E2,
    /* XROM 27,35: GROW */      0x6E3,
    /* XROM 27,39: DIM? */      0x6E7,
    /* XROM 27,40: GETM */      0x6E8,
    /* XROM 27,41: PUTM */      0x6E9,
    /* XROM 27,42: [MIN] */     0x6EA,
    /* XROM 27,43: [MAX] */     0x6EB,
    /* XROM 27,44: [FIND] */    0x6EC,
    /* XROM 27,45: RNRM */      0x6ED,
    /* XROM 29,08: PRA */       0x748,
    /* XROM 29,18: PRSigma */   0x752,
    /* XROM 29,19: PRSTK */     0x753,
    /* XROM 29,20: PRX */       0x754,
    /* XROM 29,27: MAN */       0x75B,
    /* XROM 29,28: NORM */      0x75C,
    /* XROM 29,29: TRACE */     0x75D,
    /* XROM 29,30: PON */       0x75E,
    /* XROM 29,31: POFF */      0x75F,
    /* XROM 29,32: DELAY */     0x760,
    /* XROM 29,33: PRUSR */     0x761,
    /* XROM 29,34: PRLCD */     0x762,
    /* XROM 29,35: CLLCD */     0x763,
    /* XROM 29,36: AGRAPH */    0x764,
    /* XROM 29,37: PIXEL */     0x765,
    /* sentinel */                 -1
};

int entry[256], entry_index[256];
int rom[65536], rom_size;

int entry_index_compar(const void *ap, const void *bp) {
    int a = *((int *) ap);
    int b = *((int *) bp);
    return entry[a] - entry[b];
}

char *fallback_argv[] = { "foo", "-", NULL };

void getname(char *dest, int src, int space_to_underscore) {
    int c;
	char k;
    do {
	c = rom[--src];
	k = c & 127;
	if (k >= 0 && k <= 31)
	    k += 64;
	if (k == ' ' && space_to_underscore)
	    k = '_';
	*dest++ = k;
    } while ((c & 128) == 0);
    *dest = 0;
}

int main(int argc, char *argv[]) {
    int argnum;
    FILE *in, *out;
    int rom_number, num_func, pos;
    char rom_name[256], buf[256];
    int f, e, i, j;
    int used_xrom[2048];
    int machine_code_warning;

    if (argc == 1) {
	argc = 2;
	argv = fallback_argv;
    }

    for (argnum = 1; argnum < argc; argnum++) {
	printf("\n");
	if (strcmp(argv[argnum], "-") != 0) {
	    in = fopen(argv[argnum], "rb");
	    if (in == NULL) {
		int err = errno;
		printf("Can't open \"%s\" for reading: %s (%d)\n", argv[argnum], strerror(err), err);
		continue;
	    }
	} else
	    in = stdin;

	printf("Input file: %s\n", argv[argnum]);

	rom_size = 0;
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
	if (strcmp(argv[argnum], "-") != 0)
	    fclose(in);

	rom_number = rom[0];
	if (rom_number > 31) {
	    printf("Bad ROM number (%d), aborting.\n", rom_number);
	    continue;
	}
	num_func = rom[1] - 1;
	if (num_func == 0 || num_func > 63) {
	    printf("Bad function count (%d), aborting.\n", num_func);
	    continue;
	}

	pos = ((rom[2] & 255) << 8) | (rom[3] & 255);
	if (pos <= num_func * 2 + 2 || pos >= rom_size) {
	    printf("Bad offset to ROM name (%d, rom size = %d), aborting.\n", pos, rom_size);
	    continue;
	}

	getname(rom_name, pos, 1);
	printf("Output file: %s.raw\n", rom_name);
	printf("ROM Name: %s\n", rom_name);
	printf("ROM Number: %d\n", rom_number);
	printf("%d functions (XROM %02d,01 - %02d,%02d)\n", num_func, rom_number, rom_number, num_func);
	printf("ROM Size: %d (0x%03X)\n\n", rom_size, rom_size);

	machine_code_warning = 0;

	for (f = 1; f <= num_func; f++) {
	    e = (rom[f * 2 + 2] << 8) | (rom[f * 2 + 3] & 255);
	    if ((e & 0x20000) == 0) {
		if (e >= rom_size) {
		    printf("Bad machine code entry point for XROM %02d,%02d: 0x%03X, aborting.\n", rom_number, f, e);
		    goto end_file_loop;
		}
		getname(buf, e, 0);
		printf("XROM %02d,%02d: machine code %s\n", rom_number, f, buf);
		entry[f] = 0;
		machine_code_warning = 1;
	    } else {
		e &= 0xFFFF;
		if (e >= rom_size - 5) {
		    printf("Bad user code entry point for XROM %02d,%02d: 0x%03X, aborting.\n", rom_number, f, e);
		    goto end_file_loop;
		}
		if (rom[e] & 0xF0 != 0xC0
			|| rom[e + 2] < 0xF2 || rom[e + 2] > 0xF8) {
		    printf("User code entry point (0x%03X) from XROM %02d,%02d does not point to a global label; aborting.\n", e, rom_number, f);
		    goto end_file_loop;
		}
		entry[f] = e;
		for (i = 0; i < (rom[e + 2] & 15) - 1; i++)
		    buf[i] = rom[e + 4 + i];
		buf[i] = 0;
		printf("XROM %02d,%02d: user code \"%s\"\n", rom_number, f, buf);
	    }
	    entry_index[f] = f;
	}

	if (machine_code_warning)
	    printf("Warning: this ROM contains machine code; this code cannot be translated.\n");

	qsort(entry_index + 1, num_func, sizeof(int), entry_index_compar);
	f = 1;
	while (entry[entry_index[f]] == 0 && f <= num_func)
	    f++;

	strcpy(buf, rom_name);
	strcat(buf, ".raw");
	out = fopen(buf, "wb");
	if (out == NULL) {
	    int err = errno;
	    printf("Can't open \"%s\" for writing: %s (%d)\n", buf, strerror(err), err);
	    continue;
	}

	for (i = 0; i < 2048; i++)
	    used_xrom[i] = 0;

	pos = 0;
	while (f <= num_func) {
	    unsigned char instr[16];
	    int c, k;
	    if (entry[entry_index[f]] < pos) {
		f++;
		continue;
	    }
	    pos = entry[entry_index[f]];
	    do {
		i = 0;
		do {
		    c = rom[pos++];
		    instr[i++] = c & 255;
		} while ((c & 512) == 0 && (rom[pos] & 256) == 0);
		k = instr[0];
		if (k >= 0xB1 && k <= 0xBF) {
		    /* Short-form GTO; wipe out offset (second byte) */
		    instr[1] = 0;
		} else if (k >= 0xC0 && k <= 0xCD) {
		    /* Global; wipe out offset (low nybble of 1st byte + all of 2nd byte) */
		    instr[0] &= 0xF0;
		    instr[1] = 0;
		    if (instr[2] < 0xF1)
			/* END */
			instr[2] = 0x0D;
		} else if (k >= 0xD0 && k <= 0xEF) {
		    /* Long-form GTO, and XEQ: wipe out offset (low nybble of 1st byte + all of 2nd byte) */
		    instr[0] &= 0xF0;
		    instr[1] = 0;
		} else if (k >= 0xA0 && k <= 0xA7) {
		    /* XROM */
		    int num = ((k & 7) << 8) | instr[1];
		    int modnum = num >> 6;
		    int instnum = num & 63;
		    if (modnum == rom_number) {
			/* Local XROM */
			if (entry[instnum] == 0) {
			    /* Mcode XROM, can't translate */
			    used_xrom[num] = 1;
			} else {
			    /* User code XROM, translate to XEQ */
			    int len = (rom[entry[instnum] + 2] & 15) - 1;
			    instr[0] = 0x1E;
			    instr[1] = 0xF0 + len;
			    for (i = 0; i < len; i++)
				instr[i + 2] = rom[entry[instnum] + 4 + i];
			    i = len + 2;
			}
		    } else {
			/* Nonlocal XROM; we'll separate the HP-42S XROMs out later */
			used_xrom[num] = 2;
		    }
		}
		for (j = 0; j < i; j++)
		    fputc(instr[j], out);
	    } while ((c & 512) == 0);
	}

	fclose(out);

	/* Don't complain about XROMs that match HP-42S instructions */
	for (i = 0; hp42s_xroms[i] != -1; i++)
	    used_xrom[hp42s_xroms[i]] = 0;

	j = 0;
	for (i = 0; i < 2048; i++) {
	    if (used_xrom[i] == 1) {
		int p;
		if (j == 0) {
		    j = 1;
		    printf("\nThe following machine code XROMs were called from user code:\n");
		}
		p = ((rom[(i & 63) * 2 + 2] & 255) << 8) | (rom[(i & 63) * 2 + 3] & 255);
		getname(buf, p, 0);
		printf("XROM %02d,%02d: %s\n", rom_number, i & 63, buf);
	    }
	}

	for (i = 0; i < 2048; i++) {
	    if (used_xrom[i] == 2) {
		if (j < 2) {
		    if (j == 0)
			printf("\n");
		    j = 2;
		    printf("The following non-local XROMs were called from user code:\n");
		}
		printf("XROM %02d,%02d\n", i >> 6, i & 63);
	    }
	}

	if (j != 0)
	    printf("Because of these XROM calls, the converted user code may not work.\n");

	end_file_loop:;
    }

    return 0;
}
