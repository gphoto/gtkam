########################################################################
# Process this file with automake to produce Makefile.in
########################################################################

SUBDIRS = gphoto-m4 art doc help omf-install packaging pixmaps po src test

ACLOCAL_AMFLAGS = -I m4 -I gphoto-m4

# disable scrollkeeper for distcheck
DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper

########################################################################
# Install a few support and doc files

gtkamdocdir = $(datadir)/doc/gtkam
gtkamdoc_DATA = AUTHORS COPYING NEWS README TODO CHANGES

icondir = $(datadir)/pixmaps
icon_DATA = gtkam.png

# This fixes broken INTLTOOL_DESKTOP_RULE@ (original lacks \n\t)
%.desktop:   %.desktop.in
	$(INTLTOOL_MERGE) $(top_srcdir)/po/*.po
	$(INTLTOOL_V_MERGE)LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_V_MERGE_OPTIONS) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

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

.PHONY: rpm

if ENABLE_RPM

# create RPM packages
rpm: Makefile
	$(MAKE) dist
	$(MAKE) -C packaging/rpm rpm

endif

distclean-local:
	rm -f $(DESKTOP_FILES)


########################################################################
