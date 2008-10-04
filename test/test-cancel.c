/* test-cancel.c
 *
 * Copyright 2002 Lutz Mueller <lutz@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <unistd.h>

#include <gtk/gtkmain.h>

#include <gtkam-cancel.h>

static gboolean
idle_func (gpointer data)
{
	GtkamCancel *c = GTKAM_CANCEL (data);
	int id;
	unsigned int i;

	id = gp_context_progress_start (c->context->context, 10,
					"Testing progress...");
	while (gtk_events_pending ())
		gtk_main_iteration ();
	sleep (1);

	for (i = 0; i < 10; i++) {
		gp_context_progress_update (c->context->context, id, i + 1);
		while (gtk_events_pending ())
			gtk_main_iteration ();
		if (gp_context_cancel (c->context->context) ==
						GP_CONTEXT_FEEDBACK_CANCEL)
			break;
		sleep (1);
	}

	gp_context_progress_stop (c->context->context, id);
	while (gtk_events_pending ())
		gtk_main_iteration ();
	sleep (1);

	gtk_main_quit ();

	return (FALSE);
}

int
main (int argc, char **argv)
{
	GtkWidget *w;

	gtk_init (&argc, &argv);

	w = gtkam_cancel_new ("Testing widget...");
	gtk_widget_show (w);
	gtk_idle_add (idle_func, w);

	gtk_main ();

	gtk_object_destroy (GTK_OBJECT (w));

	return (0);
}
