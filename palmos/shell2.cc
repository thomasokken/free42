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

#include "shell.h"
#include "shell2.h"
#include "shell_spool.h"
#include "myglue.h"
#include "filesys.h"
#include "core_main.h"
#include "core_display.h"
#include "shell.rcp.h"
#include "skin2prc.h"


state_type state;

BitmapType *disp_bitmap = NULL;
char *disp_bits_v3 = NULL;
BitmapType *print_bitmap = NULL;
int print_gadget_height;
int want_to_run = 0;
Int32 timeout3time = -1;
char *printout = NULL;
int4 printout_top;
int4 printout_bottom;
int4 printout_pos;
int can_draw = 0;
int softkey = 0;
bool display_enabled = true;

int updownAnn = 0;
int shiftAnn = 0;
int printAnn = 0;
int runAnn = 0;
int batteryAnn = 0;
int gAnn = 0;
int radAnn = 0;

static DmOpenRef skin_db = 0;
static MemHandle skinH = NULL;
SkinSpec *skin = NULL;
int skin_version;
SkinSpec2 *skin2;
SkinSpec2_V2 *skin2_v2;
static bool skin_tall;
static MemHandle *skin_bm_h;
static BitmapType **skin_bm_p;
static MemHandle *key_up_h;
static BitmapType **key_up_p;
static MemHandle *key_down_h;
static BitmapType **key_down_p;
static MemHandle ann_off_h[7];
static BitmapType *ann_off_p[7];
static MemHandle ann_on_h[7];
static BitmapType *ann_on_p[7];

static RGBColorType savedForeRGB;
static RGBColorType savedBackRGB;
static IndexedColorType savedForeIndex;
static IndexedColorType savedBackIndex;
static UInt16 savedCoordSys;

static int builtin_skins;
static int external_skins;
static char **skin_name = NULL;
 
static void do_import() SHELL2_SECT;
static void do_export() SHELL2_SECT;
static void do_clip_copy() SHELL2_SECT;
static void do_clip_paste() SHELL2_SECT;
static void do_delete_skin() SHELL2_SECT;
static void do_copy() SHELL2_SECT;
static void do_delete() SHELL2_SECT;
static void do_removedir() SHELL2_SECT;

#ifdef PALMOS_ARM_SHELL
void get_core_settings();
void put_core_settings();
#endif


#if !defined(BCD_MATH) && !defined(PALMOS_ARM_SHELL)
void open_math_lib() {
    Err error;

    /* TODO: better error handling */
    error = SysLibFind(MathLibName, &MathLibRef);
    if (error)
	error = SysLibLoad(LibType, MathLibCreator, &MathLibRef);
    ErrFatalDisplayIf(error, "Can't find MathLib");
    error = MathLibOpen(MathLibRef, MathLibVersion);
    ErrFatalDisplayIf(error, "Can't open MathLib");
}
#endif

static Int16 str_ptr_compare(void *a, void *b, Int32 other) SHELL2_SECT;
static Int16 str_ptr_compare(void *a, void *b, Int32 other) {
    return StrCaselessCompare(*(char **) a, *(char **) b);
}

static void make_skin_list() SHELL2_SECT;
static void make_skin_list() {
    int bufsize = 5000;
    char *buf = (char *) MemPtrNew(bufsize);
    char *p = buf;
    int len = 0;
    UInt16 id = 100;
    MemHandle h;
    DmSearchStateType search_state;
    Boolean new_search = true;
    UInt16 card;
    LocalID dbid;
    int i, n;

    bool allow_hd = false;
    // As of 1.4.49, I'm allowing tall skins regardless of the shape
    // of the physical screen. The condition that height should be greater
    // than width has the effect of suppressing tall skins on landscape
    // screens, i.e., on the Tapwave Zodiac.
    // Since the code that manages the dynamic input area is protected by
    // an API feature check, nothing bad should happen if someone were to
    // try to use a tall skin on a square screen; it won't be usable but
    // it shouldn't crash.
    bool allow_tall = true;
    if (feature_set_high_density_present()) {
	UInt32 attr;
	WinScreenGetAttribute(winScreenDensity, &attr);
	allow_hd = attr == kDensityDouble;
	//UInt32 sw, sh;
	//WinScreenGetAttribute(winScreenWidth, &sw);
	//WinScreenGetAttribute(winScreenHeight, &sh);
	//allow_tall = sh > sw;
    }

    if (buf == NULL)
	ErrFatalDisplayIf(1, "Out of memory while building skin list.");

    builtin_skins = 0;
    external_skins = 0;

    while ((h = DmGetResource('Skin', id)) != NULL) {
	SkinSpec *ss = (SkinSpec *) MemHandleLock(h);
	int namelen;
	bool tall;
	if (ss->version[0] != 0) {
	    /* Old-style Skin resource, w/o version; offset 4 bytes */
	    ss = (SkinSpec *) (((char *) ss) - 4);
	    tall = false;
	} else if (ss->version[1] > 3)
	    /* We only know about skin versions up to 3 */
	    goto done;
	else
	    tall = ss->version[1] >= 2 && ss->version[3] != 0;
	namelen = StrLen(ss->name) + 1;
	if (len + namelen <= bufsize) {
	    if (!allow_hd && ss->density != 72)
		goto done;
	    if (!allow_tall && tall)
		goto done;
	    StrCopy(p, ss->name);
	    p += namelen;
	    len += namelen;
	    builtin_skins++;
	}
	done:
	MemHandleUnlock(h);
	DmReleaseResource(h);
	id++;
    }

    while (DmGetNextDatabaseByTypeCreator(new_search, &search_state,
		'Skin', 'Fk42', false, &card, &dbid) == errNone) {
	char name[32];
	new_search = false;
	if (DmDatabaseInfo(card, dbid, name, NULL, NULL, NULL, NULL,
		    NULL, NULL, NULL, NULL, NULL, NULL) == errNone) {
	    int namelen = StrLen(name) + 1;
	    if (len + namelen <= bufsize) {
		if (!allow_hd || !allow_tall) {
		    DmOpenRef db;
		    MemHandle h;
		    SkinSpec *ss;
		    int high_density;
		    db = DmOpenDatabase(card, dbid, dmModeReadOnly);
		    if (db == 0)
			continue;
		    h = DmGet1Resource('Skin', 1000);
		    if (h == NULL) {
			DmCloseDatabase(db);
			continue;
		    }
		    ss = (SkinSpec *) MemHandleLock(h);
		    bool tall;
		    if (ss->version[0] != 0) {
			/* Old-style Skin resource, w/o version; offset 4 bytes */
			ss = (SkinSpec *) (((char *) ss) - 4);
			tall = false;
		    } else
			tall = ss->version[1] >= 2 && ss->version[3] != 0;
		    high_density = ss->density != 72;
		    MemHandleUnlock(h);
		    DmReleaseResource(h);
		    DmCloseDatabase(db);
		    if (high_density && !allow_hd || tall && !allow_tall)
			continue;
		}
		StrCopy(p, name);
		p += namelen;
		len += namelen;
		external_skins++;
	    }
	}
    }

    n = builtin_skins + external_skins;
    skin_name = (char **) MemPtrNew(len + n * sizeof(char *));
    if (skin_name == NULL)
	ErrFatalDisplayIf(1, "Out of memory while building skin list.");
    MemMove(skin_name + n, buf, len);
    p = (char *) (skin_name + n);
    for (i = 0; i < n; i++) {
	skin_name[i] = p;
	p += StrLen(p) + 1;
    }
    SysQSort(skin_name + builtin_skins, external_skins,
		sizeof(char *), str_ptr_compare, 0);
    MemPtrFree(buf);
}

static unsigned char *find_macro(int ckey) SHELL2_SECT;
static unsigned char *find_macro(int ckey) {
    int i;
    if (skin2 != NULL) {
	for (i = 0; i < skin2->nmacros; i++)
	    if (skin2->macro[i].code == ckey)
		return skin2->macro[i].macro;
    } else if (skin2_v2 != NULL) {
	for (i = 0; i < skin2_v2->nmacros; i++)
	    if (skin2_v2->macro[i].code == ckey)
		return skin2_v2->macro[i].macro;
    }
    return NULL;
}

static void update_dia_state() SHELL2_SECT;
static void update_dia_state() {
    if (pen_input_manager_present() && FrmGetActiveFormID() == calcform_id) {
	if (skin_tall) {
	    PINSetInputTriggerState(pinInputTriggerEnabled);
	    PINSetInputAreaState(pinInputAreaUser);
	} else {
	    PINSetInputTriggerState(pinInputTriggerDisabled);
	    PINSetInputAreaState(pinInputAreaOpen);
	}
    }
}

void load_skin() {
    int i, n;
    Err err;
    UInt16 id;

    if (skin_name == NULL)
	make_skin_list();
    if (state.skinName[0] == 0) {
	fallback_on_1st_builtin_skin:
	StrCopy(state.skinName, skin_name[0]);
    }

    n = builtin_skins + external_skins;
    for (i = 0; i < n; i++) {
	if (StrCaselessCompare(state.skinName, skin_name[i]) == 0)
	    goto found;
    }
    goto fallback_on_1st_builtin_skin;
    found:

    if (i < builtin_skins) {
	skinH = DmGetResource('Skin', 100 + i);
	if (skinH == NULL) {
	    if (i == 0)
		ErrFatalDisplayIf(1, "Can't load skin layout");
	    else
		goto fallback_on_1st_builtin_skin;
	}
	skin = (SkinSpec *) MemHandleLock(skinH);
	skin_db = 0;
    } else {
	LocalID dbid = DmFindDatabase(0, state.skinName);
	if (dbid == 0)
	    goto fallback_on_1st_builtin_skin;
	skin_db = DmOpenDatabase(0, dbid, dmModeReadOnly);
	if (skin_db == 0)
	    goto fallback_on_1st_builtin_skin;
	skinH = DmGetResource('Skin', 1000);
	if (skinH == NULL) {
	    DmCloseDatabase(skin_db);
	    skin_db = 0;
	    goto fallback_on_1st_builtin_skin;
	}
	skin = (SkinSpec *) MemHandleLock(skinH);
    }

    if (skin->version[0] != 0) {
	/* Old-style Skin resource, w/o version; offset 4 bytes */
	skin_version = 0;
	skin = (SkinSpec *) (((char *) skin) - 4);
	skin2 = NULL;
	skin2_v2 = NULL;
	skin_tall = false;
    } else {
	skin_version = skin->version[1];
	if (skin_version < 3) {
	    skin2 = NULL;
	    skin2_v2 = (SkinSpec2_V2 *) (skin->key + skin->nkeys);
	} else {
	    skin2 = (SkinSpec2 *) (skin->key + skin->nkeys);
	    skin2_v2 = NULL;
	}
	skin_tall = skin_version >= 2 && skin->version[3] != 0;
    }

    n = skin->sections;
    id = skin->skin_bitmap;
    skin_bm_h = (MemHandle *) MemPtrNew(n * sizeof(MemHandle));
    skin_bm_p = (BitmapType **) MemPtrNew(n * sizeof(BitmapType *));

    for (i = 0; i < n; i++) {
	skin_bm_h[i] = DmGetResource(bitmapRsc, id++);
	if (skin_bm_h[i] == NULL)
	    ErrFatalDisplayIf(1, "Can't load skin bitmap");
	skin_bm_p[i] = (BitmapType *) MemHandleLock(skin_bm_h[i]);
    }

    n = skin->nkeys;
    key_up_h = (MemHandle *) MemPtrNew(n * sizeof(MemHandle));
    key_up_p = (BitmapType **) MemPtrNew(n * sizeof(BitmapType *));
    key_down_h = (MemHandle *) MemPtrNew(n * sizeof(MemHandle));
    key_down_p = (BitmapType **) MemPtrNew(n * sizeof(BitmapType *));

    for (i = 0; i < n; i++) {
	MemHandle h = DmGetResource(bitmapRsc, skin->key[i].up_bitmap);
	if (h == NULL)
	    ErrFatalDisplayIf(1, "Can't load key bitmap");
	key_up_h[i] = h;
	key_up_p[i] = (BitmapType *) MemHandleLock(h);
	h = DmGetResource(bitmapRsc, skin->key[i].down_bitmap);
	if (h == NULL)
	    ErrFatalDisplayIf(1, "Can't load key bitmap");
	key_down_h[i] = h;
	key_down_p[i] = (BitmapType *) MemHandleLock(h);
    }

    for (i = 0; i < 7; i++) {
	MemHandle h = DmGetResource(bitmapRsc, skin->annunciator[i].off_bitmap);
	if (h == NULL)
	    ErrFatalDisplayIf(1, "Can't load annunciator bitmap");
	ann_off_h[i] = h;
	ann_off_p[i] = (BitmapType *) MemHandleLock(h);
	h = DmGetResource(bitmapRsc, skin->annunciator[i].on_bitmap);
	if (h == NULL)
	    ErrFatalDisplayIf(1, "Can't load annunciator bitmap");
	ann_on_h[i] = h;
	ann_on_p[i] = (BitmapType *) MemHandleLock(h);
    }

    if (disp_bitmap != NULL) {
	MyGlueBmpDelete(disp_bitmap);
	if (disp_bits_v3 != NULL)
	    MemPtrFree(disp_bits_v3);
    }

    disp_bitmap = MyGlueBmpCreate(131 * skin->display_xscale,
				  16 * skin->display_yscale,
				  1, NULL, &err);
    disp_bits_v3 = NULL;

    if (skin->density != 72) {
	UInt32 size = BmpBitsSize(disp_bitmap);
	disp_bits_v3 = (char *) MemPtrNew(size);
	if (disp_bits_v3 != NULL) {
	    BitmapType *bmp_v3 = (BitmapType *) BmpCreateBitmapV3(disp_bitmap,
						kDensityDouble, disp_bits_v3,
						NULL);
	    if (bmp_v3 == NULL) {
		MemPtrFree(disp_bits_v3);
		disp_bits_v3 = NULL;
	    } else {
		char *oldbits = (char *) BmpGetBits(disp_bitmap);
		MemMove(disp_bits_v3, oldbits, size);
		BmpDelete(disp_bitmap);
		disp_bitmap = bmp_v3;
	    }
	}
    }
}

