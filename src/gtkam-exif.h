/* gtkam-exif.h
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

#ifndef __GTKAM_EXIF_H__
#define __GTKAM_EXIF_H__

#include <gtkam-camera.h>
#include <gtk/gtkdialog.h>

#define GTKAM_TYPE_EXIF  (gtkam_exif_get_type ())
#define GTKAM_EXIF(o)    (GTK_CHECK_CAST((o),GTKAM_TYPE_EXIF,GtkamExif))
#define GTKAM_IS_EXIF(o) (GTK_CHECK_TYPE((o),GTKAM_TYPE_EXIF))

typedef struct _GtkamExif        GtkamExif;
typedef struct _GtkamExifPrivate GtkamExifPrivate;
typedef struct _GtkamExifClass   GtkamExifClass;

struct _GtkamExif
{
	GtkDialog parent;

	GtkamExifPrivate *priv;
};

struct _GtkamExifClass
{
	GtkDialogClass parent_class;
};

GtkType    gtkam_exif_get_type (void);
GtkWidget *gtkam_exif_new      (GtkamCamera *, const gchar *folder,
				const gchar *file);

#endif /* __GTKAM_EXIF_H__ */
