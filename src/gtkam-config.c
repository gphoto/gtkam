/* gtkam-config.c
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
#include "gtkam-config.h"

#include <stdio.h>

#include <gtk/gtktooltips.h>
#include <gtk/gtkframe.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkhscale.h>
#include <gtk/gtkcombo.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkbutton.h>

#include <gtkam-error.h>

struct _GtkamConfigPrivate
{
	GtkTooltips *tooltips;
	GtkWidget *notebook;
	GtkWidget *orphans;

	GHashTable *hash;

	Camera       *camera;
	CameraWidget *config;
};

#define PARENT_TYPE GTK_TYPE_DIALOG
static GtkDialogClass *parent_class;

static void
gtkam_config_destroy (GtkObject *object)
{
	GtkamConfig *config = GTKAM_CONFIG (object);

	if (config->priv->camera) {
		gp_camera_unref (config->priv->camera);
		config->priv->camera = NULL;
	}

	if (config->priv->config) {
		gp_widget_unref (config->priv->config);
		config->priv->config = NULL;
	}

	if (config->priv->hash) {
		g_hash_table_destroy (config->priv->hash);
		config->priv->hash = NULL;
	}

	if (config->priv->tooltips) {
		gtk_object_unref (GTK_OBJECT (config->priv->tooltips));
		config->priv->tooltips = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_config_finalize (GtkObject *object)
{
	GtkamConfig *config = GTKAM_CONFIG (object);

	g_free (config->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_config_class_init (GtkamConfigClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_config_destroy;
	object_class->finalize = gtkam_config_finalize;

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_config_init (GtkamConfig *config)
{
	config->priv = g_new0 (GtkamConfigPrivate, 1);
	config->priv->hash = g_hash_table_new (g_direct_hash, g_direct_equal);
	config->priv->tooltips = gtk_tooltips_new ();
}

GtkType
gtkam_config_get_type (void)
{
	static GtkType config_type = 0;

	if (!config_type) {
		static const GtkTypeInfo config_info = {
			"GtkamConfig",
			sizeof (GtkamConfig),
			sizeof (GtkamConfigClass),
			(GtkClassInitFunc)  gtkam_config_class_init,
			(GtkObjectInitFunc) gtkam_config_init,
			NULL, NULL, NULL};
		config_type = gtk_type_unique (PARENT_TYPE, &config_info);
	}

	return (config_type);
}

static void
gtkam_config_apply (GtkamConfig *config)
{
	int result;
	GtkWidget *dialog;

	result = gp_camera_set_config (config->priv->camera,
				       config->priv->config);
	if (result != GP_OK) {
		dialog = gtkam_error_new ("Could not apply configuration",
					  result, config->priv->camera,
					  GTK_WIDGET (config));
		gtk_widget_show (dialog);
	}
}

static void
on_config_ok_clicked (GtkButton *button, GtkamConfig *config)
{
	gtkam_config_apply (config);
	gtk_object_destroy (GTK_OBJECT (config));
}

static void
on_config_apply_clicked (GtkButton *button, GtkamConfig *config)
{
	gtkam_config_apply (config);
}

static void
on_config_close_clicked (GtkButton *button, GtkamConfig *config)
{
	gtk_object_destroy (GTK_OBJECT (config));
}

static GtkWidget *
create_page (GtkamConfig *config, CameraWidget *widget)
{
	GtkWidget *label, *vbox;
	int id;
	const char *l, *info;

	/* Label */
	if (widget) {
		gp_widget_get_info  (widget, &info);
		gp_widget_get_label (widget, &l);
		label = gtk_label_new (l);
		gtk_tooltips_set_tip (config->priv->tooltips, label, info,
				      NULL);
	} else
		label = gtk_label_new ("Others");
	gtk_widget_show (label);

	/* VBox */
	vbox = gtk_vbox_new (FALSE, 10);
	gtk_widget_show (vbox);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
	gtk_notebook_append_page (GTK_NOTEBOOK (config->priv->notebook), vbox,
				  label);

	if (widget) {
		gp_widget_get_id (widget, &id);
		g_hash_table_insert (config->priv->hash, GINT_TO_POINTER (id),
				     vbox);
	} else
		config->priv->orphans = vbox;

	return (vbox);
}

static void
on_toggle_button_toggled (GtkToggleButton *toggle, CameraWidget *widget)
{
	int value, value_new;

	gp_widget_get_value (widget, &value);
	value_new = (toggle->active ? 1 : 0);
	if (value != value_new)
		gp_widget_set_value (widget, &value_new);
}

