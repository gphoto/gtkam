## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/intl \
	-DIMAGE_DIR=\"$(datadir)/images/gtkam\" \
	@GTK_CFLAGS@

noinst_LIBRARIES = libgtkam.a
libgtkam_a_SOURCES =			\
	gtkam-chooser.c gtkam-chooser.h \
	gtkam-clock.c gtkam-clock.h     \
	gtkam-close.c gtkam-close.h     \
	gtkam-config.c gtkam-config.h   \
	gtkam-error.c gtkam-error.h     \
	gtkam-preview.c gtkam-preview.h

bin_PROGRAMS = gtkam
gtkam_SOURCES = \
	gtkam-debug.c gtkam-debug.h \
	gtkam-list.c gtkam-list.h \
	gtkam-main.c gtkam-main.h \
	gtkam-mkdir.c gtkam-mkdir.h \
	gtkam-save.c gtkam-save.h \
	gtkam-tree.c gtkam-tree.h \
	main.c \
	support.c support.h \
	util.c util.h \
	gtkiconlist.c gtkiconlist.h \
	gtkitementry.c gtkitementry.h
gtkam_LDADD = @GTK_LIBS@ libgtkam.a

if HAVE_GIMP
libexecdir = $(GIMP_PLUGIN_DIR)/plug-ins
libexec_PROGRAMS = gtkam-gimp
gtkam_gimp_SOURCES = gtkam-gimp.c
gtkam_gimp_LDADD = $(GIMP_LIBS) libgtkam.a
endif
