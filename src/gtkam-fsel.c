/* gtkam-fsel.c
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

#include <string.h>

#include "config.h"
#include "gtkam-fsel.h"
#include "i18n.h"

#include <gtk/gtkbutton.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkhbbox.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtkhpaned.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkimage.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkliststore.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gtkam-error.h"
#include "gtkam-tree.h"
#include "gtkam-status.h"
#include "gtkam-list.h"

struct _GtkamFSelPrivate
{
	GtkamTree *tree;

	GtkListStore *model;
};

#define PARENT_TYPE GTKAM_TYPE_DIALOG
static GtkamDialogClass *parent_class;

static void
gtkam_fsel_destroy (GtkObject *object)
{
	GtkamFSel *fsel = GTKAM_FSEL (object);

	fsel = NULL;

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_fsel_finalize (GObject *object)
{
	GtkamFSel *fsel = GTKAM_FSEL (object);

	g_free (fsel->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_fsel_class_init (gpointer g_class, gpointer class_data)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = GTK_OBJECT_CLASS (g_class);
	object_class->destroy  = gtkam_fsel_destroy;

	gobject_class = G_OBJECT_CLASS (g_class);
	gobject_class->finalize = gtkam_fsel_finalize;

	parent_class = g_type_class_peek_parent (g_class);
}

static void
gtkam_fsel_init (GTypeInstance *instance, gpointer g_class)
{
	GtkamFSel *fsel = GTKAM_FSEL (instance);

	fsel->priv = g_new0 (GtkamFSelPrivate, 1);
}

GType
gtkam_fsel_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo ti;

		memset (&ti, 0, sizeof (GTypeInfo));
		ti.class_size     = sizeof (GtkamFSelClass);
		ti.class_init     = gtkam_fsel_class_init;
		ti.instance_size  = sizeof (GtkamFSel);
		ti.instance_init  = gtkam_fsel_init;

		type = g_type_register_static (PARENT_TYPE, "GtkamFSel",
					       &ti, 0);
	}

	return (type);
}

static void
on_fsel_cancel_clicked (GtkButton *button, GtkamFSel *fsel)
{
	gtk_object_destroy (GTK_OBJECT (fsel));
}

static void
on_folder_selected (GtkamTree *tree, GtkamTreeFolderSelectedData *data,
		    GtkamFSel *fsel)
{
	gtkam_list_add_folder (GTKAM_LIST (fsel->list), data->camera,
			       data->folder);
}

static void
on_folder_unselected (GtkamTree *tree, GtkamTreeFolderUnselectedData *data,
		      GtkamFSel *fsel)
{
	gtkam_list_remove_folder (GTKAM_LIST (fsel->list), data->camera,
				  data->folder);
}

static void
selected_foreach_func (GtkTreeModel *model, GtkTreePath *path,
		       GtkTreeIter *iter, gpointer data)
{
	guint *count = data;

	(*count)++;
}

static void
on_file_selected (GtkamList *list, GtkamListFileSelectedData *data,
		  GtkamFSel *fsel)
{
	gtk_widget_set_sensitive (fsel->ok_button, TRUE);
}

static void
on_file_unselected (GtkamList *list, GtkamListFileUnselectedData *data,
		    GtkamFSel *fsel)
{
	GtkTreeSelection *s;
	guint count = 0;

	s = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	gtk_tree_selection_selected_foreach (s, selected_foreach_func, &count);
	gtk_widget_set_sensitive (fsel->ok_button, count != 0);
}

static void
on_new_status (GObject *object, GtkWidget *status, GtkamFSel *fsel)
{
	gtkam_dialog_add_status (GTKAM_DIALOG (fsel), status);
}

static void
on_new_dialog (GObject *object, GtkWidget *dialog, GtkamFSel *fsel)
{
	g_object_ref (G_OBJECT (dialog));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (fsel));
	gtk_widget_show (dialog);
}

static gboolean
load_tree (gpointer data)
{
	gtkam_tree_load (GTKAM_TREE (data));

	return (FALSE);
}

GtkWidget *
gtkam_fsel_new (GtkWidget *opt_window)
{
	GtkamFSel *fsel;
	GtkWidget *tree, *button, *scrolled;
	GtkWidget *hpaned;
	GtkTreeSelection *s;

	fsel = g_object_new (GTKAM_TYPE_FSEL, NULL);
	gtk_container_set_border_width (
			GTK_CONTAINER (GTK_DIALOG (fsel)->vbox), 5);

	hpaned = gtk_hpaned_new ();
	gtk_widget_show (hpaned);
	gtk_box_pack_start (GTK_BOX (GTKAM_DIALOG (fsel)->vbox), hpaned,
			    TRUE, TRUE, 0);

	/* Tree */
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolled);
	gtk_paned_pack1 (GTK_PANED (hpaned), scrolled, TRUE, TRUE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	tree = gtkam_tree_new ();
	gtk_widget_show (tree);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled),
					       tree);
	fsel->priv->tree = GTKAM_TREE (tree);
	g_signal_connect (G_OBJECT (tree), "folder_selected",
			  G_CALLBACK (on_folder_selected), fsel);
	g_signal_connect (G_OBJECT (tree), "folder_unselected",
			  G_CALLBACK (on_folder_unselected), fsel);
	g_signal_connect (G_OBJECT (tree), "new_status",
			  G_CALLBACK (on_new_status), fsel);
	g_signal_connect (G_OBJECT (tree), "new_dialog",
			  G_CALLBACK (on_new_dialog), fsel);
	gtk_idle_add (load_tree, tree);

	/* File list */
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolled);
	gtk_paned_pack2 (GTK_PANED (hpaned), scrolled, TRUE, TRUE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	fsel->list = gtkam_list_new ();
	s = gtk_tree_view_get_selection (GTK_TREE_VIEW (fsel->list));
	gtk_widget_show (fsel->list);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled),
					       fsel->list);
	g_signal_connect (G_OBJECT (fsel->list), "file_selected",
			  G_CALLBACK (on_file_selected), fsel);
	g_signal_connect (G_OBJECT (fsel->list), "file_unselected",
			  G_CALLBACK (on_file_unselected), fsel);
	g_signal_connect (G_OBJECT (fsel->list), "new_status",
			  G_CALLBACK (on_new_status), fsel);
	g_signal_connect (G_OBJECT (fsel->list), "new_dialog",
			  G_CALLBACK (on_new_dialog), fsel);

	button = gtk_button_new_from_stock (GTK_STOCK_OK);
	gtk_widget_show (button);
	gtk_widget_set_sensitive (button, FALSE);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (fsel)->action_area),
			   button);
	gtk_widget_grab_focus (button);
	fsel->ok_button = button;

	button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
	gtk_widget_show (button);
	g_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_fsel_cancel_clicked), fsel);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (fsel)->action_area),
			   button);

	if (opt_window)
		gtk_window_set_transient_for (GTK_WINDOW (fsel),
					      GTK_WINDOW (opt_window));

	return (GTK_WIDGET (fsel));
}
