/* gtkam-camera.c
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

#include "config.h"
#include "gtkam-camera.h"
#include "i18n.h"

#include <string.h>

#define PARENT_TYPE G_TYPE_OBJECT
static GObjectClass *parent_class;

static void
gtkam_camera_finalize (GObject *object)
{
	GtkamCamera *c = GTKAM_CAMERA (object);

	if (c->camera) {
		gp_camera_unref (c->camera);
		c->camera = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_camera_class_init (gpointer g_class, gpointer class_data)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (g_class);
	gobject_class->finalize = gtkam_camera_finalize;

	parent_class = g_type_class_peek_parent (g_class);
}

static void
gtkam_camera_init (GTypeInstance *instance, gpointer g_class)
{
	GtkamCamera *c = GTKAM_CAMERA (instance);

	c = NULL;
}

GType
gtkam_camera_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo ti;

		memset (&ti, 0, sizeof (GTypeInfo));
		ti.class_size    = sizeof (GtkamCameraClass);
		ti.class_init    = gtkam_camera_class_init;
		ti.instance_size = sizeof (GtkamCamera);
		ti.instance_init = gtkam_camera_init;

		type = g_type_register_static (PARENT_TYPE, "GtkamCamera",
					       &ti, 0);
	}

	return (type);
}

GtkamCamera *
gtkam_camera_new (Camera *camera, gboolean multi)
{
	GtkamCamera *c;

	c = g_object_new (GTKAM_TYPE_CAMERA, NULL);

	c->camera = camera;
	gp_camera_ref (camera);
	c->multi = multi;

	return ©;
}
