## Process this file with automake to produce Makefile.in

INCLUDES =						\
	-I$(top_srcdir)/intl				\
	-I$(top_srcdir)/libexif				\
	-DIMAGE_DIR=\"$(datadir)/images/gtkam\" 	\
	-DGTKAM_LOCALEDIR=\""$(datadir)/locale"\"	\
	@GTK_CFLAGS@

noinst_LIBRARIES = libgtkam.a
libgtkam_a_SOURCES =				\
	gdk-pixbuf-hacks.c gdk-pixbuf-hacks.h	\
	gtkam-cancel.c gtkam-cancel.h		\
	gtkam-chooser.c gtkam-chooser.h 	\
	gtkam-clist.c gtkam-clist.h		\
	gtkam-clock.c gtkam-clock.h     	\
	gtkam-close.c gtkam-close.h     	\
	gtkam-config.c gtkam-config.h   	\
	gtkam-delete.c gtkam-delete.h		\
	gtkam-error.c gtkam-error.h     	\
	gtkam-fsel.c gtkam-fsel.h		\
	gtkam-info.c gtkam-info.h		\
	gtkam-mkdir.c gtkam-mkdir.h		\
	gtkam-port.c gtkam-port.h		\
	gtkam-preview.c gtkam-preview.h		\
	gtkam-tree.c gtkam-tree.h		\
	gtkam-util.c gtkam-util.h		\
	util.c util.h				\
	support.c support.h

bin_PROGRAMS = gtkam
gtkam_SOURCES = \
	gtkam-debug.c gtkam-debug.h \
	gtkam-exif.c gtkam-exif.h \
	gtkam-list.c gtkam-list.h \
	gtkam-main.c gtkam-main.h \
	gtkam-save.c gtkam-save.h \
	main.c \
	gtkiconlist.c gtkiconlist.h \
	gtkitementry.c gtkitementry.h
gtkam_LDADD = 								\
	$(top_builddir)/libexif/libexif/.libs/libexif.al		\
	$(top_builddir)/libexif/libexif-gtk/.libs/libexif-gtk.al	\
	libgtkam.a							\
	 @GTK_LIBS@

if HAVE_GIMP
libexecdir = $(GIMP_PLUGIN_DIR)/plug-ins
libexec_PROGRAMS = gtkam-gimp
gtkam_gimp_SOURCES = gtkam-gimp.c
gtkam_gimp_LDADD = $(GIMP_LIBS) libgtkam.a
endif
