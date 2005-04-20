/* gtkam-main.c
 *
 * Copyright © 2001 Lutz Müller <lutz@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include "gtkam-main.h"
#include "i18n.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <gtk/gtkactiongroup.h>
#include <gtk/gtkuimanager.h>
#include <gtk/gtkprogressbar.h>
#include <gtk/gtkstatusbar.h>
#include <gtk/gtktoolbar.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkhpaned.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkimage.h>

#ifdef HAVE_GNOME
#  include <libgnome/gnome-help.h>
#  include <libgnomeui/gnome-about.h>
#  include <libgnomeui/gnome-href.h>
#endif

#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-port-log.h>

#include "gtkam-cancel.h"
#include "gtkam-chooser.h"
#include "gtkam-close.h"
#include "gtkam-config.h"
#include "gtkam-context.h"
#include "gtkam-debug.h"
#include "gtkam-delete.h"
#include "gtkam-error.h"
#include "gtkam-list.h"
#include "gtkam-preview.h"
#include "gtkam-status.h"
#include "gtkam-tree.h"
#include "gtkam-version.h"

struct _GtkamMainPrivate
{
	GtkWidget *tree, *list;
	GtkUIManager *ui_manager;
	GtkWidget *status;
	GtkWidget *vbox;
	gboolean view_thumbs;
};

#define PARENT_TYPE GTK_TYPE_WINDOW
static GtkWindowClass *parent_class;

static void
gtkam_main_destroy (GtkObject *object)
{
	GtkamMain *m = GTKAM_MAIN (object);

	m = NULL;

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_main_finalize (GObject *object)
{
	GtkamMain *m = GTKAM_MAIN (object);

	g_free (m->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_main_class_init (gpointer g_class, gpointer class_data)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = GTK_OBJECT_CLASS (g_class);
	object_class->destroy  = gtkam_main_destroy;

	gobject_class = G_OBJECT_CLASS (g_class);
	gobject_class->finalize = gtkam_main_finalize;

	parent_class = g_type_class_peek_parent (g_class);
}

static void
gtkam_main_init (GTypeInstance *instance, gpointer g_class)
{
	GtkamMain *m = GTKAM_MAIN (instance);

	m->priv = g_new0 (GtkamMainPrivate, 1);
}

GType
gtkam_main_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo ti;

		memset (&ti, 0, sizeof (GTypeInfo));
		ti.class_size     = sizeof (GtkamMainClass);
		ti.class_init     = gtkam_main_class_init;
		ti.instance_size  = sizeof (GtkamMain);
		ti.instance_init  = gtkam_main_init;

		type = g_type_register_static (PARENT_TYPE, "GtkamMain",
				&ti, 0);
	}

	return (type);
}

static void
gtkam_main_update_sensitivity (GtkamMain *m)
{
	guint i, s;

	i = gtkam_list_count_all (GTKAM_LIST (m->priv->list));
	s = gtkam_list_count_selected (GTKAM_LIST (m->priv->list));

	gtk_widget_set_sensitive (
		gtk_ui_manager_get_widget (m->priv->ui_manager, "/ui/MainMenu/SelectMenu/SelectNone"),
		(s != 0));
	gtk_widget_set_sensitive (
		gtk_ui_manager_get_widget (m->priv->ui_manager, "/ui/MainMenu/FileMenu/DeletePhotos/DeleteSelected"),
		(s != 0));
	gtk_widget_set_sensitive (
		gtk_ui_manager_get_widget (m->priv->ui_manager, "/ui/MainMenu/FileMenu/SavePhotos/SaveSelected"),
		(s != 0));
	gtk_widget_set_sensitive (
		gtk_ui_manager_get_widget (m->priv->ui_manager, "/ui/MainMenu/FileMenu/DeletePhotos/DeleteAll"),
		(i != 0));
	gtk_widget_set_sensitive (
		gtk_ui_manager_get_widget (m->priv->ui_manager, "/ui/MainMenu/FileMenu/SavePhotos/SaveAll"),
		(i != 0));
	gtk_widget_set_sensitive (
		gtk_ui_manager_get_widget (m->priv->ui_manager, "/ui/MainMenu/SelectMenu/SelectAll"),
		(s != i));
	gtk_widget_set_sensitive (
		gtk_ui_manager_get_widget (m->priv->ui_manager, "/ui/MainMenu/SelectMenu/SelectInverse"),
		(i != 0));
}

static void
action_save_sel (GtkAction *action, gpointer callback_data)
{
	GtkamMain *m = GTKAM_MAIN (callback_data);

	gtkam_list_save_selected (GTKAM_LIST (m->priv->list));
}

static void
action_save_all (GtkAction *action, gpointer callback_data)
{
	GtkamMain *m = GTKAM_MAIN (callback_data);

	gtkam_list_save_all (GTKAM_LIST (m->priv->list));
}

static void
action_quit (GtkAction *action, gpointer callback_data)
{
	gtk_main_quit ();
}

static void
action_delete_sel (GtkAction *action, gpointer callback_data)
{
	GtkamMain *m = GTKAM_MAIN (callback_data);

	gtkam_list_delete_selected (GTKAM_LIST (m->priv->list));
}

static void
action_delete_all (GtkAction *action, gpointer callback_data)
{
	GtkamMain *m = GTKAM_MAIN (callback_data);

	gtkam_list_delete_all (GTKAM_LIST (m->priv->list));
}

static void
action_select_all (GtkAction *action, gpointer callback_data)
{
	GtkTreeSelection *s;
	GtkamMain *m = GTKAM_MAIN (callback_data);

	s = gtk_tree_view_get_selection (GTK_TREE_VIEW (m->priv->list));
	gtk_tree_selection_select_all (s);
	gtkam_main_update_sensitivity (m);
}

static void
action_select_none (GtkAction *action, gpointer callback_data)
{
	GtkTreeSelection *s;
	GtkamMain *m = GTKAM_MAIN (callback_data);

	s = gtk_tree_view_get_selection (GTK_TREE_VIEW (m->priv->list));
	gtk_tree_selection_unselect_all (s);
	gtkam_main_update_sensitivity (m);
}

static gboolean
select_inverse_foreach_func (GtkTreeModel *model, GtkTreePath *path,
		GtkTreeIter *iter, gpointer data)
{
	GtkamMain *m = GTKAM_MAIN (data);
	GtkTreeSelection *s;

	s = gtk_tree_view_get_selection (GTK_TREE_VIEW (m->priv->list));
	if (gtk_tree_selection_path_is_selected (s, path))
		gtk_tree_selection_unselect_path (s, path);
	else
		gtk_tree_selection_select_path (s, path);

	return (FALSE);
}

static void
action_select_inverse (GtkAction *action, gpointer callback_data)
{
	GtkTreeModel *model;
	GtkamMain *m = GTKAM_MAIN (callback_data);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (m->priv->list));
	gtk_tree_model_foreach (model, select_inverse_foreach_func, m);
	gtkam_main_update_sensitivity (m);
}

static void
on_camera_selected (GtkamChooser *chooser, GtkamCamera *camera, GtkamMain *m)
{
	g_return_if_fail (GTKAM_IS_CHOOSER (chooser));
	g_return_if_fail (GTKAM_IS_CAMERA (camera));
	g_return_if_fail (GTKAM_IS_MAIN (m));

	gtkam_tree_add_camera (GTKAM_TREE (m->priv->tree), camera);
}

static void
action_add_camera (GtkAction *action, gpointer callback_data)
{
	GtkWidget *dialog;
	GtkamMain *m = GTKAM_MAIN (callback_data);

	dialog = gtkam_chooser_new ();
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (m));
	gtk_widget_show (dialog);
	g_signal_connect (GTK_OBJECT (dialog), "camera_selected",
			G_CALLBACK (on_camera_selected), m);
}

static void
action_rescan (GtkAction *action, gpointer callback_data)
{
	GtkamMain *m = GTKAM_MAIN (callback_data);

	gtk_tree_store_clear (GTK_TREE_STORE (
			gtk_tree_view_get_model (GTK_TREE_VIEW (m->priv->tree))));
	
	gtkam_tree_load (GTKAM_TREE (m->priv->tree));
}

static void
action_view_thumbnails (GtkAction *action, gpointer callback_data)
{
	GtkamMain *m = GTKAM_MAIN (callback_data);

	if (!m->priv->list)
		return;

	m->priv->view_thumbs = !m->priv->view_thumbs;

	if (m->priv->view_thumbs)
		gtkam_list_show_thumbnails (GTKAM_LIST (m->priv->list));
	else
		gtkam_list_hide_thumbnails (GTKAM_LIST (m->priv->list));
}

static void
action_zoom_in (GtkAction *action, gpointer callback_data)
{
	GtkamMain *m = GTKAM_MAIN (callback_data);

	gtkam_list_zoom_in (GTKAM_LIST (m->priv->list));
}

static void
action_zoom_out (GtkAction *action, gpointer callback_data)
{
	GtkamMain *m = GTKAM_MAIN (callback_data);

	gtkam_list_zoom_out (GTKAM_LIST (m->priv->list));
}

static void
action_zoom_100 (GtkAction *action, gpointer callback_data)
{
	GtkamMain *m = GTKAM_MAIN (callback_data);

	gtkam_list_zoom_100 (GTKAM_LIST (m->priv->list));
}

static void
on_folder_selected (GtkamTree *tree, GtkamTreeFolderSelectedData *data,
		    GtkamMain *m)
{
	gtkam_list_add_folder (GTKAM_LIST (m->priv->list),
			       data->camera, data->folder);
	gtkam_main_update_sensitivity (m);
}

static void
on_folder_unselected (GtkamTree *tree, GtkamTreeFolderUnselectedData *data,
		      GtkamMain *m)
{
	gtkam_list_remove_folder (GTKAM_LIST (m->priv->list),
				  data->camera, data->folder);
	gtkam_main_update_sensitivity (m);
}

static void
on_tree_file_added (GtkamTree *tree, GtkamTreeFileAddedData *data,
		       GtkamMain *m)
{
	if (gtkam_list_has_folder (GTKAM_LIST (m->priv->list), data->camera,
				   data->folder))
		gtkam_list_add_file (GTKAM_LIST (m->priv->list),
			data->camera, data->folder, data->name);
}

static void
on_tree_new_error (GtkamTree *tree, GtkamTreeErrorData *e, GtkamMain *m)
{
	GtkWidget *d;

	d = gtkam_error_new (e->result, e->context, NULL, "%s", e->msg);

	gtk_window_set_transient_for (GTK_WINDOW (d), GTK_WINDOW (m));
	gtk_widget_show (d);
}

static void
on_new_dialog (GtkObject *object, GtkWidget *dialog, GtkamMain *m)
{
	g_object_ref (G_OBJECT (dialog));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (m));
	gtk_widget_show (dialog);
}

#ifdef HAVE_GNOME

static void
action_help (GtkAction *action, gpointer callback_data)
{
	GError *e = NULL;
	GtkamMain *m = GTKAM_MAIN (callback_data);
	GtkWidget *d;

	gnome_help_display ("gtkam.xml", NULL, &e);
	if (e) {
		d = gtkam_close_new (e->message);
		gtk_window_set_transient_for (GTK_WINDOW (d), GTK_WINDOW (m));
		gtk_widget_show (d);
		g_error_free (e);
	}
}

#endif

static void
action_debug (GtkAction *action, gpointer callback_data)
{
	GtkamMain *m = GTKAM_MAIN (callback_data);
	GtkWidget *d;

	d = gtkam_debug_new ();
	gtk_window_set_transient_for (GTK_WINDOW (d), GTK_WINDOW (m));
	gtk_widget_show (d);
}

#define CHECK_NULL(x) { if (x == NULL) { return; } }

static void
action_about (GtkAction *action, gpointer callback_data)
{
	GtkamMain *m = GTKAM_MAIN (callback_data);
	GtkWidget *d;
	const gchar *comments =
		N_("gtkam is a program that lets you download\n"
		   "images from many digital cameras. It uses\n"
		   "libgphoto2. More info is available at\n"
		   "http://www.gphoto.org.\n"
		   "\n"
		   "Enjoy the wonderful world of gphoto!");
#ifdef HAVE_GNOME
	GtkWidget *w;
	const gchar *authors[] = {
		"Scott Fritzinger <scottf@unr.edu>",
		"Lutz Mueller <lutz@users.sourceforge.net>",
		_("Many others"), NULL};
	const gchar *documenters[] = {
		"Michael J. Rensing <michael.rensing@shaw.ca>", NULL};
	const gchar *translator_credits =
		"Keld Simonsen <keld@dkuug.dk>\n"
		"Marcus Meissner <marcus@jet.franken.de>\n"
		"Fabian Mandelbaum <fabman@mandrakesoft.com>\n"
		"Kjartan Maraas <kmaraas@online.no>\n"
		"Andraz Tori <andraz.tori1@guest.arnes.si>";
	GdkPixbuf *p;
#else
	gchar *buf;
#endif

	gchar *gcomments = NULL;
	int n;

	for (n = 0; module_versions[n].name != NULL; n++) {
		gchar *features;
		const char **v = NULL;
		char *name = module_versions[n].name;
		GPVersionFunc func = module_versions[n].version_func;
		CHECK_NULL (name);
		CHECK_NULL (func);
		v = func(GP_VERSION_SHORT);
		CHECK_NULL (v);
		CHECK_NULL (v[0]);
		CHECK_NULL (v[1]);

		/* FIXME: implicit conversion from char to gchar */
		features = g_strjoinv(", ", &v[1]);

		if (gcomments == NULL) {
			gcomments = g_strdup_printf(_("%s\n\n%s %s with options:\n    %s\n"),
						    _(comments), name, v[0], features);
		} else {
			gchar *old = gcomments;
			gcomments = g_strdup_printf(_("%s\n%s %s with options:\n    %s\n"),
						    gcomments, name, v[0], features);
			free(old);
		}

		free(features);
	}

