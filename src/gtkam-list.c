/* gtkam-list.c
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
#include "gtkam-list.h"

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

#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkmenu.h>
#include <gdk-pixbuf/gdk-pixbuf-loader.h>

#include <gphoto2/gphoto2-list.h>
#include <gphoto2/gphoto2-port-log.h>

#include "gtkam-close.h"
#include "gtkam-error.h"
#include "gtkam-exif.h"
#include "../pixmaps/no_thumbnail.xpm"
#include "gtkam-save.h"
#include "gdk-pixbuf-hacks.h"
#include "gtkam-info.h"
#include "gtkam-delete.h"
#include "gtkam-util.h"

/* Should that be configurable? */
#define ICON_WIDTH 80

struct _GtkamListPrivate
{
	Camera *camera;

	gboolean thumbnails;
	gboolean multi;
};

#define PARENT_TYPE GTK_TYPE_ICON_LIST
static GtkIconListClass *parent_class;

enum {
	CHANGED,
	DOWNLOAD_START,
	DOWNLOAD_STOP,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void
gtkam_list_destroy (GtkObject *object)
{
	GtkamList *list = GTKAM_LIST (object);

	if (list->priv->camera) {
		gp_camera_unref (list->priv->camera);
		list->priv->camera = NULL;
	}

	if (list->path) {
		g_free (list->path);
		list->path = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_list_finalize (GtkObject *object)
{
	GtkamList *list = GTKAM_LIST (object);

	g_free (list->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_list_class_init (GtkamListClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_list_destroy;
	object_class->finalize = gtkam_list_finalize;

	signals[CHANGED] = gtk_signal_new ("changed",
		GTK_RUN_LAST, object_class->type,
		GTK_SIGNAL_OFFSET (GtkamListClass, changed),
		gtk_marshal_NONE__NONE, GTK_TYPE_NONE, 0);
	signals[DOWNLOAD_START] = gtk_signal_new ("download_start",
		GTK_RUN_FIRST, object_class->type,
		GTK_SIGNAL_OFFSET (GtkamListClass, download_start),
		gtk_marshal_NONE__POINTER, GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);
	signals[DOWNLOAD_STOP] = gtk_signal_new ("download_stop",
		GTK_RUN_FIRST, object_class->type,
		GTK_SIGNAL_OFFSET (GtkamListClass, download_stop),
		gtk_marshal_NONE__POINTER, GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_list_init (GtkamList *list)
{
	list->priv = g_new0 (GtkamListPrivate, 1);
	list->priv->thumbnails = TRUE;
}

GtkType
gtkam_list_get_type (void)
{
	static GtkType list_type = 0;

	if (!list_type) {
		static const GtkTypeInfo list_info = {
			"GtkamList",
			sizeof (GtkamList),
			sizeof (GtkamListClass),
			(GtkClassInitFunc)  gtkam_list_class_init,
			(GtkObjectInitFunc) gtkam_list_init,
			NULL, NULL, NULL};
		list_type = gtk_type_unique (PARENT_TYPE, &list_info);
	}

	return (list_type);
}

void
gtkam_list_set_camera (GtkamList *list, Camera *camera, gboolean multi)
{
	g_return_if_fail (GTKAM_IS_LIST (list));

	if (list->priv->camera) {
		gp_camera_unref (list->priv->camera);
		list->priv->camera = NULL;
	}
	if (camera) {
		list->priv->camera = camera;
		gp_camera_ref (camera);
		list->priv->multi = multi;
	}

	if (list->path) {
		g_free (list->path);
		list->path = NULL;
	}

	gtkam_list_refresh (list);
}

static void
on_info_updated (GtkamInfo *info, const gchar *path, GtkamList *list)
{
	gtkam_list_refresh (list);
}

typedef struct {
	GtkWidget *menu;
	GtkamList *list;
	GtkIconListItem *item;
} PopupData;

static void
on_info_activate (GtkMenuItem *item, PopupData *data)
{
	GtkamList *list = data->list;
	GtkWidget *w, *info;
	gchar *path;

	w = gtk_widget_get_ancestor (GTK_WIDGET (list), GTK_TYPE_WINDOW);
	if (strlen (list->path) == 1)
		path = g_strdup_printf ("/%s", data->item->label);
	else
		path = g_strdup_printf ("%s/%s", list->path,
					data->item->label);
	info = gtkam_info_new (list->priv->camera, path, w);
	g_free (path);
	if (info) {
		gtk_widget_show (info);
		gtk_signal_connect_while_alive (GTK_OBJECT (info),
			"info_updated", GTK_SIGNAL_FUNC (on_info_updated),
			list, GTK_OBJECT (list));
	}
}

static void
on_file_deleted (GtkamDelete *delete, const gchar *path, GtkamList *list)
{
	gchar *dir;

	dir = g_dirname (path);
	if (!strcmp (dir, list->path))
		gtkam_list_refresh (list);
	g_free (dir);
}

static void
on_delete_activate (GtkMenuItem *menu_item, PopupData *data)
{
	GtkamList *list = data->list;
	GtkIconListItem *item = data->item;
	GtkWidget *delete, *w;
	GList *files;

	files = g_list_append (NULL, item->label);
	w = gtk_widget_get_ancestor (GTK_WIDGET (list), GTK_TYPE_WINDOW);
	delete = gtkam_delete_new (list->priv->camera, list->priv->multi, 
				   list->path, files, w);
	g_list_free (files);
	gtk_widget_show (delete);
	gtk_signal_connect_while_alive (GTK_OBJECT (delete), "file_deleted",
		GTK_SIGNAL_FUNC (on_file_deleted), list, GTK_OBJECT (list));
}

static void
on_exif_activate (GtkMenuItem *menu_item, PopupData *data)
{
	GtkamList *list = data->list;
	GtkWidget *dialog, *w;

	w = gtk_widget_get_ancestor (GTK_WIDGET (data->list), GTK_TYPE_WINDOW);
	dialog = gtkam_exif_new (list->priv->camera, list->priv->multi, 
				 list->path, data->item->label, w);
	if (dialog)
		gtk_widget_show (dialog);
}

static void
on_save_activate (GtkMenuItem *menu_item, PopupData *data)
{
	GtkamList *list = data->list;
	GtkIconListItem *item = data->item;
	GtkWidget *save, *w;
	GSList *files;

	w = gtk_widget_get_ancestor (GTK_WIDGET (list), GTK_TYPE_WINDOW);
	files = g_slist_append (NULL, item->label);
	save = gtkam_save_new (list->priv->camera, list->priv->multi,
			       list->path, files, w);
	g_slist_free (files);
	gtk_widget_show (save);
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

static gboolean
on_select_icon (GtkIconList *ilist, GtkIconListItem *item,
		GdkEventButton *event, GtkamList *list)
{
	CameraAbilities a;
	CameraFile *file;
	GtkWidget *dialog, *w;
	int result;
	gchar *msg;

	if (!event)
		return (TRUE);

	w = gtk_widget_get_ancestor (GTK_WIDGET (list), GTK_TYPE_WINDOW);
	gp_camera_get_abilities (list->priv->camera, &a);
	if ((event->type == GDK_2BUTTON_PRESS) &&
	    (a.file_operations & GP_FILE_OPERATION_PREVIEW)) {
		CameraFile *file;

		/* Double-click: Get thumbnail */
		gp_file_new (&file);
		gtk_signal_emit (GTK_OBJECT (list), signals[DOWNLOAD_START],
				 file);
		result = gp_camera_file_get (list->priv->camera,
			list->path, item->label, GP_FILE_TYPE_PREVIEW, file,
			NULL);
		gtk_signal_emit (GTK_OBJECT (list), signals[DOWNLOAD_STOP],
				 file);
		if (list->priv->multi)
			gp_camera_exit (list->priv->camera, NULL);
		if (result < 0) {
			switch (result) {
			case GP_ERROR_CANCEL:
				break;
			default:
				msg = g_strdup_printf (
					_("Could not get preview of "
					"file '%s' in folder '%s'"),
					item->label, list->path);
				dialog = gtkam_error_new (msg, result,
						list->priv->camera, w);
				g_free (msg);
				gtk_widget_show (dialog);
			}
		} else {
			GdkPixbuf *pixbuf;
			GdkPixmap *pixmap;
			GdkBitmap *bitmap;

			pixbuf = gdk_pixbuf_new_from_camera_file (file,
								ICON_WIDTH, w);
			if (pixbuf) {
				gdk_pixbuf_render_pixmap_and_mask (pixbuf,
						&pixmap, &bitmap, 127);
				gdk_pixbuf_unref (pixbuf);
				gtk_pixmap_set (GTK_PIXMAP (item->pixmap),
						pixmap, bitmap);
				item->state = GTK_STATE_SELECTED;
			}
		}
		gp_file_unref (file);

		while (gtk_events_pending ())
			gtk_main_iteration ();

		return (FALSE);
	} else if (event->type == GDK_BUTTON_PRESS) {
		if (event->button == 3) {
			CameraFileInfo info;
			PopupData *data;
			GtkWidget *i;

			while (gtk_events_pending ())
				gtk_main_iteration ();

			data = g_new0 (PopupData, 1);
			data->list = list;
			data->item = item;

			/* Right-click: Show popup menu */
			data->menu = gtk_menu_new ();
			gtk_widget_show (data->menu);
			i = gtk_menu_item_new_with_label (_("Info"));
			gtk_widget_show (i);
			gtk_container_add (GTK_CONTAINER (data->menu), i);
			gtk_signal_connect (GTK_OBJECT (i), "activate",
				GTK_SIGNAL_FUNC (on_info_activate), data);

			i = gtk_menu_item_new_with_label (_("Exif"));
			gtk_widget_show (i);
			gtk_container_add (GTK_CONTAINER (data->menu), i);
			gtk_signal_connect (GTK_OBJECT (i), "activate",
				GTK_SIGNAL_FUNC (on_exif_activate), data);
			gp_file_new (&file);
			gtk_signal_emit (GTK_OBJECT (list),
					 signals[DOWNLOAD_START], file);
			if (gp_camera_file_get (list->priv->camera,
				list->path, item->label, GP_FILE_TYPE_EXIF,
				file, NULL) < 0)
				gtk_widget_set_sensitive (i, FALSE);
			gtk_signal_emit (GTK_OBJECT (list), 
					 signals[DOWNLOAD_STOP], file);
			gp_file_unref (file);

			i = gtk_menu_item_new ();
			gtk_widget_show (i);
			gtk_container_add (GTK_CONTAINER (data->menu), i);
			gtk_widget_set_sensitive (i, FALSE);

			/* Save */
			i = gtk_menu_item_new_with_label (_("Save"));
			gtk_widget_show (i);
			gtk_container_add (GTK_CONTAINER (data->menu), i);
			gtk_signal_connect (GTK_OBJECT (i), "activate",
				GTK_SIGNAL_FUNC (on_save_activate), data);

			/* Delete */
			i = gtk_menu_item_new_with_label (_("Delete"));
			gtk_widget_show (i);
			if (!(a.file_operations & GP_FILE_OPERATION_DELETE) ||
			    (!gp_camera_file_get_info (list->priv->camera,
					list->path, item->label, &info, NULL) &&
			     (info.file.fields & GP_FILE_INFO_PERMISSIONS) && 
			     (!(info.file.permissions & GP_FILE_PERM_DELETE))))
				gtk_widget_set_sensitive (i, FALSE);
			gtk_container_add (GTK_CONTAINER (data->menu), i);
			gtk_signal_connect (GTK_OBJECT (i), "activate",
				GTK_SIGNAL_FUNC (on_delete_activate), data);

			gtk_signal_connect (GTK_OBJECT (data->menu), "hide",
					    GTK_SIGNAL_FUNC (kill_popup_menu),
					    data);
			gtk_menu_popup (GTK_MENU (data->menu), NULL, NULL,
					NULL, NULL, event->button, event->time);

			return (FALSE);
		}
	}

	return (TRUE);
}

GtkWidget *
gtkam_list_new (void)
{
        GtkamList *list;

        list = gtk_type_new (GTKAM_TYPE_LIST);
        gtk_icon_list_construct (GTK_ICON_LIST (list), ICON_WIDTH,
                                 GTK_ICON_LIST_TEXT_BELOW);
	gtk_icon_list_set_text_space (GTK_ICON_LIST (list), ICON_WIDTH);
        gtk_icon_list_set_selection_mode (GTK_ICON_LIST (list),
                                          GTK_SELECTION_MULTIPLE);
        gtk_icon_list_set_editable (GTK_ICON_LIST (list), FALSE);
        gtk_signal_connect (GTK_OBJECT (list), "select_icon",
                            GTK_SIGNAL_FUNC (on_select_icon), list);

        return (GTK_WIDGET (list));
}

void
gtkam_list_set_path (GtkamList *list, const gchar *path)
{
	GtkWidget *dialog, *win;
	GtkIconListItem *item;
	gchar *msg;
	CameraList flist;
	CameraFile *file;
	CameraFileInfo info;
	CameraAbilities a;
	int result;
	const char *name;
	GdkPixbuf *pixbuf, *tmp;
	GdkPixmap *pixmap;
	GdkBitmap *bitmap;
	gint i;
	guint w, h;

	g_return_if_fail (GTKAM_IS_LIST (list));

	/* Remove all current items */
	gtk_icon_list_freeze (GTK_ICON_LIST (list));
	gtk_icon_list_clear (GTK_ICON_LIST (list));
	gtk_icon_list_thaw (GTK_ICON_LIST (list));
	gtk_signal_emit (GTK_OBJECT (list), signals[CHANGED]);

	/* If we don't have a path, that's it */
	if (list->path) {
		g_free (list->path);
		list->path = NULL;
	}
	if (!path)
		return;
	else
		list->path = g_strdup (path);

	win = gtk_widget_get_ancestor (GTK_WIDGET (list), GTK_TYPE_WINDOW);

	/* If we don't have a camera, we can't do anything */
	if (!list->priv->camera)
		return;

	result = gp_camera_folder_list_files (list->priv->camera, path, &flist,
					      NULL);
	if (result < 0) {
		if (list->priv->multi)
			gp_camera_exit (list->priv->camera, NULL);
		msg = g_strdup_printf (_("Could not get file list for folder "
				       "'%s'"), path);
		dialog = gtkam_error_new (msg, result, list->priv->camera, win);
		gtk_widget_show (dialog);
		return;
	}

	gp_camera_get_abilities (list->priv->camera, &a);
	for (i = 0; i < gp_list_count (&flist); i++) {
		gp_list_get_name (&flist, i, &name);

		/*
		 * First step: Show the plain icon
		 */
		pixbuf = gdk_pixbuf_new_from_xpm_data (
					(const char**) no_thumbnail_xpm);
		gdk_pixbuf_render_pixmap_and_mask (pixbuf, &pixmap, &bitmap,
						   127);
		gtk_icon_list_freeze (GTK_ICON_LIST (list));
		item = gtk_icon_list_add_from_pixmap (GTK_ICON_LIST (list),
						pixmap, bitmap, name, NULL);
		gtk_icon_list_thaw (GTK_ICON_LIST (list));

		/*
		 * Second step: Get information about the image.
		 */
		result = gp_camera_file_get_info (list->priv->camera, path,
						  name, &info, NULL);
		if (result != GP_OK) {
			gp_log (GP_LOG_DEBUG, PACKAGE, "Could not get "
				"information on '%s' in '%s': %s",
				path, name, gp_result_as_string (result));
			continue;
		}

		/*
		 * Third step: Show the preview if there is one and
		 * 	       if it has been requested.
		 */
		if (list->priv->thumbnails &&
		    (a.file_operations & GP_FILE_OPERATION_PREVIEW) &&
		    info.preview.fields) {
			gp_file_new (&file);
			gtk_signal_emit (GTK_OBJECT (list),
					 signals[DOWNLOAD_START], file);
			result = gp_camera_file_get (list->priv->camera, path,
					name, GP_FILE_TYPE_PREVIEW, file, NULL);

			/* Make sure we are not shutting down */
			if (!GTKAM_IS_LIST (list)) {
				gp_file_unref (file);
				break;
			}
			gtk_signal_emit (GTK_OBJECT (list),
					 signals[DOWNLOAD_STOP], file);

			if (result < 0) {
				switch (result) {
				case GP_ERROR_CANCEL:
					break;
				default:
					msg = g_strdup_printf (_("Could not "
						"get file '%s'"), name);
					dialog = gtkam_error_new (msg, result,
						list->priv->camera, win);
					gtk_widget_show (dialog);
				}
			} else {
				tmp = gdk_pixbuf_new_from_camera_file (file,
							ICON_WIDTH, win);
				if (tmp) {
					gdk_pixbuf_unref (pixbuf);
					pixbuf = tmp;
					gdk_pixbuf_render_pixmap_and_mask (
						pixbuf, &pixmap, &bitmap, 127);
					gtk_pixmap_set (
						GTK_PIXMAP (item->pixmap),
						pixmap, bitmap);
				}
			}
			gp_file_unref (file);
		}

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

	if (!GTKAM_IS_LIST (list))
		return;
	
	if (list->priv->multi)
		gp_camera_exit (list->priv->camera, NULL);
	gtk_signal_emit (GTK_OBJECT (list), signals[CHANGED]);
}

void
gtkam_list_set_thumbnails (GtkamList *list, gboolean thumbnails)
{
	g_return_if_fail (GTKAM_IS_LIST (list));

	if (list->priv->thumbnails == thumbnails)
		return;

	list->priv->thumbnails = thumbnails;
	gtkam_list_refresh (list);
}

void
gtkam_list_save_selected (GtkamList *list)
{
	GtkIconListItem *item;
	GSList *filenames = NULL;
	GtkWidget *save, *w;
	guint i;

	g_return_if_fail (GTKAM_IS_LIST (list));

	if (!g_list_length (GTK_ICON_LIST (list)->selection))
		return;

	for (i = 0; i < g_list_length (GTK_ICON_LIST (list)->selection); i++) {
		item = g_list_nth_data (GTK_ICON_LIST (list)->selection, i);
		filenames = g_slist_append (filenames,
				gtk_entry_get_text (GTK_ENTRY (item->entry)));
	}

	w = gtk_widget_get_ancestor (GTK_WIDGET (list), GTK_TYPE_WINDOW);
	save = gtkam_save_new (list->priv->camera, list->priv->multi, 
			       list->path, filenames, w);
	g_slist_free (filenames);
	gtk_widget_show (save);
}

void
gtkam_list_refresh (GtkamList *list)
{
	gchar *path = NULL;

	g_return_if_fail (GTKAM_IS_LIST (list));

	gtk_icon_list_clear (GTK_ICON_LIST (list));

	if (list->path && list->priv->camera) {
		path = g_strdup (list->path);
		gtkam_list_set_path (list, path);
		g_free (path);
	}
}
