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

#include "config.h"

#include <gphoto2/gphoto2-port-info-list.h>

#include "gtkam-port.h"
#include "i18n.h"

#include <string.h>

#include <gtk/gtkimage.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkentry.h>
#include <gtk/gtklabel.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtktable.h>
#include <gtk/gtkstock.h>

#include <gphoto2/gphoto2-result.h>

#include "gtkam-close.h"

struct _GtkamPortPrivate
{
	GtkWidget *entry_path;
};

#define PARENT_TYPE GTKAM_TYPE_DIALOG
static GtkamDialogClass *parent_class;

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
gtkam_port_finalize (GObject *object)
{
	GtkamPort *port = GTKAM_PORT (object);

	g_free (port->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_port_class_init (gpointer g_class, gpointer class_data)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = GTK_OBJECT_CLASS (g_class);
	object_class->destroy  = gtkam_port_destroy;

	gobject_class = G_OBJECT_CLASS (g_class);
	gobject_class->finalize = gtkam_port_finalize;

	signals[PORT_ADDED] = g_signal_new ("port_added",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkamPortClass, port_added), NULL, NULL,
		g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, 
		G_TYPE_POINTER);

	parent_class = g_type_class_peek_parent (g_class);
}

static void
gtkam_port_init (GTypeInstance *instance, gpointer g_class)
{
	GtkamPort *port = GTKAM_PORT (instance);

	port->priv = g_new0 (GtkamPortPrivate, 1);
}

GType
gtkam_port_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo ti;

		memset (&ti, 0, sizeof (GTypeInfo));
		ti.class_size    = sizeof (GtkamPortClass);
		ti.class_init    = gtkam_port_class_init;
		ti.instance_size = sizeof (GtkamPort);
		ti.instance_init = gtkam_port_init;

		type = g_type_register_static (PARENT_TYPE, "GtkamPort",
					       &ti, 0);
	}

	return (type);
}

static void
on_ok_clicked (GtkButton *button, GtkamPort *port)
{
	GPPortInfoList *list;
	int index;
	const gchar *path;
	gchar *msg;
	GtkWidget *d;

	gp_port_info_list_new (&list);
	gp_port_info_list_load (list);
	path = gtk_entry_get_text (GTK_ENTRY (port->priv->entry_path));
	index = gp_port_info_list_lookup_path (list, path);
	gp_port_info_list_free (list);
	if (index < 0) {
		msg = g_strdup_printf (_("Could not find an io-driver for "
			"port '%s' ('%s')."), path,
			gp_result_as_string (index));
		d = gtkam_close_new (msg);
		g_free (msg);
		gtk_window_set_transient_for (GTK_WINDOW (d),
					      GTK_WINDOW (port));
		gtk_widget_show (d);
	} else {
		g_signal_emit (GTK_OBJECT (port), signals[PORT_ADDED], 0, path);
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
	GtkWidget *label, *button, *entry, *table;

	port = g_object_new (GTKAM_TYPE_PORT, NULL);

	table = gtk_table_new (2, 2, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (GTKAM_DIALOG (port)->vbox), table,
			    TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (table), 10);
	gtk_table_set_col_spacings (GTK_TABLE (table), 5);

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

	button = gtk_button_new_from_stock (GTK_STOCK_OK);
	gtk_widget_show (button);
	g_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_ok_clicked), port);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (port)->action_area),
			   button);

	button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
	gtk_widget_show (button);
	g_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_cancel_clicked), port);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (port)->action_area),
			   button);

	if (opt_window)
		gtk_window_set_transient_for (GTK_WINDOW (port),
					      GTK_WINDOW (opt_window));

	return (GTK_WIDGET (port));
}
