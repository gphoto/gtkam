/* gtkam-tree-item.c
 *
 * Copyright (C) 2002 Lutz Müller <lutz@users.sourceforge.net>
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
#include "gtkam-tree-item.h"

#include <string.h>

#include <gtk/gtkbutton.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtktree.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkfilesel.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gtkam-status.h"
#include "gtkam-error.h"
#include "gtkam-tree-item-dir.h"
#include "gtkam-mkdir.h"

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

struct _GtkamTreeItemPrivate
{
	Camera *camera;
	gboolean multi;

	gboolean expanded;
	gboolean online;

	gchar *folder;
	CameraList *files, *dirs;

	GtkWidget *status;
};

#define PARENT_TYPE GTK_TYPE_TREE_ITEM
static GtkTreeItemClass *parent_class;

enum {
	FILE_UPLOADED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void
gtkam_tree_item_destroy (GtkObject *object)
{
	GtkamTreeItem *item = GTKAM_TREE_ITEM (object);

	if (item->priv->files) {
		gp_list_unref (item->priv->files);
		item->priv->files = NULL;
	}

	if (item->priv->dirs) {
		gp_list_unref (item->priv->dirs);
		item->priv->dirs = NULL;
	}

	if (item->priv->folder) {
		g_free (item->priv->folder);
		item->priv->folder = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_tree_item_finalize (GtkObject *object)
{
	GtkamTreeItem *item = GTKAM_TREE_ITEM (object);

	g_free (item->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_tree_item_expand (GtkTreeItem *tree_item)
{
	GtkamTreeItem *item = GTKAM_TREE_ITEM (tree_item);
	guint i;
	const char *name;
	GtkWidget *new_item;
	gchar *folder;

	/* If we haven't expanded the subtree before, populate it. */
	if (!item->priv->expanded) {
		for (i = 0; i < gp_list_count (item->priv->dirs); i++) {
			gp_list_get_name (item->priv->dirs, i, &name);
			new_item = gtkam_tree_item_dir_new (item->priv->status);
			gtk_widget_show (new_item);
			gtk_tree_append (GTK_TREE (tree_item->subtree),
					 new_item);

			/* Configure the new item */
			if (!strcmp (item->priv->folder, "/"))
				folder = g_strdup_printf ("/%s", name);
			else
				folder = g_strdup_printf ("%s/%s",
						item->priv->folder, name);
			gtkam_tree_item_set_folder (GTKAM_TREE_ITEM (new_item),
						    folder);
			g_free (folder);
			gtkam_tree_item_set_camera (GTKAM_TREE_ITEM (new_item),
						    item->priv->camera,
						    item->priv->multi);
		}
		item->priv->expanded = TRUE;
	}

	parent_class->expand (tree_item);
}

static void
set_camera (GtkamTreeItem *item, Camera *camera, gboolean multi)
{
	if (item->priv->camera) {
		gp_camera_unref (item->priv->camera);
		item->priv->camera = NULL;
	}
	if (camera) {
		item->priv->camera = camera;
		gp_camera_ref (camera);
		item->priv->multi = multi;
	}

	/*
	 * Try to get a file listing. If that fails, put the camera
	 * offline.
	 */
	if (gp_camera_folder_list_files (camera, item->priv->folder,
					 item->priv->dirs, NULL) < 0)
		gtkam_tree_item_set_offline (item);
	else
		gtkam_tree_item_set_online (item);

	if (item->priv->multi)
		gp_camera_exit (item->priv->camera, NULL);
}

