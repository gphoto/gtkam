/* exif-entry.c
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
#include "exif-entry.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libjpeg/jpeg-marker.h>

//#define DEBUG

#undef  MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

static const unsigned char ExifHeader[] = {0x45, 0x78, 0x69, 0x66, 0x00, 0x00};

static struct {
        ExifFormat format;
        unsigned char size;
} ExifFormatSize[] = {
        {EXIF_FORMAT_UBYTE,     1},
        {EXIF_FORMAT_STRING,    1},
        {EXIF_FORMAT_USHORT,    2},
        {EXIF_FORMAT_ULONG,     4},
        {EXIF_FORMAT_URATIONAL, 8},
        {EXIF_FORMAT_SBYTE,     1},
        {EXIF_FORMAT_SSHORT,    2},
        {EXIF_FORMAT_SLONG,     4},
        {EXIF_FORMAT_SRATIONAL, 8},
        {EXIF_FORMAT_SINGLE,    4},
        {EXIF_FORMAT_DOUBLE,    8},
        {0, 0}
};

struct _ExifEntryPrivate
{
	ExifByteOrder order;

	unsigned int ref_count;
};

ExifEntry *
exif_entry_new (void)
{
	ExifEntry *entry;

	entry = malloc (sizeof (ExifEntry));
	if (!entry)
		return (NULL);
	memset (entry, 0, sizeof (ExifEntry));
	entry->priv = malloc (sizeof (ExifEntryPrivate));
	if (!entry->priv) {
		free (entry);
		return (NULL);
	}
	memset (entry->priv, 0, sizeof (ExifEntryPrivate));
	entry->priv->ref_count = 1;

	entry->content = exif_content_new ();
	if (!entry->content) {
		exif_entry_free (entry);
		return (NULL);
	}

	return (entry);
}

void
exif_entry_ref (ExifEntry *entry)
{
	entry->priv->ref_count++;
}

void
exif_entry_unref (ExifEntry *entry)
{
	entry->priv->ref_count--;
	if (!entry->priv->ref_count)
		exif_entry_free (entry);
}

void
exif_entry_free (ExifEntry *entry)
{
	if (entry->data)
		free (entry->data);
	exif_content_unref (entry->content);
	free (entry->priv);
	free (entry);
}

void
exif_entry_parse (ExifEntry *entry, const unsigned char *data,
		  unsigned int size, unsigned int offset, ExifByteOrder order)
{
	unsigned int j, s, doff;

	entry->priv->order = order;
	entry->tag         = Get16u (data + offset + 0, order);
	entry->format      = Get16u (data + offset + 2, order);
	entry->components  = Get32u (data + offset + 4, order);

#ifdef DEBUG
	printf ("Parsing entry (tag 0x%x - '%s')...\n", entry->tag,
		exif_tag_get_name (entry->tag));
#endif

	/*
	 * Size? If bigger than 4 bytes, the actual data is not
	 * in the entry but somewhere else (offset). Beware of subdirectories.
	 */
	for (s = j = 0; ExifFormatSize[j].size; j++)
		if (ExifFormatSize[j].format == entry->format) {
			s = ExifFormatSize[j].size * entry->components;
			break;
		}
	if (!s)
		return;
	if ((s > 4) || (entry->tag == EXIF_TAG_EXIF_OFFSET) ||
		       (entry->tag == EXIF_TAG_INTEROPERABILITY_OFFSET))
		doff = Get32u (data + offset + 8, order);
	else
		doff = offset + 8;

	/* Sanity check */
	if (size < doff + s)
		return;

	entry->data = malloc (sizeof (char) * s);
	if (!entry->data)
		return;
	entry->size = s;
	memcpy (entry->data, data + doff, s);

	if ((entry->tag == EXIF_TAG_EXIF_OFFSET) ||
	    (entry->tag == EXIF_TAG_INTEROPERABILITY_OFFSET))
		exif_content_parse (entry->content, data, size, doff, order);
}

void
exif_entry_dump (ExifEntry *entry, unsigned int indent)
{
	char buf[1024];
	unsigned int i;

	for (i = 0; i < 2 * indent; i++)
		buf[i] = ' ';
	buf[i] = '\0';

	if (!entry)
		return;

	printf ("%s  Tag: 0x%x ('%s')\n", buf, entry->tag,
		exif_tag_get_name (entry->tag));
	printf ("%s  Format: %i\n", buf, entry->format);
	printf ("%s  Components: %i\n", buf, (int) entry->components);
	printf ("%s  Size: %i\n", buf, entry->size);
	if (entry->content->count)
		exif_content_dump (entry->content, indent + 1);
}

const char *
exif_entry_get_value (ExifEntry *entry)
{
	static char v[1024];
	char v_char;
	int v_int;
	unsigned int v_uint;
	long v_long, p, q;
	unsigned long v_ulong;

	memset (v, 0, sizeof (v));
	switch (entry->format) {
	case EXIF_FORMAT_UBYTE:
	case EXIF_FORMAT_SBYTE:
		v_char = entry->data[0];
		snprintf (v, sizeof (v), "%i", v_char);
		break;
	case EXIF_FORMAT_USHORT:
		v_uint = Get16u (entry->data, entry->priv->order);
		snprintf (v, sizeof (v), "%i", v_uint);
		break;
	case EXIF_FORMAT_SSHORT:
		v_int = Get16s (entry->data, entry->priv->order);
		snprintf (v, sizeof (v), "%i", v_int);
		break;
	case EXIF_FORMAT_ULONG:
		v_ulong = Get32u (entry->data, entry->priv->order);
		snprintf (v, sizeof (v), "%i", (int) v_ulong);
		break;
	case EXIF_FORMAT_SLONG:
		v_long = Get32s (entry->data, entry->priv->order);
		snprintf (v, sizeof (v), "%i", (int) v_long);
		break;
	case EXIF_FORMAT_STRING:
		strncpy (v, entry->data, MIN (sizeof (v), entry->size));
		break;
	case EXIF_FORMAT_URATIONAL:
	case EXIF_FORMAT_SRATIONAL:
		p = Get32s (entry->data, entry->priv->order);
		q = Get32s (entry->data + 4, entry->priv->order);
		snprintf (v, sizeof (v), "%f", (double) p / q);
		break;
	case EXIF_FORMAT_SINGLE:
	case EXIF_FORMAT_DOUBLE:
		/* Don't know how to handle this */
		break;
	}

	return (v);
}
