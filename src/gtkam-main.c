/* gtkam-main.c
 *
 * Copyright (C) 2001 Lutz Müller <urc8@rz.uni-karlsruhe.de>
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

#include <config.h>
#include "gtkam-main.h"

#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif

#include <stdio.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkprogressbar.h>
#include <gtk/gtkstatusbar.h>
#include <gtk/gtktoolbar.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkhpaned.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmenubar.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkfilesel.h>

#include <gphoto2/gphoto2-camera.h>

#include "gtkam-list.h"
#include "gtkam-chooser.h"
#include "gtkam-config.h"
#include "gtkam-preview.h"
#include "gtkam-tree.h"
#include "gtkam-error.h"
#include "gtkam-close.h"
#include "gtkam-debug.h"
#include "gtkam-mkdir.h"
#include "gtkam-delete.h"

#include "support.h"

struct _GtkamMainPrivate
{
	Camera *camera;

	GtkamTree *tree;
	GtkamList *list;

	GtkStatusbar   *status;
	GtkProgressBar *progress;
	guint context_id, message_id;

	GtkToggleButton *toggle_preview;

	GtkWidget *item_delete, *item_delete_all, *item_capture, *item_config;
	GtkWidget *item_save, *item_information, *item_manual, *menu_delete;
	GtkWidget *item_about_driver;
	GtkWidget *select_all, *select_none, *select_inverse;
	GtkWidget *make_dir, *remove_dir, *upload;

	GtkWidget *vbox;

	gboolean multi;
};

#define PARENT_TYPE GTK_TYPE_WINDOW
static GtkWindowClass *parent_class;

static void
gtkam_main_destroy (GtkObject *object)
{
	GtkamMain *m = GTKAM_MAIN (object);

	if (m->priv->camera) {
		gp_camera_unref (m->priv->camera);
		m->priv->camera = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_main_finalize (GtkObject *object)
{
	GtkamMain *m = GTKAM_MAIN (object);

	g_free (m->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_main_class_init (GtkamMainClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_main_destroy;
	object_class->finalize = gtkam_main_finalize;

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_main_init (GtkamMain *main)
{
	main->priv = g_new0 (GtkamMainPrivate, 1);
}

GtkType
gtkam_main_get_type (void)
{
	static GtkType main_type = 0;

	if (!main_type) {
		static const GtkTypeInfo main_info = {
			"GtkamMain",
			sizeof (GtkamMain),
			sizeof (GtkamMainClass),
			(GtkClassInitFunc)  gtkam_main_class_init,
			(GtkObjectInitFunc) gtkam_main_init,
			NULL, NULL, NULL};
		main_type = gtk_type_unique (PARENT_TYPE, &main_info);
	}

	return (main_type);
}

static void
on_thumbnails_toggled (GtkToggleButton *toggle, GtkamMain *m)
{
	gtkam_list_set_thumbnails (m->priv->list, toggle->active);
}

static void
on_save_selected_photos_activate (GtkMenuItem *item, GtkamMain *m)
{
	gtkam_list_save_selected (m->priv->list);
}

static void
on_save_selected_photos_clicked (GtkMenuItem *item, GtkamMain *m)
{
	gtkam_list_save_selected (m->priv->list);
}

static void
on_exit_activate (GtkMenuItem *item, GtkamMain *m)
{
	gtk_object_destroy (GTK_OBJECT (m));
}

static void
on_file_deleted (GtkamDelete *delete, const gchar *path, GtkamMain *m)
{
	gchar *dir;

	dir = g_dirname (path);
	if (m->priv->list->path && !strcmp (dir, m->priv->list->path))
		gtkam_list_refresh (m->priv->list);
	g_free (dir);
}

static void
delete_selected (GtkamMain *m)
{
	GtkIconListItem *item;
	GtkamList *list = m->priv->list;
	guint i;
	GtkWidget *delete;
	GList *files = NULL;

	if (!list->path || !g_list_length (GTK_ICON_LIST (list)->selection))
		return;

	for (i = 0; i < g_list_length (GTK_ICON_LIST (list)->selection); i++) {
		item = g_list_nth_data (GTK_ICON_LIST (list)->selection, i);
		files = g_list_append (files, item->label);
	}
	delete = gtkam_delete_new (m->priv->camera, m->priv->multi,
				   list->path, files, GTK_WIDGET (m));
	gtk_widget_show (delete);
	gtk_signal_connect (GTK_OBJECT (delete), "file_deleted",
			    GTK_SIGNAL_FUNC (on_file_deleted), m);
}

static void
on_delete_selected_photos_activate (GtkMenuItem *item, GtkamMain *m)
{
	delete_selected (m);
}

static void
on_all_deleted (GtkamDelete *delete, const gchar *path, GtkamMain *m)
{
	g_return_if_fail (GTKAM_IS_MAIN (m));

	if (m->priv->list->path && !strcmp (path, m->priv->list->path))
		gtkam_list_refresh (m->priv->list);
}

static void
on_delete_selected_photos_clicked (GtkButton *button, GtkamMain *m)
{
	delete_selected (m);
}

static void
on_delete_all_photos_activate (GtkMenuItem *item, GtkamMain *m)
{
	GtkWidget *delete;

	if (!m->priv->list->path)
		return;

	delete = gtkam_delete_new (m->priv->camera, m->priv->multi,
				   m->priv->list->path, NULL, GTK_WIDGET (m));
	gtk_widget_show (delete);
	gtk_signal_connect (GTK_OBJECT (delete), "file_deleted",
			    GTK_SIGNAL_FUNC (on_file_deleted), m);
	gtk_signal_connect (GTK_OBJECT (delete), "all_deleted",
			    GTK_SIGNAL_FUNC (on_all_deleted), m);
}

static void
on_select_all_activate (GtkMenuItem *item, GtkamMain *m)
{
	GtkIconList *ilist = GTK_ICON_LIST (m->priv->list);
	guint i;

	for (i = 0; i < g_list_length (ilist->icons); i++)
		gtk_icon_list_select_icon (ilist,
				g_list_nth_data (ilist->icons, i));
}

static void
on_select_none_activate (GtkMenuItem *item, GtkamMain *m)
{
	gtk_icon_list_unselect_all (GTK_ICON_LIST (m->priv->list));
}

static void
on_select_inverse_activate (GtkMenuItem *menu_item, GtkamMain *m)
{
	GtkIconList *ilist = GTK_ICON_LIST (m->priv->list);
	GtkIconListItem *item;
	guint i;

	for (i = 0; i < g_list_length (ilist->icons); i++) {
		item = g_list_nth_data (ilist->icons, i);
		if (item->state == GTK_STATE_SELECTED)
			gtk_icon_list_unselect_icon (ilist, item);
		else
			gtk_icon_list_select_icon (ilist, item);
	}
}

static void
on_camera_selected (GtkamChooser *chooser, Camera *camera,
		    gboolean multi, GtkamMain *m)
{
	gtkam_main_set_camera (m, camera, multi);
}

static void
on_select_camera_activate (GtkMenuItem *item, GtkamMain *m)
{
	GtkWidget *dialog;

	dialog = gtkam_chooser_new ();
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (m));
	gtk_widget_show (dialog);
	gtk_signal_connect (GTK_OBJECT (dialog), "camera_selected",
			    GTK_SIGNAL_FUNC (on_camera_selected), m);
}

static void
on_captured (GtkamPreview *preview, const gchar *path, GtkamMain *m)
{
	gchar *dirname;

	dirname = g_dirname (path);
	if (m->priv->list->path && !strcmp (dirname, m->priv->list->path))
		gtkam_list_refresh (m->priv->list);
	g_free (dirname);
}

static void
on_capture_activate (GtkMenuItem *item, GtkamMain *m)
{
	CameraAbilities a;
	GtkWidget *dialog;
	int result;
	CameraFilePath path;

	if (!m->priv->camera)
		return;

	/* Let's first check if the camera supports previews */
	gp_camera_get_abilities (m->priv->camera, &a);
	if (a.operations & GP_OPERATION_CAPTURE_PREVIEW) {
		dialog = gtkam_preview_new (m->priv->camera, m->priv->multi);
		gtk_window_set_transient_for (GTK_WINDOW (dialog),
					      GTK_WINDOW (m));
		gtk_widget_show (dialog);
		gtk_signal_connect (GTK_OBJECT (dialog), "captured",
				    GTK_SIGNAL_FUNC (on_captured), m);
		return;
	}

	/* The camera doesn't support previews. Capture an image */
	result = gp_camera_capture (m->priv->camera, GP_CAPTURE_IMAGE, &path);
	if (m->priv->multi)
		gp_camera_exit (m->priv->camera);
	if (result < 0) {
		dialog = gtkam_error_new (_("Could not capture"), result,
					  m->priv->camera, GTK_WIDGET (m));
		gtk_widget_show (dialog);
	} else
		if (m->priv->list->path && !strcmp (path.folder,
						    m->priv->list->path))
			gtkam_list_refresh (m->priv->list);
}

