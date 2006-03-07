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
#include <sys/types.h>
#include <unistd.h>

#include "shell_main.h"
#include "shell_skin.h"


/* These are global because the skin code uses them a lot */

GtkWidget *calc_widget;
bool allow_paint = false;

state_type state;
char free42dirname[FILENAMELEN];


/* Private globals */

static char statefilename[FILENAMELEN];
static char printfilename[FILENAMELEN];


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

    // TODO
    strcpy(state.skinName, "Standard");


    /*********************************/
    /***** Build the main window *****/
    /*********************************/

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), TITLE);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    g_signal_connect(G_OBJECT(window), "delete_event",
		     G_CALLBACK(delete_cb), NULL);

    GtkAccelGroup *acc_grp = gtk_accel_group_new();
    GtkItemFactory *fac = gtk_item_factory_new(GTK_TYPE_MENU_BAR,
					       "<Main>", acc_grp);
    gtk_item_factory_create_items(fac, num_entries, entries, NULL);
    gtk_window_add_accel_group(GTK_WINDOW(window), acc_grp);
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
    gtk_container_add(GTK_CONTAINER(window), box);
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

    gtk_widget_show_all(window);
    gtk_widget_show(window);

    gtk_main ();
    return 0;
}

static void quit() {
    // Save state etc.
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
}
