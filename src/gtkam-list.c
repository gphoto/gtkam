/* gtkam-list.c
 *
 * Copyright © 2001 Lutz Müller <urc8@rz.uni-karlsruhe.de>
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
#include "gtkam-list.h"
#include "i18n.h"

#include <stdio.h>
#include <string.h>

#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkitemfactory.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcellrendererpixbuf.h>
#include <gtk/gtkstock.h>

#ifdef HAVE_BONOBO
#  include <bonobo-activation/bonobo-activation.h>
#  include <bonobo/bonobo-window.h>
#  include <bonobo/bonobo-exception.h>
#  include <bonobo/bonobo-moniker-util.h>
#  include <bonobo/bonobo-widget.h>
#  include <bonobo/bonobo-stream-memory.h>
#endif

#include <gphoto2/gphoto2-list.h>
#include <gphoto2/gphoto2-port-log.h>

#include "gtkam-close.h"
#include "gtkam-error.h"
#include "gtkam-exif.h"
#include "gtkam-save.h"
#include "gdk-pixbuf-hacks.h"
#include "gtkam-info.h"
#include "gtkam-delete.h"
#include "gtkam-status.h"
#include "gtkam-util.h"
#include "gtkam-viewer.h"

struct _GtkamListPrivate
{
	GtkListStore *store;

	gboolean thumbnails;
	GtkTreeViewColumn *col_previews;

	GtkItemFactory *factory;
	GtkTreeIter iter;
};

#define PARENT_TYPE GTK_TYPE_TREE_VIEW
static GtkTreeViewClass *parent_class;

enum {
	FILE_SELECTED,
	FILE_UNSELECTED,
	NEW_STATUS,
	NEW_DIALOG,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

enum {
	PREVIEW_COLUMN = 0,
	PREVIEW_ORIG_COLUMN,
	NAME_COLUMN,
	FOLDER_COLUMN,
	CAMERA_COLUMN,
	IS_EDITABLE_COLUMN,
	NUM_COLUMNS
};

static void
gtkam_list_destroy (GtkObject *object)
{
	GtkamList *list = GTKAM_LIST (object);

	if (list->priv->factory) {
		g_object_unref (G_OBJECT (list->priv->factory));
		list->priv->factory = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_list_finalize (GObject *object)
{
	GtkamList *list = GTKAM_LIST (object);

	g_free (list->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_list_class_init (gpointer g_class, gpointer class_data)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = GTK_OBJECT_CLASS (g_class);
	object_class->destroy  = gtkam_list_destroy;

	gobject_class = G_OBJECT_CLASS (g_class);
	gobject_class->finalize = gtkam_list_finalize;

	signals[FILE_SELECTED] = g_signal_new ("file_selected",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_FIRST,
		G_STRUCT_OFFSET (GtkamListClass, file_selected), NULL, NULL,
		g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		G_TYPE_POINTER);
	signals[FILE_UNSELECTED] = g_signal_new ("file_unselected",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_FIRST,
		G_STRUCT_OFFSET (GtkamListClass, file_unselected), NULL, NULL,
		g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		G_TYPE_POINTER);
	signals[NEW_STATUS] = g_signal_new ("new_status",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkamListClass, new_status), NULL, NULL,
		g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		G_TYPE_POINTER);
	signals[NEW_DIALOG] = g_signal_new ("new_dialog",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkamListClass, new_dialog), NULL, NULL,
		g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		G_TYPE_POINTER);

	parent_class = g_type_class_peek_parent (g_class);
}

static void
gtkam_list_init (GTypeInstance *instance, gpointer g_class)
{
	GtkamList *list = GTKAM_LIST (instance);

	list->priv = g_new0 (GtkamListPrivate, 1);
	list->priv->thumbnails = TRUE;
}

GType
gtkam_list_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo ti;

		memset (&ti, 0, sizeof (GTypeInfo));
		ti.class_size     = sizeof (GtkamListClass);
		ti.class_init     = gtkam_list_class_init;
		ti.instance_size  = sizeof (GtkamList);
		ti.instance_init  = gtkam_list_init;

		type = g_type_register_static (PARENT_TYPE, "GtkamList",
					       &ti, 0);
	}

	return (type);
}

GtkamCamera *
gtkam_list_get_camera_from_iter (GtkamList *list, GtkTreeIter *iter)
{
	GValue value = {0};
	GtkamCamera *camera;

	g_return_val_if_fail (GTKAM_IS_LIST (list), NULL);

	gtk_tree_model_get_value (GTK_TREE_MODEL (list->priv->store), iter,
				  CAMERA_COLUMN, &value);
	g_assert (G_VALUE_HOLDS (&value, GTKAM_TYPE_CAMERA));
	camera = GTKAM_CAMERA (g_value_peek_pointer (&value));
	g_value_unset (&value);

	return (camera);
}

gchar *
gtkam_list_get_folder_from_iter (GtkamList *list, GtkTreeIter *iter)
{
	GValue value = {0};
	gchar *folder;

	g_return_val_if_fail (GTKAM_IS_LIST (list), NULL);

	gtk_tree_model_get_value (GTK_TREE_MODEL (list->priv->store), iter,
				  FOLDER_COLUMN, &value);
	folder = g_strdup (g_value_get_string (&value));
	g_value_unset (&value);

	return (folder);
}

gchar *
gtkam_list_get_name_from_iter (GtkamList *list, GtkTreeIter *iter)
{
	GValue value = {0};
	gchar *name;

	g_return_val_if_fail (GTKAM_IS_LIST (list), NULL);
	
	gtk_tree_model_get_value (GTK_TREE_MODEL (list->priv->store), iter,
				  NAME_COLUMN, &value);
	name = g_strdup (g_value_get_string (&value));
	g_value_unset (&value);
	
	return (name);
}

static gboolean
gtkam_list_get_iter (GtkamList *list, GtkTreeIter *iter, GtkamCamera *camera,
		     const gchar *folder, const gchar *name)
{
        gint n, i;
        gchar *f, *na;

        n = gtk_tree_model_iter_n_children (
                        GTK_TREE_MODEL (list->priv->store), NULL);
        for (i = 0; i < n; i++) {
                gtk_tree_model_iter_nth_child (
                        GTK_TREE_MODEL (list->priv->store), iter, NULL, i);
                if (camera != gtkam_list_get_camera_from_iter (list, iter))
                        continue;
                f = gtkam_list_get_folder_from_iter (list, iter);
                if (strcmp (folder, f)) {
                        g_free (f);
                        continue;
                }
		if (name) {
	                na = gtkam_list_get_name_from_iter (list, iter);
	                if (strcmp (name, na)) {
				g_free (na);
				continue;
			}
		}
		break;
	}
	return (i != n);
}

static gfloat
gtkam_list_get_zoom_factor (GtkamList *list)
{
	GValue v = {0};
	GdkPixbuf *p;
	GtkTreeIter iter;
	gfloat w, w_orig;

	g_return_val_if_fail (GTKAM_IS_LIST (list), 1.);

	if (!gtk_tree_model_get_iter_first (
				GTK_TREE_MODEL (list->priv->store), &iter))
		return (1.);

	gtk_tree_model_get_value (GTK_TREE_MODEL (list->priv->store), &iter,
				  PREVIEW_ORIG_COLUMN, &v);
	p = g_value_peek_pointer (&v);
	w_orig = (p ? gdk_pixbuf_get_width (p) : 80);
	g_value_unset (&v);
	gtk_tree_model_get_value (GTK_TREE_MODEL (list->priv->store), &iter,
				  PREVIEW_COLUMN, &v);
	p = g_value_peek_pointer (&v);
	w = (p ? gdk_pixbuf_get_width (p) : 80);
	g_value_unset (&v);

	return (w / w_orig);
}

typedef struct _GetThumbnailData GetThumbnailData;
struct _GetThumbnailData {
	GtkamCamera *camera;
	gchar *folder;
	gchar *name;
	GtkamList *list;
	GtkTreeIter *iter;
};

static gboolean
get_thumbnail_idle (gpointer data)
{
	GetThumbnailData *d = data;
	CameraFile *file;
	GtkWidget *s;
	GdkPixbuf *pixbuf;
	GdkPixbufLoader *loader;
	int result;
	const char *fd;
	unsigned long fs;
	gfloat factor;

	s = gtkam_status_new (_("Downloading thumbnail of '%s' from "
		"folder '%s'..."), d->name, d->folder);
	g_signal_emit (G_OBJECT (d->list), signals[NEW_STATUS], 0, s);
	gp_file_new (&file);
	result = gp_camera_file_get (d->camera->camera, d->folder, d->name,
			GP_FILE_TYPE_PREVIEW, file,
			GTKAM_STATUS (s)->context->context);
	if (d->camera->multi)
		gp_camera_exit (d->camera->camera, NULL);
	if (result >= 0) {
		gp_file_get_data_and_size (file, &fd, &fs);
		loader = gdk_pixbuf_loader_new ();
		gdk_pixbuf_loader_write (loader, fd, fs, NULL);
		gdk_pixbuf_loader_close (loader, NULL);
		pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
		gtk_list_store_set (d->list->priv->store, d->iter,
				    PREVIEW_ORIG_COLUMN, pixbuf, -1);
		factor = gtkam_list_get_zoom_factor (d->list);
		pixbuf = gdk_pixbuf_scale_simple (pixbuf,
			gdk_pixbuf_get_width (pixbuf) * factor,
			gdk_pixbuf_get_height (pixbuf) * factor,
			GDK_INTERP_BILINEAR);
		g_object_unref (G_OBJECT (loader));
		gtk_list_store_set (d->list->priv->store, d->iter,
				    PREVIEW_COLUMN, pixbuf, -1);
		gdk_pixbuf_unref (pixbuf);
	}

	gp_file_unref (file);
	gtk_object_destroy (GTK_OBJECT (s));

	g_object_unref (G_OBJECT (d->camera));
	g_free (d->name);
	g_free (d->folder);
	gtk_tree_iter_free (d->iter);
	g_free (d);

	return (FALSE);
}

static gboolean
show_thumbnails_foreach_func (GtkTreeModel *model, GtkTreePath *path,
                              GtkTreeIter *iter, gpointer data)
{
        GtkamList *list = GTKAM_LIST (data);
        GtkamCamera *camera;
        gchar *folder, *name;
        CameraAbilities a;
	GetThumbnailData *d;
	CameraFileInfo info;

        camera = gtkam_list_get_camera_from_iter (list, iter);
        folder = gtkam_list_get_folder_from_iter (list, iter);
        name = gtkam_list_get_name_from_iter (list, iter);

        gp_camera_get_abilities (camera->camera, &a);
	gp_camera_file_get_info (camera->camera, folder, name, &info, NULL);
	if ((a.file_operations & GP_FILE_OPERATION_PREVIEW) &&
	    info.preview.fields) {
		d = g_new0 (GetThumbnailData, 1);
		d->camera = camera;
		g_object_ref (G_OBJECT (camera));
		d->folder = g_strdup (folder);
		d->name = g_strdup (name);
		d->iter = gtk_tree_iter_copy (iter);
		d->list = list;
		g_idle_add (get_thumbnail_idle, d);
	}
	g_free (folder);
	g_free (name);
	if (camera->multi)
		gp_camera_exit (camera->camera, NULL);

        return (FALSE);
}

void
gtkam_list_show_thumbnails (GtkamList *list)
{
        g_return_if_fail (GTKAM_IS_LIST (list));

	list->priv->thumbnails = TRUE;
	gtk_tree_view_column_set_visible (list->priv->col_previews, TRUE);

        gtk_tree_model_foreach (GTK_TREE_MODEL (list->priv->store),
                                show_thumbnails_foreach_func, list);
}

void
gtkam_list_hide_thumbnails (GtkamList *list)
{
	g_return_if_fail (GTKAM_IS_LIST (list));

	list->priv->thumbnails = FALSE;
	gtk_tree_view_column_set_visible (list->priv->col_previews, FALSE);
}

static gboolean
selection_func (GtkTreeSelection *selection, GtkTreeModel *model,
		GtkTreePath *path, gboolean path_currently_selected,
		gpointer data)
{
	GtkTreeIter iter;
	gchar *folder, *name;
	GtkamList *list = GTKAM_LIST (data);
	GtkamListFileSelectedData sd;
	GtkamListFileUnselectedData ud;

	gtk_tree_model_get_iter (model, &iter, path);
	folder = gtkam_list_get_folder_from_iter (list, &iter);
	name = gtkam_list_get_name_from_iter (list, &iter);
	if (path_currently_selected) {
		ud.camera = gtkam_list_get_camera_from_iter (list, &iter);
		ud.folder = folder;
		ud.name = name;
		g_signal_emit (G_OBJECT (list), signals[FILE_UNSELECTED],
			       0, &ud);
	} else {
		sd.camera = gtkam_list_get_camera_from_iter (list, &iter);
		sd.folder = folder;
		sd.name = name;
		g_signal_emit (G_OBJECT (list), signals[FILE_SELECTED],
			       0, &sd);
	}
	g_free (name);
	g_free (folder);

	return (TRUE);
}

#ifdef HAVE_BONOBO

typedef struct _ViewAsData ViewAsData;
struct _ViewAsData {
	GtkamList *list;
	GtkamCamera *camera;
	gchar *folder;
	gchar *file;
	gchar *iid;
};

static void
on_view_as_activate (GtkMenuItem *item, ViewAsData *d)
{
	GtkWidget *w, *c, *s;
	Bonobo_Control control;
	CORBA_Environment ev;
	CameraFile *f;
	int result;
	Bonobo_PersistStream pstream;
	BonoboObject *stream;
	const char *data = NULL;
	unsigned long int size;
	const char *type;

	g_return_if_fail (d->iid != NULL);

	CORBA_exception_init (&ev);
	control = bonobo_get_object (d->iid, "IDL:Bonobo/Control:1.0", &ev);
	if (BONOBO_EX (&ev) || (control == CORBA_OBJECT_NIL)) {
		CORBA_exception_free (&ev);
		g_warning ("Could not get control from '%s'.", d->iid);
		return;
	}

	w = bonobo_window_new (d->file, d->file);
	c = bonobo_widget_new_control_from_objref (control, CORBA_OBJECT_NIL);
	gtk_widget_show (c);
	bonobo_window_set_contents (BONOBO_WINDOW (w), c);

	gtk_widget_show (w);

	s = gtkam_status_new (_("Downloading '%s' from '%s'..."), d->file,
			      d->folder);
	g_signal_emit (G_OBJECT (d->list), signals[NEW_STATUS], 0, s);
	gp_file_new (&f);
	result = gp_camera_file_get (d->camera->camera, d->folder, d->file,
				     GP_FILE_TYPE_NORMAL, f,
				     GTKAM_STATUS (s)->context->context);
	if (d->camera->multi)
		gp_camera_exit (d->camera->camera, NULL);
	if (result >= 0) {
		CORBA_exception_init (&ev);
		pstream = Bonobo_Unknown_queryInterface (control,
					"IDL:Bonobo/PersistStream:1.0", &ev);
		if (!BONOBO_EX (&ev) && (pstream != CORBA_OBJECT_NIL)) {
			gp_file_get_data_and_size (f, &data, &size);
			gp_file_get_mime_type (f, &type);
			stream = bonobo_stream_mem_create (data, size,
							   TRUE, FALSE);
			Bonobo_PersistStream_load (pstream,
				bonobo_object_corba_objref (stream), type, &ev);
			g_object_unref (G_OBJECT (stream));
			bonobo_object_release_unref (pstream, NULL);
		}
		CORBA_exception_free (&ev);
	}
	gp_file_unref (f);
	gtk_object_destroy (GTK_OBJECT (s));
}

static void
on_menu_item_destroy (GObject *object, ViewAsData *data)
{
	g_object_unref (G_OBJECT (data->camera));
	g_free (data->folder);
	g_free (data->file);
	g_free (data->iid);
}

#endif

static gint
on_button_press_event (GtkWidget *widget, GdkEventButton *event,
		       GtkamList *list)
{
	GtkTreePath *path = NULL;
	GtkWidget *w;
	GtkamCamera *camera;
	CameraAbilities a;

	switch (event->button) {
	case 3:
		if (!gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (list),
				event->x, event->y, &path, NULL, NULL, NULL))
			return (FALSE);
		gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store),
					 &list->priv->iter, path);
		gtk_tree_path_free (path);
		camera = gtkam_list_get_camera_from_iter (list,
							  &list->priv->iter);
		gp_camera_get_abilities (camera->camera, &a);

		/* What operations does the camera support? */
		w = gtk_item_factory_get_widget (list->priv->factory,
						 "/Delete");
		gtk_widget_set_sensitive (w,
			(a.file_operations & GP_FILE_OPERATION_DELETE));

