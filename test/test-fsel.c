/* test-fsel.c:
 *
 * Copyright © 2002 Lutz Müller <lutz@users.sourceforge.net>
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

#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtktreeselection.h>

#include "gtkam-fsel.h"
#include "gtkam-list.h"

static void
selection_foreach_func (GtkTreeModel *model, GtkTreePath *path,
			GtkTreeIter *iter, gpointer data)
{
	gchar *name;
	GtkamList *list = GTKAM_LIST (data);

	name = gtkam_list_get_name_from_iter (list, iter);
	g_message (" '%s'", name);
	g_free (name);
}

static void
on_fsel_ok_clicked (GtkButton *button, GtkamFSel *fsel)
{
	GtkTreeSelection *s;

	g_message ("Ok clicked!");

	s = gtk_tree_view_get_selection (
				GTK_TREE_VIEW (GTKAM_FSEL (fsel)->list));
	gtk_tree_selection_selected_foreach (s, selection_foreach_func,
					     fsel->list);
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
