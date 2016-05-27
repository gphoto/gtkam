AC_PREREQ([2.59])
AC_INIT([GTK frontend for libgphoto2], [1.0], [gphoto-devel@lists.sourceforge.net], [gtkam])
AC_CONFIG_SRCDIR([src/gtkam-main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([auto-aux])
AM_INIT_AUTOMAKE([-Wall gnu 1.9 dist-bzip2 check-news])

GP_CHECK_SHELL_ENVIRONMENT
GP_CONFIG_MSG([Build])
GP_CONFIG_MSG([Source code location],[${srcdir}])

AM_CPPFLAGS="$CPPFLAGS"
AM_LDFLAGS="$LDFLAGS"


dnl ---------------------------------------------------------------------------
dnl Some programs we need
dnl ---------------------------------------------------------------------------
AM_PROG_AR
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
m4_ifdef([AM_PROG_AR],[AM_PROG_AR])
AM_PROG_LIBTOOL
AC_PROG_INSTALL
AM_PROG_CC_C_O
GP_PKG_CONFIG
IT_PROG_INTLTOOL
LT_INIT
AC_SUBST(POMAKEFILEDEPS)

GP_CONFIG_MSG([Compiler],[${CC}])
AC_DEFINE_UNQUOTED(HAVE_CC,"$CC",[The C compiler we're using])
AM_CPPFLAGS="$AM_CPPFLAGS -I\$(top_srcdir)"
AM_CPPFLAGS="$AM_CPPFLAGS -I\$(top_srcdir)/src"


GP_CONFIG_MSG([Features])


# ----------------------------------------------------------------------
# References to external resources (web sites, mailing listes, etc.)
# ----------------------------------------------------------------------

GP_REFERENCES()


dnl ---------------------------------------------------------------------------
dnl i18n support
dnl ---------------------------------------------------------------------------
GP_GETTEXT_HACK([],[Lutz Müller and others],[${MAIL_GPHOTO_TRANSLATION}])
ALL_LINGUAS="be cs da de eo es fi fr hu id it is ja nb nl pa pl pt_BR pt ru sk sl sr sv uk vi zh_CN"
AM_GNU_GETTEXT_VERSION([0.14.1])
AM_GNU_GETTEXT([external])
AM_ICONV()
GP_GETTEXT_FLAGS()

dnl We cannot use AC_DEFINE_UNQUOTED() for these definitions, as
dnl we require make to do insert the proper $(datadir) value
AC_SUBST([localedir],["\$(datadir)/locale"])
AM_CPPFLAGS="$AM_CPPFLAGS -DLOCALEDIR=\\\"${localedir}\\\""


dnl ---------------------------------------------------------------------------
dnl Libraries we need for gtkam
dnl ---------------------------------------------------------------------------
GP_CHECK_LIBRARY([LIBGPHOTO2], [libgphoto2], [>= 2.5.0])

save_LIBS="$LIBS"
LIBS="$LIBS $LIBGPHOTO2_LIBS"
AC_CHECK_FUNC(gp_port_info_get_name,
	AC_DEFINE(HAVE_GP_PORT_INFO_GET_NAME,1,[Define to 1 if gp_port_info_get_name is present])
)
LIBS="$save_LIBS"

PKG_CHECK_MODULES([GTK], [gtk+-2.0])
AC_SUBST([GTK_CFLAGS])
AC_SUBST([GTK_LIBS])


dnl ---------------------------------------------------------------------------
dnl bonobo-activation: Optional, but contains great stuff.
dnl ---------------------------------------------------------------------------
try_bonobo=true
bonobo_msg="no (bonobo-activation, bononoboui libs)"
have_bonobo=false
AC_ARG_WITH([bonobo],AS_HELP_STRING([--without-bonobo],
				    [do not compile with bonobo]), [
	if test x$withval = xno; then
		try_bonobo=false
	fi
])
if $try_bonobo; then
	if ${PKG_CONFIG} --exists bonobo-activation-2.0 libbonoboui-2.0; then
		PKG_CHECK_MODULES(BONOBO, bonobo-activation-2.0 libbonoboui-2.0)
		AC_SUBST(BONOBO_CFLAGS)
		AC_SUBST(BONOBO_LIBS)
		AC_DEFINE(HAVE_BONOBO,1,[whether we use bonobo])
		bonobo_msg=yes
		have_bonobo=true
	fi
fi
GP_CONFIG_MSG([bonobo support],[$bonobo_msg])


dnl ---------------------------------------------------------------------------
dnl libgnomeui: If we want to display documentation, we need gnome_help_*
dnl             and gphoto2-manual.
dnl ---------------------------------------------------------------------------
try_gnome=true
gnome_msg="no (gnomeui lib)"
have_gnome=false
AC_ARG_WITH([gnome],AS_HELP_STRING([--without-gnome],
                                   [do not compile with gnome]), [
	if test x$withval = xno; then
		try_gnome=false
	fi
])
if $try_gnome; then
	if ${PKG_CONFIG} --exists libgnomeui-2.0; then
		PKG_CHECK_MODULES(GNOME, libgnomeui-2.0)
		AC_DEFINE([HAVE_GNOME],[1],[whether we use gnome])
		gnome_msg=yes
		have_gnome=true
	fi
fi
GP_CONFIG_MSG([Gnome support],[$gnome_msg])
AC_SUBST([GNOME_CFLAGS])
AC_SUBST([GNOME_LIBS])

AC_SUBST([APP_DATADIR],["\$(datadir)"])
AM_CPPFLAGS="$AM_CPPFLAGS -DAPP_DATADIR=\\\"\$(APP_DATADIR)\\\""


dnl ---------------------------------------------------------------------------
dnl omf/scrollkeper/gnome-style docs
dnl ---------------------------------------------------------------------------

dnl FIXME: Isn't there something like "gnomehelpdir"?
AC_SUBST([GTKAM_C_IDENTIFIER],
         ["file:\$(datadir)/gnome/help/gtkam/C/gtkam.xml"])
GP_CONFIG_MSG([GTKAM_C_IDENTIFIER],[$GTKAM_C_IDENTIFIER])

AC_ARG_ENABLE([scrollkeeper],
        [AC_HELP_STRING([--disable-scrollkeeper],
                        [do not make updates to the scrollkeeper database])],,
        enable_scrollkeeper=yes)
AM_CONDITIONAL(ENABLE_SK, test "x$enable_scrollkeeper" = "xyes")


dnl ---------------------------------------------------------------------------
dnl GIMP plugin
dnl ---------------------------------------------------------------------------
try_gimp=:
gimp_msg=no
have_gimp=false
AC_ARG_WITH([gimp],
            AS_HELP_STRING([--without-gimp],[do not compile GIMP plugin]),[
       if test x$withval = xno; then
               try_gimp=false
       fi
])
if $try_gimp; then
	if ${PKG_CONFIG} --exists gimpui-2.0; then
		PKG_CHECK_MODULES([GIMP], gimpui-2.0 libgphoto2 >= 2.1)
		# remove gphoto2 libs so we can link against our own version
		GIMP_LIBS="$(echo "$GIMP_LIBS" | sed 's/-lgphoto2\(_port\)*//g')"
		AC_MSG_CHECKING([cleaned GIMP_LIBS])
		AC_MSG_RESULT([${GIMP_LIBS}])
		AC_SUBST([GIMP_CFLAGS])
		AC_SUBST([GIMP_LIBS])
		GIMP_VERSION="2.0"
		gimp_msg="yes ($GIMP_VERSION)"
		have_gimp=true
	fi
fi
AM_CONDITIONAL([HAVE_GIMP], [$have_gimp])
AC_SUBST([GIMP_VERSION])
GP_CONFIG_MSG([Gimp support],[$gimp_msg])


dnl ---------------------------------------------------------------------------
dnl libexif: If libexif(-gtk) is installed on the system, we are able
dnl          to display EXIF information.
dnl ---------------------------------------------------------------------------
GP_CHECK_LIBRARY([LIBEXIF],     [libexif],    [>= 0.3.2])
GP_CHECK_LIBRARY([LIBEXIF_GTK], [libexif-gtk])


dnl ---------------------------------------------------------------------------
dnl rpm packaging support
dnl ---------------------------------------------------------------------------
AC_ARG_WITH([rpmbuild],AS_HELP_STRING([--with-rpmbuild=PATH],
				    [program to use for building RPMs]))
AC_MSG_CHECKING([for rpmbuild or rpm])
if test -x "${with_rpmbuild}"; then
    RPMBUILD="${with_rpmbuild}"
    AC_MSG_RESULT([${RPMBUILD} (from parameter)])
else
    AC_MSG_RESULT([using autodetection])
    AC_CHECK_PROGS([RPMBUILD], [rpmbuild rpm], [/bin/false])
    AC_MSG_RESULT([${RPMBUILD} (autodetect)])
fi
AC_SUBST([RPMBUILD])
AM_CONDITIONAL([ENABLE_RPM],
		[test -x "$RPMBUILD" && test "$RPMBUILD" != "/bin/false"])


dnl ---------------------------------------------------------------------------
dnl Define application directories
dnl ---------------------------------------------------------------------------

AM_CPPFLAGS="$AM_CPPFLAGS -DIMAGE_DIR=\\\"\$(datadir)/images/gtkam\\\""
# These don't seem to be used
# AM_CPPFLAGS="$AM_CPPFLAGS -DPACKAGE_DATA_DIR=\\\"\$(datadir)/gtkam\\\""
# AM_CPPFLAGS="$AM_CPPFLAGS -DPACKAGE_SOURCE_DIR=\\\"\$(top_srcdir)\\\""


dnl ---------------------------------------------------------------------------
dnl Turn on all warnings when using gcc
dnl ---------------------------------------------------------------------------
if test "$GCC" = "yes"; then
	AM_CPPFLAGS="$AM_CPPFLAGS -Wall"
	AM_CPPFLAGS="$AM_CPPFLAGS -Wmissing-declarations -Wmissing-prototypes"
	AM_LDFLAGS="$AM_LDFLAGS -Wall"
	# FIXME: Check for --enable-debug here
	AM_CPPFLAGS="$AM_CPPFLAGS -g"
	AM_LDFLAGS="$AM_LDFLAGS -g"
fi

AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_LDFLAGS])


# ---------------------------------------------------------------------------
# Output files
# ---------------------------------------------------------------------------
AC_CONFIG_FILES([
po/Makefile.in
gphoto-m4/Makefile
Makefile
art/Makefile
pixmaps/Makefile
packaging/Makefile
packaging/rpm/Makefile
packaging/rpm/package.spec
src/Makefile
test/Makefile
doc/Makefile
omf-install/Makefile
omf-install/gtkam-C.omf
help/Makefile
help/C/Makefile
help/ru/Makefile
intltool-extract
intltool-merge
intltool-update
])
AC_OUTPUT()

GP_CONFIG_OUTPUT()
