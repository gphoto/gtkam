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

#include "gtkam-error.h"

struct _GtkamCListPrivate
{
	Camera *camera;

	gboolean thumbnails;
};

#define PARENT_TYPE GTK_TYPE_CLIST
static GtkCListClass *parent_class;

enum {
	FILE_SELECTED,
	FILE_UNSELECTED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void
gtkam_clist_destroy (GtkObject *object)
{
	GtkamCList *list = GTKAM_CLIST (object);

	if (list->priv->camera) {
		gp_camera_unref (list->priv->camera);
		list->priv->camera = NULL;
	}

	if (list->path) {
		g_free (list->path);
		list->path = NULL;
	}

	if (list->selection) {
		g_list_free (list->selection);
		list->selection = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_clist_finalize (GtkObject *object)
{
	GtkamCList *list = GTKAM_CLIST (object);

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

	signals[FILE_SELECTED] = gtk_signal_new ("file_selected",
		GTK_RUN_LAST, object_class->type,
		GTK_SIGNAL_OFFSET (GtkamCListClass, file_selected),
		gtk_marshal_NONE__POINTER, GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);
	signals[FILE_UNSELECTED] = gtk_signal_new ("file_unselected",
		GTK_RUN_LAST, object_class->type,
		GTK_SIGNAL_OFFSET (GtkamCListClass, file_unselected),
		gtk_marshal_NONE__POINTER, GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_clist_init (GtkamCList *list)
{
	list->priv = g_new0 (GtkamCListPrivate, 1);
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

void
gtkam_clist_set_camera (GtkamCList *list, Camera *camera)
{
	g_return_if_fail (GTKAM_IS_CLIST (list));

	if (list->priv->camera) {
		gp_camera_unref (list->priv->camera);
		list->priv->camera = NULL;
	}

	if (camera) {
		list->priv->camera = camera;
		gp_camera_ref (camera);
	}

	if (list->path) {
		g_free (list->path);
		list->path = NULL;
	}

	gtkam_clist_refresh (list);
}

static void
on_select_row (GtkCList *clist, gint row, gint column, GdkEvent *event,
	       GtkamCList *list)
{
	const gchar *path;
	
	path = gtk_clist_get_row_data (clist, row);
	list->selection = g_list_append (list->selection, (gchar *) path);
	gtk_signal_emit (GTK_OBJECT (list), signals[FILE_SELECTED], path);
}

static void
on_unselect_row (GtkCList *clist, gint row, gint column, GdkEvent *event,
		 GtkamCList *list)
{
	const gchar *path; 

	path = gtk_clist_get_row_data (clist, row);
	list->selection = g_list_remove (list->selection, (gchar *) path);
	gtk_signal_emit (GTK_OBJECT (list), signals[FILE_UNSELECTED], path);
}

GtkWidget *
gtkam_clist_new (void)
{
	GtkamCList *list;
	gchar *titles[] = {N_("Preview"), N_("Filename")};

	list = gtk_type_new (GTKAM_TYPE_CLIST);
	gtk_clist_construct (GTK_CLIST (list), 2, titles);
	gtk_clist_set_column_visibility (GTK_CLIST (list), 0, FALSE);
	gtk_signal_connect (GTK_OBJECT (list), "select_row",
			    GTK_SIGNAL_FUNC (on_select_row), list);
	gtk_signal_connect (GTK_OBJECT (list), "unselect_row",
			    GTK_SIGNAL_FUNC (on_unselect_row), list);

	return (GTK_WIDGET (list));
}

void
gtkam_clist_refresh (GtkamCList *list)
{
	gchar *path;

	g_return_if_fail (GTKAM_IS_CLIST (list));

	gtk_clist_unselect_all (GTK_CLIST (list));
	gtk_clist_clear (GTK_CLIST (list));

	if (list->path && list->priv->camera) {
		path = g_strdup (list->path);
		gtkam_clist_set_path (list, path);
		g_free (path);
	}
}

void
gtkam_clist_set_path (GtkamCList *list, const gchar *path)
{
	int result;
	CameraList flist;
	CameraFile *file;
	gchar *msg, *full_path;
	GtkWidget *dialog, *window;
	gchar *text[2];
	gint row;
	guint i;
	const char *name;

	g_return_if_fail (GTKAM_IS_CLIST (list));

	gtk_clist_unselect_all (GTK_CLIST (list));
	gtk_clist_clear (GTK_CLIST (list));

	/* If we don't have a path, that's it */
	if (list->path) {
		g_free (list->path);
		list->path = NULL;
	}
	if (!path)
		return;
	else
		list->path = g_strdup (path);

	/* If we don't have a camera, we can't do anything */
	if (!list->priv->camera)
		return;

	window = gtk_widget_get_ancestor (GTK_WIDGET (list), GTK_TYPE_WINDOW);

	result = gp_camera_folder_list_files (list->priv->camera, path, &flist);
	if (result < 0) {
		msg = g_strdup_printf (_("Could not get file list for folder "
				       "'%s'"), path);
		dialog = gtkam_error_new (msg, result, list->priv->camera,
					  window);
		gtk_widget_show (dialog);
		return;
	}

	gp_file_new (&file);
	for (i = 0; i < gp_list_count (&flist); i++) {
		gp_list_get_name (&flist, i, &name);
		text[0] = NULL;
		text[1] = (gchar*) name;
		row = gtk_clist_append (GTK_CLIST (list), text);

		if (strlen (list->path) > 1)
			full_path = g_strdup_printf ("/%s", text[1]);
		else
			full_path = g_strdup_printf ("%s/%s", path, text[1]);
		gtk_clist_set_row_data_full (GTK_CLIST (list), row, full_path,
					     (GtkDestroyNotify) g_free);
	}
	gp_file_unref (file);
}
