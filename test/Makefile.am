INCLUDES =			\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/src	\
	$(GTKAM_CFLAGS) $(BONOBO_CFLAGS) $(BONOBO_CFLAGS)

noinst_PROGRAMS = test-fsel test-cancel

test_fsel_LDADD   = $(GTKAM_LIBS) $(EXIF_LIBS) $(BONOBO_LIBS) ../src/libgtkam.a ../intl/libintl.a
test_cancel_LDADD = $(GTKAM_LIBS) $(EXIF_LIBS) $(BONOBO_LIBS) ../src/libgtkam.a ../intl/libintl.a
