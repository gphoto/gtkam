/* gtkam-info.h
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

#ifndef __GTKAM_INFO_H__
#define __GTKAM_INFO_H__

#include <gphoto2/gphoto2-camera.h>
#include <gtk/gtkdialog.h>

#define GTKAM_TYPE_INFO  (gtkam_info_get_type ())
#define GTKAM_INFO(o)    (GTK_CHECK_CAST((o),GTKAM_TYPE_INFO,GtkamInfo))
#define GTKAM_IS_INFO(o) (GTK_CHECK_TYPE((o),GTKAM_TYPE_INFO))

typedef struct _GtkamInfo        GtkamInfo;
typedef struct _GtkamInfoPrivate GtkamInfoPrivate;
typedef struct _GtkamInfoClass   GtkamInfoClass;

struct _GtkamInfo
{
	GtkDialog parent;

	GtkamInfoPrivate *priv;
};

struct _GtkamInfoClass
{
	GtkDialogClass parent_class;

	/* Signals */
	void (* info_updated) (GtkamInfo *info, Camera *camera,
			       gboolean multi, const gchar *folder,
			       const gchar *name);
};

GtkType    gtkam_info_get_type (void);
GtkWidget *gtkam_info_new      (Camera *camera, const gchar *path,
				GtkWidget *opt_window);

#endif /* __GTKAM_INFO_H__ */
