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
#define PLUG_IN_HELP "This plug-in will capture images using your digital camera"
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
on_captured (GtkamPreview *preview, const gchar *path, gchar **rpath)
{
	*rpath = g_strdup (path);
	gtk_main_quit ();
}

static gboolean
send_file_to_gimp (CameraFile *file)
{
	GimpDrawable *drawable;
	GimpPixelRgn pixel_rgn;
	gint32 image_id = 0;
	gint32 layer_id = 0;
	GdkPixbufLoader *loader;
	GdkPixbuf *pixbuf;
	long int size;
	const char *data;
	guchar *pixels;
	int r;
	guint w, h;

	gp_file_get_data_and_size (file, &data, &size);

	loader = gdk_pixbuf_loader_new ();
	gdk_pixbuf_loader_write (loader, data, size);
	gdk_pixbuf_loader_close (loader);

	pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
	w = gdk_pixbuf_get_width (pixbuf);
	h = gdk_pixbuf_get_height (pixbuf);
	r = gdk_pixbuf_get_rowstride (pixbuf);
	pixels = gdk_pixbuf_get_pixels (pixbuf);

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

	return image_id;
}

typedef struct {
	guint angle;
	gfloat zoom;
} PreviewParams;

static void
run (gchar *name, gint nparams, GimpParam *param, gint *nreturn_vals,
     GimpParam **return_vals)
{
	Camera *camera = NULL;
	CameraFilePath path;
	CameraAbilitiesList *al;
	CameraAbilities a;
	CameraFile *file;
	GPPortInfo info;
	GPPortInfoList *il;
	int n, p, result;
	GtkWidget *preview;
	gchar *rpath = NULL, *dir;
	char port[1024], speed[1024], model[1024];
	guint image_id;
	static GimpParam values[2];
	static PreviewParams preview_params = {0, 1.};

	values[0].type = GIMP_PDB_STATUS;
	values[0].data.d_status = GIMP_PDB_SUCCESS;
	*nreturn_vals = 1;
	*return_vals = values;

	values[1].type = GIMP_PDB_INT32;
	values[1].data.d_int32 = 0;

	/* Create the camera */
	if (((gp_setting_get ("gtkam", "camera", model) == GP_OK) ||
            (gp_setting_get ("gtkam", "model", model) == GP_OK) ||
            (gp_setting_get ("gphoto2", "model", model) == GP_OK)) &&
            ((gp_setting_get ("gtkam", "port", port) == GP_OK) ||
             (gp_setting_get ("gtkam", "port name", port) == GP_OK)) &&
            (gp_setting_get ("gtkam", "speed", speed) == GP_OK)) {
		gp_camera_new (&camera);

		gp_abilities_list_new (&al);
		gp_abilities_list_load (al);
		gp_port_info_list_new (&il);
		gp_port_info_list_load (il);

		n = gp_abilities_list_lookup_model (al, model);
		gp_abilities_list_get_abilities (al, n, &a);
		gp_abilities_list_free (al);

		p = gp_port_info_list_lookup_name (il, port);
		gp_port_info_list_get_info (il, p, &info);
		gp_port_info_list_free (il);

		gp_camera_set_abilities (camera, a);
		if (strcmp (port, "None") && strcmp (model, "Directory Browse"))
			gp_camera_set_port_info (camera, info);
		if (atoi (speed))
			gp_camera_set_port_speed (camera, atoi (speed));
	} else {
		g_warning ("Implement!");
		values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
		return;
	}

	g_assert (camera);

	switch (param[0].data.d_int32) {
	case GIMP_RUN_INTERACTIVE:

		/*
		 * Only pop up a preview window in case the camera supports
		 * capturing previews
		 */
		gp_camera_get_abilities (camera, &a);
		if (a.operations & GP_OPERATION_CAPTURE_PREVIEW) {

			/* Initialize gtk through gimp */
			gimp_ui_init ("gtkam-gimp", TRUE);

			/* Get settings from a previous run (if any) */
			gimp_get_data (PLUG_IN_NAME, &preview_params);

			/* Create the preview window */
			preview = gtkam_preview_new (camera);
			gtkam_preview_set_angle (GTKAM_PREVIEW (preview),
						 preview_params.angle);
			gtkam_preview_set_zoom (GTKAM_PREVIEW (preview),
						preview_params.zoom);
			gtk_widget_show (preview);
			gtk_signal_connect (GTK_OBJECT (preview), "captured",
				GTK_SIGNAL_FUNC (on_captured), &rpath);
			gtk_signal_connect (GTK_OBJECT (preview),
				"delete_event",
				GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
			gtk_main ();

			/* Check if the user cancelled */
			if (!rpath) {
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

			/* Get the file */
			dir = g_dirname (rpath);
			gp_file_new (&file);
			result = gp_camera_file_get (camera, dir,
				g_basename (rpath), GP_FILE_TYPE_NORMAL, file);
			gp_camera_unref (camera);
			g_free (dir);
			g_free (rpath);
			if (result < 0) {
				gp_file_unref (file);
				g_warning ("Some more error messages would "
					   "be fine...");
				values[0].data.d_status =
						GIMP_PDB_EXECUTION_ERROR;
				return;
			}

			break;
		}

	case GIMP_RUN_NONINTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:
		result = gp_camera_capture (camera, GP_CAPTURE_IMAGE, &path);
		if (result < 0) {
			g_warning ("Could not capture: %s",
				   gp_result_as_string (result));
			values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
			return;
		}
		gp_file_new (&file);
		result = gp_camera_file_get (camera, path.folder, path.name,
					     GP_FILE_TYPE_NORMAL, file);
		gp_camera_unref (camera);
		if (result < 0) {
			gp_file_unref (file);
			g_warning ("Some more error messages would be fine...");
			values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
			return;
		}

		break;
	default:
		gp_camera_unref (camera);
		values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
		return;
	}

	/* At this point, we've got a file. Display it */
	image_id = send_file_to_gimp (file);
	gp_file_unref (file);
	if (!image_id) {
		values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
		return;
	}

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
