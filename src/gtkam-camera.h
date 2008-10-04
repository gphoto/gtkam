/* gtkam-camera.h
 *
 * Copyright 2002 Lutz Mueller <lutz@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
