/* gtkam-fsel.c
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

#include <string.h>

#include <config.h>
#include "gtkam-fsel.h"

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

#include <gtk/gtkbutton.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkhbbox.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtkhpaned.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkimage.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkstock.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gtkam-error.h"
#include "gtkam-tree.h"
#include "gtkam-status.h"

struct _GtkamFSelPrivate
{
	GtkamTree *tree;

	GtkListStore *model;

	GtkWidget *status;
};

enum {
	FILENAME,
	NUM_COLUMNS
};

#define PARENT_TYPE GTK_TYPE_DIALOG
static GtkDialogClass *parent_class;

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
	GTypeInfo tinfo;

	memset (&tinfo, 0, sizeof (GTypeInfo));
	tinfo.class_size     = sizeof (GtkamFSelClass);
	tinfo.class_init     = gtkam_fsel_class_init;
	tinfo.instance_size  = sizeof (GtkamFSel);
	tinfo.instance_init  = gtkam_fsel_init;

	return (g_type_register_static (PARENT_TYPE, "GtkamFSel", &tinfo, 0));
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
	g_warning ("Fixme: on_folder_selected");
}

static void
on_folder_unselected (GtkamTree *tree, GtkamTreeFolderUnselectedData *data,
		      GtkamFSel *fsel)
{
	g_warning ("Fixme: on_folder_unselected");
}

#if 0
static void
on_entry_selected (GtkamCList *clist, GtkamCListEntry *entry, GtkamFSel *fsel)
{
	gtk_widget_set_sensitive (fsel->ok_button, TRUE);
}

static void
on_entry_unselected (GtkamCList *clist, GtkamCListEntry *entry,
		     GtkamFSel *fsel)
{
	if (!g_list_length (GTK_CLIST (clist)->selection))
		gtk_widget_set_sensitive (fsel->ok_button, FALSE);
}
#endif

static void
on_new_status (GtkObject *object, GtkWidget *status, GtkamFSel *fsel)
{
	gtk_widget_show (status);
	gtk_box_pack_start (GTK_BOX (fsel->priv->status), status,
			    FALSE, FALSE, 0);
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
	GtkWidget *tree, *button, *image, *hbox, *scrolled, *list;
	GtkWidget *hpaned;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;

	fsel = g_object_new (GTKAM_TYPE_FSEL, NULL);
	gtk_container_set_border_width (
			GTK_CONTAINER (GTK_DIALOG (fsel)->vbox), 5);

	fsel->priv->status = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (fsel->priv->status);
	gtk_box_pack_end (GTK_BOX (GTK_DIALOG (fsel)->vbox),
			  fsel->priv->status, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 5);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (fsel)->vbox), hbox,
			    FALSE, FALSE, 0);

	image = gtk_image_new_from_file (IMAGE_DIR "/gtkam-camera.png");
	gtk_widget_show (image);
	gtk_box_pack_end (GTK_BOX (hbox), image, FALSE, FALSE, 0);

	hpaned = gtk_hpaned_new ();
	gtk_widget_show (hpaned);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (fsel)->vbox), hpaned,
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
	g_signal_connect (GTK_OBJECT (tree), "folder_selected",
			    GTK_SIGNAL_FUNC (on_folder_selected), fsel);
	g_signal_connect (GTK_OBJECT (tree), "folder_unselected",
			    GTK_SIGNAL_FUNC (on_folder_unselected), fsel);
	g_signal_connect (GTK_OBJECT (tree), "new_status",
			    GTK_SIGNAL_FUNC (on_new_status), fsel);
	gtk_idle_add (load_tree, tree);

	/* File list */
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolled);
	gtk_paned_pack2 (GTK_PANED (hpaned), scrolled, TRUE, TRUE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	fsel->priv->model = gtk_list_store_new (NUM_COLUMNS, G_TYPE_STRING);
	list = gtk_tree_view_new_with_model (
					GTK_TREE_MODEL (fsel->priv->model));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
	column = gtk_tree_view_column_new_with_attributes (_("Filename"),
			gtk_cell_renderer_text_new (), "text", FILENAME, NULL);
	gtk_tree_view_column_set_sort_column_id (column, FILENAME);
	gtk_tree_view_column_set_min_width (column, 150);
	gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
	gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (list), TRUE);
	gtk_widget_show (list);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled),
					       list);

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
