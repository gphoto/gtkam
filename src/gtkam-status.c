/* gtkam-status.c
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
#include "gtkam-status.h"
#include "i18n.h"

#include <stdlib.h>
#include <string.h>

#include <gtk/gtklabel.h>
#include <gtk/gtkstatusbar.h>
#include <gtk/gtkprogressbar.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkstock.h>

#include "gtkam-close.h"

#define PARENT_TYPE GTK_TYPE_HBOX
static GtkHBoxClass *parent_class;

struct _GtkamStatusPrivate {
	GtkWidget *status;

	guint message_id;

	GPtrArray *progress;
	GArray *target;

	gboolean cancel;
};

static void
gtkam_status_destroy (GtkObject *object)
{
	GtkamStatus *status = GTKAM_STATUS (object);

	if (status->context) {
		g_object_unref (G_OBJECT (status->context));
		status->context = NULL;
	}

	g_ptr_array_set_size (status->priv->progress, 0);
	g_array_set_size (status->priv->target, 0);

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_status_finalize (GObject *object)
{
	GtkamStatus *status = GTKAM_STATUS (object);

	g_ptr_array_free (status->priv->progress, TRUE);
	g_array_free (status->priv->target, TRUE);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_status_class_init (gpointer g_class, gpointer class_data)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = GTK_OBJECT_CLASS (g_class);
	object_class->destroy  = gtkam_status_destroy;

	gobject_class = G_OBJECT_CLASS (g_class);
	gobject_class->finalize = gtkam_status_finalize;

	parent_class = g_type_class_peek_parent (g_class);
}

static void
gtkam_status_init (GTypeInstance *instance, gpointer g_class)
{
	GtkamStatus *status = GTKAM_STATUS (instance);

	status->context = gtkam_context_new ();

	status->priv = g_new0 (GtkamStatusPrivate, 1);
	status->priv->progress = g_ptr_array_new ();
	status->priv->target = g_array_new (FALSE, TRUE, sizeof (float));
}

GType
gtkam_status_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo ti;

		memset (&ti, 0, sizeof (GTypeInfo));
		ti.class_size    = sizeof (GtkamStatusClass);
		ti.class_init    = gtkam_status_class_init;
		ti.instance_size = sizeof (GtkamStatus);
		ti.instance_init = gtkam_status_init;

		type = g_type_register_static (PARENT_TYPE, "GtkamStatus",
					       &ti, 0);
	}

	return (type);
}

static void 
status_func (GPContext *c, const char *format, va_list args,
             void *data)
{
        GtkamStatus *status = GTKAM_STATUS (data);
        gchar *msg;

        /* Remove old status messages */
        if (status->priv->message_id)
                gtk_statusbar_remove (GTK_STATUSBAR (status->priv->status), 0,
                                      status->priv->message_id);
	else
		gtk_widget_show (status->priv->status);

        msg = g_strdup_vprintf (format, args);
        status->priv->message_id = gtk_statusbar_push (
                        GTK_STATUSBAR (status->priv->status), 0, msg);
        g_free (msg);
}

static void
message_func (GPContext *context, const char *format, va_list args,
	      void *data)
{
	GtkamStatus *status = GTKAM_STATUS (data);
	gchar *msg;
	GtkWidget *d;

	status = NULL;

	msg = g_strdup_vprintf (format, args);
	d = gtkam_close_new (msg);
	g_free (msg);
	gtk_widget_show (d);
}

static unsigned int
start_func (GPContext *c, float target, const char *format,
            va_list args, void *data)
{
        GtkamStatus *status = GTKAM_STATUS (data);
        GtkWidget *progress;
        gchar *msg, *msg_translated;
	guint i;

        progress = gtk_progress_bar_new ();
        gtk_box_pack_start (GTK_BOX (status), progress, FALSE, FALSE, 0);
	gtk_widget_show (progress);

	for (i = 0; i < status->priv->progress->len; i++)
		if (!status->priv->progress->pdata[i])
			break;
	if (i == status->priv->progress->len) {
	        g_ptr_array_add (status->priv->progress, progress);
	        g_array_append_val (status->priv->target, target);
	} else {
		status->priv->progress->pdata[i] = progress;
		g_array_index (status->priv->target, gfloat, i) = target;
	}

        msg = g_strdup_vprintf (format, args);
	msg_translated = g_locale_to_utf8 (msg, -1, NULL, NULL, NULL);
	g_free (msg);
        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress), msg_translated);
        g_free (msg_translated);

        return (status->priv->progress->len - 1);
}

static void
update_func (GPContext *c, unsigned int id, float current, void *data)
{
        GtkamStatus *status = GTKAM_STATUS (data);
        GtkProgressBar *p;
	gfloat target;

        g_return_if_fail (id < status->priv->progress->len);

	target = g_array_index (status->priv->target, gfloat, id);
	if (current > target) {
		g_warning ("ID %i: target %f <-> current %f", id,
			   target, current);
		return;
	}

        p = status->priv->progress->pdata[id];
	gtk_progress_bar_set_fraction (p, 
		current / g_array_index (status->priv->target, float, id));

	while (gtk_events_pending ())
		gtk_main_iteration ();
}

static void
stop_func (GPContext *c, unsigned int id, void *data)
{
        GtkamStatus *status = GTKAM_STATUS (data);

        g_return_if_fail (id < status->priv->progress->len);

	gtk_object_destroy (GTK_OBJECT (status->priv->progress->pdata[id]));
	status->priv->progress->pdata[id] = NULL;
	g_array_index (status->priv->target, gfloat, id) = 0.;
}

static void
on_cancel_clicked (GtkButton *button, GtkamStatus *status)
{
        status->priv->cancel = TRUE;
}

static GPContextFeedback
cancel_func (GPContext *c, void *data)
{
        GtkamStatus *status = GTKAM_STATUS (data);

        return (status->priv->cancel ? GP_CONTEXT_FEEDBACK_CANCEL :
				       GP_CONTEXT_FEEDBACK_OK);
}

static void
idle_func (GPContext *c, void *data)
{
	while (gtk_events_pending ())
		gtk_main_iteration ();
}

GtkWidget *
gtkam_status_new (const gchar *format, ...)
{
	GtkamStatus *status;
	va_list args;
	gchar *msg;
	GtkWidget *label, *button;
	

	status = g_object_new (GTKAM_TYPE_STATUS, NULL);

	va_start (args, format);
	msg = g_strdup_vprintf (format, args);
	va_end (args);
	label = gtk_label_new (msg);
	g_free (msg);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (status), label, FALSE, FALSE, 0);

	/* Cancellation */
	button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
	gtk_widget_show (button);
	gtk_box_pack_end (GTK_BOX (status), button, FALSE, FALSE, 0);
	g_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_cancel_clicked), status);
	gp_context_set_cancel_func (status->context->context,
				    cancel_func, status);

	/* Status information */
	status->priv->status = gtk_statusbar_new ();
	gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (status->priv->status),
					   FALSE);
	gtk_box_pack_start (GTK_BOX (status), status->priv->status,
			    TRUE, TRUE, 0);
	gp_context_set_status_func (status->context->context,
				    status_func, status);

	/* Progress information */
	gp_context_set_progress_funcs (status->context->context, start_func,
				       update_func, stop_func, status);

	/* Messages */
	gp_context_set_message_func (status->context->context, message_func,
				     status);

	/* Idle */
	gp_context_set_idle_func (status->context->context, idle_func,
				  status);

	return (GTK_WIDGET (status));
}
