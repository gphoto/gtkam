#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gphoto2.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>

#include "globals.h"
#include "gtkiconlist.h"
#include "interface.h"
#include "frontend.h"
#include "support.h"
#include "util.h"
#include "gtkam-close.h"

int
frontend_message (Camera *camera, char *message)
{
	GtkWidget *dialog;

	dialog = gtkam_close_new (message, NULL);

	return (GP_OK);
}

int frontend_status(Camera *camera, char *message) {

	GtkWidget *label;

	if (GTK_WIDGET_VISIBLE(gp_gtk_progress_window)) {
		label  = (GtkWidget*) lookup_widget(gp_gtk_progress_window, "message");
		gtk_label_set_text(GTK_LABEL(label), message);
		while (gtk_events_pending ())
			gtk_main_iteration ();
		return (GP_OK);
	}

	return (GP_OK);
}

int frontend_progress(Camera *camera, CameraFile *file, float percentage) {

	GtkWidget *progress = (GtkWidget*)lookup_widget(gp_gtk_progress_window, "progress_bar");

	gtk_progress_set_percentage(GTK_PROGRESS(progress), percentage/100.0);
	while (gtk_events_pending ())
		gtk_main_iteration ();

	return (GP_OK);
}

int frontend_confirm(Camera *camera, char *message) {

	GtkWidget *confirm = create_confirm_window();
	GtkWidget *label   = (GtkWidget*) lookup_widget(confirm, "message");
	GtkWidget *yes     = (GtkWidget*) lookup_widget(confirm, "yes");
	GtkWidget *no      = (GtkWidget*) lookup_widget(confirm, "no");
	int ret = 0;

	gtk_label_set_text(GTK_LABEL(label), message);
	ret = wait_for_hide(confirm, yes, no);

	if (ret)
		gtk_widget_destroy(confirm);

	return (ret);
}