void unload_skin() {
    int i;

    /* Unload keys */
    for (i = 0; i < skin->nkeys; i++) {
	MemHandleUnlock(key_up_h[i]);
	DmReleaseResource(key_up_h[i]);
	MemHandleUnlock(key_down_h[i]);
	DmReleaseResource(key_down_h[i]);
    }
    MemPtrFree(key_up_h);
    MemPtrFree(key_up_p);
    MemPtrFree(key_down_h);
    MemPtrFree(key_down_p);

    /* Unload annunciators */
    for (i = 0; i < 7; i++) {
	MemHandleUnlock(ann_off_h[i]);
	DmReleaseResource(ann_off_h[i]);
	MemHandleUnlock(ann_on_h[i]);
	DmReleaseResource(ann_on_h[i]);
    }

    /* Unload faceplate */
    for (i= 0; i < skin->sections; i++) {
	MemHandleUnlock(skin_bm_h[i]);
	DmReleaseResource(skin_bm_h[i]);
    }
    MemPtrFree(skin_bm_h);
    MemPtrFree(skin_bm_p);

    /* Unload layout */
    MemHandleUnlock(skinH);
    DmReleaseResource(skinH);

    /* Close database (if external skin) */
    if (skin_db != 0) {
	DmCloseDatabase(skin_db);
	skin_db = 0;
    }
}

#if !defined(BCD_MATH) && !defined(PALMOS_ARM_SHELL)
void close_math_lib() {
    UInt16 usecount;
    Err error;

    error = MathLibClose(MathLibRef, &usecount);
    ErrFatalDisplayIf(error, "Can't close MathLib");
    if (usecount == 0)
	SysLibRemove(MathLibRef);
}
#endif

void misc_cleanup() {
    if (skin_name != NULL)
	MemPtrFree(skin_name);
}

bool feature_set_3_5_present() {
    static int three_five_present = -1;
    if (three_five_present == -1) {
	UInt32 neededRom = sysMakeROMVersion(3, 5, 0, sysROMStageDevelopment, 0);
	UInt32 presentRom;
	Err err = FtrGet(sysFtrCreator, sysFtrNumROMVersion, &presentRom);
	three_five_present = err == errNone && presentRom >= neededRom;
    }
    return three_five_present != 0;
}

bool feature_set_4_0_present() {
    static int four_oh_present = -1;
    if (four_oh_present == -1) {
	UInt32 neededRom = sysMakeROMVersion(4, 0, 0, sysROMStageDevelopment, 0);
	UInt32 presentRom;
	Err err = FtrGet(sysFtrCreator, sysFtrNumROMVersion, &presentRom);
	four_oh_present = err == errNone && presentRom >= neededRom;
    }
    return four_oh_present != 0;
}

bool feature_set_high_density_present() {
    static int hd_present = -1;
    if (hd_present == -1) {
	UInt32 version;
	Err err = FtrGet(sysFtrCreator, sysFtrNumWinVersion, &version);
	hd_present = err == errNone && version >= 4;
    }
    return hd_present != 0;
}

bool pen_input_manager_present() {
    static int pim_present = -1;
    if (pim_present == -1) {
	UInt32 version;
	Err err = FtrGet(pinCreator, pinFtrAPIVersion, &version);
	pim_present = err == errNone && version > 0;
    }
    return pim_present != 0;
}

void init_shell_state(int4 version) {
    switch (version) {
	case -1:
	    state.extras = 0;
	    /* fall through */
	case 0:
	    state.printerToMemo = 0;
	    state.printerToTxtFile = 0;
	    state.printerToGifFile = 0;
	    state.printerMemoName[0] = 0;
	    state.printerTxtFileName[0] = 0;
	    state.printerGifFileName[0] = 0;
	    state.printerGifMaxLength = 256;
	    /* fall through */
	case 1:
	    state.soundVolume = sndMaxAmp / 2;
	    /* fall through */
	case 2:
	    state.skinName[0] = 0;
	    /* fall through */
	case 3:
	    /* current version (SHELL_VERSION = 3),
	     * so nothing to do here since everything
	     * was initialized from the state file.
	     */
	    ;
    }
}

int read_shell_state(int4 *ver) {
    int4 magic;
    int4 version;
    int4 state_size;
    int4 state_version;

    if (shell_read_saved_state(&magic, sizeof(int4)) != sizeof(int4))
	return 0;
    if (magic != FREE42_MAGIC)
	return 0;

    if (shell_read_saved_state(&version, sizeof(int4)) != sizeof(int4))
	return 0;
    if (version == 0) {
	/* State file version 0 does not contain shell state,
	 * only core state, so we just hard-init the shell.
	 */
	init_shell_state(-1);
	*ver = version;
	return 1;
    } else if (version > FREE42_VERSION)
	/* Unknown state file version */
	return 0;

    if (shell_read_saved_state(&state_size, sizeof(int4)) != sizeof(int4))
	return 0;
    if (shell_read_saved_state(&state_version, sizeof(int4)) != sizeof(int4))
	return 0;
    if (state_version < 0 || state_version > SHELL_VERSION)
	/* Unknown shell state version */
	return 0;
    if (shell_read_saved_state(&state, state_size) != state_size)
	return 0;

    init_shell_state(state_version);
    *ver = version;
    return 1;
}

int write_shell_state() {
    int4 magic = FREE42_MAGIC;
    int4 version = FREE42_VERSION;
    int4 state_size = sizeof(state_type);
    int4 state_version = SHELL_VERSION;

    if (!shell_write_saved_state(&magic, sizeof(int4)))
	return 0;
    if (!shell_write_saved_state(&version, sizeof(int4)))
	return 0;
    if (!shell_write_saved_state(&state_size, sizeof(int4)))
	return 0;
    if (!shell_write_saved_state(&state_version, sizeof(int4)))
	return 0;
    if (!shell_write_saved_state(&state, sizeof(state_type)))
	return 0;

    return 1;
}

void set_colors(RGBColorType *bg, RGBColorType *fg) {
    if (feature_set_4_0_present()) {
	WinSetBackColorRGB(bg, &savedBackRGB);
	WinSetForeColorRGB(fg, &savedForeRGB);
    } else if (feature_set_3_5_present()) {
	IndexedColorType idx = WinRGBToIndex(bg);
	savedBackIndex = WinSetBackColor(idx);
	idx = WinRGBToIndex(fg);
	savedForeIndex = WinSetForeColor(idx);
    }
}

void set_default_colors() {
    if (feature_set_3_5_present()) {
	RGBColorType bg, fg;
	UIColorGetTableEntryRGB(UIFormFill, &bg);
	UIColorGetTableEntryRGB(UIFormFrame, &fg);
	set_colors(&bg, &fg);
    }
}

void restore_colors() {
    if (feature_set_4_0_present()) {
	WinSetBackColorRGB(&savedBackRGB, NULL);
	WinSetForeColorRGB(&savedForeRGB, NULL);
    } else if (feature_set_3_5_present()) {
	WinSetBackColor(savedBackIndex);
	WinSetForeColor(savedForeIndex);
    }
}

void set_coord_sys() {
    if (skin->density != 72)
	savedCoordSys = WinSetCoordinateSystem(kCoordinatesDouble);
}

void restore_coord_sys() {
    if (skin->density != 72)
	WinSetCoordinateSystem(savedCoordSys);
}

void repaint_annunciator(int i) {
    if (!display_enabled)
	return;
    int state;
    switch (i) {
	case 0: state = updownAnn; break;
	case 1: state = shiftAnn; break;
	case 2: state = printAnn; break;
	case 3: state = runAnn; break;
	case 4: state = batteryAnn; break;
	case 5: state = gAnn; break;
	case 6: state = radAnn; break;
	default: return;
    }
    set_default_colors();
    set_coord_sys();
    if (state)
	WinDrawBitmap(ann_on_p[i], skin->annunciator[i].x,
				   skin->annunciator[i].y);
    else
	WinDrawBitmap(ann_off_p[i], skin->annunciator[i].x,
				    skin->annunciator[i].y);
    restore_colors();
    restore_coord_sys();
}

void repaint_printout() {
    int4 length, x, y;
    char *mybits;
    RGBColorType black, white;

    if (FrmGetActiveFormID() != printform_id)
	return;

    length = printout_bottom - printout_top;
    if (length < 0)
	length += PRINT_LINES;
    length -= printout_pos;
    if (length > print_gadget_height)
	length = print_gadget_height;
    mybits = (char *) MyGlueBmpGetBits(print_bitmap);
    for (y = 0; y < length; y++) {
	int4 Y = (printout_top + printout_pos + y) % PRINT_LINES;
	for (x = 0; x < PRINT_BYTESPERLINE; x++)
	    mybits[y * 20 + x] = printout[Y * PRINT_BYTESPERLINE + x];
    }
    for (y = length; y < print_gadget_height; y++)
	for (x = 0; x < 20; x++)
	    mybits[y * 20 + x] = (y & 1) == 0 ? 0xaa : 0x55;

    black.r = black.g = black.b = 0;
    white.r = white.g = white.b = 255;
    set_colors(&white, &black);
    set_coord_sys();
    WinDrawBitmap(print_bitmap, 0, 0);
    restore_colors();
    restore_coord_sys();
}

