/* gtkam-mkdir.h
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

#ifndef __GTKAM_MKDIR_H__
#define __GTKAM_MKDIR_H__

#include <gtkam-camera.h>
#include <gtkam-dialog.h>

#define GTKAM_TYPE_MKDIR  (gtkam_mkdir_get_type ())
#define GTKAM_MKDIR(o)    (GTK_CHECK_CAST((o),GTKAM_TYPE_MKDIR,GtkamMkdir))
#define GTKAM_IS_MKDIR(o) (GTK_CHECK_TYPE((o),GTKAM_TYPE_MKDIR))

typedef struct _GtkamMkdir        GtkamMkdir;
typedef struct _GtkamMkdirPrivate GtkamMkdirPrivate;
typedef struct _GtkamMkdirClass   GtkamMkdirClass;

struct _GtkamMkdir
{
	GtkamDialog parent;

	GtkamMkdirPrivate *priv;
};

typedef struct _GtkamMkdirDirCreatedData GtkamMkdirDirCreatedData;
struct _GtkamMkdirDirCreatedData {
	GtkamCamera *camera;
	const gchar *path;
};

struct _GtkamMkdirClass
{
	GtkamDialogClass parent_class;

	void (* dir_created) (GtkamMkdir *mkdir, GtkamMkdirDirCreatedData *);
};

GType      gtkam_mkdir_get_type (void);
GtkWidget *gtkam_mkdir_new      (GtkamCamera *, const gchar *path);

#endif /* __GTKAM_MKDIR_H__ */
