INCLUDES =              \
        -I$(top_srcdir)

lib_LTLIBRARIES = libexif.la

libexif_la_LDFLAGS = -version-info @LIBEXIF_VERSION_INFO@
libexif_la_LIBADD = -lm
libexif_la_SOURCES =		\
	exif-content.c		\
	exif-data.c		\
	exif-entry.c		\
	exif-tag.c		\
	exif-utils.c

libexifincludedir = $(includedir)/libexif
libexifinclude_HEADERS = 	\
	exif-content.h		\
	exif-data.h		\
	exif-entry.h		\
	exif-result.h		\
	exif-tag.h		\
	exif-utils.h