#ifdef HAVE_BONOBO
{
    Bonobo_ServerInfoList *l;
    guint i;
    GtkWidget *item, *c;
    CORBA_Environment ev;
    CameraFileInfo finfo;
    gchar *fo, *fi, *query;
    ViewAsData *d;
    GList *cl;

    w = gtk_item_factory_get_widget (list->priv->factory, "/View with...");
    c = gtk_item_factory_get_widget (list->priv->factory,
		    		     "/View with.../Built-in viewer");
    cl = gtk_container_get_children (GTK_CONTAINER (w));
    for (i = 0; i < g_list_length (cl); i++) {
	    if (g_list_nth_data (cl, i) != c)
		    gtk_container_remove (GTK_CONTAINER (w),
				    GTK_WIDGET (g_list_nth_data (cl, i)));
    }

    fo = gtkam_list_get_folder_from_iter (list, &list->priv->iter);
    fi = gtkam_list_get_name_from_iter (list, &list->priv->iter);
    gp_camera_file_get_info (camera->camera, fo, fi, &finfo, NULL);
    g_free (fo);
    g_free (fi);

    if (finfo.file.fields & GP_FILE_INFO_TYPE) {
	CORBA_exception_init (&ev);
	query = g_strconcat ("repo_ids.has ('IDL:Bonobo/Control:1.0') AND "
			     "bonobo:supported_mime_types.has ('", 
			     finfo.file.type, "')", NULL);
	l = bonobo_activation_query (query, NULL, &ev);
	g_free (query);
	CORBA_exception_free (&ev);

	if (l && l->_length) {
		for (i = 0; i < l->_length; i++) {
			Bonobo_ServerInfo *si = &l->_buffer[i];
			const gchar *n;

			if (!si->iid)
				continue;

			n = bonobo_server_info_prop_lookup (si, "name", NULL);
			if (!n)
				n = si->iid;
			item = gtk_menu_item_new_with_label (n);
			gtk_widget_show (item);
			gtk_menu_shell_append (GTK_MENU_SHELL (w), item);

			d = g_new0 (ViewAsData, 1);
			d->list = list;
			d->camera = camera;
			g_object_ref (G_OBJECT (d->camera));
			d->folder = gtkam_list_get_folder_from_iter (list,
							&list->priv->iter);
			d->file = gtkam_list_get_name_from_iter (list, 
							&list->priv->iter);
			d->iid = g_strdup (si->iid);
			g_signal_connect (G_OBJECT (item), "activate",
				G_CALLBACK (on_view_as_activate), d);
			g_signal_connect (G_OBJECT (item), "destroy",
				G_CALLBACK (on_menu_item_destroy), d);
		}
	}
	if (l)
		CORBA_free (l);
    }
}
#endif

		gtk_item_factory_popup (list->priv->factory, event->x_root,
				event->y_root, event->button, event->time);

		return (TRUE);
	default:
		return (FALSE);
	}
}

