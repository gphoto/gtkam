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
#include <stdlib.h>

#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtktreeitem.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkmenu.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <gphoto2/gphoto2-setting.h>

#include "support.h"
#include "gtkam-error.h"
#include "gtkam-status.h"

struct _GtkamTreePrivate
{
	Camera *camera;
	gboolean multi;

	GList *selection;

	const gchar *folder;
};

#define PARENT_TYPE GTK_TYPE_TREE_VIEW
static GtkTreeViewClass *parent_class;

enum {
	FOLDER_SELECTED,
	FOLDER_UNSELECTED,
	FILE_UPLOADED,
	NEW_STATUS,
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

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_tree_finalize (GObject *object)
{
	GtkamTree *tree = GTKAM_TREE (object);

	g_free (tree->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

#if 0
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
#endif

static void
gtkam_tree_class_init (gpointer g_class, gpointer class_data)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = GTK_OBJECT_CLASS (g_class);
	object_class->destroy  = gtkam_tree_destroy;
	
	gobject_class = G_OBJECT_CLASS (g_class);
	gobject_class->finalize = gtkam_tree_finalize;

	signals[FOLDER_SELECTED] = g_signal_new ("folder_selected",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkamTreeClass, folder_selected), NULL, NULL,
		g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		G_TYPE_POINTER);
	signals[FOLDER_UNSELECTED] = g_signal_new ("folder_unselected",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkamTreeClass, folder_unselected), NULL, NULL,
		g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		G_TYPE_POINTER);
	signals[FILE_UPLOADED] = g_signal_new ("file_uploaded",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkamTreeClass, file_uploaded), NULL, NULL,
		g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		G_TYPE_POINTER);
	signals[NEW_STATUS] = g_signal_new ("new_status",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkamTreeClass, new_status), NULL, NULL,
		g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		G_TYPE_POINTER);

	parent_class = g_type_class_peek_parent (g_class);
}

static void
gtkam_tree_init (GTypeInstance *instance, gpointer g_class)
{
	GtkamTree *tree = GTKAM_TREE (instance);

	tree->priv = g_new0 (GtkamTreePrivate, 1);
}

GType
gtkam_tree_get_type (void)
{
	GTypeInfo tinfo;

	memset (&tinfo, 0, sizeof (GTypeInfo));
	tinfo.class_size    = sizeof (GtkamTreeClass);
	tinfo.class_init    = gtkam_tree_class_init;
	tinfo.instance_size = sizeof (GtkamTree);
	tinfo.instance_init = gtkam_tree_init;

	return (g_type_register_static (PARENT_TYPE, "GtkamTree", &tinfo, 0));
}

GtkWidget *
gtkam_tree_new (void)
{
	GtkamTree *tree;

	tree = g_object_new (GTKAM_TYPE_TREE, NULL);

#if 0
	/*
	 * Watch out for changes in the selection. Allow selection of
	 * more than one folder.
	 */
	gtk_tree_set_selection_mode (GTK_TREE (tree), GTK_SELECTION_MULTIPLE);
	gtk_signal_connect (GTK_OBJECT (tree), "selection_changed",
			    GTK_SIGNAL_FUNC (on_selection_changed), tree);
#endif

	return (GTK_WIDGET (tree));
}

#if 0
static void
on_new_status (GtkamTreeItem *item, GtkWidget *status, GtkamTree *tree)
{
	gtk_signal_emit (GTK_OBJECT (tree), signals[NEW_STATUS], status);
}
#endif

void
gtkam_tree_add_camera (GtkamTree *tree, Camera *camera, gboolean multi)
{
#if 0
	GtkWidget *item;

	g_return_if_fail (GTKAM_IS_TREE (tree));
	g_return_if_fail (camera != NULL);

	item = gtkam_tree_item_cam_new ();
	gtk_widget_show (item);
	gtk_tree_append (GTK_TREE (tree), item);
	gtk_signal_connect (GTK_OBJECT (item), "new_status",
			    GTK_SIGNAL_FUNC (on_new_status), tree);

	gtkam_tree_item_set_camera (GTKAM_TREE_ITEM (item), camera);
	gtkam_tree_item_set_multi (GTKAM_TREE_ITEM (item), multi);

	gtkam_tree_save (tree);
#endif
}

#if 0
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
#endif

