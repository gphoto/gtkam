INCLUDES =			\
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	$(GP_CFLAGS)		\
	$(GDK_PIXBUF_CFLAGS)	\
	-I$(top_srcdir)/src

noinst_PROGRAMS = test-fsel

test_fsel_LDADD = $(GP_LIBS) $(GTK_LIBS) $(GDK_PIXBUF_LIBS) ../src/libgtkam.a
