/* gtkam-camera.h
 *
 * Copyright © 2002 Lutz Müller <lutz@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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

#ifndef __GTKAM_CAMERA_H__
#define __GTKAM_CAMERA_H__

#include <gphoto2/gphoto2-camera.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define GTKAM_TYPE_CAMERA  (gtkam_camera_get_type())
#define GTKAM_CAMERA(o)    (G_TYPE_CHECK_INSTANCE_CAST((o),GTKAM_TYPE_CAMERA,GtkamCamera))
#define GTKAM_IS_CAMERA(o) (G_TYPE_CHECK_INSTANCE_TYPE((o),GTKAM_TYPE_CAMERA))

typedef struct _GtkamCamera      GtkamCamera;
typedef struct _GtkamCameraClass GtkamCameraClass;

struct _GtkamCamera {
	GObject parent;

	Camera *camera;
	gboolean multi;
};

struct _GtkamCameraClass {
	GObjectClass parent_class;
};

GType        gtkam_camera_get_type (void);
GtkamCamera *gtkam_camera_new (Camera *camera, gboolean multi);

G_END_DECLS

#endif /* __GTKAM_CAMERA_H__ */
