/* gtkam-tree.c
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
#include "gtkam-tree.h"
#include "i18n.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtkitemfactory.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkstock.h>
#include <gtk/gtktooltips.h>
#include <gtk/gtkmenuitem.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <gphoto2/gphoto2-setting.h>
#include <gphoto2/gphoto2-camera.h>

#include "gtkam-camera.h"
#include "gtkam-chooser.h"
#include "gtkam-close.h"
#include "gtkam-config.h"
#include "gtkam-error.h"
#include "gtkam-status.h"
#include "gtkam-mkdir.h"
#include "gtkam-preview.h"

struct _GtkamTreePrivate
{
	GtkTreeStore *store;

	GtkItemFactory *factory;
	GtkTooltips *tooltips;

	GtkTreeIter iter;
};

#define PARENT_TYPE GTK_TYPE_TREE_VIEW
static GtkTreeViewClass *parent_class;

enum {
	FOLDER_SELECTED,
	FOLDER_UNSELECTED,
	FILE_ADDED,
	NEW_STATUS,
	NEW_ERROR,
	NEW_DIALOG,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

enum {
	FOLDER_COLUMN = 0,
	CAMERA_COLUMN,
	NUM_COLUMNS
};

static void
gtkam_tree_destroy (GtkObject *object)
{
	GtkamTree *tree = GTKAM_TREE (object);

	if (tree->priv->store) {
		g_object_unref (G_OBJECT (tree->priv->store));
		tree->priv->store = NULL;
	}

	if (tree->priv->tooltips) {
		g_object_unref (G_OBJECT (tree->priv->tooltips));
		tree->priv->tooltips = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_tree_finalize (GObject *object)
{
	GtkamTree *tree = GTKAM_TREE (object);

	g_free (tree->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_tree_class_init (gpointer g_class, gpointer class_data)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = GTK_OBJECT_CLASS (g_class);
	object_class->destroy  = gtkam_tree_destroy;
	
	gobject_class = G_OBJECT_CLASS (g_class);
	gobject_class->finalize = gtkam_tree_finalize;

	signals[FOLDER_SELECTED] = g_signal_new ("folder_selected",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkamTreeClass, folder_selected), NULL, NULL,
		g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		G_TYPE_POINTER);
	signals[FOLDER_UNSELECTED] = g_signal_new ("folder_unselected",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkamTreeClass, folder_unselected), NULL, NULL,
		g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		G_TYPE_POINTER);
	signals[FILE_ADDED] = g_signal_new ("file_added",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkamTreeClass, file_added), NULL, NULL,
		g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		G_TYPE_POINTER);
	signals[NEW_STATUS] = g_signal_new ("new_status",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkamTreeClass, new_status), NULL, NULL,
		g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		G_TYPE_POINTER);
	signals[NEW_ERROR] = g_signal_new ("new_error",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkamTreeClass, new_error), NULL, NULL,
		g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		G_TYPE_POINTER);
	signals[NEW_DIALOG] = g_signal_new ("new_dialog",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkamTreeClass, new_dialog), NULL, NULL,
		g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		G_TYPE_POINTER);

	parent_class = g_type_class_peek_parent (g_class);
}

static void
gtkam_tree_init (GTypeInstance *instance, gpointer g_class)
{
	GtkamTree *tree = GTKAM_TREE (instance);

	tree->priv = g_new0 (GtkamTreePrivate, 1);

	tree->priv->tooltips = gtk_tooltips_new ();
	g_object_ref (G_OBJECT (tree->priv->tooltips));
	gtk_object_sink (GTK_OBJECT (tree->priv->tooltips));
}

GType
gtkam_tree_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo ti;

		memset (&ti, 0, sizeof (GTypeInfo));
		ti.class_size    = sizeof (GtkamTreeClass);
		ti.class_init    = gtkam_tree_class_init;
		ti.instance_size = sizeof (GtkamTree);
		ti.instance_init = gtkam_tree_init;

		type = g_type_register_static (PARENT_TYPE, "GtkamTree",
					       &ti, 0);
	}

	return (type);
}

static GtkamCamera *
gtkam_tree_get_camera_from_iter (GtkamTree *tree, GtkTreeIter *iter)
{
	GValue value = {0};
	GtkamCamera *camera;

	gtk_tree_model_get_value (GTK_TREE_MODEL (tree->priv->store), iter,
				  CAMERA_COLUMN, &value);
	g_assert (G_VALUE_HOLDS (&value, GTKAM_TYPE_CAMERA));
	camera = GTKAM_CAMERA (g_value_peek_pointer (&value));
	g_value_unset (&value);

	return (camera);
}

static gchar *
gtkam_tree_get_path_rec (GtkamTree *tree, GtkTreeIter *iter, const gchar *path)
{
	GtkTreeIter parent, p;
	gchar *new_path = NULL, *new_path_ret = NULL;
	GValue value = {0};

	if (gtk_tree_model_iter_parent (GTK_TREE_MODEL (tree->priv->store),
					&parent, iter)) {
		if (gtk_tree_model_iter_parent (
			GTK_TREE_MODEL (tree->priv->store), &p, &parent)) {
			gtk_tree_model_get_value (
				GTK_TREE_MODEL (tree->priv->store),
				&parent, FOLDER_COLUMN, &value);
			new_path = g_strdup_printf ("%s/%s",
					g_value_get_string (&value), path);
			g_value_unset (&value);
			new_path_ret = gtkam_tree_get_path_rec (tree, &parent,
								new_path);
			g_free (new_path);
			return (new_path_ret);
		} else
			return (g_strdup_printf ("/%s", path));
	} else
		return (g_strdup ("/"));
}

static gchar *
gtkam_tree_get_path_from_iter (GtkamTree *tree, GtkTreeIter *iter)
{
	GtkTreeIter parent;
	GValue value = {0};
	gchar *path = NULL;

	if (gtk_tree_model_iter_parent (GTK_TREE_MODEL (tree->priv->store),
					&parent, iter)) {
		gtk_tree_model_get_value (GTK_TREE_MODEL (tree->priv->store),
					  iter, FOLDER_COLUMN, &value);
		path = gtkam_tree_get_path_rec (tree, iter,
						g_value_get_string (&value));
		g_value_unset (&value);
	} else
		path = g_strdup ("/");

	return (path);
}

static void
gtkam_tree_update_iter (GtkamTree *tree, GtkTreeIter *iter)
{
	gint i, n;
	GtkTreeIter child;
	CameraList *list;
	const char *name;
	int result;
	GtkamCamera *camera = NULL;
	GtkWidget *s;
	gchar *folder = NULL, *msg;
	GtkamTreeErrorData e;

	g_return_if_fail (GTKAM_IS_TREE (tree));

	/* Look up the corresponding camera */
	camera = gtkam_tree_get_camera_from_iter (tree, iter);
	g_return_if_fail (camera != NULL);
	g_return_if_fail (GTKAM_IS_CAMERA (camera));

	while (gtk_tree_model_iter_n_children (
				GTK_TREE_MODEL (tree->priv->store), iter)) {
		gtk_tree_model_iter_nth_child (
				GTK_TREE_MODEL (tree->priv->store), &child,
				iter, 0);
		gtk_tree_store_remove (tree->priv->store, &child);
	}

	/* Which folder? */
	folder = gtkam_tree_get_path_from_iter (tree, iter);

	s = gtkam_status_new (_("Listing folders in '%s'..."), folder);
	g_signal_emit (G_OBJECT (tree), signals[NEW_STATUS], 0, s);
	gp_list_new (&list);
	result = gp_camera_folder_list_folders (camera->camera, folder, list,
					GTKAM_STATUS (s)->context->context);
	if (result >= 0) {
		n = gp_list_count (list);
		for (i = 0; i < n; i++) {
			gp_list_get_name (list, i, &name);
			gtk_tree_store_append (tree->priv->store, &child,
					       iter);
			gtk_tree_store_set (tree->priv->store, &child,
					FOLDER_COLUMN, name,
					CAMERA_COLUMN, camera, -1);
		}
	} else if (result == GP_ERROR_NOT_SUPPORTED) {
		/* Do nothing */
	} else {
		e.context = GTKAM_STATUS (s)->context;
		e.result = result;
		e.msg = msg = g_strdup_printf (
				_("Could not list folders in '%s'."), folder);
		g_signal_emit (G_OBJECT (tree), signals[NEW_ERROR], 0, &e);
		g_free (msg);
	}
	g_free (folder);
	gp_list_unref (list);
	if (camera->multi)
		gp_camera_exit (camera->camera, NULL);

	gtk_object_destroy (GTK_OBJECT (s));
}

