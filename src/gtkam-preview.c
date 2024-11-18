/* gtkam-preview.c
 *
 * Copyright 2001,2002 Lutz Mueller <lutz@users.sf.net>
 * Copyright 2002,2003 Patrick Mansfield <patmans@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "gtkam-preview.h"
#include "i18n.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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
#include <gtk/gtkimage.h>
#include <gtk/gtkstock.h>

#include <gdk-pixbuf/gdk-pixbuf-loader.h>

#include <gphoto2/gphoto2-setting.h>

#include "gdk-pixbuf-hacks.h"

#include <gphoto2/gphoto2-setting.h>

#include "gtkam-config.h"
#include "gtkam-error.h"
#include "gtkam-status.h"

struct _GtkamPreviewPrivate
{
	GtkWidget *spin;
	GtkWidget *image;

	GtkamCamera *camera;

	guint rotate;

	GtkToggleButton *angle_0, *angle_90, *angle_180, *angle_270;

	GtkCheckButton *check_download;
	GtkEntry *entry_download;
	GtkFileChooserButton *button_file;

	guint32 timeout_id;

	GtkTooltips *tooltips;
};

#define PARENT_TYPE GTKAM_TYPE_DIALOG
static GtkamDialogClass *parent_class;

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
		g_object_unref (G_OBJECT (preview->priv->tooltips));
		preview->priv->tooltips = NULL;
	}

	if (preview->priv->timeout_id) {
		gtk_timeout_remove (preview->priv->timeout_id);
		preview->priv->timeout_id = 0;
	}

	if (preview->priv->camera) {
		g_object_unref (G_OBJECT (preview->priv->camera));
		preview->priv->camera = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_preview_finalize (GObject *object)
{
	GtkamPreview *preview = GTKAM_PREVIEW (object);

	g_free (preview->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_preview_class_init (gpointer g_class, gpointer class_data)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = GTK_OBJECT_CLASS (g_class);
	object_class->destroy  = gtkam_preview_destroy;

	gobject_class = G_OBJECT_CLASS (g_class);
	gobject_class->finalize = gtkam_preview_finalize;

	signals[CAPTURED] = g_signal_new ("captured",
		G_TYPE_FROM_CLASS (g_class), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkamPreviewClass, captured), NULL, NULL,
		g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, 
		G_TYPE_POINTER);

	parent_class = g_type_class_peek_parent (g_class);
}

static void
gtkam_preview_init (GTypeInstance *instance, gpointer g_class)
{
	GtkamPreview *preview = GTKAM_PREVIEW (instance);

	preview->priv = g_new0 (GtkamPreviewPrivate, 1);
}

GType
gtkam_preview_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo ti;

		memset (&ti, 0, sizeof (GTypeInfo));
		ti.class_size    = sizeof (GtkamPreviewClass);
		ti.class_init    = gtkam_preview_class_init;
		ti.instance_size = sizeof (GtkamPreview);
		ti.instance_init = gtkam_preview_init;

		type = g_type_register_static (PARENT_TYPE, "GtkamPreview",
					       &ti, 0);
	}

	return (type);
}

static gboolean
idle_destroy (gpointer data)
{
	while (gtk_events_pending ())
		gtk_main_iteration ();

	gtk_object_destroy (GTK_OBJECT (data));

	return (FALSE);
}

static void
on_preview_close_clicked (GtkButton *button, GtkamPreview *preview)
{
	gtk_widget_hide (GTK_WIDGET (preview));

	gtk_idle_add (idle_destroy, preview);
}

static void
download_captured_image(GtkWidget *parent, GtkamContext *context,
		Camera *camera, const char *path_folder, const char *path_name,
		const gchar *store_dir)
{
	int result;
	GTimeVal time;
	gchar *store_path, *suffix_pos, *time_s;
	int fd;
	CameraFile *camera_file;
	GtkWidget *dialog;

	g_get_current_time(&time);
	time.tv_usec = 0;
	time_s = g_time_val_to_iso8601(&time);
	store_path = g_build_filename(store_dir, time_s, NULL);
	g_free(time_s);

	suffix_pos = strrchr(path_name, '.');
	if (suffix_pos) {
		gchar *s = store_path;
		store_path = g_strconcat(s, suffix_pos, NULL);
		g_free(s);
	}

	fd = open(store_path, O_CREAT | O_WRONLY, 0644);
	if (fd == -1) {
		dialog = gtkam_error_new (-1, context,
			    parent, _("Failed to open target file '%s'."), store_path);
		gtk_widget_show (dialog);
		g_free(store_path);
		return;
	}
	g_free(store_path);

	result = gp_file_new_from_fd(&camera_file, fd);
	if (result != GP_OK) {
		g_warning("gphoto error, gp_file_new_from_fd");
		close(fd);
		return;
	}

	result = gp_camera_file_get (camera,
		    path_folder, path_name, GP_FILE_TYPE_NORMAL, camera_file,
		    context->context);
	if (result != GP_OK) {
		result = gp_camera_file_delete(camera,
			    path_folder, path_name,
			    context->context);
		if (result != GP_OK) {
			g_warning("Failed to erase file %s/%s", path_folder, path_name);
		}
	}

	gp_file_free(camera_file);
}

static gboolean
timeout_func (gpointer user_data)
{
	int r;
	CameraFile *file;
	const char *data = NULL;
	unsigned long int size = 0;
	GdkPixbufLoader *loader;
	GdkPixbuf *pixbuf, *rotated;
	GtkamPreview *p = GTKAM_PREVIEW (user_data);

	gp_file_new (&file);
	r = gp_camera_capture_preview (p->priv->camera->camera, file, NULL);
	if (p->priv->camera->multi)
		gp_camera_exit (p->priv->camera->camera, NULL);
	if (r < GP_OK) {
		if (r != GP_ERROR_CAMERA_BUSY)
			g_warning ("Could not capture: %s",
				   gp_result_as_string (r));
		gp_file_unref (file);
		return (FALSE);
	}

	loader = gdk_pixbuf_loader_new ();
	gp_file_get_data_and_size (file, &data, &size);
	gdk_pixbuf_loader_write (loader, data, size, NULL);
	gdk_pixbuf_loader_close (loader, NULL);
	gp_file_unref (file);
	pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
	rotated = gdk_pixbuf_rotate (pixbuf, p->priv->rotate);
	g_object_unref (G_OBJECT (loader));
	gtk_image_set_from_pixbuf (GTK_IMAGE (p->priv->image), rotated);
	g_object_unref (G_OBJECT (rotated));

	while (g_main_pending ())
		g_main_iteration (TRUE);

	return (TRUE);
}

static void
preview_enable(GtkamPreview *preview, gboolean enable)
{
	if (enable) {
		if (preview->priv->timeout_id) {
			return;
		}
		preview->priv->timeout_id = gtk_timeout_add (200, timeout_func,
				preview);
	} else {
		gtk_timeout_remove (preview->priv->timeout_id);
		preview->priv->timeout_id = 0;
	}
}

static void
on_preview_capture_clicked (GtkButton *button, GtkamPreview *preview)
{
	int result;
	CameraFilePath path;
	GtkWidget *dialog, *s;
	GtkamPreviewCapturedData data;
	gchar *filename = NULL;

	s = gtkam_status_new (_("Capturing image..."));
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (preview)->vbox), s,
			    FALSE, FALSE, 0);
	gtk_widget_show_now (s);

	if (gtk_toggle_button_get_active(
				GTK_TOGGLE_BUTTON (preview->priv->check_download))) {
		filename = gtk_file_chooser_get_filename (
				GTK_FILE_CHOOSER (preview->priv->button_file));
		if (!filename) {
			dialog = gtkam_error_new (-1, GTKAM_STATUS (s)->context,
				GTK_WIDGET (preview), _("Download directory not selected."));
			gtk_widget_show (dialog);
			gtk_object_destroy (GTK_OBJECT (s));
			return;
		}
	}

	preview_enable(preview, 0);
	result = gp_camera_capture (preview->priv->camera->camera,
		GP_CAPTURE_IMAGE, &path, GTKAM_STATUS (s)->context->context);
	if (preview->priv->camera->multi)
		gp_camera_exit (preview->priv->camera->camera, NULL);
	switch (result) {
	case GP_OK:
		memset (&data, 0, sizeof (GtkamPreviewCapturedData));
		data.camera = preview->priv->camera;
		data.folder = path.folder;
		data.name = path.name;
		g_signal_emit (GTK_OBJECT (preview), signals[CAPTURED], 0,
			       &data);
		if (filename) {
			download_captured_image(GTK_WIDGET(preview), GTKAM_STATUS (s)->context,
				    preview->priv->camera->camera,
				    data.folder, data.name, filename);
		 }
		break;
	case GP_ERROR_CANCEL:
		break;
	default:
		dialog = gtkam_error_new (result, GTKAM_STATUS (s)->context,
			GTK_WIDGET (preview), _("Could not capture image."));
		gtk_widget_show (dialog);
		break;
	}
	g_free(filename);
	gtk_object_destroy (GTK_OBJECT (s));
	preview_enable(preview, 1);
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
on_configure_clicked (GtkButton *button, GtkamPreview *preview)
{
	GtkWidget *d;

	d = gtkam_config_new (preview->priv->camera);
	if (!d)
		return;
	gtk_window_set_transient_for (GTK_WINDOW (d), GTK_WINDOW (preview));
	gtk_widget_show (d);
}

static void
on_size_allocate (GtkWidget *widget, GtkAllocation *allocation,
		  GtkamPreview *preview)
{
	GdkPixbuf *pixbuf, *scaled;
	gint w, h, target_w, target_h;

	g_return_if_fail (GTKAM_IS_PREVIEW (preview));

	pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (preview->priv->image));
	if (!pixbuf)
		return;

	w = gdk_pixbuf_get_width (pixbuf);
	h = gdk_pixbuf_get_height (pixbuf);

	target_w = MIN (allocation->width, allocation->height * w / h);
	target_h = MIN (allocation->height, allocation->width * h / w);

	if ((target_w == w) || (target_h == h))
		return;

	scaled = gdk_pixbuf_scale_simple (pixbuf, target_w, target_h,
					  GDK_INTERP_HYPER);
	gtk_image_set_from_pixbuf (GTK_IMAGE (preview->priv->image), scaled);
	g_object_unref (G_OBJECT (scaled));
}

static void
on_direct_download_toggled (GtkToggleButton *togglebutton, GtkamPreview *preview)
{
	GtkFileChooser *dialog;

	if (!gtk_toggle_button_get_active(
				GTK_TOGGLE_BUTTON (preview->priv->check_download))) {
		gp_setting_set ("gtkam-preview", "direct_download", "0");
		return;
	}

	gp_setting_set ("gtkam-preview", "direct_download", "1");
}

static void
on_button_file_selection_changed (GtkFileChooserButton *widget, GtkamPreview *preview)
{
    gchar *store_filename;

    store_filename = gtk_file_chooser_get_filename (
            GTK_FILE_CHOOSER (preview->priv->button_file));
    if (store_filename) {
        gp_setting_set ("gtkam-preview", "download_folder", store_filename);
        g_free(store_filename);
    }
}

GtkWidget *
gtkam_preview_new (GtkamCamera *camera)
{
	CameraAbilities abilities;
	GtkamPreview *preview;
	GtkWidget *button, *check, *hbox, *vbox, *radio;
	GSList *group;
	gchar buf[1024];

	g_return_val_if_fail (GTKAM_IS_CAMERA (camera), NULL);

	preview = g_object_new (GTKAM_TYPE_PREVIEW, NULL);
	gtk_window_set_title (GTK_WINDOW (preview), _("Capture"));
	gtk_container_set_border_width (GTK_CONTAINER (preview), 5);

	preview->priv->camera = camera;
	g_object_ref (G_OBJECT (camera));
	preview->priv->tooltips = gtk_tooltips_new ();
	g_object_ref (G_OBJECT (preview->priv->tooltips));
	gtk_object_sink (GTK_OBJECT (preview->priv->tooltips));

	hbox = gtk_hbox_new (FALSE, 10);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTKAM_DIALOG (preview)->vbox), hbox,
			    TRUE, TRUE, 0);

	/* Empty image (for preview) */
	preview->priv->image = gtk_image_new ();
	gtk_widget_show (preview->priv->image); 
	gtk_box_pack_start (GTK_BOX (hbox), preview->priv->image,
			    TRUE, TRUE, 0);
	g_signal_connect (G_OBJECT (preview->priv->image), "size_allocate",
			  G_CALLBACK (on_size_allocate), preview);

	/* Rotate */
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox);
	gtk_box_pack_end (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
	radio = gtk_radio_button_new_with_label (NULL, _("0 degrees"));
	gtk_widget_show (radio);
	gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);
	g_signal_connect (GTK_OBJECT (radio), "toggled",
			    GTK_SIGNAL_FUNC (on_radio_0_toggled), preview);
	preview->priv->angle_0 = GTK_TOGGLE_BUTTON (radio);
	gtk_tooltips_set_tip (preview->priv->tooltips, radio,
			      _("Don't rotate thumbnail"), NULL);
	group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));
	radio = gtk_radio_button_new_with_label (group, _("-90 degrees"));
	gtk_widget_show (radio);
	gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);
	g_signal_connect (GTK_OBJECT (radio), "toggled",
			    GTK_SIGNAL_FUNC (on_radio_270_toggled), preview);
	preview->priv->angle_90 = GTK_TOGGLE_BUTTON (radio);
	gtk_tooltips_set_tip (preview->priv->tooltips, radio,
			      _("Rotate thumbnail by -90 degrees"), NULL);
	group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));
	radio = gtk_radio_button_new_with_label (group, _("+90 degrees"));
	gtk_widget_show (radio);
	gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);
	g_signal_connect (GTK_OBJECT (radio), "toggled",
			    GTK_SIGNAL_FUNC (on_radio_90_toggled), preview);
	preview->priv->angle_180 = GTK_TOGGLE_BUTTON (radio);
	gtk_tooltips_set_tip (preview->priv->tooltips, radio, 
			      _("Rotate thumbnail by 90 degrees"), NULL);
	group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));
	radio = gtk_radio_button_new_with_label (group, _("180 degrees"));
	gtk_widget_show (radio);
	gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);
	g_signal_connect (GTK_OBJECT (radio), "toggled",
			    GTK_SIGNAL_FUNC (on_radio_180_toggled), preview);
	preview->priv->angle_270 = GTK_TOGGLE_BUTTON (radio);
	gtk_tooltips_set_tip (preview->priv->tooltips, radio, 
			      _("Rotate thumbnail by 180 degrees"), NULL);

	/* Immediate image download */
	hbox = gtk_hbox_new (FALSE, 10);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTKAM_DIALOG (preview)->vbox), hbox,
			    TRUE, TRUE, 0);

	check = gtk_check_button_new_with_label (_("Download"));
	gtk_widget_show (check);
	gtk_box_pack_start (GTK_BOX (hbox), check, FALSE, FALSE, 0);
	g_signal_connect (GTK_OBJECT (check), "toggled",
			    GTK_SIGNAL_FUNC (on_direct_download_toggled), preview);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), FALSE);
	gtk_tooltips_set_tip (preview->priv->tooltips, check, _(
			    _("Download captured images into specified directory.")), NULL);
	preview->priv->check_download = GTK_CHECK_BUTTON (check);

	button = gtk_file_chooser_button_new(_("Download target directory"),
			    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
	gtk_file_chooser_set_local_only( GTK_FILE_CHOOSER (button), TRUE);
	preview->priv->button_file = GTK_FILE_CHOOSER_BUTTON (button);
	g_signal_connect (G_OBJECT (preview->priv->button_file), "selection-changed",
            G_CALLBACK (on_button_file_selection_changed), preview);

	if (gp_setting_get ("gtkam-preview", "download_folder", buf) == GP_OK) {
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (preview->priv->button_file), buf);
	}
	if (gp_setting_get ("gtkam-preview", "direct_download", buf) == GP_OK) {
		gtk_toggle_button_set_active (
		    GTK_TOGGLE_BUTTON (preview->priv->check_download),
		    atoi(buf));
	}

	/* Buttons in action area */
	button = gtk_button_new_with_label (_("Capture"));
	gtk_widget_show (button);
	g_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_preview_capture_clicked),
			    preview);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (preview)->action_area),
			   button);
	gtk_widget_grab_focus (button);

	gp_camera_get_abilities (camera->camera, &abilities);
	if (abilities.operations & GP_OPERATION_CONFIG) {
		button = gtk_button_new_from_stock (GTK_STOCK_PREFERENCES);
		gtk_widget_show (button);
		g_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (on_configure_clicked), preview);
		gtk_container_add (
			GTK_CONTAINER (GTK_DIALOG (preview)->action_area),
			button);
	}

	button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	gtk_widget_show (button);
	g_signal_connect (GTK_OBJECT (button), "clicked",
		GTK_SIGNAL_FUNC (on_preview_close_clicked), preview);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (preview)->action_area),
			   button);

	preview_enable(preview, 1);

	return (GTK_WIDGET (preview));
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
