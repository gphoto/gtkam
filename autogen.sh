#!/bin/sh
# Run this to generate all the initial makefiles, etc.

DIE=0
srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
test "$srcdir" = "." && srcdir=`pwd`

PROJECT=gtkam

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "**Error**: You must have \`autoconf' installed."
	echo "Download the appropriate package for your distribution,"
	echo "or get the source tarball at ftp://ftp.gnu.org/gnu/autoconf/"
	DIE=1
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
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
aclocal $ACLOCAL_FLAGS
echo "Running autoheader..."
autoheader
echo "Running automake --gnu $am_opt ..."
automake --add-missing --gnu $am_opt
echo "Running autoconf ..."
autoconf

echo
echo "$PROJECT is now ready for configuration."
