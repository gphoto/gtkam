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
	GtkProgress *progress;
	GtkLabel *label;

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

	cancel = NULL;

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_cancel_finalize (GtkObject *object)
{
	GtkamCancel *cancel = GTKAM_CANCEL (object);

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
	cancel->priv = g_new0 (GtkamCancelPrivate, 1);
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

GtkWidget *
gtkam_cancel_new (GtkWidget *opt_window)
{
	GtkamCancel *cancel;
	GtkWidget *label, *button, *progress, *image, *hbox;
	GdkPixmap *pixmap;
	GdkBitmap *bitmap;
	GdkPixbuf *pixbuf;

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

	label = gtk_label_new ("");
	gtk_widget_show (label);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	cancel->priv->label = GTK_LABEL (label);

	progress = gtk_progress_bar_new ();
	gtk_widget_show (progress);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cancel)->vbox), progress,
			    FALSE, FALSE, 0);
	cancel->priv->progress = GTK_PROGRESS (progress);

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

	return (GTK_WIDGET (cancel));
}

void
gtkam_cancel_set_percentage (GtkamCancel *cancel, gfloat percentage)
{
	g_return_if_fail (GTKAM_IS_CANCEL (cancel));

	gtk_progress_set_percentage (cancel->priv->progress, percentage);
}

void
gtkam_cancel_set_message (GtkamCancel *cancel, const gchar *msg)
{
	g_return_if_fail (GTKAM_IS_CANCEL (cancel));

	gtk_label_set_text (cancel->priv->label, msg);
}

#if 0
static int
progress_func (CameraFile *file, float percentage, void *data)
{
	GtkamCancel *cancel;

	while (gtk_events_pending ())
		gtk_main_iteration ();

	if (!GTKAM_IS_CANCEL (data))
		return (GP_ERROR_CANCEL);
	cancel = GTKAM_CANCEL (data);

	gtkam_cancel_set_percentage (cancel, percentage);

	if (cancel->priv->cancelled) {
		cancel->priv->cancelled = FALSE;
		return (GP_ERROR_CANCEL);
	}

	return (GP_OK);
}
#endif

void
gtkam_cancel_start_monitoring (GtkamCancel *cancel, CameraFile *file)
{
	g_return_if_fail (GTKAM_IS_CANCEL (cancel));
	g_return_if_fail (file != NULL);
}

void
gtkam_cancel_stop_monitoring (GtkamCancel *cancel, CameraFile *file)
{
	g_return_if_fail (GTKAM_IS_CANCEL (cancel));
	g_return_if_fail (file != NULL);
}