static void
on_row_expanded (GtkTreeView *tree_view, GtkTreeIter *iter,
                 GtkTreePath *path, GtkamTree *tree)
{
        gint i, n;
        GtkTreeIter child;

        n = gtk_tree_model_iter_n_children (
			GTK_TREE_MODEL (tree->priv->store), iter);
        for (i = 0; i < n; i++) {
                gtk_tree_model_iter_nth_child (
                        GTK_TREE_MODEL (tree->priv->store), &child, iter, i);
		if (!gtk_tree_model_iter_has_child (
				GTK_TREE_MODEL (tree->priv->store), &child))
			gtkam_tree_update_iter (tree, &child); 
        }
}

static gboolean
selection_func (GtkTreeSelection *selection, GtkTreeModel *model,
		GtkTreePath *path, gboolean path_currently_selected,
		gpointer data)
{
	GtkTreeIter iter;
	gchar *folder;
	GtkamTree *tree = GTKAM_TREE (data);
	GtkamTreeFolderSelectedData sd;
	GtkamTreeFolderUnselectedData ud;

	gtk_tree_model_get_iter (model, &iter, path);
	folder = gtkam_tree_get_path_from_iter (tree, &iter);
	if (path_currently_selected) {
		ud.camera = gtkam_tree_get_camera_from_iter (tree, &iter);
		ud.folder = folder;
		g_signal_emit (G_OBJECT (tree), signals[FOLDER_UNSELECTED],
			       0, &ud);
	} else {
		sd.camera = gtkam_tree_get_camera_from_iter (tree, &iter);
		sd.folder = folder;
		g_signal_emit (G_OBJECT (tree), signals[FOLDER_SELECTED],
			       0, &sd);
	}
	g_free (folder);

	return (TRUE);
}

