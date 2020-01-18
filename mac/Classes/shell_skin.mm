/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
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

#import <dirent.h>
#import "shell_skin.h"
#import "shell_loadimage.h"
#import "core_main.h"
#import "Free42AppDelegate.h"
#import "CalcView.h"

/**************************/
/* Skin description stuff */
/**************************/

typedef struct {
    int x, y;
} SkinPoint;

typedef struct {
    int x, y, width, height;
} SkinRect;

typedef struct {
    int code, shifted_code;
    SkinRect sens_rect;
    SkinRect disp_rect;
    SkinPoint src;
} SkinKey;

#define SKIN_MAX_MACRO_LENGTH 63

typedef struct _SkinMacro {
    int code;
    bool isName;
    unsigned char macro[SKIN_MAX_MACRO_LENGTH + 1];
    struct _SkinMacro *next;
} SkinMacro;

typedef struct {
    SkinRect disp_rect;
    SkinPoint src;
} SkinAnnunciator;

static SkinKey *keylist = NULL;
static int nkeys = 0;
static int keys_cap = 0;
static int currently_pressed_key = -1;
static SkinMacro *macrolist = NULL;
static SkinAnnunciator annunciators[7];
static int annunciator_state[7];

static SkinRect skin;
static SkinPoint display_loc;
static CGPoint display_scale;
static CGColorRef display_bg;
static CGColorRef display_fg;

static FILE *external_file;

static int skin_type;
static int skin_width, skin_height;
static int skin_ncolors;
static const SkinColor *skin_colors = NULL;
static int skin_y;
static CGImageRef skin_image = NULL;
static unsigned char *skin_bitmap = NULL;
static int skin_bytesperline;
static unsigned char *disp_bitmap = NULL;
static int disp_bytesperline;

static keymap_entry *keymap = NULL;
static int keymap_length = 0;

static bool display_enabled = true;


/*****************/
/* Keymap parser */
/*****************/

keymap_entry *parse_keymap_entry(char *line, int lineno) {
    char *p;
    static keymap_entry entry;
    
    p = strchr(line, '#');
    if (p != NULL)
        *p = 0;
    p = strchr(line, '\n');
    if (p != NULL)
        *p = 0;
    p = strchr(line, '\r');
    if (p != NULL)
        *p = 0;
    
    p = strchr(line, ':');
    if (p != NULL) {
        char *val = p + 1;
        char *tok;
        bool ctrl = false;
        bool alt = false;
        bool shift = false;
        bool cshift = false;
        unsigned short keychar = 0;
        int done = 0;
        unsigned char macro[KEYMAP_MAX_MACRO_LENGTH + 1];
        int macrolen = 0;
        
        /* Parse keycode */
        *p = 0;
        tok = strtok(line, " \t");
        while (tok != NULL) {
            if (done) {
                NSLog(@"Keymap, line %d: Excess tokens in key spec.", lineno);
                return NULL;
            }
            if (strcasecmp(tok, "ctrl") == 0)
                ctrl = true;
            else if (strcasecmp(tok, "alt") == 0)
                alt = true;
            else if (strcasecmp(tok, "shift") == 0)
                shift = true;
            else if (strcasecmp(tok, "cshift") == 0)
                cshift = true;
            else {
                if (strlen(tok) == 1)
                    keychar = (unsigned char) *tok;
                else if (sscanf(tok, "0x%hx", &keychar) != 1) {
                    NSLog(@"Keymap, line %d: Bad keycode.", lineno);
                    return NULL;
                }
                done = 1;
            }
            tok = strtok(NULL, " \t");
        }
        if (!done) {
            NSLog(@"Keymap, line %d: Unrecognized keycode.", lineno);
            return NULL;
        }
        
        /* Parse macro */
        tok = strtok(val, " \t");
        while (tok != NULL) {
            char *endptr;
            long k = strtol(tok, &endptr, 10);
            if (*endptr != 0 || k < 1 || k > 255) {
                NSLog(@"Keymap, line %d: Bad value (%s) in macro.", lineno, tok);
                return NULL;
            } else if (macrolen == KEYMAP_MAX_MACRO_LENGTH) {
                NSLog(@"Keymap, line %d: Macro too long (max=%d).\n", lineno, KEYMAP_MAX_MACRO_LENGTH);
                return NULL;
            } else
                macro[macrolen++] = (unsigned char) k;
            tok = strtok(NULL, " \t");
        }
        macro[macrolen] = 0;
        
        entry.ctrl = ctrl;
        entry.alt = alt;
        entry.shift = shift;
        entry.cshift = cshift;
        entry.keychar = keychar;
        strcpy((char *) entry.macro, (const char *) macro);
        return &entry;
    } else
        return NULL;
}