static void
on_radio_button_toggled (GtkToggleButton *toggle, CameraWidget *widget)
{
	char *value = NULL, *value_new;

	if (!toggle->active)
		return;

	gp_widget_get_value (widget, &value);
	value_new = gtk_object_get_data (GTK_OBJECT (toggle), "value");
	if (!value || strcmp (value, value_new))
		gp_widget_set_value (widget, value_new);
}

static void
on_button_clicked (GtkButton *button, CameraWidget *widget)
{
	GtkamConfig *config;
	CameraWidgetCallback callback;
	int result;
	GtkWidget *dialog;

	config = GTKAM_CONFIG (gtk_widget_get_ancestor (GTK_WIDGET (button),
							GTKAM_TYPE_CONFIG));
	gp_widget_get_value (widget, &callback);
	result = callback (config->priv->camera, widget);
	if (result != GP_OK) {
		dialog = gtkam_error_new ("Could not execute command",
					  result, config->priv->camera,
					  GTK_WIDGET (config));
		gtk_widget_show (dialog);
	}
}

static void
on_entry_changed (GtkEntry *entry, CameraWidget *widget)
{
	char *value = NULL, *value_new;

	gp_widget_get_value (widget, &value);
	value_new = gtk_entry_get_text (entry);
	if (!value || strcmp (value, value_new))
		gp_widget_set_value (widget, value_new);
}

static void
on_adjustment_value_changed (GtkAdjustment *adj, CameraWidget *widget)
{
	float value, value_new;

	gp_widget_get_value (widget, &value);
	value_new = adj->value;
	if (value != value_new)
		gp_widget_set_value (widget, &value_new);
}

