/* jpeg-marker.h
 *
 * Copyright (C) 2001 Lutz Müller <lutz@users.sourceforge.net>
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
#ifndef __JPEG_MARKER_H__
#define __JPEG_MARKER_H__

typedef enum _JPEGMarker JPEGMarker;
enum _JPEGMarker {
        JPEG_MARKER_SOF0        = 0xc0,
        JPEG_MARKER_SOF1        = 0xc1,
        JPEG_MARKER_SOF2        = 0xc2,
        JPEG_MARKER_SOF3        = 0xc3,
	JPEG_MARKER_SOME0	= 0xc4,
        JPEG_MARKER_SOF5        = 0xc5,
        JPEG_MARKER_SOF6        = 0xc6,
        JPEG_MARKER_SOF7        = 0xc7,
        JPEG_MARKER_SOF8        = 0xc8,
        JPEG_MARKER_SOF9        = 0xc9,
        JPEG_MARKER_SOF10       = 0xca,
        JPEG_MARKER_SOF11       = 0xcb,
	JPEG_MARKER_SOME1	= 0xcc,
        JPEG_MARKER_SOF13       = 0xcd,
        JPEG_MARKER_SOF14       = 0xce,
        JPEG_MARKER_SOF15       = 0xcf,
        JPEG_MARKER_UNKNOWN0    = 0xd0,
        JPEG_MARKER_UNKNOWN1    = 0xd1,
        JPEG_MARKER_UNKNOWN2    = 0xd2,
        JPEG_MARKER_UNKNOWN3    = 0xd3,
        JPEG_MARKER_UNKNOWN4    = 0xd4,
	JPEG_MARKER_UNKNOWN5    = 0xd5,
        JPEG_MARKER_UNKNOWN6    = 0xd6,
        JPEG_MARKER_UNKNOWN7    = 0xd7,
        JPEG_MARKER_SOI         = 0xd8,
        JPEG_MARKER_EOI         = 0xd9,
        JPEG_MARKER_SOS         = 0xda,
        JPEG_MARKER_UNKNOWN11   = 0xdb,
        JPEG_MARKER_UNKNOWN12   = 0xdc,
        JPEG_MARKER_UNKNOWN13   = 0xdd,
        JPEG_MARKER_UNKNOWN14   = 0xde,
        JPEG_MARKER_UNKNOWN15   = 0xdf,
	JPEG_MARKER_JFIF	= 0xe0,
        JPEG_MARKER_EXIF        = 0xe1,
        JPEG_MARKER_COM         = 0xfe
};

#define JPEG_IS_MARKER(m) ((((m) >= JPEG_MARKER_SOF0) &&        \
                            ((m) <= JPEG_MARKER_SOF15)) ||      \
                           (((m) >= JPEG_MARKER_UNKNOWN0) &&    \
                            ((m) <= JPEG_MARKER_UNKNOWN15)) ||  \
                           ((m) == JPEG_MARKER_EXIF) ||         \
			   ((m) == JPEG_MARKER_JFIF) ||		\
                           ((m) == JPEG_MARKER_COM))

const char *jpeg_marker_get_name        (JPEGMarker marker);
const char *jpeg_marker_get_description (JPEGMarker marker);

#endif /* __JPEG_MARKER_H__ */