static int case_insens_comparator(const void *a, const void *b) {
    return strcasecmp(*(const char **) a, *(const char **) b);
}

void skin_menu_update(NSMenu *skinMenu) {
    while ([skinMenu numberOfItems] > 3)
        [skinMenu removeItemAtIndex:3];
    
    DIR *dir = opendir(free42dirname);
    if (dir == NULL)
        return;
    
    struct dirent *dent;
    char *skinname[100];
    int nskins = 0;
    int have_separator = 0;
    
    while ((dent = readdir(dir)) != NULL && nskins < 100) {
        int namelen = strlen(dent->d_name);
        char *skn;
        if (namelen < 7)
            continue;
        namelen -= 7;
        if (strcasecmp(dent->d_name + namelen, ".layout") != 0)
            continue;
        skn = (char *) malloc(namelen + 1);
        // TODO - handle memory allocation failure
        memcpy(skn, dent->d_name, namelen);
        skn[namelen] = 0;
        skinname[nskins++] = skn;
    }
    closedir(dir);
    
    qsort(skinname, nskins, sizeof(char *), case_insens_comparator);
    
    char buf[1024];
    NSString *path = [[NSBundle mainBundle] pathForResource:@"builtin_skins" ofType:@"txt"];
    [path getCString:buf maxLength:1024 encoding:NSUTF8StringEncoding];
    FILE *builtins = fopen(buf, "r");
    while (fgets(buf, 1024, builtins) != NULL) {
        char *context;
        char *cname = strtok_r(buf, " \t\r\n", &context);
        NSString *name = [NSString stringWithUTF8String:cname];
        NSMenuItem *item = [skinMenu addItemWithTitle:name action: @selector(selectSkin:) keyEquivalent: @""];
        bool overridden = false;
        for (int i = 0; i < nskins; i++)
            if (strcasecmp(cname, skinname[i]) == 0) {
                overridden = true;
                break;
            }
        if (overridden)
            [item setEnabled:NO];
        else if (strcasecmp(cname, state.skinName) == 0)
            [item setState:NSOnState];
    }
    fclose(builtins);

    for (int i = 0; i < nskins; i++) {
        if (!have_separator) {
            [skinMenu addItem:[NSMenuItem separatorItem]];
            have_separator = 1;
        }
        NSString *name = [NSString stringWithUTF8String:skinname[i]];
        NSMenuItem *item = [skinMenu addItemWithTitle:name action: @selector(selectSkin:) keyEquivalent: @""];
        if (strcasecmp(skinname[i], state.skinName) == 0)
            [item setState:NSOnState];
        free(skinname[i]);
    }
}

static bool skin_open(const char *skinname, bool open_layout, bool force_builtin) {
    char buf[1024];
    
    if (!force_builtin) {
        /* Look for file */
        sprintf(buf, "%s/Library/Application Support/Free42/%s.%s", getenv("HOME"), skinname, open_layout ? "layout" : "gif");
        external_file = fopen(buf, "rb");
        if (external_file != NULL)
            return true;
    }

    /* Look for built-in skin */
    NSString *path = [[NSBundle mainBundle] pathForResource:@"builtin_skins" ofType:@"txt"];
    [path getCString:buf maxLength:1024 encoding:NSUTF8StringEncoding];
    FILE *builtins = fopen(buf, "r");
    while (fgets(buf, 1024, builtins) != NULL) {
        char *context;
        char *name = strtok_r(buf, " \t\r\n", &context);
        if (strcasecmp(skinname, name) == 0) {
            char *filename = strtok_r(NULL, " \t\r\n", &context);
            NSString *skinpath = [[NSBundle mainBundle] pathForResource:[NSString stringWithUTF8String:filename]
                                                        ofType:open_layout ? @"layout" : @"gif"];
            [skinpath getCString:buf maxLength:1024 encoding:NSUTF8StringEncoding];
            external_file = fopen(buf, "r");
            fclose(builtins);
            return true;
        }
    }
    fclose(builtins);
    
    /* Nothing found */
    return false;
}