#ifdef HAVE_GNOME
	p = gdk_pixbuf_new_from_file (IMAGE_DIR "/gtkam-camera.png", NULL);
	d = gnome_about_new (PACKAGE, VERSION, "GPL", gcomments, authors,
			     documenters, translator_credits, p);
	g_object_unref (G_OBJECT (p));
	w = gnome_href_new ("http://www.gphoto.org", "http://www.gphoto.org");
	gtk_widget_show (w);
	gtk_box_pack_end (GTK_BOX (GTK_DIALOG (d)->vbox), w, FALSE, FALSE, 0);
#else
	buf = g_strdup_printf ("%s-%s\n\n%s", PACKAGE, VERSION, gcomments);
	d = gtkam_close_new (buf);
	g_free (buf);
#endif
	gtk_window_set_transient_for (GTK_WINDOW (d), GTK_WINDOW (m));
	gtk_widget_show (d);
	/* FIXME free(gcomments); */
}

#undef CHECK_NULL

static gboolean
selection_changed_idle (gpointer data)
{
	gtkam_main_update_sensitivity (GTKAM_MAIN (data));
	return (FALSE);
}

static void
on_file_selected (GtkamList *list, GtkamListFileSelectedData *data,
		  GtkamMain *m)
{
	gtk_idle_add (selection_changed_idle, m);
}