typedef struct _GtkamTreeUploadData GtkamTreeUploadData;
struct _GtkamTreeUploadData {
	GtkWidget *fsel;
	GtkamTree *tree;
	GtkTreeIter *iter;
};

static void
on_upload_destroy (GtkObject *object, GtkamTreeUploadData *ud)
{
	gtk_tree_iter_free (ud->iter);
	g_free (ud);
}

static void
on_upload_cancel_clicked (GtkButton *button, GtkFileSelection *fsel)
{
	gtk_object_destroy (GTK_OBJECT (fsel));
}

static void
on_upload_ok_clicked (GtkButton *button, GtkamTreeUploadData *ud)
{
	const gchar *path;
	int r;
	CameraFile *file;
	GtkamTreeErrorData e;
	GtkamTreeFileAddedData fud;
	GtkWidget *s;
	gchar *folder, *msg;
	GtkamCamera *c;

	path = gtk_file_selection_get_filename (GTK_FILE_SELECTION (ud->fsel));
	gp_file_new (&file);
	r = gp_file_open (file, path);
	if (r < 0) {
		e.context = NULL;
		e.result = r;
		e.msg = msg = g_strdup_printf (_("Could not open '%s'."), path);
		g_signal_emit (G_OBJECT (ud->tree), signals[NEW_ERROR], 0, &e);
		g_free (msg);
	} else {
		gtk_widget_hide (ud->fsel);
		folder = gtkam_tree_get_path_from_iter (ud->tree, ud->iter);
		c = gtkam_tree_get_camera_from_iter (ud->tree, ud->iter);
		s = gtkam_status_new (_("Uploading '%s' into "
			"folder '%s'..."), g_basename (path), folder);
		g_signal_emit (G_OBJECT (ud->tree), signals[NEW_STATUS], 0, s);
		r = gp_camera_folder_put_file (c->camera, folder, file,
				GTKAM_STATUS (s)->context->context);
		if (c->multi)
			gp_camera_exit (c->camera, NULL);
		switch (r) {
		case GP_OK:
			fud.camera = c;
			fud.folder = folder;
			fud.name = g_basename (path);
			g_signal_emit (G_OBJECT (ud->tree),
				       signals[FILE_ADDED], 0, &fud);
			break;
		case GP_ERROR_CANCEL:
			break;
		default:
			e.context = GTKAM_STATUS (s)->context;
			e.result = r;
			e.msg = msg = g_strdup_printf (_("Could not upload "
				"'%s' into '%s'."), path, folder);
			g_signal_emit (G_OBJECT (ud->tree), signals[NEW_ERROR],
				       0, &e);
			g_free (msg);
			break;
		}
		g_free (folder);
		gtk_object_destroy (GTK_OBJECT (s));
	}
	gp_file_unref (file);
	gtk_object_destroy (GTK_OBJECT (ud->fsel));
}

static void
action_upload (gpointer callback_data, guint callback_action,
	       GtkWidget *widget)
{
	GtkWidget *fsel;
	GtkamTree *tree = GTKAM_TREE (callback_data);
	GtkamTreeUploadData *ud;
	gchar *title, *folder;

	folder = gtkam_tree_get_path_from_iter (tree, &tree->priv->iter);
	title = g_strdup_printf (_("Upload into '%s'..."), folder);
	g_free (folder);
	fsel = gtk_file_selection_new (title);
	g_free (title);
	g_signal_emit (G_OBJECT (tree), signals[NEW_DIALOG], 0, fsel);
	g_object_unref (G_OBJECT (fsel));
	ud = g_new0 (GtkamTreeUploadData, 1);
	ud->fsel = fsel;
	ud->tree = tree;
	ud->iter = gtk_tree_iter_copy (&tree->priv->iter);
	g_signal_connect (G_OBJECT (fsel), "destroy",
		G_CALLBACK (on_upload_destroy), ud);
	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (fsel)->ok_button),
		"clicked", G_CALLBACK (on_upload_ok_clicked), ud);
	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (fsel)->cancel_button),
		"clicked", G_CALLBACK (on_upload_cancel_clicked), fsel);
}

typedef struct _DirCreatedData DirCreatedData;
struct _DirCreatedData {
	GtkamTree *tree;
	GtkTreeIter *iter;
};

static void
on_dir_created (GtkamMkdir *mkdir, GtkamMkdirDirCreatedData *data,
		DirCreatedData *d)
{
	gtkam_tree_update_iter (d->tree, d->iter);
}

static void
on_mkdir_destroy (GtkObject *mkdir, DirCreatedData *d)
{
	gtk_tree_iter_free (d->iter);
	g_free (d);
}

