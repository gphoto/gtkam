## Process this file with automake to produce Makefile.in

EXTRA_DIST = @PACKAGE@.spec @PACKAGE@.spec.in autogen.sh acconfig.h gphoto2-gtk.glade MANUAL

SUBDIRS = po src intl

gtkamdocdir = $(prefix)/doc/gtkam
gtkamdoc_DATA = AUTHORS COPYING MANUAL

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/pixmaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps; \
	  for pixmap in $(srcdir)/pixmaps/*; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(pkgdatadir)/pixmaps; \
	    fi \
	  done \
	fi

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi

########################################################################
# create .tar.bz2 distribution package for convenience if the
# $(TAR) on this system supports it. else do nothing.

dist-hook:
	-$(MAKE) dist-bzip2

dist-bzip2:
	-chmod -R a+r $(distdir)
	-$(TAR) chojf $(distdir).tar.bz2 $(distdir)
