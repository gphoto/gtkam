INCLUDES =				\
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-DG_LOG_DOMAIN=\"libexif\"

lib_LTLIBRARIES = libexif-gtk.la

libexif_gtkincludedir = $(includedir)/libexif-gtk
libexif_gtkinclude_HEADERS =		\
	gtk-exif-browser.h		\
	gtk-exif-content-list.h		\
	gtk-exif-entry.h		\
	gtk-exif-entry-9209.c		\
	gtk-exif-tree.h			\
	gtk-exif-tree-item.h

libexif_gtk_la_SOURCES =		\
	gtk-exif-browser.c		\
	gtk-exif-content-list.c		\
	gtk-exif-entry.c		\
	gtk-exif-entry-9209.c		\
	gtk-exif-tree.c			\
	gtk-exif-tree-item.c
libexif_gtk_la_LDFLAGS = -version-info @LIBEXIF_VERSION_INFO@
libexif_gtk_la_LIBADD =				\
	$(top_builddir)/libexif/libexif.la	\
	$(GTK_LIBS)
