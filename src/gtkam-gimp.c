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
#include "gtkam-cancel.h"
#include "gtkam-chooser.h"
#include "gtkam-clist.h"
#include "gtkam-error.h"
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

#define PLUG_IN_PRINT_NAME "GtkamGimp"

static void
query (void)
{
	static GimpParamDef load_args[] = {
		{GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
		{GIMP_PDB_STRING, "filename", "The name of the file to load"}};
	static GimpParamDef load_return_vals[] = {
		{GIMP_PDB_IMAGE, "image", "Output image"}};
	static GimpParamDef capture_args[] = {
		{GIMP_PDB_INT32, "CaptureType", "Image, Video, Audio"}};
	static GimpParamDef capture_return_vals[] = {
		{GIMP_PDB_IMAGE, "image", "Output image"}};
	static gint nload_args          = sizeof (load_args) /
				          sizeof (load_args[0]);
	static gint ncapture_args       = sizeof (capture_args) /
				          sizeof (capture_args[0]);
	static int nload_return_vals    = sizeof (load_return_vals) /
					  sizeof (load_return_vals[0]);
	static int ncapture_return_vals = sizeof (capture_return_vals) /
					  sizeof (capture_return_vals[0]);

	gimp_install_procedure ("gtkam-capture", _("Captures images from "
		"digital camera"), "Nothing here yet. Patches welcome.",
		"Lutz Müller <urc8@rz.uni-karlsruhe.de>", "GPL", VERSION,
		N_("<Toolbox>/File/Acquire/Capture from Camera..."),
		NULL, GIMP_EXTENSION, ncapture_args, ncapture_return_vals,
		capture_args, capture_return_vals);

	gimp_install_procedure ("gtkam-load", _("Loads files from digital "
		"cameras"), "Nothing here yet. Patches welcome.", 
		"Lutz Müller <urc8@rz.uni-karlsruhe.de>",
		"GPL", VERSION,
		N_("<Toolbox>/File/Acquire/Load from Camera..."),
		NULL, GIMP_EXTENSION, nload_args, nload_return_vals,
		load_args, load_return_vals);
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

typedef struct {
	Camera *camera;
	gboolean multi;
} CameraSelectionParams;

static void
on_camera_selected (GtkamChooser *chooser, Camera *camera, gboolean multi,
		    CameraSelectionParams *params)
{
	params->camera = camera;
	gp_camera_ref (camera);
	params->multi = multi;
}

typedef struct {
	guint angle;
	gfloat zoom;
} PreviewParams;

#if 0

static int
progress_func (CameraFile *file, float progress, void *data)
{
	if (!gimp_progress_update (progress))
		return (GP_ERROR_CANCEL);

	return (GP_OK);
}
#endif

static Camera *
create_camera (CameraOperation operations, gboolean *multi,
	       gint nparams, GimpParam *param, gint *nreturn_vals,
	       GimpParam **return_vals)
{
	GimpRunModeType run_mode = param[0].data.d_int32;
	GtkWidget *chooser;
	static GimpParam values[1];
	CameraSelectionParams cparams = {NULL, FALSE};

        switch (run_mode) {
        case GIMP_RUN_INTERACTIVE:

                /* Initialize gtk through gimp */
                gimp_ui_init ("gtkam-gimp", TRUE);

                /* Let the use choose a camera */
                chooser = gtkam_chooser_new ();
                gtk_widget_hide (GTKAM_CHOOSER (chooser)->apply_button);
                gtk_widget_show (chooser);
                gtkam_chooser_set_camera_mask (GTKAM_CHOOSER (chooser),
					       operations);
                gtk_signal_connect (GTK_OBJECT (chooser), "camera_selected",
                                    GTK_SIGNAL_FUNC (on_camera_selected),
                                    &cparams);
                gtk_signal_connect (GTK_OBJECT (chooser), "destroy",
                                    GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
                gtk_main ();

                /* Check if the user cancelled */
		if (!cparams.camera) {
			*nreturn_vals = 1;
			*return_vals = values;
			values[0].type = GIMP_PDB_STATUS;
                        values[0].data.d_status = GIMP_PDB_CANCEL;
                        return NULL;
                }

                break;
        case GIMP_RUN_NONINTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:

                g_warning ("Implement!");
		*nreturn_vals = 1;
		*return_vals = values;
		values[0].type = GIMP_PDB_STATUS;
                values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
                return NULL;

                break;
        }

	*multi = cparams.multi;
	return (cparams.camera);
}

static gint32
get_file (Camera *camera, CameraFilePath path, gint nparams, GimpParam *param,
	  gint *nreturn_vals, GimpParam **return_vals)
{
	GtkWidget *dialog, *c;
	CameraFile *file;
	static GimpParam values[1];
	GimpDrawable *drawable;
	GimpPixelRgn pixel_rgn;
	gint32 image_id, layer_id;
	GdkPixbufLoader *loader;
	GdkPixbuf *pixbuf;
	long int size;
	const char *data;
	guchar *pixels;
	int r, result;
	guint w, h;

	gp_file_new (&file);
	c = gtkam_cancel_new (NULL, _("Downloading '%s' from '%s'..."),
			      path.name, path.folder);
	gtk_widget_show (c);
        result = gp_camera_file_get (camera, path.folder, path.name,
		GP_FILE_TYPE_NORMAL, file, GTKAM_CANCEL (c)->context->context);
	gp_camera_exit (camera, NULL);
	switch (result) {
	case GP_OK:
		break;
	case GP_ERROR_CANCEL:
		*nreturn_vals = 1;
		*return_vals = values;
		values[0].type = GIMP_PDB_STATUS;
		values[0].data.d_status = GIMP_PDB_CANCEL;
		gtk_object_destroy (GTK_OBJECT (c));
		return -1;
	default:
                gp_file_unref (file);
		dialog = gtkam_error_new (result, GTKAM_CANCEL (c)->context,
			NULL, _("Could not "
                        "download file '%s' from folder '%s'."),
			path.name, path.folder);
                gtk_widget_show (dialog);

                /* Wait until the user closes the dialog */
                gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
                                    GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
                gtk_main ();

		*nreturn_vals = 1;
		*return_vals = values;
		values[0].type = GIMP_PDB_STATUS;
		values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
		gtk_object_destroy (GTK_OBJECT (c));
                return -1;
        }
	gtk_object_destroy (GTK_OBJECT (c));

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
        gdk_pixbuf_ref (pixbuf);
        gtk_object_unref (GTK_OBJECT (loader));

	image_id = gimp_image_new (w, h, GIMP_RGB);
        gimp_image_set_filename (image_id, path.name);
        layer_id = gimp_layer_new (image_id, _("Background"), w, h,
                                   GIMP_RGB_IMAGE, 100, GIMP_NORMAL_MODE);
        gimp_image_add_layer (image_id, layer_id, 0);
        drawable = gimp_drawable_get (layer_id);
        gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, r, h, TRUE, FALSE);
        gimp_pixel_rgn_set_rect (&pixel_rgn, pixels, 0, 0, w, h);
        gdk_pixbuf_unref (pixbuf);
        gimp_drawable_flush (drawable);
        gimp_drawable_detach (drawable);
        gimp_display_new (image_id);

	return (image_id);
}

static void
run_capture (gchar *name, gint nparams, GimpParam *param, gint *nreturn_vals,
	     GimpParam **return_vals)
{
	Camera *camera = NULL;
	CameraFilePath path;
	CameraAbilities a;
	int result;
	GtkWidget *preview;
	static GimpParam values[2];
	static PreviewParams preview_params = {0, 1.};
	GimpRunModeType run_mode = param[0].data.d_int32;
	gint32 image_id;
	gboolean multi;

	camera = create_camera (GP_OPERATION_CAPTURE_IMAGE, &multi,
				nparams, param, nreturn_vals, return_vals);
	if (!camera)
		return;

	switch (run_mode) {
	case GIMP_RUN_INTERACTIVE:

		/*
		 * Only pop up a preview window in case the camera supports
		 * capturing previews
		 */
		gp_camera_get_abilities (camera, &a);
		if (a.operations & GP_OPERATION_CAPTURE_PREVIEW) {

			/* Get settings from a previous run (if any) */
			gimp_get_data (name, &preview_params);

			/* Create the preview window */
			preview = gtkam_preview_new (camera, multi);
			gtkam_preview_set_angle (GTKAM_PREVIEW (preview),
						 preview_params.angle);
			gtkam_preview_set_zoom (GTKAM_PREVIEW (preview),
						preview_params.zoom);
			gtk_widget_show (preview);
			memset (&path, 0, sizeof (CameraFilePath));
			gtk_signal_connect (GTK_OBJECT (preview), "captured",
				GTK_SIGNAL_FUNC (on_captured), &path);
			gtk_signal_connect (GTK_OBJECT (preview), "destroy",
				GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
			gtk_main ();

			/* Check if the user cancelled */
			if (!strlen (path.folder)) {
				gp_camera_unref (camera);
				*nreturn_vals = 1;
				*return_vals = values;
				values[0].type = GIMP_PDB_STATUS;
				values[0].data.d_status = GIMP_PDB_CANCEL;
				return;
			}

			gtk_widget_hide (preview);
			while (gtk_events_pending ())
				gtk_main_iteration ();

			/* Store the settings for later runs */
			preview_params.zoom =
				gtkam_preview_get_zoom (
						GTKAM_PREVIEW (preview));
			preview_params.angle = 
				gtkam_preview_get_angle (
						GTKAM_PREVIEW (preview));
			gtk_object_destroy (GTK_OBJECT (preview));
			gimp_set_data (name, &preview_params,
				       sizeof (PreviewParams));

			break;
		}

	case GIMP_RUN_NONINTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:
		result = gp_camera_capture (camera, GP_CAPTURE_IMAGE, &path,
					    NULL);
		gp_camera_exit (camera, NULL);
		if (result < 0) {
			*nreturn_vals = 1;
			*return_vals = values;
			values[0].type = GIMP_PDB_STATUS; 
			values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
			return;
		}
	}

	image_id = get_file (camera, path,
			     nparams, param, nreturn_vals, return_vals);
	gp_camera_unref (camera);
	if (image_id < 0)
		return;

	*nreturn_vals = 2;
	*return_vals = values;
	values[0].type = GIMP_PDB_STATUS;
	values[0].data.d_status = GIMP_PDB_SUCCESS;
	values[1].type = GIMP_PDB_IMAGE;
	values[1].data.d_image = image_id;
}

static void
on_fsel_ok_clicked (GtkButton *button, gboolean *selected)
{
	*selected = TRUE;
	gtk_main_quit ();
}

static void
run_load (gchar *name, gint nparams, GimpParam *param, gint *nreturn_vals,
	  GimpParam **return_vals)
{
	CameraFilePath path;
	GtkWidget *fsel;
	gint32 image_id;
	static GimpParam values[2];
	guint i;
	gboolean selected = FALSE;
	GtkamCListEntry *entry;
	GList *list;

	/* Show the file selection dialog */
	fsel = gtkam_fsel_new (NULL);
	gtk_widget_show (fsel);
	gtk_signal_connect (GTK_OBJECT (GTKAM_FSEL (fsel)->ok_button),
		"clicked", GTK_SIGNAL_FUNC (on_fsel_ok_clicked), &selected);
	gtk_signal_connect (GTK_OBJECT (fsel), "destroy",
			    GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
	gtk_main ();

	/* Check if the user cancelled */
	if (!selected) {
		*nreturn_vals = 1;
		*return_vals = values;
		values[0].type = GIMP_PDB_STATUS;
		values[0].data.d_status = GIMP_PDB_CANCEL;
		return;
	}

	gtk_widget_hide (fsel);
	while (gtk_events_pending ())
		gtk_main_iteration ();

	/* Get the file(s) and display it */
	list = GTKAM_CLIST (GTKAM_FSEL (fsel)->clist)->selection;
	for (i = 0; i < g_list_length (list); i++) {
		entry = g_list_nth_data (list, i);
		strcpy (path.folder, entry->folder);
		strcpy (path.name, entry->name);
		image_id = get_file (entry->camera, path,
				     nparams, param, nreturn_vals, return_vals);
	}
	gtk_object_destroy (GTK_OBJECT (fsel));

	*nreturn_vals = 1;
	*return_vals = values;
	values[0].type = GIMP_PDB_STATUS;
	values[0].data.d_status = GIMP_PDB_SUCCESS;
}

static void
run (gchar *name, gint nparams, GimpParam *params, gint *nreturn_vals,
     GimpParam **return_vals)
{
	static GimpParam values[1];

	g_return_if_fail (name != NULL);

	if (!strcmp (name, "gtkam-capture"))
		run_capture (name, nparams, params, nreturn_vals, return_vals);
	else if (!strcmp (name, "gtkam-load"))
		run_load (name, nparams, params, nreturn_vals, return_vals);
	else {
		*nreturn_vals = 1;
		*return_vals = values;
		values[0].type = GIMP_PDB_STATUS;
		values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
	}
}

GimpPlugInInfo PLUG_IN_INFO = {
	NULL,
	NULL,
	query,
	run
};

MAIN ()
