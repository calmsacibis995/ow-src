#! /bin/sh
#
# @(#)%M 3.8 93/02/22 SMI; Copyright (c) 1992 by Sun Microsystems, Inc.
#

#
# Construct a partial OpenWindows heirarchy that DeskSet can install
# into
#

if [ $# -gt 0 ]; then
	DESTDIR=$1;
elif [ X = X$DESTDIR ]; then
	echo "You must either set DESTDIR or supply the destination directory \
as an argument"
	exit 1
fi

SUBDIRS="\
bin \
demo \
lib \
lib/cetables \
lib/locale \
lib/locale/C \
lib/locale/C/help \
lib/locale/templates \
lib/locale/templates/LC_MESSAGES \
share \
share/etc \
share/etc/tt \
share/include \
share/man \
share/xnews \
share/src \
share/src/dig_samples \
share/include/desktop \
share/include/images \
share/xnews/client \
share/xnews/client/templates \
"

if [ ! -d $DESTDIR ]; then
	echo "Creating $DESTDIR..."
	mkdir $DESTDIR
	chmod 777 $DESTDIR
else
	echo "$DESTDIR already exists"
fi

for d in $SUBDIRS; do
	dir=$DESTDIR/$d
	if [ ! -d $dir ]; then
		echo "Creating $dir..."
		mkdir $dir
		chmod 777 $dir
	else
		echo "$dir already exists"
	fi
done

echo "Creating symbolic links..."

if [ ! -d $DESTDIR/man ]; then
	ln -s ./share/man $DESTDIR
fi

if [ ! -d $DESTDIR/include ]; then
	ln -s ./share/include $DESTDIR
fi

if [ ! -d $DESTDIR/lib/help ]; then
	ln -s ./locale/C/help $DESTDIR/lib
fi

if [ ! -d $DESTDIR/etc ]; then
	ln -s ./share/etc $DESTDIR
fi

echo "Done creating $DESTDIR heirarchy"

