#!/bin/sh

mkdir -p build-aux
aclocal
autoconf
automake --add-missing
