/* gtkam-context.h
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

#ifndef __GTKAM_CONTEXT_H__
#define __GTKAM_CONTEXT_H__

#include <gphoto2/gphoto2-context.h>
#include <glib-object.h>

#define GTKAM_TYPE_CONTEXT  (gtkam_context_get_type ())
#define GTKAM_CONTEXT(o)    (GTK_CHECK_CAST((o),GTKAM_TYPE_CONTEXT,GtkamContext))
#define GTKAM_IS_CONTEXT(o) (GTK_CHECK_TYPE((o),GTKAM_TYPE_CONTEXT))

typedef struct _GtkamContext        GtkamContext;
typedef struct _GtkamContextPrivate GtkamContextPrivate;
typedef struct _GtkamContextClass   GtkamContextClass;

struct _GtkamContext
{
	GObject parent;

	GPContext *context;

	GPtrArray *errors;

	GtkamContextPrivate *priv;
};

struct _GtkamContextClass
{
	GObjectClass parent_class;
};

GType         gtkam_context_get_type (void);
GtkamContext *gtkam_context_new      (void);

#endif /* __GTKAM_CONTEXT_H__ */
