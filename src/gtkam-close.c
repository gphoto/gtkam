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
#include <gtk/gtkimage.h>

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
gtkam_close_finalize (GObject *object)
{
	GtkamClose *close = GTKAM_CLOSE (object);

	g_free (close->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_close_class_init (gpointer g_class, gpointer class_data)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = GTK_OBJECT_CLASS (g_class);
	object_class->destroy  = gtkam_close_destroy;

	gobject_class = G_OBJECT_CLASS (g_class);
	gobject_class->finalize = gtkam_close_finalize;

	parent_class = g_type_class_peek_parent (g_class);
}

static void
gtkam_close_init (GTypeInstance *instance, gpointer g_class)
{
	GtkamClose *close = GTKAM_CLOSE (instance);

	close->priv = g_new0 (GtkamClosePrivate, 1);
}

GType
gtkam_close_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo ti;

		memset (&ti, 0, sizeof (GTypeInfo));
		ti.class_size     = sizeof (GtkamCloseClass);
		ti.class_init     = gtkam_close_class_init;
		ti.instance_size  = sizeof (GtkamClose);
		ti.instance_init  = gtkam_close_init;

		type = g_type_register_static (PARENT_TYPE, "GtkamClose",
					       &ti, 0);
	}

	return (type);
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

	g_return_val_if_fail (msg != NULL, NULL);

	close = g_object_new (GTKAM_TYPE_CLOSE, NULL);

	hbox = gtk_hbox_new (FALSE, 10);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (close)->vbox), hbox,
			    TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);

	image = gtk_image_new_from_file (IMAGE_DIR "/gtkam-camera.png");
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

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
	g_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_close_close_clicked), close);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (close)->action_area),
			   button);
	gtk_widget_grab_focus (button);

	if (opt_window)
		gtk_window_set_transient_for (GTK_WINDOW (close),
					      GTK_WINDOW (opt_window));

	return (GTK_WIDGET (close));
}
