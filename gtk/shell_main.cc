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
#include <gdk/gdkkeysyms.h>
#include <errno.h>
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
#include "core_display.h"
#include "icon.xpm"


/* These are global because the skin code uses them a lot */

GtkWidget *calc_widget;
bool allow_paint = false;

state_type state;
char free42dirname[FILENAMELEN];

static bool quit_flag = false;
static int enqueued;


/* Private globals */

static GtkWidget *mainwindow;
static char export_file_name[FILENAMELEN];
static FILE *export_file = NULL;
static FILE *import_file = NULL;
static GdkPixbuf *icon;

static int ckey = 0;
static int skey;
static bool mouse_key;
static guint16 active_keycode = 0;
static bool just_pressed_shift = false;
static guint timeout_id = 0;
static guint timeout3_id = 0;

static int keymap_length = 0;
static keymap_entry *keymap = NULL;

static guint reminder_id = 0;
static FILE *statefile = NULL;
static char statefilename[FILENAMELEN];
static char printfilename[FILENAMELEN];

static int ann_updown = 0;
static int ann_shift = 0;
static int ann_print = 0;
static int ann_run = 0;
static int ann_battery = 0;
static int ann_g = 0;
static int ann_rad = 0;


/* Private functions */

static void read_key_map(const char *keymapfilename);
static void init_shell_state(int4 version);
static int read_shell_state(int4 *version);
static int write_shell_state();
static void quit();
static void show_message(char *title, char *message);
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
static gboolean button_cb(GtkWidget *w, GdkEventButton *event, gpointer cd);
static gboolean key_cb(GtkWidget *w, GdkEventKey *event, gpointer cd);
static void enable_reminder();
static void disable_reminder();
static gboolean repeater(gpointer cd);
static gboolean timeout1(gpointer cd);
static gboolean timeout2(gpointer cd);
static gboolean timeout3(gpointer cd);
static gboolean reminder(gpointer cd);

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

    read_key_map(keymapfilename);


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

    icon = gdk_pixbuf_new_from_xpm_data((const char **) icon_xpm);

    mainwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_icon(GTK_WINDOW(mainwindow), icon);
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

    // TODO: attach an event handler to the main window that disables
    // auto-repeat on FocusIn, and re-enables it on FocusOut.


    /****************************************/
    /* Drawing area for the calculator skin */
    /****************************************/

    int win_width, win_height;
    skin_load(&win_width, &win_height);
    GtkWidget *w = gtk_drawing_area_new();
    gtk_widget_set_size_request(w, win_width, win_height);
    gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(w), "expose_event", G_CALLBACK(expose_cb), NULL);
    gtk_widget_add_events(w, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    g_signal_connect(G_OBJECT(w), "button-press-event", G_CALLBACK(button_cb), NULL);
    g_signal_connect(G_OBJECT(w), "button-release-event", G_CALLBACK(button_cb), NULL);
    GTK_WIDGET_SET_FLAGS(w, GTK_CAN_FOCUS);
    g_signal_connect(G_OBJECT(w), "key-press-event", G_CALLBACK(key_cb), NULL);
    g_signal_connect(G_OBJECT(w), "key-release-event", G_CALLBACK(key_cb), NULL);
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

    // TODO: catch INT and TERM signals
    gtk_main();
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
	guint keyval = GDK_VoidSymbol;
	bool done = false;
	unsigned char macro[KEYMAP_MAX_MACRO_LENGTH];
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
	    else {
		keyval = gdk_keyval_from_name(tok);
		if (keyval == GDK_VoidSymbol) {
		    fprintf(stderr, "Keymap, line %d: Unrecognized KeyName.\n", lineno);
		    return NULL;
		}
		done = true;
	    }
	    tok = strtok(NULL, " \t");
	}
	if (!done) {
	    fprintf(stderr, "Keymap, line %d: Unrecognized KeyName.\n", lineno);
	    return NULL;
	}

	/* Parse macro */
	tok = strtok(val, " \t");
	memset(macro, 0, KEYMAP_MAX_MACRO_LENGTH);
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
		macro[macrolen++] = k;
	    tok = strtok(NULL, " \t");
	}

	entry.ctrl = ctrl;
	entry.alt = alt;
	entry.shift = shift;
	entry.keyval = keyval;
	memcpy(entry.macro, macro, KEYMAP_MAX_MACRO_LENGTH);
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