static void
gtkam_tree_item_class_init (GtkamTreeItemClass *klass)
{
	GtkObjectClass *object_class;
	GtkTreeItemClass *tree_item_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_tree_item_destroy;
	object_class->finalize = gtkam_tree_item_finalize;

	tree_item_class = GTK_TREE_ITEM_CLASS (klass);
	tree_item_class->expand = gtkam_tree_item_expand;

	klass->set_camera = set_camera;

	signals[FILE_UPLOADED] = gtk_signal_new ("file_uploaded",
		GTK_RUN_FIRST, object_class->type,
		GTK_SIGNAL_OFFSET (GtkamTreeItemClass, file_uploaded),
		gtk_marshal_NONE__POINTER, GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_tree_item_init (GtkamTreeItem *item)
{
	item->priv = g_new0 (GtkamTreeItemPrivate, 1);

	item->priv->folder = g_strdup ("/");
	gp_list_new (&item->priv->files);
	gp_list_new (&item->priv->dirs);
}

GtkType
gtkam_tree_item_get_type (void)
{
	static GtkType item_type = 0;

	if (!item_type) {
		static const GtkTypeInfo item_info = {
			"GtkamTreeItem",
			sizeof (GtkamTreeItem),
			sizeof (GtkamTreeItemClass),
			(GtkClassInitFunc)  gtkam_tree_item_class_init,
			(GtkObjectInitFunc) gtkam_tree_item_init,
			NULL, NULL, NULL};
		item_type = gtk_type_unique (PARENT_TYPE, &item_info);
	}

	return (item_type);
}

static void
on_dir_created (GtkamMkdir *mkdir, const gchar *path, GtkamTreeItem *item)
{
	GtkWidget *new_item;

	g_return_if_fail (GTKAM_IS_MKDIR (mkdir));
	g_return_if_fail (path != NULL);
	g_return_if_fail (GTKAM_IS_TREE_ITEM (item));

	if (item->priv->expanded) {
		new_item = gtkam_tree_item_dir_new (item->priv->status);
		gtk_widget_show (new_item);
		gtk_tree_append (GTK_TREE (GTK_TREE_ITEM (item)->subtree),
				 new_item);

		gtkam_tree_item_set_folder (GTKAM_TREE_ITEM (new_item), path);
		gtkam_tree_item_set_camera (GTKAM_TREE_ITEM (new_item),
					    item->priv->camera,
					    item->priv->multi);
	}
}

static void
on_upload_ok_clicked (GtkButton *button, gboolean *ok)
{
        *ok = TRUE;
        gtk_main_quit ();
}

typedef struct {
	GtkWidget *menu;
	GtkamTreeItem *item;
} PopupData;

static void
on_upload_activate (GtkMenuItem *menu_item, PopupData *data)
{
        CameraFile *file;
        GtkWidget *fsel, *dialog, *s;
        gboolean ok = FALSE;
        const char *path;
        int r;
	GtkamTreeItem *item = data->item;
	GtkTree *tree;

	fsel = gtk_file_selection_new (_("Upload..."));
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
                        dialog = gtkam_error_new (r, NULL, NULL,
                                _("Could not open '%s'."), path);
                        gtk_widget_show (dialog);
                } else {
                        gtk_widget_hide (fsel);
                        s = gtkam_status_new (_("Uploading '%s' into "
                                "folder '%s'..."), g_basename (path), 
				item->priv->folder);
                        gtk_widget_show (s);
                        gtk_box_pack_start (GTK_BOX (item->priv->status), s,
                                            FALSE, FALSE, 0);
                        r = gp_camera_folder_put_file (item->priv->camera,
                                item->priv->folder, file,
                                GTKAM_STATUS (s)->context->context);
                        if (item->priv->multi)
                                gp_camera_exit (item->priv->camera, NULL);
                        switch (r) {
                        case GP_OK:
				gtk_signal_emit (GTK_OBJECT (item),
					signals[FILE_UPLOADED],
					g_basename (path));
				tree = GTK_TREE (
					GTK_WIDGET (item)->parent)->root_tree;
				gtk_signal_emit_by_name (GTK_OBJECT (tree),
					"file_uploaded", item->priv->camera,
					item->priv->multi, item->priv->folder,
					g_basename (path));
                                break;
                        case GP_ERROR_CANCEL:
                                break;
                        default:
                                dialog = gtkam_error_new (r,
                                        GTKAM_STATUS (s)->context, NULL,
                                        _("Coult not upload '%s' into folder "
                                        "'%s'."), path, item->priv->folder);
                                gtk_widget_show (dialog);
                        }
                        gtk_object_destroy (GTK_OBJECT (s));
                }
                gp_file_unref (file);
        }

	gtk_object_destroy (GTK_OBJECT (fsel));
}

