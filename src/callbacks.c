#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gphoto2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "gtkiconlist.h"
#include "frontend.h"
#include "interface.h"
#include "support.h"
#include "globals.h"
#include "util.h"

#include <gtkam-config.h>
#include <gtkam-debug.h>
#include <gtkam-error.h>
#include <gtkam-preview.h>
#include <gtkam-tree.h>

#include "../pixmaps/no_thumbnail.xpm"

char *current_folder();

#define CHECK_RESULT(res) {int r = (res); if (r < 0) { GtkWidget *dialog = gtkam_error_new ("Error", r, NULL); gtk_widget_show (dialog); return; }}
#define CHECK_CAM_RESULT(c,res) {int r = (res); if (r < 0) { GtkWidget *dialog = gtkam_error_new ("Error", r, c); gtk_widget_show (dialog); return; }}

void debug_print (char *message) {
	if (gp_gtk_debug)
		printf("%s\n", message);
}

gboolean get_thumbnail (GtkWidget *widget, const char *name, GtkIconListItem *item)
{
	CameraFile *f;
	char *folder;
	GdkPixmap *pixmap;
	GdkBitmap *bitmap;
	const char *data;
	long int size;

	/* Get the thumbnail */
	gp_file_new(&f);
	folder = current_folder();
	if (gp_camera_file_get(gp_gtk_camera, folder, name, GP_FILE_TYPE_PREVIEW, f) == GP_OK) {
		gp_file_get_data_and_size (f, &data, &size);
		gdk_image_new_from_data((char*)data,size,1,&pixmap,&bitmap);
		gtk_pixmap_set(GTK_PIXMAP(item->pixmap), pixmap, bitmap);
	}
	gp_file_free(f);

	return TRUE;
}

gboolean toggle_icon (GtkWidget *widget, GdkEventButton *event, gpointer data) {

	GtkWidget *icon_list = (GtkWidget*) lookup_widget(gp_gtk_main_window, "icons");
	GtkIconListItem *item;
	GList *child = GTK_ICON_LIST(icon_list)->icons;
	CameraList list;
	const char *name;
	int x,count=0;

	while (child) {
		item = (GtkIconListItem*)child->data;
		if ((item->eventbox == widget)||(item->entry == widget)) {
			/* A double-click on an index icon retrieve the thumbnail */
			if(event->type == GDK_2BUTTON_PRESS) {
				if (gp_camera_folder_list_files(gp_gtk_camera, current_folder(), &list)!=GP_OK) {
					frontend_message(NULL, _("Could not retrieve the picture list."));
					return TRUE;
				}
				count = gp_list_count(&list);
				for (x=0;x<count;x++) {
					gp_list_get_name (&list, x, &name);
					if(strcmp(name,item->label)==0) {
						get_thumbnail(widget, name, item);
						item->state = GTK_STATE_SELECTED;
					}
				}
			}
		   if (item->state == GTK_STATE_SELECTED)
			gtk_icon_list_unselect_icon(GTK_ICON_LIST(icon_list), item);
		     else
			gtk_icon_list_select_icon(GTK_ICON_LIST(icon_list), item);
		   idle();
		   gtk_entry_set_position(GTK_ENTRY(item->entry), 10000);
		   return TRUE;
		}
		child = child->next;
	}
	return TRUE;
}

void idle() {
	/* Let GTK do some processing */

        while (gtk_events_pending())
                gtk_main_iteration();
	// usleep(100000);
        while (gtk_events_pending())
                gtk_main_iteration();
}


void icon_resize(GtkWidget *window) {

	GtkWidget *icon_list = (GtkWidget*)lookup_widget(gp_gtk_main_window, "icons");
	int x, y;

	gdk_window_get_size(gp_gtk_main_window->window, &x, &y);

	if (abs(gp_gtk_old_width-x)>60) {
		gtk_icon_list_freeze(GTK_ICON_LIST(icon_list));
		gtk_icon_list_thaw(GTK_ICON_LIST(icon_list));
		gp_gtk_old_width = x;
	}

}

