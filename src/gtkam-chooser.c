/* gtkam-chooser.c
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
#include "gtkam-chooser.h"

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
#include <stdlib.h>

#include <gtk/gtkvbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtktable.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkcombo.h>

#include <gphoto2/gphoto2-core.h>
#include <gphoto2/gphoto2-setting.h>

#include "support.h"
#include "gtkam-error.h"
#include "gtkam-close.h"

struct _GtkamChooserPrivate
{
	GtkWidget *label_speed;
	GtkEntry *entry_model, *entry_port, *entry_speed;
	GtkCombo *combo_port, *combo_speed;

	gboolean needs_update;
};

#define PARENT_TYPE GTK_TYPE_DIALOG
static GtkDialogClass *parent_class;

enum {
	CAMERA_SELECTED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void
gtkam_chooser_destroy (GtkObject *object)
{
	GtkamChooser *chooser = GTKAM_CHOOSER (object);

	chooser = NULL;

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_chooser_finalize (GtkObject *object)
{
	GtkamChooser *chooser = GTKAM_CHOOSER (object);

	g_free (chooser->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_chooser_class_init (GtkamChooserClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_chooser_destroy;
	object_class->finalize = gtkam_chooser_finalize;

	signals[CAMERA_SELECTED] = gtk_signal_new ("camera_selected",
		GTK_RUN_LAST, object_class->type,
		GTK_SIGNAL_OFFSET (GtkamChooserClass, camera_selected),
		gtk_marshal_NONE__POINTER, GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_chooser_init (GtkamChooser *chooser)
{
	chooser->priv = g_new0 (GtkamChooserPrivate, 1);
}

GtkType
gtkam_chooser_get_type (void)
{
	static GtkType chooser_type = 0;

	if (!chooser_type) {
		static const GtkTypeInfo chooser_info = {
			"GtkamChooser",
			sizeof (GtkamChooser),
			sizeof (GtkamChooserClass),
			(GtkClassInitFunc)  gtkam_chooser_class_init,
			(GtkObjectInitFunc) gtkam_chooser_init,
			NULL, NULL, NULL};
		chooser_type = gtk_type_unique (PARENT_TYPE, &chooser_info);
	}

	return (chooser_type);
}

static void
on_apply_clicked (GtkButton *button, GtkamChooser *chooser)
{
	Camera *camera;
	const gchar *model, *port, *speed;
	int result;
	GtkWidget *dialog;
	gchar *port_name, *msg;
	guint i;

	if (!chooser->priv->needs_update)
		return;

	model = gtk_entry_get_text (chooser->priv->entry_model);
	port  = gtk_entry_get_text (chooser->priv->entry_port);
	speed = gtk_entry_get_text (chooser->priv->entry_speed);

	if (!port || !*port)
		port_name = g_strdup (_("None"));
	else {
		port_name = g_strdup (port);
		for (i = 0; port_name[i] != '\0'; i++)
			if (port_name[i] == '(')
				break;
		port_name[i - 1] = '\0';
	}

	gp_camera_new (&camera);
	gp_camera_set_model (camera, model);
	if (strcmp (port_name, _("None")))
		gp_camera_set_port_name  (camera, port_name);
	if (strcmp (speed, _("Best")))
		gp_camera_set_port_speed (camera, atoi (speed));

	result = gp_camera_init (camera);
	if (result < 0) {
		msg = g_strdup_printf (_("Could not initialize '%s' "
				       "on port '%s'"), model, port);
		dialog = gtkam_error_new (msg, result, camera,
					  GTK_WIDGET (chooser));
		g_free (msg);
		gtk_widget_show (dialog);
	} else {
		gp_setting_set ("gtkam", "model", (char*) model);
		gp_setting_set ("gtkam", "port", port_name);
		gp_setting_set ("gtkam", "speed", (char*) speed);

		gtk_signal_emit (GTK_OBJECT (chooser),
				 signals[CAMERA_SELECTED], camera);
		gp_camera_unref (camera);
		chooser->priv->needs_update = FALSE;
	}

	g_free (port_name);
}

static void
on_close_clicked (GtkButton *button, GtkamChooser *chooser)
{
	gtk_object_destroy (GTK_OBJECT (chooser));
}


static void
on_ok_clicked (GtkButton *button, GtkamChooser *chooser)
{
	if (chooser->priv->needs_update)
		on_apply_clicked (button, chooser);
	on_close_clicked (button, chooser);
}

static void
on_more_options_toggled (GtkToggleButton *toggle, GtkamChooser *chooser)
{
	if (toggle->active) {
		gtk_widget_show (chooser->priv->label_speed);
		gtk_widget_show (GTK_WIDGET (chooser->priv->combo_speed));
	} else {
		gtk_widget_hide (chooser->priv->label_speed);
		gtk_widget_hide (GTK_WIDGET (chooser->priv->combo_speed));
	}
}

static void
on_model_changed (GtkEntry *entry, GtkamChooser *chooser)
{
	GtkWidget *dialog;
	CameraAbilities a;
	CameraPortInfo info;
	const gchar *model;
	gchar *msg;
	int count, result;
	guint i;
	GList *list;

	model = gtk_entry_get_text (entry);

	result = gp_camera_abilities_by_name (model, &a);
	if (result < 0) {
		msg = g_strdup_printf (_("Could not get abilities of "
					 "model '%s'"), model);
		dialog = gtkam_error_new (msg, result, NULL,
					  GTK_WIDGET (chooser));
		g_free (msg);
		gtk_widget_show (dialog);
		return;
	}

	count = gp_port_count_get ();
	if (count < 0) {
		dialog = gtkam_error_new (_("Could not get number of ports"),
					  count, NULL, GTK_WIDGET (chooser));
		gtk_widget_show (dialog);
		return;
	}

	list = NULL;
	for (i = 0; i < count; i++) {
		gp_port_info_get (i, &info);
		if (info.type & a.port)
			list = g_list_append (list,
					      g_strdup_printf ("%s (%s)",
						      info.name, info.path));
	}

	if (list) {
		gtk_combo_set_popdown_strings (chooser->priv->combo_port, list);
		gtk_widget_set_sensitive (
				GTK_WIDGET (chooser->priv->combo_port), TRUE);
	} else
		gtk_widget_set_sensitive (
				GTK_WIDGET (chooser->priv->combo_port), FALSE);

	list = g_list_append (NULL, _("Best"));
	for (i = 0; a.speed[i]; i++)
		list = g_list_append (list, g_strdup_printf ("%i", a.speed[i]));
	gtk_combo_set_popdown_strings (chooser->priv->combo_speed, list);
	gtk_widget_set_sensitive (GTK_WIDGET (chooser->priv->combo_speed),
				  TRUE);

	chooser->priv->needs_update = TRUE;
}

static void
on_port_changed (GtkEntry *entry, GtkamChooser *chooser)
{
	chooser->priv->needs_update = TRUE;
}

static void
on_speed_changed (GtkEntry *entry, GtkamChooser *chooser)
{
	chooser->priv->needs_update = TRUE;
}

static void
on_detect_clicked (GtkButton *button, GtkamChooser *chooser)
{
	GtkWidget *dialog;
	CameraList list;
	int result;
	const char *name;

	result = gp_autodetect (&list);
	if (result < 0) {
		dialog = gtkam_error_new (_("Could not detect any cameras"),
					  result, NULL, GTK_WIDGET (chooser));
		gtk_widget_show (dialog);
	} else if (!gp_list_count (&list)) {
		dialog = gtkam_close_new (_("No cameras detected."),
					  GTK_WIDGET (chooser));
		gtk_widget_show (dialog);
	} else {
//FIXME: Let user choose from the list
		gp_list_get_name (&list, 0, &name);
		gtk_entry_set_text (chooser->priv->entry_model, name);
		gtk_entry_set_text (chooser->priv->entry_port,
				    "Universal Serial Bus (usb:)");
	}
}

GtkWidget *
gtkam_chooser_new (Camera *opt_camera)
{
	GtkamChooser *chooser;
	GtkWidget *table, *label, *button, *combo, *dialog;
	const char *name;
	gint i;
	GList *list;
	int count;
	gchar *speed, *port;

	chooser = gtk_type_new (GTKAM_TYPE_CHOOSER);

	gtk_window_set_title (GTK_WINDOW (chooser), _("Select Camera"));
	gtk_container_set_border_width (GTK_CONTAINER (chooser), 5);
	gtk_window_set_policy (GTK_WINDOW (chooser), TRUE, TRUE, TRUE);

	table = gtk_table_new (3, 3, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table), 5);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (chooser)->vbox), table,
			    TRUE, TRUE, 0);

	label = gtk_label_new (_("Model:"));
	gtk_widget_show (label);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);

	combo = gtk_combo_new ();
	gtk_widget_show (combo);
	gtk_table_attach_defaults (GTK_TABLE (table), combo, 1, 2, 0, 1);
	chooser->priv->entry_model = GTK_ENTRY (GTK_COMBO (combo)->entry);

	list = NULL;
	count = gp_camera_count ();
	if (count < 0) {
		dialog = gtkam_error_new (_("Could not retrieve number of "
					  "cameras"), count, NULL, NULL);
		gtk_widget_show (dialog);
	} else {
		for (i = 0; i < count; i++) {
			gp_camera_name (i, &name);
			list = g_list_append (list, g_strdup (name));
		}
		if (list)
			gtk_combo_set_popdown_strings (GTK_COMBO (combo), list);
	}

	button = gtk_button_new_with_label (_("Detect"));
	gtk_widget_show (button);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 2, 3, 0, 1);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_detect_clicked), chooser);

	label = gtk_label_new (_("Port:"));
	gtk_widget_show (label);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);

	combo = gtk_combo_new ();
	gtk_widget_show (combo);
	gtk_table_attach_defaults (GTK_TABLE (table), combo, 1, 3, 1, 2);
	gtk_widget_set_sensitive (combo, FALSE);
	chooser->priv->entry_port = GTK_ENTRY (GTK_COMBO (combo)->entry);
	chooser->priv->combo_port = GTK_COMBO (combo);

	label = gtk_label_new (_("Speed:"));
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
	chooser->priv->label_speed = label;

	combo = gtk_combo_new ();
	gtk_table_attach_defaults (GTK_TABLE (table), combo, 1, 3, 2, 3);
	gtk_widget_set_sensitive (combo, FALSE);
	chooser->priv->entry_speed = GTK_ENTRY (GTK_COMBO (combo)->entry);
	gtk_entry_set_text (chooser->priv->entry_speed, _("Best"));
	chooser->priv->combo_speed = GTK_COMBO (combo);

	button = gtk_toggle_button_new_with_label (_("Enhanced"));
	gtk_widget_show (button);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (chooser)->action_area),
			   button);
	gtk_signal_connect (GTK_OBJECT (button), "toggled",
			    GTK_SIGNAL_FUNC (on_more_options_toggled), chooser);

	button = gtk_button_new_with_label (_("Ok"));
	gtk_widget_show (button);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (chooser)->action_area),
			   button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_ok_clicked), chooser);
	gtk_widget_grab_focus (button);

	button = gtk_button_new_with_label (_("Apply"));
	gtk_widget_show (button);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (chooser)->action_area),
			   button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_apply_clicked), chooser);

	button = gtk_button_new_with_label (_("Close"));
	gtk_widget_show (button);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (chooser)->action_area),
			   button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_close_clicked), chooser);

	gtk_signal_connect (GTK_OBJECT (chooser->priv->entry_model), "changed",
			    GTK_SIGNAL_FUNC (on_model_changed), chooser);
	gtk_signal_connect (GTK_OBJECT (chooser->priv->entry_port), "changed",
			    GTK_SIGNAL_FUNC (on_port_changed), chooser);
	gtk_signal_connect (GTK_OBJECT (chooser->priv->entry_speed), "changed",
			    GTK_SIGNAL_FUNC (on_speed_changed), chooser);

	if (opt_camera) {
		gtk_entry_set_text (chooser->priv->entry_model,
				    opt_camera->model);

		if (*opt_camera->port_info->name) {
			port = g_strdup_printf ("%s (%s)",
					opt_camera->port_info->name,
				        opt_camera->port_info->path);
			gtk_entry_set_text (chooser->priv->entry_port, port);
			g_free (port);
		}

		if (opt_camera->port_info->speed)
			speed = g_strdup_printf ("%i",
					opt_camera->port_info->speed);
		else
			speed = g_strdup_printf (_("Best"));
		gtk_entry_set_text (chooser->priv->entry_speed, speed);
		g_free (speed);

		chooser->priv->needs_update = FALSE;
	}

	return (GTK_WIDGET (chooser));
}
