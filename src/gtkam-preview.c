/* gtkam-preview.c
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
#include "gtkam-preview.h"

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
#include <gtk/gtklabel.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkpixmap.h>
#include <gdk-pixbuf/gdk-pixbuf-loader.h>

#include <gphoto2/gphoto2-camera.h>

#include <gtkam-error.h>

struct _GtkamPreviewPrivate
{
	GtkWidget *spin;
	GtkPixmap *image;

	Camera *camera;

	guint rotate;

	guint32 timeout;
	guint32 timeout_id;
};

#define PARENT_TYPE GTK_TYPE_DIALOG
static GtkDialogClass *parent_class;

enum {
	CAPTURED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void
gtkam_preview_destroy (GtkObject *object)
{
	GtkamPreview *preview = GTKAM_PREVIEW (object);

	if (preview->priv->timeout_id) {
		gtk_timeout_remove (preview->priv->timeout_id);
		preview->priv->timeout_id = 0;
	}

	if (preview->priv->camera) {
		gp_camera_unref (preview->priv->camera);
		preview->priv->camera = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_preview_finalize (GtkObject *object)
{
	GtkamPreview *preview = GTKAM_PREVIEW (object);

	g_free (preview->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_preview_class_init (GtkamPreviewClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_preview_destroy;
	object_class->finalize = gtkam_preview_finalize;

	signals[CAPTURED] = gtk_signal_new ("captured", GTK_RUN_LAST,
		object_class->type,
		GTK_SIGNAL_OFFSET (GtkamPreviewClass, captured),
		gtk_marshal_NONE__POINTER, GTK_TYPE_NONE, 1,
		GTK_TYPE_POINTER);
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_preview_init (GtkamPreview *preview)
{
	preview->priv = g_new0 (GtkamPreviewPrivate, 1);
	preview->priv->timeout = 5000;
}

GtkType
gtkam_preview_get_type (void)
{
	static GtkType preview_type = 0;

	if (!preview_type) {
		static const GtkTypeInfo preview_info = {
			"GtkamPreview",
			sizeof (GtkamPreview),
			sizeof (GtkamPreviewClass),
			(GtkClassInitFunc)  gtkam_preview_class_init,
			(GtkObjectInitFunc) gtkam_preview_init,
			NULL, NULL, NULL};
		preview_type = gtk_type_unique (PARENT_TYPE, &preview_info);
	}

	return (preview_type);
}

static void
on_preview_close_clicked (GtkButton *button, GtkamPreview *preview)
{
	gtk_object_destroy (GTK_OBJECT (preview));
}

static void
on_preview_capture_clicked (GtkButton *button, GtkamPreview *preview)
{
	int result;
	CameraFilePath path;
	GtkWidget *dialog;
	gchar *full_path;

	result = gp_camera_capture (preview->priv->camera,
				    GP_OPERATION_CAPTURE_IMAGE, &path);
	if (result != GP_OK) {
		dialog = gtkam_error_new (_("Could not capture"),
			result, preview->priv->camera, GTK_WIDGET (preview));
		gtk_widget_show (dialog);
	} else {
		if (!strcmp (path.folder, "/"))
			full_path = g_strdup_printf ("/%s", path.name);
		else
			full_path = g_strdup_printf ("%s/%s", path.folder,
						     path.name);
		gtk_signal_emit (GTK_OBJECT (preview), signals[CAPTURED],
				 full_path);
		g_free (full_path);
		gtk_object_destroy (GTK_OBJECT (preview));
	}
}

struct _GdkPixbuf {
	int ref_count;
	GdkColorspace colorspace;
	int n_channels;
	int bits_per_sample;
	int width, height;
	int rowstride;
	guchar *pixels;
	GdkPixbufDestroyNotify destroy_fn;
	gpointer destroy_fn_data;
	GdkPixbufLastUnref last_unref_fn;
	gpointer last_unref_fn_data;
	guint has_alpha : 1;
};

#define SWAP(c1,c2) {guchar c = (c1); (c1) = (c2); (c2) = c;}

static void
gdk_pixbuf_rotate (GdkPixbuf *pixbuf, guint angle)
{
	guint row, col, row_len;
	guint channels;

	g_return_if_fail (pixbuf != NULL);

	channels = pixbuf->has_alpha ? 4 : 3;
	row_len = pixbuf->width * channels;

	switch (angle) {
	case 0:
		break;
	case 90:
		break;
	case 180:
		for (row = 0; row < pixbuf->height / 2; row++) {
			for (col = 0; col < pixbuf->width; col++) {
SWAP (pixbuf->pixels[row * row_len + col * channels + 0],
      pixbuf->pixels[(pixbuf->height - row) * row_len - col * channels + 0]);
SWAP (pixbuf->pixels[row * row_len + col * channels + 1],
      pixbuf->pixels[(pixbuf->height - row) * row_len - col * channels + 1]);
SWAP (pixbuf->pixels[row * row_len + col * channels + 2],
      pixbuf->pixels[(pixbuf->height - row) * row_len - col * channels + 2]);
			}
		}
		if (pixbuf->height % 2) {
			for (col = 0; col < pixbuf->width / 2; col++) {
SWAP (pixbuf->pixels[row * row_len + col * channels + 0],
      pixbuf->pixels[(pixbuf->height - row) * row_len - col * channels + 0]);
SWAP (pixbuf->pixels[row * row_len + col * channels + 1],
      pixbuf->pixels[(pixbuf->height - row) * row_len - col * channels + 1]);
SWAP (pixbuf->pixels[row * row_len + col * channels + 2],
      pixbuf->pixels[(pixbuf->height - row) * row_len - col * channels + 2]);
			}
		}

		break;
	case 270:
		break;
	default:
		g_warning ("Rotation by %i not implemented.", angle);
		break;
	}
}

#undef SWAP

static gboolean
timeout_func (gpointer user_data)
{
	int result;
	CameraFile *file;
	const char *data;
	long int size;
	GdkPixbufLoader *loader;
	GdkPixbuf *pixbuf;
	GdkPixmap *pixmap;
	GdkBitmap *bitmap;

	GtkamPreview *preview = GTKAM_PREVIEW (user_data);

	gp_file_new (&file);
	result = gp_camera_capture_preview (preview->priv->camera, file);
	if (result != GP_OK) {
		g_warning ("Could not capture: %s",
			   gp_result_as_string (result));
		gp_file_unref (file);
		return (TRUE);
	}

	gp_file_get_data_and_size (file, &data, &size);
	loader = gdk_pixbuf_loader_new ();
	gdk_pixbuf_loader_write (loader, data, size);
	gp_file_unref (file);
	gdk_pixbuf_loader_close (loader);

	pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
	gdk_pixbuf_rotate (pixbuf, preview->priv->rotate);
	gdk_pixbuf_render_pixmap_and_mask (pixbuf, &pixmap, &bitmap, 1);
	gtk_object_unref (GTK_OBJECT (loader));

	if (!GTKAM_IS_PREVIEW (preview))
		return (FALSE);

	/* Show the new preview */
	gtk_pixmap_set (preview->priv->image, pixmap, bitmap);
	if (pixmap)
		gdk_pixmap_unref (pixmap);
	if (bitmap)
		gdk_bitmap_unref (bitmap);

	return (TRUE);
}