int skin_getchar() {
    return fgetc(external_file);
}

static int skin_gets(char *buf, int buflen) {
    int p = 0;
    int eof = -1;
    int comment = 0;
    while (p < buflen - 1) {
        int c = skin_getchar();
        if (eof == -1)
            eof = c == EOF;
        if (c == EOF || c == '\n' || c == '\r')
            break;
        /* Remove comments */
        if (c == '#')
            comment = 1;
        if (comment)
            continue;
        /* Suppress leading spaces */
        if (p == 0 && isspace(c))
            continue;
        buf[p++] = c;
    }
    buf[p++] = 0;
    return p > 1 || !eof;
}

static void skin_close() {
    if (external_file != NULL)
        fclose(external_file);
}

void skin_load(long *width, long *height) {
    char line[1024];
    int size;
    int kmcap = 0;
    int lineno = 0;
    bool force_builtin = false;
    
    if (state.skinName[0] == 0) {
    fallback_on_1st_builtin_skin:
        force_builtin = true;
        NSString *path = [[NSBundle mainBundle] pathForResource:@"builtin_skins" ofType:@"txt"];
        [path getCString:line maxLength:1024 encoding:NSUTF8StringEncoding];
        FILE *builtins = fopen(line, "r");
        fgets(line, 1024, builtins);
        char *context;
        char *cname = strtok_r(line, " \t\r\n", &context);
        strcpy(state.skinName, cname);
        fclose(builtins);
    }
    
    /*************************/
    /* Load skin description */
    /*************************/
    
    if (!skin_open(state.skinName, true, force_builtin))
        goto fallback_on_1st_builtin_skin;
    
    if (keylist != NULL)
        free(keylist);
    keylist = NULL;
    nkeys = 0;
    keys_cap = 0;
    
    while (macrolist != NULL) {
        SkinMacro *m = macrolist->next;
        free(macrolist);
        macrolist = m;
    }
    
    if (keymap != NULL)
        free(keymap);
    keymap = NULL;
    keymap_length = 0;
    
    while (skin_gets(line, 1024)) {
        lineno++;
        if (*line == 0)
            continue;
        if (strncasecmp(line, "skin:", 5) == 0) {
            int x, y, width, height;
            if (sscanf(line + 5, " %d,%d,%d,%d", &x, &y, &width, &height) == 4){
                skin.x = x;
                skin.y = y;
                skin.width = width;
                skin.height = height;
            }
        } else if (strncasecmp(line, "display:", 8) == 0) {
            int x, y;
            double xscale, yscale;
            unsigned long bg, fg;
            if (sscanf(line + 8, " %d,%d %lf %lf %lx %lx", &x, &y,
                       &xscale, &yscale, &bg, &fg) == 6) {
                display_loc.x = x;
                display_loc.y = y;
                display_scale.x = (CGFloat) xscale;
                display_scale.y = (CGFloat) yscale;
                CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
                CGFloat comps[4];
                comps[0] = ((bg >> 16) & 255) / 255.0;
                comps[1] = ((bg >> 8) & 255) / 255.0;
                comps[2] = (bg & 255) / 255.0;
                comps[3] = 1.0;
                display_bg = CGColorCreate(color_space, comps);
                comps[0] = ((fg >> 16) & 255) / 255.0;
                comps[1] = ((fg >> 8) & 255) / 255.0;
                comps[2] = (fg & 255) / 255.0;
                display_fg = CGColorCreate(color_space, comps);
                CGColorSpaceRelease(color_space);
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
                    SkinKey *key;
                    if (nkeys == keys_cap) {
                        keys_cap += 50;
                        keylist = (SkinKey *) realloc(keylist, keys_cap * sizeof(SkinKey));
                        // TODO - handle memory allocation failure
                    }
                    key = keylist + nkeys;
                    key->code = keynum;
                    key->shifted_code = shifted_keynum;
                    key->sens_rect.x = sens_x;
                    key->sens_rect.y = sens_y;
                    key->sens_rect.width = sens_width;
                    key->sens_rect.height = sens_height;
                    key->disp_rect.x = disp_x;
                    key->disp_rect.y = disp_y;
                    key->disp_rect.width = disp_width;
                    key->disp_rect.height = disp_height;
                    key->src.x = act_x;
                    key->src.y = act_y;
                    nkeys++;
                }
            }
        } else if (strncasecmp(line, "macro:", 6) == 0) {
            char *quot1 = strchr(line + 6, '"');
            if (quot1 != NULL) {
                char *quot2 = strrchr(line + 6, '"');
                if (quot2 != quot1) {
                    long len = quot2 - quot1 - 1;
                    if (len > SKIN_MAX_MACRO_LENGTH)
                        len = SKIN_MAX_MACRO_LENGTH;
                    int n;
                    if (sscanf(line + 6, "%d", &n) == 1 && n >= 38 && n <= 255) {
                        SkinMacro *macro = (SkinMacro *) malloc(sizeof(SkinMacro));
                        // TODO - handle memory allocation failure
                        macro->code = n;
                        macro->isName = true;
                        memcpy(macro->macro, quot1 + 1, len);
                        macro->macro[len] = 0;
                        macro->next = macrolist;
                        macrolist = macro;
                    }
                }
            } else {
                char *tok = strtok(line + 6, " \t");
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
                        macro->isName = false;
                    } else if (len < SKIN_MAX_MACRO_LENGTH) {
                        if (n < 1 || n > 37) {
                            /* Key code out of range; ignore this macro */
                            free(macro);
                            macro = NULL;
                            break;
                        }
                        macro->macro[len++] = (unsigned char) n;
                    }
                    tok = strtok(NULL, " \t");
                }
                if (macro != NULL) {
                    macro->macro[len++] = 0;
                    macro->next = macrolist;
                    macrolist = macro;
                }
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
                    SkinAnnunciator *ann = annunciators + (annnum - 1);
                    ann->disp_rect.x = disp_x;
                    ann->disp_rect.y = disp_y;
                    ann->disp_rect.width = disp_width;
                    ann->disp_rect.height = disp_height;
                    ann->src.x = act_x;
                    ann->src.y = act_y;
                }
            }
        } else if (strncasecmp(line, "mackey:", 7) == 0) {
            keymap_entry *entry = parse_keymap_entry(line + 7, lineno);
            if (entry != NULL) {
                if (keymap_length == kmcap) {
                    kmcap += 50;
                    keymap = (keymap_entry *) realloc(keymap, kmcap * sizeof(keymap_entry));
                    // TODO - handle memory allocation failure
                }
                memcpy(keymap + (keymap_length++), entry, sizeof(keymap_entry));
            }
        }
    }
    
    /* Fix coordinates to match the upward-increasing Y coordinates used by Quartz 2D */
    display_loc.y = skin.height - (display_loc.y + 16 * display_scale.y);
    for (int i = 0; i < 7; i++) {
        SkinAnnunciator *ann = annunciators + i;
        ann->disp_rect.y = skin.height - (ann->disp_rect.y + ann->disp_rect.height);
    }
    for (int i = 0; i < nkeys; i++) {
        SkinKey *key = keylist + i;
        key->sens_rect.y = skin.height - (key->sens_rect.y + key->sens_rect.height);
        key->disp_rect.y = skin.height - (key->disp_rect.y + key->disp_rect.height);
    }
    
    skin_close();
    
    /********************/
    /* Load skin bitmap */
    /********************/
    
    if (!skin_open(state.skinName, false, force_builtin))
        goto fallback_on_1st_builtin_skin;
    
    /* shell_loadimage() calls skin_getchar() to load the image from the
     * compiled-in or on-disk file; it calls skin_init_image(),
     * skin_put_pixels(), and skin_finish_image() to create the in-memory
     * representation.
     */
    bool success = shell_loadimage();
    skin_close();
    
    if (!success)
        goto fallback_on_1st_builtin_skin;
    
    *width = skin.width;
    *height = skin.height;
    
    /********************************/
    /* (Re)build the display bitmap */
    /********************************/
    
    if (disp_bitmap != NULL)
        free(disp_bitmap);
    disp_bytesperline = 17; // bytes needed for 131 pixels
    size = disp_bytesperline * 16;
    disp_bitmap = (unsigned char *) malloc(size);
    // TODO - handle memory allocation failure
    memset(disp_bitmap, 255, size);
}

