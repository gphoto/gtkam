## Process this file with automake to produce Makefile.in

INCLUDES =					\
 -I$(top_srcdir)/intl				\
 -DIMAGE_DIR=\"$(datadir)/images/gtkam\" 	\
 -DGTKAM_LOCALEDIR=\"$(datadir)/locale\"	\
 -DPACKAGE_DATA_DIR=\"$(datadir)/gtkam\"	\
 -DPACKAGE_SOURCE_DIR=\"$(top_srcdir)\"		\
 -DAPP_DATADIR=\"@APP_DATADIR@\"		\
 -DGTK_DISABLE_DEPRECATED			\
 $(EXIF_CFLAGS) $(GNOME_CFLAGS) $(GTKAM_CFLAGS)	\
 $(BONOBO_CFLAGS)				\
 $(GTKAM_GIMP_CFLAGS)

noinst_LIBRARIES = libgtkam.a
libgtkam_a_SOURCES =					\
	gdk-pixbuf-hacks.c	gdk-pixbuf-hacks.h  	\
	gtkam-camera.c		gtkam-camera.h		\
	gtkam-cancel.c		gtkam-cancel.h		\
	gtkam-chooser.c		gtkam-chooser.h 	\
	gtkam-clock.c		gtkam-clock.h     	\
	gtkam-close.c		gtkam-close.h     	\
	gtkam-config.c		gtkam-config.h   	\
	gtkam-context.c		gtkam-context.h		\
	gtkam-delete.c		gtkam-delete.h		\
	gtkam-dialog.c		gtkam-dialog.h		\
	gtkam-error.c		gtkam-error.h     	\
	gtkam-exif.c		gtkam-exif.h		\
	gtkam-fsel.c		gtkam-fsel.h		\
	gtkam-info.c		gtkam-info.h		\
	gtkam-list.c		gtkam-list.h		\
	gtkam-mkdir.c		gtkam-mkdir.h		\
	gtkam-port.c		gtkam-port.h		\
	gtkam-preview.c		gtkam-preview.h		\
	gtkam-save.c		gtkam-save.h		\
	gtkam-status.c		gtkam-status.h		\
	gtkam-tree.c		gtkam-tree.h		\
	gtkam-util.c		gtkam-util.h		\
	gtkam-viewer.c		gtkam-viewer.h		\
	gtkam-version.c		gtkam-version.h		\
	util.c			util.h

bin_PROGRAMS = gtkam
gtkam_SOURCES = 				\
	gtkam-debug.c gtkam-debug.h 		\
	gtkam-exif.c gtkam-exif.h 		\
	gtkam-main.c gtkam-main.h 		\
	i18n.h main.c
gtkam_LDADD =						\
	libgtkam.a					\
	$(EXIF_LIBS) $(GTKAM_LIBS) $(BONOBO_LIBS)	\
	$(GNOME_LIBS)

if HAVE_GIMP
gtkam_gimp_libexecdir = $(libdir)/gimp/1.3/plug-ins
gtkam_gimp_libexec_PROGRAMS = gtkam-gimp
gtkam_gimp_SOURCES = gtkam-gimp.c
gtkam_gimp_LDADD =					\
	$(GTKAM_GIMP_LIBS) $(EXIF_LIBS) $(BONOBO_LIBS)	\
	$(GNOME_LIBS)					\
	libgtkam.a
endif
