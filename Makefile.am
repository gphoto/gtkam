## Process this file with automake to produce Makefile.in

EXTRA_DIST = config.rpath 

SUBDIRS = m4 intl po src test art pixmaps packaging

gtkamdocdir = $(datadir)/doc/gtkam
gtkamdoc_DATA = AUTHORS COPYING

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
	-tar -c --bzip2 -h -o -f $(distdir).tar.bz2 $(distdir)

if ENABLE_RPM

# create RPM packages
.PHONY: rpm
rpm: Makefile
	$(MAKE) dist
	$(MAKE) -C packaging/rpm rpm

endif

if MAINTAINER_MODE

# add CVS tags - used only by the project maintainer to tag the files of a
# released version
.PHONY: cvs-tags
cvs-tags: Makefile
	@echo cvs tag "@PACKAGE@-$$(echo '@VERSION@' | sed 's/\./_/g')"

endif

ACLOCAL_AMFLAGS = -I m4
