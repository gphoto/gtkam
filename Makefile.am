## Process this file with automake to produce Makefile.in

SUBDIRS = intl art doc help omf-install packaging pixmaps po src test

gtkamdocdir = $(datadir)/doc/gtkam
gtkamdoc_DATA = AUTHORS COPYING

desktopdir = $(datadir)/applications
desktop_DATA = gtkam.desktop

icondir = $(datadir)/pixmaps
icon_DATA = gtkam.png

INCLUDES = \
       -DDATADIR=\""$(datadir)"\"

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