int camera_set() {

	GtkWidget *folder_tree, *message, *message_label, *icon_list;
	Camera *new_camera;
	char camera[1024], port[1024], speed[1024];

	/* Mark camera as not init'd */
	gp_gtk_camera_init = 0;

	/* Create a transient window for "Initizlizing camera" message */
	message = create_message_window_transient();
	message_label = (GtkWidget*) lookup_widget(message, "message");
	gtk_label_set_text(GTK_LABEL(message_label), _("Initializing camera..."));

	/* Retrieve which camera to use */
	if (gp_setting_get("gtkam", "camera", camera)!=GP_OK) {
		frontend_message(NULL, _("You must choose a camera"));
		camera_select();
	}

	/* Retrieve the port to use */
	gp_setting_get("gtkam", "port", port);

	/* Retrieve the speed to use */
	gp_setting_get("gtkam", "speed", speed);

	/* Create the new camera */
	if (gp_camera_new(&new_camera)!=GP_OK) {
		gtk_widget_destroy(message);
		return (GP_ERROR);
	}

	/* Set up the camera initialization */
	gp_camera_set_model (new_camera, camera);
	gp_camera_set_port_path (new_camera, port);
	if (strlen(speed)>0)
		gp_camera_set_port_speed (new_camera, atoi (speed));
	gtk_widget_show(message);
	idle();

	if (gp_camera_init(new_camera)!=GP_OK) {
		gtk_widget_destroy(message);
		return (GP_ERROR);
	}		

	gtk_widget_destroy(message);

	/* Clear out the icon listing */
	icon_list = (GtkWidget*) lookup_widget(gp_gtk_main_window, "icons");
	gtk_icon_list_clear (GTK_ICON_LIST(icon_list));

	/* Clear out the folder tree */
	folder_tree = (GtkWidget*) lookup_widget(gp_gtk_main_window, "folder_tree");
	gtkam_tree_set_camera (GTKAM_TREE (folder_tree), new_camera);

	/* Mark the camera as init'd */
	gp_gtk_camera_init = 1;

	/* Destroy the old camera after the new one is successfully loaded */
	if (gp_gtk_camera)
		gp_camera_unref(gp_gtk_camera);

	/* Set the new camera to be active */
	gp_gtk_camera = new_camera;

	return (GP_OK);
}

int main_quit(GtkWidget *widget, gpointer data) {

	char buf[1024];
	int x, y;

	/* Save the window size */
	gdk_window_get_size(gp_gtk_main_window->window, &x, &y);
	sprintf(buf, "%i", x);
	gp_setting_set("gtkam", "width", buf);
	sprintf(buf, "%i", y);
	gp_setting_set("gtkam", "height", buf);

	if (gp_gtk_camera_init)
		gp_exit();
	gtk_main_quit();

	return (FALSE);
}

/* File operations */
/* ----------------------------------------------------------- */

void open_photo() {
	
	GtkWidget *filesel;
	char buf[1024];

	debug_print("open photo");

	filesel = gtk_file_selection_new("Open a photo");
	gp_setting_get("gtkam", "cwd", buf);
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(filesel), buf);

	if (wait_for_hide(filesel, GTK_FILE_SELECTION(filesel)->ok_button,
	    GTK_FILE_SELECTION(filesel)->cancel_button)==0)
		return;
}

void open_directory() {
	debug_print("open directory");
}

void toggle_sensitivity (GtkWidget *button, gpointer data) {

	GtkWidget *widget = (GtkWidget*)data;

	if (GTK_WIDGET_SENSITIVE(widget))
		gtk_widget_set_sensitive(widget, FALSE);
	   else
		gtk_widget_set_sensitive(widget, TRUE);
}

