/* gtkam-chooser.c
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
#include "gtkam-chooser.h"

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtktable.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkcombo.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtktooltips.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <gphoto2/gphoto2-abilities-list.h>
#include <gphoto2/gphoto2-setting.h>

#include "support.h"
#include "gtkam-error.h"
#include "gtkam-close.h"
#include "gtkam-port.h"
#include "gtkam-status.h"

struct _GtkamChooserPrivate
{
	CameraAbilitiesList *al;
	GPPortInfoList *il;

	GtkWidget *label_speed, *check_multi, *button_add, *table;
	GtkEntry *entry_model, *entry_port, *entry_speed;
	GtkCombo *combo_model, *combo_port, *combo_speed;

	GtkWidget *ok;

	GtkTooltips *tooltips;

	gboolean needs_update;
};

#define PARENT_TYPE GTK_TYPE_DIALOG
static GtkDialogClass *parent_class;

enum {
	CAMERA_SELECTED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

#define gtk_marshal_NONE__POINTER_BOOL gtk_marshal_NONE__POINTER_INT

static void
gtkam_chooser_destroy (GtkObject *object)
{
	GtkamChooser *chooser = GTKAM_CHOOSER (object);

	if (chooser->priv->al) {
		gp_abilities_list_free (chooser->priv->al);
		chooser->priv->al = NULL;
	}

	if (chooser->priv->il) {
		gp_port_info_list_free (chooser->priv->il);
		chooser->priv->il = NULL;
	}

	if (chooser->priv->tooltips) {
		gtk_object_unref (GTK_OBJECT (chooser->priv->tooltips));
		chooser->priv->tooltips = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_chooser_finalize (GtkObject *object)
{
	GtkamChooser *chooser = GTKAM_CHOOSER (object);

	g_free (chooser->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_chooser_class_init (GtkamChooserClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_chooser_destroy;
	object_class->finalize = gtkam_chooser_finalize;

	signals[CAMERA_SELECTED] = gtk_signal_new ("camera_selected",
		GTK_RUN_LAST, object_class->type,
		GTK_SIGNAL_OFFSET (GtkamChooserClass, camera_selected),
		gtk_marshal_NONE__POINTER_BOOL, GTK_TYPE_NONE, 2,
		GTK_TYPE_POINTER, GTK_TYPE_BOOL);
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_chooser_init (GtkamChooser *chooser)
{
	chooser->priv = g_new0 (GtkamChooserPrivate, 1);
}

GtkType
gtkam_chooser_get_type (void)
{
	static GtkType chooser_type = 0;

	if (!chooser_type) {
		static const GtkTypeInfo chooser_info = {
			"GtkamChooser",
			sizeof (GtkamChooser),
			sizeof (GtkamChooserClass),
			(GtkClassInitFunc)  gtkam_chooser_class_init,
			(GtkObjectInitFunc) gtkam_chooser_init,
			NULL, NULL, NULL};
		chooser_type = gtk_type_unique (PARENT_TYPE, &chooser_info);
	}

	return (chooser_type);
}

static Camera *
gtkam_chooser_get_camera (GtkamChooser *chooser)
{
	GtkWidget *dialog, *status;
	GPPortInfo info;
	Camera *camera;
	CameraAbilities abilities;
	const gchar *model, *port, *speed;
	gchar *port_path, *right, *tmp;
	int m, p, r;
	gboolean multi;

	model = gtk_entry_get_text (chooser->priv->entry_model);
	port  = gtk_entry_get_text (chooser->priv->entry_port);
	speed = gtk_entry_get_text (chooser->priv->entry_speed);
	multi = GTK_TOGGLE_BUTTON (chooser->priv->check_multi)->active;

	if (!port || !*port)
		port_path = g_strdup (_("None"));
	else {
		port_path = g_strdup (port);
		right = strrchr (port_path, ')');
		*right = '\0';
		tmp = g_strdup (strrchr (port_path, '(') + 1);
		g_free (port_path);
		port_path = tmp;
	}

	gp_camera_new (&camera);

	/* Model? */
	m = gp_abilities_list_lookup_model (chooser->priv->al, model);
	gp_abilities_list_get_abilities (chooser->priv->al, m, &abilities);
	gp_camera_set_abilities (camera, abilities);

	/* Port? */
	if (strcmp (port_path, _("None"))) {
		p = gp_port_info_list_lookup_path (chooser->priv->il,
						   port_path);
		gp_port_info_list_get_info (chooser->priv->il, p, &info);
		gp_camera_set_port_info (camera, info);
	}

	/* Speed? */
	if (strcmp (speed, _("Best")))
		gp_camera_set_port_speed (camera, atoi (speed));

	/*
	 * Initialize the camera to check if it is really there. Exit
	 * afterwards because other applications could need the camera, too.
	 */
	status = gtkam_status_new (_("Initializing camera..."));
	gtk_widget_show (status);
	gtk_box_pack_end (GTK_BOX (GTK_DIALOG (chooser)->vbox), status,
			  FALSE, FALSE, 0);
	while (gtk_events_pending ())
		gtk_main_iteration ();
	r = gp_camera_init (camera, GTKAM_STATUS (status)->context->context);
	if (multi)
		gp_camera_exit (camera, NULL);
	switch (r) {
	case GP_OK:
		break;
	case GP_ERROR_CANCEL:
		g_free (port_path);
		break;
	default:
		g_free (port_path);
		dialog = gtkam_error_new (r, GTKAM_STATUS (status)->context,
			NULL, _("Could not initialize camera."));
		gtk_window_set_transient_for (GTK_WINDOW (dialog),
					      GTK_WINDOW (chooser));
		gtk_widget_show (dialog);
		gp_camera_unref (camera);
		gtk_object_destroy (GTK_OBJECT (status));
		return (NULL);
	}
	gtk_object_destroy (GTK_OBJECT (status));

	return (camera);
}

