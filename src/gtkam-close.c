/* gtkam-close.c
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
#include "gtkam-close.h"

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
#include <gtk/gtklabel.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkpixmap.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

struct _GtkamClosePrivate
{
};

#define PARENT_TYPE GTK_TYPE_DIALOG
static GtkDialogClass *parent_class;

static void
gtkam_close_destroy (GtkObject *object)
{
	GtkamClose *close = GTKAM_CLOSE (object);

	close = NULL;

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_close_finalize (GtkObject *object)
{
	GtkamClose *close = GTKAM_CLOSE (object);

	g_free (close->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_close_class_init (GtkamCloseClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_close_destroy;
	object_class->finalize = gtkam_close_finalize;

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_close_init (GtkamClose *close)
{
	close->priv = g_new0 (GtkamClosePrivate, 1);
}

GtkType
gtkam_close_get_type (void)
{
	static GtkType close_type = 0;

	if (!close_type) {
		static const GtkTypeInfo close_info = {
			"GtkamClose",
			sizeof (GtkamClose),
			sizeof (GtkamCloseClass),
			(GtkClassInitFunc)  gtkam_close_class_init,
			(GtkObjectInitFunc) gtkam_close_init,
			NULL, NULL, NULL};
		close_type = gtk_type_unique (PARENT_TYPE, &close_info);
	}

	return (close_type);
}

static void
on_close_close_clicked (GtkButton *button, GtkamClose *close)
{
	gtk_object_destroy (GTK_OBJECT (close));
}

GtkWidget *
gtkam_close_new (const gchar *msg, GtkWidget *opt_window)
{
	GtkamClose *close;
	GtkWidget *label, *button, *scrolled, *image, *hbox;
	GdkPixmap *pixmap;
	GdkBitmap *bitmap;
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (msg != NULL, NULL);

	close = gtk_type_new (GTKAM_TYPE_CLOSE);
	gtk_signal_connect (GTK_OBJECT (close), "delete_event",
			    GTK_SIGNAL_FUNC (gtk_object_destroy), NULL);

	hbox = gtk_hbox_new (FALSE, 10);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (close)->vbox), hbox,
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

	label = gtk_label_new (msg);
	gtk_widget_show (label);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

	if (strlen (msg) > 1024) {
		scrolled = gtk_scrolled_window_new (NULL, NULL);
		gtk_widget_show (scrolled);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		gtk_box_pack_start (GTK_BOX (hbox),
				    scrolled, TRUE, TRUE, 0);
		gtk_scrolled_window_add_with_viewport (
				GTK_SCROLLED_WINDOW (scrolled), label);
	} else
		gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

	button = gtk_button_new_with_label (_("Close"));
	gtk_widget_show (button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_close_close_clicked), close);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (close)->action_area),
			   button);
	gtk_widget_grab_focus (button);

	if (opt_window)
		gtk_window_set_transient_for (GTK_WINDOW (close),
					      GTK_WINDOW (opt_window));

	return (GTK_WIDGET (close));
}