typedef struct _GtkamListInfoUpdatedData GtkamListInfoUpdatedData;
struct _GtkamListInfoUpdatedData {
        GtkamList *list;
        GtkTreeIter *iter;
};

static void
on_info_updated (GtkamInfo *info, GtkamInfoInfoUpdatedData *d,
		 GtkamListInfoUpdatedData *data)
{
        gtk_list_store_set (data->list->priv->store, data->iter,
                            NAME_COLUMN, d->info.file.name, -1);
}

static void
on_info_destroy (GtkObject *object, GtkamListInfoUpdatedData *data)
{
        gtk_tree_iter_free (data->iter);
        g_free (data);
}

static void
action_info (gpointer callback_data, guint callback_action, GtkWidget *widget)
{
	GtkamList *list = GTKAM_LIST (callback_data);
	GtkWidget *d;
	GtkamCamera *camera;
	gchar *folder;
	gchar *name;
	GtkamListInfoUpdatedData *data;
	
	camera = gtkam_list_get_camera_from_iter (list, &list->priv->iter);
	folder = gtkam_list_get_folder_from_iter (list, &list->priv->iter);
	name   = gtkam_list_get_name_from_iter   (list, &list->priv->iter);
	d = gtkam_info_new (camera, folder, name);
	g_signal_emit (G_OBJECT (list), signals[NEW_DIALOG], 0, d);
	g_object_unref (G_OBJECT (d));
	g_free (folder);
	g_free (name);
	data = g_new0 (GtkamListInfoUpdatedData, 1);
	data->list = list;
	data->iter = gtk_tree_iter_copy (&list->priv->iter);
	g_signal_connect (G_OBJECT (d), "info_updated",
			  G_CALLBACK (on_info_updated), data);
	g_signal_connect (G_OBJECT (d), "destroy",
			  G_CALLBACK (on_info_destroy), data);
}

