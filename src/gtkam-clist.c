/* gtkam-clist.c
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
#include "gtkam-clist.h"

#include <string.h>

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

#include <gtk/gtkmain.h>
#include <gdk-pixbuf/gdk-pixbuf-loader.h>

#include "gtkam-error.h"
#include "gtkam-status.h"
#include "gtkam-util.h"

struct _GtkamCListPrivate
{
	GPtrArray *idle;

	gboolean thumbnails;
};

#define PARENT_TYPE GTK_TYPE_CLIST
static GtkCListClass *parent_class;

enum {
	ENTRY_SELECTED,
	ENTRY_UNSELECTED,
	NEW_STATUS,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

typedef struct {
	gint32 id;
	guint row;
	GtkamCList *clist;
} IdleData;

static void
gtkam_clist_destroy (GtkObject *object)
{
	GtkamCList *list = GTKAM_CLIST (object);
	guint i;
	IdleData *id;

	if (list->selection) {
		g_list_free (list->selection);
		list->selection = NULL;
	}

	for (i = 0; i < list->priv->idle->len; i++) {
		id = list->priv->idle->pdata[i];
		gtk_idle_remove (id->id);
		g_free (id);
	}
	g_ptr_array_set_size (list->priv->idle, 0);

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_clist_finalize (GtkObject *object)
{
	GtkamCList *list = GTKAM_CLIST (object);

	g_ptr_array_free (list->priv->idle, TRUE);

	g_free (list->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_clist_class_init (GtkamCListClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_clist_destroy;
	object_class->finalize = gtkam_clist_finalize;

	signals[ENTRY_SELECTED] = gtk_signal_new ("entry_selected",
		GTK_RUN_LAST, object_class->type,
		GTK_SIGNAL_OFFSET (GtkamCListClass, entry_selected),
		gtk_marshal_NONE__POINTER, GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);
	signals[ENTRY_UNSELECTED] = gtk_signal_new ("entry_unselected",
		GTK_RUN_LAST, object_class->type,
		GTK_SIGNAL_OFFSET (GtkamCListClass, entry_unselected),
		gtk_marshal_NONE__POINTER, GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);
	signals[NEW_STATUS] = gtk_signal_new ("new_status",
		GTK_RUN_FIRST, object_class->type,
		GTK_SIGNAL_OFFSET (GtkamCListClass, new_status),
		gtk_marshal_NONE__POINTER, GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_clist_init (GtkamCList *list)
{
	list->priv = g_new0 (GtkamCListPrivate, 1);
	list->priv->idle = g_ptr_array_new ();
}

GtkType
gtkam_clist_get_type (void)
{
        static GtkType list_type = 0;

        if (!list_type) {
                static const GtkTypeInfo list_info = {
                        "GtkamCList",
                        sizeof (GtkamCList),
                        sizeof (GtkamCListClass),
                        (GtkClassInitFunc)  gtkam_clist_class_init,
                        (GtkObjectInitFunc) gtkam_clist_init,
                        NULL, NULL, NULL};
                list_type = gtk_type_unique (PARENT_TYPE, &list_info);
        }

        return (list_type);
}

static void
on_select_row (GtkCList *clist, gint row, gint column, GdkEvent *event,
	       GtkamCList *list)
{
	GtkamCListEntry *entry;
	
	entry = gtk_clist_get_row_data (clist, row);
	list->selection = g_list_append (list->selection, entry);
	gtk_signal_emit (GTK_OBJECT (list), signals[ENTRY_SELECTED], entry);
}

static void
on_unselect_row (GtkCList *clist, gint row, gint column, GdkEvent *event,
		 GtkamCList *list)
{
	GtkamCListEntry *entry;

	entry = gtk_clist_get_row_data (clist, row);
	list->selection = g_list_remove (list->selection, entry);
	gtk_signal_emit (GTK_OBJECT (list), signals[ENTRY_UNSELECTED], entry);
}

GtkWidget *
gtkam_clist_new (void)
{
	GtkamCList *list;
	gchar *titles[] = {N_("Preview"), N_("Filename")};

	list = gtk_type_new (GTKAM_TYPE_CLIST);
	gtk_clist_construct (GTK_CLIST (list), 2, titles);
	gtk_clist_set_column_visibility (GTK_CLIST (list), 0, TRUE);
	gtk_clist_set_column_auto_resize (GTK_CLIST (list), 0, TRUE);
	gtk_clist_set_selection_mode (GTK_CLIST (list), GTK_SELECTION_MULTIPLE);
	gtk_clist_set_shadow_type (GTK_CLIST (list), GTK_SHADOW_NONE);
	gtk_signal_connect (GTK_OBJECT (list), "select_row",
			    GTK_SIGNAL_FUNC (on_select_row), list);
	gtk_signal_connect (GTK_OBJECT (list), "unselect_row",
			    GTK_SIGNAL_FUNC (on_unselect_row), list);

	return (GTK_WIDGET (list));
}

static gboolean
idle_func (gpointer idle_data)
{
	IdleData *id = idle_data;
	CameraFile *file;
	int result;
	GdkPixmap *pixmap;
	GdkBitmap *bitmap;
	GtkWidget *dialog, *win, *status;
	GdkPixbuf *pixbuf;
	guint w, h;
	GtkamCListEntry *entry;

	win = gtk_widget_get_ancestor (GTK_WIDGET (id->clist),
				       GTK_TYPE_WINDOW);

	entry = gtk_clist_get_row_data (GTK_CLIST (id->clist), id->row);
	gp_file_new (&file);
	status = gtkam_status_new (_("Getting file '%s' from folder '%s'..."),
		entry->name, entry->folder);
	gtk_signal_emit (GTK_OBJECT (id->clist), signals[NEW_STATUS], status);
	result = gp_camera_file_get (entry->camera, entry->folder,
				     entry->name, GP_FILE_TYPE_PREVIEW, file,
				     GTKAM_STATUS (status)->context->context);
	if (entry->multi)
		gp_camera_exit (entry->camera, NULL);
	switch (result) {
	case GP_OK:
		pixbuf = gdk_pixbuf_new_from_camera_file (file, 40, win);
		if (!pixbuf)
			break;

		w = gdk_pixbuf_get_width (pixbuf);
		h = gdk_pixbuf_get_height (pixbuf);
		gdk_pixbuf_render_pixmap_and_mask (pixbuf,
						   &pixmap, &bitmap, 127);
		gdk_pixbuf_unref (pixbuf);
		gtk_clist_set_pixmap (GTK_CLIST (id->clist), id->row,
				0, pixmap, bitmap);
		gtk_clist_set_row_height (GTK_CLIST (id->clist),
			MAX (h, GTK_CLIST (id->clist)->row_height));
		if (pixmap)
			gdk_pixmap_unref (pixmap);
		if (bitmap)
			gdk_bitmap_unref (bitmap);
		break;
	case GP_ERROR_CANCEL:
		break;
	default:
		dialog = gtkam_error_new (result,
			GTKAM_STATUS (status)->context, win,
			_("Could not get file '%s'."), entry->name);
		gtk_widget_show (dialog);
		break;
	}
	gtk_object_destroy (GTK_OBJECT (status));
	gp_file_unref (file);

	g_ptr_array_remove (id->clist->priv->idle, id);
	g_free (id);

	return (FALSE);
}

static void
free_entry (gpointer data)
{
	GtkamCListEntry *entry = data;

	gp_camera_unref (entry->camera);
	g_free (entry->folder);
	g_free (entry->name);
	g_free (entry);
}

void
gtkam_clist_add_folder (GtkamCList *list, Camera *camera, gboolean multi,
			const gchar *folder)
{
	int result;
	CameraList flist;
	CameraAbilities a;
	GtkWidget *dialog, *window, *status;
	gchar *text[2];
	gint row;
	guint i;
	const char *name;
	GtkamCListEntry *entry;

	g_return_if_fail (GTKAM_IS_CLIST (list));

	gtk_clist_unselect_all (GTK_CLIST (list));
	gtk_clist_clear (GTK_CLIST (list));

	window = gtk_widget_get_ancestor (GTK_WIDGET (list), GTK_TYPE_WINDOW);

	status = gtkam_status_new (_("Getting list of files in "
				   "folder '%s'..."), folder);
	gtk_signal_emit (GTK_OBJECT (list), signals[NEW_STATUS], status);
	result = gp_camera_folder_list_files (camera, folder, &flist,
				GTKAM_STATUS (status)->context->context);
	if (multi)
		gp_camera_exit (camera, NULL);
	switch (result) {
	case GP_OK:
		break;
	case GP_ERROR_CANCEL:
		gp_list_reset (&flist);
		break;
	default:
		dialog = gtkam_error_new (result,
			GTKAM_STATUS (status)->context, window,
			_("Could not get file list for folder '%s'."), folder);
		gtk_widget_show (dialog);
		gp_list_reset (&flist);
		break;
	}
	gtk_object_destroy (GTK_OBJECT (status));

	gp_camera_get_abilities (camera, &a);
	for (i = 0; i < gp_list_count (&flist); i++) {
		gp_list_get_name (&flist, i, &name);
		text[0] = NULL;
		text[1] = (gchar*) name;
		row = gtk_clist_append (GTK_CLIST (list), text);

		if (a.file_operations & GP_FILE_OPERATION_PREVIEW) {
			IdleData *id;

			id = g_new0 (IdleData, 1);
			id->row = row;
			id->clist = list;
			id->id = gtk_idle_add (idle_func, id);
			g_ptr_array_add (list->priv->idle, id);
		}

		entry = g_new0 (GtkamCListEntry, 1);
		gp_camera_ref (entry->camera = camera);
		entry->multi = multi;
		entry->folder = g_strdup (folder);
		entry->name = g_strdup (name);
		gtk_clist_set_row_data_full (GTK_CLIST (list), row, entry,
					     (GtkDestroyNotify) free_entry);
	}
}

void
gtkam_clist_remove_folder (GtkamCList *clist, Camera *camera,
			   gboolean multi, const gchar *folder)
{
	GtkamCListEntry *entry;
	guint i;

	g_return_if_fail (GTKAM_IS_CLIST (clist));

	for (i = GTK_CLIST (clist)->rows; i > 0; i--) {
		entry = gtk_clist_get_row_data (GTK_CLIST (clist), i - 1);
		if ((entry->camera == camera) &&
		    !strcmp (entry->folder, folder)) {
			clist->selection = g_list_remove (clist->selection,
							  entry);
			gtk_clist_remove (GTK_CLIST (clist), i - 1);
		}
	}
}
