INCLUDES =			\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/src	\
	$(GTKAM_CFLAGS) $(BONOBO_CFLAGS) $(BONOBO_CFLAGS) $(EXIF_CFLAGS)

noinst_PROGRAMS = test-fsel test-cancel

test_fsel_LDADD   = $(GTKAM_LIBS) $(EXIF_LIBS) $(BONOBO_LIBS) $(top_builddir)/src/libgtkam.a $(INTLLIBS)
test_cancel_LDADD = $(GTKAM_LIBS) $(EXIF_LIBS) $(BONOBO_LIBS) $(top_builddir)/src/libgtkam.a $(INTLLIBS)
