/* gtkam-dialog.h
 *
 * Copyright © 2002 Lutz Müller <lutz@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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
