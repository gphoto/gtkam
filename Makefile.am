## Process this file with automake to produce Makefile.in

EXTRA_DIST = MANUAL

SUBDIRS = po src test intl art pixmaps packaging

gtkamdocdir = $(prefix)/doc/gtkam
gtkamdoc_DATA = AUTHORS COPYING MANUAL

########################################################################
# create .tar.bz2 distribution package for convenience if the
# $(TAR) on this system supports it. else do nothing.

dist-hook:
	if [ "x${CDPATH}" != "x" ]; then \
		echo "You have set CDPATH. If make dist fails, try to unset CDPATH."; \
	fi
	-$(MAKE) dist-bzip2

dist-bzip2:
	-chmod -R a+r $(distdir)
	-$(TAR) chojf $(distdir).tar.bz2 $(distdir)

# create RPM packages
.PHONY: rpm
rpm: Makefile
	$(MAKE) dist
	$(MAKE) -C packaging/rpm rpm

# add CVS tags - used only by the project maintainer to tag the files of a
# released version
.PHONY: cvs-tags
cvs-tags: Makefile
	@echo cvs tag "@PACKAGE@-$$(echo '@VERSION@' | sed 's/\./_/g')"
