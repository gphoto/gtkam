/* gtkam-chooser.h
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

#ifndef __GTKAM_CHOOSER_H__
#define __GTKAM_CHOOSER_H__

#include <gphoto2/gphoto2-camera.h>
#include <gtk/gtkdialog.h>

#define GTKAM_TYPE_CHOOSER  (gtkam_chooser_get_type ())
#define GTKAM_CHOOSER(o)    (GTK_CHECK_CAST((o),GTKAM_TYPE_CHOOSER,GtkamChooser))
#define GTKAM_IS_CHOOSER(o) (GTK_CHECK_TYPE((o),GTKAM_TYPE_CHOOSER))

typedef struct _GtkamChooser        GtkamChooser;
typedef struct _GtkamChooserPrivate GtkamChooserPrivate;
typedef struct _GtkamChooserClass   GtkamChooserClass;

struct _GtkamChooser
{
	GtkDialog parent;

	GtkamChooserPrivate *priv;
};

struct _GtkamChooserClass
{
	GtkDialogClass parent_class;

	/* Signals */
	void (* camera_selected) (GtkamChooser *chooser, Camera *camera);
};

GtkType    gtkam_chooser_get_type (void);
GtkWidget *gtkam_chooser_new      (Camera *opt_camera);

#endif /* __GTKAM_CHOOSER_H__ */
