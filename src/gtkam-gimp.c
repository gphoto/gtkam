/* gtkam-gimp.c
 *
 * Copyright © 2001 Lutz Müller <lutz@users.sf.net>
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
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gphoto2/gphoto2-abilities-list.h>
#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-setting.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixbuf-loader.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gtkam-preview.h"
#include "gtkam-cancel.h"
#include "gtkam-chooser.h"
#include "gtkam-error.h"
#include "gtkam-fsel.h"
#include "gtkam-list.h"
#include "i18n.h"

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

	bind_textdomain_codeset (PACKAGE, "UTF-8");

	gimp_install_procedure ("gtkam-capture", _("Captures images from "
		"digital camera"), _("Captures images from digital camera"),
		"Lutz Mueller <lutz@users.sf.net>", "GPL", VERSION,
		_("<Toolbox>/File/Acquire/Capture from Camera..."),
		NULL, GIMP_EXTENSION, ncapture_args, ncapture_return_vals,
		capture_args, capture_return_vals);

	gimp_install_procedure ("gtkam-load", _("Loads files from digital "
		"cameras"), _("Loads files from digital "
		"cameras"),
		"Lutz Mueller <lutz@users.sf.net>",
		"GPL", VERSION,
		_("<Toolbox>/File/Acquire/Load from Camera..."),
		NULL, GIMP_EXTENSION, nload_args, nload_return_vals,
		load_args, load_return_vals);
}

static void
on_captured (GtkamPreview *preview, GtkamPreviewCapturedData *data,
	     CameraFilePath *path)
{
	strcpy (path->folder, data->folder);
	strcpy (path->name, data->name);

	gtk_main_quit ();
}

static void
on_camera_selected (GtkamChooser *chooser, GtkamCamera *camera,
		    GtkamCamera **ret_camera)
{
	*ret_camera = camera;
	g_object_ref (*ret_camera);
}

typedef struct {
	guint angle;
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

static GtkamCamera *
create_camera (CameraOperation operations,
	       gint nparams, GimpParam *param, gint *nreturn_vals,
	       GimpParam **return_vals)
{
	GimpRunMode run_mode = param[0].data.d_int32;
	GtkWidget *chooser;
	static GimpParam values[1];
	GtkamCamera *camera = NULL;

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
                g_signal_connect (G_OBJECT (chooser), "camera_selected",
                                  G_CALLBACK (on_camera_selected), &camera);
                g_signal_connect (G_OBJECT (chooser), "destroy",
                                  G_CALLBACK (gtk_main_quit), NULL);
                gtk_main ();

                /* Check if the user cancelled */
		if (!camera) {
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

	return (camera);
}

static gint32
get_file (GtkamCamera *camera, const gchar *folder, const gchar *name,
	  gint nparams, GimpParam *param,
	  gint *nreturn_vals, GimpParam **return_vals)
{
	GtkWidget *dialog, *c;
	CameraFile *file;
	static GimpParam values[1];
	GimpDrawable *d;
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
	c = gtkam_cancel_new (_("Downloading '%s' from '%s'..."), name, folder);
	gtk_widget_show(c);
        result = gp_camera_file_get (camera->camera, folder, name,
		GP_FILE_TYPE_NORMAL, file, GTKAM_CANCEL(c)->context->context);
	gp_camera_exit (camera->camera, NULL);
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
		dialog = gtkam_error_new (result, GTKAM_CANCEL(c)->context,
			NULL, _("Could not "
                        "download file '%s' from folder '%s'."), name, folder);
                gtk_widget_show (dialog);

                /* Wait until the user closes the dialog */
                g_signal_connect (G_OBJECT (dialog), "destroy",
                                  G_CALLBACK (gtk_main_quit), NULL);
                gtk_main ();

		*nreturn_vals = 1;
		*return_vals = values;
		values[0].type = GIMP_PDB_STATUS;
		values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
		gtk_object_destroy (GTK_OBJECT(c));
                return -1;
        }
	gtk_object_destroy (GTK_OBJECT(c));

	gp_file_get_data_and_size (file, &data, &size);
        loader = gdk_pixbuf_loader_new ();
        gdk_pixbuf_loader_write (loader, data, size, NULL);
        gp_file_unref (file);
        gdk_pixbuf_loader_close (loader, NULL);
        pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
        w = gdk_pixbuf_get_width (pixbuf);
        h = gdk_pixbuf_get_height (pixbuf);
        r = gdk_pixbuf_get_rowstride (pixbuf);
        pixels = gdk_pixbuf_get_pixels (pixbuf);
        gdk_pixbuf_ref (pixbuf);
        g_object_unref (G_OBJECT (loader));

	image_id = gimp_image_new (w, h, GIMP_RGB);
        gimp_image_set_filename (image_id, (gchar *) name);
        layer_id = gimp_layer_new (image_id, _("Image"), w, h,
                                   GIMP_RGB_IMAGE, 100, GIMP_NORMAL_MODE);
        gimp_image_add_layer (image_id, layer_id, 0);
        d = gimp_drawable_get (layer_id);
        gimp_pixel_rgn_init (&pixel_rgn, d, 0, 0, d->width, d->height,
			     TRUE, FALSE);
        gimp_pixel_rgn_set_rect (&pixel_rgn, pixels, 0, 0, w, h);
        gdk_pixbuf_unref (pixbuf);
        gimp_drawable_flush (d);
        gimp_drawable_detach (d);
        gimp_display_new (image_id);

	return (image_id);
}