static void
on_apply_clicked (GtkButton *button, GtkamChooser *chooser)
{
	Camera *camera;

	if (!chooser->priv->needs_update)
		return;

	camera = gtkam_chooser_get_camera (chooser);
	if (camera) {
		gtk_signal_emit (GTK_OBJECT (chooser),
			signals[CAMERA_SELECTED], camera,
			GTK_TOGGLE_BUTTON (chooser->priv->check_multi)->active);
		gp_camera_unref (camera);

		chooser->priv->needs_update = FALSE;
		gtk_widget_set_sensitive (chooser->apply_button, FALSE);
	}
}

static void
on_cancel_clicked (GtkButton *button, GtkamChooser *chooser)
{
	gtk_object_destroy (GTK_OBJECT (chooser));
}


static void
on_ok_clicked (GtkButton *button, GtkamChooser *chooser)
{
	Camera *camera;

	if (chooser->priv->needs_update) {
		while (gtk_events_pending ())
			gtk_main_iteration ();
		camera = gtkam_chooser_get_camera (chooser);
		if (camera) {
			gtk_signal_emit (GTK_OBJECT (chooser),
				signals[CAMERA_SELECTED], camera,
				GTK_TOGGLE_BUTTON (chooser->priv->check_multi)->active);
			gp_camera_unref (camera);
			gtk_object_destroy (GTK_OBJECT (chooser));
		}
	} else
		gtk_object_destroy (GTK_OBJECT (chooser));
}

static void
on_more_options_toggled (GtkToggleButton *toggle, GtkamChooser *chooser)
{
	if (toggle->active) {
		gtk_widget_show (chooser->priv->check_multi);
		gtk_widget_show (chooser->priv->label_speed);
		gtk_widget_show (GTK_WIDGET (chooser->priv->combo_speed));
		gtk_widget_show (chooser->priv->button_add);
		gtk_widget_ref (GTK_WIDGET (chooser->priv->combo_port));
		gtk_container_remove (GTK_CONTAINER (chooser->priv->table),
				      GTK_WIDGET (chooser->priv->combo_port));
		gtk_table_attach_defaults (GTK_TABLE (chooser->priv->table),
			GTK_WIDGET (chooser->priv->combo_port), 1, 2, 1, 2);
		gtk_widget_unref (GTK_WIDGET (chooser->priv->combo_port));
	} else {
		gtk_widget_hide (chooser->priv->check_multi);
		gtk_widget_hide (chooser->priv->label_speed);
		gtk_widget_hide (GTK_WIDGET (chooser->priv->combo_speed));
		gtk_widget_hide (chooser->priv->button_add);
		gtk_widget_ref (GTK_WIDGET (chooser->priv->combo_port));
		gtk_container_remove (GTK_CONTAINER (chooser->priv->table),
				      GTK_WIDGET (chooser->priv->combo_port));
		gtk_table_attach_defaults (GTK_TABLE (chooser->priv->table),
			GTK_WIDGET (chooser->priv->combo_port), 1, 3, 1, 2);
		gtk_widget_unref (GTK_WIDGET (chooser->priv->combo_port));
	}
}

