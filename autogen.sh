#!/bin/sh
# Run this to generate all the initial makefiles, etc.

# Call this file with AUTOCONF_SUFFIX and AUTOMAKE_SUFFIX set
# if you want us to call a specific version of autoconf or automake. 
# E.g. if you want us to call automake-1.6 instead of automake (which
# seems to be quite advisable if your automake is not already version 
# 1.6) then call this file with AUTOMAKE_SUFFIX set to "-1.6".

DIE=0
srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
test "$srcdir" = "." && srcdir=`pwd`

PROJECT=gtkam

(autoconf${AUTOCONF_SUFFIX} --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "**Error**: You must have \`autoconf' installed."
	echo "Download the appropriate package for your distribution,"
	echo "or get the source tarball at ftp://ftp.gnu.org/gnu/autoconf/"
	DIE=1
}

(automake${AUTOMAKE_SUFFIX} --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "**Error**: You must have \`automake' installed."
	echo "Download the appropriate package for your distribution,"
	echo "or get the source tarball at ftp://ftp.gnu.org/gnu/automake/"
	DIE=1
}

(gettextize --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "**Error**: You must have \`gettext' installed."
	echo "Download the appropriate package for your distribution,"
	echo "or get the source tarball at ftp://ftp.gnu.org/gnu/gettext/"
	DIE=1
}

(pkg-config --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "**Error**: You must have \`pkg-config installed."
	echo "Download the appropriate package for your distribution,"
	echo "or get the source tarball at http://www.freedesktop.org/software/pkgconfig/"
	DIE=1
}

if test "$DIE" -eq 1; then
	exit 1
fi

test -f src/gtkam-config.h || {
	echo "You must run this script in the top-level $PROJECT directory"
	exit 1
}

case $CC in
*xlc | *xlc\ * | *lcc | *lcc\ *)
	am_opt=--include-deps;;
esac

gettext_version=`gettextize --version 2>&1 | sed -n 's/^.*GNU gettext.* \([0-9]*\.[0-9.]*\).*$/\1/p'`
case $gettext_version in
0.11.*)
	gettext_opt="$gettext_opt --intl";;
esac

echo "Running gettextize --force --copy $gettext_opt"
gettextize --force --copy $gettext_opt
test -f po/Makevars.template &&
cp po/Makevars.template po/Makevars
echo "Running aclocal $ACLOCAL_FLAGS..."
aclocal${AUTOMAKE_SUFFIX} $ACLOCAL_FLAGS
echo "Running autoheader..."
autoheader${AUTOCONF_SUFFIX}
echo "Running automake --gnu $am_opt ..."
automake${AUTOMAKE_SUFFIX} --add-missing --gnu $am_opt
echo "Running autoconf ..."
autoconf${AUTOCONF_SUFFIX}

echo
echo "$PROJECT is now ready for configuration."
