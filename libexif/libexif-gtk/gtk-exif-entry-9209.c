/* gtk-exif-entry-9209.c
 *
 * Copyright (C) 2001 Lutz Müller <lutz@users.sourceforge.net>
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
#include "gtk-exif-entry-9209.h"

#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkframe.h>

struct _GtkExifEntry9209Private {
	ExifEntry *entry;

	GtkToggleButton *c;
	GtkToggleButton *r1, *r2, *r3;
};

#define PARENT_TYPE GTK_EXIF_TYPE_ENTRY
static GtkExifEntryClass *parent_class;

enum {
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void
gtk_exif_entry_9209_destroy (GtkObject *object)
{
	GtkExifEntry9209 *entry = GTK_EXIF_ENTRY_9209 (object);

	if (entry->priv->entry) {
		exif_entry_unref (entry->priv->entry);
		entry->priv->entry = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtk_exif_entry_9209_finalize (GtkObject *object)
{
	GtkExifEntry9209 *entry = GTK_EXIF_ENTRY_9209 (object);

	g_free (entry->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtk_exif_entry_9209_class_init (GtkExifEntry9209Class *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtk_exif_entry_9209_destroy;
	object_class->finalize = gtk_exif_entry_9209_finalize;

	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtk_exif_entry_9209_init (GtkExifEntry9209 *entry)
{
	entry->priv = g_new0 (GtkExifEntry9209Private, 1);
}

GtkType
gtk_exif_entry_9209_get_type (void)
{
	static GtkType entry_type = 0;

	if (!entry_type) {
		static const GtkTypeInfo entry_info = {
			"GtkExifEntry9209",
			sizeof (GtkExifEntry9209),
			sizeof (GtkExifEntry9209Class),
			(GtkClassInitFunc)  gtk_exif_entry_9209_class_init,
			(GtkObjectInitFunc) gtk_exif_entry_9209_init,
			NULL, NULL, NULL};
		entry_type = gtk_type_unique (PARENT_TYPE, &entry_info);
	}

	return (entry_type);
}

static void
on_value_changed (GtkToggleButton *toggle, GtkExifEntry9209 *entry)
{
	g_return_if_fail (GTK_EXIF_IS_ENTRY_9209 (entry));

	entry->priv->entry->data[0] &= 0xfe;
	if (entry->priv->c->active)
		entry->priv->entry->data[0] |= 0x01;

	entry->priv->entry->data[0] &= 0xf9;
	if (entry->priv->r2->active)
		entry->priv->entry->data[0] |= 0x04;
	else if (entry->priv->r3->active)
		entry->priv->entry->data[0] |= 0x06;
}

GtkWidget *
gtk_exif_entry_9209_new (ExifEntry *e)
{
	GtkExifEntry9209 *entry;
	GtkWidget *check, *frame, *vbox, *radio;
	GSList *group;

	g_return_val_if_fail (e != NULL, NULL);
	g_return_val_if_fail (e->tag == EXIF_TAG_FLASH, NULL);

	entry = gtk_type_new (GTK_EXIF_TYPE_ENTRY_9209);
	entry->priv->entry = e;
	exif_entry_ref (e);
	gtk_exif_entry_construct (GTK_EXIF_ENTRY (entry), e);

	check = gtk_check_button_new_with_label ("Flash fired");
	gtk_widget_show (check);
	gtk_box_pack_start (GTK_BOX (entry), check, FALSE, FALSE, 0);
	if (e->data[0] & (1 << 0))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), TRUE);
	gtk_signal_connect (GTK_OBJECT (check), "toggled",
			    GTK_SIGNAL_FUNC (on_value_changed), entry);
	entry->priv->c = GTK_TOGGLE_BUTTON (check);

	frame = gtk_frame_new ("Return light");
	gtk_widget_show (frame);
	gtk_box_pack_start (GTK_BOX (entry), frame, FALSE, FALSE, 0);
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	radio = gtk_radio_button_new_with_label (NULL, "No strobe return "
						 "detection function");
	gtk_widget_show (radio);
	gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (radio), "toggled",
			    GTK_SIGNAL_FUNC (on_value_changed), entry);
	entry->priv->r1 = GTK_TOGGLE_BUTTON (radio);
	group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio));
	radio = gtk_radio_button_new_with_label (group,
				"Strobe return light not detected");
	gtk_widget_show (radio);
	gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);
	entry->priv->r2 = GTK_TOGGLE_BUTTON (radio);
	group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio));
	radio = gtk_radio_button_new_with_label (group, 
					"Strobe return light detected");
	gtk_widget_show (radio);
	gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);
	entry->priv->r3 = GTK_TOGGLE_BUTTON (radio);

	return (GTK_WIDGET (entry));
}
