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

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gtkam-error.h"
#include "gtkam-tree.h"
#include "gtkam-status.h"

struct _GtkamFSelPrivate
{
	GtkamTree *tree;

	GtkListStore *model;
	GtkWidget *list;

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
gtkam_fsel_finalize (GtkObject *object)
{
	GtkamFSel *fsel = GTKAM_FSEL (object);

	g_free (fsel->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_fsel_class_init (GtkamFSelClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_fsel_destroy;
	object_class->finalize = gtkam_fsel_finalize;

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_fsel_init (GtkamFSel *fsel)
{
	fsel->priv = g_new0 (GtkamFSelPrivate, 1);
}

GtkType
gtkam_fsel_get_type (void)
{
	static GtkType fsel_type = 0;

	if (!fsel_type) {
		static const GtkTypeInfo fsel_info = {
			"GtkamFSel",
			sizeof (GtkamFSel),
			sizeof (GtkamFSelClass),
			(GtkClassInitFunc)  gtkam_fsel_class_init,
			(GtkObjectInitFunc) gtkam_fsel_init,
			NULL, NULL, NULL};
		fsel_type = gtk_type_unique (PARENT_TYPE, &fsel_info);
	}

	return (fsel_type);
}

static void
on_fsel_cancel_clicked (GtkButton *button, GtkamFSel *fsel)
{
	gtk_object_destroy (GTK_OBJECT (fsel));
}

static void
on_folder_selected (GtkamTree *tree, Camera *camera, gboolean multi,
		    const gchar *folder, GtkamFSel *fsel)
{
	gtkam_tree_store_file_add_folder (GTKAM_TREE_STORE_FILE (fsel->tsf),
					  camera, multi, folder);
}

static void
on_folder_unselected (GtkamTree *tree, Camera *camera, gboolean multi,
		      const gchar *folder, GtkamFSel *fsel)
{
	gtkam_tree_store_file_remove_folder (GTKAM_TREE_STORE_FILE (fsel->tsf),
					     camera, multi, folder);
}

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
	GtkWidget *tree, *button, *image, *hbox, *scrolled;
	GtkWidget *hpaned;
	GdkPixmap *pixmap;
	GdkBitmap *bitmap;
	GdkPixbuf *pixbuf;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;

	fsel = gtk_type_new (GTKAM_TYPE_FSEL);
	gtk_container_set_border_width (
			GTK_CONTAINER (GTK_DIALOG (fsel)->vbox), 5);
	gtk_widget_set_usize (GTK_WIDGET (fsel), 400, 400);

	gtk_signal_connect (GTK_OBJECT (fsel), "delete_event",
			    GTK_SIGNAL_FUNC (gtk_object_destroy), NULL);

	fsel->priv->status = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (fsel->priv->status);
	gtk_box_pack_end (GTK_BOX (GTK_DIALOG (fsel)->vbox),
			  fsel->priv->status, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 5);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (fsel)->vbox), hbox,
			    FALSE, FALSE, 0);

	pixbuf = gdk_pixbuf_new_from_file (IMAGE_DIR "/gtkam-camera.png");
	if (!pixbuf)
		g_warning ("Could not load " IMAGE_DIR "/gtkam-camera.png");
	else {
		gdk_pixbuf_render_pixmap_and_mask (pixbuf, &pixmap, &bitmap, 127);
		gdk_pixbuf_unref (pixbuf);
		image = gtk_pixmap_new (pixmap, bitmap);
		if (pixmap)
			gdk_pixmap_unref (pixmap);
		if (bitmap)
			gdk_bitmap_unref (bitmap);
		gtk_widget_show (image);
		gtk_box_pack_end (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	}

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
	gtk_signal_connect (GTK_OBJECT (tree), "folder_selected",
			    GTK_SIGNAL_FUNC (on_folder_selected), fsel);
	gtk_signal_connect (GTK_OBJECT (tree), "folder_unselected",
			    GTK_SIGNAL_FUNC (on_folder_unselected), fsel);
	gtk_signal_connect (GTK_OBJECT (tree), "new_status",
			    GTK_SIGNAL_FUNC (on_new_status), fsel);
	gtk_idle_add (load_tree, tree);

	/* File list */
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolled);
	gtk_paned_pack2 (GTK_PANED (hpaned), scrolled, TRUE, TRUE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	fsel->model = gtk_list_store_new (NUM_COLMNS, G_TYPE_STRING);
	fsel->list = gtk_tree_view_new_with_model (
					GTK_TREE_MODEL (fsel->model));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (fsel->list));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
	column = gtk_tree_view_column_new_with_attributes (_("Filename"),
			gtk_cell_renderer_text_new (), "text", FILENAME, NULL);
	tk_tree_view_column_set_sort_column_id (column, FILENAME);
	gtk_tree_view_column_set_min_width (column, 150);
	gtk_tree_view_append_column (GTK_TREE_VIEW (fsel->list), column);
	gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (fsel->list), TRUE);
	gtk_widget_show (fsel->list);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled),
					       fsel->list);

	button = gtk_button_new_with_label (_("Ok"));
	gtk_widget_show (button);
	gtk_widget_set_sensitive (button, FALSE);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (fsel)->action_area),
			   button);
	gtk_widget_grab_focus (button);
	fsel->ok_button = button;

	button = gtk_button_new_with_label (_("Cancel"));
	gtk_widget_show (button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_fsel_cancel_clicked), fsel);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (fsel)->action_area),
			   button);

	if (opt_window)
		gtk_window_set_transient_for (GTK_WINDOW (fsel),
					      GTK_WINDOW (opt_window));

	return (GTK_WIDGET (fsel));
}