#ifdef HAVE_EXIF
static void
action_exif (gpointer callback_data, guint callback_action, GtkWidget *widget)
{
	GtkamList *list = GTKAM_LIST (callback_data);
	GtkamCamera *camera;
	gchar *folder;
	gchar *name;
	GtkWidget *d;

	camera = gtkam_list_get_camera_from_iter (list, &list->priv->iter);
	folder = gtkam_list_get_folder_from_iter (list, &list->priv->iter);
	name   = gtkam_list_get_name_from_iter   (list, &list->priv->iter);

	d = gtkam_exif_new (camera, folder, name);
	g_free (folder);
	g_free (name);
	if (!d)
		return;
	g_signal_emit (G_OBJECT (list), signals[NEW_DIALOG], 0, d);
	g_object_unref (G_OBJECT (d));
}
#endif

static void
action_save (gpointer callback_data, guint callback_action, GtkWidget *widget)
{
        GtkWidget *s;
        GtkamList *list = GTKAM_LIST (callback_data);
        GtkamCamera *camera;
        gchar *folder;
        gchar *name;

        camera = gtkam_list_get_camera_from_iter (list, &list->priv->iter);
        folder = gtkam_list_get_folder_from_iter (list, &list->priv->iter);
        name   = gtkam_list_get_name_from_iter   (list, &list->priv->iter);

        s = gtkam_save_new ();
        gtkam_save_add (GTKAM_SAVE (s), camera, folder, name);
        g_free (folder);
        g_free (name);
        g_signal_emit (G_OBJECT (list), signals[NEW_DIALOG], 0, s);
	g_object_unref (G_OBJECT (s));
}