void save_selected_photos() {

	GtkWidget *icon_list, *window, *ok, *cancel, 
		  *use_camera_filename, *save_photos, *save_thumbs, *prefix_entry,
		  *program;
	GtkIconListItem *item;
	CameraFile *f;
	char msg[1024], fname[1024];
	char *path, *prefix=NULL, *slash;
	char *folder, *progname;
	int num=0, x;
	const char *type;

	debug_print("save selected photo");

	/* Count the number of icons selected */
	icon_list = (GtkWidget*)lookup_widget(gp_gtk_main_window, "icons");
	for (x=0; x<GTK_ICON_LIST(icon_list)->num_icons; x++) {
		item = gtk_icon_list_get_nth(GTK_ICON_LIST(icon_list), x);
		if (item->state == GTK_STATE_SELECTED)
			num++;
	}

	if (num == 0) {
		frontend_message(NULL, "Please select some photos to save first");
		return;
	}

	if (num > 1)
		window = create_save_window(1);
	   else
		window = create_save_window(0);

	ok = GTK_FILE_SELECTION(window)->ok_button;
	cancel = GTK_FILE_SELECTION(window)->cancel_button;

	if (wait_for_hide(window, ok, cancel)==0)
		return;

	/* Get some settings from the file selection window */
	path = gtk_file_selection_get_filename(GTK_FILE_SELECTION(window));	
	use_camera_filename = (GtkWidget*)lookup_widget(window, "use_camera_filename");
	save_photos = (GtkWidget*)lookup_widget(window, "save_photos");
	save_thumbs = (GtkWidget*)lookup_widget(window, "save_thumbs");
	program = (GtkWidget*)lookup_widget(window, "program");

	if (num > 1) {
		prefix_entry = (GtkWidget*)lookup_widget(window, "prefix");
		prefix = gtk_entry_get_text(GTK_ENTRY(prefix_entry));
	}

	frontend_progress(gp_gtk_camera, NULL, 0.00);
	gtk_widget_show(gp_gtk_progress_window);
	for (x=0; x<GTK_ICON_LIST(icon_list)->num_icons; x++) {
		item = gtk_icon_list_get_nth(GTK_ICON_LIST(icon_list), x);
		if (item->state == GTK_STATE_SELECTED) {
		   /* Save the photo */
		   if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(save_photos))) {
			sprintf(msg, "Saving photo #%04i of %04i\n%s", x+1,
				GTK_ICON_LIST(icon_list)->num_icons,
				(char*)gtk_object_get_data(GTK_OBJECT(item->pixmap), "name"));
			frontend_message(gp_gtk_camera, msg);
			gp_file_new(&f);
			folder = current_folder();
			if (gp_camera_file_get(gp_gtk_camera, folder,
                                               gtk_object_get_data(GTK_OBJECT(item->pixmap),"name"), GP_FILE_TYPE_NORMAL, f) < 0) {
                            sprintf(fname, "An error occurred when getting %s",
                                    (char*)gtk_object_get_data(GTK_OBJECT(item->pixmap),"name"));
                            frontend_message(gp_gtk_camera, fname);
                        } else {
                            /* determine the name to use */
                            if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(use_camera_filename))) {
                                slash = strrchr(path, '/');
                                *slash = 0;
                                sprintf(fname, "%s/%s", path,
                                        (char*)gtk_object_get_data(GTK_OBJECT(item->pixmap), "name"));
                                *slash = '/';
                            } else {
                                if (num == 1) {
                                    strcpy(fname, path);
                                } else {
				    gp_file_get_mime_type (f, &type);
                                    slash = strrchr(type, '/');
                                    slash++;
                                    if (prefix) {
                                        if (slash)
                                            sprintf(fname, "%s%s%04i.%s", path, prefix, x, slash);
                                        else
                                            sprintf(fname, "%s%s%04i", path, prefix, x);
                                    } else {
                                        if (slash)
                                            sprintf(fname, "%sphoto%04i.%s", path, x, slash);
                                        else
                                            sprintf(fname, "%sphoto%04i", path, x);
                                    }
                                }
                            }
                            progname = gtk_entry_get_text(GTK_ENTRY(program));

                            /* check for existing file */
                            if (file_exists(fname)) {
                                sprintf(msg, "%s already exists. Overwrite?", fname);
                                gtk_widget_hide(gp_gtk_progress_window);
                                if (frontend_confirm(gp_gtk_camera, msg)) {
                                    gp_file_save(f, fname);
                                    if (strlen(progname)>0)
                                        exec_command(progname, fname);
                                }
                                gtk_widget_show(gp_gtk_progress_window);
                            } else {
                                gp_file_save(f, fname);
                                if (strlen(progname)>0)
                                    exec_command(progname, fname);
                            }
                        }
                        gp_file_free(f);
                   }

		   /* Save the thumbnail */
		   if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(save_thumbs))) {
			sprintf(msg, "Saving thumbnail #%04i of %04i\n%s", x+1, num,
				(char*)gtk_object_get_data(GTK_OBJECT(item->pixmap), "name"));
			frontend_status(gp_gtk_camera, msg);
			gp_file_new(&f);
			folder = current_folder();
			gp_camera_file_get(gp_gtk_camera, folder,
			   gtk_object_get_data(GTK_OBJECT(item->pixmap), "name"), GP_FILE_TYPE_PREVIEW, f);
			/* determine the name to use */
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(use_camera_filename))) {
				slash = strrchr(path, '/');
				*slash = 0;
				sprintf(fname, "%s/thumb_%s", path, 
					(char*)gtk_object_get_data(GTK_OBJECT(item->pixmap), "name"));
				*slash = '/';
			} else {
			   if (num == 1) {
				strcpy(fname, path);
			   } else {
				if (prefix)
					sprintf(fname, "%sthumb_%s%04i", path, prefix, x);
				   else
					sprintf(fname, "%sthumb_%04i", path, x);
			   }
			}

			progname = gtk_entry_get_text(GTK_ENTRY(program));

			/* check for existing file */
			if (file_exists(fname)) {
				sprintf(msg, "%s already exists. Overwrite?", fname);
				if (frontend_confirm(gp_gtk_camera, msg)) {
					gp_file_save(f, fname);
					if (strlen(progname)>0)
						exec_command(progname, fname);
				} else {
					/* Ask for another filename! */
				}
			} else {
				gp_file_save(f, fname);
				if (strlen(progname)>0)
					exec_command(progname, fname);
			}
			gp_file_free(f);
		   }
		}
	}
	gtk_widget_destroy(window);


        frontend_progress(gp_gtk_camera, NULL, 0.00);
	gtk_widget_hide(gp_gtk_progress_window);
}

void export_gallery() {
	debug_print("export gallery");

}

void close_photo() {
	debug_print("close photo");

}

/* Selection operations */
/* ----------------------------------------------------------- */

void select_all() {
	GtkWidget *icon_list = (GtkWidget*) lookup_widget(gp_gtk_main_window, "icons");
	GtkIconListItem *item;
	int x;

	debug_print("select all");

	for (x=0; x<GTK_ICON_LIST(icon_list)->num_icons; x++) {
		item = gtk_icon_list_get_nth(GTK_ICON_LIST(icon_list), x);
		gtk_icon_list_select_icon(GTK_ICON_LIST(icon_list),item);
	}
}

