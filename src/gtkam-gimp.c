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
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#if HAVE_JPEG
#include <jpeglib.h>
#endif

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
		PLUG_IN_VERSION, N_("<Toolbox>/File/Acquire/From camera..."),
		NULL, GIMP_EXTENSION, NUMBER_IN_ARGS, NUMBER_OUT_ARGS,
		in_args, out_args);
}

static GimpParam values[2];

static void
on_captured (GtkamPreview *preview, const gchar *path, gchar **rpath)
{
	*rpath = g_strdup (path);
}

static void
send_file_to_gimp (CameraFile *file)
{
	GimpDrawable *drawable;
	GimpPixelRgn pixel_rgn;
	gint32 image_id = 0;
	gint32 layer_id = 0;
	const char *type;

	gp_file_get_mime_type (file, &type);

#if HAVE_JPEG
	if (!strcmp (type, GP_MIME_JPEG)) {
		struct jpeg_decompress_struct cinfo;
		struct jpeg_error_mgr pub;
		int i;
		unsigned char *dptr, **lptr, *lines[4];
		FILE *f;
		guint w, h;
		guchar *bitmap;

		gp_file_save (file, "/tmp/gphoto.jpg");
		f = fopen ("/tmp/gphoto.jpg", "r");
		if (!f) {
			g_warning ("Error!");
			return;
		}

		cinfo.err = jpeg_std_error (&pub);
		jpeg_create_decompress (&cinfo);
		jpeg_stdio_src (&cinfo, f);
		jpeg_read_header (&cinfo, TRUE);
		jpeg_start_decompress (&cinfo);
		cinfo.do_fancy_upsampling = FALSE;
		cinfo.do_block_smoothing = FALSE;
		cinfo.out_color_space = JCS_RGB;
		w = cinfo.output_width;
		h = cinfo.output_height;

		bitmap = g_new0 (guchar, w * h);
		dptr = bitmap;
		while (cinfo.output_scanline < cinfo.output_height) {
			lptr = lines;
			for (i = 0; i < cinfo.rec_outbuf_height; i++) {
				*lptr++ = dptr;
				dptr += w;
			}
			jpeg_read_scanlines (&cinfo, lines,
					     cinfo.rec_outbuf_height);
			if (cinfo.output_components == 1)
				printf ("HUH?!?\n");
		}

		jpeg_finish_decompress (&cinfo);
		jpeg_destroy_decompress (&cinfo);

		fclose (f);

		image_id = gimp_image_new (w / 3, h, GIMP_RGB);
		layer_id = gimp_layer_new (image_id, _("Background"),
					   w / 3, h,
					   GIMP_RGB_IMAGE, 100,
					   GIMP_NORMAL_MODE);
		gimp_image_add_layer (image_id, layer_id, 0);
		drawable = gimp_drawable_get (layer_id);

		gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, w / 3,
				     h, TRUE, FALSE);
		gimp_pixel_rgn_set_rect (&pixel_rgn, bitmap,
					 0, 0, w / 3, h);

		gimp_drawable_flush (drawable);

		gimp_drawable_detach (drawable);
		gimp_display_new (image_id);

		return;
	} else
#endif
	{
		/* Mime type not supported. */
		g_warning ("Mime type '%s' not supported.", type);
	}
}

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
	GimpRunModeType run_mode;
	int n, p, result;
	GtkWidget *preview;
	gchar *rpath = NULL, *dir;
	char port[1024], speed[1024], model[1024];

	g_message ("Setting return values...");
	values[0].type = GIMP_PDB_STATUS;
	values[0].data.d_status = GIMP_PDB_SUCCESS;
	*nreturn_vals = 1;
	*return_vals = values;

	g_message ("Yet another value...");
	values[1].type = GIMP_PDB_INT32;
	values[1].data.d_int32 = 0;

	/* Create the camera */
	g_message ("Create camera...");
	if (((gp_setting_get ("gtkam", "camera", model) == GP_OK) ||
            (gp_setting_get ("gtkam", "model", model) == GP_OK) ||
            (gp_setting_get ("gphoto2", "model", model) == GP_OK)) &&
            ((gp_setting_get ("gtkam", "port", port) == GP_OK) ||
             (gp_setting_get ("gtkam", "port name", port) == GP_OK)) &&
            (gp_setting_get ("gtkam", "speed", speed) == GP_OK)) {
		g_message ("New camera...");
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

	g_message ("We'll return 2 values...");
	*nreturn_vals = 2;

	run_mode = param[0].data.d_int32;
	switch (run_mode) {
	case GIMP_RUN_INTERACTIVE:
#if 0
		g_message ("Interactive!");
		preview = gtkam_preview_new (camera);
		g_assert (preview);
		gp_camera_unref (camera);
		gtk_widget_show (preview);
		gtk_signal_connect (GTK_OBJECT (preview), "captured",
				    GTK_SIGNAL_FUNC (on_captured), &path);
		gtk_signal_connect (GTK_OBJECT (preview), "delete_event",
				    GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
		gtk_main ();
		if (rpath) {
			gp_camera_unref (camera);
			g_warning ("Some more error messages would be fine...");
			values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
			return;
		}
		dir = g_dirname (rpath);
		gp_file_new (&file);
		result = gp_camera_file_get (camera, dir, g_basename (rpath),
					     GP_FILE_TYPE_NORMAL, file);
		gp_camera_unref (camera);
		g_free (dir);
		g_free (rpath);
		if (result < 0) {
			gp_file_unref (file);
			g_warning ("Some more error messages would be fine...");
			values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
			return;
		}
		send_file_to_gimp (file);
		gp_file_unref (file);
		break;
#endif
	case GIMP_RUN_NONINTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:
		g_message ("Non-interactive!");
		result = gp_camera_capture (camera, GP_CAPTURE_IMAGE, &path);
		if (result < 0) {
			g_warning ("Could not capture: %s",
				   gp_result_as_string (result));
			values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
			return;
		}
		gp_file_new (&file);
		g_message ("Getting file...");
		result = gp_camera_file_get (camera, path.folder, path.name,
					     GP_FILE_TYPE_NORMAL, file);
		gp_camera_unref (camera);
		if (result < 0) {
			gp_file_unref (file);
			g_warning ("Some more error messages would be fine...");
			values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
			return;
		}
		g_message ("Sending file to gimp...");
		send_file_to_gimp (file);
		gp_file_unref (file);
		break;
	default:
		gp_camera_unref (camera);
		break;
	}

	g_message ("Done!");
}

GimpPlugInInfo PLUG_IN_INFO = {
	NULL,
	NULL,
	query,
	run
};

MAIN ()
