/* gtkam-error.c
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
 */

#include "config.h"
#include "gtkam-error.h"
#include "i18n.h"

#include <stdio.h>
#include <string.h>

#include <gtk/gtkimage.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkstock.h>

#include <gphoto2/gphoto2-result.h>
#include <gphoto2/gphoto2-port-log.h>

struct _GtkamErrorPrivate
{
	GtkWidget *hbox;
};

#define PARENT_TYPE GTK_TYPE_DIALOG
static GtkDialogClass *parent_class;

static void
gtkam_error_destroy (GtkObject *object)
{
	GtkamError *error = GTKAM_ERROR (object);

	error = NULL;

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_error_finalize (GObject *object)
{
	GtkamError *error = GTKAM_ERROR (object);

	g_free (error->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_error_class_init (gpointer g_class, gpointer class_data)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = GTK_OBJECT_CLASS (g_class);
	object_class->destroy  = gtkam_error_destroy;

	gobject_class = G_OBJECT_CLASS (g_class);
	gobject_class->finalize = gtkam_error_finalize;

	parent_class = g_type_class_peek_parent (g_class);
}

static void
gtkam_error_init (GTypeInstance *instance, gpointer g_class)
{
	GtkamError *error = GTKAM_ERROR (instance);

	error->priv = g_new0 (GtkamErrorPrivate, 1);
}

GType
gtkam_error_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo ti;

		memset (&ti, 0, sizeof (GTypeInfo)); 
		ti.class_size     = sizeof (GtkamErrorClass);
		ti.class_init     = gtkam_error_class_init;
		ti.instance_size  = sizeof (GtkamError);
		ti.instance_init  = gtkam_error_init;

		type = g_type_register_static (PARENT_TYPE, "GtkamError",
					       &ti, 0);
	}

	return (type);
}

static void
on_error_ok_clicked (GtkButton *button, GtkamError *error)
{
	gtk_object_destroy (GTK_OBJECT (error));
}

GtkWidget *
gtkam_error_new (int result, GtkamContext *context, GtkWidget *opt_window,
		 const gchar *format, ...)
{
	GtkamError *error;
	GtkWidget *button, *label, *hbox,  *image;
	gchar *msg;
	va_list args;

	g_return_val_if_fail (result < 0, NULL);

	error = g_object_new (GTKAM_TYPE_ERROR, NULL);

	gtk_dialog_set_has_separator (GTK_DIALOG (error), FALSE);
	gtk_window_set_resizable (GTK_WINDOW (error), FALSE);
	gtk_window_set_title (GTK_WINDOW (error), "");
	gtk_container_set_border_width (GTK_CONTAINER (error), 6);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (error)->vbox), hbox, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
	gtk_box_set_spacing (GTK_BOX (hbox), 12);

	image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_DIALOG);
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (hbox), image, TRUE, TRUE, 0);

	va_start (args, format);
	msg = g_strdup_vprintf (format, args);
	va_end (args);
	if ((context) && (context->errors->pdata))
		label = gtk_label_new (g_strdup_printf("%s%s%s%s",
		                                       "<span weight=\"bold\" size=\"larger\">",
	                                           msg,
	                                           "</span>\n\n", 
	                                           context->errors->pdata[0]));
	else
		label = gtk_label_new (g_strdup_printf("%s%s%s",
		                                       "<span weight=\"bold\" size=\"larger\">",
		                                       msg,
		                                       "</span>"));
	g_free (msg);

	gtk_widget_show (label);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	button = gtk_button_new_from_stock (GTK_STOCK_OK);
	gtk_widget_show (button);
	g_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_error_ok_clicked), error);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (error)->action_area),
			   button);
	gtk_widget_grab_focus (button);

	if (opt_window)
		gtk_window_set_transient_for (GTK_WINDOW (error),
					      GTK_WINDOW (opt_window));

	return (GTK_WIDGET (error));
}