void select_inverse() {
	GtkWidget *icon_list = (GtkWidget*) lookup_widget(gp_gtk_main_window, "icons");
	GtkIconListItem *item;
	int x;

	debug_print("select inverse");

	for (x=0; x<GTK_ICON_LIST(icon_list)->num_icons; x++) {
		item = gtk_icon_list_get_nth(GTK_ICON_LIST(icon_list), x);
		if (item->state == GTK_STATE_SELECTED)
			gtk_icon_list_unselect_icon(GTK_ICON_LIST(icon_list),item);
		   else
			gtk_icon_list_select_icon(GTK_ICON_LIST(icon_list),item);
	}
}

void select_none() {
	GtkWidget *icon_list = (GtkWidget*) lookup_widget(gp_gtk_main_window, "icons");
	GtkIconListItem *item;
	int x;

	debug_print("select none");

	for (x=0; x<GTK_ICON_LIST(icon_list)->num_icons; x++) {
		item = gtk_icon_list_get_nth(GTK_ICON_LIST(icon_list), x);
		gtk_icon_list_unselect_icon(GTK_ICON_LIST(icon_list),item);
	}
}

/* Folder Callbacks */
/* ----------------------------------------------------------- */

char *current_folder () {

	GtkWidget *folder_tree;
	char *path;

	folder_tree = lookup_widget(gp_gtk_main_window, "folder_tree");
	path = gtkam_tree_get_path (GTKAM_TREE (folder_tree));

	return (path);
}

void folder_refresh (GtkWidget *widget, gpointer data) {

	GtkWidget *icon_list;

	debug_print("folder refresh");

	if (!gp_gtk_camera_init)
		CHECK_RESULT (camera_set ());

	icon_list = (GtkWidget*) lookup_widget(gp_gtk_main_window, "icons");
	gtk_icon_list_clear (GTK_ICON_LIST(icon_list));

	//FIXME
	camera_index(current_folder ());
}

/* Camera operations */
/* ----------------------------------------------------------- */

void camera_select_update_port(GtkWidget *entry, gpointer data) {

	GtkWidget *window, *speed;
	CameraPortType type = GP_PORT_NONE;
	char *t, *new_port, *path;
	char buf[1024];

	new_port = (char*)gtk_entry_get_text(GTK_ENTRY(entry));
	if (strlen(new_port)==0)
		return;

	sprintf(buf, "Selected port: %s", new_port);
	debug_print(buf);

	window = (GtkWidget*)data;
	speed  = (GtkWidget*)lookup_widget(window, "speed");
	t      = (char*)gtk_object_get_data (GTK_OBJECT(window), new_port);
	if (!t) {
		debug_print("port type not being set");
		return;
	}

	type   = (CameraPortType)atoi(t);
	sprintf(buf, "%s path", new_port);
	path = gtk_object_get_data(GTK_OBJECT(window), buf);

	/* Disabled by default */
	gtk_widget_set_sensitive(GTK_WIDGET(speed), FALSE);
	if (type == GP_PORT_SERIAL)
		gtk_widget_set_sensitive(GTK_WIDGET(speed), TRUE);
	   else
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(speed)->entry), "");

}

void camera_select_update_camera(GtkWidget *entry, gpointer data) {

	/* populate the speed list */
	GtkWidget *window, *port, *speed;
	GList *port_list, *speed_list;
	CameraAbilities a;
	CameraPortInfo info;
	char *new_camera;
	char buf[1024], buf1[1024];
	int num_ports, x=0, append;

	new_camera = (char*)gtk_entry_get_text(GTK_ENTRY(entry));
	if (strlen(new_camera)==0)
		return;

	window = (GtkWidget*)data;
	port   = (GtkWidget*)lookup_widget(window, "port");
	speed  = (GtkWidget*)lookup_widget(window, "speed");

	sprintf(buf, "Selected camera: %s", new_camera);
	debug_print(buf);

	if (gp_camera_abilities_by_name(new_camera, &a)!=GP_OK) {
		sprintf(buf, _("Could not get abilities for %s"), new_camera);
		frontend_message(NULL, buf);
		return;
	}

	/* Get the number of ports */
	if ((num_ports = gp_port_count_get())<0) {
		frontend_message(NULL, _("Could not retrieve number of ports"));
		return;
	}

	/* Free the old port list?? */

	/* populate the port list */
        port_list = g_list_alloc();
        for (x=0; x<num_ports; x++) {
                append=0;
                if (gp_port_info_get(x, &info) == GP_OK) {
                        if ((info.type == GP_PORT_SERIAL) &&
				SERIAL_SUPPORTED(a.port))
				append=1;
			if ((info.type == GP_PORT_PARALLEL) &&
				PARALLEL_SUPPORTED(a.port))
				append=1;
			if ((info.type == GP_PORT_IEEE1394) &&
				IEEE1394_SUPPORTED(a.port))
				append=1;
			if ((info.type == GP_PORT_NETWORK) &&
				NETWORK_SUPPORTED(a.port))
				append=1;
			if ((info.type == GP_PORT_USB) &&
				USB_SUPPORTED(a.port))
				append=1;
                        if (append) {
				sprintf(buf, "%s (%s)", info.name, info.path);
				port_list = g_list_append(port_list, strdup(buf));

				/* Associate a path with this entry */
				sprintf(buf1, "%s path", buf);
				gtk_object_set_data(GTK_OBJECT(window), 
					buf1, (gpointer)strdup(info.path));

				/* Associate a type with this entry */
				sprintf(buf1, "%i", info.type);
				gtk_object_set_data(GTK_OBJECT(window), 
					buf, (gpointer)strdup(buf1));
			}
		}  else {
			frontend_message(NULL, _("Error retrieving the port list"));
		}
	}
	gtk_combo_set_popdown_strings(GTK_COMBO(port), port_list);

	/* Free the old speed list?? */

	/* Populate the speed list */
	speed_list = g_list_alloc();
	x=0;
	while (a.speed[x] != 0) {
		sprintf(buf, "%i", a.speed[x++]);
		speed_list = g_list_append(speed_list, strdup(buf));
	}
	gtk_combo_set_popdown_strings(GTK_COMBO(speed), speed_list);

	gtk_widget_set_sensitive(GTK_WIDGET(port), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(speed), FALSE);
	if (a.port)
		gtk_widget_set_sensitive(GTK_WIDGET(port), TRUE);
}