static void
on_refresh_toggled (GtkToggleButton *toggle, GtkamPreview *preview)
{
	g_return_if_fail (GTK_IS_TOGGLE_BUTTON (toggle));
	g_return_if_fail (GTKAM_IS_PREVIEW (preview));

	if (!toggle->active && preview->priv->timeout_id) {
		gtk_timeout_remove (preview->priv->timeout_id);
		preview->priv->timeout_id = 0;
		gtk_widget_set_sensitive (preview->priv->spin, FALSE);
	}

	if (toggle->active && !preview->priv->timeout_id) {
		preview->priv->timeout_id = gtk_timeout_add (
				preview->priv->timeout, timeout_func, preview);
		gtk_widget_set_sensitive (preview->priv->spin, TRUE);
	}
}

static void
on_adjustment_value_changed (GtkAdjustment *adj, GtkamPreview *preview)
{
	preview->priv->timeout = adj->value * 1000;
}

static void
on_radio_0_toggled (GtkToggleButton *toggle, GtkamPreview *preview)
{
	g_return_if_fail (GTK_IS_TOGGLE_BUTTON (toggle));
	g_return_if_fail (GTKAM_IS_PREVIEW (preview));

	if (toggle->active)
		preview->priv->rotate = 0;
}

static void
on_radio_90_toggled (GtkToggleButton *toggle, GtkamPreview *preview)
{
	g_return_if_fail (GTK_IS_TOGGLE_BUTTON (toggle));
	g_return_if_fail (GTKAM_IS_PREVIEW (preview));

	if (toggle->active)
		preview->priv->rotate = 90;
}