void draw_softkey(int state) {
    if (!display_enabled)
	// Should never happen -- the display is only disabled during macro
	// execution, and softkey events should be impossible to generate
	// in that state. But, just staying on the safe side.
	return;

    Coord w, h;
    int x, y, xoff, yoff;
    UInt16 err;
    BitmapType *bmp;
    char *bits, *bits_v3, *disp_bits;
    UInt16 bpl, disp_bpl;

    w = 21 * skin->display_xscale;
    h = 7 * skin->display_yscale;
    bmp = MyGlueBmpCreate(w, h, 1, NULL, &err);
    bits = (char *) MyGlueBmpGetBits(bmp);
    disp_bits = (char *) MyGlueBmpGetBits(disp_bitmap);
    BmpGlueGetDimensions(bmp, NULL, NULL, &bpl);
    BmpGlueGetDimensions(disp_bitmap, NULL, NULL, &disp_bpl);
    xoff = (softkey - 1) * 22 * skin->display_xscale;
    yoff = 9 * skin->display_yscale;
    MemSet(bits, h * bpl, 0);
    for (y = 0; y < h; y++)
	for (x = 0; x < w; x++) {
	    int pix = (disp_bits[(y + yoff) * disp_bpl + ((x + xoff) >> 3)]
		& (128 >> ((x + xoff) & 7))) != 0;
	    if (state)
		pix = !pix;
	    if (pix)
		bits[y * bpl + (x >> 3)] |= 128 >> (x & 7);
	}

    if (skin->density != 72) {
	UInt32 size = BmpBitsSize(bmp);
	BitmapType *bmp_v3;
	bits_v3 = (char *) MemPtrNew(size);
	if (bits_v3 == NULL)
	    goto not_v3;
	bmp_v3 = (BitmapType *) BmpCreateBitmapV3(bmp,
					    kDensityDouble, bits_v3, NULL);
	if (bmp_v3 == NULL) {
	    MemPtrFree(bits_v3);
	    goto not_v3;
	}
	MemMove(bits_v3, bits, size);
	BmpDelete(bmp);
	bmp = bmp_v3;
    } else {
	not_v3:
	bits_v3 = NULL;
    }

    set_colors(&skin->display_bg, &skin->display_fg);
    set_coord_sys();
    WinDrawBitmap(bmp, skin->display_x + xoff, skin->display_y + yoff);
    restore_colors();
    restore_coord_sys();
    MyGlueBmpDelete(bmp);
    if (bits_v3 != NULL)
	MemPtrFree(bits_v3);
}

static Boolean prefs_handler(EventType *e) SHELL2_SECT;
static Boolean prefs_handler(EventType *e) {
    char buf[FILENAMELEN];
    int field, direction;
    const char *title, *patterns;

    switch (e->eType) {
	case ctlSelectEvent:
	    switch (e->data.ctlSelect.controlID) {
		case browse_txt_id:
		    field = prefs_printer_txt_name;
		    title = "Select Text File Name";
		    patterns = "Text (*.txt)\0txt\0All Files (*.*)\0*\0\0";
		    goto do_browse;
		    
		case browse_gif_id:
		    field = prefs_printer_gif_name;
		    title = "Select GIF File Name";
		    patterns = "GIF (*.gif)\0gif\0All Files (*.*)\0*\0\0";

		do_browse: {
		    FormType *form = FrmGetActiveForm();
		    int idx = FrmGetObjectIndex(form, field);
		    FieldType *fld = (FieldType *) FrmGetObjectPtr(form, idx);
		    char *s = FldGetTextPtr(fld);
		    StrCopy(buf, s);
		    if (select_file(title, "Select", patterns,
							buf, FILENAMELEN)) {
			set_field_text(fld, buf);
			FldDrawField(fld);
		    }
		    return true;
		}

		case sound_down_id: {
		    direction = -1;
		    goto change_sound_volume;
		}

		case sound_up_id: {
		    FormType *form;
		    int idx, oldvol, vol;
		    FieldType *fld;
		    char *s, buf[5];

		    direction = 1;
		    change_sound_volume:
		    form = FrmGetActiveForm();
		    idx = FrmGetObjectIndex(form, sound_field_id);
		    fld = (FieldType *) FrmGetObjectPtr(form, idx);
		    s = FldGetTextPtr(fld);
		    oldvol = StrAToI(s);
		    vol = oldvol + direction;
		    if (vol < 0)
			vol = 0;
		    if (vol > sndMaxAmp)
			vol = sndMaxAmp;
		    if (vol != oldvol) {
			StrIToA(buf, vol);
			set_field_text(fld, buf);
			FldDrawField(fld);
		    }
		    return true;
		}

		case sound_play_id: {
		    FormType *form = FrmGetActiveForm();
		    int idx = FrmGetObjectIndex(form, sound_field_id);
		    FieldType *fld = (FieldType *) FrmGetObjectPtr(form, idx);
		    char *s = FldGetTextPtr(fld);
		    int saved_vol = state.soundVolume;
		    state.soundVolume = StrAToI(s);
		    if (state.soundVolume < 0)
			state.soundVolume = 0;
		    else if (state.soundVolume > sndMaxAmp)
			state.soundVolume = sndMaxAmp;
		    shell_beeper(550, 250); /* TONE 9 */
		    state.soundVolume = saved_vol;
		    return true;
		}

		case sound_volume_id: {
		    int saved_vol = state.soundVolume;
		    state.soundVolume = e->data.ctlSelect.value;
		    shell_beeper(550, 250); /* TONE 9 */
		    state.soundVolume = saved_vol;
		    return true;
		}
	    }
	default:
	    return false;
    }
}

