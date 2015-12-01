#!/bin/sh

set -e

rm -rf config.cache

aclocal $ACLOCAL_FLAGS
autoheader
automake --add-missing $AUTOMAKE_FLAGS
autoconf