static void
gtkam_chooser_update_for_model (GtkamChooser *chooser)
{
	const gchar *model;
	int m, result, i;
	CameraAbilities a;
	GtkWidget *dialog;
	GList *list;

	/* Get abilities of selected model */
	model = gtk_entry_get_text (chooser->priv->entry_model);
	m = gp_abilities_list_lookup_model (chooser->priv->al, model);
	result = gp_abilities_list_get_abilities (chooser->priv->al, m, &a);
	if (result < 0) {
		dialog = gtkam_error_new (result, NULL, GTK_WIDGET (chooser),
			_("Could not get abilities of model '%s'."), model);
		gtk_widget_show (dialog);
		return;
	}

	/* Update port combo */
	gtkam_chooser_set_port_mask (chooser, a.port);

	/* Update speed combo */
	list = g_list_append (NULL, _("Best"));
	for (i = 0; a.speed[i]; i++)
		list = g_list_append (list, g_strdup_printf ("%i", a.speed[i]));
	gtk_combo_set_popdown_strings (chooser->priv->combo_speed, list);
	gtk_widget_set_sensitive (GTK_WIDGET (chooser->priv->combo_speed),
				  TRUE);
}

static void
on_model_changed (GtkEntry *entry, GtkamChooser *chooser)
{
	gtkam_chooser_update_for_model (chooser);

	chooser->priv->needs_update = TRUE;
	gtk_widget_set_sensitive (chooser->apply_button, TRUE);
}

static void
on_port_changed (GtkEntry *entry, GtkamChooser *chooser)
{
	chooser->priv->needs_update = TRUE;
	gtk_widget_set_sensitive (chooser->apply_button, TRUE);
}

static void
on_speed_changed (GtkEntry *entry, GtkamChooser *chooser)
{
	chooser->priv->needs_update = TRUE;
	gtk_widget_set_sensitive (chooser->apply_button, TRUE);
}

static void
on_detect_clicked (GtkButton *button, GtkamChooser *chooser)
{
	GtkWidget *dialog, *status;
	CameraList list;
	int result;
	const char *name;

	status = gtkam_status_new (_("Detecting cameras..."));
	gtk_widget_show (status);
	gtk_box_pack_end (GTK_BOX (GTK_DIALOG (chooser)->vbox), status,
			  FALSE, FALSE, 0);
	result = gp_abilities_list_detect (chooser->priv->al,
		chooser->priv->il, &list,
		GTKAM_STATUS (status)->context->context);
	switch (result) {
	case GP_OK:
		if (!gp_list_count (&list)) {
			dialog = gtkam_close_new (_("No cameras detected."),
						  GTK_WIDGET (chooser));
			gtk_widget_show (dialog);
		} else {
			//FIXME: Let user choose from the list
			gp_list_get_name (&list, 0, &name);
			gtk_entry_set_text (chooser->priv->entry_model, name);
			gtk_entry_set_text (chooser->priv->entry_port,
					"Universal Serial Bus (usb:)");
		}
		break;
	case GP_ERROR_CANCEL:
		break;
	default:
		dialog = gtkam_error_new (result,
			GTKAM_STATUS (status)->context, GTK_WIDGET (chooser),
			_("Could not detect any cameras."));
		gtk_widget_show (dialog);
		break;
	}
	gtk_object_destroy (GTK_OBJECT (status));
}

