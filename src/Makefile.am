## Process this file with automake to produce Makefile.in

MY_CPPFLAGS =				\
	$(CPPFLAGS) $(AM_CPPFLAGS)	\
	$(LIBEXIF_CFLAGS)		\
	$(LIBEXIF_GTK_CFLAGS)		\
	$(LIBGPHOTO2_CFLAGS)		\
	$(GNOME_CFLAGS) 		\
	$(GTK_CFLAGS)			\
	$(BONOBO_CFLAGS)		\
	$(GTKAM_GIMP_CFLAGS)

noinst_LIBRARIES = libgtkam.a
libgtkam_a_CPPFLAGS = $(MY_CPPFLAGS)
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
gtkam_CPPFLAGS = $(MY_CPPFLAGS)
gtkam_SOURCES =					\
	gtkam-debug.c gtkam-debug.h 		\
	gtkam-exif.c gtkam-exif.h 		\
	gtkam-main.c gtkam-main.h 		\
	i18n.h main.c
gtkam_LDADD = \
	libgtkam.a	\
	$(LIBEXIF_LIBS) \
	$(LIBEXIF_GTK_LIBS) \
	$(LIBGPHOTO2_LIBS)	\
	$(GTKAM_LIBS) 	\
	$(GTK_LIBS) 	\
	$(BONOBO_LIBS)	\
	$(GNOME_LIBS)	\
	$(INTLLIBS)

if HAVE_GIMP
gtkam_gimp_libexecdir = $(libdir)/gimp/$(GIMP_VERSION)/plug-ins
gtkam_gimp_libexec_PROGRAMS = gtkam-gimp
gtkam_gimp_SOURCES = gtkam-gimp.c
gtkam_gimp_CPPFLAGS = \
	$(MY_CPPFLAGS)		\
	$(GIMP_CFLAGS)
gtkam_gimp_LDADD = \
	libgtkam.a		\
	$(GIMP_LIBS)		\
	$(LIBEXIF_LIBS)		\
	$(LIBEXIF_GTK_LIBS) 	\
	$(LIBGPHOTO2_LIBS)	\
	$(GTK_LIBS)		\
	$(BONOBO_LIBS)		\
	$(GNOME_LIBS)		\
	$(INTLLIBS)
endif
