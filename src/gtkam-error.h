/* gtkam-error.h
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

#ifndef __GTKAM_ERROR_H__
#define __GTKAM_ERROR_H__

#include <gphoto2/gphoto2-camera.h>
#include <gtk/gtkdialog.h>
#include <gtkam-context.h>

#define GTKAM_TYPE_ERROR  (gtkam_error_get_type ())
#define GTKAM_ERROR(o)    (GTK_CHECK_CAST((o),GTKAM_TYPE_ERROR,GtkamError))
#define GTKAM_IS_ERROR(o) (GTK_CHECK_TYPE((o),GTKAM_TYPE_ERROR))

typedef struct _GtkamError        GtkamError;
typedef struct _GtkamErrorPrivate GtkamErrorPrivate;
typedef struct _GtkamErrorClass   GtkamErrorClass;

struct _GtkamError
{
	GtkDialog parent;

	GtkamErrorPrivate *priv;
};

struct _GtkamErrorClass
{
	GtkDialogClass parent_class;
};

GtkType    gtkam_error_get_type (void);
GtkWidget *gtkam_error_new      (int result, GtkamContext *context,
				 GtkWidget *opt_window, const gchar *format,
				 ...);

#endif /* __GTKAM_ERROR_H__ */