static void show_message(char *title, char *message) {
    // TODO
    fprintf(stderr, "Message \"%s\": %s\n", title, message);
}

static void quitCB() {
    quit();
}

static void showPrintOutCB() {
    // TODO
}

static void exportProgramCB() {
    static GtkWidget *sel_dialog = NULL;
    static GtkTreeView *tree;
    static GtkTreeSelection *select;

    if (sel_dialog == NULL) {
	sel_dialog = gtk_dialog_new_with_buttons(
			    "Export Program",
			    GTK_WINDOW(mainwindow),
			    GTK_DIALOG_MODAL,
			    GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
			    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			    NULL);
	gtk_window_set_resizable(GTK_WINDOW(sel_dialog), FALSE);
	GtkWidget *container = gtk_bin_get_child(GTK_BIN(sel_dialog));
	GtkWidget *box = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(container), box);

	GtkWidget *label = gtk_label_new("Select Programs to Export:");
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 10);

	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	tree = (GtkTreeView *) gtk_tree_view_new();
	select = gtk_tree_view_get_selection(tree);
	gtk_tree_selection_set_mode(select, GTK_SELECTION_MULTIPLE);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	//gtk_cell_renderer_text_set_fixed_height_from_font((GtkCellRendererText *) renderer, 12);
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Foo", renderer, "text", 0, NULL);
	gtk_tree_view_append_column(tree, column);
	gtk_tree_view_set_headers_visible(tree, FALSE);
	gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(tree));
	gtk_widget_set_size_request(scroll, -1, 200);
	gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(scroll), FALSE, FALSE, 10);

	gtk_widget_show_all(GTK_WIDGET(sel_dialog));
    }

    char buf[10000];
    int count = core_list_programs(buf, 10000);
    char *p = buf;

    GtkListStore *model = gtk_list_store_new(1, G_TYPE_STRING);
    GtkTreeIter iter;
    while (count-- > 0) {
	gtk_list_store_append(model, &iter);
	gtk_list_store_set(model, &iter, 0, p, -1);
	p += strlen(p) + 1;
    }
    gtk_tree_view_set_model(tree, GTK_TREE_MODEL(model));

    if (gtk_dialog_run(GTK_DIALOG(sel_dialog)) == GTK_RESPONSE_ACCEPT) {
	int count = gtk_tree_selection_count_selected_rows(select);
	if (count == 0)
	    goto done;

	static GtkWidget *save_dialog = NULL;
	if (save_dialog == NULL) {
	    // TODO: This chooser comes up in "collapsed" state by default.
	    // There's probably a property to override that, so its initial
	    // state looks more like the default "Open" dialog.
	    // TODO: Filename filtering.
	    save_dialog = gtk_file_chooser_dialog_new(
				"Export Program",
				GTK_WINDOW(sel_dialog),
				GTK_FILE_CHOOSER_ACTION_SAVE,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				NULL);
	}
	char *filename = NULL;
	if (gtk_dialog_run(GTK_DIALOG(save_dialog)) == GTK_RESPONSE_ACCEPT)
	    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(save_dialog));
	gtk_widget_hide(GTK_WIDGET(save_dialog));
	if (filename == NULL)
	    goto done;

	strcpy(export_file_name, filename);
	g_free(filename);
	export_file = fopen(export_file_name, "w");

	if (export_file == NULL) {
	    char buf[1000];
	    int err = errno;
	    snprintf(buf, 1000, "Could not open \"%s\" for writing:\n%s (%d)",
		    export_file_name, strerror(err), err);
	    show_message("Message", buf);
	} else {
	    int *p2 = (int *) malloc(count * sizeof(int));
	    GList *rows = gtk_tree_selection_get_selected_rows(select, NULL);
	    GList *item = rows;
	    int i = 0;
	    while (item != NULL) {
		GtkTreePath *path = (GtkTreePath *) item->data;
		char *pathstring = gtk_tree_path_to_string(path);
		sscanf(pathstring, "%d", p2 + i);
		item = item->next;
		i++;
	    }
	    g_list_free(rows);
	    core_export_programs(count, p2, NULL);
	    free(p2);
	    if (export_file != NULL) {
		fclose(export_file);
		export_file = NULL;
	    }
	}
    }

    done:
    gtk_widget_hide(sel_dialog);

    // TODO: does this leak list-stores? Or is everything taken case of by the
    // GObject reference-counting stuff?
}

