#!/bin/sh
# autogen.sh - initialize build tree
#
# Unfortunately, autoreconf cannot cope with intltoolize alone, so
# this appears to be the way to set up the source tree.

set -e
set -x

cd "$(dirname "$0")"

intltoolize --force --automake --debug
rm -f po/Makefile.in.in
autoreconf -vis

# End of file.