static void
gtkam_chooser_set_model_list (GtkamChooser *chooser, GList *list)
{
	gchar *model;
	guint i;

	g_return_if_fail (GTKAM_IS_CHOOSER (chooser));

        if (list) {

		/* Remember the old entry */
		model = g_strdup (gtk_entry_get_text (
					chooser->priv->entry_model));
		gtk_combo_set_popdown_strings (chooser->priv->combo_model,
					       list);
		for (i = 0; i < g_list_length (list); i++)
			if (!strcmp (g_list_nth_data (list, i), model))
				gtk_entry_set_text (chooser->priv->entry_model,
						    model);
		g_free (model);
		gtk_widget_set_sensitive (chooser->priv->ok, TRUE);
	} else {
		gtk_widget_set_sensitive (
			GTK_WIDGET (chooser->priv->combo_model), FALSE);
		gtk_widget_set_sensitive (chooser->priv->ok, FALSE);
	}
}

static void
gtkam_chooser_set_port_list (GtkamChooser *chooser, GList *list)
{
	gchar *port;
	guint i;

	g_return_if_fail (GTKAM_IS_CHOOSER (chooser));

        if (list) {
		
		/* Remember the old entry */
		port = g_strdup (gtk_entry_get_text (
						chooser->priv->entry_port));
		gtk_combo_set_popdown_strings (chooser->priv->combo_port, list);
		for (i = 0; i < g_list_length (list); i++)
			if (!strcmp (g_list_nth_data (list, i), port))
				gtk_entry_set_text (chooser->priv->entry_port,
						    port);
		g_free (port);
		gtk_widget_set_sensitive (
				GTK_WIDGET (chooser->priv->combo_port), TRUE);
		gtk_widget_set_sensitive (
				GTK_WIDGET (chooser->priv->button_add), TRUE);
	} else {
		gtk_widget_set_sensitive (
				GTK_WIDGET (chooser->priv->combo_port), FALSE);
		gtk_widget_set_sensitive (
				GTK_WIDGET (chooser->priv->button_add), FALSE);
	}
}

static void
on_multi_toggled (GtkToggleButton *toggle, GtkamChooser *chooser)
{
	chooser->priv->needs_update = TRUE;
	gtk_widget_set_sensitive (chooser->apply_button, TRUE);
}

static void
on_port_added (GtkamPort *port, const gchar *path, GtkamChooser *chooser)
{
	int index;
	GPPortInfo info;
	gchar *name;

	index = gp_port_info_list_lookup_path (chooser->priv->il, path);
	if (index < 0) {
		g_warning ("Could not look up path?!?");
		return;
	}
	gp_port_info_list_get_info (chooser->priv->il, index, &info);
	name = g_strdup_printf ("%s (%s)", info.name, info.path);
	gtk_entry_set_text (chooser->priv->entry_port, name);
	g_free (name);

	gtkam_chooser_update_for_model (chooser);

	chooser->priv->needs_update = TRUE;
	gtk_widget_set_sensitive (chooser->apply_button, TRUE);
}

static void
on_add_clicked (GtkButton *button, GtkamChooser *chooser)
{
	GtkWidget *port;

	port = gtkam_port_new (GTK_WIDGET (chooser));
	gtk_widget_show (port);
	gtk_signal_connect (GTK_OBJECT (port), "port_added",
			    GTK_SIGNAL_FUNC (on_port_added), chooser);
}