static void
on_file_unselected (GtkamList *list, GtkamListFileUnselectedData *data,
		    GtkamMain *m)
{
	gtk_idle_add (selection_changed_idle, m);
}

static void
gtkam_main_add_status (GtkamMain *m, GtkWidget *status)
{
	g_return_if_fail (GTKAM_IS_MAIN (m));

	g_object_ref (G_OBJECT (status));
	gtk_box_pack_start (GTK_BOX (m->priv->status), status, FALSE, FALSE, 0);
	gtk_widget_show (status);
}

static void
on_new_status (GObject *object, GtkWidget *status, GtkamMain *m)
{
	gtkam_main_add_status (m, status);
}

void
gtkam_main_load (GtkamMain *m)
{
	g_return_if_fail (GTKAM_IS_MAIN (m));

	gtkam_tree_load (GTKAM_TREE (m->priv->tree));
}

/* Normal menu items */
static GtkActionEntry entries[] = {
	{ "FileMenu", NULL, N_("_File") },
	{ "SavePhotos", NULL, N_("_Save Photos") },
	{ "DeletePhotos", NULL, N_("_Delete Photos") },
	{ "SaveSelected", GTK_STOCK_SAVE, N_("_Selected"), NULL, NULL, G_CALLBACK (action_save_sel) },
	{ "SaveAll", GTK_STOCK_SAVE, N_("_All"), NULL, NULL, G_CALLBACK (action_save_all) },
	{ "DeleteSelected", GTK_STOCK_DELETE, N_("_Selected"), NULL, NULL, G_CALLBACK (action_delete_sel) },
	{ "DeleteAll", GTK_STOCK_DELETE, N_("_All"), NULL, NULL, G_CALLBACK (action_delete_all) },
	{ "Quit", GTK_STOCK_QUIT, N_("_Quit"), NULL, NULL, G_CALLBACK (action_quit) },
	{ "ViewMenu", NULL, N_("_View") },
	{ "ZoomIn", GTK_STOCK_ZOOM_IN, N_("Zoom _In"), NULL, NULL, G_CALLBACK (action_zoom_in) },
	{ "Zoom100", GTK_STOCK_ZOOM_100, N_("Zoom _100"), NULL, NULL, G_CALLBACK (action_zoom_100) },
	{ "ZoomOut", GTK_STOCK_ZOOM_OUT, N_("Zoom _Out"), NULL, NULL, G_CALLBACK (action_zoom_out) },
	{ "SelectMenu", NULL, N_("_Select") },
	{ "SelectAll", NULL, N_("_All"), NULL, NULL, G_CALLBACK (action_select_all) },
	{ "SelectInverse", NULL, N_("_Inverse"), NULL, NULL, G_CALLBACK (action_select_inverse) },
	{ "SelectNone", NULL, N_("_None"), NULL, NULL, G_CALLBACK (action_select_none) },
	{ "CameraMenu", NULL, N_("_Camera") },
	{ "AddCamera", NULL, N_("_Add Camera..."), NULL, NULL, G_CALLBACK (action_add_camera) },
	{ "Rescan", GTK_STOCK_REFRESH, N_("_Rescan"), NULL, NULL, G_CALLBACK (action_rescan) },
	{ "HelpMenu", NULL, N_("_Help") },
#ifdef HAVE_GNOME
	{ "Help", GTK_STOCK_HELP, N_("_Contents"), NULL, NULL, G_CALLBACK (action_help) },
#endif
	{ "Debug", NULL, N_("_Debug"), NULL, NULL, G_CALLBACK (action_debug) },
	{ "About", GTK_STOCK_ABOUT, N_("_About"), NULL, NULL, G_CALLBACK (action_about) }
};

