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

DESKTOP_IN_FILES=gtkam.desktop.in
DESKTOP_FILES=$(DESKTOP_IN_FILES:.desktop.in=.desktop)

gtkam.desktop: gtkam.desktop.in
	tmpfiles="t-$@ n-t-$@"; \
	pofiles=`MAKEFLAGS= $(MAKE) -s -C $(top_builddir)/po print-pofiles`; \
	cp $< t-$@ || exit 1; \
	for pofile in $$pofiles; do \
		$(MSGFMT) --desktop --template t-$@ -o n-t-$@ \
			--locale $${pofile%.po} $(top_builddir)/po/$$pofile \
			|| { rm -f $$tmpfiles; exit 1; }; \
		mv n-t-$@ t-$@ || { rm -f $$tmpfiles; exit 1; } \
	done; \
	mv t-$@ $@ || { rm -f $$tmpfiles; exit 1; }


Applicationsdir = $(datadir)/applications
Applications_DATA = $(DESKTOP_FILES)

EXTRA_DIST = gtkam.png ChangeLog AUTHORS COPYING NEWS README TODO CHANGES \
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
