/* gtkam-cancel.c
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
#include "gtkam-cancel.h"

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

#include <gtk/gtkbutton.h>
#include <gtk/gtkmain.h>
#include <gtk/gtklabel.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkprogressbar.h>
#include <gtk/gtkpixmap.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <gphoto2/gphoto2-result.h>

struct _GtkamCancelPrivate
{
	GPtrArray *array_hbox, *array_progress;
	GArray *array_target;

	gboolean cancelled;
};

#define PARENT_TYPE GTK_TYPE_DIALOG
static GtkDialogClass *parent_class;

enum {
	CANCEL,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void
gtkam_cancel_destroy (GtkObject *object)
{
	GtkamCancel *cancel = GTKAM_CANCEL (object);

	if (cancel->context) {
		gtk_object_unref (GTK_OBJECT (cancel->context));
		cancel->context = NULL;
	}

	g_ptr_array_set_size (cancel->priv->array_hbox, 0);
	g_ptr_array_set_size (cancel->priv->array_progress, 0);
	g_array_set_size (cancel->priv->array_target, 0);

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_cancel_finalize (GtkObject *object)
{
	GtkamCancel *cancel = GTKAM_CANCEL (object);

	g_array_free (cancel->priv->array_target, TRUE);
	g_ptr_array_free (cancel->priv->array_hbox, TRUE);
	g_ptr_array_free (cancel->priv->array_progress, TRUE);
	g_free (cancel->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_cancel_class_init (GtkamCancelClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_cancel_destroy;
	object_class->finalize = gtkam_cancel_finalize;

	signals[CANCEL] = gtk_signal_new ("cancel",
		GTK_RUN_FIRST, object_class->type,
		GTK_SIGNAL_OFFSET (GtkamCancelClass, cancel),
		gtk_marshal_NONE__NONE, GTK_TYPE_NONE, 0);
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_cancel_init (GtkamCancel *cancel)
{
	cancel->context = gtkam_context_new ();

	cancel->priv = g_new0 (GtkamCancelPrivate, 1);
	cancel->priv->array_hbox = g_ptr_array_new ();
	cancel->priv->array_progress = g_ptr_array_new ();
	cancel->priv->array_target = g_array_new (FALSE, TRUE, sizeof (gfloat));
}

GtkType
gtkam_cancel_get_type (void)
{
	static GtkType cancel_type = 0;

	if (!cancel_type) {
		static const GtkTypeInfo cancel_info = {
			"GtkamCancel",
			sizeof (GtkamCancel),
			sizeof (GtkamCancelClass),
			(GtkClassInitFunc)  gtkam_cancel_class_init,
			(GtkObjectInitFunc) gtkam_cancel_init,
			NULL, NULL, NULL};
		cancel_type = gtk_type_unique (PARENT_TYPE, &cancel_info);
	}

	return (cancel_type);
}

static void
on_cancel_clicked (GtkButton *button, GtkamCancel *cancel)
{
	cancel->priv->cancelled = TRUE;
	gtk_signal_emit (GTK_OBJECT (cancel), signals[CANCEL]);
}

static GPContextFeedback
cancel_func (GPContext *c, void *data)
{
	GtkamCancel *cancel = GTKAM_CANCEL (data);

	while (gtk_events_pending ())
		gtk_main_iteration ();

	return (cancel->priv->cancelled ? GP_CONTEXT_FEEDBACK_CANCEL :
					  GP_CONTEXT_FEEDBACK_OK);
}

static unsigned int
start_func (GPContext *c, float target, const char *format,
	    va_list args, void *data)
{
	GtkamCancel *cancel = GTKAM_CANCEL (data);
	GtkWidget *progress, *label, *hbox;
	gchar *msg;

	hbox = gtk_hbox_new (FALSE, 5);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cancel)->vbox), hbox,
			    TRUE, TRUE, 0);
	g_ptr_array_add (cancel->priv->array_hbox, hbox);

	msg = g_strdup_vprintf (format, args);
	label = gtk_label_new (msg);
	g_free (msg);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	progress = gtk_progress_bar_new ();
	gtk_widget_show (progress);
	gtk_box_pack_start (GTK_BOX (hbox), progress, TRUE, TRUE, 0);
	g_ptr_array_add (cancel->priv->array_progress, progress);
	g_array_append_val (cancel->priv->array_target, target);

	while (gtk_events_pending ())
		gtk_main_iteration ();

	return (cancel->priv->array_progress->len - 1);
}

static void
update_func (GPContext *c, unsigned int id, float current, void *data)
{
	GtkamCancel *cancel = GTKAM_CANCEL (data);
	GtkProgress *progress;

	g_return_if_fail (id < cancel->priv->array_progress->len);

	progress = cancel->priv->array_progress->pdata[id];
	gtk_progress_set_percentage (progress,
		current / g_array_index (cancel->priv->array_target,
					 gfloat, id));

	while (gtk_events_pending ())
		gtk_main_iteration ();
}

static void
stop_func (GPContext *c, unsigned int id, void *data)
{
	GtkamCancel *cancel = GTKAM_CANCEL (data);
	GtkObject *hbox;

	g_return_if_fail (id < cancel->priv->array_progress->len);

	hbox = cancel->priv->array_hbox->pdata[id];
	g_ptr_array_remove_index (cancel->priv->array_hbox, id);
	g_ptr_array_remove_index (cancel->priv->array_progress, id);
	g_array_remove_index (cancel->priv->array_target, id);

	gtk_object_destroy (GTK_OBJECT (hbox));
}

GtkWidget *
gtkam_cancel_new (GtkWidget *opt_window, const gchar *format, ...)
{
	GtkamCancel *cancel;
	GtkWidget *label, *button, *image, *hbox;
	GdkPixmap *pixmap;
	GdkBitmap *bitmap;
	GdkPixbuf *pixbuf;
	va_list args;
	gchar *msg;

	cancel = gtk_type_new (GTKAM_TYPE_CANCEL);

	gtk_container_set_border_width (
			GTK_CONTAINER (GTK_DIALOG (cancel)->vbox), 5);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (cancel)->vbox), 5);

	hbox = gtk_hbox_new (FALSE, 5);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cancel)->vbox), hbox,
			    TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);

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

	va_start (args, format);
	msg = g_strdup_vprintf (format, args);
	va_end (args);
	label = gtk_label_new (msg);
	g_free (msg);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	button = gtk_button_new_with_label (_("Cancel"));
	gtk_widget_show (button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_cancel_clicked), cancel);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (cancel)->action_area),
			   button);
	gtk_widget_grab_focus (button);

	if (opt_window)
		gtk_window_set_transient_for (GTK_WINDOW (cancel),
					      GTK_WINDOW (opt_window));

	gp_context_set_progress_funcs (cancel->context->context, start_func,
				       update_func, stop_func, cancel);
	gp_context_set_cancel_func (cancel->context->context,
				    cancel_func, cancel);

	return (GTK_WIDGET (cancel));
}
