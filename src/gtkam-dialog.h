/* gtkam-dialog.h
 *
 * Copyright 2002 Lutz Mueller <lutz@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GTKAM_DIALOG_H__ 
#define __GTKAM_DIALOG_H__

#include <gtk/gtkdialog.h>

#define GTKAM_TYPE_DIALOG  (gtkam_dialog_get_type ())
#define GTKAM_DIALOG(o)    (GTK_CHECK_CAST((o),GTKAM_TYPE_DIALOG,GtkamDialog))
#define GTKAM_IS_DIALOG(o) (GTK_CHECK_TYPE((o),GTKAM_TYPE_DIALOG))

typedef struct _GtkamDialog        GtkamDialog;
typedef struct _GtkamDialogPrivate GtkamDialogPrivate;
typedef struct _GtkamDialogClass   GtkamDialogClass;

struct _GtkamDialog
{
	GtkDialog parent;

	GtkWidget *image, *vbox;

	GtkamDialogPrivate *priv;
};

struct _GtkamDialogClass
{
	GtkDialogClass parent_class;
};

GType gtkam_dialog_get_type (void);

void gtkam_dialog_add_status (GtkamDialog *, GtkWidget *status);

#endif /* __GTKAM_DIALOG_H__ */