Boolean form_handler(EventType *e) {
    static int inPrintForm = 0;

    switch (e->eType) {
	case frmOpenEvent: {
	    FormType *form = FrmGetActiveForm();
	    FrmDrawForm(form);
	    return true;
	}

	case ctlSelectEvent:
	    switch (e->data.ctlSelect.controlID) {
		/* ... */
	    }
	    return true;

	case penDownEvent: {
	    /* Activate menu bar if user taps in top part of screen */
	    if (e->screenY < 15 && (!inPrintForm || e->screenX < 150)) {
		EventType evt;
		evt.eType = keyDownEvent;
		evt.data.keyDown.chr = vchrMenu;
		evt.data.keyDown.keyCode = 0;
		evt.data.keyDown.modifiers = 0;
		EvtAddEventToQueue(&evt);
		/*MenuDrawMenu(MenuBarType *);*/
		return true;
	    } else
		return false;
	}

	case menuEvent:
	    switch (e->data.menu.itemID) {
		case calcform_id:
		    inPrintForm = 0;
		    FrmGotoForm(calcform_id);
		    return true;

		case printform_id:
		    inPrintForm = 1;
		    FrmGotoForm(printform_id);
		    return true;

		case importprogram_id:
		    do_import();
		    return true;

		case exportprogram_id:
		    do_export();
		    return true;

		case clearprintout_id:
		    printout_top = 0;
		    printout_bottom = 0;
		    printout_pos = 0;
		    close_gif(0);
		    if (FrmGetActiveFormID() == printform_id) {
			FormType *form = FrmGetActiveForm();
			int index = FrmGetObjectIndex(form, printscroll_id);
			ScrollBarType *sb = (ScrollBarType *)
						FrmGetObjectPtr(form, index);
			SclSetScrollBar(sb, 0, 0, 0, print_gadget_height);
			repaint_printout();
		    }
		    return true;

		case prefsform1_id: {
		    FormType *form;
		    FieldType *fld;
		    ControlType *ctl;
		    char buf[20], *s;
		    UInt16 i, min, max, value, pagesize;

		    form = FrmInitForm(feature_set_3_5_present() ?
					prefsform2_id : prefsform1_id);
		    FrmSetEventHandler(form, prefs_handler);

		    #ifdef PALMOS_ARM_SHELL
		    get_core_settings();
		    #endif
		    i = FrmGetObjectIndex(form, prefs_matrix_singularmatrix);
		    FrmSetControlValue(form, i,
					core_settings.matrix_singularmatrix);
		    i = FrmGetObjectIndex(form, prefs_matrix_outofrange);
		    FrmSetControlValue(form, i,
					core_settings.matrix_outofrange);
		    i = FrmGetObjectIndex(form, prefs_auto_repeat);
		    FrmSetControlValue(form, i,
					core_settings.auto_repeat);
		    i = FrmGetObjectIndex(form, raw_text_id);
		    FrmSetControlValue(form, i, core_settings.raw_text);

		    i = FrmGetObjectIndex(form, prefs_printer_memo);
		    FrmSetControlValue(form, i, state.printerToMemo);
		    i = FrmGetObjectIndex(form, prefs_printer_memo_name);
		    fld = (FieldType *) FrmGetObjectPtr(form, i);
		    set_field_text(fld, state.printerMemoName);

		    i = FrmGetObjectIndex(form, prefs_printer_txt);
		    FrmSetControlValue(form, i, state.printerToTxtFile);
		    i = FrmGetObjectIndex(form, prefs_printer_txt_name);
		    fld = (FieldType *) FrmGetObjectPtr(form, i);
		    set_field_text(fld, state.printerTxtFileName);

		    i = FrmGetObjectIndex(form, prefs_printer_gif);
		    FrmSetControlValue(form, i, state.printerToGifFile);
		    i = FrmGetObjectIndex(form, prefs_printer_gif_name);
		    fld = (FieldType *) FrmGetObjectPtr(form, i);
		    set_field_text(fld, state.printerGifFileName);

		    i = FrmGetObjectIndex(form, prefs_printer_gif_height);
		    fld = (FieldType *) FrmGetObjectPtr(form, i);
		    StrIToA(buf, state.printerGifMaxLength);
		    set_field_text(fld, buf);

		    if (feature_set_3_5_present()) {
			i = FrmGetObjectIndex(form, sound_volume_id);
			ctl = (ControlType *) FrmGetObjectPtr(form, i);
			min = 0;
			max = sndMaxAmp;
			value = state.soundVolume;
			if (value < min)
			    value = min;
			if (value > max)
			    value = max;
			pagesize = sndMaxAmp / 8;
			CtlSetSliderValues(ctl, &min, &max, &pagesize, &value);
		    } else {
			i = FrmGetObjectIndex(form, sound_field_id);
			fld = (FieldType *) FrmGetObjectPtr(form, i);
			if (state.soundVolume < 0)
			    state.soundVolume = 0;
			else if (state.soundVolume > sndMaxAmp)
			    state.soundVolume = sndMaxAmp;
			StrIToA(buf, state.soundVolume);
			set_field_text(fld, buf);
		    }

		    if (FrmDoDialog(form) != ok_id)
			goto done_prefs;

		    i = FrmGetObjectIndex(form, prefs_matrix_singularmatrix);
		    core_settings.matrix_singularmatrix =
					FrmGetControlValue(form, i);
		    i = FrmGetObjectIndex(form, prefs_matrix_outofrange);
		    core_settings.matrix_outofrange =
					FrmGetControlValue(form, i);
		    i = FrmGetObjectIndex(form, prefs_auto_repeat);
		    core_settings.auto_repeat =
					FrmGetControlValue(form, i);
		    i = FrmGetObjectIndex(form, raw_text_id);
		    core_settings.raw_text = FrmGetControlValue(form, i);
		    #ifdef PALMOS_ARM_SHELL
		    put_core_settings();
		    #endif

		    i = FrmGetObjectIndex(form, prefs_printer_memo);
		    state.printerToMemo = FrmGetControlValue(form, i);
		    i = FrmGetObjectIndex(form, prefs_printer_memo_name);
		    fld = (FieldType *) FrmGetObjectPtr(form, i);
		    s = FldGetTextPtr(fld);
		    if (!state.printerToMemo ||
			    StrCaselessCompare(state.printerMemoName, s) != 0)
			close_memo();
		    StrNCopy(state.printerMemoName, s, FILENAMELEN - 1);
		    state.printerMemoName[FILENAMELEN - 1] = 0;

		    i = FrmGetObjectIndex(form, prefs_printer_txt);
		    state.printerToTxtFile = FrmGetControlValue(form, i);
		    i = FrmGetObjectIndex(form, prefs_printer_txt_name);
		    fld = (FieldType *) FrmGetObjectPtr(form, i);
		    s = FldGetTextPtr(fld);
		    if (!state.printerToTxtFile ||
			   StrCaselessCompare(state.printerTxtFileName, s) != 0)
			close_txt();
		    StrNCopy(state.printerTxtFileName, s, FILENAMELEN - 1);
		    state.printerTxtFileName[FILENAMELEN - 1] = 0;

		    i = FrmGetObjectIndex(form, prefs_printer_gif);
		    state.printerToGifFile = FrmGetControlValue(form, i);
		    i = FrmGetObjectIndex(form, prefs_printer_gif_name);
		    fld = (FieldType *) FrmGetObjectPtr(form, i);
		    s = FldGetTextPtr(fld);
		    if (!state.printerToGifFile ||
			   StrCaselessCompare(state.printerGifFileName, s) != 0)
			close_gif(1);
		    StrNCopy(state.printerGifFileName, s, FILENAMELEN - 1);
		    state.printerGifFileName[FILENAMELEN - 1] = 0;

		    i = FrmGetObjectIndex(form, prefs_printer_gif_height);
		    fld = (FieldType *) FrmGetObjectPtr(form, i);
		    s = FldGetTextPtr(fld);
		    state.printerGifMaxLength = StrAToI(s);
		    if (state.printerGifMaxLength < 32)
			state.printerGifMaxLength = 32;
		    else if (state.printerGifMaxLength > 32767)
			state.printerGifMaxLength = 32767;

		    if (feature_set_3_5_present()) {
			i = FrmGetObjectIndex(form, sound_volume_id);
			ctl = (ControlType *) FrmGetObjectPtr(form, i);
			state.soundVolume = CtlGetValue(ctl);
		    } else {
			i = FrmGetObjectIndex(form, sound_field_id);
			fld = (FieldType *) FrmGetObjectPtr(form, i);
			s = FldGetTextPtr(fld);
			state.soundVolume = StrAToI(s);
			if (state.soundVolume < 0)
			    state.soundVolume = 0;
			else if (state.soundVolume > sndMaxAmp)
			    state.soundVolume = sndMaxAmp;
		    }

		    done_prefs:
		    FrmDeleteForm(form);
		    return true;
		}

		case copy_id:
		    do_clip_copy();
		    return true;

		case paste_id:
		    do_clip_paste();
		    return true;

		case selectskin_id: {
		    /* Only happens with PalmOS < 3.5; with >= 3.5, this
		     * menu item is dynamically hidden, and instead a list
		     * of skin names is appended to the menu.
		     * UPDATE: OK, this also happens with PalmOS >= 3.5
		     * and < 4.0; the reason being that on my m100, hiding
		     * the menu item doesn't work. So, with PalmOS in the
		     * range [3.5, 4.0), you get *both* the "Select..."
		     * dialog, *and* the dynamic menu. Oh, well!
		     */
		    FormType *form = FrmInitForm(selectskin_id);
		    int idx = FrmGetObjectIndex(form, skin_list_id);
		    int i, n = builtin_skins + external_skins;
		    int prev;
		    ListType *list = (ListType *) FrmGetObjectPtr(form, idx);
		    LstSetListChoices(list, skin_name, n);
		    for (i = 0; i < n; i++)
			if (StrCompare(state.skinName, skin_name[i]) == 0) {
			    prev = i;
			    LstSetSelection(list, i);
			    break;
			}
		    if (FrmDoDialog(form) == ok_id) {
			i = LstGetSelection(list);
			if (i >= 0 && i < n && i != prev) {
			    StrCopy(state.skinName, skin_name[i]);
			    unload_skin();
			    load_skin();
			    core_repaint_display();
			    update_dia_state();
			    if (feature_set_3_5_present())
				FrmUpdateForm(calcform_id, frmRedrawUpdateCode);
			    else {
				/* The FrmUpdateForm() call doesn't do the job
				 * when I'm testing with PalmOS 3.3... So,
				 * brute force.
				 */
				if (FrmGetActiveFormID() == calcform_id)
				    calcgadget_handler(NULL, formGadgetDrawCmd, NULL);
			    }
			}
		    }
		    FrmDeleteForm(form);
		    return true;
		}

		case deleteskin_id:
		    do_delete_skin();
		    return true;

		case copyfile_id: 
		    do_copy();
		    return true;

		case deletefile_id:
		    do_delete();
		    return true;

		case removedir_id:
		    do_removedir();
		    return true;

		case erasefilesys_id:
		    if (FrmAlert(erase_confirm_id) == 0)
			dbfs_erase();
		    return true;

		case forcedownload_id:
		    if (FrmAlert(forcedownload_confirm_id) == 0)
			dbfs_makedirty();
		    return true;

		case aboutform_id: {
		    FormType *form = FrmInitForm(aboutform_id);
		    /* Customize the about box here, if desired */
		    FrmDoDialog(form);
		    FrmDeleteForm(form);
		    return true;
		}

		default:
		    if (e->data.menu.itemID > (UInt16) deleteskin_id
			    && e->data.menu.itemID <= (UInt16) (deleteskin_id
				    + builtin_skins + external_skins)) {
			int i = e->data.menu.itemID - deleteskin_id - 1;
			StrCopy(state.skinName, skin_name[i]);
			unload_skin();
			load_skin();
			core_repaint_display();
			update_dia_state();
			FrmUpdateForm(calcform_id, frmRedrawUpdateCode);
			return true;
		    } else {
			/* Otherwise it's a system menu event
			 * (e.g., edit menu related), so we
			 * let the system handle it.  */
			return false;
		    }
	    }

	case sclRepeatEvent: {
	    ScrollBarType *sb = e->data.sclRepeat.pScrollBar;
	    Int16 sbvalue, sbmin, sbmax, sbpagesize;
	    SclGetScrollBar(sb, &sbvalue, &sbmin, &sbmax, &sbpagesize);
	    printout_pos = sbvalue;
	    repaint_printout();
	    /* If I return 'true' here, the scroll bar won't work properly. */
	    return false;
	}

	default:
	    return false;
    }

}

Boolean calcgadget_handler(struct FormGadgetTypeInCallback *gadgetP,
	UInt16 cmd, void *paramP) {
    switch (cmd) {

	case formGadgetDeleteCmd: {
	    /* Delete notification; free any associated data here */
	    return true;
	}

	case formGadgetDrawCmd: {
	    /* Paint request */

	    int i, yy = 0;

	    set_default_colors();
	    set_coord_sys();
	    for (i = 0; i < skin->sections; i++) {
		Coord sh;
		WinDrawBitmap(skin_bm_p[i], 0, yy);
		BmpGlueGetDimensions(skin_bm_p[i], NULL, &sh, NULL);
		yy += sh;
	    }
	    restore_colors();
	    restore_coord_sys();

	    for (i = 0; i < 7; i++)
		repaint_annunciator(i);

	    set_colors(&skin->display_bg, &skin->display_fg);
	    set_coord_sys();
	    WinDrawBitmap(disp_bitmap, skin->display_x, skin->display_y);
	    restore_colors();
	    restore_coord_sys();
	    if (softkey != 0)
		draw_softkey(1);

	    return true;
	}

	case formGadgetEraseCmd: {
	    /* We don't have to do anything here */
	    return true;
	}

	case formGadgetHandleEventCmd: {
	    /* Handle user events */
	    EventType *event = (EventType *) paramP;
	    if (event->eType == frmGadgetEnterEvent) {
		/* penDown in gadget's bounds */
		/* Look for key that was pressed */
		int i;
		int keycode = 0;
		Int16 x = event->screenX;
		Int16 y = event->screenY;
		if (skin->density == 144) {
		    x <<= 1;
		    y <<= 1;
		}

		if (core_menu()
			&& x >= skin->display_x
			&& x < skin->display_x + 131 * skin->display_xscale
			&& y >= skin->display_y + 9 * skin->display_yscale
			&& y < skin->display_y + 16 * skin->display_yscale) {
		    softkey = (x - skin->display_x)
			/ (22 * skin->display_xscale) + 1;
		    keycode = softkey;
		} else {
		    for (i = 0; i < skin->nkeys; i++) {
			KeySpec *key = skin->key + i;
			if (x >= key->sens_x
				&& x < key->sens_x + key->sens_width
				&& y >= key->sens_y
				&& y < key->sens_y + key->sens_height) {
			    if (skin_version >= 3 && shiftAnn != 0)
				keycode = key->shifted_code;
			    else
				keycode = key->code;
			    break;
			}
		    }
		    softkey = 0;
		}

		if (keycode != 0) {
		    /* PenDown on a key! */
		    Int16 x, y;
		    Boolean down;
		    int enqueued, repeat;

		    if (keycode != 28 /* shift */) {
			timeout3time = -1;
			core_timeout3(0);
		    }

		    if (keycode >= 38 && keycode <= 255) {
			/* Macro */
			unsigned char *macro = find_macro(keycode);
			if (macro == NULL || *macro == 0) {
			    squeak();
			    return true;
			}
			SndPlaySystemSound(sndClick);
			bool one_key_macro = macro[1] == 0
					|| (macro[2] == 0 && macro[0] == 28);
			if (!one_key_macro)
			    display_enabled = false;
			while (*macro != 0) {
			    want_to_run = core_keydown(*macro++, &enqueued,
								 &repeat);
			    if (*macro != 0 && !enqueued)
				core_keyup();
			}
			if (!one_key_macro) {
			    display_enabled = true;
			    set_colors(&skin->display_bg, &skin->display_fg);
			    set_coord_sys();
			    WinDrawBitmap(disp_bitmap, skin->display_x,
				    skin->display_y);
			    restore_colors();
			    restore_coord_sys();
			    for (int a = 0; a < 7; a++)
				repaint_annunciator(a);
			}
			repeat = 0;
		    } else {
			SndPlaySystemSound(sndClick);
			want_to_run = core_keydown(keycode, &enqueued, &repeat);
		    }

		    if (softkey == 0) {
			set_default_colors();
			set_coord_sys();
			WinDrawBitmap(key_down_p[i], skin->key[i].x,
						     skin->key[i].y);
			restore_colors();
			restore_coord_sys();
		    } else
			draw_softkey(1);

		    if (repeat != 0) {
			UInt16 tps = SysTicksPerSecond();
			UInt32 ticks_repeat = TimGetTicks()
					    + (repeat == 1 ? tps : (tps / 2));
			while (EvtGetPen(&x, &y, &down), down) {
			    if (TimGetTicks() > ticks_repeat) {
				repeat = core_repeat();
				if (repeat == 0)
				    goto do_the_usual;
				ticks_repeat = TimGetTicks()
						+ tps / (repeat == 1 ? 5 : 10);
			    }
			}
		    } else if (want_to_run) {
			/* We don't bother with the timeout signals
			 * in this case; the emulator says it wants to
			 * run so that means it doesn't care about
			 * those timeouts anyway.
			 * We do keep the key highlight in place until
			 * the pen is lifted.
			 */
			while (EvtGetPen(&x, &y, &down), down) {
			    int dummy1, dummy2;
			    if (want_to_run)
				want_to_run = core_keydown(0, &dummy1, &dummy2);
			}
		    } else if (!enqueued) {
			do_the_usual:
			UInt16 tps = SysTicksPerSecond();
			UInt32 ticks_down = TimGetTicks();
			UInt32 ticks_timeout1 = ticks_down + tps / 4;
			UInt32 ticks_timeout2 = ticks_timeout1 + tps * 7 / 4;
			int timeout_state = 0;
			while (EvtGetPen(&x, &y, &down), down) {
			    switch (timeout_state) {
				case 0:
				    if (TimGetTicks() > ticks_timeout1) {
					core_keytimeout1();
					timeout_state = 1;
				    }
				    break;
				case 1:
				    if (TimGetTicks() > ticks_timeout2) {
					core_keytimeout2();
					timeout_state = 2;
				    }
				    break;
			    }
			}
		    }

		    if (softkey == 0) {
			set_default_colors();
			set_coord_sys();
			WinDrawBitmap(key_up_p[i], skin->key[i].x,
						   skin->key[i].y);
			restore_colors();
			restore_coord_sys();
		    } else {
			draw_softkey(0);
			softkey = 0;
		    }

		    if (!enqueued)
			want_to_run = core_keyup();
		}
	    }
	    return true;
	}

	default:
	    return false;
    }
}