static void importProgramCB() {
    // TODO: Filename filtering.
    // BTW, In addition to GtkFileChooser, there's also something called
    // GtkFileSelection, which sounds like it's fancier. Maybe that's the
    // way to go.
    static GtkWidget *dialog = NULL;

    if (dialog == NULL) {
	dialog = gtk_file_chooser_dialog_new(
			    "Import Program",
			    GTK_WINDOW(mainwindow),
			    GTK_FILE_CHOOSER_ACTION_OPEN,
			    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			    NULL);
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
	char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	if (filename == NULL) {
	    import_file = NULL;
	    goto done;
	}
	import_file = fopen(filename, "r");
	if (import_file == NULL) {
	    char buf[1000];
	    int err = errno;
	    snprintf(buf, 1000, "Could not open \"%s\" for reading:\n%s (%d)",
		     filename, strerror(err), err);
	    g_free(filename);
	    show_message("Message", buf);
	} else {
	    g_free(filename);
	    core_import_programs(NULL);
	    redisplay();
	    if (import_file != NULL) {
		fclose(import_file);
		import_file = NULL;
	    }
	}
    }

    done:
    gtk_widget_hide(dialog);
}

static void clearPrintOutCB() {
    // TODO
}

static void preferencesCB() {
    // TODO
}

static void copyCB() {
    char buf[100];
    core_copy(buf, 100);
    GtkClipboard *clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clip, buf, -1);
    clip = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    gtk_clipboard_set_text(clip, buf, -1);
}

static void paste2(GtkClipboard *clip, const gchar *text, gpointer cd) {
    if (text == NULL)
	return;
    /* Try parsing it as a complex; if that fails, try
     * parsing it as a real, and if that fails too,
     * just paste as a string.
     */
    int len = strlen(text) + 1;
    char *text2 = (char *) malloc(len);
    if (text2 != NULL) {
	double re, im;
	strcpy(text2, text);
	core_fix_number(text2);
	if (sscanf(text2, " %lf i %lf ", &re, &im) == 2
		|| sscanf(text2, " %lf + %lf i ", &re, &im) == 2
		|| sscanf(text2, " ( %lf , %lf ) ",
		    &re, &im) == 2)
	    core_paste_complex(re, im);
	else if (sscanf(text2, " %lf ", &re) == 1)
	    core_paste_real(re);
	else
	    core_paste_string(text);
	free(text2);
    } else
	core_paste_string(text);
    redisplay();
}

static void pasteCB() {
    // TODO: When running this as a native (non-X11) Win32 app, I think I need
    // to use GDK_SELECTION_CLIPBOARD here, since GDK_SELECTION_PRIMARY will
    // probably not be populated, if it exists at all.
    GtkClipboard *clip = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    gtk_clipboard_request_text(clip, paste2, NULL);
}

static void aboutCB() {
    static GtkWidget *about = NULL;

    if (about == NULL) {
	about = gtk_dialog_new_with_buttons(
			    "About Free42",
			    GTK_WINDOW(mainwindow),
			    GTK_DIALOG_MODAL,
			    GTK_STOCK_OK,
			    GTK_RESPONSE_ACCEPT,
			    NULL);
	GtkWidget *container = gtk_bin_get_child(GTK_BIN(about));
	GtkWidget *box = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(container), box);
	GtkWidget *image = gtk_image_new_from_pixbuf(icon);
	gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 10);
	GtkWidget *label = gtk_label_new("Free42 1.4.1\n(C) 2004-2006 Thomas Okken\nthomas_okken@yahoo.com\nhttp://home.planet.nl/~demun000/thomas_projects/free42/");
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 10);
	gtk_widget_show_all(GTK_WIDGET(about));
    }

    gtk_dialog_run(GTK_DIALOG(about));
    gtk_widget_hide(GTK_WIDGET(about));
}

