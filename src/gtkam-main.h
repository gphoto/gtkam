/* gtkam-main.h
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

#ifndef __GTKAM_MAIN_H__
#define __GTKAM_MAIN_H__

#include <gphoto2/gphoto2-camera.h>
#include <gtk/gtkwindow.h>

#define GTKAM_TYPE_MAIN  (gtkam_main_get_type ())
#define GTKAM_MAIN(o)    (GTK_CHECK_CAST((o),GTKAM_TYPE_MAIN,GtkamMain))
#define GTKAM_IS_MAIN(o) (GTK_CHECK_TYPE((o),GTKAM_TYPE_MAIN))

typedef struct _GtkamMain        GtkamMain;
typedef struct _GtkamMainPrivate GtkamMainPrivate;
typedef struct _GtkamMainClass   GtkamMainClass;

struct _GtkamMain
{
	GtkWindow parent;

	GtkamMainPrivate *priv;
};

struct _GtkamMainClass
{
	GtkWindowClass parent_class;
};

GtkType    gtkam_main_get_type (void);
GtkWidget *gtkam_main_new      (void);

void       gtkam_main_load     (GtkamMain *);

#endif /* __GTKAM_MAIN_H__ */
