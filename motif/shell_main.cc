/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2006  Thomas Okken
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

#include <Xm/Xm.h>
#include <Xm/AtomMgr.h>
#include <Xm/CascadeB.h>
#include <Xm/DrawingA.h>
#include <Xm/FileSB.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/MessageB.h>
#include <Xm/MwmUtil.h>
#include <Xm/Protocols.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/Separator.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <X11/Xmu/Editres.h>
#define XK_MISCELLANY 1
#include <X11/keysymdef.h>
#include <X11/xpm.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include "shell.h"
#include "shell_main.h"
#include "shell_skin.h"
#include "shell_spool.h"
#include "core_main.h"
#include "core_display.h"
#include "icon.xpm"


/* public globals */

Display *display;
Screen *screen;
int screennumber;
Window rootwindow;
Visual *visual;
Colormap colormap;
int depth;
GC gc;
unsigned long black, white;

/* These are global because the skin code uses them a lot */

Widget calc_widget;
Window calc_canvas;
int allow_paint = 0;

state_type state;
char free42dirname[FILENAMELEN];


/* Some X error handling stuff */

static int x_errors_coredump = 0;

typedef struct {
    const char *name;
    int value;
} req_code;

/* From Xproto.h */

static req_code req_code_list[] = {
    { "X_CreateWindow",                  1 },
    { "X_ChangeWindowAttributes",        2 },
    { "X_GetWindowAttributes",           3 },
    { "X_DestroyWindow",                 4 },
    { "X_DestroySubwindows",             5 },
    { "X_ChangeSaveSet",                 6 },
    { "X_ReparentWindow",                7 },
    { "X_MapWindow",                     8 },
    { "X_MapSubwindows",                 9 },
    { "X_UnmapWindow",                  10 },
    { "X_UnmapSubwindows",              11 },
    { "X_ConfigureWindow",              12 },
    { "X_CirculateWindow",              13 },
    { "X_GetGeometry",                  14 },
    { "X_QueryTree",                    15 },
    { "X_InternAtom",                   16 },
    { "X_GetAtomName",                  17 },
    { "X_ChangeProperty",               18 },
    { "X_DeleteProperty",               19 },
    { "X_GetProperty",                  20 },
    { "X_ListProperties",               21 },
    { "X_SetSelectionOwner",            22 },
    { "X_GetSelectionOwner",            23 },
    { "X_ConvertSelection",             24 },
    { "X_SendEvent",                    25 },
    { "X_GrabPointer",                  26 },
    { "X_UngrabPointer",                27 },
    { "X_GrabButton",                   28 },
    { "X_UngrabButton",                 29 },
    { "X_ChangeActivePointerGrab",      30 },
    { "X_GrabKeyboard",                 31 },
    { "X_UngrabKeyboard",               32 },
    { "X_GrabKey",                      33 },
    { "X_UngrabKey",                    34 },
    { "X_AllowEvents",                  35 },
    { "X_GrabServer",                   36 },
    { "X_UngrabServer",                 37 },
    { "X_QueryPointer",                 38 },
    { "X_GetMotionEvents",              39 },
    { "X_TranslateCoords",              40 },
    { "X_WarpPointer",                  41 },
    { "X_SetInputFocus",                42 },
    { "X_GetInputFocus",                43 },
    { "X_QueryKeymap",                  44 },
    { "X_OpenFont",                     45 },
    { "X_CloseFont",                    46 },
    { "X_QueryFont",                    47 },
    { "X_QueryTextExtents",             48 },
    { "X_ListFonts",                    49 },
    { "X_ListFontsWithInfo",            50 },
    { "X_SetFontPath",                  51 },
    { "X_GetFontPath",                  52 },
    { "X_CreatePixmap",                 53 },
    { "X_FreePixmap",                   54 },
    { "X_CreateGC",                     55 },
    { "X_ChangeGC",                     56 },
    { "X_CopyGC",                       57 },
    { "X_SetDashes",                    58 },
    { "X_SetClipRectangles",            59 },
    { "X_FreeGC",                       60 },
    { "X_ClearArea",                    61 },
    { "X_CopyArea",                     62 },
    { "X_CopyPlane",                    63 },
    { "X_PolyPoint",                    64 },
    { "X_PolyLine",                     65 },
    { "X_PolySegment",                  66 },
    { "X_PolyRectangle",                67 },
    { "X_PolyArc",                      68 },
    { "X_FillPoly",                     69 },
    { "X_PolyFillRectangle",            70 },
    { "X_PolyFillArc",                  71 },
    { "X_PutImage",                     72 },
    { "X_GetImage",                     73 },
    { "X_PolyText8",                    74 },
    { "X_PolyText16",                   75 },
    { "X_ImageText8",                   76 },
    { "X_ImageText16",                  77 },
    { "X_CreateColormap",               78 },
    { "X_FreeColormap",                 79 },
    { "X_CopyColormapAndFree",          80 },
    { "X_InstallColormap",              81 },
    { "X_UninstallColormap",            82 },
    { "X_ListInstalledColormaps",       83 },
    { "X_AllocColor",                   84 },
    { "X_AllocNamedColor",              85 },
    { "X_AllocColorCells",              86 },
    { "X_AllocColorPlanes",             87 },
    { "X_FreeColors",                   88 },
    { "X_StoreColors",                  89 },
    { "X_StoreNamedColor",              90 },
    { "X_QueryColors",                  91 },
    { "X_LookupColor",                  92 },
    { "X_CreateCursor",                 93 },
    { "X_CreateGlyphCursor",            94 },
    { "X_FreeCursor",                   95 },
    { "X_RecolorCursor",                96 },
    { "X_QueryBestSize",                97 },
    { "X_QueryExtension",               98 },
    { "X_ListExtensions",               99 },
    { "X_ChangeKeyboardMapping",       100 },
    { "X_GetKeyboardMapping",          101 },
    { "X_ChangeKeyboardControl",       102 },
    { "X_GetKeyboardControl",          103 },
    { "X_Bell",                        104 },
    { "X_ChangePointerControl",        105 },
    { "X_GetPointerControl",           106 },
    { "X_SetScreenSaver",              107 },
    { "X_GetScreenSaver",              108 },
    { "X_ChangeHosts",                 109 },
    { "X_ListHosts",                   110 },
    { "X_SetAccessControl",            111 },
    { "X_SetCloseDownMode",            112 },
    { "X_KillClient",                  113 },
    { "X_RotateProperties",            114 },
    { "X_ForceScreenSaver",            115 },
    { "X_SetPointerMapping",           116 },
    { "X_GetPointerMapping",           117 },
    { "X_SetModifierMapping",          118 },
    { "X_GetModifierMapping",          119 },
    { "X_NoOperation",                 127 },
    { NULL,                             -1 }
};


/* Looks like I can't make PRINT_LINES bigger than 32k, because of a size
 * limit on the XmDrawingArea widget. I guess I'll have to do explicit
 * scrolling if I want to be able to display a bigger buffer.
 */
#define PRINT_LINES 32768
#define PRINT_BYTESPERLINE 36
#define PRINT_SIZE 1179648


static int printout_top;
static int printout_bottom;
static int quit_flag = 0;
static int enqueued;


/* private globals */

static FILE *print_txt = NULL;
static FILE *print_gif = NULL;
static char print_gif_name[FILENAMELEN];
static int gif_seq = -1;
static int gif_lines;

static XtAppContext appcontext;
static Widget appshell, mainwindow, printwindow, print_da, print_sb;
static Widget prefsdialog = NULL, prefs_matrix_singularmatrix,
		prefs_matrix_outofrange,
		prefs_printer_txt, prefs_printer_txt_name, prefs_raw_text,
		prefs_printer_gif, prefs_printer_gif_name,
		prefs_printer_gif_height;
static Widget printer_txt_dialog = NULL;
static Widget printer_gif_dialog = NULL;
static Widget program_select_dialog = NULL, program_list;
static Widget export_dialog = NULL;
static char export_file_name[FILENAMELEN];
static FILE *export_file = NULL;
static Widget overwrite_confirm_dialog = NULL;
static Widget import_dialog = NULL;
static FILE *import_file = NULL;
static Pixmap icon, iconmask;

static Window print_canvas;
static int ckey = 0;
static int skey;
static unsigned char *macro;
static int mouse_key;
static unsigned int active_keycode = 0;
static int just_pressed_shift = 0;
static int timeout_active = 0;
static XtIntervalId timeout_id;
static int timeout3_active = 0;
static XtIntervalId timeout3_id;
static XImage *print_image;

static int keymap_length = 0;
static keymap_entry *keymap = NULL;

static int reminder_active = 0;
static XtWorkProcId reminder_id;
static FILE *statefile = NULL;
static char statefilename[FILENAMELEN];
static char printfilename[FILENAMELEN];
static XtSignalId term_sig_id;

static int print_repaint_pending = 0;
static XtWorkProcId print_repaint_id;

static int ann_updown = 0;
static int ann_shift = 0;
static int ann_print = 0;
static int ann_run = 0;
static int ann_battery = 0;
static int ann_g = 0;
static int ann_rad = 0;


/* private functions */

static void read_key_map(const char *keymapfilename);
static void init_shell_state(int4 version);
static int read_shell_state(int4 *version);
static int write_shell_state();
static int x_error_handler(Display *display, XErrorEvent *event);
static int x_io_error_handler(Display *display);
static void int_term_handler(int sig);
static void xt_int_term_handler(XtPointer closure, XtSignalId *id);
static void quit();
static char *strclone(const char *s);
static int is_file(const char *name);
static void show_message(char *title, char *message);
static void no_gnome_resize(Widget w);
static void scroll_printout_to_bottom();
static int get_window_pos(Widget w, int *x, int *y);
static void quitCB(Widget w, XtPointer ud, XtPointer cd);
static void showPrintOutCB(Widget w, XtPointer ud, XtPointer cd);
static void exportProgramCB(Widget w, XtPointer ud, XtPointer cd);
static void make_program_select_dialog();
static Widget make_file_select_dialog(const char *title, const char *pattern,
				Widget owner,
				XtCallbackProc callback, XtPointer closure);
static void selProgButtonCB(Widget w, XtPointer ud, XtPointer cd);
static void exportFileCB(Widget w, XtPointer ud, XtPointer cd);
static void do_export(Widget w, XtPointer ud, XtPointer cd);
static void importProgramCB(Widget w, XtPointer ud, XtPointer cd);
static void importFileCB(Widget w, XtPointer ud, XtPointer cd);
static void clearPrintOutCB(Widget w, XtPointer ud, XtPointer cd);
static void preferencesCB(Widget w, XtPointer ud, XtPointer cd);
static void make_prefs_dialog();
static void prefsButtonCB(Widget w, XtPointer ud, XtPointer cd);
static void setFilePath(Widget w, char *s);
static void appendSuffix(char *path, char *suffix);
static void printerBrowseTxtCB(Widget w, XtPointer ud, XtPointer cd);
static void printerBrowseGifCB(Widget w, XtPointer ud, XtPointer cd);
static void copyCB(Widget w, XtPointer ud, XtPointer cd);
static void pasteCB(Widget w, XtPointer ud, XtPointer cd);
static void aboutCB(Widget w, XtPointer ud, XtPointer cd);
static void delete_cb(Widget w, XtPointer ud, XtPointer cd);
static void delete_print_cb(Widget w, XtPointer ud, XtPointer cd);
static void expose_cb(Widget w, XtPointer ud, XtPointer cd);
static void print_expose_cb(Widget w, XtPointer ud, XtPointer cd);
static void print_graphicsexpose_cb(Widget w, XtPointer ud, XEvent *event,
								Boolean *cont);
static Boolean print_repaint(XtPointer closure);
static void input_cb(Widget w, XtPointer ud, XtPointer cd);
static void enable_reminder();
static void disable_reminder();
static void repeater(XtPointer closure, XtIntervalId *id);
static void timeout1(XtPointer closure, XtIntervalId *id);
static void timeout2(XtPointer closure, XtIntervalId *id);
static void timeout3(XtPointer closure, XtIntervalId *id);
static void battery_checker(XtPointer closure, XtIntervalId *id);
static void repaint_printout(int x, int y, int width, int height);
static Boolean reminder(XtPointer closure);
static void txt_writer(const char *text, int length);
static void txt_newliner();
static void gif_seeker(int4 pos);
static void gif_writer(const char *text, int length);