static void
on_file_deleted (GtkamDelete *delete, GtkamDeleteFileDeletedData *data,
		 GtkamList *list)
{
	GtkTreeIter iter;
	GtkTreeSelection *sel;
	gboolean s;
	GtkamListFileUnselectedData fud;

	g_return_if_fail (GTKAM_IS_LIST (list));

	gtkam_list_get_iter (list, &iter, data->camera,
			     data->folder, data->name);
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	s = gtk_tree_selection_iter_is_selected (sel, &iter);
	gtk_list_store_remove (list->priv->store, &iter);
	if (s) {
		fud.camera = data->camera;
		fud.folder = data->folder;
		fud.name = data->name;
		g_signal_emit (G_OBJECT (list), signals[FILE_UNSELECTED], 0,
			       &fud);
	}
}

static void
on_all_deleted (GtkamDelete *delete, GtkamDeleteAllDeletedData *data,
		GtkamList *list)
{
	GtkTreeIter iter;
	gboolean s;
	GtkTreeSelection *sel;
	GtkamListFileUnselectedData fud;
	gchar *name;

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	while (gtkam_list_get_iter (list, &iter, data->camera,
				    data->folder, NULL)) {
		s = gtk_tree_selection_iter_is_selected (sel, &iter);
		gtk_list_store_remove (list->priv->store, &iter);
		if (s) {
			fud.camera = data->camera;
			fud.folder = data->folder;
			fud.name = name = gtkam_list_get_name_from_iter (
								list, &iter);
			g_signal_emit (G_OBJECT (list),
				       signals[FILE_UNSELECTED], 0, &fud);
			g_free (name);
		}
	}
}

static void
action_view (gpointer callback_data, guint callback_action,
	     GtkWidget *widget)
{
	GtkamList *list = GTKAM_LIST (callback_data);
	GtkWidget *d;
	GtkamCamera *camera;
	gchar *folder, *file;

	d = gtkam_viewer_new ();
	gtk_widget_show (d);
	g_signal_emit (G_OBJECT (list), signals[NEW_DIALOG], 0, d);

	camera = gtkam_list_get_camera_from_iter (list, &list->priv->iter);
	folder = gtkam_list_get_folder_from_iter (list, &list->priv->iter);
	file   = gtkam_list_get_name_from_iter   (list, &list->priv->iter);
	gtkam_viewer_load_file (GTKAM_VIEWER (d), camera, folder, file);
	g_free (folder);
	g_free (file);
}

static void
action_delete (gpointer callback_data, guint callback_action,
	       GtkWidget *widget)
{
	GtkWidget *d;
	GtkamList *list = GTKAM_LIST (callback_data);
	GtkamCamera *camera;
	gchar *folder;
	gchar *name;

	camera = gtkam_list_get_camera_from_iter (list, &list->priv->iter);
	folder = gtkam_list_get_folder_from_iter (list, &list->priv->iter);
	name   = gtkam_list_get_name_from_iter   (list, &list->priv->iter);

	d = gtkam_delete_new ();
	gtkam_delete_add (GTKAM_DELETE (d), camera, folder, name);
	g_free (folder);
	g_free (name);
	g_signal_connect (G_OBJECT (d), "file_deleted",
			  G_CALLBACK (on_file_deleted), list);
	g_signal_connect (G_OBJECT (d), "all_deleted",
			  G_CALLBACK (on_all_deleted), list);
	g_signal_emit (G_OBJECT (list), signals[NEW_DIALOG], 0, d);
	g_object_unref (G_OBJECT (d));
}

static void
on_edited (GtkCellRendererText *cell, const gchar *path,
	   const gchar *new_text, GtkamList *list)
{
	GtkTreeIter iter;
	gchar *folder, *name;
	GtkamCamera *camera;
	CameraFileInfo info;
	GtkWidget *s, *d;
	int r;

	g_return_if_fail (GTKAM_IS_LIST (list));

	g_return_if_fail (gtk_tree_model_get_iter_from_string (
			GTK_TREE_MODEL (list->priv->store), &iter, path));
	camera = gtkam_list_get_camera_from_iter (list, &iter);
	folder = gtkam_list_get_folder_from_iter (list, &iter);
	name   = gtkam_list_get_name_from_iter (list, &iter);

	/* Name really changed? */
	if (!strcmp (name, new_text)) {
		g_free (name);
		g_free (folder);
		return;
	}

	s = gtkam_status_new (_("Changing name of '%s' to '%s'..."),
			      name, new_text);
	g_signal_emit (G_OBJECT (list), signals[NEW_STATUS], 0, s);
	memset (&info, 0, sizeof (CameraFileInfo));
	info.file.fields = GP_FILE_INFO_NAME;
	strncpy (info.file.name, new_text, sizeof (info.file.name) - 1);
	r = gp_camera_file_set_info (camera->camera, folder, name, info,
				     GTKAM_STATUS (s)->context->context);
	if (r < 0) {
		d = gtkam_error_new (r, GTKAM_STATUS (s)->context, NULL, 
			_("Could not change the name of '%s' to '%s'."),
			name, new_text);
		g_signal_emit (G_OBJECT (list), signals[NEW_DIALOG], 0, d);
		g_object_unref (G_OBJECT (d));
	} else
		gtk_list_store_set (list->priv->store,
				    &iter, NAME_COLUMN, new_text, -1);
	gtk_object_destroy (GTK_OBJECT (s));
	g_free (folder);
	g_free (name);
}

static GtkItemFactoryEntry mi[] =
{
	{N_("/_View with..."), NULL, NULL, 0, "<Branch>"},
	{N_("/View with.../Built-in viewer"), NULL, action_view, 0, NULL},
	{"/sep0", NULL, NULL, 0, "<Separator>"},
	{N_("/_Info"), NULL, action_info, 0, NULL},
#ifdef HAVE_EXIF
	{N_("/_Exif"), NULL, action_exif, 0, NULL},
#endif
	{"/sep1", NULL, NULL, 0, "<Separator>"},
	{N_("/_Save"), NULL, action_save, 0, "<StockItem>", GTK_STOCK_SAVE},
	{N_("/_Delete"), NULL, action_delete, 0, "<StockItem>", GTK_STOCK_DELETE}
};

