/* test-fsel.c:
 *
 * Copyright (C) 2002 Lutz Müller <lutz@users.sourceforge.net>
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

#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkbutton.h>

#include "gtkam-fsel.h"
#include "gtkam-clist.h"

static void
on_fsel_ok_clicked (GtkButton *button, GtkamFSel *fsel)
{
	GList *list;
	guint i;
	GtkamCListEntry *entry;

	g_message ("Ok clicked!");

	list = GTKAM_CLIST (GTKAM_FSEL (fsel)->clist)->selection;
	for (i = 0; i < g_list_length (list); i++) {
		entry = g_list_nth_data (list, i);
		g_message (" - '%s' in '%s'", entry->name, entry->folder);
	}
	gtk_object_destroy (GTK_OBJECT (fsel));
}

int
main (int argc, char **argv)
{
	GtkWidget *fsel;

	gtk_init (&argc, &argv);

	fsel = gtkam_fsel_new (NULL);
	gtk_widget_show (fsel);
	gtk_signal_connect (GTK_OBJECT (fsel), "destroy",
			    GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
	gtk_signal_connect (GTK_OBJECT (GTKAM_FSEL (fsel)->ok_button),
		"clicked", GTK_SIGNAL_FUNC (on_fsel_ok_clicked), fsel);
	gtk_main ();

	return (0);
}
