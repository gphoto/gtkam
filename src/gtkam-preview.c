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

#include <gtk/gtktooltips.h>
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
#include <gtk/gtkhscale.h>
#include <gdk-pixbuf/gdk-pixbuf-loader.h>

#include "gtkam-error.h"
#include "gtkam-config.h"
#include "gdk-pixbuf-hacks.h"

struct _GtkamPreviewPrivate
{
	GtkWidget *spin;
	GtkPixmap *image;

	Camera *camera;

	guint rotate;
	gfloat zoom;

	GtkToggleButton *zoom_100, *zoom_150, *zoom_200, *zoom_250;
	GtkToggleButton *angle_0, *angle_90, *angle_180, *angle_270;

	guint32 idle_id;

	GtkTooltips *tooltips;

	gboolean multi;
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

	if (preview->priv->tooltips) {
		gtk_object_unref (GTK_OBJECT (preview->priv->tooltips));
		preview->priv->tooltips = NULL;
	}

	if (preview->priv->idle_id) {
		gtk_idle_remove (preview->priv->idle_id);
		preview->priv->idle_id = 0;
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

	signals[CAPTURED] = gtk_signal_new ("captured", GTK_RUN_FIRST,
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
	preview->priv->zoom = 1.;
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
				    GP_CAPTURE_IMAGE, &path);
	if (preview->priv->multi)
		gp_camera_exit (preview->priv->camera);
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
	}
}