void camera_select() {
	GtkWidget *window, *ok, *cancel, *camera, *port, *speed, *dialog;
	GList *camera_list;
	int num_cameras, x;
	char buf[128];
	const char *name;
	char *camera_name, *port_name, *port_path, *speed_name;
	int result;
	
	debug_print("camera select");

	/* Get the number of cameras */
	if ((num_cameras = gp_camera_count())<0) {
		frontend_message(NULL, _("Could not retrieve number of cameras"));
		return;
	}

	window = create_select_camera_window();
	ok     = (GtkWidget*)lookup_widget(window, "ok");
	cancel = (GtkWidget*)lookup_widget(window, "cancel");
	camera = (GtkWidget*)lookup_widget(window, "camera");
	port   = (GtkWidget*)lookup_widget(window, "port");
	speed  = (GtkWidget*)lookup_widget(window, "speed");

	gtk_widget_set_sensitive(GTK_WIDGET(port), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(speed), FALSE);

	/* populate the camera list */
	camera_list = g_list_alloc();
	for (x=0; x<num_cameras; x++) {
		if (gp_camera_name(x, &name)==GP_OK)
			camera_list = g_list_append(camera_list, strdup(name));
		   else
			camera_list = g_list_append(camera_list, "ERROR");
	}
	gtk_combo_set_popdown_strings(GTK_COMBO(camera), camera_list);

	/* Update the dialog if they change the camera */
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(camera)->entry), "changed",
		GTK_SIGNAL_FUNC (camera_select_update_camera), (gpointer)window);

	/* Update the dialog if they change the port */
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(port)->entry), "changed",
		GTK_SIGNAL_FUNC (camera_select_update_port), (gpointer)window);

	/* Retrieve the saved values */
	if (gp_setting_get("gtkam", "camera", buf)==GP_OK)
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(camera)->entry), buf);
	if (gp_setting_get("gtkam", "port name", buf)==GP_OK)
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(port)->entry), buf);
	if (gp_setting_get("gtkam", "speed", buf)==GP_OK)
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(speed)->entry), buf);

camera_select_again:
	if (wait_for_hide(window, ok, cancel) == 0)
		return;

	idle(); /* let GTK catch up */

	camera_name = (char*)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(camera)->entry));
	port_name   = (char*)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(port)->entry));
	speed_name  = (char*)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(speed)->entry));

	if (strlen(camera_name)==0) {
		frontend_message(NULL, _("You must choose a camera model"));
		goto camera_select_again;
	}

	if ((strlen(port_name)==0)&&(GTK_WIDGET_SENSITIVE(port))) {
		frontend_message(NULL, _("You must choose a port"));
		goto camera_select_again;
	}

	if ((strlen(speed_name)==0)&&(GTK_WIDGET_SENSITIVE(speed))) {
		frontend_message(NULL, _("You must choose a port speed"));
		goto camera_select_again;
	}

	gp_setting_set("gtkam", "camera", camera_name);

	if (GTK_WIDGET_SENSITIVE(port)) {
		sprintf(buf, "%s path", port_name);
		port_path = (char*)gtk_object_get_data(GTK_OBJECT(window), buf);
		gp_setting_set("gtkam", "port name", port_name);
		gp_setting_set("gtkam", "port", port_path);
	}  else {
		gp_setting_set("gtkam", "port name", "");
		gp_setting_set("gtkam", "port", "");
	}

	if (GTK_WIDGET_SENSITIVE(speed))
		gp_setting_set("gtkam", "speed", speed_name);
	   else
		gp_setting_set("gtkam", "speed", "");


	   result = camera_set ();
	   if (result != GP_OK) {
		   dialog = gtkam_error_new ("Error", result, NULL);
		   gtk_widget_show (dialog);
		   goto camera_select_again;
	   }

	/* Clean up */
	gtk_widget_destroy(window);
}