static void
action_mkdir (gpointer callback_data, guint callback_action,
	      GtkWidget *widget)
{
	GtkWidget *d;
	GtkamTree *tree = GTKAM_TREE (callback_data);
	GtkamCamera *camera;
	DirCreatedData *data;
	gchar *folder;

	camera = gtkam_tree_get_camera_from_iter (tree, &tree->priv->iter);
	folder = gtkam_tree_get_path_from_iter (tree, &tree->priv->iter);
	d = gtkam_mkdir_new (camera, folder);
	g_signal_emit (GTK_OBJECT (tree), signals[NEW_DIALOG], 0, d);
	g_object_unref (G_OBJECT (d));
	g_free (folder);
	data = g_new0 (DirCreatedData, 1);
	data->tree = tree;
	data->iter = gtk_tree_iter_copy (&tree->priv->iter);
	g_signal_connect (G_OBJECT (d), "dir_created",
			  G_CALLBACK (on_dir_created), data);
	g_signal_connect (G_OBJECT (d), "destroy",
			  G_CALLBACK (on_mkdir_destroy), data);
}

static void
action_rmdir (gpointer callback_data, guint callback_action,
	      GtkWidget *widget)
{
	GtkWidget *s;
	GtkamTree *tree = GTKAM_TREE (callback_data);
	gchar *folder, *path, *msg;
	int r;
	GtkamCamera *camera;
	GtkamTreeErrorData e;

	folder = gtkam_tree_get_path_from_iter (tree, &tree->priv->iter);
	camera = gtkam_tree_get_camera_from_iter (tree, &tree->priv->iter);
	s = gtkam_status_new (_("Removing directory '%s'..."), folder);
	g_signal_emit (G_OBJECT (tree), signals[NEW_STATUS], 0, s);
	path = g_strdup (folder);
	while (path[strlen (path) - 1] != '/')
		path[strlen (path) - 1] = '\0';
	r = gp_camera_folder_remove_dir (camera->camera, path,
		g_basename (folder), GTKAM_STATUS (s)->context->context);
	g_free (path);
	switch (r) {
	case GP_OK:
		gtk_tree_store_remove (tree->priv->store, &tree->priv->iter);
		break;
	case GP_ERROR_CANCEL:
		break;
	default:
		e.context = GTKAM_STATUS (s)->context;
		e.result = r;
		e.msg = msg = g_strdup_printf (_("Could not remove directory "
				"'%s'."), folder);
		g_signal_emit (G_OBJECT (tree), signals[NEW_ERROR], 0, &e);
		g_free (msg);
		break;
	}
	g_free (folder);
	gtk_object_destroy (GTK_OBJECT (s));
}

typedef enum _CameraTextType CameraTextType;
enum _CameraTextType {
        CAMERA_TEXT_SUMMARY,
        CAMERA_TEXT_MANUAL,
        CAMERA_TEXT_ABOUT
};

static void
action_text (GtkamTree *tree, CameraTextType type)
{
	GtkamCamera *camera;
	GtkWidget *s, *d;
	int r;
	CameraText text;

	camera = gtkam_tree_get_camera_from_iter (tree, &tree->priv->iter);
	switch (type) {
	case CAMERA_TEXT_SUMMARY:
		s = gtkam_status_new (
			_("Getting information about the camera..."));
		break;
	case CAMERA_TEXT_ABOUT:
		s = gtkam_status_new (
			_("Getting information about the driver..."));
		break;
	case CAMERA_TEXT_MANUAL:
	default:
		s = gtkam_status_new (_("Getting manual..."));
		break;
	}
	g_signal_emit (G_OBJECT (tree), signals[NEW_STATUS], 0, s);
	switch (type) {
	case CAMERA_TEXT_SUMMARY:
		r = gp_camera_get_summary (camera->camera, &text,
			GTKAM_STATUS (s)->context->context);
		break;
	case CAMERA_TEXT_ABOUT:
		r = gp_camera_get_about (camera->camera, &text,
			GTKAM_STATUS (s)->context->context);
		break;
	case CAMERA_TEXT_MANUAL:
	default:
		r = gp_camera_get_manual (camera->camera, &text,
			GTKAM_STATUS (s)->context->context);
		break;
	}
	if (camera->multi)
		gp_camera_exit (camera->camera, NULL);
	switch (r) {
	case GP_OK:
		d = gtkam_close_new (text.text);
		g_signal_emit (G_OBJECT (tree), signals[NEW_DIALOG], 0, d);
		g_object_unref (G_OBJECT (d));
		break;
	case GP_ERROR_CANCEL:
		break;
	default:
		d = gtkam_error_new (r, GTKAM_STATUS (s)->context, NULL,
			_("Could not retrieve information."));
		g_signal_emit (G_OBJECT (tree), signals[NEW_DIALOG], 0, d);
		g_object_unref (G_OBJECT (d));
	}
	gtk_object_destroy (GTK_OBJECT (s));
}

static void
action_summary (gpointer callback_data, guint callback_action,
		GtkWidget *widget)
{
	GtkamTree *tree = GTKAM_TREE (callback_data);

	action_text (tree, CAMERA_TEXT_SUMMARY);
}

