/* exif-content.c
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
#include "exif-content.h"

#include <stdlib.h>
#include <stdio.h>

#include <libjpeg/jpeg-marker.h>

//#define DEBUG

static const unsigned char ExifHeader[] = {0x45, 0x78, 0x69, 0x66, 0x00, 0x00};

struct _ExifContentPrivate
{
	unsigned int ref_count;
};

ExifContent *
exif_content_new (void)
{
	ExifContent *content;

	content = malloc (sizeof (ExifContent));
	if (!content)
		return (NULL);
	memset (content, 0, sizeof (ExifContent));
	content->priv = malloc (sizeof (ExifContentPrivate));
	if (!content->priv) {
		free (content);
		return (NULL);
	}
	memset (content->priv, 0, sizeof (ExifContentPrivate));
	content->priv->ref_count = 1;

	return (content);
}

void
exif_content_ref (ExifContent *content)
{
	content->priv->ref_count++;
}

void
exif_content_unref (ExifContent *content)
{
	content->priv->ref_count--;
	if (!content->priv->ref_count)
		exif_content_free (content);
}

void
exif_content_free (ExifContent *content)
{
	unsigned int i;

	for (i = 0; i < content->count; i++)
		exif_entry_unref (content->entries[i]);
	free (content->entries);
	free (content->priv);
	free (content);
}

void
exif_content_parse (ExifContent *content, const unsigned char *data,
		    unsigned int size, unsigned int offset,
		    ExifByteOrder order)
{
	unsigned int n, i;
	ExifEntry *entry;

	if (!content)
		return;

	/* Read number of entries */
	if (size < offset + 2)
		return;
	n = Get16u (data + offset, order);
#ifdef DEBUG
	printf ("Parsing directory with %i entries...\n", n);
#endif

	content->entries = malloc (sizeof (ExifEntry *) * n);
	if (!content->entries)
		return;
	memset (content->entries, 0, sizeof (ExifEntry *) * n);
	content->count = n;

	for (i = 0; i < n; i++) {
		entry = exif_entry_new ();
		content->entries[i] = exif_entry_new ();
		exif_entry_parse (content->entries[i], data, size,
				  offset + 2 + 12 * i, order);
	}
}

void
exif_content_dump (ExifContent *content, unsigned int indent)
{
	char buf[1024];
	unsigned int i;

	for (i = 0; i < 2 * indent; i++)
		buf[i] = ' ';
	buf[i] = '\0';

	if (!content)
		return;

	printf ("%sDumping exif content (%i entries)...\n", buf,
		content->count);
	for (i = 0; i < content->count; i++)
		exif_entry_dump (content->entries[i], indent + 1);
}
