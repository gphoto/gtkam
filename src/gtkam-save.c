/* gtkam-save.c
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
#include "gtkam-save.h"

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
#include <string.h>

#include <gtk/gtktogglebutton.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkentry.h>

#include <gphoto2/gphoto2-setting.h>

#include "util.h"
#include "gtkam-error.h"
#include "gtkam-close.h"

struct _GtkamSavePrivate
{
	GtkWidget *file_list;

	Camera *camera;
	gboolean multi;
	gchar *path;
	GSList *filenames;

	GtkToggleButton *toggle_preview, *toggle_normal, *toggle_raw,
			*toggle_audio;
	GtkToggleButton *toggle_filename_camera;
	GtkEntry *program, *prefix_entry;

	gboolean quiet;
};

#define PARENT_TYPE GTK_TYPE_FILE_SELECTION
static GtkFileSelection *parent_class;

static void
gtkam_save_destroy (GtkObject *object)
{
	GtkamSave *save = GTKAM_SAVE (object);
	guint i;

	if (save->priv->camera) {
		gp_camera_unref (save->priv->camera);
		save->priv->camera = NULL;
	}

	if (save->priv->path) {
		g_free (save->priv->path);
		save->priv->path = NULL;
	}

	if (save->priv->filenames) {
		for (i = 0; i < g_slist_length (save->priv->filenames); i++)
			g_free (g_slist_nth_data (save->priv->filenames, i));
		g_slist_free (save->priv->filenames);
		save->priv->filenames = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_save_finalize (GtkObject *object)
{
	GtkamSave *save = GTKAM_SAVE (object);

	g_free (save->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_save_class_init (GtkamSaveClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_save_destroy;
	object_class->finalize = gtkam_save_finalize;

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_save_init (GtkamSave *save)
{
	save->priv = g_new0 (GtkamSavePrivate, 1);
}

GtkType
gtkam_save_get_type (void)
{
	static GtkType save_type = 0;

	if (!save_type) {
		static const GtkTypeInfo save_info = {
			"GtkamSave",
			sizeof (GtkamSave),
			sizeof (GtkamSaveClass),
			(GtkClassInitFunc)  gtkam_save_class_init,
			(GtkObjectInitFunc) gtkam_save_init,
			NULL, NULL, NULL};
			save_type = gtk_type_unique (PARENT_TYPE, &save_info);
	}

	return (save_type);
}

static void
on_cancel_clicked (GtkButton *button, GtkamSave *save)
{
	gtk_widget_destroy (GTK_WIDGET (save));
}

static void
on_filename_camera_toggled (GtkToggleButton *toggle, GtkamSave *save)
{
	if (save->priv->prefix_entry)
		gtk_widget_set_sensitive (GTK_WIDGET (save->priv->prefix_entry),
					  !toggle->active);
	else {
		gtk_widget_set_sensitive (save->priv->file_list,
					  !toggle->active);
		gtk_widget_set_sensitive (
				GTK_FILE_SELECTION (save)->selection_entry,
				!toggle->active);
	}
}

static gchar *
create_full_filename (const gchar *filename, CameraFileType type)
{
	gchar *full_filename;

	switch (type) {
	case GP_FILE_TYPE_PREVIEW:
		full_filename = g_strdup_printf ("thumb_%s", filename);
		break;
	case GP_FILE_TYPE_RAW:
		full_filename = g_strdup_printf ("raw_%s", filename);
		break;
	case GP_FILE_TYPE_AUDIO:
		full_filename = g_strdup_printf ("audio_%s", filename);
		break;
	default:
		full_filename = g_strdup (filename);
		break;
	}

	return (full_filename);
}

static gchar *
concat_dir_and_file (const gchar *dirname, const gchar *filename)
{
	gchar *full_path;

	if (!strcmp (dirname, "/"))
		full_path = g_strdup_printf ("/%s", filename);
	else if (dirname[strlen (dirname) - 1] == '/')
		full_path = g_strdup_printf ("%s%s", dirname, filename);
	else
		full_path = g_strdup_printf ("%s/%s", dirname, filename);

	return (full_path);
}

static void
save_file (GtkamSave *save, CameraFile *file, guint n)
{
	gchar *full_path, *full_filename, *dirname, *msg, *number_filename;
	const char *filename, *mime_type;
	CameraFileType type;
	const gchar *fsel_filename, *fsel_path, *prefix, *suffix, *progname;
	GtkWidget *dialog;
	int result;

	gp_file_get_type (file, &type);
	gp_file_get_mime_type (file, &mime_type);
	gp_file_get_name (file, &filename);

	fsel_path = gtk_file_selection_get_filename (GTK_FILE_SELECTION (save));

	if (save->priv->toggle_filename_camera->active) {

		/* Use filename provided by the CameraFile */
		full_filename = create_full_filename (filename, type);

		if (g_slist_length (save->priv->filenames) > 1) {
			dirname = g_dirname (fsel_path);
			full_path = concat_dir_and_file (dirname,
							 full_filename);
			g_free (dirname);
		} else {
			full_path = concat_dir_and_file (fsel_path,
							 full_filename);
		}
		g_free (full_filename);

	} else {
		if (g_slist_length (save->priv->filenames) == 1) {

			/* Use filename provided by the GtkFileSelection */
			fsel_filename = g_basename (fsel_path);

			full_filename = create_full_filename (fsel_filename,
							      type);
			dirname = g_dirname (fsel_path);
			full_path = concat_dir_and_file (dirname,
							 full_filename);
			g_free (dirname);
			g_free (full_filename);
		
		} else {

			/* Use filename in prefix */
			prefix = gtk_entry_get_text (save->priv->prefix_entry);
			if (!prefix)
				prefix = "photo";

			suffix = strrchr (mime_type, '/');
			suffix++;
			number_filename = g_strdup_printf ("%s%04i.%s", prefix,
							   n, suffix);
			full_filename = create_full_filename (number_filename,
							      type);
			g_free (number_filename);
			full_path = concat_dir_and_file (fsel_path,
							 full_filename);
			g_free (full_filename);
		}
	}

	/* Check for existing file */
	if (!save->priv->quiet && file_exists (full_path)) {
		msg = g_strdup_printf (_("'%s' already exists."),
				       full_path);
		dialog = gtkam_close_new (msg, GTK_WIDGET (save));
		gtk_widget_show (dialog);
		g_free (msg);
		g_free (full_path);
		return;
	}

	result = gp_file_save (file, full_path);
	if (result < 0) {
		dialog = gtkam_error_new (result, NULL, GTK_WIDGET (save),
				_("Could not save file to '%s'"), full_path);
		gtk_widget_show (dialog);
	} else {
		progname = gtk_entry_get_text (save->priv->program);
		if (progname && fork ()) {
			execlp (progname, progname, full_path, NULL);
			_exit (0);
		} 
	}

	g_free (full_path);
}

