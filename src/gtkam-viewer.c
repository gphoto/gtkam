/* gtkam-viewer.c
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
#include "gtkam-viewer.h"
#include "i18n.h"

#include <string.h>

#include <gtk/gtkbutton.h>
#include <gtk/gtkimage.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkbox.h>

#include <gdk-pixbuf/gdk-pixbuf-loader.h>

#include <gtkam-error.h>
#include <gtkam-status.h>

struct _GtkamViewerPrivate
{
	GtkWidget *image;
	GdkPixbuf *pixbuf;

	guint timeout_id;

	GdkPixbufLoader *loader;
	CameraFile *file;
	guint size;
};

#define PARENT_TYPE GTKAM_TYPE_DIALOG
static GtkamDialogClass *parent_class;

static void
gtkam_viewer_destroy (GtkObject *object)
{
	GtkamViewer *viewer = GTKAM_VIEWER (object);

	if (viewer->priv->pixbuf) {
		g_object_unref (G_OBJECT (viewer->priv->pixbuf));
		viewer->priv->pixbuf = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_viewer_finalize (GObject *object)
{
	GtkamViewer *viewer = GTKAM_VIEWER (object);

	g_free (viewer->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_viewer_class_init (gpointer g_class, gpointer class_data)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = GTK_OBJECT_CLASS (g_class);
	object_class->destroy  = gtkam_viewer_destroy;

	gobject_class = G_OBJECT_CLASS (g_class);
	gobject_class->finalize = gtkam_viewer_finalize;

	parent_class = g_type_class_peek_parent (g_class);
}

static void
gtkam_viewer_init (GTypeInstance *instance, gpointer g_class)
{
	GtkamViewer *viewer = GTKAM_VIEWER (instance);

	viewer->priv = g_new0 (GtkamViewerPrivate, 1);
}

GType
gtkam_viewer_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo ti;

		memset (&ti, 0, sizeof (GTypeInfo));
		ti.class_size     = sizeof (GtkamViewerClass);
		ti.class_init     = gtkam_viewer_class_init;
		ti.instance_size  = sizeof (GtkamViewer);
		ti.instance_init  = gtkam_viewer_init;

		type = g_type_register_static (PARENT_TYPE, "GtkamViewer",
					       &ti, 0);
	}

	return (type);
}

static void
on_close_clicked (GtkButton *button, GtkamViewer *viewer)
{
	gtk_object_destroy (GTK_OBJECT (viewer));
}

static void
show_pixbuf (GtkamViewer *v, GtkAllocation *allocation, gboolean force)
{
        GdkPixbuf *scaled, *p;
        gint w, h, target_w, target_h, current_w, current_h;

	g_return_if_fail (GTKAM_IS_VIEWER (v));

        if (!v->priv->pixbuf)
                return; 

        w = gdk_pixbuf_get_width (v->priv->pixbuf);
        h = gdk_pixbuf_get_height (v->priv->pixbuf);
printf("Size is %d %d\n", w, h);
printf("Allocation size is %d %d\n", allocation->width, allocation->height);
	p = gtk_image_get_pixbuf (GTK_IMAGE (v->priv->image));
	current_w = (p ? gdk_pixbuf_get_width (p) : 0);
	current_h = (p ? gdk_pixbuf_get_height (p) : 0);
        target_w = MIN (allocation->width, allocation->height * w / h);
        target_h = MIN (allocation->height, allocation->width * h / w);
	if (((target_w == current_w) || (target_h == current_h)) && !force)
		return;
printf("About to scale to %d %d\n", target_w, target_h);
        scaled = gdk_pixbuf_scale_simple (v->priv->pixbuf, target_w, target_h,
                                          GDK_INTERP_HYPER);
        gtk_image_set_from_pixbuf (GTK_IMAGE (v->priv->image), scaled);
	gdk_pixbuf_unref (scaled);
}

static void
on_size_allocate (GtkWidget *widget, GtkAllocation *allocation,
		  GtkamViewer *v)
{
	show_pixbuf (v, allocation, FALSE);
}

GtkWidget *
gtkam_viewer_new (void)
{
	GtkamViewer *v;
	GtkWidget *button;

	v = g_object_new (GTKAM_TYPE_VIEWER, NULL);
	gtk_widget_hide (GTKAM_DIALOG (v)->image);	/* Hide camera pic */
	GtkWidget *hbox = gtk_hbox_new (FALSE, 10);
	gtk_widget_show (hbox);
	gtk_box_pack_end (GTK_BOX (GTKAM_DIALOG (v)->vbox), hbox,
			  TRUE, TRUE, 0);	
	gtk_widget_set_size_request(GTK_WIDGET (hbox), 640, 480);

	
	v->priv->image = gtk_image_new ();
	gtk_widget_show (v->priv->image);
	gtk_box_pack_start (GTK_BOX (hbox), v->priv->image,
			    TRUE, TRUE, 0);
	g_signal_connect (G_OBJECT (v->priv->image), "size_allocate",
			  G_CALLBACK (on_size_allocate), v);

	button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	gtk_widget_show (button);
	g_signal_connect (G_OBJECT (button), "clicked",
			    G_CALLBACK (on_close_clicked), v);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (v)->action_area),
			   button);
	gtk_widget_grab_focus (button);

	return (GTK_WIDGET (v));
}

