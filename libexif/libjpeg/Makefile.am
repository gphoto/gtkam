INCLUDES =		\
	-I$(top_srcdir)

lib_LTLIBRARIES = libjpeg.la
libjpeg_la_SOURCES =	\
	jpeg-data.c	\
	jpeg-marker.c
libjpeg_la_LIBADD = ../libexif/libexif.la

libjpegincludedir = $(includedir)/libjpeg
libjpeginclude_HEADERS =	\
	jpeg-data.h		\
	jpeg-marker.h