static void
get_file (GtkamSave *save, const gchar *filename, CameraFileType type, guint n)
{
	int result;
	GtkWidget *dialog;
	CameraFile *file;

	gp_file_new (&file);
	result = gp_camera_file_get (save->priv->camera,
				     save->priv->path, filename, type, file,
				     NULL);
	if (save->priv->multi)
		gp_camera_exit (save->priv->camera, NULL);
	if (result < 0) {
		switch (result) {
		case GP_ERROR_CANCEL:
			break;
		default:
			dialog = gtkam_error_new (result, NULL,
				GTK_WIDGET (save),
				_("Could not get '%s' from folder '%s'"),
				filename, save->priv->path);
			gtk_widget_show (dialog);
		}
	} else
		save_file (save, file, n);

	gp_file_unref (file);
}

static void
on_ok_clicked (GtkButton *button, GtkamSave *save)
{
	guint i, count;
	const gchar *filename;

	gtk_widget_hide (GTK_WIDGET (save));

	count = g_slist_length (save->priv->filenames);
	for (i = 0; i < count; i++) {
		filename = g_slist_nth_data (save->priv->filenames, i);

		/* Check for shutdown */
		if (!GTKAM_IS_SAVE (save))
			return;

		/* Normal */
		if (save->priv->toggle_normal &&
		    save->priv->toggle_normal->active)
			get_file (save, filename, GP_FILE_TYPE_NORMAL, i + 1);

		if (save->priv->toggle_preview &&
		    save->priv->toggle_preview->active)
			get_file (save, filename, GP_FILE_TYPE_PREVIEW, i + 1);

		if (save->priv->toggle_raw &&
		    save->priv->toggle_raw->active)
			get_file (save, filename, GP_FILE_TYPE_RAW, i + 1);
		if (save->priv->toggle_audio &&
		    save->priv->toggle_audio->active)
			get_file (save, filename, GP_FILE_TYPE_AUDIO, i + 1);
	}

	gtk_object_destroy (GTK_OBJECT (save));
}

