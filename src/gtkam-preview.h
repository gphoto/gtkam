/* gtkam-preview.h
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

#ifndef __GTKAM_PREVIEW_H__
#define __GTKAM_PREVIEW_H__

#include <gtkam-camera.h>
#include <gtkam-dialog.h>

#define GTKAM_TYPE_PREVIEW  (gtkam_preview_get_type ())
#define GTKAM_PREVIEW(o)    (GTK_CHECK_CAST((o),GTKAM_TYPE_PREVIEW,GtkamPreview))
#define GTKAM_IS_PREVIEW(o) (GTK_CHECK_TYPE((o),GTKAM_TYPE_PREVIEW))

typedef struct _GtkamPreview        GtkamPreview;
typedef struct _GtkamPreviewPrivate GtkamPreviewPrivate;
typedef struct _GtkamPreviewClass   GtkamPreviewClass;

struct _GtkamPreview
{
	GtkamDialog parent;

	GtkamPreviewPrivate *priv;
};

typedef struct _GtkamPreviewCapturedData GtkamPreviewCapturedData;
struct _GtkamPreviewCapturedData {
	GtkamCamera *camera;
	const gchar *folder;
	const gchar *name;
};

struct _GtkamPreviewClass
{
	GtkamDialogClass parent_class;

	/* Signals */
	void (* captured) (GtkamPreview *preview, GtkamPreviewCapturedData *);
};

GtkType    gtkam_preview_get_type (void);
GtkWidget *gtkam_preview_new      (GtkamCamera *camera);

void  gtkam_preview_set_angle  (GtkamPreview *preview, guint angle);
guint gtkam_preview_get_angle  (GtkamPreview *preview);

#endif /* __GTKAM_PREVIEW_H__ */