static void
frontend_prompt_build (CameraWidget *w, GtkWidget *box, GtkWidget **window) {

	GtkWidget *win, *ok, *cancel; 
	GtkWidget *gtklabel, *vbox, *notebook, *entry, *hscale, *button, *frame, *menu;
	GtkWidget *widget = NULL;
	GtkObject *adj;
	GSList *list;
	GList *options;
	CameraWidget *c;
	CameraWidgetType type;
	float min, max, increment;
        int x;
        int   vali;
        float valf;
        char* vals;
	const char *label;
	const char *choice;

	gp_widget_get_label (w, &label);
	gp_widget_get_type (w, &type);

	/* Check if packing box has been created */
	if ((type != GP_WIDGET_WINDOW) && (!box))
		return;
	
	switch (type) {
	case GP_WIDGET_WINDOW:
		win = gtk_dialog_new ();
		*window = win;
		gtk_window_set_title (GTK_WINDOW(win), label);
		gtk_window_set_modal (GTK_WINDOW(win), TRUE);
		gtk_window_set_position (GTK_WINDOW (win), 
					 GTK_WIN_POS_CENTER);

		ok = gtk_button_new_with_label ("OK");
		gtk_widget_show (ok);
		gtk_object_set_data (GTK_OBJECT(win), "ok", ok);
		gtk_box_pack_start (GTK_BOX(GTK_DIALOG(win)->action_area), ok, FALSE, TRUE, 0);

		cancel = gtk_button_new_with_label ("Cancel");
		gtk_widget_show (cancel);
		gtk_object_set_data (GTK_OBJECT(win), "cancel", cancel);
		gtk_box_pack_start (GTK_BOX(GTK_DIALOG(win)->action_area), cancel, FALSE, TRUE, 0);

		box = GTK_DIALOG(win)->vbox;
		break;
	case GP_WIDGET_SECTION:
		/* Check to see if the notebook has been created yet */
		if ((notebook=gtk_object_get_data(GTK_OBJECT(*window), "notebook"))==NULL) {
			notebook = gtk_notebook_new();
			gtk_widget_show(notebook);
			gtk_box_pack_start(GTK_BOX(box), notebook, TRUE, TRUE, 0);
			gtk_object_set_data(GTK_OBJECT(*window), "notebook", notebook);
		}

		/* Create the new label for the page */
		gtklabel = gtk_label_new (label);
		gtk_widget_show (gtklabel);

		/* Create a packing box in the page */
		vbox = gtk_vbox_new (FALSE, 10);
		gtk_widget_show (vbox);
		gtk_notebook_append_page (GTK_NOTEBOOK(notebook), vbox, 
					  gtklabel);

		box = vbox;
		break;
	case GP_WIDGET_TEXT:
		entry = gtk_entry_new();
		gtk_widget_show(entry);
                gp_widget_get_value (w, &vals);
		gtk_entry_set_text (GTK_ENTRY(entry), vals);
		gtk_object_set_data (GTK_OBJECT(*window), label, entry);

                gp_widget_get_info (w, (const char**)(&vals));
                if (vals) {
                    GtkTooltips *tips;
                    tips = gtk_tooltips_new();
                    gtk_tooltips_set_tip(tips, entry, vals, "");
                }

		frame = gtk_frame_new (label);
		gtk_widget_show(frame);
		gtk_container_add(GTK_CONTAINER(frame), entry);
		widget = frame;
		
		break;
	case GP_WIDGET_RANGE:
		gp_widget_get_range (w, &min, &max, &increment);
		gp_widget_get_value (w, &valf);
		adj = gtk_adjustment_new(valf, min, max, increment, 0, 0);
		hscale = gtk_hscale_new(GTK_ADJUSTMENT(adj));
		gtk_widget_show(hscale);
		gtk_object_set_data(GTK_OBJECT(*window), label, hscale);

		gtk_range_set_update_policy(GTK_RANGE(hscale),GTK_UPDATE_CONTINUOUS);
		gtk_scale_set_draw_value(GTK_SCALE(hscale), TRUE);
		gtk_scale_set_digits(GTK_SCALE(hscale), 0);

                gp_widget_get_info (w, (const char**)(&vals));
                if (vals) {
                    GtkTooltips *tips;
                    tips = gtk_tooltips_new();
                    gtk_tooltips_set_tip(tips, hscale, vals, "");
                }


                frame = gtk_frame_new (label);
		gtk_widget_show(frame);
		gtk_container_add(GTK_CONTAINER(frame), hscale);
		widget = frame;
		break;
	case GP_WIDGET_TOGGLE:
		button = gtk_check_button_new_with_label (label);
                gp_widget_get_value (w, &vali);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), vali);
		gtk_widget_show(button);
		gtk_object_set_data (GTK_OBJECT(*window), label, button);

                gp_widget_get_info (w, (const char**)(&vals));
                if (vals) {
                    GtkTooltips *tips;
                    tips = gtk_tooltips_new();
                    gtk_tooltips_set_tip(tips, button, vals, "");
                }


		widget = button;
		break;
	case GP_WIDGET_RADIO:
		vbox = gtk_vbox_new (FALSE, 0);
		gtk_widget_show (vbox);

		gp_widget_get_value (w, &vals);
		gp_widget_get_choice (w, 0, &choice);

		/* Add the first radio button */
		button = gtk_radio_button_new_with_label (NULL, choice);
		if (vals && !strcmp (vals, choice))
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button),TRUE);
		gtk_widget_show (button);
		gtk_object_set_data (GTK_OBJECT(*window), choice, button);
		gtk_object_set_data (GTK_OBJECT(*window), label, button);
		gtk_box_pack_start (GTK_BOX(vbox), button, TRUE, TRUE, 0);

		/* Add the rest of the radio buttons */
		for (x = 1; x < gp_widget_count_choices (w); x++) {

			gp_widget_get_choice (w, x, &choice);
			list = gtk_radio_button_group (
					GTK_RADIO_BUTTON(button)); 
			button = gtk_radio_button_new_with_label (list, choice);
		        gtk_widget_show (button);
			gtk_object_set_data (GTK_OBJECT(*window), choice, 
					     button);
			gtk_box_pack_start (GTK_BOX(vbox), button, TRUE, 
					    TRUE, 0);
			if (vals && !strcmp (vals, choice))
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button), TRUE);
                        gp_widget_get_info (w, (const char**)(&vals));
                        if (vals) {
                            GtkTooltips *tips;
                            tips = gtk_tooltips_new();
                            gtk_tooltips_set_tip(tips, button, vals, "");
                        }
                }

		frame = gtk_frame_new (label);
		gtk_widget_show(frame);
		gtk_container_add(GTK_CONTAINER(frame), vbox);
		widget = frame;

		break;
	case GP_WIDGET_MENU:
		menu = gtk_combo_new();
		gtk_widget_show(menu);
		gtk_object_set_data(GTK_OBJECT(*window), label, menu);
		gtk_entry_set_editable (GTK_ENTRY(GTK_COMBO(menu)->entry), FALSE);
		options = g_list_alloc();
		for (x = 0; x < gp_widget_count_choices (w); x++) {
			gp_widget_get_choice (w, x, &choice);
			options = g_list_append (options, 
						 strdup (choice));
		}
                gtk_combo_set_popdown_strings(GTK_COMBO(menu), options);
                gp_widget_get_value (w, &vals);
                gtk_entry_set_text (GTK_ENTRY(GTK_COMBO(menu)->entry), vals);

                gp_widget_get_info (w, (const char**)&vals);
                if (vals) {
                    GtkTooltips *tips;
                    tips = gtk_tooltips_new();
                    gtk_tooltips_set_tip(tips, GTK_COMBO(menu)->entry, vals, "");
                }


		frame = gtk_frame_new (label);
		gtk_widget_show(frame);
		gtk_container_add(GTK_CONTAINER(frame), menu);
		widget = frame;

		break;
	case GP_WIDGET_BUTTON:
		/* Not supported yet */
		break;
	default:
		break;
	}

	if (widget)
		gtk_box_pack_start (GTK_BOX(box), widget, FALSE, TRUE, 0);

	for (x = 0; x < gp_widget_count_children (w); x++) {
		gp_widget_get_child (w, x, &c);
		frontend_prompt_build (c, box, window);
	}

}

