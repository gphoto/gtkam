/* exif-utils.c
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

#include <config.h>
#include "exif-utils.h"

int
Get16s (const unsigned char *buf, ExifByteOrder order)
{
        switch (order) {
        case EXIF_BYTE_ORDER_MOTOROLA:
                return ((buf[0] << 8) | buf[1]);
        case EXIF_BYTE_ORDER_INTEL:
        default:
                return ((buf[1] << 8) | buf[0]);
        }
}

unsigned int
Get16u (const unsigned char *buf, ExifByteOrder order)
{
	return (Get16s (buf, order) & 0xffff);
}

long
Get32s (const unsigned char *b, ExifByteOrder order)
{
        switch (order) {
        case EXIF_BYTE_ORDER_MOTOROLA:
                return ((b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3]);
        case EXIF_BYTE_ORDER_INTEL:
        default:
                return ((b[3] << 24) | (b[2] << 16) | (b[1] << 8) | b[0]);
        }
}

unsigned long
Get32u (const unsigned char *buf, ExifByteOrder order)
{
        return (Get32s (buf, order) & 0xffffffff);
}

