/* gtkam-error.c
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
#include "gtkam-error.h"

#include <stdio.h>

#include <gtk/gtktext.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkvscrollbar.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtktogglebutton.h>

#include <gphoto2/gphoto2-result.h>
#include <gphoto2/gphoto2-debug.h>

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
gtkam_error_finalize (GtkObject *object)
{
	GtkamError *error = GTKAM_ERROR (object);

	g_free (error->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_error_class_init (GtkamErrorClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_error_destroy;
	object_class->finalize = gtkam_error_finalize;

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_error_init (GtkamError *error)
{
	error->priv = g_new0 (GtkamErrorPrivate, 1);
}

GtkType
gtkam_error_get_type (void)
{
	static GtkType error_type = 0;

	if (!error_type) {
		static const GtkTypeInfo error_info = {
			"GtkamError",
			sizeof (GtkamError),
			sizeof (GtkamErrorClass),
			(GtkClassInitFunc)  gtkam_error_class_init,
			(GtkObjectInitFunc) gtkam_error_init,
			NULL, NULL, NULL};
		error_type = gtk_type_unique (PARENT_TYPE, &error_info);
	}

	return (error_type);
}

static void
on_error_close_clicked (GtkButton *button, GtkamError *error)
{
	gtk_object_destroy (GTK_OBJECT (error));
}

static void
on_debug_toggled (GtkToggleButton *toggle, GtkamError *error)
{
	if (toggle->active && !GTK_WIDGET_VISIBLE (error->priv->hbox))
		gtk_widget_show (error->priv->hbox);

	if (!toggle->active && GTK_WIDGET_VISIBLE (error->priv->hbox))
		gtk_widget_hide (error->priv->hbox);
}

GtkWidget *
gtkam_error_new (int result, Camera *opt_camera)
{
	GtkamError *error;
	GtkWidget *text, *vscrollbar, *button, *label;
	const char *history;
	const char *msg;
	gchar *full_msg;

	g_return_val_if_fail (result < 0, NULL);

	error = gtk_type_new (GTKAM_TYPE_ERROR);

	if (opt_camera)
		msg = gp_camera_get_result_as_string (opt_camera, result);
	else
		msg = gp_result_as_string (result);

	full_msg = g_strdup_printf ("An error occurred:\n\n'%s'", msg);
	label = gtk_label_new (full_msg);
	g_free (full_msg);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (error)->vbox), label,
			    FALSE, FALSE, 0);

	error->priv->hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (error)->vbox),
			    error->priv->hbox, TRUE, TRUE, 0);

	text = gtk_text_new (NULL, NULL);
	gtk_widget_show (text);
	gtk_text_set_editable (GTK_TEXT (text), FALSE);
	gtk_box_pack_start (GTK_BOX (error->priv->hbox), text, TRUE, TRUE, 0);

	history = gp_debug_history_get ();
	gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL,
			 history, strlen (history));

	vscrollbar = gtk_vscrollbar_new (GTK_TEXT (text)->vadj);
	gtk_widget_show (vscrollbar);
	gtk_box_pack_end (GTK_BOX (error->priv->hbox),
			  vscrollbar, FALSE, FALSE, 0);

	button = gtk_toggle_button_new_with_label ("Show debugging messages");
	gtk_widget_show (button);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (error)->action_area),
			   button);
	gtk_signal_connect (GTK_OBJECT (button), "toggled",
			    GTK_SIGNAL_FUNC (on_debug_toggled), error); 

	button = gtk_button_new_with_label ("Close");
	gtk_widget_show (button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_error_close_clicked), error);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (error)->action_area),
			   button);
	gtk_widget_grab_focus (button);

	return (GTK_WIDGET (error));
}