static void delete_cb(GtkWidget *w, gpointer cd) {
    quit();
}

static gboolean expose_cb(GtkWidget *w, GdkEventExpose *event, gpointer cd) {
    allow_paint = true;
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
    return TRUE;
}

static void shell_keydown() {
    int repeat, keep_running;
    if (skey == -1)
	skey = skin_find_skey(ckey);
    skin_repaint_key(skey, 1);
    if (timeout3_id != 0 && ckey != 28 /* KEY_SHIFT */) {
	g_source_remove(timeout3_id);
	timeout3_id = 0;
	core_timeout3(0);
    }

    if (ckey >= 38 && ckey <= 255) {
	/* Macro */
	unsigned char *macro = skin_find_macro(ckey);
	if (macro == NULL || *macro == 0) {
	    squeak();
	    return;
	}
	while (*macro != 0) {
	    keep_running = core_keydown(*macro++, &enqueued, &repeat);
	    if (*macro != 0 && !enqueued)
		core_keyup();
	}
	repeat = 0;
    } else
	keep_running = core_keydown(ckey, &enqueued, &repeat);

    if (quit_flag)
	quit();
    if (keep_running)
	enable_reminder();
    else {
	disable_reminder();
	if (timeout_id != 0)
	    g_source_remove(timeout_id);
	if (repeat)
	    timeout_id = g_timeout_add(1000, repeater, NULL);
	else if (!enqueued)
	    timeout_id = g_timeout_add(250, timeout1, NULL);
    }
}