static void
run_capture (gchar *name, gint nparams, GimpParam *param, gint *nreturn_vals,
	     GimpParam **return_vals)
{
	GtkamCamera *camera = NULL;
	CameraFilePath path;
	CameraAbilities a;
	int result;
	GtkWidget *preview;
	static GimpParam values[2];
	static PreviewParams preview_params = {0};
	GimpRunMode run_mode = param[0].data.d_int32;
	gint32 image_id;
	gulong id;

	camera = create_camera (GP_OPERATION_CAPTURE_IMAGE,
				nparams, param, nreturn_vals, return_vals);
	if (!camera)
		return;

	switch (run_mode) {
	case GIMP_RUN_INTERACTIVE:

		/*
		 * Only pop up a preview window in case the camera supports
		 * capturing previews
		 */
		gp_camera_get_abilities (camera->camera, &a);
		if (a.operations & GP_OPERATION_CAPTURE_PREVIEW) {

			/* Get settings from a previous run (if any) */
			gimp_get_data (name, &preview_params);

			/* Create the preview window */
			preview = gtkam_preview_new (camera);
			gtkam_preview_set_angle (GTKAM_PREVIEW (preview),
						 preview_params.angle);
			gtk_widget_show (preview);
			memset (&path, 0, sizeof (CameraFilePath));
			g_signal_connect (G_OBJECT (preview), "captured",
				G_CALLBACK (on_captured), &path);
			id = g_signal_connect (G_OBJECT (preview), "destroy",
					G_CALLBACK (gtk_main_quit), NULL);
			gtk_main ();

			/* Check if the user cancelled */
			if (!strcmp (path.folder, "")) {
				g_object_unref (G_OBJECT (camera));
				*nreturn_vals = 1;
				*return_vals = values;
				values[0].type = GIMP_PDB_STATUS;
				values[0].data.d_status = GIMP_PDB_CANCEL;
				return;
			}

			/* Store the settings for later runs */
			g_signal_handler_disconnect (G_OBJECT (preview), id);
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
		result = gp_camera_capture (camera->camera, GP_CAPTURE_IMAGE,
					    &path, NULL);
		gp_camera_exit (camera->camera, NULL);
		if (result < 0) {
			*nreturn_vals = 1;
			*return_vals = values;
			values[0].type = GIMP_PDB_STATUS; 
			values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
			return;
		}
	}

	image_id = get_file (camera, path.folder, path.name,
			     nparams, param, nreturn_vals, return_vals);
	g_object_unref (G_OBJECT (camera));
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

typedef struct _LoadData LoadData;
struct _LoadData {
	GtkamList *list;
	gint nparams;
	GimpParam *param;
	gint *nreturn_vals;
	GimpParam **return_vals;
};

static void
load_foreach_func (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
		   gpointer data)
{
	gint32 image_id;
	LoadData *ld = data;
	GtkamCamera *camera;
	gchar *folder, *name;

	camera = gtkam_list_get_camera_from_iter (ld->list, iter);
	folder = gtkam_list_get_folder_from_iter (ld->list, iter);
	name   = gtkam_list_get_name_from_iter   (ld->list, iter);
	image_id = get_file (camera, folder, name, ld->nparams, ld->param,
			     ld->nreturn_vals, ld->return_vals);
	g_free (folder);
	g_free (name);
}

static void
run_load (gchar *name, gint nparams, GimpParam *param, gint *nreturn_vals,
	  GimpParam **return_vals)
{
	GtkWidget *fsel;
	static GimpParam values[2];
	gboolean selected = FALSE;
	GtkTreeSelection *s;
	GimpRunMode run_mode = param[0].data.d_int32;
	LoadData ld;
	gulong id;

	switch (run_mode) {
	case GIMP_RUN_INTERACTIVE:

		/* Initialize gtk through gimp */
		gimp_ui_init ("gtkam-gimp", TRUE);

		/* Show the file selection dialog */
		fsel = gtkam_fsel_new (NULL);
		gtk_widget_show (fsel);
		g_signal_connect (G_OBJECT (GTKAM_FSEL (fsel)->ok_button),
			"clicked", G_CALLBACK (on_fsel_ok_clicked),
			&selected);
		id = g_signal_connect (G_OBJECT (fsel), "destroy",
				       G_CALLBACK (gtk_main_quit), NULL);
		gtk_main ();

		/* Check if the user cancelled */
		if (!selected) {
			*nreturn_vals = 1;
			*return_vals = values;
			values[0].type = GIMP_PDB_STATUS;
			values[0].data.d_status = GIMP_PDB_CANCEL;
			return;
		}

		g_signal_handler_disconnect (G_OBJECT (fsel), id);
		gtk_widget_hide (fsel);
		while (gtk_events_pending ())
			gtk_main_iteration ();

		/* Get the file(s) and display it */
		s = gtk_tree_view_get_selection (
				GTK_TREE_VIEW (GTKAM_FSEL (fsel)->list));
		ld.list = GTKAM_LIST (GTKAM_FSEL (fsel)->list);
		ld.nparams = nparams;
		ld.param = param;
		ld.nreturn_vals = nreturn_vals;
		ld.return_vals = return_vals;
		gtk_tree_selection_selected_foreach (s, load_foreach_func, &ld);
		gtk_object_destroy (GTK_OBJECT (fsel));

		*nreturn_vals = 1;
		*return_vals = values;
		values[0].type = GIMP_PDB_STATUS;
		values[0].data.d_status = GIMP_PDB_SUCCESS;
		break;

	default:
		
		g_warning ("Implement!");
		*nreturn_vals = 1;
		*return_vals = values;
		values[0].type = GIMP_PDB_STATUS;
		values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
		break;
	}
}

static void
run (gchar *name, gint nparams, GimpParam *params, gint *nreturn_vals,
     GimpParam **return_vals)
{
	static GimpParam values[1];

	g_return_if_fail (name != NULL);

	bind_textdomain_codeset (PACKAGE, "UTF-8");

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