int skin_init_image(int type, int ncolors, const SkinColor *colors,
                    int width, int height) {
    if (skin_image != NULL) {
        CGImageRelease(skin_image);
        skin_image = NULL;
        skin_bitmap = NULL;
    }
    
    skin_type = type;
    skin_ncolors = ncolors;
    skin_colors = colors;
    
    switch (type) {
        case IMGTYPE_MONO:
            skin_bytesperline = (width + 7) >> 3;
            break;
        case IMGTYPE_GRAY:
            skin_bytesperline = width;
            break;
        case IMGTYPE_TRUECOLOR:
        case IMGTYPE_COLORMAPPED:
            skin_bytesperline = width * 3;
            break;
        default:
            return 0;
    }
    
    skin_bitmap = (unsigned char *) malloc(skin_bytesperline * height);
    // TODO - handle memory allocation failure
    skin_width = width;
    skin_height = height;
    skin_y = 0;
    return skin_bitmap != NULL;
}

static const char bit_flipper[] = { 0x0, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0, 0x8, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 0x4, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4, 0xc, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc, 0x2, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2, 0xa, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa, 0x6, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6, 0xe, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 0x1, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1, 0x9, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9, 0x5, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 0xd, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd, 0x3, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3, 0xb, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb, 0x7, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7, 0xf, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff };

void skin_put_pixels(unsigned const char *data) {
    unsigned char *dst = skin_bitmap + skin_y * skin_bytesperline;
    if (skin_type == IMGTYPE_MONO) {            
        for (int i = 0; i < skin_bytesperline; i++)
            dst[i] = bit_flipper[data[i]];
    } else if (skin_type == IMGTYPE_COLORMAPPED) {
        int src_bytesperline = skin_bytesperline / 3;
        for (int i = 0; i < src_bytesperline; i++) {
            int index = data[i] & 255;
            const SkinColor *c = skin_colors + index;
            *dst++ = c->r;
            *dst++ = c->g;
            *dst++ = c->b;
        }
    } else
        memcpy(dst, data, skin_bytesperline);
    skin_y++;
}

static void MyProviderReleaseData(void *info,  const void *data, size_t size) {
    free((void *) data);
}

void skin_finish_image() {
    int bits_per_component;
    int bits_per_pixel;
    CGColorSpaceRef color_space;
    
    switch (skin_type) {
        case IMGTYPE_MONO:
            bits_per_component = 1;
            bits_per_pixel = 1;
            color_space = CGColorSpaceCreateDeviceGray();
            break;
        case IMGTYPE_GRAY:
            bits_per_component = 8;
            bits_per_pixel = 8;
            color_space = CGColorSpaceCreateDeviceGray();
            break;
        case IMGTYPE_COLORMAPPED:
        case IMGTYPE_TRUECOLOR:
            bits_per_component = 8;
            bits_per_pixel = 24;
            color_space = CGColorSpaceCreateDeviceRGB();
            break;
    }
    
    int bytes_per_line = (skin_width * bits_per_pixel + 7) >> 3;
    
    CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, skin_bitmap, bytes_per_line * skin_height, MyProviderReleaseData);
    skin_image = CGImageCreate(skin_width, skin_height, bits_per_component, bits_per_pixel, bytes_per_line,
                               color_space, kCGBitmapByteOrder32Big, provider, NULL, false, kCGRenderingIntentDefault);
    CGDataProviderRelease(provider);
    CGColorSpaceRelease(color_space);
    skin_bitmap = NULL;
}