void camera_index (const gchar *path) {

	CameraFile *f;
	CameraAbilities a;
	CameraList list;
	GtkWidget *icon_list, *camera_tree, *use_thumbs;
	GtkIconListItem *item;
	GdkPixmap *pixmap;
	GdkBitmap *bitmap;
	char buf[1024];
	int x, count=0, get_thumbnails = 1;
	const char *name;
	const char *data;
	long int size;

	debug_print("camera index");
	
	if (!gp_gtk_camera_init)
		CHECK_RESULT (camera_set ());

	icon_list   = (GtkWidget*) lookup_widget(gp_gtk_main_window, "icons");
	GTK_ICON_LIST(icon_list)->is_editable = FALSE;

	if (gp_setting_get("gtkam", "camera", buf)!=GP_OK) {
		frontend_message(NULL, _("ERROR: please choose your camera model again"));
		camera_select();
		return;
	}
	if (gp_camera_abilities_by_name(buf, &a)!=GP_OK) {
		frontend_message(NULL, _("Could not retrieve the camera's abilities"));
		return;
	}

	if (gp_camera_folder_list_files(gp_gtk_camera, path, &list)!=GP_OK) {
		frontend_message(NULL, _("Could not retrieve the picture list."));
		return;
	}

	count = gp_list_count(&list);

	if (count == 0)
		return;

	gtk_icon_list_clear (GTK_ICON_LIST(icon_list));
	camera_tree = (GtkWidget*) lookup_widget(gp_gtk_main_window, "folder_tree");
	use_thumbs  = (GtkWidget*) lookup_widget(gp_gtk_main_window, "use_thumbs");
	gtk_widget_set_sensitive(camera_tree, FALSE);
	gtk_widget_show(gp_gtk_progress_window);
	x=0;
	while ((x<count)&&(GTK_WIDGET_VISIBLE(gp_gtk_progress_window))) {
		gp_list_get_name (&list, x, &name);
		sprintf(buf,_("Getting Thumbnail #%04i of %04i\n%s"), x+1, count,
			name);
		frontend_message(NULL, buf);
		idle();
		if ((get_thumbnails)&&
		    (a.file_operations & GP_FILE_OPERATION_PREVIEW)&&
		    (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(use_thumbs)))) {
			/* Get the thumbnails */
			gp_file_new(&f);
			if (gp_camera_file_get(gp_gtk_camera, 
			    path, name, GP_FILE_TYPE_PREVIEW, f) == GP_OK) {
				gp_file_get_data_and_size (f, &data, &size);
				gdk_image_new_from_data((char*)data,size,1,&pixmap,&bitmap);
				item = gtk_icon_list_add_from_data(GTK_ICON_LIST(icon_list),
					no_thumbnail_xpm,name,NULL);
				gtk_pixmap_set(GTK_PIXMAP(item->pixmap), pixmap, bitmap);
			} else {
				item = gtk_icon_list_add_from_data(GTK_ICON_LIST(icon_list),
					no_thumbnail_xpm,name,NULL);
			}
			gp_file_free(f);
		} else {
			/* Use a dummy icon */
			item = gtk_icon_list_add_from_data(GTK_ICON_LIST(icon_list),
				no_thumbnail_xpm,name,NULL);
		}
		gtk_object_set_data(GTK_OBJECT(item->pixmap), "name", strdup(name));
		gtk_signal_connect(GTK_OBJECT(item->eventbox), "button_press_event",
			GTK_SIGNAL_FUNC(toggle_icon), NULL);
		gtk_signal_connect(GTK_OBJECT(item->entry), "button_press_event",
			GTK_SIGNAL_FUNC(toggle_icon), NULL);
		x++;
	}
	gtk_widget_hide(gp_gtk_progress_window);
	gtk_widget_set_sensitive(camera_tree, TRUE);
}

void
on_folder_selected (GtkamTree *tree, const gchar *folder, gpointer data)
{
	camera_index (folder);
}