static void
on_configure_activate (GtkMenuItem *item, GtkamMain *m)
{
	GtkWidget *dialog;

	if (!m->priv->camera)
		return;

	dialog = gtkam_config_new (m->priv->camera);
	if (!dialog) {

		/* The error has already been reported */
		return;
	}

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (m));
	gtk_widget_show (dialog);
}

static void
on_configure_camera_clicked (GtkButton *button, GtkamMain *m)
{
	on_configure_activate (NULL, m);
}

static void
on_size_allocate (GtkWidget *widget, GtkAllocation *allocation, GtkamMain *m)
{
	gtk_icon_list_update (GTK_ICON_LIST (m->priv->list));
}

static void
gtkam_main_update_sensitivity (GtkamMain *m)
{
	CameraAbilities a;
	guint i, s;

	/* Make sure we are not shutting down */
	while (gtk_events_pending ())
		gtk_main_iteration ();
	if (!GTKAM_IS_MAIN (m) || !GTK_IS_WIDGET (m->priv->make_dir))
		return;

	if (!m->priv->camera ||
	    !g_list_length (GTK_TREE (m->priv->tree)->selection)) {
		gtk_widget_set_sensitive (m->priv->make_dir, FALSE);
		gtk_widget_set_sensitive (m->priv->remove_dir, FALSE);
		gtk_widget_set_sensitive (m->priv->upload, FALSE);
		return;
	}

	gp_camera_get_abilities (m->priv->camera, &a);

	/* Directory creation and removal */
	if (a.folder_operations & GP_FOLDER_OPERATION_MAKE_DIR)
		gtk_widget_set_sensitive (m->priv->make_dir, TRUE);
	else
		gtk_widget_set_sensitive (m->priv->make_dir, FALSE);
	if (a.folder_operations & GP_FOLDER_OPERATION_REMOVE_DIR)
		gtk_widget_set_sensitive (m->priv->remove_dir, TRUE);
	else
		gtk_widget_set_sensitive (m->priv->remove_dir, FALSE);

	/* Upload */
	if (a.folder_operations & GP_FOLDER_OPERATION_PUT_FILE)
		gtk_widget_set_sensitive (m->priv->upload, TRUE);
	else
		gtk_widget_set_sensitive (m->priv->upload, FALSE);

	/* Select */
	i = g_list_length (GTK_ICON_LIST (m->priv->list)->icons);
	s = g_list_length (GTK_ICON_LIST (m->priv->list)->selection);
	gtk_widget_set_sensitive (m->priv->select_none, (s != 0));
	gtk_widget_set_sensitive (m->priv->select_all, (s != i));
	gtk_widget_set_sensitive (m->priv->select_inverse, (i != 0));
}

