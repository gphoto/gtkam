/* jpeg-marker.c
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
#include "jpeg-marker.h"

#include <stdlib.h>

static struct {
        JPEGMarker marker;
        const char *name;
        const char *description;
} JPEGMarkerTable[] = {
        {JPEG_MARKER_SOF0, "SOF0", "Encoding (baseline)"},
        {JPEG_MARKER_SOF1, "SOF1", "Encoding (extended sequential)"},
        {JPEG_MARKER_SOF2, "SOF2", "Encoding (progressive)"},
        {JPEG_MARKER_SOF3, "SOF3", "Encoding (lossless)"},
        {JPEG_MARKER_SOF5, "SOF5", "Encoding (differential sequential)"},
        {JPEG_MARKER_SOF6, "SOF6", "Encoding (differential progressive)"},
        {JPEG_MARKER_SOF7, "SOF7", "Encoding (differential lossless)"},
        {JPEG_MARKER_SOF9, "SOF9",
		"Encoding (extended sequential, arithmetic)"},
        {JPEG_MARKER_SOF10, "SOF10", "Encoding (progressive, arithmetic)"},
        {JPEG_MARKER_SOF11, "SOF11", "Encoding (lossless, arithmetic)"},
        {JPEG_MARKER_SOF13, "SOF13",
		"Encoding (differential sequential, arithmetic)"},
        {JPEG_MARKER_SOF14, "SOF14",
		"Encoding (differential progressive, arithmetic)"},
        {JPEG_MARKER_SOF15, "SOF15",
		"Encoding (differential lossless, arithmetic)"},
	{JPEG_MARKER_SOI, "SOI", "Start of image"},
	{JPEG_MARKER_EOI, "EOI", "End of image"},
	{JPEG_MARKER_SOS, "SOS", "Start of image data"},
	{JPEG_MARKER_EXIF, "EXIF", "EXIF data"},
	{JPEG_MARKER_COM, "COM", "Comment"},
        {0, NULL, NULL}
};

const char *
jpeg_marker_get_name (JPEGMarker marker)
{
	unsigned int i;

	for (i = 0; JPEGMarkerTable[i].name; i++)
		if (JPEGMarkerTable[i].marker == marker)
			break;

	return (JPEGMarkerTable[i].name);
}

const char *
jpeg_marker_get_description (JPEGMarker marker)
{
	unsigned int i;

	for (i = 0; JPEGMarkerTable[i].description; i++)
		if (JPEGMarkerTable[i].marker == marker)
			break;

	return (JPEGMarkerTable[i].description);
}

