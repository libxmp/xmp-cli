#!/bin/sh

set -e

mkdir -p build-aux
"${AUTORECONF:-autoreconf}" -i
rm -rf autom4te.cache
