#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gphoto2.h>
#include <dirent.h>
#include <stdio.h>    
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <gdk-pixbuf/gdk-pixbuf-loader.h>
#include <gdk-pixbuf/gdk-pixbuf.h> 
#include "support.h"
#include "util.h"

int exec_command (char *command, char *args) {

	if (fork() == 0) {
		execlp(command, command, args, NULL);
		_exit (0);
	}

	return (GP_OK);
}

int file_exists(char *filename) {

        FILE *f;

        if ((f = fopen(filename, "r"))) {
                fclose(f);
		return (1);
        }
	return (0);
}

int gdk_image_new_from_data (char *image_data, int image_size, int scale,
	GdkPixmap **pixmap, GdkBitmap **bitmap) {

	GdkPixbufLoader *loader;
	GdkPixbuf *pixbuf;
	GdkPixbuf *spixbuf;
	float aspectratio;
	int width, height, neww, newh;

	loader = gdk_pixbuf_loader_new();

	if (!gdk_pixbuf_loader_write (loader, image_data, image_size)) {
		return (GP_ERROR);
	}

	gdk_pixbuf_loader_close (loader);
	pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);

	if (!pixbuf) {
		return (GP_ERROR);
	}

	gdk_pixbuf_ref (pixbuf);
	gtk_object_destroy (GTK_OBJECT (loader));

	if (scale) {
		width  = gdk_pixbuf_get_width(pixbuf);
		height = gdk_pixbuf_get_height(pixbuf);
		aspectratio = height/width;
		if (aspectratio > 0.75) {
                	/* Thumbnail is tall. Adjust height to 60. Width will be < 80. */
			neww = (gint)(60.0 * width / height);
			newh = 60;
		} else {
			/* Thumbnail is wide. Adjust width to 80. Height will be < 60. */
			newh = (gint)(80.0 * height / width);
			neww = 80;
		}
		spixbuf = gdk_pixbuf_scale_simple (pixbuf, neww, newh, GDK_INTERP_NEAREST);
		gdk_pixbuf_render_pixmap_and_mask (spixbuf, pixmap, bitmap, 127);
		gdk_pixbuf_unref(spixbuf);
	} else {
		gdk_pixbuf_render_pixmap_and_mask (pixbuf, pixmap, bitmap, 127);
	}
		
	gdk_pixbuf_unref(pixbuf);

	return (GP_OK);
}

void ok_click (GtkWidget *dialog) {

        gtk_object_set_data(GTK_OBJECT(dialog), "button", "OK");
        gtk_widget_hide(dialog);
}
 
int wait_for_hide (GtkWidget *dialog, 
		   GtkWidget *ok_button,
                   GtkWidget *cancel_button) {
                   
	int cont=1;
	
        gtk_object_set_data(GTK_OBJECT(dialog), "button", "CANCEL");
        gtk_signal_connect_object(
                        GTK_OBJECT(ok_button), "clicked",
                        GTK_SIGNAL_FUNC(ok_click),
                        GTK_OBJECT(dialog));
	if (cancel_button)
	        gtk_signal_connect_object(GTK_OBJECT(cancel_button),
	                        "clicked",
	                        GTK_SIGNAL_FUNC(gtk_widget_hide),
	                        GTK_OBJECT(dialog));
        gtk_widget_show(dialog);
        while (cont) {
		/* because the window manager could destroy the window */
		if(!GTK_IS_OBJECT(dialog)) {
			return 0;
		}
		if (GTK_WIDGET_VISIBLE(dialog))
			gtk_main_iteration();
		else
			cont = 0;
	}

	if(!GTK_IS_OBJECT(dialog)) {
		return 0;
	}

        if (strcmp("CANCEL",
           (char*)gtk_object_get_data(GTK_OBJECT(dialog), "button"))==0) {
		gtk_widget_destroy(dialog);
                return 0;
	}
        return 1;
}
