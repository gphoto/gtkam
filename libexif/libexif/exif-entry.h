/* exif-entry.h
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

#ifndef __EXIF_ENTRY_H__
#define __EXIF_ENTRY_H__

#include <libexif/exif-utils.h>

typedef struct _ExifEntry        ExifEntry;
typedef struct _ExifEntryPrivate ExifEntryPrivate;

#include <libexif/exif-content.h>

typedef enum _ExifFormat ExifFormat;
enum _ExifFormat {
        EXIF_FORMAT_UBYTE       =  1,
        EXIF_FORMAT_STRING      =  2,
        EXIF_FORMAT_USHORT      =  3,
        EXIF_FORMAT_ULONG       =  4,
        EXIF_FORMAT_URATIONAL   =  5,
        EXIF_FORMAT_SBYTE       =  6,
        EXIF_FORMAT_SSHORT      =  8,
        EXIF_FORMAT_SLONG       =  9,
        EXIF_FORMAT_SRATIONAL   = 10,
        EXIF_FORMAT_SINGLE      = 11,
        EXIF_FORMAT_DOUBLE      = 12
};

struct _ExifEntry {
        ExifTag tag;
        ExifFormat format;
        unsigned long components;

        unsigned char *data;
        unsigned int size;

        ExifContent *content;

	ExifEntryPrivate *priv;
};

ExifEntry  *exif_entry_new   (void);
void        exif_entry_ref   (ExifEntry *entry);
void        exif_entry_unref (ExifEntry *entry);
void        exif_entry_free  (ExifEntry *entry);

void        exif_entry_parse     (ExifEntry *entry, const unsigned char *data,
			          unsigned int size, unsigned int offset,
			          ExifByteOrder order);

const char *exif_entry_get_value (ExifEntry *entry);

void        exif_entry_dump      (ExifEntry *entry, unsigned int indent);

#endif /* __EXIF_ENTRY_H__ */