void camera_delete_common(int all) {

	GtkWidget *icon_list;
	GtkIconListItem *item;
	CameraAbilities a;
	char buf[1024];
	int x, count=0;

	if (!gp_gtk_camera_init)
		CHECK_RESULT (camera_set ());

	if (gp_setting_get("gtkam", "camera", buf)!=GP_OK) {
		frontend_message(NULL, _("ERROR: please choose your camera model again"));
		camera_select();
		return;
	}

	if (gp_camera_abilities_by_name(buf, &a)!=GP_OK) {
		frontend_message(NULL, _("Could not retrieve the camera's abilities"));
		return;
	}

	if (!(a.file_operations & GP_FILE_OPERATION_DELETE)) {
		frontend_message(NULL, _("This camera does not support deleting photos."));
		return;
	}

	if (all)
		strcpy(buf, _("Are you sure you want to DELETE ALL the photos?"));
	   else
		strcpy(buf, _("Are you sure you want to DELETE the selected photos?"));
		
	if (frontend_confirm(gp_gtk_camera,  buf)==0)
		return;

	icon_list = (GtkWidget*) lookup_widget(gp_gtk_main_window, "icons");
	count = GTK_ICON_LIST(icon_list)->num_icons;

	if (all) {
		for (x=0; x<count; x++) {
			item = gtk_icon_list_get_nth(GTK_ICON_LIST(icon_list), 0);
			if (gp_camera_file_delete(gp_gtk_camera, current_folder(), 
			    gtk_object_get_data(GTK_OBJECT(item->pixmap), "name"))!=GP_OK) {
				sprintf(buf, _("Could not delete photo #%04i. Stopping."), x);
				frontend_message(NULL, buf);
				return;
			}
			gtk_icon_list_remove(GTK_ICON_LIST(icon_list), item);
			gtk_icon_list_freeze(GTK_ICON_LIST(icon_list));
			gtk_icon_list_thaw(GTK_ICON_LIST(icon_list));
		}
		return;
	}
	
	for (x=0; x<count; x++) {
		item = gtk_icon_list_get_nth(GTK_ICON_LIST(icon_list), x);
		if (item->state == GTK_STATE_SELECTED) {			
			if (gp_camera_file_delete(gp_gtk_camera, current_folder(),
			    gtk_object_get_data(GTK_OBJECT(item->pixmap), "name"))!=GP_OK) {
				sprintf(buf, _("Could not delete photo #%04i. Stopping."), x);
				frontend_message(NULL, buf);				
				return;
			}
			gtk_icon_list_remove(GTK_ICON_LIST(icon_list), item);
			gtk_icon_list_freeze(GTK_ICON_LIST(icon_list));
			gtk_icon_list_thaw(GTK_ICON_LIST(icon_list));
			x--;
			count--;
		}
	}
}

void camera_delete_selected() {
	debug_print("camera delete selected");

	camera_delete_common(0);
}

void camera_delete_all() {

	debug_print("camera delete all");

	if (!gp_gtk_camera_init)
		CHECK_RESULT (camera_set ());

	camera_delete_common(1);
}

void camera_show_information() {

	CameraText buf;
	GtkWidget *message, *message_label;

	debug_print("camera information");

	if (!gp_gtk_camera_init)
		CHECK_RESULT (camera_set ());

	message = create_message_window_transient();
	message_label = (GtkWidget*) lookup_widget(message, "message");
	gtk_label_set_text(GTK_LABEL(message_label), _("Retrieving camera information..."));

	gtk_widget_show(message);
	idle();
	if (gp_camera_get_summary(gp_gtk_camera, &buf)!=GP_OK) {
		gtk_widget_hide(message);
		frontend_message(gp_gtk_camera, _("Could not retrieve camera information"));
		return;
	}
	gtk_widget_hide(message);

	frontend_message(gp_gtk_camera, buf.text);
}

void camera_show_manual() {

	CameraText buf;

	debug_print("camera manual");

	if (!gp_gtk_camera_init)
		CHECK_RESULT (camera_set ());

	CHECK_CAM_RESULT (gp_gtk_camera,
			  gp_camera_get_manual (gp_gtk_camera, &buf));

	frontend_message (gp_gtk_camera, buf.text);

}

void camera_show_about() {

	CameraText buf;

	debug_print("camera about");

	if (!gp_gtk_camera_init)
		CHECK_RESULT (camera_set());

	CHECK_CAM_RESULT (gp_gtk_camera,
			  gp_camera_get_about (gp_gtk_camera, &buf));

	frontend_message (gp_gtk_camera, buf.text);
}


/* Help operations */
/* ----------------------------------------------------------- */

void help_about() {


	debug_print("help about");

	frontend_message(NULL, _("gtKam was written by:\nScott Fritzinger <scottf@unr.edu>\n\ngtKam uses the gPhoto2 libraries.\nMore information is available at\nhttp://www.gphoto.net"));
}

void help_authors() {

	char buf[1024];
	CameraFile *f, *g;
	GtkWidget *window, *ok;
	const char *data;
	long int size;

	window = create_message_window_notebook();
	ok   = (GtkWidget*) lookup_widget(window, "close");

	debug_print("help authors");

	sprintf(buf, "%s/AUTHORS", DOCDIR);
	gp_file_new(&f);
	if (gp_file_open(f, buf)!=GP_OK) {
		frontend_message(NULL, _("Can't find gtKam AUTHORS file"));
		return;
	}
	gp_file_get_data_and_size (f, &data, &size);
	message_window_notebook_append(window, _("gtKam Authors"), (char*)data);

	sprintf(buf, "%s/AUTHORS", GPDOCDIR);
	gp_file_new(&g);
	if (gp_file_open(g, buf)!=GP_OK) {
		frontend_message(NULL, _("Can't find gPhoto2 AUTHORS file"));
		return;
	}
	gp_file_get_data_and_size (g, &data, &size);
	message_window_notebook_append(window, _("gPhoto2 Authors"), (char*)data);

	wait_for_hide(window, ok, NULL);

	gp_file_free(f);
	gp_file_free(g);

	gtk_widget_destroy(window);
}