static void
frontend_prompt_retrieve (CameraWidget *w, GtkWidget *window) {

	GtkWidget *widget;
	GtkAdjustment *adj;
	CameraWidget *c;
	CameraWidgetType type;
	int x;
	const char* label;

	gp_widget_get_label (w, &label);
	gp_widget_get_type (w, &type);

	widget = gtk_object_get_data (GTK_OBJECT(window), label);
	if (widget) {
		switch (type) {
		case GP_WIDGET_WINDOW:
			/* do nothing. */
			break;
		case GP_WIDGET_SECTION:
			/* do nothing. */
			break;
		case GP_WIDGET_TEXT:
			gp_widget_set_value (w, gtk_entry_get_text (GTK_ENTRY(widget)));
			break;
		case GP_WIDGET_RANGE:
			adj = gtk_range_get_adjustment  (GTK_RANGE(widget));
			gp_widget_set_value (w, &(adj->value));
			break;
		case GP_WIDGET_TOGGLE:
                        x = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
			gp_widget_set_value (w, &x);
			break;
		case GP_WIDGET_RADIO:
			for (x = 0; x < gp_widget_count_choices (w); x++) {
				const char *choice;

				gp_widget_get_choice (w, x, &choice);
				widget = gtk_object_get_data (GTK_OBJECT(window), choice);
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
					gp_widget_set_value (w, (void*) choice);
			}
			break;
		case GP_WIDGET_MENU:
			gp_widget_set_value (w, gtk_entry_get_text (
				GTK_ENTRY(GTK_COMBO(widget)->entry)));
			break;
		case GP_WIDGET_BUTTON:
		default:
			break;
		}
	}

	for (x = 0; x < gp_widget_count_children (w); x++) {
		gp_widget_get_child (w, x, &c);
		frontend_prompt_retrieve (c, window);
	}
}

int frontend_prompt (Camera *camera, CameraWidget *window) {

	GtkWidget *win = NULL, *ok, *cancel;
	CameraWidgetCallback *cb;

	gp_widget_get_value (window, &cb);

	frontend_prompt_build (window, NULL, &win);

	if (!win)
		return (GP_PROMPT_CANCEL);

	ok     = gtk_object_get_data(GTK_OBJECT(win), "ok");
	cancel = gtk_object_get_data(GTK_OBJECT(win), "cancel");

	if (!wait_for_hide(win, ok, cancel))
		return (GP_PROMPT_CANCEL);

	frontend_prompt_retrieve(window, win);

	gtk_widget_destroy(win);

/*
	if (cb)
		(*cb)(camera, window);
*/

	return (GP_PROMPT_OK);
}
