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
#include <gtk/gtklabel.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkhbbox.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkpixmap.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gtkam-clist.h"
#include "gtkam-tree.h"
#include "gtkam-mkdir.h"
#include "gtkam-error.h"

struct _GtkamFSelPrivate
{
	Camera *camera;

	GtkamTree *tree;
	GtkamCList *clist;

	GtkWidget *mkdir, *rmdir;
};

#define PARENT_TYPE GTK_TYPE_DIALOG
static GtkDialogClass *parent_class;

enum {
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void
gtkam_fsel_destroy (GtkObject *object)
{
	GtkamFSel *fsel = GTKAM_FSEL (object);

	if (fsel->priv->camera) {
		gp_camera_unref (fsel->priv->camera);
		fsel->priv->camera = NULL;
	}

	if (fsel->selection) {
		g_list_free (fsel->selection);
		fsel->selection = NULL;
	}

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

	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

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
on_folder_selected (GtkamTree *tree, const gchar *path, GtkamFSel *fsel)
{
	gtk_widget_set_sensitive (fsel->priv->mkdir, TRUE);
	gtk_widget_set_sensitive (fsel->priv->rmdir, TRUE);
	gtkam_clist_set_path (fsel->priv->clist, path);
}

static void
on_folder_unselected (GtkamTree *tree, const gchar *path, GtkamFSel *fsel)
{
	gtk_widget_set_sensitive (fsel->priv->mkdir, FALSE);
	gtk_widget_set_sensitive (fsel->priv->rmdir, FALSE);
	gtkam_clist_set_path (fsel->priv->clist, NULL);
}

static void
on_file_selected (GtkamCList *clist, const gchar *path, GtkamFSel *fsel)
{
	gtk_widget_set_sensitive (fsel->ok_button, TRUE);
	fsel->selection = g_list_append (fsel->selection, (gchar *) path);
}

static void
on_file_unselected (GtkamCList *clist, const gchar *path, GtkamFSel *fsel)
{
	gtk_widget_set_sensitive (fsel->ok_button, FALSE);
	fsel->selection = g_list_remove (fsel->selection, (gchar *) path);
}

static void
on_dir_created (GtkamMkdir *mkdir, const char *path, GtkamFSel *fsel)
{
	gtkam_tree_make_dir (fsel->priv->tree, path);
}

static void
on_mkdir_clicked (GtkButton *button, GtkamFSel *fsel)
{
	GtkWidget *mkdir;
	const gchar *path;

	path = gtkam_tree_get_path (fsel->priv->tree);
	mkdir = gtkam_mkdir_new (fsel->priv->camera, path, NULL);
	gtk_widget_show (mkdir);
	gtk_signal_connect (GTK_OBJECT (mkdir), "dir_created",
			    GTK_SIGNAL_FUNC (on_dir_created), fsel);
}

static void
on_rmdir_clicked (GtkButton *button, GtkamFSel *fsel)
{
	const gchar *path;
        gchar *dirname, *msg;
        int result;
        GtkWidget *dialog;

        path = gtkam_tree_get_path (fsel->priv->tree);
        dirname = g_dirname (path);
        result = gp_camera_folder_remove_dir (fsel->priv->camera, dirname,
                                              g_basename (path));
        if (result < 0) {
                msg = g_strdup_printf (_("Could not remove '%s' from '%s'"),
                                       g_basename (path), dirname);
                dialog = gtkam_error_new (msg, result, fsel->priv->camera,
					  NULL);
                g_free (msg);
                gtk_widget_show (dialog);
        } else
                gtkam_tree_remove_dir (fsel->priv->tree, path);
        g_free (dirname);
}

GtkWidget *
gtkam_fsel_new (Camera *camera, GtkWidget *opt_window)
{
	GtkamFSel *fsel;
	GtkWidget *tree, *button, *image, *hbox, *scrolled, *bbox, *clist;
	GdkPixmap *pixmap;
	GdkBitmap *bitmap;
	GdkPixbuf *pixbuf;
	CameraAbilities a;

	g_return_val_if_fail (camera != NULL, NULL);

	fsel = gtk_type_new (GTKAM_TYPE_FSEL);
	gtk_container_set_border_width (
			GTK_CONTAINER (GTK_DIALOG (fsel)->vbox), 5);
	gtk_widget_set_usize (GTK_WIDGET (fsel), 400, 400);

	gtk_signal_connect (GTK_OBJECT (fsel), "delete_event",
			    GTK_SIGNAL_FUNC (gtk_object_destroy), NULL);

	fsel->priv->camera = camera;
	gp_camera_ref (camera);

	hbox = gtk_hbox_new (FALSE, 5);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (fsel)->vbox), hbox,
			    FALSE, FALSE, 0);

	/* Make / remove dir */
	bbox = gtk_hbutton_box_new ();
	gtk_widget_show (bbox);
	gtk_box_pack_start (GTK_BOX (hbox), bbox, FALSE, FALSE, 0);
	button = gtk_button_new_with_label (_("Make"));
	gtk_widget_set_sensitive (button, FALSE);
	gtk_container_add (GTK_CONTAINER (bbox), button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_mkdir_clicked), fsel);
	fsel->priv->mkdir = button;
	button = gtk_button_new_with_label (_("Remove"));
	gtk_widget_set_sensitive (button, FALSE);
	gtk_container_add (GTK_CONTAINER (bbox), button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_rmdir_clicked), fsel);
	fsel->priv->rmdir = button;

	gp_camera_get_abilities (camera, &a);
	if (a.folder_operations & GP_FOLDER_OPERATION_MAKE_DIR)
		gtk_widget_show (fsel->priv->mkdir);
	if (a.folder_operations & GP_FOLDER_OPERATION_REMOVE_DIR)
		gtk_widget_show (fsel->priv->rmdir);

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

	hbox = gtk_hbox_new (TRUE, 5);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (fsel)->vbox), hbox,
			    TRUE, TRUE, 0);

	/* Tree */
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolled);
	gtk_box_pack_start (GTK_BOX (hbox), scrolled, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	tree = gtkam_tree_new ();
	gtk_widget_show (tree);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled),
					       tree);
	gtkam_tree_set_camera (GTKAM_TREE (tree), camera);
	fsel->priv->tree = GTKAM_TREE (tree);
	gtk_signal_connect (GTK_OBJECT (tree), "folder_selected",
			    GTK_SIGNAL_FUNC (on_folder_selected), fsel);
	gtk_signal_connect (GTK_OBJECT (tree), "folder_unselected",
			    GTK_SIGNAL_FUNC (on_folder_unselected), fsel);

	/* CList */
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolled);
	gtk_box_pack_start (GTK_BOX (hbox), scrolled, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	clist = gtkam_clist_new ();
	gtk_widget_show (clist);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled),
					       clist);
	gtkam_clist_set_camera (GTKAM_CLIST (clist), camera);
	fsel->priv->clist = GTKAM_CLIST (clist);
	gtk_signal_connect (GTK_OBJECT (clist), "file_selected",
			    GTK_SIGNAL_FUNC (on_file_selected), fsel);
	gtk_signal_connect (GTK_OBJECT (clist), "file_unselected",
			    GTK_SIGNAL_FUNC (on_file_unselected), fsel);

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
