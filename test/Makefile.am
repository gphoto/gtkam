INCLUDES =			\
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	$(GP_CFLAGS)		\
	$(EXIF_CFLAGS)		\
	-I$(top_srcdir)/src

noinst_PROGRAMS = test-fsel test-cancel

test_fsel_LDADD   = $(GP_LIBS) $(GTK_LIBS) $(EXIF_LIBS) ../src/libgtkam.a
test_cancel_LDADD = $(GP_LIBS) $(GTK_LIBS) $(EXIF_LIBS) ../src/libgtkam.a