static void
on_mkdir_activate (GtkMenuItem *menu_item, PopupData *data)
{
	GtkWidget *dialog;
	GtkamTreeItem *item = data->item;

	dialog = gtkam_mkdir_new (item->priv->camera, item->priv->multi,
				  item->priv->folder, NULL);
	gtk_widget_show (dialog);
	gtk_signal_connect (GTK_OBJECT (dialog), "dir_created",
			    GTK_SIGNAL_FUNC (on_dir_created), item);
}

static void
on_rmdir_activate (GtkMenuItem *menu_item, PopupData *data)
{
	GtkWidget *s, *dialog;
	gchar *path;
	int result;
	GtkamTreeItem *item = data->item;

	s = gtkam_status_new (_("Removing directory '%s'..."),
			      item->priv->folder);
	gtk_widget_show (s);
	gtk_box_pack_start (GTK_BOX (item->priv->status), s, FALSE, FALSE, 0);

	path = g_strdup (item->priv->folder);
	while (path[strlen (path) - 1] != '/')
		path[strlen (path) - 1] = '\0';
	result = gp_camera_folder_remove_dir (item->priv->camera,
			path, g_basename (item->priv->folder),
			GTKAM_STATUS (s)->context->context);
	g_free (path);
	switch (result) {
	case GP_OK:
		gtk_object_destroy (GTK_OBJECT (item));
		break;
	case GP_ERROR_CANCEL:
		break;
	default:
		dialog = gtkam_error_new (result, GTKAM_STATUS (s)->context,
			NULL, _("Could not remove directory '%s'."),
			item->priv->folder);
		gtk_widget_show (dialog);
		break;
	}

	if (item->priv->multi)
		gp_camera_exit (item->priv->camera, NULL);

	gtk_object_destroy (GTK_OBJECT (s));
}

static gboolean
kill_idle (gpointer user_data)
{
	PopupData *data = user_data;

	gtk_object_unref (GTK_OBJECT (data->menu));
	g_free (data);

	return (FALSE);
}

static void
kill_popup_menu (GtkWidget *widget, PopupData *data)
{
	gtk_idle_add (kill_idle, data);
}