static gboolean
idle_func (gpointer user_data)
{
	int result;
	CameraFile *file;
	const char *data;
	long int size;
	GdkPixbufLoader *loader;
	GdkPixbuf *pixbuf, *rotated, *scaled;
	GdkPixmap *pixmap;
	GdkBitmap *bitmap;
	guint w, h;

	GtkamPreview *preview = GTKAM_PREVIEW (user_data);

	while (gtk_events_pending ())
		gtk_main_iteration ();
	if (!GTKAM_IS_PREVIEW (preview))
		return (FALSE);

	gp_file_new (&file);
	result = gp_camera_capture_preview (preview->priv->camera, file);
	if (!GTKAM_IS_PREVIEW (preview))
		return (FALSE);
	if (preview->priv->multi)
		gp_camera_exit (preview->priv->camera);
	if (result != GP_OK) {
		g_warning ("Could not capture: %s",
			   gp_result_as_string (result));
		gp_file_unref (file);
		return (TRUE);
	}

	while (gtk_events_pending ())
		gtk_main_iteration ();
	if (!GTKAM_IS_PREVIEW (preview))
		return (FALSE);

	gp_file_get_data_and_size (file, &data, &size);
	loader = gdk_pixbuf_loader_new ();
	gdk_pixbuf_loader_write (loader, data, size);
	gp_file_unref (file);
	gdk_pixbuf_loader_close (loader);

	while (gtk_events_pending ())
		gtk_main_iteration ();
	if (!GTKAM_IS_PREVIEW (preview))
		return (FALSE);

	pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
	rotated = gdk_pixbuf_rotate (pixbuf, preview->priv->rotate);
	gtk_object_unref (GTK_OBJECT (loader));
	w = gdk_pixbuf_get_width (rotated);
	h = gdk_pixbuf_get_height (rotated);
	scaled = gdk_pixbuf_scale_simple (rotated, w * preview->priv->zoom,
					  h * preview->priv->zoom,
					  GDK_INTERP_HYPER);
	gdk_pixbuf_unref (rotated);
	gdk_pixbuf_render_pixmap_and_mask (scaled, &pixmap, &bitmap, 127);
	gdk_pixbuf_unref (scaled);

	while (gtk_events_pending ()) 
		gtk_main_iteration ();
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

static void
on_radio_100_toggled (GtkToggleButton *toggle, GtkamPreview *preview)
{
	if (toggle->active)
		preview->priv->zoom = 1;
}

static void
on_radio_150_toggled (GtkToggleButton *toggle, GtkamPreview *preview)
{
	if (toggle->active)
		preview->priv->zoom = 1.5;
}

static void
on_radio_200_toggled (GtkToggleButton *toggle, GtkamPreview *preview)
{
	if (toggle->active)
		preview->priv->zoom = 2.;
}

static void
on_radio_250_toggled (GtkToggleButton *toggle, GtkamPreview *preview)
{
	if (toggle->active)
		preview->priv->zoom = 2.5;
}

static void
on_configure_clicked (GtkButton *button, GtkamPreview *preview)
{
	GtkWidget *dialog;

	dialog = gtkam_config_new (preview->priv->camera);
	gtk_widget_show (dialog);
}

GtkWidget *
gtkam_preview_new (Camera *camera, gboolean multi)
{
	CameraAbilities abilities;
	GtkamPreview *preview;
	GtkWidget *button, *hbox, *image, *vbox, *radio;
	GdkPixbuf *pixbuf;
	GdkPixmap *pixmap;
	GdkBitmap *bitmap;
	GSList *group;

	g_return_val_if_fail (camera != NULL, NULL);

	preview = gtk_type_new (GTKAM_TYPE_PREVIEW);
	gtk_window_set_title (GTK_WINDOW (preview), _("Capture"));
	gtk_container_set_border_width (GTK_CONTAINER (preview), 5);
	gtk_window_set_policy (GTK_WINDOW (preview), TRUE, TRUE, TRUE);
	gtk_signal_connect (GTK_OBJECT (preview), "delete_event",
			    GTK_SIGNAL_FUNC (gtk_object_destroy), NULL);

	preview->priv->camera = camera;
	gp_camera_ref (camera);
	preview->priv->multi = multi;
	preview->priv->tooltips = gtk_tooltips_new ();

	hbox = gtk_hbox_new (FALSE, 5);
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
	preview->priv->angle_0 = GTK_TOGGLE_BUTTON (radio);
	gtk_tooltips_set_tip (preview->priv->tooltips, radio,
			      _("Don't rotate thumbnail"), NULL);
	group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio));
	radio = gtk_radio_button_new_with_label (group, _("-90°"));
	gtk_widget_show (radio);
	gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (radio), "toggled",
			    GTK_SIGNAL_FUNC (on_radio_270_toggled), preview);
	preview->priv->angle_90 = GTK_TOGGLE_BUTTON (radio);
	gtk_tooltips_set_tip (preview->priv->tooltips, radio,
			      _("Rotate thumbnail by -90°"), NULL);
	group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio));
	radio = gtk_radio_button_new_with_label (group, _("+90°"));
	gtk_widget_show (radio);
	gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (radio), "toggled",
			    GTK_SIGNAL_FUNC (on_radio_90_toggled), preview);
	preview->priv->angle_180 = GTK_TOGGLE_BUTTON (radio);
	gtk_tooltips_set_tip (preview->priv->tooltips, radio, 
			      _("Rotate thumbnail by 90°"), NULL);
	group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio));
	radio = gtk_radio_button_new_with_label (group, _("180°"));
	gtk_widget_show (radio);
	gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (radio), "toggled",
			    GTK_SIGNAL_FUNC (on_radio_180_toggled), preview);
	preview->priv->angle_270 = GTK_TOGGLE_BUTTON (radio);
	gtk_tooltips_set_tip (preview->priv->tooltips, radio, 
			      _("Rotate thumbnail by 180°"), NULL);

	/* Zoom */
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (preview)->vbox), hbox,
			    FALSE, FALSE, 0);
	radio = gtk_radio_button_new_with_label (NULL, _("100%"));
	gtk_widget_show (radio);
	gtk_box_pack_start (GTK_BOX (hbox), radio, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (radio), "toggled",
			    GTK_SIGNAL_FUNC (on_radio_100_toggled), preview);
	preview->priv->zoom_100 = GTK_TOGGLE_BUTTON (radio);
	gtk_tooltips_set_tip (preview->priv->tooltips, radio,
			      _("Don't enlarge thumbnail"), NULL);
	group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio));
	radio = gtk_radio_button_new_with_label (group, _("150%"));
	gtk_widget_show (radio);
	gtk_box_pack_start (GTK_BOX (hbox), radio, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (radio), "toggled",
			    GTK_SIGNAL_FUNC (on_radio_150_toggled), preview);
	preview->priv->zoom_150 = GTK_TOGGLE_BUTTON (radio);
	gtk_tooltips_set_tip (preview->priv->tooltips, radio,
			      _("Enlarge thumbnail by 50%"), NULL);
	group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio));
	radio = gtk_radio_button_new_with_label (group, _("200%"));
	gtk_widget_show (radio);
	gtk_box_pack_start (GTK_BOX (hbox), radio, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (radio), "toggled",
			    GTK_SIGNAL_FUNC (on_radio_200_toggled), preview);
	preview->priv->zoom_200 = GTK_TOGGLE_BUTTON (radio);
	gtk_tooltips_set_tip (preview->priv->tooltips, radio,
			      _("Enlarge thumbnail by 100%"), NULL);
	group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio));
	radio = gtk_radio_button_new_with_label (group, _("250%"));
	gtk_widget_show (radio);
	gtk_box_pack_start (GTK_BOX (hbox), radio, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (radio), "toggled",
			    GTK_SIGNAL_FUNC (on_radio_250_toggled), preview);
	preview->priv->zoom_250 = GTK_TOGGLE_BUTTON (radio);
	gtk_tooltips_set_tip (preview->priv->tooltips, radio,
			      _("Enlarge thumbnail by 150%"), NULL);

	button = gtk_button_new_with_label (_("Capture"));
	gtk_widget_show (button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_preview_capture_clicked),
			    preview);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (preview)->action_area),
			   button);
	gtk_widget_grab_focus (button);

	gp_camera_get_abilities (camera, &abilities);
	if (abilities.operations & GP_OPERATION_CONFIG) {
		button = gtk_button_new_with_label (_("Configure"));
		gtk_widget_show (button);
		gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (on_configure_clicked), preview);
		gtk_container_add (
			GTK_CONTAINER (GTK_DIALOG (preview)->action_area),
			button);
	}

	button = gtk_button_new_with_label (_("Close"));
	gtk_widget_show (button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
		GTK_SIGNAL_FUNC (on_preview_close_clicked), preview);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (preview)->action_area),
			   button);

	/* Start capturing previews */
	preview->priv->idle_id = gtk_idle_add (idle_func, preview);

	return (GTK_WIDGET (preview));
}