GtkWidget *
gtkam_chooser_new (void)
{
	GtkamChooser *chooser;
	GtkWidget *table, *label, *button, *combo, *hbox, *vbox, *check;
	GtkWidget *image;
	GdkPixmap *pixmap;
	GdkBitmap *bitmap;
	GdkPixbuf *pixbuf;

	chooser = gtk_type_new (GTKAM_TYPE_CHOOSER);
	chooser->priv->tooltips = gtk_tooltips_new ();

	gp_abilities_list_new (&(chooser->priv->al));
	gp_abilities_list_load (chooser->priv->al, NULL);

	gp_port_info_list_new (&(chooser->priv->il));
	gp_port_info_list_load (chooser->priv->il);

	gtk_window_set_title (GTK_WINDOW (chooser), _("Select Camera"));
	gtk_container_set_border_width (GTK_CONTAINER (chooser), 5);
	gtk_window_set_policy (GTK_WINDOW (chooser), TRUE, TRUE, TRUE);
	gtk_signal_connect (GTK_OBJECT (chooser), "delete_event",
			    GTK_SIGNAL_FUNC (gtk_object_destroy), NULL);

	hbox = gtk_hbox_new (FALSE, 10);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (chooser)->vbox), hbox,
			    TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);

	pixbuf = gdk_pixbuf_new_from_file (IMAGE_DIR "/gtkam-camera.png");
	if (!pixbuf)
		g_warning ("Could not load " IMAGE_DIR "/gtkam-camera.png");
	else {
		gdk_pixbuf_render_pixmap_and_mask (pixbuf, &pixmap, &bitmap, 127);
		gdk_pixbuf_unref (pixbuf);
		image = gtk_pixmap_new (pixmap, bitmap);
		if (pixmap)
			gdk_pixmap_unref (pixmap);
		if (bitmap)
			gdk_bitmap_unref (bitmap);
		gtk_widget_show (image);
		gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	}

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

	table = gtk_table_new (3, 3, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table), 5);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
	chooser->priv->table = table;

	label = gtk_label_new (_("Model:"));
	gtk_widget_show (label);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);

	combo = gtk_combo_new ();
	gtk_widget_show (combo);
	gtk_table_attach_defaults (GTK_TABLE (table), combo, 1, 2, 0, 1);
	chooser->priv->combo_model = GTK_COMBO (combo);
	chooser->priv->entry_model = GTK_ENTRY (GTK_COMBO (combo)->entry);
	gtk_entry_set_editable (chooser->priv->entry_model, FALSE);

	button = gtk_button_new_with_label (_("Detect"));
	gtk_widget_show (button);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 2, 3, 0, 1);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_detect_clicked), chooser);
	gtk_tooltips_set_tip (chooser->priv->tooltips, button,
			      _("Detect USB camera"), NULL);

	label = gtk_label_new (_("Port:"));
	gtk_widget_show (label);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);

	combo = gtk_combo_new ();
	gtk_widget_show (combo);
	gtk_table_attach_defaults (GTK_TABLE (table), combo, 1, 3, 1, 2);
	gtk_widget_set_sensitive (combo, FALSE);
	chooser->priv->entry_port = GTK_ENTRY (GTK_COMBO (combo)->entry);
	chooser->priv->combo_port = GTK_COMBO (combo);
	gtk_entry_set_editable (chooser->priv->entry_port, FALSE);

	button = gtk_button_new_with_label (_("Add"));
	gtk_widget_set_sensitive (button, FALSE);
	gtk_table_attach_defaults (GTK_TABLE (table), button, 2, 3, 1, 2);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_add_clicked), chooser);
	chooser->priv->button_add = button;

	label = gtk_label_new (_("Speed:"));
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
	chooser->priv->label_speed = label;

	combo = gtk_combo_new ();
	gtk_table_attach_defaults (GTK_TABLE (table), combo, 1, 3, 2, 3);
	gtk_widget_set_sensitive (combo, FALSE);
	chooser->priv->entry_speed = GTK_ENTRY (GTK_COMBO (combo)->entry);
	gtk_entry_set_text (chooser->priv->entry_speed, _("Best"));
	chooser->priv->combo_speed = GTK_COMBO (combo);
	gtk_entry_set_editable (chooser->priv->entry_speed, FALSE);

	check = gtk_check_button_new_with_label (_("Allow multiple frontends"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), FALSE);
	gtk_table_attach_defaults (GTK_TABLE (table), check, 0, 2, 3, 4);
	gtk_signal_connect (GTK_OBJECT (check), "toggled",
			    GTK_SIGNAL_FUNC (on_multi_toggled), chooser);
	chooser->priv->check_multi = check;

	button = gtk_toggle_button_new_with_label (_("Enhanced"));
	gtk_widget_show (button);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (chooser)->action_area),
			   button);
	gtk_signal_connect (GTK_OBJECT (button), "toggled",
			    GTK_SIGNAL_FUNC (on_more_options_toggled), chooser);

	button = gtk_button_new_with_label (_("Ok"));
	gtk_widget_show (button);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (chooser)->action_area),
			   button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_ok_clicked), chooser);
	gtk_widget_grab_focus (button);
	chooser->priv->ok = button;

	chooser->apply_button = gtk_button_new_with_label (_("Apply"));
	gtk_widget_show (chooser->apply_button);
	gtk_widget_set_sensitive (chooser->apply_button, FALSE);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (chooser)->action_area),
			   chooser->apply_button);
	gtk_signal_connect (GTK_OBJECT (chooser->apply_button), "clicked",
			    GTK_SIGNAL_FUNC (on_apply_clicked), chooser);

	button = gtk_button_new_with_label (_("Cancel"));
	gtk_widget_show (button);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (chooser)->action_area),
			   button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_cancel_clicked), chooser);

	/* Fill the model combo with all models */
	gtkam_chooser_set_camera_mask (chooser, GP_OPERATION_NONE);

	gtk_signal_connect (GTK_OBJECT (chooser->priv->entry_model), "changed",
			    GTK_SIGNAL_FUNC (on_model_changed), chooser);
	gtk_signal_connect (GTK_OBJECT (chooser->priv->entry_port), "changed",
			    GTK_SIGNAL_FUNC (on_port_changed), chooser);
	gtk_signal_connect (GTK_OBJECT (chooser->priv->entry_speed), "changed",
			    GTK_SIGNAL_FUNC (on_speed_changed), chooser);

	gtkam_chooser_update_for_model (chooser);

	chooser->priv->needs_update = TRUE;
	gtk_widget_set_sensitive (chooser->apply_button, TRUE);

	return (GTK_WIDGET (chooser));
}

