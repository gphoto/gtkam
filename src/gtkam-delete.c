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
#include <gtk/gtkscrolledwindow.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gtkam-error.h"
#include "gtkam-status.h"

typedef struct _GtkamDeleteData GtkamDeleteData;
struct _GtkamDeleteData {
	Camera *camera;
	gboolean multi;
	gchar *folder;
	gchar *name;
	GtkWidget *check;
};

struct _GtkamDeletePrivate
{
	Camera *camera;
	gboolean multi;

	GSList *data;
	GtkWidget *vbox, *status;

	GtkWidget *msg;
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
			gp_camera_unref (data->camera);
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
gtkam_marshal_VOID__POINTER_BOOL_POINTER_POINTER (GClosure *closure,
		GValue *return_value, guint n_param_values,
		const GValue *param_values,
		gpointer invocation_hint, gpointer marshal_data)
{
	typedef void (*GMarshalFunc_VOID__POINTER_BOOL_POINTER_POINTER) (
			gpointer, gpointer, gboolean, gpointer, gpointer,
			gpointer);
	register GMarshalFunc_VOID__POINTER_BOOL_POINTER_POINTER callback;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;

	g_return_if_fail (n_param_values == 4);

	if (G_CCLOSURE_SWAP_DATA (closure)) {
		data1 = closure->data;
		data2 = g_value_peek_pointer (param_values + 0);
	} else {
		data1 = g_value_peek_pointer (param_values + 0);
		data2 = closure->data;
	}
	callback = (GMarshalFunc_VOID__POINTER_BOOL_POINTER_POINTER) (
			marshal_data ? marshal_data : cc->callback);
	callback (data1, g_value_get_pointer (param_values + 1),
			 g_value_get_boolean (param_values + 2),
			 g_value_get_pointer (param_values + 3),
			 g_value_get_pointer (param_values + 4), data2);
}

static void
gtkam_marshal_VOID__POINTER_BOOL_POINTER (GClosure *closure,
		GValue *return_value, guint n_param_values,
		const GValue *param_values, 
		gpointer invocation_hint, gpointer marshal_data)
{
	typedef void (*GMarshalFunc_VOID__POINTER_BOOL_POINTER) (
			gpointer, gpointer, gboolean, gpointer, gpointer);
	register GMarshalFunc_VOID__POINTER_BOOL_POINTER callback;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;

	g_return_if_fail (n_param_values == 3);

	if (G_CCLOSURE_SWAP_DATA (closure)) {
		data1 = closure->data;
		data2 = g_value_peek_pointer (param_values + 0);
	} else {
		data1 = g_value_peek_pointer (param_values + 0);
		data2 = closure->data;
	}
	callback = (GMarshalFunc_VOID__POINTER_BOOL_POINTER) (
			marshal_data ? marshal_data : cc->callback);
	callback (data1, g_value_get_pointer (param_values + 1),
			 g_value_get_boolean (param_values + 2),
			 g_value_get_pointer (param_values + 3), data2);
}


static void
gtkam_delete_class_init (GObjectClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_delete_destroy;

	klass->finalize = gtkam_delete_finalize;

	signals[FILE_DELETED] = g_signal_new ("file_deleted",
		G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkamDeleteClass, file_deleted), NULL, NULL,
		gtkam_marshal_VOID__POINTER_BOOL_POINTER_POINTER,
		G_TYPE_NONE, 4, G_TYPE_POINTER, G_TYPE_BOOLEAN,
		G_TYPE_POINTER, G_TYPE_POINTER);
	signals[ALL_DELETED] = g_signal_new ("all_deleted",
		G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkamDeleteClass, all_deleted), NULL, NULL,
		gtkam_marshal_VOID__POINTER_BOOL_POINTER,
		G_TYPE_NONE, 3, G_TYPE_POINTER, G_TYPE_BOOLEAN,
		G_TYPE_POINTER);

	parent_class = g_type_class_peek_parent (klass);
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
delete_all (GtkamDelete *delete, Camera *camera, gboolean multi,
	    const gchar *folder)
{
	GtkWidget *d, *s;
	int result, r1, r2;
	CameraList l1, l2;
	const char *name;

	s = gtkam_status_new (_("Deleting all files in '%s'..."), folder);
	gtk_widget_show (s);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (delete)->vbox), s,
			    FALSE, FALSE, 0);
	r1 = gp_camera_folder_list_files (camera, folder, &l1, NULL);
	result = gp_camera_folder_delete_all (camera, folder,
					GTKAM_STATUS (s)->context->context);
	switch (result) {
	case GP_OK:
		gtk_signal_emit (GTK_OBJECT (delete),
			signals[ALL_DELETED], camera, multi, folder);
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

		/* See what files have been deleted */
		r2 = gp_camera_folder_list_files (camera, folder, &l2, NULL);
		if ((r1 == GP_OK) && (r2 == GP_OK)) {
			for (r1 = 0; r1 < gp_list_count (&l1); r1++) {
				gp_list_get_name (&l1, r1, &name);
				if (gp_list_lookup_name (&l2, name) >= 0)
					gtk_signal_emit (GTK_OBJECT (delete),
						signals[FILE_DELETED], camera,
						multi, folder, name);
			}
		}
		return (FALSE);
	}
}