static void
action_preferences (gpointer callback_data, guint callback_action,
		    GtkWidget *widget)
{
	GtkWidget *d;
	GtkamTree *tree = GTKAM_TREE (callback_data);
	GtkamCamera *camera;

	camera = gtkam_tree_get_camera_from_iter (tree, &tree->priv->iter);
	d = gtkam_config_new (camera);
	if (!d)
		return;
	g_signal_emit (G_OBJECT (tree), signals[NEW_DIALOG], 0, d);
	g_object_unref (G_OBJECT (d));
}

static void
action_about (gpointer callback_data, guint callback_action,
	      GtkWidget *widget)
{
	GtkamTree *tree = GTKAM_TREE (callback_data);

	action_text (tree, CAMERA_TEXT_ABOUT);
}

typedef struct _CameraSelectedData CameraSelectedData;
struct _CameraSelectedData {
	GtkamTree *tree;
	GtkTreeIter *iter;
};

static void
on_camera_selected (GtkamChooser *chooser, GtkamCamera *camera,
		    CameraSelectedData *data)
{
	CameraAbilities a;
	GtkTreeSelection *s;
	GtkamTreeFolderUnselectedData fud;

	s = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->tree));
	if (gtk_tree_selection_iter_is_selected (s, data->iter)) {
		fud.camera = camera;
		fud.folder = "/";
		g_signal_emit (G_OBJECT (data->tree),
			       signals[FOLDER_UNSELECTED], 0, &fud);
	}

	gp_camera_get_abilities (camera->camera, &a);
	gtk_tree_store_set (data->tree->priv->store, data->iter, 
			    FOLDER_COLUMN, a.model,
			    CAMERA_COLUMN, camera, -1);
	gtkam_tree_update_iter (data->tree, data->iter);
	gtkam_tree_save (data->tree);
}

static void
camera_selected_data_destroy (gpointer data)
{
	CameraSelectedData *d = data;

	gtk_tree_iter_free (d->iter);
	g_free (d);
}

static void
action_remove_camera (gpointer callback_data, guint callback_action,
		      GtkWidget *widget)
{
	GtkamTree *tree = GTKAM_TREE (callback_data);

	gtk_tree_store_remove (tree->priv->store, &tree->priv->iter);
	gtkam_tree_save (tree);
}

static void
action_select_camera (gpointer callback_data, guint callback_action,
		      GtkWidget *widget)
{
	GtkWidget *d;
	GtkamTree *tree = GTKAM_TREE (callback_data);
	CameraSelectedData *data;

	d = gtkam_chooser_new ();
	g_signal_emit (G_OBJECT (tree), signals[NEW_DIALOG], 0, d);
	g_object_unref (G_OBJECT (d));
	gtkam_chooser_set_camera (GTKAM_CHOOSER (d),
		gtkam_tree_get_camera_from_iter (tree, &tree->priv->iter));
	gtk_window_set_title (GTK_WINDOW (d), _("Select Camera"));
	data = g_new0 (CameraSelectedData, 1);
	data->tree = tree;
	data->iter = gtk_tree_iter_copy (&tree->priv->iter);
	g_signal_connect (G_OBJECT (d), "camera_selected",
			  G_CALLBACK (on_camera_selected), data);
	g_object_set_data_full (G_OBJECT (d), "data", data,
				camera_selected_data_destroy);
}

static void
on_captured (GtkamPreview *preview, GtkamPreviewCapturedData *data,
	     GtkamTree *tree)
{
	GtkamTreeFileAddedData fad;

	fad.folder = data->folder;
	fad.name = data->name;
	fad.camera = data->camera;
	g_signal_emit (G_OBJECT (tree), signals[FILE_ADDED], 0, &fad);
}

static void
action_capture (gpointer callback_data, guint callback_action,
		GtkWidget *widget)
{
	GtkamTree *tree = GTKAM_TREE (callback_data);
	GtkamCamera *camera;
	CameraAbilities a;
	GtkWidget *s, *d;
	int r;
	GtkamTreeErrorData e;
	GtkamTreeFileAddedData fad;
	gchar *msg;
	CameraFilePath path;

	camera = gtkam_tree_get_camera_from_iter (tree, &tree->priv->iter);

	/* Let's first check if the camera supports previews */
	gp_camera_get_abilities (camera->camera, &a);
	if (a.operations & GP_OPERATION_CAPTURE_PREVIEW) {
		d = gtkam_preview_new (camera);
		g_signal_emit (G_OBJECT (tree), signals[NEW_DIALOG], 0, d);
		g_object_unref (G_OBJECT (d));
		g_signal_connect (G_OBJECT (d), "captured",
				  G_CALLBACK (on_captured), tree);
		return;
	}

	/* The camera doesn't support previews. Capture an image. */
	s = gtkam_status_new (_("Capturing image..."));
	g_signal_emit (G_OBJECT (tree), signals[NEW_STATUS], 0, s);
	r = gp_camera_capture (camera->camera, GP_CAPTURE_IMAGE, &path,
			       GTKAM_STATUS (s)->context->context);
	if (camera->multi)
		gp_camera_exit (camera->camera, NULL);
	switch (r) {
	case GP_OK:
		fad.camera = camera;
		fad.folder = path.folder;
		fad.name = path.name;
		g_signal_emit (G_OBJECT (tree), signals[FILE_ADDED], 0, &fad);
		break;
	case GP_ERROR_CANCEL:
		break;
	default:
		e.context = GTKAM_STATUS (s)->context;
		e.result = r;
		e.msg = msg = g_strdup (_("Could not capture."));
		g_signal_emit (G_OBJECT (tree), signals[NEW_ERROR], 0, &e);
		g_free (msg);
		break;
	}
	gtk_object_destroy (GTK_OBJECT (s));
}

