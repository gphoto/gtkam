/* gtkam-clock.c
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

#include "config.h"
#include "gtkam-clock.h"

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <gtk/gtkarrow.h>
#include <gtk/gtklabel.h>
#include <gtk/gtksignal.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkmain.h>

struct _GtkamClockPrivate
{
	GTimer *timer;
	guint32 timeout;

	GtkLabel *label;

	time_t time;

	GDateWeekday weekday;
};

#define PARENT_TYPE GTK_TYPE_HBOX
static GtkHBoxClass *parent_class;

enum {
	CHANGED,
	NEXT_DAY,
	PREVIOUS_DAY,
	SET,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void
gtkam_clock_destroy (GtkObject *object)
{
	GtkamClock *clock = GTKAM_CLOCK (object);

	if (clock->priv->timeout) {
		gtk_timeout_remove (clock->priv->timeout);
		clock->priv->timeout = 0;
	}

	if (clock->priv->timer) {
		g_timer_destroy (clock->priv->timer);
		clock->priv->timer = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_clock_finalize (GObject *object)
{
	GtkamClock *clock = GTKAM_CLOCK (object);

	g_free (clock->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_clock_class_init (gpointer g_class, gpointer class_data)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = GTK_OBJECT_CLASS (g_class);
	object_class->destroy  = gtkam_clock_destroy;

	gobject_class = G_OBJECT_CLASS (g_class);
	gobject_class->finalize = gtkam_clock_finalize;

	signals[CHANGED] = g_signal_new ("changed",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkamClockClass, changed), NULL, NULL,
		g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	signals[NEXT_DAY] = g_signal_new ("next_day",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkamClockClass, next_day), NULL, NULL,
		g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	signals[PREVIOUS_DAY] = g_signal_new ("previous_day",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkamClockClass, previous_day), NULL, NULL,
		g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	signals[SET] = g_signal_new ("set",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkamClockClass, set), NULL, NULL,
		g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	parent_class = g_type_class_peek_parent (g_class);
}

static void
gtkam_clock_init (GTypeInstance *instance, gpointer g_class)
{
	GtkamClock *clock = GTKAM_CLOCK (instance);

	clock->priv = g_new0 (GtkamClockPrivate, 1);
	clock->priv->timer = g_timer_new ();
	clock->priv->time = time (NULL);
}

GType
gtkam_clock_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo ti;

		memset (&ti, 0, sizeof (GTypeInfo));
		ti.class_size     = sizeof (GtkamClockClass);
		ti.class_init     = gtkam_clock_class_init;
		ti.instance_size  = sizeof (GtkamClock);
		ti.instance_init  = gtkam_clock_init;

		type = g_type_register_static (PARENT_TYPE, "GtkamClock",
					       &ti, 0);
	}

	return (type);
}

static void
gtkam_clock_update (GtkamClock *clock)
{
	struct tm *tm;
	time_t time;
	gdouble elapsed;
	gchar *string;
	GDate *date;
	GDateWeekday weekday;

	g_return_if_fail (GTKAM_IS_CLOCK (clock));

	elapsed = g_timer_elapsed (clock->priv->timer, NULL);
	time = (time_t) ((guint) clock->priv->time + (guint) elapsed);
	tm = localtime (&time);
	string = g_strdup_printf ("%02i:%02i:%02i", tm->tm_hour, tm->tm_min,
				  tm->tm_sec);
	gtk_label_set_text (clock->priv->label, string);
	g_free (string);

	g_signal_emit (G_OBJECT (clock), signals[CHANGED], 0);

	/* New day? */
	date = g_date_new_dmy (tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);
	weekday = g_date_weekday (date);
	g_date_free (date);
	if (!clock->priv->weekday) {
		/* Initialization */
		clock->priv->weekday = weekday;
	} else if (clock->priv->weekday == weekday) {
		/* Nothing here */
	} else if (clock->priv->weekday ==
		((weekday == G_DATE_SUNDAY) ? G_DATE_MONDAY : weekday + 1)) {
		clock->priv->weekday = weekday;
		g_signal_emit (GTK_OBJECT (clock), signals[PREVIOUS_DAY], 0);
	} else if (clock->priv->weekday ==
		((weekday == G_DATE_MONDAY) ? G_DATE_SUNDAY : weekday - 1)) {
		clock->priv->weekday = weekday;
		g_signal_emit (GTK_OBJECT (clock), signals[NEXT_DAY], 0);
	} else {
		g_warning ("Strange day (%i,%i).", clock->priv->weekday,
			   weekday);
	}
}