int main(int argc, char *argv[]) {
    struct stat st;
    int free42dir_exists;
    char *home;
    Arg args[12];
    int nargs = 0, n;
    Widget form, menubar, menu, button, w, help;
    XmString s, s2;
    XtTranslations translations;
    XGCValues values;
    FILE *printfile;
    Widget scroll;
    FILE *apm;
    struct sigaction act;
    int4 version;
    int init_mode;
    int win_width, win_height;
    char keymapfilename[FILENAMELEN];
    char *skin_arg = NULL;


    /*************************************/
    /***** X-related initializations *****/
    /*************************************/

    XtSetLanguageProc(NULL, NULL, NULL);
    appshell = XtVaAppInitialize(&appcontext,	/* application context */
				 "Free42",	/* class name */
				 NULL, 0,	/* cmd line option descr. */
				 &argc, argv,	/* cmd line args */
				 NULL,		/* fallback resources */
				 NULL);		/* end of varargs list */

    display = XtDisplay(appshell);
    screen = XtScreen(appshell);
    screennumber = 0;
    while (screen != ScreenOfDisplay(display, screennumber))
	screennumber++;
    rootwindow = RootWindowOfScreen(screen);

    visual = DefaultVisual(display, screennumber);
    colormap = DefaultColormap(display, screennumber);
    depth = DefaultDepth(display, screennumber);
    black = BlackPixel(display, screennumber);
    white = WhitePixel(display, screennumber);

    for (n = 1; n < argc; n++) {
	if (strcmp(argv[n], "-xdump") == 0 && !x_errors_coredump) {
	    fprintf(stderr, "Dumping core on X errors.\n");
	    x_errors_coredump = 1;
	    XSynchronize(display, True);
	    XSetIOErrorHandler(x_io_error_handler);
	} else if (strcmp(argv[n], "-skin") == 0 && n < argc - 1) {
	    skin_arg = argv[++n];
	}
    }

    XSetErrorHandler(x_error_handler);

    values.foreground = black;
    values.background = white;
    gc = XCreateGC(display,
		   rootwindow,
		   GCForeground | GCBackground,
		   &values);


    /*****************************************************/
    /***** Try to create the $HOME/.free42 directory *****/
    /*****************************************************/

    free42dir_exists = 0;
    home = getenv("HOME");
    snprintf(free42dirname, FILENAMELEN, "%s/.free42", home);
    if (stat(free42dirname, &st) == -1 || !S_ISDIR(st.st_mode)) {
	mkdir(free42dirname, 0755);
	if (stat(free42dirname, &st) == 0 && S_ISDIR(st.st_mode)) {
	    char oldpath[FILENAMELEN], newpath[FILENAMELEN];
	    free42dir_exists = 1;
	    /* Note that we only rename the .free42rc and .free42print files
	     * if we have just created the .free42 directory; if the user
	     * creates the .free42 directory manually, they also have to take
	     * responsibility for the old-style state and print files; I don't
	     * want to do any second-guessing here.
	     */
	    snprintf(oldpath, FILENAMELEN, "%s/.free42rc", home);
	    snprintf(newpath, FILENAMELEN, "%s/.free42/state", home);
	    rename(oldpath, newpath);
	    snprintf(oldpath, FILENAMELEN, "%s/.free42print", home);
	    snprintf(newpath, FILENAMELEN, "%s/.free42/print", home);
	    rename(oldpath, newpath);
	    snprintf(oldpath, FILENAMELEN, "%s/.free42keymap", home);
	    snprintf(newpath, FILENAMELEN, "%s/.free42/keymap", home);
	    rename(oldpath, newpath);
	}
    } else
	free42dir_exists = 1;

    if (free42dir_exists) {
	snprintf(statefilename, FILENAMELEN, "%s/.free42/state", home);
	snprintf(printfilename, FILENAMELEN, "%s/.free42/print", home);
	snprintf(keymapfilename, FILENAMELEN, "%s/.free42/keymap", home);
    } else {
	snprintf(statefilename, FILENAMELEN, "%s/.free42rc", home);
	snprintf(printfilename, FILENAMELEN, "%s/.free42print", home);
	snprintf(keymapfilename, FILENAMELEN, "%s/.free42keymap", home);
    }

    
    /****************************/
    /***** Read the key map *****/
    /****************************/

    read_key_map(keymapfilename);


    /***********************************************************/
    /***** Open the state file and read the shell settings *****/
    /***********************************************************/

    statefile = fopen(statefilename, "r");
    if (statefile != NULL) {
	if (read_shell_state(&version)) {
	    if (skin_arg != NULL) {
		strncpy(state.skinName, skin_arg, FILENAMELEN - 1);
		state.skinName[FILENAMELEN - 1] = 0;
	    }
	    init_mode = 1;
	} else {
	    init_shell_state(-1);
	    init_mode = 2;
	}
    } else {
	init_shell_state(-1);
	init_mode = 0;
    }


    /*********************************/
    /***** Build the main window *****/
    /*********************************/

    XpmCreatePixmapFromData(display,
			    rootwindow,
			    icon_xpm,
			    &icon,
			    &iconmask,
			    NULL);

#ifdef BCD_MATH
#define TITLE "Free42 Decimal"
#else
#define TITLE "Free42 Binary"
#endif

    XtSetArg(args[nargs], XmNtitle, TITLE); nargs++;
    XtSetArg(args[nargs], XmNmappedWhenManaged, False); nargs++;
    XtSetArg(args[nargs], XmNallowShellResize, True); nargs++;
    XtSetArg(args[nargs], XmNdeleteResponse, XmDO_NOTHING); nargs++;
    XtSetArg(args[nargs], XmNscreen, screen); nargs++;
    XtSetArg(args[nargs], XmNiconPixmap, icon); nargs++;
    XtSetArg(args[nargs], XmNiconMask, iconmask); nargs++;
    XtSetArg(args[nargs], XmNiconName, "Free42"); nargs++;
    XtSetArg(args[nargs], XmNmwmFunctions,
	    MWM_FUNC_ALL | MWM_FUNC_RESIZE | MWM_FUNC_MAXIMIZE); nargs++;
    XtSetArg(args[nargs], XmNmwmDecorations,
	    MWM_DECOR_ALL | MWM_DECOR_RESIZEH | MWM_DECOR_MAXIMIZE); nargs++;
    if (state.mainWindowKnown) {
	XtSetArg(args[nargs], XmNx, state.mainWindowX); nargs++;
	XtSetArg(args[nargs], XmNy, state.mainWindowY); nargs++;
    }
    mainwindow = XtAppCreateShell("free42", "Free42", topLevelShellWidgetClass,
				display, args, nargs);

    XmAddWMProtocolCallback(mainwindow,
			    XmInternAtom(display, "WM_DELETE_WINDOW", False),
			    delete_cb, NULL);

    XtAddEventHandler(mainwindow, 0, True,
		      (XtEventHandler) _XEditResCheckMessages,
		      NULL);

    translations = XtParseTranslationTable(
	    "<KeyDown>: DrawingAreaInput()\n"
	    "<KeyUp>: DrawingAreaInput()\n"
	    "<BtnDown>: DrawingAreaInput()\n"
	    "<BtnUp>: DrawingAreaInput()\n"
	    "<Motion>: DrawingAreaInput()\n");

    form = XtVaCreateManagedWidget("Form",
				   xmFormWidgetClass,
				   mainwindow,
				   XmNautoUnmanage, False,
				   XmNmarginHeight, 0,
				   XmNmarginWidth, 0,
				   NULL);

    /************/
    /* Menu bar */
    /************/

    XtSetArg(args[0], XmNtopAttachment, XmATTACH_FORM);
    XtSetArg(args[1], XmNtopOffset, 0);
    XtSetArg(args[2], XmNleftAttachment, XmATTACH_FORM);
    XtSetArg(args[3], XmNleftOffset, 0);
    XtSetArg(args[4], XmNrightAttachment, XmATTACH_FORM);
    XtSetArg(args[5], XmNrightOffset, 0);
    menubar = XmCreateMenuBar(form, "MenuBar", args, 6);
    XtManageChild(menubar);

    /*************/
    /* File menu */
    /*************/

    menu = XmCreatePulldownMenu(menubar, "FileMenu", NULL, 0);

    s = XmStringCreateLocalized("File");
    XtVaCreateManagedWidget("File",
			    xmCascadeButtonWidgetClass,
			    menubar,
			    XmNsubMenuId, menu,
			    XmNlabelString, s,
			    NULL);
    XmStringFree(s);

    s = XmStringCreateLocalized("Show Print-Out");
    button = XtVaCreateManagedWidget("ShowPrintOut",
				     xmPushButtonWidgetClass,
				     menu,
				     XmNlabelString, s,
				     NULL);
    XmStringFree(s);
    XtAddCallback(button, XmNactivateCallback, showPrintOutCB, NULL);

    XtCreateManagedWidget("Separator",
			  xmSeparatorWidgetClass,
			  menu,
			  NULL, 0);

    s = XmStringCreateLocalized("Import Program...");
    button = XtVaCreateManagedWidget("ImportProgram",
				     xmPushButtonWidgetClass,
				     menu,
				     XmNlabelString, s,
				     NULL);
    XmStringFree(s);
    XtAddCallback(button, XmNactivateCallback, importProgramCB, NULL);

    s = XmStringCreateLocalized("Export Program...");
    button = XtVaCreateManagedWidget("ExportProgram",
				     xmPushButtonWidgetClass,
				     menu,
				     XmNlabelString, s,
				     NULL);
    XmStringFree(s);
    XtAddCallback(button, XmNactivateCallback, exportProgramCB, NULL);

    XtCreateManagedWidget("Separator",
			  xmSeparatorWidgetClass,
			  menu,
			  NULL, 0);

    s = XmStringCreateLocalized("Clear Print-Out");
    button = XtVaCreateManagedWidget("ClearPrintOut",
				     xmPushButtonWidgetClass,
				     menu,
				     XmNlabelString, s,
				     NULL);
    XmStringFree(s);
    XtAddCallback(button, XmNactivateCallback, clearPrintOutCB, NULL);

    s = XmStringCreateLocalized("Preferences...");
    button = XtVaCreateManagedWidget("Preferences",
				     xmPushButtonWidgetClass,
				     menu,
				     XmNlabelString, s,
				     NULL);
    XmStringFree(s);
    XtAddCallback(button, XmNactivateCallback, preferencesCB, NULL);

    XtCreateManagedWidget("Separator",
			  xmSeparatorWidgetClass,
			  menu,
			  NULL, 0);

    s = XmStringCreateLocalized("Quit");
    s2 = XmStringCreateLocalized("Ctrl+Q");
    button = XtVaCreateManagedWidget("Quit",
				     xmPushButtonWidgetClass,
				     menu,
				     XmNlabelString, s,
				     XmNaccelerator, "Ctrl<Key>Q",
				     XmNacceleratorText, s2,
				     NULL);
    XmStringFree(s);
    XmStringFree(s2);
    XtAddCallback(button, XmNactivateCallback, quitCB, NULL);

    /*************/
    /* Edit menu */
    /*************/

    menu = XmCreatePulldownMenu(menubar, "EditMenu", NULL, 0);

    s = XmStringCreateLocalized("Edit");
    XtVaCreateManagedWidget("Edit",
			    xmCascadeButtonWidgetClass,
			    menubar,
			    XmNsubMenuId, menu,
			    XmNlabelString, s,
			    NULL);
    XmStringFree(s);
    
    s = XmStringCreateLocalized("Copy");
    s2 = XmStringCreateLocalized("Ctrl+C");
    button = XtVaCreateManagedWidget("Copy",
				     xmPushButtonWidgetClass,
				     menu,
				     XmNlabelString, s,
				     XmNaccelerator, "Ctrl<Key>C",
				     XmNacceleratorText, s2,
				     NULL);
    XmStringFree(s);
    XtAddCallback(button, XmNactivateCallback, copyCB, NULL);

    s = XmStringCreateLocalized("Paste");
    s2 = XmStringCreateLocalized("Ctrl+V");
    button = XtVaCreateManagedWidget("Paste",
				     xmPushButtonWidgetClass,
				     menu,
				     XmNlabelString, s,
				     XmNaccelerator, "Ctrl<Key>V",
				     XmNacceleratorText, s2,
				     NULL);
    XmStringFree(s);
    XtAddCallback(button, XmNactivateCallback, pasteCB, NULL);

    
    /*************/
    /* Skin menu */
    /*************/
    
    /* we only create the menu here, and add no items;
     * the items are added dynamically in the menu's map callback,
     * so that the menu always accurately reflects the set of skins
     * available in the .free42 directory.
     */
    XtSetArg(args[0], XmNradioBehavior, True);
    menu = XmCreatePulldownMenu(menubar, "SkinMenu", args, 1);
    XtAddCallback(menu, XmNmapCallback, skin_menu_update, NULL);

    s = XmStringCreateLocalized("Skin");
    XtVaCreateManagedWidget("Skin",
			    xmCascadeButtonWidgetClass,
			    menubar,
			    XmNsubMenuId, menu,
			    XmNlabelString, s,
			    NULL);
    XmStringFree(s);

    /*************/
    /* Help menu */
    /*************/

    menu = XmCreatePulldownMenu(menubar, "HelpMenu", NULL, 0);

    s = XmStringCreateLocalized("Help");
    help = XtVaCreateManagedWidget("Help",
				   xmCascadeButtonWidgetClass,
				   menubar,
				   XmNsubMenuId, menu,
				   XmNlabelString, s,
				   NULL);
    XmStringFree(s);
    XtVaSetValues(menubar, XmNmenuHelpWidget, help, NULL);

    s = XmStringCreateLocalized("About Free42...");
    button = XtVaCreateManagedWidget("About",
				     xmPushButtonWidgetClass,
				     menu,
				     XmNlabelString, s,
				     NULL);
    XmStringFree(s);
    XtAddCallback(button, XmNactivateCallback, aboutCB, NULL);

    /****************************************/
    /* Drawing area for the calculator skin */
    /****************************************/

    skin_load(&win_width, &win_height);

    w = XtVaCreateManagedWidget("DrawingArea",
				xmDrawingAreaWidgetClass,
				form,
				XmNwidth, win_width,
				XmNheight, win_height,
				XmNtranslations, translations,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopOffset, 0,
				XmNtopWidget, menubar,
				XmNleftAttachment, XmATTACH_FORM,
				XmNleftOffset, 0,
				XmNrightAttachment, XmATTACH_FORM,
				XmNrightOffset, 0,
				XmNbottomAttachment, XmATTACH_FORM,
				XmNbottomOffset, 0,
				NULL);

    XtAddCallback(w, XmNexposeCallback, expose_cb, NULL);
    XtAddCallback(w, XmNinputCallback, input_cb, NULL);
    XtRealizeWidget(mainwindow);
    calc_widget = w;
    calc_canvas = XtWindow(w);


    /**************************************/
    /***** Build the print-out window *****/
    /**************************************/

    print_image = XCreateImage(display, visual, 1, XYBitmap, 0, NULL,
			       286, PRINT_LINES, 8, 0);
    print_image->data = (char *) malloc(PRINT_SIZE);
    // TODO - handle memory allocation failure

    printfile = fopen(printfilename, "r");
    if (printfile != NULL) {
	n = fread(&printout_bottom, 1, sizeof(int), printfile);
	if (n == sizeof(int)) {
	    int bytes = printout_bottom * PRINT_BYTESPERLINE;
	    n = fread(print_image->data, 1, bytes, printfile);
	    if (n != bytes)
		printout_bottom = 0;
	} else
	    printout_bottom = 0;
	fclose(printfile);
    } else
	printout_bottom = 0;
    printout_top = 0;
    for (n = printout_bottom * PRINT_BYTESPERLINE; n < PRINT_SIZE; n++)
	print_image->data[n] = 0;

    nargs = 0;
    XtSetArg(args[nargs], XmNtitle, "Free42 Print-Out"); nargs++;
    XtSetArg(args[nargs], XmNmappedWhenManaged, False); nargs++;
    XtSetArg(args[nargs], XmNallowShellResize, True); nargs++;
    XtSetArg(args[nargs], XmNdeleteResponse, XmDO_NOTHING); nargs++;
    XtSetArg(args[nargs], XmNscreen, screen); nargs++;
    XtSetArg(args[nargs], XmNiconPixmap, icon); nargs++;
    XtSetArg(args[nargs], XmNiconMask, iconmask); nargs++;
    XtSetArg(args[nargs], XmNiconName, "Free42"); nargs++;
    if (state.printWindowKnown) {
	XtSetArg(args[nargs], XmNx, state.printWindowX); nargs++;
	XtSetArg(args[nargs], XmNy, state.printWindowY); nargs++;
	XtSetArg(args[nargs], XmNheight, state.printWindowHeight); nargs++;
    }
    printwindow = XtAppCreateShell("free42", "Free42", topLevelShellWidgetClass,
				display, args, nargs);

    XmAddWMProtocolCallback(printwindow,
			    XmInternAtom(display, "WM_DELETE_WINDOW", False),
			    delete_print_cb, NULL);

    form = XtVaCreateManagedWidget("Form",
				   xmFormWidgetClass,
				   printwindow,
				   XmNautoUnmanage, False,
				   XmNmarginHeight, 0,
				   XmNmarginWidth, 0,
				   NULL);

    scroll = XtVaCreateManagedWidget("ScrolledWindow",
				     xmScrolledWindowWidgetClass,
				     form,
				     XmNscrollingPolicy, XmAUTOMATIC,
				     XmNscrollBarDisplayPolicy, XmSTATIC,
				     XmNtopAttachment, XmATTACH_FORM,
				     XmNtopOffset, 0,
				     XmNleftAttachment, XmATTACH_FORM,
				     XmNleftOffset, 0,
				     XmNrightAttachment, XmATTACH_FORM,
				     XmNrightOffset, 0,
				     XmNbottomAttachment, XmATTACH_FORM,
				     XmNbottomOffset, 0,
				     NULL);

    w = XtNameToWidget(scroll, "HorScrollBar");
    XtUnmanageChild(w);
    print_sb = XtNameToWidget(scroll, "VertScrollBar");

    {
	/* ScrolledWindow size hack.
	 * See the long comment in fw/Viewer.cc, in finish_init(),
	 * for a detailed explanation of what this is all about.
	 */
	Dimension marginwidth, marginheight, spacing, borderWidth, shadowThickness;
	Dimension mysteryBorder = 2;
	Dimension vsbwidth, vsbborder;
	Dimension sw, sh;
	nargs = 0;

	XtVaGetValues(scroll,
		      XmNscrolledWindowMarginWidth, &marginwidth,
		      XmNscrolledWindowMarginHeight, &marginheight,
		      XmNspacing, &spacing,
		      XmNborderWidth, &borderWidth,
		      XmNshadowThickness, &shadowThickness,
		      NULL);

	XtVaGetValues(print_sb,
		      XmNwidth, &vsbwidth,
		      XmNborderWidth, &vsbborder,
		      NULL);

	sw = 2 * (borderWidth + marginwidth + mysteryBorder
		+ shadowThickness + vsbborder)
		+ vsbwidth + spacing + 286;
	XtSetArg(args[nargs], XmNwidth, sw); nargs++;

	if (!state.printWindowKnown) {
	    /* If the print window geometry is known, we just set the height
	     * on the top-level shell to the saved value; if it is not known,
	     * we set the height on the scroll bar to 600.
	     */
	    sh = 2 * (borderWidth + marginheight
		    + shadowThickness) + spacing + 600;
	    XtSetArg(args[nargs], XmNheight, sh); nargs++;
	}

	XtSetValues(scroll, args, nargs);
    }

    print_da = XtVaCreateManagedWidget("DrawingArea",
				       xmDrawingAreaWidgetClass,
				       scroll,
				       XmNwidth, 286,
				       XmNheight, printout_bottom,
				       NULL);

    XtAddCallback(print_da, XmNexposeCallback, print_expose_cb, NULL);
    XtAddEventHandler(print_da, ExposureMask, True, print_graphicsexpose_cb,
									NULL);

    XtRealizeWidget(printwindow);

    {
	/* Prevent horizontal resizing */
	Dimension ww;
	XtVaGetValues(printwindow,
		      XmNwidth, &ww,
		      NULL);
	XtVaSetValues(printwindow,
		      XmNminWidth, ww,
		      XmNmaxWidth, ww,
		      NULL);
    }

    print_canvas = XtWindow(print_da);
    XtVaSetValues(print_sb, XmNincrement, 18, NULL);
    scroll_printout_to_bottom();


    /*************************************************/
    /***** Show main window & start the emulator *****/
    /*************************************************/

    if (state.printWindowKnown && state.printWindowMapped)
	XMapRaised(display, XtWindow(printwindow));
    no_gnome_resize(mainwindow);
    XMapRaised(display, XtWindow(mainwindow));

    core_init(init_mode, version);
    if (statefile != NULL) {
	fclose(statefile);
	statefile = NULL;
    }
    if (core_powercycle())
	enable_reminder();

    /* Check if /proc/apm exists and is readable, and if so,
     * start the battery checker "thread" that keeps the battery
     * annunciator on the calculator display up to date.
     */
    apm = fopen("/proc/apm", "r");
    if (apm != NULL) {
	fclose(apm);
	shell_low_battery();
	XtAppAddTimeOut(appcontext, 60000, battery_checker, NULL);
    }

    term_sig_id = XtAppAddSignal(appcontext, xt_int_term_handler, NULL);
    act.sa_handler = int_term_handler;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGINT);
    sigaddset(&act.sa_mask, SIGTERM);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    XtAppMainLoop(appcontext);
    return 0;
}

