/* gtk-exif-entry-menu.c
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
#include "gtk-exif-entry-menu.h"

#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkframe.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkhbox.h>

#include <libexif/exif-i18n.h>

struct _GtkExifEntryMenuPrivate {
	ExifEntry *entry;
};

#define PARENT_TYPE GTK_EXIF_TYPE_ENTRY
static GtkExifEntryClass *parent_class;

enum {
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void
gtk_exif_entry_menu_destroy (GtkObject *object)
{
	GtkExifEntryMenu *entry = GTK_EXIF_ENTRY_MENU (object);

	if (entry->priv->entry) {
		exif_entry_unref (entry->priv->entry);
		entry->priv->entry = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtk_exif_entry_menu_finalize (GtkObject *object)
{
	GtkExifEntryMenu *entry = GTK_EXIF_ENTRY_MENU (object);

	g_free (entry->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtk_exif_entry_menu_class_init (GtkExifEntryMenuClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtk_exif_entry_menu_destroy;
	object_class->finalize = gtk_exif_entry_menu_finalize;

	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtk_exif_entry_menu_init (GtkExifEntryMenu *entry)
{
	entry->priv = g_new0 (GtkExifEntryMenuPrivate, 1);
}

GtkType
gtk_exif_entry_menu_get_type (void)
{
	static GtkType entry_type = 0;

	if (!entry_type) {
		static const GtkTypeInfo entry_info = {
			"GtkExifEntryMenu",
			sizeof (GtkExifEntryMenu),
			sizeof (GtkExifEntryMenuClass),
			(GtkClassInitFunc)  gtk_exif_entry_menu_class_init,
			(GtkObjectInitFunc) gtk_exif_entry_menu_init,
			NULL, NULL, NULL};
		entry_type = gtk_type_unique (PARENT_TYPE, &entry_info);
	}

	return (entry_type);
}

static void
on_item_activate (GtkMenuItem *item, GtkExifEntryMenu *entry)
{
	ExifShort value;

	value = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (item),
						      "value"));
	exif_set_short (entry->priv->entry->data,
			entry->priv->entry->order, value);
	exif_entry_notify (entry->priv->entry, EXIF_ENTRY_EVENT_CHANGED);
}

void
gtk_exif_entry_menu_construct (GtkExifEntryMenu *entry, ExifEntry *e,
			       const gchar *name, GtkExifEntryMenuList *list)
{
	GtkWidget *o, *menu, *item, *hbox, *label;
	guint i, history;
	ExifShort value;

	g_return_if_fail (GTK_EXIF_IS_ENTRY_MENU (entry));
	g_return_if_fail (e != NULL);
	g_return_if_fail (e->format == EXIF_FORMAT_SHORT);
	g_return_if_fail (name != NULL);
	g_return_if_fail (list != NULL);

	entry->priv->entry = e;
	exif_entry_ref (e);
	gtk_exif_entry_construct (GTK_EXIF_ENTRY (entry),
		exif_tag_get_title (e->tag),
		exif_tag_get_description (e->tag));

	hbox = gtk_hbox_new (FALSE, 5);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (entry), hbox, TRUE, FALSE, 0);
	label = gtk_label_new (name);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	o = gtk_option_menu_new ();
	gtk_widget_show (o);
	gtk_box_pack_start (GTK_BOX (hbox), o, FALSE, FALSE, 0);
	menu = gtk_menu_new ();
	gtk_widget_show (menu);
	value = exif_get_short (e->data, e->order);
	for (history = i = 0; list[i].name; i++) {
		item = gtk_menu_item_new_with_label (list[i].name);
		gtk_widget_show (item);
		gtk_menu_append (GTK_MENU (menu), item);
		gtk_object_set_data (GTK_OBJECT (item), "value",
				GINT_TO_POINTER ((gint) list[i].value));
		gtk_signal_connect (GTK_OBJECT (item), "activate",
				    GTK_SIGNAL_FUNC (on_item_activate), entry);
		if (list[i].value == value)
			history = i;
	}
	gtk_option_menu_set_menu (GTK_OPTION_MENU (o), menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (o), history);
}