/* Toggle menu items */
static GtkToggleActionEntry toggle_entries[] = {
	{ "ViewThumbnails", NULL, N_("_View Thumbnails"), NULL, NULL, G_CALLBACK (action_view_thumbnails), TRUE }
};

static const char *ui_description =
"<ui>"
"	<menubar name='MainMenu'>"
"		<menu action='FileMenu'>"
"			<menu action='SavePhotos'>"
"				<menuitem action='SaveSelected'/>"
"				<menuitem action='SaveAll'/>"
"			</menu>"
"			<menu action='DeletePhotos'>"
"				<menuitem action='DeleteSelected'/>"
"				<menuitem action='DeleteAll'/>"
"			</menu>"
"			<separator/>"
"			<menuitem action='Quit'/>"
"		</menu>"
"		<menu action='ViewMenu'>"
"			<menuitem action='ViewThumbnails'/>"
"			<separator/>"
"			<menuitem action='ZoomIn'/>"
"			<menuitem action='Zoom100'/>"
"			<menuitem action='ZoomOut'/>"
"		</menu>"
"		<menu action='SelectMenu'>"
"			<menuitem action='SelectAll'/>"
"			<menuitem action='SelectInverse'/>"
"			<menuitem action='SelectNone'/>"
"		</menu>"
"		<menu action='CameraMenu'>"
"			<menuitem action='Rescan'/>"
"			<menuitem action='AddCamera'/>"
"		</menu>"
"		<menu action='HelpMenu'>"
#ifdef HAVE_GNOME
"			<menuitem action='Help'/>"
#endif
"			<menuitem action='Debug'/>"
"			<menuitem action='About'/>"
"		</menu>"
"	</menubar>"
"	<toolbar name='MainToolbar'>"
"		<toolitem action='ViewThumbnails'/>"
"		<toolitem action='Rescan'/>"
"		<toolitem action='ZoomIn'/>"
"		<toolitem action='Zoom100'/>"
"		<toolitem action='ZoomOut'/>"
"	</toolbar>"
"</ui>";

