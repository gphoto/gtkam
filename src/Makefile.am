## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/intl \
	@GTK_CFLAGS@

bin_PROGRAMS = gtkam

gtkam_SOURCES = \
	gtkam-config.c gtkam-config.h \
	gtkam-close.c gtkam-close.h \
	gtkam-chooser.c gtkam-chooser.h \
	gtkam-debug.c gtkam-debug.h \
	gtkam-error.c gtkam-error.h \
	gtkam-list.c gtkam-list.h \
	gtkam-main.c gtkam-main.h \
	gtkam-preview.c gtkam-preview.h \
	gtkam-save.c gtkam-save.h \
	gtkam-tree.c gtkam-tree.h \
	main.c \
	support.c support.h \
	util.c util.h \
	gtkiconlist.c gtkiconlist.h \
	gtkitementry.c gtkitementry.h

gtkam_LDADD = @GTK_LIBS@
