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

#include "gtkam-close.h"

#define PARENT_TYPE GTK_TYPE_HBOX
static GtkHBoxClass *parent_class;

struct _GtkamStatusPrivate {
	GtkWidget *status;

	guint context_id, message_id;

	GPtrArray *progress;
	GArray *target;

	gboolean cancel;
};

static void
gtkam_status_destroy (GtkObject *object)
{
	GtkamStatus *status = GTKAM_STATUS (object);

	if (status->context) {
		gtk_object_unref (GTK_OBJECT (status->context));
		status->context = NULL;
	}

	g_ptr_array_set_size (status->priv->progress, 0);
	g_array_set_size (status->priv->target, 0);

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_status_finalize (GtkObject *object)
{
	GtkamStatus *status = GTKAM_STATUS (object);

	g_ptr_array_free (status->priv->progress, TRUE);
	g_array_free (status->priv->target, TRUE);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_status_class_init (GtkamStatusClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_status_destroy;
	object_class->finalize = gtkam_status_finalize;

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_status_init (GtkamStatus *status)
{
	status->context = gtkam_context_new ();

	status->priv = g_new0 (GtkamStatusPrivate, 1);
	status->priv->progress = g_ptr_array_new ();
	status->priv->target = g_array_new (FALSE, TRUE, sizeof (gfloat));
}

GtkType
gtkam_status_get_type (void)
{
        static GtkType status_type = 0;

        if (!status_type) {
                static const GtkTypeInfo status_info = {
                        "GtkamStatus",
                        sizeof (GtkamStatus),
                        sizeof (GtkamStatusClass),
                        (GtkClassInitFunc)  gtkam_status_class_init,
                        (GtkObjectInitFunc) gtkam_status_init,
                        NULL, NULL, NULL};
		status_type = gtk_type_unique (PARENT_TYPE, &status_info);
        }

        return (status_type);
}

static void 
status_func (GPContext *c, const char *format, va_list args,
             void *data)
{
        GtkamStatus *status = GTKAM_STATUS (data);
        gchar *msg;

        /* Remove old status messages */
        if (status->priv->message_id)
                gtk_statusbar_remove (GTK_STATUSBAR (status->priv->status),
                                      status->priv->context_id,
                                      status->priv->message_id);

        msg = g_strdup_vprintf (format, args);
        status->priv->message_id = gtk_statusbar_push (
                        GTK_STATUSBAR (status->priv->status),
                        status->priv->context_id, msg);
        g_free (msg);
        while (gtk_events_pending ())
                gtk_main_iteration ();
}

static void
message_func (GPContext *context, const char *format, va_list args,
	      void *data)
{
	GtkamStatus *status = GTKAM_STATUS (data);
	gchar *msg;
	GtkWidget *d, *w;

	w = gtk_widget_get_ancestor (GTK_WIDGET (status), GTK_TYPE_WINDOW);
	msg = g_strdup_vprintf (format, args);
	d = gtkam_close_new (msg, w);
	g_free (msg);
	gtk_widget_show (d);
}

static unsigned int
start_func (GPContext *c, float target, const char *format,
            va_list args, void *data)
{
        GtkamStatus *status = GTKAM_STATUS (data);
        GtkWidget *progress;
        gchar *msg;
	guint i;

        progress = gtk_progress_bar_new ();
        gtk_widget_show (progress);
        gtk_box_pack_start (GTK_BOX (status), progress, FALSE, FALSE, 0);

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
        gtk_progress_set_format_string (GTK_PROGRESS (progress), msg);
        g_free (msg);
        gtk_progress_set_show_text (GTK_PROGRESS (progress), TRUE);

	while (gtk_events_pending ())
		gtk_main_iteration ();

        return (status->priv->progress->len - 1);
}

static void
update_func (GPContext *c, unsigned int id, float current, void *data)
{
        GtkamStatus *status = GTKAM_STATUS (data);
        GtkProgress *progress;
	gfloat target;

        g_return_if_fail (id < status->priv->progress->len);

	target = g_array_index (status->priv->target, gfloat, id);
	if (current > target) {
		g_warning ("ID %i: target %f <-> current %f", id,
			   target, current);
		return;
	}

        progress = status->priv->progress->pdata[id];
        gtk_progress_set_percentage (progress, current / target);

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

	while (gtk_events_pending ())
		gtk_main_iteration ();

        return (status->priv->cancel ? GP_CONTEXT_FEEDBACK_CANCEL :
				       GP_CONTEXT_FEEDBACK_OK);
}

GtkWidget *
gtkam_status_new (const gchar *format, ...)
{
	GtkamStatus *status;
	va_list args;
	gchar *msg;
	GtkWidget *label, *button;
	

	status = gtk_type_new (GTKAM_TYPE_STATUS);

	va_start (args, format);
	msg = g_strdup_vprintf (format, args);
	va_end (args);
	label = gtk_label_new (msg);
	g_free (msg);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (status), label, FALSE, FALSE, 0);

	/* Cancellation */
	button = gtk_button_new_with_label (_("Cancel"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (status), button,
			    FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_cancel_clicked), status);
	gp_context_set_cancel_func (status->context->context,
				    cancel_func, status);

	/* Status information */
	status->priv->status = gtk_statusbar_new ();
	gtk_widget_show (status->priv->status);
	gtk_box_pack_start (GTK_BOX (status), status->priv->status,
			    TRUE, TRUE, 0);
	status->priv->context_id = gtk_statusbar_get_context_id (
			GTK_STATUSBAR (status->priv->status), "libgphoto2");
	gp_context_set_status_func (status->context->context,
				    status_func, status);

	/* Progress information */
	gp_context_set_progress_funcs (status->context->context, start_func,
				       update_func, stop_func, status);

	/* Messages */
	gp_context_set_message_func (status->context->context, message_func,
				     status);

	return (GTK_WIDGET (status));
}
