/* gtkam-tree-item-cam.c:
 *
 * Copyright (C) 2002 Lutz Müller <lutz@users.sourceforge.net>
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
#include "gtkam-tree-item-cam.h"

#include <string.h>

#include <gtk/gtklabel.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtksignal.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkcheckbutton.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gtkam-chooser.h"
#include "gtkam-close.h"
#include "gtkam-config.h"
#include "gtkam-error.h"
#include "gtkam-preview.h"
#include "gtkam-status.h"
#include "gtkam-tree.h"

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

struct _GtkamTreeItemCamPrivate
{
};

#define PARENT_TYPE GTKAM_TYPE_TREE_ITEM
static GtkamTreeItemClass *parent_class;

static void
gtkam_tree_item_cam_destroy (GtkObject *object)
{
	GtkamTreeItemCam *item = GTKAM_TREE_ITEM_CAM (object);

	item = NULL;

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_tree_item_cam_finalize (GtkObject *object)
{
	GtkamTreeItemCam *item = GTKAM_TREE_ITEM_CAM (object);

	g_free (item->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_tree_item_cam_set_camera (GtkamTreeItem *item, Camera *camera)
{
	CameraAbilities a;

	/* Set the label to the camera's model */
	memset (&a, 0, sizeof (a));
	gp_camera_get_abilities (camera, &a);
	gtk_label_set_text (GTK_LABEL (item->label), a.model);

	parent_class->set_camera (item, camera);
}

static void
on_camera_selected (GtkamChooser *chooser, Camera *camera, gboolean multi,
		    GtkamTreeItem *item)
{
	gtkam_tree_item_set_camera (item, camera);
	gtkam_tree_item_set_multi (item, multi);
}

static void
on_select_camera_activate (GtkMenuItem *i, GtkamTreeItem *item)
{
	GtkWidget *dialog;

	dialog = gtkam_chooser_new ();
	gtk_widget_show (dialog);
	gtk_signal_connect (GTK_OBJECT (dialog), "camera_selected",
			    GTK_SIGNAL_FUNC (on_camera_selected), item);
}

static void
on_preferences_activate (GtkMenuItem *i, GtkamTreeItem *item)
{
	GtkWidget *dialog;
	Camera *camera;
	gboolean multi;

	camera = gtkam_tree_item_get_camera (item);
	multi = gtkam_tree_item_get_multi (item);

	dialog = gtkam_config_new (camera, multi, NULL);
	if (!dialog)
		return;
	gtk_widget_show (dialog);
}

typedef enum _CameraTextType CameraTextType;
enum _CameraTextType {
	CAMERA_TEXT_SUMMARY,
	CAMERA_TEXT_MANUAL,
	CAMERA_TEXT_ABOUT
};

static void
on_text_activate (GtkMenuItem *i, GtkamTreeItem *item)
{
	GtkWidget *s, *dialog;
	CameraText text;
	int result;
	Camera *camera;
	gboolean multi;
	CameraTextType text_type;

	camera = gtkam_tree_item_get_camera (item);
	multi = gtkam_tree_item_get_multi (item);
	text_type = GPOINTER_TO_INT (
		gtk_object_get_data (GTK_OBJECT (i), "text_type"));

	switch (text_type) {
	case CAMERA_TEXT_SUMMARY:
		s = gtkam_status_new (
				_("Getting information about the camera..."));
		break;
	case CAMERA_TEXT_ABOUT:
		s = gtkam_status_new (
				_("Getting information about the driver..."));
		break;
	case CAMERA_TEXT_MANUAL:
	default:
		s = gtkam_status_new (_("Getting manual..."));
		break;
	}
	gtk_widget_show (s);
	gtk_signal_emit_by_name (GTK_OBJECT (item), "new_status", s);

	switch (text_type) {
	case CAMERA_TEXT_SUMMARY:
		result = gp_camera_get_summary (camera, &text,
				GTKAM_STATUS (s)->context->context);
		break;
	case CAMERA_TEXT_ABOUT:
		result = gp_camera_get_about (camera, &text,
				GTKAM_STATUS (s)->context->context);
		break;
	default:
	case CAMERA_TEXT_MANUAL:
		result = gp_camera_get_manual (camera, &text,
				GTKAM_STATUS (s)->context->context);
		break;
	}
	if (multi)
		gp_camera_exit (camera, NULL);
	switch (result) {
	case GP_OK:
		dialog = gtkam_close_new (text.text, NULL);
		gtk_widget_show (dialog);
		break;
	case GP_ERROR_CANCEL:
		break;
	default:
		dialog = gtkam_error_new (result, GTKAM_STATUS (s)->context,
			NULL, _("Could not retrieve information."));
		gtk_widget_show (dialog);
	}
	gtk_object_destroy (GTK_OBJECT (s));
}