#ifdef ENABLE_NLS

static gchar *
translate_func (const gchar *path, gpointer data)
{
	return (_(path));
}

#endif

GtkWidget *
gtkam_list_new (void)
{
        GtkamList *list;
	GtkTreeSelection *selection;
	GtkCellRenderer *renderer;
	GtkAccelGroup *ag;
	GtkTreeViewColumn *col;

        list = g_object_new (GTKAM_TYPE_LIST, NULL);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list), FALSE);
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (list), TRUE);
	gtk_tree_view_set_reorderable (GTK_TREE_VIEW (list), TRUE);

	g_signal_connect (G_OBJECT (list), "button_press_event",
			  G_CALLBACK (on_button_press_event), list);

	ag = gtk_accel_group_new ();
	list->priv->factory = gtk_item_factory_new (GTK_TYPE_MENU, "<popup>",
						    ag);
#ifdef ENABLE_NLS
	gtk_item_factory_set_translate_func (GTK_ITEM_FACTORY (list->priv->factory),
					translate_func, NULL, NULL);
#endif
	gtk_item_factory_create_items (list->priv->factory,
				       G_N_ELEMENTS (mi), mi, list);
	g_object_ref (G_OBJECT (list->priv->factory));
	gtk_object_sink (GTK_OBJECT (list->priv->factory));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	gtk_tree_selection_set_select_function (selection, selection_func,
						list, NULL);

	list->priv->store = gtk_list_store_new (NUM_COLUMNS,
		GDK_TYPE_PIXBUF, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING,
		GTKAM_TYPE_CAMERA, G_TYPE_BOOLEAN);
	gtk_tree_view_set_model (GTK_TREE_VIEW (list),
				 GTK_TREE_MODEL (list->priv->store));

	/* Column for previews */
	renderer = gtk_cell_renderer_pixbuf_new ();
	list->priv->col_previews = gtk_tree_view_column_new_with_attributes (
		_("Preview"), renderer, "pixbuf", PREVIEW_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (list),
				     list->priv->col_previews);

	/* Column for file names */
	renderer = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (_("Name"), renderer,
		"text", NAME_COLUMN, "editable", IS_EDITABLE_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (list), col);
	g_signal_connect (G_OBJECT (renderer), "edited",
			  G_CALLBACK (on_edited), list);

        return (GTK_WIDGET (list));
}

void
gtkam_list_add_folder (GtkamList *list, GtkamCamera *camera,
		       const gchar *folder)
{
	GtkWidget *dialog, *s;
	CameraList flist;
	int result;
	const char *name;
	gint i;
	GtkTreeIter iter;

	g_return_if_fail (GTKAM_IS_LIST (list));

	s = gtkam_status_new (_("Listing files in folder '%s'..."), folder);
	g_signal_emit (G_OBJECT (list), signals[NEW_STATUS], 0, s);
	result = gp_camera_folder_list_files (camera->camera, folder, &flist,
					GTKAM_STATUS (s)->context->context);
	switch (result) {
	case GP_OK:
		break;
	case GP_ERROR_CANCEL:
		if (camera->multi)
			gp_camera_exit (camera->camera, NULL);
		gtk_object_destroy (GTK_OBJECT (s));
		return;
	default:
		if (camera->multi)
			gp_camera_exit (camera->camera, NULL);
		dialog = gtkam_error_new (result, GTKAM_STATUS (s)->context,
			NULL, _("Could not get file list for folder "
			"'%s'"), folder);
		gtk_widget_show (dialog);
		gtk_object_destroy (GTK_OBJECT (s));
		return;
	}
	gtk_object_destroy (GTK_OBJECT (s));

	for (i = 0; i < gp_list_count (&flist); i++) {
		gp_list_get_name (&flist, i, &name);
		gtk_list_store_append (list->priv->store, &iter);
		gtk_list_store_set (list->priv->store, &iter,
			NAME_COLUMN, name, FOLDER_COLUMN, folder,
			CAMERA_COLUMN, camera, IS_EDITABLE_COLUMN, TRUE, -1);
	}

	if (camera->multi)
		gp_camera_exit (camera->camera, NULL);

	if (list->priv->thumbnails)
		gtkam_list_show_thumbnails (list);

#if 0
		/*
		 * Third step: Show additional information
		 */
		if (!gdk_pixbuf_get_has_alpha (pixbuf)) {
			tmp = gdk_pixbuf_add_alpha (pixbuf, FALSE, 0, 0, 0);
			gdk_pixbuf_unref (pixbuf);
			pixbuf = tmp;
		}

		/* Check for audio data */
		if (info.audio.fields) {
			tmp = gdk_pixbuf_new_from_file (
				IMAGE_DIR "/gtkam-audio.png");
			gdk_pixbuf_add (pixbuf, 0, 0, tmp);
			gdk_pixbuf_unref (tmp);
		}

		/* Check for read-only flag */
		if ((info.file.fields & GP_FILE_INFO_PERMISSIONS) &&
		    !(info.file.permissions & GP_FILE_PERM_DELETE)) {
			tmp = gdk_pixbuf_new_from_file (
				IMAGE_DIR "/gtkam-lock.png");
			w = gdk_pixbuf_get_width (tmp);
			h = gdk_pixbuf_get_height (tmp);
			gdk_pixbuf_add (pixbuf,
				gdk_pixbuf_get_width (pixbuf) - w,
				gdk_pixbuf_get_height (pixbuf) - h,
				tmp);
			gdk_pixbuf_unref (tmp);
		}

		/* Check for downloaded flag */
		if ((info.file.fields & GP_FILE_INFO_STATUS) &&
		    (info.file.status & GP_FILE_STATUS_NOT_DOWNLOADED)){
			tmp = gdk_pixbuf_new_from_file (
				IMAGE_DIR "/gtkam-new.png");
			w = gdk_pixbuf_get_width (tmp);
			gdk_pixbuf_add (pixbuf,
				gdk_pixbuf_get_width (pixbuf) - w, 0, tmp);
			gdk_pixbuf_unref (tmp);
		}

		gdk_pixbuf_render_pixmap_and_mask (pixbuf,
					&pixmap, &bitmap, 127);
		gtk_pixmap_set (GTK_PIXMAP (item->pixmap), pixmap, bitmap);
	}