static void shell_keyup() {
    skin_repaint_key(skey, 0);
    ckey = 0;
    skey = -1;
    if (timeout_id != 0) {
	g_source_remove(timeout_id);
	timeout_id = 0;
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

static gboolean button_cb(GtkWidget *w, GdkEventButton *event, gpointer cd) {
    if (event->type == GDK_BUTTON_PRESS) {
	if (ckey == 0) {
	    int x = (int) event->x;
	    int y = (int) event->y;
	    skin_find_key(x, y, &skey, &ckey);
	    if (ckey != 0) {
		shell_keydown();
		mouse_key = 1;
	    }
	}
    } else if (event->type == GDK_BUTTON_RELEASE) {
	if (ckey != 0 && mouse_key)
	    shell_keyup();
    }
    return TRUE;
}

static gboolean key_cb(GtkWidget *w, GdkEventKey *event, gpointer cd) {
    if (event->type == GDK_KEY_PRESS) {
	if (ckey == 0 || !mouse_key) {
	    int i;
	    unsigned char *macro;

	    char *keyname = gdk_keyval_name(event->keyval);
	    int printable = strlen(keyname) == 1 && keyname[0] >= 32 && keyname[0] <= 126;
	    just_pressed_shift = false;

	    if (event->keyval == GDK_Shift_L || event->keyval == GDK_Shift_R) {
		just_pressed_shift = true;
		return TRUE;
	    }
	    bool ctrl = (event->state & GDK_CONTROL_MASK) != 0;
	    bool alt = (event->state & GDK_MOD1_MASK) != 0;
	    bool shift = (event->state & (GDK_SHIFT_MASK | GDK_LOCK_MASK)) != 0;

	    if (ckey != 0) {
		shell_keyup();
		active_keycode = 0;
	    }

	    if (!ctrl && !alt) {
		char c = keyname[0];
		if (printable && core_alpha_menu()) {
		    if (c >= 'a' && c <= 'z')
			c = c + 'A' - 'a';
		    else if (c >= 'A' && c <= 'Z')
			c = c + 'a' - 'A';
		    ckey = 1024 + c;
		    skey = -1;
		    shell_keydown();
		    mouse_key = 0;
		    active_keycode = event->hardware_keycode;
		    return TRUE;
		} else if (core_hex_menu() && ((c >= 'a' && c <= 'f')
					    || (c >= 'A' && c <= 'F'))) {
		    if (c >= 'a' && c <= 'f')
			ckey = c - 'a' + 1;
		    else
			ckey = c - 'A' + 1;
		    skey = -1;
		    shell_keydown();
		    mouse_key = 0;
		    active_keycode = event->hardware_keycode;
		    return TRUE;
		}
	    }

	    macro = skin_keymap_lookup(event->keyval, ctrl, alt, shift);
	    if (macro == NULL) {
		for (i = 0; i < keymap_length; i++) {
		    keymap_entry *entry = keymap + i;
		    if (ctrl == entry->ctrl
			    && alt == entry->alt
			    && (printable || shift == entry->shift)
			    && event->keyval == entry->keyval) {
			macro = entry->macro;
			break;
		    }
		}
	    }
	    if (macro != NULL) {
		int j;
		for (j = 0; j < KEYMAP_MAX_MACRO_LENGTH; j++)
		    if (macro[j] == 0)
			break;
		    else {
			if (ckey != 0)
			    shell_keyup();
			ckey = macro[j];
			skey = -1;
			shell_keydown();
		    }
		mouse_key = 0;
		active_keycode = event->hardware_keycode;
	    }
	}
    } else if (event->type == GDK_KEY_RELEASE) {
	if (ckey == 0) {
	    if (just_pressed_shift && (event->keyval == GDK_Shift_L
				    || event->keyval == GDK_Shift_R)) {
		ckey = 28;
		skey = -1;
		shell_keydown();
		shell_keyup();
	    }
	} else {
	    if (!mouse_key && event->hardware_keycode == active_keycode) {
		shell_keyup();
		active_keycode = 0;
	    }
	}
    }
    return TRUE;
}

static void enable_reminder() {
    if (reminder_id == 0)
	reminder_id = g_idle_add(reminder, NULL);
    if (timeout_id != 0) {
	g_source_remove(timeout_id);
	timeout_id = 0;
    }
}

static void disable_reminder() {
    if (reminder_id != 0) {
	g_source_remove(reminder_id);
	reminder_id = 0;
    }
}

static gboolean repeater(gpointer cd) {
    core_repeat();
    timeout_id = g_timeout_add(200, repeater, NULL);
    return FALSE;
}

static gboolean timeout1(gpointer cd) {
    if (ckey != 0) {
	core_keytimeout1();
	timeout_id = g_timeout_add(1750, timeout2, NULL);
    } else
	timeout_id = 0;
    return FALSE;
}

static gboolean timeout2(gpointer cd) {
    if (ckey != 0)
	core_keytimeout2();
    timeout_id = 0;
    return FALSE;
}

static gboolean timeout3(gpointer cd) {
    core_timeout3(1);
    timeout3_id = 0;
    return FALSE;
}

static gboolean reminder(gpointer cd) {
    int dummy1, dummy2;
    int keep_running = core_keydown(0, &dummy1, &dummy2);
    if (quit_flag)
	quit();
    if (keep_running)
	return TRUE;
    else {
	reminder_id = 0;
	return FALSE;
    }
}

void shell_blitter(const char *bits, int bytesperline, int x, int y,
				     int width, int height) {
    skin_display_blitter(bits, bytesperline, x, y, width, height);
    if (skey >= -7 && skey <= -2)
	skin_repaint_key(skey, 1);
}

void shell_beeper(int frequency, int duration) {
    gdk_beep();
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
    return g_main_context_pending(NULL) ? 1 : 0;
}

void shell_delay(int duration) {
    gdk_display_flush(gdk_display_get_default());
    g_usleep(duration * 1000);
}

void shell_request_timeout3(int delay) {
    if (timeout3_id != 0)
	g_source_remove(timeout3_id);
    timeout3_id = g_timeout_add(delay, timeout3, NULL);
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
    quit_flag = true;
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