static void
action_manual (gpointer callback_data, guint callback_action,
	       GtkWidget *widget)
{
	GtkamTree *tree = GTKAM_TREE (callback_data);

	action_text (tree, CAMERA_TEXT_MANUAL);
}

static void
on_new_camera_selected (GtkamChooser *chooser, GtkamCamera *camera,
			GtkamTree *tree)
{
	gtkam_tree_add_camera (tree, camera);
}

static void
on_add_camera_activate (GtkMenuItem *item, GtkamTree *tree)
{
	GtkWidget *d, *w;

	w = gtk_widget_get_ancestor (GTK_WIDGET (tree), GTK_TYPE_WINDOW);
	d = gtkam_chooser_new ();
	gtk_window_set_transient_for (GTK_WINDOW (d), GTK_WINDOW (w));
	gtk_widget_show (d);
	g_signal_connect (G_OBJECT (d), "camera_selected",
			  G_CALLBACK (on_new_camera_selected), tree);
}

static GtkItemFactoryEntry mi[] =
{
	{N_("/Upload file..."), NULL, action_upload, 0, NULL},
	{N_("/Make directory..."), NULL, action_mkdir, 0, NULL},
	{N_("/Delete directory"), NULL, action_rmdir, 0, NULL},
	{"/sep1", NULL, NULL, 0, "<Separator>"},
	{N_("/Capture image..."), NULL, action_capture, 0, NULL},
	{N_("/View camera preferences"), NULL, action_preferences, 0,
	 "<StockItem>", GTK_STOCK_PREFERENCES},
	{N_("/View camera summary"), NULL, action_summary, 0, NULL},
	{N_("/View camera manual"), NULL, action_manual, 0, NULL},
	{N_("/View driver details"), NULL, action_about, 0, NULL},
	{"/sep2", NULL, NULL, 0, "<Separator>"},
	{N_("/Select camera..."), NULL, action_select_camera, 0, NULL},
	{N_("/Remove camera"), NULL, action_remove_camera, 0, NULL},
};

#ifdef ENABLE_NLS

static gchar *
translate_func (const gchar *path, gpointer data)
{
    return (_(path));
}

#endif

static gint
on_button_press_event (GtkWidget *widget, GdkEventButton *event,
		       GtkamTree *tree)
{
	GtkTreePath *path = NULL;
	GtkamCamera *camera;
	GtkWidget *w, *m, *i;
	CameraAbilities a;

	switch (event->button) {
	case 3:

		/* Right-click on a tree item? */
		if (!gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (tree),
			event->x, event->y, &path, NULL, NULL, NULL)) {
			m = gtk_menu_new ();
			i = gtk_menu_item_new_with_mnemonic (
							_("_Add camera..."));
			gtk_widget_show (i);
			gtk_container_add (GTK_CONTAINER (m), i);
			g_signal_connect (G_OBJECT (i), "activate",
				G_CALLBACK (on_add_camera_activate), tree);
			gtk_menu_popup (GTK_MENU (m), NULL, NULL, NULL, NULL,
					event->button, event->time);
			return (TRUE);
		}

		gtk_tree_model_get_iter (GTK_TREE_MODEL (tree->priv->store),
					 &tree->priv->iter, path);
		camera = gtkam_tree_get_camera_from_iter (tree,
							  &tree->priv->iter);
		gp_camera_get_abilities (camera->camera, &a);
		gtk_tree_path_free (path);

		/* What operations does the camera support? */
		w = gtk_item_factory_get_widget (tree->priv->factory,
						 "/Make directory...");
		gtk_widget_set_sensitive (w,
			a.folder_operations & GP_FOLDER_OPERATION_MAKE_DIR);
		w = gtk_item_factory_get_widget (tree->priv->factory,
						 "/Delete directory");
		gtk_widget_set_sensitive (w,
			a.folder_operations & GP_FOLDER_OPERATION_REMOVE_DIR);
		w = gtk_item_factory_get_widget (tree->priv->factory,
						 "/Upload file...");
		gtk_widget_set_sensitive (w,
			a.folder_operations & GP_FOLDER_OPERATION_PUT_FILE);
		w = gtk_item_factory_get_widget (tree->priv->factory,
						 "/View camera preferences");
		gtk_widget_set_sensitive (w,
			a.operations & GP_OPERATION_CONFIG);
		w = gtk_item_factory_get_widget (tree->priv->factory,
						 "/Capture image...");
		gtk_widget_set_sensitive (w,
			a.operations & GP_OPERATION_CAPTURE_IMAGE);

		gtk_item_factory_popup (tree->priv->factory, event->x_root,
				event->y_root, event->button, event->time);

		return (TRUE);
	default:
		return (FALSE);
	}
}

