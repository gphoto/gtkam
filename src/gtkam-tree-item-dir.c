/* gtkam-tree-item-dir.c:
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
#include "gtkam-tree-item-dir.h"

#include <gtk/gtklabel.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtkhbox.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

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

struct _GtkamTreeItemDirPrivate
{
};

#define PARENT_TYPE GTKAM_TYPE_TREE_ITEM
static GtkamTreeItemClass *parent_class;
	
static void
gtkam_tree_item_dir_destroy (GtkObject *object)
{
	GtkamTreeItemDir *item = GTKAM_TREE_ITEM_DIR (object);

	item = NULL;

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_tree_item_dir_finalize (GtkObject *object)
{
	GtkamTreeItemDir *item = GTKAM_TREE_ITEM_DIR (object);

	g_free (item->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_tree_item_dir_class_init (GtkamTreeItemDirClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_tree_item_dir_destroy;
	object_class->finalize = gtkam_tree_item_dir_finalize;

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_tree_item_dir_init (GtkamTreeItemDir *item)
{
	item->priv = g_new0 (GtkamTreeItemDirPrivate, 1);
}

GtkType
gtkam_tree_item_dir_get_type (void)
{
	static GtkType dir_type = 0;

	if (!dir_type) {
		static const GtkTypeInfo dir_info = {
			"GtkamTreeItemDir",
			sizeof (GtkamTreeItemDir),
			sizeof (GtkamTreeItemDirClass),
			(GtkClassInitFunc)  gtkam_tree_item_dir_class_init,
			(GtkObjectInitFunc) gtkam_tree_item_dir_init,
			NULL, NULL, NULL};
		dir_type = gtk_type_unique (PARENT_TYPE, &dir_info);
	}

	return (dir_type);
}

GtkWidget *
gtkam_tree_item_dir_new (void)
{
	GtkamTreeItemDir *item;
	GdkPixbuf *pixbuf;

	item = gtk_type_new (GTKAM_TYPE_TREE_ITEM_DIR);

	pixbuf = gdk_pixbuf_new_from_file (IMAGE_DIR "/gtkam-folder.png");
	gtkam_tree_item_construct (GTKAM_TREE_ITEM (item), pixbuf);
	gdk_pixbuf_unref (pixbuf);

	return (GTK_WIDGET (item));
}