void
gtkam_tree_update (GtkamTree *tree, Camera *camera, gboolean multi,
		   const gchar *folder)
{
#if 0
	update_tree (GTK_TREE (tree), camera, multi, folder);
#endif
}

#if 0
#ifdef HAVE_GP_CAMERA_SET_TIMEOUT_FUNCS

typedef struct _TimeoutData TimeoutData;
struct _TimeoutData {
	Camera *camera;
	CameraTimeoutFunc func;
};

static gboolean
timeout_func (gpointer data)
{
	TimeoutData *td = data;

	td->func (td->camera, NULL);

	/* Repeat forever */
	return (TRUE);
}

static void
timeout_destroy_notify (gpointer data)
{
	TimeoutData *td = data;

	g_free (td);
}

static unsigned int
start_timeout_func (Camera *camera, unsigned int timeout,
		    CameraTimeoutFunc func, void *data)
{
	TimeoutData *td;

	td = g_new0 (TimeoutData, 1);
	td->camera = camera;
	td->func = func;

	return (gtk_timeout_add_full (timeout * 1000, timeout_func, NULL,
				      td, timeout_destroy_notify));
}

static void
stop_timeout_func (Camera *camera, unsigned int id, void *data)
{
	gtk_timeout_remove (id);
}

#endif
#endif

void
gtkam_tree_load (GtkamTree *tree)
{
#if 0
	GtkWidget *s, *item;
	char port[1024], speed[1024], model[1024], multi[1024];
	Camera *camera;
	CameraAbilitiesList *al;
	GPPortInfoList *il;
	GPPortInfo info;
	CameraAbilities a;
	int n, p, result;
	gchar *ms;
	guint i;

	s = gtkam_status_new (_("Loading cameras..."));
	gtk_signal_emit (GTK_OBJECT (tree), signals[NEW_STATUS], s);

	gp_abilities_list_new (&al);
	gp_abilities_list_load (al, GTKAM_STATUS (s)->context->context);
	gp_port_info_list_new (&il);
	gp_port_info_list_load (il);

	/* Old style settings */
	if ((gp_setting_get ("gtkam", "model-1", model) != GP_OK) &&
	    ((gp_setting_get ("gtkam", "model", model) == GP_OK) ||
             (gp_setting_get ("gphoto2", "model", model) == GP_OK)) &&
            ((gp_setting_get ("gtkam", "path", port) == GP_OK) ||
             (gp_setting_get ("gphoto2", "port", port) == GP_OK)) &&
            (gp_setting_get ("gtkam", "speed", speed) == GP_OK)) {
                gp_camera_new (&camera);
#ifdef HAVE_GP_CAMERA_SET_TIMEOUT_FUNCS
		gp_camera_set_timeout_funcs (camera, start_timeout_func,
					     stop_timeout_func, NULL);
#endif

                n = gp_abilities_list_lookup_model (al, model);
                gp_abilities_list_get_abilities (al, n, &a);

                p = gp_port_info_list_lookup_path (il, port);
                if (p < 0) {
                        g_warning ("Could not find '%s' in port info list "
                                "(%s)!", port, gp_result_as_string (p));
			gtk_object_destroy (GTK_OBJECT (s));
			gp_port_info_list_free (il);
			gp_abilities_list_free (al);
			gp_camera_unref (camera);
			return;
		}

                gp_port_info_list_get_info (il, p, &info);

                gp_camera_set_abilities (camera, a);
                if (strcmp (port, "None") && strcmp (model, "Directory Browse"))                        gp_camera_set_port_info (camera, info);
                if (atoi (speed))
                        gp_camera_set_port_speed (camera, atoi (speed));

		/* Create the new item */
		item = gtkam_tree_item_cam_new ();
		gtk_widget_show (item);
		gtk_tree_append (GTK_TREE (tree), item);
		gtk_signal_connect (GTK_OBJECT (item), "new_status",
				    GTK_SIGNAL_FUNC (on_new_status), tree);
		gtkam_tree_item_set_camera (GTKAM_TREE_ITEM (item), camera);
                if (gp_setting_get ("gtkam", "multi", multi) == GP_OK)
			gtkam_tree_item_set_multi (GTKAM_TREE_ITEM (item),
						   atoi (multi));
		gp_camera_unref (camera);
        }

	/* New style settings */
	for (i = 1; ; i++) {
		ms = g_strdup_printf ("model-%i", i);
		result = gp_setting_get ("gtkam", ms, model);
		g_free (ms);
		if (result < 0)
			break;
		if (!strcmp (model, ""))
			break;

		ms = g_strdup_printf ("port-%i", i);
		result = gp_setting_get ("gtkam", ms, port);
		g_free (ms);
		if (result < 0)
			break;

		ms = g_strdup_printf ("multi-%i", i);
		result = gp_setting_get ("gtkam", ms, multi);
		g_free (ms);
		if (result < 0)
			break;

		ms = g_strdup_printf ("speed-%i", i);
		result = gp_setting_get ("gtkam", ms, speed);
		g_free (ms);
		if (result < 0)
			break;

		gp_camera_new (&camera);
#ifdef HAVE_GP_CAMERA_SET_TIMEOUT_FUNCS
                gp_camera_set_timeout_funcs (camera, start_timeout_func,
                                             stop_timeout_func, NULL);
#endif

                n = gp_abilities_list_lookup_model (al, model);
                gp_abilities_list_get_abilities (al, n, &a);

		if (strcmp (port, "") && strcmp (port, "None") &&
		    strcmp (model, "Directory Browse")) {
	                p = gp_port_info_list_lookup_path (il, port);
        	        if (p < 0) {
                	        g_warning ("Could not find '%s' in port "
					"info list (%s)!", port,
					gp_result_as_string (p));
				gp_camera_unref (camera);
				continue;
			}
			gp_port_info_list_get_info (il, p, &info);
			gp_camera_set_port_info (camera, info);
		}

                gp_camera_set_abilities (camera, a);
                if (atoi (speed))
                        gp_camera_set_port_speed (camera, atoi (speed));

                /* Create the new item */
                item = gtkam_tree_item_cam_new ();
                gtk_widget_show (item);
                gtk_tree_append (GTK_TREE (tree), item);
		gtk_signal_connect (GTK_OBJECT (item), "new_status",
				    GTK_SIGNAL_FUNC (on_new_status), tree);
		gtkam_tree_item_set_camera (GTKAM_TREE_ITEM (item), camera);
		gtkam_tree_item_set_multi (GTKAM_TREE_ITEM (item),
					   atoi (multi));
		gp_camera_unref (camera);
	}

	gp_abilities_list_free (al);
	gp_port_info_list_free (il);
	gtk_object_destroy (GTK_OBJECT (s));
#endif
}