typedef struct
{
	GtkButton *button;
	GtkamClock *clock;
	gboolean forward;
	gboolean fast;
	gboolean running;
	guint x;
	time_t time;
} AdjustData;

static gboolean
adjust_func (gpointer data)
{
	AdjustData *ad = data;
	gint diff;

	g_assert (GTKAM_IS_CLOCK (ad->clock));

	/* Delay */
	if (time (NULL) <= ad->time + 1)
		return (TRUE);

	/* Still adjusting? */
	if (!ad->button->button_down) {
		if (ad->running)
			gtkam_clock_start (ad->clock);
		g_free (ad);
		return (FALSE);
	}

	diff = 1;
	if (ad->fast)
		diff += 3600. * (1. - pow (0.999, ++(ad->x)));
	if (!ad->forward)
		diff *= -1;
	ad->clock->priv->time += diff;
	gtkam_clock_update (ad->clock);
	g_signal_emit (GTK_OBJECT (ad->clock), signals[SET], 0);

	return (TRUE);
}

static void
on_forward_pressed (GtkButton *button, GtkamClock *clock)
{
	AdjustData *ad;

	clock->priv->time++;
	gtkam_clock_update (clock);

	ad = g_new0 (AdjustData, 1);
	ad->running = (clock->priv->timeout);
	ad->clock = clock;
	ad->button = button;
	ad->forward = TRUE;
	ad->fast = FALSE;
	ad->time = time (NULL);

	gtkam_clock_stop (clock);
	gtk_timeout_add (100, adjust_func, ad);
}

static void
on_backward_pressed (GtkButton *button, GtkamClock *clock)
{
	AdjustData *ad;

	clock->priv->time--;
	gtkam_clock_update (clock);

	ad = g_new0 (AdjustData, 1);
	ad->running = (clock->priv->timeout);
	ad->clock = clock;
	ad->button = button;
	ad->forward = FALSE;
	ad->fast = FALSE;
	ad->time = time (NULL);

	gtkam_clock_stop (clock);
	gtk_timeout_add (100, adjust_func, ad);
}

static void
on_fforward_pressed (GtkButton *button, GtkamClock *clock)
{
	AdjustData *ad;

	ad = g_new0 (AdjustData, 1);
	ad->running = (clock->priv->timeout);
	ad->clock = clock;
	ad->button = button;
	ad->forward = TRUE;
	ad->fast = TRUE;
	ad->time = time (NULL);

	gtkam_clock_stop (clock);
	gtk_timeout_add (100, adjust_func, ad);
}

static void
on_fbackward_pressed (GtkButton *button, GtkamClock *clock)
{
	AdjustData *ad;

	ad = g_new0 (AdjustData, 1);
	ad->running = (clock->priv->timeout);
	ad->clock = clock;
	ad->button = button;
	ad->forward = FALSE;
	ad->fast = TRUE;
	ad->time = time (NULL);

	gtkam_clock_stop (clock);
	gtk_timeout_add (100, adjust_func, ad);
}


static gboolean
timeout_func (gpointer data)
{
	gtkam_clock_update (GTKAM_CLOCK (data));
	return (TRUE);
}

static void
on_label_toggled (GtkToggleButton *toggle, GtkamClock *clock)
{
	if (toggle->active)
		gtkam_clock_stop (clock);
	else
		gtkam_clock_start (clock);
}

