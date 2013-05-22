#!/bin/sh

if `which libtoolize` ; then
	libtoolize
# on, e.g., Darwin
elif `which glibtoolize` ; then
	glibtoolize
else
	echo "libtoolize not found!"
	exit 1
fi

aclocal
autoconf
automake --add-missing
