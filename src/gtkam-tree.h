/* gtkam-tree.h
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

#ifndef __GTKAM_TREE_H__
#define __GTKAM_TREE_H__

#include <gphoto2/gphoto2-camera.h>
#include <gtk/gtktree.h>

#define GTKAM_TYPE_TREE  (gtkam_tree_get_type ())
#define GTKAM_TREE(o)    (GTK_CHECK_CAST((o),GTKAM_TYPE_TREE,GtkamTree))
#define GTKAM_IS_TREE(o) (GTK_CHECK_TYPE((o),GTKAM_TYPE_TREE))

typedef struct _GtkamTree        GtkamTree;
typedef struct _GtkamTreePrivate GtkamTreePrivate;
typedef struct _GtkamTreeClass   GtkamTreeClass;

struct _GtkamTree
{
	GtkTree parent;

	GtkamTreePrivate *priv;
};

struct _GtkamTreeClass
{
	GtkTreeClass parent_class;

	/* Signals */
	void (* folder_selected)   (GtkamTree *tree, const gchar *path);
	void (* folder_unselected) (GtkamTree *tree, const gchar *path);
};

GtkType    gtkam_tree_get_type (void);
GtkWidget *gtkam_tree_new      (void);

void         gtkam_tree_set_camera (GtkamTree *tree, Camera *camera);
const gchar *gtkam_tree_get_path   (GtkamTree *tree);
void         gtkam_tree_remove_dir (GtkamTree *tree, const gchar *path);
void         gtkam_tree_refresh    (GtkamTree *tree, const gchar *path);

#endif /* __GTKAM_TREE_H__ */
