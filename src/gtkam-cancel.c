/* gtkam-cancel.c
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

#include "config.h"
#include "gtkam-cancel.h"
#include "i18n.h"

#include <string.h>

#include <gtk/gtkbutton.h>
#include <gtk/gtkmain.h>
#include <gtk/gtklabel.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkprogressbar.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkstock.h>

#include <gphoto2/gphoto2-result.h>

#include "gtkam-close.h"

struct _GtkamCancelPrivate
{
	GPtrArray *array_hbox, *array_progress;
	GArray *target;

	gboolean cancelled;
};

#define PARENT_TYPE GTKAM_TYPE_DIALOG
static GtkamDialogClass *parent_class;

enum {
	CANCEL,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void
gtkam_cancel_destroy (GtkObject *object)
{
	GtkamCancel *cancel = GTKAM_CANCEL (object);

	if (cancel->context) {
		g_object_unref (G_OBJECT (cancel->context));
		cancel->context = NULL;
	}

	g_ptr_array_set_size (cancel->priv->array_hbox, 0);
	g_ptr_array_set_size (cancel->priv->array_progress, 0);
	g_array_set_size (cancel->priv->target, 0);

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_cancel_finalize (GObject *object)
{
	GtkamCancel *cancel = GTKAM_CANCEL (object);

	g_array_free (cancel->priv->target, TRUE);
	g_ptr_array_free (cancel->priv->array_hbox, TRUE);
	g_ptr_array_free (cancel->priv->array_progress, TRUE);
	g_free (cancel->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_cancel_class_init (gpointer g_class, gpointer class_data)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = GTK_OBJECT_CLASS (g_class);
	object_class->destroy  = gtkam_cancel_destroy;

	gobject_class = G_OBJECT_CLASS (g_class);
	gobject_class->finalize = gtkam_cancel_finalize;

	signals[CANCEL] = g_signal_new ("cancel",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkamCancelClass, cancel), NULL, NULL,
		g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	parent_class = g_type_class_peek_parent (g_class);
}

static void
gtkam_cancel_init (GTypeInstance *instance, gpointer g_class)
{
	GtkamCancel *cancel = GTKAM_CANCEL (instance);

	cancel->context = gtkam_context_new ();

	cancel->priv = g_new0 (GtkamCancelPrivate, 1);
	cancel->priv->array_hbox = g_ptr_array_new ();
	cancel->priv->array_progress = g_ptr_array_new ();
	cancel->priv->target = g_array_new (FALSE, TRUE, sizeof (float));
}

GType
gtkam_cancel_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo ti;

		memset (&ti, 0, sizeof (GTypeInfo));
		ti.class_size     = sizeof (GtkamCancelClass);
		ti.class_init     = gtkam_cancel_class_init;
		ti.instance_size  = sizeof (GtkamCancel);
		ti.instance_init  = gtkam_cancel_init;

		type = g_type_register_static (PARENT_TYPE, "GtkamCancel",
					        &ti, 0);
	}

	return (type);
}

static void
on_cancel_clicked (GtkButton *button, GtkamCancel *cancel)
{
	cancel->priv->cancelled = TRUE;
	g_signal_emit (GTK_OBJECT (cancel), signals[CANCEL], 0);
}

static GPContextFeedback
cancel_func (GPContext *c, void *data)
{
	GtkamCancel *cancel = GTKAM_CANCEL (data);

	return (cancel->priv->cancelled ? GP_CONTEXT_FEEDBACK_CANCEL :
					  GP_CONTEXT_FEEDBACK_OK);
}

static void
message_func (GPContext *context, const char *format, va_list args,
              void *data)
{
	GtkamCancel *cancel = GTKAM_CANCEL (data);
        gchar *msg;
        GtkWidget *d;

	cancel = NULL;
        msg = g_strdup_vprintf (format, args);
        d = gtkam_close_new (msg);
        g_free (msg);
        gtk_widget_show (d);
}

static unsigned int
start_func (GPContext *c, float target, const char *format,
	    va_list args, void *data)
{
	GtkamCancel *cancel = GTKAM_CANCEL (data);
	GtkWidget *p, *label, *hbox;
	gchar *msg;
	unsigned int i;

	hbox = gtk_hbox_new (FALSE, 5);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTKAM_DIALOG (cancel)->vbox), hbox,
			    TRUE, TRUE, 0);

	msg = g_strdup_vprintf (format, args);
	label = gtk_label_new (msg);
	g_free (msg);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	p = gtk_progress_bar_new ();
	gtk_widget_show (p);
	gtk_box_pack_start (GTK_BOX (hbox), p, TRUE, TRUE, 0);

	for (i = 0; i < cancel->priv->array_progress->len; i++)
		if (!cancel->priv->array_progress->pdata[i])
			break;
	if (i == cancel->priv->array_progress->len) {
		g_ptr_array_add (cancel->priv->array_hbox, hbox);
		g_ptr_array_add (cancel->priv->array_progress, p);
		g_array_append_val (cancel->priv->target, target);
	} else {
		cancel->priv->array_hbox->pdata[i] = hbox;
		cancel->priv->array_progress->pdata[i] = p;
		g_array_index (cancel->priv->target, gfloat, i) = target;
	}

	return (cancel->priv->array_progress->len - 1);
}

static void
update_func (GPContext *c, unsigned int id, float current, void *data)
{
	GtkamCancel *cancel = GTKAM_CANCEL (data);
	GtkProgressBar *p;

	g_return_if_fail (id < cancel->priv->array_progress->len);

	p = cancel->priv->array_progress->pdata[id];
	gtk_progress_bar_set_fraction (p, 
		current / g_array_index (cancel->priv->target, float, id));

	while (gtk_events_pending ())
		gtk_main_iteration ();
}

static void
stop_func (GPContext *c, unsigned int id, void *data)
{
	GtkamCancel *cancel = GTKAM_CANCEL (data);

	g_return_if_fail (id < cancel->priv->array_progress->len);

	gtk_object_destroy (GTK_OBJECT (cancel->priv->array_hbox->pdata[id]));

	cancel->priv->array_hbox->pdata[id] = NULL;
	cancel->priv->array_progress->pdata[id] = NULL;
	g_array_index (cancel->priv->target, gfloat, id) = 0.;
}

static void
idle_func (GPContext *context, void *data)
{
	while (gtk_events_pending ())
		gtk_main_iteration ();
}

GtkWidget *
gtkam_cancel_new (const gchar *format, ...)
{
	GtkamCancel *cancel;
	GtkWidget *label, *button;
	va_list args;
	gchar *msg;

	cancel = g_object_new (GTKAM_TYPE_CANCEL, NULL);

	va_start (args, format);
	msg = g_strdup_vprintf (format, args);
	va_end (args);
	label = gtk_label_new (msg);
	g_free (msg);
	gtk_widget_show (label);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (label), 0., 0.);
	gtk_box_pack_start (GTK_BOX (GTKAM_DIALOG (cancel)->vbox),
			    label, TRUE, TRUE, 0);

	button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
	gtk_widget_show (button);
	g_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_cancel_clicked), cancel);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (cancel)->action_area),
			   button);
	gtk_widget_grab_focus (button);

	gp_context_set_progress_funcs (cancel->context->context, start_func,
				       update_func, stop_func, cancel);
	gp_context_set_idle_func (cancel->context->context, idle_func, cancel);
	gp_context_set_cancel_func (cancel->context->context,
				    cancel_func, cancel);
	gp_context_set_message_func (cancel->context->context,
				     message_func, cancel);

	return (GTK_WIDGET (cancel));
}
