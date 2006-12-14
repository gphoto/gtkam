/* gtkam-context.c
 *
 * Copyright © 2001 Lutz Müller <lutz@users.sourceforge.net>
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

#include "config.h"
#include "gtkam-context.h"
#include "i18n.h"

#include <string.h>
#include <stdlib.h>

#include <gtk/gtklabel.h>
#include <gtk/gtkstatusbar.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkvbox.h>

#define PARENT_TYPE G_TYPE_OBJECT
static GObjectClass *parent_class;

struct _GtkamContextPrivate {
   int notused;
};

static void
gtkam_context_finalize (GObject *object)
{
	GtkamContext *context = GTKAM_CONTEXT (object);
	guint i;

	if (context->context) {
		gp_context_unref (context->context);
		context->context = NULL;
	}

	if (context->errors) {
		for (i = 0; i < context->errors->len; i++)
			g_free (context->errors->pdata[i]);
		g_ptr_array_free (context->errors, TRUE);
		context->errors = NULL;
	}

	g_free (context->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_context_class_init (gpointer g_class, gpointer class_data)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (g_class);
	gobject_class->finalize = gtkam_context_finalize;

	parent_class = g_type_class_peek_parent (g_class);
}

static void
gtkam_context_init (GTypeInstance *instance, gpointer g_class)
{
	GtkamContext *context = GTKAM_CONTEXT (instance);

	context->context = gp_context_new ();
	context->errors  = g_ptr_array_new ();

	context->priv = g_new (GtkamContextPrivate, 1);
}

GType
gtkam_context_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo ti;

		memset (&ti, 0, sizeof (GTypeInfo)); 
		ti.class_size     = sizeof (GtkamContextClass);
		ti.class_init     = gtkam_context_class_init;
		ti.instance_size  = sizeof (GtkamContext);
		ti.instance_init  = gtkam_context_init;

		type = g_type_register_static (PARENT_TYPE, "GtkamContext",
					       &ti, 0);
	}

	return (type);
}

static void
error_func (GPContext *c, const char *format, va_list args, void *data)
{
	GtkamContext *context = GTKAM_CONTEXT (data);

	g_ptr_array_add (context->errors, g_strdup_vprintf (format, args));
}

GtkamContext *
gtkam_context_new (void)
{
	GtkamContext *context;

	context = g_object_new (GTKAM_TYPE_CONTEXT, NULL);

	gp_context_set_error_func (context->context, error_func, context);

	return (context);
}
