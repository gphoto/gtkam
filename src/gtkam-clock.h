/* gtkam-clock.h
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

#ifndef __GTKAM_CLOCK_H__
#define __GTKAM_CLOCK_H__

#include <gtk/gtkhbox.h>

#define GTKAM_TYPE_CLOCK     (gtkam_clock_get_type())
#define GTKAM_CLOCK(o)       (GTK_CHECK_CAST((o),GTKAM_TYPE_CLOCK,GtkamClock))
#define GTKAM_IS_CLOCK(o)    (GTK_CHECK_TYPE((o),GTKAM_TYPE_CLOCK))
#define GTKAM_CLOCK_CLASS(k) (GTK_CHECK_CLASS_CAST((k),GTKAM_TYPE_CLOCK,GtkamClockClass))

typedef struct _GtkamClock        GtkamClock;
typedef struct _GtkamClockPrivate GtkamClockPrivate;
typedef struct _GtkamClockClass   GtkamClockClass;

struct _GtkamClock
{
	GtkHBox parent;

	GtkamClockPrivate *priv;
};

struct _GtkamClockClass
{
	GtkHBoxClass parent_class;

	/* Signals */
	void (* changed)      (GtkamClock *clock);
	void (* next_day)     (GtkamClock *clock);
	void (* previous_day) (GtkamClock *clock);
	void (* set)          (GtkamClock *clock);
};

GtkType    gtkam_clock_get_type (void);
GtkWidget *gtkam_clock_new      (void);

void       gtkam_clock_start (GtkamClock *clock);
void       gtkam_clock_stop  (GtkamClock *clock);

void       gtkam_clock_set   (GtkamClock *clock,
			      guchar hour, guchar minute, guchar second);
void       gtkam_clock_get   (GtkamClock *clock,
			      guchar *hour, guchar *minute, guchar *second);

#endif /* __GTKAM_CLOCK_H__ */