static void
on_captured (GtkamPreview *preview, const gchar *folder, const gchar *name,
	     GtkamTreeItem *item)
{
	gtkam_tree_item_set_online (item, FALSE);
	gtkam_tree_item_set_online (item, TRUE);
}

static void
on_capture_activate (GtkMenuItem *i, GtkamTreeItem *item)
{
	CameraAbilities a;
	Camera *camera;
	gboolean multi;
	GtkWidget *s, *dialog;
	int result;
	CameraFilePath path;

	camera = gtkam_tree_item_get_camera (item);
	multi = gtkam_tree_item_get_multi (item);

	/* Let's first check if the camera supports previews */
	gp_camera_get_abilities (camera, &a);
	if (a.operations & GP_OPERATION_CAPTURE_PREVIEW) {
		dialog = gtkam_preview_new (camera, multi);
		gtk_widget_show (dialog);
		gtk_signal_connect (GTK_OBJECT (dialog), "captured",
				    GTK_SIGNAL_FUNC (on_captured), item);
		return;
	}

	/* The camera doesn't support previews. Capture an image. */
	s = gtkam_status_new (_("Capturing image..."));
	gtk_widget_show (s);
	gtk_signal_emit_by_name (GTK_OBJECT (item), "new_status", s);
	result = gp_camera_capture (camera, GP_CAPTURE_IMAGE, &path,
				GTKAM_STATUS (s)->context->context);
	if (multi)
		gp_camera_exit (camera, NULL);
	switch (result) {
	case GP_OK:
		gtkam_tree_item_set_online (item, FALSE);
		gtkam_tree_item_set_online (item, TRUE);
		break;
	case GP_ERROR_CANCEL:
		break;
	default:
		dialog = gtkam_error_new (result, GTKAM_STATUS (s)->context,
				NULL, _("Could not capture."));
		gtk_widget_show (dialog);
		break;
	}
	gtk_object_destroy (GTK_OBJECT (s));
}

static void
on_online_toggled (GtkToggleButton *toggle, GtkamTreeItem *item)
{
	GtkWidget *menu;

	menu = gtk_widget_get_ancestor (GTK_WIDGET (toggle), GTK_TYPE_MENU);
	gtkam_tree_item_set_online (item, toggle->active);
	gtk_menu_popdown (GTK_MENU (menu));
}

static void
on_remove_camera_activate (GtkMenuItem *i, GtkamTreeItem *item)
{
	GtkWidget *tree;

	tree = gtk_widget_get_ancestor (GTK_WIDGET (item), GTKAM_TYPE_TREE);
	gtk_object_destroy (GTK_OBJECT (item));
	gtkam_tree_save (GTKAM_TREE (tree));
}

