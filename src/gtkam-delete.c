/* gtkam-delete.c
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
#include "gtkam-delete.h"

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

#include <string.h>

#include <gtk/gtkcheckbutton.h>
#include <gtk/gtklabel.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkpixmap.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gtkam-error.h"

struct _GtkamDeletePrivate
{
	Camera *camera;
	gboolean multi;
	gchar *path;

	GList *toggles;
	GtkWidget *vbox;
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

	if (delete->priv->camera) {
		gp_camera_unref (delete->priv->camera);
		delete->priv->camera = NULL;
	}

	if (delete->priv->path) {
		g_free (delete->priv->path);
		delete->priv->path = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_delete_finalize (GtkObject *object)
{
	GtkamDelete *delete = GTKAM_DELETE (object);

	g_free (delete->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_delete_class_init (GtkamDeleteClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_delete_destroy;
	object_class->finalize = gtkam_delete_finalize;

	signals[FILE_DELETED] = gtk_signal_new ("file_deleted",
		GTK_RUN_FIRST, object_class->type,
		GTK_SIGNAL_OFFSET (GtkamDeleteClass, file_deleted),
		gtk_marshal_NONE__POINTER_POINTER, GTK_TYPE_NONE, 2,
		GTK_TYPE_POINTER, GTK_TYPE_POINTER);
	signals[ALL_DELETED] = gtk_signal_new ("all_deleted",
		GTK_RUN_FIRST, object_class->type,
		GTK_SIGNAL_OFFSET (GtkamDeleteClass, all_deleted),
		gtk_marshal_NONE__POINTER_POINTER, GTK_TYPE_NONE, 2,
		GTK_TYPE_POINTER, GTK_TYPE_POINTER);
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_delete_init (GtkamDelete *delete)
{
	delete->priv = g_new0 (GtkamDeletePrivate, 1);
}

GtkType
gtkam_delete_get_type (void)
{
	static GtkType delete_type = 0;

	if (!delete_type) {
		static const GtkTypeInfo delete_info = {
			"GtkamDelete",
			sizeof (GtkamDelete),
			sizeof (GtkamDeleteClass),
			(GtkClassInitFunc)  gtkam_delete_class_init,
			(GtkObjectInitFunc) gtkam_delete_init,
			NULL, NULL, NULL};
		delete_type = gtk_type_unique (PARENT_TYPE, &delete_info);
	}

	return (delete_type);
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
delete_all (GtkamDelete *delete)
{
	GtkWidget *d;
	gchar *msg, *path;
	int result, r1, r2;
	CameraList l1, l2;
	const char *name;

	r1 = gp_camera_folder_list_files (delete->priv->camera,
					  delete->priv->path, &l1);
	result = gp_camera_folder_delete_all (delete->priv->camera,
					      delete->priv->path);
	if (result < 0) {
		msg = g_strdup_printf (_("Could not delete all files "
				       "in '%s'"), delete->priv->path);
		d = gtkam_error_new (msg, result, delete->priv->camera,
				     GTK_WIDGET (delete));
		g_free (msg);
		gtk_widget_show (d);

		/* See what files have been deleted */
		r2 = gp_camera_folder_list_files (delete->priv->camera,
						  delete->priv->path, &l2);
		if ((r1 == GP_OK) && (r2 == GP_OK)) {
			for (r1 = 0; r1 < gp_list_count (&l1); r1++) {
				gp_list_get_name (&l1, r1, &name);
				if (gp_list_lookup_name (&l2, name) >= 0) {
					if (strlen (delete->priv->path) == 1)
						path = g_strdup_printf ("/%s",
									name);
					else
						path = g_strdup_printf ("%s/%s",
							delete->priv->path,
							name);
					gtk_signal_emit (GTK_OBJECT (delete),
						signals[FILE_DELETED], path);
					g_free (path);
				}
			}
		}
		return (FALSE);
	} else {
		gtk_signal_emit (GTK_OBJECT (delete),
				 signals[ALL_DELETED], delete->priv->path);
		return (TRUE);
	}
}

static gboolean
delete_one (GtkamDelete *delete, const gchar *file)
{
	GtkWidget *d;
	gchar *path, *msg;
	int result;

	result = gp_camera_file_delete (delete->priv->camera,
					delete->priv->path, file);
	if (result < 0) {
		msg = g_strdup_printf (_("Could not delete "
			"'%s' in '%s'"), file, delete->priv->path);
		d = gtkam_error_new (msg, result, delete->priv->camera,
				     GTK_WIDGET (delete));
		g_free (msg);
		gtk_widget_show (d);
		return (FALSE);
	} else {
		if (strlen (delete->priv->path) == 1)
			path = g_strdup_printf ("/%s", file);
		else
			path = g_strdup_printf ("%s/%s", delete->priv->path,
						file);
		gtk_signal_emit (GTK_OBJECT (delete), signals[FILE_DELETED],
				 path);
		g_free (path);
		return (TRUE);
	}
}

