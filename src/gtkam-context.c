/* gtkam-context.c
 *
 * Copyright (C) 2001 Lutz Müller <lutz@users.sourceforge.net>
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

#include <config.h>
#include "gtkam-context.h"

#include <gtk/gtklabel.h>
#include <gtk/gtkstatusbar.h>
#include <gtk/gtkprogressbar.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkvbox.h>

#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif

#define PARENT_TYPE GTK_TYPE_OBJECT
static GtkObjectClass *parent_class;

struct _GtkamContextPrivate {
};

static void
gtkam_context_destroy (GtkObject *object)
{
	GtkamContext *context = GTKAM_CONTEXT (object);
	guint i;

	if (context->context) {
		gp_context_unref (context->context);
		context->context = NULL;
	}

	for (i = 0; i < context->errors->len; i++)
		g_free (context->errors->pdata[i]);
	g_ptr_array_set_size (context->errors, 0);

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_context_finalize (GtkObject *object)
{
	GtkamContext *context = GTKAM_CONTEXT (object);

	if (context->errors) {
		g_ptr_array_free (context->errors, TRUE);
		context->errors = NULL;
	}

	g_free (context->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_context_class_init (GtkamContextClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_context_destroy;
	object_class->finalize = gtkam_context_finalize;

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_context_init (GtkamContext *context)
{
	context->context = gp_context_new ();
	context->errors  = g_ptr_array_new ();

	context->priv = g_new (GtkamContextPrivate, 1);
}

GtkType
gtkam_context_get_type (void)
{
	static GtkType context_type = 0;

	if (!context_type) {
		static const GtkTypeInfo context_info = {
			"GtkamContext",
			sizeof (GtkamContext),
			sizeof (GtkamContextClass),
			(GtkClassInitFunc)  gtkam_context_class_init,
			(GtkObjectInitFunc) gtkam_context_init,
			NULL, NULL, NULL};
		context_type = gtk_type_unique (PARENT_TYPE, &context_info);
	}

	return (context_type);
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

	context = gtk_type_new (GTKAM_TYPE_CONTEXT);

	gp_context_set_error_func (context->context, error_func, context);

	return (context);
}
