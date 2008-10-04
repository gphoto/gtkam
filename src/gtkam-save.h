/* gtkam-save.h
 *
 * Copyright 2001,2002 Lutz Mueller <lutz@users.sf.net>
 * Copyright 2005      Andrew Burton <adburton@users.sourceforge.net>
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

#ifndef __GTKAM_SAVE_H__
#define __GTKAM_SAVE_H__

#include <gtkam-camera.h>

/*#include <gtk/gtkfilesel.h>*/
#include <gtk/gtkfilechooser.h>
#include <gtk/gtkfilechooserdialog.h>
#include <gtk/gtkwindow.h>

#define GTKAM_TYPE_SAVE  (gtkam_save_get_type ())
#define GTKAM_SAVE(o)    (GTK_CHECK_CAST((o),GTKAM_TYPE_SAVE,GtkamSave))
#define GTKAM_IS_SAVE(o) (GTK_CHECK_TYPE((o),GTKAM_TYPE_SAVE))

typedef struct _GtkamSave        GtkamSave;
typedef struct _GtkamSavePrivate GtkamSavePrivate;
typedef struct _GtkamSaveClass   GtkamSaveClass;

struct _GtkamSave
{
/*	GtkFileSelection parent;*/
	GtkFileChooserDialog parent;

	GtkamSavePrivate *priv;
};

struct _GtkamSaveClass
{
	/*GtkFileSelectionClass parent_class;*/
	GtkFileChooserDialogClass parent_class;
};

GtkType    gtkam_save_get_type (void);
GtkWidget *gtkam_save_new      (GtkWindow *main_window);

void       gtkam_save_add      (GtkamSave *save, GtkamCamera *,
				const gchar *folder, const gchar *name);

#endif /* __GTKAM_SAVE_H__ */