void
gtkam_tree_save (GtkamTree *tree)
{
#if 0
	guint i;
	Camera *camera;
	gboolean multi;
	GPPortInfo info;
	CameraAbilities a;
	int speed, result;
	gchar *ms, *mm;
	char model[1024];

	/* Clear old settings */
	if (gp_setting_get ("gtkam", "model", model)) {
		memset (model, 0, sizeof (model));
		gp_setting_set ("gtkam", "model", model);
	}

	/* Clear the configuration file */
	for (i = 1; ; i++) {
		ms = g_strdup_printf ("model-%i", i);
		result = gp_setting_get ("gtkam", ms, model);
		g_free (ms);
		if (result < 0)
			break;

		memset (model, 0, sizeof (model));
		ms = g_strdup_printf ("model-%i", i);
		gp_setting_set ("gtkam", ms, model);
		g_free (ms);
	}

	for (i = 0; i < g_list_length (GTK_TREE (tree)->children); i++) {
		item = g_list_nth_data (GTK_TREE (tree)->children, i);
		camera = gtkam_tree_item_get_camera (item);
		if (!camera)
			continue;
		multi = gtkam_tree_item_get_multi (item);

		gp_camera_get_abilities (camera, &a);
		gp_camera_get_port_info (camera, &info);
		speed = gp_camera_get_port_speed (camera);

		ms = g_strdup_printf ("model-%i", i + 1);
		gp_setting_set ("gtkam", ms, a.model);
		g_free (ms);

		ms = g_strdup_printf ("port-%i", i + 1);
		gp_setting_set ("gtkam", ms, info.path);
		g_free (ms);

		ms = g_strdup_printf ("multi-%i", i + 1);
		mm = g_strdup_printf ("%i", multi ? 1 : 0);
		gp_setting_set ("gtkam", ms, mm);
		g_free (mm);
		g_free (ms);

		ms = g_strdup_printf ("speed-%i", i + 1);
		mm = g_strdup_printf ("%i", speed);
		gp_setting_set ("gtkam", ms, mm);
		g_free (mm);
		g_free (ms);
	}
#endif
}
