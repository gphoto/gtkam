/* gtkam-config.c
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

#include "config.h"
#include <math.h>
#include "gtkam-config.h"
#include "i18n.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <gphoto2/gphoto2-setting.h>

#include <gtk/gtkstock.h>
#include <gtk/gtktooltips.h>
#include <gtk/gtkscrolledwindow.h>
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
#include <gtk/gtkcalendar.h>
#include <gtk/gtkmain.h>

#include "gtkam-cancel.h"
#include "gtkam-error.h"
#include "gtkam-clock.h"
#include "gtkam-status.h"

struct _GtkamConfigPrivate
{
	GtkTooltips *tooltips;
	GtkWidget *notebook;
	GtkWidget *orphans;

	GHashTable *hash;

	GtkamCamera  *camera;
	CameraWidget *config;
};

#define PARENT_TYPE GTKAM_TYPE_DIALOG
static GtkamDialogClass *parent_class;

static void
gtkam_config_destroy (GtkObject *object)
{
	GtkamConfig *config = GTKAM_CONFIG (object);

	if (config->priv->camera) {
		g_object_unref (config->priv->camera);
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
		g_object_unref (G_OBJECT (config->priv->tooltips));
		config->priv->tooltips = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_config_finalize (GObject *object)
{
	GtkamConfig *config = GTKAM_CONFIG (object);

	g_free (config->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_config_class_init (gpointer g_class, gpointer class_data)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = GTK_OBJECT_CLASS (g_class);
	object_class->destroy  = gtkam_config_destroy;
	
	gobject_class = G_OBJECT_CLASS (g_class);
	gobject_class->finalize = gtkam_config_finalize;

	parent_class = g_type_class_peek_parent (g_class);
}

static void
gtkam_config_init (GTypeInstance *instance, gpointer g_class)
{
	GtkamConfig *config = GTKAM_CONFIG (instance);

	config->priv = g_new0 (GtkamConfigPrivate, 1);
	config->priv->hash = g_hash_table_new (g_direct_hash, g_direct_equal);
	config->priv->tooltips = gtk_tooltips_new ();
	g_object_ref (G_OBJECT (config->priv->tooltips));
	gtk_object_sink (GTK_OBJECT (config->priv->tooltips));
}

GType
gtkam_config_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo ti;

		memset (&ti, 0, sizeof (GTypeInfo)); 
		ti.class_size     = sizeof (GtkamConfigClass);
		ti.class_init     = gtkam_config_class_init;
		ti.instance_size  = sizeof (GtkamConfig);
		ti.instance_init  = gtkam_config_init;

		type = g_type_register_static (PARENT_TYPE, "GtkamConfig",
					       &ti, 0);
	}

	return (type);
}

static void
gtkam_config_apply (GtkamConfig *config)
{
	int result;
	GtkWidget *dialog, *status;

	status = gtkam_status_new (_("Applying configuration..."));
	gtk_widget_show (status);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (config)->vbox), status,
			    FALSE, FALSE, 0);
	result = gp_camera_set_config (config->priv->camera->camera,
		config->priv->config, GTKAM_STATUS (status)->context->context);
	if (config->priv->camera->multi)
		gp_camera_exit (config->priv->camera->camera, NULL);
	switch (result) {
	case GP_OK:
		break;
	case GP_ERROR_CANCEL:
		break;
	default:
		dialog = gtkam_error_new (result,
			GTKAM_STATUS (status)->context, GTK_WIDGET (config),
			_("Could not apply configuration.")),
		gtk_widget_show (dialog);
	}
	gtk_object_destroy (GTK_OBJECT (status));
}

static void
gtkam_config_close (GtkamConfig *config)
{
	while (gtk_events_pending ())
		gtk_main_iteration ();

	gtk_object_destroy (GTK_OBJECT (config));
}

static void
on_size_allocate (GtkWidget *w, GtkAllocation *a)
{
	char *buf;

	buf = g_strdup_printf ("%i", a->width);
	gp_setting_set ("gtkam-config", "width", buf);
	g_free (buf);
	buf = g_strdup_printf ("%i", a->height);
	gp_setting_set ("gtkam-config", "height", buf);
	g_free (buf);
}

static void
on_config_ok_clicked (GtkButton *button, GtkamConfig *config)
{
	gtkam_config_apply (config);
	gtkam_config_close (config);
}

static void
on_config_apply_clicked (GtkButton *button, GtkamConfig *config)
{
	gtkam_config_apply (config);
}

static void
on_config_close_clicked (GtkButton *button, GtkamConfig *config)
{
	gtkam_config_close (config);
}

static GtkWidget *
create_page (GtkamConfig *config, CameraWidget *widget)
{
	GtkWidget *label, *vbox, *scrolled;
	int id;
	const char *l = NULL, *info = NULL;

	/* Label */
	if (widget) {
		gp_widget_get_label (widget, &l);
		label = gtk_label_new (l);
		gp_widget_get_info  (widget, &info);
		if (!strlen (info))
			gtk_tooltips_set_tip (config->priv->tooltips, label,
				_("No additional information available. (1)"), NULL);
		else
			gtk_tooltips_set_tip (config->priv->tooltips, label,
					      info, NULL);
	} else
		label = gtk_label_new (_("General Settings"));
	gtk_widget_show (label);

	if (gp_widget_count_children (widget) > 4) {
		scrolled = gtk_scrolled_window_new (NULL, NULL);
		gtk_widget_show (scrolled);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		gtk_container_set_border_width (GTK_CONTAINER (scrolled), 5);
		gtk_notebook_append_page (
			GTK_NOTEBOOK (config->priv->notebook),
			scrolled, label);
		vbox = gtk_vbox_new (FALSE, 10);
		gtk_widget_show (vbox);
		gtk_scrolled_window_add_with_viewport (
				GTK_SCROLLED_WINDOW (scrolled), vbox);
		gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
	} else {
		vbox = gtk_vbox_new (FALSE, 10);
		gtk_widget_show (vbox);
		gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
		gtk_notebook_append_page (
			GTK_NOTEBOOK (config->priv->notebook), 
			vbox, label);
	}

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
	value_new = g_object_get_data (G_OBJECT (toggle), "value");
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
	result = callback (config->priv->camera->camera, widget, NULL);
	if (result != GP_OK) {
		dialog = gtkam_error_new (result, NULL, GTK_WIDGET (config),
			_("Could not execute command."));
		gtk_widget_show (dialog);
	}
}

