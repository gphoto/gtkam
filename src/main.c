/* main.c
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
#include <string.h>

#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-abilities-list.h>
#include <gphoto2/gphoto2-setting.h>
#include <gtk/gtkmain.h>

#include "gtkam-main.h"
#include "gtkam-error.h"
#include "util.h"
#include "support.h"

static gboolean
idle_func (gpointer data)
{
	GtkamMain *m = GTKAM_MAIN (data);
	char port[1024], speed[1024], model[1024], multi[1024];
	Camera *camera;
	CameraAbilitiesList *al;
	GPPortInfoList *il;
	GPPortInfo info;
	CameraAbilities a;
	int n, p;

	/* Retrieve the last camera used by gtkam */
	if (((gp_setting_get ("gtkam", "camera", model) == GP_OK) ||
	    (gp_setting_get ("gtkam", "model", model) == GP_OK) ||
	    (gp_setting_get ("gphoto2", "model", model) == GP_OK)) &&
	    ((gp_setting_get ("gtkam", "port", port) == GP_OK) ||
	     (gp_setting_get ("gtkam", "port name", port) == GP_OK)) &&
	    (gp_setting_get ("gtkam", "speed", speed) == GP_OK)) {
		gp_camera_new (&camera);

		gp_abilities_list_new (&al);
		gp_abilities_list_load (al);
		gp_port_info_list_new (&il);
		gp_port_info_list_load (il);

		n = gp_abilities_list_lookup_model (al, model);
		gp_abilities_list_get_abilities (al, n, &a);
		gp_abilities_list_free (al);

		p = gp_port_info_list_lookup_name (il, port);
		gp_port_info_list_get_info (il, p, &info);
		gp_port_info_list_free (il);
		
		gp_camera_set_abilities (camera, a);
		if (strcmp (port, "None") && strcmp (model, "Directory Browse"))
			gp_camera_set_port_info (camera, info);
		if (atoi (speed))
			gp_camera_set_port_speed (camera, atoi (speed));

		if (gp_setting_get ("gtkam", "multi", multi) == GP_OK)
			gtkam_main_set_camera (m, camera, atoi (multi));
		else
			gtkam_main_set_camera (m, camera, FALSE);
		gp_camera_unref (camera);
	}

	return (FALSE);
}

int
main (int argc, char *argv[])
{
	GtkWidget *m;
	char buf[1024];
	int x, y;

	gtk_set_locale ();
	bindtextdomain (PACKAGE, GTKAM_LOCALEDIR);
	textdomain (PACKAGE);

//	g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);
	gtk_init (&argc, &argv);

	/* Create the main window */
	m = gtkam_main_new ();
        if (gp_setting_get ("gtkam", "width", buf) == GP_OK) {
		x = atoi (buf);
		if (gp_setting_get ("gtkam", "height", buf) == GP_OK) {
			y = atoi (buf);
		        gtk_widget_set_usize (GTK_WIDGET (m), x, y);
		}
	}
	gtk_widget_show (m);
	gtk_signal_connect (GTK_OBJECT (m), "destroy",
			    GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
	gtk_idle_add (idle_func, m);

	gtk_main ();

	/* Destroy the main window, but remember the size */
	sprintf (buf, "%i", m->allocation.width);
	gp_setting_set ("gtkam", "width", buf);
	sprintf (buf, "%i", m->allocation.height);
	gp_setting_set ("gtkam", "height", buf);

	return 0;
}
