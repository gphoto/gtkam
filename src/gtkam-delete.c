/* gtkam-delete.c
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
ADB Still to amend
 */

#include "config.h"
#include "gtkam-delete.h"
#include "i18n.h"

#include <string.h>

#include <gtk/gtkcheckbutton.h>
#include <gtk/gtklabel.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkstock.h>

#include "gtkam-error.h"
#include "gtkam-status.h"

typedef struct _GtkamDeleteData GtkamDeleteData;
struct _GtkamDeleteData {
	GtkamCamera *camera;
	gchar *folder;
	gchar *name;
};

struct _GtkamDeletePrivate
{
	GSList *data;
	GtkWidget *vbox;

	GtkWidget *msg;
	
	gboolean deleteall;
};

#define PARENT_TYPE GTK_TYPE_DIALOG
static GtkDialogClass *parent_class;

enum {
	ALL_DELETED,
	FILE_DELETED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void
gtkam_delete_destroy (GtkObject *object)
{
	GtkamDelete *delete = GTKAM_DELETE (object);
	gint i;
	GtkamDeleteData *data;

	if (delete->priv->data) {
		for (i = g_slist_length (delete->priv->data) - 1; i >= 0; i--) {
			data = g_slist_nth_data (delete->priv->data, i);
			g_object_unref (G_OBJECT (data->camera));
			g_free (data->folder);
			g_free (data->name);
			g_free (data);
		}
		g_slist_free (delete->priv->data);
		delete->priv->data = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_delete_finalize (GObject *object)
{
	GtkamDelete *delete = GTKAM_DELETE (object);

	g_free (delete->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_delete_class_init (gpointer g_class, gpointer class_data)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = GTK_OBJECT_CLASS (g_class);
	object_class->destroy  = gtkam_delete_destroy;

	gobject_class = G_OBJECT_CLASS (g_class);
	gobject_class->finalize = gtkam_delete_finalize;

	signals[FILE_DELETED] = g_signal_new ("file_deleted",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_FIRST,
		G_STRUCT_OFFSET (GtkamDeleteClass, file_deleted), NULL, NULL,
		g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		G_TYPE_POINTER);
	signals[ALL_DELETED] = g_signal_new ("all_deleted",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_FIRST,
		G_STRUCT_OFFSET (GtkamDeleteClass, all_deleted), NULL, NULL,
		g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		G_TYPE_POINTER);

	parent_class = g_type_class_peek_parent (g_class);
}

static void
gtkam_delete_init (GTypeInstance *instance, gpointer g_class)
{
	GtkamDelete *delete = GTKAM_DELETE (instance);

	delete->priv = g_new0 (GtkamDeletePrivate, 1);
}

GType
gtkam_delete_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo ti;

		memset (&ti, 0, sizeof (GTypeInfo)); 
		ti.class_size     = sizeof (GtkamDeleteClass);
		ti.class_init     = gtkam_delete_class_init;
		ti.instance_size  = sizeof (GtkamDelete);
		ti.instance_init  = gtkam_delete_init;

		type = g_type_register_static (PARENT_TYPE, "GtkamDelete",
					       &ti, 0);
	}

	return (type);
}

static int
gp_list_lookup_name (CameraList *list, const char *name)
{
	int i;
	const char *n;

	for (i = 0; i < gp_list_count (list); i++) {
		gp_list_get_name (list, i, &n);
		if (!strcmp (n, name))
			return (i);
	}

	return (GP_ERROR);
}

static gboolean
delete_all (GtkamDelete *delete, GtkamCamera *camera,
	    const gchar *folder)
{
	GtkWidget *d, *s;
	int result, r1, r2;
	CameraList *l1, *l2;
	const char *name;
	GtkamDeleteAllDeletedData add;
	GtkamDeleteFileDeletedData fdd;

	s = gtkam_status_new (_("Deleting all files in '%s'..."), folder);
	gtk_widget_show (s);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (delete)->vbox), s,
			    FALSE, FALSE, 0);
	gp_list_new (&l1);
	r1 = gp_camera_folder_list_files (camera->camera, folder, l1, NULL);
	result = gp_camera_folder_delete_all (camera->camera, folder,
					GTKAM_STATUS (s)->context->context);
	switch (result) {
	case GP_OK:
		add.camera = camera;
		add.folder = folder;
		g_signal_emit (G_OBJECT (delete),
			signals[ALL_DELETED], 0, &add);
		gtk_object_destroy (GTK_OBJECT (s));
		return (TRUE);
	case GP_ERROR_CANCEL:
		gtk_object_destroy (GTK_OBJECT (s));
		return (TRUE);
	default:
		d = gtkam_error_new (result, GTKAM_STATUS (s)->context,
			GTK_WIDGET (delete), _("Could not delete all files "
			"in '%s'."), folder);
		gtk_widget_show (d);
		gtk_object_destroy (GTK_OBJECT (s));

		gp_list_new (&l2);
		/* See what files have been deleted */
		r2 = gp_camera_folder_list_files (camera->camera, folder,
						  l2, NULL);
		if ((r1 == GP_OK) && (r2 == GP_OK)) {
			for (r1 = 0; r1 < gp_list_count (l1); r1++) {
				gp_list_get_name (l1, r1, &name);
				if (gp_list_lookup_name (l2, name) >= 0) {
					fdd.camera = camera;
					fdd.folder = folder;
					fdd.name = name;
					g_signal_emit (GTK_OBJECT (delete),
						signals[FILE_DELETED], 0,
						&fdd);
				}
			}
		}
		gp_list_unref (l2);
		return (FALSE);
	}
	gp_list_unref (l1);
}