Boolean printgadget_handler(struct FormGadgetTypeInCallback *gadgetP,
	UInt16 cmd, void *paramP) {
    switch (cmd) {

	case formGadgetDeleteCmd: {
	    /* Delete notification; free any associated data here */
	    return true;
	}

	case formGadgetDrawCmd: {
	    /* Paint request */
	    repaint_printout();
	    return true;
	}

	case formGadgetEraseCmd: {
	    /* We don't have to do anything here */
	    return true;
	}

	case formGadgetHandleEventCmd: {
	    /* Handle user events */
	    return true;
	}

	default:
	    return false;
    }
}

Boolean handle_event(EventType *e) {
    if (e->eType == frmLoadEvent) {
	FormType *form = FrmInitForm(e->data.frmLoad.formID);
	switch (e->data.frmLoad.formID) {
	    case calcform_id: {
		if (feature_set_3_5_present()) {
		    UInt16 index = FrmGetObjectIndex(form, calcgadget_id);
		    FrmSetGadgetHandler(form, index, calcgadget_handler);
		}
		if (pen_input_manager_present()) {
		    WinHandle wh = FrmGetWindowHandle(form);
		    WinSetConstraintsSize(wh, 160, 160, 225, 160, 160, 160);
		    FrmSetDIAPolicyAttr(form, frmDIAPolicyCustom);
		}
		FrmSetActiveForm(form);
		FrmSetEventHandler(form, form_handler);
	    }
	    break;
	    case printform_id: {
		UInt16 index;
		ScrollBarType *sb;
		int max = printout_bottom - printout_top;
		if (max < 0)
		    max += PRINT_LINES;
		max = max > print_gadget_height ? max - print_gadget_height : 0;
		index = FrmGetObjectIndex(form, printscroll_id);
		sb = (ScrollBarType *) FrmGetObjectPtr(form, index);
		SclSetScrollBar(sb, printout_pos, 0, max, print_gadget_height);
		if (feature_set_3_5_present()) {
		    index = FrmGetObjectIndex(form, printgadget_id);
		    FrmSetGadgetHandler(form, index, printgadget_handler);
		}
		if (pen_input_manager_present()) {
		    WinHandle wh = FrmGetWindowHandle(form);
		    WinSetConstraintsSize(wh, 160, 160, 225, 160, 160, 160);
		    FrmSetDIAPolicyAttr(form, frmDIAPolicyCustom);
		}
		FrmSetActiveForm(form);
		FrmSetEventHandler(form, form_handler);
	    }
	    break;
	}
	return true;
    } else if (e->eType == menuOpenEvent) {
	if (feature_set_3_5_present() && skin_name != NULL) {
	    int i, n = builtin_skins + external_skins;
	    char sep[2];

	    for (i = 0; i < n; i++)
		MenuAddItem(deleteskin_id + i, deleteskin_id + i + 1,
			    0, skin_name[i]);
	    sep[0] = MenuSeparatorChar;
	    sep[1] = 0;
	    MenuAddItem(deleteskin_id, deleteskin_id + n + 1, 0, sep);

	    if (n > 0) {
		/* On my m100, and also on POSE with the ROM image from my
		 * m100, MenuHideItem() does not work: the menu item label
		 * ("Select...") disappears alright, but it leaves a blank space
		 * behind -- the following menu items do not move up. What's
		 * worse, the last menu item is invisible (apparently the menu
		 * size *is* calculated correctly), and what's worse still, the
		 * machine tends to hang fairly consistently after posting this
		 * corrupted menu.
		 * I don't know when this bug was fixed, but I haven't noticed
		 * it with 4.1.2 (POSE), so to be safe, I'm making this call
		 * dependent on PalmOS 4.0.
		 */
		if (feature_set_4_0_present())
		    MenuHideItem(selectskin_id);
	    }
	}
	return true;
    } else if (e->eType == menuCmdBarOpenEvent) {
	if (feature_set_3_5_present()
		    && FrmGetActiveFormID() == calcform_id) {
	    /* Show "Copy" and "Paste" icons on command bar; this is usually
	     * automatic, but since the calculator form does not use text
	     * fields, we have to add them ourselves here.
	     */
	    MenuCmdBarAddButton(menuCmdBarOnRight, BarCopyBitmap,
				menuCmdBarResultMenuItem, copy_id, NULL);
	    MenuCmdBarAddButton(menuCmdBarOnRight, BarPasteBitmap,
				menuCmdBarResultMenuItem, paste_id, NULL);
	    e->data.menuCmdBarOpen.preventFieldButtons = true;
	}
	return false;
    } else if (e->eType == keyDownEvent) {
	char c = e->data.keyDown.chr;

	if (FrmGetActiveFormID() == printform_id) {
	    if (c != vchrPageUp && c != vchrPageDown)
		return false;
	    int up = c == vchrPageUp;
	    FormType *form = FrmGetFormPtr(printform_id);
	    UInt16 scl_index = FrmGetObjectIndex(form, printscroll_id);
	    ScrollBarType *sb =
		    (ScrollBarType *) FrmGetObjectPtr(form, scl_index);
	    Int16 value, min, max, page;
	    SclGetScrollBar(sb, &value, &min, &max, &page);
	    if (up) {
		value -= page;
		if (value < min)
		    value = min;
	    } else {
		value += page;
		if (value > max)
		    value = max;
	    }
	    SclSetScrollBar(sb, value, min, max, page);
	    printout_pos = value;
	    repaint_printout();
	    return true;
	}

	if (FrmGetActiveFormID() != calcform_id)
	    return false;
	if ((e->data.keyDown.modifiers & autoRepeatKeyMask) != 0)
	    return false;

	if (c == vchrPageUp || c == vchrPageDown) {
	    int up = c == vchrPageUp;
	    UInt32 mask = up ? keyBitPageUp : keyBitPageDown;
	    int enqueued, repeat;
	    int k = up ? 17 : 22;

	    want_to_run = core_keydown(k + 1, &enqueued, &repeat);
	    if (enqueued) {
		want_to_run = core_keyup();
		return true;
	    }

	    set_default_colors();
	    set_coord_sys();
	    WinDrawBitmap(key_down_p[k], skin->key[k].x, skin->key[k].y);
	    restore_colors();
	    restore_coord_sys();

	    if (repeat) {
		UInt16 tps = SysTicksPerSecond();
		UInt32 ticks_repeat = TimGetTicks() + tps;
		while ((KeyCurrentState() & mask) != 0) {
		    if (TimGetTicks() > ticks_repeat) {
			core_repeat();
			ticks_repeat = TimGetTicks() + tps / 5;
		    }
		}
	    } else if (want_to_run) {
		/* We don't bother with the timeout signals
		    * in this case; the emulator says it wants to
		    * run so that means it doesn't care about
		    * those timeouts anyway.
		    * We do keep the key highlight in place until
		    * the key is released.
		    */
		while ((KeyCurrentState() & mask) != 0) {
		    int dummy1, dummy2;
		    if (want_to_run)
			want_to_run = core_keydown(0, &dummy1, &dummy2);
		}
	    } else if (!enqueued) {
		UInt16 tps = SysTicksPerSecond();
		UInt32 ticks_down = TimGetTicks();
		UInt32 ticks_timeout1 = ticks_down + tps / 4;
		UInt32 ticks_timeout2 = ticks_timeout1 + tps * 7 / 4;
		int timeout_state = 0;
		while ((KeyCurrentState() & mask) != 0) {
		    switch (timeout_state) {
			case 0:
			    if (TimGetTicks() > ticks_timeout1) {
				core_keytimeout1();
				timeout_state = 1;
			    }
			    break;
			case 1:
			    if (TimGetTicks() > ticks_timeout2) {
				core_keytimeout2();
				timeout_state = 2;
			    }
			    break;
		    }
		}
	    }

	    want_to_run = core_keyup();
	    set_default_colors();
	    set_coord_sys();
	    WinDrawBitmap(key_up_p[k], skin->key[k].x, skin->key[k].y);
	    restore_colors();
	    restore_coord_sys();

	    return true;

	} else if ((e->data.keyDown.modifiers & commandKeyMask) == 0
		&& core_alpha_menu()) {
	    int key = 0;
	    int enqueued, dummy;
	    if (c == 8)
		key = 17; /* KEY_BSP */
	    else if (c == 10 || c == 13)
		key = 13; /* KEY_ENTER */
	    else if (c >= 32 && c <= 126) {
		if (c >= 'a' && c <= 'z')
		    c = c - 'a' + 'A';
		else if (c >= 'A' && c <= 'Z')
		    c = c - 'A' + 'a';
		key = c + 1024;
	    }
	    if (key != 0) {
		want_to_run = core_keydown(key, &enqueued, &dummy);
		if (!enqueued)
		    want_to_run = core_keyup();
		return true;
	    }
	}
	return false;
    } else if (e->eType == winExitEvent) {
	if (e->data.winExit.exitWindow ==
		    (WinHandle) FrmGetFormPtr(calcform_id)
		|| e->data.winExit.exitWindow ==
		    (WinHandle) FrmGetFormPtr(printform_id))
	    can_draw = 0;
	return true;
    } else if (e->eType == winEnterEvent) {
	if ((e->data.winEnter.enterWindow ==
		    (WinHandle) FrmGetFormPtr(calcform_id)
		|| e->data.winEnter.enterWindow ==
		    (WinHandle) FrmGetFormPtr(printform_id))
		&& e->data.winEnter.enterWindow ==
		    (WinHandle) FrmGetFirstForm()) {
	    can_draw = 1;
	    if (pen_input_manager_present()) {
		if (e->data.winEnter.enterWindow ==
				(WinHandle) FrmGetFormPtr(calcform_id)) {
		    if (skin_tall) {
			PINSetInputTriggerState(pinInputTriggerEnabled);
			PINSetInputAreaState(pinInputAreaUser);
		    } else {
			PINSetInputTriggerState(pinInputTriggerDisabled);
			PINSetInputAreaState(pinInputAreaOpen);
		    }
		} else {
		    PINSetInputTriggerState(pinInputTriggerDisabled);
		    PINSetInputAreaState(pinInputAreaClosed);
		}
		EventType evt;
		MemSet(&evt, sizeof(EventType), 0);
		evt.eType = (eventsEnum) winDisplayChangedEvent;
		EvtAddUniqueEventToQueue(&evt, 0, true);
	    }
	}
	return true;
    } else if (e->eType == winDisplayChangedEvent) {
	FormType *form = FrmGetActiveForm();
	UInt16 id = FrmGetFormId(form);
	if (id == calcform_id || id == printform_id) {
	    WinHandle wh = FrmGetWindowHandle(form);
	    RectangleType formBounds, screenBounds;
	    WinGetBounds(wh, &formBounds);
	    WinGetBounds(WinGetDisplayWindow(), &screenBounds);
	    // Very simplistic -- this is only OK as long as the calc and
	    // print-out forms *only* contain components that span the entire
	    // height of the form. This is the case at present: the calculator
	    // form only contains one gadget that covers it entirely, and the
	    // print-out form contains a gadget that covers its entire height,
	    // and most of its width, the remaining area being covered by a
	    // scroll bar.
	    UInt16 n = FrmGetNumberOfObjects(form);
	    for (UInt16 i = 0; i < n; i++) {
		RectangleType r;
		FrmGetObjectBounds(form, i, &r);
		r.extent.y = screenBounds.extent.y;
		FrmSetObjectBounds(form, i, &r);
		if (FrmGetObjectType(form, i) == frmScrollBarObj) {
		    ScrollBarType *sb =
			(ScrollBarType *) FrmGetObjectPtr(form, i);
		    Int16 value, min, max, page;
		    SclGetScrollBar(sb, &value, &min, &max, &page);
		    page = screenBounds.extent.y;
		    max = printout_bottom - printout_top;
		    if (max < 0)
			max += PRINT_LINES;
		    max = max > page ? max - page : 0;
		    SclSetScrollBar(sb, value, min, max, page);
		}
	    }
	    WinSetBounds(wh, &screenBounds);
	    print_gadget_height = screenBounds.extent.y;
	    FrmDrawForm(form);
	}
	return true;
    } else
	return false;
}

