/* gtkam-viewer.h
 *
 * Copyright © 2001 Lutz Müller <urc8@rz.uni-karlsruhe.de>
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

#ifndef __GTKAM_VIEWER_H__
#define __GTKAM_VIEWER_H__

#include <gtkam-dialog.h>
#include <gtkam-camera.h>

#define GTKAM_TYPE_VIEWER  (gtkam_viewer_get_type ())
#define GTKAM_VIEWER(o)    (GTK_CHECK_CAST((o),GTKAM_TYPE_VIEWER,GtkamViewer))
#define GTKAM_IS_VIEWER(o) (GTK_CHECK_TYPE((o),GTKAM_TYPE_VIEWER))

typedef struct _GtkamViewer        GtkamViewer;
typedef struct _GtkamViewerPrivate GtkamViewerPrivate;
typedef struct _GtkamViewerClass   GtkamViewerClass;

struct _GtkamViewer
{
	GtkamDialog parent;

	GtkamViewerPrivate *priv;
};

struct _GtkamViewerClass
{
	GtkamDialogClass parent_class;
};

GtkType    gtkam_viewer_get_type (void);
GtkWidget *gtkam_viewer_new      (void);

void       gtkam_viewer_load_file  (GtkamViewer *, GtkamCamera *,
				    const gchar *folder, const gchar *file);

#endif /* __GTKAM_VIEWER_H__ */
