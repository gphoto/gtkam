/* gtkam-clist.h
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

#ifndef __GTKAM_CLIST_H__
#define __GTKAM_CLIST_H__

#include <gphoto2/gphoto2-camera.h>
#include <gtk/gtkclist.h>

#define GTKAM_TYPE_CLIST  (gtkam_clist_get_type ())
#define GTKAM_CLIST(o)    (GTK_CHECK_CAST((o),GTKAM_TYPE_CLIST,GtkamCList))
#define GTKAM_IS_CLIST(o) (GTK_CHECK_TYPE((o),GTKAM_TYPE_CLIST))

typedef struct _GtkamCList        GtkamCList;
typedef struct _GtkamCListPrivate GtkamCListPrivate;
typedef struct _GtkamCListClass   GtkamCListClass;

struct _GtkamCList
{
	GtkCList parent;

	/* GList contains elements of type GtkamCListEntry */
	GList *selection;

	GtkamCListPrivate *priv;
};

typedef struct _GtkamCListEntry GtkamCListEntry;
struct _GtkamCListEntry {
	Camera *camera;
	gboolean multi;
	gchar *folder;
	gchar *name;
};

struct _GtkamCListClass
{
	GtkCListClass parent_class;

	/* Signals */
	void (* entry_selected)   (GtkamCList *list, GtkamCListEntry *entry);
	void (* entry_unselected) (GtkamCList *list, GtkamCListEntry *entry);
	void (* new_status)       (GtkamCList *list, GtkWidget *status);
};

GtkType    gtkam_clist_get_type (void);
GtkWidget *gtkam_clist_new      (void);

void       gtkam_clist_add_folder     (GtkamCList *list, Camera *camera,
				       gboolean multi, const gchar *folder);
void       gtkam_clist_remove_folder  (GtkamCList *list, Camera *camera,
				       gboolean multi, const gchar *folder);

void       gtkam_clist_save_selected   (GtkamCList *list);
void       gtkam_clist_delete_selected (GtkamCList *list);
void       gtkam_clist_delete_all      (GtkamCList *list);

#endif /* __GTKAM_CLIST_H__ */
