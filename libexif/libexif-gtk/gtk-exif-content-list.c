/* gtk-exif-content-list.c
 *
 * Copyright (C) 2001 Lutz Müller <lutz@users.sourceforge.net>
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
#include "gtk-exif-content-list.h"

struct _GtkExifContentListPrivate {
	ExifContent *content;
};

#define PARENT_TYPE GTK_TYPE_CLIST
static GtkCListClass *parent_class;

static void
gtk_exif_content_list_destroy (GtkObject *object)
{
	GtkExifContentList *list = GTK_EXIF_CONTENT_LIST (object);

	if (list->priv->content) {
		exif_content_unref (list->priv->content);
		list->priv->content = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtk_exif_content_list_finalize (GtkObject *object)
{
	GtkExifContentList *list = GTK_EXIF_CONTENT_LIST (object);

	g_free (list->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtk_exif_content_list_class_init (GtkExifContentListClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtk_exif_content_list_destroy;
	object_class->finalize = gtk_exif_content_list_finalize;

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtk_exif_content_list_init (GtkExifContentList *list)
{
	list->priv = g_new0 (GtkExifContentListPrivate, 1);
}

GtkType
gtk_exif_content_list_get_type (void)
{
	static GtkType content_list_type = 0;

	if (!content_list_type) {
		static const GtkTypeInfo list_info = {
			"GtkExifContentList",
			sizeof (GtkExifContentList),
			sizeof (GtkExifContentListClass),
			(GtkClassInitFunc)  gtk_exif_content_list_class_init,
			(GtkObjectInitFunc) gtk_exif_content_list_init,
			NULL, NULL, NULL};
		content_list_type = gtk_type_unique (PARENT_TYPE, &list_info);
	}

	return (content_list_type);
}

GtkWidget *
gtk_exif_content_list_new (void)
{
	GtkExifContentList *list;
	const gchar *titles[] = {"Tag", "Value"};

	list = gtk_type_new (GTK_EXIF_TYPE_CONTENT_LIST);
	gtk_clist_construct (GTK_CLIST (list), 2, (gchar **) titles);

	return (GTK_WIDGET (list));
}

void
gtk_exif_content_list_set_content (GtkExifContentList *list,
				   ExifContent *content)
{
	guint i;
	const gchar *text[2];

	g_return_if_fail (GTK_EXIF_IS_CONTENT_LIST (list));
	g_return_if_fail (content != NULL);

	if (list->priv->content)
		exif_content_unref (list->priv->content);
	list->priv->content = content;
	exif_content_ref (content);

	gtk_clist_clear (GTK_CLIST (list));
	for (i = 0; i < content->count; i++) {
		text[0] = exif_tag_get_name (content->entries[i]->tag);
		text[1] = exif_entry_get_value (content->entries[i]);
		gtk_clist_append (GTK_CLIST (list), (gchar **) text);
	}
}