keymap_entry *parse_keymap_entry(char *line, int lineno) {
    char *p;
    static keymap_entry entry;

    p =  strchr(line, '#');
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
	KeySym keysym = NoSymbol;
	bool done = false;
	unsigned char macrobuf[KEYMAP_MAX_MACRO_LENGTH + 1];
	int macrolen = 0;

	/* Parse keysym */
	*p = 0;
	tok = strtok(line, " \t");
	while (tok != NULL) {
	    if (done) {
		fprintf(stderr, "Keymap, line %d: Excess tokens in key spec.\n", lineno);
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
		keysym = XStringToKeysym(tok);
		if (keysym == NoSymbol) {
		    fprintf(stderr, "Keymap, line %d: Unrecognized KeySym.\n", lineno);
		    return NULL;
		}
		done = true;
	    }
	    tok = strtok(NULL, " \t");
	}
	if (!done) {
	    fprintf(stderr, "Keymap, line %d: Unrecognized KeySym.\n", lineno);
	    return NULL;
	}

	/* Parse macro */
	tok = strtok(val, " \t");
	while (tok != NULL) {
	    char *endptr;
	    long k = strtol(tok, &endptr, 10);
	    if (*endptr != 0 || k < 1 || k > 255) {
		fprintf(stderr, "Keymap, line %d: Bad value (%s) in macro.\n", lineno, tok);
		return NULL;
	    } else if (macrolen == KEYMAP_MAX_MACRO_LENGTH) {
		fprintf(stderr, "Keymap, line %d: Macro too long (max=%d).\n", lineno, KEYMAP_MAX_MACRO_LENGTH);
		return NULL;
	    } else
		macrobuf[macrolen++] = k;
	    tok = strtok(NULL, " \t");
	}
	macrobuf[macrolen] = 0;

	entry.ctrl = ctrl;
	entry.alt = alt;
	entry.shift = shift;
	entry.cshift = cshift;
	entry.keysym = keysym;
	strcpy((char *) entry.macro, (const char *) macrobuf);
	return &entry;
    } else
	return NULL;
}

static void read_key_map(const char *keymapfilename) {
    FILE *keymapfile = fopen(keymapfilename, "r");
    int kmcap = 0;
    char line[1024];
    int lineno = 0;

    if (keymapfile == NULL) {
	/* Try to create default keymap file */
	extern long keymap_filesize;
	extern const char keymap_filedata[];
	long n;

	keymapfile = fopen(keymapfilename, "wb");
	if (keymapfile == NULL)
	    return;
	n = fwrite(keymap_filedata, 1, keymap_filesize, keymapfile);
	if (n != keymap_filesize) {
	    int err = errno;
	    fprintf(stderr, "Error writing \"%s\": %s (%d)\n",
			    keymapfilename, strerror(err), err);
	}
	fclose(keymapfile);

	keymapfile = fopen(keymapfilename, "r");
	if (keymapfile == NULL)
	    return;
    }

    while (fgets(line, 1024, keymapfile) != NULL) {
	keymap_entry *entry = parse_keymap_entry(line, ++lineno);
	if (entry == NULL)
	    continue;
	/* Create new keymap entry */
	if (keymap_length == kmcap) {
	    kmcap += 50;
	    keymap = (keymap_entry *) realloc(keymap, kmcap * sizeof(keymap_entry));
	    // TODO - handle memory allocation failure
	}
	memcpy(keymap + (keymap_length++), entry, sizeof(keymap_entry));
    }

    fclose(keymapfile);
}