static gint
on_button_press_event (GtkWidget *widget, GdkEventButton *event,
		       GtkamTreeItem *item)
{
	PopupData *data;
	GtkWidget *i;
	CameraAbilities a;

	g_return_val_if_fail (GTKAM_IS_TREE_ITEM (item), FALSE);

	gp_camera_get_abilities (item->priv->camera, &a);

	switch (event->button) {
	case 3:
		data= g_new0 (PopupData, 1);
		data->item = item;
		data->menu = gtk_menu_new ();
		gtk_widget_show (data->menu);

		/* Upload */
		i = gtk_menu_item_new_with_label (_("Upload file..."));
		gtk_widget_show (i);
		gtk_container_add (GTK_CONTAINER (data->menu), i);
		gtk_signal_connect (GTK_OBJECT (i), "activate",
			GTK_SIGNAL_FUNC (on_upload_activate), data);
		gtk_widget_set_sensitive (i,
			a.folder_operations & GP_FOLDER_OPERATION_PUT_FILE);

		/* Separator */
		i = gtk_menu_item_new ();
		gtk_widget_show (i);
		gtk_container_add (GTK_CONTAINER (data->menu), i);

		/* mkdir */
		i= gtk_menu_item_new_with_label (_("Make directory"));
		gtk_widget_show (i);
		gtk_container_add (GTK_CONTAINER (data->menu), i);
		gtk_signal_connect (GTK_OBJECT (i), "activate",
			GTK_SIGNAL_FUNC (on_mkdir_activate), data);
		gtk_widget_set_sensitive (i,
			a.folder_operations & GP_FOLDER_OPERATION_MAKE_DIR);

		/* rmdir */
		i= gtk_menu_item_new_with_label (_("Remove directory"));
		gtk_widget_show (i);
		gtk_container_add (GTK_CONTAINER (data->menu), i);
		gtk_signal_connect (GTK_OBJECT (i), "activate",
			GTK_SIGNAL_FUNC (on_rmdir_activate), data);
		gtk_widget_set_sensitive (i,
			a.folder_operations & GP_FOLDER_OPERATION_REMOVE_DIR);

		gtk_signal_connect (GTK_OBJECT (data->menu), "hide",
				    GTK_SIGNAL_FUNC (kill_popup_menu), data);
		gtk_menu_popup (GTK_MENU (data->menu), NULL, NULL,
				NULL, NULL, event->button, event->time);

		return (TRUE);
	default:
		return (FALSE);
	}
}

void
gtkam_tree_item_construct (GtkamTreeItem *item, GtkWidget *vbox,
			   GdkPixbuf *pixbuf)
{
	GtkWidget *hbox, *image, *label;
	GdkPixbuf *scaled;
	guint w, h;
	GdkPixmap *pixmap;
	GdkBitmap *bitmap;

	g_return_if_fail (GTKAM_IS_TREE_ITEM (item));

	item->priv->status = vbox;

	hbox = gtk_hbox_new (FALSE, 3);
	gtk_widget_show (hbox);
	gtk_container_add (GTK_CONTAINER (item), hbox);

	/* Add the icon */
	w = gdk_pixbuf_get_width (pixbuf) / 2;
	h = gdk_pixbuf_get_height (pixbuf) / 2;
	scaled = gdk_pixbuf_scale_simple (pixbuf, w, h, GDK_INTERP_NEAREST);
	gdk_pixbuf_render_pixmap_and_mask (scaled, &pixmap, &bitmap, 127);
	gdk_pixbuf_unref (scaled);
	image = gtk_pixmap_new (pixmap, bitmap);
	if (pixmap)
		gdk_pixmap_unref (pixmap);
	if (bitmap)
		gdk_bitmap_unref (bitmap);
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

	/* Add the label */
	item->label = gtk_label_new (_("No camera set"));
	gtk_widget_show (item->label);
	gtk_box_pack_start (GTK_BOX (hbox), item->label, FALSE, FALSE, 0);

	/* Left bracket */
	label = gtk_label_new (" (");
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	/* Status */
	item->status = gtk_label_new (_("offline"));
	gtk_widget_show (item->status);
	gtk_box_pack_start (GTK_BOX (hbox), item->status, FALSE, FALSE, 0);

	/* Right bracket */
	label = gtk_label_new (")");
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	/* If the user clicks the right button, we'll show a popup menu. */
	gtk_signal_connect (GTK_OBJECT (item), "button_press_event",
			    GTK_SIGNAL_FUNC (on_button_press_event), item);
}

void
gtkam_tree_item_set_camera (GtkamTreeItem *item, Camera *camera,
			    gboolean multi)
{
	g_return_if_fail (GTKAM_IS_TREE_ITEM (item));

	GTKAM_TREE_ITEM_CLASS (GTK_OBJECT (item)->klass)->set_camera (item,
							camera, multi);
}

void
gtkam_tree_item_set_folder (GtkamTreeItem *item, const gchar *folder)
{
	g_return_if_fail (GTKAM_IS_TREE_ITEM (item));

	if (item->priv->folder) {
		g_free (item->priv->folder);
		item->priv->folder = NULL;
	}

	item->priv->folder = g_strdup (folder);
	gtk_label_set_text (GTK_LABEL (item->label), g_basename (folder));
}

