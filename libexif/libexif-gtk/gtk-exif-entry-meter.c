/* gtk-exif-entry-meter.c
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
#include "gtk-exif-entry-meter.h"

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

struct _GtkExifEntryMeterPrivate {
	ExifEntry *entry;
};

#define PARENT_TYPE GTK_EXIF_TYPE_ENTRY_MENU
static GtkExifEntryMenuClass *parent_class;

enum {
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void
gtk_exif_entry_meter_destroy (GtkObject *object)
{
	GtkExifEntryMeter *entry = GTK_EXIF_ENTRY_METER (object);

	if (entry->priv->entry) {
		exif_entry_unref (entry->priv->entry);
		entry->priv->entry = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtk_exif_entry_meter_finalize (GtkObject *object)
{
	GtkExifEntryMeter *entry = GTK_EXIF_ENTRY_METER (object);

	g_free (entry->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtk_exif_entry_meter_class_init (GtkExifEntryMeterClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtk_exif_entry_meter_destroy;
	object_class->finalize = gtk_exif_entry_meter_finalize;

	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtk_exif_entry_meter_init (GtkExifEntryMeter *entry)
{
	entry->priv = g_new0 (GtkExifEntryMeterPrivate, 1);
}

GtkType
gtk_exif_entry_meter_get_type (void)
{
	static GtkType entry_type = 0;

	if (!entry_type) {
		static const GtkTypeInfo entry_info = {
			"GtkExifEntryMeter",
			sizeof (GtkExifEntryMeter),
			sizeof (GtkExifEntryMeterClass),
			(GtkClassInitFunc)  gtk_exif_entry_meter_class_init,
			(GtkObjectInitFunc) gtk_exif_entry_meter_init,
			NULL, NULL, NULL};
		entry_type = gtk_type_unique (PARENT_TYPE, &entry_info);
	}

	return (entry_type);
}

static GtkExifEntryMenuList meter_list[] = {
	{  0, N_("Unknown")},
	{  1, N_("Average")},
	{  2, N_("Center-Weighted Average")},
	{  3, N_("Spot")},
	{  4, N_("Multi Spot")},
	{  5, N_("Pattern")},
	{  6, N_("Partial")},
	{255, N_("Other")},
	{  0, NULL}
};

GtkWidget *
gtk_exif_entry_meter_new (ExifEntry *e)
{
	GtkExifEntryMeter *entry;

	g_return_val_if_fail (e != NULL, NULL);
	g_return_val_if_fail (e->tag == EXIF_TAG_METERING_MODE, NULL);
	g_return_val_if_fail (e->format == EXIF_FORMAT_SHORT, NULL);

	entry = gtk_type_new (GTK_EXIF_TYPE_ENTRY_METER);
	entry->priv->entry = e;
	exif_entry_ref (e);
	gtk_exif_entry_menu_construct (GTK_EXIF_ENTRY_MENU (entry), e,
					_("Metering Mode:"), meter_list);

	return (GTK_WIDGET (entry));
}
