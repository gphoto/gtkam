/* gtkam-tree.h
 *
 * Copyright © 2001 Lutz Müller <lutz@users.sf.net>
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

#include <gtk/gtktreeview.h>

#include <gtkam-camera.h>
#include <gtkam-context.h>

#define GTKAM_TYPE_TREE  (gtkam_tree_get_type ())
#define GTKAM_TREE(o)    (GTK_CHECK_CAST((o),GTKAM_TYPE_TREE,GtkamTree))
#define GTKAM_IS_TREE(o) (GTK_CHECK_TYPE((o),GTKAM_TYPE_TREE))

typedef struct _GtkamTree        GtkamTree;
typedef struct _GtkamTreePrivate GtkamTreePrivate;
typedef struct _GtkamTreeClass   GtkamTreeClass;

struct _GtkamTree
{
	GtkTreeView parent;

	GtkamTreePrivate *priv;
};

typedef struct _GtkamTreeFolderSelectedData GtkamTreeFolderSelectedData;
struct _GtkamTreeFolderSelectedData {
	GtkamCamera *camera;
	const gchar *folder;
};
typedef struct _GtkamTreeFolderUnselectedData GtkamTreeFolderUnselectedData;
struct _GtkamTreeFolderUnselectedData {
	GtkamCamera *camera;
	const gchar *folder;
};

typedef struct _GtkamTreeFileAddedData GtkamTreeFileAddedData;
struct _GtkamTreeFileAddedData {
	GtkamCamera *camera;
	const gchar *folder;
	const gchar *name;
};

typedef struct _GtkamTreeErrorData GtkamTreeErrorData;
struct _GtkamTreeErrorData {
	const gchar *msg;
	GtkamContext *context;
	int result;
};

struct _GtkamTreeClass
{
	GtkTreeViewClass parent_class;

	/* Signals */
	void (* folder_selected)   (GtkamTree *,
				    GtkamTreeFolderSelectedData *);
	void (* folder_unselected) (GtkamTree *,
				    GtkamTreeFolderUnselectedData *);
	void (* file_added)        (GtkamTree *, GtkamTreeFileAddedData *);
	void (* new_status)        (GtkamTree *tree, GtkWidget *status);
	void (* new_error)         (GtkamTree *tree, GtkamTreeErrorData *);
	void (* new_dialog)        (GtkamTree *tree, GtkWidget *dialog);
};

GType      gtkam_tree_get_type (void);
GtkWidget *gtkam_tree_new      (void);

void         gtkam_tree_add_camera (GtkamTree *tree, GtkamCamera *camera);

void         gtkam_tree_load       (GtkamTree *tree);
void         gtkam_tree_save       (GtkamTree *tree);

#endif /* __GTKAM_TREE_H__ */