void help_license() {

	char buf[1024];
	CameraFile *f, *g;
	GtkWidget *window, *ok;
	const char *data;
	long int size;

	window = create_message_window_notebook();
	ok   = (GtkWidget*) lookup_widget(window, "close");

	debug_print("help license");

	sprintf(buf, "%s/COPYING", DOCDIR);
	gp_file_new(&f);
	if (gp_file_open(f, buf)!=GP_OK) {
		frontend_message(NULL, _("Can't find gtKam COPYING file"));
		return;
	}
	gp_file_get_data_and_size (f, &data, &size);
	message_window_notebook_append(window, _("gtKam License"), (char*)data);

	sprintf(buf, "%s/COPYING", GPDOCDIR);
	gp_file_new(&g);
	if (gp_file_open(g, buf)!=GP_OK) {
		frontend_message(NULL, _("Can't find gPhoto2 COPYING file"));
		return;
	}
	gp_file_get_data_and_size (g, &data, &size);
	message_window_notebook_append(window, _("gPhoto2 License"), (char*)data);

	wait_for_hide(window, ok, NULL);

	gp_file_free(f);
	gp_file_free(g);
	
	gtk_widget_destroy(window);
}

void help_manual() {

	char buf[1024];
	CameraFile *f;
	GtkWidget *window, *ok;
	const char *data;
	long int size;

	debug_print("help manual");

	sprintf(buf, "%s/MANUAL", DOCDIR);
	gp_file_new(&f);
	if (gp_file_open(f, buf)!=GP_OK) {
		frontend_message(NULL, _("Can't find gtKam MANUAL file"));
		return;
	}
	window = create_message_window_notebook();
	ok = (GtkWidget*) lookup_widget(window, "close");
	gp_file_get_data_and_size (f, &data, &size);
	message_window_notebook_append(window, _("gtKam Manual"), (char*)data);

	wait_for_hide(window, ok, NULL);

	gp_file_free(f);
	
	gtk_widget_destroy(window);
}

/* Menu callbacks. just calls above mentioned functions */
void on_open_photo_activate (GtkMenuItem *menuitem, gpointer user_data) {
	open_photo();
}

void on_open_directory_activate (GtkMenuItem *menuitem, gpointer user_data) {
	open_directory();
}

void on_save_photo_activate (GtkMenuItem *menuitem, gpointer user_data) {
	save_selected_photos();
}


void on_html_gallery_activate (GtkMenuItem *menuitem, gpointer user_data) {
	export_gallery();
}

void on_close_activate (GtkMenuItem *menuitem, gpointer user_data) {
	close_photo();
}


void on_exit_activate (GtkMenuItem *menuitem, gpointer user_data) {
	main_quit(NULL, NULL);
}

void on_select_all_activate (GtkMenuItem *menuitem, gpointer user_data) {
	select_all();
}


void on_select_inverse_activate (GtkMenuItem *menuitem, gpointer user_data) {
	select_inverse();
}


void on_select_none_activate (GtkMenuItem *menuitem, gpointer user_data) {
	select_none();
}


void on_select_camera_activate (GtkMenuItem *menuitem, gpointer user_data) {
	camera_select();
}

void on_delete_photos_activate (GtkMenuItem *menuitem, gpointer user_data) {
	camera_delete_selected();
}


void on_delete_all_activate (GtkMenuItem *menuitem, gpointer user_data) {
	camera_delete_all();
}

void on_capture_activate (GtkMenuItem *menuitem, gpointer user_data) {

	GtkWidget *dialog;

	if (!gp_gtk_camera_init)
		CHECK_RESULT (camera_set ());

	dialog = gtkam_preview_new (gp_gtk_camera);
	gtk_widget_show (dialog);
}

void on_configure_activate (GtkMenuItem *menuitem, gpointer user_data) {

	GtkWidget *dialog;

	if (!gp_gtk_camera_init)
		CHECK_RESULT (camera_set ());

	dialog = gtkam_config_new (gp_gtk_camera);
	gtk_widget_show (dialog); 
}


void on_driver_information_activate (GtkMenuItem *menuitem, gpointer user_data) {
	camera_show_information();
}


void on_driver_manual_activate (GtkMenuItem *menuitem, gpointer user_data) {
	camera_show_manual();
}


void on_driver_about_activate (GtkMenuItem *menuitem, gpointer user_data) {
	camera_show_about();
}


void on_manual_activate (GtkMenuItem *menuitem, gpointer user_data) {
	help_manual();
}


void on_authors_activate (GtkMenuItem *menuitem, gpointer user_data) {
	help_authors();
}


void on_license_activate (GtkMenuItem *menuitem, gpointer user_data) {
	help_license();
}


void on_about_activate (GtkMenuItem *menuitem, gpointer user_data) {
	help_about();
}

static GtkWidget *debug_window = NULL;

void on_debug_window_destroy (GtkObject *object, gpointer *data) {

	debug_window = NULL;
}

void on_debug_activate (GtkMenuItem *menuitem, gpointer user_data) {

	if (!debug_window) {
		debug_window = gtkam_debug_new ();
		gtk_widget_show (debug_window);
		gtk_signal_connect (GTK_OBJECT (debug_window), "destroy",
				    GTK_SIGNAL_FUNC (on_debug_window_destroy),
				    NULL);
	}
	gtk_widget_grab_focus (debug_window);
}