static void
on_entry_changed (GtkEntry *entry, CameraWidget *widget)
{
	char *value = NULL;
	const char *value_new;

	gp_widget_get_value (widget, &value);
	value_new = gtk_entry_get_text (entry);
	if (!value || strcmp (value, value_new))
		gp_widget_set_value (widget, (void *) value_new);
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
adjust_time (GtkCalendar *calendar, GtkamClock *clock, CameraWidget *widget)
{
	struct tm tm;
	int time;

	memset (&tm, 0, sizeof (struct tm));
	gtkam_clock_get (clock, (guchar*) &tm.tm_hour,
			 (guchar*) &tm.tm_min, (guchar*) &tm.tm_sec);
	gtk_calendar_get_date (calendar, &tm.tm_year, &tm.tm_mon, &tm.tm_mday);

	/*
	 * According to Patrick Mansfield <patman@aracnet.com>, tm_year
	 * starts counting at 1900, whereas gtk_calendar starts counting at 0.
	 */
	tm.tm_year = tm.tm_year - 1900;
	time = (int)mktime (&tm);
	gp_widget_set_value (widget, &time);
}

static void
on_day_selected (GtkCalendar *calendar, CameraWidget *widget)
{
	GtkamClock *clock;

	clock = GTKAM_CLOCK (g_object_get_data (G_OBJECT (calendar),
						  "clock"));
	adjust_time (calendar, clock, widget);
}

static void
adjust_day (GtkCalendar *calendar, gint days)
{
	GDate *date;
	guint year, month, day;
	
	gtk_calendar_get_date (calendar, &year, &month, &day);
	date = g_date_new_dmy (day, month + 1, year);
	if (days > 0)
		g_date_add_days (date, (guint) days);
	else
		g_date_subtract_days (date, (guint) ABS (days));
	gtk_calendar_select_month (calendar, g_date_month (date) - 1,
				   g_date_year (date));
	gtk_calendar_select_day (calendar, g_date_day (date));
	g_date_free (date);
}

static void
on_clock_next_day (GtkamClock *clock)
{
	GtkCalendar *calendar;

	calendar = GTK_CALENDAR (g_object_get_data (G_OBJECT (clock),
						      "calendar"));
	adjust_day (calendar, 1);
}

static void
on_clock_previous_day (GtkamClock *clock)
{
	GtkCalendar *calendar;

	calendar = GTK_CALENDAR (g_object_get_data (G_OBJECT (clock),
						      "calendar"));
	adjust_day (calendar, -1);
}

static void
on_clock_set (GtkamClock *clock, CameraWidget *widget)
{
	GtkCalendar *calendar;

	calendar = GTK_CALENDAR (g_object_get_data (G_OBJECT (clock),
						      "calendar"));
	adjust_time (calendar, clock, widget);
}

static int 
get_digits (double d)
{
	/*
	 * Returns the number of non-zero digits to the right of the decimal
	 * point, up to a max of 4.
	 */
	double x;
	int i;
	for (i = 0, x = d * 1.0; i < 4;  i++) {

		/*
		 * The small number really depends on how many digits (4)
		 * we are checking for, but as long as it's small enough
		 * this works fine. We need the "<" as the floating point
		 * arithemtic does not always give an exact 0.0 (and can
		 * even give a -0.0).
		 */
		if (fabs (x - floor (x)) < 0.000001) 
			return(i);
		x = x * 10.0;
	}
	return(i);
}

static void
on_set_system_time_clicked (GtkButton *button, GtkamClock *clock)
{
	GtkCalendar *c;
	time_t t;
	struct tm *tm;

	c = GTK_CALENDAR (g_object_get_data (G_OBJECT (clock), "calendar"));
	t = time (NULL);
	tm = localtime (&t);
	gtk_calendar_select_month (c, tm->tm_mon, tm->tm_year + 1900);
	gtk_calendar_select_day (c, tm->tm_mday);
	gtkam_clock_set (clock, tm->tm_hour, tm->tm_min, tm->tm_sec);
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
	GtkWidget *vbox, *button, *gw = NULL, *frame, *calendar;
	GtkWidget *clock, *hbox;
	GtkObject *adjustment;
	GSList *group = NULL;
	GList *options = NULL;
	gint i, id;
	struct tm *tm;
	time_t tt;

	gp_widget_get_label (widget, &label);
	frame = gtk_frame_new (label);
	gp_widget_get_info  (widget, &info);
	gp_widget_get_type  (widget, &type);

	switch (type) {
	case GP_WIDGET_WINDOW:
	case GP_WIDGET_SECTION:

		/* If section, create page */
		if (type == GP_WIDGET_SECTION)
			create_page (config, widget);

		/* If window, set the window's label */
		if (type == GP_WIDGET_WINDOW)
			gtk_window_set_title (GTK_WINDOW (config), _(label));

		/* Create sub-widgets */
		for (i = 0; i < gp_widget_count_children (widget); i++) {
			gp_widget_get_child (widget, i, &child);
			create_widgets (config, child);
		}

		return;
	
	case GP_WIDGET_BUTTON:

		gw = gtk_vbox_new (FALSE, 10);
		gtk_container_set_border_width (GTK_CONTAINER (gw), 10);
		button = gtk_button_new_with_label (_(label));
		gtk_widget_show (button);
		g_signal_connect (GTK_OBJECT (button), "clicked",
				GTK_SIGNAL_FUNC (on_button_clicked), widget);
		gtk_container_add (GTK_CONTAINER (gw), button);
		if (strlen (info))
			gtk_tooltips_set_tip (config->priv->tooltips,
				button, info, NULL);
		else
			gtk_tooltips_set_tip (config->priv->tooltips, button,
				_("No additional information available. (2)"),
				NULL);
		break;

	case GP_WIDGET_DATE:

		gp_widget_get_value (widget, &value_int);
		gw = gtk_vbox_new (FALSE, 5);
		gtk_container_set_border_width (GTK_CONTAINER (gw), 5);

		/* Create the calendar */
		calendar = gtk_calendar_new ();
		gtk_widget_show (calendar);
		gtk_box_pack_start (GTK_BOX (gw), calendar, FALSE, FALSE, 0);

		/* Create the clock */
		clock = gtkam_clock_new ();
		gtk_widget_show (clock);
		hbox = gtk_hbox_new (5, FALSE);
		gtk_widget_show (hbox);
		gtk_box_pack_start (GTK_BOX (gw), hbox, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), clock, FALSE, FALSE, 0);

		/* Set date & time */
		tt = value_int;
		tm = localtime (&tt);
		gtk_calendar_select_month (GTK_CALENDAR (calendar),
					   tm->tm_mon, tm->tm_year + 1900);
		gtk_calendar_select_day (GTK_CALENDAR (calendar), tm->tm_mday); 
		gtkam_clock_set (GTKAM_CLOCK (clock), tm->tm_hour, tm->tm_min,
				 tm->tm_sec);

		/* We need clock and calendar together */
		g_object_set_data (G_OBJECT (clock), "calendar", calendar);
		g_object_set_data (G_OBJECT (calendar), "clock", clock);

		/* Make it easy for users to select the system's time */
		button = gtk_button_new_with_label (_("Set to system's time"));
		gtk_widget_show (button);
		gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
		g_signal_connect (G_OBJECT (button), "clicked",
				  G_CALLBACK (on_set_system_time_clicked),
				  clock);

		/* Connect the signals */
		g_signal_connect (GTK_OBJECT (clock), "next_day",
			GTK_SIGNAL_FUNC (on_clock_next_day), NULL);
		g_signal_connect (GTK_OBJECT (clock), "previous_day",
			GTK_SIGNAL_FUNC (on_clock_previous_day), NULL);
		g_signal_connect (GTK_OBJECT (clock), "set",
			GTK_SIGNAL_FUNC (on_clock_set), widget);
		g_signal_connect (GTK_OBJECT (calendar), "day_selected",
			GTK_SIGNAL_FUNC (on_day_selected), widget);

		break;

	case GP_WIDGET_TEXT:

		gp_widget_get_value (widget, &value_char);
		gw = gtk_entry_new ();
		if (value_char)
			gtk_entry_set_text (GTK_ENTRY (gw),
					    _(value_char));
		g_signal_connect (GTK_OBJECT (gw), "changed",
				GTK_SIGNAL_FUNC (on_entry_changed), widget);
		if (strlen (info))
			gtk_tooltips_set_tip (config->priv->tooltips,
				gw, info, NULL);
		else
			gtk_tooltips_set_tip (config->priv->tooltips,
				gw,
				_("No additional information available. (3)"), NULL);
		break;
	
	case GP_WIDGET_RANGE:

		gp_widget_get_value (widget, &value_float);
		gp_widget_get_range (widget, &min, &max, &increment);
		adjustment = gtk_adjustment_new (value_float, min, max,
						 increment, 
						 MAX((max - min) / 20.0, 
						     increment),
						 0);
		g_signal_connect (adjustment, "value_changed",
			GTK_SIGNAL_FUNC (on_adjustment_value_changed), widget);
		gw = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
		gtk_scale_set_digits (GTK_SCALE (gw), 
				      get_digits (increment));
		gtk_range_set_update_policy (GTK_RANGE (gw),
					     GTK_UPDATE_DISCONTINUOUS);
		if (strlen (info))
			gtk_tooltips_set_tip (config->priv->tooltips,
				      gw, info, NULL);
		else
			gtk_tooltips_set_tip (config->priv->tooltips,
				gw,
				_("No additional information available. (4)"), NULL);
		break;

	case GP_WIDGET_MENU:

		gw = gtk_combo_new ();
		gp_widget_get_value (widget, &value_char);
		for (i = 0; i < gp_widget_count_choices (widget); i++) {
			gp_widget_get_choice (widget, i, &choice);
			options = g_list_append (options, g_strdup (_(choice)));
		}
		gtk_combo_set_popdown_strings (GTK_COMBO (gw), options);
		gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (gw)->entry),
				    _(value_char));
		g_signal_connect (GTK_OBJECT (GTK_COMBO (gw)->entry),
			"changed", GTK_SIGNAL_FUNC (on_entry_changed), widget);
		break;

	case GP_WIDGET_RADIO:

		if (gp_widget_count_choices (widget) < 6)
			gw = gtk_hbox_new (FALSE, 5);
		else
			gw = gtk_vbox_new (FALSE, 5);
		gp_widget_get_value (widget, &value_char);
		for (i = 0; i < gp_widget_count_choices (widget); i++) {
			gp_widget_get_choice (widget, i, &choice);
			button = gtk_radio_button_new_with_label (group,
								  _(choice));
			gtk_widget_show (button);
			group = gtk_radio_button_get_group (
					GTK_RADIO_BUTTON (button));
			g_object_set_data (G_OBJECT (button), "value",
					     (char*) choice);
			gtk_box_pack_start (GTK_BOX (gw), button,
					    FALSE, FALSE, 0);
			if (value_char && !strcmp (value_char, choice))
				gtk_toggle_button_set_active (
					GTK_TOGGLE_BUTTON (button), TRUE);
			g_signal_connect (GTK_OBJECT (button), "toggled",
				GTK_SIGNAL_FUNC (on_radio_button_toggled),
				widget);
			if (strlen (info))
				gtk_tooltips_set_tip (config->priv->tooltips,
					      button, info, NULL);
			else
				gtk_tooltips_set_tip (config->priv->tooltips,
					button, _("No additional information "
					"available. (5)"), NULL);
						
		}
		break;

	case GP_WIDGET_TOGGLE:

		gw = gtk_check_button_new_with_label (label);
		gp_widget_get_value (widget, &value_int);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gw),
					      (value_int != 0));
		g_signal_connect (GTK_OBJECT (gw), "toggled",
				    GTK_SIGNAL_FUNC (on_toggle_button_toggled),
				    widget);
		if (strlen (info))
			gtk_tooltips_set_tip (config->priv->tooltips,
				      gw, info, NULL);
		else
			gtk_tooltips_set_tip (config->priv->tooltips, gw,
				_("No additional information available. (6)"), NULL);
		break;

	default:

		g_warning ("Widget '%s' if of unknown type!", label);
		return;
	}

	gtk_widget_show (gw);
	gtk_widget_show (frame);
	gtk_container_add (GTK_CONTAINER (frame), gw);

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
gtkam_config_new (GtkamCamera *camera)
{
	GtkamConfig *config;
	GtkWidget *button, *dialog, *cancel;
	CameraWidget *config_widget;
	int result;
	char width[1024], height[1024];

	g_return_val_if_fail (camera != NULL, NULL);

	cancel = gtkam_cancel_new (_("Getting configuration..."));
	gtk_widget_show (cancel);
	result = gp_camera_get_config (camera->camera, &config_widget,
		GTKAM_CANCEL (cancel)->context->context);
	if (camera->multi)
		gp_camera_exit (camera->camera, NULL);
	switch (result) {
	case GP_OK:
		break;
	case GP_ERROR_CANCEL:
		gtk_object_destroy (GTK_OBJECT (cancel));
		return (NULL);
	default:
		dialog = gtkam_error_new (result,
			GTKAM_CANCEL (cancel)->context, NULL,
			_("Could not get configuration."));
		gtk_widget_show (dialog);
		gtk_object_destroy (GTK_OBJECT (cancel));
		return (NULL);
	}
	gtk_object_destroy (GTK_OBJECT (cancel));

	config = g_object_new (GTKAM_TYPE_CONFIG, NULL);

	config->priv->camera = camera;
	g_object_ref (G_OBJECT (camera));
	config->priv->config = config_widget;

	config->priv->notebook = gtk_notebook_new ();
	gtk_container_set_border_width (GTK_CONTAINER (config->priv->notebook),
					5);

	if ((gp_setting_get ("gtkam-config", "width", width) == GP_OK) &&
		(gp_setting_get ("gtkam-config", "height", height) == GP_OK))
		gtk_window_set_default_size (GTK_WINDOW(config), atoi (width),
					     atoi (height));
	g_signal_connect (GTK_OBJECT (config), "size_allocate", G_CALLBACK
			  (on_size_allocate), NULL);

	gtk_widget_show (config->priv->notebook);
	gtk_box_pack_start (GTK_BOX (GTKAM_DIALOG (config)->vbox),
			    config->priv->notebook, TRUE, TRUE, 0);
	create_widgets (config, config_widget);

	button = gtk_button_new_from_stock (GTK_STOCK_OK);
	gtk_widget_show (button);
	g_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_config_ok_clicked), config);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (config)->action_area),
			   button);

	button = gtk_button_new_from_stock (GTK_STOCK_APPLY);
	gtk_widget_show (button);
	g_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_config_apply_clicked), config);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (config)->action_area),
			   button);

	button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	gtk_widget_show (button);
	g_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_config_close_clicked), config);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (config)->action_area),
			   button);
	gtk_widget_grab_focus (button);

	return (GTK_WIDGET (config));
}