void skin_repaint(NSRect *rect) {
    CGContextRef myContext = (CGContextRef) [[NSGraphicsContext currentContext] graphicsPort];
    
    // Optimize for the common case that *only* the display needs painting
    bool paintOnlyDisplay = rect->origin.x >= display_loc.x && rect->origin.y >= display_loc.y
            && rect->origin.x + rect->size.width <= display_loc.x + display_scale.x * 131
            && rect->origin.y + rect->size.height <= display_loc.y + display_scale.y * 16;
    
    if (!paintOnlyDisplay) {
        // Redisplay skin
        CGImageRef si = CGImageCreateWithImageInRect(skin_image, CGRectMake(skin.x, skin.y, skin.width, skin.height));
        CGContextDrawImage(myContext, CGRectMake(0, 0, skin.width, skin.height), si);
        CGImageRelease(si);
        
        // Repaint pressed hard key, if any
        if (currently_pressed_key >= 0 && currently_pressed_key < nkeys) {
            SkinKey *k = keylist + currently_pressed_key;
            CGImageRef key_image = CGImageCreateWithImageInRect(skin_image, CGRectMake(k->src.x, k->src.y, k->disp_rect.width, k->disp_rect.height));
            CGContextDrawImage(myContext, CGRectMake(k->disp_rect.x, k->disp_rect.y, k->disp_rect.width, k->disp_rect.height), key_image);
            CGImageRelease(key_image);
        }
    }
    
    // Repaint display (and pressed softkey, if any)
    CGContextSaveGState(myContext);
    CGContextTranslateCTM(myContext, display_loc.x, display_loc.y);
    CGContextScaleCTM(myContext, display_scale.x, display_scale.y);
    
    int x1 = (int) ((rect->origin.x - display_loc.x) / display_scale.x);
    int y1 = (int) ((rect->origin.y - display_loc.y) / display_scale.y);
    int x2 = (int) ceil((rect->origin.x + rect->size.width - display_loc.x) / display_scale.x);
    int y2 = (int) ceil((rect->origin.y + rect->size.height - display_loc.y) / display_scale.y);
    if (x1 < 0)
        x1 = 0;
    else if (x1 > 131)
        x1 = 131;
    if (y1 < 0)
        y1 = 0;
    else if (y1 > 16)
        y1 = 16;
    if (x2 < x1)
        x2 = x1;
    else if (x2 > 131)
        x2 = 131;
    if (y2 < y1)
        y2 = y1;
    else if (y2 > 16)
        y2 = 16;
    
    if (x2 > x1 && y2 > y1) {
        CGContextSetFillColorWithColor(myContext, display_bg);
        CGContextFillRect(myContext, CGRectMake(x1, y1, x2 - x1, y2 - y1));
        CGContextSetFillColorWithColor(myContext, display_fg);
        bool softkey_pressed = currently_pressed_key >= -7 && currently_pressed_key <= -2;
        int skx1, skx2, sky1, sky2;
        if (softkey_pressed) {
            skx1 = (-2 - currently_pressed_key) * 22;
            skx2 = skx1 + 21;
            sky1 = 0;
            sky2 = 7;
        }
        for (int v = y1; v < y2; v++) {
            for (int h = x1; h < x2; h++) {
                int pixel = (disp_bitmap[v * disp_bytesperline + (h >> 3)] & (128 >> (h & 7))) != 0;
                if (softkey_pressed && h >= skx1 && h < skx2 && v >= sky1 && v < sky2)
                    pixel = !pixel;
                if (pixel)
                    CGContextFillRect(myContext, CGRectMake(h, v, 1, 1));
            }
        }
    }
    
    CGContextRestoreGState(myContext);  
    
    if (!paintOnlyDisplay) {
        // Repaint annunciators
        for (int i = 0; i < 7; i++) {
            if (annunciator_state[i]) {
                SkinAnnunciator *ann = annunciators + i;
                CGImageRef ann_image = CGImageCreateWithImageInRect(skin_image, CGRectMake(ann->src.x, ann->src.y, ann->disp_rect.width, ann->disp_rect.height));
                CGContextDrawImage(myContext, CGRectMake(ann->disp_rect.x, ann->disp_rect.y, ann->disp_rect.width, ann->disp_rect.height), ann_image);
                CGImageRelease(ann_image);
            }
        }
    }
}

void skin_update_annunciator(int which, int state) {
    if (which < 1 || which > 7)
        return;
    which--;
    if (annunciator_state[which] == state)
        return;
    annunciator_state[which] = state;
    SkinRect *r = &annunciators[which].disp_rect;
    Free42AppDelegate *delegate = (Free42AppDelegate *) [NSApp delegate];
    [delegate.calcView setNeedsDisplayInRect:CGRectMake(r->x, r->y, r->width, r->height)];
}