static void init_shell_state(int4 version) {
    switch (version) {
	case -1:
	    state.extras = 0;
	    /* fall through */
	case 0:
	    state.printerToTxtFile = 0;
	    state.printerToGifFile = 0;
	    state.printerTxtFileName[0] = 0;
	    state.printerGifFileName[0] = 0;
	    state.printerGifMaxLength = 256;
	    /* fall through */
	case 1:
	    state.mainWindowKnown = 0;
	    state.printWindowKnown = 0;
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

static int read_shell_state(int4 *ver) {
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

static int write_shell_state() {
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

/* Defining my own X error handler. There are two things I don't like
 * about the default error handler: first, it treats all errors as fatal,
 * killing the application, even though many are not, and it can be better
 * to keep the application running, so one may try to experiment and find
 * out *why* certain errors occur; two, if we're going to abort anyway,
 * dump core while you're at it, so we can do postmortem debugging.
 */
static int x_error_handler(Display *display, XErrorEvent *event) {
    char buf[1024];
    const char *req_name = "???";
    int i;

    XGetErrorText(display, event->error_code, buf, 1024);
    for (i = 0; req_code_list[i].value != -1; i++)
	if (req_code_list[i].value == event->request_code) {
	    req_name = req_code_list[i].name;
	    break;
	}

    fprintf(stderr, "X Error of failed request:  %s\n", buf);
    fprintf(stderr, "  Major opcode of failed request:  %u (%s)\n",
		    event->request_code, req_name);
    fprintf(stderr, "  Resource id in failed request:  0x%lx\n",
		    event->resourceid);
    fprintf(stderr, "  Serial number of failed request:  %lu\n", event->serial);

    /* OK, I'm trying to mimic the output from the default error handler,
     * but this one has me stumped. How to you find the current serial number
     * in the X output stream? Is that even possible without access to Xlib
     * internals?
     */
    fprintf(stderr, "  Current serial number in output stream:  ???\n");

    if (x_errors_coredump)
	kill(0, SIGQUIT);
    else
	fprintf(stderr, "\007\n");
    return 0;
}

/* An alternative X I/O error handler, which dumps core so we can do
 * post-mortem debugging. This handler is only used when Free42 is run
 * with the -xdump option; that option also causes the X error handler
 * (above) to dump core on all errors, and it sets the connection to
 * synchronous mode, to maximize the odds that those core dumps will
 * actually contain some indication of what type of X protocol abuse
 * we have committed.
 */
static int x_io_error_handler(Display *display) {
    fprintf(stderr, "X I/O Error!\n");
    fprintf(stderr, "Sending myself a QUIT signal...\n");
    kill(0, SIGQUIT);
    return 0;
}

static void int_term_handler(int sig) {
    XtNoticeSignal(term_sig_id);
}

static void xt_int_term_handler(XtPointer closure, XtSignalId *id) {
    quit();
}

static void quit() {
    FILE *printfile;
    int n, length;

    printfile = fopen(printfilename, "w");
    if (printfile != NULL) {
	length = printout_bottom - printout_top;
	if (length < 0)
	    length += PRINT_LINES;
	n = fwrite(&length, 1, sizeof(int), printfile);
	if (n != sizeof(int))
	    goto failed;
	if (printout_bottom >= printout_top) {
	    n = fwrite(print_image->data + PRINT_BYTESPERLINE * printout_top,
		       1, PRINT_BYTESPERLINE * length, printfile);
	    if (n != PRINT_BYTESPERLINE * length)
		goto failed;
	} else {
	    n = fwrite(print_image->data + PRINT_BYTESPERLINE * printout_top,
		       1, PRINT_SIZE - PRINT_BYTESPERLINE * printout_top,
		       printfile);
	    if (n != PRINT_SIZE - PRINT_BYTESPERLINE * printout_top)
		goto failed;
	    n = fwrite(print_image->data, 1,
		       PRINT_BYTESPERLINE * printout_bottom, printfile);
	    if (n != PRINT_BYTESPERLINE * printout_bottom)
		goto failed;
	}

	fclose(printfile);
	goto done;

	failed:
	fclose(printfile);
	remove(printfilename);

	done:
	;
    }

    if (print_txt != NULL)
	fclose(print_txt);

    if (print_gif != NULL) {
	shell_finish_gif(gif_seeker, gif_writer);
	fclose(print_gif);
    }

    state.mainWindowKnown = get_window_pos(mainwindow,
				&state.mainWindowX, &state.mainWindowY);
    if (state.printWindowKnown) {
	Dimension height;
	state.printWindowKnown = get_window_pos(printwindow,
				&state.printWindowX, &state.printWindowY);
	XtVaGetValues(printwindow, XmNheight, &height, NULL);
	state.printWindowHeight = height;
    }

    statefile = fopen(statefilename, "w");
    if (statefile != NULL)
	write_shell_state();
    core_quit();
    if (statefile != NULL)
	fclose(statefile);

    shell_spool_exit();
    
    exit(0);
}

static char *strclone(const char *s) {
    char *s2 = (char *) malloc(strlen(s) + 1);
    if (s2 != NULL)
	strcpy(s2, s);
    return s2;
}

static int is_file(const char *name) {
    struct stat st;
    if (stat(name, &st) == -1)
	return 0;
    return S_ISREG(st.st_mode);
}

static void show_message(char *title, char *message) {
    static Widget msg = NULL;

    if (msg == NULL) {
	Arg args[3];
	Widget button;
	XmString s;

	s = XmStringCreateLocalized(message);
	XtSetArg(args[0], XmNmessageString, s);
	XtSetArg(args[1], XmNtitle, title);
	XtSetArg(args[2], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
	msg = XmCreateMessageDialog(mainwindow, "Message", args, 3);
	XmStringFree(s);

	button = XmMessageBoxGetChild(msg, XmDIALOG_CANCEL_BUTTON);
	XtUnmanageChild(button);
	button = XmMessageBoxGetChild(msg, XmDIALOG_HELP_BUTTON);
	XtUnmanageChild(button);
    } else {
	XmString s = XmStringCreateLocalized(message);
	XtVaSetValues(msg, XmNmessageString, s, NULL);
	XtVaSetValues(XtParent(msg), XmNtitle, title, NULL);
	XmStringFree(s);
    }

    XtManageChild(msg);
}

static void no_gnome_resize(Widget w) {
    // metacity, and probably other window managers as well, does not honor all
    // the mwm decor/function hints -- the result is that you get a window with
    // no maximize button (good) but which can still be resized by dragging its
    // edges (bad). Setting the WM size hints plugs this hole.
    // It is a pain in the neck, though, because these hints also prevent the
    // window from being resized under program control -- so whenever that is
    // needed (i.e. when changing skins on the calculator window), the hints
    // must be relaxed.
    XtRealizeWidget(w);
    Dimension width, height;
    XtVaGetValues(w,
		  XmNwidth, &width,
		  XmNheight, &height,
		  NULL);
    XtVaSetValues(w,
		  XmNminWidth, width,
		  XmNmaxWidth, width,
		  XmNminHeight, height,
		  XmNmaxHeight, height,
		  NULL);
}

void allow_mainwindow_resize() {
    XtVaSetValues(mainwindow,
		  XmNminWidth, 1,
		  XmNminHeight, 1,
		  XmNmaxWidth, 32767,
		  XmNmaxHeight, 32767,
		  NULL);
}

void disallow_mainwindow_resize() {
    no_gnome_resize(mainwindow);
}

static void scroll_printout_to_bottom() {
    XmScrollBarCallbackStruct cbs;
    int maximum, slidersize;

    XtVaGetValues(print_sb, XmNmaximum, &maximum,
			    XmNsliderSize, &slidersize,
			    NULL);
    XtVaSetValues(print_sb, XmNvalue, maximum - slidersize,
			    NULL);
    cbs.reason = XmCR_VALUE_CHANGED;
    cbs.event = NULL;
    cbs.value = maximum - slidersize;
    XtCallCallbacks(print_sb, XmNvalueChangedCallback, &cbs);
}

static int get_window_pos(Widget w, int *wx, int *wy) {
    Widget pw;
    Window win;

    /* Find topmost widget */
    while ((pw = XtParent(w)) != NULL)
	w = pw;

    /* Find topmost window */
    win = XtWindow(w);
    while (1) {
	Window root;
	Window parent;
	Window *children;
	unsigned int nchildren;
	if (XQueryTree(display, win, &root, &parent, &children, &nchildren) ==0)
	    /* Fatal error */
	    return 0;
	if (children != NULL)
	    XFree(children);
	if (root == parent)
	    break;
	win = parent;
    }

    /* Find position of topmost window */
    {
	Window root;
	int x, y;
	unsigned int width, height;
	unsigned int border_width, depth;
	if (XGetGeometry(display, win, &root, &x, &y, &width, &height,
						&border_width, &depth) == 0)
	    /* Fatal error */
	    return 0;
	*wx = x;
	*wy = y;
	return 1;
    }
}

static void quitCB(Widget w, XtPointer ud, XtPointer cd) {
    quit();
}

static void showPrintOutCB(Widget w, XtPointer ud, XtPointer cd) {
    XMapRaised(display, XtWindow(printwindow));
    state.printWindowKnown = 1;
    state.printWindowMapped = 1;
}

static void exportProgramCB(Widget w, XtPointer ud, XtPointer cd) {
    char buf[10000];
    int count;
    XmString *stringtab;
    char *p = buf;
    int i;

    if (program_select_dialog == NULL)
	make_program_select_dialog();

    count = core_list_programs(buf, 10000);
    stringtab = (XmString *) malloc(count * sizeof(XmString));
    // TODO - handle memory allocation failure
    for (i = 0; i < count; i++) {
	stringtab[i] = XmStringCreateLocalized(p);
	p += strlen(p) + 1;
    }
    XtVaSetValues(program_list, XmNitemCount, count,
				XmNitems, stringtab,
				NULL);
    XmListDeselectAllItems(program_list);
    for (i = 0; i < count; i++)
	XmStringFree(stringtab[i]);
    free(stringtab);

    XtManageChild(program_select_dialog);
    no_gnome_resize(XtParent(program_select_dialog));
}

static void make_program_select_dialog() {
    Arg args[9];
    Widget label, button, panel;
    XmString s;

    XtSetArg(args[0], XmNtitle, "Export Program");
    XtSetArg(args[1], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
    XtSetArg(args[2], XmNmwmFunctions,
		        MWM_FUNC_ALL | MWM_FUNC_RESIZE | MWM_FUNC_MAXIMIZE);
    XtSetArg(args[3], XmNmwmDecorations,
			MWM_DECOR_ALL | MWM_DECOR_RESIZEH | MWM_DECOR_MAXIMIZE);
    XtSetArg(args[4], XmNmarginWidth, 5);
    XtSetArg(args[5], XmNmarginHeight, 5);

    program_select_dialog = XmCreateFormDialog(mainwindow,
					"ProgramSelectDialog", args, 6);

    s = XmStringCreateLocalized("Select Programs to Export:");
    label = XtVaCreateManagedWidget("SelectProgramsLabel",
			xmLabelWidgetClass,
			program_select_dialog,
			XmNlabelString, s,
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);
    XmStringFree(s);

    XtSetArg(args[0], XmNscrollingPolicy, XmAUTOMATIC);
    XtSetArg(args[1], XmNscrollBarDisplayPolicy, XmAS_NEEDED);
    XtSetArg(args[2], XmNvisibleItemCount, 12);
    XtSetArg(args[3], XmNselectionPolicy, XmEXTENDED_SELECT);
    XtSetArg(args[4], XmNtopAttachment, XmATTACH_WIDGET);
    XtSetArg(args[5], XmNtopOffset, 5);
    XtSetArg(args[6], XmNtopWidget, label);
    XtSetArg(args[7], XmNleftAttachment, XmATTACH_FORM);
    XtSetArg(args[8], XmNrightAttachment, XmATTACH_FORM);
    program_list = XmCreateScrolledList(program_select_dialog,
			"ProgramList",
			args, 9);
    XtManageChild(program_list);

    panel = XtVaCreateManagedWidget(
			"OK_Cancel_Panel",
			xmRowColumnWidgetClass,
			program_select_dialog,
			XmNorientation, XmHORIZONTAL,
			XmNpacking, XmPACK_COLUMN,
			XmNentryAlignment, XmALIGNMENT_CENTER,
			XmNrowColumnType, XmWORK_AREA,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, XtParent(program_list),
			XmNtopOffset, 5,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);

    s = XmStringCreateLocalized("OK");
    button = XtVaCreateManagedWidget(
			"OK",
			xmPushButtonWidgetClass,
			panel,
			XmNlabelString, s,
			XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
			NULL);
    XmStringFree(s);
    XtAddCallback(button, XmNactivateCallback, selProgButtonCB, (XtPointer) 0);

    s = XmStringCreateLocalized("Cancel");
    button = XtVaCreateManagedWidget(
			"Cancel",
			xmPushButtonWidgetClass,
			panel,
			XmNlabelString, s,
			XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
			NULL);
    XmStringFree(s);
    XtAddCallback(button, XmNactivateCallback, selProgButtonCB, (XtPointer) 1);
}

/*********************************************/
/***** Beginning of File Selection stuff *****/
/*********************************************/

static int case_insens_compar(const void *a, const void *b) {
    return strcasecmp(*(char **) a, *(char **) b);
}

static void my_search_proc(Widget fsb, XtPointer cd) {
    XmFileSelectionBoxCallbackStruct *cbs =
			    (XmFileSelectionBoxCallbackStruct *) cd;
    char *dirname, *pattern;
    int patlen;
    DIR *dir;
    struct dirent *d;
    char *buf = NULL;
    int buflen = 0;
    int bufcap = 0;
    int n = 0;
    char **list1;
    XmString *list2;
    int i;
    char *p;

    if (!XmStringGetLtoR(cbs->dir, XmFONTLIST_DEFAULT_TAG, &dirname)) {
	nothing:
	XtVaSetValues(fsb,
		      XmNfileListItems, NULL,
		      XmNfileListItemCount, 0,
		      XmNlistUpdated, True,
		      NULL);
	return;
    }
    if (!XmStringGetLtoR(cbs->pattern, XmFONTLIST_DEFAULT_TAG, &pattern))
	pattern = NULL;
    
    /* NOTE: this code assumes that the pattern is either "*" or "*.<ext>"
     * Patterns such as those supported by the Windows file selection box,
     * or Free42's PalmOS file selection box (see free42/palmos/filesys.c),
     * namely, "*.(<ext>|<ext>...)", are not supported here yet.
     */

    if (strcmp(pattern, "*") == 0) {
	no_pattern:
	XtFree(pattern);
	pattern = NULL;
    } else {
	patlen = strlen(pattern) - 1;
	if (patlen < 2)
	    goto no_pattern;
    }

    /* From here on, pattern == NULL means match everything; any other
     * pattern is assumed to be "*.<ext>", and we do a case-insensitive
     * match between the end of the file name and pattern+1.
     */

    dir = opendir(dirname);
    if (dir == NULL) {
	XtFree(dirname);
	if (pattern != NULL)
	    XtFree(pattern);
	goto nothing;
    }

    while ((d = readdir(dir)) != NULL) {
	struct stat st;
	char path[256];
	int len;

	strncpy(path, dirname, 255);
	strncat(path, "/", 255);
	strncat(path, d->d_name, 255);
	path[255] = 0;
	if (stat(path, &st) == -1 || S_ISDIR(st.st_mode))
	    continue;

	len = strlen(d->d_name);
	if (pattern != NULL) {
	    if (len < patlen)
		continue;
	    if (strcasecmp(d->d_name + len - patlen, pattern + 1) != 0)
		continue;
	}
	if (buflen + len + 1 > bufcap) {
	    char *b2;
	    bufcap += 1024;
	    b2 = (char *) realloc(buf, bufcap);
	    if (b2 == NULL)
		break;
	    buf = b2;
	}
	strcpy(buf + buflen, d->d_name);
	buflen += len + 1;
	n++;
    }

    closedir(dir);
    XtFree(dirname);
    if (pattern != NULL)
	XtFree(pattern);

    if (n == 0)
	goto nothing;

    list1 = (char **) malloc(n * sizeof(char *));
    if (list1 == NULL) {
	free(buf);
	goto nothing;
    }
    list2 = (XmString *) malloc(n * sizeof(XmString));
    if (list2 == NULL) {
	free(buf);
	free(list1);
	goto nothing;
    }

    p = buf;
    for (i = 0; i < n; i++) {
	list1[i] = p;
	p += strlen(p) + 1;
    }
    qsort(list1, n, sizeof(char *), case_insens_compar);

    for (i = 0; i < n; i++) {
	XmString s = XmStringCreateLocalized(list1[i]);
	if (s == NULL)
	    break;
	list2[i] = s;
    }
    XtVaSetValues(fsb,
		  XmNfileListItems, list2,
		  XmNfileListItemCount, i,
		  XmNlistUpdated, True,
		  NULL);
    while (--i >= 0)
	XmStringFree(list2[i]);
    free(list1);
    free(list2);
}

static void patternChangeCB(Widget w, XtPointer ud, XtPointer cd) {
    while (w != NULL && !XmIsFileSelectionBox(w))
	w = XtParent(w);
    if (w != NULL) {
	char buf[34];
	const char *pat = (const char *) ud;
	XmString s;

	if (strcmp(pat, "*") == 0)
	    s = XmStringCreateLocalized((char *) pat);
	else {
	    strcpy(buf, "*.");
	    strcat(buf, pat);
	    s = XmStringCreateLocalized(buf);
	}
	XtVaSetValues(w, XmNpattern, s, NULL);
	XmStringFree(s);
    }
}

static Widget make_file_select_dialog(const char *title, const char *pattern,
				Widget owner,
				XtCallbackProc callback, XtPointer closure) {
    Widget fsd, w;
    Arg args[3];
    int nargs;

    nargs = 0;
    XtSetArg(args[nargs], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
    nargs++;
    XtSetArg(args[nargs], XmNtitle, title);
    nargs++;
    if (0) {
	/* Leaving this out until I can figure out why Open Motif 2.2.2 won't
	 * resize the file selection box properly once the custom search
	 * procedure is in place. It's not a big loss - the only point of the
	 * custom proc is to do case-insensitive matching on the extension.
	 * Oh, well.
	 */
	XtSetArg(args[2], XmNfileSearchProc, my_search_proc);
	nargs++;
    }
    fsd = XmCreateFileSelectionDialog(owner, "FileSelDialog", args, nargs);

    /* Pattern selection menu */
    if (pattern != NULL) {
	Widget pulldown, option;
	XmString s;
	const char *p;
	int first = 1;

	pulldown = XmCreatePulldownMenu(fsd, "TypeList", NULL, 0);
	s = XmStringCreateLocalized("File Type:");
	XtSetArg(args[0], XmNlabelString, s);
	XtSetArg(args[1], XmNsubMenuId, pulldown);
	option = XmCreateOptionMenu(fsd, "Type", args, 2);
	XmStringFree(s);
	XtManageChild(option);

	w = XtNameToWidget(option, "OptionButton");
	XtSetArg(args[0], XmNalignment, XmALIGNMENT_BEGINNING);
	XtSetValues(w, args, 1);

	p = pattern;
	while (1) {
	    const char *descr, *ext;
	    int n = strlen(p);
	    if (n == 0)
		break;
	    descr = p;
	    p += n + 1;
	    n = strlen(p);
	    if (n == 0)
		break;
	    ext = p;
	    p += n + 1;

	    s = XmStringCreateLocalized((char *) descr);
	    w = XtVaCreateManagedWidget(
			"PatternButton",
			xmPushButtonWidgetClass,
			pulldown,
			XmNlabelString, s,
			NULL);
	    XmStringFree(s);

	    /* NOTE: this code assumes that the pattern is a constant
	     * string, so it's OK to take a pointer to it and hang onto
	     * it indefinitely (for the lifetime of the application!).
	     */
	    XtAddCallback(w, XmNactivateCallback, patternChangeCB, (XtPointer) ext);
	    
	    if (first) {
		char patbuf[34];
		if (strcmp(ext, "*") == 0)
		    strcpy(patbuf, ext);
		else {
		    strcpy(patbuf, "*.");
		    strcat(patbuf, ext);
		}
		s = XmStringCreateLocalized(patbuf);
		XtVaSetValues(fsd, XmNpattern, s, NULL);
		XmStringFree(s);
		first = 0;
	    }
	}
    }

    w = XmFileSelectionBoxGetChild(fsd, XmDIALOG_FILTER_LABEL);
    XtUnmanageChild(w);
    w = XmFileSelectionBoxGetChild(fsd, XmDIALOG_FILTER_TEXT);
    XtUnmanageChild(w);
    w = XmFileSelectionBoxGetChild(fsd, XmDIALOG_HELP_BUTTON);
    XtUnmanageChild(w);

    XtAddCallback(fsd, XmNokCallback, callback, closure);
    XtAddCallback(fsd, XmNcancelCallback, callback, closure);
    return fsd;
}

/***************************************/
/***** End of File Selection stuff *****/
/***************************************/

static void selProgButtonCB(Widget w, XtPointer ud, XtPointer cd) {
    int id = (int) ud;
    int count;

    XtUnmanageChild(program_select_dialog);
    if (id == 1)
	return;
    XtVaGetValues(program_list, XmNselectedPositionCount, &count, NULL);

    if (count == 0)
	return;

    if (export_dialog == NULL) {
	export_dialog = make_file_select_dialog(
					"Export Program",
					"Program Files (*.raw)\0raw\0"
					    "All Files (*.*)\0*\0\0",
					mainwindow,
					exportFileCB,
					NULL);
    }

    XtManageChild(export_dialog);
}

static void exportFileCB(Widget w, XtPointer ud, XtPointer cd) {
    XmSelectionBoxCallbackStruct *cbs = (XmSelectionBoxCallbackStruct *) cd;
    char *filename;
    char export_format[32] = "*";

    XtUnmanageChild(export_dialog);
    if (cbs->reason != XmCR_OK)
	return;

    while (w != NULL && !XmIsFileSelectionBox(w))
	w = XtParent(w);
    if (w != NULL) {
	XmString s;
	XtVaGetValues(w, XmNpattern, &s, NULL);
	if (s != NULL) {
	    char *pattern;
	    if (XmStringGetLtoR(s, XmFONTLIST_DEFAULT_TAG, &pattern)) {
		if (strncmp(pattern, "*.", 2) == 0)
		    strcpy(export_format, pattern + 2);
		XtFree(pattern);
	    }
	    XmStringFree(s);
	}
    }

    if (XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &filename)) {
	strcpy(export_file_name, filename);
	XtFree(filename);
	if (strcmp(export_format, "*") != 0) {
	    int len = strlen(export_file_name);
	    int extlen = strlen(export_format);
	    if (len < extlen || export_file_name[len - extlen - 1] != '.'
		|| strcasecmp(export_file_name + len - extlen,
						export_format) != 0) {
		strcat(export_file_name, ".");
		strcat(export_file_name, export_format);
	    }
	}

	if (is_file(export_file_name)) {
	    XmString s;
	    char buf[1000];
	    snprintf(buf, 1000, "Replace existing \"%s\"?", export_file_name);
	    s = XmStringCreateLocalized(buf);

	    if (overwrite_confirm_dialog == NULL) {
		Arg args[3];
		Widget b;
		XtSetArg(args[0], XmNmessageString, s);
		XtSetArg(args[1], XmNtitle, "Replace?");
		XtSetArg(args[2], XmNdialogType, XmDIALOG_QUESTION);
		overwrite_confirm_dialog =
			XmCreateMessageDialog(mainwindow, "Replace?", args, 3);

		b = XmMessageBoxGetChild(overwrite_confirm_dialog,
						XmDIALOG_HELP_BUTTON);
		XtUnmanageChild(b);
		b = XmMessageBoxGetChild(overwrite_confirm_dialog,
						XmDIALOG_OK_BUTTON);
		XtAddCallback(b, XmNactivateCallback, do_export, NULL);
	    } else
		XtVaSetValues(overwrite_confirm_dialog,
			      XmNmessageString, s, NULL);

	    XmStringFree(s);
	    XtManageChild(overwrite_confirm_dialog);
	} else
	    do_export(NULL, NULL, NULL);
    }
}

static void do_export(Widget w, XtPointer ud, XtPointer cd) {
    int count, i;
    unsigned int *positions, *p2;

    XtVaGetValues(program_list, XmNselectedPositionCount, &count,
				XmNselectedPositions, &positions,
				NULL);
    if (count == 0)
	return;

    export_file = fopen(export_file_name, "w");
    if (export_file == NULL) {
	char buf[1000];
	int err = errno;
	snprintf(buf, 1000, "Could not open \"%s\" for writing:\n%s (%d)",
		 export_file_name, strerror(err), err);
	show_message("Message", buf);
    } else {
	p2 = (unsigned int *) malloc(count * sizeof(unsigned int));
	// TODO - handle memory allocation failure
	for (i = 0; i < count; i++)
	    p2[i] = positions[i] - 1;
	core_export_programs(count, (int *) p2, NULL);
	free(p2);
	if (export_file != NULL) {
	    fclose(export_file);
	    export_file = NULL;
	}
    }
}

static void importProgramCB(Widget w, XtPointer ud, XtPointer cd) {
    if (import_dialog == NULL) {
	import_dialog = make_file_select_dialog(
					"Import Program",
					"Program Files (*.raw)\0raw\0"
					    "All Files (*.*)\0*\0\0",
					mainwindow,
					importFileCB,
					NULL);
    }

    XtManageChild(import_dialog);
}

static void importFileCB(Widget w, XtPointer ud, XtPointer cd) {
    XmSelectionBoxCallbackStruct *cbs = (XmSelectionBoxCallbackStruct *) cd;
    char *filename;

    XtUnmanageChild(import_dialog);
    if (cbs->reason != XmCR_OK)
	return;

    if (XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &filename)) {
	import_file = fopen(filename, "r");
	if (import_file == NULL) {
	    char buf[1000];
	    int err = errno;
	    snprintf(buf, 1000, "Could not open \"%s\" for reading:\n%s (%d)",
		     filename, strerror(err), err);
	    XtFree(filename);
	    show_message("Message", buf);
	} else {
	    XtFree(filename);
	    core_import_programs(NULL);
	    redisplay();
	    if (import_file != NULL) {
		fclose(import_file);
		import_file = NULL;
	    }
	}
    }
}

static void clearPrintOutCB(Widget w, XtPointer ud, XtPointer cd) {
    printout_top = 0;
    printout_bottom = 0;
    XtVaSetValues(print_da, XmNheight, 1, NULL);

    if (print_gif != NULL) {
	shell_finish_gif(gif_seeker, gif_writer);
	fclose(print_gif);
	print_gif = NULL;
    }
}

static void preferencesCB(Widget w, XtPointer ud, XtPointer cd) {
    char maxlen[6];

    if (prefsdialog == NULL)
	make_prefs_dialog();

    XmToggleButtonSetState(prefs_matrix_singularmatrix,
			   core_settings.matrix_singularmatrix, False);
    XmToggleButtonSetState(prefs_matrix_outofrange,
			   core_settings.matrix_outofrange, False);
    XmToggleButtonSetState(prefs_printer_txt, state.printerToTxtFile, False);
    XmTextSetString(prefs_printer_txt_name, state.printerTxtFileName);
    XmToggleButtonSetState(prefs_raw_text, core_settings.raw_text, False);
    XmToggleButtonSetState(prefs_printer_gif, state.printerToGifFile, False);
    XmTextSetString(prefs_printer_gif_name, state.printerGifFileName);
    snprintf(maxlen, 6, "%d", state.printerGifMaxLength);
    XmTextSetString(prefs_printer_gif_height, maxlen);

    XtManageChild(prefsdialog);
    no_gnome_resize(XtParent(prefsdialog));
}

static void make_prefs_dialog() {
    Arg args[7];
    Widget button, panel;
    XmString s;

    XtSetArg(args[0], XmNtitle, "Preferences");
    XtSetArg(args[1], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
    XtSetArg(args[2], XmNmwmFunctions,
		        MWM_FUNC_ALL | MWM_FUNC_RESIZE | MWM_FUNC_MAXIMIZE);
    XtSetArg(args[3], XmNmwmDecorations,
			MWM_DECOR_ALL | MWM_DECOR_RESIZEH | MWM_DECOR_MAXIMIZE);
    XtSetArg(args[4], XmNmarginWidth, 5);
    XtSetArg(args[5], XmNmarginHeight, 5);
    XtSetArg(args[6], XmNautoUnmanage, False);

    prefsdialog = XmCreateFormDialog(mainwindow, "PrefsDialog", args, 7);

    s = XmStringCreateLocalized("Inverting or solving a singular matrix yields \"Singular Matrix\" error");
    prefs_matrix_singularmatrix = XtVaCreateManagedWidget(
			"Matrix_SingularMatrix",
			xmToggleButtonWidgetClass,
			prefsdialog,
			XmNlabelString, s,
			XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			NULL);
    XmStringFree(s);

    s = XmStringCreateLocalized("Overflows during matrix operations yield \"Out of Range\" error");
    prefs_matrix_outofrange = XtVaCreateManagedWidget(
			"Matrix_OutOfRange",
			xmToggleButtonWidgetClass,
			prefsdialog,
			XmNlabelString, s,
			XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, prefs_matrix_singularmatrix,
			XmNleftAttachment, XmATTACH_FORM,
			NULL);
    XmStringFree(s);

    s = XmStringCreateLocalized("Print to text file:");
    prefs_printer_txt = XtVaCreateManagedWidget(
			"Printer_TXT",
			xmToggleButtonWidgetClass,
			prefsdialog,
			XmNlabelString, s,
			XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, prefs_matrix_outofrange,
			XmNtopOffset, 10,
			XmNleftAttachment, XmATTACH_FORM,
			NULL);
    XmStringFree(s);

    prefs_printer_txt_name = XtVaCreateManagedWidget(
			"Printer_TXT_Name",
			xmTextWidgetClass,
			prefsdialog,
			XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
			XmNcolumns, 40,
			XmNmarginWidth, 2,
			XmNmarginHeight, 2,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, prefs_printer_txt,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, prefs_printer_txt,
			NULL);

    s = XmStringCreateLocalized("Browse...");
    button = XtVaCreateManagedWidget(
			"Printer_TXT_Browse",
			xmPushButtonWidgetClass,
			prefsdialog,
			XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
			XmNlabelString, s,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, prefs_printer_txt_name,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, prefs_printer_txt_name,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);
    XmStringFree(s);
    XtAddCallback(button, XmNactivateCallback, prefsButtonCB, (XtPointer) 0);

    s = XmStringCreateLocalized("Raw text");
    prefs_raw_text = XtVaCreateManagedWidget(
			"RawText",
			xmToggleButtonWidgetClass,
			prefsdialog,
			XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
			XmNlabelString, s,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, prefs_printer_txt_name,
			XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNrightWidget, prefs_printer_txt_name,
			NULL);

    s = XmStringCreateLocalized("Print to GIF file:");
    prefs_printer_gif = XtVaCreateManagedWidget(
			"Printer_GIF",
			xmToggleButtonWidgetClass,
			prefsdialog,
			XmNlabelString, s,
			XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
			NULL);
    XmStringFree(s);

    prefs_printer_gif_name = XtVaCreateManagedWidget(
			"Printer_GIF_Name",
			xmTextWidgetClass,
			prefsdialog,
			XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
			XmNcolumns, 40,
			XmNmarginWidth, 2,
			XmNmarginHeight, 2,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, prefs_raw_text,
			XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNrightWidget, prefs_raw_text,
			NULL);

    XtVaSetValues(prefs_printer_gif,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, prefs_printer_gif_name,
			XmNleftAttachment, XmATTACH_FORM,
			NULL);

    s = XmStringCreateLocalized("Browse...");
    button = XtVaCreateManagedWidget(
			"Printer_GIF_Browse",
			xmPushButtonWidgetClass,
			prefsdialog,
			XmNlabelString, s,
			XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, prefs_printer_gif_name,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, prefs_printer_gif_name,
			NULL);
    XmStringFree(s);
    XtAddCallback(button, XmNactivateCallback, prefsButtonCB, (XtPointer) 1);

    prefs_printer_gif_height = XtVaCreateManagedWidget(
			"Printer_GIF_Height",
			xmTextWidgetClass,
			prefsdialog,
			XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
			XmNcolumns, 5,
			XmNmarginWidth, 2,
			XmNmarginHeight, 2,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, prefs_printer_gif_name,
			XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNrightWidget, prefs_printer_gif_name,
			NULL);

    s = XmStringCreateLocalized("Maximum GIF height (pixels):");
    XtVaCreateManagedWidget("Printer_GIF_Height_Label",
			xmLabelWidgetClass,
			prefsdialog,
			XmNlabelString, s,
			XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, prefs_printer_gif_height,
			XmNrightAttachment, XmATTACH_WIDGET,
			XmNrightWidget, prefs_printer_gif_height,
			NULL);
    XmStringFree(s);

    panel = XtVaCreateManagedWidget(
			"OK_Cancel_Panel",
			xmRowColumnWidgetClass,
			prefsdialog,
			XmNorientation, XmHORIZONTAL,
			XmNpacking, XmPACK_COLUMN,
			XmNentryAlignment, XmALIGNMENT_CENTER,
			XmNrowColumnType, XmWORK_AREA,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, prefs_printer_gif_height,
			XmNtopOffset, 5,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);

    s = XmStringCreateLocalized("OK");
    button = XtVaCreateManagedWidget(
			"OK",
			xmPushButtonWidgetClass,
			panel,
			XmNlabelString, s,
			XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
			NULL);
    XmStringFree(s);
    XtAddCallback(button, XmNactivateCallback, prefsButtonCB, (XtPointer) 2);

    s = XmStringCreateLocalized("Cancel");
    button = XtVaCreateManagedWidget(
			"Cancel",
			xmPushButtonWidgetClass,
			panel,
			XmNlabelString, s,
			XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
			NULL);
    XmStringFree(s);
    XtAddCallback(button, XmNactivateCallback, prefsButtonCB, (XtPointer) 3);
}


static void prefsButtonCB(Widget w, XtPointer ud, XtPointer cd) {
    int id = (int) ud;
    char *s, *old;

    switch (id) {
	case 0:
	    if (printer_txt_dialog == NULL) {
		printer_txt_dialog = make_file_select_dialog(
				    "Select Text File Name",
				    "Text (*.txt)\0txt\0All Files (*.*)\0*\0\0",
				    prefsdialog,
				    printerBrowseTxtCB,
				    NULL);
	    }
	    s = XmTextGetString(prefs_printer_txt_name);
	    setFilePath(printer_txt_dialog, s);
	    XtFree(s);
	    XtManageChild(printer_txt_dialog);
	    return;

	case 1:
	    if (printer_gif_dialog == NULL) {
		printer_gif_dialog = make_file_select_dialog(
				    "Select GIF File Name",
				    "GIF (*.gif)\0gif\0All Files (*.*)\0*\0\0",
				    prefsdialog,
				    printerBrowseGifCB,
				    NULL);
	    }
	    s = XmTextGetString(prefs_printer_gif_name);
	    setFilePath(printer_gif_dialog, s);
	    XtFree(s);
	    XtManageChild(printer_gif_dialog);
	    return;

	case 2:
	    core_settings.matrix_singularmatrix =
		XmToggleButtonGetState(prefs_matrix_singularmatrix);
	    core_settings.matrix_outofrange =
		XmToggleButtonGetState(prefs_matrix_outofrange);
	    core_settings.raw_text =
		XmToggleButtonGetState(prefs_raw_text);

	    state.printerToTxtFile =
		XmToggleButtonGetState(prefs_printer_txt);
	    old = strclone(state.printerTxtFileName);
	    s = XmTextGetString(prefs_printer_txt_name);
	    strncpy(state.printerTxtFileName, s, FILENAMELEN);
	    state.printerTxtFileName[FILENAMELEN - 1] = 0;
	    appendSuffix(state.printerTxtFileName, ".txt");
	    XtFree(s);
	    if (print_txt != NULL && (!state.printerToTxtFile
			|| strcmp(state.printerTxtFileName, old) != 0)) {
		fclose(print_txt);
		print_txt = NULL;
	    }
	    free(old);
	    state.printerToGifFile =
		XmToggleButtonGetState(prefs_printer_gif);
	    old = strclone(state.printerGifFileName);
	    s = XmTextGetString(prefs_printer_gif_name);
	    strncpy(state.printerGifFileName, s, FILENAMELEN);
	    state.printerGifFileName[FILENAMELEN - 1] = 0;
	    appendSuffix(state.printerGifFileName, ".gif");
	    XtFree(s);
	    if (print_gif != NULL && (!state.printerToGifFile
			|| strcmp(state.printerGifFileName, old) != 0)) {
		shell_finish_gif(gif_seeker, gif_writer);
		fclose(print_gif);
		print_gif = NULL;
	    }
	    free(old);
	    s = XmTextGetString(prefs_printer_gif_height);
	    if (sscanf(s, "%d", &state.printerGifMaxLength) == 1) {
		if (state.printerGifMaxLength < 32)
		    state.printerGifMaxLength = 32;
		else if (state.printerGifMaxLength > 32767)
		    state.printerGifMaxLength = 32767;
	    } else
		state.printerGifMaxLength = 256;
	    XtFree(s);
	    /* fall through */

	case 3:
	    XtUnmanageChild(prefsdialog);
	    return;

	default:
	    XBell(display, 100);
	    return;
    }
}

static void setFilePath(Widget fsb, char *filename) {
    char *lastslash;
    Widget text;

    lastslash = strrchr(filename, '/');
    if (lastslash != NULL) {
	XmString xms;
	int len = lastslash - filename + 2;
	char *dirname = (char *) malloc(len);
	// TODO - handle memory allocation failure
	strncpy(dirname, filename, len - 1);
	dirname[len - 1] = 0;
	xms = XmStringCreateLocalized(dirname);
	XtVaSetValues(fsb, XmNdirectory, xms, NULL);
	XmStringFree(xms);
	free(dirname);
    }

    text = XtNameToWidget(fsb, "Text");
    XtVaSetValues(text, XmNvalue, filename,
			XmNcursorPosition, strlen(filename),
			NULL);
}

static void appendSuffix(char *path, char *suffix) {
    int len = strlen(path);
    int slen = strlen(suffix);
    if (len == 0 || len >= FILENAMELEN - slen)
	return;
    if (len >= slen && strcasecmp(path + len - slen, suffix) != 0)
	strcat(path, suffix);
}

static void printerBrowseTxtCB(Widget w, XtPointer ud, XtPointer cd) {
    XmSelectionBoxCallbackStruct *cbs = (XmSelectionBoxCallbackStruct *) cd;
    char *filename;

    XtUnmanageChild(printer_txt_dialog);
    if (cbs->reason != XmCR_OK)
	return;

    if (XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &filename)) {
	int len = strlen(filename);
	if (len < 4 || strcasecmp(filename + len - 4, ".txt") != 0) {
	    char *s = (char *) malloc(len + 5);
	    // TODO - handle memory allocation failure
	    strcpy(s, filename);
	    strcat(s, ".txt");
	    XmTextSetString(prefs_printer_txt_name, s);
	    free(s);
	} else
	    XmTextSetString(prefs_printer_txt_name, filename);
	XtFree(filename);
    }
}

static void printerBrowseGifCB(Widget w, XtPointer ud, XtPointer cd) {
    XmSelectionBoxCallbackStruct *cbs = (XmSelectionBoxCallbackStruct *) cd;
    char *filename;

    XtUnmanageChild(printer_gif_dialog);
    if (cbs->reason != XmCR_OK)
	return;

    if (XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &filename)) {
	int len = strlen(filename);
	if (len < 4 || strcasecmp(filename + len - 4, ".gif") != 0) {
	    char *s = (char *) malloc(len + 5);
	    // TODO - handle memory allocation failure
	    strcpy(s, filename);
	    strcat(s, ".gif");
	    XmTextSetString(prefs_printer_gif_name, s);
	    free(s);
	} else
	    XmTextSetString(prefs_printer_gif_name, filename);
	XtFree(filename);
    }
}

static Atom CUT_BUFFER0 = None, STRING, PRIMARY;
static void intern_atoms() {
    if (CUT_BUFFER0 == None) {
	CUT_BUFFER0 = XmInternAtom(display, "CUT_BUFFER0", False);
	STRING = XmInternAtom(display, "STRING", False);
	PRIMARY = XmInternAtom(display, "PRIMARY", False);
    }
}

static void copyCB(Widget w, XtPointer ud, XtPointer cd) {
    char buf[100];
    int len;

    intern_atoms();

    core_copy(buf, 100);
    len = strlen(buf) + 1;
    XSetSelectionOwner(display, PRIMARY, None, CurrentTime);
    XChangeProperty(display, rootwindow, CUT_BUFFER0, STRING, 8,
		    PropModeReplace, (unsigned char *) buf, len);
}

static void pasteCB(Widget w, XtPointer ud, XtPointer cd) {
    Atom type;
    int format;
    unsigned long items, bytes;
    char *data;
    int res;

    intern_atoms();

    res = XGetWindowProperty(display, rootwindow, CUT_BUFFER0, 0, 256, False,
			     STRING, &type, &format, &items, &bytes,
			     (unsigned char **) &data);
    if (res == Success) {
	if (type == STRING) {
	    core_paste(data);
	    redisplay();
	}
	XFree(data);
    }
}

static void aboutCB(Widget w, XtPointer ud, XtPointer cd) {
    static Widget about = NULL;

    if (about == NULL) {
	Arg args[6];
	Widget button;
	XmString s;

	s = XmStringCreateLtoR("Free42 1.4.24d\n(C) 2004-2006 Thomas Okken\nthomas_okken@yahoo.com\nhttp://home.planet.nl/~demun000/thomas_projects/free42/", XmFONTLIST_DEFAULT_TAG);
	XtSetArg(args[0], XmNmessageString, s);
	XtSetArg(args[1], XmNtitle, "About Free42");
	XtSetArg(args[2], XmNsymbolPixmap, icon);
	XtSetArg(args[3], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
	XtSetArg(args[4], XmNmwmFunctions,
			    MWM_FUNC_ALL | MWM_FUNC_RESIZE | MWM_FUNC_MAXIMIZE);
	XtSetArg(args[5], XmNmwmDecorations,
			    MWM_DECOR_ALL | MWM_DECOR_RESIZEH | MWM_DECOR_MAXIMIZE);
	about = XmCreateMessageDialog(mainwindow, "About", args, 6);
	XmStringFree(s);

	button = XmMessageBoxGetChild(about, XmDIALOG_CANCEL_BUTTON);
	XtUnmanageChild(button);
	button = XmMessageBoxGetChild(about, XmDIALOG_HELP_BUTTON);
	XtUnmanageChild(button);
    }

    XtManageChild(about);
    no_gnome_resize(XtParent(about));
}

static void delete_cb(Widget w, XtPointer ud, XtPointer cd) {
    quit();
}

static void delete_print_cb(Widget w, XtPointer ud, XtPointer cd) {
    state.printWindowMapped = 0;
    if (print_repaint_pending) {
	XtRemoveWorkProc(print_repaint_id);
	print_repaint_pending = 0;
    }
    XWithdrawWindow(display, XtWindow(printwindow), screennumber);
}

static void expose_cb(Widget w, XtPointer ud, XtPointer cd) {
    allow_paint = 1;
    skin_repaint();
    skin_repaint_display();
    skin_repaint_annunciator(1, ann_updown);
    skin_repaint_annunciator(2, ann_shift);
    skin_repaint_annunciator(3, ann_print);
    skin_repaint_annunciator(4, ann_run);
    skin_repaint_annunciator(5, ann_battery);
    skin_repaint_annunciator(6, ann_g);
    skin_repaint_annunciator(7, ann_rad);
    if (ckey != 0)
	skin_repaint_key(skey, 1);
}

static void print_expose_cb(Widget w, XtPointer ud, XtPointer cd) {
    static int full_repaint = 0;
    XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct *) cd;
    XExposeEvent *e = &cbs->event->xexpose;
    if (e->count != 0) {
	full_repaint = 1;
	return;
    } else {
	if (full_repaint) {
	    int height = printout_bottom - printout_top;
	    if (height < 0)
		height += PRINT_LINES;
	    repaint_printout(0, 0, 286, height);
	    full_repaint = 0;
	} else
	    repaint_printout(e->x, e->y, e->width, e->height);
    }
}

static void print_graphicsexpose_cb(Widget w, XtPointer ud, XEvent *event,
								Boolean *cont) {
    if (event->type != GraphicsExpose) {
	*cont = True;
	return;
    } else {
	/* I'd prefer to just repaint the area reported in the GraphicsExpose
	 * event, but since there may have been additional scrolling between
	 * when the GraphicsExpose was caused (by XCopyArea() in shell_print())
	 * and when this callback gets invoked, I have no way of knowing what
	 * area to repaint. It would help if I could scroll the update region
	 * at the time of the XCopyArea() call, but I don't know how to do
	 * that. So, I just schedule a full repaint. Ugly but it does the job.
	 * TODO: find a better way!
	 * UPDATE: the officially sanctioned and blessed way is to *wait* for
	 * a GraphicsExpose or NoExpose event after each XCopyArea() call; by
	 * doing it that way, you can ensure that no additional scrolling will
	 * take place before the damage caused by the last scroll operation has
	 * been repaired.
	 */
	if (!print_repaint_pending) {
	    print_repaint_id = XtAppAddWorkProc(appcontext, print_repaint,NULL);
	    print_repaint_pending = 1;
	}
	*cont = False;
    }
}

static Boolean print_repaint(XtPointer closure) {
    int height = printout_bottom - printout_top;
    if (height < 0)
	height += PRINT_LINES;
    repaint_printout(0, 0, 286, height);
    print_repaint_pending = 0;
    return True;
}

static void shell_keydown() {
    int repeat, keep_running;
    if (skey == -1)
	skey = skin_find_skey(ckey);
    skin_repaint_key(skey, 1);
    if (timeout3_active && (macro != NULL || ckey != 28 /* KEY_SHIFT */)) {
	XtRemoveTimeOut(timeout3_id);
	timeout3_active = 0;
	core_timeout3(0);
    }

    if (macro != NULL) {
	if (*macro == 0) {
	    squeak();
	    return;
	}
	bool one_key_macro = macro[1] == 0 || (macro[2] == 0 && macro[0] == 28);
	if (!one_key_macro)
	    skin_display_set_enabled(false);
	while (*macro != 0) {
	    keep_running = core_keydown(*macro++, &enqueued, &repeat);
	    if (*macro != 0 && !enqueued)
		core_keyup();
	}
	if (!one_key_macro) {
	    skin_display_set_enabled(true);
	    skin_repaint_display();
	    skin_repaint_annunciator(1, ann_updown);
	    skin_repaint_annunciator(2, ann_shift);
	    skin_repaint_annunciator(3, ann_print);
	    skin_repaint_annunciator(4, ann_run);
	    skin_repaint_annunciator(5, ann_battery);
	    skin_repaint_annunciator(6, ann_g);
	    skin_repaint_annunciator(7, ann_rad);
	    repeat = 0;
	}
    } else
	keep_running = core_keydown(ckey, &enqueued, &repeat);

    if (quit_flag)
	quit();
    if (keep_running)
	enable_reminder();
    else {
	disable_reminder();
	if (timeout_active)
	    XtRemoveTimeOut(timeout_id);
	if (repeat != 0) {
	    timeout_id = XtAppAddTimeOut(appcontext, repeat == 1 ? 1000 : 500, repeater, NULL);
	    timeout_active = 1;
	} else if (!enqueued) {
	    timeout_id = XtAppAddTimeOut(appcontext, 250, timeout1, NULL);
	    timeout_active = 1;
	}
    }
}

static void shell_keyup() {
    skin_repaint_key(skey, 0);
    ckey = 0;
    skey = -1;
    if (timeout_active) {
	XtRemoveTimeOut(timeout_id);
	timeout_active = 0;
    }
    if (!enqueued) {
	int keep_running = core_keyup();
	if (quit_flag)
	    quit();
	if (keep_running)
	    enable_reminder();
	else
	    disable_reminder();
    }
}

static void input_cb(Widget w, XtPointer ud, XtPointer cd) {
    XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct *) cd;
    XEvent *event = cbs->event;

    if (event->type == ButtonPress) {
	if (ckey == 0) {
	    int x = event->xbutton.x;
	    int y = event->xbutton.y;
	    skin_find_key(x, y, ann_shift != 0, &skey, &ckey);
	    if (ckey != 0) {
		macro = skin_find_macro(ckey);
		shell_keydown();
		mouse_key = 1;
	    }
	}
    } else if (event->type == ButtonRelease) {
	if (ckey != 0 && mouse_key)
	    shell_keyup();
    } else if (event->type == KeyPress) {
	if (ckey == 0 || !mouse_key) {
	    char buf[32];
	    KeySym ks;
	    int i;

	    int len = XLookupString(&event->xkey, buf, 32, &ks, NULL);
	    bool printable = len == 1 && buf[0] >= 32 && buf[0] <= 126;
	    just_pressed_shift = 0;

	    if (ks == XK_Shift_L || ks == XK_Shift_R) {
		just_pressed_shift = 1;
		return;
	    }
	    bool ctrl = (event->xkey.state & ControlMask) != 0;
	    bool alt = (event->xkey.state & Mod1Mask) != 0;
	    bool shift = (event->xkey.state & (ShiftMask | LockMask)) != 0;
	    bool cshift = ann_shift != 0;

	    if (ckey != 0) {
		shell_keyup();
		active_keycode = 0;
	    }

	    if (!ctrl && !alt) {
		char c = buf[0];
		if (printable && core_alpha_menu()) {
		    if (c >= 'a' && c <= 'z')
			c = c + 'A' - 'a';
		    else if (c >= 'A' && c <= 'Z')
			c = c + 'a' - 'A';
		    ckey = 1024 + c;
		    skey = -1;
		    macro = NULL;
		    shell_keydown();
		    mouse_key = 0;
		    active_keycode = event->xkey.keycode;
		    return;
		} else if (core_hex_menu() && ((c >= 'a' && c <= 'f')
					    || (c >= 'A' && c <= 'F'))) {
		    if (c >= 'a' && c <= 'f')
			ckey = c - 'a' + 1;
		    else
			ckey = c - 'A' + 1;
		    skey = -1;
		    macro = NULL;
		    shell_keydown();
		    mouse_key = 0;
		    active_keycode = event->xkey.keycode;
		    return;
		}
	    }

	    bool exact;
	    unsigned char *key_macro = skin_keymap_lookup(ks, printable,
					    ctrl, alt, shift, cshift, &exact);
	    if (key_macro == NULL || !exact) {
		for (i = 0; i < keymap_length; i++) {
		    keymap_entry *entry = keymap + i;
		    if (ctrl == entry->ctrl
			    && alt == entry->alt
			    && (printable || shift == entry->shift)
			    && ks == entry->keysym) {
			if (cshift == entry->cshift) {
			    key_macro = entry->macro;
			    break;
			} else {
			    if (key_macro == NULL)
				key_macro = entry->macro;
			}
		    }
		}
	    }
	    if (key_macro != NULL) {
		// A keymap entry is a sequence of zero or more calculator
		// keystrokes (1..37) and/or macros (38..255). We expand
		// macros here before invoking shell_keydown().
		// If the keymap entry is one key, or two keys with the
		// first being 'shift', we highlight the key in question
		// by setting ckey; otherwise, we set ckey to -10, which
		// means no skin key will be highlighted.
		ckey = -10;
		skey = -1;
		if (key_macro[0] != 0)
		    if (key_macro[1] == 0)
			ckey = key_macro[0];
		    else if (key_macro[2] == 0 && key_macro[0] == 28)
			ckey = key_macro[1];
		bool needs_expansion = false;
		for (int j = 0; key_macro[j] != 0; j++)
		    if (key_macro[j] > 37) {
			needs_expansion = true;
			break;
		    }
		if (needs_expansion) {
		    static unsigned char macrobuf[1024];
		    int p = 0;
		    for (int j = 0; key_macro[j] != 0 && p < 1023; j++) {
			int c = key_macro[j];
			if (c <= 37)
			    macrobuf[p++] = c;
			else {
			    unsigned char *m = skin_find_macro(c);
			    if (m != NULL)
				while (*m != 0 && p < 1023)
				    macrobuf[p++] = *m++;
			}
		    }
		    macrobuf[p] = 0;
		    macro = macrobuf;
		} else
		    macro = key_macro;
		shell_keydown();
		mouse_key = 0;
		active_keycode = event->xkey.keycode;
	    }
	}
    } else if (event->type == KeyRelease) {
	// If a KeyRelease event is immediately followed by a KeyPress, with
	// the same keycode and timestamp, that means there's auto-repeat
	// taking place. Free42 does its own auto-repeat, so we simply ignore
	// the auto-repeat events the X server is sending us.
	// NOTE: the idea of this comes from GDK, which uses this heuristic to
	// generate detectable auto-repeat. I have modified it slightly by
	// allowing the two events to be up to 2 milliseconds apart; I noticed
	// on my machine there was occasionally a 1-millisecond gap, and so
	// I am playing safe (I hope).
	if (XtAppPending(appcontext)) {
	    XEvent ev2;
	    XtAppPeekEvent(appcontext, &ev2);
	    if (ev2.type == KeyPress
		    && ev2.xkey.keycode == event->xkey.keycode
		    && ev2.xkey.time - event->xkey.time <= 2) {
		XtAppNextEvent(appcontext, &ev2);
		return;
	    }
	}
	char buf[10];
	KeySym ks;
	XLookupString(&event->xkey, buf, 10, &ks, NULL);
	if (ckey == 0) {
	    if (just_pressed_shift && (ks == XK_Shift_L || ks == XK_Shift_R)) {
		ckey = 28;
		skey = -1;
		macro = NULL;
		shell_keydown();
		shell_keyup();
	    }
	} else {
	    if (!mouse_key && event->xkey.keycode == active_keycode) {
		shell_keyup();
		active_keycode = 0;
	    }
	}
    }
}

static void enable_reminder() {
    if (!reminder_active) {
	reminder_id = XtAppAddWorkProc(appcontext, reminder, NULL);
	reminder_active = 1;
    }
    if (timeout_active) {
	XtRemoveTimeOut(timeout_id);
	timeout_active = 0;
    }
}

static void disable_reminder() {
    if (reminder_active) {
	XtRemoveWorkProc(reminder_id);
	reminder_active = 0;
    }
}

static void repeater(XtPointer closure, XtIntervalId *id) {
    int repeat = core_repeat();
    if (repeat != 0)
	timeout_id = XtAppAddTimeOut(appcontext, repeat == 1 ? 200 : 100, repeater, NULL);
    else
	timeout_id = XtAppAddTimeOut(appcontext, 250, timeout1, NULL);
    timeout_active = 1;
}

static void timeout1(XtPointer closure, XtIntervalId *id) {
    if (ckey != 0) {
	core_keytimeout1();
	timeout_id = XtAppAddTimeOut(appcontext, 1750, timeout2, NULL);
	timeout_active = 1;
    } else
	timeout_active = 0;
}

static void timeout2(XtPointer closure, XtIntervalId *id) {
    if (ckey != 0)
	core_keytimeout2();
    timeout_active = 0;
}

static void timeout3(XtPointer closure, XtIntervalId *id) {
    core_timeout3(1);
    timeout3_active = 0;
}

static void battery_checker(XtPointer closure, XtIntervalId *id) {
    shell_low_battery();
    XtAppAddTimeOut(appcontext, 60000, battery_checker, NULL);
}

static void repaint_printout(int x, int y, int width, int height) {
    if (printout_bottom >= printout_top)
	/* The buffer is not wrapped */
	XPutImage(display, print_canvas, gc, print_image,
		    x, printout_top + y, x, y, width, height);
    else {
	/* The buffer is wrapped */
	if (printout_top + y < PRINT_LINES) {
	    if (printout_top + y + height <= PRINT_LINES)
		/* The rectangle is in the lower part of the buffer */
		XPutImage(display, print_canvas, gc, print_image,
			    x, printout_top + y, x, y, width, height);
	    else {
		/* The rectangle spans both parts of the buffer */
		int part1_height = PRINT_LINES - printout_top - y;
		int part2_height = height - part1_height;
		XPutImage(display, print_canvas, gc, print_image,
			    x, printout_top + y, x, y,
			    width, part1_height);
		XPutImage(display, print_canvas, gc, print_image,
			    x, 0, x, y + part1_height,
			    width, part2_height);
	    }
	} else
	    /* The rectangle is in the upper part of the buffer */
	    XPutImage(display, print_canvas, gc, print_image,
			    x, y + printout_top - PRINT_LINES,
			    x, y, width, height);
    }
}


static Boolean reminder(XtPointer closure) {
    int dummy1, dummy2;
    int keep_running = core_keydown(0, &dummy1, &dummy2);
    if (quit_flag)
	quit();
    if (keep_running)
	return False;
    else {
	reminder_active = 0;
	return True;
    }
}


/* Callbacks used by shell_print() and shell_spool_txt() / shell_spool_gif() */

static void txt_writer(const char *text, int length) {
    int n;
    if (print_txt == NULL)
	return;
    n = fwrite(text, 1, length, print_txt);
    if (n != length) {
	char buf[1000];
	state.printerToTxtFile = 0;
	fclose(print_txt);
	print_txt = NULL;
	snprintf(buf, 1000, "Error while writing to \"%s\".\nPrinting to text file disabled", state.printerTxtFileName);
	show_message("Message", buf);
    }
}

static void txt_newliner() {
    if (print_txt == NULL)
	return;
    fputc('\n', print_txt);
    fflush(print_txt);
}

static void gif_seeker(int4 pos) {
    if (print_gif == NULL)
	return;
    if (fseek(print_gif, pos, SEEK_SET) == -1) {
	char buf[1000];
	state.printerToGifFile = 0;
	fclose(print_gif);
	print_gif = NULL;
	snprintf(buf, 1000, "Error while seeking \"%s\".\nPrinting to GIF file disabled", print_gif_name);
	show_message("Message", buf);
    }
}

static void gif_writer(const char *text, int length) {
    int n;
    if (print_gif == NULL)
	return;
    n = fwrite(text, 1, length, print_gif);
    if (n != length) {
	char buf[1000];
	state.printerToGifFile = 0;
	fclose(print_gif);
	print_gif = NULL;
	snprintf(buf, 1000, "Error while writing to \"%s\".\nPrinting to GIF file disabled", print_gif_name);
	show_message("Message", buf);
    }
}

void shell_blitter(const char *bits, int bytesperline, int x, int y,
	                             int width, int height) {
    skin_display_blitter(bits, bytesperline, x, y, width, height);
    if (skey >= -7 && skey <= -2)
	skin_repaint_key(skey, 1);
}

void shell_beeper(int frequency, int duration) {
    XBell(display, 100);
}

void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad) {
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
}

int shell_wants_cpu() {
    return XtAppPending(appcontext) != 0;
}

void shell_delay(int duration) {
    struct timespec ts;
    ts.tv_sec = duration / 1000;
    ts.tv_nsec = (duration % 1000) * 1000000;
    XSync(display, False);
    nanosleep(&ts, NULL);
}

void shell_request_timeout3(int delay) {
    if (timeout3_active)
	XtRemoveTimeOut(timeout3_id);
    timeout3_id = XtAppAddTimeOut(appcontext, delay, timeout3, NULL);
    timeout3_active = 1;
}

int4 shell_read_saved_state(void *buf, int4 bufsize) {
    if (statefile == NULL)
	return -1;
    else {
	int4 n = fread(buf, 1, bufsize, statefile);
	if (n != bufsize && ferror(statefile)) {
	    fclose(statefile);
	    statefile = NULL;
	    return -1;
	} else
	    return n;
    }
}

bool shell_write_saved_state(const void *buf, int4 nbytes) {
    if (statefile == NULL)
	return false;
    else {
	int4 n = fwrite(buf, 1, nbytes, statefile);
	if (n != nbytes) {
	    fclose(statefile);
	    remove(statefilename);
	    statefile = NULL;
	    return false;
	} else
	    return true;
    }
}

int shell_get_mem() {
    FILE *meminfo = fopen("/proc/meminfo", "r");
    char line[1024];
    int bytes = 0;
    if (meminfo == NULL)
	return 0;
    while (fgets(line, 1024, meminfo) != NULL) {
	if (strncmp(line, "MemFree:", 8) == 0) {
	    int kbytes;
	    if (sscanf(line + 8, "%d", &kbytes) == 1)
		bytes = 1024 * kbytes;
	    break;
	}
    }
    fclose(meminfo);
    return bytes;
}

int shell_low_battery() {

    /* /proc/apm partial legend:
     *
     * 1.16 1.2 0x03 0x01 0x03 0x09 9% -1 ?
     *               ^^^^ ^^^^
     *                 |    +-- Battery status (0 = full, 1 = low,
     *                 |			2 = critical, 3 = charging)
     *                 +------- AC status (0 = offline, 1 = online)
     */

    FILE *apm = fopen("/proc/apm", "r");
    char line[1024];
    int lowbat = 0;
    int ac_stat, bat_stat;
    if (apm == NULL)
	goto done2;
    if (fgets(line, 1024, apm) == NULL)
	goto done1;
    if (sscanf(line, "%*s %*s %*s %x %x", &ac_stat, &bat_stat) == 2)
	lowbat = ac_stat != 1 && (bat_stat == 1 || bat_stat == 2);
    done1:
    fclose(apm);
    done2:
    if (lowbat != ann_battery) {
	ann_battery = lowbat;
	if (allow_paint)
	    skin_repaint_annunciator(5, ann_battery);
    }
    return lowbat;
}

void shell_powerdown() {
    /* We defer the actual shutdown so the emulator core can
     * return from core_keyup() or core_keydown() and isn't
     * asked to save its state while still in the middle of
     * executing the OFF instruction...
     */
    quit_flag = 1;
}

double shell_random_seed() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((tv.tv_sec * 1000000L + tv.tv_usec) & 0xffffffffL) / 4294967296.0;
}

uint4 shell_milliseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint4) (tv.tv_sec * 1000L + tv.tv_usec / 1000);
}