GtkWidget *
gtkam_save_new (Camera *camera, gboolean multi, const gchar *path,
		GSList *filenames, GtkWidget *opt_window)
{
	GtkamSave *save;
	GtkWidget *hbox, *frame, *check, *label, *entry;
	GtkTooltips *tooltips;
	GList *child;
	guint i;
	CameraAbilities a;
	gchar *title;

	g_return_val_if_fail (camera != NULL, NULL);
	g_return_val_if_fail (path != NULL, NULL);
	g_return_val_if_fail (filenames != NULL, NULL);

	save = gtk_type_new (GTKAM_TYPE_SAVE);

	save->priv->camera = camera;
	gp_camera_ref (camera);
	save->priv->multi = multi;
	save->priv->path = g_strdup (path);
	for (i = 0; i < g_slist_length (filenames); i++)
		save->priv->filenames = g_slist_append (
			save->priv->filenames,
			g_strdup (g_slist_nth_data (filenames, i)));

	child = gtk_container_children (
			GTK_CONTAINER (GTK_FILE_SELECTION (save)->main_vbox));
	child = gtk_container_children (
			GTK_CONTAINER (child->next->next->data));
	save->priv->file_list = GTK_WIDGET (child->next->data);

	if (g_slist_length (filenames) == 1)
		title = g_strdup_printf (_("Save '%s'..."),
					 (gchar*) filenames->data);
	else
		title = g_strdup (_("Save photos..."));
	gtk_window_set_title (GTK_WINDOW (save), title);
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (save)->ok_button),
			    "clicked", GTK_SIGNAL_FUNC (on_ok_clicked), save);
	gtk_signal_connect (
			GTK_OBJECT (GTK_FILE_SELECTION (save)->cancel_button),
			"clicked", GTK_SIGNAL_FUNC (on_cancel_clicked), save);
	
	tooltips = gtk_tooltips_new ();

	frame = gtk_frame_new (_("What to save:"));
	gtk_widget_show (frame);
	gtk_box_pack_start (GTK_BOX (GTK_FILE_SELECTION (save)->main_vbox),
			    frame, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (TRUE, 5);
	gtk_widget_show (hbox);
	gtk_container_add (GTK_CONTAINER (frame), hbox);

	check = gtk_check_button_new_with_label (_("Save photos"));
	gtk_widget_show (check);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), TRUE);
	gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, TRUE, 0);
	gtk_tooltips_set_tip (tooltips, check, _("Photos will be saved if "
			      "this is checked"), NULL);
	save->priv->toggle_normal = GTK_TOGGLE_BUTTON (check);

	gp_camera_get_abilities (camera, &a);
	if (a.file_operations & GP_FILE_OPERATION_RAW) {
		check = gtk_check_button_new_with_label (_("Save raw data"));
		gtk_widget_show (check);
		gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, TRUE, 0);
		gtk_tooltips_set_tip (tooltips, check, _("Raw data will be "
				      "saved if this is checked"), NULL);
		save->priv->toggle_raw = GTK_TOGGLE_BUTTON (check);
	}

	if (a.file_operations & GP_FILE_OPERATION_AUDIO) {
		check = gtk_check_button_new_with_label (_("Save audio data"));
		gtk_widget_show (check);
		gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, TRUE, 0);
		gtk_tooltips_set_tip (tooltips, check, _("Audio data will be "
				      "saved if this is checked"), NULL);
		save->priv->toggle_audio = GTK_TOGGLE_BUTTON (check);
	}

	if (a.file_operations & GP_FILE_OPERATION_PREVIEW) {
		check = gtk_check_button_new_with_label (_("Save thumbnails"));
		gtk_widget_show (check);
		gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, TRUE, 0);
		gtk_tooltips_set_tip (tooltips, check, _("Thumbnails will be "
				      "saved if this is checked"), NULL);
		save->priv->toggle_preview = GTK_TOGGLE_BUTTON (check);
	}

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_FILE_SELECTION (save)->main_vbox),
			    hbox, TRUE, TRUE, 0);

	label = gtk_label_new (_("Open image(s) with:"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	entry = gtk_entry_new ();
	gtk_widget_show (entry);
	gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
	gtk_tooltips_set_tip (tooltips, entry, _("Type in the name of the "
			      "program you want to run, leave blank "
			      "for none"), NULL);
	save->priv->program = GTK_ENTRY (entry);

	/* Filenames provided by camera */
	check = gtk_check_button_new_with_label (_("Use filenames provided "
						 "by the camera"));
	gtk_widget_show (check);
	gtk_box_pack_start (GTK_BOX (GTK_FILE_SELECTION (save)->main_vbox),
			    check, TRUE, TRUE, 0);
	gtk_tooltips_set_tip (tooltips, check, _("Choose whether to use the "
			      "filename provided by the camera"), NULL);
	gtk_signal_connect (GTK_OBJECT (check), "toggled",
			    GTK_SIGNAL_FUNC (on_filename_camera_toggled), save);
	save->priv->toggle_filename_camera = GTK_TOGGLE_BUTTON (check);
	gtk_toggle_button_set_active (save->priv->toggle_filename_camera, TRUE);

	/* If we don't save multiple files, we are done */
	if (g_slist_length (filenames) <= 1)
		return (GTK_WIDGET (save));
	
	gtk_widget_set_sensitive (GTK_FILE_SELECTION (save)->selection_entry,
				  FALSE);
	gtk_widget_hide (save->priv->file_list);

	hbox = gtk_hbox_new (FALSE, 5);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_FILE_SELECTION (save)->main_vbox),
			    hbox, TRUE, TRUE, 0);
	gtk_box_reorder_child (GTK_BOX (GTK_FILE_SELECTION (save)->main_vbox),
			       hbox, 256);
	
	label = gtk_label_new (_("Filename prefix: "));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	entry = gtk_entry_new ();
	gtk_widget_show (entry);
	gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
	save->priv->prefix_entry = GTK_ENTRY (entry);
	gtk_widget_set_sensitive (entry, FALSE);

	gtk_widget_set_sensitive (save->priv->file_list, FALSE);

	if (opt_window)
		gtk_window_set_transient_for (GTK_WINDOW (save),
					      GTK_WINDOW (opt_window));

	return (GTK_WIDGET (save));
}
