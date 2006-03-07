///////////////////////////////////////////////////////////////////////////////
// Free42 -- a free HP-42S calculator clone
// Copyright (C) 2004-2006  Thomas Okken
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
///////////////////////////////////////////////////////////////////////////////

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "shell.h"
#include "shell_main.h"
#include "shell_skin.h"
#include "core_main.h"


/* These are global because the skin code uses them a lot */

GtkWidget *calc_widget;
bool allow_paint = false;

state_type state;
char free42dirname[FILENAMELEN];


/* Private globals */

static GtkWidget *mainwindow;

static FILE *statefile = NULL;
static char statefilename[FILENAMELEN];
static char printfilename[FILENAMELEN];


/* Private functions */

static void init_shell_state(int4 version);
static int read_shell_state(int4 *version);
static int write_shell_state();
static void quit();
static void quitCB();
static void showPrintOutCB();
static void exportProgramCB();
static void importProgramCB();
static void clearPrintOutCB();
static void preferencesCB();
static void copyCB();
static void pasteCB();
static void aboutCB();
static void delete_cb(GtkWidget *w, gpointer cd);
static gboolean expose_cb(GtkWidget *w, GdkEventExpose *event, gpointer cd);

#ifdef BCD_MATH
#define TITLE "Free42 Decimal"
#else
#define TITLE "Free42 Binary"
#endif

static GtkItemFactoryEntry entries[] = {
    { "/File", NULL, NULL, 0, "<Branch>" },
    { "/File/Show Print-Out", NULL, showPrintOutCB, 0, "<Item>" },
    { "/File/sep1", NULL, NULL, 0, "<Separator>" },
    { "/File/Import Program...", NULL, importProgramCB, 0, "<Item>" },
    { "/File/Export Program...", NULL, exportProgramCB, 0, "<Item>" },
    { "/File/sep2", NULL, NULL, 0, "<Separator>" },
    { "/File/Clear Print-Out", NULL, clearPrintOutCB, 0, "<Item>" },
    { "/File/Preferences...", NULL, preferencesCB, 0, "<Item>" },
    { "/File/sep3", NULL, NULL, 0, "<Separator>" },
    { "/File/Quit", "<CTRL>Q", quitCB, 0, "<Item>" },
    { "/Edit", NULL, NULL, 0, "<Branch>" },
    { "/Edit/Copy", "<CTRL>C", copyCB, 0, "<Item>" },
    { "/Edit/Paste", "<CTRL>V", pasteCB, 0, "<Item>" },
    { "/Skin", NULL, NULL, 0, "<Branch>" },
    { "/Help", NULL, NULL, 0, "<LastBranch>" },
    { "/Help/About Free42...", NULL, aboutCB, 0, "<Item>" }
};

