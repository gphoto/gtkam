/* gtkam-gimp.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gphoto2/gphoto2-abilities-list.h>
#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-setting.h>
#include <gdk-pixbuf/gdk-pixbuf-loader.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gtkam-preview.h"
#include "gtkam-chooser.h"
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

#define PLUG_IN_NAME "gtkam-gimp"
#define PLUG_IN_PRINT_NAME "GtkamGimp"
#define PLUG_IN_DESCRIPTION "Capture images using your digital camera"
#define PLUG_IN_HELP "This plug-in will let you capture images using your digital camera"
#define PLUG_IN_AUTHOR "Lutz Müller <urc8@rz.uni-karlsruhe.de>"
#define PLUG_IN_COPYRIGHT "GPL"
#define PLUG_IN_VERSION VERSION

#define NUMBER_IN_ARGS 1
#define IN_ARGS {GIMP_PDB_INT32, "CaptureType", "GP_CAPTURE_IMAGE, GP_CAPTURE_VIDEO, GP_CAPTURE_AUDIO"}

#define NUMBER_OUT_ARGS 1
#define OUT_ARGS {GIMP_PDB_IMAGE, "image", "Output image"}

static void
query (void)
{
	static GimpParamDef in_args[] = {IN_ARGS};
	static GimpParamDef out_args[] = {OUT_ARGS};

	gimp_install_procedure (PLUG_IN_NAME, PLUG_IN_DESCRIPTION,
		PLUG_IN_HELP, PLUG_IN_AUTHOR, PLUG_IN_COPYRIGHT,
		PLUG_IN_VERSION, N_("<Toolbox>/File/Acquire/From Camera..."),
		NULL, GIMP_EXTENSION, NUMBER_IN_ARGS, NUMBER_OUT_ARGS,
		in_args, out_args);
}

static void
on_captured (GtkamPreview *preview, const gchar *path, CameraFilePath *fpath)
{
	gchar *dir;

	dir = g_dirname (path);
	strncpy (fpath->folder, dir, sizeof (fpath->folder));
	g_free (dir);
	strncpy (fpath->name, g_basename (path), sizeof (fpath->name));
	gtk_main_quit ();
}

static void
on_camera_selected (GtkamChooser *chooser, Camera *camera, Camera **gimp_camera)
{
	*gimp_camera = camera;
	gp_camera_ref (camera);
	gtk_main_quit ();
}

typedef struct {
	guint angle;
	gfloat zoom;
} PreviewParams;

static void
progress_func (Camera *camera, float progress, void *data)
{
	gimp_progress_update (progress);
}

static void
run (gchar *name, gint nparams, GimpParam *param, gint *nreturn_vals,
     GimpParam **return_vals)
{
	Camera *camera = NULL;
	CameraFilePath path;
	CameraAbilities a;
	CameraFile *file;
	int result;
	GtkWidget *preview, *chooser, *dialog;
	static GimpParam values[2];
	static PreviewParams preview_params = {0, 1.};
	GimpDrawable *drawable;
	GimpPixelRgn pixel_rgn;
	gint32 image_id, layer_id;
	GdkPixbufLoader *loader;
	GdkPixbuf *pixbuf;
	long int size;
	const char *data;
	guchar *pixels;
	int r;
	guint w, h;
	GimpRunModeType run_mode = param[0].data.d_int32;

	values[0].type = GIMP_PDB_STATUS;
	values[0].data.d_status = GIMP_PDB_SUCCESS;
	*nreturn_vals = 1;
	*return_vals = values;

	values[1].type = GIMP_PDB_INT32;
	values[1].data.d_int32 = 0;

	/* Create the camera */
	switch (run_mode) {
	case GIMP_RUN_INTERACTIVE:

		/* Initialize gtk through gimp */
		gimp_ui_init ("gtkam-gimp", TRUE);

		/* Let the use choose a camera */
		chooser = gtkam_chooser_new ();
		gtk_widget_show (chooser);
		gtk_widget_hide (GTKAM_CHOOSER (chooser)->apply_button);
		gtkam_chooser_set_camera_mask (GTKAM_CHOOSER (chooser),
					       GP_OPERATION_CAPTURE_IMAGE |
					       GP_OPERATION_CAPTURE_PREVIEW);
		gtk_signal_connect (GTK_OBJECT (chooser), "camera_selected",
				    GTK_SIGNAL_FUNC (on_camera_selected),
				    &camera);
		gtk_signal_connect (GTK_OBJECT (chooser), "delete_event",
				    GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
		gtk_main ();

		/* Check if the user cancelled */
		if (!camera) {
			values[0].data.d_status = GIMP_PDB_CANCEL;
			return;
		}

		break;
	case GIMP_RUN_NONINTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:

		g_warning ("Implement!");
		values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
		return;

		break;
	}

	g_assert (camera);

	switch (run_mode) {
	case GIMP_RUN_INTERACTIVE:

		/*
		 * Only pop up a preview window in case the camera supports
		 * capturing previews
		 */
		gp_camera_get_abilities (camera, &a);
		if (a.operations & GP_OPERATION_CAPTURE_PREVIEW) {

			/* Get settings from a previous run (if any) */
			gimp_get_data (PLUG_IN_NAME, &preview_params);

			/* Create the preview window */
			preview = gtkam_preview_new (camera);
			gtkam_preview_set_angle (GTKAM_PREVIEW (preview),
						 preview_params.angle);
			gtkam_preview_set_zoom (GTKAM_PREVIEW (preview),
						preview_params.zoom);
			gtk_widget_show (preview);
			memset (&path, 0, sizeof (CameraFilePath));
			gtk_signal_connect (GTK_OBJECT (preview), "captured",
				GTK_SIGNAL_FUNC (on_captured), &path);
			gtk_signal_connect (GTK_OBJECT (preview),
				"delete_event",
				GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
			gtk_main ();

			/* Check if the user cancelled */
			if (!strlen (path.folder)) {
				gp_camera_unref (camera);
				values[0].data.d_status = GIMP_PDB_CANCEL;
				return;
			}

			gtk_widget_hide (preview);

			/* Store the settings for later runs */
			preview_params.zoom =
				gtkam_preview_get_zoom (
						GTKAM_PREVIEW (preview));
			preview_params.angle = 
				gtkam_preview_get_angle (
						GTKAM_PREVIEW (preview));
			gtk_object_destroy (GTK_OBJECT (preview));
			gimp_set_data (PLUG_IN_NAME, &preview_params,
				       sizeof (PreviewParams));

			break;
		}

	case GIMP_RUN_NONINTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:
		result = gp_camera_capture (camera, GP_CAPTURE_IMAGE, &path);
		if (result < 0) {
			dialog = gtkam_error_new (_("Could not capture"),
				result, camera, NULL);
			gtk_widget_show (dialog);
			gp_camera_unref (camera);

			/* Wait until the user closes the dialog */
			gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
				GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
			gtk_main ();

			values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
			return;
		}

		break;
	default:
		gp_camera_unref (camera);
		values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
		return;
	}

	/*
	 * At this point, the camera has captured an image. Download the 
	 * file
	 */
	gp_file_new (&file);
	gimp_progress_init (_("Downloading file"));
	gp_camera_set_progress_func (camera, progress_func, NULL);
	result = gp_camera_file_get (camera, path.folder, path.name,
				     GP_FILE_TYPE_NORMAL, file);
	gp_camera_set_progress_func (camera, NULL, NULL);
	if (result < 0) {
		gp_file_unref (file);
		dialog = gtkam_error_new (_("Could not "
			"download file"), result, camera, NULL);
		gtk_widget_show (dialog);
		gp_camera_unref (camera);

		/* Wait until the user closes the dialog */
		gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
				    GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
		gtk_main ();

		values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
		return;
	}
	gp_camera_unref (camera);

	/* At this point, we've got a file. Make a pixbuf out of it. */
	gp_file_get_data_and_size (file, &data, &size);
	loader = gdk_pixbuf_loader_new ();
	gdk_pixbuf_loader_write (loader, data, size);
	gp_file_unref (file);
	gdk_pixbuf_loader_close (loader);
	pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
	w = gdk_pixbuf_get_width (pixbuf);
	h = gdk_pixbuf_get_height (pixbuf);
	r = gdk_pixbuf_get_rowstride (pixbuf);
	pixels = gdk_pixbuf_get_pixels (pixbuf);

	/* Take the pixbuf and make a gimp window out of it. */
	image_id = gimp_image_new (w, h, GIMP_RGB);
	layer_id = gimp_layer_new (image_id, _("Background"), w, h,
				   GIMP_RGB_IMAGE, 100, GIMP_NORMAL_MODE);
	gimp_image_add_layer (image_id, layer_id, 0);
	drawable = gimp_drawable_get (layer_id);
	gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, r, h, TRUE, FALSE);
	gimp_pixel_rgn_set_rect (&pixel_rgn, pixels, 0, 0, w, h);
	gtk_object_unref (GTK_OBJECT (loader));
	gimp_drawable_flush (drawable);
	gimp_drawable_detach (drawable);
	gimp_display_new (image_id);

	/* Tell gimp about the new image */
	*nreturn_vals = 2;
	values[1].type = GIMP_PDB_IMAGE;
	values[1].data.d_image = image_id;
}

GimpPlugInInfo PLUG_IN_INFO = {
	NULL,
	NULL,
	query,
	run
};

MAIN ()
