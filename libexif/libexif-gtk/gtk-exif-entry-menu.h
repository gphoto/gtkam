/* gtk-exif-entry-menu.h
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

#ifndef __GTK_EXIF_ENTRY_MENU_H__
#define __GTK_EXIF_ENTRY_MENU_H__

#include <libexif-gtk/gtk-exif-entry.h>

#define GTK_EXIF_TYPE_ENTRY_MENU     (gtk_exif_entry_menu_get_type())
#define GTK_EXIF_ENTRY_MENU(o)       (GTK_CHECK_CAST((o),GTK_EXIF_TYPE_ENTRY_MENU,GtkExifEntryMenu))
#define GTK_EXIF_ENTRY_MENU_CLASS(k) (GTK_CHECK_CLASS_CAST((k),GTK_EXIF_TYPE_ENTRY_MENU,GtkExifEntryMenuClass))
#define GTK_EXIF_IS_ENTRY_MENU(o)    (GTK_CHECK_TYPE((o),GTK_EXIF_TYPE_ENTRY_MENU))

typedef struct _GtkExifEntryMenu        GtkExifEntryMenu;
typedef struct _GtkExifEntryMenuPrivate GtkExifEntryMenuPrivate;
typedef struct _GtkExifEntryMenuClass   GtkExifEntryMenuClass;

struct _GtkExifEntryMenu
{
	GtkExifEntry parent;

	GtkExifEntryMenuPrivate *priv;
};

struct _GtkExifEntryMenuClass
{
	GtkExifEntryClass parent_class;
};

typedef struct _GtkExifEntryMenuList GtkExifEntryMenuList;
struct _GtkExifEntryMenuList {
	ExifShort value;
	const gchar *name;
};

GtkType    gtk_exif_entry_menu_get_type (void);
void       gtk_exif_entry_menu_construct (GtkExifEntryMenu *entry, 
					  ExifEntry *e,
					  const gchar *label, 
					  GtkExifEntryMenuList *list);

#endif /* __GTK_EXIF_ENTRY_MENU_H__ */