static void
on_folder_selected (GtkamTree *tree, const gchar *folder, GtkamMain *m)
{
	/*
	 * Don't let the user switch folders while the list is downloading
	 * the file listing or thumbnails. If you want to give the user this
	 * possibility, you need to fix a reentrancy issue first.
	 */
	gtk_widget_set_sensitive (m->priv->vbox, FALSE);
	gtkam_list_set_path (m->priv->list, folder);
	gtk_widget_set_sensitive (m->priv->vbox, TRUE);

	gtkam_main_update_sensitivity (m);
}

static void
on_folder_unselected (GtkamTree *tree, const gchar *folder, GtkamMain *m)
{
	gtkam_list_set_path (m->priv->list, NULL);
	gtkam_main_update_sensitivity (m);
}

static void
on_information_activate (GtkMenuItem *item, GtkamMain *m)
{
	int result;
	CameraText text;
	GtkWidget *dialog;

	if (!m->priv->camera)
		return;

	result = gp_camera_get_summary (m->priv->camera, &text);
	if (m->priv->multi)
		gp_camera_exit (m->priv->camera);
	if (result < 0)
		dialog = gtkam_error_new (_("Could not retrieve information"),
					  result, m->priv->camera,
					  GTK_WIDGET (m));
	else
		dialog = gtkam_close_new (text.text, GTK_WIDGET (m));
	gtk_widget_show (dialog);
}

static void
on_manual_activate (GtkMenuItem *item, GtkamMain *m)
{
	int result;
	CameraText text;
	GtkWidget *dialog;

	if (!m->priv->camera)
		return;

	result = gp_camera_get_manual (m->priv->camera, &text);
	if (m->priv->multi)
		gp_camera_exit (m->priv->camera);
	if (result < 0)
		dialog = gtkam_error_new (_("Could not retrieve manual"),
					  result, m->priv->camera,
					  GTK_WIDGET (m));
	else
		dialog = gtkam_close_new (text.text, GTK_WIDGET (m));
	gtk_widget_show (dialog);
}

static void
on_about_driver_activate (GtkMenuItem *item, GtkamMain *m)
{
	int result;
	CameraText text;
	GtkWidget *dialog;

	if (!m->priv->camera)
		return;

	result = gp_camera_get_about (m->priv->camera, &text);
	if (m->priv->multi)
		gp_camera_exit (m->priv->camera);
	if (result < 0)
		dialog = gtkam_error_new (_("Could not get information "
					  "about the driver"), result, 
					  m->priv->camera, GTK_WIDGET (m));
	else
		dialog = gtkam_close_new (text.text, GTK_WIDGET (m));
	gtk_widget_show (dialog);
}

static void
on_debug_activate (GtkMenuItem *item, GtkamMain *m)
{
	GtkWidget *debug;

	debug = gtkam_debug_new ();
	gtk_widget_show (debug);
}

