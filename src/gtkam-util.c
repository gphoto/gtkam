/* gtkam-util.c
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
#include "config.h"
#include "gtkam-util.h"
#include "i18n.h"

#include <gdk-pixbuf/gdk-pixbuf-loader.h>

#include "gtkam-close.h"

GdkPixbuf *
gdk_pixbuf_new_from_camera_file (CameraFile *file, guint max_dim,
				 GtkWidget *opt_window)
{
        GdkPixbufLoader *loader;
        GdkPixbuf *pixbuf;
        const char *data, *name, *type;
        unsigned long size;
        guint w, h; 
        gfloat scale;
        GtkWidget *dialog;
        gchar *msg;

        gp_file_get_data_and_size (file, &data, &size);
        loader = gdk_pixbuf_loader_new ();
        if (!gdk_pixbuf_loader_write (loader, data, size, NULL)) {
                gp_file_get_name (file, &name);
                gp_file_get_mime_type (file, &type);
                msg = g_strdup_printf (_("Could not display '%s'. Either "
                        "the image type ('%s') is not supported by gtk or "
                        "the file itself is corrupt."), name, type);
                dialog = gtkam_close_new (msg);
                g_free (msg);
                gtk_widget_show (dialog);
                gtk_object_destroy (GTK_OBJECT (loader));
                return (NULL);
        }
        gdk_pixbuf_loader_close (loader, NULL);
        pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
        w = gdk_pixbuf_get_width (pixbuf);
        h = gdk_pixbuf_get_height (pixbuf);
        if ((w > max_dim) || (h > max_dim)) {
                scale = MIN ((gfloat) max_dim / w, (gfloat) max_dim / h);
                pixbuf = gdk_pixbuf_scale_simple (pixbuf, scale * w, scale * h,
                                                  GDK_INTERP_NEAREST);
        } else
                gdk_pixbuf_ref (pixbuf);
        g_object_unref (G_OBJECT (loader));

        return (pixbuf);
}