static gboolean
timeout_func (gpointer data)
{
	GtkamViewer *v = GTKAM_VIEWER (data);
	const char *d;
	unsigned long int s;

	g_return_val_if_fail (v->priv->file, FALSE);
	g_return_val_if_fail (GDK_IS_PIXBUF_LOADER (v->priv->loader), FALSE);

	gp_file_get_data_and_size (v->priv->file, &d, &s);
	if (s > v->priv->size) {
		gdk_pixbuf_loader_write (v->priv->loader, d + v->priv->size,
					 s - v->priv->size, NULL);
	}

	return (TRUE);
}

static void
on_area_prepared (GdkPixbufLoader *loader, GtkamViewer *v)
{
	g_return_if_fail (GTKAM_IS_VIEWER (v));

	if (v->priv->pixbuf)
		g_object_unref (G_OBJECT (v->priv->pixbuf));
	v->priv->pixbuf = gdk_pixbuf_copy (
				gdk_pixbuf_loader_get_pixbuf (loader));
	show_pixbuf (v, &v->priv->image->allocation, TRUE);
}

void
gtkam_viewer_load_file (GtkamViewer *v, GtkamCamera *camera,
			const gchar *folder, const gchar *file)
{
	int r;
	GtkWidget *s, *e;
	GError *error = NULL;

	g_return_if_fail (GTKAM_IS_VIEWER (v));
	g_return_if_fail (GTKAM_IS_CAMERA (camera));
	g_return_if_fail (folder != NULL);
	g_return_if_fail (file != NULL);

	if (v->priv->timeout_id) {
		gtk_timeout_remove (v->priv->timeout_id);
		v->priv->timeout_id = 0;
	}

	s = gtkam_status_new (_("Downloading '%s' from folder '%s'..."),
			      file, folder);
	gtkam_dialog_add_status (GTKAM_DIALOG (v), s);

	/* Prepare the loading */
	if (v->priv->file)
		gp_file_unref (v->priv->file);
	gp_file_new (&v->priv->file);
	v->priv->size = 0;
	v->priv->loader = gdk_pixbuf_loader_new ();
	g_signal_connect (G_OBJECT (v->priv->loader), "area_prepared",
			  G_CALLBACK (on_area_prepared), v);
	v->priv->timeout_id = gtk_timeout_add (1000, timeout_func, v);
	if (v->priv->pixbuf)
		g_object_unref (G_OBJECT (v->priv->pixbuf));
	v->priv->pixbuf = NULL;
	gtk_image_set_from_pixbuf (GTK_IMAGE (v->priv->image), NULL);

	/* Load */
	r = gp_camera_file_get (camera->camera, folder, file,
		GP_FILE_TYPE_NORMAL, v->priv->file,
		GTKAM_STATUS (s)->context->context);

	/* Call the timeout function once again to write remaining data. */
	gtk_timeout_remove (v->priv->timeout_id);
	timeout_func (v);
	v->priv->timeout_id = 0;
	gdk_pixbuf_loader_close (v->priv->loader, &error);
	if (error) {
		g_error_free (error);
		error = NULL;
	} else {
		if (v->priv->pixbuf)
			g_object_unref (G_OBJECT (v->priv->pixbuf));
		v->priv->pixbuf = gdk_pixbuf_loader_get_pixbuf (
							v->priv->loader);
		g_object_ref (G_OBJECT (v->priv->pixbuf));
		show_pixbuf (v, &v->priv->image->allocation, TRUE);

		gtk_window_set_title (GTK_WINDOW (v), file);
	}

	/* Clean up */
	g_object_unref (G_OBJECT (v->priv->loader));
	v->priv->loader = NULL;
	gp_file_unref (v->priv->file);
	v->priv->file = NULL;
	v->priv->size = 0;

	if (camera->multi)
		gp_camera_exit (camera->camera, NULL);
	if (r < 0) {
		e = gtkam_error_new (r, GTKAM_STATUS (s)->context, NULL,
			_("Could not download '%s' from folder '%s'."),
			file, folder);
		gtk_widget_show (e);
		gtk_window_set_transient_for (GTK_WINDOW (e), GTK_WINDOW (v));
		gtk_object_destroy (GTK_OBJECT (s));
		return;
	}
	gtk_object_destroy (GTK_OBJECT (s));
}