void shell_print(const char *text, int length,
		 const char *bits, int bytesperline,
		 int x, int y, int width, int height) {
    int xx, yy;
    int oldlength, newlength;

    for (yy = 0; yy < height; yy++) {
	int4 Y = (printout_bottom + 2 * yy) % PRINT_LINES;
	for (xx = 0; xx < 143; xx++) {
	    int bit, px, py;
	    if (xx < width) {
		char c = bits[(y + yy) * bytesperline + ((x + xx) >> 3)];
		bit = (c & (1 << ((x + xx) & 7))) != 0;
	    } else
		bit = 0;
	    for (px = xx * 2; px < (xx + 1) * 2; px++)
		for (py = Y; py < Y + 2; py++)
		    if (bit)
			print_image->data[py * PRINT_BYTESPERLINE + (px >> 3)]
			    |= 1 << (px & 7);
		    else
			print_image->data[py * PRINT_BYTESPERLINE + (px >> 3)]
			    &= ~(1 << (px & 7));
	}
    }

    oldlength = printout_bottom - printout_top;
    if (oldlength < 0)
	oldlength += PRINT_LINES;
    printout_bottom = (printout_bottom + 2 * height) % PRINT_LINES;
    newlength = oldlength + 2 * height;

    if (newlength >= PRINT_LINES) {
	int offset;
	printout_top = (printout_bottom + 2) % PRINT_LINES;
	newlength = PRINT_LINES - 2;
	if (newlength != oldlength) {
	    XtVaSetValues(print_da, XmNheight, newlength, NULL);
	    XtVaSetValues(print_sb, XmNincrement, 18, NULL);
	}
	scroll_printout_to_bottom();
	offset = 2 * height - newlength + oldlength;
	XCopyArea(display, print_canvas, print_canvas, gc,
		    0, offset, 286, oldlength - offset, 0, 0);
	repaint_printout(0, newlength - 2 * height, 286, 2 * height);
    } else {
	/* Is there a way to suppress the expose events that are caused
	 * by resizing the widget? I'd rather not have to resort to
	 * application-defined scrolling...
	 */
	XtVaSetValues(print_da, XmNheight, newlength, NULL);
	XtVaSetValues(print_sb, XmNincrement, 18, NULL);
	scroll_printout_to_bottom();
	repaint_printout(0, oldlength, 286, 2 * height);
    }

    if (state.printerToTxtFile) {
	int err;
	char buf[1000];

	if (print_txt == NULL) {
	    print_txt = fopen(state.printerTxtFileName, "a");
	    if (print_txt == NULL) {
		err = errno;
		state.printerToTxtFile = 0;
		snprintf(buf, 1000, "Can't open \"%s\" for output:\n%s (%d)\nPrinting to text file disabled.", state.printerTxtFileName, strerror(err), err);
		show_message("Message", buf);
		goto done_print_txt;
	    }
	}

	shell_spool_txt(text, length, txt_writer, txt_newliner);
	done_print_txt:;
    }

    if (state.printerToGifFile) {
	int err;
	char buf[1000];

	if (print_gif != NULL
		&& gif_lines + height > state.printerGifMaxLength) {
	    shell_finish_gif(gif_seeker, gif_writer);
	    fclose(print_gif);
	    print_gif = NULL;
	}

	if (print_gif == NULL) {
	    while (1) {
		int len, p;

		gif_seq = (gif_seq + 1) % 10000;

		strcpy(print_gif_name, state.printerGifFileName);
		len = strlen(print_gif_name);

		/* Strip ".gif" extension, if present */
		if (len >= 4 &&
			strcasecmp(print_gif_name + len - 4, ".gif") == 0) {
		    len -= 4;
		    print_gif_name[len] = 0;
		}

		/* Strip ".[0-9]+", if present */
		p = len;
		while (p > 0 && print_gif_name[p] >= '0'
			     && print_gif_name[p] <= '9')
		    p--;
		if (p < len && p >= 0 && print_gif_name[p] == '.')
		    print_gif_name[p] = 0;

		/* Make sure we have enough space for the ".nnnn.gif" */
		p = 1000 - 10;
		print_gif_name[p] = 0;
		p = strlen(print_gif_name);
		snprintf(print_gif_name + p, 6, ".%04d", gif_seq);
		strcat(print_gif_name, ".gif");

		if (!is_file(print_gif_name))
		    break;
	    }
	    print_gif = fopen(print_gif_name, "w+");
	    if (print_gif == NULL) {
		err = errno;
		state.printerToGifFile = 0;
		snprintf(buf, 1000, "Can't open \"%s\" for output:\n%s (%d)\nPrinting to GIF file disabled.", print_gif_name, strerror(err), err);
		show_message("Message", buf);
		goto done_print_gif;
	    }
	    if (!shell_start_gif(gif_writer, state.printerGifMaxLength)) {
		state.printerToGifFile = 0;
		show_message("Message", "Not enough memory for the GIF encoder.\nPrinting to GIF file disabled.");
		goto done_print_gif;
	    }
	    gif_lines = 0;
	}

	shell_spool_gif(bits, bytesperline, x, y, width, height, gif_writer);
	gif_lines += height;
	done_print_gif:;
    }
}

