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
#include <gtk/gtkwindow.h>

#include <gphoto2/gphoto2-setting.h>

#include "util.h"
#include "gtkam-error.h"
#include "gtkam-close.h"
#include "gtkam-status.h"

typedef struct _GtkamSaveData GtkamSaveData;
struct _GtkamSaveData {
	Camera *camera;
	gboolean multi;
	gchar *folder;
	gchar *name;
};


struct _GtkamSavePrivate
{
	GtkWidget *file_list;

	GSList *data;

	GtkToggleButton *toggle_preview, *toggle_normal, *toggle_raw,
			*toggle_audio, *toggle_exif;
	GtkToggleButton *toggle_filename_camera;
	GtkEntry *program, *prefix_entry;
	GtkSpinButton *spin_entry;

	gboolean quiet, err_shown;

	GtkWidget *status;
};

#define PARENT_TYPE GTK_TYPE_FILE_SELECTION
static GtkFileSelection *parent_class;

static void
gtkam_save_destroy (GtkObject *object)
{
	GtkamSave *save = GTKAM_SAVE (object);
	gint i;
	GtkamSaveData *data;

	if (save->priv->status) {
		gtk_object_unref (GTK_OBJECT (save->priv->status));
		save->priv->status = NULL;
	}

	if (save->priv->data) {
		for (i = g_slist_length (save->priv->data) - 1; i >= 0; i--) {
			data = g_slist_nth_data (save->priv->data, i);
			gp_camera_unref (data->camera);
			g_free (data->folder);
			g_free (data->name);
			g_free (data);
		}
		g_slist_free (save->priv->data);
		save->priv->data = NULL;
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
	if (save->priv->prefix_entry) {
		gtk_widget_set_sensitive (GTK_WIDGET (save->priv->prefix_entry),
					  !toggle->active);
		gtk_widget_set_sensitive (GTK_WIDGET (save->priv->spin_entry),
					  !toggle->active);
	} else {
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

		if (g_slist_length (save->priv->data) > 1) {
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
		if (g_slist_length (save->priv->data) == 1) {

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
				_("Could not save file to '%s'."), full_path);
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
get_file (GtkamSave *save, Camera *camera, gboolean multi,
	  const gchar *folder, const gchar *name, CameraFileType type, guint n,
	  GtkamStatus *s)
{
	int result;
	GtkWidget *dialog;
	CameraFile *file;

	gp_file_new (&file);
	result = gp_camera_file_get (camera, folder, name, type, file,
				     s->context->context);
	if (multi)
		gp_camera_exit (camera, NULL);
	switch (result) {
	case GP_OK:
		save_file (save, file, n);
		break;
	case GP_ERROR_CANCEL:
		break;
	default:
		if (!save->priv->err_shown) {
			dialog = gtkam_error_new (result, s->context,
				GTK_WIDGET (save), _("Could not get '%s' "
				"from folder '%s'."),
				name, folder);
			gtk_widget_show (dialog);
			save->priv->err_shown = TRUE;
		}
	}
	gp_file_unref (file);
}

static void
on_ok_clicked (GtkButton *button, GtkamSave *save)
{
	guint i, count, j = 1;
	GtkWidget *s;
	unsigned int id;
	GtkamSaveData *data;

	gtk_widget_hide (GTK_WIDGET (save));

	count = g_slist_length (save->priv->data);
	s = gtkam_status_new (_("Downloading %i files..."), count);
	gtk_widget_show (s);
	gtk_box_pack_start (GTK_BOX (save->priv->status), s, FALSE, FALSE, 0);

	id = gp_context_progress_start (GTKAM_STATUS (s)->context->context,
		count, _("Downloading %i files..."), count);

	if (!save->priv->toggle_filename_camera->active)
		j = gtk_spin_button_get_value_as_int (save->priv->spin_entry);
	
	for (i = 0; i < count; i++) {
		data = g_slist_nth_data (save->priv->data, i);

		/* Check for shutdown */
		if (!GTKAM_IS_SAVE (save))
			return;

		if (save->priv->toggle_normal &&
		    save->priv->toggle_normal->active)
			get_file (save, data->camera, data->multi,
				  data->folder, data->name,
				  GP_FILE_TYPE_NORMAL, i + j,
				  GTKAM_STATUS (s));
		if (save->priv->toggle_preview &&
		    save->priv->toggle_preview->active)
			get_file (save, data->camera, data->multi,
				  data->folder, data->name,
				  GP_FILE_TYPE_PREVIEW, i + j,
				  GTKAM_STATUS (s));
		if (save->priv->toggle_raw &&
		    save->priv->toggle_raw->active)
			get_file (save, data->camera, data->multi,
				  data->folder, data->name, GP_FILE_TYPE_RAW,
				  i + j, GTKAM_STATUS (s));
		if (save->priv->toggle_audio &&
		    save->priv->toggle_audio->active)
			get_file (save, data->camera, data->multi,
				  data->folder, data->name, GP_FILE_TYPE_AUDIO,
				  i + j, GTKAM_STATUS (s));
		if (save->priv->toggle_exif &&
		    save->priv->toggle_exif->active)
			get_file (save, data->camera, data->multi,
				  data->folder, data->name, GP_FILE_TYPE_EXIF,
				  i + j, GTKAM_STATUS (s));

		gp_context_progress_update (GTKAM_STATUS (s)->context->context,
					   id, i + 1);
	}
	gp_context_progress_stop (GTKAM_STATUS (s)->context->context, id);
	gtk_object_destroy (GTK_OBJECT (s));

	gtk_object_destroy (GTK_OBJECT (save));
}

GtkWidget *
gtkam_save_new (GtkWidget *vbox)
{
	GtkamSave *save;
	GtkWidget *hbox, *frame, *check, *label, *entry, *w;
	GtkTooltips *tooltips;
	GList *child;
//	CameraAbilities a;

	g_return_val_if_fail (GTK_IS_VBOX (vbox), NULL);

	save = gtk_type_new (GTKAM_TYPE_SAVE);

	child = gtk_container_children (
			GTK_CONTAINER (GTK_FILE_SELECTION (save)->main_vbox));
	child = gtk_container_children (
			GTK_CONTAINER (child->next->next->data));
	save->priv->file_list = GTK_WIDGET (child->next->data);

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

//	gp_camera_get_abilities (camera, &a);
//	if (a.file_operations & GP_FILE_OPERATION_RAW) {
		check = gtk_check_button_new_with_label (_("Save raw data"));
		gtk_widget_show (check);
		gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, TRUE, 0);
		gtk_tooltips_set_tip (tooltips, check, _("Raw data will be "
				      "saved if this is checked"), NULL);
		save->priv->toggle_raw = GTK_TOGGLE_BUTTON (check);
//	}

//	if (a.file_operations & GP_FILE_OPERATION_AUDIO) {
		check = gtk_check_button_new_with_label (_("Save audio data"));
		gtk_widget_show (check);
		gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, TRUE, 0);
		gtk_tooltips_set_tip (tooltips, check, _("Audio data will be "
				      "saved if this is checked"), NULL);
		save->priv->toggle_audio = GTK_TOGGLE_BUTTON (check);
//	}

//	if (a.file_operations & GP_FILE_OPERATION_PREVIEW) {
		check = gtk_check_button_new_with_label (_("Save thumbnails"));
		gtk_widget_show (check);
		gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, TRUE, 0);
		gtk_tooltips_set_tip (tooltips, check, _("Thumbnails will be "
				      "saved if this is checked"), NULL);
		save->priv->toggle_preview = GTK_TOGGLE_BUTTON (check);
//	}

//	if (a.file_operations & GP_FILE_OPERATION_EXIF) {
		check = gtk_check_button_new_with_label (_("Save EXIF data"));
		gtk_widget_show (check);
		gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, TRUE, 0);
		gtk_tooltips_set_tip (tooltips, check, _("EXIF data will be "
			"saved if this is checked"), NULL);
		save->priv->toggle_exif = GTK_TOGGLE_BUTTON (check);
//	}

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_FILE_SELECTION (save)->main_vbox),
			    hbox, TRUE, TRUE, 0);

	label = gtk_label_new (_("Open image(s) with: "));
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

	w = gtk_widget_get_ancestor (vbox, GTK_TYPE_WINDOW);
	gtk_window_set_transient_for (GTK_WINDOW (save), GTK_WINDOW (w));
	save->priv->status = vbox;
	gtk_object_ref (GTK_OBJECT (vbox));

	return (GTK_WIDGET (save));
}

