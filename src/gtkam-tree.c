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
#include <string.h>

#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtktreeitem.h>
#include <gtk/gtkpixmap.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "support.h"
#include "gtkam-error.h"
#include "gtkam-status.h"
#include "gtkam-tree-item-cam.h"
#include "gtkam-tree-item-dir.h"

struct _GtkamTreePrivate
{
	Camera *camera;
	gboolean multi;

	GtkWidget *status;

	GList *selection;

	const gchar *folder;
};

#define PARENT_TYPE GTK_TYPE_TREE
static GtkTreeClass *parent_class;

enum {
	FOLDER_SELECTED,
	FOLDER_UNSELECTED,
	FILE_UPLOADED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void
gtkam_tree_destroy (GtkObject *object)
{
	GtkamTree *tree = GTKAM_TREE (object);

	if (tree->priv->camera) {
		gp_camera_unref (tree->priv->camera);
		tree->priv->camera = NULL;
	}

	if (tree->priv->status) {
		gtk_object_unref (GTK_OBJECT (tree->priv->status));
		tree->priv->status = NULL;
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
on_selection_changed (GtkTree *t, GtkamTree *tree)
{
	GtkamTreeItem *item;
	guint i;

	/* Any folder selected? */
	for (i = 0; i < g_list_length (t->selection); i++) {
		item = g_list_nth_data (t->selection, i);
		if (!g_list_find (tree->priv->selection, item)) {
			gtk_signal_emit (GTK_OBJECT (tree),
				signals[FOLDER_SELECTED],
				gtkam_tree_item_get_camera (item),
				gtkam_tree_item_get_multi (item),
				gtkam_tree_item_get_folder (item));
			tree->priv->selection =
				g_list_append (tree->priv->selection, item);
		}
	}

	/* Any folder unselected? */
	for (i = 0; i < g_list_length (tree->priv->selection); i++) {
		item = g_list_nth_data (tree->priv->selection, i);
		if (!g_list_find (t->selection, item)) {
			gtk_signal_emit (GTK_OBJECT (tree), 
				signals[FOLDER_UNSELECTED],
				gtkam_tree_item_get_camera (item),
				gtkam_tree_item_get_multi (item),
				gtkam_tree_item_get_folder (item));
			tree->priv->selection = 
				g_list_remove (tree->priv->selection, item);
		}
	}
}

typedef void (* GtkamSignal_NONE__POINTER_BOOL_POINTER) (GtkObject *object,
	 gpointer arg1, gboolean arg2, gpointer arg3, gpointer user_data);

static void
gtkam_marshal_NONE__POINTER_BOOL_POINTER (GtkObject *object,
					  GtkSignalFunc func,
					  gpointer func_data,
					  GtkArg *args)
{
	GtkamSignal_NONE__POINTER_BOOL_POINTER rfunc;

	rfunc = (GtkamSignal_NONE__POINTER_BOOL_POINTER) func;
	(*rfunc) (object, GTK_VALUE_POINTER (args[0]),
		  	  GTK_VALUE_BOOL (args[1]),
			  GTK_VALUE_POINTER (args[2]), func_data);
}

typedef void (* GtkamSignal_NONE__POINTER_BOOL_POINTER_POINTER)
	(GtkObject *object, gpointer arg1, gboolean arg2, gpointer arg3, 
	 gpointer arg4, gpointer user_data);

static void
gtkam_marshal_NONE__POINTER_BOOL_POINTER_POINTER (GtkObject *object,
						  GtkSignalFunc func,
						  gpointer func_data,
						  GtkArg *args)
{
	GtkamSignal_NONE__POINTER_BOOL_POINTER_POINTER rfunc;

	rfunc = (GtkamSignal_NONE__POINTER_BOOL_POINTER_POINTER) func;
	(*rfunc) (object, GTK_VALUE_POINTER (args[0]),
		  	  GTK_VALUE_BOOL (args[1]),
			  GTK_VALUE_POINTER (args[2]),
			  GTK_VALUE_POINTER (args[3]), func_data);
}

static void
gtkam_tree_class_init (GtkamTreeClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_tree_destroy;
	object_class->finalize = gtkam_tree_finalize;

	signals[FOLDER_SELECTED] = gtk_signal_new ("folder_selected",
		GTK_RUN_FIRST, object_class->type,
		GTK_SIGNAL_OFFSET (GtkamTreeClass, folder_selected),
		gtkam_marshal_NONE__POINTER_BOOL_POINTER, GTK_TYPE_NONE, 3,
		GTK_TYPE_POINTER, GTK_TYPE_BOOL, GTK_TYPE_POINTER);
	signals[FOLDER_UNSELECTED] = gtk_signal_new ("folder_unselected",
		GTK_RUN_FIRST, object_class->type,
		GTK_SIGNAL_OFFSET (GtkamTreeClass, folder_unselected),
		gtkam_marshal_NONE__POINTER_BOOL_POINTER, GTK_TYPE_NONE, 3,
		GTK_TYPE_POINTER, GTK_TYPE_BOOL, GTK_TYPE_POINTER);
	signals[FILE_UPLOADED] = gtk_signal_new ("file_uploaded",
		GTK_RUN_FIRST, object_class->type,
		GTK_SIGNAL_OFFSET (GtkamTreeClass, file_uploaded),
		gtkam_marshal_NONE__POINTER_BOOL_POINTER_POINTER,
		GTK_TYPE_NONE, 4, GTK_TYPE_POINTER, GTK_TYPE_BOOL,
		GTK_TYPE_POINTER, GTK_TYPE_POINTER);
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

GtkWidget *
gtkam_tree_new (GtkWidget *vbox)
{
	GtkamTree *tree;

	g_return_val_if_fail (GTK_IS_VBOX (vbox), NULL);

	tree = gtk_type_new (GTKAM_TYPE_TREE);

	tree->priv->status = vbox;
	gtk_object_ref (GTK_OBJECT (tree->priv->status));

	/*
	 * Watch out for changes in the selection. Allow selection of
	 * more than one folder.
	 */
	gtk_tree_set_selection_mode (GTK_TREE (tree), GTK_SELECTION_MULTIPLE);
	gtk_signal_connect (GTK_OBJECT (tree), "selection_changed",
			    GTK_SIGNAL_FUNC (on_selection_changed), tree);

	return (GTK_WIDGET (tree));
}

void
gtkam_tree_add_camera (GtkamTree *tree, Camera *camera, gboolean multi)
{
	GtkWidget *item;

	g_return_if_fail (GTKAM_IS_TREE (tree));
	g_return_if_fail (camera != NULL);

	item = gtkam_tree_item_cam_new (tree->priv->status);
	gtk_widget_show (item);
	gtk_tree_append (GTK_TREE (tree), item);

	gtkam_tree_item_set_camera (GTKAM_TREE_ITEM (item), camera, multi);
}

static void
update_tree (GtkTree *tree, Camera *camera, gboolean multi,
	     const gchar *folder)
{
	GtkamTreeItem *item;
	gint i;
	const gchar *f;

	g_return_if_fail (GTKAM_IS_TREE (tree));

	for (i = 0; i < g_list_length (GTK_TREE (tree)->children); i++) {
		item = g_list_nth_data (GTK_TREE (tree)->children, i);
		if (camera != gtkam_tree_item_get_camera (item))
			continue;
		if (multi != gtkam_tree_item_get_multi (item))
			continue;
		f = gtkam_tree_item_get_folder (item);
		if (!strcmp (f, folder))
			gtkam_tree_item_update (item);
		else if (GTK_TREE_ITEM (item)->subtree)
			update_tree (GTK_TREE (GTK_TREE_ITEM (item)->subtree),
				     camera, multi, folder);
	}
}

void
gtkam_tree_update (GtkamTree *tree, Camera *camera, gboolean multi,
		   const gchar *folder)
{
	update_tree (GTK_TREE (tree), camera, multi, folder);
}
