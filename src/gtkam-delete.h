/* gtkam-delete.h
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

#ifndef __GTKAM_DELETE_H__
#define __GTKAM_DELETE_H__

#include <gphoto2/gphoto2-camera.h>
#include <gtk/gtkdialog.h>

#define GTKAM_TYPE_DELETE  (gtkam_delete_get_type ())
#define GTKAM_DELETE(o)    (GTK_CHECK_CAST((o),GTKAM_TYPE_DELETE,GtkamDelete))
#define GTKAM_IS_DELETE(o) (GTK_CHECK_TYPE((o),GTKAM_TYPE_DELETE))

typedef struct _GtkamDelete        GtkamDelete;
typedef struct _GtkamDeletePrivate GtkamDeletePrivate;
typedef struct _GtkamDeleteClass   GtkamDeleteClass;

struct _GtkamDelete
{
	GtkDialog parent;

	GtkamDeletePrivate *priv;
};

struct _GtkamDeleteClass
{
	GtkDialogClass parent_class;

	/* Signals */
	void (* all_deleted)  (GtkamDelete *delete, const gchar *path);
	void (* file_deleted) (GtkamDelete *delete, const gchar *path);
};

GtkType    gtkam_delete_get_type (void);
GtkWidget *gtkam_delete_new      (Camera *camera, gboolean multi,
				  const gchar *path,
				  GList *files, GtkWidget *opt_window);

#endif /* __GTKAM_DELETE_H__ */