#endif
}

typedef struct _GtkamListRemoveData GtkamListRemoveData;
struct _GtkamListRemoveData {
	GtkamList *list;
	GtkamCamera *camera;
	const gchar *folder;
	GtkTreeIter *iter;
};

static gboolean
remove_foreach_func (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
		     gpointer data)
{
	GtkamListRemoveData *rd = data;
	GtkamCamera *camera;
	gchar *folder;

	camera = gtkam_list_get_camera_from_iter (rd->list, iter);
	folder = gtkam_list_get_folder_from_iter (rd->list, iter);
	g_return_val_if_fail (folder != NULL, FALSE);

	if ((camera == rd->camera) && (!strcmp (folder, rd->folder))) {
		rd->iter = gtk_tree_iter_copy (iter);
		g_free (folder);
		return (TRUE);
	}

	g_free (folder);
	return (FALSE);
}

void
gtkam_list_remove_folder (GtkamList *list, GtkamCamera *camera,
			  const gchar *folder)
{
	GtkamListRemoveData rd;

	g_return_if_fail (GTKAM_IS_LIST (list));

	rd.camera = camera;
	rd.folder = folder;
	rd.list = list;
	rd.iter = NULL;
	gtk_tree_model_foreach (GTK_TREE_MODEL (list->priv->store),
				remove_foreach_func, &rd);
	while (rd.iter) {
		gtk_list_store_remove (list->priv->store, rd.iter);
		gtk_tree_iter_free (rd.iter);
		rd.iter = NULL;
		gtk_tree_model_foreach (GTK_TREE_MODEL (list->priv->store),
					remove_foreach_func, &rd);
	}
}

typedef struct _SaveAllData SaveAllData;
struct _SaveAllData {
        GtkamList *list;
        GtkWidget *save;
        gboolean all;
};

static gboolean
save_foreach_func (GtkTreeModel *model, GtkTreePath *path,
                         GtkTreeIter *iter, gpointer data)
{
        SaveAllData *sad = data;
        GtkamCamera *camera;
        gchar *folder;
        gchar *name;
        GtkTreeSelection *s;

        s = gtk_tree_view_get_selection (GTK_TREE_VIEW (sad->list));
        if (!sad->all && !gtk_tree_selection_path_is_selected (s, path))
                return (FALSE);

        camera = gtkam_list_get_camera_from_iter (sad->list, iter);
        folder = gtkam_list_get_folder_from_iter (sad->list, iter);
        name   = gtkam_list_get_name_from_iter (sad->list, iter);
        gtkam_save_add (GTKAM_SAVE (sad->save), camera, folder, name);
        g_free (folder);
        g_free (name);

	return (FALSE);
}

static void
gtkam_list_save_common (GtkamList *list, gboolean all)
{
        SaveAllData sad;

        g_return_if_fail (GTKAM_IS_LIST (list));

        if (!gtk_tree_model_iter_n_children (
                                GTK_TREE_MODEL (list->priv->store), NULL))
                return;

        sad.list = list;
        sad.save = gtkam_save_new ();
        sad.all = all;
        gtk_tree_model_foreach (GTK_TREE_MODEL (list->priv->store),
                                save_foreach_func, &sad);
        g_signal_emit (G_OBJECT (list), signals[NEW_DIALOG], 0, sad.save);
	g_object_unref (G_OBJECT (sad.save));
}

void
gtkam_list_save_selected (GtkamList *list)
{
	g_return_if_fail (GTKAM_IS_LIST (list));

	gtkam_list_save_common (list, FALSE);
}

void
gtkam_list_save_all (GtkamList *list)
{
	g_return_if_fail (GTKAM_IS_LIST (list));

	gtkam_list_save_common (list, TRUE);
}

typedef struct _DeleteAllData DeleteAllData;
struct _DeleteAllData {
	GtkamList *list;
	GtkWidget *delete;
	gboolean all;
};

static gboolean
delete_foreach_func (GtkTreeModel *model, GtkTreePath *path,
			 GtkTreeIter *iter, gpointer data)
{
	DeleteAllData *dad = data;
	GtkamCamera *camera;
	gchar *folder;
	gchar *name;
	GtkTreeSelection *s;

	s = gtk_tree_view_get_selection (GTK_TREE_VIEW (dad->list));
	if (!dad->all && !gtk_tree_selection_path_is_selected (s, path))
		return (FALSE);

	camera = gtkam_list_get_camera_from_iter (dad->list, iter);
	folder = gtkam_list_get_folder_from_iter (dad->list, iter);
	name   = gtkam_list_get_name_from_iter (dad->list, iter);
	gtkam_delete_add (GTKAM_DELETE (dad->delete), camera, folder,
			  name);
	g_free (folder);
	g_free (name);

	return (FALSE);
}

static void
gtkam_list_delete_common (GtkamList *list, gboolean all)
{
	DeleteAllData dad;

	g_return_if_fail (GTKAM_IS_LIST (list));

	if (!gtk_tree_model_iter_n_children (
				GTK_TREE_MODEL (list->priv->store), NULL))
		return;

	dad.list = list;
	dad.delete = gtkam_delete_new ();
	dad.all = all;
	gtk_tree_model_foreach (GTK_TREE_MODEL (list->priv->store),
				delete_foreach_func, &dad);
	g_signal_connect (G_OBJECT (dad.delete), "file_deleted",
			  G_CALLBACK (on_file_deleted), list);
	g_signal_connect (G_OBJECT (dad.delete), "all_deleted",
			  G_CALLBACK (on_all_deleted), list);
	g_signal_emit (G_OBJECT (list), signals[NEW_DIALOG], 0, dad.delete);
	g_object_unref (G_OBJECT (dad.delete));
}

