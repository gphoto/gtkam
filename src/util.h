#include <gtk/gtk.h>

int exec_command (char *command, char *args);
int file_exists (char *filename);
int gdk_image_new_from_data (char *image_data, int image_size, int scale,
	GdkPixmap **pixmap, GdkBitmap **bitmap);