static void
on_about_activate (GtkMenuItem *item, GtkamMain *m)
{
	GtkWidget *dialog;
	char buf[4096];
	
	snprintf(buf, sizeof(buf), 
		 N_("%s %s\n\n"
		    "gtKam was written by:\n"
		    " - Scott Fritzinger <scottf@unr.edu>,\n"
		    " - Lutz Mueller <urc8@rz.uni-karlsruhe.de>,\n"
		    " - and many others.\n"
		    "\n"
		    "gtKam uses libgphoto2, a library to access a\n"
		    "multitude of digital cameras. More \n"
		    "information is available at\n"
		    "http://www.gphoto.net.\n"
		    "\n"
		    "Enjoy the wonderful world of gphoto!"),
		 PACKAGE, VERSION);

	dialog = gtkam_close_new (buf, GTK_WIDGET (m));
	gtk_widget_show (dialog);
}

static void
on_dir_created (GtkamMkdir *mkdir, const gchar *path, GtkamMain *m)
{
	gtkam_tree_make_dir (m->priv->tree, path);
}

static void
on_make_dir_activate (GtkMenuItem *item, GtkamMain *m)
{
	GtkWidget *mkdir;
	const gchar *path;

	path = gtkam_tree_get_path (m->priv->tree);
	mkdir = gtkam_mkdir_new (m->priv->camera, path, GTK_WIDGET (m));
	gtk_widget_show (mkdir);
	gtk_signal_connect (GTK_OBJECT (mkdir), "dir_created",
			    GTK_SIGNAL_FUNC (on_dir_created), m);
}

static void
on_remove_dir_activate (GtkMenuItem *item, GtkamMain *m)
{
	const gchar *path;
	gchar *dirname, *msg;
	int result;
	GtkWidget *dialog;

	path = gtkam_tree_get_path (m->priv->tree);
	dirname = g_dirname (path);
	result = gp_camera_folder_remove_dir (m->priv->camera, dirname,
					      g_basename (path));
	if (m->priv->multi)
		gp_camera_exit (m->priv->camera);
	if (result < 0) {
		msg = g_strdup_printf (_("Could not remove '%s' from '%s'"),
				       g_basename (path), dirname);
		dialog = gtkam_error_new (msg, result, m->priv->camera,
					  GTK_WIDGET (m));
		g_free (msg);
		gtk_widget_show (dialog);
	} else
		gtkam_tree_remove_dir (m->priv->tree, path);
	g_free (dirname);
}

static void
on_upload_ok_clicked (GtkButton *button, gboolean *ok)
{
	*ok = TRUE;
	gtk_main_quit ();
}

static void
on_upload_activate (GtkMenuItem *item, GtkamMain *m)
{
	CameraFile *file;
	GtkWidget *fsel, *dialog;
	gboolean ok = FALSE;
	const char *path;
	int r;
	gchar *msg;
	const char *folder;

	folder = gtkam_tree_get_path (m->priv->tree);

	fsel = gtk_file_selection_new (_("Upload"));
	gtk_widget_show (fsel);
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (fsel)->ok_button),
		"clicked", GTK_SIGNAL_FUNC (on_upload_ok_clicked), &ok);
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (fsel)->cancel_button),
		"clicked", GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
	gtk_signal_connect (GTK_OBJECT (fsel), "delete_event",
			    GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
	gtk_main ();

	if (ok) {
		path = gtk_file_selection_get_filename (
						GTK_FILE_SELECTION (fsel));
		gp_file_new (&file);
		r = gp_file_open (file, path);
		if (r < 0) {
			msg = g_strdup_printf (_("Could not open '%s'"), path);
			dialog = gtkam_error_new (msg, r, NULL, GTK_WIDGET (m));
			g_free (msg);
			gtk_widget_show (dialog);
		} else {
			r = gp_camera_folder_put_file (m->priv->camera,
						       folder, file);
			if (m->priv->multi)
				gp_camera_exit (m->priv->camera);
			if (r < 0) {
				msg = g_strdup_printf (_("Coult not upload "
					"'%s' into folder '%s'"), path, folder);
				dialog = gtkam_error_new (msg, r,
					m->priv->camera, GTK_WIDGET (m));
				g_free (msg);
				gtk_widget_show (dialog);
			} else
				gtkam_list_refresh (m->priv->list);
		}
		gp_file_unref (file);
	}

	gtk_object_destroy (GTK_OBJECT (fsel));
}

static gboolean
on_select_icon (GtkIconList *list, GtkIconListItem *item, GdkEvent *event,
		GtkamMain *m)
{
	guint i, s;

	gtkam_main_update_sensitivity (m);

	/*
	 * The problem is that the icon has not yet been selected. Therefore,
	 * we have to update the sensitivity manually.
	 */
	i = g_list_length (GTK_ICON_LIST (m->priv->list)->icons);
	s = g_list_length (GTK_ICON_LIST (m->priv->list)->selection) + 1;
	gtk_widget_set_sensitive (m->priv->select_none, (s != 0));
	gtk_widget_set_sensitive (m->priv->select_all, (s != i));
	gtk_widget_set_sensitive (m->priv->select_inverse, (i != 0)); 

	return (TRUE);
}