static gint num_entries = sizeof(entries) / sizeof(entries[0]);

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    /*****************************************************/
    /***** Try to create the $HOME/.free42 directory *****/
    /*****************************************************/

    bool free42dir_exists = false;
    char *home = getenv("HOME");
    snprintf(free42dirname, FILENAMELEN, "%s/.free42", home);
    struct stat st;
    if (stat(free42dirname, &st) == -1 || !S_ISDIR(st.st_mode)) {
	mkdir(free42dirname, 0755);
	if (stat(free42dirname, &st) == 0 && S_ISDIR(st.st_mode)) {
	    char oldpath[FILENAMELEN], newpath[FILENAMELEN];
	    free42dir_exists = true;
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
	free42dir_exists = true;

    char keymapfilename[FILENAMELEN];
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

    // TODO
    //read_key_map(keymapfilename);


    /***********************************************************/
    /***** Open the state file and read the shell settings *****/
    /***********************************************************/

    int4 version;
    int init_mode;

    statefile = fopen(statefilename, "r");
    if (statefile != NULL) {
	if (read_shell_state(&version))
	    init_mode = 1;
	else {
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

    mainwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(mainwindow), TITLE);
    gtk_window_set_resizable(GTK_WINDOW(mainwindow), FALSE);
    g_signal_connect(G_OBJECT(mainwindow), "delete_event",
		     G_CALLBACK(delete_cb), NULL);
    if (state.mainWindowKnown)
	gtk_window_move(GTK_WINDOW(mainwindow), state.mainWindowX,
					    state.mainWindowY);

    GtkAccelGroup *acc_grp = gtk_accel_group_new();
    GtkItemFactory *fac = gtk_item_factory_new(GTK_TYPE_MENU_BAR,
					       "<Main>", acc_grp);
    gtk_item_factory_create_items(fac, num_entries, entries, NULL);
    gtk_window_add_accel_group(GTK_WINDOW(mainwindow), acc_grp);
    GtkWidget *menubar = gtk_item_factory_get_widget(fac, "<Main>");

    // The "Skin" menu is dynamic; we don't populate any items in it here.
    // Instead, we attach a callback which scans the .free42 directory for
    // available skins; this callback is invoked when the menu is about to
    // be mapped.
    GList *children = gtk_container_get_children(GTK_CONTAINER(menubar));
    GtkMenuItem *skin_btn = (GtkMenuItem *) children->next->next->data;
    g_list_free(children);

    g_signal_connect(G_OBJECT(skin_btn), "activate",
		     G_CALLBACK(skin_menu_update), NULL);

    GtkWidget *box = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(mainwindow), box);
    gtk_box_pack_start(GTK_BOX(box), menubar, FALSE, FALSE, 0);


    /****************************************/
    /* Drawing area for the calculator skin */
    /****************************************/

    int win_width, win_height;
    skin_load(&win_width, &win_height);
    GtkWidget *w = gtk_drawing_area_new();
    gtk_widget_set_size_request(w, win_width, win_height);
    gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(w), "expose_event", G_CALLBACK(expose_cb), NULL);
    calc_widget = w;


    /*************************************************/
    /***** Show main window & start the emulator *****/
    /*************************************************/

    gtk_widget_show_all(mainwindow);
    gtk_widget_show(mainwindow);

    core_init(init_mode, version);
    if (statefile != NULL) {
	fclose(statefile);
	statefile = NULL;
    }

    gtk_main();
    return 0;
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

static void quit() {
    gint x, y;
    gtk_window_get_position(GTK_WINDOW(mainwindow), &x, &y);
    state.mainWindowKnown = 1;
    state.mainWindowX = x;
    state.mainWindowY = y;

    statefile = fopen(statefilename, "w");
    if (statefile != NULL)
	write_shell_state();
    core_quit();
    if (statefile != NULL)
	fclose(statefile);

    exit(0);
}

static void quitCB() {
    quit();
}

static void showPrintOutCB() {
    //
}

static void exportProgramCB() {
    //
}

static void importProgramCB() {
    //
}

static void clearPrintOutCB() {
    //
}

static void preferencesCB() {
    //
}

static void copyCB() {
    //
}

static void pasteCB() {
    //
}

static void aboutCB() {
    //
}

static void delete_cb(GtkWidget *w, gpointer cd) {
    quit();
}

static gboolean expose_cb(GtkWidget *w, GdkEventExpose *event, gpointer cd) {
    allow_paint = true;
    skin_repaint();
    skin_repaint_display();
    /*
    skin_repaint_annunciator(1, ann_updown);
    skin_repaint_annunciator(2, ann_shift);
    skin_repaint_annunciator(3, ann_print);
    skin_repaint_annunciator(4, ann_run);
    skin_repaint_annunciator(5, ann_battery);
    skin_repaint_annunciator(6, ann_g);
    skin_repaint_annunciator(7, ann_rad);
    if (ckey != 0)
	skin_repaint_key(skey, 1);
    */
    return TRUE;
}

void shell_blitter(const char *bits, int bytesperline, int x, int y,
				     int width, int height) {
    skin_display_blitter(bits, bytesperline, x, y, width, height);
    // TODO
    /*
    if (skey >= -7 && skey <= -2)
	skin_repaint_key(skey, 1);
    */
}

void shell_beeper(int frequency, int duration) {
    // TODO
}

void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad) {
    // TODO
}

int shell_wants_cpu() {
    // TODO
    return 0;
}

void shell_delay(int duration) {
    // TODO
}

void shell_request_timeout3(int delay) {
    // TODO
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
     *                 |                        2 = critical, 3 = charging)
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
//    TODO
//    if (lowbat != ann_battery) {
//	ann_battery = lowbat;
//	if (allow_paint)
//	    skin_repaint_annunciator(5, ann_battery);
//    }
    return lowbat;
}

void shell_powerdown() {
    // TODO
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
    // TODO
}

int shell_write(const char *buf, int4 buflen) {
    // TODO
    return 0;
}

int shell_read(char *buf, int4 buflen) {
    // TODO
    return -1;
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
