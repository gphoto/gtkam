########################################################################
# Process this file with automake to produce Makefile.in
########################################################################

SUBDIRS = m4m art doc help omf-install packaging pixmaps po src test

ACLOCAL_AMFLAGS = -I m4m

# disable scrollkeeper for distcheck
DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper

########################################################################
# Install a few support and doc files

gtkamdocdir = $(datadir)/doc/gtkam
gtkamdoc_DATA = AUTHORS COPYING NEWS README TODO CHANGES

icondir = $(datadir)/pixmaps
icon_DATA = gtkam.png

@INTLTOOL_DESKTOP_RULE@

DESKTOP_IN_FILES=gtkam.desktop.in
DESKTOP_FILES=$(DESKTOP_IN_FILES:.desktop.in=.desktop)

Applicationsdir = $(datadir)/applications
Applications_DATA = $(DESKTOP_FILES)

EXTRA_DIST = gtkam.png ChangeLog AUTHORS COPYING NEWS README TODO CHANGES \
	intltool-extract.in     \
	intltool-merge.in       \
	intltool-update.in      \
	$(DESKTOP_IN_FILES)     \
	$(DESKTOP_FILES)

########################################################################
# Packaging convenience

.PHONY: rpm cvs-tags

if ENABLE_RPM

# create RPM packages
rpm: Makefile
	$(MAKE) dist
	$(MAKE) -C packaging/rpm rpm

endif

distclean-local:
	rm $(DESKTOP_FILES)


########################################################################
# Maintainer's convenience

# add CVS tags - used only by the project maintainer to tag the files of a
# released version
cvs-tags: Makefile
	@echo cvs tag "@PACKAGE@-$$(echo '@VERSION@' | sed 's/\./_/g')"

########################################################################
