/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2009  Thomas Okken
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "skin2prc.h"


#define NUM_DEPTHS 5
int bitmap_id = 200;
int density = 72;
int depth[NUM_DEPTHS] = { 1, 4, 5, 8, 16 };
int do_depth[NUM_DEPTHS] = { 1, 1, 0, 1, 0 };
int dither = 0;
int sections = 1;
char gif[256] = "";
FILE *script = NULL;
FILE *rcp = NULL;

#define SKIN_MAX_MACRO_LENGTH 31

typedef struct _SkinMacro {
    int code;
    unsigned char macro[SKIN_MAX_MACRO_LENGTH + 1];
    struct _SkinMacro *next;
} SkinMacro;

SkinMacro *macrolist = NULL;
int nmacros = 0;


void usage() {
    fprintf(stderr, "Usage: skin2prc -layout <layoutfile> \\\n"
		    "                -gif <giffile> \\\n"
		    "                -script <generated driver script> \\\n"
		    "                -rcp <generated PilRC resource script> \\\n"
		    "                -name <skin's display name> \\\n"
		    "                [-dither <on or off>; default off] \\\n"
		    "                [-sections <number of pieces to cut skin into>]\\\n"
		    "                [-skin_id <skin resource id>; default 100] \\\n"
		    "                [-bitmap_id <starting bitmap resource id>; default 200] \\\n"
		    "                [-depths <1,4,5,8,16 or any subset>; default 1,4,8] \\\n"
		    "                [-density <72 or 144>; default 72]\n");
    exit(0);
}

int2 swap_int2(int2 x) {
    int q = 0x01020304;
    if (*(char *) &q == 0x01)
	return x;
    else {
	uint2 y = x;
	return (y << 8) | (y >> 8);
    }
}

void generate_bitmap(int x, int y, int width, int height) {
    int i;
    static int inited = 0;

    if (!inited) {
	for (i = 0; i < NUM_DEPTHS; i++) {
	    char *map;
	    if (!do_depth[i])
		continue;
	    switch (depth[i]) {
		case 1:
		    map = "2grays.ppm";
		    break;
		case 4:
		    map = "16grays.ppm";
		    break;
		case 5:
		    map = "16colors.ppm";
		    break;
		case 8:
		    map = "256colors.ppm";
		    break;
		case 16:
		    break;
		default:
		    fprintf(stderr, "Unsupported bitmap depth (%d) specified.\n", depth[i]);
		    exit(1);
	    }
	    if (depth[i] == 16)
		fprintf(script, "giftopnm %s | ppmtoppm >tmp_skin.16.ppm\n", gif);
	    else
		fprintf(script, "giftopnm %s | ppmtoppm | pnmremap %s -mapfile=%s >tmp_skin.%d.ppm\n", gif, dither ? "-fs" : "", map, depth[i]);
	}
	inited = 1;
    }

    fprintf(rcp, "bitmap id %d\n", bitmap_id);
    fprintf(rcp, "compress\n");
    fprintf(rcp, "begin\n");
    for (i = 0; i < NUM_DEPTHS; i++) {
	char *map;
	int d2;
	if (!do_depth[i])
	    continue;
	d2 = depth[i];
	if (d2 == 5)
	    d2 = 4;
	if (d2 == 16)
	    fprintf(script, "pnmcut %d %d %d %d tmp_skin.16.ppm | ppmtobmp -bpp=24 >tmp_skin.%d.16.bmp\n", x, y, width, height, bitmap_id);
	else
	    fprintf(script, "pnmcut %d %d %d %d tmp_skin.%d.ppm | ppmtobmp -bpp=%d >tmp_skin.%d.%d.bmp\n", x, y, width, height, depth[i], d2, bitmap_id, depth[i]);
	fprintf(rcp, "  bitmap \"tmp_skin.%d.%d.bmp\" BPP %d DENSITY %d\n", bitmap_id, depth[i], depth[i], density);
    }
    fprintf(rcp, "end\n\n");
    bitmap_id++;
}

