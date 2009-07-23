MY_CPPFLAGS =				\
	$(CPPFLAGS) $(AM_CPPFLAGS)	\
	$(LIBEXIF_CFLAGS)		\
	$(LIBEXIF_GTK_CFLAGS)		\
	$(LIBGPHOTO2_CFLAGS)		\
	$(GNOME_CFLAGS) 		\
	$(GTK_CFLAGS)			\
	$(BONOBO_CFLAGS)		\
	$(GTKAM_GIMP_CFLAGS)

MY_LIBS = \
	$(top_builddir)/src/libgtkam.a	\
	$(LIBEXIF_LIBS)			\
	$(LIBEXIF_GTK_LIBS)		\
	$(LIBGPHOTO2_LIBS)	\
	$(GTK_LIBS)			\
	$(BONOBO_LIBS)			\
	$(INTLLIBS)

noinst_PROGRAMS = test-fsel test-cancel

test_fsel_SOURCES = test-fsel.c
test_fsel_CPPFLAGS = $(MY_CPPFLAGS)
test_fsel_LDADD   = $(MY_LIBS)

test_cancel_SOURCES = test-cancel.c
test_cancel_CPPFLAGS = $(MY_CPPFLAGS)
test_cancel_LDADD = $(MY_LIBS)