static void
on_unselect_icon (GtkIconList *list, GtkIconListItem *item, GdkEvent *event,
		  GtkamMain *m)
{
	gtkam_main_update_sensitivity (m);
}

static void
on_changed (GtkamList *list, GtkamMain *m)
{
	gtkam_main_update_sensitivity (m);
}

GtkWidget *
gtkam_main_new (void)
{
	GtkamMain *m;
	GtkWidget *vbox, *menubar, *menu, *item, *separator, *submenu;
	GtkWidget *frame, *scrolled, *check, *tree, *list, *label, *hbox;
	GtkWidget *button, *hpaned, *toolbar, *icon, *status, *progress;
	GtkAccelGroup *accel_group, *accels, *subaccels;
	GtkTooltips *tooltips;
	guint key;

	m = gtk_type_new (GTKAM_TYPE_MAIN);

	gtk_window_set_title (GTK_WINDOW (m), PACKAGE);
	gtk_window_set_default_size (GTK_WINDOW (m), 640, 480);
	gtk_window_set_policy (GTK_WINDOW (m), TRUE, TRUE, TRUE);
	gtk_signal_connect (GTK_OBJECT (m), "delete_event",
			    GTK_SIGNAL_FUNC (gtk_object_destroy), NULL);

	vbox = gtk_vbox_new (FALSE, 1);
	gtk_widget_show (vbox);
	gtk_container_add (GTK_CONTAINER (m), vbox);

	menubar = gtk_menu_bar_new ();
	gtk_widget_show (menubar);
	gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);

	accel_group = gtk_accel_group_new ();
	tooltips = gtk_tooltips_new ();

	/*
	 * File menu
	 */
	item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_File"));
	gtk_widget_add_accelerator (item, "activate_item", accel_group, key,
				    GDK_MOD1_MASK, 0);
	gtk_container_add (GTK_CONTAINER (menubar), item);

	menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
	accels = gtk_menu_ensure_uline_accel_group (GTK_MENU (menu));

	item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	gtk_widget_set_sensitive (item, FALSE);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_Save Selected Photos..."));
	gtk_widget_add_accelerator (item, "activate_item", accels, key, 0, 0);
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_widget_add_accelerator (item, "activate", accels, GDK_s,
				    GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
		GTK_SIGNAL_FUNC (on_save_selected_photos_activate), m);
	m->priv->item_save = item;

	item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_Delete"));
	gtk_widget_add_accelerator (item, "activate_item", accels, key, 0, 0);
	gtk_container_add (GTK_CONTAINER (menu), item);
	m->priv->menu_delete = item;

	submenu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
	subaccels = gtk_menu_ensure_uline_accel_group (GTK_MENU (submenu));

	item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	gtk_widget_set_sensitive (item, FALSE);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_Selected Photos"));
	gtk_widget_add_accelerator (item, "activate_item", subaccels,
				    key, 0, 0);
	gtk_container_add (GTK_CONTAINER (submenu), item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
		GTK_SIGNAL_FUNC (on_delete_selected_photos_activate), m);
	m->priv->item_delete = item;

	item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_All Photos"));
	gtk_widget_add_accelerator (item, "activate_item", subaccels,
				    key, 0, 0);
	gtk_container_add (GTK_CONTAINER (submenu), item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
		GTK_SIGNAL_FUNC (on_delete_all_photos_activate), m);
	m->priv->item_delete_all = item;
	gtk_widget_set_sensitive (item, FALSE);

	separator = gtk_menu_item_new ();
	gtk_widget_show (separator);
	gtk_container_add (GTK_CONTAINER (menu), separator);
	gtk_widget_set_sensitive (separator, FALSE);

	item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_Exit"));
	gtk_widget_add_accelerator (item, "activate_item", accels, key, 0, 0);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
			    GTK_SIGNAL_FUNC (on_exit_activate), m);
	gtk_container_add (GTK_CONTAINER (menu), item);

	/*
	 * Folder menu
	 */
	item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("F_older"));
	gtk_widget_add_accelerator (item, "activate_item", accel_group, key,
				    GDK_MOD1_MASK, 0);
	gtk_container_add (GTK_CONTAINER (menubar), item);
	
	menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
	accels = gtk_menu_ensure_uline_accel_group (GTK_MENU (menu));

	item = item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	gtk_widget_set_sensitive (item, FALSE);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_Create"));
	gtk_widget_add_accelerator (item, "activate_item", accels, key, 0, 0);
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
			    GTK_SIGNAL_FUNC (on_make_dir_activate), m);
	m->priv->make_dir = item;

	item = item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	gtk_widget_set_sensitive (item, FALSE);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_Remove"));
	gtk_widget_add_accelerator (item, "activate_item", accels, key, 0, 0);
	gtk_container_add (GTK_CONTAINER (menu), item); 
	gtk_signal_connect (GTK_OBJECT (item), "activate", 
			    GTK_SIGNAL_FUNC (on_remove_dir_activate), m);
	m->priv->remove_dir = item;

	separator = gtk_menu_item_new (); 
	gtk_widget_show (separator);
	gtk_container_add (GTK_CONTAINER (menu), separator);
	gtk_widget_set_sensitive (separator, FALSE);

	item = item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	gtk_widget_set_sensitive (item, FALSE);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_Upload file")); 
	gtk_widget_add_accelerator (item, "activate_item", accels, key, 0, 0);
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
			    GTK_SIGNAL_FUNC (on_upload_activate), m);
	m->priv->upload = item;

	/*
	 * Select menu
	 */
	item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_Select"));
	gtk_widget_add_accelerator (item, "activate_item", accel_group, key,
				    GDK_MOD1_MASK, 0);
	gtk_container_add (GTK_CONTAINER (menubar), item);

	menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
	accels = gtk_menu_ensure_uline_accel_group (GTK_MENU (menu));

	item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	gtk_widget_set_sensitive (item, FALSE);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_All"));
	gtk_widget_add_accelerator (item, "activate_item", accels, key, 0, 0);
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
			    GTK_SIGNAL_FUNC (on_select_all_activate), m);
	m->priv->select_all = item;

	item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	gtk_widget_set_sensitive (item, FALSE);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_Inverse"));
	gtk_widget_add_accelerator (item, "activate_item", accels, key, 0, 0);
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
			    GTK_SIGNAL_FUNC (on_select_inverse_activate), m);
	m->priv->select_inverse = item;

	item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	gtk_widget_set_sensitive (item, FALSE);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_None"));
	gtk_widget_add_accelerator (item, "activate_item", accels, key, 0, 0);
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
			    GTK_SIGNAL_FUNC (on_select_none_activate), m);
	m->priv->select_none = item;

	/*
	 * Camera menu
	 */
	item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_Camera"));
	gtk_widget_add_accelerator (item, "activate_item", accel_group, key,
				    GDK_MOD1_MASK, 0);
	gtk_container_add (GTK_CONTAINER (menubar), item);

	menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
	accels = gtk_menu_ensure_uline_accel_group (GTK_MENU (menu));

	item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_Select Camera..."));
	gtk_widget_add_accelerator (item, "activate_item", accels, key, 0, 0);
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
			    GTK_SIGNAL_FUNC (on_select_camera_activate), m);

	separator = gtk_menu_item_new ();
	gtk_widget_show (separator);
	gtk_container_add (GTK_CONTAINER (menu), separator);
	gtk_widget_set_sensitive (separator, FALSE);

	item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	gtk_widget_set_sensitive (item, FALSE);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("Ca_pture"));
	gtk_widget_add_accelerator (item, "activate_item", accels, key, 0, 0);
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
			    GTK_SIGNAL_FUNC (on_capture_activate), m);
	m->priv->item_capture = item;

	item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	gtk_widget_set_sensitive (item, FALSE);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_Configure"));
	gtk_widget_add_accelerator (item, "activate_item", accels, key, 0, 0);
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
			    GTK_SIGNAL_FUNC (on_configure_activate), m);
	m->priv->item_config = item;

	item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	gtk_widget_set_sensitive (item, FALSE);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_Information"));
	gtk_widget_add_accelerator (item, "activate_item", accels, key, 0, 0);
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
			    GTK_SIGNAL_FUNC (on_information_activate), m);
	m->priv->item_information = item;

	item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	gtk_widget_set_sensitive (item, FALSE);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_Manual"));
	gtk_widget_add_accelerator (item, "activate_item", accels, key, 0, 0);
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
			    GTK_SIGNAL_FUNC (on_manual_activate), m);
	m->priv->item_manual = item;

	item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	gtk_widget_set_sensitive (item, FALSE);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_About the Driver"));
	gtk_widget_add_accelerator (item, "activate_item", accels, key, 0, 0);
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
			    GTK_SIGNAL_FUNC (on_about_driver_activate), m);
	m->priv->item_about_driver = item;

	/*
	 * Help menu
	 */
	item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_Help"));
	gtk_widget_add_accelerator (item, "activate_item", accel_group,
				    key, 0, 0);
	gtk_container_add (GTK_CONTAINER (menubar), item);

	menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
	accels = gtk_menu_ensure_uline_accel_group (GTK_MENU (menu));

	item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_Debug"));
	gtk_widget_add_accelerator (item, "activate_item", accels, key, 0, 0);
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
			    GTK_SIGNAL_FUNC (on_debug_activate), m);

	item = gtk_menu_item_new_with_label ("");
	gtk_widget_show (item);
	key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (item)->child),
				     _("_About"));
	gtk_widget_add_accelerator (item, "activate_item", accels, key, 0, 0);
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
			    GTK_SIGNAL_FUNC (on_about_activate), m);

	/*
	 * Toolbar
	 */
	toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL,
				   GTK_TOOLBAR_ICONS);
	gtk_widget_show (toolbar);
	gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);

	icon = create_pixmap (GTK_WIDGET (m), "save_current_image.xpm");
	button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
			GTK_TOOLBAR_CHILD_BUTTON, NULL, NULL, NULL, NULL,
			icon, NULL, NULL);
	gtk_widget_show (button);
	gtk_tooltips_set_tip (tooltips, button, _("Save selected photos"),
			      NULL);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
		GTK_SIGNAL_FUNC (on_save_selected_photos_clicked), m);

	icon = create_pixmap (GTK_WIDGET (m), "delete_images.xpm");
	button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
			GTK_TOOLBAR_CHILD_BUTTON, NULL, NULL, NULL, NULL,
			icon, NULL, NULL);
	gtk_widget_show (button);
	gtk_tooltips_set_tip (tooltips, button, _("Delete selected photos"),
			      NULL);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
		GTK_SIGNAL_FUNC (on_delete_selected_photos_clicked), m);

	label = gtk_label_new ("      ");
	gtk_widget_show (label);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar), label, NULL, NULL);

	icon = create_pixmap (GTK_WIDGET (m), "configure.xpm");
	button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
			GTK_TOOLBAR_CHILD_BUTTON, NULL, NULL, NULL, NULL,
			icon, NULL, NULL);
	gtk_widget_show (button);
	gtk_tooltips_set_tip (tooltips, button, _("Configure camera"), NULL);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
		GTK_SIGNAL_FUNC (on_configure_camera_clicked), m);

	label = gtk_label_new ("      ");
	gtk_widget_show (label);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar), label, NULL, NULL);

	icon = create_pixmap (GTK_WIDGET (m), "exit.xpm");
	button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
			GTK_TOOLBAR_CHILD_BUTTON, NULL, NULL, NULL, NULL,
			icon, NULL, NULL);
	gtk_widget_show (button);
	gtk_tooltips_set_tip (tooltips, button, _("Exit"), NULL);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
		GTK_SIGNAL_FUNC (on_exit_activate), m);

	/*
	 * Status and progress bar
	 */
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	status = gtk_statusbar_new ();
	gtk_widget_show (status);
	gtk_box_pack_start (GTK_BOX (hbox), status, TRUE, TRUE, 0);
	m->priv->status = GTK_STATUSBAR (status);
	m->priv->context_id = gtk_statusbar_get_context_id (m->priv->status,
							    "libgphoto2");

	progress = gtk_progress_bar_new ();
	gtk_widget_show (progress);
	gtk_box_pack_end (GTK_BOX (hbox), progress, FALSE, FALSE, 0);
	m->priv->progress = GTK_PROGRESS_BAR (progress);

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
	vbox = gtk_vbox_new (FALSE, 5);
	gtk_widget_show (vbox);
	gtk_paned_pack1 (GTK_PANED (hpaned), vbox, FALSE, TRUE);
	m->priv->vbox = vbox;

	frame = gtk_frame_new (_("Index Settings"));
	gtk_widget_show (frame);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

	check = gtk_check_button_new_with_label (_("View Thumbnails"));
	gtk_widget_show (check);
	gtk_widget_set_sensitive (check, FALSE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), TRUE);
	gtk_container_add (GTK_CONTAINER (frame), check);
	gtk_signal_connect (GTK_OBJECT (check), "toggled",
			    GTK_SIGNAL_FUNC (on_thumbnails_toggled), m);
	m->priv->toggle_preview = GTK_TOGGLE_BUTTON (check);

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolled);
	gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	tree = gtkam_tree_new ();
	gtk_widget_show (tree);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled),
					       tree);
	gtk_signal_connect (GTK_OBJECT (tree), "folder_selected",
			    GTK_SIGNAL_FUNC (on_folder_selected), m);
	gtk_signal_connect (GTK_OBJECT (tree), "folder_unselected",
			    GTK_SIGNAL_FUNC (on_folder_unselected), m);
	m->priv->tree = GTKAM_TREE (tree);

	/*
	 * Right
	 */
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolled);
	gtk_paned_pack2 (GTK_PANED (hpaned), scrolled, TRUE, TRUE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	list = gtkam_list_new ();
	gtk_widget_show (list);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled),
					       list);
	m->priv->list = GTKAM_LIST (list);
	gtk_signal_connect (GTK_OBJECT (list), "changed",
			    GTK_SIGNAL_FUNC (on_changed), m);
	gtk_signal_connect (GTK_OBJECT (list), "select_icon",
			    GTK_SIGNAL_FUNC (on_select_icon), m);
	gtk_signal_connect (GTK_OBJECT (list), "unselect_icon",
			    GTK_SIGNAL_FUNC (on_unselect_icon), m);
	gtk_signal_connect (GTK_OBJECT (scrolled), "size_allocate",
			    GTK_SIGNAL_FUNC (on_size_allocate), m);

	return (GTK_WIDGET (m));
}

