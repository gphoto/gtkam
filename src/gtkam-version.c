/* gtkam-version.c
 *
 * Copyright (C) 2002 Hans Ulrich Niedermann <gp@n-dimensional.de
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

#include <stdlib.h>

#include "gtkam-version.h"

static const char **gtkam_frontend_version(GPVersionVerbosity verbose)
{
	/* we could also compute/parse the short strings from the long
	   ones, but the current method is easier for now :-) */
	static const char *verb[] = {
		VERSION,
#ifdef HAVE_CC
		HAVE_CC " (C compiler used)",
#endif
#ifdef HAVE_BONOBO
		"bonobo (for FIXME)",
#else
		"no bonobo (for FIXME)",
#endif
#ifdef HAVE_GNOME
		"gnome (for FIXME)",
#else
		"no gnome (for FIXME)",
#endif
#ifdef HAVE_GIMP
		"gimp (plugin for The Gimp)",
#else
		"no gimp (plugin for The Gimp)",
#endif
#ifdef HAVE_EXIF
		"exif (for displaying exif information)",
#else
		"no exif (for displaying exif information)",
#endif
		NULL
	};
	static const char *shrt[] = {
		VERSION,
#ifdef HAVE_CC
		HAVE_CC,
#endif
#ifdef HAVE_BONOBO
		"bonobo",
#else
		"no bonobo",
#endif
#ifdef HAVE_GNOME
		"gnome",
#else
		"no gnome",
#endif
#ifdef HAVE_GIMP
		"gimp",
#else
		"no gimp",
#endif
#ifdef HAVE_EXIF
		"exif",
#else
		"no exif",
#endif
		NULL
	};
	return((verbose == GP_VERSION_VERBOSE)?verb:shrt);
}

const module_version module_versions[] = {
  	{ "gtkam", gtkam_frontend_version },
  	{ "libgphoto2", gp_library_version },
	{ "libgphoto2_port", gp_port_library_version },
	{ NULL, NULL }
};

#define CHECK_NULL(x) { if (x == NULL) { return; } }

void log_version(GPLogLevel level, const char *domain)
{
	int n;

	gp_log (level, domain, _("ALWAYS INCLUDE THE FOLLOWING LINES "
				 "WHEN SENDING DEBUG MESSAGES TO THE "
				 "MAILING LIST:"));

	for (n = 0; module_versions[n].name != NULL; n++) {
		int i;
		const char **v = NULL;
		char *name = module_versions[n].name;
		GPVersionFunc func = module_versions[n].version_func;
		CHECK_NULL (name);
		CHECK_NULL (func);
		v = func(GP_VERSION_VERBOSE);
		CHECK_NULL (v);
		CHECK_NULL (v[0]);
		gp_log (level, domain, "%s %s", name, v[0]);
		gp_log (level, domain, "%s has been compiled with the following options:", name);
		for (i = 1; v[i] != NULL; i++) {
			gp_log (level, domain, " + %s", v[i]);
		}
	}
}
