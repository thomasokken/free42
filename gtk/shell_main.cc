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
#include <stdlib.h>

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

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), TITLE);
    g_signal_connect(G_OBJECT(window), "delete_event",
		     G_CALLBACK(delete_cb), NULL);

    GtkAccelGroup *acc_grp = gtk_accel_group_new();
    GtkItemFactory *fac = gtk_item_factory_new(GTK_TYPE_MENU_BAR,
					       "<Main>", acc_grp);
    gtk_item_factory_create_items(fac, num_entries, entries, NULL);
    gtk_window_add_accel_group(GTK_WINDOW(window), acc_grp);
    GtkWidget *menubar = gtk_item_factory_get_widget(fac, "<Main>");

    gtk_container_add(GTK_CONTAINER(window), menubar);

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