static gboolean
delete_one (GtkamDelete *delete, GtkamCamera *camera,
	    const gchar *folder, const gchar *name)
{
	GtkWidget *d, *s;
	int result;
	GtkamDeleteFileDeletedData fdd;

	s = gtkam_status_new (_("Deleting file '%s' from folder '%s'..."),
			      name, folder);
	gtk_widget_show (s);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (delete)->vbox), s,
			    FALSE, FALSE, 0);
	result = gp_camera_file_delete (camera->camera, folder, name,
				        GTKAM_STATUS (s)->context->context);
	switch (result) {
	case GP_OK:
		gtk_object_destroy (GTK_OBJECT (s));
		fdd.camera = camera;
		fdd.folder = folder;
		fdd.name = name;
		g_signal_emit (GTK_OBJECT (delete), signals[FILE_DELETED], 0,
			       &fdd);
		return (TRUE);
	case GP_ERROR_CANCEL:
		gtk_object_destroy (GTK_OBJECT (s));
		return (FALSE);
	default:
		d = gtkam_error_new (result, GTKAM_STATUS (s)->context,
			GTK_WIDGET (delete), _("Could not delete "
			"file '%s' in folder '%s'."), name, folder);
		gtk_widget_show (d);
		gtk_object_destroy (GTK_OBJECT (s));
		return (FALSE);
	}
}

static void
on_delete_clicked (GtkButton *button, GtkamDelete *delete)
{
	gboolean success = TRUE;
	gint i;
	GtkamDeleteData *data;

	if (delete->priv->deleteall) {
		data = g_slist_nth_data (delete->priv->data, 0);

		if (delete_all (delete, data->camera, data->folder)) {
/*			g_object_unref (data->camera);
			g_free (data->name);
			g_free (data->folder);
			delete->priv->data = g_slist_remove (
					delete->priv->data, data);
			g_free (data);*/
		} else
			success = FALSE;
	} else {
		for (i = g_slist_length (delete->priv->data) - 1; i >= 0; i--) {
			data = g_slist_nth_data (delete->priv->data, i);

			if (delete_one (delete, data->camera,
					data->folder, data->name)) {
/*				g_object_unref (data->camera);
				g_free (data->name);
				g_free (data->folder);
				delete->priv->data = g_slist_remove (
						delete->priv->data, data);
				g_free (data);*/
			} else
				success = FALSE;
		}
	}

	if (success) {
				g_object_unref (data->camera);
				g_free (data->name);
				g_free (data->folder);
				delete->priv->data = g_slist_remove (
						delete->priv->data, data);
				g_free (data);		
		gtk_object_destroy (GTK_OBJECT (delete));
	}
}

static void
on_cancel_clicked (GtkButton *button, GtkamDelete *delete)
{
	gtk_object_destroy (GTK_OBJECT (delete));
}

GtkWidget *
gtkam_delete_new (void)
{
	GtkamDelete *delete;
	GtkWidget *button, *hbox, *image;

	delete = g_object_new (GTKAM_TYPE_DELETE, NULL);

	gtk_dialog_set_has_separator (GTK_DIALOG (delete), FALSE);
	gtk_window_set_resizable (GTK_WINDOW (delete), FALSE);
	gtk_window_set_title (GTK_WINDOW (delete), "");
	gtk_container_set_border_width (GTK_CONTAINER (delete), 6);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (delete)->vbox), hbox, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
	gtk_box_set_spacing (GTK_BOX (hbox), 12);

	image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (hbox), image, TRUE, TRUE, 0);

	delete->priv->msg = gtk_label_new("");
	gtk_widget_show (delete->priv->msg);
	gtk_label_set_line_wrap (GTK_LABEL (delete->priv->msg), TRUE);
	gtk_label_set_use_markup (GTK_LABEL (delete->priv->msg), TRUE);
	gtk_label_set_selectable (GTK_LABEL (delete->priv->msg), TRUE);
	gtk_box_pack_start (GTK_BOX (hbox), delete->priv->msg, FALSE, FALSE, 0);

	button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
	gtk_widget_show (button);
	g_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_cancel_clicked), delete);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (delete)->action_area),
			   button);

	button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
	gtk_widget_show (button);
	g_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_delete_clicked), delete);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (delete)->action_area),
			   button);
	gtk_widget_grab_focus (button);

	return (GTK_WIDGET (delete));
}

void
gtkam_delete_add (GtkamDelete *delete, GtkamCamera *camera,
		  const gchar *folder, const gchar *name, gboolean all)
{
	GtkamDeleteData *data;
	gchar *msg;

	g_return_if_fail (GTKAM_IS_DELETE (delete));
	g_return_if_fail (GTKAM_IS_CAMERA (camera));
	g_return_if_fail (folder != NULL);

	data = g_new0 (GtkamDeleteData, 1);
	data->camera = camera;
	g_object_ref (G_OBJECT (camera));
	data->folder = g_strdup (folder);
	data->name = g_strdup (name);
	delete->priv->deleteall = all;
	delete->priv->data = g_slist_append (delete->priv->data, data);

	if (delete->priv->deleteall)
		msg = g_strdup_printf (_("Do you really want to "
				"delete all the files in '%s'?"), data->folder);
	else if (g_slist_length (delete->priv->data) == 1)
		msg = g_strdup_printf (_("Do you really want to "
				"delete '%s'?"), data->name);
	else
		msg = g_strdup_printf (_("Do you really want to "
				"delete the selected %i files?"),
				g_slist_length (delete->priv->data));
	gtk_label_set_markup (GTK_LABEL (delete->priv->msg),
				g_strdup_printf("%s%s%s%s%s",
		           				"<span weight=\"bold\" size=\"larger\">",
								_("Delete these files?"),
								"</span>\n\n", 
								msg,
								" Deleted files cannot be undeleted."));
	g_free (msg);
}
