/* gtkam-list.h
 *
 * Copyright © 2001 Lutz Müller <urc8@rz.uni-karlsruhe.de>
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

#ifndef __GTKAM_LIST_H__
#define __GTKAM_LIST_H__

#include <gphoto2/gphoto2-camera.h>

#include <gtk/gtktreeview.h>

#include <gtkam-camera.h>

#define GTKAM_TYPE_LIST  (gtkam_list_get_type ())
#define GTKAM_LIST(o)    (GTK_CHECK_CAST((o),GTKAM_TYPE_LIST,GtkamList))
#define GTKAM_IS_LIST(o) (GTK_CHECK_TYPE((o),GTKAM_TYPE_LIST))

typedef struct _GtkamList        GtkamList;
typedef struct _GtkamListPrivate GtkamListPrivate;
typedef struct _GtkamListClass   GtkamListClass;

struct _GtkamList
{
	GtkTreeView parent;

	GtkamListPrivate *priv;
};

typedef struct _GtkamListFileSelectedData GtkamListFileSelectedData;
struct _GtkamListFileSelectedData {
	GtkamCamera *camera;
	const char *folder;
	const char *name;
};

typedef struct _GtkamListFileUnselectedData GtkamListFileUnselectedData;
struct _GtkamListFileUnselectedData {
	GtkamCamera *camera;
	const char *folder;
	const char *name;
};

struct _GtkamListClass
{
	GtkTreeViewClass parent_class;

	/* Signals */
	void (* file_selected)   (GtkamList *, GtkamListFileSelectedData *);
	void (* file_unselected) (GtkamList *, GtkamListFileUnselectedData *);

	void (* new_status)      (GtkamList *, GtkWidget *status);
	void (* new_dialog)      (GtkamList *, GtkWidget *dialog);
};

GtkType    gtkam_list_get_type (void);
GtkWidget *gtkam_list_new      (void);

void      gtkam_list_add_folder    (GtkamList *, GtkamCamera *, const gchar *);
void      gtkam_list_remove_folder (GtkamList *, GtkamCamera *, const gchar *);
gboolean  gtkam_list_has_folder    (GtkamList *, GtkamCamera *, const gchar *);
void      gtkam_list_add_file      (GtkamList *, GtkamCamera *,
				    const gchar *folder, const gchar *name);

GtkamCamera *gtkam_list_get_camera_from_iter (GtkamList *, GtkTreeIter *);
gchar       *gtkam_list_get_folder_from_iter (GtkamList *, GtkTreeIter *);
gchar       *gtkam_list_get_name_from_iter   (GtkamList *, GtkTreeIter *);

void       gtkam_list_show_thumbnails (GtkamList *list);
void       gtkam_list_hide_thumbnails (GtkamList *list);

void       gtkam_list_save_selected   (GtkamList *list);
void       gtkam_list_save_all        (GtkamList *list);

void       gtkam_list_delete_selected (GtkamList *list);
void       gtkam_list_delete_all      (GtkamList *list);

guint      gtkam_list_count_all      (GtkamList *list);
guint      gtkam_list_count_selected (GtkamList *list);

void       gtkam_list_zoom_in  (GtkamList *);
void       gtkam_list_zoom_out (GtkamList *);
void       gtkam_list_zoom_100 (GtkamList *);

#endif /* __GTKAM_LIST_H__ */
