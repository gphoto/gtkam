/* gtkam-dialog.c
 *
 * Copyright © 2002 Lutz Müller <lutz@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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
#include "gtkam-dialog.h"
#include "i18n.h"

#include <string.h>

#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkimage.h>

struct _GtkamDialogPrivate
{
	GtkWidget *status;
};

#define PARENT_TYPE GTK_TYPE_DIALOG
static GtkDialogClass *parent_class;

static void
gtkam_dialog_destroy (GtkObject *object)
{
	GtkamDialog *dialog = GTKAM_DIALOG (object);

	dialog = NULL;

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_dialog_finalize (GObject *object)
{
	GtkamDialog *dialog = GTKAM_DIALOG (object);

	g_free (dialog->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_dialog_class_init (gpointer g_class, gpointer class_data)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = GTK_OBJECT_CLASS (g_class);
	object_class->destroy = gtkam_dialog_destroy;

	gobject_class = G_OBJECT_CLASS (g_class);
	gobject_class->finalize = gtkam_dialog_finalize;

	parent_class = g_type_class_peek_parent (g_class);
}

static void
gtkam_dialog_init (GTypeInstance *instance, gpointer g_class)
{
	GtkamDialog *dialog = GTKAM_DIALOG (instance);
	GtkWidget *hbox;

	dialog->priv = g_new0 (GtkamDialogPrivate, 1);

	dialog->priv->status = gtk_vbox_new (FALSE, 10);
	gtk_widget_show (dialog->priv->status);
	gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dialog)->vbox),
			  dialog->priv->status, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 10);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox,
			    TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);

	dialog->image = gtk_image_new_from_file (
					IMAGE_DIR "/gtkam-camera.png");
	gtk_widget_show (dialog->image);
	gtk_box_pack_start (GTK_BOX (hbox), dialog->image, FALSE, FALSE, 0);

	dialog->vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (dialog->vbox);
	gtk_box_pack_start (GTK_BOX (hbox), dialog->vbox, TRUE, TRUE, 0);
}

GType
gtkam_dialog_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo ti;

		memset (&ti, 0, sizeof (GTypeInfo));
		ti.class_size     = sizeof (GtkamDialogClass);
		ti.class_init     = gtkam_dialog_class_init;
		ti.instance_size  = sizeof (GtkamDialog);
		ti.instance_init  = gtkam_dialog_init;

		type = g_type_register_static (PARENT_TYPE, "GtkamDialog",
					       &ti, 0);
	}

	return (type);
}

void
gtkam_dialog_add_status (GtkamDialog *dialog, GtkWidget *status)
{
	g_return_if_fail (GTKAM_IS_DIALOG (dialog));
	g_return_if_fail (GTK_IS_WIDGET (status));

	g_object_ref (G_OBJECT (status));
	gtk_box_pack_start (GTK_BOX (dialog->priv->status), status,
			    FALSE, FALSE, 0);
	gtk_widget_show (status);
	gtk_widget_queue_draw (status);
}
