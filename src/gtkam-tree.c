/* gtkam-tree.c
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
#include "gtkam-tree.h"

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

#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtktreeitem.h>

#include "support.h"
#include "gtkam-error.h"

struct _GtkamTreePrivate
{
	Camera *camera;

	GtkWidget *root;
};

#define PARENT_TYPE GTK_TYPE_TREE
static GtkTreeClass *parent_class;

enum {
	FOLDER_SELECTED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void on_tree_item_expand (GtkTreeItem *item, GtkamTree *tree);

static void
gtkam_tree_destroy (GtkObject *object)
{
	GtkamTree *tree = GTKAM_TREE (object);

	if (tree->priv->camera) {
		gp_camera_unref (tree->priv->camera);
		tree->priv->camera = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_tree_finalize (GtkObject *object)
{
	GtkamTree *tree = GTKAM_TREE (object);

	g_free (tree->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_tree_class_init (GtkamTreeClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_tree_destroy;
	object_class->finalize = gtkam_tree_finalize;

	signals[FOLDER_SELECTED] = gtk_signal_new ("folder_selected",
		GTK_RUN_LAST, object_class->type,
		GTK_SIGNAL_OFFSET (GtkamTreeClass, folder_selected),
		gtk_marshal_NONE__POINTER, GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_tree_init (GtkamTree *tree)
{
	tree->priv = g_new0 (GtkamTreePrivate, 1);
}

GtkType
gtkam_tree_get_type (void)
{
	static GtkType tree_type = 0;

	if (!tree_type) {
		static const GtkTypeInfo tree_info = {
			"GtkamTree",
			sizeof (GtkamTree),
			sizeof (GtkamTreeClass),
			(GtkClassInitFunc)  gtkam_tree_class_init,
			(GtkObjectInitFunc) gtkam_tree_init,
			NULL, NULL, NULL};
		tree_type = gtk_type_unique (PARENT_TYPE, &tree_info);
	}

	return (tree_type);
}

static void
on_tree_item_select (GtkTreeItem *item, GtkamTree *tree)
{
	const gchar *path;

	path = gtk_object_get_data (GTK_OBJECT (item), "path");
	gtk_signal_emit (GTK_OBJECT (tree), signals[FOLDER_SELECTED], path);
}

static void
create_item (GtkamTree *tree, GtkTree *tree_to_add_to, const gchar *path)
{
	CameraList *list;
	GtkWidget *item, *pixmap, *label, *subtree, *hbox, *dialog, *window;
	int result;
	gchar *msg;

	item = gtk_tree_item_new ();
	gtk_widget_show (item);
	gtk_tree_append (tree_to_add_to, item);
	gtk_object_set_data_full (GTK_OBJECT (item), "path", g_strdup (path),
				  (GtkDestroyNotify) g_free);
	gtk_signal_connect (GTK_OBJECT (item), "expand",
			    GTK_SIGNAL_FUNC (on_tree_item_expand), tree);
	gtk_signal_connect (GTK_OBJECT (item), "select",
			    GTK_SIGNAL_FUNC (on_tree_item_select), tree);
	if (!strcmp (path, "/"))
		tree->priv->root = item;

	hbox = gtk_hbox_new (FALSE, 3);
	gtk_widget_show (hbox);
	gtk_container_add (GTK_CONTAINER (item), hbox);

	if (!strcmp (path, "/"))
		pixmap = create_pixmap (GTK_WIDGET (tree), "camera.xpm");
	else
		pixmap = create_pixmap (GTK_WIDGET (tree), "folder.xpm");
	gtk_widget_show (pixmap);
	gtk_box_pack_start (GTK_BOX (hbox), pixmap, FALSE, FALSE, 0);

	if (!strcmp (path, "/")) {
		if (tree->priv->camera)
			label = gtk_label_new (tree->priv->camera->model);
		else
			label = gtk_label_new (_("No camera set"));
	} else
		label = gtk_label_new (g_basename (path));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	/* Return if we don't have a camera */
	if (!tree->priv->camera)
		return;

	/* Subdirectories? */
	gp_list_new (&list);
	result = gp_camera_folder_list_folders (tree->priv->camera, path, list);
	if (result < 0) {
		window = gtk_widget_get_ancestor (GTK_WIDGET (tree),
						  GTK_TYPE_WINDOW);
		msg = g_strdup_printf (_("Could not get list of folders for "
				       "folder '%s'."), path);
		dialog = gtkam_error_new (msg, result,
					  tree->priv->camera, window);
		g_free (msg);
		gtk_widget_show (dialog);
	} else if (gp_list_count (list) > 0) {
		subtree = gtk_tree_new ();
		gtk_widget_show (subtree);
		gtk_tree_item_set_subtree (GTK_TREE_ITEM (item), subtree);
		gp_list_ref (list);
		gtk_object_set_data_full (GTK_OBJECT (item), "list", list,
					  (GtkDestroyNotify) gp_list_unref);
	}
	gp_list_unref (list);
}

static void
on_tree_item_expand (GtkTreeItem *item, GtkamTree *tree)
{
	CameraList *list;
	const char *name;
	const gchar *path;
	gchar *new_path;
	gint i;

	/* Check if we've expanded this item before */
	if (gtk_object_get_data (GTK_OBJECT (item), "expanded"))
		return;

	path = gtk_object_get_data (GTK_OBJECT (item), "path");
	list = gtk_object_get_data (GTK_OBJECT (item), "list");
	if (list) {
		for (i = 0; i < gp_list_count (list); i++) {
			gp_list_get_name (list, i, &name);
			if (!strcmp (path, "/"))
				new_path = g_strdup_printf ("/%s", name);
			else
				new_path = g_strdup_printf ("%s/%s",
							    path, name);
			create_item (tree,
				GTK_TREE (GTK_TREE_ITEM_SUBTREE (item)),
				new_path);
			g_free (new_path);
		}

		gtk_object_set_data (GTK_OBJECT (item), "expanded", 
				     GINT_TO_POINTER (TRUE));
		gtk_tree_item_expand (item);
	}
}

GtkWidget *
gtkam_tree_new (void)
{
	GtkamTree *tree;

	tree = gtk_type_new (GTKAM_TYPE_TREE);

	create_item (tree, GTK_TREE (tree), "/");

	return (GTK_WIDGET (tree));
}

void
gtkam_tree_set_camera (GtkamTree *tree, Camera *camera)
{
	g_return_if_fail (GTKAM_IS_TREE (tree));
	g_return_if_fail (camera != NULL);

	if (tree->priv->camera)
		gp_camera_unref (tree->priv->camera);
	tree->priv->camera = camera;
	gp_camera_ref (camera);

	gtk_container_remove (GTK_CONTAINER (tree), tree->priv->root);
	create_item (tree, GTK_TREE (tree), "/");
}

const gchar *
gtkam_tree_get_path (GtkamTree *tree)
{
	GList *selection;

	g_return_val_if_fail (GTKAM_IS_TREE (tree), NULL);

	selection = GTK_TREE_SELECTION (tree);
	g_return_val_if_fail (g_list_length (selection) == 1, NULL);

	return (gtk_object_get_data (GTK_OBJECT (selection->data), "path"));
}
