/* gtkam-debug.c
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
#include "gtkam-debug.h"

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

#include <gtk/gtktext.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkvscrollbar.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtkbutton.h>

#include <gphoto2/gphoto2-debug.h>

struct _GtkamDebugPrivate
{
	GtkText *text;
};

#define PARENT_TYPE GTK_TYPE_DIALOG
static GtkDialogClass *parent_class;

static void
gtkam_debug_destroy (GtkObject *object)
{
	GtkamDebug *debug = GTKAM_DEBUG (object);

	debug = NULL;

	gp_debug_set_func (NULL, NULL);

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtkam_debug_finalize (GtkObject *object)
{
	GtkamDebug *debug = GTKAM_DEBUG (object);

	g_free (debug->priv);

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtkam_debug_class_init (GtkamDebugClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy  = gtkam_debug_destroy;
	object_class->finalize = gtkam_debug_finalize;

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
gtkam_debug_init (GtkamDebug *debug)
{
	debug->priv = g_new0 (GtkamDebugPrivate, 1);
}

GtkType
gtkam_debug_get_type (void)
{
	static GtkType debug_type = 0;

	if (!debug_type) {
		static const GtkTypeInfo debug_info = {
			"GtkamDebug",
			sizeof (GtkamDebug),
			sizeof (GtkamDebugClass),
			(GtkClassInitFunc)  gtkam_debug_class_init,
			(GtkObjectInitFunc) gtkam_debug_init,
			NULL, NULL, NULL};
		debug_type = gtk_type_unique (PARENT_TYPE, &debug_info);
	}

	return (debug_type);
}

static void
on_no_debugging_activate (GtkMenuItem *item, GtkamDebug *debug)
{
	gp_debug_set_level (GP_DEBUG_NONE);
}

static void
on_low_debugging_activate (GtkMenuItem *item, GtkamDebug *debug)
{
	gp_debug_set_level (GP_DEBUG_LOW);
}

static void
on_medium_debugging_activate (GtkMenuItem *item, GtkamDebug *debug)
{
	gp_debug_set_level (GP_DEBUG_MEDIUM);
}

static void
on_high_debugging_activate (GtkMenuItem *item, GtkamDebug *debug)
{
	gp_debug_set_level (GP_DEBUG_HIGH);
}

static void
on_ok_clicked (GtkButton *button, gboolean *ok)
{
	*ok = TRUE;
	gtk_main_quit ();
}

static void
on_debug_save_as_clicked (GtkButton *button, GtkamDebug *debug)
{
	GtkWidget *fsel;
	gboolean ok = FALSE;
	const char *fname;

	fsel = gtk_file_selection_new (_("Save As..."));
	gtk_window_set_transient_for (GTK_WINDOW (fsel), GTK_WINDOW (debug));
	gtk_widget_show (fsel);

	gtk_file_selection_set_filename (GTK_FILE_SELECTION (fsel),
					 "gtkam.debug");
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (fsel)->ok_button),
			    "clicked", GTK_SIGNAL_FUNC (on_ok_clicked), &ok);
	gtk_signal_connect (GTK_OBJECT (
				GTK_FILE_SELECTION (fsel)->cancel_button),
			    "clicked", GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
	gtk_signal_connect (GTK_OBJECT (fsel), "delete_event",
			    GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

	gtk_main ();

	if (ok) {
		FILE *file;
		gchar *buffer;

		fname = gtk_file_selection_get_filename (
						GTK_FILE_SELECTION (fsel));
		file = fopen (fname, "w");
		if (!file)
			g_warning (_("Could not open '%s'!"), fname);
		else {
			buffer = gtk_editable_get_chars (
				GTK_EDITABLE (debug->priv->text), 0,
				GTK_EDITABLE (debug->priv->text)->current_pos);
			fputs (buffer, file);
			g_free (buffer);
			fclose (file);
		}
	}

	gtk_object_destroy (GTK_OBJECT (fsel));
}

static void
on_debug_close_clicked (GtkButton *button, GtkamDebug *debug)
{
	gtk_object_destroy (GTK_OBJECT (debug));
}

static void
debug_func (const char *id, const char *msg, void *data)
{
	GtkamDebug *debug;

	g_return_if_fail (GTKAM_IS_DEBUG (data));

	debug = GTKAM_DEBUG (data);

	/* Show the message */
	gtk_text_insert (debug->priv->text, NULL, NULL, NULL,
			 msg, strlen (msg));
	gtk_text_insert (debug->priv->text, NULL, NULL, NULL, "\n", 1);
}

GtkWidget *
gtkam_debug_new (void)
{
	GtkamDebug *debug;
	GtkWidget *hbox, *button, *text, *vscrollbar, *omenu, *menu, *item;

	debug = gtk_type_new (GTKAM_TYPE_DEBUG);

	omenu = gtk_option_menu_new ();

	menu = gtk_menu_new ();
	gtk_widget_show (menu);

	item = gtk_menu_item_new_with_label (_("No debugging"));
	gtk_widget_show (item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
			    GTK_SIGNAL_FUNC (on_no_debugging_activate),
			    debug);
	gtk_menu_append (GTK_MENU (menu), item);

	item = gtk_menu_item_new_with_label (_("Low debugging"));
	gtk_widget_show (item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
			    GTK_SIGNAL_FUNC (on_low_debugging_activate),
			    debug);
	gtk_menu_append (GTK_MENU (menu), item);

	item = gtk_menu_item_new_with_label (_("Medium debugging"));
	gtk_widget_show (item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
			    GTK_SIGNAL_FUNC (on_medium_debugging_activate),
			    debug);
	gtk_menu_append (GTK_MENU (menu), item);

	item = gtk_menu_item_new_with_label (_("High debugging"));
	gtk_widget_show (item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
			    GTK_SIGNAL_FUNC (on_high_debugging_activate),
			    debug);
	gtk_menu_append (GTK_MENU (menu), item);

	gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (omenu), 3);
	gtk_widget_show (omenu);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (debug)->vbox), omenu,
			    FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (debug)->vbox), hbox,
			    TRUE, TRUE, 0);

	text = gtk_text_new (NULL, NULL);
	gtk_widget_show (text);
	gtk_text_set_editable (GTK_TEXT (text), FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), text, TRUE, TRUE, 0);
	debug->priv->text = GTK_TEXT (text);

	vscrollbar = gtk_vscrollbar_new (GTK_TEXT (text)->vadj);
	gtk_widget_show (vscrollbar);
	gtk_box_pack_end (GTK_BOX (hbox), vscrollbar, FALSE, FALSE, 0);

	button = gtk_button_new_with_label (_("Save As"));
	gtk_widget_show (button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_debug_save_as_clicked), debug);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (debug)->action_area),
			   button);

	button = gtk_button_new_with_label (_("Close"));
	gtk_widget_show (button);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (on_debug_close_clicked), debug);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (debug)->action_area),
			   button);
	gtk_widget_grab_focus (button);

	gp_debug_set_func (debug_func, debug);
	gp_debug_set_level (GP_DEBUG_HIGH);

	return (GTK_WIDGET (debug));
}