int shell_write(const char *buf, int4 buflen) {
    int4 written;
    if (export_file == NULL)
	return 0;
    written = fwrite(buf, 1, buflen, export_file);
    if (written != buflen) {
	char buf[1000];
	fclose(export_file);
	export_file = NULL;
	snprintf(buf, 1000, "Writing \"%s\" failed.", export_file_name);
	show_message("Message", buf);
	return 0;
    } else
	return 1;
}

int shell_read(char *buf, int4 buflen) {
    int4 nread;
    if (import_file == NULL)
	return -1;
    nread = fread(buf, 1, buflen, import_file);
    if (nread != buflen && ferror(import_file)) {
	fclose(import_file);
	import_file = NULL;
	show_message("Message",
		     "An error occurred; import was terminated prematurely.");
	return -1;
    } else
	return nread;
}

shell_bcd_table_struct *shell_get_bcd_table() {
    return NULL;
}

shell_bcd_table_struct *shell_put_bcd_table(shell_bcd_table_struct *bcdtab,
					    uint4 size) {
    return bcdtab;
}

void shell_release_bcd_table(shell_bcd_table_struct *bcdtab) {
    free(bcdtab);
}

#if 1
void logtofile(const char *message) {
    printf("%s\n", message);
}
void lognumber(int4 num) {
    printf("num=%d (0x%x)\n", num, num);
}
void logdouble(double d) {
    union {
	double d;
	struct {
	    int x, y;
	} i;
    } u;
    u.d = d;
    printf("dbl=%g (0x%08x%08x)\n", d, u.i.x, u.i.y);
}
#endif
