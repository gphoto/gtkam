/* gdk-pixbuf-hacks.h
 *
 * Copyright 2001 Lutz Mueller <lutz@users.sf.net>
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

#ifndef __GDK_PIXBUF_HACKS__
#define __GDK_PIXBUF_HACKS__

#include <gdk-pixbuf/gdk-pixbuf.h>

GdkPixbuf *gdk_pixbuf_rotate (GdkPixbuf *pixbuf, guint angle);

void gdk_pixbuf_add (GdkPixbuf *pixbuf, int offset_x, int offset_y,
		     GdkPixbuf *pixbuf_to_add);

#endif /* __GDK_PIXBUF_HACKS__ */