static void
on_radio_180_toggled (GtkToggleButton *toggle, GtkamPreview *preview)
{
	g_return_if_fail (GTK_IS_TOGGLE_BUTTON (toggle));
	g_return_if_fail (GTKAM_IS_PREVIEW (preview));

	if (toggle->active)
		preview->priv->rotate = 180;
}

static void
on_radio_270_toggled (GtkToggleButton *toggle, GtkamPreview *preview)
{
	g_return_if_fail (GTK_IS_TOGGLE_BUTTON (toggle));
	g_return_if_fail (GTKAM_IS_PREVIEW (preview));

	if (toggle->active)
		preview->priv->rotate = 270;
}

GtkWidget *
gtkam_preview_new (Camera *camera)
{
	GtkamPreview *preview;
	GtkWidget *button, *check, *label, *hbox, *image, *vbox, *radio;
	GtkObject *adj;
	GdkPixbuf *pixbuf;
	GdkPixmap *pixmap;
	GdkBitmap *bitmap;
	GSList *group;

	g_return_val_if_fail (camera != NULL, NULL);

	preview = gtk_type_new (GTKAM_TYPE_PREVIEW);

	preview->priv->camera = camera;
	gp_camera_ref (camera);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (preview)->vbox),
			    hbox, TRUE, TRUE, 0);

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
		gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	}

	/* Empty pixbuf (for preview) */
	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 80, 60);
	gdk_pixbuf_render_pixmap_and_mask (pixbuf, &pixmap, &bitmap, 127);
	gdk_pixbuf_unref (pixbuf); 
	image = gtk_pixmap_new (pixmap, bitmap);
	if (pixmap) 
		gdk_pixmap_unref (pixmap); 
	if (bitmap) 
		gdk_bitmap_unref (bitmap);
	gtk_widget_show (image); 
	gtk_box_pack_start (GTK_BOX (hbox), image, TRUE, TRUE, 0);
	preview->priv->image = GTK_PIXMAP (image);

	/* Rotate */
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox);
	gtk_box_pack_end (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
	radio = gtk_radio_button_new_with_label (NULL, _("0°"));
	gtk_widget_show (radio);
	gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);
	gtk_signal_connect (GTK_OBJECT (radio), "toggled",
			    GTK_SIGNAL_FUNC (on_radio_0_toggled), preview);
	group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio));
	radio = gtk_radio_button_new_with_label (group, _("-90°"));
//	gtk_widget_show (radio);
	gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (radio), "toggled",
			    GTK_SIGNAL_FUNC (on_radio_270_toggled), preview);
	radio = gtk_radio_button_new_with_label (group, _("+90°"));
//	gtk_widget_show (radio);
	gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (radio), "toggled",
			    GTK_SIGNAL_FUNC (on_radio_90_toggled), preview);
	radio = gtk_radio_button_new_with_label (group, _("180°"));
	gtk_widget_show (radio);
	gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (radio), "toggled",
			    GTK_SIGNAL_FUNC (on_radio_180_toggled), preview);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (preview)->vbox), hbox,
			    FALSE, FALSE, 0);
	check = gtk_check_button_new_with_label (_("Refresh every "));
	gtk_widget_show (check);
	gtk_box_pack_start (GTK_BOX (hbox), check, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (check), "toggled",
			    GTK_SIGNAL_FUNC (on_refresh_toggled), preview);

	adj = gtk_adjustment_new (preview->priv->timeout / 1000,
				  1., 10., 1., 0., 0.);
	preview->priv->spin = gtk_spin_button_new (GTK_ADJUSTMENT (adj),
						   1., 0.);
	gtk_widget_show (preview->priv->spin);
	gtk_box_pack_start (GTK_BOX (hbox), preview->priv->spin,
			    FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			    GTK_SIGNAL_FUNC (on_adjustment_value_changed), 
			    preview);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), TRUE);

	label = gtk_label_new (_(" second(s)"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	button = gtk_button_new_with_label (_("Capture"));
	gtk_widget_show (button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_preview_capture_clicked),
			    preview);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (preview)->action_area),
			   button);
	gtk_widget_grab_focus (button);

	button = gtk_button_new_with_label (_("Close"));
	gtk_widget_show (button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
		GTK_SIGNAL_FUNC (on_preview_close_clicked), preview);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (preview)->action_area),
			   button);

	/* Capture preview now */
	timeout_func (preview);

	return (GTK_WIDGET (preview));
}