void
gtkam_preview_set_zoom (GtkamPreview *preview, gfloat zoom)
{
	g_return_if_fail (GTKAM_IS_PREVIEW (preview));

	switch ((guint) (zoom * 100.)) {
	case 100:
		gtk_toggle_button_set_active (preview->priv->zoom_100, TRUE);
		break;
	case 150:
		gtk_toggle_button_set_active (preview->priv->zoom_150, TRUE);
		break;
	case 200:
		gtk_toggle_button_set_active (preview->priv->zoom_200, TRUE);
		break;
	case 250:
		gtk_toggle_button_set_active (preview->priv->zoom_250, TRUE);
		break;
	default:
		g_warning ("Zoom of %f not implemented!", zoom);
		break;
	}
}

gfloat
gtkam_preview_get_zoom (GtkamPreview *preview)
{
	g_return_val_if_fail (GTKAM_IS_PREVIEW (preview), 0.);

	return (preview->priv->zoom);
}

void
gtkam_preview_set_angle (GtkamPreview *preview, guint angle)
{
	g_return_if_fail (GTKAM_IS_PREVIEW (preview));

	switch (angle) {
	case 0:
		gtk_toggle_button_set_active (preview->priv->angle_0, TRUE);
		break;
	case 90:
		gtk_toggle_button_set_active (preview->priv->angle_90, TRUE);
		break;
	case 180:
		gtk_toggle_button_set_active (preview->priv->angle_180, TRUE);
		break;
	case 270:
		gtk_toggle_button_set_active (preview->priv->angle_270, TRUE);
		break;
	default:
		g_warning ("Rotation by %i not implemented!", angle);
	}
}

guint
gtkam_preview_get_angle (GtkamPreview *preview)
{
	g_return_val_if_fail (GTKAM_IS_PREVIEW (preview), 0);

	return (preview->priv->rotate);
}
