/* gtkam-debug.h
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

#ifndef __GTKAM_DEBUG_H__
#define __GTKAM_DEBUG_H__

#include <gtkam-dialog.h>

#define GTKAM_TYPE_DEBUG  (gtkam_debug_get_type ())
#define GTKAM_DEBUG(o)    (GTK_CHECK_CAST((o),GTKAM_TYPE_DEBUG,GtkamDebug))
#define GTKAM_IS_DEBUG(o) (GTK_CHECK_TYPE((o),GTKAM_TYPE_DEBUG))

typedef struct _GtkamDebug        GtkamDebug;
typedef struct _GtkamDebugPrivate GtkamDebugPrivate;
typedef struct _GtkamDebugClass   GtkamDebugClass;

struct _GtkamDebug
{
	GtkamDialog parent;

	GtkamDebugPrivate *priv;
};

struct _GtkamDebugClass
{
	GtkamDialogClass parent_class;
};

GtkType    gtkam_debug_get_type (void);
GtkWidget *gtkam_debug_new      (void);

#endif /* __GTKAM_DEBUG_H__ */
