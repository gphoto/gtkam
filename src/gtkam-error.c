/* gtkam-error.c
 *
 * Copyright (C) 2001 Lutz Müller <urc8@rz.uni-karlsruhe.de>
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
#include "gtkam-error.h"

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

#include <stdio.h>

#include <gtk/gtktext.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkvscrollbar.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtktogglebutton.h>

#include <gphoto2/gphoto2-result.h>
#include <gphoto2/gphoto2-port-log.h>

struct _GtkamErrorPrivate
{
	GtkWidget *hbox;
};

#define PARENT_TYPE GTK_TYPE_DIALOG
static GtkDialogClass *parent_class;

static void
gtkam_error_destroy (GtkObject *object)
{
	GtkamError *error = GTKAM_ERROR (object);

	error = NULL;

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_error_finalize (GtkObject *object)
{
	GtkamError *error = GTKAM_ERROR (object);

	g_free (error->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_error_class_init (GtkamErrorClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_error_destroy;
	object_class->finalize = gtkam_error_finalize;

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_error_init (GtkamError *error)
{
	error->priv = g_new0 (GtkamErrorPrivate, 1);
}

GtkType
gtkam_error_get_type (void)
{
	static GtkType error_type = 0;

	if (!error_type) {
		static const GtkTypeInfo error_info = {
			"GtkamError",
			sizeof (GtkamError),
			sizeof (GtkamErrorClass),
			(GtkClassInitFunc)  gtkam_error_class_init,
			(GtkObjectInitFunc) gtkam_error_init,
			NULL, NULL, NULL};
		error_type = gtk_type_unique (PARENT_TYPE, &error_info);
	}

	return (error_type);
}

static void
on_error_close_clicked (GtkButton *button, GtkamError *error)
{
	gtk_object_destroy (GTK_OBJECT (error));
}

static void
on_debug_toggled (GtkToggleButton *toggle, GtkamError *error)
{
	if (toggle->active && !GTK_WIDGET_VISIBLE (error->priv->hbox))
		gtk_widget_show (error->priv->hbox);

	if (!toggle->active && GTK_WIDGET_VISIBLE (error->priv->hbox))
		gtk_widget_hide (error->priv->hbox);
}

GtkWidget *
gtkam_error_new (const gchar *msg, int result, Camera *opt_camera,
		 GtkWidget *opt_window)
{
	GtkamError *error;
	GtkWidget *text, *vscrollbar, *button, *label;
	const char *history;
	const char *gmsg;
	gchar *full_msg;

	g_return_val_if_fail (result < 0, NULL);

	error = gtk_type_new (GTKAM_TYPE_ERROR);
	gtk_window_set_policy (GTK_WINDOW (error), TRUE, TRUE, TRUE);

	if (opt_camera)
		gmsg = gp_camera_get_result_as_string (opt_camera, result);
	else
		gmsg = gp_result_as_string (result);

	full_msg = g_strdup_printf ("%s:\n\n'%s'", msg, gmsg);
	label = gtk_label_new (full_msg);
	g_free (full_msg);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (error)->vbox), label,
			    FALSE, FALSE, 0);

	error->priv->hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (error)->vbox),
			    error->priv->hbox, TRUE, TRUE, 0);

	text = gtk_text_new (NULL, NULL);
	gtk_widget_show (text);
	gtk_text_set_editable (GTK_TEXT (text), FALSE);
	gtk_box_pack_start (GTK_BOX (error->priv->hbox), text, TRUE, TRUE, 0);

	history = gp_log_history_get ();
	gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL,
			 history, strlen (history));

	vscrollbar = gtk_vscrollbar_new (GTK_TEXT (text)->vadj);
	gtk_widget_show (vscrollbar);
	gtk_box_pack_end (GTK_BOX (error->priv->hbox),
			  vscrollbar, FALSE, FALSE, 0);

	button = gtk_toggle_button_new_with_label (
					_("Show debugging messages"));
	gtk_widget_show (button);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (error)->action_area),
			   button);
	gtk_signal_connect (GTK_OBJECT (button), "toggled",
			    GTK_SIGNAL_FUNC (on_debug_toggled), error); 

	button = gtk_button_new_with_label (_("Close"));
	gtk_widget_show (button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_error_close_clicked), error);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (error)->action_area),
			   button);
	gtk_widget_grab_focus (button);

	if (opt_window)
		gtk_window_set_transient_for (GTK_WINDOW (error),
					      GTK_WINDOW (opt_window));

	return (GTK_WIDGET (error));
}
