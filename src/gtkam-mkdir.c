/* gtkam-mkdir.c
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
#include "gtkam-mkdir.h"

#include <gtk/gtklabel.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkpixmap.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gtkam-error.h"

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

struct _GtkamMkdirPrivate
{
	Camera *camera;
	gchar *path;

	GtkEntry *entry;
};

enum {
	DIR_CREATED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

#define PARENT_TYPE GTK_TYPE_DIALOG
static GtkDialogClass *parent_class;

static void
gtkam_mkdir_destroy (GtkObject *object)
{
	GtkamMkdir *mkdir = GTKAM_MKDIR (object);

	if (mkdir->priv->camera) {
		gp_camera_unref (mkdir->priv->camera);
		mkdir->priv->camera = NULL;
	}

	if (mkdir->priv->path) {
		g_free (mkdir->priv->path);
		mkdir->priv->path = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_mkdir_finalize (GtkObject *object)
{
	GtkamMkdir *mkdir = GTKAM_MKDIR (object);

	g_free (mkdir->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_mkdir_class_init (GtkamMkdirClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_mkdir_destroy;
	object_class->finalize = gtkam_mkdir_finalize;

	signals[DIR_CREATED] = gtk_signal_new ("dir_created",
		GTK_RUN_LAST, object_class->type,
		GTK_SIGNAL_OFFSET (GtkamMkdirClass, dir_created),
		gtk_marshal_NONE__POINTER, GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_mkdir_init (GtkamMkdir *mkdir)
{
	mkdir->priv = g_new0 (GtkamMkdirPrivate, 1);
}

GtkType
gtkam_mkdir_get_type (void)
{
	static GtkType mkdir_type = 0;

	if (!mkdir_type) {
		static const GtkTypeInfo mkdir_info = {
			"GtkamMkdir",
			sizeof (GtkamMkdir),
			sizeof (GtkamMkdirClass),
			(GtkClassInitFunc)  gtkam_mkdir_class_init,
			(GtkObjectInitFunc) gtkam_mkdir_init,
			NULL, NULL, NULL};
		mkdir_type = gtk_type_unique (PARENT_TYPE, &mkdir_info);
	}

	return (mkdir_type);
}

static void
on_cancel_clicked (GtkButton *button, GtkamMkdir *mkdir)
{
	gtk_object_destroy (GTK_OBJECT (mkdir));
}

static void
on_ok_clicked (GtkButton *button, GtkamMkdir *mkdir)
{
	GtkWidget *dialog;
	int r;
	const gchar *path;
	gchar *msg;

	path = gtk_entry_get_text (mkdir->priv->entry);
	r = gp_camera_folder_make_dir (mkdir->priv->camera, mkdir->priv->path,
				       g_basename (path));
	if (r < 0) {
		msg = g_strdup_printf (_("Could not create new directory "
				       "'%s'"), path);
		dialog = gtkam_error_new (msg, r, mkdir->priv->camera,
					  GTK_WIDGET (mkdir));
		g_free (msg);
		gtk_widget_show (dialog);
	} else
		gtk_signal_emit (GTK_OBJECT (mkdir),
				 signals[DIR_CREATED], path);
	gtk_object_destroy (GTK_OBJECT (mkdir));
}

GtkWidget *
gtkam_mkdir_new (Camera *camera, const gchar *path, GtkWidget *opt_window)
{
	GtkamMkdir *mkdir;
	GtkWidget *label, *entry, *button, *hbox, *vbox, *image;
	gchar *msg;
	GdkPixmap *pixmap;
	GdkBitmap *bitmap;
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (camera != NULL, NULL);
	g_return_val_if_fail (path != NULL, NULL);

	mkdir = gtk_type_new (GTKAM_TYPE_MKDIR);
	mkdir->priv->path = g_strdup (path);
	mkdir->priv->camera = camera;
	gp_camera_ref (camera);
	if (opt_window)
		gtk_window_set_transient_for (GTK_WINDOW (mkdir),
					      GTK_WINDOW (opt_window));

	hbox = gtk_hbox_new (FALSE, 10);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (mkdir)->vbox), hbox,
			    TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);

	pixbuf = gdk_pixbuf_new_from_file (IMAGE_DIR "/gtkam-folder.png");
	if (!pixbuf) {
		g_warning ("Could not load " IMAGE_DIR "/gtkam-folder.png");
	} else {
		gdk_pixbuf_render_pixmap_and_mask (pixbuf, &pixmap, &bitmap, 127);
		gdk_pixbuf_unref (pixbuf);
		image = gtk_pixmap_new (pixmap, bitmap);
		if (pixmap)
			gdk_pixmap_unref (pixmap);
		if (bitmap)
			gdk_bitmap_unref (bitmap);
		gtk_widget_show (image);
		gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	}

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

	msg = g_strdup_printf (_("Please choose a name "
				 "for the directory that "
				 "should be created in "
				 "'%s':"), path);
	label = gtk_label_new (msg);
	g_free (msg);
	gtk_widget_show (label);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start (GTK_BOX (vbox), label,
			    TRUE, TRUE, 0);

	entry = gtk_entry_new ();
	gtk_widget_show (entry);
	gtk_box_pack_start (GTK_BOX (vbox), entry,
			    TRUE, TRUE, 0);
	gtk_entry_set_text (GTK_ENTRY (entry), _("New directory"));
	mkdir->priv->entry = GTK_ENTRY (entry);

	button = gtk_button_new_with_label (_("Ok"));
	gtk_widget_show (button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_ok_clicked), mkdir);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (mkdir)->action_area),
			   button);
	gtk_widget_grab_focus (button);

	button = gtk_button_new_with_label (_("Cancel"));
	gtk_widget_show (button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_cancel_clicked), mkdir);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (mkdir)->action_area),
			   button);

	return (GTK_WIDGET (mkdir));
}
