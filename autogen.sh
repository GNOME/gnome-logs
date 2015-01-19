#!/bin/sh
# Run this to generate all the initial Makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

(test -f $srcdir/src/gl-main.c) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level gnome-logs directory"
    exit 1
}

mkdir -p "$srcdir"/build-aux "$srcdir"/m4

which gnome-autogen.sh || {
    echo "You need to install gnome-common"
    exit 1
}

. gnome-autogen.sh