static void
status_func (Camera *camera, const char *status, void *data)
{
	GtkamMain *m = GTKAM_MAIN (data);

	if (m->priv->message_id)
		gtk_statusbar_remove (m->priv->status, m->priv->context_id,
				      m->priv->message_id);
	if (status && *status)
		m->priv->message_id = gtk_statusbar_push (m->priv->status,
						m->priv->context_id, status);
	while (gtk_events_pending ())
		gtk_main_iteration ();
}

static void
message_func (Camera *camera, const char *message, void *data)
{
	GtkamMain *m = GTKAM_MAIN (data);
	GtkWidget *dialog;

	dialog = gtkam_close_new (message, GTK_WIDGET (m));
	gtk_widget_show (dialog);
}

static void
progress_func (Camera *camera, float progress, void *data)
{
	GtkamMain *m = data;

	/* Make sure we are not shutting down */
	if (!GTKAM_IS_MAIN (data))
		return;

	gtk_progress_bar_update (m->priv->progress, progress);
	while (gtk_events_pending ())
		gtk_main_iteration ();
}

void
gtkam_main_set_camera (GtkamMain *m, Camera *camera, gboolean multi)
{
	CameraAbilities a;

	g_return_if_fail (GTKAM_IS_MAIN (m));
	g_return_if_fail (camera != NULL);

	if (camera) {
		gp_camera_set_progress_func (camera, progress_func, m);
		gp_camera_set_status_func (camera, status_func, m);
		gp_camera_set_message_func (camera, message_func, m);
	}

	if (m->priv->camera)
		gp_camera_unref (m->priv->camera);
	m->priv->camera = camera;
	if (camera)
		gp_camera_ref (camera);
	m->priv->multi = multi;

	gp_camera_get_abilities (camera, &a);

	/* Previews */
	if (camera && a.file_operations & GP_FILE_OPERATION_PREVIEW)
		gtk_widget_set_sensitive (GTK_WIDGET (m->priv->toggle_preview),
					  TRUE);
	else {
		gtk_widget_set_sensitive (GTK_WIDGET (m->priv->toggle_preview),
					  FALSE);
		gtk_toggle_button_set_active (m->priv->toggle_preview, FALSE);
	}

	/* Capture */
	if (camera && a.operations & (GP_OPERATION_CAPTURE_PREVIEW |
				      GP_OPERATION_CAPTURE_IMAGE))
		gtk_widget_set_sensitive (m->priv->item_capture, TRUE);
	else
		gtk_widget_set_sensitive (m->priv->item_capture, FALSE);

	/* Delete */
	if (camera && a.file_operations & GP_FILE_OPERATION_DELETE)
		gtk_widget_set_sensitive (m->priv->item_delete, TRUE);
	else
		gtk_widget_set_sensitive (m->priv->item_delete, FALSE);

	/* Delete all */
	if (camera && a.folder_operations & GP_FOLDER_OPERATION_DELETE_ALL)
		gtk_widget_set_sensitive (m->priv->item_delete_all, TRUE);
	else
		gtk_widget_set_sensitive (m->priv->item_delete_all, FALSE);

	/* Overall deletion */
	if (camera && ((a.file_operations & GP_FILE_OPERATION_DELETE) ||
		       (a.folder_operations & GP_FOLDER_OPERATION_DELETE_ALL)))
		gtk_widget_set_sensitive (m->priv->menu_delete, TRUE);
	else
		gtk_widget_set_sensitive (m->priv->menu_delete, FALSE);

	/* Configuration */
	if (camera && a.operations & GP_OPERATION_CONFIG)
		gtk_widget_set_sensitive (m->priv->item_config, TRUE);
	else
		gtk_widget_set_sensitive (m->priv->item_config, FALSE);

	gtkam_main_update_sensitivity (m);

	/* The remaining */
	if (camera) {
		gtk_widget_set_sensitive (m->priv->item_save, TRUE);
		gtk_widget_set_sensitive (m->priv->item_information, TRUE);
		gtk_widget_set_sensitive (m->priv->item_manual, TRUE);
		gtk_widget_set_sensitive (m->priv->item_about_driver, TRUE);
	} else {
		gtk_widget_set_sensitive (m->priv->item_save, FALSE);
		gtk_widget_set_sensitive (m->priv->item_information, FALSE);
		gtk_widget_set_sensitive (m->priv->item_manual, FALSE);
		gtk_widget_set_sensitive (m->priv->item_about_driver, FALSE);
	}

	gtkam_list_set_camera (m->priv->list, camera, multi);
	gtkam_tree_set_camera (m->priv->tree, camera, multi);
}
