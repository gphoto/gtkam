/* gtkam-fsel.h
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

#ifndef __GTKAM_FSEL_H__
#define __GTKAM_FSEL_H__

#include <gphoto2/gphoto2-camera.h>
#include <gtkam-dialog.h>

#define GTKAM_TYPE_FSEL  (gtkam_fsel_get_type ())
#define GTKAM_FSEL(o)    (GTK_CHECK_CAST((o),GTKAM_TYPE_FSEL,GtkamFSel))
#define GTKAM_IS_FSEL(o) (GTK_CHECK_TYPE((o),GTKAM_TYPE_FSEL))

typedef struct _GtkamFSel        GtkamFSel;
typedef struct _GtkamFSelPrivate GtkamFSelPrivate;
typedef struct _GtkamFSelClass   GtkamFSelClass;

struct _GtkamFSel
{
	GtkamDialog parent;

	GtkWidget *ok_button;
	GtkWidget *list; /* GtkamList */

	GtkamFSelPrivate *priv;
};

struct _GtkamFSelClass
{
	GtkamDialogClass parent_class;
};

GtkType    gtkam_fsel_get_type (void);
GtkWidget *gtkam_fsel_new      (GtkWidget *opt_window);

#endif /* __GTKAM_FSEL_H__ */
