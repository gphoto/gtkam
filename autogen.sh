#!/bin/sh
# autogen.sh - initialize build tree
#
# Unfortunately, autoreconf cannot cope with intltoolize alone, so
# this appears to be the way to set up the source tree.

set -e
set -x

cd "$(dirname "$0")"

if test -f intltool-extract.in
then
    :
else
    rm -f po/Makefile.in.in
    autoreconf -vis || :
    intltoolize --force --automake --debug
fi

rm -f po/Makefile.in.in
autoreconf -vis
intltoolize --force --automake --debug

# End of file.
