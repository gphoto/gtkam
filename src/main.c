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
#include "gtkam-version.h"

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
#include <gphoto2/gphoto2-port-log.h>

#include <gtk/gtkmain.h>

#ifdef HAVE_BONOBO
#  include <bonobo-activation/bonobo-activation-init.h>
#  include <bonobo/bonobo-ui-main.h>
#endif

#ifdef HAVE_GNOME
#  include <libgnome/gnome-program.h>
#  include <libgnomeui/gnome-ui-init.h>
#endif

#include "gtkam-main.h"
#include "gtkam-error.h"
#include "util.h"

static void
log_func (GPLogLevel level, const char *domain, const char *format,
	  va_list args, void *data)
{
	fprintf (stderr, domain);
	fprintf (stderr, ": ");
	vfprintf (stderr, format, args);
	fprintf (stderr, "\n");
}

static void
on_size_allocate (GtkWidget *w, GtkAllocation *a)
{
	char *buf;

	buf = g_strdup_printf ("%i", a->width);
	gp_setting_set ("gtkam", "width", buf);
	g_free (buf);
	buf = g_strdup_printf ("%i", a->height);
	gp_setting_set ("gtkam", "height", buf);
	g_free (buf);
}

int
main (int argc, char *argv[])
{
	GtkWidget *m;
	int x, log = -1;
	char width[1024], height[1024];

	gtk_set_locale ();
	bindtextdomain (PACKAGE, GTKAM_LOCALEDIR);
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	textdomain (PACKAGE);

	/* Process command-line parameters */
	for (x = 1; x < argc; x++) {
		if (!strcmp (argv[x], "--debug") || !strcmp (argv[x], "-d")) {
			log = gp_log_add_func (GP_LOG_DATA, log_func, NULL);
			log_version(GP_LOG_DEBUG, "gtkam-debug");
		} else if (!strcmp (argv[x], "--fatal") ||
			   !strcmp (argv[x], "-f")) {
			g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);
		} else if (!strcmp (argv[x], "--version") ||
			   !strcmp (argv[x], "-v")) {
			printf ("%s\n", VERSION);
			return (0);
		} else if (!strcmp (argv[x], "--help") ||
			   !strcmp (argv[x], "-h")) {
			printf ("%s-%s\n", PACKAGE, VERSION);
			printf (" -h --help        Print this message\n");
			printf (" -d --debug       Print debugging output\n");
			printf (" -f --fatal       Make warnings fatal\n");
			printf (" -v --version     Print version\n");
			return (0);
		} else
			g_warning ("Unknown option '%s'!", argv[x]);
	}

#ifdef HAVE_GNOME
	gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE, argc, argv,
			    GNOME_PARAM_HUMAN_READABLE_NAME,
			    _("Image Download Program"),
			    GNOME_PARAM_APP_DATADIR, APP_DATADIR, NULL);
#elif defined (HAVE_BONOBO)
	bonobo_ui_init (PACKAGE, VERSION, &argc, argv);
#else
	gtk_init (&argc, &argv);
#endif

	/* Create the main window */
	m = gtkam_main_new ();
	if ((gp_setting_get ("gtkam", "width", width) == GP_OK) &&
	    (gp_setting_get ("gtkam", "height", height) == GP_OK))
		gtk_widget_set_size_request (m, atoi (width), atoi (height));
	GTK_WINDOW (m)->allow_shrink = TRUE;
	gtk_widget_show (m);
	gtkam_main_load (GTKAM_MAIN (m));
	g_signal_connect (GTK_OBJECT (m), "destroy",
			  G_CALLBACK (gtk_main_quit), NULL);
	g_signal_connect (GTK_OBJECT (m), "size_allocate",
			  G_CALLBACK (on_size_allocate), NULL);

#ifdef HAVE_BONOBO
	bonobo_ui_main ();
#else
	gtk_main ();
#endif

	if (log < 0)
		gp_log_remove_func (log);

	return 0;
}