void
gtkam_chooser_set_camera_mask (GtkamChooser *chooser,
			       CameraOperation operations)
{
	GtkWidget *dialog;
	CameraAbilities a;
	GList *list = NULL;
	guint i;
	int n;

	g_return_if_fail (GTKAM_IS_CHOOSER (chooser));

	n = gp_abilities_list_count (chooser->priv->al);
	if (n < 0) {
		dialog = gtkam_error_new (n, NULL, GTK_WIDGET (chooser),
			_("Could not get number of "
			"supported camera models"));
		gtk_widget_show (dialog);
		return;
	}

	/* Search for models that fulfil the criteria */
	for (i = 0; i < n; i++) {
		gp_abilities_list_get_abilities (chooser->priv->al, i, &a);
		if (!operations || (a.operations & operations))
			list = g_list_append (list, g_strdup (a.model));
	}

	gtkam_chooser_set_model_list (chooser, list);
}

void
gtkam_chooser_set_port_mask (GtkamChooser *chooser, GPPortType types)
{
	GtkWidget *dialog;
	GPPortInfo info;
	int n;
	guint i;
	GList *list = NULL;

	g_return_if_fail (GTKAM_IS_CHOOSER (chooser));

	n = gp_port_info_list_count (chooser->priv->il);
	if (n < 0) {
		dialog = gtkam_error_new (n, NULL, GTK_WIDGET (chooser),
			_("Could not get number of ports."));
		gtk_widget_show (dialog);
		return;
	}

	/* Search for ports that fulfil the criteria */
	for (i = 0; i < n; i++) {
		gp_port_info_list_get_info (chooser->priv->il, i, &info);
		if (info.type & types)
			list = g_list_append (list, g_strdup_printf ("%s (%s)",
					      info.name, info.path));
	}

	gtkam_chooser_set_port_list (chooser, list);
}

void
gtkam_chooser_set_camera (GtkamChooser *chooser, Camera *camera)
{
	CameraAbilities a;
	GPPortInfo info;
	gchar *full_info;

	g_return_if_fail (GTKAM_IS_CHOOSER (chooser));
	g_return_if_fail (camera != NULL);

	gp_camera_get_abilities (camera, &a);
	gp_camera_get_port_info (camera, &info);

	full_info = g_strdup_printf ("%s (%s)", info.name, info.path);
	gtk_entry_set_text (chooser->priv->entry_port, full_info);
	g_free (full_info);

	gtk_entry_set_text (chooser->priv->entry_model, a.model);

	chooser->priv->needs_update = FALSE;
	gtk_widget_set_sensitive (chooser->apply_button, FALSE);
}

void
gtkam_chooser_set_multi (GtkamChooser *chooser, gboolean multi)
{
	g_return_if_fail (GTKAM_IS_CHOOSER (chooser));

	gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON (chooser->priv->check_multi), multi);
}