void show_message(char *message) {
    FrmCustomAlert(msg_alert_id, message, "", "");
}

void set_field_text(FieldType *fld, const char *text) {
    MemHandle oldh = FldGetTextHandle(fld);
    MemHandle newh = MemHandleNew(StrLen(text) + 1);
    if (newh != NULL) {
	MemPtr p = MemHandleLock(newh);
	StrCopy((char *) p, text);
	MemHandleUnlock(newh);
	FldSetTextHandle(fld, newh);
	if (oldh != NULL)
	    MemHandleFree(oldh);
    }
}


/*******************************************/
/***** Printer emulation stuff (Memos) *****/
/*******************************************/

static DmOpenRef memodb = 0;
static MemHandle memoh = NULL;
static int memoseq;
static int4 memolen;
static UInt16 memoindex;

static void memo_writer(const char *text, int length) SHELL2_SECT;
static void memo_writer(const char *text, int length) {
    MemPtr mp;
    char *cp;
    int namelen;

    if (!state.printerToMemo)
	return;
    namelen = StrLen(state.printerMemoName);

    /* Make sure Memo DB is open */

    if (memodb == 0) {
	memodb = DmOpenDatabaseByTypeCreator('DATA', 'memo', dmModeReadWrite);
	if (memodb == 0) {
	    show_message("Could not open Memo database.\n"
			 "Printing to memos disabled.");
	    state.printerToMemo = 0;
	    return;
	}
    }

    /* Open highest-numbered output memo */

    if (memoh == NULL) {
	UInt16 n = DmNumRecords(memodb);
	UInt16 i;
	memoseq = -1;
	for (i = 0; i < n; i++) {
	    MemHandle h;
	    int mtlen;
	    int4 msize;
	    h = DmQueryRecord(memodb, i);
	    if (h == NULL)
		continue;
	    cp = (char *) MemHandleLock(h);
	    msize = MemHandleSize(h);
	    mtlen = 0;
	    while (mtlen < msize && cp[mtlen] != 0 && cp[mtlen] != 10)
		mtlen++;
	    if (mtlen == namelen + 5
		    && cp[namelen] == ' '
		    && StrNCaselessCompare(cp,
			    state.printerMemoName, namelen) == 0) {
		int j, k = 0;
		for (j = namelen + 1; j < mtlen; j++) {
		    char c = cp[j];
		    if (c < '0' || c > '9')
			goto nomatch;
		    k = k * 10 + c - '0';
		}
		if (k > memoseq) {
		    memoseq = k;
		    memoindex = i;
		    memolen = msize;
		}
	    }
	    nomatch:
	    MemHandleUnlock(h);
	}
	if (memoseq != -1) {
	    memoh = DmGetRecord(memodb, memoindex);
	    if (memoh == NULL) {
		show_message("Could not read current output memo.\n"
			     "Printing to memos disabled.");
		state.printerToMemo = 0;
		DmCloseDatabase(memodb);
		memodb = 0;
		return;
	    }
	}
    }

    /* Check if current memo has enough space to add the new text.
     * NOTE: I've successfully created memos of up to 32767 bytes this
     * way, and if I hadn't accidentally used an 'int' instead of an
     * 'int4' for the size, I might have been able to go up to 65535.
     * However, on older versions of PalmOS (specifically, 3.5.1 on the
     * m100), Memo Pad may not let you edit memos that size, and on
     * newer versions (specifically, 5.2.1 on the Tungsten E), PACE may
     * truncate memos to 4096 characters when you try to read them.
     * So, I'm using a 4096 byte maximum size, at least until I find a
     * reliable way of finding the true maximum at run time.
     */

    if (memoh != NULL && memolen + length + 1 > 4096) {
	DmReleaseRecord(memodb, memoindex, true);
	memoh = NULL;
    }

    /* If we have no memo, either because none existed or because the
     * highest-numbered one is too full, create a new memo, store the
     * title on line 1, and the text on line 2; else, grow the current
     * memo and append the text to it.
     */

    if (memoh == NULL) {
	char *namebuf;
	int s, t;

	memoindex = DmNumRecords(memodb);
	memoh = DmNewRecord(memodb, &memoindex, namelen + 6 + length + 2);
	if (memoh == NULL) {
	    show_message("Could not create new memo.\n"
			    "Printing to memos disabled.");
	    state.printerToMemo = 0;
	    DmCloseDatabase(memodb);
	    memodb = 0;
	    return;
	}
	mp = MemHandleLock(memoh);

	memoseq = (memoseq + 1) % 10000;
	namebuf = (char *) MemPtrNew(namelen + 6);
	MemMove(namebuf, state.printerMemoName, namelen);
	namebuf[namelen] = ' ';
	t = memoseq;
	for (s = 4; s >= 1; s--) {
	    namebuf[namelen + s] = t % 10 + '0';
	    t /= 10;
	}
	namebuf[namelen + 5] = 10;
	DmWrite(mp, 0, namebuf, namelen + 6);
	MemPtrFree(namebuf);

	DmWrite(mp, namelen + 6, text, length);
	DmWrite(mp, namelen + 6 + length, "\n", 2);
	MemHandleUnlock(memoh);
	memolen = namelen + 6 + length + 2;
    } else {
	MemHandle newh = DmResizeRecord(memodb, memoindex,
						memolen + length + 1);
	if (newh == NULL) {
	    show_message("Could not resize output memo.\n"
			 "Printing to memos disabled.");
	    state.printerToMemo = 0;
	    DmReleaseRecord(memodb, memoindex, true);
	    DmCloseDatabase(memodb);
	    memodb = 0;
	    return;
	}
	memoh = newh;
	mp = MemHandleLock(memoh);
	DmWrite(mp, memolen - 1, text, length);
	DmWrite(mp, memolen - 1 + length, "\n", 2);
	MemHandleUnlock(memoh);
	memolen += length + 1;
    }
}

static void memo_newliner() SHELL2_SECT;
static void memo_newliner() {
    /* No-op; memo_writer() takes care of the newlines */

    /* TODO: this is not good; it means that shell_spool_txt() can't break
     * a large block up into several writer() calls, since memo_writer() will
     * then insert unwanted newlines.
     * The correct fix for this would be to maintain a 32 kilobyte cache, and
     * cache entire memos there, so they can be sent to DmWrite() all in one
     * go. Using such an approach, individual writes are cheap, and it's no
     * longer as desirable to handle newlines along with the preceding text;
     * it fixes this bug-waiting-to-happen AND improves performance.
     */

    /* On second thought, don't make it a 32 kilobyte buffer, but something
     * smaller. Beware the tiny m100 heap!
     */
}

void print_to_memo(const char *text, int length) {
    shell_spool_txt(text, length, memo_writer, memo_newliner);
}

void close_memo() {
    if (memodb == 0)
	return;
    if (memoh != NULL) {
	DmReleaseRecord(memodb, memoindex, true);
	memoh = NULL;
    }
    DmCloseDatabase(memodb);
    memodb = 0;
}


/*******************************************/
/***** Printer emulation stuff (files) *****/
/*******************************************/

static fsa_obj *txtfile = NULL;

static void txt_writer(const char *text, int length) SHELL2_SECT;
static void txt_writer(const char *text, int length) {
    uint4 n;
    int err;

    if (!state.printerToTxtFile)
	return;

    if (txtfile == NULL) {
	char basename[FILENAMELEN];
	int len;
	fsa_obj *dir;
	err = fsa_resolve(state.printerTxtFileName, &dir, 1, basename);
	if (err != FSA_ERR_NONE) {
	    show_message("Can't find directory for writing text file.\n"
			 "Printing to text file disabled.");
	    state.printerToTxtFile = 0;
	    return;
	}
	len = StrLen(basename);
	if (len < 4 || StrNCaselessCompare(basename + len - 4, ".txt", 4)) {
	    StrNCat(basename, ".txt", FILENAMELEN - 1);
	    basename[FILENAMELEN - 1] = 0;
	}
	err = fsa_create(dir, basename, &txtfile);
	fsa_release(dir);
	if (err != FSA_ERR_NONE && err != FSA_ERR_FILE_EXISTS) {
	    show_message("Can't create text file.\n"
			 "Printing to text file disabled.");
	    state.printerToTxtFile = 0;
	    return;
	}
	err = fsa_open(txtfile, FSA_MODE_READWRITE);
	if (err != FSA_ERR_NONE) {
	    show_message("Can't open text file.\n"
			 "Printing to text file disabled.");
	    state.printerToTxtFile = 0;
	    return;
	}
	err = fsa_seek(txtfile, FSA_SEEK_END, 0);
	if (err != FSA_ERR_NONE) {
	    fsa_release(txtfile);
	    txtfile = NULL;
	    show_message("Can't seek to end of text file.\n"
			 "Printing to text file disabled.");
	    state.printerToTxtFile = 0;
	    return;
	}
    }

    n = (uint4) length;
    err = fsa_write(txtfile, text, &n);
    if (err == FSA_ERR_NONE && n == (uint4) length)
	return;

    fsa_release(txtfile);
    txtfile = NULL;
    state.printerToTxtFile = 0;
    show_message("Error writing to text file.\n"
		 "Printing to text file disabled.");
}

static void txt_newliner() SHELL2_SECT;
static void txt_newliner() {
    txt_writer("\n", 1);
}

void print_to_txt(const char *text, int length) {
    shell_spool_txt(text, length, txt_writer, txt_newliner);
}

void close_txt() {
    if (txtfile != NULL) {
	fsa_release(txtfile);
	txtfile = NULL;
    }
}

static fsa_obj *giffile = NULL;
static int gifseq = -1;
static int4 gif_lines;
static char giffile_name[FILENAMELEN];

