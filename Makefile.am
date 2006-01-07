########################################################################
# Process this file with automake to produce Makefile.in
########################################################################

SUBDIRS = m4m art doc help omf-install packaging pixmaps po src test

ACLOCAL_AMFLAGS = -I m4m


########################################################################
# Install a few support and doc files

gtkamdocdir = $(datadir)/doc/gtkam
gtkamdoc_DATA = AUTHORS COPYING NEWS README TODO CHANGES

desktopdir = $(datadir)/applications
desktop_DATA = gtkam.desktop

icondir = $(datadir)/pixmaps
icon_DATA = gtkam.png

EXTRA_DIST = gtkam.desktop gtkam.png


########################################################################
# Packaging convenience

.PHONY: rpm cvs-tags

if ENABLE_RPM

# create RPM packages
rpm: Makefile
	$(MAKE) dist
	$(MAKE) -C packaging/rpm rpm

endif


########################################################################
# Maintainer's convenience

# add CVS tags - used only by the project maintainer to tag the files of a
# released version
cvs-tags: Makefile
	@echo cvs tag "@PACKAGE@-$$(echo '@VERSION@' | sed 's/\./_/g')"

########################################################################
