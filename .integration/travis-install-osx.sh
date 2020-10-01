#!/bin/bash

### Global packages

function brew_install
{
    (brew install "$1") || (echo "Error installing $1"; return 1)
}

brew update
#brew_install xxx yyy
#brew link --force xxx yyy

### Install google test and google mock
wget https://github.com/google/googletest/archive/release-1.10.0.tar.gz
tar xf release-1.10.0.tar.gz
(cd googletest-release-1.10.0
 CXX=$COMPILER cmake -DBUILD_SHARED_LIBS=ON .
 make install
)