static void
on_row_deleted (GtkTreeModel *model, GtkTreePath *path,
		GtkamTree *tree)
{
	GtkTreeIter iter;
	gchar *folder;
	GtkamTreeFolderUnselectedData fdd;

	if (!gtk_tree_model_get_iter (model, &iter, path))
		return;

	fdd.camera = gtkam_tree_get_camera_from_iter (tree, &iter);
	fdd.folder = folder = gtkam_tree_get_path_from_iter (tree, &iter);
	g_signal_emit (G_OBJECT (tree), signals[FOLDER_UNSELECTED], 0, &fdd);
	g_free (folder);
}

static void
on_drag_data_received (GtkWidget *widget, GdkDragContext *context,
		       gint x, gint y, GtkSelectionData *selection_data,
		       guint info, guint time, GtkamTree *tree)
{
	g_warning ("Fixme: on_drag_data_received");
}

#if 0
static GtkTargetEntry targets[] = {
	{"text/uri-list", 0, 0}
};
#endif

GtkWidget *
gtkam_tree_new (void)
{
        GtkamTree *tree;
        GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	GtkAccelGroup *ag;

        tree = g_object_new (GTKAM_TYPE_TREE, NULL);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);
#if 0
	gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (tree),
		targets, G_N_ELEMENTS (targets),
		GDK_ACTION_COPY);
#endif
	g_signal_connect (G_OBJECT (tree), "drag_data_received",
			  G_CALLBACK (on_drag_data_received), tree);
	g_signal_connect (G_OBJECT (tree), "button_press_event",
			  G_CALLBACK (on_button_press_event), tree);

	gtk_tooltips_set_tip (tree->priv->tooltips, GTK_WIDGET (tree),
		(_("Please right-click to access additional menus")), NULL);

	ag = gtk_accel_group_new ();
	tree->priv->factory = gtk_item_factory_new (GTK_TYPE_MENU,
						    "<popup>", ag);
#ifdef ENABLE_NLS
	gtk_item_factory_set_translate_func (GTK_ITEM_FACTORY (tree->priv->factory),
					translate_func, NULL, NULL);
#endif
	gtk_item_factory_create_items (tree->priv->factory, 
				       G_N_ELEMENTS (mi), mi, tree);
	g_object_ref (G_OBJECT (tree->priv->factory));
	gtk_object_sink (GTK_OBJECT (tree->priv->factory));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	gtk_tree_selection_set_select_function (selection, selection_func,
						tree, NULL);

        tree->priv->store = gtk_tree_store_new (NUM_COLUMNS, G_TYPE_STRING,
						GTKAM_TYPE_CAMERA);
	g_signal_connect (G_OBJECT (tree->priv->store), "row_deleted",
			  G_CALLBACK (on_row_deleted), tree);
        gtk_tree_view_set_model (GTK_TREE_VIEW (tree),
                                 GTK_TREE_MODEL (tree->priv->store));

        /* Column for folder names. */
        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tree),
                        -1, _("Folder"), renderer, "text",
                        FOLDER_COLUMN, NULL);
        g_signal_connect (G_OBJECT (tree), "row_expanded",
                          G_CALLBACK (on_row_expanded), tree);

        return (GTK_WIDGET (tree));
}

void
gtkam_tree_add_camera (GtkamTree *tree, GtkamCamera *camera)
{
        CameraAbilities a;
        GtkTreeIter iter;

        g_return_if_fail (GTKAM_IS_TREE (tree));
	g_return_if_fail (GTKAM_IS_CAMERA (camera));

        gp_camera_get_abilities (camera->camera, &a);
        gtk_tree_store_append (tree->priv->store, &iter, NULL);
        gtk_tree_store_set (tree->priv->store, &iter,
                            FOLDER_COLUMN, a.model, 
			    CAMERA_COLUMN, camera, -1);
        gtkam_tree_update_iter (tree, &iter);
        gtkam_tree_save (tree);
}

#ifdef HAVE_GP_CAMERA_SET_TIMEOUT_FUNCS

typedef struct _TimeoutData TimeoutData;
struct _TimeoutData {
	Camera *camera;
	CameraTimeoutFunc func;
};

static gboolean
timeout_func (gpointer data)
{
	TimeoutData *td = data;

	td->func (td->camera, NULL);

	/* Repeat forever */
	return (TRUE);
}

static void
timeout_destroy_notify (gpointer data)
{
	TimeoutData *td = data;

	g_free (td);
}

static unsigned int
start_timeout_func (Camera *camera, unsigned int timeout,
		    CameraTimeoutFunc func, void *data)
{
	TimeoutData *td;

	td = g_new0 (TimeoutData, 1);
	td->camera = camera;
	td->func = func;

	return (gtk_timeout_add_full (timeout * 1000, timeout_func, NULL,
				      td, timeout_destroy_notify));
}

static void
stop_timeout_func (Camera *camera, unsigned int id, void *data)
{
	gtk_timeout_remove (id);
}

#endif

