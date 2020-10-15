#!/bin/bash

# This script is called by continuous integration tools for checking
# if main project and sub-projects still compile, as well as uni tests.
# And if unit-tests can be compiled and if project has no regression.
# This script is architecture agnostic and should be launched for any
# operating systems (Linux, OS X, Windows).

EXEC=OpenGlassBox

function file_exists
{
  test -e $1 && echo -e "\033[32mThe file $1 exists\033[00m" || (echo -e "\033[31mThe file $1 does not exist\033[00m" && exit 1)
}

function dir_exists
{
  test -d $1 && echo -e "\033[32mThe directory $1 exists\033[00m" || (echo -e "\033[31mThe directory $1 does not exist\033[00m" && exit 1)
}

# Installation directory when CI
CI_DESTDIR=/tmp

# For OS X and homebrew >= 2.60
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/opt/libffi/lib/pkgconfig:$CI_DESTDIR/usr/lib/pkgconfig:/usr/local/lib/pkgconfig
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/:$CI_DESTDIR/usr/lib/
export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:$CI_DESTDIR/usr/lib/

# Clone and compile 3thpart librairies that project depends on.
make download-external-libs || exit 1
#make compile-external-libs || exit 1

# Build the project
V=1 make CXX=$COMPILER $JCORES || exit 1

# Build unit tests and non-regression tests
(cd tests && V=1 make check DESTDIR=$CI_DESTDIR $JCORES) || exit 1