static gboolean
delete_one (GtkamDelete *delete, Camera *camera, gboolean multi,
	    const gchar *folder, const gchar *name)
{
	GtkWidget *d, *s;
	int result;

	s = gtkam_status_new (_("Deleting file '%s' from folder '%s'..."),
			      name, folder);
	gtk_widget_show (s);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (delete)->vbox), s,
			    FALSE, FALSE, 0);
	result = gp_camera_file_delete (camera, folder, name,
				        GTKAM_STATUS (s)->context->context);
	switch (result) {
	case GP_OK:
		gtk_object_destroy (GTK_OBJECT (s));
		gtk_signal_emit (GTK_OBJECT (delete), signals[FILE_DELETED],
				 camera, multi, folder, name);
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

	for (i = g_slist_length (delete->priv->data) - 1; i >= 0; i--) {
		data = g_slist_nth_data (delete->priv->data, i);
		if (!GTK_TOGGLE_BUTTON (data->check)->active)
			continue;
		if (data->name) {
			if (delete_one (delete, data->camera, data->multi,
					data->folder, data->name)) {
				gp_camera_unref (data->camera);
				g_free (data->name);
				g_free (data->folder);
				delete->priv->data = g_slist_remove (
					delete->priv->data, data);
				gtk_container_remove (
					GTK_CONTAINER (delete->priv->vbox),
					GTK_WIDGET (data->check));
				g_free (data);
			} else
				success = FALSE;
		} else if (!delete_all (delete, data->camera, data->multi,
					data->folder))
			success = FALSE;
	}

	if (success)
		gtk_object_destroy (GTK_OBJECT (delete));
}

static void
on_cancel_clicked (GtkButton *button, GtkamDelete *delete)
{
	gtk_object_destroy (GTK_OBJECT (delete));
}

GtkWidget *
gtkam_delete_new (GtkWidget *status)
{
	GtkamDelete *delete;
	GtkWidget *button, *image, *hbox, *vbox, *scrolled;
	GdkPixmap *pixmap;
	GdkBitmap *bitmap;
	GdkPixbuf *pixbuf;
	GError *e;

	g_return_val_if_fail (status != NULL, NULL);

	delete = gtk_type_new (GTKAM_TYPE_DELETE);
	gtk_signal_connect (GTK_OBJECT (delete), "delete_event",
			    GTK_SIGNAL_FUNC (gtk_object_destroy), NULL);
	delete->priv->status = status;

	hbox = gtk_hbox_new (FALSE, 10);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (delete)->vbox), hbox,
			    TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);

	pixbuf = gdk_pixbuf_new_from_file (IMAGE_DIR "/gtkam-camera.png", &e);
	if (!pixbuf) {
		g_assert (e != NULL);
		g_warning ("Could not load " IMAGE_DIR "/gtkam-camera.png: "
			   "'%s'.", e->message);
		g_error_free (e);
	} else {
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

	/* VBox for message and scrolled window */
	vbox = gtk_vbox_new (FALSE, 5);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

	/* Message */
	delete->priv->msg = gtk_label_new ("");
	gtk_widget_show (delete->priv->msg);
	gtk_label_set_justify (GTK_LABEL (delete->priv->msg), GTK_JUSTIFY_LEFT);
	gtk_label_set_line_wrap (GTK_LABEL (delete->priv->msg), TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), delete->priv->msg, FALSE, FALSE, 0);

	/* Scrolled window */
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolled);
	gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (
				GTK_SCROLLED_WINDOW (scrolled),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	delete->priv->vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (delete->priv->vbox);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled),
					       delete->priv->vbox);

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

	return (GTK_WIDGET (delete));
}

void
gtkam_delete_add (GtkamDelete *delete, Camera *camera, gboolean multi,
		  const gchar *folder, const gchar *name)
{
	GtkamDeleteData *data;
	gchar *label, *msg;

	data = g_new0 (GtkamDeleteData, 1);
	data->camera = camera;
	gp_camera_ref (camera);
	data->multi = multi;
	data->folder = g_strdup (folder);
	data->name = g_strdup (name);
	delete->priv->data = g_slist_append (delete->priv->data, data);

	if (g_slist_length (delete->priv->data) == 1)
		msg = g_strdup_printf (_("Do you really want to "
					 "delete the following file?"));
	else
		msg = g_strdup_printf (_("Do you really want to "
					 "delete the following %i files?"),
					g_slist_length (delete->priv->data));
	gtk_label_set_text (GTK_LABEL (delete->priv->msg), msg);
	g_free (msg);

	if (name)
		label = g_strdup_printf (_("'%s' in folder '%s'"), name,
					 folder);
	else
		label = g_strdup_printf (_("All files in folder '%s'"),
					 folder);
	data->check = gtk_check_button_new_with_label (label);
	g_free (label);
	gtk_widget_show (data->check);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->check), TRUE);
	gtk_box_pack_start (GTK_BOX (delete->priv->vbox), data->check,
			    FALSE, FALSE, 0);
}
