/* gdk-pixbuf-hacks.c
 *
 * Copyright © 2001 Lutz Müller <lutz@users.sf.net>
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
#include "config.h"
#include "gdk-pixbuf-hacks.h"

#define COPY90                                                          \
        if ((r2 < h1) && (c2 < w1)) {                                   \
          if ((w1 <= h1) && (r1 < h2))                                  \
            for (i = 0; i < c; i++)                                     \
              new_pixels[r1 * rs2 + c1 * c + i] =                      	\
                pixels[r2 * rs1 + c2 * c + i];                  	\
          if ((w1 > h1) && (c1 > (w1 - h2)))                            \
            for (i = 0; i < c; i++)                                     \
              new_pixels[r1 * rs2 + (c1 - (w1 - h1)) * c + i] =        	\
                pixels[r2 * rs1 + c2 * c + i];                  	\
        }

#define COPY270                                                         \
        if ((r2 < h1) && (c2 < w1)) {                                   \
          if ((h1 > w1) && (r1 > (h1 - w1)))                            \
            for (i = 0; i < c; i++)                                     \
              new_pixels[(r1 - (h1 - w1)) * rs2 + c1 * c + i] =        	\
                pixels[r2 * rs1 + c2 * c + i];                  	\
          if ((h1 <= w1) && (c1 < w2))                                  \
            for (i = 0; i < c; i++)                                     \
              new_pixels[r1 * rs2 + c1 * c + i] =               	\
                pixels[r2 * rs1 + c2 * c + i];                  	\
        }

GdkPixbuf *
gdk_pixbuf_rotate (GdkPixbuf *pixbuf, guint angle)
{
        GdkPixbuf *new = NULL;
        guint row, col, w1, h1, w2, h2;
        guint r1, r2, c1, c2;
        guint rs1, rs2;
        guint c;
        guint i;
	guchar *pixels, *new_pixels;

        g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

        switch (angle) {
        case 0:
                return (gdk_pixbuf_copy (pixbuf));
        case 180:
                new = gdk_pixbuf_new (
			gdk_pixbuf_get_colorspace (pixbuf),
			gdk_pixbuf_get_has_alpha (pixbuf),
			gdk_pixbuf_get_bits_per_sample (pixbuf),
			gdk_pixbuf_get_width (pixbuf),
			gdk_pixbuf_get_height (pixbuf));
                break;
        case 90:
        case 270:
                new = gdk_pixbuf_new (
			gdk_pixbuf_get_colorspace (pixbuf),
			gdk_pixbuf_get_has_alpha (pixbuf),
			gdk_pixbuf_get_bits_per_sample (pixbuf),
			gdk_pixbuf_get_height (pixbuf),
			gdk_pixbuf_get_width (pixbuf));
                break;
        default:
                g_warning ("Rotation by %i not implemented.", angle);
                break;
        }

        rs1 = gdk_pixbuf_get_rowstride (pixbuf);
        rs2 = gdk_pixbuf_get_rowstride (new);

        c = gdk_pixbuf_get_has_alpha (pixbuf) ? 4 : 3;

        w1 = gdk_pixbuf_get_width (pixbuf);
        h1 = gdk_pixbuf_get_height (pixbuf);
        w2 = gdk_pixbuf_get_width (new);
        h2 = gdk_pixbuf_get_height (new);

	pixels = gdk_pixbuf_get_pixels (pixbuf);
	new_pixels = gdk_pixbuf_get_pixels (new);

        /*
         * For rotation by 90 or 270, we assume the pixbuf to be a
         * square and move (r2,c2) to (r1,c1):
         */
        switch (angle) {
        case 90:
                for (row = 0; row < MAX (w1, h1) / 2; row++) {
                        for (col = row; col < MAX (w1, h1) - row - 1; col++) {
                                r1 = row;
                                c1 = col;
                                r2 = MAX (w1, h1) - col - 1;
                                c2 = row;
                                COPY90;
                                r1 = r2;
                                c1 = c2;
                                r2 = MAX (w1, h1) - row - 1;
                                c2 = MAX (w1, h1) - col - 1;
                                COPY90;
                                r1 = r2;
                                c1 = c2;
                                r2 = col;
                                c2 = MAX (w1, h1) - row - 1;
                                COPY90;
                                r1 = r2;
                                c1 = c2;
                                r2 = row;
                                c2 = col;
                                COPY90;
                        }
                }
                break;
        case 270:
                for (row = 0; row < MAX (w1, h1) / 2; row++) {
                        for (col = row; col < MAX (w1, h1) - row - 1; col++) {
                                r1 = row;
                                c1 = col;
                                r2 = col;
                                c2 = MAX (w1, h1) - row - 1;
                                COPY270;
                                r1 = r2;
                                c1 = c2;
                                r2 = MAX (w1, h1) - row - 1;
                                c2 = MAX (w1, h1) - col - 1;
                                COPY270;
                                r1 = r2;
                                c1 = c2;
                                r2 = MAX (w1, h1) - col - 1;
                                c2 = row;
                                COPY270;
                                r1 = r2;
                                c1 = c2;
                                r2 = row;
                                c2 = col;
                                COPY270;
                        }
                }
                break;
        case 180:
                for (row = 0; row < h1; row++) {
                        for (col = 0; col < w1; col++) {
                                r1 = row;
                                c1 = col;
                                r2 = h1 - row - 1;
                                c2 = w1 - col - 1;
                                for (i = 0; i < c; i++) {
                                  new_pixels[r2 * rs2 + c2 * c + i] =
                                    pixels[r1 * rs1 + c1 * c + i];
                                }
                        }
                }
                break;
        default:
                g_warning ("Rotation by %i not implemented.", angle);
                break;
        }

        return (new);
}

void
gdk_pixbuf_add (GdkPixbuf *pixbuf, int offset_x, int offset_y,
		GdkPixbuf *pixbuf_to_add)
{
	guchar *p1, *p2, a1, a2;
	guint w1, h1, w2, h2, r1, r2;
	guint row, col, i, pos1, pos2;

	g_return_if_fail (pixbuf != NULL);
	g_return_if_fail (pixbuf_to_add != NULL);

	w1 = gdk_pixbuf_get_width (pixbuf);
	h1 = gdk_pixbuf_get_height (pixbuf);
	w2 = gdk_pixbuf_get_width (pixbuf_to_add);
	h2 = gdk_pixbuf_get_height (pixbuf_to_add);
	g_return_if_fail (w1 >= offset_x + w2);
	g_return_if_fail (h1 >= offset_y + h2);

	p1 = gdk_pixbuf_get_pixels (pixbuf);
	p2 = gdk_pixbuf_get_pixels (pixbuf_to_add);
	r1 = gdk_pixbuf_get_rowstride (pixbuf);
	r2 = gdk_pixbuf_get_rowstride (pixbuf_to_add);
	for (row = 0; row < h2; row++) {
		for (col = 0; col < w2; col++) {
			pos1 = (row + offset_y) * r1 + (col + offset_x) * 4;
			pos2 = row * r2 + col * 4;
			a1 = p1[pos1 + 3];
			a2 = p2[pos2 + 3];

			for (i = 0; i < 3; i++) {
				p1[pos1 + i] *=
					((gfloat) (0xff - a2) / (gfloat) 0xff);
				p1[pos1 + i] += (p2[pos2 + i] * 
					((gfloat) a2 / (gfloat) 0xff));
			}
			p1[pos1 + 3] = MAX (a1, a2);
		}
	}
}