void skin_find_key(int x, int y, bool cshift, int *skey, int *ckey) {
    int i;
    if (core_menu()
            && x >= display_loc.x
            && x < display_loc.x + 131 * display_scale.x
            && y >= display_loc.y
            && y < display_loc.y + 8 * display_scale.y) {
        int softkey = (x - display_loc.x) / (22 * display_scale.x) + 1;
        *skey = -1 - softkey;
        *ckey = softkey;
        return;
    }
    for (i = 0; i < nkeys; i++) {
        SkinKey *k = keylist + i;
        int rx = x - k->sens_rect.x;
        int ry = y - k->sens_rect.y;
        if (rx >= 0 && rx < k->sens_rect.width
                && ry >= 0 && ry < k->sens_rect.height) {
            *skey = i;
            *ckey = cshift ? k->shifted_code : k->code;
            return;
        }
    }
    *skey = -1;
    *ckey = 0;
}

int skin_find_skey(int ckey) {
    int i;
    for (i = 0; i < nkeys; i++)
        if (keylist[i].code == ckey || keylist[i].shifted_code == ckey)
            return i;
    return -1;
}

unsigned char *skin_find_macro(int ckey, bool *is_name) {
    SkinMacro *m = macrolist;
    while (m != NULL) {
        if (m->code == ckey) {
            *is_name = m->isName;
            return m->macro;
        }
        m = m->next;
    }
    return NULL;
}

unsigned char *skin_keymap_lookup(unsigned short keychar, bool printable,
                                  bool ctrl, bool alt, bool shift, bool cshift,
                                  bool *exact) {
    int i;
    unsigned char *macro = NULL;
    for (i = 0; i < keymap_length; i++) {
        keymap_entry *entry = keymap + i;
        if (ctrl == entry->ctrl
            && alt == entry->alt
            && (printable || shift == entry->shift)
            && keychar == entry->keychar) {
            if (cshift == entry->cshift) {
                *exact = true;
                return entry->macro;
            }
            if (cshift)
                macro = entry->macro;
        }
    }
    *exact = false;
    return macro;
}

static void invalidate_key(int key) {
    if (key == -1)
        return;
    if (key >= -7 && key <= -2) {
        int k = -1 - key;
        int x = (k - 1) * 22 * display_scale.x + display_loc.x;
        int y = display_loc.y;
        int w = 21 * display_scale.x;
        int h = 7 * display_scale.y;
        Free42AppDelegate *delegate = (Free42AppDelegate *) [NSApp delegate];
        [delegate.calcView setNeedsDisplayInRect:CGRectMake(x, y, w, h)];
    } else if (key >= 0 && key < nkeys) {
        SkinRect *r = &keylist[key].disp_rect;
        Free42AppDelegate *delegate = (Free42AppDelegate *) [NSApp delegate];
        [delegate.calcView setNeedsDisplayInRect:CGRectMake(r->x, r->y, r->width, r->height)];
    }
}

void skin_set_pressed_key(int key) {
    if (key == currently_pressed_key)
        return;
    invalidate_key(currently_pressed_key);
    currently_pressed_key = key;
    invalidate_key(currently_pressed_key);
}

void skin_display_blitter(const char *bits, int bytesperline, int x, int y, int width, int height) {
    int h, v;
    
    for (v = y; v < y + height; v++)
        for (h = x; h < x + width; h++) {
            int pixel = (bits[v * bytesperline + (h >> 3)] & (1 << (h & 7))) == 0;
            if (pixel)
                disp_bitmap[(15 - v) * disp_bytesperline + (h >> 3)] &= ~(128 >> (h & 7));
            else
                disp_bitmap[(15 - v) * disp_bytesperline + (h >> 3)] |= 128 >> (h & 7);
        }
    
    Free42AppDelegate *delegate = (Free42AppDelegate *) [NSApp delegate];
    [delegate.calcView setNeedsDisplayInRect:CGRectMake(display_loc.x + x * display_scale.x,
                                                              display_loc.y + (16 - y - height) * display_scale.y,
                                                              width * display_scale.x,
                                                              height * display_scale.y)];
}

void skin_repaint_display() {
    if (!display_enabled)
        // Prevent screen flashing during macro execution
        return;
    Free42AppDelegate *delegate = (Free42AppDelegate *) [NSApp delegate];
    [delegate.calcView setNeedsDisplayInRect:CGRectMake(display_loc.x, display_loc.y, 131 * display_scale.x, 16 * display_scale.y)];
}

void skin_display_set_enabled(bool enable) {
    display_enabled = enable;
}
