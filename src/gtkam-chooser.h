/* gtkam-chooser.h
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

#ifndef __GTKAM_CHOOSER_H__
#define __GTKAM_CHOOSER_H__

#include <gphoto2/gphoto2-camera.h>

#include <gtkam-dialog.h>

#include <gtkam-camera.h>

#define GTKAM_TYPE_CHOOSER  (gtkam_chooser_get_type ())
#define GTKAM_CHOOSER(o)    (GTK_CHECK_CAST((o),GTKAM_TYPE_CHOOSER,GtkamChooser))
#define GTKAM_IS_CHOOSER(o) (GTK_CHECK_TYPE((o),GTKAM_TYPE_CHOOSER))

typedef struct _GtkamChooser        GtkamChooser;
typedef struct _GtkamChooserPrivate GtkamChooserPrivate;
typedef struct _GtkamChooserClass   GtkamChooserClass;

struct _GtkamChooser
{
	GtkamDialog parent;

	GtkWidget *apply_button;

	GtkamChooserPrivate *priv;
};

struct _GtkamChooserClass
{
	GtkamDialogClass parent_class;

	/* Signals */
	void (* camera_selected) (GtkamChooser *, GtkamCamera *);
};

GtkType    gtkam_chooser_get_type (void);
GtkWidget *gtkam_chooser_new      (void);

void       gtkam_chooser_set_camera (GtkamChooser *chooser, GtkamCamera *);

void       gtkam_chooser_set_camera_mask (GtkamChooser *chooser, 
					  CameraOperation operations);
void       gtkam_chooser_set_port_mask   (GtkamChooser *chooser,
					  GPPortType types);

#endif /* __GTKAM_CHOOSER_H__ */
