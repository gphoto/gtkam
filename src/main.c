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
#include <gphoto2/gphoto2-port-log.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtkmain.h>

#include "gtkam-main.h"
#include "gtkam-error.h"
#include "util.h"
#include "support.h"

static void
log_func (GPLogLevel level, const char *domain, const char *format,
	  va_list args, void *data)
{
	fprintf (stderr, domain);
	fprintf (stderr, ": ");
	vfprintf (stderr, format, args);
	fprintf (stderr, "\n");
}

int
main (int argc, char *argv[])
{
	GtkWidget *m;
	int x, log = -1;
	GdkPixbuf *pixbuf;

	gtk_set_locale ();
	bindtextdomain (PACKAGE, GTKAM_LOCALEDIR);
	textdomain (PACKAGE);

	/* Process command-line parameters */
	for (x = 1; x < argc; x++) {
		if (!strcmp (argv[x], "--debug") || !strcmp (argv[x], "-d")) {
			log = gp_log_add_func (GP_LOG_DATA, log_func, NULL);
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

	gtk_init (&argc, &argv);

	/* Create the main window */
	m = gtkam_main_new ();
	gtk_widget_show (m);
	g_signal_connect (GTK_OBJECT (m), "destroy",
			    GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

	pixbuf = gdk_pixbuf_new_from_file (IMAGE_DIR "/gtkam-camera.png", NULL);
	gtk_window_set_icon (GTK_WINDOW (m), pixbuf);
	gdk_pixbuf_unref (pixbuf);

	gtk_main ();

	if (log < 0)
		gp_log_remove_func (log);

	return 0;
}