static void
create_widgets (GtkamConfig *config, CameraWidget *widget)
{
	CameraWidget *parent, *child;
	CameraWidgetType type;
	const gchar *label, *info;
	char *value_char;
	int value_int;
	float value_float, min, max, increment;
	const char *choice;
	GtkWidget *vbox, *button, *gtk_widget = NULL, *frame;
	GtkObject *adjustment;
	GSList *group = NULL;
	GList *options = NULL;
	gint i, id;

	gp_widget_get_label (widget, &label);
	gp_widget_get_info  (widget, &info);
	gp_widget_get_type  (widget, &type);

	switch (type) {
	case GP_WIDGET_WINDOW:
	case GP_WIDGET_SECTION:

		/* If section, create page */
		if (type == GP_WIDGET_SECTION)
			create_page (config, widget);

		/* Create sub-widgets */
		for (i = 0; i < gp_widget_count_children (widget); i++) {
			gp_widget_get_child (widget, i, &child);
			create_widgets (config, child);
		}

		return;
	
	case GP_WIDGET_BUTTON:

		gtk_widget = gtk_vbox_new (FALSE, 10);
		gtk_container_set_border_width (GTK_CONTAINER (gtk_widget), 10);
		button = gtk_button_new_with_label (label);
		gtk_widget_show (button);
		gtk_signal_connect (GTK_OBJECT (button), "clicked",
				GTK_SIGNAL_FUNC (on_button_clicked), widget);
		gtk_container_add (GTK_CONTAINER (gtk_widget), button);
		gtk_tooltips_set_tip (config->priv->tooltips, button,
				      info, NULL);
		break;

	case GP_WIDGET_DATE:

		gtk_widget = gtk_label_new ("Date & Time not implemented!");
		break;

	case GP_WIDGET_TEXT:

		gp_widget_get_value (widget, &value_char);
		gtk_widget = gtk_entry_new ();
		if (value_char)
			gtk_entry_set_text (GTK_ENTRY (gtk_widget), value_char);
		gtk_signal_connect (GTK_OBJECT (gtk_widget), "changed",
				GTK_SIGNAL_FUNC (on_entry_changed), widget);
		gtk_tooltips_set_tip (config->priv->tooltips,
				      gtk_widget, info, NULL);
		break;
	
	case GP_WIDGET_RANGE:

		gp_widget_get_value (widget, &value_float);
		gp_widget_get_range (widget, &min, &max, &increment);
		adjustment = gtk_adjustment_new (value_float, min, max,
						 increment, 0, 0);
		gtk_signal_connect (adjustment, "value_changed",
			GTK_SIGNAL_FUNC (on_adjustment_value_changed), widget);
		gtk_widget = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
		gtk_scale_set_digits (GTK_SCALE (gtk_widget), 0);
		gtk_range_set_update_policy (GTK_RANGE (gtk_widget),
					     GTK_UPDATE_DISCONTINUOUS);
		gtk_tooltips_set_tip (config->priv->tooltips,
				      gtk_widget, info, NULL);
		break;

	case GP_WIDGET_MENU:

		gtk_widget = gtk_combo_new ();
		gp_widget_get_value (widget, &value_char);
		for (i = 0; i < gp_widget_count_choices (widget); i++) {
			gp_widget_get_choice (widget, i, &choice);
			options = g_list_append (options, g_strdup (choice));
		}
		gtk_combo_set_popdown_strings (GTK_COMBO (gtk_widget), options);
		gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (gtk_widget)->entry),
				    value_char);
		break;

	case GP_WIDGET_RADIO:

		if (gp_widget_count_choices (widget) < 6)
			gtk_widget = gtk_hbox_new (FALSE, 5);
		else
			gtk_widget = gtk_vbox_new (FALSE, 5);
		gp_widget_get_value (widget, &value_char);
		for (i = 0; i < gp_widget_count_choices (widget); i++) {
			gp_widget_get_choice (widget, i, &choice);
			button = gtk_radio_button_new_with_label (group,
								  choice);
			gtk_widget_show (button);
			group = gtk_radio_button_group (
					GTK_RADIO_BUTTON (button));
			gtk_object_set_data (GTK_OBJECT (button), "value",
					     (char*) choice);
			gtk_box_pack_start (GTK_BOX (gtk_widget), button,
					    FALSE, FALSE, 0);
			if (value_char && !strcmp (value_char, choice))
				gtk_toggle_button_set_active (
					GTK_TOGGLE_BUTTON (button), TRUE);
			gtk_signal_connect (GTK_OBJECT (button), "toggled",
				GTK_SIGNAL_FUNC (on_radio_button_toggled),
				widget);
			gtk_tooltips_set_tip (config->priv->tooltips,
					      button, info, NULL);
		}
		break;

	case GP_WIDGET_TOGGLE:

		gtk_widget = gtk_check_button_new_with_label (label);
		gp_widget_get_value (widget, &value_int);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_widget),
					      (value_int != 0));
		gtk_signal_connect (GTK_OBJECT (gtk_widget), "toggled",
				    GTK_SIGNAL_FUNC (on_toggle_button_toggled),
				    widget);
		gtk_tooltips_set_tip (config->priv->tooltips,
				      gtk_widget, info, NULL);
		break;

	default:

		g_warning ("Widget '%s' if of unknown type!", label);
		return;
	}

	gtk_widget_show (gtk_widget);
	frame = gtk_frame_new (label);
	gtk_widget_show (frame);
	gtk_container_add (GTK_CONTAINER (frame), gtk_widget);

	gp_widget_get_parent (widget, &parent);
	gp_widget_get_type (parent, &type);

	if (type == GP_WIDGET_SECTION) {
		gp_widget_get_parent (widget, &parent);
		gp_widget_get_id (parent, &id);
		vbox = g_hash_table_lookup (config->priv->hash,
					    GINT_TO_POINTER (id));
	} else {
		if (!config->priv->orphans)
			create_page (config, NULL);
		vbox = config->priv->orphans;
	}
	
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
}

GtkWidget *
gtkam_config_new (Camera *camera)
{
	GtkamConfig *config;
	GtkWidget *button, *dialog;
	CameraWidget *config_widget;
	int result;

	g_return_val_if_fail (camera != NULL, NULL);

	result = gp_camera_get_config (camera, &config_widget);
	if (result != GP_OK) {
		dialog = gtkam_error_new ("Could not get configuration",
					  result, camera, NULL);
		gtk_widget_show (dialog);
		return (NULL);
	}

	config = gtk_type_new (GTKAM_TYPE_CONFIG);

	config->priv->camera = camera;
	config->priv->config = config_widget;
	gp_camera_ref (camera);

	config->priv->notebook = gtk_notebook_new ();
	gtk_widget_show (config->priv->notebook);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (config)->vbox),
			    config->priv->notebook, TRUE, TRUE, 0);
	create_widgets (config, config_widget);

	button = gtk_button_new_with_label ("Ok");
	gtk_widget_show (button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_config_ok_clicked), config);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (config)->action_area),
			   button);

	button = gtk_button_new_with_label ("Apply");
	gtk_widget_show (button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_config_apply_clicked), config);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (config)->action_area),
			   button);

	button = gtk_button_new_with_label ("Close");
	gtk_widget_show (button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_config_close_clicked), config);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (config)->action_area),
			   button);
	gtk_widget_grab_focus (button);

	return (GTK_WIDGET (config));
}