void
gtkam_tree_load (GtkamTree *tree)
{
	GtkWidget *s;
	char port[1024], speed[1024], model[1024], multi[1024];
	Camera *camera;
	CameraAbilitiesList *al;
	GPPortInfoList *il;
	GPPortInfo info;
	CameraAbilities a;
	int n, p, result;
	gchar *ms;
	guint i;
	GtkTreeIter iter;
	GtkamCamera *c;

	g_return_if_fail (GTKAM_IS_TREE (tree));

	s = gtkam_status_new (_("Loading cameras..."));
	g_signal_emit (G_OBJECT (tree), signals[NEW_STATUS], 0, s);

	gp_abilities_list_new (&al);
	gp_abilities_list_load (al, GTKAM_STATUS (s)->context->context);
	gp_port_info_list_new (&il);
	gp_port_info_list_load (il);

	gtk_object_destroy (GTK_OBJECT (s));

	/* Load settings */
	for (i = 1; ; i++) {
		ms = g_strdup_printf ("model-%i", i);
		result = gp_setting_get ("gtkam", ms, model);
		g_free (ms);
		if (result < 0)
			break;
		if (!strcmp (model, ""))
			break;

		ms = g_strdup_printf ("port-%i", i);
		result = gp_setting_get ("gtkam", ms, port);
		g_free (ms);
		if (result < 0)
			break;

		ms = g_strdup_printf ("multi-%i", i);
		result = gp_setting_get ("gtkam", ms, multi);
		g_free (ms);
		if (result < 0)
			break;

		ms = g_strdup_printf ("speed-%i", i);
		result = gp_setting_get ("gtkam", ms, speed);
		g_free (ms);
		if (result < 0)
			break;

		gp_camera_new (&camera);
#ifdef HAVE_GP_CAMERA_SET_TIMEOUT_FUNCS
                gp_camera_set_timeout_funcs (camera, start_timeout_func,
                                             stop_timeout_func, NULL);
#endif

                n = gp_abilities_list_lookup_model (al, model);
                gp_abilities_list_get_abilities (al, n, &a);

		if (strcmp (port, "") && strcmp (port, "None") &&
		    strcmp (model, "Directory Browse")) {
	                p = gp_port_info_list_lookup_path (il, port);
        	        if (p < 0) {
                	        g_warning ("Could not find '%s' in port "
					"info list (%s)!", port,
					gp_result_as_string (p));
				gp_camera_unref (camera);
				continue;
			}
			gp_port_info_list_get_info (il, p, &info);
			gp_camera_set_port_info (camera, info);
		}

                gp_camera_set_abilities (camera, a);
                if (atoi (speed))
                        gp_camera_set_port_speed (camera, atoi (speed));

                /* Add this camera to our tree. */
		gtk_tree_store_append (tree->priv->store, &iter, NULL);
		c = gtkam_camera_new (camera, atoi (multi));
		gtk_tree_store_set (tree->priv->store, &iter,
				    FOLDER_COLUMN, model,
				    CAMERA_COLUMN, c, -1);
		g_object_unref (G_OBJECT (c));
		gtkam_tree_update_iter (tree, &iter);
	}

	gp_abilities_list_free (al);
	gp_port_info_list_free (il);
}

void
gtkam_tree_save (GtkamTree *tree)
{
	GtkTreeIter child;
	guint i, n;
	GtkamCamera *camera;
	GPPortInfo info;
	CameraAbilities a;
	int speed, result;
	gchar *ms, *mm;
	char model[1024];

	/* Clear the configuration file */
	for (i = 1; ; i++) {
		ms = g_strdup_printf ("model-%i", i);
		result = gp_setting_get ("gtkam", ms, model);
		g_free (ms);
		if (result < 0)
			break;

		memset (model, 0, sizeof (model));
		ms = g_strdup_printf ("model-%i", i);
		gp_setting_set ("gtkam", ms, model);
		g_free (ms);
	}

	/* Iterate over all cameras. */
	n = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (tree->priv->store),
					    NULL);
	for (i = 0; i < n; i++) {
		gtk_tree_model_iter_nth_child (
					GTK_TREE_MODEL (tree->priv->store),
					&child, NULL, i);
		camera = gtkam_tree_get_camera_from_iter (tree, &child);

		gp_camera_get_abilities (camera->camera, &a);
		gp_camera_get_port_info (camera->camera, &info);
		speed = gp_camera_get_port_speed (camera->camera);

		ms = g_strdup_printf ("model-%i", i + 1);
		gp_setting_set ("gtkam", ms, a.model);
		g_free (ms);

		ms = g_strdup_printf ("port-%i", i + 1);
		gp_setting_set ("gtkam", ms, info.path);
		g_free (ms);

		ms = g_strdup_printf ("multi-%i", i + 1);
		mm = g_strdup_printf ("%i", camera->multi ? 1 : 0);
		gp_setting_set ("gtkam", ms, mm);
		g_free (mm);
		g_free (ms);

		ms = g_strdup_printf ("speed-%i", i + 1);
		mm = g_strdup_printf ("%i", speed);
		gp_setting_set ("gtkam", ms, mm);
		g_free (mm);
		g_free (ms);
	}
}
