/* gtkam-tree-item-cam.h
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
#ifndef __GTKAM_TREE_ITEM_CAM_H__
#define __GTKAM_TREE_ITEM_CAM_H__

#include <gphoto2/gphoto2-camera.h>
#include <gtkam-tree-item.h>

#define GTKAM_TYPE_TREE_ITEM_CAM  (gtkam_tree_item_cam_get_type ())
#define GTKAM_TREE_ITEM_CAM(o)    (GTK_CHECK_CAST((o),GTKAM_TYPE_TREE_ITEM_CAM,GtkamTreeItemCam))
#define GTKAM_IS_TREE_ITEM_CAM(o) (GTK_CHECK_TYPE((o),GTKAM_TYPE_TREE_ITEM_CAM))

typedef struct _GtkamTreeItemCam        GtkamTreeItemCam;
typedef struct _GtkamTreeItemCamPrivate GtkamTreeItemCamPrivate;
typedef struct _GtkamTreeItemCamClass   GtkamTreeItemCamClass;

struct _GtkamTreeItemCam {
	GtkamTreeItem parent;

	GtkamTreeItemCamPrivate *priv;
};

struct _GtkamTreeItemCamClass {
	GtkamTreeItemClass parent_class;
};

GtkType    gtkam_tree_item_cam_get_type (void);
GtkWidget *gtkam_tree_item_cam_new      (GtkWidget *vbox);

#endif /* __GTKAM_TREE_ITEM_CAM_H__ */
