/* gtkam-exif.c
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
#include "gtkam-exif.h"

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

#include <string.h>

#include <gtk/gtkbutton.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkbox.h>

#include <libexif-gtk/gtk-exif-browser.h>

#include "gtkam-error.h"

struct _GtkamExifPrivate
{
};

#define PARENT_TYPE GTK_TYPE_DIALOG
static GtkDialogClass *parent_class;

static void
gtkam_exif_destroy (GtkObject *object)
{
	GtkamExif *exif = GTKAM_EXIF (object);

	exif = NULL;

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_exif_finalize (GtkObject *object)
{
	GtkamExif *exif = GTKAM_EXIF (object);

	g_free (exif->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_exif_class_init (GtkamExifClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_exif_destroy;
	object_class->finalize = gtkam_exif_finalize;

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_exif_init (GtkamExif *exif)
{
	exif->priv = g_new0 (GtkamExifPrivate, 1);
}

GtkType
gtkam_exif_get_type (void)
{
	static GtkType exif_type = 0;

	if (!exif_type) {
		static const GtkTypeInfo exif_info = {
			"GtkamExif",
			sizeof (GtkamExif),
			sizeof (GtkamExifClass),
			(GtkClassInitFunc)  gtkam_exif_class_init,
			(GtkObjectInitFunc) gtkam_exif_init,
			NULL, NULL, NULL};
		exif_type = gtk_type_unique (PARENT_TYPE, &exif_info);
	}

	return (exif_type);
}

static void
on_exif_close_clicked (GtkButton *button, GtkamExif *exif)
{
	gtk_object_destroy (GTK_OBJECT (exif));
}

GtkWidget *
gtkam_exif_new (Camera *camera, gboolean multi,
		const gchar *folder, const gchar *file, GtkWidget *opt_window)
{
	GtkamExif *exif;
	GtkWidget *button, *dialog, *browser;
	CameraFile *cfile;
	int result;
	gchar *msg;
	const char *data;
	long int size;
	ExifData *edata;

	g_return_val_if_fail (camera != NULL, NULL);

	/* Get exif data */
	gp_file_new (&cfile);
	result = gp_camera_file_get (camera, folder, file, GP_FILE_TYPE_EXIF,
				     cfile);
	if (result < 0) {
		gp_file_unref (cfile);
		msg = g_strdup_printf (_("Could not get exif information for "
			"'%s' in folder '%s'"), file, folder);
		dialog = gtkam_error_new (msg, result, camera, opt_window);
		g_free (msg);
		gtk_widget_show (dialog);
		return (NULL);
	}
	gp_file_get_data_and_size (cfile, &data, &size);
	edata = exif_data_new_from_data (data, size);
	gp_file_unref (cfile);
	if (!edata) {
		dialog = gtkam_error_new ("Could not interpret exif data",
				GP_ERROR_CORRUPTED_DATA, NULL, opt_window);
		gtk_widget_show (dialog);
	}

	exif = gtk_type_new (GTKAM_TYPE_EXIF);
	gtk_signal_connect (GTK_OBJECT (exif), "delete_event",
			    GTK_SIGNAL_FUNC (gtk_object_destroy), NULL);

	browser = gtk_exif_browser_new ();
	gtk_exif_browser_set_data (GTK_EXIF_BROWSER (browser), edata);
	exif_data_unref (edata);
	gtk_widget_show (browser);
	gtk_container_set_border_width (GTK_CONTAINER (browser), 5);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (exif)->vbox), browser,
			    TRUE, TRUE, 0);

	button = gtk_button_new_with_label (_("Close"));
	gtk_widget_show (button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_exif_close_clicked), exif);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (exif)->action_area),
			   button);
	gtk_widget_grab_focus (button);

	if (opt_window)
		gtk_window_set_transient_for (GTK_WINDOW (exif),
					      GTK_WINDOW (opt_window));

	return (GTK_WIDGET (exif));
}