static void gif_seeker(int4 pos) SHELL2_SECT;
static void gif_seeker(int4 pos) {
    int err;
    if (!state.printerToGifFile || giffile == NULL)
	return;
    err = fsa_seek(giffile, FSA_SEEK_START, pos);
    if (err != FSA_ERR_NONE) {
	char buf[FILENAMELEN + 100];
	state.printerToGifFile = 0;
	fsa_release(giffile);
	giffile = NULL;
	StrCopy(buf, "Error while seeking \"");
	StrCat(buf, giffile_name);
	StrCat(buf, "\".\nPrinting to GIF file disabled.");
	show_message(buf);
    }
}

static void gif_writer(const char *text, int length) SHELL2_SECT;
static void gif_writer(const char *text, int length) {
    int err;
    uint4 n;
    if (!state.printerToGifFile || giffile == NULL)
	return;
    n = (uint4) length;
    err = fsa_write(giffile, text, &n);
    if (err != FSA_ERR_NONE || n != (uint4) length) {
	char buf[FILENAMELEN + 100];
	state.printerToGifFile = 0;
	fsa_release(giffile);
	giffile = NULL;
	StrCopy(buf, "Error while writing \"");
	StrCat(buf, giffile_name);
	StrCat(buf, "\".\nPrinting to GIF file disabled.");
	show_message(buf);
    }
}

void print_to_gif(const char *bits, short bytesperline, short x, short y,
		  short width, short height) {
    char msg[FILENAMELEN + 100];
    if (giffile != NULL && gif_lines + height > state.printerGifMaxLength) {
	close_gif(0);
    }
    if (giffile == NULL) {
	int err;
	int gifseq_final = gifseq == -1 ? 9999 : gifseq;
	int len;
	fsa_obj *dir;
	err = fsa_resolve(state.printerGifFileName, &dir, 1, giffile_name);
	if (err != FSA_ERR_NONE) {
	    show_message("Can't find directory for writing GIF file.\n"
			 "Printing to GIF file disabled.");
	    state.printerToGifFile = 0;
	    return;
	}
	len = StrLen(giffile_name);
	if (len >= 4 &&
		StrNCaselessCompare(giffile_name + len - 4, ".gif", 4) == 0) {
	    len -= 4;
	    giffile_name[len] = 0;
	}
	/* Ensure we have enough space for the ".nnnn.gif" */
	if (len > FILENAMELEN - 10) {
	    len = FILENAMELEN - 10;
	    giffile_name[len] = 0;
	}
	StrCat(giffile_name, ".nnnn.gif");
	while (1) {
	    int n, s;
	    gifseq = (gifseq + 1) % 10000;
	    s = gifseq;
	    for (n = len + 4; n > len; n--) {
		giffile_name[n] = '0' + s % 10;
		s /= 10;
	    }
	    err = fsa_create(dir, giffile_name, &giffile);
	    if (err == FSA_ERR_NONE)
		break;
	    else if (err == FSA_ERR_FILE_EXISTS) {
		fsa_release(giffile);
		if (gifseq == gifseq_final) {
		    giffile = NULL;
		    fsa_release(dir);
		    state.printerToGifFile = 0;
		    show_message(
			    "Can't create GIF file - all sequence numbers "
			    "used up.\nPrinting to GIF file disabled.");
		    return;
		} else
		    continue;
	    } else {
		giffile = NULL;
		fsa_release(dir);
		state.printerToGifFile = 0;
		StrCopy(msg, "Error creating GIF file \"");
		StrCat(msg, giffile_name);
		StrCat(msg, "\".\nPrinting to GIF file disabled.");
		show_message(msg);
		return;
	    }
	}
	fsa_release(dir);
	err = fsa_open(giffile, FSA_MODE_READWRITE);
	if (err != FSA_ERR_NONE) {
	    fsa_delete(giffile);
	    giffile = NULL;
	    state.printerToGifFile = 0;
	    StrCopy(msg, "Error opening GIF file \"");
	    StrCat(msg, giffile_name);
	    StrCat(msg, "\".\nPrinting to GIF file disabled.");
	    show_message(msg);
	    return;
	}
	if (!shell_start_gif(gif_writer, state.printerGifMaxLength)) {
	    state.printerToGifFile = 0;
	    show_message("Not enough memory for the GIF encoder.\n"
			 "Printing to GIF file disabled.");
	    return;
	}
	gif_lines = 0;
    }

    shell_spool_gif(bits, bytesperline, x, y, width, height, gif_writer);
    gif_lines += height;
}

void close_gif(int reset_sequence) {
    if (giffile != NULL) {
	shell_finish_gif(gif_seeker, gif_writer);
	fsa_release(giffile);
	giffile = NULL;
	if (reset_sequence)
	    gifseq = -1;
    }
}


/**********************************/
/***** Select Programs dialog *****/
/**********************************/

static struct {
    TableType *table;
    int nprogs;
    Int16 tbl_rows;
    int scl_max;
    int scl_oldval;
    int *prog_sel;
    char **prog_name;
} selprog;

static void text_cell_renderer(void *tablep, Int16 row, Int16 column,
					    RectangleType *bounds) SHELL2_SECT;
static void text_cell_renderer(void *tablep, Int16 row, Int16 column,
					    RectangleType *bounds) {
    TableType *table = (TableType *) tablep;
    char *text;
    FontID fid = FntGlueGetDefaultFontID(defaultSystemFont);
    FontID prevFid = FntSetFont(fid);
    
    text = (char *) MyGlueTblGetItemPtr(table, row, column);
    WinEraseRectangle(bounds, 0);
    WinGlueDrawTruncChars(text, StrLen(text), bounds->topLeft.x,
					  bounds->topLeft.y, bounds->extent.x);

    FntSetFont(prevFid);
}

static Boolean selectprogram_handler(EventType *e) SHELL2_SECT;
static Boolean selectprogram_handler(EventType *e) {
    switch (e->eType) {
	case sclRepeatEvent: {
	    ScrollBarType *sb = e->data.sclRepeat.pScrollBar;
	    Int16 sbvalue, sbmin, sbmax, sbpagesize;
	    int i;
	    SclGetScrollBar(sb, &sbvalue, &sbmin, &sbmax, &sbpagesize);
	    for (i = 0; i < selprog.tbl_rows; i++)
		selprog.prog_sel[i + selprog.scl_oldval] = TblGetItemInt(selprog.table, i, 0);
	    for (i = 0; i < selprog.tbl_rows; i++) {
		TblSetItemInt(selprog.table, i, 0, selprog.prog_sel[i + sbvalue]);
		TblSetItemPtr(selprog.table, i, 1, selprog.prog_name[i + sbvalue]);
	    }
	    selprog.scl_oldval = sbvalue;
	    TblDrawTable(selprog.table);
	    /* If I return 'true' here, the scroll bar won't work properly. */
	    return false;
	}

	case tblEnterEvent: {
	    /* We don't want tblSelectEvents, because then the table row
	     * will get highlighted, which we don't want -- we use a
	     * checkbox instead. So, by handling the enter event and returning
	     * 'true', we can operate the check box and prevent the selection.
	     */
	    int trow = e->data.tblSelect.row;
	    int prow = trow + selprog.scl_oldval;
	    int v = !TblGetItemInt(selprog.table, trow, 0);
	    TblSetItemInt(selprog.table, trow, 0, v);
	    selprog.prog_sel[prow] = v;
	    TblMarkRowInvalid(selprog.table, trow);
	    TblRedrawTable(selprog.table);
	    return true;
	}

	default:
	    return false;
    }
}


/*******************************/
/***** Import/Export stuff *****/
/*******************************/

static fsa_obj *export_file = NULL;
static fsa_obj *import_file = NULL;

#ifndef PALMOS_ARM_SHELL

static ProgressPtr progress;

static Boolean progress_text_cb(PrgCallbackDataPtr cb) SHELL2_SECT;
static Boolean progress_text_cb(PrgCallbackDataPtr cb) {
    StrNCopy(cb->textP, cb->message, cb->textLen - 1);
    cb->textP[cb->textLen - 1] = 0;
    return true;
}

static int progress_report_cb(const char *message) SHELL2_SECT;
static int progress_report_cb(const char *message) {
    if (message != NULL)
	PrgUpdateDialog(progress, errNone, 0, message, true);
    EventType event;
    while (EvtGetEvent(&event, 0), event.eType != nilEvent)
	if (!PrgHandleEvent(progress, &event))
	    if (PrgUserCancel(progress))
		return 1;
    return 0;
}

#endif

static void do_import() {
    char path[FILENAMELEN];
    int err;

    path[0] = 0;
    if (!select_file("Import Program", "Open", "Program Files (*.raw)\0raw\0All Files (*.*)\0*\0\0", path, FILENAMELEN))
	return;

    err = fsa_resolve(path, &import_file, 0, NULL);
    if (err == FSA_ERR_VOLUME_NOT_FOUND) {
	show_message("Can't find volume.");
	return;
    } else if (err != FSA_ERR_NONE) {
	show_message("Can't open file for reading.");
	return;
    }
    if (fsa_open(import_file, FSA_MODE_READONLY) != FSA_ERR_NONE) {
	fsa_release(import_file);
	import_file = NULL;
	show_message("Can't open file for reading.");
	return;
    }

#ifdef PALMOS_ARM_SHELL
    // ARM machines are fast enough that there's no point in presenting a
    // progress dialog...
    core_import_programs(NULL);
#else
    // ...but 68k machines are not.
    progress = PrgStartDialogV31("Importing Program", progress_text_cb);
    PrgUpdateDialog(progress, errNone, 0, "Importing...", true);
    core_import_programs(progress_report_cb);
    PrgStopDialog(progress, true);
#endif

    redisplay();
    repaint_annunciator(0);

    if (import_file != NULL) {
	fsa_release(import_file);
	import_file = NULL;
    }
}

