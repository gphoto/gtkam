/* gdk-pixbuf-hacks.h
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

#ifndef __GDK_PIXBUF_HACKS__
#define __GDK_PIXBUF_HACKS__

#include <gdk-pixbuf/gdk-pixbuf.h>

GdkPixbuf *gdk_pixbuf_rotate (GdkPixbuf *pixbuf, guint angle);

void gdk_pixbuf_add (GdkPixbuf *pixbuf, int offset_x, int offset_y,
		     GdkPixbuf *pixbuf_to_add);

#endif /* __GDK_PIXBUF_HACKS__ */
