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

#include <string.h>

#include <gtk/gtklabel.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtkimage.h>

#include "gtkam-error.h"
#include "gtkam-status.h"

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

	gboolean multi;

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
gtkam_mkdir_finalize (GObject *object)
{
	GtkamMkdir *mkdir = GTKAM_MKDIR (object);

	g_free (mkdir->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_mkdir_class_init (gpointer g_class, gpointer class_data)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = GTK_OBJECT_CLASS (g_class);
	object_class->destroy  = gtkam_mkdir_destroy;

	gobject_class = G_OBJECT_CLASS (g_class);
	gobject_class->finalize = gtkam_mkdir_finalize;

	signals[DIR_CREATED] = g_signal_new ("dir_created",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_FIRST, 
		G_STRUCT_OFFSET (GtkamMkdirClass, dir_created), NULL, NULL,
		g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		G_TYPE_POINTER);

	parent_class = g_type_class_peek_parent (g_class);
}

static void
gtkam_mkdir_init (GTypeInstance *instance, gpointer g_class)
{
	GtkamMkdir *mkdir = GTKAM_MKDIR (instance);

	mkdir->priv = g_new0 (GtkamMkdirPrivate, 1);
}

GType
gtkam_mkdir_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo ti;

		memset (&ti, 0, sizeof (GTypeInfo));
		ti.class_size     = sizeof (GtkamMkdirClass);
		ti.class_init     = gtkam_mkdir_class_init;
		ti.instance_size  = sizeof (GtkamMkdir);
		ti.instance_init  = gtkam_mkdir_init;

		type = g_type_register_static (PARENT_TYPE, "GtkamMkdir",
					       &ti, 0);
	}

	return (type);
}

static void
on_cancel_clicked (GtkButton *button, GtkamMkdir *mkdir)
{
	gtk_object_destroy (GTK_OBJECT (mkdir));
}

static void
on_ok_clicked (GtkButton *button, GtkamMkdir *mkdir)
{
	GtkWidget *dialog, *s;
	int r;
	const gchar *path;
	gchar *full_path;
	GtkamMkdirDirCreatedData data;

	path = gtk_entry_get_text (mkdir->priv->entry);
	s = gtkam_status_new (_("Creating folder '%s' in "
		"folder '%s'..."), g_basename (path), mkdir->priv->path);
	gtk_widget_show (s);
	gtk_box_pack_end (GTK_BOX (GTK_DIALOG (mkdir)->vbox), s,
			  FALSE, FALSE, 0);
	r = gp_camera_folder_make_dir (mkdir->priv->camera, mkdir->priv->path,
			g_basename (path), GTKAM_STATUS (s)->context->context);
	if (mkdir->priv->multi)
		gp_camera_exit (mkdir->priv->camera, NULL);
	switch (r) {
	case GP_OK:

		/* Emit the signal */
		memset (&data, 0, sizeof (GtkamMkdirDirCreatedData));
		if (strlen (mkdir->priv->path) > 1)
			full_path = g_strdup_printf ("%s/%s",
					mkdir->priv->path, path);
		else
			full_path = g_strdup_printf ("/%s", path);
		data.camera = mkdir->priv->camera;
		data.multi  = mkdir->priv->multi;
		data.path   = full_path;
		g_signal_emit (GTK_OBJECT (mkdir), signals[DIR_CREATED], 0, 
			       &data);
		g_free (full_path);

		gtk_object_destroy (GTK_OBJECT (mkdir));
		break;
	case GP_ERROR_CANCEL:
		gtk_object_destroy (GTK_OBJECT (mkdir));
		break;
	default:
		dialog = gtkam_error_new (r, GTKAM_STATUS (s)->context,
			GTK_WIDGET (mkdir),
			_("Could not create folder '%s' in folder '%s'."),
			g_basename (path), mkdir->priv->path);
		gtk_widget_show (dialog);
		gtk_object_destroy (GTK_OBJECT (s));
		break;
	}
}

GtkWidget *
gtkam_mkdir_new (Camera *camera, gboolean multi, const gchar *path)
{
	GtkamMkdir *mkdir;
	GtkWidget *label, *entry, *button, *hbox, *vbox, *image;
	gchar *msg;

	g_return_val_if_fail (camera != NULL, NULL);
	g_return_val_if_fail (path != NULL, NULL);

	mkdir = g_object_new (GTKAM_TYPE_MKDIR, NULL);

	mkdir->priv->path = g_strdup (path);
	mkdir->priv->camera = camera;
	gp_camera_ref (camera);
	mkdir->priv->multi = multi;

	hbox = gtk_hbox_new (FALSE, 10);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (mkdir)->vbox), hbox,
			    TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);

	image = gtk_image_new_from_file (IMAGE_DIR "/gtkam-folder.png");
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

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
	g_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_ok_clicked), mkdir);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (mkdir)->action_area),
			   button);
	gtk_widget_grab_focus (button);

	button = gtk_button_new_with_label (_("Cancel"));
	gtk_widget_show (button);
	g_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_cancel_clicked), mkdir);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (mkdir)->action_area),
			   button);

	return (GTK_WIDGET (mkdir));
}