void
gtkam_save_add (GtkamSave *save, Camera *camera, gboolean multi,
		const gchar *folder, const gchar *name)
{
	GtkamSaveData *data;
	GtkWidget *label, *hbox, *entry, *spin_entry;
	GtkAdjustment *spin_entry_adj;

	g_return_if_fail (GTKAM_IS_SAVE (save));
	g_return_if_fail (camera != NULL);
	g_return_if_fail (folder != NULL);
	g_return_if_fail (name != NULL);

	data = g_new0 (GtkamSaveData, 1);
	data->camera = camera;
	gp_camera_ref (camera);
	data->multi = multi;
	data->folder = g_strdup (folder);
	data->name = g_strdup (name);
	save->priv->data = g_slist_append (save->priv->data, data);

	/* If we only have 1 file, things are easy. */
	if (g_slist_length (save->priv->data) == 1) {
		gchar *title = g_strdup_printf (_("Save '%s'..."), name);
		gtk_window_set_title (GTK_WINDOW (save), title);
		g_free (title);
		return;
	}

	gtk_window_set_title (GTK_WINDOW (save), _("Save photos..."));
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

        hbox = gtk_hbox_new (FALSE, 5);
        gtk_widget_show (hbox);
        gtk_box_pack_start (GTK_BOX (GTK_FILE_SELECTION (save)->main_vbox),
                            hbox, TRUE, TRUE, 0);
        gtk_box_reorder_child (GTK_BOX (GTK_FILE_SELECTION (save)->main_vbox),
                               hbox, 256);
        
        label = gtk_label_new (_("Start numbering with: "));
        gtk_widget_show (label);
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

        spin_entry_adj = GTK_ADJUSTMENT (gtk_adjustment_new(1.0, 1.0, 10000.0, 1.0, 10.0, 10.0));
        spin_entry = gtk_spin_button_new (spin_entry_adj, 1.0, 0);
        gtk_widget_show (spin_entry);
        gtk_box_pack_start (GTK_BOX (hbox), spin_entry, TRUE, TRUE, 0);
        save->priv->spin_entry = GTK_SPIN_BUTTON (spin_entry);
        gtk_widget_set_sensitive (spin_entry, FALSE);

        gtk_widget_set_sensitive (save->priv->file_list, FALSE);
}