static guint
gtkam_tree_item_cam_add_menu_items (GtkamTreeItem *item, GtkWidget *menu)
{
	GtkWidget *i, *t;
	Camera *camera;
	CameraAbilities a;

	camera = gtkam_tree_item_get_camera (item);
	memset (&a, 0, sizeof (CameraAbilities));
	if (camera)
		gp_camera_get_abilities (camera, &a);

	/* Capture */
	i = gtk_menu_item_new_with_label (_("Capture"));
	gtk_widget_show (i);
	gtk_menu_append (GTK_MENU (menu), i);
	gtk_signal_connect (GTK_OBJECT (i), "activate",
			    GTK_SIGNAL_FUNC (on_capture_activate), item);
	gtk_widget_set_sensitive (i, FALSE);
	if (camera && (a.operations & (GP_OPERATION_CAPTURE_PREVIEW |
				       GP_OPERATION_CAPTURE_IMAGE)))
		gtk_widget_set_sensitive (i, TRUE);

	/* Preferences */
	i = gtk_menu_item_new_with_label (_("Preferences"));
	gtk_widget_show (i);
	gtk_menu_append (GTK_MENU (menu), i);
	gtk_signal_connect (GTK_OBJECT (i), "activate",
			    GTK_SIGNAL_FUNC (on_preferences_activate), item);
	gtk_widget_set_sensitive (i, FALSE);
	if (camera && (a.operations & GP_OPERATION_CONFIG))
		gtk_widget_set_sensitive (i, TRUE);

	/* Summary */
	i = gtk_menu_item_new_with_label (_("Summary"));
	gtk_widget_show (i);
	gtk_menu_append (GTK_MENU (menu), i);
	gtk_object_set_data (GTK_OBJECT (i), "text_type",
			     GINT_TO_POINTER (CAMERA_TEXT_SUMMARY));
	gtk_signal_connect (GTK_OBJECT (i), "activate",
			    GTK_SIGNAL_FUNC (on_text_activate), item);
	if (!camera)
		gtk_widget_set_sensitive (i, FALSE);

	/* Manual */
	i = gtk_menu_item_new_with_label (_("Manual"));
	gtk_widget_show (i);
	gtk_menu_append (GTK_MENU (menu), i);
	gtk_object_set_data (GTK_OBJECT (i), "text_type",
			     GINT_TO_POINTER (CAMERA_TEXT_MANUAL));
	gtk_signal_connect (GTK_OBJECT (i), "activate",
			    GTK_SIGNAL_FUNC (on_text_activate), item);
	if (!camera)
		gtk_widget_set_sensitive (i, FALSE);

	/* About */
	i = gtk_menu_item_new_with_label (_("About the driver"));
	gtk_widget_show (i);
	gtk_menu_append (GTK_MENU (menu), i);
	gtk_object_set_data (GTK_OBJECT (i), "text_type",
			     GINT_TO_POINTER (CAMERA_TEXT_ABOUT));
	gtk_signal_connect (GTK_OBJECT (i), "activate",
			    GTK_SIGNAL_FUNC (on_text_activate), item);
	if (!camera)
		gtk_widget_set_sensitive (i, FALSE);

	/* Separator */
	i = gtk_menu_item_new ();
	gtk_widget_show (i);
	gtk_menu_append (GTK_MENU (menu), i);

	/* Select camera */
	i = gtk_menu_item_new_with_label (_("Select Camera"));
	gtk_widget_show (i);
	gtk_menu_append (GTK_MENU (menu), i);
	gtk_signal_connect (GTK_OBJECT (i), "activate",
			    GTK_SIGNAL_FUNC (on_select_camera_activate), item);

	/* On-/Offline */
	i = gtk_menu_item_new ();
	gtk_widget_show (i);
	gtk_menu_append (GTK_MENU (menu), i);
	t = gtk_check_button_new_with_label (_("Online"));
	gtk_widget_show (t);
	gtk_container_add (GTK_CONTAINER (i), t);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (t),
				      gtkam_tree_item_get_online (item));
	gtk_signal_connect (GTK_OBJECT (t), "toggled",
			    GTK_SIGNAL_FUNC (on_online_toggled), item);

	/* Remove camera */
	i = gtk_menu_item_new_with_label (_("Remove Camera"));
	gtk_widget_show (i);
	gtk_menu_append (GTK_MENU (menu), i);
	gtk_signal_connect (GTK_OBJECT (i), "activate",
			    GTK_SIGNAL_FUNC (on_remove_camera_activate), item);

	return (8);
}

static void
gtkam_tree_item_cam_class_init (GtkamTreeItemCamClass *klass)
{
	GtkObjectClass *object_class;
	GtkamTreeItemClass *item_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_tree_item_cam_destroy;
	object_class->finalize = gtkam_tree_item_cam_finalize;

	item_class = GTKAM_TREE_ITEM_CLASS (klass);
	item_class->set_camera     = gtkam_tree_item_cam_set_camera;
	item_class->add_menu_items = gtkam_tree_item_cam_add_menu_items;

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_tree_item_cam_init (GtkamTreeItemCam *item)
{
	item->priv = g_new0 (GtkamTreeItemCamPrivate, 1);
}

GtkType
gtkam_tree_item_cam_get_type (void)
{
	static GtkType cam_type = 0;

	if (!cam_type) {
		static const GtkTypeInfo cam_info = {
			"GtkamTreeItemCam",
			sizeof (GtkamTreeItemCam),
			sizeof (GtkamTreeItemCamClass),
			(GtkClassInitFunc)  gtkam_tree_item_cam_class_init,
			(GtkObjectInitFunc) gtkam_tree_item_cam_init,
			NULL, NULL, NULL};
		cam_type = gtk_type_unique (PARENT_TYPE, &cam_info);
	}

	return (cam_type);
}

GtkWidget *
gtkam_tree_item_cam_new (void)
{
	GtkamTreeItemCam *item;
	GdkPixbuf *pixbuf;

	item = gtk_type_new (GTKAM_TYPE_TREE_ITEM_CAM);

	pixbuf = gdk_pixbuf_new_from_file (IMAGE_DIR "/gtkam-camera.png");
	gtkam_tree_item_construct (GTKAM_TREE_ITEM (item), pixbuf);
	gdk_pixbuf_unref (pixbuf);

	return (GTK_WIDGET (item));
}
