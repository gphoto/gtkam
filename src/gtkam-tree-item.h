/* gtkam-tree-item.h
 *
 * Copyright (C) 2002 Lutz Müller <lutz@users.sourceforge.net>
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
#ifndef __GTKAM_TREE_ITEM_H__
#define __GTKAM_TREE_ITEM_H__

#include <gphoto2/gphoto2-camera.h>
#include <gtk/gtktreeitem.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#define GTKAM_TYPE_TREE_ITEM     (gtkam_tree_item_get_type ())
#define GTKAM_TREE_ITEM(o)       (GTK_CHECK_CAST((o),GTKAM_TYPE_TREE_ITEM,GtkamTreeItem))
#define GTKAM_TREE_ITEM_CLASS(k) (GTK_CHECK_CLASS_CAST((k),GTKAM_TYPE_TREE_ITEM,GtkamTreeItemClass))
#define GTKAM_IS_TREE_ITEM(o)    (GTK_CHECK_TYPE((o),GTKAM_TYPE_TREE_ITEM))

typedef struct _GtkamTreeItem        GtkamTreeItem;
typedef struct _GtkamTreeItemPrivate GtkamTreeItemPrivate;
typedef struct _GtkamTreeItemClass   GtkamTreeItemClass;

struct _GtkamTreeItem {
	GtkTreeItem parent;

	/* GtkLabel */
	GtkWidget *label, *status;

	GtkamTreeItemPrivate *priv;
};

struct _GtkamTreeItemClass {
	GtkTreeItemClass parent_class;

	/* Signals */
	void (* file_uploaded) (GtkamTreeItem *item, const gchar *name);
	void (* new_status)    (GtkamTreeItem *item, GtkWidget *status);

	/* Virtual methods */
	void  (* set_camera)     (GtkamTreeItem *item, Camera *camera);
	void  (* set_multi)      (GtkamTreeItem *item, gboolean multi);
	guint (* add_menu_items) (GtkamTreeItem *item, GtkWidget *menu);
};

GtkType    gtkam_tree_item_get_type   (void);
GtkWidget *gtkam_tree_item_new        (GdkPixbuf *pixbuf);
void       gtkam_tree_item_construct  (GtkamTreeItem *item, GdkPixbuf *pixbuf);

void       gtkam_tree_item_set_camera (GtkamTreeItem *item, Camera *camera);
void       gtkam_tree_item_set_multi  (GtkamTreeItem *item, gboolean multi);
Camera    *gtkam_tree_item_get_camera (GtkamTreeItem *item);
gboolean   gtkam_tree_item_get_multi  (GtkamTreeItem *item);

void         gtkam_tree_item_set_folder    (GtkamTreeItem *item,
				            const gchar *folder);
const gchar *gtkam_tree_item_get_folder    (GtkamTreeItem *item);

void         gtkam_tree_item_update (GtkamTreeItem *item);

gboolean gtkam_tree_item_get_online (GtkamTreeItem *item);
void     gtkam_tree_item_set_online (GtkamTreeItem *item, gboolean online);

#endif /* __GTKAM_TREE_ITEM_H__ */