#ifdef ENABLE_NLS

static gchar *
translate_func (const gchar *path, gpointer data)
{
	return (_(path));
}

#endif

GtkWidget *
gtkam_main_new (void)
{
	GtkamMain *m;
	GdkPixbuf *pixbuf;
	GtkActionGroup *action_group;
	GtkUIManager *ui_manager;
	GtkAccelGroup *accel_group;
	GtkWidget *widget, *vbox, *scrolled, *hpaned, *t;

	m = g_object_new (GTKAM_TYPE_MAIN, NULL);
	gtk_window_set_title (GTK_WINDOW (m), PACKAGE);
	pixbuf = gdk_pixbuf_new_from_file (IMAGE_DIR "/gtkam-camera.png", NULL);
	gtk_window_set_icon (GTK_WINDOW (m), pixbuf);
	gdk_pixbuf_unref (pixbuf);

	m->priv->view_thumbs = TRUE;

	vbox = gtk_vbox_new (FALSE, 1);
	gtk_widget_show (vbox);
	gtk_container_add (GTK_CONTAINER (m), vbox);

	/* Menu */
	action_group = gtk_action_group_new ("MenuActions");
	gtk_action_group_add_actions (action_group, entries, G_N_ELEMENTS (entries), m);
	gtk_action_group_add_toggle_actions (action_group, toggle_entries, G_N_ELEMENTS (toggle_entries), m);
	gtk_action_group_set_translate_func (action_group, translate_func, NULL, NULL);

	ui_manager = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

	accel_group = gtk_ui_manager_get_accel_group (ui_manager);
	gtk_window_add_accel_group (GTK_WINDOW (m), accel_group);

	gtk_ui_manager_add_ui_from_string(ui_manager, ui_description, -1, NULL);
	widget = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
	m->priv->ui_manager = GTK_UI_MANAGER (ui_manager);

	/* Toolbar */
	t = gtk_ui_manager_get_widget (ui_manager, "/MainToolbar");
	gtk_toolbar_set_style (GTK_TOOLBAR (t), GTK_TOOLBAR_BOTH);
	gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);

	/*
	 * Context information
	 */
	m->priv->status = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (m->priv->status);
	gtk_box_pack_end (GTK_BOX (vbox), m->priv->status, FALSE, FALSE, 0);

	/*
	 * Main content
	 */
	hpaned = gtk_hpaned_new ();
	gtk_widget_show (hpaned);
	gtk_box_pack_start (GTK_BOX (vbox), hpaned, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hpaned), 2);
	gtk_paned_set_position (GTK_PANED (hpaned), 200);

	/*
	 * Left
	 */
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolled);
	gtk_paned_pack1 (GTK_PANED (hpaned), scrolled, TRUE, TRUE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	m->priv->tree = gtkam_tree_new ();
	gtk_widget_show (m->priv->tree);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled),
					       m->priv->tree);
	g_signal_connect (G_OBJECT (m->priv->tree), "folder_selected",
			    G_CALLBACK (on_folder_selected), m);
	g_signal_connect (G_OBJECT (m->priv->tree), "folder_unselected",
			    G_CALLBACK (on_folder_unselected), m);
	g_signal_connect (G_OBJECT (m->priv->tree), "new_status",
			  G_CALLBACK (on_new_status), m);
	g_signal_connect (G_OBJECT (m->priv->tree), "new_error",
			  G_CALLBACK (on_tree_new_error), m);
	g_signal_connect (G_OBJECT (m->priv->tree), "file_added",
			  G_CALLBACK (on_tree_file_added), m);
	g_signal_connect (G_OBJECT (m->priv->tree), "new_dialog",
			  G_CALLBACK (on_new_dialog), m);

	/*
	 * Right
	 */
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolled);
	gtk_paned_pack2 (GTK_PANED (hpaned), scrolled, TRUE, TRUE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	m->priv->list = gtkam_list_new ();
	gtk_widget_show (m->priv->list);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled),
					       m->priv->list);
	g_signal_connect (G_OBJECT (m->priv->list), "file_selected",
			  G_CALLBACK (on_file_selected), m);
	g_signal_connect (G_OBJECT (m->priv->list), "file_unselected",
			  G_CALLBACK (on_file_unselected), m);
	g_signal_connect (G_OBJECT (m->priv->list), "new_status",
			  G_CALLBACK (on_new_status), m);
	g_signal_connect (G_OBJECT (m->priv->list), "new_dialog",
			  G_CALLBACK (on_new_dialog), m);

	gtkam_main_update_sensitivity (m);

	return (GTK_WIDGET (m));
}