const gchar *
gtkam_tree_item_get_folder (GtkamTreeItem *item)
{
	g_return_val_if_fail (GTKAM_IS_TREE_ITEM (item), NULL);

	return (item->priv->folder);
}

void
gtkam_tree_item_set_online (GtkamTreeItem *item)
{
	int result;
	GtkWidget *s, *dialog, *tree;
	gchar *status;

	g_return_if_fail (GTKAM_IS_TREE_ITEM (item));
	g_return_if_fail (item->priv->camera != NULL);

	item->priv->online = TRUE;

	s = gtkam_status_new (_("Listing contents of folder '%s'..."),
			      item->priv->folder);
	gtk_widget_show (s);
	gtk_box_pack_start (GTK_BOX (item->priv->status), s, FALSE, FALSE, 0);

	/* Get a list of files */
	result = gp_camera_folder_list_files (item->priv->camera,
			item->priv->folder, item->priv->files,
			GTKAM_STATUS (s)->context->context);
	switch (result) {
	case GP_OK:
		status = g_strdup_printf ("%i",
					  gp_list_count (item->priv->dirs));
		gtk_label_set_text (GTK_LABEL (item->status), status);
		g_free (status);
		break;
	case GP_ERROR_CANCEL:
		break;
	default:
		dialog = gtkam_error_new (result, GTKAM_STATUS (s)->context,
			NULL,
			_("Could not retrieve file list for folder '%s'."),
			item->priv->folder);
		gtk_widget_show (dialog);
	}

	/* Get a list of folders */
	result = gp_camera_folder_list_folders (item->priv->camera,
		item->priv->folder, item->priv->dirs, 
		GTKAM_STATUS (s)->context->context);
	switch (result) {
	case GP_OK:
		if (gp_list_count (item->priv->dirs) > 0) {
			tree = gtk_tree_new ();
			gtk_widget_show (tree);
			gtk_tree_item_set_subtree (GTK_TREE_ITEM (item), tree);
		}
		break;
	case GP_ERROR_CANCEL:
		break;
	default:
		dialog = gtkam_error_new (result, GTKAM_STATUS (s)->context,
			NULL, 
			_("Could not get list of folders for folder '%s'."),
			item->priv->folder);
		gtk_widget_show (dialog);
	}

	if (item->priv->multi)
		gp_camera_exit (item->priv->camera, NULL);

	gtk_object_destroy (GTK_OBJECT (s));
}

void
gtkam_tree_item_set_offline (GtkamTreeItem *item)
{
	g_return_if_fail (GTKAM_IS_TREE_ITEM (item));

	item->priv->online = FALSE;
	gp_list_reset (item->priv->dirs);
	gp_list_reset (item->priv->files);
	if (GTK_TREE_ITEM (item)->subtree)
		gtk_tree_item_remove_subtree (GTK_TREE_ITEM (item));
	item->priv->expanded = FALSE;

	gtk_label_set_text (GTK_LABEL (item->status), _("offline"));
}

gboolean 
gtkam_tree_item_get_multi (GtkamTreeItem *item)
{
	g_return_val_if_fail (GTKAM_IS_TREE_ITEM (item), FALSE);

	return (item->priv->multi);
}

Camera *
gtkam_tree_item_get_camera (GtkamTreeItem *item)
{
	g_return_val_if_fail (GTKAM_IS_TREE_ITEM (item), NULL);

	return (item->priv->camera);
}

void
gtkam_tree_item_update (GtkamTreeItem *item)
{
	g_return_if_fail (GTKAM_IS_TREE_ITEM (item));

	gtkam_tree_item_set_offline (item);
	gtkam_tree_item_set_online (item);
}
