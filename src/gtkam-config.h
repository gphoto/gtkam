/* gtkam-config.h
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

#ifndef __GTKAM_CONFIG_H__
#define __GTKAM_CONFIG_H__

#include <gphoto2/gphoto2-camera.h>
#include <gtk/gtkdialog.h>

#define GTKAM_TYPE_CONFIG  (gtkam_config_get_type ())
#define GTKAM_CONFIG(o)    (GTK_CHECK_CAST((o),GTKAM_TYPE_CONFIG,GtkamConfig))
#define GTKAM_IS_CONFIG(o) (GTK_CHECK_TYPE((o),GTKAM_TYPE_CONFIG))

typedef struct _GtkamConfig        GtkamConfig;
typedef struct _GtkamConfigPrivate GtkamConfigPrivate;
typedef struct _GtkamConfigClass   GtkamConfigClass;

struct _GtkamConfig
{
	GtkDialog parent;

	GtkamConfigPrivate *priv;
};

struct _GtkamConfigClass
{
	GtkDialogClass parent_class;
};

GtkType    gtkam_config_get_type (void);
GtkWidget *gtkam_config_new      (Camera *camera, gboolean multi);

#endif /* __GTKAM_CONFIG_H__ */
