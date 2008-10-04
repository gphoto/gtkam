/* gtkam-version.h
 *
 * Copyright 2002 Hans Ulrich Niedermann <gp@n-dimensional.de
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

#ifndef __GTKAM_VERSION_H__
#define __GTKAM_VERSION_H__

/* for detailed version message */
#include <gphoto2/gphoto2-version.h>

/* for logging of version */
#include <gphoto2/gphoto2-port-log.h>

typedef struct {
	char *name;
	GPVersionFunc version_func;
} module_version;

extern const module_version module_versions[];

extern void log_version (GPLogLevel level, const char *domain);

#endif /* __GTKAM_VERSION_H__ */
