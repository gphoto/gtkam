/* gtkam-port.c
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

#include <gphoto2/gphoto2-port-info-list.h>

#include "gtkam-port.h"

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
#include <gtk/gtkentry.h>
#include <gtk/gtklabel.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtktable.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <gphoto2/gphoto2-result.h>

#include "gtkam-close.h"

struct _GtkamPortPrivate
{
	GtkWidget *entry_path;
};

#define PARENT_TYPE GTK_TYPE_DIALOG
static GtkDialogClass *parent_class;

enum {
	PORT_ADDED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void
gtkam_port_destroy (GtkObject *object)
{
	GtkamPort *port = GTKAM_PORT (object);

	port = NULL;

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_port_finalize (GtkObject *object)
{
	GtkamPort *port = GTKAM_PORT (object);

	g_free (port->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_port_class_init (GtkamPortClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_port_destroy;
	object_class->finalize = gtkam_port_finalize;

	signals[PORT_ADDED] = gtk_signal_new ("port_added", GTK_RUN_FIRST,
		object_class->type,
		GTK_SIGNAL_OFFSET (GtkamPortClass, port_added),
		gtk_marshal_NONE__POINTER, GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_port_init (GtkamPort *port)
{
	port->priv = g_new0 (GtkamPortPrivate, 1);
}

GtkType
gtkam_port_get_type (void)
{
	static GtkType port_type = 0;

	if (!port_type) {
		static const GtkTypeInfo port_info = {
			"GtkamPort",
			sizeof (GtkamPort),
			sizeof (GtkamPortClass),
			(GtkClassInitFunc)  gtkam_port_class_init,
			(GtkObjectInitFunc) gtkam_port_init,
			NULL, NULL, NULL};
		port_type = gtk_type_unique (PARENT_TYPE, &port_info);
	}

	return (port_type);
}

static void
on_ok_clicked (GtkButton *button, GtkamPort *port)
{
	GPPortInfoList *list;
	int index;
	const gchar *path;
	gchar *msg;
	GtkWidget *dialog;

	gp_port_info_list_new (&list);
	gp_port_info_list_load (list);
	path = gtk_entry_get_text (GTK_ENTRY (port->priv->entry_path));
	index = gp_port_info_list_lookup_path (list, path);
	gp_port_info_list_free (list);
	if (index < 0) {
		msg = g_strdup_printf (_("Could not find and io-driver for "
			"port '%s' ('%s')."), path,
			gp_result_as_string (index));
		dialog = gtkam_close_new (msg, GTK_WIDGET (port));
		g_free (msg);
		gtk_widget_show (dialog);
	} else {
		gtk_signal_emit (GTK_OBJECT (port), signals[PORT_ADDED],
				 path);
		gtk_object_destroy (GTK_OBJECT (port));
	}
}

static void
on_cancel_clicked (GtkButton *button, GtkamPort *port)
{
	gtk_object_destroy (GTK_OBJECT (port));
}

GtkWidget *
gtkam_port_new (GtkWidget *opt_window)
{
	GtkamPort *port;
	GtkWidget *label, *button, *entry, *image, *table;
	GdkPixmap *pixmap;
	GdkBitmap *bitmap;
	GdkPixbuf *pixbuf;

	port = gtk_type_new (GTKAM_TYPE_PORT);
	gtk_signal_connect (GTK_OBJECT (port), "delete_event",
			    GTK_SIGNAL_FUNC (gtk_object_destroy), NULL);

	table = gtk_table_new (2, 2, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (port)->vbox), table,
			    TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (table), 10);
	gtk_table_set_col_spacings (GTK_TABLE (table), 5);

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
		gtk_table_attach_defaults (GTK_TABLE (table), image,
					   0, 1, 1, 2);
	}

	label = gtk_label_new (_("Please specify the (extended) path to "
		"the port you would like to use:"));
	gtk_widget_show (label);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 0, 1);

	entry = gtk_entry_new ();
	gtk_widget_show (entry);
	gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, 1, 2);
	port->priv->entry_path = entry;

	button = gtk_button_new_with_label (_("Ok"));
	gtk_widget_show (button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_ok_clicked), port);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (port)->action_area),
			   button);

	button = gtk_button_new_with_label (_("Cancel"));
	gtk_widget_show (button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_cancel_clicked), port);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (port)->action_area),
			   button);

	if (opt_window)
		gtk_window_set_transient_for (GTK_WINDOW (port),
					      GTK_WINDOW (opt_window));

	return (GTK_WIDGET (port));
}