int main(int argc, char *argv[]) {
    int skin_id = 100;
    int i, nkeys = 0;
    
    FILE *layout = NULL;
    FILE *tmp;

    SkinSpec *skin;
    SkinSpec2 *skin2;
    int skinsize;
    char line[1024];

    skin = (SkinSpec *) malloc(sizeof(SkinSpec) + 99 * sizeof(KeySpec));
    // TODO - handle memory allocation failure
    skin->version[0] = 0;
    skin->version[1] = 3;
    // Starting with version 2, version[3] specifies whether the skin is tall.
    // This is populated later.


    /**************************************/
    /***** Parse command line options *****/
    /**************************************/

    if (argc < 3)
	usage();

    for (i = 1; i < argc; i++) {
	char *opt = argv[i];
	if (strcmp(opt, "-h") == 0
		|| strcmp(opt, "-help") == 0
		|| strcmp(opt, "--help") == 0)
	    usage();
	else if (strcmp(opt, "-depths") == 0) {
	    char *s = argv[++i];
	    char *t;
	    int j;
	    for (j = 0; j < NUM_DEPTHS; j++)
		do_depth[j] = 0;
	    while ((t = strtok(s, ",")) != NULL) {
		int d;
		s = NULL;
		if (sscanf(t, "%d", &d) != 1)
		    continue;
		for (j = 0; j < NUM_DEPTHS; j++)
		    if (d == depth[j]) {
			do_depth[j] = 1;
			break;
		    }
	    }
	} else if (strcmp(opt, "-density") == 0) {
	    if (sscanf(argv[++i], "%d", &density) != 1
		    || (density != 72 && density != 144)) {
		fprintf(stderr, "Bad -density value (must be 72 or 144)\n");
		usage();
	    }
	} else if (strcmp(opt, "-skin_id") == 0) {
	    sscanf(argv[++i], "%d", &skin_id);
	} else if (strcmp(opt, "-bitmap_id") == 0) {
	    sscanf(argv[++i], "%d", &bitmap_id);
	} else if (strcmp(opt, "-layout") == 0) {
	    char *layout_name = argv[++i];
	    layout = fopen(layout_name, "r");
	    if (layout == NULL) {
		int err = errno;
		fprintf(stderr, "Can't open \"%s\" for input: %s (%d)\n", layout_name, strerror(err), err);
		exit(1);
	    }
	} else if (strcmp(opt, "-gif") == 0) {
	    strcpy(gif, argv[++i]);
	} else if (strcmp(opt, "-script") == 0) {
	    char *script_name = argv[++i];
	    script = fopen(script_name, "w");
	    if (script == NULL) {
		int err = errno;
		fprintf(stderr, "Can't open \"%s\" for output: %s (%d)\n", script_name, strerror(err), err);
		exit(1);
	    }
	} else if (strcmp(opt, "-rcp") == 0) {
	    char *rcp_name = argv[++i];
	    rcp = fopen(rcp_name, "w");
	    if (rcp == NULL) {
		int err = errno;
		fprintf(stderr, "Can't open \"%s\" for output: %s (%d)\n", rcp_name, strerror(err), err);
		exit(1);
	    }
	} else if (strcmp(opt, "-name") == 0) {
	    strncpy(skin->name, argv[++i], 31);
	    skin->name[31] = 0;
	} else if (strcmp(opt, "-dither") == 0) {
	    char *v = argv[++i];
	    if (strcmp(v, "on") == 0)
		dither = 1;
	    else if (strcmp(v, "off") == 0)
		dither = 0;
	    else {
		fprintf(stderr, "Bad -dither value (must be \"on\" or \"off\")\n");
		usage();
	    }
	} else if (strcmp(opt, "-sections") == 0) {
	    sscanf(argv[++i], "%d", &sections);
	    if (sections < 1)
		sections = 1;
	    if (sections > 16)
		sections = 16;
	} else {
	    fprintf(stderr, "Unrecognized option \"%s\"\n", opt);
	    usage();
	}
    }

    if (gif[0] == 0 || layout == NULL || script == NULL || rcp == NULL)
	usage();

    
    /****************************/
    /***** Read skin layout *****/
    /****************************/

    skin->density = swap_int2(density);

    fprintf(script, "#!/bin/sh\n");

    while (fgets(line, 1024, layout) != NULL) {
	char *c = strchr(line, '#');
	if (c != NULL)
	    *c = 0;
	c = strchr(line, '\n');
	if (c != NULL)
	    *c = 0;
	c = strchr(line, '\r');
	if (c != NULL)
	    *c = 0;
	if (line[0] == 0)
	    continue;
	if (strncasecmp(line, "skin:", 5) == 0) {
	    int x, y, width, height;
	    if (sscanf(line + 5, " %d,%d,%d,%d", &x, &y, &width, &height) == 4) {
		int yy = 0;
		int sh = (height + sections - 1) / sections;

		skin->skin_bitmap = swap_int2(bitmap_id);
		while (yy < height) {
		    if (yy + sh > height)
			generate_bitmap(x, yy, width, height - yy);
		    else
			generate_bitmap(x, yy, width, sh);
		    yy += sh;
		}
	    }
	    // Flag to indicate if this is a "tall" skin
	    // Such skins will be suppressed on square-screen devices.
	    skin->version[3] = height > width;
	} else if (strncasecmp(line, "display:", 8) == 0) {
	    int x, y, xscale, yscale;
	    unsigned long fg, bg;
	    if (sscanf(line + 8, " %d,%d %d %d %lx %lx", &x, &y,
			&xscale, &yscale, &bg, &fg) == 6) {
		skin->display_x = swap_int2(x);
		skin->display_y = swap_int2(y);
		skin->display_xscale = swap_int2(xscale);
		skin->display_yscale = swap_int2(yscale);
		skin->display_bg.r = bg >> 16;
		skin->display_bg.g = bg >> 8;
		skin->display_bg.b = bg;
		skin->display_bg.index = 0;
		skin->display_fg.r = fg >> 16;
		skin->display_fg.g = fg >> 8;
		skin->display_fg.b = fg;
		skin->display_fg.index = 0;
	    }
	} else if (strncasecmp(line, "key:", 4) == 0) {
	    char keynumbuf[20];
	    int keynum, shifted_keynum;
	    int sens_x, sens_y, sens_width, sens_height;
	    int disp_x, disp_y, disp_width, disp_height;
	    int act_x, act_y;
	    if (sscanf(line + 4, " %s %d,%d,%d,%d %d,%d,%d,%d %d,%d",
			keynumbuf,
			&sens_x, &sens_y, &sens_width, &sens_height,
			&disp_x, &disp_y, &disp_width, &disp_height,
			&act_x, &act_y) == 11) {
		int n = sscanf(keynumbuf, "%d,%d", &keynum, &shifted_keynum);
		if (n > 0) {
		    if (n == 1)
			shifted_keynum = keynum;
		    KeySpec *k = skin->key + nkeys;
		    k->code = swap_int2(keynum);
		    k->shifted_code = swap_int2(shifted_keynum);
		    k->sens_x = swap_int2(sens_x);
		    k->sens_y = swap_int2(sens_y);
		    k->sens_width = swap_int2(sens_width);
		    k->sens_height = swap_int2(sens_height);
		    k->x = swap_int2(disp_x);
		    k->y = swap_int2(disp_y);
		    k->up_bitmap = swap_int2(bitmap_id);
		    generate_bitmap(disp_x, disp_y, disp_width, disp_height);
		    k->down_bitmap = swap_int2(bitmap_id);
		    generate_bitmap(act_x, act_y, disp_width, disp_height);
		    nkeys++;
		}
	    }
	} else if (strncasecmp(line, "macro:", 6) == 0) {
	    char *tok = strtok(line + 6, " ");
	    int len = 0;
	    SkinMacro *macro = NULL;
	    while (tok != NULL) {
		char *endptr;
		long n = strtol(tok, &endptr, 10);
		if (*endptr != 0) {
		    /* Not a proper number; ignore this macro */
		    if (macro != NULL) {
			free(macro);
			macro = NULL;
			break;
		    }
		}
		if (macro == NULL) {
		    if (n < 38 || n > 255)
			/* Macro code out of range; ignore this macro */
			break;
		    macro = (SkinMacro *) malloc(sizeof(SkinMacro));
		    // TODO - handle memory allocation failure
		    macro->code = n;
		} else if (len < SKIN_MAX_MACRO_LENGTH) {
		    if (n < 1 || n > 37) {
			/* Key code out of range; ignore this macro */
			free(macro);
			macro = NULL;
			break;
		    }
		    macro->macro[len++] = n;
		}
		tok = strtok(NULL, " ");
	    }
	    if (macro != NULL) {
		macro->macro[len++] = 0;
		macro->next = macrolist;
		macrolist = macro;
		nmacros++;
	    }
	} else if (strncasecmp(line, "annunciator:", 12) == 0) {
	    int annnum;
	    int disp_x, disp_y, disp_width, disp_height;
	    int act_x, act_y;
	    if (sscanf(line + 12, " %d %d,%d,%d,%d %d,%d",
			&annnum,
			&disp_x, &disp_y, &disp_width, &disp_height,
			&act_x, &act_y) == 7) {
		if (annnum >= 1 && annnum <= 7) {
		    AnnunciatorSpec *a = skin->annunciator + (annnum - 1);
		    a->x = swap_int2(disp_x);
		    a->y = swap_int2(disp_y);
		    a->off_bitmap = swap_int2(bitmap_id);
		    generate_bitmap(disp_x, disp_y, disp_width, disp_height);
		    a->on_bitmap = swap_int2(bitmap_id);
		    generate_bitmap(act_x, act_y, disp_width, disp_height);
		}
	    }
	}
    }

    fclose(layout);
    fclose(script);
    skin->nkeys = swap_int2(nkeys);
    skin->sections = swap_int2(sections);


    /*****************************************/
    /***** Append macros to the SkinSpec *****/
    /*****************************************/

    skinsize = sizeof(SkinSpec) + (nkeys - 1) * sizeof(KeySpec)
		+ sizeof(SkinSpec2) + (nmacros - 1) * sizeof(MacroSpec);
    skin = (SkinSpec *) realloc(skin, skinsize);
    // TODO - handle memory allocation failure
    skin2 = (SkinSpec2 *) (skin->key + nkeys);
    skin2->nmacros = swap_int2(nmacros);
    for (i = 0; i < nmacros; i++) {
	skin2->macro[i].code = swap_int2(macrolist->code);
	strcpy((char *) skin2->macro[i].macro, (const char *) macrolist->macro);
	macrolist = macrolist->next;
    }


    /****************************************/
    /***** Generate the 'Skin' resource *****/
    /****************************************/

    tmp = fopen("tmp_skin.data", "w");
    if (tmp == NULL) {
	int err = errno;
	fprintf(stderr, "Can't open \"tmp_skin.data\" for output: %s (%d)\n", strerror(err), err);
	exit(1);
    }
    fwrite(skin, 1, skinsize, tmp);
    fclose(tmp);

    fprintf(rcp, "data \"Skin\" id %d \"tmp_skin.data\"\n", skin_id);
    fclose(rcp);
    exit(0);
}