GtkWidget *
gtkam_clock_new (void)
{
	GtkamClock *clock;
	GtkWidget *arrow, *label, *button, *toggle;

	clock = g_object_new (GTKAM_TYPE_CLOCK, NULL);
	gtk_box_set_spacing (GTK_BOX (clock), 0);
	gtk_box_set_homogeneous (GTK_BOX (clock), FALSE);

	button = gtk_button_new ();
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (clock), button, FALSE, FALSE, 0);
	arrow = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_OUT);
	gtk_widget_show (arrow);
	gtk_container_add (GTK_CONTAINER (button), arrow);
	g_signal_connect (G_OBJECT (button), "pressed",
			  G_CALLBACK (on_fbackward_pressed), clock);

	button = gtk_button_new ();
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (clock), button, FALSE, FALSE, 0);
	arrow = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_OUT);
	gtk_widget_show (arrow);
	gtk_container_add (GTK_CONTAINER (button), arrow);
	g_signal_connect (GTK_OBJECT (button), "pressed",
			  G_CALLBACK (on_backward_pressed), clock);

	toggle = gtk_toggle_button_new ();
	gtk_button_set_relief (GTK_BUTTON (toggle), GTK_RELIEF_NONE);
	gtk_widget_show (toggle);
	gtk_box_pack_start (GTK_BOX (clock), toggle, FALSE, FALSE, 0);
	g_signal_connect (GTK_OBJECT (toggle), "toggled",
			    G_CALLBACK (on_label_toggled), clock);
	label = gtk_label_new ("00:00:00");
	gtk_widget_show (label);
	gtk_container_add (GTK_CONTAINER (toggle), label);
	clock->priv->label = GTK_LABEL (label);

	button = gtk_button_new ();
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (clock), button, FALSE, FALSE, 0);
	arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
	gtk_widget_show (arrow);
	gtk_container_add (GTK_CONTAINER (button), arrow);
	g_signal_connect (G_OBJECT (button), "pressed",
			    G_CALLBACK (on_forward_pressed), clock);

	button = gtk_button_new (); 
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (clock), button, FALSE, FALSE, 0);
	arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
	gtk_widget_show (arrow);
	gtk_container_add (GTK_CONTAINER (button), arrow);
	g_signal_connect (G_OBJECT (button), "pressed",
			  G_CALLBACK (on_fforward_pressed), clock);

	gtkam_clock_start (clock);

	return (GTK_WIDGET (clock));
}

void
gtkam_clock_set (GtkamClock *clock, guchar hour, guchar minute, guchar second)
{
	struct tm *tm;

	g_return_if_fail (GTKAM_IS_CLOCK (clock));

	tm = localtime (&clock->priv->time);
	tm->tm_hour = hour;
	tm->tm_min  = minute;
	tm->tm_sec  = second;
	clock->priv->time = mktime (tm);
}

void
gtkam_clock_get (GtkamClock *clock, guchar *hour, guchar *minute,
		 guchar *second)
{
	struct tm *tm;

	g_return_if_fail (GTKAM_IS_CLOCK (clock));
	g_return_if_fail (hour && minute && second);

	tm = localtime (&clock->priv->time);
	*hour = tm->tm_hour;
	*minute = tm->tm_min;
	*second = tm->tm_sec;
}

void
gtkam_clock_start (GtkamClock *clock)
{
	g_return_if_fail (GTKAM_IS_CLOCK (clock));

	if (!clock->priv->timeout) {
		clock->priv->timeout = gtk_timeout_add (1000, timeout_func,
							clock);
		g_timer_start (clock->priv->timer);
	}
}

void
gtkam_clock_stop (GtkamClock *clock)
{
	g_return_if_fail (GTKAM_IS_CLOCK (clock));

	if (clock->priv->timeout) {
		gtk_timeout_remove (clock->priv->timeout);
		g_timer_stop (clock->priv->timer);
		clock->priv->timeout = 0;
		clock->priv->time += g_timer_elapsed (clock->priv->timer, NULL);
	}
}