void
gtkam_list_delete_all (GtkamList *list)
{
	g_return_if_fail (GTKAM_IS_LIST (list));

	gtkam_list_delete_common (list, TRUE);
}

void
gtkam_list_delete_selected (GtkamList *list)
{
	g_return_if_fail (GTKAM_IS_LIST (list));

	gtkam_list_delete_common (list, FALSE);
}

guint
gtkam_list_count_all (GtkamList *list)
{
	g_return_val_if_fail (GTKAM_IS_LIST (list), 0);

	return (gtk_tree_model_iter_n_children (
				GTK_TREE_MODEL (list->priv->store), NULL));
}

static void
count_foreach_func (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
		    gpointer data)
{
	guint *n = data;

	(*n)++;
}

guint
gtkam_list_count_selected (GtkamList *list)
{
	GtkTreeSelection *selection;
	guint n = 0;

	g_return_val_if_fail (GTKAM_IS_LIST (list), 0);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	gtk_tree_selection_selected_foreach (selection, count_foreach_func, &n);

	return (n);
}

gboolean
gtkam_list_has_folder (GtkamList *list, GtkamCamera *camera,
		       const gchar *folder)
{
	GtkTreeIter iter;

	g_return_val_if_fail (GTKAM_IS_LIST (list), FALSE);

	return (gtkam_list_get_iter (list, &iter, camera, folder, NULL));
}

void
gtkam_list_add_file (GtkamList *list, GtkamCamera *camera,
		     const gchar *folder, const gchar *name)
{
	GtkTreeIter iter;

	g_return_if_fail (GTKAM_IS_LIST (list));
	g_return_if_fail (GTKAM_IS_CAMERA (camera));

	gtk_list_store_append (list->priv->store, &iter);
	gtk_list_store_set (list->priv->store, &iter,
		NAME_COLUMN, name, FOLDER_COLUMN, folder,
		CAMERA_COLUMN, camera, IS_EDITABLE_COLUMN, TRUE, -1); 
}

typedef enum {
	ZOOM_IN,
	ZOOM_OUT,
	ZOOM_100
} Zoom;

static void
zoom_factor (GtkamList *list, GtkTreeIter *iter, Zoom zoom)
{
	GValue v = {0};
	GdkPixbuf *pixbuf_orig, *pixbuf, *new;
	guint w, h;

	gtk_tree_model_get_value (GTK_TREE_MODEL (list->priv->store), iter,
				  PREVIEW_ORIG_COLUMN, &v);
	pixbuf_orig = g_value_peek_pointer (&v);
	if (!GDK_IS_PIXBUF (pixbuf_orig)) {
		g_value_unset (&v);
		return;
	}

	g_object_ref (G_OBJECT (pixbuf_orig));
	g_value_unset (&v);
	gtk_tree_model_get_value (GTK_TREE_MODEL (list->priv->store), iter, 
				  PREVIEW_COLUMN, &v);
	pixbuf = g_value_peek_pointer (&v);
	g_object_ref (G_OBJECT (pixbuf));
	g_value_unset (&v);

	switch (zoom) {
	case ZOOM_100:
		gtk_list_store_set (list->priv->store, iter, PREVIEW_COLUMN,
				    pixbuf_orig, -1);
		break;
	case ZOOM_IN:
		new = gdk_pixbuf_scale_simple (pixbuf_orig,
			gdk_pixbuf_get_width (pixbuf) * 1.2,
			gdk_pixbuf_get_height (pixbuf) * 1.2,
			GDK_INTERP_BILINEAR);
		gtk_list_store_set (list->priv->store, iter, PREVIEW_COLUMN,
				    new, -1);
		g_object_unref (G_OBJECT (new));
		break;
	case ZOOM_OUT:
		w = gdk_pixbuf_get_width (pixbuf) * 0.8;
		h = gdk_pixbuf_get_height (pixbuf) * 0.8;
		if (w && h) {
			new = gdk_pixbuf_scale_simple (pixbuf_orig, w, h,
						       GDK_INTERP_BILINEAR);
			gtk_list_store_set (list->priv->store, iter,
					    PREVIEW_COLUMN, new, -1);
			g_object_unref (G_OBJECT (new));
		}
		break;
	}
	gdk_pixbuf_unref (pixbuf);
	gdk_pixbuf_unref (pixbuf_orig);

	gtk_tree_view_columns_autosize (GTK_TREE_VIEW (list));
}

static gboolean
zoom_out_func (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
	       gpointer data)
{
	zoom_factor (GTKAM_LIST (data), iter, ZOOM_OUT);

	return (FALSE);
}

static gboolean
zoom_100_func (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
	       gpointer data)
{
	zoom_factor (GTKAM_LIST (data), iter, ZOOM_100);

	return (FALSE);
}

static gboolean
zoom_in_func (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
	      gpointer data)
{
	zoom_factor (GTKAM_LIST (data), iter, ZOOM_IN);

	return (FALSE);
}

void
gtkam_list_zoom_out (GtkamList *list)
{
	g_return_if_fail (GTKAM_IS_LIST (list));

	gtk_tree_model_foreach (GTK_TREE_MODEL (list->priv->store),
				zoom_out_func, list);
}

void
gtkam_list_zoom_100 (GtkamList *list)
{
	g_return_if_fail (GTKAM_IS_LIST (list));

	gtk_tree_model_foreach (GTK_TREE_MODEL (list->priv->store),
				zoom_100_func, list);
}

void
gtkam_list_zoom_in (GtkamList *list)
{
	g_return_if_fail (GTKAM_IS_LIST (list));

	gtk_tree_model_foreach (GTK_TREE_MODEL (list->priv->store),
				zoom_in_func, list);
}
