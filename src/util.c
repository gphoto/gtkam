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
#include <gtk/gtk.h>
#include "support.h"
#include "util.h"

int file_exists(char *filename) {

        FILE *f;

        if ((f = fopen(filename, "r"))) {
                fclose(f);
		return (1);
        }
	return (0);
}