static void
on_delete_clicked (GtkButton *button, GtkamDelete *delete)
{
	GList *t;
	const gchar *file;
	gboolean success = TRUE;

	if (delete->priv->toggles) {
		for (t = delete->priv->toggles; t; t = t->next) {
			if (!GTK_TOGGLE_BUTTON (t->data)->active)
				continue;
			file = GTK_LABEL (GTK_BIN (t->data)->child)->label;
			if (delete_one (delete, file))
				gtk_container_remove (
					GTK_CONTAINER (delete->priv->vbox),
					GTK_WIDGET (t->data));
			else
				success = FALSE;
		}
	} else if (!delete_all (delete))
		success = FALSE;

	if (delete->priv->multi)
		gp_camera_exit (delete->priv->camera);

	if (success)
		gtk_object_destroy (GTK_OBJECT (delete));
}

static void
on_cancel_clicked (GtkButton *button, GtkamDelete *delete)
{
	gtk_object_destroy (GTK_OBJECT (delete));
}

GtkWidget *
gtkam_delete_new (Camera *camera, gboolean multi, const gchar *path,
		  GList *files, GtkWidget *opt_window)
{
	GtkamDelete *delete;
	GtkWidget *label, *button, *image, *hbox, *vbox, *check;
	GdkPixmap *pixmap;
	GdkBitmap *bitmap;
	GdkPixbuf *pixbuf;
	guint i;
	gchar *msg;

	g_return_val_if_fail (camera != NULL, NULL);
	g_return_val_if_fail (path != NULL, NULL);

	delete = gtk_type_new (GTKAM_TYPE_DELETE);
	gtk_signal_connect (GTK_OBJECT (delete), "delete_event",
			    GTK_SIGNAL_FUNC (gtk_object_destroy), NULL);
	delete->priv->camera = camera;
	gp_camera_ref (camera);
	delete->priv->path = g_strdup (path);
	delete->priv->multi = multi;

	hbox = gtk_hbox_new (FALSE, 10);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (delete)->vbox), hbox,
			    TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);

	pixbuf = gdk_pixbuf_new_from_file (IMAGE_DIR "/gtkam-camera.png");
	if (!pixbuf)
		g_warning ("Could not load " IMAGE_DIR "/gtkam-camera.png");
	else {
		gdk_pixbuf_render_pixmap_and_mask (pixbuf, &pixmap, &bitmap, 127);
		gdk_pixbuf_unref (pixbuf);
		image = gtk_pixmap_new (pixmap, bitmap);
		if (pixmap)
			gdk_pixmap_unref (pixmap);
		if (bitmap)
			gdk_bitmap_unref (bitmap);
		gtk_widget_show (image);
		gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	}

	if (files) {
		vbox = gtk_vbox_new (FALSE, 5);
		gtk_widget_show (vbox);
		gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
		delete->priv->vbox = vbox;
		
		if (g_list_length (files) == 1)
			msg = g_strdup_printf (_("Do you really want to "
				"delete the following file?"));
		else
			msg = g_strdup_printf (_("Do you really want to "
				"delete the following %i files?"),
				g_list_length (files));
		label = gtk_label_new (msg);
		g_free (msg);
		gtk_widget_show (label);
		gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
		gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
		gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

		for (i = 0; i < g_list_length (files); i++) {
			check = gtk_check_button_new_with_label (
						g_list_nth_data (files, i));
			gtk_widget_show (check);
			gtk_toggle_button_set_active (
					GTK_TOGGLE_BUTTON (check), TRUE);
			gtk_box_pack_start (GTK_BOX (vbox), check,
					    FALSE, FALSE, 0);
			delete->priv->toggles = g_list_append (
						delete->priv->toggles, check);
		}
	} else {
		msg = g_strdup_printf (_("Do you really want to delete ALL "
			"files in '%s'?"), path);
		label = gtk_label_new (msg);
		g_free (msg);
		gtk_widget_show (label);
		gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
		gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
		gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	}

	button = gtk_button_new_with_label (_("Delete"));
	gtk_widget_show (button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_delete_clicked), delete);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (delete)->action_area),
			   button);

	button = gtk_button_new_with_label (_("Cancel"));
	gtk_widget_show (button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_cancel_clicked), delete);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (delete)->action_area),
			   button);
	gtk_widget_grab_focus (button);

	if (opt_window)
		gtk_window_set_transient_for (GTK_WINDOW (delete),
					      GTK_WINDOW (opt_window));

	return (GTK_WIDGET (delete));
}