static void do_export() {
    FormType *form = FrmInitForm(selectprogram_id);
    UInt16 tbl_index = FrmGetObjectIndex(form, progtable_id);
    TableType *table =
		(TableType *) FrmGetObjectPtr(form, tbl_index);
    UInt16 scl_index = FrmGetObjectIndex(form, progscroll_id);
    ScrollBarType *sb =
	    (ScrollBarType *) FrmGetObjectPtr(form, scl_index);
    char buf[1000], *p;
    int np, nt, i;
    char path[FILENAMELEN], basename[FILENAMELEN];
    fsa_obj *dir;
    int err, file_existed;

    FrmSetEventHandler(form, selectprogram_handler);

    selprog.table = table;
    nt = TblGetNumberOfRows(table);
    selprog.nprogs = np = core_list_programs(buf, 1000);
    selprog.tbl_rows = np < nt ? np : nt;
    selprog.scl_max = np - selprog.tbl_rows;
    selprog.scl_oldval = 0;

    selprog.prog_sel = (int *) MemPtrNew(np * sizeof(int));
    selprog.prog_name = (char **) MemPtrNew(np * sizeof(char *));
    p = buf;
    for (i = 0; i < selprog.nprogs; i++) {
	selprog.prog_sel[i] = 0;
	selprog.prog_name[i] = p;
	p += StrLen(p) + 1;
    }

    TblSetColumnUsable(table, 0, true);
    TblSetColumnUsable(table, 1, true);
    TblSetCustomDrawProcedure(table, 1, text_cell_renderer);
    SclSetScrollBar(sb, 0, 0, selprog.nprogs - selprog.tbl_rows,
					selprog.tbl_rows - 1);

    for (i = 0; i < selprog.tbl_rows; i++) {
	TblInsertRow(table, i);
	TblSetItemStyle(table, i, 0, checkboxTableItem);
	TblSetItemInt(table, i, 0, 0);
	TblSetItemStyle(table, i, 1, customTableItem);
	TblSetItemPtr(table, i, 1, selprog.prog_name[i]);
	TblSetRowUsable(table, i, true);
    }

    for (i = selprog.tbl_rows; i < nt; i++)
	TblSetRowUsable(table, i, false);

    FrmShowObject(form, tbl_index);
    if (FrmDoDialog(form) != ok_id)
	goto done_export;

    np = 0;
    for (i = 0; i < selprog.nprogs; i++) {
	if (selprog.prog_sel[i])
	    selprog.prog_sel[np++] = i;
    }
    if (np == 0)
	/* Nothing selected */
	goto done_export;

    path[0] = 0;
    if (!select_file("Export Program", "Save", "Program Files (*.raw)\0raw\0All Files (*.*)\0*\0\0", path, FILENAMELEN))
	goto done_export;

    err = fsa_resolve(path, &dir, 1, basename);
    if (err == FSA_ERR_VOLUME_NOT_FOUND) {
	show_message("Can't find volume.");
	goto done_export;
    } else if (err != FSA_ERR_NONE) {
	show_message("Can't find directory.");
	goto done_export;
    }
    err = fsa_create(dir, basename, &export_file);
    fsa_release(dir);
    file_existed = err == FSA_ERR_FILE_EXISTS;
    if (err != FSA_ERR_NONE && err != FSA_ERR_FILE_EXISTS) {
	show_message("Can't create file.");
	goto done_export;
    }
    err = fsa_open(export_file, FSA_MODE_WRITEONLY);
    if (err != FSA_ERR_NONE) {
	if (file_existed)
	    fsa_release(export_file);
	else
	    fsa_delete(export_file);
	export_file = NULL;
	show_message("Can't open file for writing.");
	goto done_export;
    }

#ifdef PALMOS_ARM_SHELL
    // ARM machines are fast enough that there's no point in presenting a
    // progress dialog...
    core_export_programs(np, selprog.prog_sel, NULL);
    if (export_file != NULL) {
	fsa_release(export_file);
	export_file = NULL;
    }
#else
    // ...but 68k machines are not.
    progress = PrgStartDialogV31("Exporting Program", progress_text_cb);
    PrgUpdateDialog(progress, errNone, 0, "Exporting...", true);
    int cancelled;
    cancelled = core_export_programs(np, selprog.prog_sel, progress_report_cb);
    PrgStopDialog(progress, true);
    if (export_file != NULL) {
	if (cancelled)
	    fsa_delete(export_file);
	else
	    fsa_release(export_file);
	export_file = NULL;
    }
#endif

    done_export:
    FrmDeleteForm(form);
    MemPtrFree(selprog.prog_sel);
    MemPtrFree(selprog.prog_name);
}

static void do_clip_copy() {
    char buf[100];
    core_copy(buf, 100);
    ClipboardAddItem(clipboardText, buf, StrLen(buf));
}

static void do_clip_paste() {
    UInt16 len;
    char buf[100];
    MemHandle h;
    char *p;

    h = ClipboardGetItem(clipboardText, &len);
    if (h == NULL)
	return;
    p = (char *) MemHandleLock(h);
    if (len > 99)
	len = 99;

    MemMove(buf, p, len);
    buf[len] = 0;
    core_paste(buf);

    redisplay();
    MemHandleUnlock(h);
}

static void do_delete_skin() {
    int bufsize = 5000;
    char *buf = (char *) MemPtrNew(bufsize);
    char *p = buf;
    int len = 0;
    DmSearchStateType search_state;
    Boolean new_search = true;
    UInt16 card;
    LocalID dbid;
    int i, j, k;
    int nskins = 0;
    char **namelist = NULL;
    FormType *form;
    int idx;
    ListType *list;

    if (buf == NULL)
	ErrFatalDisplayIf(1, "Out of memory while building skin list.");

    while (DmGetNextDatabaseByTypeCreator(new_search, &search_state,
		'Skin', 'Fk42', false, &card, &dbid) == errNone) {
	char name[32];
	new_search = false;
	if (DmDatabaseInfo(card, dbid, name, NULL, NULL, NULL, NULL,
		    NULL, NULL, NULL, NULL, NULL, NULL) == errNone) {
	    int namelen = StrLen(name) + 1;
	    if (len + namelen <= bufsize) {
		StrCopy(p, name);
		p += namelen;
		len += namelen;
		nskins++;
	    }
	}
    }

    if (nskins > 0) {
	namelist = (char **) MemPtrNew(len + nskins * sizeof(char *));
	if (namelist == NULL)
	    ErrFatalDisplayIf(1, "Out of memory while building skin list.");
	MemMove(namelist + nskins, buf, len);
    }
    MemPtrFree(buf);
    p = (char *) (namelist + nskins);
    for (i = 0; i < nskins; i++) {
	namelist[i] = p;
	p += StrLen(p) + 1;
    }
    SysQSort(namelist, nskins, sizeof(char *), str_ptr_compare, 0);

    form = FrmInitForm(deleteskin_id);
    idx = FrmGetObjectIndex(form, skin_list_id);
    list = (ListType *) FrmGetObjectPtr(form, idx);
    LstSetListChoices(list, namelist, nskins);
    if (FrmDoDialog(form) == ok_id) {
	i = LstGetSelection(list);
	if (i >= 0 && i < nskins) {
	    /* If we're deleting the currently active skin, switch to the
	     * default skin (the first item in the skin_name array)
	     * before proceeding.
	     */
	    if (StrCaselessCompare(namelist[i], state.skinName) == 0) {
		StrCopy(state.skinName, skin_name[0]);
		unload_skin();
		load_skin();
		core_repaint_display();
		update_dia_state();
		if (feature_set_3_5_present())
		    FrmUpdateForm(calcform_id, frmRedrawUpdateCode);
		else {
		    /* The FrmUpdateForm() call doesn't do the job
		     * when I'm testing with PalmOS 3.3... So,
		     * brute force.
		     */
		    if (FrmGetActiveFormID() == calcform_id)
			calcgadget_handler(NULL, formGadgetDrawCmd, NULL);
		}
	    }

	    /* Delete the PRC */
	    dbid = DmFindDatabase(0, namelist[i]);
	    DmDeleteDatabase(0, dbid);

	    /* Remove the skin from the skin_name array.
	     * It isn't necessary to hide the menu item from the dynamic menu,
	     * since the forms containing that menu aren't active at the
	     * moment, so the menu will be rebuilt anyway.
	     */
	    for (j = builtin_skins; j < builtin_skins + external_skins; j++) {
		if (StrCaselessCompare(namelist[i], skin_name[j]) != 0)
		    continue;
		/* Remove the name from the skin_name array */
		for (k = j; k < builtin_skins + external_skins - 1; k++)
		    skin_name[k] = skin_name[k + 1];
		external_skins--;
		break;
	    }
	}
    }

    if (namelist != NULL)
	MemPtrFree(namelist);
    FrmDeleteForm(form);
}

int shell_write(const char *buf, int4 buflen) {
    uint4 n = (uint4) buflen;
    int err;
    if (export_file == NULL)
	return 0;
    err = fsa_write(export_file, buf, &n);
    if (err != FSA_ERR_NONE || n != (uint4) buflen) {
	show_message("Error writing file.");
	fsa_delete(export_file);
	export_file = NULL;
	return 0;
    } else
	return 1;
}

int4 shell_read(char *buf, int4 buflen) {
    UInt32 n = buflen;
    int err;
    if (import_file == NULL)
	return -1;
    err = fsa_read(import_file, buf, &n);
    if (err != FSA_ERR_NONE) {
	show_message("Error reading file.");
	fsa_release(import_file);
	import_file = NULL;
	return -1;
    } else
	return n;
}


/*************************/
/***** Utility stuff *****/
/*************************/

static void do_copy() {
    fsa_obj *src, *dst, *dstdir;
    UInt32 sz, cnt;
    char *buf, path[FILENAMELEN], srcpath[FILENAMELEN];
    int err;
    int we_created;

    path[0] = 0;
    if (!select_file("Copy From...", "Select",
		"All Files (*.*)\0*\0\0", path, FILENAMELEN))
	return;
    StrCopy(srcpath, path);
    if (fsa_resolve(path, &src, 0, NULL) != FSA_ERR_NONE) {
	show_message("Could not find source file.");
	return;
    }
    if (fsa_open(src, FSA_MODE_READONLY) != FSA_ERR_NONE) {
	fsa_release(src);
	show_message("Could not open source file.");
	return;
    }
    if (!select_file("Copy To...", "Select",
		"All Files (*.*)\0*\0\0", path, FILENAMELEN)) {
	fsa_release(src);
	return;
    }
    if (StrCaselessCompare(path, srcpath) == 0) {
	fsa_release(src);
	show_message("Source and destination are the same file.");
	return;
    }
    if (fsa_resolve(path, &dstdir, 1, path) != FSA_ERR_NONE) {
	fsa_release(src);
	show_message("Could not find destination directory.");
	return;
    }
    err = fsa_create(dstdir, path, &dst);
    fsa_release(dstdir);
    if (err != FSA_ERR_NONE && err != FSA_ERR_FILE_EXISTS) {
	fsa_release(src);
	show_message("Could not create destination file.");
	return;
    }
    we_created = err == FSA_ERR_NONE;
    if (fsa_open(dst, FSA_MODE_WRITEONLY) != FSA_ERR_NONE) {
	fsa_release(src);
	if (we_created)
	    fsa_delete(dst);
	else
	    fsa_release(dst);
	show_message("Could not open destination file.");
	return;
    }

    /* Try to allocate a nice big buffer. If it fails, try halving the
     * size until we succeed. If we still fail at 512 bytes, use the
     * 'path' local instead.
     */
    sz = 8192;
    while (sz > 256) {
	buf = (char *) MemPtrNew(sz);
	if (buf != NULL)
	    break;
	sz /= 2;
    }
    if (buf == NULL) {
	buf = path;
	sz = FILENAMELEN;
    }
    while (1) {
	cnt = sz;
	if (fsa_read(src, buf, &cnt) != FSA_ERR_NONE)
	    goto failed;
	if (cnt == 0)
	    break;
	if (fsa_write(dst, buf, &cnt) != FSA_ERR_NONE)
	    goto failed;
    }
    fsa_release(src);
    fsa_release(dst);
    if (buf != path)
	MemPtrFree(buf);
    show_message("File copied.");
    return;

    failed:
    fsa_release(src);
    fsa_delete(dst);
    if (buf != path)
	MemPtrFree(buf);
    show_message("Copy failed.");
}


static void do_delete() {
    fsa_obj *file;
    char path[FILENAMELEN];

    path[0] = 0;
    if (!select_file("Delete File...", "Delete",
		"All Files (*.*)\0*\0\0", path, FILENAMELEN))
	return;
    if (fsa_resolve(path, &file, 0, NULL) != FSA_ERR_NONE) {
	show_message("Could not find file to delete.");
	return;
    }
    fsa_delete(file);
    if (fsa_resolve(path, &file, 0, NULL) == FSA_ERR_NONE) {
	fsa_release(file);
	show_message("Delete failed.");
    } else {
	show_message("File deleted.");
    }
}

static void do_removedir() {
    fsa_obj *dir;
    char path[FILENAMELEN];

    path[0] = 0;
    if (!select_dir("Remove Directory...", "RmDir",
		"All Files (*.*)\0*\0\0", path, FILENAMELEN))
	return;
    if (fsa_resolve(path, &dir, 0, NULL) != FSA_ERR_NONE) {
	show_message("Could not find directory to remove.");
	return;
    }
    fsa_delete(dir);
    if (fsa_resolve(path, &dir, 0, NULL) == FSA_ERR_NONE) {
	fsa_release(dir);
	show_message("Remove failed.");
    } else {
	show_message("Directory removed.");
    }
}
